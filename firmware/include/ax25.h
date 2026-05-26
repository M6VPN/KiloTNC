/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/include/ax25.h */

#ifndef AX25_H
#define AX25_H

#include <sys/types.h>

#include <stdbool.h>
#include <stdint.h>

#include "kilotnc_limits.h"

#define AX25_CONTROL_UI	0x03U
#define AX25_PID_NONE	0xF0U

enum ax25_result {
	AX25_OK = 0,
	AX25_ERR_ARG,
	AX25_ERR_SMALL,
	AX25_ERR_BAD_CALL,
	AX25_ERR_BAD_SSID,
	AX25_ERR_BAD_LEN,
	AX25_ERR_BAD_FCS,
	AX25_ERR_MALFORMED
};

struct ax25_addr {
	char callsign[KILOTNC_AX25_MAX_CALLSIGN + 1U];
	uint8_t ssid;
	bool repeated;
};

struct ax25_frame {
	struct ax25_addr dst;
	struct ax25_addr src;
	struct ax25_addr digis[KILOTNC_AX25_MAX_DIGIS];
	size_t ndigis;
	uint8_t pid;
	uint8_t info[KILOTNC_AX25_MAX_INFO];
	size_t info_len;
};

enum ax25_result ax25_decode_ui(const uint8_t *, size_t,
	struct ax25_frame *);
enum ax25_result ax25_decode_ui_fcs(const uint8_t *, size_t,
	struct ax25_frame *);
enum ax25_result ax25_encode_ui(const struct ax25_frame *, uint8_t *,
	size_t, size_t *);
enum ax25_result ax25_encode_ui_fcs(const struct ax25_frame *, uint8_t *,
	size_t, size_t *);
bool ax25_is_valid_addr(const struct ax25_addr *);

#endif
