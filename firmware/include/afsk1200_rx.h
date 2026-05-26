/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/include/afsk1200_rx.h */

#ifndef AFSK1200_RX_H
#define AFSK1200_RX_H

#include <sys/types.h>

#include <stdint.h>

#include "kilotnc_limits.h"

enum afsk1200_rx_result {
	AFSK1200_RX_OK = 0,
	AFSK1200_RX_ERR_ARG,
	AFSK1200_RX_ERR_SMALL,
	AFSK1200_RX_ERR_BAD_LEN,
	AFSK1200_RX_ERR_NO_FRAME,
	AFSK1200_RX_ERR_PARTIAL
};

struct afsk1200_rx_stats {
	size_t bits_decoded;
	size_t flags_seen;
	size_t frames_seen;
	size_t frames_ok;
	size_t frames_bad_fcs;
	size_t frames_too_large;
	size_t frames_malformed;
	uint16_t dcd_score;
	uint16_t confidence_avg;
};

struct afsk1200_rx_frame {
	uint8_t data[KILOTNC_AX25_MAX_FRAME];
	size_t len;
};

enum afsk1200_rx_result afsk1200_rx_decode_frames(const int16_t *, size_t,
	struct afsk1200_rx_frame *, size_t, size_t *,
	struct afsk1200_rx_stats *);

#endif
