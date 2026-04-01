---
id: pla-51ze
status: closed
deps: [pla-nsfc]
links: []
created: 2026-04-01T20:37:39Z
type: task
priority: 2
assignee: Stavros Korokithakis
---
# DNS captive portal and HTTP server

Add a DNS server that responds to all queries with the ESP32's AP IP (192.168.4.1), triggering captive portal detection on clients. Add an HTTP server that: 1) handles common captive portal detection URLs (/generate_204, /hotspot-detect.html, /connecttest.txt, /redirect, etc.) by redirecting to /. 2) Serves a placeholder 'hello' page at / for now (will be replaced by chat UI in next task). Use ESPAsyncWebServer and AsyncTCP. Libraries: ESPAsyncWebServer, AsyncTCP.

## Acceptance Criteria

Connecting to the AP triggers captive portal popup on Android/iOS/desktop. All HTTP requests redirect to the chat page.

