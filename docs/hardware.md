# Hardware

This file defines the first hardware block diagram in text form. It does not start schematic or PCB work.

## Hardware Goals

- Use an external audio codec for the main RX/TX audio path.
- Support at least 48 kHz sample rate and at least 16-bit audio.
- Keep analog filtering and level control close to the radio connector and codec.
- Keep PTT fail-safe in hardware and firmware.
- Add ESD and line protection at every external connector.
- Prefer a 4-layer KiCad rev A PCB after documentation review.

## Block Diagram

```text
USB-C device port
	|
	| ESD protection, CC resistors, power filtering
	v
3V3/codec rails and power supervision
	|
	v
STM32H743/STM32H753 class MCU
	|-- USB CDC ACM KISS
	|-- optional diagnostic CDC
	|-- SAI/I2S audio to codec
	|-- I2C/SPI codec control
	|-- GPIO PTT, COS, LEDs, jumpers
	|-- timers, DMA, watchdogs
	|
	v
External stereo audio codec
	|-- RX ADC path
	|-- TX DAC path
	|
	v
Analog front end
	|-- RX anti-alias filter
	|-- selectable RX gain paths
	|-- TX reconstruction filter
	|-- selectable TX level paths for mic input radios
	|-- selectable TX level paths for data input radios
	|-- AC/DC coupling options where useful
	|
	v
Radio interfaces
	|-- 6-pin mini-DIN data port
	|-- optional DE-9 or terminal block adapter
	|-- PTT open-drain or opto-isolated output
	|-- COS/squelch/DCD protected input
	|-- optional CAT header or USB CDC CAT bridge
	|-- test pads for RX audio, TX audio, PTT, COS, GND, 3V3, 5V
```

## MCU Target

Primary target: STM32H743/STM32H753 class.

The current selection is a class target, not a final part number. Package, stock, board assembly limits, and available dev boards must be checked before schematic work.

Required MCU features:

- Enough CPU margin for fixed-point or floating-point modem DSP.
- RAM margin for DMA rings, packet queues, counters, and memory pools.
- USB device support for CDC ACM and future composite USB options.
- SAI/I2S for external codec audio.
- DMA with bounded interrupt service.
- Timers for audio clocks, PTT watchdog, and modem timing.
- Independent watchdog and reset cause reporting.
- Brownout reset support.
- CRC engine if useful for settings and test acceleration.

STM32H753 may be preferred if crypto acceleration or security services become useful, but no native modem feature depends on crypto hardware.

Secondary target: RP2350 for experiments only. It may be used for portable module tests if the design does not depend on STM32-only APIs. It is not the baseline for the first full native hardware TNC.

## Audio Codec Candidates

Candidate class:

- Stereo codec.
- I2S or compatible serial audio.
- I2C or SPI control.
- At least 48 kHz.
- At least 16-bit samples.
- Programmable input gain or enough inputs to support selectable gain paths.

Initial candidates:

| Part            | Reason to evaluate                                  |
| --------------- | --------------------------------------------------- |
| TLV320AIC3204   | 192 kHz max sample rate, I2S, programmable I/O      |
| TLV320AIC3104   | 96 kHz max sample rate, I2S, multiple analog I/O    |
| WM8731 class    | Common hobbyist codec class, verify availability    |

No codec is selected until dev-board audio tests and availability checks are complete.

## Radio Interface Requirements

6-pin mini-DIN data port first:

| Signal     | Direction | Requirement                                  |
| ---------- | --------- | -------------------------------------------- |
| RX audio   | Input     | Protected, filtered, selectable gain         |
| TX audio   | Output    | Filtered, level-limited, selectable level    |
| PTT        | Output    | Open-drain or opto-isolated, fail-safe off   |
| COS/DCD    | Input     | Protected input, threshold strategy reviewed |
| GND        | Common    | Low-impedance return with layout control     |
| Data audio | Varies    | Radio-specific pinout verified per adapter   |

Adapter options:

- DE-9 adapter board.
- Terminal block adapter.
- Radio-specific cable harnesses.

## Power and Protection

Required:

- USB power input.
- USB-C ESD protection near connector.
- Radio connector ESD protection near connector.
- Ferrite or common-mode filtering on noisy external lines where tests justify it.
- Brownout reset enabled.
- External supervisor if MCU reset behavior or rail timing tests show a gap.
- Hardware PTT pull state that defaults TX off.
- PTT watchdog circuit or independent cutoff path if practical.

External power is deferred. If added later, it needs reverse protection, overcurrent protection, input filtering, and clear ground strategy.

## PCB Rules for Rev A

- KiCad project only after M0 review.
- 4-layer board preferred.
- Continuous ground plane.
- Analog and digital placement zones.
- Codec near analog filtering and connector area.
- ESD parts close to connectors.
- Short SAI/I2S traces.
- Labelled jumpers and test pads.
- Test pads for RX audio, TX audio, PTT, COS, GND, 3V3, 5V.

## Preliminary BOM Classes

This is not a purchasing BOM.

| Class              | Examples to evaluate                         |
| ------------------ | -------------------------------------------- |
| MCU                | STM32H743 or STM32H753 package and dev board |
| Audio codec        | TLV320AIC3204, TLV320AIC3104, WM8731 class   |
| USB protection     | USB-C ESD array, CC resistors, polyfuse      |
| Power              | 3V3 regulator, ferrites, supervisor          |
| Analog RX          | op-amp, resistor ladder, filter capacitors   |
| Analog TX          | op-amp or buffer, attenuator, filter parts   |
| Isolation          | optocoupler or open-drain PTT transistor     |
| Connectors         | USB-C, mini-DIN 6, headers, test pads        |
| Indicators         | LEDs for power, USB, RX, TX, DCD, fault      |

## Shopping List for Prototyping

- STM32H743 or STM32H753 dev board with USB and audio-capable SAI/I2S pins exposed.
- External codec breakout or eval board matching one candidate codec.
- 6-pin mini-DIN breakout or connector samples.
- USB-C ESD/protection samples for later schematic evaluation.
- Op-amp assortment suitable for single-supply audio at 3V3 or 5V.
- Optocoupler and logic MOSFET samples for PTT tests.
- Bench radio cable parts and dummy audio load.
- Logic analyzer for USB/UART/GPIO timing.
- Audio interface or oscilloscope for level and filter checks.

## Sources checked

| Source title                                  | Date checked | Note                                      |
| --------------------------------------------- | ------------ | ----------------------------------------- |
| STM32H743BG product page, STMicroelectronics  | 2026-05-26   | CPU, RAM, USB, SAI/I2S, memory features   |
| STM32H753BI product page, STMicroelectronics  | 2026-05-26   | H753 class and crypto-capable variant     |
| RP2350 documentation, Raspberry Pi            | 2026-05-26   | Experimental MCU feature context          |
| TLV320AIC3204 product page, Texas Instruments | 2026-05-26   | Codec sample rate, I2S, programmable I/O  |
| TLV320AIC3104 product page, Texas Instruments | 2026-05-26   | Codec sample rate, I2S, analog I/O        |

