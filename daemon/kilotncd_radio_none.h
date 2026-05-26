/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/daemon/kilotncd_radio_none.h */

#ifndef KILOTNCD_RADIO_NONE_H
#define KILOTNCD_RADIO_NONE_H

#include "kilotncd_radio.h"

enum kilotncd_radio_result kilotncd_radio_none_open(
	struct kilotncd_radio *);
enum kilotncd_radio_result kilotncd_radio_none_set_ptt(
	struct kilotncd_radio *, enum kilotncd_radio_ptt);

#endif
