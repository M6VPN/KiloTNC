# Embedded Firmware Workspace

This directory is the M2 workspace for future dev-board firmware.

M2.6 adds a host-native embedded TNC core integration skeleton. No real USB diagnostics channel, USB stack, descriptors, ADC, DAC, SAI, I2S, audio DMA, codec, real GPIO PTT, HAL, board driver code, or modem audio path is implemented here yet.

The host-side M1 portable core remains in `firmware/`. Embedded firmware will use adapters around that portable core rather than copying daemon file, socket, PTY, or host audio code.

Planned layout:

- `targets/`: target-specific board support.
- `platform/`: platform abstraction for tick, watchdog, reset, GPIO, USB, audio, diagnostics, and config.
- `app/`: embedded application state machine.
- `tests/`: embedded compile and board test notes.
- `common/`: shared embedded-only helpers if needed later.

The selected M2 primary target path is documented in `docs/m2-target-selection.md`.

Run the host-native skeleton test with:

```text
make embedded-test
```

The USB CDC skeleton uses fixed buffers and the existing portable KISS parser. It can echo bytes or loop KISS data frames back in tests, but it does not call TinyUSB, STM32Cube, CMSIS, hardware registers, or endpoint drivers.

The diagnostics bridge captures a bounded snapshot and formats it into caller-provided text buffers. It is intended for future USB CDC diagnostics, but M2.4 does not send diagnostics over USB.

The audio loopback path uses fixed sample buffers and mono signed 16-bit 48 kHz format metadata. Tests can inject RX samples, capture TX samples, and verify overflow and underflow counters. M2.5 does not connect audio samples to a modem or transmitter path.

The embedded TNC skeleton reads KISS bytes from the USB CDC stub, handles basic KISS commands, maps Nino-compatible SETHW mode 6 and 22 through the shared mode registry, keeps unsupported modes inactive, and can loop accepted data frames back to USB in tests. M2.6 does not connect KISS frames to modem audio or hardware PTT.
