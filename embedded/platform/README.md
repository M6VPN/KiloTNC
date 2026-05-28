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

M2.13 adds a USB stack boundary placeholder. The only implemented USB path remains the host-native USB CDC stub. `tinyusb`, `stm32cube`, and `custom` are recognized as planned stack names but return unsupported. No TinyUSB, STM32Cube, CMSIS, HAL, descriptor, endpoint, interrupt, hardware register, or vendor SDK include is added.

M2.14 adds USB descriptor planning metadata for KISS-only and KISS-plus-diagnostics CDC ACM profiles. The descriptor plan is host-tested data only. It does not provide binary descriptors, VID/PID claims, endpoint handlers, TinyUSB binding, STM32Cube binding, or hardware USB access.

M2.14 also adds board abstraction planning. STM32H753 remains the flagship target, while H743, H735, and H750 remain future STM32H7-family possibilities behind board and resource metadata. RP2350 is a separate low-cost target path, and ESP32-S3 is a possible connectivity companion only.

M2.15 adds clock, reset, boot, and watchdog planning metadata. The future platform adapter must provide a timebase, 10 ms control tick, reset-cause capture, watchdog policy, and safe-off boot ordering. The current platform stub remains host-test only and does not configure clocks, read reset registers, enable hardware watchdogs, or access GPIO hardware.

M2.17 adds queue and backpressure planning for future ISR, DMA, USB, audio, diagnostics, and control boundaries. Platform adapters must keep queue overflow bounded and counted. Safety and control events must not be silently dropped, and watchdog progress must not depend on congested USB, audio, or diagnostics queues.

M2.19 adds host-tested scheduler quorum rules above the platform layer. Future platform tick and watchdog adapters must refresh the real watchdog only after scheduler quorum allows it. Platform code must keep the control/PTT safe-off path independent of congested USB, audio, diagnostics, or config work.
