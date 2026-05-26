# Interoperability

M1.26 defines the external interoperability plan. It does not implement required external-tool tests.

## Scope

KiloTNC uses black-box interoperability testing only. External projects and devices may be invoked as installed programs or attached hardware in optional local tests, but their source code, internal tests, vectors, tables, and implementation details are not imported.

Normal builds and CI do not require external tools. Optional local wrappers live under `interop/` and skip safely when prerequisites are missing.

## Targets

Dire Wolf:

- External black-box software TNC target.
- Planned checks include KISS TCP or PTY framing, AX.25 UI frames, and generated AFSK PCM or WAV decode where practical.
- Dire Wolf is not a code source for KiloTNC.

NinoTNC:

- Compatibility target for KISS SETHW mode behavior and Nino-compatible mode values.
- Planned checks include `NINO_MODE=` mapping, SETHW `6` and `22`, and later IL2Pc behavior when KiloTNC has matching modem support.
- NinoTNC firmware is not a code source for KiloTNC.

Linux AX.25 tools:

- Optional local KISS client target where the host has AX.25 tools configured.
- Planned checks use local PTY or TCP KISS paths where practical.

Other KISS clients:

- Optional local-only compatibility targets.
- Planned checks focus on KISS byte streams, command frames, and escaped payloads.

## Test Classes

Planned optional black-box checks:

- KISS byte-stream framing.
- Escaped FEND and FESC handling.
- SETHW mode command behavior.
- KISS TCP localhost behavior.
- PTY KISS behavior.
- Generated WAV or PCM audio decoded by external software.
- External-generated KISS input accepted by KiloTNC.
- Future hardware serial or USB tests.

## Safety

- No internet-to-RF bridge is enabled by default.
- RF transmit paths must pass operator policy, mode validation, queue limits, DCD/channel access, and max TX watchdog logic.
- Optional tests that transmit RF must require explicit operator action and legal authorization.
- Default optional checks are dummy, file, local IPC, or generated audio checks only.
- No optional wrapper uses `sudo`, installs packages, downloads files, or assumes hardware is attached.

## Status

Implemented now:

- Internal deterministic KISS compatibility tests across local daemon transports.
- Optional wrapper placeholders under `interop/`.
- `make interop-help`.

Planned:

- External black-box checks for installed tools and attached hardware.

Not implemented:

- External-tool invocation in CI.
- Real RF interoperability tests.
- Imported external code, tests, vectors, tables, or algorithms.

## References Checked

| Source                          | Date checked | Note                                  |
| ------------------------------- | ------------ | ------------------------------------- |
| Dire Wolf GitHub README         | 2026-05-26   | Software AX.25 packet modem/TNC role  |
| Linux kernel AX.25 documentation | 2026-05-26   | AX.25 utilities context               |
| TARPN NinoTNC operator manual   | 2026-05-26   | SETHW mode values and temporary style |
