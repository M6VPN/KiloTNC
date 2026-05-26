/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/app/embedded_app.c */

#include <sys/types.h>

#include <stdint.h>
#include <string.h>

#include "embedded_app.h"

static enum embedded_app_result embedded_app_ptt_off(struct embedded_app *);
static int embedded_app_platform_ready(const struct kilotnc_platform *);

static enum embedded_app_result
embedded_app_ptt_off(struct embedded_app *app)
{
	if (app->platform->ptt_set(app->platform->ctx, 0) !=
	    KILOTNC_PLATFORM_OK)
		return EMBEDDED_APP_ERR_PLATFORM;

	app->status.ptt_state = 0;
	return EMBEDDED_APP_OK;
}

static int
embedded_app_platform_ready(const struct kilotnc_platform *platform)
{
	if (platform == NULL)
		return 0;
	if (platform->tick_ms == NULL)
		return 0;
	if (platform->watchdog_kick == NULL)
		return 0;
	if (platform->ptt_set == NULL)
		return 0;

	return 1;
}

enum embedded_app_result
embedded_app_fault(struct embedded_app *app)
{
	enum embedded_app_result result;

	if (app == NULL || !embedded_app_platform_ready(app->platform))
		return EMBEDDED_APP_ERR_ARG;

	app->status.faulted = 1;
	result = embedded_app_ptt_off(app);
	if (result != EMBEDDED_APP_OK)
		return result;

	return EMBEDDED_APP_OK;
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
	result = embedded_app_ptt_off(app);
	if (result != EMBEDDED_APP_OK)
		return result;

	return EMBEDDED_APP_OK;
}

enum embedded_app_result
embedded_app_shutdown(struct embedded_app *app)
{
	enum embedded_app_result result;

	if (app == NULL || !embedded_app_platform_ready(app->platform))
		return EMBEDDED_APP_ERR_ARG;

	app->status.shutdown_requested = 1;
	result = embedded_app_ptt_off(app);
	if (result != EMBEDDED_APP_OK)
		return result;

	return EMBEDDED_APP_OK;
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
	uint32_t tick_ms;

	if (app == NULL || !embedded_app_platform_ready(app->platform))
		return EMBEDDED_APP_ERR_ARG;
	if (app->status.faulted != 0)
		return EMBEDDED_APP_ERR_FAULT;

	if (app->platform->tick_ms(app->platform->ctx, &tick_ms) !=
	    KILOTNC_PLATFORM_OK)
		return EMBEDDED_APP_ERR_PLATFORM;
	if (app->platform->watchdog_kick(app->platform->ctx) !=
	    KILOTNC_PLATFORM_OK) {
		(void)embedded_app_ptt_off(app);
		return EMBEDDED_APP_ERR_PLATFORM;
	}

	app->status.tick_ms = tick_ms;
	app->status.steps++;
	app->status.watchdog_kicks++;
	return EMBEDDED_APP_OK;
}
