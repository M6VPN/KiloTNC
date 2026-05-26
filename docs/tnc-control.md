# TNC Control Simulation

This document covers the M1.8 host-side channel-access and PTT safety simulation. It is portable C logic for deterministic tests. It does not touch GPIO, USB, codec drivers, STM32 HAL, DMA, or radio hardware.

## Scope

`tnc_control` models the control path needed around AFSK1200 TX:

- p-persistence channel access.
- DCD busy-channel gating.
- FullDuplex behavior.
- Simulated PTT state.
- TXDELAY before audio emission.
- TXTAIL after frame audio completion.
- Maximum TX watchdog timeout.
- Abort and fail-safe PTT-off behavior.

All time advances through `tnc_control_tick_10ms()`. There are no system timers, sleeps, threads, or hardware calls.

## Channel Access

The P value uses the KISS command byte semantics already parsed by the KISS module:

- `p = 255` grants on a clear channel.
- `p = 0` always defers.
- Values between 1 and 254 use a deterministic local PRNG.

If FullDuplex is off and DCD is busy, TX is denied and retried on slot boundaries. If FullDuplex is on, DCD busy does not block TX in the host simulation.

SlotTime is stored in 10 ms units. The simulation retries access only when its slot counter reaches zero.

## PTT Timing

PTT is off after init. A granted TX request turns PTT on. TXDELAY holds PTT on while `tnc_control_can_emit_audio()` returns false. After TXDELAY expires, audio may start.

When the caller marks TX complete, the control state enters TXTAIL. PTT stays on until TXTAIL expires, then returns off.

Abort always forces PTT off and clears timing state. The maximum TX watchdog also forces PTT off, increments the timeout counter, and returns the state to idle.

## Stats

The stats cover:

- TX requests and grants.
- DCD busy denials.
- p-persistence deferrals.
- TX watchdog timeouts.
- TX aborts.
- PTT on and off transitions.

The counters are diagnostic host-side counters for M1.8. They are not yet the final embedded diagnostic interface.

## Limitations

- Host-side simulation only.
- No GPIO or physical PTT pin control.
- No hardware watchdog.
- No USB CDC path.
- No real squelch or COS input.
- No DCD quality gate tied to live audio yet.
- No transmit queue beyond the one pending frame in `tnc1200`.
- No embedded timing source.
