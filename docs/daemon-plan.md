# Daemon Plan

Working name: `kilotncd`.

`kilotncd` is a planned hardware-independent Linux/BSD daemon. It should use the portable KiloTNC core where practical and provide a KISS TNC service through host audio and host radio-control interfaces.

M1.14 implements a minimal deterministic daemon skeleton. It supports config parsing, file/stdin-style adapters, status output, TX once, RX once, and loopback once.

M1.15 implements a localhost-only KISS TCP adapter for single-client once-mode testing. It does not implement remote service, multi-client serving, real audio, PTYs, Unix sockets, serial PTT, CAT, daemonization, or hardware access.

M1.16 implements a local Unix socket KISS once-mode adapter and explicit stdin/stdout file-stream behavior. It does not implement persistent socket serving, multi-client serving, real audio, PTYs, serial PTT, CAT, daemonization, or hardware access.

M1.17 implements a local PTY KISS once-mode adapter. It does not implement persistent PTY service, real serial hardware, serial PTT, real audio, CAT, daemonization, or hardware access.

## Scope

- Run on Linux and BSD systems where the selected adapters are available.
- Run without KiloTNC hardware.
- Use portable KiloTNC modules for KISS, AX.25, HDLC, FCS, AFSK1200, modes, diagnostics, and channel-access logic.
- Keep OS audio, PTY, sockets, serial, CAT, and config parsing outside the portable core.
- Provide a host-side role similar to a software TNC concept, without copying Dire Wolf code or behavior text.

## Process Model

- One foreground daemon process.
- Optional test mode using stdin/stdout and raw PCM files.
- Adapter threads or event loops may be added later only after the core interfaces are stable.
- RF transmit decisions must pass through channel-access, DCD, operator policy, and max-TX safety logic.

## Configuration File

Planned configuration fields:

- Mode, using KiloTNC mode names or Nino-compatible `NINO_MODE=` values.
- KISS interface type and bind path or address.
- Audio backend and device names.
- PTT/CAT backend and device names.
- TXDELAY, TXTAIL, p-persistence, SlotTime, FullDuplex, and max TX timeout.
- Network allowlist for any non-local client.
- Diagnostics output level.

M1.14 implements a bounded daemon config file parser. Persistent storage and live config reload are not implemented.

M1.13 raw PCM, WAV, KISS, and loopback vectors are intended as first daemon regression fixtures before real audio or socket adapters are added. M1.14 uses those fixtures for the first `make daemon-test` flow.

## Audio Backends

Planned first-pass backends:

- POSIX raw device/file backend for deterministic tests. File mode exists in M1.14.
- stdin/stdout file-stream mode for deterministic tests. Explicit bounded stdin/stdout handling exists in M1.16.
- ALSA for Linux as the likely first real Linux audio backend.
- sndio for OpenBSD as the likely BSD-friendly backend.
- OSS as a possible fallback on BSDs where available.

Optional future host integrations:

- PulseAudio.
- PipeWire.

PulseAudio and PipeWire are not first core targets because the first daemon should keep the audio adapter narrow and testable.

## KISS Interfaces

Planned interfaces:

- TCP server, bound to localhost by default. Single-client once mode exists in M1.15.
- Unix socket. Single-client local once mode exists in M1.16.
- PTY. Single-client local once mode exists in M1.17.
- stdin/stdout test mode. Explicit bounded stdin/stdout mode exists in M1.16.

KISS-over-TCP, Unix sockets, and PTYs are host interfaces, not on-air protocols.

## Radio Control

Planned adapters:

- VOX/no-PTT mode.
- Serial RTS/DTR PTT.
- CAT PTT.
- GPIO through platform-specific adapters only.

The portable core must not call GPIO, serial, or CAT APIs directly.

## Safety

Default daemon safety policy:

- Max TX timeout enabled.
- DCD gating enabled unless FullDuplex is explicitly set.
- p-persistence and SlotTime honored before transmit.
- Network clients restricted by explicit allowlist.
- No default internet-to-RF bridge.
- No unauthenticated remote control listener.
- Operator must explicitly enable any non-local packet input path.

## Diagnostics

Planned diagnostics:

- Text status snapshot.
- Counters from KISS, modem RX/TX, channel access, PTT, and fault records.
- Future local control socket.

No USB CDC, network control protocol, or persistent fault store is implemented in this pass.

## Portability

Target platforms:

- Linux.
- OpenBSD.
- FreeBSD.
- NetBSD.

Each platform should use an adapter layer for audio and radio control. Platform support must be tested before it is marked supported.

## Future Modes

The daemon should use the shared mode registry.

- Nino-compatible mode names and values are compatibility inputs.
- Only 1200 AFSK AX.25 is implemented now.
- Other Nino-compatible modes remain planned or research entries until modem support exists and tests pass.

## Sources checked

| Source title                             | Date checked | Note                                      |
| ---------------------------------------- | ------------ | ----------------------------------------- |
| ALSA PCM interface documentation         | 2026-05-26   | Linux PCM audio backend planning          |
| sndio project and OpenBSD manual         | 2026-05-26   | OpenBSD audio backend planning            |
| FreeBSD Architecture Handbook, Sound     | 2026-05-26   | OSS/pcm context for FreeBSD audio         |
| NetBSD audio(4) manual                   | 2026-05-26   | NetBSD audio device interface context     |
| OpenBSD posix_openpt and ptsname manuals | 2026-05-26   | PTY adapter API planning and portability  |
