/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/include/hdlc.h */

#ifndef HDLC_H
#define HDLC_H

#include <sys/types.h>

#include <stdint.h>

#include "kilotnc_limits.h"

enum hdlc_result {
	HDLC_OK = 0,
	HDLC_ERR_ARG,
	HDLC_ERR_SMALL,
	HDLC_ERR_MALFORMED
};

enum hdlc_result hdlc_bitstuff(const uint8_t *, size_t, uint8_t *,
	size_t, size_t *);
enum hdlc_result hdlc_unstuff(const uint8_t *, size_t, uint8_t *,
	size_t, size_t *);

#endif
