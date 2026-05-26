# KiloTNC

KiloTNC is a resilient hardware TNC project for amateur packet radio. The first design target is a robust AX.25/KISS TNC with clean audio, fail-safe PTT, diagnostics, and a path toward IL2Pc support.

The project may support host-side bridge modes for external software modems such as VARA, ARDOP, and Mercury. VARA is treated only as an external software modem bridge target, not as a native implementation.

No Dire Wolf code is copied into this repo. Dire Wolf may be used only as an external interoperability reference and test tool unless the project license changes.

## Table of Contents

- [Status](#status)
- [Scope](#scope)
- [Milestones](#milestones)
- [Build and Test](#build-and-test)
- [Repository Layout](#repository-layout)
- [License](#license)

## Status

Current stage: M1.21 daemon control/status command surface.

No hardware or PCB work has started. PCB work is blocked until protocol and architecture documents are reviewed.

## Scope

Native targets:

- KISS host interface over USB CDC ACM.
- AX.25 UI frame handling first.
- 300 baud AX.25 AFSK research path.
- 1200 baud AX.25 Bell 202 AFSK.
- 9600 baud G3RUH-like/GFSK research path.
- IL2P and IL2Pc research path.

Bridge targets:

- VARA through host software, USB audio, PTT, and CAT where possible.
- ARDOP through host software.
- Mercury through host software and TCP TNC compatibility testing.

Planned platform targets:

- Linux/BSD daemon target using the portable core and host audio interfaces.
- Future internet/node services with safety gates and local-only defaults.
- Future Ethernet or Wi-Fi hardware variants after the USB/audio/PTT path is proven.

These platform targets remain planning items except for the minimal host daemon file adapter, daemon config profiles, one-shot daemon control/status commands, raw PCM audio backend abstraction, daemon radio-control abstraction with no-PTT and log backends, stdin/stdout mode, localhost-only TCP KISS test adapter, local Unix socket once-mode adapter, and local PTY once-mode adapter.

## Milestones

| Stage | Target                                                             |
| ----- | ------------------------------------------------------------------ |
| M0    | Research matrix, architecture docs, protocol scope                 |
| M1    | Host-side simulator: KISS parser, AX.25 encode/decode, FCS tests   |
| M2    | Dev-board prototype: USB CDC KISS plus loopback modem test harness |
| M3    | 1200 baud Bell 202 AFSK TX/RX with WAV/golden-vector tests         |
| M4    | Radio bench test with dummy audio/radio interface, PTT safety      |
| M5    | 9600 baud G3RUH-like/GFSK mode                                     |
| M6    | IL2Pc research implementation and interop testing                  |
| M7    | KiCad rev A schematic and PCB                                      |
| M8    | 72-hour burn-in, USB reconnect, brownout, malformed KISS fuzzing   |
| M9    | Optional bridge mode for VARA/Mercury/ARDOP via host software      |

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

GitHub Actions runs `make test`, `make tools`, `make tool-test`, `make daemon`, `make daemon-test`, and `make sanitize` on push and pull request.

## Repository Layout

| Path                    | Purpose                                                  |
| ----------------------- | -------------------------------------------------------- |
| `daemon/`               | Minimal host daemon skeleton and local IPC adapters      |
| `daemon/examples/`      | Safe daemon profile examples for local test modes        |
| `docs/`                 | Architecture, protocol, hardware, firmware               |
| `firmware/`             | Portable protocol library and future firmware workspace  |
| `firmware/include/`     | Protocol library headers                                 |
| `firmware/src/`         | Portable protocol source                                 |
| `firmware/test/`        | Host-side protocol tests                                 |
| `hardware/`             | Hardware planning and PCB assets                         |
| `hardware/kicad/`       | Future KiCad project files                               |
| `hardware/datasheets/`  | Local datasheet references if redistribution permits     |
| `hardware/simulations/` | Future analog and signal simulations                     |
| `tools/`                | Future host tools and scripts                            |
| `lab/`                  | Bench notes, measurements, and test records              |

## License

ISC License. See [LICENSE](LICENSE).
