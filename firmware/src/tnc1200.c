/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/src/tnc1200.c */

#include <sys/types.h>

#include <stdint.h>
#include <string.h>

#include "tnc1200.h"

#define TNC1200_KISS_BATCH	4U

static void tnc1200_apply_kiss_settings(struct tnc1200 *);
static enum tnc1200_result tnc1200_map_tx(enum afsk1200_tx_result);
static enum tnc1200_result tnc1200_start_tx(struct tnc1200 *,
	const struct kiss_frame *);
static enum tnc1200_result tnc1200_validate_config(
	const struct tnc1200_config *, struct afsk1200_tx_config *);

enum tnc1200_result
tnc1200_abort_tx(struct tnc1200 *tnc)
{
	if (tnc == NULL)
		return TNC1200_ERR_ARG;
	if (afsk1200_tx_abort(&tnc->tx) != AFSK1200_TX_OK)
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
	tres = tnc1200_validate_config(config, &tnc->tx_config);
	if (tres != TNC1200_OK)
		return tres;
	tnc->kiss.txdelay = (uint8_t)tnc->tx_config.txdelay_flags;
	tnc->kiss.txtail = (uint8_t)tnc->tx_config.txtail_flags;
	if (afsk1200_tx_init(&tnc->tx, &tnc->tx_config) != AFSK1200_TX_OK)
		return TNC1200_ERR_ARG;
	if (afsk1200_stream_init(&tnc->rx) != AFSK1200_STREAM_OK)
		return TNC1200_ERR_ARG;
	tnc->p = tnc->kiss.p;
	tnc->slottime = tnc->kiss.slottime;
	tnc->fullduplex = tnc->kiss.fullduplex ? 1U : 0U;

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
tnc1200_stats(const struct tnc1200 *tnc, struct tnc1200_stats *stats)
{
	if (tnc == NULL || stats == NULL)
		return TNC1200_ERR_ARG;

	*stats = tnc->stats;
	return TNC1200_OK;
}

enum tnc1200_result
tnc1200_tx_process(struct tnc1200 *tnc, int16_t *out, size_t out_cap,
	size_t *out_samples)
{
	struct afsk1200_tx_stats tx_stats;
	size_t before;
	enum afsk1200_tx_result txres;

	if (tnc == NULL || out_samples == NULL)
		return TNC1200_ERR_ARG;
	*out_samples = 0U;
	if (out == NULL)
		return TNC1200_ERR_ARG;

	before = 0U;
	(void)afsk1200_tx_stats(&tnc->tx, &tx_stats);
	before = tx_stats.frames_done;
	txres = afsk1200_tx_process(&tnc->tx, out, out_cap, out_samples);
	tnc->stats.pcm_samples_out += *out_samples;
	if (afsk1200_tx_stats(&tnc->tx, &tx_stats) == AFSK1200_TX_OK &&
	    tx_stats.frames_done > before)
		tnc->stats.tx_frames_done += tx_stats.frames_done - before;
	if (txres == AFSK1200_TX_DONE && *out_samples == 0U)
		return TNC1200_ERR_NO_DATA;

	return tnc1200_map_tx(txres);
}

static void
tnc1200_apply_kiss_settings(struct tnc1200 *tnc)
{
	int active;

	tnc->tx_config.txdelay_flags = tnc->kiss.txdelay;
	tnc->tx_config.txtail_flags = tnc->kiss.txtail;
	tnc->p = tnc->kiss.p;
	tnc->slottime = tnc->kiss.slottime;
	tnc->fullduplex = tnc->kiss.fullduplex ? 1U : 0U;
	if (tnc->kiss.sethw_len != 0U) {
		(void)memcpy(tnc->sethw, tnc->kiss.sethw, tnc->kiss.sethw_len);
		tnc->sethw_len = tnc->kiss.sethw_len;
	}

	active = 0;
	if (afsk1200_tx_is_active(&tnc->tx, &active) == AFSK1200_TX_OK &&
	    active == 0)
		(void)afsk1200_tx_init(&tnc->tx, &tnc->tx_config);
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
tnc1200_start_tx(struct tnc1200 *tnc, const struct kiss_frame *frame)
{
	enum afsk1200_tx_result txres;

	txres = afsk1200_tx_start_frame(&tnc->tx, frame->data, frame->len);
	if (txres == AFSK1200_TX_OK) {
		tnc->stats.tx_frames_started++;
		return TNC1200_OK;
	}
	tnc->stats.tx_frames_rejected++;
	if (txres == AFSK1200_TX_ERR_BUSY)
		return TNC1200_ERR_BUSY;
	if (txres == AFSK1200_TX_ERR_BAD_FRAME)
		return TNC1200_OK;

	return tnc1200_map_tx(txres);
}

static enum tnc1200_result
tnc1200_validate_config(const struct tnc1200_config *config,
	struct afsk1200_tx_config *tx_config)
{
	if (config == NULL) {
		tx_config->txdelay_flags = AFSK1200_TX_DEFAULT_TXDELAY_FLAGS;
		tx_config->txtail_flags = AFSK1200_TX_DEFAULT_TXTAIL_FLAGS;
		tx_config->amplitude = AFSK1200_PCM_AMPLITUDE;
		return TNC1200_OK;
	}
	if (config->amplitude <= 0)
		return TNC1200_ERR_ARG;
	if (config->txdelay_flags > 255U || config->txtail_flags > 255U)
		return TNC1200_ERR_ARG;

	tx_config->txdelay_flags = config->txdelay_flags;
	tx_config->txtail_flags = config->txtail_flags;
	tx_config->amplitude = config->amplitude;
	return TNC1200_OK;
}
