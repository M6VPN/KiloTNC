# KiloTNC

KiloTNC is a hardware TNC project for amateur packet radio. The first design target is a robust AX.25/KISS TNC with clean audio, fail-safe PTT, diagnostics, and a path toward IL2Pc support.

KiloTNC will support host-side bridge modes for external software modems such as VARA, ARDOP, and Mercury (VARA is an external software modem bridge target, not as a native implementation).

## Table of Contents

- [Status](#status)
- [Scope](#scope)
- [Milestones](#milestones)
- [Build and Test](#build-and-test)
- [Repository Layout](#repository-layout)
- [License](#license)

## Status

Current stage: M2.17 embedded queue/backpressure planning.

No PCB work, hardware build, flashable firmware image, RF transmit, real radio keying, real codec driver work, real USB stack, embedded USB implementation, real audio hardware path, real audio driver work, real clock setup, real watchdog setup, real interrupt or DMA boundary, linker script, or final embedded memory map has started.

## Scope

Native targets:

- KISS host interface over USB CDC ACM.
- AX.25 UI frame handling first.
- 300 baud AX.25 AFSK research path.
- 1200 baud AX.25 Bell 202 AFSK.
- 9600 baud G3RUH-like/GFSK research path.
- IL2P and IL2Pc research path.

DSP modems throughout to ensure best signal/audio quality.

Bridge targets:

- VARA through host software, USB audio, PTT, and CAT where possible.
- ARDOP through host software.
- Mercury through host software and TCP TNC compatibility testing.

Planned platform targets:

- Linux/BSD daemon target using the portable core and host audio interfaces.
- Future internet/node services with safety gates and local-only defaults.
- Future Ethernet or Wi-Fi hardware variants after the USB/audio/PTT path is proven.
- LoRa experimentations

These platform targets remain planning items except for the minimal host daemon file adapter, daemon config profiles, foreground daemon loop skeleton, one-shot daemon control/status commands, raw PCM audio backend abstraction, compile-gated ALSA, sndio, and OSS stub boundaries, daemon radio-control abstraction with no-PTT and log backends, stdin/stdout mode, localhost-only TCP KISS test adapter, local Unix socket once-mode adapter, local PTY once-mode adapter, KISS compatibility tests across local daemon transports, external black-box interoperability planning placeholders, the M2.1 embedded compile-only skeleton, M2.2 platform stubs, the M2.3 host-native USB CDC KISS bridge skeleton, the M2.4 embedded diagnostics bridge, the M2.5 host-native audio loopback/test path, the M2.6 embedded TNC core integration skeleton, the M2.7 embedded modem/audio boundary skeleton, the M2.8 embedded RX audio/modem boundary skeleton, the M2.9 embedded full host-test loopback skeleton, the M2.10 compile-gated STM32H753 target skeleton, the M2.11 STM32H753 pin/resource planning layer, the M2.12 opt-in STM32H753 cross-compile skeleton, the M2.13 USB CDC stack selection and dependency-boundary planning, the M2.14 USB descriptor planning plus MCU family abstraction metadata, the M2.15 STM32H7 clock/reset/watchdog planning metadata, the M2.16 memory/flash/CPU budget planning metadata, and the M2.17 embedded queue/backpressure policy metadata.

## Milestones

| Stage | Target                                                   |
| ----- | -------------------------------------------------------- |
| M0    | Initial research and project scope                       |
| M1    | Portable host-side core and daemon groundwork            |
| M2    | Dev-board firmware prototype                             |
| M3    | Hardware audio and PTT bench validation                  |
| M4    | Rev A schematic and PCB                                  |
| M5    | Rev A bring-up and burn-in                               |
| M6    | Additional modem modes and NinoTNC compatibility growth  |
| M7    | Linux/BSD daemon real audio backends                     |
| M8    | Network and node services                                |
| M9    | Future Ethernet or Wi-Fi hardware variants               |

Detailed milestone notes are in [docs/milestones.md](docs/milestones.md). M2 dev-board readiness is in [docs/m2-devboard-readiness.md](docs/m2-devboard-readiness.md), target selection is in [docs/m2-target-selection.md](docs/m2-target-selection.md), MCU family planning is in [docs/m2-mcu-family-plan.md](docs/m2-mcu-family-plan.md), board abstraction planning is in [docs/m2-board-abstraction-plan.md](docs/m2-board-abstraction-plan.md), STM32H753 resource planning is in [docs/m2-stm32h753-resource-plan.md](docs/m2-stm32h753-resource-plan.md), clock/reset/watchdog planning is in [docs/m2-clock-reset-watchdog-plan.md](docs/m2-clock-reset-watchdog-plan.md), memory/flash/CPU budget planning is in [docs/m2-memory-flash-cpu-budget.md](docs/m2-memory-flash-cpu-budget.md), queue/backpressure planning is in [docs/m2-queue-backpressure-plan.md](docs/m2-queue-backpressure-plan.md), USB stack planning is in [docs/m2-usb-stack-plan.md](docs/m2-usb-stack-plan.md), the embedded build strategy is in [docs/m2-embedded-build-strategy.md](docs/m2-embedded-build-strategy.md), and the M2 safety plan is in [docs/m2-safety-plan.md](docs/m2-safety-plan.md). Platform and daemon tracks are covered in [docs/platform-roadmap.md](docs/platform-roadmap.md) and [docs/daemon-plan.md](docs/daemon-plan.md).

## Build and Test

Build and run the host-side protocol and AFSK1200 tests:

```text
make test
```

Clean generated files:

```text
make clean
```

Run the optional sanitizer build:

```text
make sanitize
```

Build host-side debug tools:

```text
make tools
```

Run deterministic CLI vector checks:

```text
make tool-test
```

Build the minimal host daemon skeleton:

```text
make daemon
```

Run deterministic daemon checks:

```text
make daemon-test
```

Run deterministic KISS compatibility checks across local daemon transports:

```text
make kiss-compat-test
```

Run the host-native embedded skeleton test:

```text
make embedded-test
```

Show opt-in STM32H753 target skeleton guidance:

```text
make embedded-target-help
```

Run the skip-safe target skeleton object check:

```text
make embedded-target-check
```

Show optional external interoperability wrapper guidance:

```text
make interop-help
```

Show embedded build planning guidance:

```text
make embedded-help
```

GitHub Actions runs `make test`, `make tools`, `make tool-test`, `make daemon`, `make daemon-test`, `make kiss-compat-test`, `make embedded-test`, `make embedded-target-check`, and `make sanitize` on push and pull request.

## Repository Layout

| Path                    | Purpose                                                  |
| ----------------------- | -------------------------------------------------------- |
| `daemon/`               | Minimal host daemon skeleton and local IPC adapters      |
| `daemon/examples/`      | Safe daemon profile examples for local test modes        |
| `docs/`                 | Architecture, protocol, hardware, firmware               |
| `embedded/`             | M2 dev-board firmware planning and skeleton workspace    |
| `firmware/`             | Portable protocol library and future firmware workspace  |
| `firmware/include/`     | Protocol library headers                                 |
| `firmware/src/`         | Portable protocol source                                 |
| `firmware/test/`        | Host-side protocol tests                                 |
| `hardware/`             | Hardware planning and PCB assets                         |
| `hardware/kicad/`       | Future KiCad project files                               |
| `hardware/datasheets/`  | Local datasheet references if redistribution permits     |
| `hardware/simulations/` | Future analog and signal simulations                     |
| `interop/`              | Optional external black-box interop wrappers             |
| `tools/`                | Future host tools and scripts                            |
| `lab/`                  | Bench notes, measurements, and test records              |

## License

ISC License. See [LICENSE](LICENSE).
