# M2 Embedded Configuration And Persistence Plan

M2.18 defines the embedded configuration model before any real flash storage or settings writes exist.

## Scope

This pass is planning plus host-testable config validation only.

- No real flash writes.
- No EEPROM emulation.
- No linker flash region use.
- No wear-leveling implementation.
- No persistent storage enabled.
- No filesystem persistence in embedded core.
- No hardware register access.

## Config Classes

Planned config classes:

- Factory defaults: compiled safe defaults used when no valid config exists.
- Runtime volatile config: active settings used by the embedded app.
- Persistent config candidate: future explicit-commit settings, not active in M2.18.
- Temporary KISS/SETHW config: no-flash changes from host commands.
- Diagnostic/runtime counters: not persisted by default.
- Fault records: future persistence only after wear and power-loss rules exist.

## Settings To Model

M2.18 models:

- Current mode.
- `NINO_MODE` compatibility setting.
- KISS SETHW mode selection.
- Temporary/no-flash mode requests, including NinoTNC +16 behavior.
- TXDELAY.
- P.
- SlotTime.
- TXTAIL.
- FullDuplex.
- Max TX timeout.
- Audio level placeholders.
- USB descriptor profile placeholder.
- Diagnostics verbosity placeholder.
- Safety policy flags.

## Validation Rules

- Only implemented modes may become active for TX/RX.
- Known but unimplemented modes may be recorded as requested state only when safe.
- Invalid modes are rejected.
- TX timeout must be nonzero.
- P range is 0 through 255.
- SlotTime range is 0 through 255, with default 10.
- TXDELAY range is 0 through 255, with default 50.
- TXTAIL range is 0 through 255, with default 0.
- FullDuplex is boolean.
- USB descriptor profile must be a known planned profile.
- Diagnostics level must stay within the documented bounded range.
- Unsafe settings never key PTT directly.
- Corrupted or invalid config must revert to factory defaults in future persistence work.

## Persistence Format Plan

Future persistent records should include:

- Config magic.
- Schema version.
- Payload length.
- CRC32 or CRC16, to be decided and implemented later.
- Monotonic generation counter.
- Active and backup bank.
- Valid flag or commit marker.
- Rollback to last valid record.
- Fallback to factory defaults.

No real flash layout, linker region, erase, write, or commit marker is implemented in M2.18.

## Flash And Wear Safety

- Do not write flash on every KISS command.
- Temporary/no-flash SETHW requests must not write.
- Persistent writes require an explicit commit command later.
- Writes must be rate-limited.
- Power-loss safety should use double-bank records and commit markers.
- M2.18 implements none of the storage path.

## NinoTNC Compatibility

- `NINO_MODE=6` maps to implemented 1200 AFSK AX.25.
- `NINO_MODE=22` maps to the same mode as temporary/no-flash.
- SETHW mode 6 may request persistent-style behavior, but KiloTNC must not write flash until explicit persistence support exists.
- SETHW mode 22 is temporary/no-flash compatibility behavior.
- Unsupported modes are counted and not activated.
- Invalid modes are counted and rejected.

## M2.18 Host-Tested Metadata

`embedded/include/kilotnc_config.h` defines:

- Config magic.
- Schema version.
- Fixed-size config struct.
- Factory defaults.
- Validation rules.
- KISS setting application.
- Nino-compatible SETHW application.
- Load and persist stubs that return not implemented.

`make embedded-test` verifies defaults, validation, SETHW 6 and 22, unsupported and invalid modes, persistence stubs, diagnostics fields, and PTT safe-off behavior.
