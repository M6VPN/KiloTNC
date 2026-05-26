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

Future embedded targets should be explicit and opt-in. They must not break host CI when the embedded toolchain is absent.

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
