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

static int test_afsk1200_full_chain(void);
static int test_afsk1200_impairments(void);
static int test_afsk1200_null_args(void);
static int test_afsk1200_pcm(void);
static int test_afsk1200_put_bit(uint8_t *, size_t, size_t *, uint8_t);
static int test_afsk1200_unpack_bits(const uint8_t *, size_t, uint8_t *,
	size_t);
static int test_afsk1200_pack_bits(const uint8_t *, size_t, uint8_t *,
	size_t);
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
	struct afsk1200_metrics metrics;
	size_t count;
	size_t samples;
	size_t impaired_samples;
	size_t i;
	uint32_t noise;
	int32_t sample;
	enum afsk1200_result res;

	res = afsk1200_nrzi_encode(data_bits, 16U, nrzi, sizeof(nrzi), &count);
	CHECK(res == AFSK1200_OK);
	res = afsk1200_encode_pcm(nrzi, count, pcm, sizeof(pcm) /
	    sizeof(pcm[0]), &samples);
	CHECK(res == AFSK1200_OK);

	(void)memset(impaired, 0, sizeof(impaired));
	for (i = 0U; i < samples; i++) {
		noise = (uint32_t)((i * 1103515245U) + 12345U);
		sample = pcm[i];
		sample = (sample * 3) / 4;
		sample += 350;
		sample += (int32_t)(noise % 401U) - 200;
		if (sample > 9000)
			sample = 9000;
		if (sample < -9000)
			sample = -9000;
		impaired[i + 13U] = (int16_t)sample;
	}
	impaired_samples = samples + 53U;

	res = afsk1200_decode_pcm_metrics(impaired, impaired_samples,
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
