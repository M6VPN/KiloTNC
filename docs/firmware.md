# Firmware

This file defines firmware module boundaries and real-time safety rules. No firmware logic exists in this pass.

## Firmware Goals

- No dynamic allocation in the real-time audio path.
- Bounded interrupt service routines.
- Fixed-size queues and memory pools.
- Parser recovery from malformed host input.
- Fail-safe PTT behavior.
- Watchdog coverage across main, USB, audio, and PTT safety tasks.
- Versioned persistent settings with CRC and rollback.

## Module Layout

| Module       | Responsibility                                      |
| ------------ | --------------------------------------------------- |
| `app`        | Main loop, task scheduling, watchdog quorum         |
| `usb`        | USB device, CDC ACM KISS, diagnostic CDC            |
| `kiss`       | KISS frame parser, escaping, command dispatch       |
| `packet`     | Packet queues, AX.25, future IL2P/IL2Pc boundary   |
| `hdlc`       | HDLC flags, bit-stuffing, FCS boundary             |
| `modem`      | AFSK and future G3RUH-like/GFSK DSP                |
| `audio`      | Codec driver, DMA rings, sample clock checks        |
| `radio`      | PTT, COS/DCD, CAT bridge hooks                      |
| `config`     | Settings storage, schema version, CRC, rollback     |
| `diag`       | Counters, diagnostic CLI, fault records             |
| `led`        | LED status codes                                    |
| `platform`   | MCU HAL boundary and board definitions              |
| `test`       | Host and target test harness support                |

## Real-Time Rules

- Audio DMA uses ring buffers.
- Audio callbacks copy or mark buffers only.
- ISRs do bounded work and defer parsing/DSP work to tasks.
- No heap allocation in audio, modem, or ISR code.
- Packet buffers come from fixed pools.
- Queue push failure drops the new packet and increments a drop counter.
- External input is length-checked and range-checked before use.
- Command values outside supported ranges are rejected or clamped only where the protocol allows it.

## KISS Parser Rules

- Parser accepts byte streams and reconstructs FEND-delimited frames.
- FESC TFEND decodes to FEND.
- FESC TFESC decodes to FESC.
- Invalid escape sequences increment `kiss_parse_errors`.
- Overlength frames are dropped and counted.
- Unsupported commands are ignored.
- Return command is a no-op in v0.1.
- Parser returns to a known state after malformed input.

## Packet Rules

- AX.25 UI frames are first.
- AX.25 addressing, control, PID, information, FCS, and HDLC handling stay separate from modem DSP.
- FCS failure increments `crc_errors` and drops the frame.
- IL2P/IL2Pc stays behind a separate interface until interop tests exist.

## PTT Safety Rules

PTT must be off:

- On boot.
- On reset.
- On watchdog fault.
- On brownout reset.
- On USB disconnect unless configured otherwise.
- On maximum TX watchdog timeout.
- On PTT safety task failure.

The PTT safety task owns the final permission to transmit. Modem code can request TX, but safety code must approve it.

## Watchdog Rules

The watchdog heartbeat requires recent progress from:

- Main loop.
- USB task.
- Audio task.
- PTT safety task.

If any required task stops reporting progress, the watchdog must not be refreshed.

## Persistent Settings

Settings storage must include:

- Schema version.
- Length field.
- CRC.
- Active and backup banks, or equivalent rollback.
- Factory defaults if both banks fail validation.

Flash writes must be rate-limited. KISS SetHardware commands must not cause repeated flash writes during normal startup scripts unless explicitly requested.

## Diagnostics

Required counters:

- RX frames.
- TX frames.
- CRC errors.
- Audio overruns.
- Audio underruns.
- KISS parse errors.
- Queue drops.
- Watchdog resets.
- Brownout resets.

Required interfaces:

- Diagnostic CDC CLI.
- LED status for power, USB, RX, TX, DCD, and fault.
- Fault record readable after reboot.

## Sources checked

| Source title                               | Date checked | Note                                                |
| ------------------------------------------ | ------------ | --------------------------------------------------- |
| The KISS TNC, Chepponis and Karn           | 2026-05-26   | Parser recovery, unsupported commands, drop policy  |
| AX.25 Link Access Protocol v2.2, TAPR/ARRL | 2026-05-26   | AX.25/HDLC/FCS boundaries                           |
| IL2P Specification Draft v0.6              | 2026-05-26   | IL2P/AX.25 translation and transparent encapsulation |
| STM32H743BG product page, STMicroelectronics | 2026-05-26 | DMA, timers, USB, watchdog-capable MCU context      |

