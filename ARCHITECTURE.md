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
- Serving `/` marks the client IP as "visited" in `std::set<uint32_t> visited_ips`.
- Subsequent OS probe requests from a known IP return the expected success responses (HTTP 204, 200 with correct body, etc.) so the OS stops showing "no internet" warnings.
- `visited_ips` is capped at 20 entries and cleared entirely when full (ephemeral AP connections, no need for LRU eviction).

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

- **Join** (`WS_EVT_CONNECT`): after sending `welcome` and `history` to the new client, the server calls `make_message("System", nick + " has joined")`, pushes it to the buffer, and broadcasts it to all clients.
- **Leave** (`WS_EVT_DISCONNECT`): the server calls `make_message("System", nick + " has left")`, pushes it to the buffer, and broadcasts it.

Both are regular `{type:"message", nick:"System", text:"..."}` packets — the frontend identifies them as system messages by checking `msg.nick === 'System'`.

### WebSocket protocol (JSON)

All messages are JSON text frames. Fragmented or binary frames are silently ignored.

**Server → client:**

| Packet | When | Fields |
|---|---|---|
| `welcome` | On connect (and after nick change) | `{type, nick}` |
| `history` | Immediately after `welcome` | `{type, messages: [serialised JSON strings]}` |
| `message` | New chat message or join/leave | `{type, nick, text}` + optional `pm: true` |
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

## Open ticket

- `pla-61dv` (open): add logging for new WiFi station connections (currently only WebSocket connect/disconnect is logged, not the lower-level WiFi association event).
