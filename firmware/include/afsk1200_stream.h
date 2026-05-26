/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/include/afsk1200_stream.h */

#ifndef AFSK1200_STREAM_H
#define AFSK1200_STREAM_H

#include <sys/types.h>

#include <stdbool.h>
#include <stdint.h>

#include "afsk1200.h"
#include "kilotnc_limits.h"

enum afsk1200_stream_result {
	AFSK1200_STREAM_OK = 0,
	AFSK1200_STREAM_ERR_ARG,
	AFSK1200_STREAM_ERR_SMALL,
	AFSK1200_STREAM_ERR_BAD_LEN,
	AFSK1200_STREAM_ERR_FRAME_DROPPED
};

enum afsk1200_stream_state {
	AFSK1200_STREAM_SEARCH_FLAG = 0,
	AFSK1200_STREAM_IN_FRAME,
	AFSK1200_STREAM_DROP_OVERSIZE
};

struct afsk1200_stream_stats {
	size_t samples_seen;
	size_t bits_decoded;
	size_t flags_seen;
	size_t frames_seen;
	size_t frames_ok;
	size_t frames_bad_fcs;
	size_t frames_too_large;
	size_t frames_malformed;
	size_t frames_dropped;
	size_t chunks_processed;
	uint16_t dcd_score;
	uint16_t confidence_avg;
};

struct afsk1200_stream_frame {
	uint8_t data[KILOTNC_AX25_MAX_FRAME];
	size_t len;
};

struct afsk1200_stream {
	int16_t sample_window[AFSK1200_SAMPLES_PER_BIT];
	size_t sample_count;
	uint8_t nrzi_prev;
	uint8_t flag_shift;
	size_t flag_bits;
	uint8_t frame_bits[AFSK1200_MAX_TEST_BITS];
	size_t frame_bits_count;
	bool signal_started;
	bool have_last_sample;
	int16_t last_sample;
	uint32_t confidence_total;
	size_t confidence_count;
	enum afsk1200_stream_state state;
	struct afsk1200_stream_stats stats;
};

enum afsk1200_stream_result afsk1200_stream_flush(
	struct afsk1200_stream *, struct afsk1200_stream_frame *, size_t,
	size_t *);
enum afsk1200_stream_result afsk1200_stream_init(
	struct afsk1200_stream *);
enum afsk1200_stream_result afsk1200_stream_process(
	struct afsk1200_stream *, const int16_t *, size_t,
	struct afsk1200_stream_frame *, size_t, size_t *);
enum afsk1200_stream_result afsk1200_stream_stats(
	const struct afsk1200_stream *, struct afsk1200_stream_stats *);

#endif
