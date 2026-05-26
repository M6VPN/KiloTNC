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

M2.3 adds a separate host-native USB CDC stub. The stub models a CDC byte stream with fixed RX/TX buffers, connection state, read/write counters, and safe overflow rejection.

The future STM32 adapter should implement the same interface. M2.3 does not add hardware drivers, USB descriptors, endpoint code, TinyUSB, HAL calls, or register access.

M2.4 diagnostics reads platform counters through the existing app/platform boundary and USB counters through the CDC stats hook. The stub exposes reset cause, PTT state, watchdog kicks, diagnostic writes, RX/TX byte totals, and overflow counters for host tests.

M2.5 adds a host-native audio stub. It exposes 48 kHz mono signed 16-bit format metadata, fixed RX/TX sample buffers, RX injection, TX capture, sample counters, and overflow/underflow counters. It does not implement ADC, DAC, SAI, I2S, DMA, codec, or hardware audio paths.
