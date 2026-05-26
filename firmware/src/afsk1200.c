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
#define AFSK1200_SILENCE_THRESHOLD	1000000U

static enum afsk1200_result afsk1200_check_bits(const uint8_t *, size_t);
static uint32_t afsk1200_confidence(int64_t, int64_t);
static void afsk1200_generate_tone(uint8_t, int16_t *);
static int64_t afsk1200_goertzel_power(const int16_t *, size_t, int32_t);
static int afsk1200_has_signal_tail(const int16_t *, size_t);
static int64_t afsk1200_window_energy(const int16_t *, size_t);
static void afsk1200_metrics_add(struct afsk1200_metrics *, uint8_t,
	uint32_t, uint32_t, uint32_t);
static void afsk1200_metrics_finish(struct afsk1200_metrics *);
static void afsk1200_metrics_init(struct afsk1200_metrics *);

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
	return afsk1200_decode_pcm_metrics(pcm, sample_count, out_nrzi_bits,
	    out_cap, out_bits, NULL);
}

enum afsk1200_result
afsk1200_decode_pcm_metrics(const int16_t *pcm, size_t sample_count,
	uint8_t *out_nrzi_bits, size_t out_cap, size_t *out_bits,
	struct afsk1200_metrics *metrics)
{
	size_t bit_count;
	size_t best_offset;
	size_t best_bits;
	uint32_t best_score;
	uint32_t score;
	size_t candidate_offset;
	size_t candidate_bits;
	size_t offset;
	size_t i;
	int64_t mark_power;
	int64_t space_power;
	int64_t energy;
	uint32_t mark_store;
	uint32_t space_store;
	uint32_t confidence;

	if (out_bits == NULL)
		return AFSK1200_ERR_ARG;
	*out_bits = 0U;
	afsk1200_metrics_init(metrics);

	if ((pcm == NULL && sample_count != 0U) || out_nrzi_bits == NULL)
		return AFSK1200_ERR_ARG;
	if (sample_count < AFSK1200_SAMPLES_PER_BIT)
		return AFSK1200_ERR_BAD_LEN;

	best_offset = AFSK1200_SAMPLES_PER_BIT;
	best_bits = 0U;
	best_score = 0U;
	for (offset = 0U; offset < AFSK1200_SAMPLES_PER_BIT; offset++) {
		bit_count = (sample_count - offset) / AFSK1200_SAMPLES_PER_BIT;
		for (i = 0U; i < bit_count; i++) {
			energy = afsk1200_window_energy(&pcm[offset +
			    (i * AFSK1200_SAMPLES_PER_BIT)], 0U);
			if (energy > AFSK1200_SILENCE_THRESHOLD)
				break;
		}
		if (i == bit_count)
			continue;

		candidate_offset = offset + (i * AFSK1200_SAMPLES_PER_BIT);
		candidate_bits = bit_count - i;
		while (candidate_bits != 0U) {
			energy = afsk1200_window_energy(&pcm[candidate_offset +
			    ((candidate_bits - 1U) * AFSK1200_SAMPLES_PER_BIT)],
			    0U);
			if (energy > AFSK1200_SILENCE_THRESHOLD)
				break;
			candidate_bits--;
		}
		if (candidate_bits == 0U)
			continue;
		if (afsk1200_has_signal_tail(pcm, candidate_offset))
			continue;
		if (afsk1200_has_signal_tail(&pcm[candidate_offset +
		    (candidate_bits * AFSK1200_SAMPLES_PER_BIT)],
		    sample_count - (candidate_offset +
		    (candidate_bits * AFSK1200_SAMPLES_PER_BIT))))
			continue;

		score = 0U;
		for (i = 0U; i < candidate_bits; i++) {
			mark_power = afsk1200_goertzel_power(
			    &pcm[candidate_offset + (i *
			    AFSK1200_SAMPLES_PER_BIT)], 0U,
			    AFSK1200_MARK_COEFF);
			space_power = afsk1200_goertzel_power(
			    &pcm[candidate_offset + (i *
			    AFSK1200_SAMPLES_PER_BIT)], 0U,
			    AFSK1200_SPACE_COEFF);
			score += afsk1200_confidence(mark_power, space_power) /
			    (uint32_t)candidate_bits;
		}
		if (best_offset == AFSK1200_SAMPLES_PER_BIT ||
		    score > best_score) {
			best_offset = candidate_offset;
			best_bits = candidate_bits;
			best_score = score;
		}
	}

	if (best_offset == AFSK1200_SAMPLES_PER_BIT)
		return AFSK1200_ERR_BAD_LEN;
	if (best_bits == 0U)
		return AFSK1200_ERR_BAD_LEN;

	bit_count = best_bits;
	if (bit_count > out_cap)
		return AFSK1200_ERR_SMALL;

	for (i = 0U; i < bit_count; i++) {
		mark_power = afsk1200_goertzel_power(
		    &pcm[best_offset + (i * AFSK1200_SAMPLES_PER_BIT)], 0U,
		    AFSK1200_MARK_COEFF);
		space_power = afsk1200_goertzel_power(
		    &pcm[best_offset + (i * AFSK1200_SAMPLES_PER_BIT)], 0U,
		    AFSK1200_SPACE_COEFF);
		out_nrzi_bits[i] = mark_power >= space_power ?
		    AFSK1200_TONE_MARK : AFSK1200_TONE_SPACE;
		mark_store = mark_power > UINT32_MAX ? UINT32_MAX :
		    (uint32_t)mark_power;
		space_store = space_power > UINT32_MAX ? UINT32_MAX :
		    (uint32_t)space_power;
		confidence = afsk1200_confidence(mark_power, space_power);
		afsk1200_metrics_add(metrics, out_nrzi_bits[i], mark_store,
		    space_store, confidence);
	}

	afsk1200_metrics_finish(metrics);
	*out_bits = bit_count;
	return AFSK1200_OK;
}

static int
afsk1200_has_signal_tail(const int16_t *samples, size_t sample_count)
{
	size_t i;
	int32_t sample;

	for (i = 0U; i < sample_count; i++) {
		sample = samples[i];
		if (sample < 0)
			sample = -sample;
		if (sample > 32)
			return 1;
	}

	return 0;
}

static uint32_t
afsk1200_confidence(int64_t mark_power, int64_t space_power)
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
afsk1200_goertzel_power(const int16_t *samples, size_t phase, int32_t coeff)
{
	int64_t q0;
	int64_t q1;
	int64_t q2;
	int64_t mean;
	int64_t sample;
	size_t i;

	mean = 0;
	for (i = 0U; i < AFSK1200_SAMPLES_PER_BIT; i++)
		mean += samples[(i + phase) % AFSK1200_SAMPLES_PER_BIT];
	mean /= AFSK1200_SAMPLES_PER_BIT;

	q1 = 0;
	q2 = 0;
	for (i = 0U; i < AFSK1200_SAMPLES_PER_BIT; i++) {
		sample = samples[(i + phase) % AFSK1200_SAMPLES_PER_BIT] -
		    mean;
		q0 = sample + (((int64_t)coeff * q1) / AFSK1200_Q) - q2;
		q2 = q1;
		q1 = q0;
	}

	return (q1 * q1) + (q2 * q2) -
	    (((int64_t)coeff * q1 * q2) / AFSK1200_Q);
}

static void
afsk1200_metrics_add(struct afsk1200_metrics *metrics, uint8_t tone,
	uint32_t mark, uint32_t space, uint32_t confidence)
{
	if (metrics == NULL)
		return;

	metrics->bits_total++;
	if (tone == AFSK1200_TONE_MARK)
		metrics->mark_bits++;
	else
		metrics->space_bits++;
	if (UINT32_MAX - metrics->energy_mark_total < mark)
		metrics->energy_mark_total = UINT32_MAX;
	else
		metrics->energy_mark_total += mark;
	if (UINT32_MAX - metrics->energy_space_total < space)
		metrics->energy_space_total = UINT32_MAX;
	else
		metrics->energy_space_total += space;
	if (UINT32_MAX - metrics->confidence_total < confidence)
		metrics->confidence_total = UINT32_MAX;
	else
		metrics->confidence_total += confidence;
	if (confidence < metrics->confidence_min)
		metrics->confidence_min = (uint16_t)confidence;
}

static void
afsk1200_metrics_finish(struct afsk1200_metrics *metrics)
{
	uint32_t avg;

	if (metrics == NULL)
		return;
	if (metrics->bits_total == 0U) {
		metrics->confidence_min = 0U;
		return;
	}

	avg = metrics->confidence_total / (uint32_t)metrics->bits_total;
	if (avg > 65535U)
		avg = 65535U;
	metrics->confidence_avg = (uint16_t)avg;
	metrics->dcd_score = metrics->confidence_avg;
}

static void
afsk1200_metrics_init(struct afsk1200_metrics *metrics)
{
	if (metrics == NULL)
		return;

	metrics->bits_total = 0U;
	metrics->mark_bits = 0U;
	metrics->space_bits = 0U;
	metrics->energy_mark_total = 0U;
	metrics->energy_space_total = 0U;
	metrics->confidence_total = 0U;
	metrics->confidence_min = UINT16_MAX;
	metrics->confidence_avg = 0U;
	metrics->dcd_score = 0U;
}

static int64_t
afsk1200_window_energy(const int16_t *samples, size_t phase)
{
	int64_t energy;
	int64_t mean;
	int64_t sample;
	size_t i;

	mean = 0;
	for (i = 0U; i < AFSK1200_SAMPLES_PER_BIT; i++)
		mean += samples[(i + phase) % AFSK1200_SAMPLES_PER_BIT];
	mean /= AFSK1200_SAMPLES_PER_BIT;

	energy = 0;
	for (i = 0U; i < AFSK1200_SAMPLES_PER_BIT; i++) {
		sample = samples[(i + phase) % AFSK1200_SAMPLES_PER_BIT] -
		    mean;
		energy += sample * sample;
	}

	return energy;
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
