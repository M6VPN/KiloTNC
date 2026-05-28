# M2 Safety Plan

M2 dev-board firmware work must prove safe defaults before any real radio PTT, codec path, or RF transmit path exists.

## Hard Limits

- No RF transmit in M2.
- No actual radio PTT connection in M2.
- No audio path connected to a transmitter in M2.
- No network control path in M2.
- No PCB work in M2.
- PTT tests use a GPIO test pin only.

## PTT Defaults

- PTT state is off on boot.
- PTT state is off after reset.
- PTT state is off after watchdog reset.
- PTT GPIO is not enabled until explicitly configured in test firmware.
- Failures force the test PTT state off where the platform permits it.
- M2.1 host-native embedded tests verify app init, shutdown, and fault paths force the stub PTT state off.
- M2.2 host-native embedded tests verify simulated watchdog fault sets reset cause to watchdog, increments the platform fault counter, and forces the stub PTT state off.
- M2.3 host-native USB CDC tests verify echo, KISS loopback, malformed KISS recovery, and unsupported KISS commands do not change the stub PTT state.
- M2.4 host-native diagnostics tests verify PTT state, reset cause, watchdog fault state, USB counters, and KISS parse counters are visible through a bounded snapshot.
- M2.5 host-native audio tests verify audio loopback and audio error counters do not change the stub PTT state.
- M2.6 host-native embedded TNC tests verify KISS commands, malformed input, unsupported modes, and invalid modes do not change the stub PTT state.
- M2.7 host-native modem boundary tests verify simulated AFSK sample generation does not change the stub PTT state.
- M2.8 host-native RX modem tests verify audio RX processing and KISS output do not change the stub PTT state.
- M2.9 host-native full loopback tests verify USB KISS input, simulated audio TX/RX, and USB KISS output keep the stub PTT state off.
- M2.10 target skeleton files contain no pin assignments, no real PTT pin, no real watchdog hardware path, no startup code, and no flashable firmware image.
- M2.11 resource planning keeps the test PTT GPIO as `TBD` and marks all real pin assignments unverified.
- M2.12 target object checks do not link firmware, create flashable images, assign pins, access hardware registers, or key PTT.
- M2.13 USB stack planning keeps TinyUSB and STM32Cube unlinked and keeps the USB CDC stub as the only implemented path.
- M2.14 USB descriptor planning is metadata only and does not activate hardware USB, assign endpoints to a stack, or claim a final VID/PID.
- M2.14 board variant planning keeps PTT safe-off semantics required across H753, H743, H735, H750, RP2350, and any ESP32-S3 companion path.
- M2.15 clock, reset, boot, and watchdog planning keeps clock setup, watchdog enable, and reset register reads as future hardware-adapter work only.
- M2.16 memory, flash, and CPU budget planning keeps measured usage flags false until linker maps, high-water marks, and cycle measurements exist.
- M2.17 queue and backpressure planning keeps overflow policy explicit for every planned queue and marks control/PTT events as safety critical.

## Watchdog And Timeout

M2 firmware planning must include:

- Watchdog enabled only after a safe startup state is established.
- Watchdog reset path that returns PTT to off.
- Max TX timeout carried into embedded control state.
- Abort path that forces TX inactive and PTT off.
- Audio loopback errors that do not bypass watchdog or PTT safe-off.
- KISS/USB input that cannot directly key PTT.
- Unsupported or invalid mode requests that cannot activate unsafe paths.
- Simulated modem sample generation that cannot key PTT.
- Watchdog faults that abort active simulated modem TX.
- RX audio processing that cannot key PTT.
- Malformed or noise-like RX audio that cannot affect watchdog or PTT safe-off.
- Full host-test loopback that exits by max iteration timeout instead of locking up.
- Watchdog faults that abort full host-test loopback safely.
- USB and future network input that cannot directly key PTT.
- Future USB disconnect behavior that forces TX idle and PTT off.
- Future USB stack adapter failures that cannot bypass watchdog or PTT safe-off.
- Boot, debug, power, clock, USB, and Ethernet pins that are avoided for test PTT planning.
- Audio TX path that remains disconnected from any transmitter.

## M2.11 Pin Safety Rules

- No pin can be treated as real PTT until it is verified against ST documentation and the selected board schematic.
- No pin can be treated as real PTT until it is bench-tested with no radio connected.
- The test PTT GPIO must default off.
- USB and future network input must not directly key PTT.
- Boot, debug, power, clock, USB, and Ethernet pins must be avoided for test PTT planning.
- Audio TX must remain disconnected from any transmitter.

## M2.12 Target Check Safety Rules

- No flashable firmware image exists.
- The target object check cannot key PTT.
- No pin assignments are active.
- No linker script or startup vector table is present.
- No flash programming command is present.
- Target C files must remain free of STM32 HAL, CMSIS, TinyUSB, vendor headers, and hardware registers.

## M2.13 USB Safety Rules

- USB host input cannot directly key PTT.
- USB disconnect must force safe TX or idle state in future real stack work.
- Watchdog behavior must remain independent of USB task service.
- USB RX and TX queues must remain bounded.
- Malformed USB KISS input must be counted and recovered without unsafe state changes.
- TinyUSB and STM32Cube paths remain unsupported until explicitly integrated behind the boundary.

## M2.14 Descriptor And Board Safety Rules

- USB descriptor planning does not imply hardware USB is active.
- No final VID or PID is claimed.
- No binary descriptor arrays are bound to a stack.
- Board variants must preserve PTT off on boot, reset, watchdog fault, shutdown, and malformed input.
- H735 and H750 paths must not be enabled until memory, flash, and resource margins are checked.
- RP2350 must remain a separate target path, not a hidden substitute for STM32H753.
- ESP32-S3 companion planning must not place modem DSP or PTT authority on the companion.

## M2.15 Boot And Watchdog Safety Rules

- PTT or the test GPIO must be forced safe/off as early as possible in future startup.
- Reset cause must be captured early and reported through diagnostics.
- Real watchdog enable must wait until the safe-off path is proven.
- Watchdog refresh must require progress from the main loop, USB adapter, audio adapter, and PTT-control path.
- Watchdog fault must force PTT off and must be visible in diagnostics.
- USB and audio input must not be able to key PTT during boot.
- Clock tree values, PLL values, reset register reads, and watchdog register values remain unfinalized in M2.15.

## M2.16 Memory And Queue Safety Rules

- Memory exhaustion must not produce unsafe PTT behavior.
- Queue overflow must drop and count data instead of crashing.
- Real-time paths must keep heap usage zero or explicitly bounded.
- H735 and H750 paths must not be accepted until linker-map and runtime high-water measurements prove margin.
- Future CPU budget checks must include malformed KISS input, worst-case modem RX/TX, and watchdog progress timing.

## M2.17 Queue And Backpressure Safety Rules

- Queue overflow must never key PTT.
- Safety and control events must not be silently dropped.
- Watchdog refresh must not depend on a congested data queue.
- Malformed USB or KISS input must not starve the safety loop.
- USB TX overflow must drop and count data or diagnostics deterministically.
- Audio RX overflow and audio TX underflow must be counted and must not bypass safe-off state.
- Control/PTT event queue overflow is a safety fault until a verified runtime policy exists.

## Diagnostics Visibility

M2 must expose enough diagnostics for board tests:

- Boot/reset cause where available.
- Watchdog event counter where available.
- PTT state.
- USB/KISS parse error state.
- Audio overflow and underflow counters.
- Embedded TNC mode, KISS command, and mode request counters.
- Embedded modem TX active, request, done, rejected, abort, and sample counters.
- Embedded modem RX frame, sample, audio error, and output drop counters.
- Full loopback USB, audio, modem, watchdog, timeout, and final PTT counters.
- TX active state.
- Max TX timeout event.
- Fault counter snapshot.

The first diagnostics path may be a simple debug output or USB test path. It does not require a full embedded CLI in M2.0.

## Test Sequence Before Any Hardware PTT

Before any future real radio PTT connection:

1. Compile-only firmware skeleton builds.
2. Board boots with PTT test pin off.
3. Reset leaves PTT test pin off.
4. Watchdog reset leaves PTT test pin off.
5. Max TX timeout forces PTT test pin off.
6. Abort forces PTT test pin off.
7. Diagnostics show the tested state transitions.
8. USB KISS loopback is tested without any RF path.
9. Malformed USB/KISS input is tested without changing PTT state.
10. Unsupported mode requests are tested without changing PTT state.
11. Simulated modem TX is tested without changing PTT state.
12. Watchdog fault aborts simulated modem TX.
13. Simulated RX audio decode is tested without changing PTT state.
14. Malformed RX audio is tested without changing PTT state.
15. Full host-test loopback is tested without changing PTT state.
16. Full host-test loopback timeout and watchdog fault paths are tested.

Only after these gates should a later milestone consider hardware PTT connection planning.
