# AFSK1200 Modem

This document covers the M1.2 host-side 1200 baud Bell 202 AFSK simulator. It is for deterministic clean-vector tests only.

## Scope

M1.2 adds portable host-side tone generation and clean-vector decode. It does not add STM32 firmware, USB CDC, codec drivers, DMA audio, DCD, squelch/COS, timing recovery, clock drift handling, noisy decode, or real-radio testing.

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

The M1.2 decoder uses fixed 40-sample windows and simple tone detection. It is expected to decode clean internally generated vectors and mild amplitude scaling only.

## Limitations

- Clean generated vectors only.
- No timing recovery.
- No DCD.
- No squelch/COS integration.
- No noisy decode claim.
- No sample clock drift tolerance claim.
- No real-radio test claim.
- No embedded audio DMA.
- No production modem filtering.

## Sources checked

| Source title                                             | Date checked | Note                                           |
| -------------------------------------------------------- | ------------ | ---------------------------------------------- |
| APRS Protocol Reference 1.0.1, APRS Working Group        | 2026-05-26   | APRS over AX.25 context                        |
| Clarifying the Amateur Bell 202 Modem, TAPR DCC 2014     | 2026-05-26   | Amateur Bell 202/APRS modem context            |
| AX.25 Link Access Protocol v2.2, TAPR/ARRL               | 2026-05-26   | AX.25 link-layer scope separate from modem DSP |
