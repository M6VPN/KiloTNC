# Embedded Tests

M2.9 expands the host-native embedded skeleton test with full USB KISS to simulated audio TX/RX to USB KISS loopback coverage.

Planned M2 test sequence:

- Embedded compile.
- Host-native `make embedded-test`.
- USB CDC stub RX/TX buffer handling.
- USB echo mode.
- KISS data-frame loopback.
- KISS escaping and malformed input recovery.
- Embedded diagnostics capture and formatting.
- USB and KISS diagnostic counters.
- Audio format, RX injection, TX capture, loopback, overflow, and underflow counters.
- Embedded TNC KISS command handling.
- Embedded TNC Nino-compatible SETHW mode mapping.
- Embedded TNC diagnostics counters.
- Embedded TNC KISS data loopback.
- Embedded modem start, process, and abort.
- Simulated AFSK1200 sample generation into the audio stub.
- TNC-to-modem TX request handling.
- Modem diagnostics counters.
- AFSK1200 streaming RX from audio stub samples.
- Decoded AX.25 frame KISS output to the USB CDC stub.
- RX output drop counters.
- Full host-test loopback through USB, TNC, modem TX, audio TX/RX, modem RX, and USB output.
- Full-loopback timeout and watchdog-fault handling.
- Watchdog fault and PTT-off diagnostic reporting.
- Board boot with PTT test pin off.
- Reset safe-off check.
- Watchdog safe-off check.
- USB CDC KISS echo or loopback.
- Fault counter visibility.
- GPIO-only PTT default-off test.

These tests must not connect RF transmit hardware during M2.
