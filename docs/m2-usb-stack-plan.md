# M2 USB Stack Plan

M2.13 records the future USB CDC stack direction for the `stm32h753-nucleo` target path.

## Scope

M2.13 chooses a strategy only.

- No USB stack is imported.
- No USB descriptors are used by firmware.
- No real USB hardware is touched.
- No TinyUSB, STM32Cube, CMSIS, HAL, or vendor-generated project is added.
- No flashable firmware is produced.

## Requirements

Future USB CDC KISS support needs:

- USB CDC ACM device interface.
- KISS byte stream over CDC.
- Optional separate diagnostics CDC interface later.
- Safe behavior on USB disconnect.
- PTT off on disconnect.
- Malformed host input must not break watchdog or PTT safety.
- No dynamic allocation in the real-time path.
- Bounded USB RX and TX queues.

## Candidate Stacks

| Stack | Benefits | Risks | Boundary |
| ----- | -------- | ----- | -------- |
| TinyUSB | Portable device stack, CDC support, STM32 H7 support, RP2040/RP2350 family relevance, MIT license | Requires project integration, descriptors, board port, and regular task servicing | Keep outside repo, adapter under `embedded/platform/usb_*`, path by `TINYUSB_PATH` later |
| STM32Cube USB Device middleware | ST-supported STM32 path, fits STM32CubeH7 package model, CDC examples and middleware path exist | Tighter STM32/HAL coupling, weaker fit for RP2350 experiments, version consistency with Cube packages matters | Keep outside repo, adapter under `embedded/platform/usb_*`, path by `STM32CUBE_H7_PATH` later |
| Custom or minimal stack | Full control and small surface | High maintenance, USB compliance risk, not portable, likely distracts from TNC work | Research only, no implementation unless forced |

## Recommendation

Use TinyUSB as the future first USB CDC stack path if licensing and target integration remain acceptable.

Keep STM32Cube USB Device middleware as a fallback and reference path if TinyUSB integration proves unsuitable for the selected STM32H753 board.

Do not write a custom USB device stack unless both maintained stack options are blocked.

This is a planning recommendation only. M2.13 does not import or build either real stack.

## Dependency Boundary

External dependencies stay outside this repository.

Future dependency paths should be set with environment variables:

```text
TINYUSB_PATH=/path/to/tinyusb
STM32CUBE_H7_PATH=/path/to/STM32CubeH7
KILOTNC_USB_STACK=tinyusb
```

Future USB adapter files should live under `embedded/platform/usb_*`. The portable embedded USB byte-stream interface remains `kilotnc_usb_cdc`.

M2.13 adds only `usb_stack_boundary`, which recognizes planned stack names and reports that only the host-test stub path is implemented.

## Descriptor Planning

Planned descriptor direction:

- One CDC ACM interface for KISS.
- Optional second CDC ACM interface for diagnostics later.
- Stable manufacturer, product, and serial string policy later.
- No final VID or PID claim in M2.13.
- No generated descriptor source file in M2.13.

## Safety

- USB host input cannot directly key PTT.
- Future USB disconnect handling must force safe TX or idle state.
- Watchdog behavior remains independent of USB.
- USB RX and TX buffers must remain bounded.
- Malformed KISS input over USB must be counted and recovered without unsafe state changes.

## M2.13 Decision

- Selected future first path: TinyUSB adapter boundary.
- Fallback path: STM32Cube USB Device adapter boundary.
- Research-only path: custom or minimal stack.
- Implemented now: USB CDC stub only.
- Not implemented now: TinyUSB, STM32Cube USB Device, descriptors, endpoint handlers, interrupts, hardware access, or board flashing.

## Sources Checked

Sources checked on 2026-05-27:

- TinyUSB documentation: https://docs.tinyusb.org/
  - Device stack includes CDC support.
  - Supported CPU table lists STM32 H7 family support.
- TinyUSB FAQ: https://docs.tinyusb.org/en/latest/faq.html
  - Lists STM32 and RP2040 family support and MIT license.
- TinyUSB supported boards reference: https://docs.tinyusb.org/en/latest/reference/boards.html
  - Board support packages are example and test support, not required when integrating TinyUSB into a larger project.
- ST introduction to USB with STM32: https://wiki.st.com/stm32mcu/wiki/Introduction_to_USB_with_STM32
  - Documents STM32 USB device application, USB device library, and HAL or low-layer interaction.
- STM32CubeH7 getting started manual: https://www.st.com/resource/en/user_manual/dm00386433-getting-started-with-stm32cubeh7-for-stm32h7-series-stmicroelectronics.pdf
  - Documents STM32CubeH7 middleware components including USB Host and Device libraries.
- STM32 USBX middleware repository: https://github.com/STMicroelectronics/stm32-mw-usbx
  - Notes USBX middleware as part of STM32Cube MCU components and version consistency with Cube firmware packages.
- STM32H7 Nucleo-144 boards user manual UM2407: https://www.st.com/resource/en/user_manual/um2407-stm32h7-nucleo144-board-stmicroelectronics.pdf
  - Documents STLINK-V3E USB Micro-B connector CN1 and USB OTG FS connector CN13.
