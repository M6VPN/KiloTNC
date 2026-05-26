/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/src/hdlc.c */

#include <sys/types.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "hdlc.h"

static bool hdlc_get_bit(const uint8_t *, size_t);
static enum hdlc_result hdlc_put_bit(uint8_t *, size_t, size_t *, bool);

enum hdlc_result
hdlc_bitstuff(const uint8_t *in, size_t in_bits, uint8_t *out,
	size_t out_cap, size_t *out_bits)
{
	size_t i;
	size_t pos;
	uint8_t ones;
	bool bit;
	enum hdlc_result res;

	if (out_bits == NULL)
		return HDLC_ERR_ARG;
	*out_bits = 0U;

	if ((in == NULL && in_bits != 0U) || out == NULL)
		return HDLC_ERR_ARG;

	(void)memset(out, 0, out_cap);
	pos = 0U;
	ones = 0U;

	for (i = 0U; i < in_bits; i++) {
		bit = hdlc_get_bit(in, i);
		res = hdlc_put_bit(out, out_cap, &pos, bit);
		if (res != HDLC_OK)
			return res;

		if (bit) {
			ones++;
			if (ones == 5U) {
				res = hdlc_put_bit(out, out_cap, &pos, false);
				if (res != HDLC_OK)
					return res;
				ones = 0U;
			}
		} else {
			ones = 0U;
		}
	}

	*out_bits = pos;
	return HDLC_OK;
}

static bool
hdlc_get_bit(const uint8_t *buf, size_t bit)
{
	return ((buf[bit / 8U] >> (bit % 8U)) & 1U) != 0U;
}

static enum hdlc_result
hdlc_put_bit(uint8_t *buf, size_t cap, size_t *pos, bool bit)
{
	if (*pos / 8U >= cap)
		return HDLC_ERR_SMALL;

	if (bit)
		buf[*pos / 8U] |= (uint8_t)(1U << (*pos % 8U));
	(*pos)++;

	return HDLC_OK;
}

enum hdlc_result
hdlc_unstuff(const uint8_t *in, size_t in_bits, uint8_t *out,
	size_t out_cap, size_t *out_bits)
{
	size_t i;
	size_t pos;
	uint8_t ones;
	bool bit;
	enum hdlc_result res;

	if (out_bits == NULL)
		return HDLC_ERR_ARG;
	*out_bits = 0U;

	if ((in == NULL && in_bits != 0U) || out == NULL)
		return HDLC_ERR_ARG;

	(void)memset(out, 0, out_cap);
	pos = 0U;
	ones = 0U;

	for (i = 0U; i < in_bits; i++) {
		bit = hdlc_get_bit(in, i);
		if (ones == 5U) {
			if (bit)
				return HDLC_ERR_MALFORMED;
			ones = 0U;
			continue;
		}

		res = hdlc_put_bit(out, out_cap, &pos, bit);
		if (res != HDLC_OK)
			return res;

		if (bit)
			ones++;
		else
			ones = 0U;
	}

	*out_bits = pos;
	return HDLC_OK;
}
