/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/app/embedded_diag.c */

#include <sys/types.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "embedded_audio.h"
#include "embedded_app.h"
#include "embedded_diag.h"
#include "embedded_modem.h"
#include "embedded_tnc.h"
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
	struct embedded_audio_stats audio_bridge_stats;
	struct embedded_modem_status modem_status;
	struct embedded_tnc_status tnc_status;
	struct embedded_usb_bridge_stats bridge_stats;
	struct kilotnc_audio_stats audio_stats;
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
		goto audio;

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

audio:
	if (app->audio_bridge == NULL)
		goto tnc;

	if (embedded_audio_stats(app->audio_bridge, &audio_bridge_stats) !=
	    EMBEDDED_AUDIO_OK)
		return EMBEDDED_DIAG_ERR_ARG;
	if (app->audio_bridge->audio != NULL &&
	    app->audio_bridge->audio->stats != NULL &&
	    app->audio_bridge->audio->stats(app->audio_bridge->audio->ctx,
	    &audio_stats) == KILOTNC_AUDIO_OK) {
		snapshot->audio_rx_samples =
		    embedded_diag_size_to_u32(audio_stats.rx_samples_read);
		snapshot->audio_tx_samples =
		    embedded_diag_size_to_u32(audio_stats.tx_samples_written);
		snapshot->audio_rx_overflows =
		    embedded_diag_size_to_u32(audio_stats.rx_overflows);
		snapshot->audio_tx_overflows =
		    embedded_diag_size_to_u32(audio_stats.tx_overflows);
		snapshot->audio_rx_underflows =
		    embedded_diag_size_to_u32(audio_stats.rx_underflows);
		snapshot->audio_tx_underflows =
		    embedded_diag_size_to_u32(audio_stats.tx_underflows);
	}
	snapshot->audio_loopback_blocks =
	    embedded_diag_size_to_u32(audio_bridge_stats.loopback_blocks);

tnc:
	if (app->tnc == NULL)
		goto modem;

	if (embedded_tnc_status(app->tnc, &tnc_status) != EMBEDDED_TNC_OK)
		return EMBEDDED_DIAG_ERR_ARG;

	snapshot->tnc_kiss_frames_in =
	    embedded_diag_size_to_u32(tnc_status.kiss_frames_in);
	snapshot->tnc_kiss_frames_out =
	    embedded_diag_size_to_u32(tnc_status.kiss_frames_out);
	snapshot->tnc_kiss_parse_errors =
	    embedded_diag_size_to_u32(tnc_status.kiss_parse_errors);
	snapshot->tnc_kiss_ignored_commands =
	    embedded_diag_size_to_u32(tnc_status.kiss_ignored_commands);
	snapshot->tnc_mode_set_requests =
	    embedded_diag_size_to_u32(tnc_status.mode_set_requests);
	snapshot->tnc_mode_unsupported =
	    embedded_diag_size_to_u32(tnc_status.unsupported_mode_requests);
	snapshot->tnc_mode_invalid =
	    embedded_diag_size_to_u32(tnc_status.invalid_mode_requests);
	snapshot->tnc_modem_tx_requests =
	    embedded_diag_size_to_u32(tnc_status.modem_tx_requests);
	snapshot->tnc_modem_tx_accepted =
	    embedded_diag_size_to_u32(tnc_status.modem_tx_accepted);
	snapshot->tnc_modem_tx_rejected =
	    embedded_diag_size_to_u32(tnc_status.modem_tx_rejected);
	snapshot->tnc_current_mode = (uint8_t)tnc_status.current_mode;
	snapshot->tnc_txdelay = tnc_status.txdelay;
	snapshot->tnc_p = tnc_status.p;
	snapshot->tnc_slottime = tnc_status.slottime;
	snapshot->tnc_txtail = tnc_status.txtail;
	snapshot->tnc_fullduplex = tnc_status.fullduplex;
	snapshot->tnc_loopback_enabled = tnc_status.loopback_enabled;
	snapshot->tnc_modem_tx_enabled = tnc_status.modem_tx_enabled;

modem:
	if (app->modem == NULL)
		return EMBEDDED_DIAG_OK;

	if (embedded_modem_status(app->modem, &modem_status) !=
	    EMBEDDED_MODEM_OK)
		return EMBEDDED_DIAG_ERR_ARG;
	snapshot->modem_tx_frames_started =
	    embedded_diag_size_to_u32(modem_status.tx_frames_started);
	snapshot->modem_tx_frames_rejected =
	    embedded_diag_size_to_u32(modem_status.tx_frames_rejected);
	snapshot->modem_tx_frames_done =
	    embedded_diag_size_to_u32(modem_status.tx_frames_done);
	snapshot->modem_tx_samples_generated =
	    embedded_diag_size_to_u32(modem_status.tx_samples_generated);
	snapshot->modem_tx_audio_errors =
	    embedded_diag_size_to_u32(modem_status.tx_audio_errors);
	snapshot->modem_aborts =
	    embedded_diag_size_to_u32(modem_status.aborts);
	snapshot->modem_tx_active = modem_status.tx_active;
	snapshot->modem_current_mode = (uint8_t)modem_status.current_mode;

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
	    "audio_rx_samples=%u audio_tx_samples=%u "
	    "audio_rx_overflows=%u audio_tx_overflows=%u "
	    "audio_rx_underflows=%u audio_tx_underflows=%u "
	    "audio_loopback_blocks=%u "
	    "tnc_mode=%u tnc_kiss_frames_in=%u tnc_kiss_frames_out=%u "
	    "tnc_kiss_parse_errors=%u tnc_kiss_ignored=%u "
	    "tnc_mode_set_requests=%u tnc_mode_unsupported=%u "
	    "tnc_mode_invalid=%u tnc_txdelay=%u tnc_p=%u "
	    "tnc_slottime=%u tnc_txtail=%u tnc_fullduplex=%u "
	    "tnc_loopback=%u tnc_modem_tx_enabled=%u "
	    "tnc_modem_tx_requests=%u tnc_modem_tx_accepted=%u "
	    "tnc_modem_tx_rejected=%u modem_tx_active=%u "
	    "modem_mode=%u modem_tx_frames_started=%u "
	    "modem_tx_frames_rejected=%u modem_tx_frames_done=%u "
	    "modem_tx_samples_generated=%u modem_tx_audio_errors=%u "
	    "modem_aborts=%u "
	    "app_state=%u reset_cause=%u ptt=%u usb_connected=%u",
	    snapshot->app_steps, snapshot->app_faults,
	    snapshot->platform_ticks, snapshot->watchdog_kicks,
	    snapshot->diagnostics_writes, snapshot->usb_rx_bytes,
	    snapshot->usb_tx_bytes, snapshot->usb_rx_overflows,
	    snapshot->usb_tx_overflows, snapshot->kiss_frames_in,
	    snapshot->kiss_frames_out, snapshot->kiss_parse_errors,
	    snapshot->kiss_ignored_commands,
	    snapshot->kiss_overlength_frames, snapshot->audio_rx_samples,
	    snapshot->audio_tx_samples, snapshot->audio_rx_overflows,
	    snapshot->audio_tx_overflows, snapshot->audio_rx_underflows,
	    snapshot->audio_tx_underflows, snapshot->audio_loopback_blocks,
	    snapshot->tnc_current_mode, snapshot->tnc_kiss_frames_in,
	    snapshot->tnc_kiss_frames_out, snapshot->tnc_kiss_parse_errors,
	    snapshot->tnc_kiss_ignored_commands,
	    snapshot->tnc_mode_set_requests, snapshot->tnc_mode_unsupported,
	    snapshot->tnc_mode_invalid, snapshot->tnc_txdelay,
	    snapshot->tnc_p, snapshot->tnc_slottime, snapshot->tnc_txtail,
	    snapshot->tnc_fullduplex, snapshot->tnc_loopback_enabled,
	    snapshot->tnc_modem_tx_enabled, snapshot->tnc_modem_tx_requests,
	    snapshot->tnc_modem_tx_accepted,
	    snapshot->tnc_modem_tx_rejected, snapshot->modem_tx_active,
	    snapshot->modem_current_mode, snapshot->modem_tx_frames_started,
	    snapshot->modem_tx_frames_rejected,
	    snapshot->modem_tx_frames_done,
	    snapshot->modem_tx_samples_generated,
	    snapshot->modem_tx_audio_errors, snapshot->modem_aborts,
	    snapshot->app_state, snapshot->reset_cause, snapshot->ptt_state,
	    snapshot->usb_connected);
	if (ret < 0)
		return EMBEDDED_DIAG_ERR_SMALL;
	if ((size_t)ret >= buflen)
		return EMBEDDED_DIAG_ERR_SMALL;

	*out_len = (size_t)ret;
	return EMBEDDED_DIAG_OK;
}
