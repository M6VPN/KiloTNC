# M2 Board Abstraction Plan

M2 board planning keeps the STM32H753 Nucleo path useful while leaving room for related STM32H7 parts and later custom boards.

## Scope

This document is planning only.

- No board driver exists.
- No pin initialization exists.
- No STM32 HAL, CMSIS, TinyUSB, or STM32Cube code is imported.
- No hardware register access is added.
- No PCB work starts here.

## Target Identity Layers

Future embedded target identity should be split into these layers:

| Layer             | Example for current path        | Purpose                         |
| ----------------- | ------------------------------- | ------------------------------- |
| MCU class         | STM32H7                         | Shared family-level assumptions |
| MCU part          | STM32H753ZI                     | Memory and peripheral limits    |
| Board             | NUCLEO-H753ZI Nucleo-144        | Connectors and debug path       |
| Hardware revision | TBD                             | Board-specific wiring changes   |
| Feature flags     | USB, watchdog, audio, test PTT  | Compile-time capability checks  |

The current metadata in `embedded/targets/stm32h753-nucleo/` records only planning flags. It does not select pins or initialize hardware.

## Why Board Abstraction Matters

STM32H753 remains the flagship target, but H743, H735, and H750 should stay possible through clear boundaries.

The abstraction must prevent these from being mixed together:

- MCU memory and peripheral limits.
- MCU clock, reset, and watchdog constraints.
- Board connector and solder-bridge choices.
- USB stack choice.
- Audio adapter choice.
- PTT GPIO safety policy.
- Diagnostic output path.
- Optional network companion role.

This keeps a future custom H753/H743 Rev A board from being tied to Nucleo-only details. It also prevents a later H735 or H750 cost-reduction path from silently inheriting unsafe assumptions.

## Rev A Direction

Custom Rev A should remain USB/audio/PTT first.

Rev A should not require:

- Ethernet.
- Wi-Fi.
- ESP32-S3 companion.
- External network service mode.
- RF transmit validation before safety gates.

Ethernet, Wi-Fi, and companion connectivity remain later tracks after the USB/audio/PTT path is proven.

## Metadata Boundary

Allowed now:

- Target name.
- MCU family group.
- Board family string.
- Planned feature flags.
- Compatibility notes.
- `TBD` placeholders for unverified resources.
- Clock, reset, boot, and watchdog planning flags.

Not allowed now:

- Pin assignments.
- Alternate-function selections.
- Clock tree setup.
- PLL values.
- Reset register reads.
- Watchdog register values.
- HAL init code.
- USB descriptors bound to a stack.
- PTT GPIO driver code.
- Codec or audio peripheral driver code.

## Current Metadata

The `stm32h753-nucleo` target metadata records:

- STM32H753 as flagship.
- STM32H7 as family group.
- Nucleo-144 as board family.
- H743 possible for a first custom serious board path.
- H735 possible only after resource validation.
- H750 possible only after memory and flash validation.
- No connectivity companion present on the target.
- Pin assignments still unverified.
