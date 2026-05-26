/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/src/tnc_diag.c */

#include <sys/types.h>

#include <stdio.h>
#include <string.h>

#include "tnc_diag.h"

static int tnc_diag_fault_valid(enum tnc_diag_fault);

enum tnc_diag_result
tnc_diag_capture_tnc1200(struct tnc_diag *diag, const struct tnc1200 *tnc)
{
	struct tnc1200_stats stats;
	struct tnc1200_status status;
	enum tnc_diag_fault last_fault;

	if (diag == NULL || tnc == NULL)
		return TNC_DIAG_ERR_ARG;
	if (tnc1200_stats(tnc, &stats) != TNC1200_OK)
		return TNC_DIAG_ERR_ARG;
	if (tnc1200_status(tnc, &status) != TNC1200_OK)
		return TNC_DIAG_ERR_ARG;

	last_fault = diag->snapshot.last_fault;
	(void)memset(&diag->snapshot, 0, sizeof(diag->snapshot));
	diag->snapshot.kiss_frames_in = stats.kiss_frames_in;
	diag->snapshot.kiss_frames_out = stats.kiss_frames_out;
	diag->snapshot.kiss_parse_errors = stats.kiss_parse_errors;
	diag->snapshot.kiss_ignored_commands = stats.kiss_ignored_commands;
	diag->snapshot.tx_frames_started = stats.tx_frames_started;
	diag->snapshot.tx_frames_done = stats.tx_frames_done;
	diag->snapshot.tx_frames_rejected = stats.tx_frames_rejected;
	diag->snapshot.tx_samples_out = stats.pcm_samples_out;
	diag->snapshot.rx_frames_ok = stats.rx_frames_ok;
	diag->snapshot.rx_frames_bad_fcs = stats.rx_frames_bad_fcs;
	diag->snapshot.rx_frames_malformed = stats.rx_frames_malformed;
	diag->snapshot.rx_frames_dropped = stats.rx_frames_dropped;
	diag->snapshot.rx_samples_in = stats.pcm_samples_in;
	diag->snapshot.channel_tx_requests = stats.channel_tx_requests;
	diag->snapshot.channel_tx_grants = stats.channel_tx_grants;
	diag->snapshot.channel_tx_denied_busy =
	    stats.channel_tx_denied_busy;
	diag->snapshot.channel_tx_persistence_deferrals =
	    stats.channel_tx_persistence_deferrals;
	diag->snapshot.channel_tx_timeouts = stats.channel_tx_timeouts;
	diag->snapshot.channel_tx_aborts = stats.channel_tx_aborts;
	diag->snapshot.ptt_on_events = stats.ptt_on_events;
	diag->snapshot.ptt_off_events = stats.ptt_off_events;
	diag->snapshot.mode_set_requests = stats.mode_set_requests;
	diag->snapshot.mode_set_unsupported = stats.mode_set_unsupported;
	diag->snapshot.mode_set_invalid = stats.mode_set_invalid;
	diag->snapshot.rx_dcd_score = status.rx_dcd_score;
	diag->snapshot.rx_confidence_avg = status.rx_confidence_avg;
	diag->snapshot.p = status.p;
	diag->snapshot.slottime_10ms = status.slottime_10ms;
	diag->snapshot.fullduplex = status.fullduplex;
	diag->snapshot.ptt_state = status.ptt_state;
	diag->snapshot.tx_active = status.tx_active;
	diag->snapshot.audio_ready = status.audio_ready;
	diag->snapshot.dcd_busy = status.dcd_busy;
	diag->snapshot.last_nino_sethw = status.last_nino_sethw;
	diag->snapshot.last_mode_temporary = status.last_mode_temporary;
	diag->snapshot.current_mode = status.current_mode;
	diag->snapshot.last_requested_mode = status.last_requested_mode;
	diag->snapshot.last_fault = last_fault;

	return TNC_DIAG_OK;
}

enum tnc_diag_result
tnc_diag_faults(const struct tnc_diag *diag, enum tnc_diag_fault *faults,
	size_t fault_cap, size_t *fault_count)
{
	size_t start;
	size_t i;

	if (diag == NULL || fault_count == NULL)
		return TNC_DIAG_ERR_ARG;
	*fault_count = diag->fault_count;
	if (diag->fault_count == 0U)
		return TNC_DIAG_OK;
	if (faults == NULL)
		return TNC_DIAG_ERR_ARG;
	if (fault_cap < diag->fault_count)
		return TNC_DIAG_ERR_SMALL;

	start = diag->fault_count == TNC_DIAG_FAULT_RING ?
	    diag->fault_head : 0U;
	for (i = 0U; i < diag->fault_count; i++) {
		faults[i] = diag->fault_ring[
		    (start + i) % TNC_DIAG_FAULT_RING];
	}

	return TNC_DIAG_OK;
}

enum tnc_diag_result
tnc_diag_format_snapshot(const struct tnc_diag_snapshot *snapshot,
	char *buf, size_t buf_cap, size_t *out_len)
{
	int written;

	if (snapshot == NULL || buf == NULL || out_len == NULL)
		return TNC_DIAG_ERR_ARG;
	*out_len = 0U;
	if (buf_cap == 0U)
		return TNC_DIAG_ERR_SMALL;

	written = snprintf(buf, buf_cap,
	    "kiss_in=%zu kiss_out=%zu kiss_parse_errors=%zu "
	    "kiss_ignored=%zu tx_started=%zu tx_done=%zu "
	    "tx_rejected=%zu tx_samples=%zu rx_ok=%zu rx_bad_fcs=%zu "
	    "rx_malformed=%zu rx_dropped=%zu rx_samples=%zu "
	    "chan_req=%zu chan_grant=%zu chan_busy=%zu chan_defers=%zu "
	    "chan_timeouts=%zu chan_aborts=%zu ptt_on=%zu ptt_off=%zu "
	    "mode_req=%zu mode_unsup=%zu mode_invalid=%zu rx_dcd=%u "
	    "rx_conf=%u p=%u slot=%u fullduplex=%u ptt=%u tx_active=%u "
	    "audio_ready=%u dcd=%u current_mode=%u last_mode=%u "
	    "last_nino=%u mode_temp=%u last_fault=%u",
	    snapshot->kiss_frames_in, snapshot->kiss_frames_out,
	    snapshot->kiss_parse_errors, snapshot->kiss_ignored_commands,
	    snapshot->tx_frames_started, snapshot->tx_frames_done,
	    snapshot->tx_frames_rejected, snapshot->tx_samples_out,
	    snapshot->rx_frames_ok, snapshot->rx_frames_bad_fcs,
	    snapshot->rx_frames_malformed, snapshot->rx_frames_dropped,
	    snapshot->rx_samples_in, snapshot->channel_tx_requests,
	    snapshot->channel_tx_grants, snapshot->channel_tx_denied_busy,
	    snapshot->channel_tx_persistence_deferrals,
	    snapshot->channel_tx_timeouts, snapshot->channel_tx_aborts,
	    snapshot->ptt_on_events, snapshot->ptt_off_events,
	    snapshot->mode_set_requests, snapshot->mode_set_unsupported,
	    snapshot->mode_set_invalid,
	    (unsigned int)snapshot->rx_dcd_score,
	    (unsigned int)snapshot->rx_confidence_avg,
	    (unsigned int)snapshot->p,
	    (unsigned int)snapshot->slottime_10ms,
	    (unsigned int)snapshot->fullduplex,
	    (unsigned int)snapshot->ptt_state,
	    (unsigned int)snapshot->tx_active,
	    (unsigned int)snapshot->audio_ready,
	    (unsigned int)snapshot->dcd_busy,
	    (unsigned int)snapshot->current_mode,
	    (unsigned int)snapshot->last_requested_mode,
	    (unsigned int)snapshot->last_nino_sethw,
	    (unsigned int)snapshot->last_mode_temporary,
	    (unsigned int)snapshot->last_fault);
	if (written < 0)
		return TNC_DIAG_ERR_SMALL;
	if ((size_t)written >= buf_cap)
		return TNC_DIAG_ERR_SMALL;
	*out_len = (size_t)written;

	return TNC_DIAG_OK;
}

enum tnc_diag_result
tnc_diag_init(struct tnc_diag *diag)
{
	if (diag == NULL)
		return TNC_DIAG_ERR_ARG;

	(void)memset(diag, 0, sizeof(*diag));
	return TNC_DIAG_OK;
}

enum tnc_diag_result
tnc_diag_record_fault(struct tnc_diag *diag, enum tnc_diag_fault fault)
{
	if (diag == NULL)
		return TNC_DIAG_ERR_ARG;
	if (!tnc_diag_fault_valid(fault))
		return TNC_DIAG_ERR_RANGE;

	diag->fault_ring[diag->fault_head] = fault;
	diag->fault_head = (diag->fault_head + 1U) % TNC_DIAG_FAULT_RING;
	if (diag->fault_count < TNC_DIAG_FAULT_RING)
		diag->fault_count++;
	diag->snapshot.last_fault = fault;

	return TNC_DIAG_OK;
}

enum tnc_diag_result
tnc_diag_snapshot(const struct tnc_diag *diag,
	struct tnc_diag_snapshot *snapshot)
{
	if (diag == NULL || snapshot == NULL)
		return TNC_DIAG_ERR_ARG;

	*snapshot = diag->snapshot;
	return TNC_DIAG_OK;
}

static int
tnc_diag_fault_valid(enum tnc_diag_fault fault)
{
	return fault > TNC_DIAG_FAULT_NONE &&
	    fault <= TNC_DIAG_FAULT_AUDIO_OVERRUN;
}
