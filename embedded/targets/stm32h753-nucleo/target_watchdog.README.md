# STM32H753 Nucleo Watchdog Plan

M2.10 does not enable a hardware watchdog.

The future watchdog adapter must preserve the current safety rule: PTT state is off before watchdog enable, after reset, and on every fault path. Hardware watchdog setup must be verified on the selected board before any real radio control path exists.

Intentionally absent in M2.10:

- No watchdog peripheral setup.
- No reset cause register reads.
- No clock assumptions.
- No hardware fault injection.
- No real board reset test.
