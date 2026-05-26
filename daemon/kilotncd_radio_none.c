/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/daemon/kilotncd_radio_none.c */

#include <sys/types.h>

#include <stddef.h>

#include "kilotncd_radio.h"
#include "kilotncd_radio_none.h"

enum kilotncd_radio_result
kilotncd_radio_none_open(struct kilotncd_radio *radio)
{
	if (radio == NULL)
		return KILOTNCD_RADIO_ERR_ARG;
	radio->ptt = KILOTNCD_RADIO_PTT_OFF;
	radio->opened = 1;

	return KILOTNCD_RADIO_OK;
}

enum kilotncd_radio_result
kilotncd_radio_none_set_ptt(struct kilotncd_radio *radio,
	enum kilotncd_radio_ptt ptt)
{
	if (radio == NULL)
		return KILOTNCD_RADIO_ERR_ARG;
	if (ptt != KILOTNCD_RADIO_PTT_OFF &&
	    ptt != KILOTNCD_RADIO_PTT_ON)
		return KILOTNCD_RADIO_ERR_RANGE;
	radio->ptt = ptt;

	return KILOTNCD_RADIO_OK;
}
