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
