---
id: pla-6tvy
status: closed
deps: []
links: []
created: 2026-04-02T00:52:52Z
type: task
priority: 2
assignee: Stavros Korokithakis
---
# Server: send structured presence events

Replace join/leave System messages with structured JSON presence events. On WS_EVT_CONNECT, broadcast {type:'join', nick:'X'} instead of make_message/push_message/broadcast of a System message. On WS_EVT_DISCONNECT, same with {type:'leave', nick:'X'}. Remove both push_message calls so presence is not stored in history. Keep the Serial.printf logging as-is. Non-goals: do not touch nick-change messages, do not modify the frontend.

## Acceptance Criteria

pio run builds. No push_message calls for join/leave. Broadcast sends JSON with type='join' or type='leave' and a nick field.

