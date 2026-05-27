# STM32H753 Nucleo Watchdog Plan

M2.10 does not enable a hardware watchdog.

The future watchdog adapter must preserve the current safety rule: PTT state is off before watchdog enable, after reset, and on every fault path. Hardware watchdog setup must be verified on the selected board before any real radio control path exists.

M2.11 planning is tracked in [docs/m2-stm32h753-resource-plan.md](../../../docs/m2-stm32h753-resource-plan.md). Watchdog and reset-cause resources remain planned only. No watchdog register access, reset-cause register reads, or board reset tests are added.

Intentionally absent in M2.10:

- No watchdog peripheral setup.
- No reset cause register reads.
- No clock assumptions.
- No hardware fault injection.
- No real board reset test.

M2.15 planning is tracked in [docs/m2-clock-reset-watchdog-plan.md](../../../docs/m2-clock-reset-watchdog-plan.md). `target_watchdog_config.h` records conservative metadata only:

- Independent watchdog policy planned.
- Window watchdog not selected.
- Watchdog quorum planned for main loop, USB, audio, and PTT-control progress.
- Hardware watchdog enable at boot remains off.
- Watchdog register values remain unfinalized.

No real watchdog setup, watchdog register values, reset register reads, startup integration, HAL, CMSIS, or hardware access is added.
