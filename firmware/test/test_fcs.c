/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/test/test_fcs.c */

#include <sys/types.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "fcs.h"

#define CHECK(expr) do { if (!(expr)) return __LINE__; } while (0)

int
test_fcs(void)
{
	const uint8_t vector[] = {
		'1', '2', '3', '4', '5', '6', '7', '8', '9'
	};
	uint8_t frame[32];
	size_t frame_len;
	enum fcs_result res;

	CHECK(fcs_ax25(vector, sizeof(vector)) == 0x906EU);

	res = fcs_append_ax25(vector, sizeof(vector), frame, sizeof(frame),
	    &frame_len);
	CHECK(res == FCS_OK);
	CHECK(frame_len == sizeof(vector) + 2U);
	CHECK(fcs_validate_ax25(frame, frame_len));

	frame[0] ^= 0x01U;
	CHECK(!fcs_validate_ax25(frame, frame_len));

	return 0;
}
