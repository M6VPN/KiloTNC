/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/app/embedded_modem.c */

#include <sys/types.h>

#include <stdint.h>
#include <string.h>

#include "embedded_modem.h"

static int embedded_modem_audio_ready(const struct kilotnc_audio *);

static int
embedded_modem_audio_ready(const struct kilotnc_audio *audio)
{
	if (audio == NULL)
		return 0;
	if (audio->write_tx == NULL)
		return 0;
	if (audio->format == NULL)
		return 0;

	return 1;
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
	modem->status.current_mode = TNC_MODE_1200_AFSK_AX25;
	modem->status.tx_done = 1U;
	return EMBEDDED_MODEM_OK;
}

enum embedded_modem_result
embedded_modem_process_tx(struct embedded_modem *modem,
	const struct kilotnc_audio *audio, size_t chunk_samples)
{
	struct kilotnc_audio_format format;
	int16_t samples[EMBEDDED_MODEM_TX_CHUNK_MAX];
	enum afsk1200_tx_result tx_result;
	enum kilotnc_audio_result audio_result;
	size_t produced;
	size_t written;

	if (modem == NULL || !embedded_modem_audio_ready(audio))
		return EMBEDDED_MODEM_ERR_ARG;
	if (chunk_samples == 0U ||
	    chunk_samples > EMBEDDED_MODEM_TX_CHUNK_MAX)
		return EMBEDDED_MODEM_ERR_ARG;
	if (audio->format(audio->ctx, &format) != KILOTNC_AUDIO_OK)
		return EMBEDDED_MODEM_ERR_AUDIO;
	if (format.sample_rate != KILOTNC_AUDIO_SAMPLE_RATE ||
	    format.channels != KILOTNC_AUDIO_CHANNELS ||
	    format.bits_per_sample != KILOTNC_AUDIO_BITS_PER_SAMPLE)
		return EMBEDDED_MODEM_ERR_UNSUPPORTED;
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
