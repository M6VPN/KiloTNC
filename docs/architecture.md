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

The daemon, network node, and network-capable hardware targets are planned only. They are not implemented in M1.12.

## MCU Decision

Primary target: STM32H743/STM32H753 class.

Reasons:

- Cortex-M7 at up to 480 MHz gives enough margin for modem DSP, USB, diagnostics, and safety tasks.
- Up to 1 MB RAM supports fixed-size audio buffers, packet queues, memory pools, and diagnostics without dynamic allocation in the audio path.
- USB OTG FS and HS options support USB CDC ACM and later composite USB designs.
- SAI/I2S support fits external audio codecs.
- ADC/DAC are available for test hooks or fallback paths, but the main design prefers an external codec.
- DMA, timers, independent watchdog, window watchdog, CRC engine, and brownout/reset features fit the reliability goals.
- STM32H753 adds crypto acceleration, but no design requirement depends on it.

Secondary experimental target: RP2350.

RP2350 can be used only if firmware modules stay portable and no capability is documented unless the RP2350 design can meet it with real peripherals or verified external parts. It is useful for host-side experiments and low-cost prototypes, but it has less CPU and peripheral margin than STM32H7 for the full native target.

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
| STM32H743BG product page, STMicroelectronics      | 2026-05-26   | MCU CPU, RAM, USB, SAI/I2S, memory features  |
| STM32H753BI product page, STMicroelectronics      | 2026-05-26   | MCU class and crypto-capable H753 variant    |
| RP2350 documentation, Raspberry Pi                | 2026-05-26   | RP2350 CPU, USB, and experimental target fit |
| TLV320AIC3204 product page, Texas Instruments     | 2026-05-26   | External codec capability candidate          |
| The KISS TNC, Chepponis and Karn                  | 2026-05-26   | Host/TNC split and graceful packet drops     |
| IL2P Specification Draft v0.6                     | 2026-05-26   | IL2P relationship to KISS and AX.25          |
| ARDOP Specification Revision 0.3.1                | 2026-05-26   | External modem bridge and PTT/CAT context    |
| Mercury GitHub README, Rhizomatica                | 2026-05-26   | Mercury external modem and TCP interface     |
