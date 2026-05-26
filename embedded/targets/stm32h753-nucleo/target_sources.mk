# KiloTNC - Developed by M6VPN (M6VPN@tuta.com)
# KiloTNC/embedded/targets/stm32h753-nucleo/target_sources.mk

KILOTNC_STM32H753_TARGET_NAME = stm32h753-nucleo
KILOTNC_STM32H753_TARGET_GATE = KILOTNC_TARGET_STM32H753_NUCLEO

KILOTNC_STM32H753_TARGET_SRCS = \
	embedded/targets/stm32h753-nucleo/target_main.c \
	embedded/targets/stm32h753-nucleo/target_platform.c

KILOTNC_STM32H753_TARGET_INCLUDES = \
	embedded/include \
	embedded/app \
	embedded/platform \
	embedded/targets/stm32h753-nucleo

KILOTNC_STM32H753_VENDOR_INCLUDES =
KILOTNC_STM32H753_LINKER_SCRIPT =
KILOTNC_STM32H753_STARTUP_OBJECT =
KILOTNC_STM32H753_FLASH_IMAGE =
