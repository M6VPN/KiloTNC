# Embedded Tests

M2.1 adds a host-native embedded skeleton test.

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
