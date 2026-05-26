# STM32H753 Nucleo Linker Plan

M2.10 does not add a linker script.

A later opt-in target build will need a linker script that matches the selected STM32H753 memory map, boot mode, vector table placement, stack, heap policy, and firmware image layout. That work must be based on verified target documentation and must stay separate from the host build.

Intentionally absent in M2.10:

- No flashable image.
- No memory region definitions.
- No bootloader assumptions.
- No vendor linker script import.
- No generated IDE project file.
