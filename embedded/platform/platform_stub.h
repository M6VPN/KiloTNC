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
	uint32_t monotonic_ms;
	uint32_t control_ticks_10ms;
	size_t watchdog_kicks;
	size_t diag_writes;
	size_t platform_faults;
	int watchdog_faulted;
	enum kilotnc_reset_cause reset_cause;
	enum kilotnc_gpio_state ptt_state;
	char diag_last[PLATFORM_STUB_DIAG_MAX];
};

enum kilotnc_platform_result platform_stub_advance_ms(
	struct platform_stub *, uint32_t);
enum kilotnc_platform_result platform_stub_ptt_state(
	const struct platform_stub *, enum kilotnc_gpio_state *);
const struct kilotnc_platform *platform_stub_platform(
	struct platform_stub *);
void platform_stub_init(struct platform_stub *);
enum kilotnc_platform_result platform_stub_simulate_watchdog_fault(
	struct platform_stub *);

#endif
