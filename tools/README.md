# Tools

`kilotnc_cli` is a host-side debug tool for the portable KiloTNC harness. It is not embedded firmware and does not talk to USB, radios, GPIO, or hardware audio devices.

The `kilotncd` daemon is separate from this CLI. The CLI remains a manual debug and test-vector tool.

Build it with:

```text
make tools
```

Run deterministic vector checks:

```text
make tool-test
```

The CLI uses raw signed 16-bit little-endian mono PCM for audio files:

- 48,000 Hz sample rate.
- 1200 baud Bell 202 AFSK generated audio.

The CLI can also write RIFF/WAVE files:

- Mono signed 16-bit PCM.
- 48,000 Hz sample rate.
- Generated under `build/` during `make tool-test`.

Only `1200 AFSK AX.25` is implemented. Other Nino-compatible modes are registry entries only and are rejected for TX/RX operations.

## Examples

Parse NinoTNC-compatible modes:

```text
build/kilotnc_cli mode --mode NINO_MODE=6
build/kilotnc_cli mode --mode NINO_MODE=22
build/kilotnc_cli inspect-mode --mode NINO_MODE=6
build/kilotnc_cli mode --mode KILOTNC_MODE=1200-afsk-ax25
```

Generate vectors:

```text
build/kilotnc_cli generate-kiss --out build/vectors/kilotnc.kiss --dst APZKTN --src M6VPN --info "KiloTNC test"
build/kilotnc_cli generate-pcm --out build/vectors/kilotnc.pcm --dst APZKTN --src M6VPN --info "KiloTNC test" --mode NINO_MODE=6
build/kilotnc_cli generate-wav --out build/vectors/kilotnc.wav --dst APZKTN --src M6VPN --info "KiloTNC test" --mode NINO_MODE=22
build/kilotnc_cli vector-loopback --prefix build/vectors/kilotnc --mode NINO_MODE=6
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

Run a one-shot local control command:

```text
build/kilotnc_cli control --cmd status
build/kilotnc_cli control --cmd diag
build/kilotnc_cli control --cmd "mode NINO_MODE=6"
build/kilotnc_cli control --cmd "dcd 1"
```

`NINO_MODE=22` maps to the same internal mode as `NINO_MODE=6` and marks the request as temporary/no-flash compatible.
