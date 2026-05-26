/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/test/test_tnc1200.c */

#include <sys/types.h>

#include <stdint.h>
#include <string.h>

#include "afsk1200_rx.h"
#include "afsk1200_stream.h"
#include "ax25.h"
#include "kiss.h"
#include "tnc1200.h"

#define CHECK(expr) do { if (!(expr)) return __LINE__; } while (0)
#define TEST_TNC_PCM_MAX \
	((AFSK1200_MAX_TEST_BITS + (80U * 8U)) * AFSK1200_SAMPLES_PER_BIT)
#define TEST_TNC_KISS_MAX	(KILOTNC_KISS_MAX_FRAME * 2U)

static int test_collect_tnc_tx(struct tnc1200 *, size_t, int16_t *,
	size_t, size_t *);
static int test_decode_kiss_payload(const uint8_t *, size_t, uint8_t *,
	size_t, size_t *);
static void test_fill_addr(struct ax25_addr *, const char *, uint8_t);
static int test_make_frame(const char *, uint8_t *, size_t, size_t *);
static int test_make_kiss_data(const uint8_t *, size_t, uint8_t *, size_t,
	size_t *);
static int test_tnc1200_commands_errors(void);
static int test_tnc1200_control_commands(void);
static int test_tnc1200_dcd_gating(void);
static int test_tnc1200_init_args(void);
static int test_tnc1200_loopback(void);
static int test_tnc1200_mode_sethw(void);
static int test_tnc1200_rx_to_kiss(void);
static int test_tnc1200_tail_abort_timeout(void);
static int test_tnc1200_tx_from_kiss(void);

int
test_tnc1200(void)
{
	int subres;

	subres = test_tnc1200_init_args();
	if (subres != 0)
		return subres;
	subres = test_tnc1200_tx_from_kiss();
	if (subres != 0)
		return subres;
	subres = test_tnc1200_rx_to_kiss();
	if (subres != 0)
		return subres;
	subres = test_tnc1200_loopback();
	if (subres != 0)
		return subres;
	subres = test_tnc1200_mode_sethw();
	if (subres != 0)
		return subres;
	subres = test_tnc1200_commands_errors();
	if (subres != 0)
		return subres;
	subres = test_tnc1200_control_commands();
	if (subres != 0)
		return subres;
	subres = test_tnc1200_dcd_gating();
	if (subres != 0)
		return subres;
	subres = test_tnc1200_tail_abort_timeout();
	if (subres != 0)
		return subres;

	return 0;
}

static int
test_collect_tnc_tx(struct tnc1200 *tnc, size_t chunk, int16_t *pcm,
	size_t pcm_cap, size_t *out_samples)
{
	size_t emitted;
	size_t off;
	size_t idle_count;
	enum tnc1200_result res;

	off = 0U;
	idle_count = 0U;
	for (;;) {
		if (off >= pcm_cap)
			return __LINE__;
		res = tnc1200_tx_process(tnc, &pcm[off], chunk, &emitted);
		if (res == TNC1200_ERR_NO_DATA) {
			if (off != 0U)
				break;
			idle_count++;
			if (idle_count > 5000U)
				return __LINE__;
			continue;
		}
		if (res != TNC1200_OK)
			return __LINE__;
		off += emitted;
	}

	*out_samples = off;
	return 0;
}

static int
test_tnc1200_control_commands(void)
{
	uint8_t frame[KILOTNC_AX25_MAX_FRAME];
	uint8_t kiss_buf[TEST_TNC_KISS_MAX];
	uint8_t cmd[4];
	int16_t pcm[AFSK1200_SAMPLES_PER_BIT];
	struct tnc1200 tnc;
	struct tnc1200_stats stats;
	size_t frame_len;
	size_t kiss_len;
	size_t emitted;

	CHECK(test_make_frame("ctrl", frame, sizeof(frame), &frame_len) == 0);
	CHECK(test_make_kiss_data(frame, frame_len, kiss_buf, sizeof(kiss_buf),
	    &kiss_len) == 0);
	CHECK(tnc1200_init(&tnc, NULL) == TNC1200_OK);

	cmd[0] = KISS_FEND;
	cmd[1] = KISS_CMD_TXDELAY;
	cmd[2] = 3U;
	cmd[3] = KISS_FEND;
	CHECK(tnc1200_host_input(&tnc, cmd, sizeof(cmd)) == TNC1200_OK);
	cmd[1] = KISS_CMD_TXTAIL;
	cmd[2] = 2U;
	CHECK(tnc1200_host_input(&tnc, cmd, sizeof(cmd)) == TNC1200_OK);
	CHECK(tnc1200_host_input(&tnc, kiss_buf, kiss_len) == TNC1200_OK);
	CHECK(tnc1200_tx_process(&tnc, pcm, sizeof(pcm) / sizeof(pcm[0]),
	    &emitted) == TNC1200_ERR_NO_DATA);
	CHECK(emitted == 0U);
	CHECK(tnc1200_tx_process(&tnc, pcm, sizeof(pcm) / sizeof(pcm[0]),
	    &emitted) == TNC1200_ERR_NO_DATA);
	CHECK(emitted == 0U);
	CHECK(tnc1200_tx_process(&tnc, pcm, sizeof(pcm) / sizeof(pcm[0]),
	    &emitted) == TNC1200_OK);
	CHECK(emitted != 0U);

	CHECK(tnc1200_init(&tnc, NULL) == TNC1200_OK);
	cmd[1] = KISS_CMD_P;
	cmd[2] = 0U;
	CHECK(tnc1200_host_input(&tnc, cmd, sizeof(cmd)) == TNC1200_OK);
	CHECK(tnc1200_host_input(&tnc, kiss_buf, kiss_len) == TNC1200_OK);
	CHECK(tnc1200_tx_process(&tnc, pcm, sizeof(pcm) / sizeof(pcm[0]),
	    &emitted) == TNC1200_ERR_NO_DATA);
	CHECK(emitted == 0U);
	CHECK(tnc1200_stats(&tnc, &stats) == TNC1200_OK);
	CHECK(stats.channel_tx_persistence_deferrals != 0U);
	CHECK(stats.tx_frames_started == 0U);

	return 0;
}

static int
test_tnc1200_dcd_gating(void)
{
	uint8_t frame[KILOTNC_AX25_MAX_FRAME];
	uint8_t kiss_buf[TEST_TNC_KISS_MAX];
	uint8_t cmd[4];
	int16_t pcm[TEST_TNC_PCM_MAX];
	struct tnc1200 tnc;
	struct tnc1200_stats stats;
	enum tnc_control_ptt ptt;
	size_t frame_len;
	size_t kiss_len;
	size_t samples;
	size_t emitted;

	CHECK(test_make_frame("dcd", frame, sizeof(frame), &frame_len) == 0);
	CHECK(test_make_kiss_data(frame, frame_len, kiss_buf, sizeof(kiss_buf),
	    &kiss_len) == 0);

	CHECK(tnc1200_init(&tnc, NULL) == TNC1200_OK);
	CHECK(tnc1200_set_dcd(&tnc, 1) == TNC1200_OK);
	CHECK(tnc1200_host_input(&tnc, kiss_buf, kiss_len) == TNC1200_OK);
	CHECK(tnc1200_tx_process(&tnc, pcm, sizeof(pcm) / sizeof(pcm[0]),
	    &emitted) == TNC1200_ERR_NO_DATA);
	CHECK(emitted == 0U);
	CHECK(tnc1200_ptt_state(&tnc, &ptt) == TNC1200_OK);
	CHECK(ptt == TNC_CONTROL_PTT_OFF);
	CHECK(tnc1200_stats(&tnc, &stats) == TNC1200_OK);
	CHECK(stats.channel_tx_denied_busy != 0U);

	CHECK(tnc1200_init(&tnc, NULL) == TNC1200_OK);
	cmd[0] = KISS_FEND;
	cmd[1] = KISS_CMD_FULLDUPLEX;
	cmd[2] = 1U;
	cmd[3] = KISS_FEND;
	CHECK(tnc1200_host_input(&tnc, cmd, sizeof(cmd)) == TNC1200_OK);
	CHECK(tnc1200_set_dcd(&tnc, 1) == TNC1200_OK);
	CHECK(tnc1200_host_input(&tnc, kiss_buf, kiss_len) == TNC1200_OK);
	CHECK(test_collect_tnc_tx(&tnc, 17U, pcm, sizeof(pcm) / sizeof(pcm[0]),
	    &samples) == 0);
	CHECK(samples != 0U);
	CHECK(tnc1200_stats(&tnc, &stats) == TNC1200_OK);
	CHECK(stats.tx_frames_done == 1U);

	return 0;
}

static int
test_decode_kiss_payload(const uint8_t *kiss_data, size_t kiss_len,
	uint8_t *payload, size_t payload_cap, size_t *payload_len)
{
	struct kiss_parser parser;
	struct kiss_frame frame;
	size_t frame_count;
	enum kiss_result res;

	kiss_parser_init(&parser);
	res = kiss_parse_bytes(&parser, kiss_data, kiss_len, &frame, 1U,
	    &frame_count);
	if (res != KISS_OK || frame_count != 1U)
		return __LINE__;
	if (frame.command != KISS_CMD_DATA || frame.len > payload_cap)
		return __LINE__;
	(void)memcpy(payload, frame.data, frame.len);
	*payload_len = frame.len;

	return 0;
}

static void
test_fill_addr(struct ax25_addr *addr, const char *callsign, uint8_t ssid)
{
	(void)memset(addr, 0, sizeof(*addr));
	(void)memcpy(addr->callsign, callsign, strlen(callsign) + 1U);
	addr->ssid = ssid;
	addr->repeated = 0;
}

static int
test_make_frame(const char *info, uint8_t *out, size_t out_cap,
	size_t *out_len)
{
	struct ax25_frame frame;
	enum ax25_result res;

	(void)memset(&frame, 0, sizeof(frame));
	test_fill_addr(&frame.dst, "APZKTN", 0U);
	test_fill_addr(&frame.src, "M6VPN", 0U);
	frame.pid = AX25_PID_NONE;
	frame.info_len = strlen(info);
	(void)memcpy(frame.info, info, frame.info_len);

	res = ax25_encode_ui_fcs(&frame, out, out_cap, out_len);
	if (res != AX25_OK)
		return __LINE__;

	return 0;
}

static int
test_make_kiss_data(const uint8_t *frame, size_t frame_len, uint8_t *out,
	size_t out_cap, size_t *out_len)
{
	enum kiss_result res;

	res = kiss_encode_frame(0U, KISS_CMD_DATA, frame, frame_len, out,
	    out_cap, out_len);
	if (res != KISS_OK)
		return __LINE__;

	return 0;
}

static int
test_tnc1200_commands_errors(void)
{
	uint8_t frame[KILOTNC_AX25_MAX_FRAME];
	uint8_t kiss_buf[TEST_TNC_KISS_MAX];
	uint8_t cmd[8];
	int16_t pcm[TEST_TNC_PCM_MAX];
	struct tnc1200 tnc;
	struct tnc1200_stats stats;
	size_t frame_len;
	size_t kiss_len;
	size_t rejected_before;
	size_t samples;
	size_t i;
	enum tnc1200_result res;

	CHECK(test_make_frame("cmd", frame, sizeof(frame), &frame_len) == 0);
	CHECK(test_make_kiss_data(frame, frame_len, kiss_buf, sizeof(kiss_buf),
	    &kiss_len) == 0);
	CHECK(tnc1200_init(&tnc, NULL) == TNC1200_OK);

	cmd[0] = KISS_FEND;
	cmd[1] = KISS_CMD_TXDELAY;
	cmd[2] = 3U;
	cmd[3] = KISS_FEND;
	CHECK(tnc1200_host_input(&tnc, cmd, 4U) == TNC1200_OK);
	CHECK(tnc.tx_config.txdelay_flags == 3U);
	cmd[1] = KISS_CMD_TXTAIL;
	cmd[2] = 1U;
	CHECK(tnc1200_host_input(&tnc, cmd, 4U) == TNC1200_OK);
	CHECK(tnc.tx_config.txtail_flags == 1U);
	cmd[1] = KISS_CMD_P;
	cmd[2] = 55U;
	CHECK(tnc1200_host_input(&tnc, cmd, 4U) == TNC1200_OK);
	CHECK(tnc.p == 55U);
	cmd[1] = KISS_CMD_SLOTTIME;
	cmd[2] = 9U;
	CHECK(tnc1200_host_input(&tnc, cmd, 4U) == TNC1200_OK);
	CHECK(tnc.slottime == 9U);
	cmd[1] = KISS_CMD_FULLDUPLEX;
	cmd[2] = 1U;
	CHECK(tnc1200_host_input(&tnc, cmd, 4U) == TNC1200_OK);
	CHECK(tnc.fullduplex == 1U);
	cmd[1] = KISS_CMD_RETURN;
	CHECK(tnc1200_host_input(&tnc, cmd, 3U) == TNC1200_OK);

	cmd[1] = 0x07U;
	cmd[2] = 1U;
	CHECK(tnc1200_host_input(&tnc, cmd, 4U) == TNC1200_OK);
	CHECK(tnc1200_stats(&tnc, &stats) == TNC1200_OK);
	CHECK(stats.kiss_ignored_commands == 1U);

	cmd[0] = KISS_FEND;
	cmd[1] = 0U;
	cmd[2] = KISS_FESC;
	cmd[3] = 0x01U;
	cmd[4] = KISS_FEND;
	CHECK(tnc1200_host_input(&tnc, cmd, 5U) == TNC1200_OK);
	CHECK(tnc1200_stats(&tnc, &stats) == TNC1200_OK);
	CHECK(stats.kiss_parse_errors == 1U);

	CHECK(tnc1200_host_input(&tnc, kiss_buf, kiss_len) == TNC1200_OK);
	CHECK(tnc1200_stats(&tnc, &stats) == TNC1200_OK);
	rejected_before = stats.tx_frames_rejected;
	res = tnc1200_host_input(&tnc, kiss_buf, kiss_len);
	CHECK(res == TNC1200_ERR_BUSY);
	CHECK(tnc1200_stats(&tnc, &stats) == TNC1200_OK);
	CHECK(stats.tx_frames_rejected == rejected_before + 1U);
	CHECK(test_collect_tnc_tx(&tnc, 31U, pcm, sizeof(pcm) / sizeof(pcm[0]),
	    &samples) == 0);
	CHECK(samples != 0U);

	for (i = 0U; i < sizeof(kiss_buf); i++)
		kiss_buf[i] = KISS_FEND;
	CHECK(tnc1200_rx_process(&tnc, pcm, samples, kiss_buf, 2U,
	    &kiss_len) == TNC1200_ERR_SMALL);
	CHECK(tnc1200_stats(&tnc, &stats) == TNC1200_OK);
	CHECK(stats.rx_frames_dropped == 1U);

	return 0;
}

static int
test_tnc1200_init_args(void)
{
	uint8_t buf[1];
	int16_t pcm[1];
	struct tnc1200 tnc;
	struct tnc1200_config config;
	struct tnc1200_stats stats;
	enum tnc_control_ptt ptt;
	size_t len;
	int can_emit;

	CHECK(tnc1200_init(NULL, NULL) == TNC1200_ERR_ARG);
	CHECK(tnc1200_init(&tnc, NULL) == TNC1200_OK);
	CHECK(tnc.tx_config.txdelay_flags == AFSK1200_TX_DEFAULT_TXDELAY_FLAGS);
	CHECK(tnc.tx_config.txtail_flags == AFSK1200_TX_DEFAULT_TXTAIL_FLAGS);

	(void)memset(&config, 0, sizeof(config));
	config.txdelay_flags = 2U;
	config.txtail_flags = 1U;
	config.amplitude = 6000;
	config.p = 255U;
	config.slottime_10ms = 1U;
	config.rng_seed = 1U;
	CHECK(tnc1200_init(&tnc, &config) == TNC1200_OK);
	CHECK(tnc.tx_config.txdelay_flags == 2U);
	CHECK(tnc.tx_config.txtail_flags == 1U);
	CHECK(tnc.tx_config.amplitude == 6000);
	config.amplitude = 0;
	CHECK(tnc1200_init(&tnc, &config) == TNC1200_ERR_ARG);

	CHECK(tnc1200_host_input(NULL, buf, sizeof(buf)) == TNC1200_ERR_ARG);
	CHECK(tnc1200_host_input(&tnc, NULL, 1U) == TNC1200_ERR_ARG);
	CHECK(tnc1200_tx_process(NULL, pcm, 1U, &len) == TNC1200_ERR_ARG);
	CHECK(tnc1200_tx_process(&tnc, NULL, 1U, &len) == TNC1200_ERR_ARG);
	CHECK(tnc1200_tx_process(&tnc, pcm, 1U, NULL) == TNC1200_ERR_ARG);
	CHECK(tnc1200_rx_process(NULL, pcm, 1U, buf, sizeof(buf), &len) ==
	    TNC1200_ERR_ARG);
	CHECK(tnc1200_rx_process(&tnc, NULL, 1U, buf, sizeof(buf), &len) ==
	    TNC1200_ERR_ARG);
	CHECK(tnc1200_rx_process(&tnc, pcm, 1U, NULL, sizeof(buf), &len) ==
	    TNC1200_ERR_ARG);
	CHECK(tnc1200_rx_process(&tnc, pcm, 1U, buf, sizeof(buf), NULL) ==
	    TNC1200_ERR_ARG);
	CHECK(tnc1200_abort_tx(NULL) == TNC1200_ERR_ARG);
	CHECK(tnc1200_stats(NULL, &stats) == TNC1200_ERR_ARG);
	CHECK(tnc1200_stats(&tnc, NULL) == TNC1200_ERR_ARG);
	CHECK(tnc1200_can_emit_audio(NULL, &can_emit) == TNC1200_ERR_ARG);
	CHECK(tnc1200_can_emit_audio(&tnc, NULL) == TNC1200_ERR_ARG);
	CHECK(tnc1200_ptt_state(NULL, &ptt) == TNC1200_ERR_ARG);
	CHECK(tnc1200_ptt_state(&tnc, NULL) == TNC1200_ERR_ARG);
	CHECK(tnc1200_set_dcd(NULL, 1) == TNC1200_ERR_ARG);

	return 0;
}

static int
test_tnc1200_loopback(void)
{
	uint8_t frame[KILOTNC_AX25_MAX_FRAME];
	uint8_t kiss_in[TEST_TNC_KISS_MAX];
	uint8_t kiss_out[TEST_TNC_KISS_MAX];
	uint8_t payload[KILOTNC_AX25_MAX_FRAME];
	int16_t pcm[TEST_TNC_PCM_MAX];
	struct tnc1200 tx_tnc;
	struct tnc1200 rx_tnc;
	struct tnc1200_stats stats;
	size_t frame_len;
	size_t kiss_in_len;
	size_t kiss_out_len;
	size_t payload_len;
	size_t samples;
	size_t emitted;
	size_t off;
	size_t take;

	CHECK(test_make_frame("loop", frame, sizeof(frame), &frame_len) == 0);
	CHECK(test_make_kiss_data(frame, frame_len, kiss_in, sizeof(kiss_in),
	    &kiss_in_len) == 0);
	CHECK(tnc1200_init(&tx_tnc, NULL) == TNC1200_OK);
	CHECK(tnc1200_init(&rx_tnc, NULL) == TNC1200_OK);
	for (off = 0U; off < kiss_in_len; off += 2U) {
		take = 2U;
		if (take > kiss_in_len - off)
			take = kiss_in_len - off;
		CHECK(tnc1200_host_input(&tx_tnc, &kiss_in[off], take) ==
		    TNC1200_OK);
	}
	CHECK(test_collect_tnc_tx(&tx_tnc, 7U, pcm,
	    sizeof(pcm) / sizeof(pcm[0]), &samples) == 0);

	kiss_out_len = 0U;
	for (off = 0U; off < samples; off += 13U) {
		take = 13U;
		if (take > samples - off)
			take = samples - off;
		CHECK(tnc1200_rx_process(&rx_tnc, &pcm[off], take,
		    &kiss_out[kiss_out_len], sizeof(kiss_out) - kiss_out_len,
		    &emitted) == TNC1200_OK);
		kiss_out_len += emitted;
	}
	CHECK(test_decode_kiss_payload(kiss_out, kiss_out_len, payload,
	    sizeof(payload), &payload_len) == 0);
	CHECK(payload_len == frame_len);
	CHECK(memcmp(payload, frame, frame_len) == 0);
	CHECK(tnc1200_stats(&tx_tnc, &stats) == TNC1200_OK);
	CHECK(stats.kiss_frames_in == 1U);
	CHECK(stats.tx_frames_started == 1U);
	CHECK(stats.tx_frames_done == 1U);
	CHECK(tnc1200_stats(&rx_tnc, &stats) == TNC1200_OK);
	CHECK(stats.kiss_frames_out == 1U);
	CHECK(stats.rx_frames_ok == 1U);

	return 0;
}

static int
test_tnc1200_mode_sethw(void)
{
	uint8_t frame[KILOTNC_AX25_MAX_FRAME];
	uint8_t kiss_buf[TEST_TNC_KISS_MAX];
	uint8_t sethw[1];
	int16_t pcm[TEST_TNC_PCM_MAX];
	struct tnc1200 tnc;
	struct tnc1200_stats stats;
	struct tnc1200_status status;
	size_t frame_len;
	size_t kiss_len;
	size_t sethw_len;
	size_t samples;

	CHECK(test_make_frame("mode", frame, sizeof(frame), &frame_len) == 0);
	CHECK(test_make_kiss_data(frame, frame_len, kiss_buf, sizeof(kiss_buf),
	    &kiss_len) == 0);
	CHECK(tnc1200_init(&tnc, NULL) == TNC1200_OK);
	CHECK(tnc1200_status(&tnc, &status) == TNC1200_OK);
	CHECK(status.current_mode == TNC_MODE_1200_AFSK_AX25);

	sethw[0] = 6U;
	CHECK(kiss_encode_frame(0U, KISS_CMD_SETHARDWARE, sethw,
	    sizeof(sethw), kiss_buf, sizeof(kiss_buf), &sethw_len) ==
	    KISS_OK);
	CHECK(tnc1200_host_input(&tnc, kiss_buf, sethw_len) == TNC1200_OK);
	CHECK(tnc1200_status(&tnc, &status) == TNC1200_OK);
	CHECK(status.current_mode == TNC_MODE_1200_AFSK_AX25);
	CHECK(status.last_requested_mode == TNC_MODE_1200_AFSK_AX25);
	CHECK(status.last_nino_sethw == 6U);
	CHECK(status.last_mode_temporary == 0U);
	CHECK(tnc1200_stats(&tnc, &stats) == TNC1200_OK);
	CHECK(stats.mode_set_requests == 1U);
	CHECK(stats.mode_set_unsupported == 0U);
	CHECK(stats.mode_set_invalid == 0U);

	sethw[0] = 22U;
	CHECK(kiss_encode_frame(0U, KISS_CMD_SETHARDWARE, sethw,
	    sizeof(sethw), kiss_buf, sizeof(kiss_buf), &sethw_len) ==
	    KISS_OK);
	CHECK(tnc1200_host_input(&tnc, kiss_buf, sethw_len) == TNC1200_OK);
	CHECK(tnc1200_status(&tnc, &status) == TNC1200_OK);
	CHECK(status.current_mode == TNC_MODE_1200_AFSK_AX25);
	CHECK(status.last_requested_mode == TNC_MODE_1200_AFSK_AX25);
	CHECK(status.last_nino_sethw == 22U);
	CHECK(status.last_mode_temporary == 1U);

	sethw[0] = 0U;
	CHECK(kiss_encode_frame(0U, KISS_CMD_SETHARDWARE, sethw,
	    sizeof(sethw), kiss_buf, sizeof(kiss_buf), &sethw_len) ==
	    KISS_OK);
	CHECK(tnc1200_host_input(&tnc, kiss_buf, sethw_len) == TNC1200_OK);
	CHECK(tnc1200_status(&tnc, &status) == TNC1200_OK);
	CHECK(status.current_mode == TNC_MODE_1200_AFSK_AX25);
	CHECK(status.last_requested_mode == TNC_MODE_9600_GFSK_AX25);
	CHECK(tnc1200_stats(&tnc, &stats) == TNC1200_OK);
	CHECK(stats.mode_set_unsupported == 1U);

	sethw[0] = 31U;
	CHECK(kiss_encode_frame(0U, KISS_CMD_SETHARDWARE, sethw,
	    sizeof(sethw), kiss_buf, sizeof(kiss_buf), &sethw_len) ==
	    KISS_OK);
	CHECK(tnc1200_host_input(&tnc, kiss_buf, sethw_len) == TNC1200_OK);
	CHECK(tnc1200_status(&tnc, &status) == TNC1200_OK);
	CHECK(status.current_mode == TNC_MODE_1200_AFSK_AX25);
	CHECK(status.last_requested_mode == TNC_MODE_UNSUPPORTED);
	CHECK(tnc1200_stats(&tnc, &stats) == TNC1200_OK);
	CHECK(stats.mode_set_invalid == 1U);

	sethw[0] = 6U;
	CHECK(kiss_encode_frame(0U, KISS_CMD_SETHARDWARE, sethw,
	    sizeof(sethw), kiss_buf, sizeof(kiss_buf), &sethw_len) ==
	    KISS_OK);
	CHECK(tnc1200_host_input(&tnc, kiss_buf, sethw_len) == TNC1200_OK);
	CHECK(test_make_kiss_data(frame, frame_len, kiss_buf, sizeof(kiss_buf),
	    &kiss_len) == 0);
	CHECK(tnc1200_host_input(&tnc, kiss_buf, kiss_len) == TNC1200_OK);
	CHECK(test_collect_tnc_tx(&tnc, 23U, pcm, sizeof(pcm) / sizeof(pcm[0]),
	    &samples) == 0);
	CHECK(samples != 0U);

	return 0;
}

static int
test_tnc1200_rx_to_kiss(void)
{
	uint8_t frame[KILOTNC_AX25_MAX_FRAME];
	uint8_t kiss_in[TEST_TNC_KISS_MAX];
	uint8_t kiss_out[TEST_TNC_KISS_MAX];
	uint8_t payload[KILOTNC_AX25_MAX_FRAME];
	int16_t pcm[TEST_TNC_PCM_MAX];
	struct tnc1200 tnc;
	size_t frame_len;
	size_t kiss_len;
	size_t payload_len;
	size_t samples;
	size_t out_len;

	CHECK(test_make_frame("rx", frame, sizeof(frame), &frame_len) == 0);
	CHECK(test_make_kiss_data(frame, frame_len, kiss_in, sizeof(kiss_in),
	    &kiss_len) == 0);
	CHECK(tnc1200_init(&tnc, NULL) == TNC1200_OK);
	CHECK(tnc1200_host_input(&tnc, kiss_in, kiss_len) == TNC1200_OK);
	CHECK(test_collect_tnc_tx(&tnc, 19U, pcm, sizeof(pcm) / sizeof(pcm[0]),
	    &samples) == 0);

	CHECK(tnc1200_init(&tnc, NULL) == TNC1200_OK);
	CHECK(tnc1200_rx_process(&tnc, pcm, samples, kiss_out,
	    sizeof(kiss_out), &out_len) == TNC1200_OK);
	CHECK(test_decode_kiss_payload(kiss_out, out_len, payload,
	    sizeof(payload), &payload_len) == 0);
	CHECK(payload_len == frame_len);
	CHECK(memcmp(payload, frame, frame_len) == 0);

	return 0;
}

static int
test_tnc1200_tail_abort_timeout(void)
{
	uint8_t frame[KILOTNC_AX25_MAX_FRAME];
	uint8_t kiss_buf[TEST_TNC_KISS_MAX];
	int16_t pcm[TEST_TNC_PCM_MAX];
	struct tnc1200 tnc;
	struct tnc1200_config config;
	struct tnc1200_stats stats;
	enum tnc_control_ptt ptt;
	size_t frame_len;
	size_t kiss_len;
	size_t samples;
	size_t emitted;

	CHECK(test_make_frame("tail", frame, sizeof(frame), &frame_len) == 0);
	CHECK(test_make_kiss_data(frame, frame_len, kiss_buf, sizeof(kiss_buf),
	    &kiss_len) == 0);

	(void)memset(&config, 0, sizeof(config));
	config.txdelay_flags = 0U;
	config.txtail_flags = 2U;
	config.amplitude = AFSK1200_PCM_AMPLITUDE;
	config.p = 255U;
	config.slottime_10ms = 1U;
	config.max_tx_ms = 30000U;
	config.rng_seed = 1U;
	CHECK(tnc1200_init(&tnc, &config) == TNC1200_OK);
	CHECK(tnc1200_host_input(&tnc, kiss_buf, kiss_len) == TNC1200_OK);
	CHECK(test_collect_tnc_tx(&tnc, 83U, pcm, sizeof(pcm) / sizeof(pcm[0]),
	    &samples) == 0);
	CHECK(samples != 0U);
	CHECK(tnc1200_ptt_state(&tnc, &ptt) == TNC1200_OK);
	CHECK(ptt == TNC_CONTROL_PTT_ON);
	CHECK(tnc1200_tx_process(&tnc, pcm, sizeof(pcm) / sizeof(pcm[0]),
	    &emitted) == TNC1200_ERR_NO_DATA);
	CHECK(emitted == 0U);
	CHECK(tnc1200_ptt_state(&tnc, &ptt) == TNC1200_OK);
	CHECK(ptt == TNC_CONTROL_PTT_OFF);

	CHECK(tnc1200_init(&tnc, &config) == TNC1200_OK);
	CHECK(tnc1200_host_input(&tnc, kiss_buf, kiss_len) == TNC1200_OK);
	CHECK(tnc1200_tx_process(&tnc, pcm, sizeof(pcm) / sizeof(pcm[0]),
	    &emitted) == TNC1200_OK);
	CHECK(emitted != 0U);
	CHECK(tnc1200_abort_tx(&tnc) == TNC1200_OK);
	CHECK(tnc1200_ptt_state(&tnc, &ptt) == TNC1200_OK);
	CHECK(ptt == TNC_CONTROL_PTT_OFF);

	(void)memset(&config, 0, sizeof(config));
	config.txdelay_flags = 0U;
	config.txtail_flags = 0U;
	config.amplitude = AFSK1200_PCM_AMPLITUDE;
	config.p = 255U;
	config.slottime_10ms = 1U;
	config.max_tx_ms = 10U;
	config.rng_seed = 1U;
	CHECK(tnc1200_init(&tnc, &config) == TNC1200_OK);
	CHECK(tnc1200_host_input(&tnc, kiss_buf, kiss_len) == TNC1200_OK);
	CHECK(tnc1200_tx_process(&tnc, pcm, sizeof(pcm) / sizeof(pcm[0]),
	    &emitted) == TNC1200_ERR_TIMEOUT);
	CHECK(emitted == 0U);
	CHECK(tnc1200_ptt_state(&tnc, &ptt) == TNC1200_OK);
	CHECK(ptt == TNC_CONTROL_PTT_OFF);
	CHECK(tnc1200_stats(&tnc, &stats) == TNC1200_OK);
	CHECK(stats.channel_tx_timeouts == 1U);

	return 0;
}

static int
test_tnc1200_tx_from_kiss(void)
{
	uint8_t frame[KILOTNC_AX25_MAX_FRAME];
	uint8_t kiss_buf[TEST_TNC_KISS_MAX];
	int16_t pcm[TEST_TNC_PCM_MAX];
	struct afsk1200_rx_frame rx_frames[1];
	struct afsk1200_stream stream;
	struct afsk1200_stream_frame stream_frames[1];
	struct tnc1200 tnc;
	struct tnc1200_stats stats;
	size_t frame_len;
	size_t kiss_len;
	size_t samples;
	size_t out_count;
	size_t emitted;
	size_t off;
	size_t take;

	CHECK(test_make_frame("tx", frame, sizeof(frame), &frame_len) == 0);
	CHECK(test_make_kiss_data(frame, frame_len, kiss_buf, sizeof(kiss_buf),
	    &kiss_len) == 0);
	CHECK(tnc1200_init(&tnc, NULL) == TNC1200_OK);
	CHECK(tnc1200_host_input(&tnc, kiss_buf, kiss_len) == TNC1200_OK);
	CHECK(test_collect_tnc_tx(&tnc, 5U, pcm, sizeof(pcm) / sizeof(pcm[0]),
	    &samples) == 0);
	CHECK(samples != 0U);
	CHECK(afsk1200_rx_decode_frames(pcm, samples, rx_frames, 1U,
	    &out_count, NULL) == AFSK1200_RX_OK);
	CHECK(out_count == 1U);
	CHECK(rx_frames[0].len == frame_len);
	CHECK(memcmp(rx_frames[0].data, frame, frame_len) == 0);

	CHECK(afsk1200_stream_init(&stream) == AFSK1200_STREAM_OK);
	out_count = 0U;
	for (off = 0U; off < samples; off += 11U) {
		take = 11U;
		if (take > samples - off)
			take = samples - off;
		CHECK(afsk1200_stream_process(&stream, &pcm[off], take,
		    &stream_frames[out_count], 1U - out_count, &emitted) ==
		    AFSK1200_STREAM_OK);
		out_count += emitted;
	}
	CHECK(out_count == 1U);
	CHECK(stream_frames[0].len == frame_len);
	CHECK(memcmp(stream_frames[0].data, frame, frame_len) == 0);
	CHECK(tnc1200_stats(&tnc, &stats) == TNC1200_OK);
	CHECK(stats.kiss_frames_in == 1U);
	CHECK(stats.tx_frames_started == 1U);
	CHECK(stats.tx_frames_done == 1U);
	CHECK(stats.pcm_samples_out == samples);
	CHECK(tnc1200_abort_tx(&tnc) == TNC1200_OK);

	return 0;
}
