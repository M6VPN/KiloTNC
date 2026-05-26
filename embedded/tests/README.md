# Embedded Tests

Embedded tests will be added after the compile-only skeleton exists.

Planned M2 test sequence:

- Embedded compile.
- Board boot with PTT test pin off.
- Reset safe-off check.
- Watchdog safe-off check.
- USB CDC KISS echo or loopback.
- Fault counter visibility.
- GPIO-only PTT default-off test.

These tests must not connect RF transmit hardware during M2.
