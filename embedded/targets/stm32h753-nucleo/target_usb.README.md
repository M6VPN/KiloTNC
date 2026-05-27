# STM32H753 Nucleo USB Plan

M2.10 does not implement USB descriptors, endpoints, or a USB device stack.

The future USB CDC KISS path must live behind the embedded USB CDC adapter boundary. It should be added only after the target build skeleton is opt-in and the selected USB FS or HS path has been verified.

M2.11 planning is tracked in [docs/m2-stm32h753-resource-plan.md](../../../docs/m2-stm32h753-resource-plan.md). USB OTG FS on CN13 is the first planning path, but no USB pins, descriptors, endpoints, or driver code are implemented or finalized.

Intentionally absent in M2.10:

- No TinyUSB import.
- No STM32 HAL USB calls.
- No descriptors.
- No endpoint handlers.
- No USB interrupt handling.
- No host-visible USB device.

M2.13 USB stack planning is tracked in [docs/m2-usb-stack-plan.md](../../../docs/m2-usb-stack-plan.md).

Planned direction:

- Prefer TinyUSB as the future first USB CDC adapter path.
- Keep STM32Cube USB Device middleware as a fallback and reference path.
- Keep the current USB CDC stub as the only implemented path.
- Keep VID, PID, string descriptors, endpoint layout, and optional diagnostics CDC interface as future planning items.

No real USB stack, TinyUSB source, STM32Cube USB Device source, CMSIS, HAL, descriptors, endpoint handlers, interrupt handlers, hardware register access, or generated vendor project is added in M2.13.

M2.14 descriptor planning is tracked in `embedded/app/usb_descriptor_plan.c` and [docs/m2-usb-stack-plan.md](../../../docs/m2-usb-stack-plan.md).

Planned descriptor profiles:

- `kiss-only`: one CDC ACM function for KISS.
- `kiss-plus-diag`: two CDC ACM functions, one for KISS and one for future diagnostics.

M2.14 does not claim a final VID or PID. It does not add binary USB descriptor arrays, endpoint numbers, TinyUSB descriptor callbacks, STM32Cube descriptor tables, hardware USB access, or a host-visible USB device.

M2.15 clock planning is tracked in [docs/m2-clock-reset-watchdog-plan.md](../../../docs/m2-clock-reset-watchdog-plan.md). USB clock source and PLL values remain `TBD` and must be verified by future USB stack work before any real USB device path exists.

Safe boot planning requires PTT safe/off before USB byte-stream input can affect app state. M2.15 does not add USB clock setup, endpoint hardware setup, register access, HAL, CMSIS, TinyUSB, STM32Cube, or descriptor binding.
