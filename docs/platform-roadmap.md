# Platform Roadmap

KiloTNC is a multi-target TNC project. The shared core should stay portable while each target owns its platform adapters.

## Project Targets

### MCU Firmware

The MCU firmware target is the physical KiloTNC hardware:

- USB CDC KISS host interface.
- External audio codec path.
- PTT, COS, and CAT interfaces.
- Fail-safe continuous operation.
- Watchdog, timeout, and fault-counter behavior.

This target is not implemented yet.

### Linux/BSD Daemon

The daemon target, `kilotncd`, is a hardware-independent host TNC process. It can run in the same general role as a software TNC, but it must not copy Dire Wolf code, docs, comments, tests, or behavior text.

Planned daemon functions:

- Host audio input and output.
- KISS over TCP, PTY, stdin/stdout, Unix socket, or local files.
- Serial/CAT/PTT adapters where available.
- Linux, OpenBSD, FreeBSD, and NetBSD support where practical.

This target is planned only.

### Network-Capable Hardware

Network-capable physical devices are future variants or add-on modules. Rev A remains USB, audio, and PTT focused.

Possible network hardware options:

- Ethernet MAC plus external PHY.
- SPI Ethernet controller.
- External Wi-Fi module.
- MCU or SBC companion.

This target is planned only.

### Internet and Node Services

Internet and node services are optional layers around the TNC core. They must not create an automatic internet-to-RF bridge by default.

Possible future functions:

- KISS-over-TCP local service.
- Remote diagnostics.
- AX.25 routing, BBS, APRS, or iGate-style integrations where legal and technically appropriate.
- Store-and-forward or VPN/mesh integrations.

This target is planned only.

## Shared Portable Core

The shared core is the code intended to move between host tools, the future daemon, and MCU firmware:

- KISS parser and encoder.
- AX.25, HDLC, and FCS.
- AFSK1200 RX and TX.
- TNC1200 harness.
- Mode registry.
- Diagnostics snapshots.
- Channel access and PTT simulation.

The current implementation is host-side C. It does not include MCU HAL, host audio backends, sockets, or hardware drivers.

M1.13 generated vectors provide repeatable KISS, PCM, WAV, loopback, and diagnostics outputs for later MCU firmware, daemon, and hardware validation.

## Target-Specific Adapters

Platform adapters should wrap the shared core:

| Adapter             | Target responsibility                              |
| ------------------- | -------------------------------------------------- |
| MCU HAL             | Timers, USB device, GPIO, watchdog, DMA, reset     |
| Host audio          | ALSA, sndio, OSS, raw file, or test audio backend  |
| Host KISS           | TCP, Unix socket, PTY, stdin/stdout, file I/O      |
| Host PTT/CAT        | Serial RTS/DTR, CAT commands, platform GPIO        |
| Network             | Local services, allowlists, remote diagnostics     |
| Persistence/config  | Versioned settings, operator policy, rollback      |

Adapters must not bypass mode validation, DCD/channel access, max TX timeout, diagnostics, or PTT safety rules.

## Non-Goals For Now

- No PCB work.
- No real RF transmit tests.
- No internet-to-RF auto-bridge.
- No native VARA modem.
- No Dire Wolf source import.
- No daemon implementation.
- No network hardware implementation.

## Sources checked

| Source title                                      | Date checked | Note                                  |
| ------------------------------------------------- | ------------ | ------------------------------------- |
| ALSA PCM interface documentation                  | 2026-05-26   | Linux PCM audio backend planning      |
| sndio project page and OpenBSD sndio manual       | 2026-05-26   | OpenBSD and portable sndio planning   |
| FreeBSD Architecture Handbook OSS section         | 2026-05-26   | FreeBSD OSS/pcm planning              |
| NetBSD audio(4) manual page                       | 2026-05-26   | NetBSD audio backend planning         |
