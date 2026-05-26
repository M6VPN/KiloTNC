/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/daemon/kilotncd_config.h */

#ifndef KILOTNCD_CONFIG_H
#define KILOTNCD_CONFIG_H

#include <sys/types.h>

#include <stdint.h>

#include "tnc_mode.h"

#define KILOTNCD_PATH_MAX	512U
#define KILOTNCD_LINE_MAX	512U

enum kilotncd_config_result {
	KILOTNCD_CONFIG_OK = 0,
	KILOTNCD_CONFIG_ERR_ARG,
	KILOTNCD_CONFIG_ERR_IO,
	KILOTNCD_CONFIG_ERR_PARSE,
	KILOTNCD_CONFIG_ERR_RANGE,
	KILOTNCD_CONFIG_ERR_UNKNOWN
};

struct kilotncd_config {
	char mode_text[KILOTNCD_LINE_MAX];
	char kiss_in[KILOTNCD_PATH_MAX];
	char kiss_out[KILOTNCD_PATH_MAX];
	char pcm_in[KILOTNCD_PATH_MAX];
	char pcm_out[KILOTNCD_PATH_MAX];
	enum tnc_mode_id mode;
	int mode_temporary;
	uint8_t p;
	uint8_t slottime_10ms;
	uint8_t fullduplex;
	uint32_t max_tx_ms;
	int have_mode;
	int have_kiss_in;
	int have_kiss_out;
	int have_pcm_in;
	int have_pcm_out;
};

enum kilotncd_config_result kilotncd_config_apply_arg(
	struct kilotncd_config *, const char *, const char *);
enum kilotncd_config_result kilotncd_config_init(
	struct kilotncd_config *);
enum kilotncd_config_result kilotncd_config_load_file(
	struct kilotncd_config *, const char *);

#endif
