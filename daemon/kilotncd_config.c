/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/daemon/kilotncd_config.c */

#include <sys/types.h>

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kilotncd_config.h"

static enum kilotncd_config_result kilotncd_config_apply_pair(
	struct kilotncd_config *, const char *, const char *);
static enum kilotncd_config_result kilotncd_config_copy_path(char *,
	size_t, const char *);
static char *kilotncd_config_trim(char *);
static enum kilotncd_config_result kilotncd_config_parse_u8(const char *,
	uint8_t *);
static enum kilotncd_config_result kilotncd_config_parse_u32(const char *,
	uint32_t *);

enum kilotncd_config_result
kilotncd_config_apply_arg(struct kilotncd_config *config, const char *key,
	const char *value)
{
	if (config == NULL || key == NULL || value == NULL)
		return KILOTNCD_CONFIG_ERR_ARG;
	return kilotncd_config_apply_pair(config, key, value);
}

enum kilotncd_config_result
kilotncd_config_init(struct kilotncd_config *config)
{
	if (config == NULL)
		return KILOTNCD_CONFIG_ERR_ARG;

	(void)memset(config, 0, sizeof(*config));
	if (tnc_mode_default(&config->mode) != TNC_MODE_OK)
		return KILOTNCD_CONFIG_ERR_PARSE;
	kilotncd_audio_default_format(&config->audio_format);
	config->audio_backend = KILOTNCD_AUDIO_BACKEND_RAW_FILE;
	config->profile = KILOTNCD_PROFILE_UNSET;
	config->radio_backend = KILOTNCD_RADIO_BACKEND_NONE;
	config->mode_temporary = 0;
	config->p = 255U;
	config->slottime_10ms = 10U;
	config->fullduplex = 0U;
	config->kiss_tcp_once = 0U;
	config->allow_nonlocal_bind = 0U;
	config->kiss_unix_once = 0U;
	config->unlink_stale_socket = 0U;
	config->kiss_pty = 0U;
	config->kiss_pty_once = 0U;
	config->max_tx_ms = 30000U;

	return KILOTNCD_CONFIG_OK;
}

enum kilotncd_config_result
kilotncd_config_load_file(struct kilotncd_config *config, const char *path)
{
	FILE *fp;
	char line[KILOTNCD_LINE_MAX];
	char *key;
	char *value;
	char *eq;
	size_t len;
	enum kilotncd_config_result res;

	if (config == NULL || path == NULL)
		return KILOTNCD_CONFIG_ERR_ARG;
	fp = fopen(path, "r");
	if (fp == NULL)
		return KILOTNCD_CONFIG_ERR_IO;

	while (fgets(line, sizeof(line), fp) != NULL) {
		len = strlen(line);
		if (len == sizeof(line) - 1U && line[len - 1U] != '\n') {
			(void)fclose(fp);
			return KILOTNCD_CONFIG_ERR_RANGE;
		}
		key = kilotncd_config_trim(line);
		if (key[0] == '\0' || key[0] == '#')
			continue;
		eq = strchr(key, '=');
		if (eq == NULL) {
			(void)fclose(fp);
			return KILOTNCD_CONFIG_ERR_PARSE;
		}
		*eq = '\0';
		value = kilotncd_config_trim(eq + 1);
		key = kilotncd_config_trim(key);
		res = kilotncd_config_apply_pair(config, key, value);
		if (res != KILOTNCD_CONFIG_OK) {
			(void)fclose(fp);
			return res;
		}
	}
	if (ferror(fp)) {
		(void)fclose(fp);
		return KILOTNCD_CONFIG_ERR_IO;
	}
	if (fclose(fp) != 0)
		return KILOTNCD_CONFIG_ERR_IO;

	return KILOTNCD_CONFIG_OK;
}

static enum kilotncd_config_result
kilotncd_config_apply_pair(struct kilotncd_config *config, const char *key,
	const char *value)
{
	enum tnc_mode_result mres;

	if (strcmp(key, "mode") == 0) {
		if (kilotncd_config_copy_path(config->mode_text,
		    sizeof(config->mode_text), value) != KILOTNCD_CONFIG_OK)
			return KILOTNCD_CONFIG_ERR_RANGE;
		mres = tnc_mode_parse_option(value, &config->mode,
		    &config->mode_temporary);
		if (mres != TNC_MODE_OK)
			return KILOTNCD_CONFIG_ERR_PARSE;
		config->have_mode = 1;
		return KILOTNCD_CONFIG_OK;
	}
	if (strcmp(key, "kiss_in") == 0) {
		config->have_kiss_in = 1;
		return kilotncd_config_copy_path(config->kiss_in,
		    sizeof(config->kiss_in), value);
	}
	if (strcmp(key, "kiss_out") == 0) {
		config->have_kiss_out = 1;
		return kilotncd_config_copy_path(config->kiss_out,
		    sizeof(config->kiss_out), value);
	}
	if (strcmp(key, "pcm_in") == 0) {
		config->have_pcm_in = 1;
		return kilotncd_config_copy_path(config->pcm_in,
		    sizeof(config->pcm_in), value);
	}
	if (strcmp(key, "pcm_out") == 0) {
		config->have_pcm_out = 1;
		return kilotncd_config_copy_path(config->pcm_out,
		    sizeof(config->pcm_out), value);
	}
	if (strcmp(key, "control") == 0) {
		config->have_control = 1;
		return kilotncd_config_copy_path(config->control,
		    sizeof(config->control), value);
	}
	if (strcmp(key, "audio_backend") == 0) {
		if (kilotncd_audio_parse_backend(value,
		    &config->audio_backend) != KILOTNCD_AUDIO_OK)
			return KILOTNCD_CONFIG_ERR_PARSE;
		return KILOTNCD_CONFIG_OK;
	}
	if (strcmp(key, "audio_sample_rate") == 0)
		return kilotncd_config_parse_u32(value,
		    &config->audio_format.sample_rate);
	if (strcmp(key, "audio_channels") == 0)
		return kilotncd_config_parse_u8(value,
		    &config->audio_format.channels);
	if (strcmp(key, "audio_bits") == 0)
		return kilotncd_config_parse_u8(value,
		    &config->audio_format.bits_per_sample);
	if (strcmp(key, "profile") == 0) {
		if (kilotncd_profile_parse(value, &config->profile) !=
		    KILOTNCD_PROFILE_OK)
			return KILOTNCD_CONFIG_ERR_PARSE;
		config->have_profile = 1;
		return KILOTNCD_CONFIG_OK;
	}
	if (strcmp(key, "radio_backend") == 0) {
		if (kilotncd_radio_parse_backend(value,
		    &config->radio_backend) != KILOTNCD_RADIO_OK)
			return KILOTNCD_CONFIG_ERR_PARSE;
		return KILOTNCD_CONFIG_OK;
	}
	if (strcmp(key, "radio_log") == 0) {
		config->have_radio_log = 1;
		return kilotncd_config_copy_path(config->radio_log,
		    sizeof(config->radio_log), value);
	}
	if (strcmp(key, "kiss_tcp_listen") == 0) {
		if (kilotncd_tcp_parse_listen(value,
		    &config->kiss_tcp_addr) != KILOTNCD_TCP_OK)
			return KILOTNCD_CONFIG_ERR_PARSE;
		config->have_kiss_tcp_listen = 1;
		return KILOTNCD_CONFIG_OK;
	}
	if (strcmp(key, "kiss_unix_listen") == 0) {
		config->have_kiss_unix_listen = 1;
		return kilotncd_config_copy_path(config->kiss_unix_listen,
		    sizeof(config->kiss_unix_listen), value);
	}
	if (strcmp(key, "pty_path_out") == 0) {
		config->have_pty_path_out = 1;
		return kilotncd_config_copy_path(config->pty_path_out,
		    sizeof(config->pty_path_out), value);
	}
	if (strcmp(key, "kiss_tcp_once") == 0) {
		if (kilotncd_config_parse_u8(value,
		    &config->kiss_tcp_once) != KILOTNCD_CONFIG_OK)
			return KILOTNCD_CONFIG_ERR_PARSE;
		if (config->kiss_tcp_once > 1U)
			return KILOTNCD_CONFIG_ERR_RANGE;
		return KILOTNCD_CONFIG_OK;
	}
	if (strcmp(key, "allow_nonlocal_bind") == 0) {
		if (kilotncd_config_parse_u8(value,
		    &config->allow_nonlocal_bind) != KILOTNCD_CONFIG_OK)
			return KILOTNCD_CONFIG_ERR_PARSE;
		if (config->allow_nonlocal_bind > 1U)
			return KILOTNCD_CONFIG_ERR_RANGE;
		return KILOTNCD_CONFIG_OK;
	}
	if (strcmp(key, "kiss_unix_once") == 0) {
		if (kilotncd_config_parse_u8(value,
		    &config->kiss_unix_once) != KILOTNCD_CONFIG_OK)
			return KILOTNCD_CONFIG_ERR_PARSE;
		if (config->kiss_unix_once > 1U)
			return KILOTNCD_CONFIG_ERR_RANGE;
		return KILOTNCD_CONFIG_OK;
	}
	if (strcmp(key, "unlink_stale_socket") == 0) {
		if (kilotncd_config_parse_u8(value,
		    &config->unlink_stale_socket) != KILOTNCD_CONFIG_OK)
			return KILOTNCD_CONFIG_ERR_PARSE;
		if (config->unlink_stale_socket > 1U)
			return KILOTNCD_CONFIG_ERR_RANGE;
		return KILOTNCD_CONFIG_OK;
	}
	if (strcmp(key, "kiss_pty") == 0) {
		if (kilotncd_config_parse_u8(value,
		    &config->kiss_pty) != KILOTNCD_CONFIG_OK)
			return KILOTNCD_CONFIG_ERR_PARSE;
		if (config->kiss_pty > 1U)
			return KILOTNCD_CONFIG_ERR_RANGE;
		return KILOTNCD_CONFIG_OK;
	}
	if (strcmp(key, "kiss_pty_once") == 0) {
		if (kilotncd_config_parse_u8(value,
		    &config->kiss_pty_once) != KILOTNCD_CONFIG_OK)
			return KILOTNCD_CONFIG_ERR_PARSE;
		if (config->kiss_pty_once > 1U)
			return KILOTNCD_CONFIG_ERR_RANGE;
		return KILOTNCD_CONFIG_OK;
	}
	if (strcmp(key, "p") == 0)
		return kilotncd_config_parse_u8(value, &config->p);
	if (strcmp(key, "slottime_10ms") == 0)
		return kilotncd_config_parse_u8(value,
		    &config->slottime_10ms);
	if (strcmp(key, "fullduplex") == 0) {
		if (kilotncd_config_parse_u8(value,
		    &config->fullduplex) != KILOTNCD_CONFIG_OK)
			return KILOTNCD_CONFIG_ERR_PARSE;
		if (config->fullduplex > 1U)
			return KILOTNCD_CONFIG_ERR_RANGE;
		return KILOTNCD_CONFIG_OK;
	}
	if (strcmp(key, "max_tx_ms") == 0)
		return kilotncd_config_parse_u32(value, &config->max_tx_ms);

	return KILOTNCD_CONFIG_ERR_UNKNOWN;
}

static enum kilotncd_config_result
kilotncd_config_copy_path(char *dst, size_t dst_cap, const char *src)
{
	size_t len;

	if (dst == NULL || src == NULL || dst_cap == 0U)
		return KILOTNCD_CONFIG_ERR_ARG;
	len = strlen(src);
	if (len == 0U || len >= dst_cap)
		return KILOTNCD_CONFIG_ERR_RANGE;
	(void)memcpy(dst, src, len);
	dst[len] = '\0';

	return KILOTNCD_CONFIG_OK;
}

static char *
kilotncd_config_trim(char *s)
{
	char *end;

	while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n')
		s++;
	end = s + strlen(s);
	while (end > s && (end[-1] == ' ' || end[-1] == '\t' ||
	    end[-1] == '\r' || end[-1] == '\n')) {
		end--;
		*end = '\0';
	}

	return s;
}

static enum kilotncd_config_result
kilotncd_config_parse_u8(const char *value, uint8_t *out)
{
	uint32_t parsed;
	enum kilotncd_config_result res;

	res = kilotncd_config_parse_u32(value, &parsed);
	if (res != KILOTNCD_CONFIG_OK)
		return res;
	if (parsed > UINT8_MAX)
		return KILOTNCD_CONFIG_ERR_RANGE;
	*out = (uint8_t)parsed;

	return KILOTNCD_CONFIG_OK;
}

static enum kilotncd_config_result
kilotncd_config_parse_u32(const char *value, uint32_t *out)
{
	char *end;
	unsigned long parsed;

	if (value == NULL || out == NULL || value[0] == '\0')
		return KILOTNCD_CONFIG_ERR_ARG;
	errno = 0;
	parsed = strtoul(value, &end, 10);
	if (errno != 0 || end == value || *end != '\0')
		return KILOTNCD_CONFIG_ERR_PARSE;
	if (parsed > UINT32_MAX)
		return KILOTNCD_CONFIG_ERR_RANGE;
	*out = (uint32_t)parsed;

	return KILOTNCD_CONFIG_OK;
}
