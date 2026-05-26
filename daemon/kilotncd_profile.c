/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/daemon/kilotncd_profile.c */

#include <sys/types.h>

#include <stdio.h>
#include <string.h>

#include "kilotncd_audio.h"
#include "kilotncd_config.h"
#include "kilotncd_profile.h"
#include "kilotncd_radio.h"
#include "kilotncd_tcp.h"
#include "tnc_mode.h"

static int kilotncd_profile_binary_stdout(const struct kilotncd_config *);
static int kilotncd_profile_binary_stdin(const struct kilotncd_config *);
static int kilotncd_profile_has_listener(const struct kilotncd_config *);
static int kilotncd_profile_mode_implemented(enum tnc_mode_id);
static enum kilotncd_profile_result kilotncd_profile_validate_common(
	const struct kilotncd_config *);
static enum kilotncd_profile_result kilotncd_profile_validate_file_tx(
	const struct kilotncd_config *);
static enum kilotncd_profile_result kilotncd_profile_validate_file_rx(
	const struct kilotncd_config *);
static enum kilotncd_profile_result kilotncd_profile_validate_file_loopback(
	const struct kilotncd_config *);
static enum kilotncd_profile_result kilotncd_profile_validate_stdio_tx(
	const struct kilotncd_config *);
static enum kilotncd_profile_result kilotncd_profile_validate_stdio_rx(
	const struct kilotncd_config *);
static enum kilotncd_profile_result kilotncd_profile_validate_tcp(
	const struct kilotncd_config *);
static enum kilotncd_profile_result kilotncd_profile_validate_unix(
	const struct kilotncd_config *);
static enum kilotncd_profile_result kilotncd_profile_validate_pty(
	const struct kilotncd_config *);
static enum kilotncd_profile_result kilotncd_profile_validate_status(
	const struct kilotncd_config *);

enum kilotncd_profile_result
kilotncd_profile_apply_defaults(struct kilotncd_config *config)
{
	if (config == NULL)
		return KILOTNCD_PROFILE_ERR_ARG;
	if (config->profile == KILOTNCD_PROFILE_TCP_KISS_ONCE)
		config->kiss_tcp_once = 1U;
	if (config->profile == KILOTNCD_PROFILE_UNIX_KISS_ONCE)
		config->kiss_unix_once = 1U;
	if (config->profile == KILOTNCD_PROFILE_PTY_KISS_ONCE) {
		config->kiss_pty = 1U;
		config->kiss_pty_once = 1U;
	}

	return KILOTNCD_PROFILE_OK;
}

enum kilotncd_profile_result
kilotncd_profile_error(enum kilotncd_profile_result res,
	enum kilotncd_profile profile, char *buf, size_t buf_cap)
{
	const char *profile_name;
	const char *reason;
	int n;

	if (buf == NULL || buf_cap == 0U)
		return KILOTNCD_PROFILE_ERR_ARG;
	if (kilotncd_profile_format(profile, &profile_name) !=
	    KILOTNCD_PROFILE_OK)
		profile_name = "unset";
	reason = "invalid profile";
	if (res == KILOTNCD_PROFILE_ERR_ARG)
		reason = "invalid argument";
	if (res == KILOTNCD_PROFILE_ERR_CONFLICT)
		reason = "conflicting options";
	if (res == KILOTNCD_PROFILE_ERR_MISSING)
		reason = "missing required option";
	if (res == KILOTNCD_PROFILE_ERR_UNSUPPORTED)
		reason = "unsupported setting";
	if (res == KILOTNCD_PROFILE_ERR_UNSAFE)
		reason = "unsafe setting";
	n = snprintf(buf, buf_cap, "profile %s: %s", profile_name, reason);
	if (n < 0 || (size_t)n >= buf_cap)
		return KILOTNCD_PROFILE_ERR_ARG;

	return KILOTNCD_PROFILE_OK;
}

enum kilotncd_profile_result
kilotncd_profile_format(enum kilotncd_profile profile, const char **name)
{
	if (name == NULL)
		return KILOTNCD_PROFILE_ERR_ARG;
	if (profile == KILOTNCD_PROFILE_FILE_TX) {
		*name = "file-tx";
		return KILOTNCD_PROFILE_OK;
	}
	if (profile == KILOTNCD_PROFILE_FILE_RX) {
		*name = "file-rx";
		return KILOTNCD_PROFILE_OK;
	}
	if (profile == KILOTNCD_PROFILE_FILE_LOOPBACK) {
		*name = "file-loopback";
		return KILOTNCD_PROFILE_OK;
	}
	if (profile == KILOTNCD_PROFILE_STDIO_TX) {
		*name = "stdio-tx";
		return KILOTNCD_PROFILE_OK;
	}
	if (profile == KILOTNCD_PROFILE_STDIO_RX) {
		*name = "stdio-rx";
		return KILOTNCD_PROFILE_OK;
	}
	if (profile == KILOTNCD_PROFILE_TCP_KISS_ONCE) {
		*name = "tcp-kiss-once";
		return KILOTNCD_PROFILE_OK;
	}
	if (profile == KILOTNCD_PROFILE_UNIX_KISS_ONCE) {
		*name = "unix-kiss-once";
		return KILOTNCD_PROFILE_OK;
	}
	if (profile == KILOTNCD_PROFILE_PTY_KISS_ONCE) {
		*name = "pty-kiss-once";
		return KILOTNCD_PROFILE_OK;
	}
	if (profile == KILOTNCD_PROFILE_STATUS) {
		*name = "status";
		return KILOTNCD_PROFILE_OK;
	}
	if (profile == KILOTNCD_PROFILE_UNSET) {
		*name = "unset";
		return KILOTNCD_PROFILE_OK;
	}

	return KILOTNCD_PROFILE_ERR_UNSUPPORTED;
}

enum kilotncd_profile_result
kilotncd_profile_infer(const struct kilotncd_config *config,
	enum kilotncd_profile *profile)
{
	if (config == NULL || profile == NULL)
		return KILOTNCD_PROFILE_ERR_ARG;
	if (config->have_kiss_tcp_listen) {
		*profile = KILOTNCD_PROFILE_TCP_KISS_ONCE;
		return KILOTNCD_PROFILE_OK;
	}
	if (config->have_kiss_unix_listen) {
		*profile = KILOTNCD_PROFILE_UNIX_KISS_ONCE;
		return KILOTNCD_PROFILE_OK;
	}
	if (config->kiss_pty != 0U) {
		*profile = KILOTNCD_PROFILE_PTY_KISS_ONCE;
		return KILOTNCD_PROFILE_OK;
	}
	if (config->have_kiss_in && config->have_pcm_out) {
		if (strcmp(config->kiss_in, "-") == 0 ||
		    strcmp(config->pcm_out, "-") == 0)
			*profile = KILOTNCD_PROFILE_STDIO_TX;
		else
			*profile = KILOTNCD_PROFILE_FILE_TX;
		return KILOTNCD_PROFILE_OK;
	}
	if (config->have_pcm_in && config->have_kiss_out) {
		if (strcmp(config->pcm_in, "-") == 0 ||
		    strcmp(config->kiss_out, "-") == 0)
			*profile = KILOTNCD_PROFILE_STDIO_RX;
		else
			*profile = KILOTNCD_PROFILE_FILE_RX;
		return KILOTNCD_PROFILE_OK;
	}
	if (config->have_kiss_in && config->have_kiss_out) {
		*profile = KILOTNCD_PROFILE_FILE_LOOPBACK;
		return KILOTNCD_PROFILE_OK;
	}

	return KILOTNCD_PROFILE_ERR_MISSING;
}

enum kilotncd_profile_result
kilotncd_profile_parse(const char *name, enum kilotncd_profile *profile)
{
	if (name == NULL || profile == NULL)
		return KILOTNCD_PROFILE_ERR_ARG;
	if (strcmp(name, "file-tx") == 0) {
		*profile = KILOTNCD_PROFILE_FILE_TX;
		return KILOTNCD_PROFILE_OK;
	}
	if (strcmp(name, "file-rx") == 0) {
		*profile = KILOTNCD_PROFILE_FILE_RX;
		return KILOTNCD_PROFILE_OK;
	}
	if (strcmp(name, "file-loopback") == 0) {
		*profile = KILOTNCD_PROFILE_FILE_LOOPBACK;
		return KILOTNCD_PROFILE_OK;
	}
	if (strcmp(name, "stdio-tx") == 0) {
		*profile = KILOTNCD_PROFILE_STDIO_TX;
		return KILOTNCD_PROFILE_OK;
	}
	if (strcmp(name, "stdio-rx") == 0) {
		*profile = KILOTNCD_PROFILE_STDIO_RX;
		return KILOTNCD_PROFILE_OK;
	}
	if (strcmp(name, "tcp-kiss-once") == 0) {
		*profile = KILOTNCD_PROFILE_TCP_KISS_ONCE;
		return KILOTNCD_PROFILE_OK;
	}
	if (strcmp(name, "unix-kiss-once") == 0) {
		*profile = KILOTNCD_PROFILE_UNIX_KISS_ONCE;
		return KILOTNCD_PROFILE_OK;
	}
	if (strcmp(name, "pty-kiss-once") == 0) {
		*profile = KILOTNCD_PROFILE_PTY_KISS_ONCE;
		return KILOTNCD_PROFILE_OK;
	}
	if (strcmp(name, "status") == 0) {
		*profile = KILOTNCD_PROFILE_STATUS;
		return KILOTNCD_PROFILE_OK;
	}

	return KILOTNCD_PROFILE_ERR_UNSUPPORTED;
}

enum kilotncd_profile_result
kilotncd_profile_validate(const struct kilotncd_config *config)
{
	enum kilotncd_profile_result res;

	if (config == NULL)
		return KILOTNCD_PROFILE_ERR_ARG;
	res = kilotncd_profile_validate_common(config);
	if (res != KILOTNCD_PROFILE_OK)
		return res;
	if (config->profile == KILOTNCD_PROFILE_FILE_TX)
		return kilotncd_profile_validate_file_tx(config);
	if (config->profile == KILOTNCD_PROFILE_FILE_RX)
		return kilotncd_profile_validate_file_rx(config);
	if (config->profile == KILOTNCD_PROFILE_FILE_LOOPBACK)
		return kilotncd_profile_validate_file_loopback(config);
	if (config->profile == KILOTNCD_PROFILE_STDIO_TX)
		return kilotncd_profile_validate_stdio_tx(config);
	if (config->profile == KILOTNCD_PROFILE_STDIO_RX)
		return kilotncd_profile_validate_stdio_rx(config);
	if (config->profile == KILOTNCD_PROFILE_TCP_KISS_ONCE)
		return kilotncd_profile_validate_tcp(config);
	if (config->profile == KILOTNCD_PROFILE_UNIX_KISS_ONCE)
		return kilotncd_profile_validate_unix(config);
	if (config->profile == KILOTNCD_PROFILE_PTY_KISS_ONCE)
		return kilotncd_profile_validate_pty(config);
	if (config->profile == KILOTNCD_PROFILE_STATUS)
		return kilotncd_profile_validate_status(config);

	return KILOTNCD_PROFILE_ERR_MISSING;
}

static int
kilotncd_profile_binary_stdout(const struct kilotncd_config *config)
{
	if (config->have_pcm_out && strcmp(config->pcm_out, "-") == 0)
		return 1;
	if (config->have_kiss_out && strcmp(config->kiss_out, "-") == 0)
		return 1;
	return 0;
}

static int
kilotncd_profile_binary_stdin(const struct kilotncd_config *config)
{
	if (config->have_kiss_in && strcmp(config->kiss_in, "-") == 0)
		return 1;
	if (config->have_pcm_in && strcmp(config->pcm_in, "-") == 0)
		return 1;
	return 0;
}

static int
kilotncd_profile_has_listener(const struct kilotncd_config *config)
{
	return config->have_kiss_tcp_listen || config->have_kiss_unix_listen ||
	    config->kiss_pty != 0U;
}

static int
kilotncd_profile_mode_implemented(enum tnc_mode_id mode)
{
	const struct tnc_mode_desc *desc;

	if (tnc_mode_get(mode, &desc) != TNC_MODE_OK)
		return 0;
	return desc->support == TNC_MODE_SUPPORT_IMPLEMENTED;
}

static enum kilotncd_profile_result
kilotncd_profile_validate_common(const struct kilotncd_config *config)
{
	if (config->audio_backend != KILOTNCD_AUDIO_BACKEND_RAW_FILE)
		return KILOTNCD_PROFILE_ERR_UNSUPPORTED;
	if (kilotncd_audio_validate_format(&config->audio_format) !=
	    KILOTNCD_AUDIO_OK)
		return KILOTNCD_PROFILE_ERR_UNSUPPORTED;
	if (config->radio_backend != KILOTNCD_RADIO_BACKEND_NONE &&
	    config->radio_backend != KILOTNCD_RADIO_BACKEND_SIM &&
	    config->radio_backend != KILOTNCD_RADIO_BACKEND_LOG)
		return KILOTNCD_PROFILE_ERR_UNSUPPORTED;
	if (config->radio_backend == KILOTNCD_RADIO_BACKEND_LOG &&
	    !config->have_radio_log)
		return KILOTNCD_PROFILE_ERR_MISSING;
	if (config->have_kiss_tcp_listen &&
	    kilotncd_tcp_reject_nonlocal(&config->kiss_tcp_addr,
	    config->allow_nonlocal_bind) != KILOTNCD_TCP_OK)
		return KILOTNCD_PROFILE_ERR_UNSAFE;
	if (config->profile != KILOTNCD_PROFILE_STATUS &&
	    !kilotncd_profile_mode_implemented(config->mode))
		return KILOTNCD_PROFILE_ERR_UNSUPPORTED;
	if (config->profile != KILOTNCD_PROFILE_STATUS &&
	    config->profile != KILOTNCD_PROFILE_FILE_RX &&
	    config->profile != KILOTNCD_PROFILE_STDIO_RX &&
	    config->max_tx_ms == 0U)
		return KILOTNCD_PROFILE_ERR_UNSAFE;
	if (config->have_kiss_in && config->have_pcm_in &&
	    strcmp(config->kiss_in, "-") == 0 &&
	    strcmp(config->pcm_in, "-") == 0)
		return KILOTNCD_PROFILE_ERR_CONFLICT;
	if (config->have_kiss_out && config->have_pcm_out &&
	    strcmp(config->kiss_out, "-") == 0 &&
	    strcmp(config->pcm_out, "-") == 0)
		return KILOTNCD_PROFILE_ERR_CONFLICT;

	return KILOTNCD_PROFILE_OK;
}

static enum kilotncd_profile_result
kilotncd_profile_validate_file_tx(const struct kilotncd_config *config)
{
	if (!config->have_kiss_in || !config->have_pcm_out)
		return KILOTNCD_PROFILE_ERR_MISSING;
	if (kilotncd_profile_has_listener(config) || config->have_pcm_in ||
	    config->have_kiss_out)
		return KILOTNCD_PROFILE_ERR_CONFLICT;
	if (kilotncd_profile_binary_stdin(config) ||
	    kilotncd_profile_binary_stdout(config))
		return KILOTNCD_PROFILE_ERR_CONFLICT;

	return KILOTNCD_PROFILE_OK;
}

static enum kilotncd_profile_result
kilotncd_profile_validate_file_rx(const struct kilotncd_config *config)
{
	if (!config->have_pcm_in || !config->have_kiss_out)
		return KILOTNCD_PROFILE_ERR_MISSING;
	if (kilotncd_profile_has_listener(config) || config->have_kiss_in ||
	    config->have_pcm_out)
		return KILOTNCD_PROFILE_ERR_CONFLICT;
	if (kilotncd_profile_binary_stdin(config) ||
	    kilotncd_profile_binary_stdout(config))
		return KILOTNCD_PROFILE_ERR_CONFLICT;

	return KILOTNCD_PROFILE_OK;
}

static enum kilotncd_profile_result
kilotncd_profile_validate_file_loopback(const struct kilotncd_config *config)
{
	if (!config->have_kiss_in || !config->have_kiss_out)
		return KILOTNCD_PROFILE_ERR_MISSING;
	if (kilotncd_profile_has_listener(config) || config->have_pcm_in ||
	    config->have_pcm_out)
		return KILOTNCD_PROFILE_ERR_CONFLICT;
	if (strcmp(config->kiss_in, "-") == 0 ||
	    strcmp(config->kiss_out, "-") == 0)
		return KILOTNCD_PROFILE_ERR_CONFLICT;

	return KILOTNCD_PROFILE_OK;
}

static enum kilotncd_profile_result
kilotncd_profile_validate_stdio_tx(const struct kilotncd_config *config)
{
	if (!config->have_kiss_in || !config->have_pcm_out)
		return KILOTNCD_PROFILE_ERR_MISSING;
	if (kilotncd_profile_has_listener(config) || config->have_pcm_in ||
	    config->have_kiss_out)
		return KILOTNCD_PROFILE_ERR_CONFLICT;
	if (strcmp(config->kiss_in, "-") != 0 &&
	    strcmp(config->pcm_out, "-") != 0)
		return KILOTNCD_PROFILE_ERR_CONFLICT;

	return KILOTNCD_PROFILE_OK;
}

static enum kilotncd_profile_result
kilotncd_profile_validate_stdio_rx(const struct kilotncd_config *config)
{
	if (!config->have_pcm_in || !config->have_kiss_out)
		return KILOTNCD_PROFILE_ERR_MISSING;
	if (kilotncd_profile_has_listener(config) || config->have_kiss_in ||
	    config->have_pcm_out)
		return KILOTNCD_PROFILE_ERR_CONFLICT;
	if (strcmp(config->pcm_in, "-") != 0 &&
	    strcmp(config->kiss_out, "-") != 0)
		return KILOTNCD_PROFILE_ERR_CONFLICT;

	return KILOTNCD_PROFILE_OK;
}

static enum kilotncd_profile_result
kilotncd_profile_validate_tcp(const struct kilotncd_config *config)
{
	if (!config->have_kiss_tcp_listen || config->kiss_tcp_once == 0U)
		return KILOTNCD_PROFILE_ERR_MISSING;
	if (config->have_kiss_unix_listen || config->kiss_pty != 0U ||
	    config->have_kiss_in || config->have_pcm_in ||
	    config->have_kiss_out)
		return KILOTNCD_PROFILE_ERR_CONFLICT;
	if (!config->have_pcm_out)
		return KILOTNCD_PROFILE_ERR_MISSING;

	return KILOTNCD_PROFILE_OK;
}

static enum kilotncd_profile_result
kilotncd_profile_validate_unix(const struct kilotncd_config *config)
{
	if (!config->have_kiss_unix_listen || config->kiss_unix_once == 0U)
		return KILOTNCD_PROFILE_ERR_MISSING;
	if (config->have_kiss_tcp_listen || config->kiss_pty != 0U ||
	    config->have_kiss_in || config->have_pcm_in ||
	    config->have_kiss_out)
		return KILOTNCD_PROFILE_ERR_CONFLICT;
	if (!config->have_pcm_out)
		return KILOTNCD_PROFILE_ERR_MISSING;

	return KILOTNCD_PROFILE_OK;
}

static enum kilotncd_profile_result
kilotncd_profile_validate_pty(const struct kilotncd_config *config)
{
	if (config->kiss_pty == 0U || config->kiss_pty_once == 0U)
		return KILOTNCD_PROFILE_ERR_MISSING;
	if (config->have_kiss_tcp_listen || config->have_kiss_unix_listen ||
	    config->have_kiss_in || config->have_pcm_in ||
	    config->have_kiss_out)
		return KILOTNCD_PROFILE_ERR_CONFLICT;
	if (!config->have_pcm_out)
		return KILOTNCD_PROFILE_ERR_MISSING;

	return KILOTNCD_PROFILE_OK;
}

static enum kilotncd_profile_result
kilotncd_profile_validate_status(const struct kilotncd_config *config)
{
	if (config->have_kiss_in || config->have_kiss_out ||
	    config->have_pcm_in || config->have_pcm_out ||
	    kilotncd_profile_has_listener(config))
		return KILOTNCD_PROFILE_ERR_CONFLICT;

	return KILOTNCD_PROFILE_OK;
}
