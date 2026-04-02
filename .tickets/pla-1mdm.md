---
id: pla-1mdm
status: closed
deps: []
links: []
created: 2026-04-02T00:02:19Z
type: task
priority: 2
assignee: Stavros Korokithakis
---
# Add serial debugging/logging


## Notes

**2026-04-02T00:17:00Z**

Add Serial.println logging to main.cpp for key events: WebSocket connect/disconnect (client ID, IP, nickname), messages (nick, public vs PM, target), nickname changes (old/new), errors (JSON parse, user not found), and captive portal probe hits (endpoint, IP, redirected vs success). Keep log lines concise — one line per event. No log-level framework, just plain Serial.println.
