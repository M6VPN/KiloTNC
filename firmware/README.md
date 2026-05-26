# Firmware

M1 contains portable host-side protocol code for KISS, AX.25 UI frames, HDLC bit helpers, and AX.25/HDLC FCS helpers.

No STM32 HAL, board support, USB driver, codec driver, or modem DSP code is implemented yet.

Layout:

| Path       | Purpose                         |
| ---------- | ------------------------------- |
| `include/` | Portable protocol headers       |
| `src/`     | Portable protocol source files  |
| `test/`    | Host-side protocol tests        |

Rules for protocol code:

- No dynamic allocation in the real-time audio path.
- KISS parser must recover from malformed frames.
- Queue overflow must drop and count, not crash.
- PTT must fail safe.
- Watchdog heartbeat must depend on main loop, USB, audio, and PTT safety progress.
