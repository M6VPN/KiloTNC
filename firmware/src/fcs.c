/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/src/fcs.c */

#include <sys/types.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "fcs.h"

#define FCS_AX25_INIT	0xFFFFU
#define FCS_AX25_POLY	0x8408U
#define FCS_AX25_XOROUT	0xFFFFU

/*
 * AX.25 uses the HDLC 16-bit FCS. This implementation uses the reflected
 * CRC-16/X-25 form: init 0xffff, reflected polynomial 0x8408, final xor
 * 0xffff. Appended FCS bytes are low byte first.
 */
uint16_t
fcs_ax25(const uint8_t *data, size_t len)
{
	uint16_t crc;
	uint8_t bit;
	size_t i;

	if (data == NULL && len != 0U)
		return 0U;

	crc = FCS_AX25_INIT;
	for (i = 0U; i < len; i++) {
		crc ^= data[i];
		for (bit = 0U; bit < 8U; bit++) {
			if ((crc & 1U) != 0U)
				crc = (uint16_t)((crc >> 1U) ^ FCS_AX25_POLY);
			else
				crc = (uint16_t)(crc >> 1U);
		}
	}

	return (uint16_t)(crc ^ FCS_AX25_XOROUT);
}

enum fcs_result
fcs_append_ax25(const uint8_t *data, size_t len, uint8_t *out,
	size_t out_cap, size_t *out_len)
{
	uint16_t fcs;

	if (out_len == NULL)
		return FCS_ERR_ARG;
	*out_len = 0U;

	if ((data == NULL && len != 0U) || out == NULL)
		return FCS_ERR_ARG;
	if (len > out_cap || out_cap - len < 2U)
		return FCS_ERR_SMALL;

	if (len != 0U && out != data)
		(void)memmove(out, data, len);

	fcs = fcs_ax25(data, len);
	out[len] = (uint8_t)(fcs & 0xFFU);
	out[len + 1U] = (uint8_t)((fcs >> 8U) & 0xFFU);
	*out_len = len + 2U;

	return FCS_OK;
}

bool
fcs_validate_ax25(const uint8_t *frame, size_t len)
{
	uint16_t actual;
	uint16_t expected;

	if (frame == NULL || len < 2U)
		return false;

	actual = fcs_ax25(frame, len - 2U);
	expected = (uint16_t)frame[len - 2U] |
	    (uint16_t)((uint16_t)frame[len - 1U] << 8U);

	return actual == expected;
}
