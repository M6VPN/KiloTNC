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

M2.2 adds host-native platform stubs for:

- Monotonic millisecond tick.
- 10 ms control ticks.
- Watchdog kick counter.
- Simulated watchdog fault.
- Reset-cause reporting.
- Test GPIO/PTT state.
- Diagnostic write counter.
- Platform fault counter.

The future STM32 adapter should implement the same interface. M2.2 does not add hardware drivers or register access.
