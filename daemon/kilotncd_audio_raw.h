/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/daemon/kilotncd_audio_raw.h */

#ifndef KILOTNCD_AUDIO_RAW_H
#define KILOTNCD_AUDIO_RAW_H

#include <sys/types.h>

#include <stdint.h>

#include "kilotncd_audio.h"

enum kilotncd_audio_result kilotncd_audio_raw_read(
	const struct kilotncd_audio_config *, int16_t *, size_t, size_t *);
enum kilotncd_audio_result kilotncd_audio_raw_write(
	const struct kilotncd_audio_config *, const int16_t *, size_t);

#endif
