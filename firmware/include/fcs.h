/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/include/fcs.h */

#ifndef FCS_H
#define FCS_H

#include <sys/types.h>

#include <stdbool.h>
#include <stdint.h>

enum fcs_result {
	FCS_OK = 0,
	FCS_ERR_ARG,
	FCS_ERR_SMALL,
	FCS_ERR_BAD
};

uint16_t fcs_ax25(const uint8_t *, size_t);
enum fcs_result fcs_append_ax25(const uint8_t *, size_t, uint8_t *,
	size_t, size_t *);
bool fcs_validate_ax25(const uint8_t *, size_t);

#endif
