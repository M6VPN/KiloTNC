# STM32H753 Nucleo GPIO Plan

M2.10 does not assign pins or drive GPIO.

The future GPIO adapter must start with a test-only PTT pin, default low/off, and no radio PTT connection. Pin candidates must be planned and reviewed before any HAL or register-level implementation.

Intentionally absent in M2.10:

- No pin assignments.
- No alternate-function setup.
- No hardware PTT pin.
- No GPIO register access.
- No STM32 HAL GPIO calls.
