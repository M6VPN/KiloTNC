# Test Plan

No PCB order is allowed until the tests in this file are reviewed and the applicable pre-PCB tests pass on dev hardware or host simulation.

## Pre-PCB Test Gates

| Gate | Required before PCB order                         |
| ---- | ------------------------------------------------- |
| T0   | Protocol matrix reviewed                          |
| T1   | KISS parser host tests pass                       |
| T2   | AX.25 UI encode/decode and FCS tests pass         |
| T3   | HDLC bit-stuffing tests pass                      |
| T4   | 1200 baud AFSK WAV/golden-vector tests pass       |
| T5   | PTT safety tests pass on dev hardware             |
| T6   | USB disconnect and reconnect tests pass           |
| T7   | Brownout and watchdog reset behavior verified     |
| T8   | Audio path level and filter plan measured         |
| T9   | Radio connector protection plan reviewed          |

M1 host-side status checked with `make clean && make test` on 2026-05-26:

| Gate | Status |
| ---- | ------ |
| T1   | Pass   |
| T2   | Pass   |
| T3   | Pass   |

## KISS Tests

- Encode and decode FEND, FESC, TFEND, and TFESC.
- Decode back-to-back FEND without creating empty frames.
- Recover after invalid escape sequence.
- Ignore unsupported command.
- Treat Return command as no-op.
- Drop overlength frame and increment counter.
- Drop packet on full queue and increment counter.
- Fuzz malformed byte streams without crash.

M1 implemented tests:

- Plain payload encode/decode.
- Escaped FEND and FESC encode/decode.
- Repeated FEND handling.
- Invalid escape recovery.
- Overlength frame drop.
- Unsupported command ignore.
- SetHardware bounded payload.
- TXDELAY, P, SlotTime, TXtail, FullDuplex, and Return command parsing.

M1.1 edge tests:

- Empty input and single or repeated FEND input.
- Command-only data frame.
- Maximum and over-maximum payloads.
- Incomplete and invalid escape handling.
- Unsupported command payload ignore.
- Port 15 data frame handling.
- SetHardware exact-max and over-max payload handling.
- Encode output buffer too small.
- NULL public argument handling.

## AX.25 and HDLC Tests

- Encode destination and source callsign/SSID fields.
- Decode UI frame control and PID fields.
- Reject invalid address lengths.
- Compute and verify 16-bit FCS.
- Reject frames with bad FCS.
- Apply HDLC bit-stuffing after five contiguous one bits.
- Remove stuffed bits on receive.
- Keep AX.25 byte handling separate from modem sample handling.

M1 implemented tests:

- FCS known vector.
- FCS append and validate round trip.
- FCS modified-frame rejection.
- HDLC stuffing after five contiguous one bits.
- HDLC unstuffing round trip.
- HDLC output buffer overflow rejection.
- HDLC malformed six-one sequence rejection.
- AX.25 UI encode/decode.
- AX.25 invalid callsign rejection.
- AX.25 invalid SSID rejection.
- AX.25 corrupted FCS rejection.
- AX.25 address extension bit handling.

M1.1 edge tests:

- FCS empty buffer, known ASCII vector, separate append, in-place append, short output, NULL arguments, and short validate.
- HDLC empty input, no-stuff input, multiple stuffing events, exact and short output buffers, malformed runs, and NULL arguments.
- AX.25 shortest and longest callsigns, SSID 0 and 15, empty and maximum information fields, oversize information rejection, malformed address chains, output buffer too small, and NULL arguments.

## Fuzz Tests

M1.1 deterministic fuzz tests:

- KISS parser random byte streams.
- HDLC unstuffing random byte streams.
- AX.25 decode random byte streams.
- FCS validate random byte streams.

The fuzz harness uses a fixed seed, bounded iterations, fixed-size buffers, and no heap allocation.

## AFSK1200 Host Vector Tests

M1.2 host-side tests checked with `make clean && make test` and `make sanitize` on 2026-05-26:

- AFSK1200 constants for 48 kHz, 1200 baud, 40 samples per bit, 1200 Hz mark, and 2200 Hz space.
- Mark-only and space-only tone generation.
- Encoded sample count.
- Output buffer-too-small handling.
- NULL public argument handling.
- NRZI known short pattern.
- NRZI decode round trip.
- Clean AFSK encode/decode round trip for a short bit pattern.
- Clean generated AX.25 UI frame through FCS, HDLC bit-stuffing, NRZI, AFSK PCM, decode, NRZI decode, HDLC unstuff, and AX.25 FCS validation.
- Mild amplitude-scaled generated audio decode.
- Truncated sample count rejection.
- PCM int16_t range check.

M1.3 host-side tests checked with `make clean && make test` and `make sanitize` on 2026-05-26:

- DCD score low for silence.
- DCD score low for deterministic random noise.
- DCD score higher for valid generated AFSK1200.
- Decode search API output buffer-too-small handling.
- Leading and trailing silence around generated AFSK1200 audio.
- Small bit-window phase offset through leading silence.
- Mild amplitude reduction.
- Mild amplitude increase without overflow.
- DC offset.
- Mild deterministic additive noise.
- Mild clipping.
- Decoder metrics for bit counts, tone counts, detector energy, confidence, and diagnostic DCD score.
- Sample count not divisible by 40 rejected safely.
- NULL argument handling for search and DCD APIs.

Pending:

- Full timing recovery loop.
- Production DCD.
- Squelch/COS integration.
- Real noisy decode.
- Real sample clock drift handling.
- Real-radio testing.
- Embedded audio DMA.

## Modem Tests

1200 baud Bell 202 AFSK:

- Generate known WAV vectors.
- Decode known WAV vectors.
- Verify timing recovery across low, nominal, and high sample clock offsets.
- Verify DCD behavior with signal, silence, and noise.
- Verify p-persistence behavior with busy and clear channel inputs.

300 baud AFSK:

- Research-only until tones, filtering, and NinoTNC interop targets are reviewed.

9600 baud G3RUH-like/GFSK:

- Research-only until scrambling, filtering, deviation, and data-radio interface requirements are reviewed.

IL2P/IL2Pc:

- Research-only until published vector sources or locally generated interop vectors are approved.
- Test Type 0 transparent encapsulation.
- Test Type 1 translated header limits.
- Test CRC behavior for IL2Pc separately from Reed-Solomon decode success.

## Hardware Bench Tests

- Verify TX audio level range into dummy load.
- Verify RX audio gain range using signal generator.
- Verify anti-alias filter response.
- Verify TX reconstruction filter response.
- Verify PTT open-drain or opto output default-off state.
- Verify COS/DCD input protection and threshold behavior.
- Verify USB ESD/protection footprint choices against datasheets.
- Verify no PTT assertion during bootloader, reset, firmware crash, or watchdog reset.

## Fault Tests

- Force audio overrun.
- Force audio underrun.
- Fill TX queue.
- Fill RX queue.
- Send malformed KISS streams for at least 1 hour.
- Disconnect USB during TX.
- Reset MCU during TX.
- Trigger watchdog during TX.
- Sweep supply through brownout threshold.
- Confirm fault counters persist where required.

## Bridge Tests

VARA:

- Verify host software can use KiloTNC only as audio/PTT/CAT bridge.
- Verify no native VARA claim is made in firmware or docs.

ARDOP:

- Verify host software bridge over audio/PTT/CAT where supported.
- Verify PTT timing expectations against ARDOP behavior.

Mercury:

- Verify TCP TNC compatibility expectations on host side.
- Verify KiloTNC bridge does not claim Mercury on-air compatibility unless tested with Mercury peers.

## Burn-In Tests

- 72-hour USB connected idle test.
- 72-hour receive audio test.
- 24-hour repeated TX key/unkey test into dummy load.
- Repeated USB reconnect test.
- Repeated brownout/reset test.
- Malformed KISS fuzz test with diagnostics active.

## Sources checked

| Source title                               | Date checked | Note                                                |
| ------------------------------------------ | ------------ | --------------------------------------------------- |
| The KISS TNC, Chepponis and Karn           | 2026-05-26   | KISS parser, command, and buffer behavior           |
| AX.25 Link Access Protocol v2.2, TAPR/ARRL | 2026-05-26   | AX.25 fields, HDLC bit-stuffing, and FCS scope      |
| IL2P Specification Draft v0.6              | 2026-05-26   | IL2P/IL2Pc research test boundaries                 |
| ARDOP Specification Revision 0.3.1         | 2026-05-26   | Bridge, host interface, PTT/radio control context   |
| Mercury GitHub README, Rhizomatica         | 2026-05-26   | TCP TNC and external modem bridge context           |
| VARA Modem official site                   | 2026-05-26   | VARA host software modem scope                      |
