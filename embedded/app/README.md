# Embedded Application

The embedded application layer will connect the portable TNC core to platform adapters.

Planned responsibilities:

- Initialize safe default state.
- Keep PTT off unless explicitly enabled by test firmware.
- Route USB CDC KISS bytes to the core.
- Route audio test data through the core after loopback paths exist.
- Publish diagnostics and fault counters.
- Enforce watchdog and max TX timeout policy.

M2.3 adds an optional USB bridge hook. Tests can attach the host-native USB CDC stub so `embedded_app_step()` services byte echo or KISS data-frame loopback while still kicking the watchdog and keeping PTT off.

The bridge is not a USB device stack. It has no descriptors, endpoints, TinyUSB calls, HAL calls, or hardware access.

M2.4 adds an embedded diagnostics bridge. It captures app state, reset cause, PTT state, platform ticks, watchdog kicks, diagnostic writes, USB byte counters, USB overflow counters, and KISS parser counters into a fixed snapshot. The formatter writes stable text into caller-provided buffers and detects truncation.
