# Generated Vectors

M1.13 adds deterministic vector export through `kilotnc_cli`. The generated files are for lab and debug use and are written under `build/`.

## Purpose

Generated vectors provide repeatable host-side artifacts for:

- KISS frame inspection.
- Raw AFSK1200 PCM inspection.
- WAV listening and spectrum inspection.
- KISS-to-PCM-to-KISS loopback checks.
- Diagnostics snapshot capture.

The same fixtures can later support daemon, firmware, and hardware validation.

## File Types

| Extension   | Contents                                            |
| ----------- | --------------------------------------------------- |
| `.kiss`     | KISS command 0 data frame carrying AX.25 UI + FCS   |
| `.pcm`      | Raw signed 16-bit little-endian mono PCM            |
| `.wav`      | RIFF/WAVE mono signed 16-bit PCM at 48,000 Hz       |
| `.out.kiss` | Loopback KISS output decoded from generated PCM     |
| `.diag.txt` | Text diagnostics from TX and RX loopback instances  |

## Regeneration

Build the CLI and generate vectors:

```text
make tools
make tool-test
```

Manual vector generation:

```text
build/kilotnc_cli vector-loopback --prefix build/vectors/kilotnc --mode NINO_MODE=6
```

`NINO_MODE=22` maps to the same implemented 1200 AFSK AX.25 mode and marks the request as temporary/no-flash compatible.

## Repository Policy

Generated vectors are not committed. They stay under `build/`, which is ignored by Git.

## Limitations

- Generated host vectors only.
- No real radio captures.
- No calibrated audio level claim.
- No interop claim with external TNCs yet.
- No daemon, socket, PTY, USB, or hardware audio path in this pass.

## Sources checked

| Source title                                   | Date checked | Note                                |
| ---------------------------------------------- | ------------ | ----------------------------------- |
| Microsoft RIFF documentation                   | 2026-05-26   | RIFF/WAVE chunk structure           |
| Microsoft waveform audio data types reference  | 2026-05-26   | 16-bit PCM sample representation    |
