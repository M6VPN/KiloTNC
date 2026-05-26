/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/src/kiss.c */

#include <sys/types.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "kiss.h"

static enum kiss_result kiss_emit_byte(uint8_t *, size_t, size_t *, uint8_t);
static enum kiss_result kiss_finish_frame(struct kiss_parser *,
	struct kiss_frame *, size_t, size_t *);
static void kiss_store_command(struct kiss_parser *, uint8_t, const uint8_t *,
	size_t);

enum kiss_result
kiss_encode_frame(uint8_t port, uint8_t command, const uint8_t *data,
	size_t len, uint8_t *out, size_t out_cap, size_t *out_len)
{
	size_t i;
	size_t pos;
	uint8_t type;
	enum kiss_result res;

	if (out_len == NULL)
		return KISS_ERR_ARG;
	*out_len = 0U;

	if ((data == NULL && len != 0U) || out == NULL || port > 15U)
		return KISS_ERR_ARG;
	if (command != KISS_CMD_RETURN && command > 15U)
		return KISS_ERR_ARG;

	pos = 0U;
	res = kiss_emit_byte(out, out_cap, &pos, KISS_FEND);
	if (res != KISS_OK)
		return res;

	if (command == KISS_CMD_RETURN)
		type = KISS_CMD_RETURN;
	else
		type = (uint8_t)((port << 4U) | (command & 0x0FU));

	res = kiss_emit_byte(out, out_cap, &pos, type);
	if (res != KISS_OK)
		return res;

	for (i = 0U; i < len; i++) {
		if (data[i] == KISS_FEND) {
			res = kiss_emit_byte(out, out_cap, &pos, KISS_FESC);
			if (res != KISS_OK)
				return res;
			res = kiss_emit_byte(out, out_cap, &pos, KISS_TFEND);
		} else if (data[i] == KISS_FESC) {
			res = kiss_emit_byte(out, out_cap, &pos, KISS_FESC);
			if (res != KISS_OK)
				return res;
			res = kiss_emit_byte(out, out_cap, &pos, KISS_TFESC);
		} else {
			res = kiss_emit_byte(out, out_cap, &pos, data[i]);
		}
		if (res != KISS_OK)
			return res;
	}

	res = kiss_emit_byte(out, out_cap, &pos, KISS_FEND);
	if (res != KISS_OK)
		return res;

	*out_len = pos;
	return KISS_OK;
}

static enum kiss_result
kiss_emit_byte(uint8_t *out, size_t out_cap, size_t *pos, uint8_t byte)
{
	if (*pos >= out_cap)
		return KISS_ERR_SMALL;
	out[*pos] = byte;
	(*pos)++;

	return KISS_OK;
}

static enum kiss_result
kiss_finish_frame(struct kiss_parser *parser, struct kiss_frame *frames,
	size_t frame_cap, size_t *frame_count)
{
	struct kiss_frame *frame;
	uint8_t type;
	uint8_t command;
	uint8_t port;
	size_t payload_len;

	if (parser->dropping) {
		parser->dropping = false;
		parser->len = 0U;
		parser->escaped = false;
		return KISS_OK;
	}

	if (parser->len == 0U) {
		parser->escaped = false;
		return KISS_OK;
	}

	type = parser->buf[0];
	if (type == KISS_CMD_RETURN) {
		command = KISS_CMD_RETURN;
		port = 15U;
	} else {
		command = (uint8_t)(type & 0x0FU);
		port = (uint8_t)((type >> 4U) & 0x0FU);
	}

	payload_len = parser->len - 1U;

	if (command == KISS_CMD_DATA) {
		if (*frame_count >= frame_cap)
			return KISS_ERR_SMALL;
		frame = &frames[*frame_count];
		(void)memset(frame, 0, sizeof(*frame));
		frame->port = port;
		frame->command = command;
		frame->len = payload_len;
		if (payload_len != 0U)
			(void)memcpy(frame->data, &parser->buf[1], payload_len);
		(*frame_count)++;
		parser->counters.decoded_frames++;
	} else if (command >= KISS_CMD_TXDELAY &&
	    command <= KISS_CMD_SETHARDWARE) {
		kiss_store_command(parser, command, &parser->buf[1], payload_len);
	} else if (command == KISS_CMD_RETURN) {
	} else {
		parser->counters.ignored_commands++;
	}

	parser->len = 0U;
	parser->escaped = false;
	return KISS_OK;
}

enum kiss_result
kiss_parse_bytes(struct kiss_parser *parser, const uint8_t *data, size_t len,
	struct kiss_frame *frames, size_t frame_cap, size_t *frame_count)
{
	enum kiss_result res;
	size_t i;
	uint8_t byte;
	bool append;

	if (frame_count == NULL)
		return KISS_ERR_ARG;
	*frame_count = 0U;

	if (parser == NULL || (data == NULL && len != 0U) ||
	    (frames == NULL && frame_cap != 0U))
		return KISS_ERR_ARG;

	for (i = 0U; i < len; i++) {
		byte = data[i];
		append = true;
		if (byte == KISS_FEND) {
			res = kiss_finish_frame(parser, frames, frame_cap,
			    frame_count);
			if (res != KISS_OK)
				return res;
			continue;
		}

		if (parser->dropping)
			continue;

		if (parser->escaped) {
			if (byte == KISS_TFEND)
				byte = KISS_FEND;
			else if (byte == KISS_TFESC)
				byte = KISS_FESC;
			else {
				parser->counters.parse_errors++;
				append = false;
			}
			parser->escaped = false;
			if (!append)
				continue;
		} else if (byte == KISS_FESC) {
			parser->escaped = true;
			continue;
		}

		if (parser->len >= sizeof(parser->buf)) {
			parser->dropping = true;
			parser->len = 0U;
			parser->escaped = false;
			parser->counters.overlength_frames++;
			continue;
		}
		parser->buf[parser->len] = byte;
		parser->len++;
	}

	return KISS_OK;
}

void
kiss_parser_init(struct kiss_parser *parser)
{
	if (parser == NULL)
		return;

	(void)memset(parser, 0, sizeof(*parser));
	parser->txdelay = 50U;
	parser->p = 63U;
	parser->slottime = 10U;
	parser->txtail = 0U;
	parser->fullduplex = false;
}

static void
kiss_store_command(struct kiss_parser *parser, uint8_t command,
	const uint8_t *payload, size_t payload_len)
{
	size_t len;

	if (payload_len == 0U && command != KISS_CMD_SETHARDWARE)
		return;

	if (command == KISS_CMD_TXDELAY)
		parser->txdelay = payload[0];
	else if (command == KISS_CMD_P)
		parser->p = payload[0];
	else if (command == KISS_CMD_SLOTTIME)
		parser->slottime = payload[0];
	else if (command == KISS_CMD_TXTAIL)
		parser->txtail = payload[0];
	else if (command == KISS_CMD_FULLDUPLEX)
		parser->fullduplex = payload[0] != 0U;
	else if (command == KISS_CMD_SETHARDWARE) {
		len = payload_len;
		if (len > sizeof(parser->sethw))
			len = sizeof(parser->sethw);
		if (len != 0U)
			(void)memcpy(parser->sethw, payload, len);
		parser->sethw_len = len;
	}
}
