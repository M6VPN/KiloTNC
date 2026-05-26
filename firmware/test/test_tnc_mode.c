/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/test/test_tnc_mode.c */

#include <sys/types.h>

#include <string.h>

#include "tnc_mode.h"

#define CHECK(expr) do { if (!(expr)) return __LINE__; } while (0)

static int test_tnc_mode_lookup(void);
static int test_tnc_mode_nino_mapping(void);
static int test_tnc_mode_parse_format(void);

int
test_tnc_mode(void)
{
	int subres;

	subres = test_tnc_mode_lookup();
	if (subres != 0)
		return subres;
	subres = test_tnc_mode_nino_mapping();
	if (subres != 0)
		return subres;
	subres = test_tnc_mode_parse_format();
	if (subres != 0)
		return subres;

	return 0;
}

static int
test_tnc_mode_lookup(void)
{
	const struct tnc_mode_desc *desc;
	enum tnc_mode_id id;

	CHECK(tnc_mode_default(NULL) == TNC_MODE_ERR_ARG);
	CHECK(tnc_mode_default(&id) == TNC_MODE_OK);
	CHECK(id == TNC_MODE_1200_AFSK_AX25);
	CHECK(tnc_mode_get(TNC_MODE_1200_AFSK_AX25, &desc) ==
	    TNC_MODE_OK);
	CHECK(desc->support == TNC_MODE_SUPPORT_IMPLEMENTED);
	CHECK(desc->nino_switch_mode == 6U);
	CHECK(desc->nino_brd_swch_mod == 0x02U);
	CHECK(desc->nino_sethw_persistent == 6U);
	CHECK(desc->nino_sethw_temporary == 22U);
	CHECK(desc->baud == 1200U);
	CHECK(desc->bps == 1200U);
	CHECK(strcmp(desc->name, "1200 AFSK AX.25") == 0);

	CHECK(tnc_mode_get(TNC_MODE_9600_GFSK_AX25, &desc) ==
	    TNC_MODE_OK);
	CHECK(desc->support == TNC_MODE_SUPPORT_PLANNED);
	CHECK(desc->nino_switch_mode == 0U);
	CHECK(desc->nino_brd_swch_mod == 0x00U);
	CHECK(tnc_mode_get(TNC_MODE_300_AFSK_IL2PC, &desc) ==
	    TNC_MODE_OK);
	CHECK(desc->support == TNC_MODE_SUPPORT_RESEARCH);
	CHECK(desc->nino_switch_mode == 14U);
	CHECK(desc->nino_brd_swch_mod == 0x23U);
	CHECK(tnc_mode_get(TNC_MODE_UNSUPPORTED, &desc) ==
	    TNC_MODE_ERR_RANGE);
	CHECK(tnc_mode_get(TNC_MODE_1200_AFSK_AX25, NULL) ==
	    TNC_MODE_ERR_ARG);

	return 0;
}

static int
test_tnc_mode_nino_mapping(void)
{
	enum tnc_mode_id id;
	uint8_t value;
	int temporary;

	CHECK(tnc_mode_from_nino_switch(6U, &id) == TNC_MODE_OK);
	CHECK(id == TNC_MODE_1200_AFSK_AX25);
	CHECK(tnc_mode_from_nino_switch(15U, &id) ==
	    TNC_MODE_ERR_UNSUPPORTED);
	CHECK(id == TNC_MODE_SET_FROM_KISS);
	CHECK(tnc_mode_from_nino_switch(16U, &id) == TNC_MODE_ERR_RANGE);
	CHECK(tnc_mode_from_nino_switch(6U, NULL) == TNC_MODE_ERR_ARG);

	CHECK(tnc_mode_from_nino_sethw(6U, &id, &temporary) == TNC_MODE_OK);
	CHECK(id == TNC_MODE_1200_AFSK_AX25);
	CHECK(temporary == 0);
	CHECK(tnc_mode_from_nino_sethw(22U, &id, &temporary) ==
	    TNC_MODE_OK);
	CHECK(id == TNC_MODE_1200_AFSK_AX25);
	CHECK(temporary == 1);
	CHECK(tnc_mode_from_nino_sethw(0U, &id, &temporary) == TNC_MODE_OK);
	CHECK(id == TNC_MODE_9600_GFSK_AX25);
	CHECK(temporary == 0);
	CHECK(tnc_mode_from_nino_sethw(12U, &id, &temporary) ==
	    TNC_MODE_OK);
	CHECK(id == TNC_MODE_300_AFSK_AX25);
	CHECK(temporary == 0);
	CHECK(tnc_mode_from_nino_sethw(15U, &id, &temporary) ==
	    TNC_MODE_ERR_UNSUPPORTED);
	CHECK(tnc_mode_from_nino_sethw(31U, &id, &temporary) ==
	    TNC_MODE_ERR_RANGE);
	CHECK(tnc_mode_from_nino_sethw(6U, NULL, &temporary) ==
	    TNC_MODE_ERR_ARG);
	CHECK(tnc_mode_from_nino_sethw(6U, &id, NULL) == TNC_MODE_ERR_ARG);

	CHECK(tnc_mode_to_nino_sethw(TNC_MODE_1200_AFSK_AX25, 0,
	    &value) == TNC_MODE_OK);
	CHECK(value == 6U);
	CHECK(tnc_mode_to_nino_sethw(TNC_MODE_1200_AFSK_AX25, 1,
	    &value) == TNC_MODE_OK);
	CHECK(value == 22U);
	CHECK(tnc_mode_to_nino_sethw(TNC_MODE_SET_FROM_KISS, 0,
	    &value) == TNC_MODE_ERR_UNSUPPORTED);
	CHECK(tnc_mode_to_nino_sethw(TNC_MODE_UNSUPPORTED, 0,
	    &value) == TNC_MODE_ERR_RANGE);
	CHECK(tnc_mode_to_nino_sethw(TNC_MODE_1200_AFSK_AX25, 0,
	    NULL) == TNC_MODE_ERR_ARG);

	return 0;
}

static int
test_tnc_mode_parse_format(void)
{
	char name[32];
	enum tnc_mode_id id;
	size_t out_len;
	int temporary;

	CHECK(tnc_mode_parse_option("NINO_MODE=6", &id, &temporary) ==
	    TNC_MODE_OK);
	CHECK(id == TNC_MODE_1200_AFSK_AX25);
	CHECK(temporary == 0);
	CHECK(tnc_mode_parse_option("NINO_MODE=22", &id, &temporary) ==
	    TNC_MODE_OK);
	CHECK(id == TNC_MODE_1200_AFSK_AX25);
	CHECK(temporary == 1);
	CHECK(tnc_mode_parse_option("KILOTNC_MODE=1200-afsk-ax25", &id,
	    &temporary) == TNC_MODE_OK);
	CHECK(id == TNC_MODE_1200_AFSK_AX25);
	CHECK(temporary == 0);
	CHECK(tnc_mode_parse_option("1200-afsk-ax25", &id, &temporary) ==
	    TNC_MODE_OK);
	CHECK(id == TNC_MODE_1200_AFSK_AX25);
	CHECK(temporary == 0);
	CHECK(tnc_mode_parse_option("NINO_MODE=x", &id, &temporary) ==
	    TNC_MODE_ERR_UNSUPPORTED);
	CHECK(tnc_mode_parse_option("KILOTNC_MODE=9600-gfsk-ax25", &id,
	    &temporary) == TNC_MODE_ERR_UNSUPPORTED);
	CHECK(tnc_mode_parse_option(NULL, &id, &temporary) ==
	    TNC_MODE_ERR_ARG);
	CHECK(tnc_mode_parse_option("NINO_MODE=6", NULL, &temporary) ==
	    TNC_MODE_ERR_ARG);
	CHECK(tnc_mode_parse_option("NINO_MODE=6", &id, NULL) ==
	    TNC_MODE_ERR_ARG);

	CHECK(tnc_mode_name(TNC_MODE_1200_AFSK_AX25, name, sizeof(name),
	    &out_len) == TNC_MODE_OK);
	CHECK(strcmp(name, "1200 AFSK AX.25") == 0);
	CHECK(out_len == strlen("1200 AFSK AX.25"));
	CHECK(tnc_mode_name(TNC_MODE_1200_AFSK_AX25, name, 4U,
	    &out_len) == TNC_MODE_ERR_SMALL);
	CHECK(tnc_mode_name(TNC_MODE_UNSUPPORTED, name, sizeof(name),
	    &out_len) == TNC_MODE_ERR_RANGE);
	CHECK(tnc_mode_name(TNC_MODE_1200_AFSK_AX25, NULL, sizeof(name),
	    &out_len) == TNC_MODE_ERR_ARG);
	CHECK(tnc_mode_name(TNC_MODE_1200_AFSK_AX25, name, sizeof(name),
	    NULL) == TNC_MODE_ERR_ARG);

	return 0;
}
