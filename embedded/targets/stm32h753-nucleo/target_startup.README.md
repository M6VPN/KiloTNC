# STM32H753 Nucleo Startup Plan

M2.10 does not add startup code or vector tables.

A later target build will need startup code for reset entry, vector table placement, stack setup, data/BSS initialization, safe early PTT-off state, and handoff into the embedded app. That code must not be copied from vendor examples into this repo.

Intentionally absent in M2.10:

- No interrupt vector table.
- No reset handler.
- No clock tree setup.
- No CMSIS startup file.
- No STM32Cube generated startup file.
