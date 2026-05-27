# M2 MCU Family Plan

M2 keeps STM32H753 as the flagship KiloTNC development target while leaving room for related STM32H7 parts and separate low-cost variants.

## Scope

This document is planning only.

- No PCB work.
- No pin assignments.
- No hardware drivers.
- No STM32 HAL or CMSIS code.
- No USB stack.
- No flashable firmware.

## Hardware Tiers

| Stage                         | Recommended MCU                                      |
| ----------------------------- | ---------------------------------------------------- |
| Current M2/M3 development     | STM32H753 Nucleo / STM32H753ZI                       |
| First custom serious board    | STM32H753ZI or STM32H743ZI                           |
| Cost-reduced production board | STM32H735ZG, after proving RAM/flash margins         |
| Ultra-cheap/simple TNC variant | RP2350 or smaller STM32, separate target            |
| Connectivity companion        | ESP32-S3, not modem DSP core                         |

## Flagship Target

STM32H753 remains the flagship M2 target.

Reasons:

- It is the selected Nucleo development path.
- STM32H753ZI class parts provide 480 MHz Cortex-M7 class performance, 2 MB flash, and 1 MB RAM.
- The Nucleo-144 board path gives integrated ST-LINK debug, expansion headers, and USB/audio/PTT planning room.
- The H753 path has enough headroom to keep fixed buffers, diagnostics, USB CDC, modem DSP, watchdog, and safety work conservative.

## Related STM32H7 Variants

H743 should remain possible for the first custom board path because STM32H743ZI class parts are close to the H753ZI memory and performance class.

H735ZG is a later cost-reduced production candidate only after memory and flash margins are measured with real firmware. It has less published memory headroom than the H753ZI/H743ZI class, so it should not drive early architecture limits.

H750 should remain a possible STM32H7-family path only after memory and boot storage planning. ST documents STM32H750 Value line parts with 128 KB embedded flash and 1 MB SRAM, so any H750 path needs a separate code-placement, external-memory, or boot-storage review before use.

## Separate Variant Targets

RP2350 is a separate low-cost or simple TNC variant target. It is not a drop-in replacement for the STM32H753 Nucleo path.

Use RP2350 later for:

- Portability checks.
- Low-cost simple KISS TNC experiments.
- PIO or simple audio experiments.

Do not let RP2350 constraints reduce the STM32H753 flagship design before margins are known.

## Connectivity Companion

ESP32-S3 is a possible network or connectivity companion, not the modem DSP core.

Use it later for:

- Wi-Fi or Bluetooth companion experiments.
- Local management or bridge functions.
- Separation from real-time modem and PTT safety paths.

Do not place modem DSP, PTT authority, or watchdog safety ownership on an ESP32-S3 companion in M2 planning.

## Abstraction Requirements

The embedded architecture must keep these layers separate:

- MCU family.
- MCU part.
- Board.
- USB stack.
- Audio interface.
- PTT and GPIO.
- Diagnostics.
- Optional connectivity companion.

The portable core remains shared. STM32H7 target code, board resource metadata, and future USB/audio/PTT adapters must remain behind explicit boundaries.

## Clock, Reset, And Watchdog Contract

Future H743, H735, and H750 variants must satisfy the same platform contract as the H753 flagship:

- Stable platform timebase.
- 10 ms control tick.
- 48 kHz audio-rate planning where the audio path is enabled.
- Reset-cause capture before diagnostics output.
- Watchdog policy that requires task progress before refresh.
- PTT safe/off before USB or audio input is accepted.
- Diagnostics visibility for reset cause and watchdog fault state.

H735 and H750 variants remain validation-gated until memory, flash, clock, and watchdog margins are checked against the same contract.

## Memory And Flash Gates

Memory, flash, and CPU budget planning is tracked in [m2-memory-flash-cpu-budget.md](m2-memory-flash-cpu-budget.md).

H753 and H743 remain the main high-headroom paths until linker-map and runtime high-water measurements exist.

H735 remains a cost-reduced candidate only after:

- ARM linker-map flash and RAM use are measured.
- Runtime stack high-water marks are measured.
- USB and audio queue high-water marks are measured.
- Modem RX/TX worst-case paths are measured.
- Watchdog progress timing is measured.

H750 remains gated on an additional flash and memory-placement review because the H750 Value line uses a much smaller embedded flash class than H753/H743.

## Sources Checked

Sources checked on 2026-05-27:

- STM32H753ZI product page, STMicroelectronics: https://www.st.com/en/microcontrollers-microprocessors/stm32h753zi.html
- STM32H743ZI product page, STMicroelectronics: https://www.st.com/en/microcontrollers-microprocessors/stm32h743zi.html
- STM32H735ZG product page, STMicroelectronics: https://www.st.com/en/microcontrollers-microprocessors/stm32h735zg.html
- STM32H750 Value line page, STMicroelectronics: https://www.st.com/en/microcontrollers-microprocessors/stm32h750-value-line.html
- RP2350 documentation, Raspberry Pi: https://www.raspberrypi.com/documentation/microcontrollers/rp2350.html
- ESP32-S3 product page, Espressif: https://www.espressif.com/en/products/socs/esp32-s3
