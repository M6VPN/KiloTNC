# M2 Safety Plan

M2 dev-board firmware work must prove safe defaults before any real radio PTT, codec path, or RF transmit path exists.

## Hard Limits

- No RF transmit in M2.
- No actual radio PTT connection in M2.
- No audio path connected to a transmitter in M2.
- No network control path in M2.
- No PCB work in M2.
- PTT tests use a GPIO test pin only.

## PTT Defaults

- PTT state is off on boot.
- PTT state is off after reset.
- PTT state is off after watchdog reset.
- PTT GPIO is not enabled until explicitly configured in test firmware.
- Failures force the test PTT state off where the platform permits it.

## Watchdog And Timeout

M2 firmware planning must include:

- Watchdog enabled only after a safe startup state is established.
- Watchdog reset path that returns PTT to off.
- Max TX timeout carried into embedded control state.
- Abort path that forces TX inactive and PTT off.

## Diagnostics Visibility

M2 must expose enough diagnostics for board tests:

- Boot/reset cause where available.
- Watchdog event counter where available.
- PTT state.
- TX active state.
- Max TX timeout event.
- Fault counter snapshot.

The first diagnostics path may be a simple debug output or USB test path. It does not require a full embedded CLI in M2.0.

## Test Sequence Before Any Hardware PTT

Before any future real radio PTT connection:

1. Compile-only firmware skeleton builds.
2. Board boots with PTT test pin off.
3. Reset leaves PTT test pin off.
4. Watchdog reset leaves PTT test pin off.
5. Max TX timeout forces PTT test pin off.
6. Abort forces PTT test pin off.
7. Diagnostics show the tested state transitions.
8. USB KISS loopback is tested without any RF path.

Only after these gates should a later milestone consider hardware PTT connection planning.
