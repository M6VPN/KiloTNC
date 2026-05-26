/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/tests/test_embedded_loopback.c */

#include <sys/types.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ax25.h"
#include "embedded_loopback.h"
#include "kiss.h"
#include "tnc_mode.h"

#define TEST_LOOPBACK_INFO_MAX 128U

static int append_kiss_frame(uint8_t *, size_t, size_t *, uint8_t,
	const uint8_t *, size_t);
static int build_ax25_info(const uint8_t *, size_t, uint8_t *, size_t,
	size_t *);
static int build_kiss_info(const uint8_t *, size_t, uint8_t *, size_t,
	size_t *);
static int check_kiss_info(const uint8_t *, size_t, const uint8_t *,
	size_t);
static void loopback_default_config(struct embedded_loopback_config *);
static int run_loopback_info(const uint8_t *, size_t,
	const struct embedded_loopback_config *,
	struct embedded_loopback_stats *);
static int test_embedded_loopback_escaped(void);
static int test_embedded_loopback_large_usb_input(void);
static int test_embedded_loopback_malformed(void);
static int test_embedded_loopback_null_args(void);
static int test_embedded_loopback_sethw_mode_6(void);
static int test_embedded_loopback_sethw_mode_22(void);
static int test_embedded_loopback_simple(void);
static int test_embedded_loopback_small_audio_chunks(void);
static int test_embedded_loopback_small_output(void);
static int test_embedded_loopback_timeout(void);
static int test_embedded_loopback_unsupported_command(void);
static int test_embedded_loopback_unsupported_mode(void);
static int test_embedded_loopback_watchdog_fault(void);

static int
append_kiss_frame(uint8_t *out, size_t out_cap, size_t *out_len,
	uint8_t command, const uint8_t *payload, size_t payload_len)
{
	uint8_t encoded[KILOTNC_KISS_MAX_FRAME];
	size_t encoded_len;

	if (kiss_encode_frame(0, command, payload, payload_len, encoded,
	    sizeof(encoded), &encoded_len) != KISS_OK)
		return __LINE__;
	if (encoded_len > out_cap - *out_len)
		return __LINE__;

	(void)memcpy(out + *out_len, encoded, encoded_len);
	*out_len += encoded_len;
	return 0;
}

static int
build_ax25_info(const uint8_t *info, size_t info_len, uint8_t *out,
	size_t out_cap, size_t *out_len)
{
	struct ax25_frame frame;

	(void)memset(&frame, 0, sizeof(frame));
	(void)memcpy(frame.dst.callsign, "APZKTN", 6);
	(void)memcpy(frame.src.callsign, "M6VPN", 5);
	frame.pid = AX25_PID_NONE;
	(void)memcpy(frame.info, info, info_len);
	frame.info_len = info_len;
	if (ax25_encode_ui_fcs(&frame, out, out_cap, out_len) != AX25_OK)
		return __LINE__;

	return 0;
}

static int
build_kiss_info(const uint8_t *info, size_t info_len, uint8_t *out,
	size_t out_cap, size_t *out_len)
{
	uint8_t ax25[KILOTNC_AX25_MAX_FRAME];
	size_t ax25_len;
	int line;

	*out_len = 0U;
	line = build_ax25_info(info, info_len, ax25, sizeof(ax25),
	    &ax25_len);
	if (line != 0)
		return line;
	return append_kiss_frame(out, out_cap, out_len, KISS_CMD_DATA,
	    ax25, ax25_len);
}

static int
check_kiss_info(const uint8_t *buf, size_t len, const uint8_t *info,
	size_t info_len)
{
	struct ax25_frame decoded;
	struct kiss_parser parser;
	struct kiss_frame frames[1];
	size_t frame_count;

	kiss_parser_init(&parser);
	if (kiss_parse_bytes(&parser, buf, len, frames,
	    sizeof(frames) / sizeof(frames[0]), &frame_count) != KISS_OK)
		return __LINE__;
	if (frame_count != 1U || frames[0].command != KISS_CMD_DATA)
		return __LINE__;
	if (ax25_decode_ui_fcs(frames[0].data, frames[0].len, &decoded) !=
	    AX25_OK)
		return __LINE__;
	if (strcmp(decoded.dst.callsign, "APZKTN") != 0)
		return __LINE__;
	if (strcmp(decoded.src.callsign, "M6VPN") != 0)
		return __LINE__;
	if (decoded.info_len != info_len)
		return __LINE__;
	if (memcmp(decoded.info, info, info_len) != 0)
		return __LINE__;

	return 0;
}

static void
loopback_default_config(struct embedded_loopback_config *config)
{
	config->max_iterations = EMBEDDED_LOOPBACK_DEFAULT_MAX_ITERATIONS;
	config->audio_copy_chunk = EMBEDDED_LOOPBACK_DEFAULT_AUDIO_COPY;
	config->simulate_watchdog_fault = 0;
	config->watchdog_fault_iteration = 0U;
}

static int
run_loopback_info(const uint8_t *info, size_t info_len,
	const struct embedded_loopback_config *config,
	struct embedded_loopback_stats *stats)
{
	uint8_t kiss_in[KILOTNC_KISS_MAX_FRAME];
	uint8_t kiss_out[KILOTNC_KISS_MAX_FRAME];
	size_t kiss_in_len;
	size_t kiss_out_len;
	int line;

	line = build_kiss_info(info, info_len, kiss_in, sizeof(kiss_in),
	    &kiss_in_len);
	if (line != 0)
		return line;
	if (embedded_loopback_run_once_config(kiss_in, kiss_in_len,
	    kiss_out, sizeof(kiss_out), &kiss_out_len, config, stats) !=
	    EMBEDDED_LOOPBACK_OK)
		return __LINE__;
	line = check_kiss_info(kiss_out, kiss_out_len, info, info_len);
	if (line != 0)
		return line;
	if (stats->ptt_state != 0U || stats->watchdog_kicks == 0U)
		return __LINE__;
	if (stats->audio_tx_samples == 0U ||
	    stats->audio_copied_samples == 0U ||
	    stats->modem_tx_frames != 1U || stats->modem_rx_frames != 1U ||
	    stats->kiss_frames_out != 1U)
		return __LINE__;
	if (stats->diag.modem_tx_frames_started != 1U ||
	    stats->diag.modem_rx_frames_ok != 1U ||
	    stats->diag.tnc_modem_rx_kiss_frames != 1U)
		return __LINE__;

	return 0;
}

static int
test_embedded_loopback_escaped(void)
{
	struct embedded_loopback_config config;
	struct embedded_loopback_stats stats;
	uint8_t info[5];

	info[0] = 'A';
	info[1] = KISS_FEND;
	info[2] = 'B';
	info[3] = KISS_FESC;
	info[4] = 'C';
	loopback_default_config(&config);
	return run_loopback_info(info, sizeof(info), &config, &stats);
}

static int
test_embedded_loopback_large_usb_input(void)
{
	struct embedded_loopback_config config;
	struct embedded_loopback_stats stats;
	uint8_t info[96];
	size_t i;

	for (i = 0U; i < sizeof(info); i++)
		info[i] = (uint8_t)('a' + (i % 26U));
	loopback_default_config(&config);
	return run_loopback_info(info, sizeof(info), &config, &stats);
}

static int
test_embedded_loopback_malformed(void)
{
	struct embedded_loopback_config config;
	struct embedded_loopback_stats stats;
	uint8_t malformed[5];
	uint8_t kiss_out[128];
	size_t kiss_out_len;

	malformed[0] = KISS_FEND;
	malformed[1] = 0U;
	malformed[2] = KISS_FESC;
	malformed[3] = 0U;
	malformed[4] = KISS_FEND;
	loopback_default_config(&config);
	config.max_iterations = 64U;
	if (embedded_loopback_run_once_config(malformed, sizeof(malformed),
	    kiss_out, sizeof(kiss_out), &kiss_out_len, &config, &stats) !=
	    EMBEDDED_LOOPBACK_ERR_TIMEOUT)
		return __LINE__;
	if (kiss_out_len != 0U || stats.timeout == 0U || stats.ptt_state != 0U)
		return __LINE__;
	if (stats.diag.tnc_kiss_parse_errors == 0U)
		return __LINE__;

	return 0;
}

static int
test_embedded_loopback_null_args(void)
{
	struct embedded_loopback_config config;
	struct embedded_loopback_stats stats;
	uint8_t kiss[16];
	uint8_t out[16];
	size_t out_len;

	loopback_default_config(&config);
	if (embedded_loopback_run_once(NULL, sizeof(kiss), out, sizeof(out),
	    &out_len, &stats) != EMBEDDED_LOOPBACK_ERR_ARG)
		return __LINE__;
	if (embedded_loopback_run_once(kiss, sizeof(kiss), NULL,
	    sizeof(out), &out_len, &stats) != EMBEDDED_LOOPBACK_ERR_ARG)
		return __LINE__;
	if (embedded_loopback_run_once(kiss, sizeof(kiss), out, sizeof(out),
	    NULL, &stats) != EMBEDDED_LOOPBACK_ERR_ARG)
		return __LINE__;
	if (embedded_loopback_run_once_config(kiss, sizeof(kiss), out,
	    sizeof(out), &out_len, NULL, &stats) !=
	    EMBEDDED_LOOPBACK_ERR_ARG)
		return __LINE__;
	config.max_iterations = 0U;
	if (embedded_loopback_run_once_config(kiss, sizeof(kiss), out,
	    sizeof(out), &out_len, &config, &stats) !=
	    EMBEDDED_LOOPBACK_ERR_ARG)
		return __LINE__;

	return 0;
}

static int
test_embedded_loopback_sethw_mode_6(void)
{
	struct embedded_loopback_config config;
	struct embedded_loopback_stats stats;
	uint8_t stream[KILOTNC_KISS_MAX_FRAME * 2U];
	uint8_t data[KILOTNC_KISS_MAX_FRAME];
	uint8_t out[KILOTNC_KISS_MAX_FRAME];
	uint8_t mode;
	uint8_t info[] = "sethw6";
	size_t data_len;
	size_t out_len;
	size_t stream_len;
	int line;

	loopback_default_config(&config);
	stream_len = 0U;
	mode = 6U;
	line = append_kiss_frame(stream, sizeof(stream), &stream_len,
	    KISS_CMD_SETHARDWARE, &mode, 1U);
	if (line != 0)
		return line;
	line = build_kiss_info(info, sizeof(info) - 1U, data, sizeof(data),
	    &data_len);
	if (line != 0)
		return line;
	(void)memcpy(stream + stream_len, data, data_len);
	stream_len += data_len;
	if (embedded_loopback_run_once_config(stream, stream_len, out,
	    sizeof(out), &out_len, &config, &stats) != EMBEDDED_LOOPBACK_OK)
		return __LINE__;
	line = check_kiss_info(out, out_len, info, sizeof(info) - 1U);
	if (line != 0)
		return line;
	if (stats.diag.tnc_mode_set_requests != 1U ||
	    stats.diag.tnc_mode_unsupported != 0U)
		return __LINE__;

	return 0;
}

static int
test_embedded_loopback_sethw_mode_22(void)
{
	struct embedded_loopback_config config;
	struct embedded_loopback_stats stats;
	uint8_t stream[KILOTNC_KISS_MAX_FRAME * 2U];
	uint8_t data[KILOTNC_KISS_MAX_FRAME];
	uint8_t out[KILOTNC_KISS_MAX_FRAME];
	uint8_t mode;
	uint8_t info[] = "sethw22";
	size_t data_len;
	size_t out_len;
	size_t stream_len;
	int line;

	loopback_default_config(&config);
	stream_len = 0U;
	mode = 22U;
	line = append_kiss_frame(stream, sizeof(stream), &stream_len,
	    KISS_CMD_SETHARDWARE, &mode, 1U);
	if (line != 0)
		return line;
	line = build_kiss_info(info, sizeof(info) - 1U, data, sizeof(data),
	    &data_len);
	if (line != 0)
		return line;
	(void)memcpy(stream + stream_len, data, data_len);
	stream_len += data_len;
	if (embedded_loopback_run_once_config(stream, stream_len, out,
	    sizeof(out), &out_len, &config, &stats) != EMBEDDED_LOOPBACK_OK)
		return __LINE__;
	line = check_kiss_info(out, out_len, info, sizeof(info) - 1U);
	if (line != 0)
		return line;
	if (stats.diag.tnc_mode_set_requests != 1U ||
	    stats.diag.tnc_mode_unsupported != 0U)
		return __LINE__;

	return 0;
}

static int
test_embedded_loopback_simple(void)
{
	struct embedded_loopback_config config;
	struct embedded_loopback_stats stats;
	uint8_t info[] = "embedded loopback";

	loopback_default_config(&config);
	return run_loopback_info(info, sizeof(info) - 1U, &config, &stats);
}

static int
test_embedded_loopback_small_audio_chunks(void)
{
	struct embedded_loopback_config config;
	struct embedded_loopback_stats stats;
	uint8_t info[] = "small audio chunks";

	loopback_default_config(&config);
	config.audio_copy_chunk = 8U;
	return run_loopback_info(info, sizeof(info) - 1U, &config, &stats);
}

static int
test_embedded_loopback_small_output(void)
{
	struct embedded_loopback_config config;
	struct embedded_loopback_stats stats;
	uint8_t kiss[KILOTNC_KISS_MAX_FRAME];
	uint8_t out[1];
	uint8_t info[] = "small output";
	size_t kiss_len;
	size_t out_len;
	int line;

	line = build_kiss_info(info, sizeof(info) - 1U, kiss, sizeof(kiss),
	    &kiss_len);
	if (line != 0)
		return line;
	loopback_default_config(&config);
	if (embedded_loopback_run_once_config(kiss, kiss_len, out,
	    sizeof(out), &out_len, &config, &stats) !=
	    EMBEDDED_LOOPBACK_ERR_SMALL)
		return __LINE__;
	if (stats.ptt_state != 0U)
		return __LINE__;

	return 0;
}

static int
test_embedded_loopback_timeout(void)
{
	struct embedded_loopback_config config;
	struct embedded_loopback_stats stats;
	uint8_t kiss[KILOTNC_KISS_MAX_FRAME];
	uint8_t out[KILOTNC_KISS_MAX_FRAME];
	uint8_t info[] = "timeout";
	size_t kiss_len;
	size_t out_len;
	int line;

	line = build_kiss_info(info, sizeof(info) - 1U, kiss, sizeof(kiss),
	    &kiss_len);
	if (line != 0)
		return line;
	loopback_default_config(&config);
	config.max_iterations = 1U;
	if (embedded_loopback_run_once_config(kiss, kiss_len, out,
	    sizeof(out), &out_len, &config, &stats) !=
	    EMBEDDED_LOOPBACK_ERR_TIMEOUT)
		return __LINE__;
	if (out_len != 0U || stats.timeout == 0U || stats.ptt_state != 0U)
		return __LINE__;

	return 0;
}

static int
test_embedded_loopback_unsupported_command(void)
{
	struct embedded_loopback_config config;
	struct embedded_loopback_stats stats;
	uint8_t stream[KILOTNC_KISS_MAX_FRAME * 2U];
	uint8_t data[KILOTNC_KISS_MAX_FRAME];
	uint8_t out[KILOTNC_KISS_MAX_FRAME];
	uint8_t value;
	uint8_t info[] = "unsupported command";
	size_t data_len;
	size_t out_len;
	size_t stream_len;
	int line;

	loopback_default_config(&config);
	stream_len = 0U;
	value = 1U;
	line = append_kiss_frame(stream, sizeof(stream), &stream_len, 7U,
	    &value, 1U);
	if (line != 0)
		return line;
	line = build_kiss_info(info, sizeof(info) - 1U, data, sizeof(data),
	    &data_len);
	if (line != 0)
		return line;
	(void)memcpy(stream + stream_len, data, data_len);
	stream_len += data_len;
	if (embedded_loopback_run_once_config(stream, stream_len, out,
	    sizeof(out), &out_len, &config, &stats) != EMBEDDED_LOOPBACK_OK)
		return __LINE__;
	line = check_kiss_info(out, out_len, info, sizeof(info) - 1U);
	if (line != 0)
		return line;
	if (stats.diag.tnc_kiss_ignored_commands != 1U)
		return __LINE__;

	return 0;
}

static int
test_embedded_loopback_unsupported_mode(void)
{
	struct embedded_loopback_config config;
	struct embedded_loopback_stats stats;
	uint8_t stream[KILOTNC_KISS_MAX_FRAME * 2U];
	uint8_t data[KILOTNC_KISS_MAX_FRAME];
	uint8_t out[KILOTNC_KISS_MAX_FRAME];
	uint8_t mode;
	uint8_t info[] = "blocked mode";
	size_t data_len;
	size_t out_len;
	size_t stream_len;
	int line;

	loopback_default_config(&config);
	config.max_iterations = 64U;
	stream_len = 0U;
	mode = 0U;
	line = append_kiss_frame(stream, sizeof(stream), &stream_len,
	    KISS_CMD_SETHARDWARE, &mode, 1U);
	if (line != 0)
		return line;
	line = build_kiss_info(info, sizeof(info) - 1U, data, sizeof(data),
	    &data_len);
	if (line != 0)
		return line;
	(void)memcpy(stream + stream_len, data, data_len);
	stream_len += data_len;
	if (embedded_loopback_run_once_config(stream, stream_len, out,
	    sizeof(out), &out_len, &config, &stats) !=
	    EMBEDDED_LOOPBACK_ERR_TIMEOUT)
		return __LINE__;
	if (out_len != 0U || stats.modem_tx_frames != 0U ||
	    stats.diag.tnc_mode_unsupported != 1U || stats.ptt_state != 0U)
		return __LINE__;

	return 0;
}

static int
test_embedded_loopback_watchdog_fault(void)
{
	struct embedded_loopback_config config;
	struct embedded_loopback_stats stats;
	uint8_t kiss[KILOTNC_KISS_MAX_FRAME];
	uint8_t out[KILOTNC_KISS_MAX_FRAME];
	uint8_t info[] = "watchdog";
	size_t kiss_len;
	size_t out_len;
	int line;

	line = build_kiss_info(info, sizeof(info) - 1U, kiss, sizeof(kiss),
	    &kiss_len);
	if (line != 0)
		return line;
	loopback_default_config(&config);
	config.simulate_watchdog_fault = 1;
	config.watchdog_fault_iteration = 1U;
	if (embedded_loopback_run_once_config(kiss, kiss_len, out,
	    sizeof(out), &out_len, &config, &stats) !=
	    EMBEDDED_LOOPBACK_ERR_FAULT)
		return __LINE__;
	if (out_len != 0U || stats.faulted == 0U || stats.ptt_state != 0U)
		return __LINE__;
	if (stats.diag.app_state == 0U || stats.diag.modem_aborts == 0U)
		return __LINE__;

	return 0;
}

int
test_embedded_loopback(void)
{
	int line;

	line = test_embedded_loopback_simple();
	if (line != 0)
		goto fail;
	line = test_embedded_loopback_escaped();
	if (line != 0)
		goto fail;
	line = test_embedded_loopback_small_audio_chunks();
	if (line != 0)
		goto fail;
	line = test_embedded_loopback_large_usb_input();
	if (line != 0)
		goto fail;
	line = test_embedded_loopback_unsupported_command();
	if (line != 0)
		goto fail;
	line = test_embedded_loopback_sethw_mode_6();
	if (line != 0)
		goto fail;
	line = test_embedded_loopback_sethw_mode_22();
	if (line != 0)
		goto fail;
	line = test_embedded_loopback_unsupported_mode();
	if (line != 0)
		goto fail;
	line = test_embedded_loopback_malformed();
	if (line != 0)
		goto fail;
	line = test_embedded_loopback_timeout();
	if (line != 0)
		goto fail;
	line = test_embedded_loopback_small_output();
	if (line != 0)
		goto fail;
	line = test_embedded_loopback_watchdog_fault();
	if (line != 0)
		goto fail;
	line = test_embedded_loopback_null_args();
	if (line != 0)
		goto fail;

	(void)printf("ok embedded_loopback\n");
	return 0;

fail:
	(void)printf("not ok embedded_loopback line %d\n", line);
	return 1;
}
