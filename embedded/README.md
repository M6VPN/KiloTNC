# Embedded Firmware Workspace

This directory is the M2 workspace for future dev-board firmware.

M2.12 adds an opt-in STM32H753 target object check. No real USB diagnostics channel, USB stack, descriptors, ADC, DAC, SAI, I2S, audio DMA, codec, real GPIO PTT, HAL, board driver code, startup vector table, linker script, flashable firmware image, or hardware audio path is implemented here yet.

The host-side M1 portable core remains in `firmware/`. Embedded firmware will use adapters around that portable core rather than copying daemon file, socket, PTY, or host audio code.

Planned layout:

- `targets/`: target-specific board support.
- `platform/`: platform abstraction for tick, watchdog, reset, GPIO, USB, audio, diagnostics, and config.
- `app/`: embedded application state machine.
- `tests/`: embedded compile and board test notes.
- `common/`: shared embedded-only helpers if needed later.

The selected M2 primary target path is documented in `docs/m2-target-selection.md`.

Run the host-native skeleton test with:

```text
make embedded-test
```

Show target skeleton guidance with:

```text
make embedded-target-help
```

Run the opt-in skip-safe target skeleton object check with:

```text
make embedded-target-check
```

The USB CDC skeleton uses fixed buffers and the existing portable KISS parser. It can echo bytes or loop KISS data frames back in tests, but it does not call TinyUSB, STM32Cube, CMSIS, hardware registers, or endpoint drivers.

The diagnostics bridge captures a bounded snapshot and formats it into caller-provided text buffers. It is intended for future USB CDC diagnostics, but M2.4 does not send diagnostics over USB.

The audio loopback path uses fixed sample buffers and mono signed 16-bit 48 kHz format metadata. Tests can inject RX samples, capture TX samples, and verify overflow and underflow counters. M2.5 does not connect audio samples to a modem or transmitter path.

The embedded TNC skeleton reads KISS bytes from the USB CDC stub, handles basic KISS commands, maps Nino-compatible SETHW mode 6 and 22 through the shared mode registry, keeps unsupported modes inactive, and can loop accepted data frames back to USB in tests. M2.6 does not connect KISS frames to modem audio or hardware PTT.

The embedded modem boundary can accept AX.25 frames with FCS from the embedded TNC test path and generate simulated AFSK1200 samples into the audio stub. This path is disabled by default, uses fixed buffers, does not key PTT, and does not connect samples to hardware or RF.

The embedded RX modem boundary reads generated test samples from the audio RX stub, feeds them through the portable AFSK1200 streaming RX path, and emits decoded AX.25 frames as KISS data frames to the USB CDC stub when RX is explicitly enabled. It is host-tested only and does not use receiver hardware or key PTT.

The embedded full host-test loopback coordinates the USB CDC stub, embedded TNC, embedded modem TX, audio TX stub, audio RX stub, modem RX, and USB KISS output in one bounded run. Tests copy generated TX samples into the RX stub, verify the decoded KISS output, enforce a max iteration timeout, and keep PTT off.

The STM32H753 target skeleton under `embedded/targets/stm32h753-nucleo/` contains metadata, compile-gated placeholder source files, and planning notes for future linker, startup, USB, watchdog, GPIO, and audio work. The target source files do not include STM32 HAL, CMSIS, TinyUSB, vendor headers, hardware registers, pin assignments, or real drivers.

The M2.11 resource metadata header records planned USB, test PTT GPIO, diagnostics, audio, watchdog, and reset resources while keeping all real pin assignments marked unverified. The resource plan is in `docs/m2-stm32h753-resource-plan.md`.

M2.12 adds target-local build metadata in `target_sources.mk` and `target_build.mk`, plus `check_target_compile.sh`. The check compiles only target skeleton objects under `build/embedded-target/` when `arm-none-eabi-gcc` is available, skips cleanly when it is absent, and does not link or create ELF, BIN, HEX, startup, linker, HAL, CMSIS, TinyUSB, or vendor output.

M2.13 adds USB CDC stack selection planning and a dependency-boundary placeholder. The future preferred path is a TinyUSB adapter boundary, with STM32Cube USB Device as fallback. The current implemented path remains the host-native USB CDC stub only.
