/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/test/test_tnc_diag.c */

#include <sys/types.h>

#include <stdint.h>
#include <string.h>

#include "ax25.h"
#include "kiss.h"
#include "tnc_diag.h"

#define CHECK(expr) do { if (!(expr)) return __LINE__; } while (0)
#define TEST_DIAG_PCM_MAX \
	((AFSK1200_MAX_TEST_BITS + (80U * 8U)) * AFSK1200_SAMPLES_PER_BIT)
#define TEST_DIAG_KISS_MAX	(KILOTNC_KISS_MAX_FRAME * 2U)

static int test_collect_tnc_tx(struct tnc1200 *, size_t, int16_t *,
	size_t, size_t *);
static void test_fill_addr(struct ax25_addr *, const char *, uint8_t);
static enum tnc_diag_fault test_fault_for_index(size_t);
static int test_make_frame(const char *, uint8_t *, size_t, size_t *);
static int test_make_kiss_data(const uint8_t *, size_t, uint8_t *, size_t,
	size_t *);
static int test_tnc_diag_capture_rx(void);
static int test_tnc_diag_capture_states(void);
static int test_tnc_diag_fault_ring(void);
static int test_tnc_diag_format(void);
static int test_tnc_diag_init_args(void);

int
test_tnc_diag(void)
{
	int subres;

	subres = test_tnc_diag_init_args();
	if (subres != 0)
		return subres;
	subres = test_tnc_diag_fault_ring();
	if (subres != 0)
		return subres;
	subres = test_tnc_diag_capture_states();
	if (subres != 0)
		return subres;
	subres = test_tnc_diag_capture_rx();
	if (subres != 0)
		return subres;
	subres = test_tnc_diag_format();
	if (subres != 0)
		return subres;

	return 0;
}

static int
test_collect_tnc_tx(struct tnc1200 *tnc, size_t chunk, int16_t *pcm,
	size_t pcm_cap, size_t *out_samples)
{
	size_t emitted;
	size_t idle_count;
	size_t off;
	enum tnc1200_result res;

	idle_count = 0U;
	off = 0U;
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

static void
test_fill_addr(struct ax25_addr *addr, const char *callsign, uint8_t ssid)
{
	(void)memset(addr, 0, sizeof(*addr));
	(void)memcpy(addr->callsign, callsign, strlen(callsign) + 1U);
	addr->ssid = ssid;
	addr->repeated = 0;
}

static enum tnc_diag_fault
test_fault_for_index(size_t idx)
{
	static const enum tnc_diag_fault faults[] = {
		TNC_DIAG_FAULT_KISS_PARSE,
		TNC_DIAG_FAULT_KISS_OVERLENGTH,
		TNC_DIAG_FAULT_TX_BUSY_DROP,
		TNC_DIAG_FAULT_RX_BAD_FCS,
		TNC_DIAG_FAULT_RX_MALFORMED,
		TNC_DIAG_FAULT_RX_OVERSIZE,
		TNC_DIAG_FAULT_RX_OUTPUT_DROP,
		TNC_DIAG_FAULT_TX_TIMEOUT,
		TNC_DIAG_FAULT_TX_ABORT,
		TNC_DIAG_FAULT_AUDIO_UNDERRUN,
		TNC_DIAG_FAULT_AUDIO_OVERRUN
	};

	return faults[idx % (sizeof(faults) / sizeof(faults[0]))];
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
test_tnc_diag_capture_rx(void)
{
	static int16_t pcm[TEST_DIAG_PCM_MAX];
	uint8_t frame[KILOTNC_AX25_MAX_FRAME];
	uint8_t kiss_in[TEST_DIAG_KISS_MAX];
	uint8_t kiss_out[TEST_DIAG_KISS_MAX];
	struct tnc1200 tx_tnc;
	struct tnc1200 rx_tnc;
	struct tnc_diag diag;
	struct tnc_diag_snapshot snapshot;
	size_t frame_len;
	size_t kiss_in_len;
	size_t kiss_out_len;
	size_t samples;

	CHECK(test_make_frame("diag-rx", frame, sizeof(frame),
	    &frame_len) == 0);
	CHECK(test_make_kiss_data(frame, frame_len, kiss_in, sizeof(kiss_in),
	    &kiss_in_len) == 0);
	CHECK(tnc1200_init(&tx_tnc, NULL) == TNC1200_OK);
	CHECK(tnc1200_host_input(&tx_tnc, kiss_in, kiss_in_len) ==
	    TNC1200_OK);
	CHECK(test_collect_tnc_tx(&tx_tnc, 29U, pcm,
	    sizeof(pcm) / sizeof(pcm[0]), &samples) == 0);

	CHECK(tnc1200_init(&rx_tnc, NULL) == TNC1200_OK);
	CHECK(tnc1200_rx_process(&rx_tnc, pcm, samples, kiss_out,
	    sizeof(kiss_out), &kiss_out_len) == TNC1200_OK);
	CHECK(kiss_out_len != 0U);
	CHECK(tnc_diag_init(&diag) == TNC_DIAG_OK);
	CHECK(tnc_diag_capture_tnc1200(&diag, &rx_tnc) == TNC_DIAG_OK);
	CHECK(tnc_diag_snapshot(&diag, &snapshot) == TNC_DIAG_OK);
	CHECK(snapshot.kiss_frames_out == 1U);
	CHECK(snapshot.rx_frames_ok == 1U);
	CHECK(snapshot.rx_samples_in == samples);
	CHECK(snapshot.rx_dcd_score != 0U);
	CHECK(snapshot.rx_confidence_avg != 0U);

	return 0;
}

static int
test_tnc_diag_capture_states(void)
{
	static int16_t pcm[TEST_DIAG_PCM_MAX];
	uint8_t frame[KILOTNC_AX25_MAX_FRAME];
	uint8_t kiss_buf[TEST_DIAG_KISS_MAX];
	uint8_t cmd[4];
	struct tnc1200 tnc;
	struct tnc_diag diag;
	struct tnc_diag_snapshot snapshot;
	size_t frame_len;
	size_t kiss_len;
	size_t samples;
	size_t emitted;

	CHECK(test_make_frame("diag", frame, sizeof(frame), &frame_len) == 0);
	CHECK(test_make_kiss_data(frame, frame_len, kiss_buf, sizeof(kiss_buf),
	    &kiss_len) == 0);
	CHECK(tnc1200_init(&tnc, NULL) == TNC1200_OK);
	CHECK(tnc_diag_init(&diag) == TNC_DIAG_OK);
	CHECK(tnc_diag_capture_tnc1200(&diag, &tnc) == TNC_DIAG_OK);
	CHECK(tnc_diag_snapshot(&diag, &snapshot) == TNC_DIAG_OK);
	CHECK(snapshot.p == 255U);
	CHECK(snapshot.slottime_10ms == 10U);
	CHECK(snapshot.fullduplex == 0U);
	CHECK(snapshot.ptt_state == TNC_CONTROL_PTT_OFF);
	CHECK(snapshot.current_mode == TNC_MODE_1200_AFSK_AX25);
	CHECK(snapshot.last_requested_mode == TNC_MODE_UNSUPPORTED);

	cmd[0] = KISS_FEND;
	cmd[1] = KISS_CMD_P;
	cmd[2] = 99U;
	cmd[3] = KISS_FEND;
	CHECK(tnc1200_host_input(&tnc, cmd, sizeof(cmd)) == TNC1200_OK);
	cmd[1] = KISS_CMD_SLOTTIME;
	cmd[2] = 7U;
	CHECK(tnc1200_host_input(&tnc, cmd, sizeof(cmd)) == TNC1200_OK);
	cmd[1] = KISS_CMD_FULLDUPLEX;
	cmd[2] = 1U;
	CHECK(tnc1200_host_input(&tnc, cmd, sizeof(cmd)) == TNC1200_OK);
	cmd[1] = KISS_CMD_TXDELAY;
	cmd[2] = 2U;
	CHECK(tnc1200_host_input(&tnc, cmd, sizeof(cmd)) == TNC1200_OK);
	CHECK(tnc1200_set_dcd(&tnc, 1) == TNC1200_OK);
	CHECK(tnc_diag_capture_tnc1200(&diag, &tnc) == TNC_DIAG_OK);
	CHECK(tnc_diag_snapshot(&diag, &snapshot) == TNC_DIAG_OK);
	CHECK(snapshot.p == 99U);
	CHECK(snapshot.slottime_10ms == 7U);
	CHECK(snapshot.fullduplex == 1U);
	CHECK(snapshot.dcd_busy == 1U);
	cmd[1] = KISS_CMD_SETHARDWARE;
	cmd[2] = 0U;
	CHECK(tnc1200_host_input(&tnc, cmd, sizeof(cmd)) == TNC1200_OK);
	cmd[2] = 31U;
	CHECK(tnc1200_host_input(&tnc, cmd, sizeof(cmd)) == TNC1200_OK);
	CHECK(tnc_diag_capture_tnc1200(&diag, &tnc) == TNC_DIAG_OK);
	CHECK(tnc_diag_snapshot(&diag, &snapshot) == TNC_DIAG_OK);
	CHECK(snapshot.current_mode == TNC_MODE_1200_AFSK_AX25);
	CHECK(snapshot.last_requested_mode == TNC_MODE_UNSUPPORTED);
	CHECK(snapshot.mode_set_requests == 2U);
	CHECK(snapshot.mode_set_unsupported == 1U);
	CHECK(snapshot.mode_set_invalid == 1U);
	cmd[1] = KISS_CMD_P;
	cmd[2] = 255U;
	CHECK(tnc1200_host_input(&tnc, cmd, sizeof(cmd)) == TNC1200_OK);
	CHECK(tnc1200_host_input(&tnc, kiss_buf, kiss_len) == TNC1200_OK);
	CHECK(tnc_diag_capture_tnc1200(&diag, &tnc) == TNC_DIAG_OK);
	CHECK(tnc_diag_snapshot(&diag, &snapshot) == TNC_DIAG_OK);
	CHECK(snapshot.kiss_frames_in == 1U);
	CHECK(snapshot.channel_tx_requests == 1U);
	CHECK(snapshot.p == 255U);
	CHECK(snapshot.slottime_10ms == 7U);
	CHECK(snapshot.fullduplex == 1U);
	CHECK(snapshot.dcd_busy == 1U);
	CHECK(snapshot.ptt_state == TNC_CONTROL_PTT_ON);
	CHECK(snapshot.audio_ready == 0U);

	CHECK(tnc1200_tx_process(&tnc, pcm, AFSK1200_SAMPLES_PER_BIT,
	    &emitted) == TNC1200_ERR_NO_DATA);
	CHECK(tnc1200_tx_process(&tnc, pcm, AFSK1200_SAMPLES_PER_BIT,
	    &emitted) == TNC1200_OK);
	CHECK(emitted != 0U);
	CHECK(tnc_diag_capture_tnc1200(&diag, &tnc) == TNC_DIAG_OK);
	CHECK(tnc_diag_snapshot(&diag, &snapshot) == TNC_DIAG_OK);
	CHECK(snapshot.tx_frames_started == 1U);
	CHECK(snapshot.tx_active == 1U);
	CHECK(snapshot.audio_ready == 1U);
	CHECK(snapshot.tx_samples_out == emitted);

	CHECK(test_collect_tnc_tx(&tnc, 37U, pcm,
	    sizeof(pcm) / sizeof(pcm[0]), &samples) == 0);
	CHECK(samples != 0U);
	CHECK(tnc_diag_capture_tnc1200(&diag, &tnc) == TNC_DIAG_OK);
	CHECK(tnc_diag_snapshot(&diag, &snapshot) == TNC_DIAG_OK);
	CHECK(snapshot.tx_frames_done == 1U);
	CHECK(snapshot.tx_samples_out > emitted);

	return 0;
}

static int
test_tnc_diag_fault_ring(void)
{
	struct tnc_diag diag;
	struct tnc_diag_snapshot snapshot;
	enum tnc_diag_fault faults[TNC_DIAG_FAULT_RING];
	size_t count;
	size_t i;

	CHECK(tnc_diag_init(&diag) == TNC_DIAG_OK);
	CHECK(tnc_diag_record_fault(&diag, TNC_DIAG_FAULT_NONE) ==
	    TNC_DIAG_ERR_RANGE);
	CHECK(tnc_diag_record_fault(&diag, TNC_DIAG_FAULT_KISS_PARSE) ==
	    TNC_DIAG_OK);
	CHECK(tnc_diag_snapshot(&diag, &snapshot) == TNC_DIAG_OK);
	CHECK(snapshot.last_fault == TNC_DIAG_FAULT_KISS_PARSE);
	CHECK(tnc_diag_faults(&diag, faults, 0U, &count) ==
	    TNC_DIAG_ERR_SMALL);
	CHECK(count == 1U);
	CHECK(tnc_diag_faults(&diag, faults, 1U, &count) == TNC_DIAG_OK);
	CHECK(count == 1U);
	CHECK(faults[0] == TNC_DIAG_FAULT_KISS_PARSE);

	CHECK(tnc_diag_init(&diag) == TNC_DIAG_OK);
	for (i = 0U; i < 20U; i++) {
		CHECK(tnc_diag_record_fault(&diag, test_fault_for_index(i)) ==
		    TNC_DIAG_OK);
	}
	CHECK(tnc_diag_faults(&diag, faults, TNC_DIAG_FAULT_RING - 1U,
	    &count) == TNC_DIAG_ERR_SMALL);
	CHECK(count == TNC_DIAG_FAULT_RING);
	CHECK(tnc_diag_faults(&diag, faults, TNC_DIAG_FAULT_RING, &count) ==
	    TNC_DIAG_OK);
	CHECK(count == TNC_DIAG_FAULT_RING);
	for (i = 0U; i < TNC_DIAG_FAULT_RING; i++)
		CHECK(faults[i] == test_fault_for_index(i + 4U));
	CHECK(tnc_diag_snapshot(&diag, &snapshot) == TNC_DIAG_OK);
	CHECK(snapshot.last_fault == test_fault_for_index(19U));

	return 0;
}

static int
test_tnc_diag_format(void)
{
	char buf[768];
	char small[16];
	struct tnc_diag diag;
	struct tnc_diag_snapshot snapshot;
	size_t out_len;
	const char *expected;

	expected = "kiss_in=1 kiss_out=2 kiss_parse_errors=3 "
	    "kiss_ignored=4 tx_started=5 tx_done=6 tx_rejected=7 "
	    "tx_samples=8 rx_ok=9 rx_bad_fcs=10 rx_malformed=11 "
	    "rx_dropped=12 rx_samples=13 chan_req=14 chan_grant=15 "
	    "chan_busy=16 chan_defers=17 chan_timeouts=18 chan_aborts=19 "
	    "ptt_on=20 ptt_off=21 mode_req=22 mode_unsup=23 "
	    "mode_invalid=24 rx_dcd=25 rx_conf=26 p=27 slot=28 "
	    "fullduplex=1 ptt=1 tx_active=1 audio_ready=0 dcd=1 "
	    "current_mode=6 last_mode=0 last_nino=31 mode_temp=1 "
	    "last_fault=3";

	(void)memset(&snapshot, 0, sizeof(snapshot));
	snapshot.kiss_frames_in = 1U;
	snapshot.kiss_frames_out = 2U;
	snapshot.kiss_parse_errors = 3U;
	snapshot.kiss_ignored_commands = 4U;
	snapshot.tx_frames_started = 5U;
	snapshot.tx_frames_done = 6U;
	snapshot.tx_frames_rejected = 7U;
	snapshot.tx_samples_out = 8U;
	snapshot.rx_frames_ok = 9U;
	snapshot.rx_frames_bad_fcs = 10U;
	snapshot.rx_frames_malformed = 11U;
	snapshot.rx_frames_dropped = 12U;
	snapshot.rx_samples_in = 13U;
	snapshot.channel_tx_requests = 14U;
	snapshot.channel_tx_grants = 15U;
	snapshot.channel_tx_denied_busy = 16U;
	snapshot.channel_tx_persistence_deferrals = 17U;
	snapshot.channel_tx_timeouts = 18U;
	snapshot.channel_tx_aborts = 19U;
	snapshot.ptt_on_events = 20U;
	snapshot.ptt_off_events = 21U;
	snapshot.mode_set_requests = 22U;
	snapshot.mode_set_unsupported = 23U;
	snapshot.mode_set_invalid = 24U;
	snapshot.rx_dcd_score = 25U;
	snapshot.rx_confidence_avg = 26U;
	snapshot.p = 27U;
	snapshot.slottime_10ms = 28U;
	snapshot.fullduplex = 1U;
	snapshot.ptt_state = 1U;
	snapshot.tx_active = 1U;
	snapshot.audio_ready = 0U;
	snapshot.dcd_busy = 1U;
	snapshot.last_nino_sethw = 31U;
	snapshot.last_mode_temporary = 1U;
	snapshot.current_mode = TNC_MODE_1200_AFSK_AX25;
	snapshot.last_requested_mode = TNC_MODE_9600_GFSK_AX25;
	snapshot.last_fault = TNC_DIAG_FAULT_TX_BUSY_DROP;

	CHECK(tnc_diag_format_snapshot(NULL, buf, sizeof(buf), &out_len) ==
	    TNC_DIAG_ERR_ARG);
	CHECK(tnc_diag_format_snapshot(&snapshot, NULL, sizeof(buf),
	    &out_len) == TNC_DIAG_ERR_ARG);
	CHECK(tnc_diag_format_snapshot(&snapshot, buf, sizeof(buf), NULL) ==
	    TNC_DIAG_ERR_ARG);
	CHECK(tnc_diag_format_snapshot(&snapshot, small, 0U, &out_len) ==
	    TNC_DIAG_ERR_SMALL);
	CHECK(tnc_diag_format_snapshot(&snapshot, small, sizeof(small),
	    &out_len) == TNC_DIAG_ERR_SMALL);
	CHECK(tnc_diag_format_snapshot(&snapshot, buf, sizeof(buf),
	    &out_len) == TNC_DIAG_OK);
	CHECK(out_len == strlen(expected));
	CHECK(strcmp(buf, expected) == 0);

	CHECK(tnc_diag_init(&diag) == TNC_DIAG_OK);
	CHECK(tnc_diag_snapshot(&diag, &snapshot) == TNC_DIAG_OK);
	CHECK(snapshot.last_fault == TNC_DIAG_FAULT_NONE);

	return 0;
}

static int
test_tnc_diag_init_args(void)
{
	struct tnc_diag diag;
	struct tnc1200 tnc;
	struct tnc_diag_snapshot snapshot;
	enum tnc_diag_fault faults[1];
	size_t count;

	CHECK(tnc_diag_init(NULL) == TNC_DIAG_ERR_ARG);
	CHECK(tnc_diag_init(&diag) == TNC_DIAG_OK);
	CHECK(tnc_diag_capture_tnc1200(NULL, &tnc) == TNC_DIAG_ERR_ARG);
	CHECK(tnc_diag_capture_tnc1200(&diag, NULL) == TNC_DIAG_ERR_ARG);
	CHECK(tnc_diag_faults(NULL, faults, 1U, &count) == TNC_DIAG_ERR_ARG);
	CHECK(tnc_diag_faults(&diag, faults, 1U, NULL) == TNC_DIAG_ERR_ARG);
	CHECK(tnc_diag_faults(&diag, NULL, 0U, &count) == TNC_DIAG_OK);
	CHECK(count == 0U);
	CHECK(tnc_diag_record_fault(NULL, TNC_DIAG_FAULT_KISS_PARSE) ==
	    TNC_DIAG_ERR_ARG);
	CHECK(tnc_diag_snapshot(NULL, &snapshot) == TNC_DIAG_ERR_ARG);
	CHECK(tnc_diag_snapshot(&diag, NULL) == TNC_DIAG_ERR_ARG);

	CHECK(tnc1200_init(&tnc, NULL) == TNC1200_OK);
	CHECK(tnc_diag_capture_tnc1200(&diag, &tnc) == TNC_DIAG_OK);
	CHECK(tnc_diag_snapshot(&diag, &snapshot) == TNC_DIAG_OK);
	CHECK(snapshot.kiss_frames_in == 0U);
	CHECK(snapshot.p == 255U);
	CHECK(snapshot.slottime_10ms == 10U);
	CHECK(snapshot.current_mode == TNC_MODE_1200_AFSK_AX25);
	CHECK(snapshot.last_requested_mode == TNC_MODE_UNSUPPORTED);
	CHECK(snapshot.last_fault == TNC_DIAG_FAULT_NONE);

	return 0;
}
