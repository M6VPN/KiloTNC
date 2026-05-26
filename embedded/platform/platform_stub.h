/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/platform/platform_stub.h */

#ifndef PLATFORM_STUB_H
#define PLATFORM_STUB_H

#include <sys/types.h>

#include <stdint.h>

#include "kilotnc_platform.h"

#define PLATFORM_STUB_DIAG_MAX 64

struct platform_stub {
	struct kilotnc_platform platform;
	uint32_t tick_ms;
	size_t watchdog_kicks;
	size_t diag_writes;
	int ptt_state;
	char diag_last[PLATFORM_STUB_DIAG_MAX];
};

const struct kilotnc_platform *platform_stub_platform(
	struct platform_stub *);
void platform_stub_init(struct platform_stub *);

#endif
