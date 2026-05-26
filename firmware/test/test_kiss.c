/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/test/test_kiss.c */

#include <sys/types.h>

#include <stdint.h>
#include <string.h>

#include "kiss.h"

#define CHECK(expr) do { if (!(expr)) return __LINE__; } while (0)

static int test_kiss_commands(void);
static int test_kiss_decode(const uint8_t *, size_t, struct kiss_parser *,
	struct kiss_frame *, size_t *);

int
test_kiss(void)
{
	struct kiss_parser parser;
	struct kiss_frame frames[2];
	uint8_t encoded[128];
	uint8_t over[KILOTNC_KISS_MAX_FRAME + 4U];
	size_t encoded_len;
	size_t frame_count;
	size_t i;
	const uint8_t payload[] = { 0x01U, 0x02U, 0x03U };
	const uint8_t escaped_payload[] = { 0x01U, KISS_FEND, KISS_FESC, 0x02U };
	const uint8_t repeated_fend[] = { KISS_FEND, KISS_FEND, KISS_FEND };
	const uint8_t bad_escape[] = {
		KISS_FEND, 0x00U, KISS_FESC, 0x00U, 0x41U, KISS_FEND,
		KISS_FEND, 0x00U, 0x42U, KISS_FEND
	};
	const uint8_t unsupported[] = {
		KISS_FEND, 0x07U, 0x55U, KISS_FEND
	};
	enum kiss_result res;

	kiss_parser_init(&parser);
	res = kiss_encode_frame(0U, KISS_CMD_DATA, payload, sizeof(payload),
	    encoded, sizeof(encoded), &encoded_len);
	CHECK(res == KISS_OK);
	CHECK(test_kiss_decode(encoded, encoded_len, &parser, frames,
	    &frame_count) == 0);
	CHECK(frame_count == 1U);
	CHECK(frames[0].port == 0U);
	CHECK(frames[0].command == KISS_CMD_DATA);
	CHECK(frames[0].len == sizeof(payload));
	CHECK(memcmp(frames[0].data, payload, sizeof(payload)) == 0);

	kiss_parser_init(&parser);
	res = kiss_encode_frame(2U, KISS_CMD_DATA, escaped_payload,
	    sizeof(escaped_payload), encoded, sizeof(encoded), &encoded_len);
	CHECK(res == KISS_OK);
	CHECK(test_kiss_decode(encoded, encoded_len, &parser, frames,
	    &frame_count) == 0);
	CHECK(frame_count == 1U);
	CHECK(frames[0].port == 2U);
	CHECK(frames[0].len == sizeof(escaped_payload));
	CHECK(memcmp(frames[0].data, escaped_payload,
	    sizeof(escaped_payload)) == 0);

	kiss_parser_init(&parser);
	CHECK(test_kiss_decode(repeated_fend, sizeof(repeated_fend), &parser,
	    frames, &frame_count) == 0);
	CHECK(frame_count == 0U);

	kiss_parser_init(&parser);
	CHECK(test_kiss_decode(bad_escape, sizeof(bad_escape), &parser, frames,
	    &frame_count) == 0);
	CHECK(parser.counters.parse_errors == 1U);
	CHECK(frame_count == 2U);
	CHECK(frames[0].len == 1U);
	CHECK(frames[0].data[0] == 0x41U);
	CHECK(frames[1].len == 1U);
	CHECK(frames[1].data[0] == 0x42U);

	kiss_parser_init(&parser);
	for (i = 0U; i < sizeof(over); i++)
		over[i] = 0x55U;
	over[0] = KISS_FEND;
	over[1] = 0x00U;
	over[sizeof(over) - 1U] = KISS_FEND;
	CHECK(test_kiss_decode(over, sizeof(over), &parser, frames,
	    &frame_count) == 0);
	CHECK(frame_count == 0U);
	CHECK(parser.counters.overlength_frames == 1U);

	kiss_parser_init(&parser);
	CHECK(test_kiss_decode(unsupported, sizeof(unsupported), &parser,
	    frames, &frame_count) == 0);
	CHECK(frame_count == 0U);
	CHECK(parser.counters.ignored_commands == 1U);

	CHECK(test_kiss_commands() == 0);

	return 0;
}

static int
test_kiss_commands(void)
{
	struct kiss_parser parser;
	struct kiss_frame frames[2];
	uint8_t encoded[256];
	uint8_t sethw[KILOTNC_SETHW_MAX_PAYLOAD + 16U];
	size_t encoded_len;
	size_t frame_count;
	size_t i;
	enum kiss_result res;

	kiss_parser_init(&parser);
	res = kiss_encode_frame(0U, KISS_CMD_TXDELAY, (const uint8_t *)"\x20",
	    1U, encoded, sizeof(encoded), &encoded_len);
	CHECK(res == KISS_OK);
	CHECK(test_kiss_decode(encoded, encoded_len, &parser, frames,
	    &frame_count) == 0);
	CHECK(parser.txdelay == 0x20U);

	res = kiss_encode_frame(0U, KISS_CMD_P, (const uint8_t *)"\x33", 1U,
	    encoded, sizeof(encoded), &encoded_len);
	CHECK(res == KISS_OK);
	CHECK(test_kiss_decode(encoded, encoded_len, &parser, frames,
	    &frame_count) == 0);
	CHECK(parser.p == 0x33U);

	res = kiss_encode_frame(0U, KISS_CMD_SLOTTIME,
	    (const uint8_t *)"\x04", 1U, encoded, sizeof(encoded),
	    &encoded_len);
	CHECK(res == KISS_OK);
	CHECK(test_kiss_decode(encoded, encoded_len, &parser, frames,
	    &frame_count) == 0);
	CHECK(parser.slottime == 0x04U);

	res = kiss_encode_frame(0U, KISS_CMD_TXTAIL,
	    (const uint8_t *)"\x05", 1U, encoded, sizeof(encoded),
	    &encoded_len);
	CHECK(res == KISS_OK);
	CHECK(test_kiss_decode(encoded, encoded_len, &parser, frames,
	    &frame_count) == 0);
	CHECK(parser.txtail == 0x05U);

	res = kiss_encode_frame(0U, KISS_CMD_FULLDUPLEX,
	    (const uint8_t *)"\x01", 1U, encoded, sizeof(encoded),
	    &encoded_len);
	CHECK(res == KISS_OK);
	CHECK(test_kiss_decode(encoded, encoded_len, &parser, frames,
	    &frame_count) == 0);
	CHECK(parser.fullduplex);

	for (i = 0U; i < sizeof(sethw); i++)
		sethw[i] = (uint8_t)i;
	res = kiss_encode_frame(0U, KISS_CMD_SETHARDWARE, sethw,
	    sizeof(sethw), encoded, sizeof(encoded), &encoded_len);
	CHECK(res == KISS_OK);
	CHECK(test_kiss_decode(encoded, encoded_len, &parser, frames,
	    &frame_count) == 0);
	CHECK(parser.sethw_len == KILOTNC_SETHW_MAX_PAYLOAD);
	CHECK(memcmp(parser.sethw, sethw, KILOTNC_SETHW_MAX_PAYLOAD) == 0);

	res = kiss_encode_frame(0U, KISS_CMD_RETURN, NULL, 0U, encoded,
	    sizeof(encoded), &encoded_len);
	CHECK(res == KISS_OK);
	CHECK(test_kiss_decode(encoded, encoded_len, &parser, frames,
	    &frame_count) == 0);
	CHECK(frame_count == 0U);

	return 0;
}

static int
test_kiss_decode(const uint8_t *data, size_t len, struct kiss_parser *parser,
	struct kiss_frame *frames, size_t *frame_count)
{
	enum kiss_result res;

	res = kiss_parse_bytes(parser, data, len, frames, 2U, frame_count);
	if (res != KISS_OK)
		return __LINE__;

	return 0;
}
