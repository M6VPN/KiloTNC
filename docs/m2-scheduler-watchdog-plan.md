# M2 Scheduler Watchdog Plan

## Scope

M2.19 defines a cooperative scheduler model for host tests only. It does not add an RTOS, interrupts, DMA, hardware watchdog setup, real timers, hardware USB, hardware audio, or hardware PTT control.

The model exists to prove task-progress and watchdog-quorum rules before any future target adapter maps them to a real STM32H7 platform.

## Task Model

Planned embedded task IDs:

| Task ID             | Purpose                                      |
| ------------------- | -------------------------------------------- |
| main/app task       | Top-level embedded application progress      |
| USB service task    | USB CDC byte movement and KISS input/output  |
| audio service task  | Audio adapter sample movement                |
| modem TX task       | Simulated or future modem transmit progress  |
| modem RX task       | Simulated or future modem receive progress   |
| control/PTT task    | Safety state, PTT safe-off, and control tick |
| diagnostics task    | Diagnostic snapshot and formatting work      |
| config/control task | Config validation and future commit control  |

The current implementation is still a simple host-native cooperative model. It uses fixed state, task masks, and explicit progress marks.

## Watchdog Quorum

The watchdog should only be kicked when required tasks have reported progress for the current cycle.

Required by default:

- Main/app task.
- Control/PTT safety task.

Conditionally required by tests or future configuration:

- USB service task when USB movement is part of the active path.
- Audio service task when audio loopback or audio movement is part of the active path.
- Modem TX task when modem transmit progress must be proven.
- Modem RX task when modem receive progress must be proven.

Diagnostics must not block watchdog or safety. Config work must not block the real-time path.

## Failure Rules

- A stalled required task causes a scheduler fault.
- Scheduler fault forces PTT off.
- Scheduler fault aborts active simulated modem TX.
- Scheduler fault increments diagnostics-visible counters.
- Scheduler fault prevents watchdog kick.
- Malformed USB or KISS input must not starve the control/PTT task.
- Queue congestion must not block control/PTT progress.

## Future Hardware Mapping

This model can later map to:

- A single superloop.
- ISR plus bounded ring buffers.
- RTOS tasks if a later milestone chooses an RTOS.

M2.19 does not choose an RTOS and does not add any real interrupt or DMA boundary.

## M2.19 Host-Tested State

M2.19 adds host tests for:

- Task enabled, required, and progress masks.
- Watchdog allowed and blocked states.
- Missing required task faults.
- App-step integration.
- PTT safe-off on scheduler fault.
- Active simulated modem TX abort on scheduler fault.
- Diagnostics fields for scheduler state.
