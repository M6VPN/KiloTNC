# Interoperability Harnesses

This directory holds optional local wrappers for future black-box interoperability checks. They are not part of normal builds or CI.

The wrappers do not import external source code, tests, vectors, tables, or implementation details. They only check whether external tools or hardware paths are present and then skip unless explicitly enabled.

## Safety Defaults

- No script uses `sudo`.
- No script installs packages.
- No script downloads files.
- No script transmits RF.
- No script assumes hardware is attached.
- Generated output belongs under `build/interop/`.
- `KILOTNC_INTEROP_RUN=1` is required before a wrapper does more than print help or a skip message.

## Wrappers

```text
interop/run_direwolf_optional.sh
interop/run_ninotnc_optional.sh
interop/run_ax25_optional.sh
```

The wrappers are placeholders for later local testing. They currently document the planned checks and exit safely when prerequisites are missing.

## Planned Checks

- KISS TCP framing with external KISS clients.
- PTY KISS framing with external KISS clients.
- Escaped FEND and FESC handling.
- SETHW mode `6` and `22` behavior.
- Generated KISS input accepted by KiloTNC.
- KiloTNC generated KISS accepted by external tools.
- Generated PCM or WAV decoded by external software where practical.

External tools are black-box targets only. Their source code and internal tests are not copied into KiloTNC.
