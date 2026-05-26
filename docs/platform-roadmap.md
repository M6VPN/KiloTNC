# Platform Roadmap

KiloTNC is a multi-target TNC project. The shared core should stay portable while each target owns its platform adapters.

M1 is complete enough to start M2 as host-side groundwork. M2 starts dev-board firmware only. Real daemon audio backends, network services, and network-capable hardware are later tracks.

## Project Targets

### MCU Firmware

The MCU firmware target is the physical KiloTNC hardware:

- USB CDC KISS host interface.
- External audio codec path.
- PTT, COS, and CAT interfaces.
- Fail-safe continuous operation.
- Watchdog, timeout, and fault-counter behavior.

This target starts in M2 as a dev-board firmware prototype. M2.0 selects the `stm32h753-nucleo` target path and adds documentation/build skeleton only. M2.1 adds a host-native compile-only embedded skeleton. M2.2 adds host-native platform tick, watchdog, reset-cause, diagnostics, and GPIO/PTT test stubs. M2.3 adds a host-native USB CDC byte-stream and KISS bridge skeleton. M2.4 adds a host-native embedded diagnostics bridge. M2.5 adds a host-native audio stub and sample loopback path. M2.6 adds a host-native embedded TNC core integration skeleton. M2.7 adds a host-native modem/audio boundary that writes simulated AFSK1200 samples into the audio stub. M2.8 adds a host-native RX audio/modem boundary that emits decoded frames as KISS to the USB stub. M2.9 adds a bounded full host-test loopback through USB KISS, simulated TX audio, simulated RX audio, and USB KISS output. M2.10 adds compile-gated STM32H753 target skeleton files and opt-in target syntax checking. It does not include PCB work, RF transmit, real radio keying, real USB stack integration, real codec drivers, real audio peripheral drivers, GPIO PTT drivers, startup code, linker scripts, or flashable firmware.

### Linux/BSD Daemon

The daemon target, `kilotncd`, is a hardware-independent host TNC process. It can run in the same general role as a software TNC, but it must not copy Dire Wolf code, docs, comments, tests, or behavior text.

Planned daemon functions:

- Host audio input and output.
- KISS over TCP, PTY, stdin/stdout, Unix socket, or local files.
- Serial/CAT/PTT adapters where available.
- Linux, OpenBSD, FreeBSD, and NetBSD support where practical.

The full daemon target is planned. The current implementation is a minimal host-side skeleton with config profile validation, raw PCM audio backend abstraction, ALSA, sndio, and OSS stub boundaries, file, stdin/stdout, localhost TCP once-mode, local Unix socket once-mode, and local PTY once-mode adapters. Real ALSA, sndio, and OSS runtime backends are later daemon-track work.

The daemon track is separate from M2 firmware. It remains useful for host regression tests and future local TNC service work while embedded firmware proceeds around the portable core.

### Network-Capable Hardware

Network-capable physical devices are future variants or add-on modules. Rev A remains USB, audio, and PTT focused.

Possible network hardware options:

- Ethernet MAC plus external PHY.
- SPI Ethernet controller.
- External Wi-Fi module.
- MCU or SBC companion.

This target is later than Rev A.

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

M1.14 adds the first `kilotncd` skeleton around the shared core. M1.15 adds a localhost-only KISS TCP test adapter. M1.16 adds a local Unix socket once-mode adapter and explicit stdin/stdout file-stream behavior. M1.17 adds a local PTY KISS once-mode adapter. M1.18 adds a raw-only audio backend abstraction. M1.19 adds a radio-control backend boundary with no-PTT, simulated, and log backends only. M1.20 adds profile validation to keep adapter combinations explicit before real audio or radio-control backends are added. M1.23 adds an ALSA stub boundary while keeping raw files as the only implemented audio backend. M1.24 adds sndio and OSS stub boundaries. The daemon still has no real audio, remote network service, real serial, USB, or hardware adapters.

## Target-Specific Adapters

Platform adapters should wrap the shared core:

| Adapter             | Target responsibility                              |
| ------------------- | -------------------------------------------------- |
| MCU HAL             | Timers, USB device, GPIO, watchdog, DMA, reset     |
| Host audio          | ALSA, sndio, OSS, raw file, or test audio backend  |
| Host KISS           | TCP, Unix socket, PTY, stdin/stdout, file I/O      |
| Host PTT/CAT        | No-PTT, simulated/log, serial RTS/DTR, CAT, GPIO   |
| Network             | Local services, allowlists, remote diagnostics     |
| Persistence/config  | Versioned settings, operator policy, rollback      |

Adapters must not bypass mode validation, DCD/channel access, max TX timeout, diagnostics, or PTT safety rules.

## Non-Goals For Now

- No PCB work.
- No real RF transmit tests.
- No internet-to-RF auto-bridge.
- No native VARA modem.
- No Dire Wolf source import.
- No real audio daemon implementation.
- No network hardware implementation.

## Sources checked

| Source title                                      | Date checked | Note                                  |
| ------------------------------------------------- | ------------ | ------------------------------------- |
| ALSA PCM interface documentation                  | 2026-05-26   | Linux PCM audio backend planning      |
| sndio project page and OpenBSD sndio manual       | 2026-05-26   | OpenBSD and portable sndio planning   |
| FreeBSD Architecture Handbook OSS section         | 2026-05-26   | FreeBSD OSS/pcm planning              |
| NetBSD audio(4) manual page                       | 2026-05-26   | NetBSD audio backend planning         |
