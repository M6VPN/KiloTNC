/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/targets/stm32h753-nucleo/target_main.c */

#ifndef KILOTNC_TARGET_STM32H753_NUCLEO
#error "Define KILOTNC_TARGET_STM32H753_NUCLEO for target skeleton builds."
#endif

#include <sys/types.h>

#include "target_config.h"
#include "target_platform.h"

int
kilotnc_target_main(void)
{
	struct stm32h753_nucleo_platform platform;

	if (stm32h753_nucleo_platform_init(&platform) !=
	    KILOTNC_PLATFORM_OK)
		return 1;

	return 0;
}
