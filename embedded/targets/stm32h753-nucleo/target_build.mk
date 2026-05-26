# KiloTNC - Developed by M6VPN (M6VPN@tuta.com)
# KiloTNC/embedded/targets/stm32h753-nucleo/target_build.mk

ARM_NONE_EABI_CC ?= arm-none-eabi-gcc
KILOTNC_STM32H753_BUILD_DIR ?= build/embedded-target

include embedded/targets/stm32h753-nucleo/target_sources.mk

KILOTNC_STM32H753_TARGET_CFLAGS = \
	-std=c99 \
	-Wall \
	-Wextra \
	-Wconversion \
	-Wsign-conversion \
	-Werror \
	$(addprefix -D,$(KILOTNC_STM32H753_TARGET_GATE)) \
	$(addprefix -I,$(KILOTNC_STM32H753_TARGET_INCLUDES))

KILOTNC_STM32H753_TARGET_OBJS = \
	$(KILOTNC_STM32H753_BUILD_DIR)/target_main.o \
	$(KILOTNC_STM32H753_BUILD_DIR)/target_platform.o

embedded-target-object-check: $(KILOTNC_STM32H753_TARGET_OBJS)
	@printf '%s\n' 'checked stm32h753-nucleo target skeleton objects'
	@printf '%s\n' 'no ELF/bin/hex generated'

embedded-target-print-sources:
	@printf '%s\n' 'stm32h753-nucleo target sources:'
	@printf '%s\n' $(KILOTNC_STM32H753_TARGET_SRCS)

$(KILOTNC_STM32H753_BUILD_DIR)/target_main.o: embedded/targets/stm32h753-nucleo/target_main.c
	mkdir -p $(KILOTNC_STM32H753_BUILD_DIR)
	$(ARM_NONE_EABI_CC) $(KILOTNC_STM32H753_TARGET_CFLAGS) -c $< -o $@

$(KILOTNC_STM32H753_BUILD_DIR)/target_platform.o: embedded/targets/stm32h753-nucleo/target_platform.c
	mkdir -p $(KILOTNC_STM32H753_BUILD_DIR)
	$(ARM_NONE_EABI_CC) $(KILOTNC_STM32H753_TARGET_CFLAGS) -c $< -o $@

.PHONY: embedded-target-object-check embedded-target-print-sources
