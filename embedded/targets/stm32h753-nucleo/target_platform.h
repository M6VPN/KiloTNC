/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/targets/stm32h753-nucleo/target_platform.h */

#ifndef TARGET_PLATFORM_STM32H753_NUCLEO_H
#define TARGET_PLATFORM_STM32H753_NUCLEO_H

#include <sys/types.h>

#include <stdint.h>

#include "kilotnc_platform.h"
#include "target_config.h"

struct stm32h753_nucleo_platform {
	struct kilotnc_platform platform;
	uint32_t tick_ms;
	size_t diag_writes;
	enum kilotnc_gpio_state ptt_state;
};

enum kilotnc_platform_result stm32h753_nucleo_platform_init(
	struct stm32h753_nucleo_platform *);

#endif
