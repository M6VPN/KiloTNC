/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/test/test_afsk1200_stream.c */

#include <sys/types.h>

#include <stdint.h>
#include <string.h>

#include "afsk1200.h"
#include "afsk1200_rx.h"
#include "afsk1200_stream.h"
#include "ax25.h"
#include "hdlc.h"

#define CHECK(expr) do { if (!(expr)) return __LINE__; } while (0)
#define TEST_STREAM_BITS_MAX	AFSK1200_MAX_TEST_BITS
#define TEST_STREAM_PCM_MAX \
	(TEST_STREAM_BITS_MAX * AFSK1200_SAMPLES_PER_BIT)

static void test_add_noise(int16_t *, size_t, uint32_t, int32_t);
static int test_append_flag(uint8_t *, size_t, size_t *);
static int test_append_frame_bits(const char *, const char *, const char *,
	uint8_t *, size_t, size_t *);
static int test_append_raw_bits(size_t, uint8_t *, size_t, size_t *);
static int test_build_pcm(const uint8_t *, size_t, size_t, size_t, int16_t *,
	size_t, size_t *);
static int test_compare_whole_buffer(void);
static int test_feed_chunks(const int16_t *, size_t, const size_t *, size_t,
	struct afsk1200_stream_frame *, size_t, size_t *,
	struct afsk1200_stream_stats *);
static int test_feed_fixed(const int16_t *, size_t, size_t,
	struct afsk1200_stream_frame *, size_t, size_t *,
	struct afsk1200_stream_stats *);
static void test_fill_addr(struct ax25_addr *, const char *, uint8_t);
static uint32_t test_next_noise(uint32_t *);
static int test_put_bit(uint8_t *, size_t, size_t *, uint8_t);
static int test_stream_bad_recovery(void);
static int test_stream_boundaries(void);
static int test_stream_chunking(void);
static int test_stream_flags_only(void);
static int test_stream_impairments(void);
static int test_stream_init_stats(void);
static int test_stream_multi(void);
static int test_stream_one(void);

int
test_afsk1200_stream(void)
{
	int subres;

	subres = test_stream_one();
	if (subres != 0)
		return subres;
	subres = test_stream_chunking();
	if (subres != 0)
		return subres;
	subres = test_stream_multi();
	if (subres != 0)
		return subres;
	subres = test_stream_bad_recovery();
	if (subres != 0)
		return subres;
	subres = test_stream_flags_only();
	if (subres != 0)
		return subres;
	subres = test_stream_boundaries();
	if (subres != 0)
		return subres;
	subres = test_stream_init_stats();
	if (subres != 0)
		return subres;
	subres = test_stream_impairments();
	if (subres != 0)
		return subres;
	subres = test_compare_whole_buffer();
	if (subres != 0)
		return subres;

	return 0;
}

static int
test_compare_whole_buffer(void)
{
	uint8_t bits[TEST_STREAM_BITS_MAX];
	int16_t pcm[TEST_STREAM_PCM_MAX];
	struct afsk1200_rx_frame rx_frames[2];
	struct afsk1200_stream_frame stream_frames[2];
	struct afsk1200_rx_stats rx_stats;
	struct afsk1200_stream_stats stream_stats;
	size_t bit_count;
	size_t samples;
	size_t rx_count;
	size_t stream_count;
	enum afsk1200_rx_result rxres;

	bit_count = 0U;
	CHECK(test_append_flag(bits, sizeof(bits), &bit_count) == 0);
	CHECK(test_append_frame_bits("APZKTN", "M6VPN", "compare", bits,
	    sizeof(bits), &bit_count) == 0);
	CHECK(test_append_frame_bits("APZKTN", "N0CALL", "same", bits,
	    sizeof(bits), &bit_count) == 0);
	CHECK(test_build_pcm(bits, bit_count, 0U, 0U, pcm,
	    sizeof(pcm) / sizeof(pcm[0]), &samples) == 0);

	rxres = afsk1200_rx_decode_frames(pcm, samples, rx_frames, 2U,
	    &rx_count, &rx_stats);
	CHECK(rxres == AFSK1200_RX_OK);
	CHECK(test_feed_fixed(pcm, samples, 29U, stream_frames, 2U,
	    &stream_count, &stream_stats) == 0);
	CHECK(stream_count == rx_count);
	CHECK(stream_count == 2U);
	CHECK(stream_frames[0].len == rx_frames[0].len);
	CHECK(stream_frames[1].len == rx_frames[1].len);
	CHECK(memcmp(stream_frames[0].data, rx_frames[0].data,
	    stream_frames[0].len) == 0);
	CHECK(memcmp(stream_frames[1].data, rx_frames[1].data,
	    stream_frames[1].len) == 0);

	return 0;
}

static int
test_stream_bad_recovery(void)
{
	uint8_t bits[TEST_STREAM_BITS_MAX];
	int16_t pcm[TEST_STREAM_PCM_MAX];
	struct afsk1200_stream_frame frames[2];
	struct afsk1200_stream_stats stats;
	size_t bit_count;
	size_t samples;
	size_t count;
	size_t bad_start;
	size_t bad_end;
	size_t i;

	bit_count = 0U;
	CHECK(test_append_flag(bits, sizeof(bits), &bit_count) == 0);
	bad_start = bit_count;
	CHECK(test_append_frame_bits("APZKTN", "M6VPN", "bad", bits,
	    sizeof(bits), &bit_count) == 0);
	bad_end = bit_count;
	for (i = bad_start; i < bad_end; i++) {
		if (bits[i] != 0U) {
			bits[i] = 0U;
			break;
		}
	}
	CHECK(test_append_frame_bits("APZKTN", "N0CALL", "good", bits,
	    sizeof(bits), &bit_count) == 0);
	CHECK(test_build_pcm(bits, bit_count, 0U, 0U, pcm,
	    sizeof(pcm) / sizeof(pcm[0]), &samples) == 0);

	CHECK(test_feed_fixed(pcm, samples, 17U, frames, 2U, &count,
	    &stats) == 0);
	CHECK(count == 1U);
	CHECK(stats.frames_seen == 2U);
	CHECK(stats.frames_bad_fcs == 1U);
	CHECK(stats.frames_ok == 1U);

	return 0;
}

static int
test_stream_boundaries(void)
{
	uint8_t bits[TEST_STREAM_BITS_MAX];
	int16_t pcm[TEST_STREAM_PCM_MAX];
	struct afsk1200_stream stream;
	struct afsk1200_stream_frame frames[2];
	struct afsk1200_stream_stats stats;
	size_t bit_count;
	size_t samples;
	size_t count;
	size_t flushed;
	enum afsk1200_stream_result res;

	CHECK(afsk1200_stream_init(&stream) == AFSK1200_STREAM_OK);
	res = afsk1200_stream_process(NULL, pcm, 1U, frames, 1U, &count);
	CHECK(res == AFSK1200_STREAM_ERR_ARG);
	res = afsk1200_stream_process(&stream, NULL, 1U, frames, 1U, &count);
	CHECK(res == AFSK1200_STREAM_ERR_ARG);
	res = afsk1200_stream_process(&stream, pcm, 1U, NULL, 1U, &count);
	CHECK(res == AFSK1200_STREAM_ERR_ARG);
	res = afsk1200_stream_process(&stream, pcm, 1U, frames, 1U, NULL);
	CHECK(res == AFSK1200_STREAM_ERR_ARG);
	CHECK(afsk1200_stream_stats(NULL, &stats) == AFSK1200_STREAM_ERR_ARG);
	CHECK(afsk1200_stream_stats(&stream, NULL) ==
	    AFSK1200_STREAM_ERR_ARG);
	CHECK(afsk1200_stream_flush(NULL, frames, 1U, &flushed) ==
	    AFSK1200_STREAM_ERR_ARG);
	CHECK(afsk1200_stream_flush(&stream, NULL, 1U, &flushed) ==
	    AFSK1200_STREAM_ERR_ARG);
	CHECK(afsk1200_stream_flush(&stream, frames, 1U, NULL) ==
	    AFSK1200_STREAM_ERR_ARG);

	bit_count = 0U;
	CHECK(test_append_flag(bits, sizeof(bits), &bit_count) == 0);
	CHECK(test_append_frame_bits("APZKTN", "M6VPN", "one", bits,
	    sizeof(bits), &bit_count) == 0);
	CHECK(test_append_frame_bits("APZKTN", "N0CALL", "two", bits,
	    sizeof(bits), &bit_count) == 0);
	CHECK(test_build_pcm(bits, bit_count, 0U, 0U, pcm,
	    sizeof(pcm) / sizeof(pcm[0]), &samples) == 0);
	CHECK(test_feed_fixed(pcm, samples, 23U, frames, 1U, &count,
	    &stats) == AFSK1200_STREAM_ERR_FRAME_DROPPED);
	CHECK(count == 1U);
	CHECK(stats.frames_ok == 1U);
	CHECK(stats.frames_dropped == 1U);

	bit_count = 0U;
	CHECK(test_append_flag(bits, sizeof(bits), &bit_count) == 0);
	CHECK(test_append_raw_bits((KILOTNC_AX25_MAX_FRAME + 1U) * 8U,
	    bits, sizeof(bits), &bit_count) == 0);
	CHECK(test_append_flag(bits, sizeof(bits), &bit_count) == 0);
	CHECK(test_build_pcm(bits, bit_count, 0U, 0U, pcm,
	    sizeof(pcm) / sizeof(pcm[0]), &samples) == 0);
	CHECK(test_feed_fixed(pcm, samples, 31U, frames, 2U, &count,
	    &stats) == 0);
	CHECK(count == 0U);
	CHECK(stats.frames_too_large == 1U);

	return 0;
}

static int
test_stream_chunking(void)
{
	uint8_t bits[TEST_STREAM_BITS_MAX];
	int16_t pcm[TEST_STREAM_PCM_MAX + 80U];
	struct afsk1200_stream_frame frames[1];
	struct afsk1200_stream_stats stats;
	size_t chunks[] = { 3U, 11U, 7U, 41U, 19U, 5U, 83U };
	size_t bit_count;
	size_t samples;
	size_t count;

	bit_count = 0U;
	CHECK(test_append_flag(bits, sizeof(bits), &bit_count) == 0);
	CHECK(test_append_frame_bits("APZKTN", "M6VPN", "chunk", bits,
	    sizeof(bits), &bit_count) == 0);
	CHECK(test_build_pcm(bits, bit_count, 40U, 40U, pcm,
	    sizeof(pcm) / sizeof(pcm[0]), &samples) == 0);

	CHECK(test_feed_fixed(pcm, samples, samples, frames, 1U, &count,
	    &stats) == 0);
	CHECK(count == 1U);
	CHECK(test_feed_fixed(pcm, samples, 1U, frames, 1U, &count,
	    &stats) == 0);
	CHECK(count == 1U);
	CHECK(test_feed_fixed(pcm, samples, 13U, frames, 1U, &count,
	    &stats) == 0);
	CHECK(count == 1U);
	CHECK(test_feed_chunks(pcm, samples, chunks,
	    sizeof(chunks) / sizeof(chunks[0]), frames, 1U, &count,
	    &stats) == 0);
	CHECK(count == 1U);

	return 0;
}

static int
test_stream_flags_only(void)
{
	uint8_t bits[TEST_STREAM_BITS_MAX];
	int16_t pcm[TEST_STREAM_PCM_MAX];
	struct afsk1200_stream stream;
	struct afsk1200_stream_frame frames[1];
	struct afsk1200_stream_stats stats;
	size_t bit_count;
	size_t samples;
	size_t count;
	size_t flushed;
	size_t i;

	bit_count = 0U;
	for (i = 0U; i < 10U; i++)
		CHECK(test_append_flag(bits, sizeof(bits), &bit_count) == 0);
	CHECK(test_build_pcm(bits, bit_count, 0U, 0U, pcm,
	    sizeof(pcm) / sizeof(pcm[0]), &samples) == 0);
	CHECK(test_feed_fixed(pcm, samples, 9U, frames, 1U, &count,
	    &stats) == 0);
	CHECK(count == 0U);
	CHECK(stats.flags_seen == 10U);
	CHECK(stats.frames_seen == 0U);

	bit_count = 0U;
	CHECK(test_append_flag(bits, sizeof(bits), &bit_count) == 0);
	CHECK(test_append_frame_bits("APZKTN", "M6VPN", "partial", bits,
	    sizeof(bits), &bit_count) == 0);
	bit_count -= 8U;
	CHECK(test_build_pcm(bits, bit_count, 0U, 0U, pcm,
	    sizeof(pcm) / sizeof(pcm[0]), &samples) == 0);
	CHECK(afsk1200_stream_init(&stream) == AFSK1200_STREAM_OK);
	CHECK(afsk1200_stream_process(&stream, pcm, samples, frames, 1U,
	    &count) == AFSK1200_STREAM_OK);
	CHECK(count == 0U);
	CHECK(afsk1200_stream_flush(&stream, frames, 1U, &flushed) ==
	    AFSK1200_STREAM_OK);
	CHECK(flushed == 0U);
	CHECK(afsk1200_stream_stats(&stream, &stats) ==
	    AFSK1200_STREAM_OK);
	CHECK(stats.frames_malformed == 1U);

	CHECK(afsk1200_stream_init(&stream) == AFSK1200_STREAM_OK);
	CHECK(afsk1200_stream_process(&stream, pcm, 17U, frames, 1U,
	    &count) == AFSK1200_STREAM_OK);
	CHECK(afsk1200_stream_flush(&stream, frames, 1U, &flushed) ==
	    AFSK1200_STREAM_OK);
	CHECK(flushed == 0U);

	return 0;
}

static int
test_stream_impairments(void)
{
	uint8_t bits[TEST_STREAM_BITS_MAX];
	int16_t pcm[TEST_STREAM_PCM_MAX + 80U];
	struct afsk1200_stream_frame frames[1];
	struct afsk1200_stream_stats stats;
	size_t bit_count;
	size_t samples;
	size_t count;
	size_t i;

	bit_count = 0U;
	CHECK(test_append_flag(bits, sizeof(bits), &bit_count) == 0);
	CHECK(test_append_frame_bits("APZKTN", "M6VPN", "noise", bits,
	    sizeof(bits), &bit_count) == 0);
	CHECK(test_build_pcm(bits, bit_count, 40U, 40U, pcm,
	    sizeof(pcm) / sizeof(pcm[0]), &samples) == 0);
	for (i = 40U; i < samples - 40U; i++) {
		pcm[i] = (int16_t)(((int32_t)pcm[i] * 3) / 4);
		pcm[i] = (int16_t)(pcm[i] + 200);
	}
	test_add_noise(&pcm[40U], samples - 80U, 0xA5A55A5AU, 120);

	CHECK(test_feed_fixed(pcm, samples, 17U, frames, 1U, &count,
	    &stats) == 0);
	CHECK(count == 1U);
	CHECK(stats.frames_ok == 1U);

	return 0;
}

static int
test_stream_init_stats(void)
{
	uint8_t bits[TEST_STREAM_BITS_MAX];
	int16_t pcm[TEST_STREAM_PCM_MAX];
	struct afsk1200_stream stream;
	struct afsk1200_stream_frame frames[1];
	struct afsk1200_stream_stats stats;
	size_t bit_count;
	size_t samples;
	size_t count;

	bit_count = 0U;
	CHECK(test_append_flag(bits, sizeof(bits), &bit_count) == 0);
	CHECK(test_append_frame_bits("APZKTN", "M6VPN", "reset", bits,
	    sizeof(bits), &bit_count) == 0);
	CHECK(test_build_pcm(bits, bit_count, 0U, 0U, pcm,
	    sizeof(pcm) / sizeof(pcm[0]), &samples) == 0);

	CHECK(afsk1200_stream_init(&stream) == AFSK1200_STREAM_OK);
	CHECK(afsk1200_stream_process(&stream, pcm, samples, frames, 1U,
	    &count) == AFSK1200_STREAM_OK);
	CHECK(count == 1U);
	CHECK(afsk1200_stream_stats(&stream, &stats) ==
	    AFSK1200_STREAM_OK);
	CHECK(stats.samples_seen == samples);
	CHECK(stats.bits_decoded == bit_count);
	CHECK(stats.flags_seen == 2U);
	CHECK(stats.frames_seen == 1U);
	CHECK(stats.frames_ok == 1U);
	CHECK(stats.chunks_processed == 1U);
	CHECK(stats.dcd_score != 0U);
	CHECK(stats.confidence_avg != 0U);

	CHECK(afsk1200_stream_init(&stream) == AFSK1200_STREAM_OK);
	CHECK(afsk1200_stream_stats(&stream, &stats) ==
	    AFSK1200_STREAM_OK);
	CHECK(stats.samples_seen == 0U);
	CHECK(stats.bits_decoded == 0U);
	CHECK(stats.frames_ok == 0U);

	return 0;
}

static int
test_stream_multi(void)
{
	uint8_t bits[TEST_STREAM_BITS_MAX];
	int16_t pcm[TEST_STREAM_PCM_MAX];
	struct afsk1200_stream_frame frames[2];
	struct afsk1200_stream_stats stats;
	size_t chunks[] = { 37U, 2U, 59U, 5U, 101U, 11U };
	size_t bit_count;
	size_t samples;
	size_t count;

	bit_count = 0U;
	CHECK(test_append_flag(bits, sizeof(bits), &bit_count) == 0);
	CHECK(test_append_frame_bits("APZKTN", "M6VPN", "one", bits,
	    sizeof(bits), &bit_count) == 0);
	CHECK(test_append_frame_bits("APZKTN", "N0CALL", "two", bits,
	    sizeof(bits), &bit_count) == 0);
	CHECK(test_build_pcm(bits, bit_count, 0U, 0U, pcm,
	    sizeof(pcm) / sizeof(pcm[0]), &samples) == 0);

	CHECK(test_feed_chunks(pcm, samples, chunks,
	    sizeof(chunks) / sizeof(chunks[0]), frames, 2U, &count,
	    &stats) == 0);
	CHECK(count == 2U);
	CHECK(stats.frames_seen == 2U);
	CHECK(stats.frames_ok == 2U);
	CHECK(stats.frames_bad_fcs == 0U);

	return 0;
}

static int
test_stream_one(void)
{
	uint8_t bits[TEST_STREAM_BITS_MAX];
	int16_t pcm[TEST_STREAM_PCM_MAX];
	struct afsk1200_stream_frame frames[1];
	struct afsk1200_stream_stats stats;
	struct ax25_frame decoded;
	size_t bit_count;
	size_t samples;
	size_t count;
	enum ax25_result axres;

	bit_count = 0U;
	CHECK(test_append_flag(bits, sizeof(bits), &bit_count) == 0);
	CHECK(test_append_frame_bits("APZKTN", "M6VPN", "stream", bits,
	    sizeof(bits), &bit_count) == 0);
	CHECK(test_build_pcm(bits, bit_count, 0U, 0U, pcm,
	    sizeof(pcm) / sizeof(pcm[0]), &samples) == 0);

	CHECK(test_feed_fixed(pcm, samples, samples, frames, 1U, &count,
	    &stats) == 0);
	CHECK(count == 1U);
	axres = ax25_decode_ui_fcs(frames[0].data, frames[0].len, &decoded);
	CHECK(axres == AX25_OK);
	CHECK(strcmp(decoded.src.callsign, "M6VPN") == 0);
	CHECK(decoded.info_len == 6U);
	CHECK(memcmp(decoded.info, "stream", 6U) == 0);

	return 0;
}

static void
test_add_noise(int16_t *pcm, size_t len, uint32_t seed, int32_t amplitude)
{
	size_t i;
	int32_t span;
	int32_t noise;
	int32_t sample;

	span = (amplitude * 2) + 1;
	for (i = 0U; i < len; i++) {
		noise = (int32_t)(test_next_noise(&seed) % (uint32_t)span) -
		    amplitude;
		sample = (int32_t)pcm[i] + noise;
		if (sample > 32767)
			sample = 32767;
		if (sample < -32768)
			sample = -32768;
		pcm[i] = (int16_t)sample;
	}
}

static int
test_append_flag(uint8_t *bits, size_t cap, size_t *pos)
{
	size_t i;

	for (i = 0U; i < 8U; i++) {
		if (test_put_bit(bits, cap, pos,
		    (uint8_t)((KILOTNC_HDLC_FLAG >> i) & 1U)) != 0)
			return __LINE__;
	}

	return 0;
}

static int
test_append_frame_bits(const char *dst, const char *src, const char *info,
	uint8_t *bits, size_t cap, size_t *pos)
{
	uint8_t raw[KILOTNC_AX25_MAX_FRAME];
	uint8_t stuffed[(AFSK1200_MAX_TEST_BITS + 7U) / 8U];
	struct ax25_frame frame;
	size_t raw_len;
	size_t stuffed_bits;
	size_t i;
	uint32_t byte;
	enum ax25_result axres;
	enum hdlc_result hres;

	(void)memset(&frame, 0, sizeof(frame));
	test_fill_addr(&frame.dst, dst, 0U);
	test_fill_addr(&frame.src, src, 0U);
	frame.pid = AX25_PID_NONE;
	frame.info_len = strlen(info);
	(void)memcpy(frame.info, info, frame.info_len);

	axres = ax25_encode_ui_fcs(&frame, raw, sizeof(raw), &raw_len);
	if (axres != AX25_OK)
		return __LINE__;
	hres = hdlc_bitstuff(raw, raw_len * 8U, stuffed, sizeof(stuffed),
	    &stuffed_bits);
	if (hres != HDLC_OK)
		return __LINE__;

	for (i = 0U; i < stuffed_bits; i++) {
		byte = stuffed[i / 8U];
		if (test_put_bit(bits, cap, pos,
		    (uint8_t)((byte >> (i % 8U)) & 1U)) != 0)
			return __LINE__;
	}
	if (test_append_flag(bits, cap, pos) != 0)
		return __LINE__;

	return 0;
}

static int
test_append_raw_bits(size_t bit_count, uint8_t *dst, size_t cap, size_t *pos)
{
	size_t i;

	for (i = 0U; i < bit_count; i++) {
		if (test_put_bit(dst, cap, pos, 0U) != 0)
			return __LINE__;
	}

	return 0;
}

static int
test_build_pcm(const uint8_t *bits, size_t bit_count, size_t lead,
	size_t trail, int16_t *pcm, size_t pcm_cap, size_t *out_samples)
{
	uint8_t nrzi[TEST_STREAM_BITS_MAX];
	int16_t local_pcm[TEST_STREAM_PCM_MAX];
	size_t nrzi_bits;
	size_t samples;
	size_t i;
	enum afsk1200_result res;

	res = afsk1200_nrzi_encode(bits, bit_count, nrzi, sizeof(nrzi),
	    &nrzi_bits);
	if (res != AFSK1200_OK)
		return __LINE__;
	res = afsk1200_encode_pcm(nrzi, nrzi_bits, local_pcm,
	    sizeof(local_pcm) / sizeof(local_pcm[0]), &samples);
	if (res != AFSK1200_OK)
		return __LINE__;
	if (lead + samples + trail > pcm_cap)
		return __LINE__;

	for (i = 0U; i < lead; i++)
		pcm[i] = 0;
	(void)memcpy(&pcm[lead], local_pcm, samples * sizeof(local_pcm[0]));
	for (i = 0U; i < trail; i++)
		pcm[lead + samples + i] = 0;
	*out_samples = lead + samples + trail;

	return 0;
}

static int
test_feed_chunks(const int16_t *pcm, size_t samples, const size_t *chunks,
	size_t chunk_count, struct afsk1200_stream_frame *frames,
	size_t frame_cap, size_t *out_count, struct afsk1200_stream_stats *stats)
{
	struct afsk1200_stream stream;
	size_t chunk;
	size_t emitted;
	size_t flushed;
	size_t off;
	size_t take;
	enum afsk1200_stream_result res;
	enum afsk1200_stream_result final_res;

	CHECK(afsk1200_stream_init(&stream) == AFSK1200_STREAM_OK);
	*out_count = 0U;
	off = 0U;
	chunk = 0U;
	final_res = AFSK1200_STREAM_OK;
	while (off < samples) {
		take = chunks[chunk % chunk_count];
		if (take > samples - off)
			take = samples - off;
		res = afsk1200_stream_process(&stream, &pcm[off], take,
		    *out_count < frame_cap ? &frames[*out_count] : frames,
		    *out_count < frame_cap ? frame_cap - *out_count : 0U,
		    &emitted);
		if (res == AFSK1200_STREAM_ERR_FRAME_DROPPED)
			final_res = res;
		else if (res != AFSK1200_STREAM_OK)
			return __LINE__;
		if (*out_count + emitted > frame_cap)
			return __LINE__;
		*out_count += emitted;
		off += take;
		chunk++;
	}
	res = afsk1200_stream_flush(&stream, frames, frame_cap, &flushed);
	if (res != AFSK1200_STREAM_OK)
		return __LINE__;
	if (flushed != 0U)
		return __LINE__;
	CHECK(afsk1200_stream_stats(&stream, stats) == AFSK1200_STREAM_OK);
	if (final_res == AFSK1200_STREAM_ERR_FRAME_DROPPED)
		return AFSK1200_STREAM_ERR_FRAME_DROPPED;

	return 0;
}

static int
test_feed_fixed(const int16_t *pcm, size_t samples, size_t chunk,
	struct afsk1200_stream_frame *frames, size_t frame_cap,
	size_t *out_count, struct afsk1200_stream_stats *stats)
{
	size_t chunks[1];

	chunks[0] = chunk;
	return test_feed_chunks(pcm, samples, chunks, 1U, frames, frame_cap,
	    out_count, stats);
}

static void
test_fill_addr(struct ax25_addr *addr, const char *callsign, uint8_t ssid)
{
	(void)memset(addr, 0, sizeof(*addr));
	(void)memcpy(addr->callsign, callsign, strlen(callsign) + 1U);
	addr->ssid = ssid;
	addr->repeated = 0;
}

static uint32_t
test_next_noise(uint32_t *state)
{
	uint32_t x;

	x = *state;
	x ^= x << 13U;
	x ^= x >> 17U;
	x ^= x << 5U;
	*state = x;

	return x;
}

static int
test_put_bit(uint8_t *bits, size_t cap, size_t *pos, uint8_t bit)
{
	if (*pos >= cap)
		return __LINE__;
	bits[*pos] = bit == 0U ? (uint8_t)0U : (uint8_t)1U;
	(*pos)++;

	return 0;
}
