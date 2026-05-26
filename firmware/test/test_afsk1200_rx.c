/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/test/test_afsk1200_rx.c */

#include <sys/types.h>

#include <stdint.h>
#include <string.h>

#include "afsk1200.h"
#include "afsk1200_rx.h"
#include "ax25.h"
#include "hdlc.h"

#define CHECK(expr) do { if (!(expr)) return __LINE__; } while (0)
#define TEST_RX_BITS_MAX	AFSK1200_MAX_TEST_BITS
#define TEST_RX_PCM_MAX		(TEST_RX_BITS_MAX * AFSK1200_SAMPLES_PER_BIT)

static void test_add_noise(int16_t *, size_t, uint32_t, int32_t);
static int test_append_flag(uint8_t *, size_t, size_t *);
static int test_append_frame_bits(const char *, const char *, const char *,
	uint8_t *, size_t, size_t *);
static int test_append_raw_bits(const uint8_t *, size_t, uint8_t *, size_t,
	size_t *);
static int test_build_pcm(const uint8_t *, size_t, size_t, size_t, int16_t *,
	size_t, size_t *);
static int test_decode_bad_recovery(void);
static int test_decode_boundaries(void);
static int test_decode_flags_only(void);
static int test_decode_impairments(void);
static int test_decode_multi(void);
static int test_decode_one(void);
static void test_fill_addr(struct ax25_addr *, const char *, uint8_t);
static uint32_t test_next_noise(uint32_t *);
static int test_put_bit(uint8_t *, size_t, size_t *, uint8_t);

int
test_afsk1200_rx(void)
{
	int subres;

	subres = test_decode_one();
	if (subres != 0)
		return subres;
	subres = test_decode_multi();
	if (subres != 0)
		return subres;
	subres = test_decode_bad_recovery();
	if (subres != 0)
		return subres;
	subres = test_decode_flags_only();
	if (subres != 0)
		return subres;
	subres = test_decode_boundaries();
	if (subres != 0)
		return subres;
	subres = test_decode_impairments();
	if (subres != 0)
		return subres;

	return 0;
}

static int
test_decode_bad_recovery(void)
{
	uint8_t bits[TEST_RX_BITS_MAX];
	int16_t pcm[TEST_RX_PCM_MAX];
	struct afsk1200_rx_frame frames[2];
	struct afsk1200_rx_stats stats;
	size_t bit_count;
	size_t samples;
	size_t out_count;
	size_t bad_start;
	size_t bad_end;
	size_t i;
	enum afsk1200_rx_result res;

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
	res = afsk1200_rx_decode_frames(pcm, samples, frames, 2U, &out_count,
	    &stats);
	CHECK(res == AFSK1200_RX_OK);
	CHECK(out_count == 1U);
	CHECK(stats.frames_seen == 2U);
	CHECK(stats.frames_bad_fcs == 1U);
	CHECK(stats.frames_ok == 1U);

	return 0;
}

static int
test_decode_boundaries(void)
{
	uint8_t bits[TEST_RX_BITS_MAX];
	int16_t pcm[TEST_RX_PCM_MAX];
	int16_t noise[AFSK1200_SAMPLES_PER_BIT * 24U];
	struct afsk1200_rx_frame frames[2];
	struct afsk1200_rx_stats stats;
	size_t bit_count;
	size_t samples;
	size_t out_count;
	enum afsk1200_rx_result res;

	bit_count = 0U;
	CHECK(test_append_flag(bits, sizeof(bits), &bit_count) == 0);
	CHECK(test_append_frame_bits("APZKTN", "M6VPN", "one", bits,
	    sizeof(bits), &bit_count) == 0);
	CHECK(test_append_frame_bits("APZKTN", "N0CALL", "two", bits,
	    sizeof(bits), &bit_count) == 0);
	CHECK(test_build_pcm(bits, bit_count, 0U, 0U, pcm,
	    sizeof(pcm) / sizeof(pcm[0]), &samples) == 0);

	res = afsk1200_rx_decode_frames(pcm, samples, frames, 1U, &out_count,
	    &stats);
	CHECK(res == AFSK1200_RX_ERR_SMALL);
	CHECK(out_count == 1U);
	CHECK(stats.frames_ok == 1U);

	res = afsk1200_rx_decode_frames(NULL, samples, frames, 1U, &out_count,
	    &stats);
	CHECK(res == AFSK1200_RX_ERR_ARG);
	res = afsk1200_rx_decode_frames(pcm, samples, NULL, 1U, &out_count,
	    &stats);
	CHECK(res == AFSK1200_RX_ERR_ARG);
	res = afsk1200_rx_decode_frames(pcm, samples, frames, 1U, NULL,
	    &stats);
	CHECK(res == AFSK1200_RX_ERR_ARG);
	res = afsk1200_rx_decode_frames(pcm, samples, frames, 1U, &out_count,
	    NULL);
	CHECK(res == AFSK1200_RX_ERR_SMALL);

	(void)memset(noise, 0, sizeof(noise));
	test_add_noise(noise, sizeof(noise) / sizeof(noise[0]), 0x13572468U,
	    600);
	res = afsk1200_rx_decode_frames(noise,
	    sizeof(noise) / sizeof(noise[0]), frames, 2U, &out_count,
	    &stats);
	CHECK(res == AFSK1200_RX_ERR_NO_FRAME ||
	    res == AFSK1200_RX_ERR_PARTIAL);

	bit_count = 0U;
	CHECK(test_append_flag(bits, sizeof(bits), &bit_count) == 0);
	CHECK(test_append_raw_bits(bits, (KILOTNC_AX25_MAX_FRAME + 1U) * 8U,
	    bits, sizeof(bits), &bit_count) == 0);
	CHECK(test_append_flag(bits, sizeof(bits), &bit_count) == 0);
	CHECK(test_build_pcm(bits, bit_count, 0U, 0U, pcm,
	    sizeof(pcm) / sizeof(pcm[0]), &samples) == 0);
	res = afsk1200_rx_decode_frames(pcm, samples, frames, 2U, &out_count,
	    &stats);
	CHECK(res == AFSK1200_RX_ERR_NO_FRAME);
	CHECK(stats.frames_too_large == 1U);

	return 0;
}

static int
test_decode_flags_only(void)
{
	uint8_t bits[TEST_RX_BITS_MAX];
	int16_t pcm[TEST_RX_PCM_MAX];
	struct afsk1200_rx_frame frames[1];
	struct afsk1200_rx_stats stats;
	size_t bit_count;
	size_t samples;
	size_t out_count;
	size_t i;
	enum afsk1200_rx_result res;

	bit_count = 0U;
	for (i = 0U; i < 8U; i++)
		CHECK(test_append_flag(bits, sizeof(bits), &bit_count) == 0);
	CHECK(test_build_pcm(bits, bit_count, 0U, 0U, pcm,
	    sizeof(pcm) / sizeof(pcm[0]), &samples) == 0);

	res = afsk1200_rx_decode_frames(pcm, samples, frames, 1U, &out_count,
	    &stats);
	CHECK(res == AFSK1200_RX_ERR_NO_FRAME);
	CHECK(out_count == 0U);
	CHECK(stats.flags_seen == 8U);
	CHECK(stats.frames_seen == 0U);

	bit_count = 0U;
	CHECK(test_append_flag(bits, sizeof(bits), &bit_count) == 0);
	CHECK(test_append_frame_bits("APZKTN", "M6VPN", "partial", bits,
	    sizeof(bits), &bit_count) == 0);
	bit_count -= 8U;
	CHECK(test_build_pcm(bits, bit_count, 0U, 0U, pcm,
	    sizeof(pcm) / sizeof(pcm[0]), &samples) == 0);
	res = afsk1200_rx_decode_frames(pcm, samples, frames, 1U, &out_count,
	    &stats);
	CHECK(res == AFSK1200_RX_ERR_PARTIAL);
	CHECK(out_count == 0U);

	return 0;
}

static int
test_decode_impairments(void)
{
	uint8_t bits[TEST_RX_BITS_MAX];
	int16_t pcm[TEST_RX_PCM_MAX + 53U];
	struct afsk1200_rx_frame frames[1];
	struct afsk1200_rx_stats stats;
	size_t bit_count;
	size_t samples;
	size_t out_count;
	size_t i;
	enum afsk1200_rx_result res;

	bit_count = 0U;
	CHECK(test_append_flag(bits, sizeof(bits), &bit_count) == 0);
	CHECK(test_append_flag(bits, sizeof(bits), &bit_count) == 0);
	CHECK(test_append_frame_bits("APZKTN", "M6VPN", "impaired", bits,
	    sizeof(bits), &bit_count) == 0);
	CHECK(test_append_flag(bits, sizeof(bits), &bit_count) == 0);
	CHECK(test_build_pcm(bits, bit_count, 13U, 40U, pcm,
	    sizeof(pcm) / sizeof(pcm[0]), &samples) == 0);

	for (i = 13U; i < samples - 40U; i++) {
		pcm[i] = (int16_t)(((int32_t)pcm[i] * 3) / 4);
		pcm[i] = (int16_t)(pcm[i] + 250);
	}
	test_add_noise(&pcm[13U], samples - 53U, 0x2468ACE0U, 150);

	res = afsk1200_rx_decode_frames(pcm, samples, frames, 1U, &out_count,
	    &stats);
	CHECK(res == AFSK1200_RX_OK);
	CHECK(out_count == 1U);
	CHECK(stats.frames_ok == 1U);
	CHECK(stats.flags_seen >= 3U);

	return 0;
}

static int
test_decode_multi(void)
{
	uint8_t bits[TEST_RX_BITS_MAX];
	int16_t pcm[TEST_RX_PCM_MAX];
	struct afsk1200_rx_frame frames[2];
	struct afsk1200_rx_stats stats;
	size_t bit_count;
	size_t samples;
	size_t out_count;
	enum afsk1200_rx_result res;

	bit_count = 0U;
	CHECK(test_append_flag(bits, sizeof(bits), &bit_count) == 0);
	CHECK(test_append_frame_bits("APZKTN", "M6VPN", "one", bits,
	    sizeof(bits), &bit_count) == 0);
	CHECK(test_append_frame_bits("APZKTN", "N0CALL", "two", bits,
	    sizeof(bits), &bit_count) == 0);

	CHECK(test_build_pcm(bits, bit_count, 0U, 0U, pcm,
	    sizeof(pcm) / sizeof(pcm[0]), &samples) == 0);
	res = afsk1200_rx_decode_frames(pcm, samples, frames, 2U, &out_count,
	    &stats);
	CHECK(res == AFSK1200_RX_OK);
	CHECK(out_count == 2U);
	CHECK(stats.frames_seen == 2U);
	CHECK(stats.frames_ok == 2U);
	CHECK(stats.frames_bad_fcs == 0U);

	return 0;
}

static int
test_decode_one(void)
{
	uint8_t bits[TEST_RX_BITS_MAX];
	int16_t pcm[TEST_RX_PCM_MAX + 160U];
	struct afsk1200_rx_frame frames[1];
	struct afsk1200_rx_stats stats;
	size_t bit_count;
	size_t samples;
	size_t out_count;
	enum ax25_result axres;
	enum afsk1200_rx_result res;
	struct ax25_frame decoded;

	bit_count = 0U;
	CHECK(test_append_flag(bits, sizeof(bits), &bit_count) == 0);
	CHECK(test_append_flag(bits, sizeof(bits), &bit_count) == 0);
	CHECK(test_append_frame_bits("APZKTN", "M6VPN", "KiloTNC", bits,
	    sizeof(bits), &bit_count) == 0);
	CHECK(test_append_flag(bits, sizeof(bits), &bit_count) == 0);
	CHECK(test_append_flag(bits, sizeof(bits), &bit_count) == 0);

	CHECK(test_build_pcm(bits, bit_count, 40U, 80U, pcm,
	    sizeof(pcm) / sizeof(pcm[0]), &samples) == 0);
	res = afsk1200_rx_decode_frames(pcm, samples, frames, 1U, &out_count,
	    &stats);
	CHECK(res == AFSK1200_RX_OK);
	CHECK(out_count == 1U);
	CHECK(stats.bits_decoded == bit_count);
	CHECK(stats.flags_seen == 5U);
	CHECK(stats.frames_seen == 1U);
	CHECK(stats.frames_ok == 1U);
	CHECK(stats.dcd_score != 0U);
	CHECK(stats.confidence_avg != 0U);

	axres = ax25_decode_ui_fcs(frames[0].data, frames[0].len, &decoded);
	CHECK(axres == AX25_OK);
	CHECK(strcmp(decoded.dst.callsign, "APZKTN") == 0);
	CHECK(strcmp(decoded.src.callsign, "M6VPN") == 0);
	CHECK(decoded.info_len == 7U);
	CHECK(memcmp(decoded.info, "KiloTNC", 7U) == 0);

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
test_append_raw_bits(const uint8_t *src, size_t bit_count, uint8_t *dst,
	size_t cap, size_t *pos)
{
	size_t i;

	(void)src;
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
	uint8_t nrzi[TEST_RX_BITS_MAX];
	int16_t local_pcm[TEST_RX_PCM_MAX];
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
