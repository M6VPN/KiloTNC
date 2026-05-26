/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/src/afsk1200_rx.c */

#include <sys/types.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "afsk1200.h"
#include "afsk1200_rx.h"
#include "ax25.h"
#include "hdlc.h"

#define AFSK1200_RX_FLAG_BITS	8U

static enum afsk1200_rx_result afsk1200_rx_handle_candidate(
	const uint8_t *, size_t, struct afsk1200_rx_frame *, size_t,
	size_t *, struct afsk1200_rx_stats *);
static bool afsk1200_rx_is_flag(const uint8_t *, size_t);
static enum afsk1200_rx_result afsk1200_rx_map_decode(
	enum afsk1200_result);
static enum afsk1200_rx_result afsk1200_rx_pack_bits(const uint8_t *,
	size_t, uint8_t *, size_t);
static void afsk1200_rx_stats_init(struct afsk1200_rx_stats *);

/*
 * Decode one host-side PCM buffer into zero or more AX.25 UI frames with FCS.
 * All output storage is caller-owned and bounded by frame_cap.
 */
enum afsk1200_rx_result
afsk1200_rx_decode_frames(const int16_t *pcm, size_t sample_count,
	struct afsk1200_rx_frame *frames, size_t frame_cap, size_t *out_frames,
	struct afsk1200_rx_stats *stats)
{
	uint8_t tone_bits[AFSK1200_MAX_TEST_BITS];
	uint8_t data_bits[AFSK1200_MAX_TEST_BITS];
	struct afsk1200_metrics metrics;
	size_t tone_count;
	size_t data_count;
	size_t frame_start;
	size_t i;
	bool have_start;
	bool partial;
	enum afsk1200_result ares;
	enum afsk1200_rx_result rres;

	if (out_frames == NULL)
		return AFSK1200_RX_ERR_ARG;
	*out_frames = 0U;
	afsk1200_rx_stats_init(stats);

	if ((pcm == NULL && sample_count != 0U) || frames == NULL)
		return AFSK1200_RX_ERR_ARG;

	ares = afsk1200_decode_pcm_search(pcm, sample_count, tone_bits,
	    sizeof(tone_bits), &tone_count, &metrics);
	if (ares != AFSK1200_OK)
		return afsk1200_rx_map_decode(ares);

	ares = afsk1200_nrzi_decode(tone_bits, tone_count, data_bits,
	    sizeof(data_bits), &data_count);
	if (ares != AFSK1200_OK)
		return afsk1200_rx_map_decode(ares);

	if (stats != NULL) {
		stats->bits_decoded = data_count;
		stats->dcd_score = metrics.dcd_score;
		stats->confidence_avg = metrics.confidence_avg;
	}

	have_start = false;
	partial = false;
	frame_start = 0U;
	for (i = 0U; i + AFSK1200_RX_FLAG_BITS <= data_count; i++) {
		if (!afsk1200_rx_is_flag(data_bits, i))
			continue;
		if (stats != NULL)
			stats->flags_seen++;
		if (have_start && i > frame_start) {
			rres = afsk1200_rx_handle_candidate(
			    &data_bits[frame_start], i - frame_start, frames,
			    frame_cap, out_frames, stats);
			if (rres != AFSK1200_RX_OK)
				return rres;
		}
		have_start = true;
		frame_start = i + AFSK1200_RX_FLAG_BITS;
		i += AFSK1200_RX_FLAG_BITS - 1U;
	}

	if (have_start && frame_start < data_count)
		partial = true;
	if (*out_frames != 0U)
		return AFSK1200_RX_OK;
	if (partial)
		return AFSK1200_RX_ERR_PARTIAL;
	return AFSK1200_RX_ERR_NO_FRAME;
}

static enum afsk1200_rx_result
afsk1200_rx_handle_candidate(const uint8_t *bits, size_t bit_count,
	struct afsk1200_rx_frame *frames, size_t frame_cap, size_t *out_frames,
	struct afsk1200_rx_stats *stats)
{
	uint8_t packed[(AFSK1200_MAX_TEST_BITS + 7U) / 8U];
	uint8_t unstuffed[KILOTNC_AX25_MAX_FRAME];
	struct ax25_frame decoded;
	size_t unstuffed_bits;
	size_t frame_len;
	enum hdlc_result hres;
	enum ax25_result axres;
	enum afsk1200_rx_result rres;

	if (bit_count == 0U)
		return AFSK1200_RX_OK;
	if (stats != NULL)
		stats->frames_seen++;

	rres = afsk1200_rx_pack_bits(bits, bit_count, packed, sizeof(packed));
	if (rres != AFSK1200_RX_OK) {
		if (stats != NULL)
			stats->frames_too_large++;
		return AFSK1200_RX_OK;
	}

	hres = hdlc_unstuff(packed, bit_count, unstuffed, sizeof(unstuffed),
	    &unstuffed_bits);
	if (hres == HDLC_ERR_SMALL) {
		if (stats != NULL)
			stats->frames_too_large++;
		return AFSK1200_RX_OK;
	}
	if (hres != HDLC_OK || (unstuffed_bits % 8U) != 0U) {
		if (stats != NULL)
			stats->frames_malformed++;
		return AFSK1200_RX_OK;
	}

	frame_len = unstuffed_bits / 8U;
	if (frame_len > KILOTNC_AX25_MAX_FRAME) {
		if (stats != NULL)
			stats->frames_too_large++;
		return AFSK1200_RX_OK;
	}

	axres = ax25_decode_ui_fcs(unstuffed, frame_len, &decoded);
	if (axres == AX25_ERR_BAD_FCS) {
		if (stats != NULL)
			stats->frames_bad_fcs++;
		return AFSK1200_RX_OK;
	}
	if (axres != AX25_OK) {
		if (stats != NULL)
			stats->frames_malformed++;
		return AFSK1200_RX_OK;
	}
	if (*out_frames >= frame_cap)
		return AFSK1200_RX_ERR_SMALL;

	(void)memcpy(frames[*out_frames].data, unstuffed, frame_len);
	frames[*out_frames].len = frame_len;
	(*out_frames)++;
	if (stats != NULL)
		stats->frames_ok++;

	return AFSK1200_RX_OK;
}

static bool
afsk1200_rx_is_flag(const uint8_t *bits, size_t off)
{
	size_t i;
	uint8_t value;

	value = 0U;
	for (i = 0U; i < AFSK1200_RX_FLAG_BITS; i++) {
		if (bits[off + i] != 0U)
			value |= (uint8_t)(1U << i);
	}

	return value == KILOTNC_HDLC_FLAG;
}

static enum afsk1200_rx_result
afsk1200_rx_map_decode(enum afsk1200_result res)
{
	if (res == AFSK1200_ERR_ARG)
		return AFSK1200_RX_ERR_ARG;
	if (res == AFSK1200_ERR_SMALL)
		return AFSK1200_RX_ERR_SMALL;
	if (res == AFSK1200_ERR_BAD_LEN)
		return AFSK1200_RX_ERR_NO_FRAME;
	return AFSK1200_RX_ERR_BAD_LEN;
}

static enum afsk1200_rx_result
afsk1200_rx_pack_bits(const uint8_t *bits, size_t bit_count, uint8_t *out,
	size_t out_cap)
{
	size_t i;
	size_t needed;

	needed = (bit_count + 7U) / 8U;
	if (needed > out_cap)
		return AFSK1200_RX_ERR_SMALL;

	(void)memset(out, 0, out_cap);
	for (i = 0U; i < bit_count; i++) {
		if (bits[i] != 0U)
			out[i / 8U] |= (uint8_t)(1U << (i % 8U));
	}

	return AFSK1200_RX_OK;
}

static void
afsk1200_rx_stats_init(struct afsk1200_rx_stats *stats)
{
	if (stats == NULL)
		return;

	(void)memset(stats, 0, sizeof(*stats));
}
