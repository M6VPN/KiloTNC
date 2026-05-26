/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/daemon/kilotncd_radio_log.c */

#include <sys/types.h>

#include <stdio.h>
#include <string.h>

#include "kilotncd_radio.h"
#include "kilotncd_radio_log.h"

static enum kilotncd_radio_result kilotncd_radio_log_write(
	const struct kilotncd_radio *, enum kilotncd_radio_ptt);

enum kilotncd_radio_result
kilotncd_radio_log_open(struct kilotncd_radio *radio)
{
	FILE *fp;

	if (radio == NULL)
		return KILOTNCD_RADIO_ERR_ARG;
	if (radio->config.path[0] == '\0' ||
	    strcmp(radio->config.path, "-") == 0)
		return KILOTNCD_RADIO_ERR_RANGE;
	fp = fopen(radio->config.path, "w");
	if (fp == NULL)
		return KILOTNCD_RADIO_ERR_IO;
	if (fclose(fp) != 0)
		return KILOTNCD_RADIO_ERR_IO;
	radio->ptt = KILOTNCD_RADIO_PTT_OFF;
	radio->opened = 1;

	return KILOTNCD_RADIO_OK;
}

enum kilotncd_radio_result
kilotncd_radio_log_set_ptt(struct kilotncd_radio *radio,
	enum kilotncd_radio_ptt ptt)
{
	enum kilotncd_radio_result res;

	if (radio == NULL)
		return KILOTNCD_RADIO_ERR_ARG;
	if (ptt != KILOTNCD_RADIO_PTT_OFF &&
	    ptt != KILOTNCD_RADIO_PTT_ON)
		return KILOTNCD_RADIO_ERR_RANGE;
	res = kilotncd_radio_log_write(radio, ptt);
	if (res != KILOTNCD_RADIO_OK)
		return res;
	radio->ptt = ptt;

	return KILOTNCD_RADIO_OK;
}

static enum kilotncd_radio_result
kilotncd_radio_log_write(const struct kilotncd_radio *radio,
	enum kilotncd_radio_ptt ptt)
{
	FILE *fp;
	const char *line;

	if (radio == NULL)
		return KILOTNCD_RADIO_ERR_ARG;
	line = (ptt == KILOTNCD_RADIO_PTT_ON) ? "ptt=on\n" : "ptt=off\n";
	fp = fopen(radio->config.path, "a");
	if (fp == NULL)
		return KILOTNCD_RADIO_ERR_IO;
	if (fputs(line, fp) == EOF) {
		(void)fclose(fp);
		return KILOTNCD_RADIO_ERR_IO;
	}
	if (fclose(fp) != 0)
		return KILOTNCD_RADIO_ERR_IO;

	return KILOTNCD_RADIO_OK;
}
