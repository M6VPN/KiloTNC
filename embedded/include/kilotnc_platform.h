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

struct kilotnc_platform {
	void *ctx;
	enum kilotnc_platform_result (*tick_ms)(void *, uint32_t *);
	enum kilotnc_platform_result (*watchdog_kick)(void *);
	enum kilotnc_platform_result (*ptt_set)(void *, int);
	enum kilotnc_platform_result (*diag_write)(void *, const char *);
	enum kilotnc_platform_result (*usb_poll)(void *);
	enum kilotnc_platform_result (*audio_poll)(void *);
};

#endif
