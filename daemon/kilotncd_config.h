/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/daemon/kilotncd_config.h */

#ifndef KILOTNCD_CONFIG_H
#define KILOTNCD_CONFIG_H

#include <sys/types.h>

#include <stdint.h>

#include "kilotncd_audio.h"
#include "kilotncd_profile.h"
#include "kilotncd_radio.h"
#include "kilotncd_tcp.h"
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
	char kiss_unix_listen[KILOTNCD_PATH_MAX];
	char pty_path_out[KILOTNCD_PATH_MAX];
	char radio_log[KILOTNCD_PATH_MAX];
	struct kilotncd_tcp_addr kiss_tcp_addr;
	enum kilotncd_audio_backend audio_backend;
	enum kilotncd_profile profile;
	enum kilotncd_radio_backend radio_backend;
	struct kilotncd_audio_format audio_format;
	enum tnc_mode_id mode;
	int mode_temporary;
	uint8_t p;
	uint8_t slottime_10ms;
	uint8_t fullduplex;
	uint8_t kiss_tcp_once;
	uint8_t allow_nonlocal_bind;
	uint8_t kiss_unix_once;
	uint8_t unlink_stale_socket;
	uint8_t kiss_pty;
	uint8_t kiss_pty_once;
	uint32_t max_tx_ms;
	int have_mode;
	int have_kiss_in;
	int have_kiss_out;
	int have_pcm_in;
	int have_pcm_out;
	int have_kiss_tcp_listen;
	int have_kiss_unix_listen;
	int have_pty_path_out;
	int have_profile;
	int have_radio_log;
};

enum kilotncd_config_result kilotncd_config_apply_arg(
	struct kilotncd_config *, const char *, const char *);
enum kilotncd_config_result kilotncd_config_init(
	struct kilotncd_config *);
enum kilotncd_config_result kilotncd_config_load_file(
	struct kilotncd_config *, const char *);

#endif
