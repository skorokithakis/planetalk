---
id: pla-ab2e
status: closed
deps: [pla-6tvy]
links: []
created: 2026-04-02T00:53:01Z
type: task
priority: 2
assignee: Stavros Korokithakis
---
# Frontend: coalesce presence events

Handle new 'join' and 'leave' WebSocket message types. Maintain a pending presence group: an ordered list of {nick, actions} where actions tracks whether someone joined, left, or both. On receiving a join/leave event, update the group and render/re-render a single system-styled DOM element at the bottom of the chat. When a real chat message (type='message') arrives, seal the current group (stop updating that DOM element) and clear the pending state so future presence events create a new element. Rendering rules: group by action. 'X has joined', 'X, Y have joined', 'X has joined and left', mixed: 'X, Y have joined, Z has left'. Use 'has' for single, 'have' for multiple. Non-goals: do not change how nick-change messages work, do not persist presence in any way.

## Acceptance Criteria

Consecutive joins/leaves without intervening chat render as one coalesced line. A chat message seals the group. Refresh loses all presence lines. pio run builds.

