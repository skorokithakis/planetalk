---
id: pla-epwh
status: closed
deps: []
links: []
created: 2026-04-01T23:19:47Z
type: task
priority: 2
assignee: Stavros Korokithakis
---
# Fix mobile keyboard pushing messages off screen

When the soft keyboard opens on mobile, the messages area is pushed up and out of viewport bounds instead of the viewport shrinking to fit. This is a common mobile web issue.

Fix in src/index_html.h:
- Use the visualViewport API to dynamically resize the chat layout when the keyboard opens/closes. Listen to visualViewport 'resize' events and set the body/container height to visualViewport.height.
- Alternatively, use position:fixed for the input area and adjust the messages container bottom padding/margin accordingly.
- Ensure auto-scroll to bottom still works after keyboard open.
- Test mental model: messages should remain visible and scrollable with keyboard open, input area pinned to just above the keyboard.

## Acceptance Criteria

Opening the keyboard on mobile keeps messages visible and scrollable, input stays above keyboard.

