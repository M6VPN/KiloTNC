/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/daemon/kilotncd_radio.c */

#include <sys/types.h>

#include <string.h>

#include "kilotncd_radio.h"
#include "kilotncd_radio_log.h"
#include "kilotncd_radio_none.h"

enum kilotncd_radio_result
kilotncd_radio_backend_name(enum kilotncd_radio_backend backend,
	const char **name)
{
	if (name == NULL)
		return KILOTNCD_RADIO_ERR_ARG;
	if (backend == KILOTNCD_RADIO_BACKEND_NONE) {
		*name = "none";
		return KILOTNCD_RADIO_OK;
	}
	if (backend == KILOTNCD_RADIO_BACKEND_SIM) {
		*name = "sim";
		return KILOTNCD_RADIO_OK;
	}
	if (backend == KILOTNCD_RADIO_BACKEND_LOG) {
		*name = "log";
		return KILOTNCD_RADIO_OK;
	}
	if (backend == KILOTNCD_RADIO_BACKEND_SERIAL_RTS) {
		*name = "serial-rts";
		return KILOTNCD_RADIO_OK;
	}
	if (backend == KILOTNCD_RADIO_BACKEND_SERIAL_DTR) {
		*name = "serial-dtr";
		return KILOTNCD_RADIO_OK;
	}
	if (backend == KILOTNCD_RADIO_BACKEND_CAT) {
		*name = "cat";
		return KILOTNCD_RADIO_OK;
	}
	if (backend == KILOTNCD_RADIO_BACKEND_GPIO) {
		*name = "gpio";
		return KILOTNCD_RADIO_OK;
	}

	return KILOTNCD_RADIO_ERR_UNSUPPORTED;
}

enum kilotncd_radio_result
kilotncd_radio_close(struct kilotncd_radio *radio)
{
	if (radio == NULL)
		return KILOTNCD_RADIO_ERR_ARG;
	radio->opened = 0;
	radio->ptt = KILOTNCD_RADIO_PTT_OFF;

	return KILOTNCD_RADIO_OK;
}

enum kilotncd_radio_result
kilotncd_radio_get_ptt(const struct kilotncd_radio *radio,
	enum kilotncd_radio_ptt *ptt)
{
	if (radio == NULL || ptt == NULL)
		return KILOTNCD_RADIO_ERR_ARG;
	*ptt = radio->ptt;

	return KILOTNCD_RADIO_OK;
}

enum kilotncd_radio_result
kilotncd_radio_open(struct kilotncd_radio *radio,
	const struct kilotncd_radio_config *config)
{
	if (radio == NULL || config == NULL)
		return KILOTNCD_RADIO_ERR_ARG;
	(void)memset(radio, 0, sizeof(*radio));
	radio->config = *config;
	radio->ptt = KILOTNCD_RADIO_PTT_OFF;
	if (config->backend == KILOTNCD_RADIO_BACKEND_NONE ||
	    config->backend == KILOTNCD_RADIO_BACKEND_SIM)
		return kilotncd_radio_none_open(radio);
	if (config->backend == KILOTNCD_RADIO_BACKEND_LOG)
		return kilotncd_radio_log_open(radio);

	return KILOTNCD_RADIO_ERR_UNSUPPORTED;
}

enum kilotncd_radio_result
kilotncd_radio_parse_backend(const char *name,
	enum kilotncd_radio_backend *backend)
{
	if (name == NULL || backend == NULL)
		return KILOTNCD_RADIO_ERR_ARG;
	if (strcmp(name, "none") == 0) {
		*backend = KILOTNCD_RADIO_BACKEND_NONE;
		return KILOTNCD_RADIO_OK;
	}
	if (strcmp(name, "sim") == 0 ||
	    strcmp(name, "simulated") == 0) {
		*backend = KILOTNCD_RADIO_BACKEND_SIM;
		return KILOTNCD_RADIO_OK;
	}
	if (strcmp(name, "log") == 0) {
		*backend = KILOTNCD_RADIO_BACKEND_LOG;
		return KILOTNCD_RADIO_OK;
	}
	if (strcmp(name, "serial-rts") == 0) {
		*backend = KILOTNCD_RADIO_BACKEND_SERIAL_RTS;
		return KILOTNCD_RADIO_ERR_UNSUPPORTED;
	}
	if (strcmp(name, "serial-dtr") == 0) {
		*backend = KILOTNCD_RADIO_BACKEND_SERIAL_DTR;
		return KILOTNCD_RADIO_ERR_UNSUPPORTED;
	}
	if (strcmp(name, "cat") == 0) {
		*backend = KILOTNCD_RADIO_BACKEND_CAT;
		return KILOTNCD_RADIO_ERR_UNSUPPORTED;
	}
	if (strcmp(name, "gpio") == 0) {
		*backend = KILOTNCD_RADIO_BACKEND_GPIO;
		return KILOTNCD_RADIO_ERR_UNSUPPORTED;
	}

	return KILOTNCD_RADIO_ERR_UNSUPPORTED;
}

enum kilotncd_radio_result
kilotncd_radio_set_ptt(struct kilotncd_radio *radio,
	enum kilotncd_radio_ptt ptt)
{
	if (radio == NULL)
		return KILOTNCD_RADIO_ERR_ARG;
	if (!radio->opened)
		return KILOTNCD_RADIO_ERR_ARG;
	if (radio->config.backend == KILOTNCD_RADIO_BACKEND_NONE ||
	    radio->config.backend == KILOTNCD_RADIO_BACKEND_SIM)
		return kilotncd_radio_none_set_ptt(radio, ptt);
	if (radio->config.backend == KILOTNCD_RADIO_BACKEND_LOG)
		return kilotncd_radio_log_set_ptt(radio, ptt);

	return KILOTNCD_RADIO_ERR_UNSUPPORTED;
}
