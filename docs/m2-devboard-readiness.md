# M2 Dev-Board Readiness

M2 is the first dev-board firmware prototype stage. It starts embedded work without starting PCB design, RF transmit, real-radio keying, or network hardware work.

## Candidate Board Criteria

Primary M2.0 target:

- `stm32h753-nucleo`, based on NUCLEO-H753ZI or a current equivalent STM32H753 Nucleo-144 board.

Primary target class:

- STM32H753 or STM32H743 class, with STM32H753 preferred for the first M2 path because the NUCLEO-H753ZI product page is currently active.

Secondary experimental target:

- RP2350, only for experiments that fit its peripheral and performance limits.

Required dev-board features:

- USB device support.
- Timers suitable for modem and control timing.
- Independent watchdog or equivalent watchdog path.
- Enough RAM for fixed buffers and diagnostics.
- Exposed I2S, SAI, DAC, ADC, or a test audio path.
- GPIO available for a test PTT pin.
- Debug access for reset, fault, and watchdog tests.

## Firmware Boundaries

The embedded firmware should keep the portable core separate from platform adapters:

- Portable core: KISS, AX.25, HDLC, FCS, AFSK1200, TNC control, diagnostics, and modes.
- Platform HAL adapter: clocks, timers, GPIO, reset, and watchdog.
- USB CDC adapter: host KISS byte stream.
- Audio adapter: simulated or loopback audio first, real codec later.
- PTT adapter: default-off GPIO test path first.
- Diagnostics adapter: counters and fault output.
- Config and persistence adapter: planned after the safety path is stable.

Host daemon code is not MCU firmware. It shares the portable core and validates behavior before firmware adapters are added.

M2.0 planned adapter stack:

```text
portable core
	|
	embedded app glue
	|
	platform adapters
	|-- clock and tick adapter
	|-- GPIO and test-only PTT adapter
	|-- watchdog and reset adapter
	|-- USB CDC byte-stream adapter
	|-- audio loopback or test adapter
	|-- diagnostics adapter
```

M2.1 adds the compile-only skeleton and host-native stub test. M2.2 adds tick, watchdog, reset-cause, diagnostics, and GPIO/PTT test stubs. M2.3 adds a host-native USB CDC byte-stream stub and KISS echo/loopback bridge. M2.4 adds a host-native embedded diagnostics bridge. M2.5 adds a host-native audio stub and sample loopback path. M2.6 adds host-native embedded TNC core integration for KISS command handling, mode mapping, control state, and diagnostics. M2.7 adds a host-native modem/audio boundary that writes simulated AFSK1200 samples into the audio stub. M2.8 adds a host-native RX audio/modem boundary that decodes generated test samples and emits KISS to the USB stub. M2.9 adds a host-native full loopback from USB KISS input through simulated audio TX/RX and back to USB KISS output. M2.10 adds a compile-gated STM32H753 target skeleton with metadata, placeholder target hooks, and opt-in target checking. M2.11 adds STM32H753 resource planning and target resource metadata with real pin assignments still unverified. M2.12 adds opt-in target-local object-check metadata and a skip-safe cross-compile wrapper. M2.13 adds USB CDC stack selection and dependency-boundary planning with the stub as the only implemented USB path. M2.14 adds USB descriptor planning metadata and MCU family/board abstraction planning for H753, H743, H735, H750, RP2350, and ESP32-S3 roles. M2.15 adds clock, reset, boot, and watchdog planning metadata with no clock setup, reset register reads, watchdog enable, startup code, or hardware access. M2.16 adds memory, flash, and CPU budget planning metadata with no linker map or final measured embedded usage claims. M2.17 adds queue and backpressure planning metadata with no ISR, DMA, scheduler, USB hardware, audio hardware, or PTT hardware implementation. Real board build, flash, USB stack, audio peripheral integration, GPIO PTT, clock setup, watchdog setup, and RF transmit remain future work.

## Build Strategy

- Keep the host build intact.
- Add the embedded build separately.
- Do not require vendor IDE tooling in core docs.
- Choose Makefile or CMake after the target board is selected.
- Keep generated build output under `build/` or target-specific ignored directories.
- Keep no-heap and fixed-buffer rules for core modem, protocol, and control code.
- Keep STM32CubeIDE and vendor-generated output outside the repo unless a later milestone explicitly changes that policy.
- Keep `make embedded-test` host-native so CI does not require an ARM toolchain.
- Keep `make embedded-target-check` opt-in, object-only, and skip-safe when `arm-none-eabi-gcc` is absent.

## Safety Gates

- PTT is off on boot.
- PTT is off on reset.
- PTT GPIO is not enabled until explicitly configured in test firmware.
- Watchdog failure forces the PTT test pin off.
- Max TX timeout forces transmit state off.
- Fault counters or diagnostics are visible during tests.
- No RF transmit path is connected during M2.

## Test Plan

M2 tests must keep M1 host checks passing and add dev-board checks:

- Host unit tests.
- Embedded compile.
- USB CDC KISS echo or loopback.
- Fault and reset behavior.
- Watchdog safe-off behavior.
- GPIO-only PTT default-off test.
- No RF transmit.

M2 must not skip sanitizer, tool, daemon, or compatibility targets without a documented reason.
