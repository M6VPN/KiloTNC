/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/daemon/kilotncd_audio.c */

#include <sys/types.h>

#include <stdint.h>
#include <string.h>

#include "afsk1200.h"
#include "kilotncd_audio.h"
#include "kilotncd_audio_raw.h"

enum kilotncd_audio_result
kilotncd_audio_backend_name(enum kilotncd_audio_backend backend,
	const char **name)
{
	if (name == NULL)
		return KILOTNCD_AUDIO_ERR_ARG;
	if (backend == KILOTNCD_AUDIO_BACKEND_RAW_FILE) {
		*name = "raw";
		return KILOTNCD_AUDIO_OK;
	}
	if (backend == KILOTNCD_AUDIO_BACKEND_ALSA) {
		*name = "alsa";
		return KILOTNCD_AUDIO_OK;
	}
	if (backend == KILOTNCD_AUDIO_BACKEND_SNDIO) {
		*name = "sndio";
		return KILOTNCD_AUDIO_OK;
	}
	if (backend == KILOTNCD_AUDIO_BACKEND_OSS) {
		*name = "oss";
		return KILOTNCD_AUDIO_OK;
	}

	return KILOTNCD_AUDIO_ERR_UNSUPPORTED;
}

enum kilotncd_audio_result
kilotncd_audio_close(struct kilotncd_audio *audio)
{
	if (audio == NULL)
		return KILOTNCD_AUDIO_ERR_ARG;
	audio->opened = 0;

	return KILOTNCD_AUDIO_OK;
}

void
kilotncd_audio_default_format(struct kilotncd_audio_format *format)
{
	if (format == NULL)
		return;
	format->sample_rate = AFSK1200_SAMPLE_RATE;
	format->channels = 1U;
	format->bits_per_sample = 16U;
	format->little_endian = 1U;
}

enum kilotncd_audio_result
kilotncd_audio_open_input(struct kilotncd_audio *audio,
	const struct kilotncd_audio_config *config)
{
	if (audio == NULL || config == NULL)
		return KILOTNCD_AUDIO_ERR_ARG;
	if (config->backend != KILOTNCD_AUDIO_BACKEND_RAW_FILE)
		return KILOTNCD_AUDIO_ERR_UNSUPPORTED;
	if (kilotncd_audio_validate_format(&config->format) !=
	    KILOTNCD_AUDIO_OK)
		return KILOTNCD_AUDIO_ERR_UNSUPPORTED;
	(void)memset(audio, 0, sizeof(*audio));
	audio->config = *config;
	audio->output = 0;
	audio->opened = 1;

	return KILOTNCD_AUDIO_OK;
}

enum kilotncd_audio_result
kilotncd_audio_open_output(struct kilotncd_audio *audio,
	const struct kilotncd_audio_config *config)
{
	if (audio == NULL || config == NULL)
		return KILOTNCD_AUDIO_ERR_ARG;
	if (config->backend != KILOTNCD_AUDIO_BACKEND_RAW_FILE)
		return KILOTNCD_AUDIO_ERR_UNSUPPORTED;
	if (kilotncd_audio_validate_format(&config->format) !=
	    KILOTNCD_AUDIO_OK)
		return KILOTNCD_AUDIO_ERR_UNSUPPORTED;
	(void)memset(audio, 0, sizeof(*audio));
	audio->config = *config;
	audio->output = 1;
	audio->opened = 1;

	return KILOTNCD_AUDIO_OK;
}

enum kilotncd_audio_result
kilotncd_audio_parse_backend(const char *name,
	enum kilotncd_audio_backend *backend)
{
	if (name == NULL || backend == NULL)
		return KILOTNCD_AUDIO_ERR_ARG;
	if (strcmp(name, "raw") == 0 || strcmp(name, "raw-file") == 0) {
		*backend = KILOTNCD_AUDIO_BACKEND_RAW_FILE;
		return KILOTNCD_AUDIO_OK;
	}
	if (strcmp(name, "alsa") == 0) {
		*backend = KILOTNCD_AUDIO_BACKEND_ALSA;
		return KILOTNCD_AUDIO_ERR_UNSUPPORTED;
	}
	if (strcmp(name, "sndio") == 0) {
		*backend = KILOTNCD_AUDIO_BACKEND_SNDIO;
		return KILOTNCD_AUDIO_ERR_UNSUPPORTED;
	}
	if (strcmp(name, "oss") == 0) {
		*backend = KILOTNCD_AUDIO_BACKEND_OSS;
		return KILOTNCD_AUDIO_ERR_UNSUPPORTED;
	}

	return KILOTNCD_AUDIO_ERR_UNSUPPORTED;
}

enum kilotncd_audio_result
kilotncd_audio_read(struct kilotncd_audio *audio, int16_t *pcm,
	size_t cap, size_t *samples)
{
	if (audio == NULL || pcm == NULL || samples == NULL)
		return KILOTNCD_AUDIO_ERR_ARG;
	if (!audio->opened || audio->output)
		return KILOTNCD_AUDIO_ERR_ARG;
	if (audio->config.backend == KILOTNCD_AUDIO_BACKEND_RAW_FILE)
		return kilotncd_audio_raw_read(&audio->config, pcm, cap,
		    samples);

	return KILOTNCD_AUDIO_ERR_UNSUPPORTED;
}

enum kilotncd_audio_result
kilotncd_audio_validate_format(const struct kilotncd_audio_format *format)
{
	if (format == NULL)
		return KILOTNCD_AUDIO_ERR_ARG;
	if (format->sample_rate != AFSK1200_SAMPLE_RATE ||
	    format->channels != 1U ||
	    format->bits_per_sample != 16U ||
	    format->little_endian != 1U)
		return KILOTNCD_AUDIO_ERR_UNSUPPORTED;

	return KILOTNCD_AUDIO_OK;
}

enum kilotncd_audio_result
kilotncd_audio_write(struct kilotncd_audio *audio, const int16_t *pcm,
	size_t samples)
{
	if (audio == NULL || (pcm == NULL && samples != 0U))
		return KILOTNCD_AUDIO_ERR_ARG;
	if (!audio->opened || !audio->output)
		return KILOTNCD_AUDIO_ERR_ARG;
	if (audio->config.backend == KILOTNCD_AUDIO_BACKEND_RAW_FILE)
		return kilotncd_audio_raw_write(&audio->config, pcm,
		    samples);

	return KILOTNCD_AUDIO_ERR_UNSUPPORTED;
}
