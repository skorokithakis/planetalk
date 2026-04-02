---
id: pla-wxrz
status: closed
deps: []
links: []
created: 2026-04-02T11:45:22Z
type: task
priority: 2
assignee: Stavros Korokithakis
---
# Replace visited_ips with MAC-based client identifier set

Replace `visited_ips` (std::set<uint32_t> of raw IPs) with a `std::set<std::string>` keyed by MAC address (falling back to IP string when MAC lookup fails). Use the existing `mac_for_ip()` helper to resolve client identity. Rename the variable and constant to `visited_clients`/`VISITED_CLIENTS_MAX`. Update all call sites: the `GET /` handler (which inserts) and every probe handler + `onNotFound` (which checks membership). Same eviction strategy (clear-all at cap), same cap of 20. Extract a small helper (e.g. `client_id_for_ip`) that calls `mac_for_ip` and falls back to `remote_ip.toString()` to avoid repeating the pattern at every call site. Update comments accordingly.

## Acceptance Criteria

All references to visited_ips replaced. Compiles cleanly with `pio run`.

