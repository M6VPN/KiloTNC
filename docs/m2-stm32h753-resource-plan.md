# M2 STM32H753 Resource Plan

This document records M2.11 planning for the `stm32h753-nucleo` target path.

## Scope

M2.11 is planning and metadata only.

No pin initialization, alternate-function setup, STM32 HAL code, CMSIS code, TinyUSB integration, vendor USB stack, hardware register access, real USB driver, real audio driver, real GPIO PTT control, PCB pinout commitment, or RF path is added in this stage.

All candidate resources remain planned until they are checked against the selected board revision, the ST user manual, the ST schematic package, and a bench test with no radio connected.

## Source Documents To Check

Sources checked on 2026-05-27:

| Source key | Document | URL | M2.11 use |
| ---------- | -------- | --- | --------- |
| ST-Nucleo-H753ZI | NUCLEO-H753ZI product page | https://www.st.com/en/evaluation-tools/nucleo-h753zi.html | Board status, ST morpho, ST Zio, ST-LINK, schematic-pack availability |
| ST-STM32H753ZI | STM32H753ZI product page | https://www.st.com/en/microcontrollers-microprocessors/stm32h753zi.html | MCU class, RAM, flash, timers, USB, ADC, DAC, SAI/I2S class |
| ST-DS12117 | STM32H753xI datasheet | https://www.st.com/resource/en/datasheet/stm32h753zi.pdf | Pin and peripheral verification before future assignments |
| ST-UM2407 | STM32H7 Nucleo-144 boards user manual UM2407 | https://www.st.com/resource/en/user_manual/um2407-stm32h7-nucleo144-board-stmicroelectronics.pdf | Board connectors, LEDs, user button, USB OTG FS, VCP, Ethernet conflicts, solder bridges |
| ST-RM0433 | STM32H742, STM32H743/753, and STM32H750 reference manual | https://www.st.com/en/microcontrollers-microprocessors/stm32h743-753/documentation.html | Future peripheral behavior checks |
| ST-MB1364-H753ZI-C01 | MB1364-H753ZI-C01 board schematic | https://www.st.com/resource/en/schematic_pack/mb1364-h753zi-c01-schematic.pdf | Future board-revision pin and net verification |
| ST-MB1364-H753ZI-E01 | MB1364-H753ZI-E01 board schematic | https://www.st.com/resource/en/schematic_pack/mb1364-h753zi-e01-schematic.pdf | Future board-revision pin and net verification |

The repo source list in `docs/sources.md` is updated with the M2.11 checks.

## Resource Classes

### Debug And Programming

The NUCLEO-H753ZI board includes an on-board ST-LINK debugger/programmer. ST-LINK and SWD resources must remain available during M2.

Planning rules:

- Do not repurpose debug pins.
- Do not disable reset/debug access.
- Treat SWO on PB3 as debug-related when enabled by solder bridge.
- Keep ST-LINK VCP separate from normal firmware requirements.
- Treat bootloader wiring and solder-bridge changes as future bench actions only.

### USB KISS

The first USB planning path is USB OTG FS on CN13, because UM2407 identifies a USB OTG FS Micro-AB connector and board-level USB FS pin configuration.

Planning rules:

- Use USB device role first for future USB CDC KISS.
- Do not implement descriptors, endpoints, TinyUSB, HAL USB, or interrupt handlers in M2.11.
- Verify VBUS, ID, power enable, and overcurrent wiring before any USB driver work.
- Treat USB HS as TBD until the selected board revision and schematic are checked for the intended path.
- Do not let USB input directly key PTT.

### Test PTT GPIO

No test PTT GPIO is selected in M2.11.

Candidate requirements:

- Default low/off.
- Available on an expansion header.
- Not a debug, reset, boot, clock, USB, Ethernet, power, or oscillator pin.
- Not shared with user button/LED unless explicitly chosen for a no-radio bench test.
- Verified against UM2407 and the exact schematic revision before use.
- Bench-tested with no radio connected before any real PTT work.

### Diagnostics

UM2407 documents user LEDs on PB0, PE1, and PB14. These are diagnostic candidates only, not PTT candidates.

Planning rules:

- Use LEDs for simple future heartbeat or fault indication if they do not conflict with higher-value resources.
- Keep UART/VCP diagnostics optional.
- Do not require UART for normal tests.
- Keep USB diagnostics behind the embedded diagnostics bridge when a real USB stack is added later.

### Audio Test Path

The STM32H753 class has ADC, DAC, SAI, and I2S-capable resources, and UM2407 exposes some SAI/I2S-related connector options. M2.11 does not choose an audio pinout.

Planning rules:

- Prefer SAI or I2S for a later external codec path.
- Treat ADC, DAC, PWM, or internal loopback as early experiments only.
- Verify candidate audio pins against the board revision, Ethernet conflicts, ST Zio/ST morpho connectors, and the STM32H753 datasheet.
- Keep transmitter audio disconnected in M2.
- Do not implement audio DMA or real peripheral setup in M2.11.

### Timing And Watchdog

The STM32H753 class provides timers and watchdog resources. M2.11 does not configure them.

Planning rules:

- Use host-tested 10 ms control tick behavior as the reference model.
- Plan SysTick or a general-purpose timer for future app ticks.
- Plan independent watchdog behavior only after safe boot and PTT-off state are verified.
- Plan reset-cause reporting before board fault tests.

### Boot And Config Pins To Avoid

Avoid these resource classes until each use is explicitly verified:

- Boot pins.
- SWD, SWO, and reset/debug pins.
- Oscillator pins.
- Power, VREF, VBAT, and voltage-selection pins.
- USB FS pins and USB board-control pins.
- Ethernet RMII pins when the board Ethernet PHY is connected.
- User button and LED pins unless the function is diagnostic-only.
- Any pin whose solder-bridge state changes the board function.

## Candidate Resource Table

| Function | Candidate resource | Confidence | Source | Notes |
| -------- | ------------------ | ---------- | ------ | ----- |
| Debug/programming | On-board ST-LINK and SWD | high | ST-Nucleo-H753ZI, ST-UM2407 | Keep available for all M2 work |
| Debug trace | SWO on PB3 when enabled | high | ST-UM2407 | Debug-only, not application GPIO |
| Reset | NRST through board reset and ST-LINK control | high | ST-UM2407 | Must stay available |
| USB KISS first path | USB OTG FS Micro-AB connector CN13 | high | ST-UM2407 | Future USB device CDC path only |
| USB FS data | PA11/PA12 through CN13 USB DM/DP | high | ST-UM2407 | Planning note only, no pin macros |
| USB FS board control | PA9 VBUS, PA10 ID, PD10 power enable, PG7 overcurrent | high | ST-UM2407 | Verify before driver work |
| USB HS | TBD, verify selected board schematic | low | ST-DS12117, ST-RM0433, ST schematic package | Not first path |
| Test PTT GPIO | TBD, expansion header GPIO | verify-before-use | ST-UM2407, ST schematic package | Must avoid debug, USB, Ethernet, clock, boot, power |
| User LED diagnostics | LD1 PB0, LD2 PE1, LD3 PB14 | high | ST-UM2407 | Diagnostic-only candidates |
| User button input | B1 PC13 default or PA0 alternate | high | ST-UM2407 | Not a PTT candidate |
| ST-LINK VCP diagnostics | USART3 PD8/PD9 default VCP | high | ST-UM2407 | Optional diagnostics only |
| SAI audio planning | PE2 MCLK and PE6 SD connector options | medium | ST-UM2407 | Verify full SAI signal set before use |
| I2S audio planning | PB13 I2S_A_CK if Ethernet conflict is removed | medium | ST-UM2407 | Avoid while Ethernet PHY path is active |
| ADC experiment | A4/A5/A6/A7/A8 connector options | medium | ST-UM2407 | Early experiment only, not modem baseline |
| Ethernet pins | RMII pins connected to PHY by default | high | ST-UM2407 | Avoid for M2 USB/audio/PTT work |
| 32 kHz oscillator | PC14/PC15 path | high | ST-UM2407 | Avoid for GPIO use |
| Main clock pins | PF0/PH0 and PF1/PH1 clock path options | high | ST-UM2407 | Avoid until clock plan is reviewed |
| Watchdog | Independent watchdog planning | medium | ST-STM32H753ZI, ST-RM0433 | No hardware watchdog setup in M2.11 |
| Reset cause | Reset-cause reporting path | medium | ST-STM32H753ZI, ST-RM0433 | No register reads in M2.11 |

## M2.11 Decisions

- USB FS through CN13 is the first USB KISS planning path.
- Test PTT GPIO remains `TBD`.
- Audio resource remains `TBD`.
- No pin assignments are verified for firmware use.
- No concrete target pin macros are added.
- No target resource may be treated as final until checked against the exact board revision schematic.
