/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/include/kilotnc_audio.h */

#ifndef KILOTNC_AUDIO_H
#define KILOTNC_AUDIO_H

#include <sys/types.h>

#include <stdint.h>

#define KILOTNC_AUDIO_SAMPLE_RATE 48000U
#define KILOTNC_AUDIO_CHANNELS 1U
#define KILOTNC_AUDIO_BITS_PER_SAMPLE 16U

enum kilotnc_audio_result {
	KILOTNC_AUDIO_OK = 0,
	KILOTNC_AUDIO_ERR_ARG,
	KILOTNC_AUDIO_ERR_SMALL,
	KILOTNC_AUDIO_ERR_OVERFLOW,
	KILOTNC_AUDIO_ERR_UNDERFLOW,
	KILOTNC_AUDIO_ERR_UNSUPPORTED
};

struct kilotnc_audio_format {
	uint32_t sample_rate;
	uint8_t channels;
	uint8_t bits_per_sample;
};

struct kilotnc_audio_stats {
	size_t rx_samples_read;
	size_t tx_samples_written;
	size_t rx_overflows;
	size_t tx_overflows;
	size_t rx_underflows;
	size_t tx_underflows;
};

struct kilotnc_audio {
	void *ctx;
	enum kilotnc_audio_result (*read_rx)(void *, int16_t *, size_t,
	    size_t *);
	enum kilotnc_audio_result (*write_tx)(void *, const int16_t *, size_t,
	    size_t *);
	enum kilotnc_audio_result (*format)(void *,
	    struct kilotnc_audio_format *);
	enum kilotnc_audio_result (*stats)(void *,
	    struct kilotnc_audio_stats *);
};

#endif
