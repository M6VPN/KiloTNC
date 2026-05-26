/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/test/test_hdlc.c */

#include <sys/types.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "hdlc.h"

#define CHECK(expr) do { if (!(expr)) return __LINE__; } while (0)

static bool test_get_bit(const uint8_t *, size_t);
static bool test_bits_equal(const uint8_t *, const uint8_t *, size_t);

int
test_hdlc(void)
{
	const uint8_t five_ones[] = { 0x1FU };
	const uint8_t payload[] = { 0x5FU, 0xA3U, 0x01U };
	const uint8_t invalid[] = { 0x3FU };
	uint8_t stuffed[16];
	uint8_t unstuffed[16];
	size_t stuffed_bits;
	size_t unstuffed_bits;
	enum hdlc_result res;

	res = hdlc_bitstuff(five_ones, 5U, stuffed, sizeof(stuffed),
	    &stuffed_bits);
	CHECK(res == HDLC_OK);
	CHECK(stuffed_bits == 6U);
	CHECK(!test_get_bit(stuffed, 5U));

	res = hdlc_bitstuff(payload, 17U, stuffed, sizeof(stuffed),
	    &stuffed_bits);
	CHECK(res == HDLC_OK);
	res = hdlc_unstuff(stuffed, stuffed_bits, unstuffed,
	    sizeof(unstuffed), &unstuffed_bits);
	CHECK(res == HDLC_OK);
	CHECK(unstuffed_bits == 17U);
	CHECK(test_bits_equal(payload, unstuffed, 17U));

	res = hdlc_bitstuff(payload, 17U, stuffed, 1U, &stuffed_bits);
	CHECK(res == HDLC_ERR_SMALL);

	res = hdlc_unstuff(invalid, 6U, unstuffed, sizeof(unstuffed),
	    &unstuffed_bits);
	CHECK(res == HDLC_ERR_MALFORMED);

	return 0;
}

static bool
test_bits_equal(const uint8_t *a, const uint8_t *b, size_t bits)
{
	size_t i;

	for (i = 0U; i < bits; i++) {
		if (test_get_bit(a, i) != test_get_bit(b, i))
			return false;
	}

	return true;
}

static bool
test_get_bit(const uint8_t *buf, size_t bit)
{
	return ((buf[bit / 8U] >> (bit % 8U)) & 1U) != 0U;
}
