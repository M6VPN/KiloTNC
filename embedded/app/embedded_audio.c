/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/app/embedded_audio.c */

#include <sys/types.h>

#include <stdint.h>
#include <string.h>

#include "embedded_audio.h"

static int embedded_audio_ready(const struct kilotnc_audio *);
static enum embedded_audio_result embedded_audio_validate_format(
	const struct kilotnc_audio *);

static int
embedded_audio_ready(const struct kilotnc_audio *audio)
{
	if (audio == NULL)
		return 0;
	if (audio->read_rx == NULL)
		return 0;
	if (audio->write_tx == NULL)
		return 0;
	if (audio->format == NULL)
		return 0;
	if (audio->stats == NULL)
		return 0;

	return 1;
}

static enum embedded_audio_result
embedded_audio_validate_format(const struct kilotnc_audio *audio)
{
	struct kilotnc_audio_format format;

	if (!embedded_audio_ready(audio))
		return EMBEDDED_AUDIO_ERR_ARG;
	if (audio->format(audio->ctx, &format) != KILOTNC_AUDIO_OK)
		return EMBEDDED_AUDIO_ERR_AUDIO;
	if (format.sample_rate != KILOTNC_AUDIO_SAMPLE_RATE)
		return EMBEDDED_AUDIO_ERR_UNSUPPORTED;
	if (format.channels != KILOTNC_AUDIO_CHANNELS)
		return EMBEDDED_AUDIO_ERR_UNSUPPORTED;
	if (format.bits_per_sample != KILOTNC_AUDIO_BITS_PER_SAMPLE)
		return EMBEDDED_AUDIO_ERR_UNSUPPORTED;

	return EMBEDDED_AUDIO_OK;
}

enum embedded_audio_result
embedded_audio_init(struct embedded_audio *bridge,
	const struct kilotnc_audio *audio, size_t block_samples)
{
	enum embedded_audio_result result;

	if (bridge == NULL || block_samples == 0 ||
	    block_samples > EMBEDDED_AUDIO_BLOCK_MAX)
		return EMBEDDED_AUDIO_ERR_ARG;

	result = embedded_audio_validate_format(audio);
	if (result != EMBEDDED_AUDIO_OK)
		return result;

	(void)memset(bridge, 0, sizeof(*bridge));
	bridge->audio = audio;
	bridge->block_samples = block_samples;
	return EMBEDDED_AUDIO_OK;
}

enum embedded_audio_result
embedded_audio_process(struct embedded_audio *bridge)
{
	int16_t samples[EMBEDDED_AUDIO_BLOCK_MAX];
	enum kilotnc_audio_result audio_result;
	size_t read_len;
	size_t written;

	if (bridge == NULL || !embedded_audio_ready(bridge->audio) ||
	    bridge->block_samples == 0 ||
	    bridge->block_samples > EMBEDDED_AUDIO_BLOCK_MAX)
		return EMBEDDED_AUDIO_ERR_ARG;

	audio_result = bridge->audio->read_rx(bridge->audio->ctx, samples,
	    bridge->block_samples, &read_len);
	if (audio_result == KILOTNC_AUDIO_ERR_UNDERFLOW) {
		bridge->stats.rx_underflows++;
		return EMBEDDED_AUDIO_OK;
	}
	if (audio_result != KILOTNC_AUDIO_OK) {
		bridge->stats.audio_errors++;
		return EMBEDDED_AUDIO_ERR_AUDIO;
	}
	if (read_len == 0) {
		bridge->stats.rx_underflows++;
		return EMBEDDED_AUDIO_OK;
	}

	bridge->stats.rx_samples += read_len;
	audio_result = bridge->audio->write_tx(bridge->audio->ctx, samples,
	    read_len, &written);
	if (audio_result == KILOTNC_AUDIO_ERR_OVERFLOW) {
		bridge->stats.tx_overflows++;
		return EMBEDDED_AUDIO_OK;
	}
	if (audio_result != KILOTNC_AUDIO_OK || written != read_len) {
		bridge->stats.audio_errors++;
		return EMBEDDED_AUDIO_ERR_AUDIO;
	}

	bridge->stats.tx_samples += written;
	bridge->stats.loopback_blocks++;
	return EMBEDDED_AUDIO_OK;
}

enum embedded_audio_result
embedded_audio_stats(const struct embedded_audio *bridge,
	struct embedded_audio_stats *stats)
{
	if (bridge == NULL || stats == NULL)
		return EMBEDDED_AUDIO_ERR_ARG;

	*stats = bridge->stats;
	return EMBEDDED_AUDIO_OK;
}
