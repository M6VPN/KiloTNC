/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/app/embedded_app.h */

#ifndef EMBEDDED_APP_H
#define EMBEDDED_APP_H

#include <sys/types.h>

#include <stdint.h>

#include "kilotnc_platform.h"
#include "kilotnc_scheduler.h"

struct embedded_audio;
struct embedded_modem;
struct embedded_tnc;
struct embedded_usb_bridge;
struct kilotnc_usb_cdc;
struct kilotnc_audio;

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
	size_t scheduler_cycles;
	size_t scheduler_faults;
	uint32_t scheduler_enabled_mask;
	uint32_t scheduler_required_mask;
	uint32_t scheduler_progress_mask;
	enum embedded_task_id scheduler_last_failed_task;
	enum kilotnc_reset_cause reset_cause;
	enum kilotnc_gpio_state ptt_state;
	size_t fault_count;
	int faulted;
	int shutdown_requested;
	int scheduler_watchdog_allowed;
};

struct embedded_app {
	const struct kilotnc_platform *platform;
	struct embedded_scheduler scheduler;
	struct embedded_audio *audio_bridge;
	struct embedded_modem *modem;
	const struct kilotnc_audio *modem_audio;
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
enum embedded_app_result embedded_app_modem(struct embedded_app *,
	struct embedded_modem *, const struct kilotnc_audio *);
enum embedded_app_result embedded_app_shutdown(struct embedded_app *);
enum embedded_app_result embedded_app_status(const struct embedded_app *,
	struct embedded_app_status *);
enum embedded_app_result embedded_app_step(struct embedded_app *);
enum embedded_app_result embedded_app_tnc(struct embedded_app *,
	struct embedded_tnc *, const struct kilotnc_usb_cdc *);
enum embedded_app_result embedded_app_usb_bridge(struct embedded_app *,
	struct embedded_usb_bridge *);

#endif
