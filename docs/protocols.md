# Protocols

This page defines protocol scope for M0. It is not a firmware specification yet.

## Protocol Matrix

| Protocol or feature        | Native v0.1 | Native v0.2 | Bridge-only | Research-only | Not planned |
| -------------------------- | ----------- | ----------- | ----------- | ------------- | ----------- |
| USB CDC ACM KISS           | Yes         | Yes         | No          | No            | No          |
| KISS data frame command 0  | Yes         | Yes         | No          | No            | No          |
| KISS TXDELAY command 1     | Yes         | Yes         | No          | No            | No          |
| KISS P command 2           | Yes         | Yes         | No          | No            | No          |
| KISS SlotTime command 3    | Yes         | Yes         | No          | No            | No          |
| KISS TXtail command 4      | No          | Maybe       | No          | Yes           | No          |
| KISS FullDuplex command 5  | Yes         | Yes         | No          | No            | No          |
| KISS SetHardware command 6 | Limited     | Yes         | No          | Yes           | No          |
| KISS Return command 255    | No-op       | No-op       | No          | No            | No          |
| AX.25 UI frames            | Yes         | Yes         | No          | No            | No          |
| AX.25 connected mode       | No          | Maybe       | No          | Yes           | No          |
| HDLC bit-stuffing          | Yes         | Yes         | No          | No            | No          |
| AX.25 16-bit FCS           | Yes         | Yes         | No          | No            | No          |
| 1200 baud Bell 202 AFSK    | Yes         | Yes         | No          | No            | No          |
| 300 baud AX.25 AFSK        | No          | Maybe       | No          | Yes           | No          |
| 9600 baud G3RUH-like/GFSK  | No          | Maybe       | No          | Yes           | No          |
| IL2P                       | No          | Maybe       | No          | Yes           | No          |
| IL2Pc                      | No          | Maybe       | No          | Yes           | No          |
| FX.25                      | No          | No          | No          | Yes           | No          |
| VARA native modem          | No          | No          | No          | No            | Yes         |
| VARA host bridge           | No          | No          | Yes         | Yes           | No          |
| ARDOP host bridge          | No          | No          | Yes         | Yes           | No          |
| Mercury host bridge        | No          | No          | Yes         | Yes           | No          |
| Dire Wolf source import    | No          | No          | No          | No            | Yes         |

## KISS Requirements

KiloTNC must implement KISS as the first host interface.

Special bytes:

| Name  | Value  | Meaning                  |
| ----- | ------ | ------------------------ |
| FEND  | `0xC0` | Frame end                |
| FESC  | `0xDB` | Escape marker            |
| TFEND | `0xDC` | Escaped FEND replacement |
| TFESC | `0xDD` | Escaped FESC replacement |

Framing rules:

- Each frame is delimited by FEND.
- FEND in payload is encoded as FESC TFEND.
- FESC in payload is encoded as FESC TFESC.
- Back-to-back FEND bytes do not create empty frames.
- A malformed escape sequence increments a parser error counter and parser recovery continues.
- Unsupported commands are ignored.
- Queue overflow drops the offending packet, increments a counter, and never crashes the firmware.

Command byte:

- Low nibble is the command.
- High nibble is the port.
- Single-port KiloTNC uses port 0 first.

Command handling:

| Command | Name       | v0.1 behavior                                      |
| ------- | ---------- | -------------------------------------------------- |
| `0x00`  | Data       | Queue rest of frame for transmit                   |
| `0x01`  | TXDELAY    | Set key-up delay in 10 ms units                    |
| `0x02`  | P          | Set p-persistence byte                             |
| `0x03`  | SlotTime   | Set p-persistence slot time in 10 ms units         |
| `0x04`  | TXtail     | Ignore unless later compatibility requires support |
| `0x05`  | FullDuplex | Set half/full duplex behavior                      |
| `0x06`  | SetHardware | Limited hardware-specific command namespace       |
| `0xFF`  | Return     | No-op; stay in KISS mode                           |

## Mode Registry and SETHW Scope

M1.10 adds a KiloTNC internal mode registry and a NinoTNC-compatible boundary mapping. KiloTNC uses its own mode IDs internally.

NinoTNC-compatible KISS SETHW mode values are accepted only as compatibility input:

- SETHW payload values 0 through 14 map to known NinoTNC modes.
- Values 16 through 30 map to the same modes as 0 through 14 and are treated as temporary/no-flash requests.
- Switch mode 15 is treated as the NinoTNC set-from-KISS switch state, not as an on-air mode.
- Only 1200 AFSK AX.25 is implemented in KiloTNC M1.10.
- Known but unimplemented modes are counted and leave the current safe mode unchanged.

## AX.25 Scope

AX.25 v2.2 is a link-layer protocol. It is documented separately from modem DSP.

Native v0.1 handles UI frames first:

- Address field with destination and source callsign/SSID encoding.
- Optional digipeater fields after source address.
- Control field for UI frames.
- PID field for UI frames.
- Information field payload.
- 16-bit FCS.
- HDLC flags and bit-stuffing.

Implementation boundaries:

- AX.25 encode/decode works on bytes and bits before modem symbol mapping.
- HDLC bit-stuffing prevents accidental flag patterns inside a frame.
- FCS validation rejects corrupted AX.25 frames before delivery to the host.
- NRZI is a modem/line-coding step where relevant, not an AX.25 address or FCS rule.

## Native Modem Scope

1200 baud Bell 202 AFSK is the first native modem target. The initial work must include WAV test vectors and golden decode/encode cases before radio tests.

300 baud AX.25 AFSK and 9600 baud G3RUH-like/GFSK are research targets until their full modulation, scrambling, filtering, timing recovery, and radio interface requirements are verified.

DCD behavior and p-persistence behavior must be tested with real squelch/COS inputs and open-squelch audio before PCB order.

## IL2P and IL2Pc Scope

IL2P is a separate Layer 2 packet format with FEC, packet-synchronous scrambling, and different on-air framing than AX.25.

IL2P can carry host-facing AX.25 KISS frames by translation or transparent encapsulation, but this does not make the on-air frame AX.25-compatible.

IL2Pc is treated as IL2P with the later CRC improvement. Interoperability claims require testing against current NinoTNC behavior and published IL2P v0.6 details.

IL2P/IL2Pc research items:

- Reed-Solomon FEC parameters.
- Packet-synchronous scrambling.
- Sync word detection.
- Optional/supplemental CRC behavior.
- Type 0 transparent encapsulation limits.
- Type 1 translated AX.25 header compatibility limits.

## FX.25 Scope

FX.25 is research-only in this pass. It wraps AX.25 with FEC while preserving legacy AX.25 reception in compatible cases. It may be worth revisiting after IL2Pc because IL2P v0.6 explicitly cites FX.25 as an influence but chooses a different on-air design.

## External Modem Scope

VARA:

- Proprietary external software modem.
- Native implementation is not planned.
- Bridge support means USB audio, PTT, and CAT support where host software can use it.

ARDOP:

- Open external modem option.
- Bridge support means audio/PTT/CAT support and host-side interface testing.

Mercury:

- Open modem candidate.
- Bridge support means host-side TCP TNC compatibility testing and audio/PTT/CAT support.
- Mercury on-air compatibility is with Mercury peers, not VARA peers.

## Host and Network Interface Scope

KISS-over-TCP, Unix sockets, PTYs, and stdin/stdout are daemon host interfaces. They are not on-air protocols.

KISS-over-TCP transports KISS frames between a host client and a daemon. It does not change AX.25, HDLC, FCS, or modem framing.

PTY KISS gives local host programs a serial-like transport for KISS frames. It does not change AX.25, HDLC, FCS, or modem framing and does not access real serial hardware in M1.17.

M1.25 KISS compatibility tests exercise file/stdin/stdout, localhost TCP, Unix socket, PTY, and CLI loopback paths with deterministic local KISS streams. These tests cover local host transport behavior only and do not claim external TNC or on-air interoperability.

Internet bridging is future research and is disabled by default in planning. Any future internet-originated packet path must pass through operator policy, mode validation, queue limits, DCD/channel access, and max TX watchdog logic before RF transmission.

## Sources checked

| Source title                                      | Date checked | Note                                                |
| ------------------------------------------------- | ------------ | --------------------------------------------------- |
| The KISS TNC, Chepponis and Karn                  | 2026-05-26   | KISS framing, commands, p-persistence, buffer drops |
| AX.25 Link Access Protocol v2.2, TAPR/ARRL        | 2026-05-26   | AX.25 fields, UI scope, bit-stuffing, FCS           |
| IL2P Specification Draft v0.6                     | 2026-05-26   | IL2P FEC, scrambling, sync word, CRC, AX.25 mapping |
| TARPN IL2P overview                              | 2026-05-26   | IL2P publication history and current spec link      |
| OARC NinoTNC page                                | 2026-05-26   | NinoTNC mode table, BrdSwchMod, SETHW behavior      |
| TARPN NinoTNC N9600A Operator Manual             | 2026-05-26   | NinoTNC modes and SETHW +16 behavior                |
| TARPN Protocols and Modulation page              | 2026-05-26   | NinoTNC 300, 1200, 4800, 9600 mode context          |
| FX.25 Forward Error Correction page              | 2026-05-26   | FX.25 wrapper research context                      |
| VARA Modem official site                         | 2026-05-26   | VARA external software modem and license model      |
| ARDOP Specification Revision 0.3.1                | 2026-05-26   | ARDOP open protocol and host/radio interface        |
| Mercury GitHub README, Rhizomatica                | 2026-05-26   | Mercury external modem and TCP TNC interface        |
