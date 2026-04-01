---
id: pla-hosu
status: closed
deps: []
links: []
created: 2026-04-01T21:02:30Z
type: task
priority: 2
assignee: Stavros Korokithakis
---
# Add private messaging via nickname tap

Add private messaging to the chat. No IRC commands — this is UI-driven.

**Backend (src/main.cpp):**
- When a message has an optional 'to' field: {type:'message', text:'...', to:'NickName'}, deliver it only to the client with that nickname AND echo it back to the sender. Do NOT store PMs in the message buffer.
- If the 'to' nickname doesn't match any connected client, send {type:'error', text:'User not found'} back to the sender.
- PM delivery messages should include a 'pm' field set to true so the client can style them differently: {type:'message', nick:'SenderNick', text:'...', pm:true}
- For the recipient, also include a 'from' field so they know who sent it (the nick field already covers this, but the 'pm':true flag tells the UI to style it as private).

**Frontend (src/index_html.h):**
- Tapping/clicking a nickname in the message list enters PM mode for that user.
- A banner appears directly above the input area (between messages and the input box): 'Private message to CoolGuy ✕'. Use a subtle distinct style (e.g. a muted purple/indigo background strip). The ✕ closes PM mode and returns to public chat.
- While in PM mode, sent messages include the 'to' field with the target nickname.
- PM messages (incoming and outgoing) are styled differently in the message list: add a small 'PM' badge and/or a different accent color to distinguish from public messages.
- Nicknames in the message list need to be clickable (wrap in a <span> with cursor:pointer and a click handler). Don't make System message nicks clickable.
- When the user whose nick you're PMing disconnects (you see them leave), auto-close PM mode.

## Acceptance Criteria

Tapping a nick enters PM mode. PMs are delivered only to the target and sender. PMs styled distinctly. Banner shows above input with close button. PM mode auto-closes on target disconnect.

