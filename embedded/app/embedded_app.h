/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/app/embedded_app.h */

#ifndef EMBEDDED_APP_H
#define EMBEDDED_APP_H

#include <sys/types.h>

#include <stdint.h>

#include "kilotnc_platform.h"

enum embedded_app_result {
	EMBEDDED_APP_OK = 0,
	EMBEDDED_APP_ERR_ARG,
	EMBEDDED_APP_ERR_PLATFORM,
	EMBEDDED_APP_ERR_FAULT
};

struct embedded_app_status {
	uint32_t tick_ms;
	size_t steps;
	size_t watchdog_kicks;
	int ptt_state;
	int faulted;
	int shutdown_requested;
};

struct embedded_app {
	const struct kilotnc_platform *platform;
	struct embedded_app_status status;
};

enum embedded_app_result embedded_app_fault(struct embedded_app *);
enum embedded_app_result embedded_app_init(struct embedded_app *,
	const struct kilotnc_platform *);
enum embedded_app_result embedded_app_shutdown(struct embedded_app *);
enum embedded_app_result embedded_app_status(const struct embedded_app *,
	struct embedded_app_status *);
enum embedded_app_result embedded_app_step(struct embedded_app *);

#endif
