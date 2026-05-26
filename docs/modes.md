# Modes

This document covers the M1.10 KiloTNC mode registry and NinoTNC compatibility mapping. It defines mode identity and host-side switching behavior only. It does not implement new modems.

## KiloTNC Registry

KiloTNC uses internal mode IDs. NinoTNC switch and SETHW values are accepted only as compatibility inputs at the KISS boundary.

Only `1200 AFSK AX.25` is implemented in M1.10. Other modes are planned or research entries so they can be selected, counted, and reported without claiming modem support.

| KiloTNC mode        | Support     | Nino switch | BrdSwchMod | SETHW | Temp SETHW | Baud  | bps   | Mod  | Protocol | Usage          |
| ------------------- | ----------- | ----------- | ---------- | ----- | ---------- | ----- | ----- | ---- | -------- | -------------- |
| 9600 GFSK AX.25     | planned     | 0           | 0x00       | 0     | 16         | 9600  | 9600  | GFSK | AX.25    | FM data-radio  |
| 19200 4FSK IL2Pc    | research    | 1           | 0x41       | 1     | 17         | 19200 | 19200 | 4FSK | IL2Pc    | FM data-radio  |
| 9600 GFSK IL2Pc     | research    | 2           | 0xB0       | 2     | 18         | 9600  | 9600  | GFSK | IL2Pc    | FM data-radio  |
| 9600 4FSK IL2Pc     | research    | 3           | 0x40       | 3     | 19         | 9600  | 9600  | 4FSK | IL2Pc    | FM data-radio  |
| 4800 GFSK IL2Pc     | research    | 4           | 0xA3       | 4     | 20         | 4800  | 4800  | GFSK | IL2Pc    | FM data-radio  |
| 3600 QPSK IL2Pc     | research    | 5           | 0xF1       | 5     | 21         | 1800  | 3600  | QPSK | IL2Pc    | FM mic/speaker |
| 1200 AFSK AX.25     | implemented | 6           | 0x02       | 6     | 22         | 1200  | 1200  | AFSK | AX.25    | FM mic/speaker |
| 1200 AFSK IL2Pc     | research    | 7           | 0x93       | 7     | 23         | 1200  | 1200  | AFSK | IL2Pc    | FM mic/speaker |
| 300 BPSK IL2Pc      | research    | 8           | 0x91       | 8     | 24         | 300   | 300   | BPSK | IL2Pc    | SSB HF         |
| 600 QPSK IL2Pc      | research    | 9           | 0x92       | 9     | 25         | 300   | 600   | QPSK | IL2Pc    | SSB HF         |
| 1200 BPSK IL2Pc     | research    | 10          | 0xA0       | 10    | 26         | 1200  | 1200  | BPSK | IL2Pc    | SSB HF         |
| 2400 QPSK IL2Pc     | research    | 11          | 0xA2       | 11    | 27         | 1200  | 2400  | QPSK | IL2Pc    | SSB HF         |
| 300 AFSK AX.25      | planned     | 12          | 0x31       | 12    | 28         | 300   | 300   | AFSK | AX.25    | SSB HF         |
| 300 AFSK IL2P       | research    | 13          | 0x22       | 13    | 29         | 300   | 300   | AFSK | IL2P     | SSB HF         |
| 300 AFSK IL2Pc      | research    | 14          | 0x23       | 14    | 30         | 300   | 300   | AFSK | IL2Pc    | SSB HF         |
| Set from KISS       | unsupported | 15          | 0xF3       | n/a   | n/a        | n/a   | n/a   | n/a  | n/a      | mode switch    |

## NinoTNC Compatibility

NinoTNC switch mode `1111` means the operating mode can be selected from KISS. KiloTNC models that as `TNC_MODE_SET_FROM_KISS`, not as an on-air modem.

KISS SetHardware command 6 is parsed as a compatibility mode request when the payload starts with one NinoTNC mode value:

- Values 0 through 14 map to the NinoTNC switch table.
- Values 16 through 30 map to the same modes as 0 through 14 and set a temporary/no-flash flag.
- Values 15, 31, and higher are rejected for SETHW mode selection.
- KiloTNC does not write persistent storage or flash in M1.10.

Examples:

| Input             | KiloTNC result                         |
| ----------------- | -------------------------------------- |
| `NINO_MODE=6`     | `1200 AFSK AX.25`, persistent-style    |
| `NINO_MODE=22`    | `1200 AFSK AX.25`, temporary-style     |
| `KISS SETHW 6 6`  | accept and keep 1200 AFSK AX.25        |
| `KISS SETHW 6 22` | accept and keep 1200 AFSK AX.25        |
| `KISS SETHW 6 0`  | count as known but unsupported         |
| `KISS SETHW 6 31` | count as invalid                       |

## TNC1200 Behavior

TNC1200 defaults to `1200 AFSK AX.25`. It only transmits and receives in that implemented mode.

If a known but unimplemented mode is requested, TNC1200 records the request and keeps the current implemented mode. TX/RX continues to use 1200 AFSK AX.25.

If an invalid mode is requested, TNC1200 counts it and keeps the current implemented mode.

## Sources checked

| Source title                         | Date checked | Note                                        |
| ------------------------------------ | ------------ | ------------------------------------------- |
| OARC NinoTNC page                    | 2026-05-26   | Mode table, BrdSwchMod values, SETHW notes  |
| TARPN NinoTNC N9600A Operator Manual | 2026-05-26   | Modes, SETHW +16 behavior, compatibility    |
