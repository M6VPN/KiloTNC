/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/src/tnc1200.c */

#include <sys/types.h>

#include <stdint.h>
#include <string.h>

#include "ax25.h"
#include "tnc1200.h"

#define TNC1200_KISS_BATCH	4U

static void tnc1200_apply_kiss_settings(struct tnc1200 *);
static void tnc1200_clear_pending(struct tnc1200 *);
static enum tnc1200_result tnc1200_map_tx(enum afsk1200_tx_result);
static enum tnc1200_result tnc1200_maybe_start_audio(struct tnc1200 *);
static enum tnc1200_result tnc1200_start_tx(struct tnc1200 *,
	const struct kiss_frame *);
static void tnc1200_sync_control_config(struct tnc1200 *);
static void tnc1200_sync_control_stats(const struct tnc1200 *,
	struct tnc1200_stats *);
static enum tnc1200_result tnc1200_validate_config(
	const struct tnc1200_config *, struct afsk1200_tx_config *,
	struct tnc_control_config *);

enum tnc1200_result
tnc1200_abort_tx(struct tnc1200 *tnc)
{
	if (tnc == NULL)
		return TNC1200_ERR_ARG;
	if (afsk1200_tx_abort(&tnc->tx) != AFSK1200_TX_OK)
		return TNC1200_ERR_ARG;
	if (tnc_control_abort_tx(&tnc->control) != TNC_CONTROL_OK)
		return TNC1200_ERR_ARG;
	tnc1200_clear_pending(tnc);

	return TNC1200_OK;
}

enum tnc1200_result
tnc1200_can_emit_audio(const struct tnc1200 *tnc, int *can_emit)
{
	if (tnc == NULL || can_emit == NULL)
		return TNC1200_ERR_ARG;
	if (tnc_control_can_emit_audio(&tnc->control, can_emit) !=
	    TNC_CONTROL_OK)
		return TNC1200_ERR_ARG;

	return TNC1200_OK;
}

enum tnc1200_result
tnc1200_host_input(struct tnc1200 *tnc, const uint8_t *data, size_t len)
{
	struct kiss_frame frames[TNC1200_KISS_BATCH];
	size_t frame_count;
	size_t i;
	size_t parse_errors;
	size_t ignored_commands;
	enum kiss_result kres;
	enum tnc1200_result res;
	enum tnc1200_result final_res;

	if (tnc == NULL || (data == NULL && len != 0U))
		return TNC1200_ERR_ARG;

	parse_errors = tnc->kiss.counters.parse_errors;
	ignored_commands = tnc->kiss.counters.ignored_commands;
	kres = kiss_parse_bytes(&tnc->kiss, data, len, frames,
	    sizeof(frames) / sizeof(frames[0]), &frame_count);
	tnc->stats.kiss_parse_errors +=
	    tnc->kiss.counters.parse_errors - parse_errors;
	tnc->stats.kiss_ignored_commands +=
	    tnc->kiss.counters.ignored_commands - ignored_commands;
	tnc1200_apply_kiss_settings(tnc);
	if (kres == KISS_ERR_ARG)
		return TNC1200_ERR_ARG;
	if (kres == KISS_ERR_SMALL)
		return TNC1200_ERR_SMALL;

	final_res = TNC1200_OK;
	for (i = 0U; i < frame_count; i++) {
		tnc->stats.kiss_frames_in++;
		res = tnc1200_start_tx(tnc, &frames[i]);
		if (res == TNC1200_ERR_BUSY)
			final_res = res;
		else if (res != TNC1200_OK)
			return res;
	}

	return final_res;
}

enum tnc1200_result
tnc1200_init(struct tnc1200 *tnc, const struct tnc1200_config *config)
{
	enum tnc1200_result tres;

	if (tnc == NULL)
		return TNC1200_ERR_ARG;

	(void)memset(tnc, 0, sizeof(*tnc));
	kiss_parser_init(&tnc->kiss);
	tres = tnc1200_validate_config(config, &tnc->tx_config,
	    &tnc->control_config);
	if (tres != TNC1200_OK)
		return tres;
	tnc->kiss.txdelay = (uint8_t)tnc->tx_config.txdelay_flags;
	tnc->kiss.txtail = (uint8_t)tnc->tx_config.txtail_flags;
	if (afsk1200_tx_init(&tnc->tx, &tnc->tx_config) != AFSK1200_TX_OK)
		return TNC1200_ERR_ARG;
	if (tnc_control_init(&tnc->control, &tnc->control_config) !=
	    TNC_CONTROL_OK)
		return TNC1200_ERR_ARG;
	if (afsk1200_stream_init(&tnc->rx) != AFSK1200_STREAM_OK)
		return TNC1200_ERR_ARG;
	tnc->kiss.p = tnc->control_config.p;
	tnc->kiss.slottime = tnc->control_config.slottime_10ms;
	tnc->kiss.fullduplex = tnc->control_config.fullduplex != 0U;
	tnc->p = tnc->kiss.p;
	tnc->slottime = tnc->kiss.slottime;
	tnc->fullduplex = tnc->kiss.fullduplex ? 1U : 0U;

	return TNC1200_OK;
}

enum tnc1200_result
tnc1200_ptt_state(const struct tnc1200 *tnc, enum tnc_control_ptt *ptt)
{
	if (tnc == NULL || ptt == NULL)
		return TNC1200_ERR_ARG;
	if (tnc_control_ptt_state(&tnc->control, ptt) != TNC_CONTROL_OK)
		return TNC1200_ERR_ARG;

	return TNC1200_OK;
}

enum tnc1200_result
tnc1200_rx_process(struct tnc1200 *tnc, const int16_t *pcm,
	size_t sample_count, uint8_t *out, size_t out_cap, size_t *out_len)
{
	struct afsk1200_stream_frame frames[2];
	struct afsk1200_stream_stats rx_stats;
	size_t frame_count;
	size_t encoded;
	size_t i;
	enum afsk1200_stream_result sres;
	enum kiss_result kres;
	enum tnc1200_result final_res;

	if (tnc == NULL || out_len == NULL)
		return TNC1200_ERR_ARG;
	*out_len = 0U;
	if ((pcm == NULL && sample_count != 0U) || out == NULL)
		return TNC1200_ERR_ARG;

	sres = afsk1200_stream_process(&tnc->rx, pcm, sample_count, frames,
	    sizeof(frames) / sizeof(frames[0]), &frame_count);
	tnc->stats.pcm_samples_in += sample_count;
	if (afsk1200_stream_stats(&tnc->rx, &rx_stats) == AFSK1200_STREAM_OK) {
		tnc->stats.rx_frames_bad_fcs = rx_stats.frames_bad_fcs;
		tnc->stats.rx_frames_malformed = rx_stats.frames_malformed;
	}
	if (sres != AFSK1200_STREAM_OK &&
	    sres != AFSK1200_STREAM_ERR_FRAME_DROPPED)
		return TNC1200_ERR_ARG;

	final_res = sres == AFSK1200_STREAM_ERR_FRAME_DROPPED ?
	    TNC1200_ERR_FRAME_DROPPED : TNC1200_OK;
	for (i = 0U; i < frame_count; i++) {
		kres = kiss_encode_frame(0U, KISS_CMD_DATA, frames[i].data,
		    frames[i].len, &out[*out_len], out_cap - *out_len,
		    &encoded);
		if (kres == KISS_ERR_SMALL) {
			tnc->stats.rx_frames_dropped++;
			final_res = TNC1200_ERR_SMALL;
			continue;
		}
		if (kres != KISS_OK)
			return TNC1200_ERR_ARG;
		*out_len += encoded;
		tnc->stats.kiss_frames_out++;
		tnc->stats.rx_frames_ok++;
	}

	return final_res;
}

enum tnc1200_result
tnc1200_set_dcd(struct tnc1200 *tnc, int busy)
{
	if (tnc == NULL)
		return TNC1200_ERR_ARG;
	if (tnc_control_set_dcd(&tnc->control, busy) != TNC_CONTROL_OK)
		return TNC1200_ERR_ARG;

	return TNC1200_OK;
}

enum tnc1200_result
tnc1200_stats(const struct tnc1200 *tnc, struct tnc1200_stats *stats)
{
	if (tnc == NULL || stats == NULL)
		return TNC1200_ERR_ARG;

	tnc1200_sync_control_stats(tnc, stats);
	return TNC1200_OK;
}

enum tnc1200_result
tnc1200_tx_process(struct tnc1200 *tnc, int16_t *out, size_t out_cap,
	size_t *out_samples)
{
	struct afsk1200_tx_stats tx_stats;
	size_t before;
	enum tnc_control_result cres;
	enum afsk1200_tx_result txres;
	enum tnc1200_result tres;

	if (tnc == NULL || out_samples == NULL)
		return TNC1200_ERR_ARG;
	*out_samples = 0U;
	if (out == NULL)
		return TNC1200_ERR_ARG;

	cres = tnc_control_tick_10ms(&tnc->control);
	if (cres == TNC_CONTROL_ERR_TIMEOUT) {
		(void)afsk1200_tx_abort(&tnc->tx);
		tnc1200_clear_pending(tnc);
		return TNC1200_ERR_TIMEOUT;
	}
	if (cres != TNC_CONTROL_OK && cres != TNC_CONTROL_ERR_DENIED)
		return TNC1200_ERR_ARG;

	tres = tnc1200_maybe_start_audio(tnc);
	if (tres != TNC1200_OK)
		return tres;
	if (!tnc->tx_started)
		return TNC1200_ERR_NO_DATA;

	(void)afsk1200_tx_stats(&tnc->tx, &tx_stats);
	before = tx_stats.frames_done;
	txres = afsk1200_tx_process(&tnc->tx, out, out_cap, out_samples);
	tnc->stats.pcm_samples_out += *out_samples;
	if (afsk1200_tx_stats(&tnc->tx, &tx_stats) == AFSK1200_TX_OK &&
	    tx_stats.frames_done > before) {
		tnc->stats.tx_frames_done += tx_stats.frames_done - before;
		(void)tnc_control_complete_tx(&tnc->control);
		tnc1200_clear_pending(tnc);
	}
	if (txres == AFSK1200_TX_DONE && *out_samples == 0U)
		return TNC1200_ERR_NO_DATA;

	return tnc1200_map_tx(txres);
}

static void
tnc1200_apply_kiss_settings(struct tnc1200 *tnc)
{
	tnc->tx_config.txdelay_flags = tnc->kiss.txdelay;
	tnc->tx_config.txtail_flags = tnc->kiss.txtail;
	tnc->p = tnc->kiss.p;
	tnc->slottime = tnc->kiss.slottime;
	tnc->fullduplex = tnc->kiss.fullduplex ? 1U : 0U;
	tnc1200_sync_control_config(tnc);
	if (tnc->kiss.sethw_len != 0U) {
		(void)memcpy(tnc->sethw, tnc->kiss.sethw, tnc->kiss.sethw_len);
		tnc->sethw_len = tnc->kiss.sethw_len;
	}

	if (!tnc->pending_frame_valid && !tnc->tx_started &&
	    tnc->control.state == TNC_CONTROL_IDLE)
		(void)afsk1200_tx_init(&tnc->tx, &tnc->tx_config);
}

static void
tnc1200_clear_pending(struct tnc1200 *tnc)
{
	tnc->pending_frame_valid = false;
	tnc->pending_frame_len = 0U;
	tnc->tx_started = false;
}

static enum tnc1200_result
tnc1200_map_tx(enum afsk1200_tx_result res)
{
	if (res == AFSK1200_TX_OK)
		return TNC1200_OK;
	if (res == AFSK1200_TX_ERR_ARG)
		return TNC1200_ERR_ARG;
	if (res == AFSK1200_TX_ERR_SMALL)
		return TNC1200_ERR_SMALL;
	if (res == AFSK1200_TX_ERR_BUSY)
		return TNC1200_ERR_BUSY;
	if (res == AFSK1200_TX_DONE)
		return TNC1200_OK;
	return TNC1200_ERR_ARG;
}

static enum tnc1200_result
tnc1200_maybe_start_audio(struct tnc1200 *tnc)
{
	int can_emit;

	if (!tnc->pending_frame_valid || tnc->tx_started)
		return TNC1200_OK;
	if (tnc_control_can_emit_audio(&tnc->control, &can_emit) !=
	    TNC_CONTROL_OK)
		return TNC1200_ERR_ARG;
	if (!can_emit)
		return TNC1200_OK;

	if (afsk1200_tx_init(&tnc->tx, &tnc->tx_config) != AFSK1200_TX_OK)
		return TNC1200_ERR_ARG;
	if (afsk1200_tx_start_frame(&tnc->tx, tnc->pending_frame,
	    tnc->pending_frame_len) != AFSK1200_TX_OK) {
		tnc->stats.tx_frames_rejected++;
		tnc1200_clear_pending(tnc);
		(void)tnc_control_abort_tx(&tnc->control);
		return TNC1200_ERR_ARG;
	}
	tnc->tx_started = true;
	tnc->stats.tx_frames_started++;

	return TNC1200_OK;
}

static enum tnc1200_result
tnc1200_start_tx(struct tnc1200 *tnc, const struct kiss_frame *frame)
{
	enum tnc_control_result cres;
	struct ax25_frame decoded;

	if (tnc->pending_frame_valid || tnc->tx_started ||
	    tnc->control.state != TNC_CONTROL_IDLE) {
		tnc->stats.tx_frames_rejected++;
		return TNC1200_ERR_BUSY;
	}
	if (frame->len > sizeof(tnc->pending_frame) ||
	    ax25_decode_ui_fcs(frame->data, frame->len, &decoded) != AX25_OK) {
		tnc->stats.tx_frames_rejected++;
		return TNC1200_OK;
	}

	(void)memcpy(tnc->pending_frame, frame->data, frame->len);
	tnc->pending_frame_len = frame->len;
	tnc->pending_frame_valid = true;
	cres = tnc_control_request_tx(&tnc->control);
	if (cres == TNC_CONTROL_OK || cres == TNC_CONTROL_ERR_DENIED)
		return TNC1200_OK;
	if (cres == TNC_CONTROL_ERR_BUSY) {
		tnc->stats.tx_frames_rejected++;
		tnc1200_clear_pending(tnc);
		return TNC1200_ERR_BUSY;
	}
	tnc1200_clear_pending(tnc);
	return TNC1200_ERR_ARG;
}

static void
tnc1200_sync_control_config(struct tnc1200 *tnc)
{
	tnc->control_config.p = tnc->kiss.p;
	tnc->control_config.slottime_10ms = tnc->kiss.slottime;
	tnc->control_config.fullduplex = tnc->kiss.fullduplex ? 1U : 0U;
	tnc->control_config.txdelay_ms = (uint32_t)tnc->kiss.txdelay * 10U;
	tnc->control_config.txtail_ms = (uint32_t)tnc->kiss.txtail * 10U;
	tnc->control.config = tnc->control_config;
	tnc->p = tnc->kiss.p;
	tnc->slottime = tnc->kiss.slottime;
	tnc->fullduplex = tnc->kiss.fullduplex ? 1U : 0U;
}

static void
tnc1200_sync_control_stats(const struct tnc1200 *tnc,
	struct tnc1200_stats *stats)
{
	struct tnc_control_stats control_stats;

	*stats = tnc->stats;
	if (tnc_control_stats(&tnc->control, &control_stats) != TNC_CONTROL_OK)
		return;
	stats->channel_tx_requests = control_stats.tx_requests;
	stats->channel_tx_grants = control_stats.tx_grants;
	stats->channel_tx_denied_busy = control_stats.tx_denied_busy;
	stats->channel_tx_persistence_deferrals =
	    control_stats.tx_persistence_deferrals;
	stats->channel_tx_timeouts = control_stats.tx_timeouts;
	stats->channel_tx_aborts = control_stats.tx_aborts;
	stats->ptt_on_events = control_stats.ptt_on_events;
	stats->ptt_off_events = control_stats.ptt_off_events;
}

static enum tnc1200_result
tnc1200_validate_config(const struct tnc1200_config *config,
	struct afsk1200_tx_config *tx_config,
	struct tnc_control_config *control_config)
{
	if (config == NULL) {
		tx_config->txdelay_flags = AFSK1200_TX_DEFAULT_TXDELAY_FLAGS;
		tx_config->txtail_flags = AFSK1200_TX_DEFAULT_TXTAIL_FLAGS;
		tx_config->amplitude = AFSK1200_PCM_AMPLITUDE;
		control_config->p = 255U;
		control_config->slottime_10ms = 10U;
		control_config->fullduplex = 0U;
		control_config->txdelay_ms =
		    (uint32_t)AFSK1200_TX_DEFAULT_TXDELAY_FLAGS * 80U;
		control_config->txtail_ms =
		    (uint32_t)AFSK1200_TX_DEFAULT_TXTAIL_FLAGS * 80U;
		control_config->max_tx_ms = 30000U;
		control_config->rng_seed = 1U;
		return TNC1200_OK;
	}
	if (config->amplitude <= 0)
		return TNC1200_ERR_ARG;
	if (config->txdelay_flags > 255U || config->txtail_flags > 255U)
		return TNC1200_ERR_ARG;

	tx_config->txdelay_flags = config->txdelay_flags;
	tx_config->txtail_flags = config->txtail_flags;
	tx_config->amplitude = config->amplitude;
	control_config->p = config->p;
	control_config->slottime_10ms = config->slottime_10ms;
	control_config->fullduplex = config->fullduplex;
	control_config->txdelay_ms = (uint32_t)config->txdelay_flags * 80U;
	control_config->txtail_ms = (uint32_t)config->txtail_flags * 80U;
	control_config->max_tx_ms = config->max_tx_ms == 0U ? 30000U :
	    config->max_tx_ms;
	control_config->rng_seed = config->rng_seed == 0U ? 1U :
	    config->rng_seed;
	return TNC1200_OK;
}
