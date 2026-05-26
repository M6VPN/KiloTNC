# KISS Compatibility Tests

M1.25 adds deterministic host-side checks for KISS framing across local daemon transports. The tests use generated inputs under `build/kiss-compat/` and do not commit binary vectors.

## Tested Transports

- File input and output.
- Stdin and stdout file-stream mode.
- Localhost TCP once mode.
- Unix socket once mode.
- PTY once mode.
- CLI loopback.

These are host transports for KISS frames. They are not on-air protocols and do not use real radio hardware.

## Tested KISS Behavior

- Plain AX.25 UI data frames.
- Payloads containing KISS FEND `0xC0`.
- Payloads containing KISS FESC `0xDB`.
- Multiple KISS frames in one byte stream.
- Repeated FEND bytes before a data frame.
- TXDELAY, P, SlotTime, TXtail, and FullDuplex command frames.
- SETHW mode `6` and `22`.
- Unsupported command ignore and counter update.
- Malformed escape recovery or safe diagnostic failure.

## Running

```text
make kiss-compat-test
```

The target builds the daemon, local transport test clients, CLI, and the KISS compatibility helper. It writes all generated input, output, PCM, and diagnostics files under `build/kiss-compat/`.

## Limits

- No external TNC interoperability is claimed yet.
- Dire Wolf is not used in this test path.
- NinoTNC hardware is not used in this test path.
- No real serial hardware is used.
- No real RF receive or transmit is used.
- No remote network service is started.
