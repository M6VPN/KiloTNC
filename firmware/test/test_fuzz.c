/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/test/test_fuzz.c */

#include <sys/types.h>

#include <stdint.h>

#include "ax25.h"
#include "fcs.h"
#include "hdlc.h"
#include "kiss.h"

#define CHECK(expr) do { if (!(expr)) return __LINE__; } while (0)
#define FUZZ_ITERS	10000U
#define FUZZ_MAX_LEN	256U

static void fuzz_fill(uint32_t *, uint8_t *, size_t);
static uint32_t fuzz_next(uint32_t *);
static int fuzz_ax25(uint32_t *);
static int fuzz_fcs(uint32_t *);
static int fuzz_hdlc(uint32_t *);
static int fuzz_kiss(uint32_t *);

int
test_fuzz(void)
{
	uint32_t state;

	state = 0x4B544E43U;
	CHECK(fuzz_kiss(&state) == 0);
	CHECK(fuzz_hdlc(&state) == 0);
	CHECK(fuzz_ax25(&state) == 0);
	CHECK(fuzz_fcs(&state) == 0);

	return 0;
}

static int
fuzz_ax25(uint32_t *state)
{
	struct ax25_frame frame;
	uint8_t data[KILOTNC_AX25_MAX_FRAME + 8U];
	size_t i;
	size_t len;

	for (i = 0U; i < FUZZ_ITERS; i++) {
		len = (size_t)(fuzz_next(state) %
		    (uint32_t)(sizeof(data) + 1U));
		fuzz_fill(state, data, len);
		(void)ax25_decode_ui(data, len, &frame);
		(void)ax25_decode_ui_fcs(data, len, &frame);
	}

	return 0;
}

static void
fuzz_fill(uint32_t *state, uint8_t *buf, size_t len)
{
	size_t i;

	for (i = 0U; i < len; i++)
		buf[i] = (uint8_t)(fuzz_next(state) & 0xFFU);
}

static int
fuzz_fcs(uint32_t *state)
{
	uint8_t data[FUZZ_MAX_LEN];
	size_t i;
	size_t len;

	for (i = 0U; i < FUZZ_ITERS; i++) {
		len = (size_t)(fuzz_next(state) % (uint32_t)(sizeof(data) + 1U));
		fuzz_fill(state, data, len);
		(void)fcs_validate_ax25(data, len);
		(void)fcs_ax25(data, len);
	}

	return 0;
}

static int
fuzz_hdlc(uint32_t *state)
{
	uint8_t data[FUZZ_MAX_LEN];
	uint8_t out[FUZZ_MAX_LEN];
	size_t i;
	size_t len;
	size_t out_bits;

	for (i = 0U; i < FUZZ_ITERS; i++) {
		len = (size_t)(fuzz_next(state) % (uint32_t)(sizeof(data) + 1U));
		fuzz_fill(state, data, len);
		(void)hdlc_unstuff(data, len * 8U, out, sizeof(out), &out_bits);
	}

	return 0;
}

static int
fuzz_kiss(uint32_t *state)
{
	struct kiss_parser parser;
	struct kiss_frame frames[4];
	uint8_t data[FUZZ_MAX_LEN];
	size_t i;
	size_t len;
	size_t frame_count;

	for (i = 0U; i < FUZZ_ITERS; i++) {
		len = (size_t)(fuzz_next(state) % (uint32_t)(sizeof(data) + 1U));
		fuzz_fill(state, data, len);
		kiss_parser_init(&parser);
		(void)kiss_parse_bytes(&parser, data, len, frames, 4U,
		    &frame_count);
	}

	return 0;
}

static uint32_t
fuzz_next(uint32_t *state)
{
	uint32_t x;

	x = *state;
	x ^= x << 13U;
	x ^= x >> 17U;
	x ^= x << 5U;
	*state = x;

	return x;
}
