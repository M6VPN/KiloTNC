# Embedded Firmware Workspace

This directory is the M2 workspace for future dev-board firmware.

M2.0 is documentation and skeleton only. No USB CDC, audio DMA, codec, GPIO PTT, HAL, or board driver code is implemented here yet.

The host-side M1 portable core remains in `firmware/`. Embedded firmware will use adapters around that portable core rather than copying daemon file, socket, PTY, or host audio code.

Planned layout:

- `targets/`: target-specific board support.
- `platform/`: platform abstraction for tick, watchdog, reset, GPIO, USB, audio, diagnostics, and config.
- `app/`: embedded application state machine.
- `tests/`: embedded compile and board test notes.
- `common/`: shared embedded-only helpers if needed later.

The selected M2 primary target path is documented in `docs/m2-target-selection.md`.
