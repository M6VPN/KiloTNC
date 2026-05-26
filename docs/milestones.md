# Milestones

This document keeps detailed project milestones out of the README while preserving the current M1 host-side work record.

## Completed M1 Work

M1 is complete enough to start M2. It remains the portable host-side core, test, tool, and daemon groundwork stage.

Protocol core:

- KISS framing, escaping, command parsing, parser recovery, and fuzz tests.
- AX.25 UI frame encode/decode.
- HDLC bit-stuffing and unstuffing.
- AX.25 FCS append and validation.

AFSK1200 modem simulator:

- 48 kHz Bell 202 AFSK1200 constants and generated tones.
- Clean host vectors and deterministic impairments.
- Continuous acquisition.
- Streaming RX.
- Streaming TX.
- Whole-buffer and streaming loopback tests.

TNC harness:

- KISS data input to AFSK1200 TX PCM.
- AFSK1200 RX PCM to KISS output.
- Channel access and p-persistence simulation.
- DCD gating and PTT simulation.
- Diagnostics snapshots and fault counters.
- Mode registry and Nino-compatible SETHW mapping.

CLI and vectors:

- Generated KISS test frames.
- Raw signed 16-bit little-endian PCM.
- WAV export.
- CLI loopback.
- Deterministic vector generation under `build/`.

`kilotncd` daemon groundwork:

- File and stdin/stdout once-mode adapters.
- Localhost TCP once-mode adapter.
- Unix socket once-mode adapter.
- PTY once-mode adapter.
- Raw PCM audio backend abstraction.
- ALSA, sndio, and OSS compile-gated stubs.
- Radio-control abstraction with none, simulated, and log backends.
- Config profiles and validation.
- One-shot control/status commands.
- Foreground loop skeleton.

Interoperability planning:

- Internal KISS compatibility tests across local daemon transports.
- External black-box interoperability plan.
- Optional skip-safe wrapper placeholders under `interop/`.

## M2 Dev-Board Firmware Prototype

M2 starts firmware prototype work on a development board. It must keep the M1 portable core and host tests intact.

M2.0 status:

- Select `stm32h753-nucleo` as the primary dev-board path.
- Keep RP2350/Pico 2 as a secondary experimental target only.
- Add embedded workspace documentation under `embedded/`.
- Document the embedded build strategy.
- Document M2 safety gates before any PTT or RF path.

Next planned M2 passes:

- M2.1: embedded compile-only skeleton.
- M2.2: platform tick, watchdog, and GPIO test stubs.
- M2.3: USB CDC KISS echo or loopback.
- M2.4: diagnostics bridge.
- M2.5: audio loopback or test path.

M2 scope:

- Select the initial dev-board target.
- Create a platform boundary around the portable core.
- Add an embedded build skeleton.
- Bring up USB CDC KISS loopback.
- Bring up timer, watchdog, and fault-counter basics.
- Keep audio simulated or loopback-only at first.
- Use GPIO test pins only for PTT safety checks.

M2 does not include:

- PCB design.
- RF transmit tests.
- Real radio keying.
- Codec driver completion.
- DMA audio integration.
- Network services.

## M2 Exit Criteria

- Firmware builds separately from the host daemon.
- Host tests still pass.
- USB CDC KISS echo or loopback works on the dev board.
- Watchdog reset behavior is tested.
- PTT default-off behavior is proven on a GPIO test pin only.
- Max TX timeout behavior is represented in firmware control state.
- Diagnostics or fault counters are visible through a test path.
- No unintended TX path exists.

## M3 Hardware Audio and PTT Bench Validation

M3 moves from loopback firmware tests to controlled bench validation.

M3 scope:

- External codec evaluation board or dev-board audio path.
- Dummy audio loads.
- Logic analyzer and oscilloscope checks.
- GPIO-only PTT safety validation.
- Audio level and clipping checks.

M3 excludes RF output until the safety path and dummy-load bench checks pass.

## M4 Rev A Schematic and PCB

M4 starts Rev A hardware design after M2 and M3 gates are reviewed.

M4 scope:

- KiCad schematic.
- Rev A USB, audio, PTT, and radio connector path.
- Connector protection and grounding review.
- PCB layout only after architecture, firmware safety, and test plan review.

M4 excludes Ethernet and Wi-Fi as Rev A requirements.

## Later Tracks

- M5: Rev A bring-up and burn-in.
- M6: Additional modem modes and NinoTNC compatibility expansion.
- M7: Linux/BSD daemon real audio backends.
- M8: Network and node services.
- M9: Future Ethernet or Wi-Fi hardware variants.
