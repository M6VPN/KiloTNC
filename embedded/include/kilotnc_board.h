/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/include/kilotnc_board.h */

#ifndef KILOTNC_BOARD_H
#define KILOTNC_BOARD_H

#include <sys/types.h>

#include <stdint.h>

#define KILOTNC_BOARD_FEATURE_USB_DEVICE_PLANNED		(1u << 0)
#define KILOTNC_BOARD_FEATURE_WATCHDOG_PLANNED		(1u << 1)
#define KILOTNC_BOARD_FEATURE_SAI_I2S_PLANNED		(1u << 2)
#define KILOTNC_BOARD_FEATURE_GPIO_TEST_PTT_PLANNED	(1u << 3)

struct kilotnc_board_desc {
	const char *target_name;
	const char *mcu_family;
	const char *board_class;
	uint32_t features;
};

#endif
