# kilotncd

`kilotncd` is the planned KiloTNC host daemon. M1.14 implements a deterministic file/stdin-style skeleton for testing the portable core from a daemon-shaped command. M1.15 adds a localhost-only KISS TCP test adapter. M1.16 adds local Unix socket once-mode and explicit stdin/stdout file-stream behavior.

It is not a background service yet. It does not use real audio devices, PTYs, serial PTT, CAT, GPIO, USB, or radio hardware.

## M1.16 Scope

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

Not implemented:

- Daemonization, fork, PID files, or syslog.
- ALSA, sndio, OSS, PulseAudio, or PipeWire.
- PTYs.
- Serial PTT, CAT, GPIO, or hardware PTT.
- Real radio receive or transmit.
- Multi-client TCP server.
- Persistent Unix socket server.
- Remote internet service.

## Config Format

Config files use `key=value` lines. Blank lines and lines beginning with `#` are ignored.

Allowed keys:

```text
mode=NINO_MODE=6
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
```

Unknown keys, invalid numbers, invalid mode strings, overlong lines, and overlong paths are rejected.

Command-line options override config values.

## Commands

Print status:

```text
build/kilotncd --status
build/kilotncd --status --mode NINO_MODE=6
```

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

## Safety Defaults

- Mode defaults to `1200 AFSK AX.25`.
- `p=255`.
- `slottime_10ms=10`.
- `fullduplex=0`.
- `max_tx_ms=30000`.
- No network listeners.
- TCP listeners start only when explicitly requested.
- Unix socket listeners start only when explicitly requested.
- No hardware PTT.
- No real audio device.
- No internet-to-RF path.

## Future Adapters

Planned later adapters:

- ALSA.
- sndio.
- OSS.
- PTY.
- Serial PTT and CAT.

Future adapters must keep mode validation, DCD/channel access, diagnostics, and max TX timeout in the path.
