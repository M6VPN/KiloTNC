# Daemon Audio Backends

`kilotncd` keeps audio I/O behind daemon-side adapters. Portable firmware
modules do not include file I/O, ALSA, sndio, OSS, PulseAudio, PipeWire, or
other host audio APIs.

## Implemented Backend

M1.18 implemented the raw PCM backend:

- Signed 16-bit little-endian PCM.
- Mono.
- 48,000 Hz.
- File, stdin, or stdout through existing daemon path handling.

This backend is used by deterministic daemon tests and generated host vectors.

## Backend Support Matrix

| Backend    | M1.24 state       | Default headers/libs required |
| ---------- | ----------------- | ----------------------------- |
| raw        | Implemented       | None                          |
| ALSA       | Planned stub      | None                          |
| sndio      | Planned stub      | None                          |
| OSS        | Planned stub      | None                          |
| PulseAudio | Future, not core  | None                          |
| PipeWire   | Future, not core  | None                          |

## M1.23 ALSA Stub

M1.23 adds a compile-gated ALSA boundary:

- `audio_backend=alsa` is a known backend name.
- Default builds compile an ALSA stub only.
- The stub returns unsupported for open, read, write, and close operations.
- Default builds do not include `alsa/asoundlib.h`.
- Default builds do not link `-lasound`.
- `ENABLE_ALSA` is reserved for a later implementation and does not enable real
  ALSA runtime support in M1.23.

Selecting ALSA in M1.23 should fail during daemon validation or backend open.
Raw PCM behavior remains unchanged.

## Future ALSA Design

The future Linux ALSA backend should stay inside the daemon adapter layer and
use the same format as the core modem path:

- 48,000 Hz sample rate.
- Mono capture and playback.
- Signed 16-bit little-endian PCM.
- Explicit capture and playback device names.
- Bounded period and buffer sizes.
- Deterministic underrun and overrun counters.
- Clean close paths that leave TX and simulated PTT safe.

The backend must not key PTT directly. TX still has to pass through mode
validation, channel access, TX timeout, and the daemon radio-control backend.

## M1.24 sndio Stub

M1.24 adds a compile-gated sndio boundary:

- `audio_backend=sndio` is a known backend name.
- Default builds compile a sndio stub only.
- The stub returns unsupported for open, read, write, and close operations.
- Default builds do not include sndio headers.
- Default builds do not link sndio libraries.
- `ENABLE_SNDIO` is reserved for a later implementation and does not enable real
  sndio runtime support in M1.24.

Future sndio support should target OpenBSD-friendly audio first, with 48,000 Hz
mono signed 16-bit PCM, explicit capture/playback device names, bounded latency
planning, and deterministic error counters.

## M1.24 OSS Stub

M1.24 adds a compile-gated OSS boundary:

- `audio_backend=oss` is a known backend name.
- Default builds compile an OSS stub only.
- The stub returns unsupported for open, read, write, and close operations.
- Default builds do not include OSS headers such as `sys/soundcard.h`.
- Default builds do not link OSS-specific libraries.
- `ENABLE_OSS` is reserved for a later implementation and does not enable real
  OSS runtime support in M1.24.

Future OSS support should remain a BSD fallback path with 48,000 Hz mono signed
16-bit PCM, explicit device path configuration, bounded buffer planning, and
deterministic overrun/underrun diagnostics.

## Not Implemented

- Real ALSA capture or playback.
- sndio runtime support.
- OSS runtime support.
- PulseAudio or PipeWire runtime support.
- Audio device enumeration.
- Audio clock drift handling.
- Production overrun or underrun recovery.
- Real-radio audio testing.

## Sources checked

| Source title                     | Date checked | Note                                  |
| -------------------------------- | ------------ | ------------------------------------- |
| ALSA PCM interface documentation | 2026-05-26   | PCM parameters and backend planning   |
| sndio project page               | 2026-05-26   | sndio platform scope                  |
| sndio OpenBSD manual             | 2026-05-26   | OpenBSD audio backend planning        |
| FreeBSD Architecture Handbook    | 2026-05-26   | OSS and pcm interface planning        |
