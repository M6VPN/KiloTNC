# M2 Embedded Build Strategy

M2 embedded work must stay separate from the host-side M1 build, daemon, tools, and tests.

## Rules For M2.0

- No vendor IDE requirement.
- No STM32Cube, CMSIS, Pico SDK, TinyUSB, or generated IDE project vendored into the repo.
- No embedded toolchain required for `make test`.
- No cross-compiler required in CI yet.
- No generated embedded build output committed.
- Host build remains the default project build.
- Embedded build is opt-in after the first compile-only skeleton exists.

STM32CubeIDE is available locally and may be used for inspection or later generated-project experiments, but M2.0 does not commit IDE output.

## M2.1 Host-Native Skeleton Test

M2.1 adds a host-native embedded skeleton test:

```text
make embedded-test
```

This target compiles the embedded app skeleton, platform stub, target metadata, and embedded test under the host compiler. It does not use `arm-none-eabi-gcc`, STM32 HAL, CMSIS, TinyUSB, STM32Cube, Pico SDK, or generated vendor projects.

The target verifies:

- Embedded app init works.
- Init forces the stub PTT state off.
- Step advances stub time.
- Step kicks the watchdog counter.
- Fault and shutdown force PTT off.
- Target metadata is visible.

## M2.2 Platform Stub Coverage

`make embedded-test` now also verifies:

- Monotonic millisecond tick state.
- 10 ms control tick state.
- Watchdog kick counter.
- Simulated watchdog fault.
- Reset-cause reporting.
- Test GPIO/PTT default-off and safe-off behavior.
- Diagnostic write counter.
- Platform fault counter.

## M2.3 USB CDC Skeleton Coverage

`make embedded-test` now also verifies:

- USB CDC stub connection state.
- Bounded RX and TX byte-stream buffers.
- RX and TX overflow rejection.
- USB echo mode.
- KISS data-frame loopback.
- KISS FEND and FESC escaping.
- Malformed KISS recovery counters.
- Unsupported KISS command ignore counters.
- App step services the USB bridge while keeping PTT off.

This remains host-native. It does not use TinyUSB, STM32 HAL, CMSIS, STM32Cube, descriptors, endpoints, hardware registers, or a USB device controller.

## M2.4 Embedded Diagnostics Coverage

`make embedded-test` now also verifies:

- Diagnostic snapshot capture from a fresh app.
- App step, platform tick, watchdog, and diagnostic write counters.
- USB RX/TX byte counters.
- USB RX/TX overflow counters.
- KISS frame, parse error, ignored command, and overlength counters.
- Watchdog fault state, reset cause, and PTT-off state.
- Bounded text formatting and truncation rejection.

This remains host-native. It does not create a USB diagnostic channel or call a real USB device stack.

## M2.5 Audio Loopback Coverage

`make embedded-test` now also verifies:

- 48 kHz mono signed 16-bit audio format metadata.
- Fixed-buffer RX sample injection.
- Fixed-buffer TX sample capture.
- Audio loopback from RX samples to TX samples.
- Partial-block loopback.
- RX underflow counters.
- TX overflow counters.
- Audio counters in embedded diagnostics.
- App step services audio loopback while kicking the watchdog.
- PTT remains off during audio loopback.

This remains host-native. It does not use ADC, DAC, SAI, I2S, DMA, codec drivers, hardware registers, or a transmitter audio path.

## M2.6 Embedded TNC Coverage

`make embedded-test` now also verifies:

- Embedded TNC init and default 1200 AFSK AX.25 mode.
- KISS data-frame parsing from the USB CDC stub.
- Optional KISS data-frame loopback to the USB CDC stub.
- KISS FEND and FESC escaping through loopback.
- Repeated FEND handling.
- Malformed KISS recovery counters.
- Unsupported KISS command counters.
- TXDELAY, P, SlotTime, TXtail, and FullDuplex command state.
- SETHW mode 6 and 22 through the shared mode registry.
- Known but unimplemented modes remain inactive and counted.
- Invalid mode requests remain inactive and counted.
- Embedded TNC counters in embedded diagnostics.
- App step services the embedded TNC while kicking the watchdog.
- PTT remains off during embedded TNC tests.

This remains host-native. It does not use a real USB stack, modem audio,
GPIO PTT, hardware registers, or hardware drivers.

## M2.7 Embedded Modem Boundary Coverage

`make embedded-test` now also verifies:

- Embedded modem init, start, process, and abort paths.
- Valid 1200 AFSK AX.25 frame acceptance.
- Unsupported mode rejection.
- Malformed frame rejection.
- Busy TX rejection.
- Simulated AFSK1200 sample generation into the audio stub.
- Small bounded TX chunk processing.
- TNC-to-modem request handling when explicitly enabled.
- Modem TX remains disabled by default.
- Unsupported mode requests block simulated modem TX.
- App step services modem TX while kicking the watchdog.
- Watchdog fault and safe shutdown abort active modem TX.
- PTT remains off during modem sample generation.
- Modem counters in embedded diagnostics.

This remains host-native. It does not use real USB, ADC, DAC, SAI, I2S,
DMA, codec drivers, GPIO PTT, hardware registers, or transmitter audio.

## M2.8 Embedded RX Modem Boundary Coverage

`make embedded-test` now also verifies:

- Embedded modem RX init and reset paths.
- Empty audio RX underflow handling.
- Generated AFSK1200 PCM decode through portable streaming RX.
- AX.25 destination, source, and info verification after decode.
- Small bounded RX chunk processing.
- Noise input does not produce bogus frames.
- RX output frame array too small is handled safely.
- Modem RX is disabled by default.
- Explicitly enabled modem RX emits KISS data frames to the USB stub.
- USB TX overflow while emitting RX frames is counted and safe.
- App step services RX while kicking the watchdog.
- Watchdog fault blocks RX processing and leaves PTT off.
- Modem RX counters in embedded diagnostics.

This remains host-native. It does not use real RF receive, real USB, ADC,
DAC, SAI, I2S, DMA, codec drivers, GPIO PTT, or hardware registers.

## Future Build Approaches

Candidate approaches for M2.1 and later:

- CMake toolchain file for the selected STM32 target.
- Makefile wrapper for the selected embedded target.
- External vendor SDK path supplied by environment variable.
- Compile-only embedded skeleton that links portable core objects without enabling USB, audio, GPIO, or PTT drivers.

## Future Environment Variables

These names are reserved for later embedded work. They are not required in M2.0.

```text
KILOTNC_EMBEDDED_TARGET=stm32h753-nucleo
STM32CUBE_PATH=/path/to/STM32Cube...
ARM_NONE_EABI_CC=arm-none-eabi-gcc
```

## Build Separation

Host targets remain:

- `make test`
- `make sanitize`
- `make tools`
- `make tool-test`
- `make daemon`
- `make daemon-test`
- `make kiss-compat-test`

Future embedded cross-compile targets should be explicit and opt-in. They must not break host CI when the embedded toolchain is absent.

## Repository Policy

Allowed in M2.0:

- Documentation.
- Embedded directory skeleton.
- Build-help text.

Not allowed in M2.0:

- STM32 HAL driver code.
- USB CDC implementation.
- Codec or DMA audio driver code.
- GPIO PTT driver code.
- Generated STM32CubeIDE projects.
- Pico SDK projects.
- Vendor source trees.
