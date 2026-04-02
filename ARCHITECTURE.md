# Architecture

## Overview

PlaneTalk is ESP32 firmware that creates an infrastructure-free group chat room via a WiFi access point and captive portal. No internet connection is required. Up to 10 devices can connect simultaneously.

## Stack

| Layer | Technology | Evidence |
|---|---|---|
| Language | C++ (Arduino framework) | `src/main.cpp`, `platformio.ini` |
| Build system | PlatformIO | `platformio.ini` |
| Target hardware | ESP32-C3 (DevKitC-02) | `platformio.ini` `board = esp32-c3-devkitc-02` |
| HTTP server | ESPAsyncWebServer 3.10.3 | `platformio.ini` `lib_deps` |
| TCP layer | AsyncTCP 3.4.10 | `platformio.ini` `lib_deps` |
| JSON | ArduinoJson 7.4.1 | `platformio.ini` `lib_deps` |
| Frontend | Vanilla HTML/CSS/JS (no framework, no external deps) | `src/index_html.h` |

## Project structure

```
platformio.ini        — PlatformIO build config (board, framework, lib_deps)
src/
  main.cpp            — All firmware logic: WiFi AP, DNS, HTTP, WebSocket
  index_html.h        — Entire chat UI as a C++ raw string literal
.tickets/             — Local ticket tracker (markdown files, pla-* IDs)
README.md
```

There are only two source files. Everything lives in `main.cpp` and `index_html.h`.

## How the app works

### Network setup (`setup()` in `main.cpp`)

1. `WiFi.softAP("PlaneTalk", nullptr, 1, 0, 10)` — open AP, channel 1, max 10 stations.
2. `DNSServer` on port 53 answers every DNS query with the AP IP (`192.168.4.1`), which triggers captive portal detection on Android, iOS, Windows, and Firefox.
3. `AsyncWebServer` on port 80 serves the chat UI and handles captive portal probe URLs.
4. `AsyncWebSocket` at `/ws` handles all real-time chat.
5. `loop()` only calls `dns_server.processNextRequest()` and `web_socket.cleanupClients()`.

### Captive portal flow

- First-time visitor: all HTTP requests (including OS probe URLs) redirect to `http://192.168.4.1/`.
- Serving `/` marks the client as "visited" in `std::set<std::string> visited_clients`, keyed by MAC address (via `client_id_for_ip()`, falling back to IP string when MAC lookup fails).
- Subsequent OS probe requests from a known client return the expected success responses (HTTP 204, 200 with correct body, etc.) so the OS stops showing "no internet" warnings.
- `visited_clients` is capped at 20 entries and cleared entirely when full (ephemeral AP connections, no need for LRU eviction).

### Message storage and persistence

Messages are stored **in RAM only** in a circular buffer:

```cpp
static std::array<std::string, 50> message_buffer;
static int message_buffer_head = 0;  // index of oldest slot
static int message_buffer_count = 0;
```

Each entry is a pre-serialised JSON string (e.g. `{"type":"message","nick":"JollyBanana","text":"hello"}`). Storing serialised strings avoids re-serialising when replaying history to new clients.

There is **no persistent storage** — messages are lost on reboot.

Private messages (PMs) are **never stored** in the buffer; they are ephemeral point-to-point deliveries only.

### Join/leave messages

Generated entirely on the server side in `handle_websocket_event`:

- **Join** (`WS_EVT_CONNECT`): after sending `welcome` and `history` to the new client, the server broadcasts `{type:"join", nick}` to all clients.
- **Leave** (`WS_EVT_DISCONNECT`): the server broadcasts `{type:"leave", nick}` to all clients.

These are **ephemeral** — they are **not** pushed to the message buffer and are therefore not replayed to new joiners.

The frontend coalesces consecutive join/leave events into a single DOM element (`presenceEl`) using a `presenceGroup` array. Each entry tracks `{nick, action}` where `action` is `'joined'`, `'left'`, or `'joined_and_left'`. The element is "sealed" (a new one started) when any real chat message or nick-change arrives. This prevents a burst of joins/leaves from flooding the message list.

### WebSocket protocol (JSON)

All messages are JSON text frames. Fragmented or binary frames are silently ignored.

**Server → client:**

| Packet | When | Fields |
|---|---|---|
| `welcome` | On connect (and after nick change) | `{type, nick}` |
| `history` | Immediately after `welcome` | `{type, messages: [serialised JSON strings]}` |
| `message` | New public or private chat message | `{type, nick, text}` + optional `pm: true` |
| `join` | A client connects | `{type, nick}` |
| `leave` | A client disconnects | `{type, nick}` |
| `nick` | Any user renames | `{type, old, new}` |
| `error` | Bad JSON, invalid nick, user not found | `{type, text}` |

**Client → server:**

| Packet | Action | Fields |
|---|---|---|
| `message` | Send public message | `{type, text}` |
| `message` + `to` | Send private message | `{type, text, to: targetNick}` |
| `nick` | Change nickname | `{type, nick}` |

### Nickname system

- On WebSocket connect, the server looks up the client's MAC address via `esp_wifi_ap_get_sta_list_with_ip`. If the MAC lookup fails, it falls back to the client IP.
- The MAC/IP string is hashed with two independent polynomial hashes to pick one of 20 adjectives and one of 20 fruits/vegetables (e.g. `ExcitableBanana`). Same device always gets the same auto-nickname.
- Users can rename themselves. The server validates: non-empty, max 20 chars, alphanumeric + spaces only. On success it broadcasts a `nick` packet to all clients and sends a `welcome` packet directly to the renaming client (the authoritative identity update).
- Nicknames are stored in `std::map<uint32_t, std::string> client_nicknames` keyed by WebSocket client ID.

### IP address usage (complete inventory)

IP addresses appear in three distinct roles in `src/main.cpp`:

#### 1. `visited_clients` — captive portal "known client" set

```cpp
static std::set<std::string> visited_clients;  // MAC string, or IP string as fallback
static const size_t VISITED_CLIENTS_MAX = 20;
```

- **Purpose:** distinguish first-time visitors from returning ones so that OS captive-portal probe URLs (`/generate_204`, `/hotspot-detect.html`, `/connecttest.txt`, `/redirect`, `/success.txt`, `/fwlink/*`) return the correct success response instead of a redirect once the user has already loaded the chat page.
- **Populated:** in the `GET /` handler — `visited_clients.insert(client_id_for_ip(ip))` — only after the root page is served.
- **Checked:** in every captive-portal probe handler and in `onNotFound` via `visited_clients.count(client_id_for_ip(request->client()->remoteIP()))`.
- **Eviction:** no LRU; when `visited_clients.size() >= 20` the entire set is cleared before inserting the new entry. This is intentional — connections are ephemeral and the set only needs to survive for the duration of a single AP session.
- **Scope:** RAM only, lost on reboot. Never written to flash.
- **Type stored:** `std::string` — MAC address (e.g. `"AA:BB:CC:DD:EE:FF"`) when `mac_for_ip()` succeeds, otherwise the IP string (e.g. `"192.168.4.2"`). Using MAC as the primary key means a client that renews its DHCP lease is still recognised as already-visited.
- **Key resolution:** the `client_id_for_ip(const IPAddress&)` helper encapsulates this logic — it calls `mac_for_ip()` and falls back to `ip.toString()` — so the pattern is not repeated at every call site.

#### 2. Nickname generation fallback

```cpp
IPAddress remote_ip = client->remoteIP();
String mac = mac_for_ip(remote_ip);
String hash_input = mac.length() > 0 ? mac : remote_ip.toString();
std::string nick = generate_nickname(hash_input);
```

- **Purpose:** if `mac_for_ip()` cannot find the client in the AP's DHCP lease table (e.g. the lookup races with DHCP assignment), the IP string is used as the hash input instead of the MAC. The resulting nickname is still deterministic for that IP.
- **Preferred identifier:** MAC address (stable across reconnects). IP is only the fallback.
- **Not stored:** the IP is used transiently during nickname generation and is not retained after `WS_EVT_CONNECT` returns.

#### 3. `mac_for_ip()` — IP-to-MAC resolution

```cpp
static String mac_for_ip(const IPAddress& ip) {
    // calls esp_wifi_ap_get_sta_list() then esp_wifi_ap_get_sta_list_with_ip()
    // iterates mac_ip_list.sta[] matching on sta[i].ip.addr == (uint32_t)ip
}
```

- **Purpose:** translate the WebSocket client's remote IP to its MAC address so the nickname is stable across DHCP lease renewals.
- **Scope:** purely transient; the IP is only used as a lookup key and is not stored.

#### Summary table

| Variable / call site | Type | Stored? | Purpose |
|---|---|---|---|
| `visited_clients` | `std::set<std::string>` | RAM only | Captive portal "already visited" gate |
| `client_id_for_ip(remote_ip)` | transient `std::string` | No | Resolve IP → MAC (or IP string fallback) for `visited_clients` |
| `mac_for_ip(remote_ip)` | transient `IPAddress` | No | Resolve IP → MAC for nickname hashing and `client_id_for_ip()` |
| `hash_input = remote_ip.toString()` | transient `String` | No | Fallback hash seed when MAC lookup fails |
| `request->client()->remoteIP()` in probe handlers | transient `IPAddress` | No | Check membership in `visited_clients` via `client_id_for_ip()` |

### Frontend (index_html.h)

The entire UI is a single HTML file embedded as a C++ raw string literal. No external dependencies, no build step.

Key frontend behaviours:
- Connects to `ws://<host>/ws` on page load; reconnects after 2 seconds on disconnect.
- On `welcome`: stores `currentNick`, updates the topbar display.
- On `history`: replays buffered messages without timestamps (timestamps are only added to live messages).
- On `message`: calls `appendMessage()` which creates DOM nodes directly (no innerHTML). System messages (`nick === 'System'`) get italic grey styling. PM messages get a purple `PM` badge.
- On `nick`: appends a system-style "X is now known as Y" line. Does **not** update `currentNick` — that only comes from `welcome`.
- Clicking a non-system, non-self nickname enters PM mode: sets `pmTarget`, shows a banner above the input. Sending while in PM mode adds `to: pmTarget` to the payload.
- PM mode auto-closes when a `System` message matching `"<pmTarget> has left"` arrives.
- `visualViewport` resize listener adjusts `body.style.height` to keep the input bar above the soft keyboard on mobile.
- Welcome modal shown on first load (per session via `sessionStorage`), explaining the "no internet" warning and how to open the chat in a real browser.

## Build and flash commands

```sh
pio run              # compile
pio run -t upload    # flash to connected ESP32
pio device monitor   # serial monitor at 115200 baud
```

## Tickets

All 15 tickets in `.tickets/` are closed. There are no open tickets.
