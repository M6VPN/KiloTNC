# AFSK1200 Modem

This document covers the host-side 1200 baud Bell 202 AFSK simulator. It is for deterministic generated-vector tests only.

## Scope

M1.2 added portable host-side tone generation and clean-vector decode. M1.3 adds synthetic impairment tests and simple decoder metrics. This does not add STM32 firmware, USB CDC, codec drivers, DMA audio, squelch/COS, full timing recovery, real DCD, or real-radio testing.

## Verified Parameters

| Item             | Value              |
| ---------------- | ------------------ |
| Modem family     | Bell 202 AFSK      |
| Baud rate        | 1200 baud          |
| Mark tone        | 1200 Hz            |
| Space tone       | 2200 Hz            |
| Host sample rate | 48,000 Hz          |
| Samples per bit  | 40                 |
| PCM format       | Signed 16-bit mono |

The host sample rate is 48 kHz because it is common for audio devices and gives exactly 40 samples per 1200 baud bit period. That makes deterministic host vectors simple before real timing recovery exists.

## TX Pipeline

```text
AX.25 UI frame
	|
	FCS append
	|
	HDLC bit-stuffing
	|
	NRZI line coding
	|
	1200/2200 Hz AFSK tone generation
	|
	signed 16-bit mono PCM samples
```

AX.25 encode/decode remains separate from modem audio. The AFSK module accepts unpacked bits and PCM buffers. It does not know about callsigns, SSIDs, FCS, KISS, or radio control.

## RX Pipeline

```text
signed 16-bit mono PCM samples
	|
	per-bit tone decision
	|
	NRZI decode
	|
	HDLC unstuff
	|
	FCS check
	|
	AX.25 UI frame
```

The M1.3 decoder uses fixed 40-sample windows, leading/trailing silence trimming, per-window DC removal, simple tone detection, and best-offset selection across one bit period. It is expected to decode deterministic generated vectors with mild synthetic impairments.

## Metrics

The metrics API reports:

- Total decoded bits.
- Mark and space bit counts.
- Total mark and space detector energy.
- Total, minimum, and average tone-decision confidence.
- DCD score, currently equal to average confidence.

These metrics are diagnostic only. They do not yet represent production DCD behavior.

## Limitations

- Generated host vectors only.
- Best-offset selection only, not a full timing recovery loop.
- Diagnostic DCD score only, not production DCD.
- No squelch/COS integration.
- Mild synthetic noise only.
- No real sample clock drift tolerance claim.
- No real-radio test claim.
- No embedded audio DMA.
- No production modem filtering.

## Sources checked

| Source title                                             | Date checked | Note                                           |
| -------------------------------------------------------- | ------------ | ---------------------------------------------- |
| APRS Protocol Reference 1.0.1, APRS Working Group        | 2026-05-26   | APRS over AX.25 context                        |
| Clarifying the Amateur Bell 202 Modem, TAPR DCC 2014     | 2026-05-26   | Amateur Bell 202/APRS modem context            |
| AX.25 Link Access Protocol v2.2, TAPR/ARRL               | 2026-05-26   | AX.25 link-layer scope separate from modem DSP |
