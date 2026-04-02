---
id: pla-kbz9
status: closed
deps: []
links: []
created: 2026-04-02T01:21:32Z
type: task
priority: 2
assignee: Stavros Korokithakis
---
# Enable USB CDC serial output and add WiFi station logging

Two changes: (1) Add build_flags to platformio.ini for ESP32-C3 USB CDC: -DARDUINO_USB_MODE=1 and -DARDUINO_USB_CDC_ON_BOOT=1. (2) Register a WiFi.onEvent callback in setup_wifi() that logs station connect/disconnect with MAC address, using the existing [TAG] format (e.g. [WiFi] Station connected: AA:BB:CC:DD:EE:FF). Build with 'pio run' after changes.

## Acceptance Criteria

pio run succeeds. platformio.ini has USB CDC build flags. WiFi event handler registered in setup_wifi() logging connects and disconnects with MAC.

