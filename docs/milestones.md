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

M2.1 status:

- Add embedded C headers and source layout.
- Define the platform adapter interface.
- Add safe platform stub behavior.
- Add target metadata for `stm32h753-nucleo`.
- Add host-native `make embedded-test`.
- Keep ARM cross-compilation optional and absent from normal CI.

M2.2 status:

- Expand platform interface with monotonic tick and 10 ms control tick concepts.
- Add watchdog kick and simulated watchdog fault behavior.
- Add reset-cause reporting.
- Add safe test GPIO/PTT state in the stub.
- Add diagnostic write and platform fault counters.
- Keep all behavior host-testable with no STM32 HAL or hardware register access.

M2.3 status:

- Add a host-native USB CDC byte-stream interface.
- Add a fixed-buffer USB CDC stub.
- Add embedded USB echo mode.
- Add embedded KISS data-frame loopback mode.
- Test KISS escaping and malformed input recovery.
- Keep PTT off during USB/KISS tests.
- Keep real USB stack integration, descriptors, endpoints, TinyUSB, and HAL out of the repo.

M2.4 status:

- Add embedded diagnostics snapshot.
- Add bounded diagnostics formatter.
- Capture app, platform, watchdog, reset, PTT, USB, and KISS bridge counters.
- Test malformed KISS and unsupported KISS command counters.
- Keep real USB diagnostics channel work out of the repo.

M2.5 status:

- Add embedded audio interface.
- Add fixed-buffer audio stub.
- Add host-native RX sample injection and TX sample capture.
- Add app-level audio loopback path.
- Add audio overflow and underflow counters to diagnostics.
- Keep ADC, DAC, SAI, I2S, DMA, codec, and transmitter audio paths out of the repo.

M2.6 status:

- Add embedded TNC state and status.
- Parse KISS byte streams from the USB CDC stub.
- Handle KISS data frames and basic KISS commands.
- Map Nino-compatible SETHW mode 6 and 22 through the mode registry.
- Count known unimplemented and invalid mode requests without activating them.
- Add embedded TNC counters to diagnostics.
- Keep modem audio, real USB, hardware PTT, and hardware drivers out of the repo.

M2.7 status:

- Add embedded modem boundary around portable AFSK1200 TX.
- Accept AX.25 frames with FCS from the embedded TNC test path.
- Generate simulated AFSK1200 samples into the audio stub.
- Keep modem TX disabled by default and explicitly test-enabled only.
- Abort simulated modem TX on watchdog fault and safe shutdown.
- Add modem TX counters to diagnostics.
- Keep real audio hardware, GPIO PTT, USB hardware, and RF transmit out of the repo.

Next planned M2 passes:

- M2.8: embedded RX audio/modem boundary skeleton.

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
