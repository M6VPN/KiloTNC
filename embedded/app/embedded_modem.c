/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/app/embedded_modem.c */

#include <sys/types.h>

#include <stdint.h>
#include <string.h>

#include "embedded_modem.h"

static enum embedded_modem_result embedded_modem_check_format(
	const struct kilotnc_audio *);
static int embedded_modem_audio_rx_ready(const struct kilotnc_audio *);
static int embedded_modem_audio_tx_ready(const struct kilotnc_audio *);
static void embedded_modem_sync_rx_stats(struct embedded_modem *);

static int
embedded_modem_audio_rx_ready(const struct kilotnc_audio *audio)
{
	if (audio == NULL)
		return 0;
	if (audio->read_rx == NULL)
		return 0;
	if (audio->format == NULL)
		return 0;

	return 1;
}

static int
embedded_modem_audio_tx_ready(const struct kilotnc_audio *audio)
{
	if (audio == NULL)
		return 0;
	if (audio->write_tx == NULL)
		return 0;
	if (audio->format == NULL)
		return 0;

	return 1;
}

static enum embedded_modem_result
embedded_modem_check_format(const struct kilotnc_audio *audio)
{
	struct kilotnc_audio_format format;

	if (audio->format(audio->ctx, &format) != KILOTNC_AUDIO_OK)
		return EMBEDDED_MODEM_ERR_AUDIO;
	if (format.sample_rate != KILOTNC_AUDIO_SAMPLE_RATE ||
	    format.channels != KILOTNC_AUDIO_CHANNELS ||
	    format.bits_per_sample != KILOTNC_AUDIO_BITS_PER_SAMPLE)
		return EMBEDDED_MODEM_ERR_UNSUPPORTED;

	return EMBEDDED_MODEM_OK;
}

static void
embedded_modem_sync_rx_stats(struct embedded_modem *modem)
{
	struct afsk1200_stream_stats stats;

	if (afsk1200_stream_stats(&modem->rx, &stats) !=
	    AFSK1200_STREAM_OK) {
		modem->status.rx_audio_errors++;
		return;
	}

	modem->status.rx_frames_ok = stats.frames_ok;
	modem->status.rx_frames_bad_fcs = stats.frames_bad_fcs;
	modem->status.rx_frames_malformed = stats.frames_malformed;
	modem->status.rx_frames_dropped = stats.frames_dropped;
	modem->status.rx_samples_consumed = stats.samples_seen;
}

enum embedded_modem_result
embedded_modem_abort(struct embedded_modem *modem)
{
	if (modem == NULL)
		return EMBEDDED_MODEM_ERR_ARG;

	if (afsk1200_tx_abort(&modem->tx) != AFSK1200_TX_OK)
		return EMBEDDED_MODEM_ERR_ARG;
	modem->status.tx_active = 0U;
	modem->status.aborts++;
	return EMBEDDED_MODEM_OK;
}

enum embedded_modem_result
embedded_modem_init(struct embedded_modem *modem)
{
	struct afsk1200_tx_config config;

	if (modem == NULL)
		return EMBEDDED_MODEM_ERR_ARG;

	(void)memset(modem, 0, sizeof(*modem));
	config.txdelay_flags = AFSK1200_TX_DEFAULT_TXDELAY_FLAGS;
	config.txtail_flags = AFSK1200_TX_DEFAULT_TXTAIL_FLAGS;
	config.amplitude = AFSK1200_PCM_AMPLITUDE;
	if (afsk1200_tx_init(&modem->tx, &config) != AFSK1200_TX_OK)
		return EMBEDDED_MODEM_ERR_ARG;
	if (afsk1200_stream_init(&modem->rx) != AFSK1200_STREAM_OK)
		return EMBEDDED_MODEM_ERR_ARG;
	modem->status.current_mode = TNC_MODE_1200_AFSK_AX25;
	modem->status.tx_done = 1U;
	modem->status.rx_active = 1U;
	return EMBEDDED_MODEM_OK;
}

enum embedded_modem_result
embedded_modem_process_tx(struct embedded_modem *modem,
	const struct kilotnc_audio *audio, size_t chunk_samples)
{
	int16_t samples[EMBEDDED_MODEM_TX_CHUNK_MAX];
	enum afsk1200_tx_result tx_result;
	enum kilotnc_audio_result audio_result;
	enum embedded_modem_result modem_result;
	size_t produced;
	size_t written;

	if (modem == NULL || !embedded_modem_audio_tx_ready(audio))
		return EMBEDDED_MODEM_ERR_ARG;
	if (chunk_samples == 0U ||
	    chunk_samples > EMBEDDED_MODEM_TX_CHUNK_MAX)
		return EMBEDDED_MODEM_ERR_ARG;
	modem_result = embedded_modem_check_format(audio);
	if (modem_result != EMBEDDED_MODEM_OK)
		return modem_result;
	if (modem->status.tx_active == 0U)
		return EMBEDDED_MODEM_DONE;

	tx_result = afsk1200_tx_process(&modem->tx, samples, chunk_samples,
	    &produced);
	if (tx_result == AFSK1200_TX_ERR_SMALL) {
		modem->status.tx_underflows++;
		return EMBEDDED_MODEM_ERR_SMALL;
	}
	if (tx_result != AFSK1200_TX_OK && tx_result != AFSK1200_TX_DONE) {
		modem->status.tx_audio_errors++;
		modem->status.tx_active = 0U;
		return EMBEDDED_MODEM_ERR_AUDIO;
	}
	if (produced != 0U) {
		audio_result = audio->write_tx(audio->ctx, samples, produced,
		    &written);
		if (audio_result == KILOTNC_AUDIO_ERR_OVERFLOW) {
			modem->status.tx_audio_overflows++;
			modem->status.tx_audio_errors++;
			return EMBEDDED_MODEM_ERR_AUDIO;
		}
		if (audio_result != KILOTNC_AUDIO_OK ||
		    written != produced) {
			modem->status.tx_audio_errors++;
			return EMBEDDED_MODEM_ERR_AUDIO;
		}
		modem->status.tx_samples_generated += written;
	}
	if (tx_result == AFSK1200_TX_DONE) {
		modem->status.tx_active = 0U;
		modem->status.tx_done = 1U;
		modem->status.tx_frames_done++;
		return EMBEDDED_MODEM_DONE;
	}

	return EMBEDDED_MODEM_OK;
}

enum embedded_modem_result
embedded_modem_process_rx(struct embedded_modem *modem,
	const struct kilotnc_audio *audio, struct embedded_modem_rx_frame *frames,
	size_t frame_cap, size_t *out_frames)
{
	struct afsk1200_stream_frame stream_frames[EMBEDDED_MODEM_RX_FRAME_CAP];
	int16_t samples[EMBEDDED_MODEM_RX_CHUNK_MAX];
	enum afsk1200_stream_result stream_result;
	enum embedded_modem_result modem_result;
	enum kilotnc_audio_result audio_result;
	size_t read_len;
	size_t stream_cap;
	size_t stream_count;
	size_t i;

	if (modem == NULL || !embedded_modem_audio_rx_ready(audio) ||
	    out_frames == NULL)
		return EMBEDDED_MODEM_ERR_ARG;
	if (frames == NULL && frame_cap != 0U)
		return EMBEDDED_MODEM_ERR_ARG;
	*out_frames = 0U;
	modem_result = embedded_modem_check_format(audio);
	if (modem_result != EMBEDDED_MODEM_OK)
		return modem_result;
	if (modem->status.current_mode != TNC_MODE_1200_AFSK_AX25)
		return EMBEDDED_MODEM_ERR_UNSUPPORTED;

	audio_result = audio->read_rx(audio->ctx, samples,
	    EMBEDDED_MODEM_RX_CHUNK_MAX, &read_len);
	if (audio_result == KILOTNC_AUDIO_ERR_UNDERFLOW) {
		modem->status.rx_audio_underflows++;
		return EMBEDDED_MODEM_OK;
	}
	if (audio_result == KILOTNC_AUDIO_ERR_OVERFLOW) {
		modem->status.rx_audio_overflows++;
		modem->status.rx_audio_errors++;
		return EMBEDDED_MODEM_ERR_AUDIO;
	}
	if (audio_result != KILOTNC_AUDIO_OK) {
		modem->status.rx_audio_errors++;
		return EMBEDDED_MODEM_ERR_AUDIO;
	}

	stream_cap = frame_cap;
	if (stream_cap > EMBEDDED_MODEM_RX_FRAME_CAP)
		stream_cap = EMBEDDED_MODEM_RX_FRAME_CAP;
	stream_result = afsk1200_stream_process(&modem->rx, samples,
	    read_len, stream_frames, stream_cap, &stream_count);
	embedded_modem_sync_rx_stats(modem);
	if (stream_result != AFSK1200_STREAM_OK &&
	    stream_result != AFSK1200_STREAM_ERR_FRAME_DROPPED) {
		modem->status.rx_audio_errors++;
		return EMBEDDED_MODEM_ERR_AUDIO;
	}
	if (stream_result == AFSK1200_STREAM_ERR_FRAME_DROPPED) {
		if (stream_count > stream_cap)
			stream_count = stream_cap;
	}
	for (i = 0U; i < stream_count; i++) {
		if (i >= frame_cap || frames == NULL) {
			modem->status.rx_frames_dropped++;
			return EMBEDDED_MODEM_ERR_SMALL;
		}
		(void)memcpy(frames[i].data, stream_frames[i].data,
		    stream_frames[i].len);
		frames[i].len = stream_frames[i].len;
		(*out_frames)++;
	}
	if (stream_result == AFSK1200_STREAM_ERR_FRAME_DROPPED)
		return EMBEDDED_MODEM_ERR_SMALL;

	return EMBEDDED_MODEM_OK;
}

enum embedded_modem_result
embedded_modem_rx_reset(struct embedded_modem *modem)
{
	if (modem == NULL)
		return EMBEDDED_MODEM_ERR_ARG;
	if (afsk1200_stream_init(&modem->rx) != AFSK1200_STREAM_OK)
		return EMBEDDED_MODEM_ERR_ARG;

	modem->status.rx_active = 1U;
	modem->status.rx_frames_ok = 0U;
	modem->status.rx_frames_bad_fcs = 0U;
	modem->status.rx_frames_malformed = 0U;
	modem->status.rx_frames_dropped = 0U;
	modem->status.rx_samples_consumed = 0U;
	modem->status.rx_audio_errors = 0U;
	modem->status.rx_audio_underflows = 0U;
	modem->status.rx_audio_overflows = 0U;
	return EMBEDDED_MODEM_OK;
}

enum embedded_modem_result
embedded_modem_start_ax25(struct embedded_modem *modem,
	const uint8_t *frame, size_t frame_len, enum tnc_mode_id mode)
{
	int active;
	enum afsk1200_tx_result tx_result;

	if (modem == NULL || frame == NULL)
		return EMBEDDED_MODEM_ERR_ARG;
	if (frame_len == 0U || frame_len > KILOTNC_AX25_MAX_FRAME) {
		modem->status.tx_frames_rejected++;
		return EMBEDDED_MODEM_ERR_SMALL;
	}
	if (mode != TNC_MODE_1200_AFSK_AX25) {
		modem->status.tx_frames_rejected++;
		return EMBEDDED_MODEM_ERR_UNSUPPORTED;
	}
	if (afsk1200_tx_is_active(&modem->tx, &active) != AFSK1200_TX_OK)
		return EMBEDDED_MODEM_ERR_ARG;
	if (active != 0) {
		modem->status.tx_frames_rejected++;
		return EMBEDDED_MODEM_ERR_BUSY;
	}

	tx_result = afsk1200_tx_start_frame(&modem->tx, frame, frame_len);
	if (tx_result == AFSK1200_TX_ERR_BUSY) {
		modem->status.tx_frames_rejected++;
		return EMBEDDED_MODEM_ERR_BUSY;
	}
	if (tx_result == AFSK1200_TX_ERR_BAD_FRAME ||
	    tx_result == AFSK1200_TX_ERR_SMALL) {
		modem->status.tx_frames_rejected++;
		return EMBEDDED_MODEM_ERR_SMALL;
	}
	if (tx_result != AFSK1200_TX_OK) {
		modem->status.tx_frames_rejected++;
		return EMBEDDED_MODEM_ERR_ARG;
	}

	modem->status.current_mode = mode;
	modem->status.tx_frames_started++;
	modem->status.tx_active = 1U;
	modem->status.tx_done = 0U;
	return EMBEDDED_MODEM_OK;
}

enum embedded_modem_result
embedded_modem_status(const struct embedded_modem *modem,
	struct embedded_modem_status *status)
{
	if (modem == NULL || status == NULL)
		return EMBEDDED_MODEM_ERR_ARG;

	*status = modem->status;
	return EMBEDDED_MODEM_OK;
}
