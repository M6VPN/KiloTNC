/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/src/afsk1200.c */

#include <sys/types.h>

#include <stddef.h>
#include <stdint.h>

#include "afsk1200.h"

#define AFSK1200_Q			16384
#define AFSK1200_MARK_COEFF		32365
#define AFSK1200_SPACE_COEFF		31419
#define AFSK1200_MARK_START		1877
#define AFSK1200_SPACE_START		3408
#define AFSK1200_TONE_MARK		1U
#define AFSK1200_TONE_SPACE		0U

static enum afsk1200_result afsk1200_check_bits(const uint8_t *, size_t);
static int64_t afsk1200_goertzel_power(const int16_t *, int32_t);
static void afsk1200_generate_tone(uint8_t, int16_t *);

static enum afsk1200_result
afsk1200_check_bits(const uint8_t *bits, size_t bit_count)
{
	size_t i;

	if (bits == NULL && bit_count != 0U)
		return AFSK1200_ERR_ARG;

	for (i = 0U; i < bit_count; i++) {
		if (bits[i] != 0U && bits[i] != 1U)
			return AFSK1200_ERR_BIT;
	}

	return AFSK1200_OK;
}

enum afsk1200_result
afsk1200_decode_pcm(const int16_t *pcm, size_t sample_count,
	uint8_t *out_nrzi_bits, size_t out_cap, size_t *out_bits)
{
	size_t bit_count;
	size_t i;
	int64_t mark_power;
	int64_t space_power;

	if (out_bits == NULL)
		return AFSK1200_ERR_ARG;
	*out_bits = 0U;

	if ((pcm == NULL && sample_count != 0U) || out_nrzi_bits == NULL)
		return AFSK1200_ERR_ARG;
	if ((sample_count % AFSK1200_SAMPLES_PER_BIT) != 0U)
		return AFSK1200_ERR_BAD_LEN;

	bit_count = sample_count / AFSK1200_SAMPLES_PER_BIT;
	if (bit_count > out_cap)
		return AFSK1200_ERR_SMALL;

	for (i = 0U; i < bit_count; i++) {
		mark_power = afsk1200_goertzel_power(
		    &pcm[i * AFSK1200_SAMPLES_PER_BIT], AFSK1200_MARK_COEFF);
		space_power = afsk1200_goertzel_power(
		    &pcm[i * AFSK1200_SAMPLES_PER_BIT], AFSK1200_SPACE_COEFF);
		out_nrzi_bits[i] = mark_power >= space_power ?
		    AFSK1200_TONE_MARK : AFSK1200_TONE_SPACE;
	}

	*out_bits = bit_count;
	return AFSK1200_OK;
}

enum afsk1200_result
afsk1200_encode_pcm(const uint8_t *nrzi_bits, size_t bit_count,
	int16_t *out, size_t out_cap, size_t *out_samples)
{
	enum afsk1200_result res;
	size_t needed;
	size_t i;

	if (out_samples == NULL)
		return AFSK1200_ERR_ARG;
	*out_samples = 0U;

	if ((nrzi_bits == NULL && bit_count != 0U) || out == NULL)
		return AFSK1200_ERR_ARG;

	res = afsk1200_check_bits(nrzi_bits, bit_count);
	if (res != AFSK1200_OK)
		return res;

	res = afsk1200_samples_for_bits(bit_count, &needed);
	if (res != AFSK1200_OK)
		return res;
	if (needed > out_cap)
		return AFSK1200_ERR_SMALL;

	for (i = 0U; i < bit_count; i++) {
		afsk1200_generate_tone(nrzi_bits[i],
		    &out[i * AFSK1200_SAMPLES_PER_BIT]);
	}

	*out_samples = needed;
	return AFSK1200_OK;
}

static void
afsk1200_generate_tone(uint8_t tone, int16_t *out)
{
	int32_t coeff;
	int32_t prev;
	int32_t curr;
	int32_t next;
	size_t i;

	if (tone == AFSK1200_TONE_MARK) {
		coeff = AFSK1200_MARK_COEFF;
		curr = AFSK1200_MARK_START;
	} else {
		coeff = AFSK1200_SPACE_COEFF;
		curr = AFSK1200_SPACE_START;
	}
	prev = 0;
	out[0] = 0;

	for (i = 1U; i < AFSK1200_SAMPLES_PER_BIT; i++) {
		out[i] = (int16_t)curr;
		next = (int32_t)((((int64_t)coeff * curr) / AFSK1200_Q) -
		    prev);
		prev = curr;
		curr = next;
	}
}

static int64_t
afsk1200_goertzel_power(const int16_t *samples, int32_t coeff)
{
	int64_t q0;
	int64_t q1;
	int64_t q2;
	size_t i;

	q1 = 0;
	q2 = 0;
	for (i = 0U; i < AFSK1200_SAMPLES_PER_BIT; i++) {
		q0 = samples[i] + (((int64_t)coeff * q1) / AFSK1200_Q) - q2;
		q2 = q1;
		q1 = q0;
	}

	return (q1 * q1) + (q2 * q2) -
	    (((int64_t)coeff * q1 * q2) / AFSK1200_Q);
}

enum afsk1200_result
afsk1200_nrzi_decode(const uint8_t *nrzi_bits, size_t bit_count,
	uint8_t *out, size_t out_cap, size_t *out_count)
{
	enum afsk1200_result res;
	size_t i;
	uint8_t prev;

	if (out_count == NULL)
		return AFSK1200_ERR_ARG;
	*out_count = 0U;

	if ((nrzi_bits == NULL && bit_count != 0U) || out == NULL)
		return AFSK1200_ERR_ARG;
	if (bit_count > out_cap)
		return AFSK1200_ERR_SMALL;

	res = afsk1200_check_bits(nrzi_bits, bit_count);
	if (res != AFSK1200_OK)
		return res;

	prev = AFSK1200_TONE_MARK;
	for (i = 0U; i < bit_count; i++) {
		out[i] = nrzi_bits[i] == prev ? 1U : 0U;
		prev = nrzi_bits[i];
	}

	*out_count = bit_count;
	return AFSK1200_OK;
}

enum afsk1200_result
afsk1200_nrzi_encode(const uint8_t *bits, size_t bit_count, uint8_t *out,
	size_t out_cap, size_t *out_count)
{
	enum afsk1200_result res;
	size_t i;
	uint8_t state;

	if (out_count == NULL)
		return AFSK1200_ERR_ARG;
	*out_count = 0U;

	if ((bits == NULL && bit_count != 0U) || out == NULL)
		return AFSK1200_ERR_ARG;
	if (bit_count > out_cap)
		return AFSK1200_ERR_SMALL;

	res = afsk1200_check_bits(bits, bit_count);
	if (res != AFSK1200_OK)
		return res;

	state = AFSK1200_TONE_MARK;
	for (i = 0U; i < bit_count; i++) {
		if (bits[i] == 0U)
			state ^= 1U;
		out[i] = state;
	}

	*out_count = bit_count;
	return AFSK1200_OK;
}

enum afsk1200_result
afsk1200_samples_for_bits(size_t bit_count, size_t *out_samples)
{
	if (out_samples == NULL)
		return AFSK1200_ERR_ARG;
	*out_samples = 0U;

	if (bit_count > ((size_t)-1) / AFSK1200_SAMPLES_PER_BIT)
		return AFSK1200_ERR_BAD_LEN;

	*out_samples = bit_count * AFSK1200_SAMPLES_PER_BIT;
	return AFSK1200_OK;
}
