# kilotncd

`kilotncd` is the planned KiloTNC host daemon. M1.14 implements a deterministic file/stdin-style skeleton for testing the portable core from a daemon-shaped command. M1.15 adds a localhost-only KISS TCP test adapter. M1.16 adds local Unix socket once-mode and explicit stdin/stdout file-stream behavior. M1.17 adds local PTY KISS once-mode. M1.18 adds a raw PCM audio backend abstraction. M1.19 adds a radio-control backend abstraction with no-PTT, simulated, and log backends. M1.20 adds explicit daemon config profiles and validation. M1.21 adds a one-shot local control/status command surface. M1.22 adds a bounded foreground loop skeleton. M1.23 adds an ALSA planning boundary and compile-gated stub. M1.24 adds sndio and OSS planning boundaries and compile-gated stubs. M1.25 adds deterministic KISS compatibility tests across local daemon transports.

It is not a background service yet. It does not use real audio devices, real serial PTT, CAT, GPIO, USB, or radio hardware.

## M1.25 Scope

Implemented:

- Bounded config parser.
- File and stdin/stdout-style byte adapters.
- Raw signed 16-bit little-endian PCM file adapters.
- TX once: KISS input to PCM output.
- RX once: PCM input to KISS output.
- Loopback once: KISS input to KISS output through generated PCM.
- Status output with mode and diagnostics.
- Localhost KISS TCP single-client once mode.
- Local Unix socket single-client once mode.
- Local PTY KISS single-client once mode.
- Daemon audio backend interface with raw PCM file/stdin/stdout backend.
- Daemon radio-control backend interface.
- No-PTT and simulated PTT backends.
- Log PTT backend for deterministic tests.
- Config profile parser, inference, defaults, and validation.
- Safe example configs under `daemon/examples/`.
- One-shot local control/status command parser.
- `--control` status, diagnostics, mode, DCD, PTT, stats, abort, and help commands.
- Foreground bounded loop skeleton.
- Dry-run foreground mode.
- Periodic loop diagnostics to stderr.
- ALSA backend name and compile-gated unsupported stub.
- sndio backend name and compile-gated unsupported stub.
- OSS backend name and compile-gated unsupported stub.
- KISS compatibility helper for deterministic local transport tests.
- `make kiss-compat-test` for file, stdin/stdout, localhost TCP, Unix socket, PTY, and CLI loopback checks.

Not implemented:

- Daemonization, fork, PID files, or syslog.
- Real ALSA, sndio, OSS, PulseAudio, PipeWire, or audio devices.
- Real serial PTT, CAT, GPIO, or hardware PTT.
- Real radio receive or transmit.
- Multi-client TCP server.
- Persistent Unix socket server.
- Persistent PTY service.
- Remote internet service.
- Persistent control socket or interactive shell.
- Background daemonization, PID files, and syslog.

## Config Format

Config files use `key=value` lines. Blank lines and lines beginning with `#` are ignored.

Allowed keys:

```text
mode=NINO_MODE=6
profile=file-tx
kiss_in=build/vectors/kilotnc.kiss
kiss_out=build/daemon/out.kiss
pcm_in=build/vectors/kilotnc.pcm
pcm_out=build/daemon/out.pcm
max_tx_ms=30000
p=255
slottime_10ms=10
fullduplex=0
kiss_tcp_listen=127.0.0.1:8001
kiss_tcp_once=1
allow_nonlocal_bind=0
kiss_unix_listen=build/daemon/kilotnc.sock
kiss_unix_once=1
unlink_stale_socket=0
kiss_pty=1
kiss_pty_once=1
pty_path_out=build/daemon/kilotnc.pty
audio_backend=raw
audio_sample_rate=48000
audio_channels=1
audio_bits=16
radio_backend=none
radio_log=build/daemon/ptt.log
control=status
foreground=1
dry_run=1
max_iterations=3
diag_interval=1
```

Unknown keys, invalid numbers, invalid mode strings, overlong lines, and overlong paths are rejected.

Command-line options override config values.

## Profiles

M1.20 profiles describe the complete daemon mode before any real audio or radio-control adapters are added.

| Profile          | Required shape                            |
| ---------------- | ----------------------------------------- |
| `file-tx`        | KISS input file to raw PCM output file    |
| `file-rx`        | Raw PCM input file to KISS output file    |
| `file-loopback`  | KISS input file to KISS output file       |
| `stdio-tx`       | KISS stdin or PCM stdout participates     |
| `stdio-rx`       | PCM stdin or KISS stdout participates     |
| `tcp-kiss-once`  | One localhost TCP KISS client to PCM file |
| `unix-kiss-once` | One local Unix socket KISS client         |
| `pty-kiss-once`  | One local PTY KISS client                 |
| `status`         | Mode and diagnostics only                 |

Use a profile explicitly:

```text
build/kilotncd --profile file-tx --mode NINO_MODE=6 --kiss-in frame.kiss --pcm-out tx.pcm
```

If `--profile` is omitted, `kilotncd` infers the profile from the selected inputs, outputs, and listeners. Explicit profiles reject conflicting adapters. TX-capable profiles require an implemented mode and nonzero `max_tx_ms`. Status may report known but unimplemented modes without starting TX or RX.

Example configs are in `daemon/examples/`.

## Commands

Print status:

```text
build/kilotncd --status
build/kilotncd --status --mode NINO_MODE=6
```

Run one local control command:

```text
build/kilotncd --control status
build/kilotncd --control diag
build/kilotncd --control "mode NINO_MODE=6"
build/kilotncd --control "dcd 1"
build/kilotncd --control abort-tx
```

M1.21 control commands are one-shot only. They parse one bounded ASCII command, print one bounded response, and exit. There is no persistent control socket, remote listener, or interactive shell.

Run a bounded foreground dry run:

```text
build/kilotncd --foreground --dry-run --max-iterations 1
build/kilotncd --foreground --dry-run --max-iterations 10 --diag-interval 1
```

M1.22 foreground mode does not fork, write a PID file, use syslog, open real audio devices, or create persistent services. `--max-iterations` bounds test runs. `--diag-interval 0` disables periodic diagnostics.

Run TX once:

```text
build/kilotncd --mode NINO_MODE=6 --kiss-in frame.kiss --pcm-out tx.pcm --once
```

Run RX once:

```text
build/kilotncd --mode NINO_MODE=6 --pcm-in rx.pcm --kiss-out out.kiss --once
```

Run loopback once:

```text
build/kilotncd --mode NINO_MODE=6 --kiss-in frame.kiss --kiss-out out.kiss --loopback-once
```

Run with config:

```text
build/kilotncd --config daemon/example.conf --once
```

Listen for one localhost KISS TCP client and write generated PCM:

```text
build/kilotncd --kiss-tcp-listen 127.0.0.1:8001 --kiss-tcp-once --pcm-out build/daemon/tcp_tx.pcm --once
```

Listen for one local Unix socket KISS client and write generated PCM:

```text
build/kilotncd --kiss-unix-listen build/daemon/kilotnc.sock --kiss-unix-once --pcm-out build/daemon/unix_tx.pcm --once
```

Open one local PTY KISS adapter and write generated PCM:

```text
build/kilotncd --kiss-pty --kiss-pty-once --pty-path-out build/daemon/kilotnc.pty --pcm-out build/daemon/pty_tx.pcm --once
```

The PTY path file contains the slave device path that a local client can open as a serial-like KISS port.

Log simulated PTT changes during TX once:

```text
build/kilotncd --radio-backend log --radio-log build/daemon/ptt.log --kiss-in build/vectors/kilotnc.kiss --pcm-out build/daemon/radio_log_tx.pcm --once
```

Use `-` for stdin or stdout on file-like inputs and outputs. Diagnostics are printed to stderr when binary output is written to stdout.

Read KISS from stdin and write generated PCM to a file:

```text
build/kilotncd --mode NINO_MODE=6 --kiss-in - --pcm-out build/daemon/stdin_tx.pcm --once
```

Read raw PCM from a file and write KISS to stdout:

```text
build/kilotncd --mode NINO_MODE=6 --pcm-in build/vectors/kilotnc.pcm --kiss-out - --once
```

TCP KISS in M1.15 is localhost-only by default. Binding to anything other than `127.0.0.1` or `localhost` is rejected unless `--allow-nonlocal-bind` is set. If that flag is used, `kilotncd` prints a warning to stderr.

Unix socket KISS in M1.16 is local IPC only. The server accepts one client, processes bounded KISS input, exits, and removes the socket path on clean exit. Stale socket unlink is allowed under `build/` for tests, or when `--unlink-stale-socket` is explicitly set. TCP and Unix socket listeners cannot both be enabled in M1.16.

PTY KISS in M1.17 is local-only and serial-like. It opens a pseudoterminal, writes the slave path to `--pty-path-out` when provided, processes one bounded KISS input transaction, exits, and does not access real serial hardware. TCP, Unix socket, and PTY listeners cannot be enabled together in M1.17.

## Audio Backend

M1.18 adds a daemon-side audio backend boundary. Only the raw backend is implemented.

Raw backend format:

- Signed 16-bit little-endian PCM.
- Mono.
- 48,000 Hz.
- File, stdin, or stdout paths through existing `-` semantics.

Unsupported audio formats are rejected.

M1.23 and M1.24 add planned audio backend stubs:

- `audio_backend=alsa` is recognized as a known backend name.
- `audio_backend=sndio` is recognized as a known backend name.
- `audio_backend=oss` is recognized as a known backend name.
- Default builds compile only unsupported stubs for these backends.
- Default builds do not require `alsa/asoundlib.h`.
- Default builds do not require sndio headers.
- Default builds do not require OSS headers such as `sys/soundcard.h`.
- Default builds do not link `-lasound`.
- Default builds do not link sndio or OSS libraries.
- `ENABLE_ALSA`, `ENABLE_SNDIO`, and `ENABLE_OSS` are reserved for later work and do not enable real runtime support in M1.24.

ALSA, sndio, and OSS remain unsupported for active daemon use in M1.25.

## KISS Compatibility Tests

M1.25 adds a generated KISS compatibility suite for local daemon transports:

```text
make kiss-compat-test
```

The target generates deterministic KISS inputs under `build/kiss-compat/` and checks plain AX.25 UI frames, escaped FEND and FESC payload bytes, command frames, SETHW mode `6`, SETHW mode `22`, unsupported commands, malformed escape recovery, and repeated FEND handling.

The checked transports are file/stdin/stdout, localhost TCP once mode, Unix socket once mode, PTY once mode, and CLI loopback. The tests do not use external TNCs, real audio devices, serial hardware, RF, or remote network service.

## Radio Control Backend

M1.19 adds a daemon-side radio-control backend boundary. Only host-safe backends are implemented:

- `radio_backend=none` accepts PTT changes and does not control hardware.
- `radio_backend=sim` tracks PTT state in memory and does not control hardware.
- `radio_backend=log` writes bounded `ptt=on` and `ptt=off` lines to `radio_log`.

Planned real backends are recognized as names but rejected if selected:

- `serial-rts`.
- `serial-dtr`.
- `cat`.
- `gpio`.

PTT starts off. TX once asserts the selected daemon radio backend before PCM emission and forces it off after TX finishes or an error path is taken where possible. The log backend must not write to stdout.

## Safety Defaults

- Mode defaults to `1200 AFSK AX.25`.
- `p=255`.
- `slottime_10ms=10`.
- `fullduplex=0`.
- `max_tx_ms=30000`.
- No network listeners.
- TCP listeners start only when explicitly requested.
- Unix socket listeners start only when explicitly requested.
- PTY adapters start only when explicitly requested.
- No hardware PTT.
- Radio backend defaults to no-PTT.
- No real audio device.
- No internet-to-RF path.

## Future Adapters

Planned later adapters:

- ALSA.
- sndio.
- OSS.
- Serial PTT and CAT.
- GPIO PTT.

Future adapters must keep mode validation, DCD/channel access, diagnostics, and max TX timeout in the path.
