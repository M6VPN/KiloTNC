/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/daemon/kilotncd_radio.h */

#ifndef KILOTNCD_RADIO_H
#define KILOTNCD_RADIO_H

#include <sys/types.h>

#define KILOTNCD_RADIO_PATH_MAX	512U

enum kilotncd_radio_result {
	KILOTNCD_RADIO_OK = 0,
	KILOTNCD_RADIO_ERR_ARG,
	KILOTNCD_RADIO_ERR_IO,
	KILOTNCD_RADIO_ERR_UNSUPPORTED,
	KILOTNCD_RADIO_ERR_RANGE
};

enum kilotncd_radio_backend {
	KILOTNCD_RADIO_BACKEND_NONE = 0,
	KILOTNCD_RADIO_BACKEND_SIM,
	KILOTNCD_RADIO_BACKEND_LOG,
	KILOTNCD_RADIO_BACKEND_SERIAL_RTS,
	KILOTNCD_RADIO_BACKEND_SERIAL_DTR,
	KILOTNCD_RADIO_BACKEND_CAT,
	KILOTNCD_RADIO_BACKEND_GPIO
};

enum kilotncd_radio_ptt {
	KILOTNCD_RADIO_PTT_OFF = 0,
	KILOTNCD_RADIO_PTT_ON
};

struct kilotncd_radio_config {
	enum kilotncd_radio_backend backend;
	char path[KILOTNCD_RADIO_PATH_MAX];
};

struct kilotncd_radio {
	struct kilotncd_radio_config config;
	enum kilotncd_radio_ptt ptt;
	int opened;
};

enum kilotncd_radio_result kilotncd_radio_backend_name(
	enum kilotncd_radio_backend, const char **);
enum kilotncd_radio_result kilotncd_radio_close(struct kilotncd_radio *);
enum kilotncd_radio_result kilotncd_radio_get_ptt(
	const struct kilotncd_radio *, enum kilotncd_radio_ptt *);
enum kilotncd_radio_result kilotncd_radio_open(struct kilotncd_radio *,
	const struct kilotncd_radio_config *);
enum kilotncd_radio_result kilotncd_radio_parse_backend(const char *,
	enum kilotncd_radio_backend *);
enum kilotncd_radio_result kilotncd_radio_set_ptt(struct kilotncd_radio *,
	enum kilotncd_radio_ptt);

#endif
