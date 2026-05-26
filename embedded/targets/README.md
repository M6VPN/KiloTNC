# Embedded Targets

Target-specific board code will live under `embedded/targets/<target>/`.

M2.0 selects `stm32h753-nucleo` as the primary target name, but no target C files or generated vendor projects are added in this pass.

Future target directories should contain only project-owned adapter glue and build metadata. Vendor SDKs, STM32Cube generated projects, CMSIS trees, Pico SDK trees, and TinyUSB trees must stay external unless a later milestone explicitly changes that policy.
