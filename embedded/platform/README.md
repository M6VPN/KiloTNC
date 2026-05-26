# Embedded Platform Layer

The platform layer will adapt dev-board hardware services to the portable core.

Planned adapters:

- Clock and tick.
- Watchdog and reset cause.
- GPIO and test-only PTT.
- USB CDC byte stream.
- Audio loopback or test path.
- Diagnostics output.
- Config and persistence.

M2.0 does not implement these adapters. M2.1 should add the first compile-only boundary before any hardware driver work starts.
