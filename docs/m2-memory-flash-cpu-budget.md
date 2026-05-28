# M2 Memory, Flash, And CPU Budget

M2.16 records conservative planning budgets before real STM32 target firmware begins.

## Scope

This document is planning only.

- No linker map exists yet.
- No final measured embedded binary size exists.
- No final measured stack high-water data exists.
- No final CPU cycle measurements exist.
- No flashable firmware exists.
- No real hardware target build is required.
- No linker script or startup code is added.

Host object sizes and host binary sizes are not MCU flash estimates.

## Target Memory Classes

Official ST product pages and datasheets give these headline classes:

| Target class | Flash headline | RAM headline | M2.16 role |
| ------------ | -------------- | ------------ | ---------- |
| STM32H753ZI  | 2 MB           | 1 MB         | Flagship M2/M3 development path |
| STM32H743ZI  | up to 2 MB     | up to 1 MB   | First custom serious board option |
| STM32H735ZG  | 1 MB           | up to 564 KB | Cost-reduced option after measured margins |
| STM32H750    | 128 KB         | 1 MB SRAM    | Memory and flash validation-gated option |
| RP2350       | 4 MB external flash on Pico 2 class boards, 520 KB SRAM | 520 KB SRAM | Separate low-cost/simple target context |

Exact usable RAM regions and linker placement are future work. Not all SRAM is equally usable for DMA, cache, peripheral paths, and time-critical data. Final memory layout requires a linker script, a target reference manual pass, and measured linker-map output.

## Budget Categories

Planning categories:

- Protocol buffers.
- KISS parser buffers.
- AX.25 frame buffers.
- HDLC bit buffers.
- AFSK1200 RX state.
- AFSK1200 TX state.
- Audio RX and TX buffers.
- USB RX and TX queues.
- Diagnostics and fault ring.
- Mode and config state.
- Platform and app state.
- Future IL2Pc and FEC buffers.
- Future 9600 GFSK buffers.
- Future USB stack buffers.
- Future codec and DMA buffers.

## Current Conservative Estimate

These are planning estimates derived from current fixed limits and stub buffers where practical. They are not linker-map measurements.

| Category                 | Estimated RAM | Confidence | Notes |
| ------------------------ | ------------- | ---------- | ----- |
| AX.25 max frame buffer   | 330 bytes     | high       | Derived from current `KILOTNC_AX25_MAX_FRAME` limit |
| KISS parser frame buffer | 1025 bytes    | high       | Derived from current `KILOTNC_KISS_MAX_FRAME` limit |
| USB RX queue stub        | 512 bytes     | high       | Current host-test USB CDC stub RX buffer |
| USB TX queue stub        | 512 bytes     | high       | Current host-test USB CDC stub TX buffer |
| Audio RX stub buffer     | 512 bytes     | high       | 256 signed 16-bit samples |
| Audio TX stub buffer     | 512 bytes     | high       | 256 signed 16-bit samples |
| Audio block buffer       | 128 bytes     | high       | 64 signed 16-bit samples |
| Modem TX chunk           | 128 bytes     | high       | 64 signed 16-bit samples |
| Modem RX chunk           | 128 bytes     | high       | 64 signed 16-bit samples |
| Diagnostics fault ring   | small         | medium     | Current ring is 16 enum entries; enum width is compiler-dependent |
| AFSK1200 RX state        | TBD           | medium     | Includes sample window and parser state, final size needs target compile |
| AFSK1200 TX state        | TBD           | medium     | Current fixed state is small, final size needs target compile |
| Embedded app state       | TBD           | medium     | Depends on enabled bridges and target build options |
| Future USB stack buffers | TBD           | low        | Depends on TinyUSB or STM32Cube integration |
| Future codec/DMA buffers | TBD           | low        | Depends on audio peripheral and DMA design |
| Future IL2Pc/FEC buffers | TBD           | low        | Future mode work |
| Future 9600 mode buffers | TBD           | low        | Future mode work |

## Flash And Code Budget

Current host object sizes are not embedded flash sizes. Final flash use requires:

- ARM object build.
- Linker script.
- Linker map.
- Dead-code elimination policy.
- Chosen USB stack.
- Chosen diagnostics formatting path.
- Target compiler flags.

Future flash budget categories:

- Portable core.
- Embedded app.
- USB stack.
- Diagnostics.
- AFSK1200 modem.
- Future IL2Pc and FEC.
- Future 9600 mode.
- Config and persistence.
- Startup and target platform adapter.

M2.16 makes no final flash usage claim.

## CPU Budget

Host tests do not prove MCU real-time CPU margin.

AFSK1200 is a low-rate modem, but embedded processing still needs bounded timing for:

- Samples per block.
- Bits per frame.
- KISS parsing.
- AX.25/FCS checks.
- USB queue service.
- Audio queue service.
- Watchdog progress reporting.

Future CPU work:

- Estimate cycles per sample.
- Estimate cycles per bit.
- Estimate cycles per decoded frame.
- Measure on target using DWT cycle counter or a timer once hardware code exists.
- Verify watchdog progress timing under malformed KISS and noisy audio cases.

M2.16 makes no final CPU margin claim.

## Variant Gates

STM32H753 and STM32H743:

- Must have comfortable RAM and flash headroom after USB, audio, modem, diagnostics, and safety paths are included.
- Must pass linker-map review and runtime high-water checks before hardware PTT work.

STM32H735:

- Cost-reduced board candidate only after measured linker-map and runtime high-water marks prove margin.
- Must prove USB, audio, modem, diagnostics, and watchdog paths fit without reducing safety buffers below reviewed limits.

STM32H750:

- Candidate only after flash and memory assumptions are verified.
- The 128 KB embedded flash class requires explicit code-placement and external-memory or external-flash implications review before use.
- Must not drive the flagship H753 design limits before measured evidence exists.

RP2350:

- Separate target, not a drop-in STM32H7 variant.
- Useful for low-cost or simple TNC experiments after the flagship path has stable boundaries.

ESP32-S3:

- Connectivity companion only.
- Not the modem DSP core and not the owner of PTT safety.

## Required Future Measurements

Before accepting cost-reduced or constrained targets:

- Linker map flash use.
- Linker map RAM use by region.
- Stack high-water marks.
- Heap usage, which should stay zero or bounded.
- Audio buffer high-water marks.
- USB queue high-water marks.
- Worst-case KISS malformed input handling.
- Worst-case modem RX path.
- Worst-case modem TX path.
- Watchdog task progress timing.
- CPU cycles per sample.
- CPU cycles per frame.

## M2.16 Metadata

`embedded/include/kilotnc_budget.h` records host-testable planning guards:

- Budget finalized flag is false.
- Linker map available flag is false.
- Stack high-water measured flag is false.
- CPU cycles measured flag is false.
- Audio sample rate is 48000 Hz.
- Control tick is 10 ms.
- Current key fixed-buffer limits are nonzero and bounded.

These constants are not final target measurements.

## M2.17 Queue Capacity Impact

`docs/m2-queue-backpressure-plan.md` defines planned queue boundaries and overflow policy. Queue capacities must be included in future RAM budget and linker-map review.

Current queue policy metadata references the existing USB, audio, frame, diagnostics, and control planning constants where possible. Future runtime queue work must update this budget with:

- Actual queue storage per target.
- Queue high-water marks.
- Overflow counters.
- ISR or DMA ownership rules.
- Safety/control queue fault behavior.
- Diagnostics/event queue retention behavior.

## Sources Checked

Sources checked on 2026-05-27:

- STM32H753ZI product page, STMicroelectronics: https://www.st.com/en/microcontrollers-microprocessors/stm32h753zi.html
- STM32H753xI datasheet, STMicroelectronics: https://www.st.com/resource/en/datasheet/stm32h753zi.pdf
- STM32H743ZI product page, STMicroelectronics: https://www.st.com/en/microcontrollers-microprocessors/stm32h743zi.html
- STM32H735ZG product page, STMicroelectronics: https://www.st.com/en/microcontrollers-microprocessors/stm32h735zg.html
- STM32H750 Value line page, STMicroelectronics: https://www.st.com/en/microcontrollers-microprocessors/stm32h750-value-line.html
- RP2350 documentation, Raspberry Pi: https://www.raspberrypi.com/documentation/microcontrollers/rp2350.html
