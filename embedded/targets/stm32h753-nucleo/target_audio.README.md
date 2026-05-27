# STM32H753 Nucleo Audio Plan

M2.10 does not implement ADC, DAC, SAI, I2S, DMA, or codec drivers.

The future audio adapter must preserve the current embedded audio boundary: mono signed 16-bit samples at 48 kHz. Early target work should use test loopback or dummy sample paths before any transmitter audio connection exists.

M2.11 planning is tracked in [docs/m2-stm32h753-resource-plan.md](../../../docs/m2-stm32h753-resource-plan.md). SAI or I2S remains the preferred later external-codec direction, while ADC, DAC, PWM, or loopback paths remain early experiments only. No audio pinout is selected.

Intentionally absent in M2.10:

- No codec driver.
- No SAI or I2S setup.
- No ADC or DAC path.
- No DMA.
- No audio pin assignment.
- No transmitter audio path.

M2.15 clock planning is tracked in [docs/m2-clock-reset-watchdog-plan.md](../../../docs/m2-clock-reset-watchdog-plan.md). The metadata keeps 48000 Hz as the planned audio sample rate, but the audio clock source, PLL values, SAI/I2S clocking, and codec clocking remain `TBD`.

Safe boot planning requires audio initialization only after PTT safe/off and diagnostics are established. M2.15 does not add audio clocks, SAI/I2S setup, codec code, DMA, HAL, CMSIS, or hardware register access.
