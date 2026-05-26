# Embedded Tests

M2.3 expands the host-native embedded skeleton test with USB CDC stub and KISS bridge coverage.

Planned M2 test sequence:

- Embedded compile.
- Host-native `make embedded-test`.
- USB CDC stub RX/TX buffer handling.
- USB echo mode.
- KISS data-frame loopback.
- KISS escaping and malformed input recovery.
- Board boot with PTT test pin off.
- Reset safe-off check.
- Watchdog safe-off check.
- USB CDC KISS echo or loopback.
- Fault counter visibility.
- GPIO-only PTT default-off test.

These tests must not connect RF transmit hardware during M2.
