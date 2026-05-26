# M2 Dev-Board Readiness

M2 is the first dev-board firmware prototype stage. It starts embedded work without starting PCB design, RF transmit, real-radio keying, or network hardware work.

## Candidate Board Criteria

Primary target class:

- STM32H743 or STM32H753 class.

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

## Build Strategy

- Keep the host build intact.
- Add the embedded build separately.
- Do not require vendor IDE tooling in core docs.
- Choose Makefile or CMake after the target board is selected.
- Keep generated build output under `build/` or target-specific ignored directories.
- Keep no-heap and fixed-buffer rules for core modem, protocol, and control code.

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
