# Sources

Sources checked on 2026-05-26.

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
| RP2350 documentation, Raspberry Pi            | 2026-05-26   | https://www.raspberrypi.com/documentation/microcontrollers/rp2350.html  | RP2350 experimental target features      |
| TLV320AIC3204 product page, Texas Instruments | 2026-05-26   | https://www.ti.com/product/TLV320AIC3204                                | Codec sample rate, I2S, programmable I/O |
| TLV320AIC3104 product page, Texas Instruments | 2026-05-26   | https://www.ti.com/product/TLV320AIC3104                                | Codec sample rate, I2S, analog I/O       |

## Host Platforms

| Source title                         | Date checked | URL                                                            | Note                                  |
| ------------------------------------ | ------------ | -------------------------------------------------------------- | ------------------------------------- |
| ALSA PCM interface documentation     | 2026-05-26   | https://www.alsa-project.org/alsa-doc/alsa-lib/pcm.html        | Linux PCM audio backend planning      |
| sndio project page                   | 2026-05-26   | https://sndio.org/                                             | sndio platform scope                  |
| sndio OpenBSD manual                 | 2026-05-26   | https://man.openbsd.org/sndio                                  | OpenBSD audio backend planning        |
| FreeBSD Architecture Handbook, Sound | 2026-05-26   | https://docs.freebsd.org/en/books/arch-handbook/sound/         | FreeBSD pcm and OSS interface context |
| NetBSD audio(4) manual               | 2026-05-26   | https://man.netbsd.org/NetBSD-10.0/audio.4                     | NetBSD audio device interface context |

## File Formats

| Source title                                  | Date checked | URL                                                                                  | Note                                  |
| --------------------------------------------- | ------------ | ------------------------------------------------------------------------------------ | ------------------------------------- |
| Microsoft RIFF documentation                  | 2026-05-26   | https://learn.microsoft.com/en-us/windows/win32/xaudio2/resource-interchange-file-format--riff- | RIFF/WAVE chunk layout                |
| Microsoft waveform audio data types reference | 2026-05-26   | https://learn.microsoft.com/is-is/windows/win32/multimedia/devices-and-data-types    | 16-bit PCM sample representation      |

## Use Restrictions

Dire Wolf may be used as an external interoperability reference and test tool only. No Dire Wolf source code is copied into this repository in this pass.
