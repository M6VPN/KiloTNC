/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/daemon/kilotncd_audio_alsa.c */

#include <sys/types.h>

#include <stddef.h>
#include <stdint.h>

#include "kilotncd_audio_alsa.h"

#ifndef KILOTNCD_ENABLE_ALSA

enum kilotncd_audio_result
kilotncd_audio_alsa_close(struct kilotncd_audio *audio)
{
	if (audio == NULL)
		return KILOTNCD_AUDIO_ERR_ARG;
	audio->opened = 0;

	return KILOTNCD_AUDIO_ERR_UNSUPPORTED;
}

enum kilotncd_audio_result
kilotncd_audio_alsa_open_input(struct kilotncd_audio *audio,
	const struct kilotncd_audio_config *config)
{
	(void)audio;
	(void)config;

	if (audio == NULL || config == NULL)
		return KILOTNCD_AUDIO_ERR_ARG;

	return KILOTNCD_AUDIO_ERR_UNSUPPORTED;
}

enum kilotncd_audio_result
kilotncd_audio_alsa_open_output(struct kilotncd_audio *audio,
	const struct kilotncd_audio_config *config)
{
	(void)audio;
	(void)config;

	if (audio == NULL || config == NULL)
		return KILOTNCD_AUDIO_ERR_ARG;

	return KILOTNCD_AUDIO_ERR_UNSUPPORTED;
}

enum kilotncd_audio_result
kilotncd_audio_alsa_read(struct kilotncd_audio *audio, int16_t *pcm,
	size_t cap, size_t *samples)
{
	(void)audio;
	(void)pcm;
	(void)cap;
	(void)samples;

	if (audio == NULL || pcm == NULL || samples == NULL)
		return KILOTNCD_AUDIO_ERR_ARG;

	return KILOTNCD_AUDIO_ERR_UNSUPPORTED;
}

enum kilotncd_audio_result
kilotncd_audio_alsa_write(struct kilotncd_audio *audio,
	const int16_t *pcm, size_t samples)
{
	(void)audio;
	(void)pcm;
	(void)samples;

	if (audio == NULL || (pcm == NULL && samples != 0U))
		return KILOTNCD_AUDIO_ERR_ARG;

	return KILOTNCD_AUDIO_ERR_UNSUPPORTED;
}

#else

enum kilotncd_audio_result
kilotncd_audio_alsa_close(struct kilotncd_audio *audio)
{
	if (audio == NULL)
		return KILOTNCD_AUDIO_ERR_ARG;
	audio->opened = 0;

	return KILOTNCD_AUDIO_ERR_UNSUPPORTED;
}

enum kilotncd_audio_result
kilotncd_audio_alsa_open_input(struct kilotncd_audio *audio,
	const struct kilotncd_audio_config *config)
{
	(void)audio;
	(void)config;

	if (audio == NULL || config == NULL)
		return KILOTNCD_AUDIO_ERR_ARG;

	return KILOTNCD_AUDIO_ERR_UNSUPPORTED;
}

enum kilotncd_audio_result
kilotncd_audio_alsa_open_output(struct kilotncd_audio *audio,
	const struct kilotncd_audio_config *config)
{
	(void)audio;
	(void)config;

	if (audio == NULL || config == NULL)
		return KILOTNCD_AUDIO_ERR_ARG;

	return KILOTNCD_AUDIO_ERR_UNSUPPORTED;
}

enum kilotncd_audio_result
kilotncd_audio_alsa_read(struct kilotncd_audio *audio, int16_t *pcm,
	size_t cap, size_t *samples)
{
	(void)audio;
	(void)pcm;
	(void)cap;
	(void)samples;

	if (audio == NULL || pcm == NULL || samples == NULL)
		return KILOTNCD_AUDIO_ERR_ARG;

	return KILOTNCD_AUDIO_ERR_UNSUPPORTED;
}

enum kilotncd_audio_result
kilotncd_audio_alsa_write(struct kilotncd_audio *audio,
	const int16_t *pcm, size_t samples)
{
	(void)audio;
	(void)pcm;
	(void)samples;

	if (audio == NULL || (pcm == NULL && samples != 0U))
		return KILOTNCD_AUDIO_ERR_ARG;

	return KILOTNCD_AUDIO_ERR_UNSUPPORTED;
}

#endif
