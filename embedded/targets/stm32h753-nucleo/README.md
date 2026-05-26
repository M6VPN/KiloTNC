# stm32h753-nucleo Target

This is the selected primary M2 target path.

Board path:

- NUCLEO-H753ZI or current equivalent STM32H753 Nucleo-144 board.

M2.6 status:

- Compile-only skeleton.
- Host-native platform stub tests.
- Host-native USB CDC byte-stream and KISS bridge tests.
- Host-native embedded diagnostics bridge tests.
- Host-native audio stub and loopback tests.
- Host-native embedded TNC KISS, mode, and diagnostics tests.
- No vendor project committed.
- No STM32 HAL or Cube code committed.
- No CMSIS tree committed.
- No real USB implementation.
- No codec driver.
- No real audio peripheral implementation.
- No audio DMA.
- No GPIO PTT implementation.
- No pinout finalized.
- No clock tree finalized.

The target header contains metadata and planned feature flags only.
