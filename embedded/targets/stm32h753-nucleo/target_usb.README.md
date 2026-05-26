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
