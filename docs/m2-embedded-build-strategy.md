# M2 Embedded Build Strategy

M2 embedded work must stay separate from the host-side M1 build, daemon, tools, and tests.

## Rules For M2.0

- No vendor IDE requirement.
- No STM32Cube, CMSIS, Pico SDK, TinyUSB, or generated IDE project vendored into the repo.
- No embedded toolchain required for `make test`.
- No cross-compiler required in CI yet.
- No generated embedded build output committed.
- Host build remains the default project build.
- Embedded build is opt-in after the first compile-only skeleton exists.

STM32CubeIDE is available locally and may be used for inspection or later generated-project experiments, but M2.0 does not commit IDE output.

## M2.1 Host-Native Skeleton Test

M2.1 adds a host-native embedded skeleton test:

```text
make embedded-test
```

This target compiles the embedded app skeleton, platform stub, target metadata, and embedded test under the host compiler. It does not use `arm-none-eabi-gcc`, STM32 HAL, CMSIS, TinyUSB, STM32Cube, Pico SDK, or generated vendor projects.

The target verifies:

- Embedded app init works.
- Init forces the stub PTT state off.
- Step advances stub time.
- Step kicks the watchdog counter.
- Fault and shutdown force PTT off.
- Target metadata is visible.

## M2.2 Platform Stub Coverage

`make embedded-test` now also verifies:

- Monotonic millisecond tick state.
- 10 ms control tick state.
- Watchdog kick counter.
- Simulated watchdog fault.
- Reset-cause reporting.
- Test GPIO/PTT default-off and safe-off behavior.
- Diagnostic write counter.
- Platform fault counter.

## M2.3 USB CDC Skeleton Coverage

`make embedded-test` now also verifies:

- USB CDC stub connection state.
- Bounded RX and TX byte-stream buffers.
- RX and TX overflow rejection.
- USB echo mode.
- KISS data-frame loopback.
- KISS FEND and FESC escaping.
- Malformed KISS recovery counters.
- Unsupported KISS command ignore counters.
- App step services the USB bridge while keeping PTT off.

This remains host-native. It does not use TinyUSB, STM32 HAL, CMSIS, STM32Cube, descriptors, endpoints, hardware registers, or a USB device controller.

## M2.4 Embedded Diagnostics Coverage

`make embedded-test` now also verifies:

- Diagnostic snapshot capture from a fresh app.
- App step, platform tick, watchdog, and diagnostic write counters.
- USB RX/TX byte counters.
- USB RX/TX overflow counters.
- KISS frame, parse error, ignored command, and overlength counters.
- Watchdog fault state, reset cause, and PTT-off state.
- Bounded text formatting and truncation rejection.

This remains host-native. It does not create a USB diagnostic channel or call a real USB device stack.

## M2.5 Audio Loopback Coverage

`make embedded-test` now also verifies:

- 48 kHz mono signed 16-bit audio format metadata.
- Fixed-buffer RX sample injection.
- Fixed-buffer TX sample capture.
- Audio loopback from RX samples to TX samples.
- Partial-block loopback.
- RX underflow counters.
- TX overflow counters.
- Audio counters in embedded diagnostics.
- App step services audio loopback while kicking the watchdog.
- PTT remains off during audio loopback.

This remains host-native. It does not use ADC, DAC, SAI, I2S, DMA, codec drivers, hardware registers, or a transmitter audio path.

## M2.6 Embedded TNC Coverage

`make embedded-test` now also verifies:

- Embedded TNC init and default 1200 AFSK AX.25 mode.
- KISS data-frame parsing from the USB CDC stub.
- Optional KISS data-frame loopback to the USB CDC stub.
- KISS FEND and FESC escaping through loopback.
- Repeated FEND handling.
- Malformed KISS recovery counters.
- Unsupported KISS command counters.
- TXDELAY, P, SlotTime, TXtail, and FullDuplex command state.
- SETHW mode 6 and 22 through the shared mode registry.
- Known but unimplemented modes remain inactive and counted.
- Invalid mode requests remain inactive and counted.
- Embedded TNC counters in embedded diagnostics.
- App step services the embedded TNC while kicking the watchdog.
- PTT remains off during embedded TNC tests.

This remains host-native. It does not use a real USB stack, modem audio,
GPIO PTT, hardware registers, or hardware drivers.

## M2.7 Embedded Modem Boundary Coverage

`make embedded-test` now also verifies:

- Embedded modem init, start, process, and abort paths.
- Valid 1200 AFSK AX.25 frame acceptance.
- Unsupported mode rejection.
- Malformed frame rejection.
- Busy TX rejection.
- Simulated AFSK1200 sample generation into the audio stub.
- Small bounded TX chunk processing.
- TNC-to-modem request handling when explicitly enabled.
- Modem TX remains disabled by default.
- Unsupported mode requests block simulated modem TX.
- App step services modem TX while kicking the watchdog.
- Watchdog fault and safe shutdown abort active modem TX.
- PTT remains off during modem sample generation.
- Modem counters in embedded diagnostics.

This remains host-native. It does not use real USB, ADC, DAC, SAI, I2S,
DMA, codec drivers, GPIO PTT, hardware registers, or transmitter audio.

## M2.8 Embedded RX Modem Boundary Coverage

`make embedded-test` now also verifies:

- Embedded modem RX init and reset paths.
- Empty audio RX underflow handling.
- Generated AFSK1200 PCM decode through portable streaming RX.
- AX.25 destination, source, and info verification after decode.
- Small bounded RX chunk processing.
- Noise input does not produce bogus frames.
- RX output frame array too small is handled safely.
- Modem RX is disabled by default.
- Explicitly enabled modem RX emits KISS data frames to the USB stub.
- USB TX overflow while emitting RX frames is counted and safe.
- App step services RX while kicking the watchdog.
- Watchdog fault blocks RX processing and leaves PTT off.
- Modem RX counters in embedded diagnostics.

This remains host-native. It does not use real RF receive, real USB, ADC,
DAC, SAI, I2S, DMA, codec drivers, GPIO PTT, or hardware registers.

## M2.9 Full Host-Test Loopback Coverage

`make embedded-test` now also verifies:

- USB CDC KISS input through the embedded TNC.
- Simulated AFSK1200 TX sample generation into the audio stub.
- Bounded copy from audio TX samples into audio RX samples.
- AFSK1200 RX decode from the audio stub back into AX.25 frames.
- KISS data-frame output through the USB CDC stub.
- AX.25 destination, source, and info equality across the full path.
- KISS FEND and FESC escaping across the full path.
- SETHW mode 6 and 22 before data frames.
- Unsupported command handling before a data frame.
- Unsupported mode blocking without unsafe activation.
- Max iteration timeout behavior.
- Watchdog-fault abort behavior.
- Full-loopback counters and diagnostics.
- PTT remains off throughout the loopback.

This remains host-native. It does not use real USB, ADC, DAC, SAI, I2S,
DMA, codec drivers, GPIO PTT, RF receive, RF transmit, or hardware registers.

## M2.10 STM32H753 Target Skeleton

M2.10 adds a compile-gated target skeleton under:

```text
embedded/targets/stm32h753-nucleo/
```

Normal host CI still uses:

```text
make embedded-test
```

The opt-in target guidance and target check are:

```text
make embedded-target-help
make embedded-target-check
```

`make embedded-target-check` exits success with a skip message when `arm-none-eabi-gcc` is absent. If the compiler is available, it checks only safe target skeleton files with `KILOTNC_TARGET_STM32H753_NUCLEO` defined.

The target skeleton does not require STM32Cube, CMSIS, STM32 HAL, TinyUSB, startup code, linker scripts, hardware registers, pin assignments, or a vendor IDE project. It does not link firmware and does not produce a flashable image.

## M2.11 STM32H753 Resource Planning

M2.11 adds planning metadata and documentation only:

- `docs/m2-stm32h753-resource-plan.md`.
- `embedded/targets/stm32h753-nucleo/target_resources.h`.
- Target metadata tests for resource flags and unverified pin assignments.

`make embedded-test` remains host-native and does not require an ARM toolchain. No HAL, CMSIS, TinyUSB, vendor project, pin initialization, alternate-function setup, hardware register access, real USB, real GPIO, real audio, real PTT, linker script, startup code, or flashable firmware is added.

## M2.12 Opt-In STM32H753 Target Check

M2.12 moves target check metadata into:

```text
embedded/targets/stm32h753-nucleo/target_sources.mk
embedded/targets/stm32h753-nucleo/target_build.mk
embedded/targets/stm32h753-nucleo/check_target_compile.sh
```

`target_sources.mk` lists only safe target skeleton sources and include paths. It defines the `stm32h753-nucleo` target name and compile gate macro name.

`target_build.mk` builds temporary target skeleton object files under `build/embedded-target/`. It has no linker script, startup object, vendor SDK path, flash image, or link rule.

`check_target_compile.sh` uses `${ARM_NONE_EABI_CC:-arm-none-eabi-gcc}`. If the compiler is absent, it prints `skip: arm-none-eabi-gcc not found` and exits success. If the compiler is present, it object-compiles the target skeleton sources only.

No ELF, BIN, HEX, startup vector table, linker script, STM32Cube, CMSIS, STM32 HAL, TinyUSB, vendor project, hardware register access, pin initialization, or flash programming is added.

## M2.13 USB Dependency Boundary

M2.13 chooses the future USB CDC stack direction without importing or building a real USB stack.

- TinyUSB is the preferred future first adapter path.
- STM32Cube USB Device middleware remains a fallback and reference path.
- Custom USB device stack work remains research-only.
- The implemented path remains the host-native USB CDC stub.
- `usb_stack_boundary` recognizes `stub`, `tinyusb`, `stm32cube`, and `custom`.
- Only `stub` reports implemented support.
- No TinyUSB, STM32Cube, CMSIS, HAL, descriptors, endpoint handlers, interrupt handlers, hardware registers, or vendor SDK paths are added.

Future USB dependency environment variables are reserved only:

```text
TINYUSB_PATH=/path/to/tinyusb
STM32CUBE_H7_PATH=/path/to/STM32CubeH7
KILOTNC_USB_STACK=tinyusb
```

## M2.14 USB Descriptor And Board Metadata

`make embedded-test` now also verifies:

- KISS-only USB descriptor planning metadata.
- KISS-plus-diagnostics USB descriptor planning metadata.
- No final VID or PID is claimed.
- Descriptor plan formatting rejects too-small buffers.
- STM32H753 remains marked as the flagship target.
- H743 remains a possible related STM32H7 path.
- H735 and H750 remain validation-gated future paths.
- No connectivity companion is present on the target.

The descriptor planning module does not emit binary USB descriptors, include TinyUSB or STM32Cube headers, define endpoints for hardware, or bind descriptor data to a real stack.

Board and MCU family planning is documented in:

```text
docs/m2-mcu-family-plan.md
docs/m2-board-abstraction-plan.md
```

## M2.15 Clock, Reset, And Watchdog Metadata

`make embedded-test` now also verifies:

- Control tick metadata is 10 ms.
- Platform timebase metadata is 1 ms.
- Audio sample-rate metadata is 48000 Hz.
- Clock tree finalized flag remains false.
- PLL, USB clock, audio clock, timer clock, and HSE finalized flags remain false.
- Reset-cause finalized flag remains false.
- Watchdog finalized flag remains false.
- Hardware watchdog enable at boot remains false.

The metadata lives in:

```text
embedded/targets/stm32h753-nucleo/target_clock.h
embedded/targets/stm32h753-nucleo/target_reset.h
embedded/targets/stm32h753-nucleo/target_watchdog_config.h
```

These headers do not include STM32 HAL, CMSIS, vendor headers, register values, startup code, linker data, or hardware addresses.

## M2.16 Budget Metadata

`make embedded-test` now also verifies:

- Budget finalized flag remains false.
- Linker map available flag remains false.
- Stack high-water measured flag remains false.
- CPU cycles measured flag remains false.
- Audio sample-rate metadata remains 48000 Hz.
- Control tick metadata remains 10 ms.
- Key buffer sizes are nonzero and bounded.

The budget metadata is in:

```text
embedded/include/kilotnc_budget.h
```

Budget planning is documented in:

```text
docs/m2-memory-flash-cpu-budget.md
```

Future target builds must add linker-map review, stack high-water checks, queue high-water checks, and CPU cycle measurements before H735 or H750 can be accepted. Host binary size is not an MCU flash estimate.

## M2.17 Queue Policy Metadata

`make embedded-test` now also verifies:

- Queue policy finalized flag remains false.
- Every planned queue has a nonzero name.
- Every planned queue has a nonzero capacity.
- Every planned queue has an explicit overflow policy.
- Control/PTT event queue is safety critical.
- Control/PTT event queue uses a safety-fault policy instead of silent drops.
- Queue policy formatting rejects too-small buffers.

The queue policy metadata is in:

```text
embedded/include/kilotnc_queue_policy.h
```

Queue and backpressure planning is documented in:

```text
docs/m2-queue-backpressure-plan.md
```

Future target builds must keep ISR, DMA, USB, audio, diagnostics, and control queues bounded and counted. M2.17 does not add runtime queues, interrupt handlers, DMA ownership, RTOS tasks, or scheduler behavior.

## Future Build Approaches

Candidate approaches for M2.1 and later:

- CMake toolchain file for the selected STM32 target.
- Makefile wrapper for the selected embedded target.
- External vendor SDK path supplied by environment variable.
- Compile-only embedded skeleton that links portable core objects without enabling USB, audio, GPIO, or PTT drivers.

## Future Environment Variables

These names are reserved for later embedded work. They are not required for normal host builds.

```text
KILOTNC_EMBEDDED_TARGET=stm32h753-nucleo
STM32CUBE_PATH=/path/to/STM32Cube...
ARM_NONE_EABI_CC=arm-none-eabi-gcc
TINYUSB_PATH=/path/to/tinyusb
STM32CUBE_H7_PATH=/path/to/STM32CubeH7
KILOTNC_USB_STACK=tinyusb
```

## Build Separation

Host targets remain:

- `make test`
- `make sanitize`
- `make tools`
- `make tool-test`
- `make daemon`
- `make daemon-test`
- `make kiss-compat-test`

Future embedded cross-compile targets should be explicit and opt-in. They must not break host CI when the embedded toolchain is absent.

## Repository Policy

Allowed in M2.0:

- Documentation.
- Embedded directory skeleton.
- Build-help text.

Not allowed in M2.0:

- STM32 HAL driver code.
- USB CDC implementation.
- Codec or DMA audio driver code.
- GPIO PTT driver code.
- Generated STM32CubeIDE projects.
- Pico SDK projects.
- Vendor source trees.
