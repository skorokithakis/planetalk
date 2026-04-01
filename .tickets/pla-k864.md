---
id: pla-k864
status: closed
deps: []
links: []
created: 2026-04-01T23:20:09Z
type: task
priority: 2
assignee: Stavros Korokithakis
---
# Spoof connectivity checks after first visit

After a client has loaded the chat page at least once, return the expected success responses for OS connectivity checks so the device stops showing 'no internet' warnings and doesn't auto-disconnect from the AP.

Backend changes (src/main.cpp):
- Track which client IPs have visited the main page (a simple std::set<uint32_t> of IP addresses, capped at ~20 entries).
- When a connectivity check URL is requested (/generate_204, /hotspot-detect.html, /connecttest.txt, /redirect, /success.txt, /fwlink/*):
  - If the client IP is NOT in the visited set: redirect to / (triggers captive portal as before).
  - If the client IP IS in the visited set: return the expected success response:
    - /generate_204 → HTTP 204 No Content
    - /hotspot-detect.html → HTTP 200, body: '<HTML><HEAD><TITLE>Success</TITLE></HEAD><BODY>Success</BODY></HTML>'
    - /connecttest.txt → HTTP 200, body: 'Microsoft Connect Test'
    - /success.txt → HTTP 200, body: 'success\n'
    - /redirect and /fwlink/* → HTTP 200 empty body
- When serving / (the main page), add the client IP to the visited set.
- Evict oldest entries if the set exceeds ~20 (matches max 10 stations with some headroom for IP reuse).

## Acceptance Criteria

First connection triggers captive portal. After visiting the chat page, the device stops showing 'no internet' warnings.

