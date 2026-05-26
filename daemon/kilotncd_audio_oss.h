/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/daemon/kilotncd_audio_oss.h */

#ifndef KILOTNCD_AUDIO_OSS_H
#define KILOTNCD_AUDIO_OSS_H

#include <sys/types.h>

#include <stdint.h>

#include "kilotncd_audio.h"

enum kilotncd_audio_result kilotncd_audio_oss_close(
	struct kilotncd_audio *);
enum kilotncd_audio_result kilotncd_audio_oss_open_input(
	struct kilotncd_audio *, const struct kilotncd_audio_config *);
enum kilotncd_audio_result kilotncd_audio_oss_open_output(
	struct kilotncd_audio *, const struct kilotncd_audio_config *);
enum kilotncd_audio_result kilotncd_audio_oss_read(
	struct kilotncd_audio *, int16_t *, size_t, size_t *);
enum kilotncd_audio_result kilotncd_audio_oss_write(
	struct kilotncd_audio *, const int16_t *, size_t);

#endif
