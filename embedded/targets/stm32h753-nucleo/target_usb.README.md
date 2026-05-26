# STM32H753 Nucleo USB Plan

M2.10 does not implement USB descriptors, endpoints, or a USB device stack.

The future USB CDC KISS path must live behind the embedded USB CDC adapter boundary. It should be added only after the target build skeleton is opt-in and the selected USB FS or HS path has been verified.

Intentionally absent in M2.10:

- No TinyUSB import.
- No STM32 HAL USB calls.
- No descriptors.
- No endpoint handlers.
- No USB interrupt handling.
- No host-visible USB device.
