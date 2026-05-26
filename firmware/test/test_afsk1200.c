/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/test/test_afsk1200.c */

#include <sys/types.h>

#include <stdint.h>
#include <string.h>

#include "afsk1200.h"
#include "ax25.h"
#include "hdlc.h"

#define CHECK(expr) do { if (!(expr)) return __LINE__; } while (0)
#define TEST_BITS_MAX	(AFSK1200_MAX_TEST_BITS + 16U)
#define TEST_PCM_MAX	(TEST_BITS_MAX * AFSK1200_SAMPLES_PER_BIT)

static void test_add_dc(int16_t *, size_t, int32_t);
static void test_add_noise(int16_t *, size_t, uint32_t, int32_t);
static int test_afsk1200_full_chain(void);
static int test_afsk1200_impairments(void);
static int test_afsk1200_null_args(void);
static int test_afsk1200_pcm(void);
static void test_clip(int16_t *, size_t, int32_t, int32_t);
static void test_copy_with_silence(const int16_t *, size_t, int16_t *,
	size_t, size_t, size_t *);
static uint32_t test_next_noise(uint32_t *);
static int test_afsk1200_put_bit(uint8_t *, size_t, size_t *, uint8_t);
static int test_afsk1200_unpack_bits(const uint8_t *, size_t, uint8_t *,
	size_t);
static int test_afsk1200_pack_bits(const uint8_t *, size_t, uint8_t *,
	size_t);
static void test_scale(int16_t *, size_t, int32_t, int32_t);
static int16_t test_saturate(int32_t);
static void test_fill_addr(struct ax25_addr *, const char *, uint8_t);

int
test_afsk1200(void)
{
	uint8_t bits[] = { 1U, 0U, 0U, 1U, 1U, 0U };
	uint8_t expected[] = { 1U, 0U, 1U, 1U, 1U, 0U };
	uint8_t nrzi[16];
	uint8_t decoded[16];
	size_t count;
	int subres;
	enum afsk1200_result res;

	CHECK(AFSK1200_SAMPLE_RATE == 48000U);
	CHECK(AFSK1200_BAUD == 1200U);
	CHECK(AFSK1200_SAMPLES_PER_BIT == 40U);
	CHECK(AFSK1200_MARK_HZ == 1200U);
	CHECK(AFSK1200_SPACE_HZ == 2200U);

	res = afsk1200_samples_for_bits(6U, &count);
	CHECK(res == AFSK1200_OK);
	CHECK(count == 240U);
	res = afsk1200_samples_for_bits(6U, NULL);
	CHECK(res == AFSK1200_ERR_ARG);

	res = afsk1200_nrzi_encode(bits, 6U, nrzi, sizeof(nrzi), &count);
	CHECK(res == AFSK1200_OK);
	CHECK(count == 6U);
	CHECK(memcmp(nrzi, expected, 6U) == 0);
	res = afsk1200_nrzi_decode(nrzi, count, decoded, sizeof(decoded),
	    &count);
	CHECK(res == AFSK1200_OK);
	CHECK(count == 6U);
	CHECK(memcmp(decoded, bits, 6U) == 0);

	subres = test_afsk1200_pcm();
	if (subres != 0)
		return subres;
	subres = test_afsk1200_full_chain();
	if (subres != 0)
		return subres;
	subres = test_afsk1200_impairments();
	if (subres != 0)
		return subres;
	subres = test_afsk1200_null_args();
	if (subres != 0)
		return subres;

	return 0;
}

static int
test_afsk1200_full_chain(void)
{
	static int16_t pcm[TEST_PCM_MAX];
	static uint8_t packed[KILOTNC_AX25_MAX_FRAME];
	static uint8_t stuffed[(AFSK1200_MAX_TEST_BITS + 7U) / 8U];
	static uint8_t flagged[(TEST_BITS_MAX + 7U) / 8U];
	static uint8_t unpacked[TEST_BITS_MAX];
	static uint8_t nrzi[TEST_BITS_MAX];
	static uint8_t tone_bits[TEST_BITS_MAX];
	static uint8_t decoded_bits[TEST_BITS_MAX];
	static uint8_t stripped[TEST_BITS_MAX];
	static uint8_t stripped_packed[(TEST_BITS_MAX + 7U) / 8U];
	static uint8_t unstuffed[KILOTNC_AX25_MAX_FRAME];
	struct ax25_frame frame;
	struct ax25_frame decoded;
	size_t packed_len;
	size_t stuffed_bits;
	size_t flagged_bits;
	size_t unpacked_bits;
	size_t nrzi_bits;
	size_t pcm_samples;
	size_t tone_count;
	size_t decoded_count;
	size_t stripped_bits;
	size_t unstuffed_bits;
	size_t i;
	uint32_t byte;
	enum ax25_result axres;
	enum hdlc_result hres;
	enum afsk1200_result ares;

	(void)memset(&frame, 0, sizeof(frame));
	test_fill_addr(&frame.dst, "APZKTN", 0U);
	test_fill_addr(&frame.src, "M6VPN", 0U);
	frame.pid = AX25_PID_NONE;
	(void)memcpy(frame.info, "KiloTNC test", 12U);
	frame.info_len = 12U;

	axres = ax25_encode_ui_fcs(&frame, packed, sizeof(packed), &packed_len);
	CHECK(axres == AX25_OK);

	hres = hdlc_bitstuff(packed, packed_len * 8U, stuffed,
	    sizeof(stuffed), &stuffed_bits);
	CHECK(hres == HDLC_OK);

	(void)memset(flagged, 0, sizeof(flagged));
	flagged_bits = 0U;
	for (i = 0U; i < 8U; i++)
		CHECK(test_afsk1200_put_bit(flagged, sizeof(flagged),
		    &flagged_bits, (uint8_t)((KILOTNC_HDLC_FLAG >> i) & 1U)) == 0);
	for (i = 0U; i < stuffed_bits; i++) {
		byte = stuffed[i / 8U];
		CHECK(test_afsk1200_put_bit(flagged, sizeof(flagged),
		    &flagged_bits, (uint8_t)((byte >> (i % 8U)) & 1U)) == 0);
	}
	for (i = 0U; i < 8U; i++)
		CHECK(test_afsk1200_put_bit(flagged, sizeof(flagged),
		    &flagged_bits, (uint8_t)((KILOTNC_HDLC_FLAG >> i) & 1U)) == 0);

	CHECK(test_afsk1200_unpack_bits(flagged, flagged_bits, unpacked,
	    sizeof(unpacked)) == 0);
	unpacked_bits = flagged_bits;
	ares = afsk1200_nrzi_encode(unpacked, unpacked_bits, nrzi,
	    sizeof(nrzi), &nrzi_bits);
	CHECK(ares == AFSK1200_OK);
	ares = afsk1200_encode_pcm(nrzi, nrzi_bits, pcm, sizeof(pcm) /
	    sizeof(pcm[0]), &pcm_samples);
	CHECK(ares == AFSK1200_OK);
	ares = afsk1200_decode_pcm(pcm, pcm_samples, tone_bits,
	    sizeof(tone_bits), &tone_count);
	CHECK(ares == AFSK1200_OK);
	CHECK(tone_count == nrzi_bits);
	ares = afsk1200_nrzi_decode(tone_bits, tone_count, decoded_bits,
	    sizeof(decoded_bits), &decoded_count);
	CHECK(ares == AFSK1200_OK);
	CHECK(decoded_count == unpacked_bits);

	stripped_bits = decoded_count - 16U;
	for (i = 0U; i < stripped_bits; i++)
		stripped[i] = decoded_bits[i + 8U];
	CHECK(test_afsk1200_pack_bits(stripped, stripped_bits, stripped_packed,
	    sizeof(stripped_packed)) == 0);

	hres = hdlc_unstuff(stripped_packed, stripped_bits, unstuffed,
	    sizeof(unstuffed), &unstuffed_bits);
	CHECK(hres == HDLC_OK);
	CHECK((unstuffed_bits % 8U) == 0U);

	axres = ax25_decode_ui_fcs(unstuffed, unstuffed_bits / 8U, &decoded);
	CHECK(axres == AX25_OK);
	CHECK(strcmp(decoded.dst.callsign, "APZKTN") == 0);
	CHECK(strcmp(decoded.src.callsign, "M6VPN") == 0);
	CHECK(decoded.info_len == 12U);
	CHECK(memcmp(decoded.info, "KiloTNC test", 12U) == 0);

	return 0;
}

static int
test_afsk1200_impairments(void)
{
	uint8_t data_bits[] = {
		1U, 0U, 1U, 1U, 0U, 0U, 1U, 0U,
		1U, 1U, 0U, 1U, 0U, 1U, 1U, 0U
	};
	uint8_t nrzi[32];
	uint8_t decoded_nrzi[32];
	int16_t pcm[AFSK1200_SAMPLES_PER_BIT * 32U];
	int16_t impaired[(AFSK1200_SAMPLES_PER_BIT * 32U) + 96U];
	int16_t silence[AFSK1200_SAMPLES_PER_BIT * 4U];
	int16_t noise_only[AFSK1200_SAMPLES_PER_BIT * 16U];
	struct afsk1200_metrics metrics;
	size_t count;
	size_t samples;
	size_t impaired_samples;
	size_t i;
	uint16_t silence_score;
	uint16_t noise_score;
	uint16_t valid_score;
	enum afsk1200_result res;

	(void)memset(silence, 0, sizeof(silence));
	res = afsk1200_dcd_score(silence, sizeof(silence) / sizeof(silence[0]),
	    &silence_score);
	CHECK(res == AFSK1200_OK);
	CHECK(silence_score < 1000U);

	(void)memset(noise_only, 0, sizeof(noise_only));
	test_add_noise(noise_only, sizeof(noise_only) / sizeof(noise_only[0]),
	    0x12345678U, 600);
	res = afsk1200_dcd_score(noise_only,
	    sizeof(noise_only) / sizeof(noise_only[0]), &noise_score);
	CHECK(res == AFSK1200_OK);
	CHECK(noise_score < 30000U);

	res = afsk1200_nrzi_encode(data_bits, 16U, nrzi, sizeof(nrzi), &count);
	CHECK(res == AFSK1200_OK);
	res = afsk1200_encode_pcm(nrzi, count, pcm, sizeof(pcm) /
	    sizeof(pcm[0]), &samples);
	CHECK(res == AFSK1200_OK);
	res = afsk1200_dcd_score(pcm, samples, &valid_score);
	CHECK(res == AFSK1200_OK);
	CHECK(valid_score > noise_score);

	(void)memcpy(impaired, pcm, samples * sizeof(pcm[0]));
	test_scale(impaired, samples, 1, 2);
	res = afsk1200_decode_pcm_search(impaired, samples, decoded_nrzi,
	    sizeof(decoded_nrzi), &count, &metrics);
	CHECK(res == AFSK1200_OK);
	CHECK(memcmp(decoded_nrzi, nrzi, count) == 0);

	(void)memcpy(impaired, pcm, samples * sizeof(pcm[0]));
	test_scale(impaired, samples, 3, 2);
	res = afsk1200_decode_pcm_search(impaired, samples, decoded_nrzi,
	    sizeof(decoded_nrzi), &count, &metrics);
	CHECK(res == AFSK1200_OK);
	CHECK(memcmp(decoded_nrzi, nrzi, count) == 0);

	(void)memcpy(impaired, pcm, samples * sizeof(pcm[0]));
	test_add_dc(impaired, samples, 350);
	res = afsk1200_decode_pcm_search(impaired, samples, decoded_nrzi,
	    sizeof(decoded_nrzi), &count, &metrics);
	CHECK(res == AFSK1200_OK);
	CHECK(memcmp(decoded_nrzi, nrzi, count) == 0);

	(void)memcpy(impaired, pcm, samples * sizeof(pcm[0]));
	test_clip(impaired, samples, -9000, 9000);
	res = afsk1200_decode_pcm_search(impaired, samples, decoded_nrzi,
	    sizeof(decoded_nrzi), &count, &metrics);
	CHECK(res == AFSK1200_OK);
	CHECK(memcmp(decoded_nrzi, nrzi, count) == 0);

	(void)memcpy(impaired, pcm, samples * sizeof(pcm[0]));
	test_add_noise(impaired, samples, 0xA5A55A5AU, 200);
	res = afsk1200_decode_pcm_search(impaired, samples, decoded_nrzi,
	    sizeof(decoded_nrzi), &count, &metrics);
	CHECK(res == AFSK1200_OK);
	CHECK(memcmp(decoded_nrzi, nrzi, count) == 0);

	test_copy_with_silence(pcm, samples, impaired, 13U, 40U,
	    &impaired_samples);
	test_scale(&impaired[13U], samples, 3, 4);
	test_add_dc(&impaired[13U], samples, 350);
	test_add_noise(&impaired[13U], samples, 0xA5A55A5AU, 200);
	test_clip(&impaired[13U], samples, -9000, 9000);

	res = afsk1200_decode_pcm_search(impaired, impaired_samples,
	    decoded_nrzi, sizeof(decoded_nrzi), &count, &metrics);
	CHECK(res == AFSK1200_OK);
	CHECK(count == 16U);
	CHECK(memcmp(decoded_nrzi, nrzi, count) == 0);
	CHECK(metrics.bits_total == 16U);
	CHECK(metrics.mark_bits + metrics.space_bits == 16U);
	CHECK(metrics.energy_mark_total != 0U);
	CHECK(metrics.energy_space_total != 0U);
	CHECK(metrics.confidence_min != 0U);
	CHECK(metrics.confidence_avg != 0U);
	CHECK(metrics.dcd_score == metrics.confidence_avg);

	res = afsk1200_decode_pcm_search(impaired, impaired_samples,
	    decoded_nrzi, 1U, &count, &metrics);
	CHECK(res == AFSK1200_ERR_SMALL);

	res = afsk1200_decode_pcm_search(pcm, samples - 1U, decoded_nrzi,
	    sizeof(decoded_nrzi), &count, &metrics);
	CHECK(res == AFSK1200_ERR_BAD_LEN);

	(void)memcpy(impaired, pcm, samples * sizeof(pcm[0]));
	impaired_samples = samples - 3U;
	for (i = 0U; i < impaired_samples; i++)
		impaired[i] = impaired[i + 3U];
	res = afsk1200_decode_pcm_search(impaired, impaired_samples,
	    decoded_nrzi, sizeof(decoded_nrzi), &count, &metrics);
	CHECK(res == AFSK1200_ERR_BAD_LEN);

	return 0;
}

static int
test_afsk1200_null_args(void)
{
	uint8_t bits[] = { 1U, 0U };
	uint8_t out_bits[2];
	int16_t pcm[AFSK1200_SAMPLES_PER_BIT * 2U];
	size_t count;
	size_t samples;
	uint16_t score;
	enum afsk1200_result res;

	res = afsk1200_nrzi_encode(NULL, 1U, out_bits, sizeof(out_bits),
	    &count);
	CHECK(res == AFSK1200_ERR_ARG);
	res = afsk1200_nrzi_encode(bits, 2U, NULL, sizeof(out_bits), &count);
	CHECK(res == AFSK1200_ERR_ARG);
	res = afsk1200_nrzi_encode(bits, 2U, out_bits, sizeof(out_bits), NULL);
	CHECK(res == AFSK1200_ERR_ARG);
	res = afsk1200_nrzi_encode(bits, 2U, out_bits, 1U, &count);
	CHECK(res == AFSK1200_ERR_SMALL);
	bits[1] = 2U;
	res = afsk1200_nrzi_encode(bits, 2U, out_bits, sizeof(out_bits),
	    &count);
	CHECK(res == AFSK1200_ERR_BIT);
	bits[1] = 0U;

	res = afsk1200_nrzi_decode(NULL, 1U, out_bits, sizeof(out_bits),
	    &count);
	CHECK(res == AFSK1200_ERR_ARG);
	res = afsk1200_encode_pcm(NULL, 1U, pcm, sizeof(pcm) / sizeof(pcm[0]),
	    &count);
	CHECK(res == AFSK1200_ERR_ARG);
	res = afsk1200_encode_pcm(bits, 2U, NULL, sizeof(pcm) / sizeof(pcm[0]),
	    &count);
	CHECK(res == AFSK1200_ERR_ARG);
	res = afsk1200_encode_pcm(bits, 2U, pcm, sizeof(pcm) / sizeof(pcm[0]),
	    NULL);
	CHECK(res == AFSK1200_ERR_ARG);
	res = afsk1200_decode_pcm(NULL, AFSK1200_SAMPLES_PER_BIT, out_bits,
	    sizeof(out_bits), &count);
	CHECK(res == AFSK1200_ERR_ARG);
	res = afsk1200_decode_pcm(pcm, AFSK1200_SAMPLES_PER_BIT, NULL,
	    sizeof(out_bits), &count);
	CHECK(res == AFSK1200_ERR_ARG);
	res = afsk1200_decode_pcm(pcm, AFSK1200_SAMPLES_PER_BIT, out_bits,
	    sizeof(out_bits), NULL);
	CHECK(res == AFSK1200_ERR_ARG);
	res = afsk1200_encode_pcm(bits, 1U, pcm, sizeof(pcm) / sizeof(pcm[0]),
	    &samples);
	CHECK(res == AFSK1200_OK);
	res = afsk1200_decode_pcm_metrics(pcm, AFSK1200_SAMPLES_PER_BIT,
	    out_bits, sizeof(out_bits), &count, NULL);
	CHECK(res == AFSK1200_OK);
	res = afsk1200_decode_pcm_search(NULL, AFSK1200_SAMPLES_PER_BIT,
	    out_bits, sizeof(out_bits), &count, NULL);
	CHECK(res == AFSK1200_ERR_ARG);
	res = afsk1200_decode_pcm_search(pcm, AFSK1200_SAMPLES_PER_BIT,
	    NULL, sizeof(out_bits), &count, NULL);
	CHECK(res == AFSK1200_ERR_ARG);
	res = afsk1200_decode_pcm_search(pcm, AFSK1200_SAMPLES_PER_BIT,
	    out_bits, sizeof(out_bits), NULL, NULL);
	CHECK(res == AFSK1200_ERR_ARG);
	res = afsk1200_dcd_score(NULL, AFSK1200_SAMPLES_PER_BIT, &score);
	CHECK(res == AFSK1200_ERR_ARG);
	res = afsk1200_dcd_score(pcm, AFSK1200_SAMPLES_PER_BIT, NULL);
	CHECK(res == AFSK1200_ERR_ARG);

	return 0;
}

static int
test_afsk1200_pcm(void)
{
	uint8_t mark_bits[] = { 1U, 1U, 1U, 1U };
	uint8_t space_bits[] = { 0U, 0U, 0U, 0U };
	uint8_t data_bits[] = { 1U, 0U, 1U, 1U, 0U, 0U, 1U, 0U };
	uint8_t nrzi[16];
	uint8_t decoded_nrzi[16];
	uint8_t decoded_data[16];
	int16_t pcm[AFSK1200_SAMPLES_PER_BIT * 16U];
	size_t count;
	size_t samples;
	size_t i;
	enum afsk1200_result res;

	res = afsk1200_encode_pcm(mark_bits, 4U, pcm, sizeof(pcm) /
	    sizeof(pcm[0]), &samples);
	CHECK(res == AFSK1200_OK);
	CHECK(samples == 4U * AFSK1200_SAMPLES_PER_BIT);
	CHECK(pcm[1] != 0);
	res = afsk1200_encode_pcm(space_bits, 4U, pcm, sizeof(pcm) /
	    sizeof(pcm[0]), &samples);
	CHECK(res == AFSK1200_OK);
	CHECK(pcm[1] != 0);
	res = afsk1200_encode_pcm(mark_bits, 4U, pcm,
	    (4U * AFSK1200_SAMPLES_PER_BIT) - 1U, &samples);
	CHECK(res == AFSK1200_ERR_SMALL);

	res = afsk1200_nrzi_encode(data_bits, 8U, nrzi, sizeof(nrzi), &count);
	CHECK(res == AFSK1200_OK);
	res = afsk1200_encode_pcm(nrzi, count, pcm, sizeof(pcm) /
	    sizeof(pcm[0]), &samples);
	CHECK(res == AFSK1200_OK);
	CHECK(samples == count * AFSK1200_SAMPLES_PER_BIT);
	for (i = 0U; i < samples; i++)
		CHECK(pcm[i] >= -32768 && pcm[i] <= 32767);

	res = afsk1200_decode_pcm(pcm, samples, decoded_nrzi,
	    sizeof(decoded_nrzi), &count);
	CHECK(res == AFSK1200_OK);
	CHECK(memcmp(decoded_nrzi, nrzi, count) == 0);
	res = afsk1200_nrzi_decode(decoded_nrzi, count, decoded_data,
	    sizeof(decoded_data), &count);
	CHECK(res == AFSK1200_OK);
	CHECK(memcmp(decoded_data, data_bits, count) == 0);

	for (i = 0U; i < samples; i++)
		pcm[i] = (int16_t)(pcm[i] / 2);
	res = afsk1200_decode_pcm(pcm, samples, decoded_nrzi,
	    sizeof(decoded_nrzi), &count);
	CHECK(res == AFSK1200_OK);
	CHECK(memcmp(decoded_nrzi, nrzi, count) == 0);

	res = afsk1200_decode_pcm(pcm, samples - 1U, decoded_nrzi,
	    sizeof(decoded_nrzi), &count);
	CHECK(res == AFSK1200_ERR_BAD_LEN);

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

static void
test_add_dc(int16_t *pcm, size_t len, int32_t offset)
{
	size_t i;

	for (i = 0U; i < len; i++)
		pcm[i] = test_saturate((int32_t)pcm[i] + offset);
}

static void
test_add_noise(int16_t *pcm, size_t len, uint32_t seed, int32_t amplitude)
{
	size_t i;
	int32_t span;
	int32_t noise;

	span = (amplitude * 2) + 1;
	for (i = 0U; i < len; i++) {
		noise = (int32_t)(test_next_noise(&seed) % (uint32_t)span) -
		    amplitude;
		pcm[i] = test_saturate((int32_t)pcm[i] + noise);
	}
}

static void
test_clip(int16_t *pcm, size_t len, int32_t min, int32_t max)
{
	size_t i;
	int32_t sample;

	for (i = 0U; i < len; i++) {
		sample = pcm[i];
		if (sample < min)
			sample = min;
		if (sample > max)
			sample = max;
		pcm[i] = (int16_t)sample;
	}
}

static void
test_copy_with_silence(const int16_t *src, size_t src_len, int16_t *dst,
	size_t lead, size_t trail, size_t *out_len)
{
	size_t i;

	for (i = 0U; i < lead + src_len + trail; i++)
		dst[i] = 0;
	for (i = 0U; i < src_len; i++)
		dst[i + lead] = src[i];
	*out_len = lead + src_len + trail;
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

static void
test_scale(int16_t *pcm, size_t len, int32_t num, int32_t den)
{
	size_t i;

	for (i = 0U; i < len; i++)
		pcm[i] = test_saturate(((int32_t)pcm[i] * num) / den);
}

static int16_t
test_saturate(int32_t sample)
{
	if (sample > 32767)
		return 32767;
	if (sample < -32768)
		return -32768;
	return (int16_t)sample;
}

static int
test_afsk1200_pack_bits(const uint8_t *bits, size_t bit_count, uint8_t *out,
	size_t out_cap)
{
	size_t i;

	if (((bit_count + 7U) / 8U) > out_cap)
		return __LINE__;
	(void)memset(out, 0, out_cap);
	for (i = 0U; i < bit_count; i++) {
		if (bits[i] != 0U)
			out[i / 8U] |= (uint8_t)(1U << (i % 8U));
	}

	return 0;
}

static int
test_afsk1200_put_bit(uint8_t *buf, size_t cap, size_t *pos, uint8_t bit)
{
	if ((*pos / 8U) >= cap)
		return __LINE__;
	if (bit != 0U)
		buf[*pos / 8U] |= (uint8_t)(1U << (*pos % 8U));
	(*pos)++;

	return 0;
}

static int
test_afsk1200_unpack_bits(const uint8_t *packed, size_t bit_count,
	uint8_t *out, size_t out_cap)
{
	size_t i;
	uint32_t byte;

	if (bit_count > out_cap)
		return __LINE__;

	for (i = 0U; i < bit_count; i++) {
		byte = packed[i / 8U];
		out[i] = (uint8_t)((byte >> (i % 8U)) & 1U);
	}

	return 0;
}
