# Architecture

KiloTNC is split into a real-time embedded TNC path and optional host-side bridge paths. Native firmware owns KISS framing, packet queues, modem DSP, radio control, diagnostics, and fail-safe recovery. Bridge modes use a host computer for external modems and keep KiloTNC in an audio/PTT/CAT interface role.

## Goals

- Keep PTT fail-safe under boot, reset, watchdog fault, USB disconnect, queue overflow, and firmware fault.
- Keep AX.25/HDLC/FCS handling separate from modem DSP.
- Keep IL2P/IL2Pc separate from AX.25 on-air compatibility.
- Keep external proprietary or GPL software out of the firmware and repo source tree.
- Require review of protocol and architecture docs before schematic or PCB work starts.

## Native System Path

```text
Host application
	|
	| USB CDC ACM KISS
	v
KISS parser and command handler
	|
	| fixed-size packet queues
	v
Packet layer
	|-- AX.25 UI frame encode/decode
	|-- FCS and HDLC framing
	|-- IL2P/IL2Pc research encoder/decoder
	|
	v
Modem DSP
	|-- 1200 baud Bell 202 AFSK
	|-- 300 baud AFSK research path
	|-- 9600 baud G3RUH-like/GFSK research path
	|
	v
Codec I2S/SAI DMA audio path
	|
	v
Analog filtering, level control, PTT, COS, radio connector
```

## Bridge Path

```text
Host software modem
	|-- VARA external modem
	|-- ARDOP external modem
	|-- Mercury external modem
	|
	| USB audio, USB CDC diagnostics, CAT/PTT where supported
	v
KiloTNC bridge firmware
	|
	v
Codec audio path, PTT safety, CAT bridge, radio connector
```

VARA native modulation is not planned. VARA support means host-side external software using KiloTNC audio, PTT, and CAT facilities where the host software supports that arrangement.

## Layered Architecture

```text
Applications and tests
	|
	| KISS, files, CLI, local IPC, future USB CDC
	v
Target adapters
	|-- Host daemon adapters: file, TCP, Unix socket, PTY, raw PCM
	|-- Future MCU adapters: USB CDC, timers, watchdog, GPIO, audio
	|-- Future hardware board: codec, PTT, COS, radio connector
	|
	v
Portable core
	|-- KISS
	|-- AX.25, HDLC, FCS
	|-- AFSK1200 RX/TX
	|-- TNC control, diagnostics, modes
```

M1 host daemon code is not MCU firmware. It validates the portable core and host-side adapter boundaries. M2 starts separate dev-board firmware adapters around the same portable core.

M2.0 embedded architecture boundary:

```text
Portable core
	|
	v
Embedded app glue
	|
	v
Platform adapters
	|-- clock and tick adapter
	|-- GPIO and test-only PTT adapter
	|-- watchdog and reset adapter
	|-- USB CDC byte-stream adapter
	|-- audio loopback or test adapter
	|-- diagnostics adapter
```

M2.1 adds the first embedded compile-only skeleton and host-native platform stub test. M2.2 adds host-native platform tick, watchdog, reset-cause, diagnostics, and GPIO/PTT test stubs. M2.3 adds a host-native USB CDC byte-stream stub and KISS echo/loopback bridge. M2.4 adds a host-native embedded diagnostics bridge for app, platform, USB, and KISS counters. It does not implement a real USB stack, TinyUSB, STM32 HAL, descriptors, endpoint code, or hardware adapters.

Before embedded use, adapter boundaries need review for:

- No heap allocation in core modem, packet, and control paths.
- Fixed-size queues and buffers at platform boundaries.
- Clear ownership of timing ticks, watchdog service, and PTT fail-safe state.
- Diagnostics that can be exposed without file or socket APIs.

## Shared Core and Platform Targets

The portable core is shared by firmware, host tools, and planned daemon work:

- KISS parser and encoder.
- AX.25, HDLC, and FCS.
- AFSK1200 RX/TX.
- TNC1200 host harness.
- Mode registry.
- Diagnostics.
- Channel-access and PTT safety simulation.

Target-specific adapters are planned around that core:

- MCU firmware target: USB CDC KISS, codec audio, PTT, COS, CAT, watchdogs, and fail-safe operation on physical hardware.
- Linux/BSD daemon target: host audio, host KISS services, serial/CAT/PTT adapters, and diagnostics without KiloTNC hardware.
- Network/node target: optional local KISS-over-TCP, future node services, and remote diagnostics behind explicit safety gates.
- Future network-capable hardware target: Ethernet or Wi-Fi variants after the Rev A USB/audio/PTT design is proven.

The daemon groundwork is host-side only. The MCU firmware target starts in M2 and must not depend on host daemon file, socket, PTY, or raw-file audio code.

## MCU Decision

Primary M2.0 target path: `stm32h753-nucleo`, based on NUCLEO-H753ZI or a current equivalent STM32H753 Nucleo-144 board.

Reasons:

- Cortex-M7 at up to 480 MHz gives enough margin for modem DSP, USB, diagnostics, and safety tasks.
- Up to 1 MB RAM supports fixed-size audio buffers, packet queues, memory pools, and diagnostics without dynamic allocation in the audio path.
- USB OTG FS and HS options support USB CDC ACM and later composite USB designs.
- SAI/I2S support fits external audio codecs.
- ADC/DAC are available for test hooks or fallback paths, but the main design prefers an external codec.
- DMA, timers, independent watchdog, window watchdog, CRC engine, and brownout/reset features fit the reliability goals.
- The NUCLEO-H753ZI product page currently lists the board as active and in volume production.
- STM32CubeIDE is available locally, but normal host builds and M2.0 docs do not require proprietary IDE output.

Secondary experimental target: RP2350.

RP2350/Pico 2 can be used only if firmware modules stay portable and no capability is documented unless the RP2350 design can meet it with real peripherals or verified external parts. It is useful for portability and low-cost experiments, but it has less CPU and peripheral margin than STM32H7 for the full native target.

## Major Modules

| Module          | Responsibility                                      |
| --------------- | --------------------------------------------------- |
| USB             | CDC ACM KISS, diagnostic CDC, future USB audio      |
| KISS            | Framing, escaping, commands, parser recovery        |
| Packet          | AX.25, HDLC, FCS, IL2P/IL2Pc research boundaries    |
| Modem           | AFSK, G3RUH-like/GFSK, timing recovery, DCD         |
| Audio           | Codec control, DMA rings, sample-rate discipline    |
| Radio I/O       | PTT, COS, CAT bridge, connector protection          |
| Config          | Versioned settings, CRC, rollback                   |
| Diagnostics     | Counters, CLI, LEDs, fault records                  |
| Safety          | Watchdog quorum, PTT timeout, boot/reset TX inhibit |
| Test Harness    | WAV vectors, loopback, fuzzing, bench validation    |

## Fault Policy

- TX is off on boot, reset, watchdog fault, and USB disconnect unless explicitly configured otherwise.
- PTT has a hardware and firmware timeout.
- Queue overflow drops the offending packet and increments a counter.
- Malformed host input never reaches the real-time audio path unchecked.
- Watchdog heartbeat requires progress from the main loop, USB task, audio task, and PTT safety task.

## Stage Gate

PCB design starts only after:

- Protocol matrix review.
- Hardware block diagram review.
- Firmware safety rules review.
- Test plan review.

## Sources checked

| Source title                                      | Date checked | Note                                         |
| ------------------------------------------------- | ------------ | -------------------------------------------- |
| NUCLEO-H753ZI product page, STMicroelectronics    | 2026-05-26   | Board status, Nucleo-144 features, ST-LINK   |
| STM32H753ZI product page, STMicroelectronics      | 2026-05-26   | MCU CPU, RAM, USB, and peripheral class      |
| STM32H753xI datasheet, STMicroelectronics         | 2026-05-26   | USB, SAI, I2S-capable SPI, memory details    |
| STM32H7 Nucleo-144 board user manual, ST          | 2026-05-26   | Nucleo board family and H753 board details   |
| STM32H743BG product page, STMicroelectronics      | 2026-05-26   | MCU CPU, RAM, USB, SAI/I2S, memory features  |
| STM32H753BI product page, STMicroelectronics      | 2026-05-26   | MCU class and crypto-capable H753 variant    |
| Pico-series documentation, Raspberry Pi           | 2026-05-26   | Pico 2 board CPU, RAM, USB, and GPIO details |
| RP2350 datasheet, Raspberry Pi                    | 2026-05-26   | RP2350 USB and PIO details                   |
| TLV320AIC3204 product page, Texas Instruments     | 2026-05-26   | External codec capability candidate          |
| The KISS TNC, Chepponis and Karn                  | 2026-05-26   | Host/TNC split and graceful packet drops     |
| IL2P Specification Draft v0.6                     | 2026-05-26   | IL2P relationship to KISS and AX.25          |
| ARDOP Specification Revision 0.3.1                | 2026-05-26   | External modem bridge and PTT/CAT context    |
| Mercury GitHub README, Rhizomatica                | 2026-05-26   | Mercury external modem and TCP interface     |
