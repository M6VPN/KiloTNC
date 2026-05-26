/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/src/afsk1200_tx.c */

#include <sys/types.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "afsk1200_tx.h"
#include "ax25.h"
#include "hdlc.h"

#define AFSK1200_TX_MARK_START		1877
#define AFSK1200_TX_SPACE_START		3408
#define AFSK1200_TX_MARK_COEFF		32365
#define AFSK1200_TX_SPACE_COEFF		31419
#define AFSK1200_TX_Q			16384
#define AFSK1200_TX_TONE_MARK		1U
#define AFSK1200_TX_TONE_SPACE		0U

static enum afsk1200_tx_result afsk1200_tx_configure(
	struct afsk1200_tx *, const struct afsk1200_tx_config *);
static uint8_t afsk1200_tx_get_bit(const uint8_t *, size_t);
static uint8_t afsk1200_tx_next_data_bit(struct afsk1200_tx *);
static enum afsk1200_tx_result afsk1200_tx_next_sample(
	struct afsk1200_tx *, int16_t *);
static int16_t afsk1200_tx_sample(const struct afsk1200_tx *, uint8_t,
	size_t);
static int16_t afsk1200_tx_scale_sample(int32_t, int16_t);
static void afsk1200_tx_set_idle(struct afsk1200_tx *);
static enum afsk1200_tx_result afsk1200_tx_unpack_bits(
	const uint8_t *, size_t, uint8_t *, size_t, size_t *);
static enum afsk1200_tx_result afsk1200_tx_validate_config(
	const struct afsk1200_tx_config *);

enum afsk1200_tx_result
afsk1200_tx_abort(struct afsk1200_tx *tx)
{
	if (tx == NULL)
		return AFSK1200_TX_ERR_ARG;

	afsk1200_tx_set_idle(tx);
	return AFSK1200_TX_OK;
}

enum afsk1200_tx_result
afsk1200_tx_init(struct afsk1200_tx *tx,
	const struct afsk1200_tx_config *config)
{
	enum afsk1200_tx_result res;

	if (tx == NULL)
		return AFSK1200_TX_ERR_ARG;

	(void)memset(tx, 0, sizeof(*tx));
	res = afsk1200_tx_configure(tx, config);
	if (res != AFSK1200_TX_OK)
		return res;
	afsk1200_tx_set_idle(tx);

	return AFSK1200_TX_OK;
}

enum afsk1200_tx_result
afsk1200_tx_is_active(const struct afsk1200_tx *tx, int *active)
{
	if (tx == NULL || active == NULL)
		return AFSK1200_TX_ERR_ARG;

	*active = tx->state != AFSK1200_TX_IDLE &&
	    tx->state != AFSK1200_TX_DONE_STATE;
	return AFSK1200_TX_OK;
}

enum afsk1200_tx_result
afsk1200_tx_process(struct afsk1200_tx *tx, int16_t *out, size_t out_cap,
	size_t *out_samples)
{
	size_t pos;
	enum afsk1200_tx_result res;

	if (tx == NULL || out_samples == NULL)
		return AFSK1200_TX_ERR_ARG;
	*out_samples = 0U;
	if (out == NULL)
		return AFSK1200_TX_ERR_ARG;
	if (out_cap == 0U) {
		tx->stats.underruns++;
		return AFSK1200_TX_ERR_SMALL;
	}
	if (tx->state == AFSK1200_TX_IDLE ||
	    tx->state == AFSK1200_TX_DONE_STATE)
		return AFSK1200_TX_DONE;

	pos = 0U;
	while (pos < out_cap) {
		res = afsk1200_tx_next_sample(tx, &out[pos]);
		if (res == AFSK1200_TX_DONE)
			break;
		if (res != AFSK1200_TX_OK)
			return res;
		pos++;
		tx->stats.samples_total++;
	}

	*out_samples = pos;
	if (pos != 0U)
		tx->stats.chunks_emitted++;
	if (tx->state == AFSK1200_TX_DONE_STATE && pos == 0U)
		return AFSK1200_TX_DONE;
	if (tx->state == AFSK1200_TX_DONE_STATE && pos < out_cap)
		return AFSK1200_TX_DONE;

	return AFSK1200_TX_OK;
}

enum afsk1200_tx_result
afsk1200_tx_start_frame(struct afsk1200_tx *tx, const uint8_t *frame,
	size_t frame_len)
{
	uint8_t stuffed[(AFSK1200_MAX_TEST_BITS + 7U) / 8U];
	struct ax25_frame decoded;
	size_t stuffed_bits;
	enum hdlc_result hres;

	if (tx == NULL || frame == NULL)
		return AFSK1200_TX_ERR_ARG;
	if (tx->state != AFSK1200_TX_IDLE &&
	    tx->state != AFSK1200_TX_DONE_STATE)
		return AFSK1200_TX_ERR_BUSY;
	if (frame_len == 0U || frame_len > KILOTNC_AX25_MAX_FRAME) {
		tx->stats.frames_rejected++;
		return AFSK1200_TX_ERR_BAD_FRAME;
	}
	if (ax25_decode_ui_fcs(frame, frame_len, &decoded) != AX25_OK) {
		tx->stats.frames_rejected++;
		return AFSK1200_TX_ERR_BAD_FRAME;
	}

	hres = hdlc_bitstuff(frame, frame_len * 8U, stuffed,
	    sizeof(stuffed), &stuffed_bits);
	if (hres != HDLC_OK) {
		tx->stats.frames_rejected++;
		return AFSK1200_TX_ERR_BAD_FRAME;
	}
	if (afsk1200_tx_unpack_bits(stuffed, stuffed_bits, tx->frame_bits,
	    sizeof(tx->frame_bits), &tx->frame_bits_count) != AFSK1200_TX_OK) {
		tx->stats.frames_rejected++;
		return AFSK1200_TX_ERR_BAD_FRAME;
	}

	tx->frame_bits_pos = 0U;
	tx->flag_bits_pos = 0U;
	tx->current_bit_sample = 0U;
	tx->nrzi_state = AFSK1200_TX_TONE_MARK;
	tx->current_tone = AFSK1200_TX_TONE_MARK;
	tx->have_tone = false;
	tx->state = tx->config.txdelay_flags == 0U ? AFSK1200_TX_FRAME_BITS :
	    AFSK1200_TX_PREAMBLE_FLAGS;
	tx->stats.frames_queued++;

	return AFSK1200_TX_OK;
}

enum afsk1200_tx_result
afsk1200_tx_stats(const struct afsk1200_tx *tx, struct afsk1200_tx_stats *stats)
{
	if (tx == NULL || stats == NULL)
		return AFSK1200_TX_ERR_ARG;

	*stats = tx->stats;
	return AFSK1200_TX_OK;
}

static enum afsk1200_tx_result
afsk1200_tx_configure(struct afsk1200_tx *tx,
	const struct afsk1200_tx_config *config)
{
	struct afsk1200_tx_config defaults;
	enum afsk1200_tx_result res;

	defaults.txdelay_flags = AFSK1200_TX_DEFAULT_TXDELAY_FLAGS;
	defaults.txtail_flags = AFSK1200_TX_DEFAULT_TXTAIL_FLAGS;
	defaults.amplitude = AFSK1200_PCM_AMPLITUDE;
	if (config == NULL)
		config = &defaults;

	res = afsk1200_tx_validate_config(config);
	if (res != AFSK1200_TX_OK)
		return res;
	tx->config = *config;

	return AFSK1200_TX_OK;
}

static uint8_t
afsk1200_tx_get_bit(const uint8_t *buf, size_t bit)
{
	uint32_t byte;

	byte = buf[bit / 8U];
	return (uint8_t)((byte >> (bit % 8U)) & 1U);
}

static uint8_t
afsk1200_tx_next_data_bit(struct afsk1200_tx *tx)
{
	uint8_t bit;

	if (tx->state == AFSK1200_TX_PREAMBLE_FLAGS ||
	    tx->state == AFSK1200_TX_TAIL_FLAGS) {
		bit = (uint8_t)((KILOTNC_HDLC_FLAG >>
		    (tx->flag_bits_pos % 8U)) & 1U);
		tx->flag_bits_pos++;
		return bit;
	}

	bit = tx->frame_bits[tx->frame_bits_pos];
	tx->frame_bits_pos++;
	return bit;
}

static enum afsk1200_tx_result
afsk1200_tx_next_sample(struct afsk1200_tx *tx, int16_t *sample)
{
	uint8_t bit;

	if (tx->state == AFSK1200_TX_IDLE ||
	    tx->state == AFSK1200_TX_DONE_STATE)
		return AFSK1200_TX_DONE;

	if (!tx->have_tone) {
		bit = afsk1200_tx_next_data_bit(tx);
		if (bit == 0U)
			tx->nrzi_state ^= 1U;
		tx->current_tone = tx->nrzi_state;
		tx->have_tone = true;
		tx->current_bit_sample = 0U;
		tx->stats.bits_total++;
	}

	*sample = afsk1200_tx_sample(tx, tx->current_tone,
	    tx->current_bit_sample);
	tx->current_bit_sample++;
	if (tx->current_bit_sample < AFSK1200_SAMPLES_PER_BIT)
		return AFSK1200_TX_OK;

	tx->have_tone = false;
	tx->current_bit_sample = 0U;
	if (tx->state == AFSK1200_TX_PREAMBLE_FLAGS &&
	    tx->flag_bits_pos >= tx->config.txdelay_flags * 8U) {
		tx->state = AFSK1200_TX_FRAME_BITS;
		tx->flag_bits_pos = 0U;
	}
	if (tx->state == AFSK1200_TX_FRAME_BITS &&
	    tx->frame_bits_pos >= tx->frame_bits_count) {
		if (tx->config.txtail_flags == 0U) {
			tx->state = AFSK1200_TX_DONE_STATE;
			tx->stats.frames_done++;
		} else {
			tx->state = AFSK1200_TX_TAIL_FLAGS;
			tx->flag_bits_pos = 0U;
		}
	}
	if (tx->state == AFSK1200_TX_TAIL_FLAGS &&
	    tx->flag_bits_pos >= tx->config.txtail_flags * 8U) {
		tx->state = AFSK1200_TX_DONE_STATE;
		tx->stats.frames_done++;
	}

	return AFSK1200_TX_OK;
}

static int16_t
afsk1200_tx_sample(const struct afsk1200_tx *tx, uint8_t tone, size_t index)
{
	int32_t coeff;
	int32_t prev;
	int32_t curr;
	int32_t next;
	size_t i;

	if (index == 0U)
		return 0;
	if (tone == AFSK1200_TX_TONE_MARK) {
		coeff = AFSK1200_TX_MARK_COEFF;
		curr = AFSK1200_TX_MARK_START;
	} else {
		coeff = AFSK1200_TX_SPACE_COEFF;
		curr = AFSK1200_TX_SPACE_START;
	}

	prev = 0;
	for (i = 1U; i < index; i++) {
		next = (int32_t)((((int64_t)coeff * curr) /
		    AFSK1200_TX_Q) - prev);
		prev = curr;
		curr = next;
	}

	return afsk1200_tx_scale_sample(curr, tx->config.amplitude);
}

static int16_t
afsk1200_tx_scale_sample(int32_t sample, int16_t amplitude)
{
	int64_t scaled;

	scaled = ((int64_t)sample * amplitude) / AFSK1200_PCM_AMPLITUDE;
	if (scaled > 32767)
		return 32767;
	if (scaled < -32768)
		return -32768;
	return (int16_t)scaled;
}

static void
afsk1200_tx_set_idle(struct afsk1200_tx *tx)
{
	tx->state = AFSK1200_TX_IDLE;
	tx->frame_bits_count = 0U;
	tx->frame_bits_pos = 0U;
	tx->flag_bits_pos = 0U;
	tx->current_bit_sample = 0U;
	tx->nrzi_state = AFSK1200_TX_TONE_MARK;
	tx->current_tone = AFSK1200_TX_TONE_MARK;
	tx->have_tone = false;
}

static enum afsk1200_tx_result
afsk1200_tx_unpack_bits(const uint8_t *packed, size_t bit_count, uint8_t *out,
	size_t out_cap, size_t *out_count)
{
	size_t i;

	if (bit_count > out_cap)
		return AFSK1200_TX_ERR_SMALL;

	for (i = 0U; i < bit_count; i++)
		out[i] = afsk1200_tx_get_bit(packed, i);
	*out_count = bit_count;

	return AFSK1200_TX_OK;
}

static enum afsk1200_tx_result
afsk1200_tx_validate_config(const struct afsk1200_tx_config *config)
{
	if (config->amplitude <= 0)
		return AFSK1200_TX_ERR_ARG;
	if (config->txdelay_flags > (AFSK1200_MAX_TEST_BITS / 8U) ||
	    config->txtail_flags > (AFSK1200_MAX_TEST_BITS / 8U))
		return AFSK1200_TX_ERR_ARG;

	return AFSK1200_TX_OK;
}
