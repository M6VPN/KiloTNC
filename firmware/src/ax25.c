/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/src/ax25.c */

#include <sys/types.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "ax25.h"
#include "fcs.h"

#define AX25_SSID_RESERVED	0x60U
#define AX25_SSID_REPEATED	0x80U
#define AX25_SSID_MASK		0x1EU
#define AX25_ADDR_EXT		0x01U

static enum ax25_result ax25_decode_addr(const uint8_t *,
	struct ax25_addr *);
static enum ax25_result ax25_encode_addr(const struct ax25_addr *, bool,
	uint8_t *);
static enum ax25_result ax25_check_addr(const struct ax25_addr *);
static bool ax25_is_call_char(char);
static size_t ax25_call_len(const char *);

enum ax25_result
ax25_decode_ui(const uint8_t *frame, size_t frame_len,
	struct ax25_frame *out)
{
	enum ax25_result res;
	size_t addr_count;
	size_t info_len;
	size_t off;
	bool found_ext;

	if (frame == NULL || out == NULL)
		return AX25_ERR_ARG;
	if (frame_len < ((KILOTNC_AX25_MIN_ADDRS * KILOTNC_AX25_ADDR_LEN) + 2U))
		return AX25_ERR_BAD_LEN;

	(void)memset(out, 0, sizeof(*out));
	addr_count = 0U;
	off = 0U;
	found_ext = false;

	while (off + KILOTNC_AX25_ADDR_LEN <= frame_len) {
		if (addr_count >= KILOTNC_AX25_MAX_ADDRS)
			return AX25_ERR_MALFORMED;
		if (off + KILOTNC_AX25_ADDR_LEN + 2U > frame_len)
			return AX25_ERR_MALFORMED;

		if (addr_count == 0U)
			res = ax25_decode_addr(&frame[off], &out->dst);
		else if (addr_count == 1U)
			res = ax25_decode_addr(&frame[off], &out->src);
		else
			res = ax25_decode_addr(&frame[off],
			    &out->digis[addr_count - KILOTNC_AX25_MIN_ADDRS]);
		if (res != AX25_OK)
			return res;

		addr_count++;
		off += KILOTNC_AX25_ADDR_LEN;
		if ((frame[off - 1U] & AX25_ADDR_EXT) != 0U) {
			found_ext = true;
			break;
		}
	}

	if (addr_count < KILOTNC_AX25_MIN_ADDRS)
		return AX25_ERR_MALFORMED;
	if (!found_ext)
		return AX25_ERR_MALFORMED;
	if (addr_count > KILOTNC_AX25_MIN_ADDRS)
		out->ndigis = addr_count - KILOTNC_AX25_MIN_ADDRS;
	if (off + 2U > frame_len)
		return AX25_ERR_BAD_LEN;
	if (frame[off] != AX25_CONTROL_UI)
		return AX25_ERR_MALFORMED;

	out->pid = frame[off + 1U];
	off += 2U;
	info_len = frame_len - off;
	if (info_len > KILOTNC_AX25_MAX_INFO)
		return AX25_ERR_BAD_LEN;
	if (info_len != 0U)
		(void)memcpy(out->info, &frame[off], info_len);
	out->info_len = info_len;

	return AX25_OK;
}

static enum ax25_result
ax25_decode_addr(const uint8_t *raw, struct ax25_addr *addr)
{
	size_t i;
	size_t out;
	char c;
	bool padding;

	(void)memset(addr, 0, sizeof(*addr));
	out = 0U;
	padding = false;

	for (i = 0U; i < KILOTNC_AX25_MAX_CALLSIGN; i++) {
		c = (char)(raw[i] >> 1U);
		if (c == ' ') {
			padding = true;
			continue;
		}
		if (padding || !ax25_is_call_char(c))
			return AX25_ERR_BAD_CALL;
		if (out >= KILOTNC_AX25_MAX_CALLSIGN)
			return AX25_ERR_BAD_CALL;
		addr->callsign[out] = c;
		out++;
	}

	if (out == 0U)
		return AX25_ERR_BAD_CALL;

	addr->callsign[out] = '\0';
	addr->ssid = (uint8_t)((raw[6] & AX25_SSID_MASK) >> 1U);
	addr->repeated = (raw[6] & AX25_SSID_REPEATED) != 0U;

	return AX25_OK;
}

enum ax25_result
ax25_decode_ui_fcs(const uint8_t *frame, size_t frame_len,
	struct ax25_frame *out)
{
	if (!fcs_validate_ax25(frame, frame_len))
		return AX25_ERR_BAD_FCS;

	return ax25_decode_ui(frame, frame_len - 2U, out);
}

static enum ax25_result
ax25_encode_addr(const struct ax25_addr *addr, bool last, uint8_t *out)
{
	size_t i;
	size_t len;
	enum ax25_result res;

	res = ax25_check_addr(addr);
	if (res != AX25_OK)
		return res;

	len = ax25_call_len(addr->callsign);
	for (i = 0U; i < KILOTNC_AX25_MAX_CALLSIGN; i++) {
		if (i < len)
			out[i] = (uint8_t)((uint8_t)addr->callsign[i] << 1U);
		else
			out[i] = (uint8_t)(' ' << 1U);
	}

	out[6] = (uint8_t)(AX25_SSID_RESERVED | ((addr->ssid & 0x0FU) << 1U));
	if (addr->repeated)
		out[6] |= AX25_SSID_REPEATED;
	if (last)
		out[6] |= AX25_ADDR_EXT;

	return AX25_OK;
}

enum ax25_result
ax25_encode_ui(const struct ax25_frame *frame, uint8_t *out,
	size_t out_cap, size_t *out_len)
{
	enum ax25_result res;
	size_t i;
	size_t needed;
	size_t off;

	if (out_len == NULL)
		return AX25_ERR_ARG;
	*out_len = 0U;

	if (frame == NULL || out == NULL)
		return AX25_ERR_ARG;
	res = ax25_check_addr(&frame->dst);
	if (res != AX25_OK)
		return res;
	res = ax25_check_addr(&frame->src);
	if (res != AX25_OK)
		return res;
	if (frame->ndigis > KILOTNC_AX25_MAX_DIGIS ||
	    frame->info_len > KILOTNC_AX25_MAX_INFO)
		return AX25_ERR_BAD_LEN;

	for (i = 0U; i < frame->ndigis; i++) {
		res = ax25_check_addr(&frame->digis[i]);
		if (res != AX25_OK)
			return res;
	}

	needed = ((KILOTNC_AX25_MIN_ADDRS + frame->ndigis) *
	    KILOTNC_AX25_ADDR_LEN) + 2U + frame->info_len;
	if (needed > out_cap)
		return AX25_ERR_SMALL;

	off = 0U;
	res = ax25_encode_addr(&frame->dst, false, &out[off]);
	if (res != AX25_OK)
		return res;
	off += KILOTNC_AX25_ADDR_LEN;

	res = ax25_encode_addr(&frame->src, frame->ndigis == 0U, &out[off]);
	if (res != AX25_OK)
		return res;
	off += KILOTNC_AX25_ADDR_LEN;

	for (i = 0U; i < frame->ndigis; i++) {
		res = ax25_encode_addr(&frame->digis[i],
		    i + 1U == frame->ndigis, &out[off]);
		if (res != AX25_OK)
			return res;
		off += KILOTNC_AX25_ADDR_LEN;
	}

	out[off] = AX25_CONTROL_UI;
	out[off + 1U] = frame->pid;
	off += 2U;
	if (frame->info_len != 0U)
		(void)memcpy(&out[off], frame->info, frame->info_len);
	off += frame->info_len;
	*out_len = off;

	return AX25_OK;
}

enum ax25_result
ax25_encode_ui_fcs(const struct ax25_frame *frame, uint8_t *out,
	size_t out_cap, size_t *out_len)
{
	enum ax25_result res;
	size_t no_fcs_len;

	if (out_len == NULL)
		return AX25_ERR_ARG;
	*out_len = 0U;

	res = ax25_encode_ui(frame, out, out_cap, &no_fcs_len);
	if (res != AX25_OK)
		return res;

	if (out_cap - no_fcs_len < 2U)
		return AX25_ERR_SMALL;

	return fcs_append_ax25(out, no_fcs_len, out, out_cap, out_len) ==
	    FCS_OK ? AX25_OK : AX25_ERR_SMALL;
}

static bool
ax25_is_call_char(char c)
{
	return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

static enum ax25_result
ax25_check_addr(const struct ax25_addr *addr)
{
	size_t i;
	size_t len;

	if (addr == NULL)
		return AX25_ERR_ARG;
	if (addr->ssid > 15U)
		return AX25_ERR_BAD_SSID;

	len = ax25_call_len(addr->callsign);
	if (len == 0U || len > KILOTNC_AX25_MAX_CALLSIGN)
		return AX25_ERR_BAD_CALL;

	for (i = 0U; i < len; i++) {
		if (!ax25_is_call_char(addr->callsign[i]))
			return AX25_ERR_BAD_CALL;
	}

	return AX25_OK;
}

bool
ax25_is_valid_addr(const struct ax25_addr *addr)
{
	return ax25_check_addr(addr) == AX25_OK;
}

static size_t
ax25_call_len(const char *callsign)
{
	size_t len;

	if (callsign == NULL)
		return 0U;

	for (len = 0U; len < KILOTNC_AX25_MAX_CALLSIGN + 1U; len++) {
		if (callsign[len] == '\0')
			return len;
	}

	return KILOTNC_AX25_MAX_CALLSIGN + 1U;
}
