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

Current stage: M1.2 host-side 1200 baud Bell 202 AFSK simulator and clean-vector tests.

PCB work is blocked until protocol and architecture documents are reviewed.

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

GitHub Actions runs `make test` and `make sanitize` on push and pull request.

## Repository Layout

| Path                    | Purpose                                                  |
| ----------------------- | -------------------------------------------------------- |
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
