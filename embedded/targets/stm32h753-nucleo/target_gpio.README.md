# STM32H753 Nucleo GPIO Plan

M2.10 does not assign pins or drive GPIO.

The future GPIO adapter must start with a test-only PTT pin, default low/off, and no radio PTT connection. Pin candidates must be planned and reviewed before any HAL or register-level implementation.

M2.11 planning is tracked in [docs/m2-stm32h753-resource-plan.md](../../../docs/m2-stm32h753-resource-plan.md). The test PTT GPIO remains `TBD`. No GPIO pin is selected for real PTT use, and no pin can be treated as real PTT until it is checked against ST docs, checked against the selected board schematic, and bench-tested with no radio connected.

Intentionally absent in M2.10:

- No pin assignments.
- No alternate-function setup.
- No hardware PTT pin.
- No GPIO register access.
- No STM32 HAL GPIO calls.
