# Network Hardware Plan

Baseline KiloTNC Rev A remains a USB/audio/PTT hardware TNC target. Ethernet and Wi-Fi are future variants or add-on modules, not Rev A requirements.

## Baseline Rev A

Rev A should stay focused on:

- USB-C device power and data.
- USB CDC KISS and diagnostics.
- External audio codec.
- Radio audio, PTT, COS, and optional CAT headers.
- ESD protection, filtering, test pads, and fail-safe PTT.

Network-capable hardware should wait until the modem core and safety path are proven.

## Ethernet Options

Future Ethernet options:

- MCU with integrated Ethernet MAC plus external PHY.
- SPI Ethernet controller module.
- USB-to-Ethernet only if the host or hardware design supports it.

Ethernet is not required for the first PCB.

## Wi-Fi Options

Future Wi-Fi options:

- External module controlled over UART or SPI.
- MCU or SBC companion.
- Separate network coprocessor.

Wi-Fi is not required for the first PCB.

## Safety Architecture

Network services must not directly key PTT.

Network packet input must pass through:

- Fixed-size input queue.
- Mode validation.
- Operator policy.
- DCD/channel-access logic.
- Max TX watchdog.
- Local PTT safety logic.

PTT fail-safe should remain local and hardware-enforced where possible.

## Security

Default security policy:

- Local-only services by default.
- No open access point mode by default.
- No unauthenticated command server by default.
- Separate diagnostic/control and packet-data roles.
- Explicit operator enablement for any remote control or remote packet-data path.

## Power, Noise, and Layout Concerns

Network-capable hardware adds layout and noise risks:

- Ethernet PHY clocks and magnetics can add noise.
- Wi-Fi radios can couple into radio/audio paths.
- External network connectors need ESD protection.
- Noisy external lines may need ferrites or common-mode filtering.
- Audio, clock, and RF-sensitive paths need placement separation.
- Continuous ground plane and connector-proximate protection remain required.

## Recommendation

- Rev A should remain USB/audio/PTT focused.
- Networked hardware should be Rev B or an add-on after core modem and safety behavior are proven.
- Any network-capable design must keep RF modem safety logic separate from IP/network services.

## Sources checked

| Source title                                     | Date checked | Note                                      |
| ------------------------------------------------ | ------------ | ----------------------------------------- |
| STM32H743BG product page, STMicroelectronics     | 2026-05-26   | MCU class already selected for Rev A path |
| TLV320AIC3204 product page, Texas Instruments    | 2026-05-26   | Codec remains separate from networking    |
| KiloTNC M1.8 TNC control design in this repo     | 2026-05-26   | Channel access and PTT safety model       |
