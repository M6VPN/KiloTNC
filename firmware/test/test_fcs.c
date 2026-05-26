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
	uint8_t in_place[32];
	size_t frame_len;
	enum fcs_result res;

	CHECK(fcs_ax25(NULL, 0U) == 0x0000U);
	CHECK(fcs_ax25(NULL, 1U) == 0x0000U);
	CHECK(fcs_ax25(vector, sizeof(vector)) == 0x906EU);
	CHECK(fcs_validate_ax25(NULL, 0U) == false);
	CHECK(fcs_validate_ax25(frame, 0U) == false);
	CHECK(fcs_validate_ax25(frame, 1U) == false);

	CHECK(fcs_ax25(vector, sizeof(vector)) == 0x906EU);

	res = fcs_append_ax25(vector, sizeof(vector), frame, sizeof(frame),
	    &frame_len);
	CHECK(res == FCS_OK);
	CHECK(frame_len == sizeof(vector) + 2U);
	CHECK(fcs_validate_ax25(frame, frame_len));
	CHECK(memcmp(frame, vector, sizeof(vector)) == 0);

	(void)memcpy(in_place, vector, sizeof(vector));
	res = fcs_append_ax25(in_place, sizeof(vector), in_place,
	    sizeof(in_place), &frame_len);
	CHECK(res == FCS_OK);
	CHECK(frame_len == sizeof(vector) + 2U);
	CHECK(fcs_validate_ax25(in_place, frame_len));

	res = fcs_append_ax25(vector, sizeof(vector), frame, sizeof(vector),
	    &frame_len);
	CHECK(res == FCS_ERR_SMALL);
	CHECK(frame_len == 0U);

	res = fcs_append_ax25(NULL, 1U, frame, sizeof(frame), &frame_len);
	CHECK(res == FCS_ERR_ARG);
	res = fcs_append_ax25(vector, sizeof(vector), NULL, sizeof(frame),
	    &frame_len);
	CHECK(res == FCS_ERR_ARG);
	res = fcs_append_ax25(vector, sizeof(vector), frame, sizeof(frame),
	    NULL);
	CHECK(res == FCS_ERR_ARG);

	res = fcs_append_ax25(vector, sizeof(vector), frame, sizeof(frame),
	    &frame_len);
	CHECK(res == FCS_OK);
	frame[0] ^= 0x01U;
	CHECK(!fcs_validate_ax25(frame, frame_len));

	return 0;
}
