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

## M1.3 Decoder Realism

M1.3 adds a bounded phase-search decode API:

```text
afsk1200_decode_pcm_search()
```

The search tries sample offsets across one 40-sample bit period, rejects non-silent partial tails, scores candidates by tone-decision confidence, and returns the best decoded bit stream. The existing `afsk1200_decode_pcm()` and `afsk1200_decode_pcm_metrics()` APIs remain available.

Synthetic tests now cover:

- Silence DCD score.
- Random-noise DCD score.
- Valid generated AFSK1200 DCD score.
- Decoder metrics on clean and impaired generated vectors.
- Amplitude reduction and increase.
- DC offset.
- Mild clipping.
- Mild deterministic additive noise.
- Leading and trailing silence.
- Small phase offsets through leading silence.
- Truncated sample count rejection.

## Metrics

The metrics API reports:

- Total decoded bits.
- Mark and space bit counts.
- Total mark and space detector energy.
- Total, minimum, and average tone-decision confidence.
- DCD score, currently based on average confidence.

These metrics are diagnostic only. They do not yet represent production DCD behavior.

## DCD Score

`afsk1200_dcd_score()` returns a bounded host-side score from PCM input. It is designed to score silence and deterministic random noise lower than valid generated Bell 202 AFSK. It is not a final squelch, carrier detect, or radio DCD design.

## M1.4 Continuous Frame Acquisition

M1.4 adds a bounded whole-buffer frame acquisition API:

```text
afsk1200_rx_decode_frames()
```

The M1.4 receive pipeline is:

```text
continuous PCM stream
	|
	AFSK1200 tone decode and metrics
	|
	NRZI decode
	|
	HDLC flag search and frame extraction
	|
	HDLC unstuff
	|
	AX.25 FCS validation
	|
	raw AX.25 UI frame bytes with FCS
```

HDLC flags are detected in the decoded bit stream. Bits before the first flag are ignored. Repeated flags are treated as idle or preamble and do not create empty frames. Frame bits between flag pairs are unstuffed and checked with AX.25 FCS before being returned.

The acquisition layer can return multiple valid frames from one PCM buffer. If a candidate frame has bad FCS or malformed HDLC, the layer counts it and keeps scanning for later valid frames.

Frame acquisition stats report:

- Decoded bit count.
- Flags seen.
- Candidate frames seen.
- Valid frames returned.
- Bad-FCS frame count.
- Oversize frame count.
- Malformed frame count.
- DCD score and average confidence from the AFSK decoder.

## Limitations

- Generated host vectors only.
- Best-offset selection only, not a full timing recovery loop.
- Diagnostic DCD score only, not production DCD.
- Whole-buffer frame acquisition only, not a streaming ring-buffer API.
- No squelch/COS integration.
- Mild synthetic noise only.
- No real sample clock drift tolerance claim.
- No real-radio test claim.
- No embedded audio DMA.
- No embedded audio performance claim.
- No production modem filtering.

## Sources checked

| Source title                                             | Date checked | Note                                           |
| -------------------------------------------------------- | ------------ | ---------------------------------------------- |
| APRS Protocol Reference 1.0.1, APRS Working Group        | 2026-05-26   | APRS over AX.25 context                        |
| Clarifying the Amateur Bell 202 Modem, TAPR DCC 2014     | 2026-05-26   | Amateur Bell 202/APRS modem context            |
| AX.25 Link Access Protocol v2.2, TAPR/ARRL               | 2026-05-26   | AX.25 link-layer scope separate from modem DSP |
