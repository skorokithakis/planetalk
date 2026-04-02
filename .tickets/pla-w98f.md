---
id: pla-w98f
status: closed
deps: []
links: []
created: 2026-04-01T23:26:59Z
type: task
priority: 2
assignee: Stavros Korokithakis
---
# Use client MAC address for nickname generation

Change nickname_for_ip to use the client's MAC address instead of IP for more stable nicknames across reconnects.

In src/main.cpp:
- On WebSocket connect, use esp_wifi_ap_get_sta_list() and esp_netif_get_sta_list() to look up the client's MAC by matching their IP.
- Hash the MAC string (e.g. 'AA:BB:CC:DD:EE:FF') instead of the IP string.
- If the MAC lookup fails for any reason, fall back to hashing the IP as before.
- Rename the function to reflect it no longer only takes IP (or keep it generic).

## Acceptance Criteria

Same device gets the same nickname across reconnects. Falls back to IP-based nickname if MAC lookup fails.

