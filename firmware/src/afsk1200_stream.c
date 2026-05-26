/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/src/afsk1200_stream.c */

#include <sys/types.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "afsk1200_stream.h"
#include "ax25.h"
#include "hdlc.h"

#define AFSK1200_STREAM_Q			16384
#define AFSK1200_STREAM_MARK_COEFF		32365
#define AFSK1200_STREAM_SPACE_COEFF		31419
#define AFSK1200_STREAM_TONE_MARK		1U
#define AFSK1200_STREAM_SIGNAL_THRESHOLD	32
#define AFSK1200_STREAM_FLAG_BITS		8U

static enum afsk1200_stream_result afsk1200_stream_add_bit(
	struct afsk1200_stream *, uint8_t, struct afsk1200_stream_frame *,
	size_t, size_t *);
static uint32_t afsk1200_stream_confidence(int64_t, int64_t);
static uint8_t afsk1200_stream_decode_tone(const int16_t *, uint32_t *);
static enum afsk1200_stream_result afsk1200_stream_emit_candidate(
	struct afsk1200_stream *, struct afsk1200_stream_frame *, size_t,
	size_t *);
static int64_t afsk1200_stream_goertzel_power(const int16_t *, int32_t);
static enum afsk1200_stream_result afsk1200_stream_handle_sample(
	struct afsk1200_stream *, int16_t, struct afsk1200_stream_frame *,
	size_t, size_t *);
static bool afsk1200_stream_is_flag(struct afsk1200_stream *);
static enum afsk1200_stream_result afsk1200_stream_pack_bits(
	const uint8_t *, size_t, uint8_t *, size_t);
static void afsk1200_stream_reset_rx(struct afsk1200_stream *);
static void afsk1200_stream_update_confidence(struct afsk1200_stream *,
	uint32_t);

enum afsk1200_stream_result
afsk1200_stream_flush(struct afsk1200_stream *stream,
	struct afsk1200_stream_frame *frames, size_t frame_cap,
	size_t *out_frames)
{
	(void)frames;
	(void)frame_cap;

	if (stream == NULL || frames == NULL || out_frames == NULL)
		return AFSK1200_STREAM_ERR_ARG;

	*out_frames = 0U;
	if (stream->state == AFSK1200_STREAM_IN_FRAME &&
	    stream->frame_bits_count != 0U)
		stream->stats.frames_malformed++;
	afsk1200_stream_reset_rx(stream);

	return AFSK1200_STREAM_OK;
}

enum afsk1200_stream_result
afsk1200_stream_init(struct afsk1200_stream *stream)
{
	if (stream == NULL)
		return AFSK1200_STREAM_ERR_ARG;

	(void)memset(stream, 0, sizeof(*stream));
	stream->nrzi_prev = AFSK1200_STREAM_TONE_MARK;
	stream->state = AFSK1200_STREAM_SEARCH_FLAG;

	return AFSK1200_STREAM_OK;
}

enum afsk1200_stream_result
afsk1200_stream_process(struct afsk1200_stream *stream, const int16_t *pcm,
	size_t sample_count, struct afsk1200_stream_frame *frames,
	size_t frame_cap, size_t *out_frames)
{
	enum afsk1200_stream_result res;
	enum afsk1200_stream_result final_res;
	size_t i;

	if (stream == NULL || out_frames == NULL)
		return AFSK1200_STREAM_ERR_ARG;
	*out_frames = 0U;
	if ((pcm == NULL && sample_count != 0U) || frames == NULL)
		return AFSK1200_STREAM_ERR_ARG;

	stream->stats.chunks_processed++;
	final_res = AFSK1200_STREAM_OK;
	for (i = 0U; i < sample_count; i++) {
		stream->stats.samples_seen++;
		res = afsk1200_stream_handle_sample(stream, pcm[i], frames,
		    frame_cap, out_frames);
		if (res == AFSK1200_STREAM_ERR_FRAME_DROPPED)
			final_res = res;
		else if (res != AFSK1200_STREAM_OK)
			return res;
	}

	return final_res;
}

enum afsk1200_stream_result
afsk1200_stream_stats(const struct afsk1200_stream *stream,
	struct afsk1200_stream_stats *stats)
{
	if (stream == NULL || stats == NULL)
		return AFSK1200_STREAM_ERR_ARG;

	*stats = stream->stats;
	return AFSK1200_STREAM_OK;
}

static enum afsk1200_stream_result
afsk1200_stream_add_bit(struct afsk1200_stream *stream, uint8_t bit,
	struct afsk1200_stream_frame *frames, size_t frame_cap,
	size_t *out_frames)
{
	enum afsk1200_stream_result res;

	stream->stats.bits_decoded++;
	stream->flag_shift = (uint8_t)((stream->flag_shift >> 1U) |
	    (bit != 0U ? 0x80U : 0U));
	if (stream->flag_bits < AFSK1200_STREAM_FLAG_BITS)
		stream->flag_bits++;

	if (stream->state == AFSK1200_STREAM_IN_FRAME) {
		if (stream->frame_bits_count >= sizeof(stream->frame_bits)) {
			stream->state = AFSK1200_STREAM_DROP_OVERSIZE;
			stream->stats.frames_seen++;
			stream->stats.frames_too_large++;
		} else {
			stream->frame_bits[stream->frame_bits_count] = bit;
			stream->frame_bits_count++;
		}
	}

	if (!afsk1200_stream_is_flag(stream))
		return AFSK1200_STREAM_OK;

	stream->stats.flags_seen++;
	if (stream->state == AFSK1200_STREAM_IN_FRAME) {
		if (stream->frame_bits_count >= AFSK1200_STREAM_FLAG_BITS)
			stream->frame_bits_count -= AFSK1200_STREAM_FLAG_BITS;
		if (stream->frame_bits_count != 0U) {
			res = afsk1200_stream_emit_candidate(stream, frames,
			    frame_cap, out_frames);
			stream->frame_bits_count = 0U;
			return res;
		}
		stream->frame_bits_count = 0U;
	} else if (stream->state == AFSK1200_STREAM_DROP_OVERSIZE) {
		stream->frame_bits_count = 0U;
	}

	stream->state = AFSK1200_STREAM_IN_FRAME;
	return AFSK1200_STREAM_OK;
}

static uint32_t
afsk1200_stream_confidence(int64_t mark_power, int64_t space_power)
{
	int64_t diff;
	int64_t total;

	diff = mark_power > space_power ? mark_power - space_power :
	    space_power - mark_power;
	total = mark_power + space_power;
	if (total <= 0)
		return 0U;

	diff = (diff * 65535) / total;
	if (diff > 65535)
		return 65535U;

	return (uint32_t)diff;
}

static uint8_t
afsk1200_stream_decode_tone(const int16_t *samples, uint32_t *confidence)
{
	int64_t mark_power;
	int64_t space_power;

	mark_power = afsk1200_stream_goertzel_power(samples,
	    AFSK1200_STREAM_MARK_COEFF);
	space_power = afsk1200_stream_goertzel_power(samples,
	    AFSK1200_STREAM_SPACE_COEFF);
	*confidence = afsk1200_stream_confidence(mark_power, space_power);

	return mark_power >= space_power ? 1U : 0U;
}

static enum afsk1200_stream_result
afsk1200_stream_emit_candidate(struct afsk1200_stream *stream,
	struct afsk1200_stream_frame *frames, size_t frame_cap,
	size_t *out_frames)
{
	uint8_t packed[(AFSK1200_MAX_TEST_BITS + 7U) / 8U];
	uint8_t unstuffed[KILOTNC_AX25_MAX_FRAME];
	struct ax25_frame decoded;
	size_t unstuffed_bits;
	size_t frame_len;
	enum hdlc_result hres;
	enum ax25_result axres;
	enum afsk1200_stream_result res;

	stream->stats.frames_seen++;
	res = afsk1200_stream_pack_bits(stream->frame_bits,
	    stream->frame_bits_count, packed, sizeof(packed));
	if (res != AFSK1200_STREAM_OK) {
		stream->stats.frames_too_large++;
		return AFSK1200_STREAM_OK;
	}

	hres = hdlc_unstuff(packed, stream->frame_bits_count, unstuffed,
	    sizeof(unstuffed), &unstuffed_bits);
	if (hres == HDLC_ERR_SMALL) {
		stream->stats.frames_too_large++;
		return AFSK1200_STREAM_OK;
	}
	if (hres != HDLC_OK || (unstuffed_bits % 8U) != 0U) {
		stream->stats.frames_malformed++;
		return AFSK1200_STREAM_OK;
	}

	frame_len = unstuffed_bits / 8U;
	if (frame_len > KILOTNC_AX25_MAX_FRAME) {
		stream->stats.frames_too_large++;
		return AFSK1200_STREAM_OK;
	}

	axres = ax25_decode_ui_fcs(unstuffed, frame_len, &decoded);
	if (axres == AX25_ERR_BAD_FCS) {
		stream->stats.frames_bad_fcs++;
		return AFSK1200_STREAM_OK;
	}
	if (axres != AX25_OK) {
		stream->stats.frames_malformed++;
		return AFSK1200_STREAM_OK;
	}

	if (*out_frames >= frame_cap) {
		stream->stats.frames_dropped++;
		return AFSK1200_STREAM_ERR_FRAME_DROPPED;
	}

	(void)memcpy(frames[*out_frames].data, unstuffed, frame_len);
	frames[*out_frames].len = frame_len;
	(*out_frames)++;
	stream->stats.frames_ok++;

	return AFSK1200_STREAM_OK;
}

static int64_t
afsk1200_stream_goertzel_power(const int16_t *samples, int32_t coeff)
{
	int64_t q0;
	int64_t q1;
	int64_t q2;
	int64_t mean;
	int64_t sample;
	size_t i;

	mean = 0;
	for (i = 0U; i < AFSK1200_SAMPLES_PER_BIT; i++)
		mean += samples[i];
	mean /= AFSK1200_SAMPLES_PER_BIT;

	q1 = 0;
	q2 = 0;
	for (i = 0U; i < AFSK1200_SAMPLES_PER_BIT; i++) {
		sample = samples[i] - mean;
		q0 = sample + (((int64_t)coeff * q1) /
		    AFSK1200_STREAM_Q) - q2;
		q2 = q1;
		q1 = q0;
	}

	return (q1 * q1) + (q2 * q2) -
	    (((int64_t)coeff * q1 * q2) / AFSK1200_STREAM_Q);
}

static enum afsk1200_stream_result
afsk1200_stream_handle_sample(struct afsk1200_stream *stream, int16_t sample,
	struct afsk1200_stream_frame *frames, size_t frame_cap,
	size_t *out_frames)
{
	uint32_t confidence;
	int32_t mag;
	uint8_t nrzi_bit;
	uint8_t data_bit;

	if (!stream->signal_started) {
		mag = sample;
		if (mag < 0)
			mag = -mag;
		if (mag <= AFSK1200_STREAM_SIGNAL_THRESHOLD) {
			stream->last_sample = sample;
			stream->have_last_sample = true;
			return AFSK1200_STREAM_OK;
		}
		stream->signal_started = true;
		if (stream->have_last_sample) {
			stream->sample_window[0] = stream->last_sample;
			stream->sample_count = 1U;
		}
	}

	stream->sample_window[stream->sample_count] = sample;
	stream->sample_count++;
	if (stream->sample_count < AFSK1200_SAMPLES_PER_BIT)
		return AFSK1200_STREAM_OK;

	nrzi_bit = afsk1200_stream_decode_tone(stream->sample_window,
	    &confidence);
	afsk1200_stream_update_confidence(stream, confidence);
	data_bit = nrzi_bit == stream->nrzi_prev ? 1U : 0U;
	stream->nrzi_prev = nrzi_bit;
	stream->sample_count = 0U;

	return afsk1200_stream_add_bit(stream, data_bit, frames, frame_cap,
	    out_frames);
}

static bool
afsk1200_stream_is_flag(struct afsk1200_stream *stream)
{
	return stream->flag_bits >= AFSK1200_STREAM_FLAG_BITS &&
	    stream->flag_shift == KILOTNC_HDLC_FLAG;
}

static enum afsk1200_stream_result
afsk1200_stream_pack_bits(const uint8_t *bits, size_t bit_count, uint8_t *out,
	size_t out_cap)
{
	size_t i;
	size_t needed;

	needed = (bit_count + 7U) / 8U;
	if (needed > out_cap)
		return AFSK1200_STREAM_ERR_SMALL;

	(void)memset(out, 0, out_cap);
	for (i = 0U; i < bit_count; i++) {
		if (bits[i] != 0U)
			out[i / 8U] |= (uint8_t)(1U << (i % 8U));
	}

	return AFSK1200_STREAM_OK;
}

static void
afsk1200_stream_reset_rx(struct afsk1200_stream *stream)
{
	stream->sample_count = 0U;
	stream->nrzi_prev = AFSK1200_STREAM_TONE_MARK;
	stream->flag_shift = 0U;
	stream->flag_bits = 0U;
	stream->frame_bits_count = 0U;
	stream->signal_started = false;
	stream->have_last_sample = false;
	stream->last_sample = 0;
	stream->state = AFSK1200_STREAM_SEARCH_FLAG;
}

static void
afsk1200_stream_update_confidence(struct afsk1200_stream *stream,
	uint32_t confidence)
{
	uint32_t avg;

	if (UINT32_MAX - stream->confidence_total < confidence)
		stream->confidence_total = UINT32_MAX;
	else
		stream->confidence_total += confidence;
	stream->confidence_count++;
	avg = stream->confidence_total / (uint32_t)stream->confidence_count;
	if (avg > 65535U)
		avg = 65535U;
	stream->stats.confidence_avg = (uint16_t)avg;
	stream->stats.dcd_score = stream->stats.confidence_avg;
}
