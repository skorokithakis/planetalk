#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <map>
#include <set>
#include <string>
#include <array>
#include <esp_wifi.h>
#include <esp_netif.h>
#include <esp_wifi_ap_get_sta_list.h>

#include "index_html.h"

static const char* AP_SSID = "PlaneTalk";
static const int AP_MAX_STATIONS = 10;
// DNS port 53 is the standard DNS port; all queries are answered with the AP
// IP so that every hostname resolves to us, triggering captive portal detection.
static const uint8_t DNS_PORT = 53;

static const int MESSAGE_BUFFER_SIZE = 50;

static DNSServer dns_server;
static AsyncWebServer http_server(80);
static AsyncWebSocket web_socket("/ws");

// Circular buffer of recent messages stored as serialised JSON strings so they
// can be forwarded verbatim to newly-connected clients without re-serialising.
static std::array<std::string, MESSAGE_BUFFER_SIZE> message_buffer;
static int message_buffer_head = 0;  // Index of the oldest message slot.
static int message_buffer_count = 0;

static std::map<uint32_t, std::string> client_nicknames;

// Tracks which client IPs have loaded the main page at least once. Used to
// distinguish first-time visitors (who should see the captive portal popup)
// from returning visitors (who should get real success responses so the OS
// stops showing "no internet" warnings). Capped at 20 entries; cleared
// entirely when full because these are ephemeral AP connections and we never
// need to evict a specific entry.
static std::set<uint32_t> visited_ips;
static const size_t VISITED_IPS_MAX = 20;

static const char* ADJECTIVES[] = {
    "Excitable", "Sneaky", "Jolly", "Grumpy", "Witty",
    "Brave", "Sleepy", "Fuzzy", "Mighty", "Gentle",
    "Zesty", "Cosmic", "Dizzy", "Lucky", "Peppy",
    "Stormy", "Chill", "Bouncy", "Sassy", "Quirky",
};
static const int ADJECTIVES_COUNT = 20;

static const char* FRUITS[] = {
    "Banana", "Tomato", "Avocado", "Mango", "Radish",
    "Lemon", "Pepper", "Cherry", "Turnip", "Olive",
    "Papaya", "Celery", "Coconut", "Potato", "Apricot",
    "Cabbage", "Ginger", "Peach", "Carrot", "Parsnip",
};
static const int FRUITS_COUNT = 20;

// Derive a deterministic nickname from an arbitrary string by hashing each
// character with a simple polynomial hash, then taking modulo the list sizes.
// Using two independent hash seeds ensures the adjective and fruit selections
// are uncorrelated even when inputs differ by only one character.
static std::string generate_nickname(const String& input) {
    uint32_t hash_a = 5381;
    uint32_t hash_b = 0x811c9dc5;
    for (size_t i = 0; i < (size_t)input.length(); i++) {
        hash_a = hash_a * 33 ^ (uint8_t)input[i];
        hash_b = (hash_b ^ (uint8_t)input[i]) * 0x01000193;
    }
    std::string nick = ADJECTIVES[hash_a % ADJECTIVES_COUNT];
    nick += FRUITS[hash_b % FRUITS_COUNT];
    return nick;
}

// Look up the MAC address of a connected station by matching its IP against
// the AP's DHCP lease table. Returns a formatted MAC string ("AA:BB:CC:DD:EE:FF")
// on success, or an empty string if the station is not found or the lookup fails.
// MAC addresses are stable across reconnects (unlike DHCP-assigned IPs), making
// them a better key for nickname generation.
static String mac_for_ip(const IPAddress& ip) {
    wifi_sta_list_t wifi_sta_list;
    if (esp_wifi_ap_get_sta_list(&wifi_sta_list) != ESP_OK) {
        return String();
    }

    wifi_sta_mac_ip_list_t mac_ip_list;
    if (esp_wifi_ap_get_sta_list_with_ip(&wifi_sta_list, &mac_ip_list) != ESP_OK) {
        return String();
    }

    uint32_t target_ip = (uint32_t)ip;
    for (int i = 0; i < mac_ip_list.num; i++) {
        if (mac_ip_list.sta[i].ip.addr == target_ip) {
            const uint8_t* mac = mac_ip_list.sta[i].mac;
            char buf[18];
            snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            return String(buf);
        }
    }
    return String();
}

static void push_message(const std::string& json_string) {
    message_buffer[message_buffer_head] = json_string;
    message_buffer_head = (message_buffer_head + 1) % MESSAGE_BUFFER_SIZE;
    if (message_buffer_count < MESSAGE_BUFFER_SIZE) {
        message_buffer_count++;
    }
}

// Serialise the message buffer into a {type:"history", messages:[...]} packet
// and send it to a single client.
static void send_history(AsyncWebSocketClient* client) {
    JsonDocument doc;
    doc["type"] = "history";
    JsonArray messages = doc["messages"].to<JsonArray>();

    // Walk the buffer from oldest to newest.
    int start = (message_buffer_count < MESSAGE_BUFFER_SIZE)
                    ? 0
                    : message_buffer_head;
    for (int i = 0; i < message_buffer_count; i++) {
        int index = (start + i) % MESSAGE_BUFFER_SIZE;
        messages.add(message_buffer[index].c_str());
    }

    std::string output;
    serializeJson(doc, output);
    client->text(output.c_str());
}

static void broadcast(const std::string& json_string) {
    web_socket.textAll(json_string.c_str());
}

static std::string make_message(const char* nick, const char* text) {
    JsonDocument doc;
    doc["type"] = "message";
    doc["nick"] = nick;
    doc["text"] = text;
    std::string output;
    serializeJson(doc, output);
    return output;
}

static void handle_websocket_event(
    AsyncWebSocket* server,
    AsyncWebSocketClient* client,
    AwsEventType event_type,
    void* arg,
    uint8_t* data,
    size_t length
) {
    if (event_type == WS_EVT_CONNECT) {
        IPAddress remote_ip = client->remoteIP();
        String mac = mac_for_ip(remote_ip);
        String hash_input = mac.length() > 0 ? mac : remote_ip.toString();
        std::string nick = generate_nickname(hash_input);
        client_nicknames[client->id()] = nick;

        Serial.printf("[WS] connect id=%u ip=%s mac=%s nick=%s\r\n",
            client->id(),
            remote_ip.toString().c_str(),
            mac.length() > 0 ? mac.c_str() : "no MAC",
            nick.c_str());

        // Tell this client its assigned nickname before sending history so the
        // UI can display it before any messages are rendered.
        JsonDocument welcome;
        welcome["type"] = "welcome";
        welcome["nick"] = nick.c_str();
        std::string welcome_str;
        serializeJson(welcome, welcome_str);
        client->text(welcome_str.c_str());

        send_history(client);

        JsonDocument join_doc;
        join_doc["type"] = "join";
        join_doc["nick"] = nick.c_str();
        std::string join_msg;
        serializeJson(join_doc, join_msg);
        broadcast(join_msg);

    } else if (event_type == WS_EVT_DISCONNECT) {
        auto it = client_nicknames.find(client->id());
        if (it != client_nicknames.end()) {
            Serial.printf("[WS] disconnect id=%u nick=%s\r\n",
                client->id(), it->second.c_str());
            JsonDocument leave_doc;
            leave_doc["type"] = "leave";
            leave_doc["nick"] = it->second.c_str();
            std::string leave_msg;
            serializeJson(leave_doc, leave_msg);
            broadcast(leave_msg);
            client_nicknames.erase(it);
        }

    } else if (event_type == WS_EVT_DATA) {
        AwsFrameInfo* info = (AwsFrameInfo*)arg;
        // Only handle complete, single-frame text messages to keep things simple.
        // Fragmented or binary messages are silently ignored.
        if (!info->final || info->index != 0 || info->opcode != WS_TEXT) {
            return;
        }

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, data, length);
        if (error) {
            Serial.printf("[WS] JSON parse error id=%u\r\n", client->id());
            JsonDocument err;
            err["type"] = "error";
            err["text"] = "Invalid JSON";
            std::string err_str;
            serializeJson(err, err_str);
            client->text(err_str.c_str());
            return;
        }

        const char* type = doc["type"];
        if (!type) return;

        if (strcmp(type, "message") == 0) {
            const char* text = doc["text"];
            if (!text || strlen(text) == 0) return;
            if (strlen(text) > 500) return;

            auto it = client_nicknames.find(client->id());
            if (it == client_nicknames.end()) return;

            const char* to_nick = doc["to"];
            if (to_nick && strlen(to_nick) > 0) {
                if (it->second == to_nick) return;

                // Private message: find the target client by nickname.
                AsyncWebSocketClient* target_client = nullptr;
                for (auto& entry : client_nicknames) {
                    if (entry.second == to_nick) {
                        target_client = web_socket.client(entry.first);
                        break;
                    }
                }

                if (!target_client) {
                    Serial.printf("[PM] %s -> %s: target not found\r\n",
                        it->second.c_str(), to_nick);
                    JsonDocument err;
                    err["type"] = "error";
                    err["text"] = "User not found";
                    std::string err_str;
                    serializeJson(err, err_str);
                    client->text(err_str.c_str());
                    return;
                }

                // Build the PM payload once; both recipient and sender see it.
                JsonDocument pm_doc;
                pm_doc["type"] = "message";
                pm_doc["nick"] = it->second.c_str();
                pm_doc["text"] = text;
                pm_doc["pm"]   = true;
                std::string pm_str;
                serializeJson(pm_doc, pm_str);

                Serial.printf("[PM] %s -> %s: %s\r\n",
                    it->second.c_str(), to_nick, text);
                target_client->text(pm_str.c_str());
                // Echo back to sender only if they are not messaging themselves.
                if (target_client->id() != client->id()) {
                    client->text(pm_str.c_str());
                }
                // PMs are ephemeral — not stored in the message buffer.
                return;
            }

            Serial.printf("[MSG] %s: %s\r\n", it->second.c_str(), text);
            std::string msg = make_message(it->second.c_str(), text);
            push_message(msg);
            broadcast(msg);

        } else if (strcmp(type, "nick") == 0) {
            const char* new_nick_raw = doc["nick"];
            if (!new_nick_raw) return;

            // Strip leading/trailing whitespace.
            std::string new_nick = new_nick_raw;
            size_t first = new_nick.find_first_not_of(" \t\r\r\n");
            size_t last  = new_nick.find_last_not_of(" \t\r\r\n");
            if (first == std::string::npos) {
                new_nick = "";
            } else {
                new_nick = new_nick.substr(first, last - first + 1);
            }

            // Validate: non-empty, max 20 chars, letters/digits/spaces only.
            bool valid = !new_nick.empty() && new_nick.size() <= 20;
            if (valid) {
                for (char c : new_nick) {
                    if (!isalnum((unsigned char)c) && c != ' ') { valid = false; break; }
                }
            }

            if (!valid) {
                Serial.printf("[NICK] rejected: %s\r\n", new_nick_raw);
                JsonDocument err;
                err["type"] = "error";
                err["text"] = "Nickname must be 1-20 characters (letters, numbers, spaces)";
                std::string err_str;
                serializeJson(err, err_str);
                client->text(err_str.c_str());
                return;
            }

            auto it = client_nicknames.find(client->id());
            if (it == client_nicknames.end()) return;

            std::string old_nick = it->second;
            it->second = new_nick;

            Serial.printf("[NICK] %s -> %s\r\n", old_nick.c_str(), new_nick.c_str());

            JsonDocument nick_doc;
            nick_doc["type"] = "nick";
            nick_doc["old"]  = old_nick.c_str();
            nick_doc["new"]  = new_nick.c_str();
            std::string nick_str;
            serializeJson(nick_doc, nick_str);
            broadcast(nick_str);

            // Tell the requesting client its confirmed new nick via the welcome
            // message type. Clients only update their own nick display from
            // welcome messages, so this is the authoritative identity update.
            JsonDocument welcome_doc;
            welcome_doc["type"] = "welcome";
            welcome_doc["nick"] = new_nick.c_str();
            std::string welcome_str;
            serializeJson(welcome_doc, welcome_str);
            client->text(welcome_str.c_str());
        }
    }
}

// Redirect the request to the captive portal root using an absolute URL so
// that OSes which follow the redirect land on the correct AP address rather
// than resolving "/" relative to whatever hostname they probed.
static void redirect_to_portal(AsyncWebServerRequest* request) {
    String url = "http://";
    url += WiFi.softAPIP().toString();
    url += "/";
    request->redirect(url);
}


void setup() {
    Serial.begin(115200);

    // Open AP with no password; channel 1, not hidden, max 10 connected stations.
    WiFi.softAP(AP_SSID, nullptr, 1, 0, AP_MAX_STATIONS);

    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        const uint8_t* mac = info.wifi_ap_staconnected.mac;
        Serial.printf("[WiFi] Station connected: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }, ARDUINO_EVENT_WIFI_AP_STACONNECTED);

    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        const uint8_t* mac = info.wifi_ap_stadisconnected.mac;
        Serial.printf("[WiFi] Station disconnected: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }, ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);

    Serial.print("AP started. IP: ");
    Serial.println(WiFi.softAPIP());

    // Wildcard DNS: every hostname resolves to the AP IP, which is what causes
    // Android/iOS/Windows/Firefox to detect a captive portal and show a popup.
    dns_server.start(DNS_PORT, "*", WiFi.softAPIP());

    web_socket.onEvent(handle_websocket_event);
    http_server.addHandler(&web_socket);

    // Captive portal detection endpoints — each OS probes a different URL.
    // On first visit the client is redirected to / to trigger the portal popup.
    // After the client has loaded / at least once, return the real success
    // response so the OS stops showing "no internet" warnings.

    // Android probes /generate_204 and expects HTTP 204 once connected.
    http_server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest* request) {
        bool known = visited_ips.count(request->client()->remoteIP());
        Serial.printf("[HTTP] /generate_204 ip=%s -> %s\r\n",
            request->client()->remoteIP().toString().c_str(),
            known ? "success" : "redirect");
        if (known) {
            request->send(204);
        } else {
            redirect_to_portal(request);
        }
    });
    // Apple probes /hotspot-detect.html.
    http_server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest* request) {
        bool known = visited_ips.count(request->client()->remoteIP());
        Serial.printf("[HTTP] /hotspot-detect.html ip=%s -> %s\r\n",
            request->client()->remoteIP().toString().c_str(),
            known ? "success" : "redirect");
        if (known) {
            request->send(200, "text/html",
                "<HTML><HEAD><TITLE>Success</TITLE></HEAD><BODY>Success</BODY></HTML>");
        } else {
            redirect_to_portal(request);
        }
    });
    // Windows probes /connecttest.txt.
    http_server.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest* request) {
        bool known = visited_ips.count(request->client()->remoteIP());
        Serial.printf("[HTTP] /connecttest.txt ip=%s -> %s\r\n",
            request->client()->remoteIP().toString().c_str(),
            known ? "success" : "redirect");
        if (known) {
            request->send(200, "text/plain", "Microsoft Connect Test");
        } else {
            redirect_to_portal(request);
        }
    });
    // Windows also probes /redirect.
    http_server.on("/redirect", HTTP_GET, [](AsyncWebServerRequest* request) {
        bool known = visited_ips.count(request->client()->remoteIP());
        Serial.printf("[HTTP] /redirect ip=%s -> %s\r\n",
            request->client()->remoteIP().toString().c_str(),
            known ? "success" : "redirect");
        if (known) {
            request->send(200);
        } else {
            redirect_to_portal(request);
        }
    });
    // Firefox probes /success.txt.
    http_server.on("/success.txt", HTTP_GET, [](AsyncWebServerRequest* request) {
        bool known = visited_ips.count(request->client()->remoteIP());
        Serial.printf("[HTTP] /success.txt ip=%s -> %s\r\n",
            request->client()->remoteIP().toString().c_str(),
            known ? "success" : "redirect");
        if (known) {
            request->send(200, "text/plain", "success\r\n");
        } else {
            redirect_to_portal(request);
        }
    });
    // Microsoft uses /fwlink/ paths; the wildcard prefix matches all of them.
    http_server.on("/fwlink/*", HTTP_GET, [](AsyncWebServerRequest* request) {
        bool known = visited_ips.count(request->client()->remoteIP());
        Serial.printf("[HTTP] %s ip=%s -> %s\r\n",
            request->url().c_str(),
            request->client()->remoteIP().toString().c_str(),
            known ? "success" : "redirect");
        if (known) {
            request->send(200);
        } else {
            redirect_to_portal(request);
        }
    });

    http_server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        // If the request arrived on a spoofed hostname (e.g. the user typed
        // "example.com" and our wildcard DNS resolved it here), redirect to
        // the real AP IP so the URL bar shows the correct address.
        String host = request->host();
        if (host.length() > 0 && host != WiFi.softAPIP().toString()) {
            redirect_to_portal(request);
            return;
        }

        uint32_t ip = request->client()->remoteIP();
        if (visited_ips.size() >= VISITED_IPS_MAX) {
            visited_ips.clear();
        }
        visited_ips.insert(ip);
        request->send(200, "text/html", INDEX_HTML);
    });

    // Known IPs get a 200 so unhandled OS probe URLs don't re-trigger captive
    // portal detection for users who have already loaded the chat. Unknown IPs
    // are still redirected to root to trigger the initial portal popup.
    http_server.onNotFound([](AsyncWebServerRequest* request) {
        bool known = visited_ips.count(request->client()->remoteIP());
        Serial.printf("[HTTP] %s ip=%s -> %s\r\n",
            request->url().c_str(),
            request->client()->remoteIP().toString().c_str(),
            known ? "success" : "redirect");
        if (known) {
            request->send(200);
        } else {
            redirect_to_portal(request);
        }
    });

    http_server.begin();
}

void loop() {
    dns_server.processNextRequest();
    web_socket.cleanupClients();
    // Yield to the RTOS idle task so the CPU isn't spinning at 100%, which
    // reduces power draw and heat significantly.
    delay(1);
}
