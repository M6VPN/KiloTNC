# M2 Target Selection

M2 starts dev-board firmware prototype work. It does not start PCB design, RF transmit, real radio keying, real codec drivers, or irreversible target-specific firmware implementation.

## Selection Criteria

The first embedded target should reduce firmware bring-up risk while keeping enough headroom for later modem, diagnostics, and safety work.

Required criteria:

- USB device support for future USB CDC KISS.
- Reliable debug access.
- Watchdog and reset-cause support.
- Timer precision for modem and control timing.
- Exposed GPIO for a test-only PTT pin.
- Audio path potential:
	- SAI or I2S preferred for a later external codec.
	- ADC, DAC, PWM, or loopback paths are acceptable only for early experiments.
- Enough RAM for fixed buffers, diagnostics, and packet queues.
- Open documentation.
- Current availability.
- Low risk for M2 compile and board bring-up.

## Primary Candidate

Recommended M2.0 primary path: `stm32h753-nucleo`, based on the NUCLEO-H753ZI or a current equivalent STM32H753 Nucleo-144 board.

Reasons:

- The ST product page for NUCLEO-H753ZI currently lists it as active and in volume production.
- The Nucleo-144 board family provides ST morpho headers, Arduino/ST Zio expansion, integrated ST-LINK debugger/programmer, and STM32Cube package support.
- STM32H753ZI is a Cortex-M7 class MCU with 480 MHz performance class, 2 MB flash, and up to 1 MB RAM.
- STM32H753ZI includes USB OTG interfaces and serial audio options, including SAI and I2S-capable SPI peripherals.
- The board class gives enough GPIO and debug access for watchdog, reset, diagnostics, and test-only PTT checks.
- STM32CubeIDE is available locally, but M2.0 does not require committing generated IDE projects or requiring the IDE for normal host builds.

Older STM32H743 Nucleo variants should be re-checked before use. A 2025 ST community report describes NUCLEO-H743ZI2 as obsolete according to STM32CubeIDE marketing status, so M2 should prefer a currently listed H753 Nucleo path unless a specific H743 board is verified again.

## Secondary Experimental Candidate

Secondary experimental path: RP2350 or Raspberry Pi Pico 2 class.

RP2350/Pico 2 is useful for low-cost portability experiments, but it is not the baseline full hardware TNC target for M2.

Reasons to keep it secondary:

- Pico 2 uses the RP2350 MCU with dual Cortex-M33 or Hazard3 processors up to 150 MHz.
- Pico 2 has 520 KB SRAM, 4 MB onboard flash, USB 1.1 host/device support, and PIO blocks for custom peripheral experiments.
- RP2350 has less RAM and less peripheral headroom than the STM32H753 class for a full TNC target with modem DSP, diagnostics, USB, watchdog, and later external codec work.
- PIO/audio experiments may be useful later, but they should not complicate the first M2 firmware skeleton.

## M2.0 Decision

M2.0 selects:

- Primary target name: `stm32h753-nucleo`.
- Primary board path: NUCLEO-H753ZI or current equivalent STM32H753 Nucleo-144 board.
- Secondary experimental target: RP2350/Pico 2 class, documentation-only for now.

No purchase or hardware order is made in this pass. No vendor code, STM32Cube project, CMSIS tree, Pico SDK, TinyUSB tree, or generated IDE output is committed.

## M2.10 Target Skeleton

The `stm32h753-nucleo` target path now has a compile-gated skeleton under:

```text
embedded/targets/stm32h753-nucleo/
```

The skeleton records target metadata and planned feature flags for the NUCLEO-H753ZI or current equivalent STM32H753 Nucleo-144 board path. It does not include STM32 HAL, CMSIS, TinyUSB, startup vectors, linker scripts, pin assignments, hardware register access, or a flashable firmware image.

## M2.11 Resource Planning

M2.11 adds `docs/m2-stm32h753-resource-plan.md` and target resource metadata for the selected `stm32h753-nucleo` path.

USB, diagnostics, audio, watchdog, reset, and test PTT GPIO resources remain planning items. No real pin assignment is final, and the test PTT GPIO remains `TBD`.

## M2.12 Target Check

M2.12 adds opt-in target-local build metadata and a skip-safe compile wrapper for `stm32h753-nucleo`.

The check object-compiles safe skeleton sources only when `arm-none-eabi-gcc` is available. It does not link firmware, produce ELF, BIN, or HEX output, add startup code, add a linker script, import vendor SDKs, access hardware registers, or flash a board.

## Sources Checked

Sources checked on 2026-05-26:

- STMicroelectronics NUCLEO-H753ZI product page: https://www.st.com/en/evaluation-tools/nucleo-h753zi.html
- STMicroelectronics STM32H753ZI product page: https://www.st.com/en/microcontrollers-microprocessors/stm32h753zi.html
- STMicroelectronics STM32H753xI datasheet: https://www.st.com/resource/en/datasheet/stm32h753zi.pdf
- STMicroelectronics STM32H7 Nucleo-144 boards user manual UM2407: https://www.st.com/resource/en/user_manual/dm00499160-stm32h7-nucleo-144-boards-mb1364-stmicroelectronics.pdf
- STMicroelectronics community report for NUCLEO-H743ZI2 CubeIDE obsolete status: https://community.st.com/t5/stm32cubeide-mcus/nucleo-h743zi2-board-not-supported-in-stm32cubeide-obsolete/td-p/823344
- Raspberry Pi Pico-series microcontrollers documentation: https://www.raspberrypi.com/documentation/microcontrollers/raspberry-pi-pico.html
- Raspberry Pi RP2350 datasheet: https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf
