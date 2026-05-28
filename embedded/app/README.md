# Embedded Application

The embedded application layer will connect the portable TNC core to platform adapters.

Planned responsibilities:

- Initialize safe default state.
- Keep PTT off unless explicitly enabled by test firmware.
- Route USB CDC KISS bytes to the core.
- Route audio test data through the core after loopback paths exist.
- Publish diagnostics and fault counters.
- Enforce watchdog and max TX timeout policy.

M2.3 adds an optional USB bridge hook. Tests can attach the host-native USB CDC stub so `embedded_app_step()` services byte echo or KISS data-frame loopback while still kicking the watchdog and keeping PTT off.

The bridge is not a USB device stack. It has no descriptors, endpoints, TinyUSB calls, HAL calls, or hardware access.

M2.4 adds an embedded diagnostics bridge. It captures app state, reset cause, PTT state, platform ticks, watchdog kicks, diagnostic writes, USB byte counters, USB overflow counters, and KISS parser counters into a fixed snapshot. The formatter writes stable text into caller-provided buffers and detects truncation.

M2.5 adds an optional audio loopback hook. Tests can attach the host-native audio stub so `embedded_app_step()` copies injected RX samples to captured TX samples while still kicking the watchdog and keeping PTT off. This is sample-path only and does not run modem DSP.

M2.6 adds an optional embedded TNC hook. Tests can attach the host-native USB CDC stub so `embedded_app_step()` routes KISS bytes through the embedded TNC skeleton, updates mode and KISS counters, and keeps PTT off. This does not emit modem audio or key hardware.

M2.7 adds an optional embedded modem hook. Tests can enable simulated modem TX so accepted embedded TNC KISS data frames request AFSK1200 sample generation into the audio stub. `embedded_app_step()` services bounded modem chunks, keeps the watchdog path active, aborts modem TX on shutdown or watchdog fault, and keeps PTT off.

M2.8 extends the modem hook with RX audio processing. Tests can enable modem RX so `embedded_app_step()` reads samples from the audio RX stub, decodes frames through the portable AFSK1200 streaming RX path, and writes KISS data frames to the USB CDC stub. RX is disabled by default and does not control PTT.

M2.9 adds a full host-test loopback helper. It injects USB KISS input, enables simulated modem TX/RX, copies generated TX audio samples into the RX stub, and verifies KISS output from the USB stub. The helper is bounded by max iterations, records counters, and does not add real USB, audio, GPIO, or RF behavior.

M2.17 documents queue boundaries between USB RX/TX, KISS frames, modem TX/RX frames, audio TX/RX samples, diagnostics events, and control/PTT events. The current app code still uses host-test stubs and direct calls; runtime queues, ISR boundaries, DMA boundaries, and scheduler behavior remain future work.
