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

M2.1 adds the first compile-only platform boundary and a safe host-native stub. It does not add hardware drivers.
