---
id: pla-ynrr
status: closed
deps: [pla-51ze]
links: []
created: 2026-04-01T20:37:57Z
type: task
priority: 2
assignee: Stavros Korokithakis
---
# WebSocket chat with nickname generation and chat UI

Replace the placeholder page with a full chat system:

**Backend (C++):**
- WebSocket endpoint at /ws.
- Circular message buffer (~50 messages). On new client connect, send the buffer as history.
- Nickname generation: hash client IP to deterministically pick from a list of ~20 adjectives (Excitable, Sneaky, Jolly, Grumpy, Witty, Brave, Sleepy, Fuzzy, Mighty, Gentle, Zesty, Cosmic, Dizzy, Lucky, Peppy, Stormy, Chill, Bouncy, Sassy, Quirky) and ~20 fruits/vegetables (Banana, Tomato, Avocado, Mango, Radish, Lemon, Pepper, Cherry, Turnip, Olive, Papaya, Celery, Coconut, Potato, Apricot, Cabbage, Ginger, Peach, Carrot, Parsnip). Same IP always gets the same name.
- Protocol: JSON messages. Client sends {type:'message', text:'...'} or {type:'nick', nick:'...'}. Server broadcasts {type:'message', nick:'...', text:'...'} and {type:'nick', old:'...', new:'...'} and {type:'history', messages:[...]}. Include a server timestamp in messages.
- On nick change, validate: non-empty, max 20 chars, alphanumeric only. Reject with {type:'error', text:'...'} if invalid.
- On client disconnect, optionally broadcast a leave notice.

**Frontend (HTML/CSS/JS, embedded in PROGMEM):**
- Single-page chat UI. Message list, input box, send button.
- Nickname shown at top with a click-to-edit or small edit button.
- Auto-scroll to bottom on new messages. Mobile-friendly (full viewport, large touch targets).
- Clean, minimal styling. Dark theme preferred.
- Connect via WebSocket, handle reconnection on disconnect.
- Render history on connect.

## Design

Embed the HTML as a raw string literal in a header file (e.g. src/index_html.h) to keep main.cpp readable. Use a single global array for the message buffer and a std::map<client_id, nickname> for tracking nicknames.

## Acceptance Criteria

Multiple clients can chat in real time. Nicknames are auto-assigned as AdjectiveFruit. Nickname change works. New joiners see recent history.

