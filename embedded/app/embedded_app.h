/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/app/embedded_app.h */

#ifndef EMBEDDED_APP_H
#define EMBEDDED_APP_H

#include <sys/types.h>

#include <stdint.h>

#include "kilotnc_platform.h"

struct embedded_audio;
struct embedded_tnc;
struct embedded_usb_bridge;
struct kilotnc_usb_cdc;

enum embedded_app_result {
	EMBEDDED_APP_OK = 0,
	EMBEDDED_APP_ERR_ARG,
	EMBEDDED_APP_ERR_PLATFORM,
	EMBEDDED_APP_ERR_FAULT
};

enum embedded_app_state {
	EMBEDDED_APP_INIT = 0,
	EMBEDDED_APP_RUNNING,
	EMBEDDED_APP_FAULT,
	EMBEDDED_APP_STOPPED
};

struct embedded_app_status {
	enum embedded_app_state state;
	uint32_t tick_ms;
	uint32_t control_ticks_10ms;
	size_t steps;
	size_t watchdog_kicks;
	enum kilotnc_reset_cause reset_cause;
	enum kilotnc_gpio_state ptt_state;
	size_t fault_count;
	int faulted;
	int shutdown_requested;
};

struct embedded_app {
	const struct kilotnc_platform *platform;
	struct embedded_audio *audio_bridge;
	struct embedded_tnc *tnc;
	const struct kilotnc_usb_cdc *tnc_usb;
	struct embedded_usb_bridge *usb_bridge;
	struct embedded_app_status status;
};

enum embedded_app_result embedded_app_audio_bridge(struct embedded_app *,
	struct embedded_audio *);
enum embedded_app_result embedded_app_fault(struct embedded_app *);
enum embedded_app_result embedded_app_init(struct embedded_app *,
	const struct kilotnc_platform *);
enum embedded_app_result embedded_app_shutdown(struct embedded_app *);
enum embedded_app_result embedded_app_status(const struct embedded_app *,
	struct embedded_app_status *);
enum embedded_app_result embedded_app_step(struct embedded_app *);
enum embedded_app_result embedded_app_tnc(struct embedded_app *,
	struct embedded_tnc *, const struct kilotnc_usb_cdc *);
enum embedded_app_result embedded_app_usb_bridge(struct embedded_app *,
	struct embedded_usb_bridge *);

#endif
