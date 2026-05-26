# Embedded Tests

M2.4 expands the host-native embedded skeleton test with diagnostics snapshot and formatter coverage.

Planned M2 test sequence:

- Embedded compile.
- Host-native `make embedded-test`.
- USB CDC stub RX/TX buffer handling.
- USB echo mode.
- KISS data-frame loopback.
- KISS escaping and malformed input recovery.
- Embedded diagnostics capture and formatting.
- USB and KISS diagnostic counters.
- Watchdog fault and PTT-off diagnostic reporting.
- Board boot with PTT test pin off.
- Reset safe-off check.
- Watchdog safe-off check.
- USB CDC KISS echo or loopback.
- Fault counter visibility.
- GPIO-only PTT default-off test.

These tests must not connect RF transmit hardware during M2.
