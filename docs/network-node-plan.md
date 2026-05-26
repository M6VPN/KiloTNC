# Network Node Plan

KiloTNC may grow optional internet and node services around the portable TNC core. These services are planned only. No network listener, router, BBS, iGate, or remote-control code is implemented in M1.12.

## Scope

Planned node functions:

- KISS-over-TCP local service.
- Remote KISS only when explicitly enabled.
- Optional authenticated remote control later.
- APRS/iGate-style integration as future research.
- AX.25 router and BBS integration as future research.
- IP mesh or VPN integration as future research.
- Store-and-forward services as future research.
- Remote diagnostics through a controlled status path.

## KISS-over-TCP

KISS-over-TCP is a host interface. It is not an on-air protocol.

Default behavior should be:

- Bind to localhost.
- Accept local clients only.
- Require explicit operator configuration for remote clients.
- Apply queue limits and rate limits before any RF transmit path.

## Remote Control

Remote diagnostics and control must be separate from packet-data input.

Future remote control must include:

- Authentication.
- Explicit operator enablement.
- Rate limits.
- Audit counters.
- No direct PTT control.

Firmware update or device-control functions must not share an unchecked path with RF transmit input.

## RF Safety Gates

Any internet-originated packet that can reach RF must pass through:

- Operator policy.
- Mode validation.
- Queue bounds.
- DCD gating.
- p-persistence.
- Max TX watchdog.
- PTT fail-safe logic.

Automatic internet-to-RF transmission is disabled by default in planning.

## Research Areas

Future research areas:

- APRS/iGate-style operation.
- AX.25 routing.
- BBS integration.
- Store-and-forward packet services.
- IP mesh or VPN integration.
- Remote diagnostics dashboards.

These are not implemented and are not required for Rev A hardware.

## Security Defaults

- Bind localhost by default.
- Do not run an unauthenticated internet listener.
- Do not automatically transmit internet input to RF.
- Use rate limits for host and network inputs.
- Keep max TX watchdog enabled.
- Log and count drops, denials, parse errors, timeouts, and aborts.
- Require explicit operator enablement for every remote packet-data or control path.

## Sources checked

| Source title                     | Date checked | Note                                  |
| -------------------------------- | ------------ | ------------------------------------- |
| The KISS TNC, Chepponis and Karn | 2026-05-26   | KISS is a host-to-TNC framing scheme  |
| APRS Protocol Reference 1.0.1    | 2026-05-26   | APRS/iGate-style work remains future  |
| AX.25 Link Access Protocol v2.2  | 2026-05-26   | AX.25 routing/BBS work remains future |
