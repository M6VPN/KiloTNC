/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/include/kiss.h */

#ifndef KISS_H
#define KISS_H

#include <sys/types.h>

#include <stdbool.h>
#include <stdint.h>

#include "kilotnc_limits.h"

#define KISS_FEND	0xC0U
#define KISS_FESC	0xDBU
#define KISS_TFEND	0xDCU
#define KISS_TFESC	0xDDU

enum kiss_command {
	KISS_CMD_DATA		= 0x00U,
	KISS_CMD_TXDELAY	= 0x01U,
	KISS_CMD_P		= 0x02U,
	KISS_CMD_SLOTTIME	= 0x03U,
	KISS_CMD_TXTAIL		= 0x04U,
	KISS_CMD_FULLDUPLEX	= 0x05U,
	KISS_CMD_SETHARDWARE	= 0x06U,
	KISS_CMD_RETURN		= 0xFFU
};

enum kiss_result {
	KISS_OK = 0,
	KISS_ERR_ARG,
	KISS_ERR_SMALL
};

struct kiss_counters {
	size_t parse_errors;
	size_t overlength_frames;
	size_t ignored_commands;
	size_t decoded_frames;
};

struct kiss_frame {
	uint8_t port;
	uint8_t command;
	uint8_t data[KILOTNC_KISS_MAX_PAYLOAD];
	size_t len;
};

struct kiss_parser {
	uint8_t buf[KILOTNC_KISS_MAX_FRAME];
	size_t len;
	bool escaped;
	bool dropping;
	uint8_t txdelay;
	uint8_t p;
	uint8_t slottime;
	uint8_t txtail;
	bool fullduplex;
	uint8_t sethw[KILOTNC_SETHW_MAX_PAYLOAD];
	size_t sethw_len;
	struct kiss_counters counters;
};

enum kiss_result kiss_encode_frame(uint8_t, uint8_t, const uint8_t *,
	size_t, uint8_t *, size_t, size_t *);
enum kiss_result kiss_parse_bytes(struct kiss_parser *, const uint8_t *,
	size_t, struct kiss_frame *, size_t, size_t *);
void kiss_parser_init(struct kiss_parser *);

#endif
