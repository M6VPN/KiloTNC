/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/include/kilotnc_platform.h */

#ifndef KILOTNC_PLATFORM_H
#define KILOTNC_PLATFORM_H

#include <sys/types.h>

#include <stdint.h>

enum kilotnc_platform_result {
	KILOTNC_PLATFORM_OK = 0,
	KILOTNC_PLATFORM_ERR_ARG,
	KILOTNC_PLATFORM_ERR_UNSUPPORTED,
	KILOTNC_PLATFORM_ERR_FAULT
};

enum kilotnc_reset_cause {
	KILOTNC_RESET_UNKNOWN = 0,
	KILOTNC_RESET_POWER_ON,
	KILOTNC_RESET_SOFTWARE,
	KILOTNC_RESET_WATCHDOG,
	KILOTNC_RESET_BROWNOUT
};

enum kilotnc_gpio_state {
	KILOTNC_GPIO_LOW = 0,
	KILOTNC_GPIO_HIGH
};

struct kilotnc_platform {
	void *ctx;
	enum kilotnc_platform_result (*monotonic_ms)(void *, uint32_t *);
	enum kilotnc_platform_result (*tick_10ms)(void *, uint32_t *);
	enum kilotnc_platform_result (*watchdog_kick)(void *);
	enum kilotnc_platform_result (*watchdog_faulted)(void *, int *);
	enum kilotnc_platform_result (*reset_cause)(void *,
		enum kilotnc_reset_cause *);
	enum kilotnc_platform_result (*ptt_set)(void *,
		enum kilotnc_gpio_state);
	enum kilotnc_platform_result (*ptt_get)(void *,
		enum kilotnc_gpio_state *);
	enum kilotnc_platform_result (*diag_write)(void *, const char *);
	enum kilotnc_platform_result (*diag_count)(void *, size_t *);
	enum kilotnc_platform_result (*fault_count)(void *, size_t *);
	enum kilotnc_platform_result (*usb_poll)(void *);
	enum kilotnc_platform_result (*audio_poll)(void *);
};

#endif
