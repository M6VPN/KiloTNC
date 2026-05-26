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
- Opt-in target skeleton check through `make embedded-target-check`.
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

M2.12 status:

- Target-local source metadata is in `target_sources.mk`.
- Target-local object-check metadata is in `target_build.mk`.
- The opt-in check wrapper is `check_target_compile.sh`.
- `make embedded-target-check` runs the wrapper.
- `ARM_NONE_EABI_CC=/path/to/arm-none-eabi-gcc make embedded-target-check` selects a specific compiler.
- If `arm-none-eabi-gcc` is absent, the check prints `skip: arm-none-eabi-gcc not found` and exits success.
- If the compiler is present, only `target_main.c` and `target_platform.c` are compiled to temporary object files under `build/embedded-target/`.
- No link step runs.
- No ELF, BIN, HEX, linker script, startup vector table, HAL, CMSIS, TinyUSB, vendor SDK, flash command, or flashable firmware image is produced.

M2.13 status:

- USB CDC stack planning is documented in [docs/m2-usb-stack-plan.md](../../../docs/m2-usb-stack-plan.md).
- TinyUSB is the preferred future first adapter path.
- STM32Cube USB Device is the fallback and reference path.
- The current implemented USB path remains the host-native USB CDC stub.
- `tinyusb`, `stm32cube`, and `custom` are recognized by the boundary but unsupported.
- No real USB stack, descriptors, endpoints, interrupts, hardware access, HAL, CMSIS, TinyUSB import, STM32Cube import, vendor SDK, or generated project is present.

M2.14 status:

- USB descriptor planning is represented by `usb_descriptor_plan`.
- Planned descriptor profiles are `kiss-only` and `kiss-plus-diag`.
- No final VID or PID is claimed.
- No binary descriptors are bound to a real USB stack.
- STM32H753 remains the flagship target for this path.
- STM32H743 remains a possible first custom serious board path.
- STM32H735 remains a cost-reduced candidate only after resource validation.
- STM32H750 remains a candidate only after memory and flash validation.
- RP2350 is a separate low-cost target path, not a drop-in board variant.
- ESP32-S3 is a possible connectivity companion only, not the modem DSP core.
- Board abstraction planning is documented in [docs/m2-board-abstraction-plan.md](../../../docs/m2-board-abstraction-plan.md).
- MCU family planning is documented in [docs/m2-mcu-family-plan.md](../../../docs/m2-mcu-family-plan.md).
