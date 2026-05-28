# M2 Queue And Backpressure Plan

M2.17 defines queue and backpressure policy before real interrupt, DMA, USB, or audio hardware work starts.

## Scope

This document is planning plus host-testable metadata only.

- No real ISR is implemented.
- No real DMA is implemented.
- No hardware USB or audio path is implemented.
- No scheduler or RTOS commitment is made.
- No PCB or RF work is started.
- No heap allocation is introduced for queue handling.

## Queue Boundaries

Planned embedded queues:

| Queue | Direction | Planned use |
| ----- | --------- | ----------- |
| USB RX byte queue | Host to embedded | Host KISS bytes into the embedded parser |
| USB TX byte queue | Embedded to host | KISS output and future diagnostics to host |
| KISS frame queue | Parser to TNC | Parsed KISS data and command frames |
| Modem TX frame queue | TNC to modem TX | Accepted AX.25 frames awaiting simulated or real TX |
| Audio TX sample queue | Modem TX to audio adapter | Generated modem samples to the audio adapter |
| Audio RX sample queue | Audio adapter to modem RX | Input samples to the modem RX path |
| Modem RX frame queue | Modem RX to TNC/KISS | Decoded AX.25 frames to KISS output |
| Diagnostics/event queue | Platform/app to diagnostics | Bounded event and fault records |
| Control/PTT event queue | Control path | Future safe state transitions only |

Config commands and future commit requests belong on the control path. They must not bypass validation, mode support checks, max TX timeout policy, or PTT safe-off state.

Queue congestion must not prevent the control/PTT task from reporting scheduler progress. If a future data queue can block control progress, that queue design is unsafe and must be revised before hardware work.

## Backpressure Rules

- No unbounded buffers.
- No heap allocation in the real-time path.
- Queue overflow must drop, reject, fault, and count deterministically instead of crashing.
- Malformed USB or KISS input must not starve the watchdog path.
- USB TX overflow must drop and count diagnostics or data deterministically.
- Audio RX overflow means samples are dropped and counted.
- Audio TX underflow means silence or fail-safe behavior in future hardware work, counted now.
- TX frame queue full rejects the new TX frame and counts the rejection.
- Diagnostics queue full uses an explicit drop-oldest policy so the newest fault context can remain visible.
- PTT and safety events must not be dropped silently.

## Priority And Safety

Future queue service priority:

1. PTT safe-off, watchdog, and fault state.
2. Control timing and max TX timeout.
3. Audio real-time path.
4. USB RX/TX byte movement.
5. Packet and modem frame processing.
6. Diagnostics formatting.

Future implementation may use a cooperative loop or interrupt-driven adapters, but safety state must remain independent of congested data queues.

## M2.17 Queue Policy Metadata

`embedded/include/kilotnc_queue_policy.h` records the current planning table:

| Queue | Capacity source | Overflow policy | Safety critical |
| ----- | --------------- | --------------- | --------------- |
| `usb_rx` | USB RX budget metadata | Drop newest | No |
| `usb_tx` | USB TX budget metadata | Drop newest | No |
| `kiss_frame` | Planning constant | Reject | No |
| `modem_tx_frame` | Planning constant | Reject | No |
| `audio_tx_sample` | Audio TX budget metadata | Drop newest | No |
| `audio_rx_sample` | Audio RX budget metadata | Drop newest | No |
| `modem_rx_frame` | Planning constant | Drop newest | No |
| `diag_event` | Diagnostics ring budget metadata | Drop oldest | No |
| `control_event` | Planning constant | Safety fault | Yes |

The queue policy finalized flag remains false until real ISR, DMA, USB, audio, and scheduler boundaries exist.

## KiloNode Future AX.25 Work

`M6VPN/KiloNode` is a sibling project with ongoing AX.25 work. Future KiloTNC AX.25 changes may evaluate KiloNode as a reference source, but no KiloNode code is imported or copied in M2.17.

Future integration rules:

- Do not break existing KiloTNC tests.
- Preserve portable-core APIs.
- Preserve fixed-buffer and no-heap policy.
- Add golden vectors and compatibility tests.
- No blind import.
- Document copied or shared code provenance if code is later intentionally reused.

## Future Work

- Runtime queue counters.
- Queue high-water diagnostics.
- ISR/DMA boundary tests.
- USB disconnect backpressure behavior.
- Audio overflow and underflow hardware behavior.
- Control-event fault injection tests.
