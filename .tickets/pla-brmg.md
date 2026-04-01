---
id: pla-brmg
status: closed
deps: []
links: []
created: 2026-04-01T23:23:36Z
type: task
priority: 2
assignee: Stavros Korokithakis
---
# Redirect to http://192.168.4.1 instead of relative /

Currently redirect_to_root does request->redirect('/'), which keeps the original hostname. Change all redirects (captive portal detection endpoints, onNotFound) to redirect to the absolute URL http://192.168.4.1/ so the browser shows the real IP. Use WiFi.softAPIP() to build the URL rather than hardcoding, in case the AP IP ever changes.

Change in src/main.cpp: update redirect_to_root to redirect to 'http://' + WiFi.softAPIP().toString() + '/' instead of '/'.

## Acceptance Criteria

Visiting any HTTP URL on the AP redirects to http://192.168.4.1/ (the actual AP IP).

