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
