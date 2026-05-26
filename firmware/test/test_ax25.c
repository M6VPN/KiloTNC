/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/test/test_ax25.c */

#include <sys/types.h>

#include <stdint.h>
#include <string.h>

#include "ax25.h"

#define CHECK(expr) do { if (!(expr)) return __LINE__; } while (0)

static void test_fill_addr(struct ax25_addr *, const char *, uint8_t);
static void test_fill_frame(struct ax25_frame *);

int
test_ax25(void)
{
	struct ax25_frame frame;
	struct ax25_frame decoded;
	uint8_t raw[KILOTNC_AX25_MAX_FRAME];
	size_t raw_len;
	enum ax25_result res;

	test_fill_frame(&frame);
	res = ax25_encode_ui_fcs(&frame, raw, sizeof(raw), &raw_len);
	CHECK(res == AX25_OK);
	res = ax25_decode_ui_fcs(raw, raw_len, &decoded);
	CHECK(res == AX25_OK);
	CHECK(strcmp(decoded.dst.callsign, "APRS") == 0);
	CHECK(decoded.dst.ssid == 0U);
	CHECK(strcmp(decoded.src.callsign, "N0CALL") == 0);
	CHECK(decoded.src.ssid == 7U);
	CHECK(decoded.pid == AX25_PID_NONE);
	CHECK(decoded.info_len == 5U);
	CHECK(memcmp(decoded.info, "hello", 5U) == 0);

	test_fill_addr(&frame.dst, "aprs", 0U);
	res = ax25_encode_ui(&frame, raw, sizeof(raw), &raw_len);
	CHECK(res == AX25_ERR_BAD_CALL);

	test_fill_addr(&frame.dst, "APRS", 16U);
	res = ax25_encode_ui(&frame, raw, sizeof(raw), &raw_len);
	CHECK(res == AX25_ERR_BAD_SSID);

	test_fill_frame(&frame);
	res = ax25_encode_ui_fcs(&frame, raw, sizeof(raw), &raw_len);
	CHECK(res == AX25_OK);
	raw[raw_len - 3U] ^= 0x01U;
	res = ax25_decode_ui_fcs(raw, raw_len, &decoded);
	CHECK(res == AX25_ERR_BAD_FCS);

	test_fill_frame(&frame);
	test_fill_addr(&frame.digis[0], "WIDE1", 1U);
	frame.ndigis = 1U;
	res = ax25_encode_ui(&frame, raw, sizeof(raw), &raw_len);
	CHECK(res == AX25_OK);
	CHECK((raw[6] & 0x01U) == 0U);
	CHECK((raw[13] & 0x01U) == 0U);
	CHECK((raw[20] & 0x01U) == 1U);
	res = ax25_decode_ui(raw, raw_len, &decoded);
	CHECK(res == AX25_OK);
	CHECK(decoded.ndigis == 1U);
	CHECK(strcmp(decoded.digis[0].callsign, "WIDE1") == 0);

	raw[20] &= (uint8_t)~0x01U;
	res = ax25_decode_ui(raw, raw_len, &decoded);
	CHECK(res == AX25_ERR_BAD_LEN || res == AX25_ERR_MALFORMED);

	return 0;
}

static void
test_fill_addr(struct ax25_addr *addr, const char *callsign, uint8_t ssid)
{
	(void)memset(addr, 0, sizeof(*addr));
	(void)memcpy(addr->callsign, callsign, strlen(callsign) + 1U);
	addr->ssid = ssid;
	addr->repeated = false;
}

static void
test_fill_frame(struct ax25_frame *frame)
{
	(void)memset(frame, 0, sizeof(*frame));
	test_fill_addr(&frame->dst, "APRS", 0U);
	test_fill_addr(&frame->src, "N0CALL", 7U);
	frame->pid = AX25_PID_NONE;
	(void)memcpy(frame->info, "hello", 5U);
	frame->info_len = 5U;
}
