# M2 Clock, Reset, And Watchdog Plan

M2.15 records the future STM32H7 clock, reset-cause, boot, and watchdog strategy for the `stm32h753-nucleo` target path.

## Scope

This pass is planning and metadata only.

- No RCC code.
- No STM32 HAL code.
- No CMSIS code.
- No register reads or writes.
- No startup code.
- No linker or startup integration.
- No flashable firmware.
- No board flashing.

## Clock Planning

Future firmware needs clocks for:

- Stable system tick for app and control timing.
- 10 ms control tick.
- USB clock requirements, to be verified by future USB stack work.
- 48 kHz audio sample-rate path.
- Later SAI or I2S codec clocking.
- Timer resources for modem and channel-control timing.
- Watchdog source planning.

Clock setup must stay target-specific and must live behind the platform adapter. The portable core must not depend on STM32 clock-tree details.

M2.15 does not commit final PLL values, RCC settings, timer prescalers, USB clock source, or audio clock source.

Planned metadata:

| Item                   | M2.15 value | Notes                                           |
| ---------------------- | ----------- | ----------------------------------------------- |
| Control tick           | 10 ms       | Host-tested metadata only                       |
| Platform timebase      | 1 ms        | Planning value only                             |
| Audio sample rate      | 48000 Hz    | Shared with host audio stubs                    |
| Clock tree finalized   | 0           | Final values remain future work                 |
| USB clock source       | TBD         | Verify against USB stack and RM0433             |
| Audio clock source     | TBD         | Verify against codec and SAI/I2S requirements   |
| HSE source             | TBD         | Verify against UM2407 board configuration       |

UM2407 documents Nucleo-144 HSE clock source options. The default HSE planning path on supported Nucleo-144 boards uses ST-LINK MCO at 8 MHz into PF0/PH0-OSC_IN, but M2.15 does not convert that into firmware configuration.

## Reset Cause Planning

Future reset-cause tracking should cover:

- Power-on reset.
- Software reset.
- Independent watchdog reset.
- Brownout or low-power reset where supported.
- Unknown reset cause fallback.

Reset cause must be captured early in boot before it is cleared by later platform initialization. It must be reported through diagnostics.

M2.15 does not read reset registers. `target_reset.h` only records planned reset-cause categories and marks reset-cause handling as not finalized.

## Watchdog Planning

The independent watchdog is the preferred fail-safe path because it is intended to reset the MCU when firmware stops making progress.

Future watchdog policy:

- Do not refresh the watchdog unless critical tasks report progress.
- Keep watchdog setup behind the platform adapter.
- Keep watchdog reset visible in diagnostics.
- Force PTT off on watchdog fault or after watchdog reset.
- Keep real watchdog enable disabled until the safe-off path is proven.

Planned watchdog quorum:

- Main loop.
- USB task or adapter.
- Audio task or adapter.
- PTT safety and control task.

The window watchdog may be considered later if it adds useful timing coverage. It is not part of the M2.15 policy.

M2.15 does not enable a hardware watchdog and does not commit watchdog register values.

## Safe Boot Order

Future safe boot order:

1. CPU reset and startup.
2. PTT or test GPIO forced safe and off as early as possible.
3. Capture reset cause.
4. Initialize minimal platform timebase.
5. Initialize diagnostics.
6. Initialize watchdog policy.
7. Enable real watchdog only after the safe-off path is proven.
8. Initialize USB byte-stream adapter.
9. Initialize audio adapter only after safe defaults.
10. Start app loop.

USB and audio input must not be able to key PTT during boot.

## Family Portability

STM32H753 remains the flagship path.

Future STM32H743, STM32H735, and STM32H750 variants must implement the same platform contract:

- Stable tick service.
- 10 ms control tick.
- Reset-cause capture.
- Watchdog policy.
- PTT safe-off before input processing.
- Diagnostics visibility.

H735 and H750 require memory, flash, and resource margin checks before use. H750 needs extra caution because ST documents STM32H750 Value line parts with a smaller embedded flash profile than H743/H753 class parts.

No family-specific clock values are committed in M2.15.

## Sources Checked

Sources checked on 2026-05-27:

- STM32H753ZI product page, STMicroelectronics: https://www.st.com/en/microcontrollers-microprocessors/stm32h753zi.html
- RM0433 STM32H743/753 and STM32H750 reference manual, STMicroelectronics: https://www.st.com/resource/en/reference_manual/rm0433-stm32h743-753-and-stm32h750-value-line-advanced-arm-based-32-bit-mcus-stmicroelectronics.pdf
- STM32H7 Nucleo-144 boards user manual UM2407, STMicroelectronics: https://www.st.com/resource/en/user_manual/um2407-stm32h7-nucleo144-board-stmicroelectronics.pdf
- NUCLEO-H753ZI product page, STMicroelectronics: https://www.st.com/en/evaluation-tools/nucleo-h753zi.html
- STM32H750 Value line page, STMicroelectronics: https://www.st.com/en/microcontrollers-microprocessors/stm32h750-value-line.html
