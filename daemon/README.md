# kilotncd

`kilotncd` is the planned KiloTNC host daemon. M1.14 implements only a deterministic file/stdin-style skeleton for testing the portable core from a daemon-shaped command.

It is not a background service yet. It does not use real audio devices, sockets, PTYs, serial PTT, CAT, GPIO, USB, or radio hardware.

## M1.14 Scope

Implemented:

- Bounded config parser.
- File and stdin/stdout-style byte adapters.
- Raw signed 16-bit little-endian PCM file adapters.
- TX once: KISS input to PCM output.
- RX once: PCM input to KISS output.
- Loopback once: KISS input to KISS output through generated PCM.
- Status output with mode and diagnostics.

Not implemented:

- Daemonization, fork, PID files, or syslog.
- ALSA, sndio, OSS, PulseAudio, or PipeWire.
- TCP KISS, Unix sockets, or PTYs.
- Serial PTT, CAT, GPIO, or hardware PTT.
- Real radio receive or transmit.

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

Use `-` for stdin or stdout on file-like inputs and outputs. Diagnostics are printed to stderr when binary output is written to stdout.

## Safety Defaults

- Mode defaults to `1200 AFSK AX.25`.
- `p=255`.
- `slottime_10ms=10`.
- `fullduplex=0`.
- `max_tx_ms=30000`.
- No network listeners.
- No hardware PTT.
- No real audio device.
- No internet-to-RF path.

## Future Adapters

Planned later adapters:

- ALSA.
- sndio.
- OSS.
- TCP KISS on localhost first.
- Unix socket.
- PTY.
- Serial PTT and CAT.

Future adapters must keep mode validation, DCD/channel access, diagnostics, and max TX timeout in the path.
