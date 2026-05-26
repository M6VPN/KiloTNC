/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/app/embedded_diag.c */

#include <sys/types.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "embedded_app.h"
#include "embedded_diag.h"
#include "embedded_usb_bridge.h"

static uint32_t embedded_diag_size_to_u32(size_t);

static uint32_t
embedded_diag_size_to_u32(size_t value)
{
	if (value > UINT32_MAX)
		return UINT32_MAX;

	return (uint32_t)value;
}

enum embedded_diag_result
embedded_diag_capture(const struct embedded_app *app,
	struct embedded_diag_snapshot *snapshot)
{
	struct embedded_app_status app_status;
	struct embedded_usb_bridge_stats bridge_stats;
	struct kilotnc_usb_cdc_stats usb_stats;
	size_t diag_writes;

	if (app == NULL || snapshot == NULL)
		return EMBEDDED_DIAG_ERR_ARG;
	if (embedded_app_status(app, &app_status) != EMBEDDED_APP_OK)
		return EMBEDDED_DIAG_ERR_ARG;

	(void)memset(snapshot, 0, sizeof(*snapshot));
	snapshot->app_steps = embedded_diag_size_to_u32(app_status.steps);
	snapshot->app_faults = embedded_diag_size_to_u32(app_status.fault_count);
	snapshot->platform_ticks = app_status.tick_ms;
	snapshot->watchdog_kicks =
	    embedded_diag_size_to_u32(app_status.watchdog_kicks);
	snapshot->app_state = (uint8_t)app_status.state;
	snapshot->reset_cause = (uint8_t)app_status.reset_cause;
	snapshot->ptt_state = (uint8_t)app_status.ptt_state;

	if (app->platform != NULL && app->platform->diag_count != NULL &&
	    app->platform->diag_count(app->platform->ctx, &diag_writes) ==
	    KILOTNC_PLATFORM_OK)
		snapshot->diagnostics_writes =
		    embedded_diag_size_to_u32(diag_writes);

	if (app->usb_bridge == NULL)
		return EMBEDDED_DIAG_OK;

	if (embedded_usb_bridge_stats(app->usb_bridge, &bridge_stats) !=
	    EMBEDDED_USB_BRIDGE_OK)
		return EMBEDDED_DIAG_ERR_ARG;
	if (app->usb_bridge->usb != NULL && app->usb_bridge->usb->stats !=
	    NULL && app->usb_bridge->usb->stats(app->usb_bridge->usb->ctx,
	    &usb_stats) == KILOTNC_USB_OK) {
		snapshot->usb_rx_bytes =
		    embedded_diag_size_to_u32(usb_stats.rx_bytes);
		snapshot->usb_tx_bytes =
		    embedded_diag_size_to_u32(usb_stats.tx_bytes);
		snapshot->usb_rx_overflows =
		    embedded_diag_size_to_u32(usb_stats.rx_overflows);
		snapshot->usb_tx_overflows =
		    embedded_diag_size_to_u32(usb_stats.tx_overflows);
		snapshot->usb_connected = usb_stats.connected != 0;
	}

	snapshot->kiss_frames_in =
	    embedded_diag_size_to_u32(bridge_stats.kiss_frames_in);
	snapshot->kiss_frames_out =
	    embedded_diag_size_to_u32(bridge_stats.kiss_frames_out);
	snapshot->kiss_parse_errors =
	    embedded_diag_size_to_u32(bridge_stats.kiss_parse_errors);
	snapshot->kiss_ignored_commands =
	    embedded_diag_size_to_u32(bridge_stats.kiss_ignored_commands);
	snapshot->kiss_overlength_frames =
	    embedded_diag_size_to_u32(bridge_stats.kiss_overlength);

	return EMBEDDED_DIAG_OK;
}

enum embedded_diag_result
embedded_diag_format(const struct embedded_diag_snapshot *snapshot, char *buf,
	size_t buflen, size_t *out_len)
{
	int ret;

	if (snapshot == NULL || buf == NULL || out_len == NULL)
		return EMBEDDED_DIAG_ERR_ARG;
	*out_len = 0;
	if (buflen == 0)
		return EMBEDDED_DIAG_ERR_SMALL;

	ret = snprintf(buf, buflen, "app_steps=%u app_faults=%u "
	    "platform_ticks=%u watchdog_kicks=%u diagnostics_writes=%u "
	    "usb_rx_bytes=%u usb_tx_bytes=%u usb_rx_overflows=%u "
	    "usb_tx_overflows=%u kiss_frames_in=%u kiss_frames_out=%u "
	    "kiss_parse_errors=%u kiss_ignored=%u kiss_overlength=%u "
	    "app_state=%u reset_cause=%u ptt=%u usb_connected=%u",
	    snapshot->app_steps, snapshot->app_faults,
	    snapshot->platform_ticks, snapshot->watchdog_kicks,
	    snapshot->diagnostics_writes, snapshot->usb_rx_bytes,
	    snapshot->usb_tx_bytes, snapshot->usb_rx_overflows,
	    snapshot->usb_tx_overflows, snapshot->kiss_frames_in,
	    snapshot->kiss_frames_out, snapshot->kiss_parse_errors,
	    snapshot->kiss_ignored_commands,
	    snapshot->kiss_overlength_frames, snapshot->app_state,
	    snapshot->reset_cause, snapshot->ptt_state,
	    snapshot->usb_connected);
	if (ret < 0)
		return EMBEDDED_DIAG_ERR_SMALL;
	if ((size_t)ret >= buflen)
		return EMBEDDED_DIAG_ERR_SMALL;

	*out_len = (size_t)ret;
	return EMBEDDED_DIAG_OK;
}
