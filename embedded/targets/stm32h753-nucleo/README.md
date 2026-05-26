# stm32h753-nucleo Target

This is the selected primary M2 target path.

Board path:

- NUCLEO-H753ZI or current equivalent STM32H753 Nucleo-144 board.

M2.10 status:

- Compile-only skeleton.
- Host-native platform stub tests.
- Host-native USB CDC byte-stream and KISS bridge tests.
- Host-native embedded diagnostics bridge tests.
- Host-native audio stub and loopback tests.
- Host-native embedded TNC KISS, mode, and diagnostics tests.
- Host-native embedded modem/audio boundary tests.
- Simulated AFSK1200 sample generation into the audio stub.
- Host-native RX audio/modem boundary tests.
- Decoded AX.25 frames emitted as KISS to the USB stub.
- Host-native full USB KISS to simulated audio TX/RX to USB KISS loopback tests.
- Timeout and watchdog-fault loopback tests.
- Compile-gated STM32H753 target skeleton files.
- Opt-in target syntax check through `make embedded-target-check`.
- Placeholder target platform hooks that return safe defaults or unsupported.
- Linker, startup, USB, watchdog, GPIO, and audio planning notes.
- No vendor project committed.
- No STM32 HAL or Cube code committed.
- No CMSIS tree committed.
- No TinyUSB tree committed.
- No startup vector table committed.
- No linker script committed.
- No real USB implementation.
- No codec driver.
- No real audio peripheral implementation.
- No audio DMA.
- No GPIO PTT implementation.
- No RF transmit path.
- No pinout finalized.
- No clock tree finalized.

The target headers contain metadata, planned feature flags, and safe compile-gated placeholders only. The target C files are not part of normal host builds and do not produce flashable firmware.

M2.11 status:

- STM32H753 resource planning is documented in [docs/m2-stm32h753-resource-plan.md](../../../docs/m2-stm32h753-resource-plan.md).
- Resource metadata is recorded in `target_resources.h`.
- USB, test PTT GPIO, diagnostics, audio, timing, watchdog, reset, and debug resources are planning items only.
- No pin assignments are final.
- No hardware drivers exist.
- No radio connection exists.
- No PTT pin is selected for real use.
