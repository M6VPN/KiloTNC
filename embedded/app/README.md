# Embedded Application

The embedded application layer will connect the portable TNC core to platform adapters.

Planned responsibilities:

- Initialize safe default state.
- Keep PTT off unless explicitly enabled by test firmware.
- Route USB CDC KISS bytes to the core.
- Route audio test data through the core after loopback paths exist.
- Publish diagnostics and fault counters.
- Enforce watchdog and max TX timeout policy.

M2.2 extends the application skeleton with app state, reset cause, 10 ms tick count, watchdog kick count, PTT state, and fault count in status. It does not process real USB or audio data yet.
