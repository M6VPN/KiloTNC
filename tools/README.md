# Tools

`kilotnc_cli` is a host-side debug tool for the portable KiloTNC harness. It is not embedded firmware and does not talk to USB, radios, GPIO, or hardware audio devices.

Build it with:

```text
make tools
```

The CLI uses raw signed 16-bit little-endian mono PCM for audio files:

- 48,000 Hz sample rate.
- 1200 baud Bell 202 AFSK generated audio.
- No WAV header in M1.11.

Only `1200 AFSK AX.25` is implemented. Other Nino-compatible modes are registry entries only and are rejected for TX/RX operations.

## Examples

Parse NinoTNC-compatible modes:

```text
build/kilotnc_cli mode --mode NINO_MODE=6
build/kilotnc_cli mode --mode NINO_MODE=22
build/kilotnc_cli mode --mode KILOTNC_MODE=1200-afsk-ax25
```

Convert KISS to raw PCM:

```text
build/kilotnc_cli kiss-to-pcm --in frame.kiss --out tx.pcm --mode NINO_MODE=6
```

Convert raw PCM to KISS:

```text
build/kilotnc_cli pcm-to-kiss --in rx.pcm --out frame.kiss --mode NINO_MODE=6
```

Run a host-side loopback:

```text
build/kilotnc_cli loopback --in frame.kiss --out out.kiss --mode NINO_MODE=6
```

Print diagnostics:

```text
build/kilotnc_cli diag --mode NINO_MODE=6
```

`NINO_MODE=22` maps to the same internal mode as `NINO_MODE=6` and marks the request as temporary/no-flash compatible.
