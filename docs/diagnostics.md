# Diagnostics

This document covers the M1.9 host-side diagnostics layer. It consolidates counters, state, and recent faults from the portable host-side TNC modules.

## Scope

`tnc_diag` is portable C with fixed storage only. It does not know about USB CDC, UART, GPIO, STM32 HAL, files, real clocks, persistent storage, or hardware reset causes.

The diagnostics layer captures snapshots from `tnc1200`, which already owns the KISS parser, AFSK1200 streaming RX/TX, and channel-control simulation.

## Snapshot

A diagnostic snapshot contains:

- KISS frame, parse-error, and ignored-command counters.
- TX frame and sample counters.
- RX frame and sample counters.
- Channel-access counters.
- PTT transition counters.
- Mode set request counters.
- RX DCD score and confidence average.
- Current P, SlotTime, FullDuplex, PTT, TX active, audio-ready, and DCD busy state.
- Current mode, last requested mode, last NinoTNC SETHW value, and temporary/no-flash flag.
- Last explicitly recorded fault.

Snapshots are copied into caller-provided structs. No strings are allocated during capture.

## Fault Ring

`tnc_diag` keeps the latest 16 recorded faults in a fixed ring. New faults overwrite the oldest entry once full. Fault retrieval returns retained faults in oldest-to-newest order.

`TNC_DIAG_FAULT_NONE` is not recorded and returns a range error. Faults are explicit diagnostic records in M1.9. The TNC1200 behavior is not changed to auto-record faults in this pass.

## Text Format

`tnc_diag_format_snapshot()` writes a stable key-value line into a caller-provided buffer:

```text
kiss_in=1 kiss_out=1 kiss_parse_errors=0 tx_started=1 rx_ok=1 ptt=0 audio_ready=0 dcd=0 last_fault=0
```

The actual output includes all snapshot fields. The formatter uses bounded `snprintf` and returns `TNC_DIAG_ERR_SMALL` if the output buffer is too small.

## Future Use

This layer is intended to feed a later diagnostic CDC CLI and embedded fault reporting path. It also provides a stable host-side test target before any USB or hardware work starts.

M1.11 exposes formatted snapshots through `kilotnc_cli diag` and prints snapshots after KISS-to-PCM, PCM-to-KISS, and loopback commands.

## Limitations

- No USB CDC CLI yet.
- No persistent storage yet.
- No hardware reset-cause capture yet.
- No brownout or hardware watchdog counters yet.
- No real audio ISR overrun or underrun counters yet.
- No real-time clock timestamps.
- No GPIO, CAT, or radio hardware state.
