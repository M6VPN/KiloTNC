# Firmware

Firmware is not implemented in M0.

This directory is reserved for embedded firmware after the protocol matrix, architecture, hardware plan, firmware safety rules, and test plan are reviewed.

Planned layout:

| Path       | Purpose                         |
| ---------- | ------------------------------- |
| `include/` | Public firmware headers         |
| `src/`     | Firmware source files           |
| `test/`    | Host and target firmware tests  |

Rules before code starts:

- No dynamic allocation in the real-time audio path.
- KISS parser must recover from malformed frames.
- Queue overflow must drop and count, not crash.
- PTT must fail safe.
- Watchdog heartbeat must depend on main loop, USB, audio, and PTT safety progress.

