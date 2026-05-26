/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/src/tnc_mode.c */

#include <sys/types.h>

#include <stdint.h>
#include <string.h>

#include "tnc_mode.h"

#define TNC_MODE_OPTION_NINO		"NINO_MODE="
#define TNC_MODE_OPTION_KILOTNC		"KILOTNC_MODE="
#define TNC_MODE_SLUG_1200_AFSK_AX25	"1200-afsk-ax25"

static const struct tnc_mode_desc tnc_mode_table[] = {
	{
		TNC_MODE_9600_GFSK_AX25, TNC_MODE_SUPPORT_PLANNED,
		0U, 0x00U, 0U, 16U, 9600U, 9600U,
		"9600 GFSK AX.25", "GFSK", "AX.25", "FM data-radio"
	},
	{
		TNC_MODE_19200_4FSK_IL2PC, TNC_MODE_SUPPORT_RESEARCH,
		1U, 0x41U, 1U, 17U, 19200U, 19200U,
		"19200 4FSK IL2Pc", "4FSK", "IL2Pc", "FM data-radio"
	},
	{
		TNC_MODE_9600_GFSK_IL2PC, TNC_MODE_SUPPORT_RESEARCH,
		2U, 0xB0U, 2U, 18U, 9600U, 9600U,
		"9600 GFSK IL2Pc", "GFSK", "IL2Pc", "FM data-radio"
	},
	{
		TNC_MODE_9600_4FSK_IL2PC, TNC_MODE_SUPPORT_RESEARCH,
		3U, 0x40U, 3U, 19U, 9600U, 9600U,
		"9600 4FSK IL2Pc", "4FSK", "IL2Pc", "FM data-radio"
	},
	{
		TNC_MODE_4800_GFSK_IL2PC, TNC_MODE_SUPPORT_RESEARCH,
		4U, 0xA3U, 4U, 20U, 4800U, 4800U,
		"4800 GFSK IL2Pc", "GFSK", "IL2Pc", "FM data-radio"
	},
	{
		TNC_MODE_3600_QPSK_IL2PC, TNC_MODE_SUPPORT_RESEARCH,
		5U, 0xF1U, 5U, 21U, 1800U, 3600U,
		"3600 QPSK IL2Pc", "QPSK", "IL2Pc", "FM mic/speaker"
	},
	{
		TNC_MODE_1200_AFSK_AX25, TNC_MODE_SUPPORT_IMPLEMENTED,
		6U, 0x02U, 6U, 22U, 1200U, 1200U,
		"1200 AFSK AX.25", "AFSK", "AX.25", "FM mic/speaker"
	},
	{
		TNC_MODE_1200_AFSK_IL2PC, TNC_MODE_SUPPORT_RESEARCH,
		7U, 0x93U, 7U, 23U, 1200U, 1200U,
		"1200 AFSK IL2Pc", "AFSK", "IL2Pc", "FM mic/speaker"
	},
	{
		TNC_MODE_300_BPSK_IL2PC, TNC_MODE_SUPPORT_RESEARCH,
		8U, 0x91U, 8U, 24U, 300U, 300U,
		"300 BPSK IL2Pc", "BPSK", "IL2Pc", "SSB HF"
	},
	{
		TNC_MODE_600_QPSK_IL2PC, TNC_MODE_SUPPORT_RESEARCH,
		9U, 0x92U, 9U, 25U, 300U, 600U,
		"600 QPSK IL2Pc", "QPSK", "IL2Pc", "SSB HF"
	},
	{
		TNC_MODE_1200_BPSK_IL2PC, TNC_MODE_SUPPORT_RESEARCH,
		10U, 0xA0U, 10U, 26U, 1200U, 1200U,
		"1200 BPSK IL2Pc", "BPSK", "IL2Pc", "SSB HF"
	},
	{
		TNC_MODE_2400_QPSK_IL2PC, TNC_MODE_SUPPORT_RESEARCH,
		11U, 0xA2U, 11U, 27U, 1200U, 2400U,
		"2400 QPSK IL2Pc", "QPSK", "IL2Pc", "SSB HF"
	},
	{
		TNC_MODE_300_AFSK_AX25, TNC_MODE_SUPPORT_PLANNED,
		12U, 0x31U, 12U, 28U, 300U, 300U,
		"300 AFSK AX.25", "AFSK", "AX.25", "SSB HF"
	},
	{
		TNC_MODE_300_AFSK_IL2P, TNC_MODE_SUPPORT_RESEARCH,
		13U, 0x22U, 13U, 29U, 300U, 300U,
		"300 AFSK IL2P", "AFSK", "IL2P", "SSB HF"
	},
	{
		TNC_MODE_300_AFSK_IL2PC, TNC_MODE_SUPPORT_RESEARCH,
		14U, 0x23U, 14U, 30U, 300U, 300U,
		"300 AFSK IL2Pc", "AFSK", "IL2Pc", "SSB HF"
	},
	{
		TNC_MODE_SET_FROM_KISS, TNC_MODE_SUPPORT_UNSUPPORTED,
		15U, 0xF3U, TNC_MODE_NINO_NONE, TNC_MODE_NINO_NONE,
		0U, 0U, "Set from KISS", "none", "none", "mode switch"
	}
};

static enum tnc_mode_result tnc_mode_from_number(const char *,
	enum tnc_mode_id *, int *);
static const struct tnc_mode_desc *tnc_mode_lookup(enum tnc_mode_id);
static enum tnc_mode_result tnc_mode_parse_uint8(const char *, uint8_t *);

enum tnc_mode_result
tnc_mode_default(enum tnc_mode_id *id)
{
	if (id == NULL)
		return TNC_MODE_ERR_ARG;

	*id = TNC_MODE_1200_AFSK_AX25;
	return TNC_MODE_OK;
}

enum tnc_mode_result
tnc_mode_from_nino_sethw(uint8_t value, enum tnc_mode_id *id,
	int *temporary)
{
	size_t i;

	if (id == NULL || temporary == NULL)
		return TNC_MODE_ERR_ARG;
	*temporary = 0;
	if (value >= 16U) {
		if (value > 30U)
			return TNC_MODE_ERR_RANGE;
		*temporary = 1;
		value = (uint8_t)(value - 16U);
	}
	if (value >= TNC_MODE_NINO_SET_FROM_KISS)
		return TNC_MODE_ERR_UNSUPPORTED;

	for (i = 0U; i < sizeof(tnc_mode_table) / sizeof(tnc_mode_table[0]);
	    i++) {
		if (tnc_mode_table[i].nino_switch_mode == value) {
			*id = tnc_mode_table[i].id;
			return TNC_MODE_OK;
		}
	}

	return TNC_MODE_ERR_RANGE;
}

enum tnc_mode_result
tnc_mode_from_nino_switch(uint8_t value, enum tnc_mode_id *id)
{
	size_t i;

	if (id == NULL)
		return TNC_MODE_ERR_ARG;
	if (value == TNC_MODE_NINO_SET_FROM_KISS) {
		*id = TNC_MODE_SET_FROM_KISS;
		return TNC_MODE_ERR_UNSUPPORTED;
	}
	if (value > TNC_MODE_NINO_SET_FROM_KISS)
		return TNC_MODE_ERR_RANGE;

	for (i = 0U; i < sizeof(tnc_mode_table) / sizeof(tnc_mode_table[0]);
	    i++) {
		if (tnc_mode_table[i].nino_switch_mode == value) {
			*id = tnc_mode_table[i].id;
			return TNC_MODE_OK;
		}
	}

	return TNC_MODE_ERR_RANGE;
}

enum tnc_mode_result
tnc_mode_get(enum tnc_mode_id id, const struct tnc_mode_desc **desc)
{
	if (desc == NULL)
		return TNC_MODE_ERR_ARG;

	*desc = tnc_mode_lookup(id);
	if (*desc == NULL)
		return TNC_MODE_ERR_RANGE;
	return TNC_MODE_OK;
}

enum tnc_mode_result
tnc_mode_name(enum tnc_mode_id id, char *buf, size_t buf_cap,
	size_t *out_len)
{
	const struct tnc_mode_desc *desc;
	size_t len;

	if (buf == NULL || out_len == NULL)
		return TNC_MODE_ERR_ARG;
	*out_len = 0U;
	desc = tnc_mode_lookup(id);
	if (desc == NULL)
		return TNC_MODE_ERR_RANGE;
	len = strlen(desc->name);
	if (buf_cap <= len)
		return TNC_MODE_ERR_SMALL;
	(void)memcpy(buf, desc->name, len + 1U);
	*out_len = len;

	return TNC_MODE_OK;
}

enum tnc_mode_result
tnc_mode_parse_option(const char *option, enum tnc_mode_id *id,
	int *temporary)
{
	const char *value;

	if (option == NULL || id == NULL || temporary == NULL)
		return TNC_MODE_ERR_ARG;
	*temporary = 0;
	if (strncmp(option, TNC_MODE_OPTION_NINO,
	    strlen(TNC_MODE_OPTION_NINO)) == 0) {
		value = option + strlen(TNC_MODE_OPTION_NINO);
		return tnc_mode_from_number(value, id, temporary);
	}
	if (strncmp(option, TNC_MODE_OPTION_KILOTNC,
	    strlen(TNC_MODE_OPTION_KILOTNC)) == 0) {
		value = option + strlen(TNC_MODE_OPTION_KILOTNC);
	} else {
		value = option;
	}
	if (strcmp(value, TNC_MODE_SLUG_1200_AFSK_AX25) == 0) {
		*id = TNC_MODE_1200_AFSK_AX25;
		return TNC_MODE_OK;
	}

	return TNC_MODE_ERR_UNSUPPORTED;
}

enum tnc_mode_result
tnc_mode_to_nino_sethw(enum tnc_mode_id id, int temporary, uint8_t *value)
{
	const struct tnc_mode_desc *desc;

	if (value == NULL)
		return TNC_MODE_ERR_ARG;
	desc = tnc_mode_lookup(id);
	if (desc == NULL)
		return TNC_MODE_ERR_RANGE;
	if (temporary) {
		if (desc->nino_sethw_temporary == TNC_MODE_NINO_NONE)
			return TNC_MODE_ERR_UNSUPPORTED;
		*value = desc->nino_sethw_temporary;
	} else {
		if (desc->nino_sethw_persistent == TNC_MODE_NINO_NONE)
			return TNC_MODE_ERR_UNSUPPORTED;
		*value = desc->nino_sethw_persistent;
	}

	return TNC_MODE_OK;
}

static enum tnc_mode_result
tnc_mode_from_number(const char *text, enum tnc_mode_id *id,
	int *temporary)
{
	uint8_t value;
	enum tnc_mode_result res;

	res = tnc_mode_parse_uint8(text, &value);
	if (res != TNC_MODE_OK)
		return res;
	return tnc_mode_from_nino_sethw(value, id, temporary);
}

static const struct tnc_mode_desc *
tnc_mode_lookup(enum tnc_mode_id id)
{
	size_t i;

	for (i = 0U; i < sizeof(tnc_mode_table) / sizeof(tnc_mode_table[0]);
	    i++) {
		if (tnc_mode_table[i].id == id)
			return &tnc_mode_table[i];
	}

	return NULL;
}

static enum tnc_mode_result
tnc_mode_parse_uint8(const char *text, uint8_t *value)
{
	size_t i;
	unsigned int acc;

	if (text == NULL || value == NULL || text[0] == '\0')
		return TNC_MODE_ERR_ARG;

	acc = 0U;
	for (i = 0U; text[i] != '\0'; i++) {
		if (text[i] < '0' || text[i] > '9')
			return TNC_MODE_ERR_UNSUPPORTED;
		acc = (acc * 10U) + (unsigned int)(text[i] - '0');
		if (acc > 255U)
			return TNC_MODE_ERR_RANGE;
	}
	*value = (uint8_t)acc;

	return TNC_MODE_OK;
}
