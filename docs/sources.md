# Sources

Sources checked through 2026-05-27.

## Protocols

| Source title                                             | Date checked | URL                                                                                                      | Note                                                |
| -------------------------------------------------------- | ------------ | -------------------------------------------------------------------------------------------------------- | --------------------------------------------------- |
| The KISS TNC, Chepponis and Karn                         | 2026-05-26   | https://www.ka9q.net/papers/kiss.html                                                                   | KISS framing, command byte, commands, drop behavior |
| AX.25 Link Access Protocol v2.2, TAPR/ARRL               | 2026-05-26   | https://hamgate.ampr.org/docs/AX25.2.2-Jul%2098-2.pdf                                                   | AX.25 fields, HDLC bit-stuffing, FCS, UI frames     |
| APRS Protocol Reference 1.0.1, APRS Working Group        | 2026-05-26   | https://web.tapr.org/software_library/aprs/aprsspec/spec/aprs101/APRS101.pdf                            | APRS over AX.25 context                             |
| Clarifying the Amateur Bell 202 Modem, TAPR DCC 2014     | 2026-05-26   | https://tapr.org/wp-content/uploads/DCC2014-Amateur-Bell-202-Modem-W6KWF-and-Bridget-Benson.pdf         | Amateur Bell 202/APRS modem context                 |
| IL2P Specification Draft v0.6                            | 2026-05-26   | https://tarpn.net/t/il2p/il2p-specification_draft_v0-6.pdf                                              | IL2P FEC, scrambling, sync word, CRC, AX.25 mapping |
| TARPN IL2P overview                                      | 2026-05-26   | https://tarpn.net/t/il2p/il2p.html                                                                       | IL2P spec link and revision notes                   |
| OARC NinoTNC page                                        | 2026-05-26   | https://wiki.oarc.uk/packet%3Aninotnc                                                                    | NinoTNC modes and SETHW mode control notes          |
| TARPN NinoTNC N9600A Operator Manual                     | 2026-05-26   | https://tarpn.net/t/nino-tnc/n9600a/n9600a_operation.html                                                | NinoTNC modes, SETHW +16 behavior, compatibility    |
| TARPN Protocols and Modulation page                      | 2026-05-26   | https://tarpn.net/t/builder/builders_tarpn_protocols.html                                               | NinoTNC modulation and bitrate overview             |
| FX.25 Forward Error Correction page                      | 2026-05-26   | https://en.wikipedia.org/wiki/FX.25_Forward_Error_Correction                                            | FX.25 research context and archived spec pointer    |

## External Modems

| Source title                       | Date checked | URL                                                                              | Note                                            |
| ---------------------------------- | ------------ | -------------------------------------------------------------------------------- | ----------------------------------------------- |
| VARA Modem official site           | 2026-05-26   | https://varamodem.com/                                                           | VARA software modem, modes, license model       |
| ARDOP Specification Revision 0.3.1 | 2026-05-26   | https://www.winlink.org/sites/default/files/downloads/_ardop_specification.pdf   | ARDOP protocol, host interface, PTT/CAT context |
| Mercury GitHub README, Rhizomatica | 2026-05-26   | https://github.com/Rhizomatica/mercury                                           | Mercury TCP TNC interface and radio/audio modes |
| HERMES system documentation        | 2026-05-26   | https://wiki.hermes.radio/index.php?title=System_Documentation                   | Mercury as HERMES modem option                  |

## Hardware

| Source title                                  | Date checked | URL                                                                     | Note                                     |
| --------------------------------------------- | ------------ | ----------------------------------------------------------------------- | ---------------------------------------- |
| STM32H743BG product page, STMicroelectronics  | 2026-05-26   | https://www.st.com/en/microcontrollers-microprocessors/stm32h743bg.html | CPU, RAM, USB, SAI/I2S, memory features  |
| STM32H753BI product page, STMicroelectronics  | 2026-05-26   | https://www.st.com/en/microcontrollers-microprocessors/stm32h753bi.html | H753 class and crypto-capable variant    |
| NUCLEO-H753ZI product page, STMicroelectronics | 2026-05-27   | https://www.st.com/en/evaluation-tools/nucleo-h753zi.html               | Nucleo-144 board status, ST-LINK, connector, and schematic-pack references |
| STM32H753ZI product page, STMicroelectronics  | 2026-05-27   | https://www.st.com/en/microcontrollers-microprocessors/stm32h753zi.html | H753ZI memory, USB, timer, watchdog, ADC, DAC, SAI, and I2S class |
| STM32H743ZI product page, STMicroelectronics  | 2026-05-27   | https://www.st.com/en/microcontrollers-microprocessors/stm32h743zi.html | H743ZI memory and future custom board planning |
| STM32H735ZG product page, STMicroelectronics  | 2026-05-27   | https://www.st.com/en/microcontrollers-microprocessors/stm32h735zg.html | H735ZG cost-reduced candidate flash and RAM planning |
| STM32H750 Value line page, STMicroelectronics | 2026-05-27   | https://www.st.com/en/microcontrollers-microprocessors/stm32h750-value-line.html | H750 flash, RAM, and memory planning caution |
| STM32H753xI datasheet, STMicroelectronics     | 2026-05-27   | https://www.st.com/resource/en/datasheet/stm32h753zi.pdf                | Future pin and peripheral verification |
| STM32H7 Nucleo-144 boards user manual, ST     | 2026-05-27   | https://www.st.com/resource/en/user_manual/um2407-stm32h7-nucleo144-board-stmicroelectronics.pdf | NUCLEO-H753ZI board connectors, LEDs, USB OTG FS, VCP, Ethernet conflicts, and solder bridges |
| STM32H743/753 documentation page, STMicroelectronics | 2026-05-27 | https://www.st.com/en/microcontrollers-microprocessors/stm32h743-753/documentation.html | RM0433 reference manual listing for future peripheral behavior checks |
| RM0433 STM32H743/753 and STM32H750 reference manual, STMicroelectronics | 2026-05-27 | https://www.st.com/resource/en/reference_manual/rm0433-stm32h743-753-and-stm32h750-value-line-advanced-arm-based-32-bit-mcus-stmicroelectronics.pdf | Clock, reset, RCC reset status, and watchdog planning |
| MB1364-H753ZI-C01 schematic, STMicroelectronics | 2026-05-27 | https://www.st.com/resource/en/schematic_pack/mb1364-h753zi-c01-schematic.pdf | Future board-revision resource verification |
| MB1364-H753ZI-E01 schematic, STMicroelectronics | 2026-05-27 | https://www.st.com/resource/en/schematic_pack/mb1364-h753zi-e01-schematic.pdf | Future board-revision resource verification |
| NUCLEO-H743ZI2 CubeIDE community report, ST   | 2026-05-26   | https://community.st.com/t5/stm32cubeide-mcus/nucleo-h743zi2-board-not-supported-in-stm32cubeide-obsolete/td-p/823344 | H743ZI2 obsolete-status risk note |
| RP2350 documentation, Raspberry Pi            | 2026-05-26   | https://www.raspberrypi.com/documentation/microcontrollers/rp2350.html  | RP2350 experimental target features      |
| Pico-series documentation, Raspberry Pi       | 2026-05-26   | https://www.raspberrypi.com/documentation/microcontrollers/raspberry-pi-pico.html | Pico 2 board features                    |
| RP2350 datasheet, Raspberry Pi                | 2026-05-26   | https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf          | RP2350 USB and PIO details               |
| ESP32-S3 product page, Espressif              | 2026-05-27   | https://www.espressif.com/en/products/socs/esp32-s3                    | Connectivity companion role planning     |
| TLV320AIC3204 product page, Texas Instruments | 2026-05-26   | https://www.ti.com/product/TLV320AIC3204                                | Codec sample rate, I2S, programmable I/O |
| TLV320AIC3104 product page, Texas Instruments | 2026-05-26   | https://www.ti.com/product/TLV320AIC3104                                | Codec sample rate, I2S, analog I/O       |

## Host Platforms

| Source title                         | Date checked | URL                                                            | Note                                  |
| ------------------------------------ | ------------ | -------------------------------------------------------------- | ------------------------------------- |
| ALSA PCM interface documentation     | 2026-05-26   | https://www.alsa-project.org/alsa-doc/alsa-lib/pcm.html        | Linux PCM audio backend planning      |
| sndio project page                   | 2026-05-26   | https://sndio.org/                                             | sndio platform scope                  |
| sndio OpenBSD manual                 | 2026-05-26   | https://man.openbsd.org/sndio                                  | OpenBSD audio backend planning        |
| OpenBSD sio_open manual              | 2026-05-26   | https://man.openbsd.org/sio_open                               | sndio audio API planning              |
| OpenBSD posix_openpt manual          | 2026-05-26   | https://man.openbsd.org/man3/posix_openpt.3                    | PTY master allocation API             |
| OpenBSD ptsname manual               | 2026-05-26   | https://man.openbsd.org/man3/ptsname.3                         | PTY slave path and unlock flow        |
| FreeBSD Architecture Handbook, Sound | 2026-05-26   | https://docs.freebsd.org/en/books/arch-handbook/sound/         | FreeBSD pcm and OSS interface context |
| NetBSD audio(4) manual               | 2026-05-26   | https://man.netbsd.org/NetBSD-10.0/audio.4                     | NetBSD audio device interface context |

## USB Stack Planning

| Source title                              | Date checked | URL                                                                                                      | Note                                               |
| ----------------------------------------- | ------------ | -------------------------------------------------------------------------------------------------------- | -------------------------------------------------- |
| TinyUSB documentation                     | 2026-05-27   | https://docs.tinyusb.org/                                                                                | Device stack, CDC support, STM32 H7 support table  |
| TinyUSB FAQ                               | 2026-05-27   | https://docs.tinyusb.org/en/latest/faq.html                                                              | MCU family support and MIT license                 |
| TinyUSB supported boards reference        | 2026-05-27   | https://docs.tinyusb.org/en/latest/reference/boards.html                                                 | BSP boundary for examples and tests                |
| ST Introduction to USB with STM32         | 2026-05-27   | https://wiki.st.com/stm32mcu/wiki/Introduction_to_USB_with_STM32                                         | STM32 USB device library and driver layering       |
| STM32CubeH7 getting started user manual   | 2026-05-27   | https://www.st.com/resource/en/user_manual/dm00386433-getting-started-with-stm32cubeh7-for-stm32h7-series-stmicroelectronics.pdf | STM32CubeH7 middleware includes USB Host and Device libraries |
| STM32 USBX middleware repository          | 2026-05-27   | https://github.com/STMicroelectronics/stm32-mw-usbx                                                      | STM32Cube USBX middleware and version consistency  |
| STM32H7 Nucleo-144 boards user manual, ST | 2026-05-27   | https://www.st.com/resource/en/user_manual/um2407-stm32h7-nucleo144-board-stmicroelectronics.pdf         | CN1 STLINK USB and CN13 USB OTG FS connector       |
| Arm MDK USB CDC ACM class reference       | 2026-05-27   | https://arm-software.github.io/MDK-Middleware/latest/USB/USB_Classes.html                                | CDC ACM interface and endpoint role planning       |

## File Formats

| Source title                                  | Date checked | URL                                                                                  | Note                                  |
| --------------------------------------------- | ------------ | ------------------------------------------------------------------------------------ | ------------------------------------- |
| Microsoft RIFF documentation                  | 2026-05-26   | https://learn.microsoft.com/en-us/windows/win32/xaudio2/resource-interchange-file-format--riff- | RIFF/WAVE chunk layout                |
| Microsoft waveform audio data types reference | 2026-05-26   | https://learn.microsoft.com/is-is/windows/win32/multimedia/devices-and-data-types    | 16-bit PCM sample representation      |

## Use Restrictions

Dire Wolf may be used as an external interoperability reference and test tool only. No Dire Wolf source code is copied into this repository in this pass.
