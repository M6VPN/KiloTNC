# Embedded Tests

M2.2 expands the host-native embedded skeleton test with platform tick, watchdog, reset-cause, diagnostics, and test GPIO/PTT stub coverage.

Planned M2 test sequence:

- Embedded compile.
- Host-native `make embedded-test`.
- Board boot with PTT test pin off.
- Reset safe-off check.
- Watchdog safe-off check.
- USB CDC KISS echo or loopback.
- Fault counter visibility.
- GPIO-only PTT default-off test.

These tests must not connect RF transmit hardware during M2.
