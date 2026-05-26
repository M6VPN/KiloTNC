/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/app/embedded_app.c */

#include <sys/types.h>

#include <stdint.h>
#include <string.h>

#include "embedded_audio.h"
#include "embedded_app.h"
#include "embedded_tnc.h"
#include "embedded_usb_bridge.h"

static enum embedded_app_result embedded_app_ptt_off(struct embedded_app *);
static enum embedded_app_result embedded_app_refresh_status(
	struct embedded_app *);
static enum embedded_app_result embedded_app_set_fault(struct embedded_app *);
static int embedded_app_platform_ready(const struct kilotnc_platform *);

static enum embedded_app_result
embedded_app_ptt_off(struct embedded_app *app)
{
	if (app->platform->ptt_set(app->platform->ctx, KILOTNC_GPIO_LOW) !=
	    KILOTNC_PLATFORM_OK)
		return EMBEDDED_APP_ERR_PLATFORM;

	app->status.ptt_state = KILOTNC_GPIO_LOW;
	return EMBEDDED_APP_OK;
}

static enum embedded_app_result
embedded_app_refresh_status(struct embedded_app *app)
{
	if (app->platform->monotonic_ms(app->platform->ctx,
	    &app->status.tick_ms) != KILOTNC_PLATFORM_OK)
		return EMBEDDED_APP_ERR_PLATFORM;
	if (app->platform->reset_cause(app->platform->ctx,
	    &app->status.reset_cause) != KILOTNC_PLATFORM_OK)
		return EMBEDDED_APP_ERR_PLATFORM;
	if (app->platform->ptt_get(app->platform->ctx,
	    &app->status.ptt_state) != KILOTNC_PLATFORM_OK)
		return EMBEDDED_APP_ERR_PLATFORM;
	if (app->platform->fault_count(app->platform->ctx,
	    &app->status.fault_count) != KILOTNC_PLATFORM_OK)
		return EMBEDDED_APP_ERR_PLATFORM;

	return EMBEDDED_APP_OK;
}

static enum embedded_app_result
embedded_app_set_fault(struct embedded_app *app)
{
	enum embedded_app_result result;

	app->status.state = EMBEDDED_APP_FAULT;
	app->status.faulted = 1;
	result = embedded_app_ptt_off(app);
	if (result != EMBEDDED_APP_OK)
		return result;

	return embedded_app_refresh_status(app);
}

static int
embedded_app_platform_ready(const struct kilotnc_platform *platform)
{
	if (platform == NULL)
		return 0;
	if (platform->monotonic_ms == NULL)
		return 0;
	if (platform->tick_10ms == NULL)
		return 0;
	if (platform->watchdog_kick == NULL)
		return 0;
	if (platform->watchdog_faulted == NULL)
		return 0;
	if (platform->reset_cause == NULL)
		return 0;
	if (platform->ptt_set == NULL)
		return 0;
	if (platform->ptt_get == NULL)
		return 0;
	if (platform->fault_count == NULL)
		return 0;

	return 1;
}

enum embedded_app_result
embedded_app_fault(struct embedded_app *app)
{
	if (app == NULL || !embedded_app_platform_ready(app->platform))
		return EMBEDDED_APP_ERR_ARG;

	return embedded_app_set_fault(app);
}

enum embedded_app_result
embedded_app_init(struct embedded_app *app,
	const struct kilotnc_platform *platform)
{
	enum embedded_app_result result;

	if (app == NULL || !embedded_app_platform_ready(platform))
		return EMBEDDED_APP_ERR_ARG;

	(void)memset(app, 0, sizeof(*app));
	app->platform = platform;
	app->status.state = EMBEDDED_APP_INIT;
	result = embedded_app_ptt_off(app);
	if (result != EMBEDDED_APP_OK)
		return result;
	result = embedded_app_refresh_status(app);
	if (result != EMBEDDED_APP_OK)
		return result;

	app->status.state = EMBEDDED_APP_RUNNING;
	return EMBEDDED_APP_OK;
}

enum embedded_app_result
embedded_app_shutdown(struct embedded_app *app)
{
	enum embedded_app_result result;

	if (app == NULL || !embedded_app_platform_ready(app->platform))
		return EMBEDDED_APP_ERR_ARG;

	app->status.shutdown_requested = 1;
	app->status.state = EMBEDDED_APP_STOPPED;
	result = embedded_app_ptt_off(app);
	if (result != EMBEDDED_APP_OK)
		return result;

	return embedded_app_refresh_status(app);
}

enum embedded_app_result
embedded_app_status(const struct embedded_app *app,
	struct embedded_app_status *status)
{
	if (app == NULL || status == NULL)
		return EMBEDDED_APP_ERR_ARG;

	*status = app->status;
	return EMBEDDED_APP_OK;
}

enum embedded_app_result
embedded_app_step(struct embedded_app *app)
{
	uint32_t control_ticks;
	int watchdog_faulted;

	if (app == NULL || !embedded_app_platform_ready(app->platform))
		return EMBEDDED_APP_ERR_ARG;
	if (app->status.state == EMBEDDED_APP_FAULT)
		return EMBEDDED_APP_ERR_FAULT;
	if (app->status.state == EMBEDDED_APP_STOPPED)
		return EMBEDDED_APP_ERR_FAULT;

	if (app->platform->watchdog_faulted(app->platform->ctx,
	    &watchdog_faulted) != KILOTNC_PLATFORM_OK)
		return EMBEDDED_APP_ERR_PLATFORM;
	if (watchdog_faulted != 0) {
		if (embedded_app_set_fault(app) != EMBEDDED_APP_OK)
			return EMBEDDED_APP_ERR_PLATFORM;
		return EMBEDDED_APP_ERR_FAULT;
	}

	if (app->platform->tick_10ms(app->platform->ctx, &control_ticks) !=
	    KILOTNC_PLATFORM_OK) {
		if (embedded_app_set_fault(app) != EMBEDDED_APP_OK)
			return EMBEDDED_APP_ERR_PLATFORM;
		return EMBEDDED_APP_ERR_FAULT;
	}
	if (app->platform->watchdog_kick(app->platform->ctx) !=
	    KILOTNC_PLATFORM_OK) {
		(void)embedded_app_set_fault(app);
		return EMBEDDED_APP_ERR_FAULT;
	}
	if (app->tnc != NULL && app->tnc_usb != NULL &&
	    embedded_tnc_process_usb(app->tnc, app->tnc_usb) !=
	    EMBEDDED_TNC_OK) {
		if (embedded_app_set_fault(app) != EMBEDDED_APP_OK)
			return EMBEDDED_APP_ERR_PLATFORM;
		return EMBEDDED_APP_ERR_FAULT;
	}
	if (app->usb_bridge != NULL &&
	    embedded_usb_bridge_service(app->usb_bridge) !=
	    EMBEDDED_USB_BRIDGE_OK) {
		if (embedded_app_set_fault(app) != EMBEDDED_APP_OK)
			return EMBEDDED_APP_ERR_PLATFORM;
		return EMBEDDED_APP_ERR_FAULT;
	}
	if (app->audio_bridge != NULL &&
	    embedded_audio_process(app->audio_bridge) != EMBEDDED_AUDIO_OK) {
		if (embedded_app_set_fault(app) != EMBEDDED_APP_OK)
			return EMBEDDED_APP_ERR_PLATFORM;
		return EMBEDDED_APP_ERR_FAULT;
	}

	app->status.control_ticks_10ms = control_ticks;
	app->status.steps++;
	app->status.watchdog_kicks++;
	return embedded_app_refresh_status(app);
}

enum embedded_app_result
embedded_app_audio_bridge(struct embedded_app *app,
	struct embedded_audio *bridge)
{
	if (app == NULL || !embedded_app_platform_ready(app->platform))
		return EMBEDDED_APP_ERR_ARG;

	app->audio_bridge = bridge;
	return EMBEDDED_APP_OK;
}

enum embedded_app_result
embedded_app_tnc(struct embedded_app *app, struct embedded_tnc *tnc,
	const struct kilotnc_usb_cdc *usb)
{
	if (app == NULL || !embedded_app_platform_ready(app->platform) ||
	    tnc == NULL || usb == NULL)
		return EMBEDDED_APP_ERR_ARG;

	app->tnc = tnc;
	app->tnc_usb = usb;
	return EMBEDDED_APP_OK;
}

enum embedded_app_result
embedded_app_usb_bridge(struct embedded_app *app,
	struct embedded_usb_bridge *bridge)
{
	if (app == NULL || !embedded_app_platform_ready(app->platform))
		return EMBEDDED_APP_ERR_ARG;

	app->usb_bridge = bridge;
	return EMBEDDED_APP_OK;
}
