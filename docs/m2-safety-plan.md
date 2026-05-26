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
- M2.1 host-native embedded tests verify app init, shutdown, and fault paths force the stub PTT state off.
- M2.2 host-native embedded tests verify simulated watchdog fault sets reset cause to watchdog, increments the platform fault counter, and forces the stub PTT state off.
- M2.3 host-native USB CDC tests verify echo, KISS loopback, malformed KISS recovery, and unsupported KISS commands do not change the stub PTT state.
- M2.4 host-native diagnostics tests verify PTT state, reset cause, watchdog fault state, USB counters, and KISS parse counters are visible through a bounded snapshot.
- M2.5 host-native audio tests verify audio loopback and audio error counters do not change the stub PTT state.
- M2.6 host-native embedded TNC tests verify KISS commands, malformed input, unsupported modes, and invalid modes do not change the stub PTT state.
- M2.7 host-native modem boundary tests verify simulated AFSK sample generation does not change the stub PTT state.
- M2.8 host-native RX modem tests verify audio RX processing and KISS output do not change the stub PTT state.

## Watchdog And Timeout

M2 firmware planning must include:

- Watchdog enabled only after a safe startup state is established.
- Watchdog reset path that returns PTT to off.
- Max TX timeout carried into embedded control state.
- Abort path that forces TX inactive and PTT off.
- Audio loopback errors that do not bypass watchdog or PTT safe-off.
- KISS/USB input that cannot directly key PTT.
- Unsupported or invalid mode requests that cannot activate unsafe paths.
- Simulated modem sample generation that cannot key PTT.
- Watchdog faults that abort active simulated modem TX.
- RX audio processing that cannot key PTT.
- Malformed or noise-like RX audio that cannot affect watchdog or PTT safe-off.

## Diagnostics Visibility

M2 must expose enough diagnostics for board tests:

- Boot/reset cause where available.
- Watchdog event counter where available.
- PTT state.
- USB/KISS parse error state.
- Audio overflow and underflow counters.
- Embedded TNC mode, KISS command, and mode request counters.
- Embedded modem TX active, request, done, rejected, abort, and sample counters.
- Embedded modem RX frame, sample, audio error, and output drop counters.
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
9. Malformed USB/KISS input is tested without changing PTT state.
10. Unsupported mode requests are tested without changing PTT state.
11. Simulated modem TX is tested without changing PTT state.
12. Watchdog fault aborts simulated modem TX.
13. Simulated RX audio decode is tested without changing PTT state.
14. Malformed RX audio is tested without changing PTT state.

Only after these gates should a later milestone consider hardware PTT connection planning.
