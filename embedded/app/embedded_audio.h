/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/app/embedded_audio.h */

#ifndef EMBEDDED_AUDIO_H
#define EMBEDDED_AUDIO_H

#include <sys/types.h>

#include <stdint.h>

#include "kilotnc_audio.h"

#define EMBEDDED_AUDIO_BLOCK_MAX 64

enum embedded_audio_result {
	EMBEDDED_AUDIO_OK = 0,
	EMBEDDED_AUDIO_ERR_ARG,
	EMBEDDED_AUDIO_ERR_AUDIO,
	EMBEDDED_AUDIO_ERR_UNSUPPORTED
};

struct embedded_audio_stats {
	size_t loopback_blocks;
	size_t rx_samples;
	size_t tx_samples;
	size_t rx_underflows;
	size_t tx_overflows;
	size_t audio_errors;
};

struct embedded_audio {
	const struct kilotnc_audio *audio;
	size_t block_samples;
	struct embedded_audio_stats stats;
};

enum embedded_audio_result embedded_audio_init(struct embedded_audio *,
	const struct kilotnc_audio *, size_t);
enum embedded_audio_result embedded_audio_process(struct embedded_audio *);
enum embedded_audio_result embedded_audio_stats(const struct embedded_audio *,
	struct embedded_audio_stats *);

#endif
