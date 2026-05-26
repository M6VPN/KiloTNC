/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/daemon/kilotncd_audio_raw.c */

#include <sys/types.h>

#include <stddef.h>
#include <stdint.h>

#include "kilotncd_audio_raw.h"
#include "kilotncd_file.h"

enum kilotncd_audio_result
kilotncd_audio_raw_read(const struct kilotncd_audio_config *config,
	int16_t *pcm, size_t cap, size_t *samples)
{
	enum kilotncd_file_result res;

	if (config == NULL || pcm == NULL || samples == NULL)
		return KILOTNCD_AUDIO_ERR_ARG;
	res = kilotncd_file_read_pcm16(config->path, pcm, cap, samples);
	if (res == KILOTNCD_FILE_OK)
		return KILOTNCD_AUDIO_OK;
	if (res == KILOTNCD_FILE_ERR_RANGE)
		return KILOTNCD_AUDIO_ERR_SMALL;
	return KILOTNCD_AUDIO_ERR_IO;
}

enum kilotncd_audio_result
kilotncd_audio_raw_write(const struct kilotncd_audio_config *config,
	const int16_t *pcm, size_t samples)
{
	enum kilotncd_file_result res;

	if (config == NULL || (pcm == NULL && samples != 0U))
		return KILOTNCD_AUDIO_ERR_ARG;
	res = kilotncd_file_write_pcm16(config->path, pcm, samples);
	if (res == KILOTNCD_FILE_OK)
		return KILOTNCD_AUDIO_OK;
	return KILOTNCD_AUDIO_ERR_IO;
}
