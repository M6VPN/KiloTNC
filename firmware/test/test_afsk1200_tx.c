/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/test/test_afsk1200_tx.c */

#include <sys/types.h>

#include <stdint.h>
#include <string.h>

#include "afsk1200_rx.h"
#include "afsk1200_stream.h"
#include "afsk1200_tx.h"
#include "ax25.h"
#include "hdlc.h"

#define CHECK(expr) do { if (!(expr)) return __LINE__; } while (0)
#define TEST_TX_PCM_MAX \
	((AFSK1200_MAX_TEST_BITS + (80U * 8U)) * AFSK1200_SAMPLES_PER_BIT)

static int test_collect_tx(struct afsk1200_tx *, size_t, int16_t *,
	size_t, size_t *);
static int test_collect_tx_pattern(struct afsk1200_tx *, const size_t *,
	size_t, int16_t *, size_t, size_t *);
static void test_fill_addr(struct ax25_addr *, const char *, uint8_t);
static int test_make_frame(const char *, uint8_t *, size_t, size_t *);
static int test_stuffed_bits(const uint8_t *, size_t, size_t *);
static int test_tx_abort_stats(void);
static int test_tx_chunking(void);
static int test_tx_decode_roundtrip(void);
static int test_tx_init_and_args(void);
static int test_tx_validation(void);

int
test_afsk1200_tx(void)
{
	int subres;

	subres = test_tx_init_and_args();
	if (subres != 0)
		return subres;
	subres = test_tx_validation();
	if (subres != 0)
		return subres;
	subres = test_tx_chunking();
	if (subres != 0)
		return subres;
	subres = test_tx_abort_stats();
	if (subres != 0)
		return subres;
	subres = test_tx_decode_roundtrip();
	if (subres != 0)
		return subres;

	return 0;
}

static int
test_collect_tx(struct afsk1200_tx *tx, size_t chunk, int16_t *pcm,
	size_t pcm_cap, size_t *out_samples)
{
	size_t chunks[1];

	chunks[0] = chunk;
	return test_collect_tx_pattern(tx, chunks, 1U, pcm, pcm_cap,
	    out_samples);
}

static int
test_collect_tx_pattern(struct afsk1200_tx *tx, const size_t *chunks,
	size_t chunk_count, int16_t *pcm, size_t pcm_cap, size_t *out_samples)
{
	size_t emitted;
	size_t off;
	size_t chunk_pos;
	size_t take;
	enum afsk1200_tx_result res;

	off = 0U;
	chunk_pos = 0U;
	for (;;) {
		take = chunks[chunk_pos % chunk_count];
		if (take > pcm_cap - off)
			take = pcm_cap - off;
		res = afsk1200_tx_process(tx, &pcm[off], take, &emitted);
		if (res == AFSK1200_TX_DONE) {
			off += emitted;
			break;
		}
		if (res != AFSK1200_TX_OK)
			return __LINE__;
		off += emitted;
		if (off >= pcm_cap)
			return __LINE__;
		chunk_pos++;
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
test_stuffed_bits(const uint8_t *frame, size_t frame_len, size_t *out_bits)
{
	uint8_t stuffed[(AFSK1200_MAX_TEST_BITS + 7U) / 8U];
	enum hdlc_result res;

	res = hdlc_bitstuff(frame, frame_len * 8U, stuffed, sizeof(stuffed),
	    out_bits);
	if (res != HDLC_OK)
		return __LINE__;

	return 0;
}

static int
test_tx_abort_stats(void)
{
	uint8_t frame[KILOTNC_AX25_MAX_FRAME];
	int16_t pcm[AFSK1200_SAMPLES_PER_BIT];
	struct afsk1200_tx tx;
	struct afsk1200_tx_stats stats;
	size_t frame_len;
	size_t emitted;
	int active;

	CHECK(test_make_frame("abort", frame, sizeof(frame), &frame_len) == 0);
	CHECK(afsk1200_tx_init(&tx, NULL) == AFSK1200_TX_OK);
	CHECK(afsk1200_tx_start_frame(&tx, frame, frame_len) == AFSK1200_TX_OK);
	CHECK(afsk1200_tx_process(&tx, pcm, sizeof(pcm) / sizeof(pcm[0]),
	    &emitted) == AFSK1200_TX_OK);
	CHECK(emitted == AFSK1200_SAMPLES_PER_BIT);
	CHECK(afsk1200_tx_abort(&tx) == AFSK1200_TX_OK);
	CHECK(afsk1200_tx_is_active(&tx, &active) == AFSK1200_TX_OK);
	CHECK(active == 0);
	CHECK(afsk1200_tx_process(&tx, pcm, sizeof(pcm) / sizeof(pcm[0]),
	    &emitted) == AFSK1200_TX_DONE);
	CHECK(emitted == 0U);
	CHECK(afsk1200_tx_stats(&tx, &stats) == AFSK1200_TX_OK);
	CHECK(stats.frames_queued == 1U);
	CHECK(stats.frames_done == 0U);
	CHECK(stats.samples_total == AFSK1200_SAMPLES_PER_BIT);

	CHECK(afsk1200_tx_start_frame(&tx, frame, frame_len) == AFSK1200_TX_OK);
	CHECK(test_collect_tx(&tx, 97U, pcm,
	    sizeof(pcm) / sizeof(pcm[0]), &emitted) != 0);

	return 0;
}

static int
test_tx_chunking(void)
{
	static int16_t pcm_one[TEST_TX_PCM_MAX];
	static int16_t pcm_irregular[TEST_TX_PCM_MAX];
	uint8_t frame[KILOTNC_AX25_MAX_FRAME];
	size_t chunks[] = { 1U, 7U, 39U, 40U, 113U, 3U };
	struct afsk1200_tx tx;
	struct afsk1200_tx_config config;
	struct afsk1200_tx_stats stats;
	size_t frame_len;
	size_t stuffed_bits;
	size_t expected_samples;
	size_t samples_one;
	size_t samples_irregular;
	size_t i;
	int active;

	CHECK(test_make_frame("chunk", frame, sizeof(frame), &frame_len) == 0);
	CHECK(test_stuffed_bits(frame, frame_len, &stuffed_bits) == 0);
	config.txdelay_flags = 3U;
	config.txtail_flags = 1U;
	config.amplitude = 9000;
	expected_samples = ((config.txdelay_flags + config.txtail_flags) *
	    8U + stuffed_bits) * AFSK1200_SAMPLES_PER_BIT;

	CHECK(afsk1200_tx_init(&tx, &config) == AFSK1200_TX_OK);
	CHECK(afsk1200_tx_start_frame(&tx, frame, frame_len) == AFSK1200_TX_OK);
	CHECK(test_collect_tx(&tx, 1U, pcm_one,
	    sizeof(pcm_one) / sizeof(pcm_one[0]), &samples_one) == 0);
	CHECK(samples_one == expected_samples);
	CHECK(afsk1200_tx_is_active(&tx, &active) == AFSK1200_TX_OK);
	CHECK(active == 0);
	CHECK(afsk1200_tx_stats(&tx, &stats) == AFSK1200_TX_OK);
	CHECK(stats.frames_done == 1U);
	CHECK(stats.bits_total == expected_samples / AFSK1200_SAMPLES_PER_BIT);
	CHECK(stats.samples_total == expected_samples);
	CHECK(stats.chunks_emitted == expected_samples);

	for (i = 0U; i < samples_one; i++)
		CHECK(pcm_one[i] >= -32768 && pcm_one[i] <= 32767);

	CHECK(afsk1200_tx_start_frame(&tx, frame, frame_len) == AFSK1200_TX_OK);
	CHECK(test_collect_tx_pattern(&tx, chunks,
	    sizeof(chunks) / sizeof(chunks[0]), pcm_irregular,
	    sizeof(pcm_irregular) / sizeof(pcm_irregular[0]),
	    &samples_irregular) == 0);
	CHECK(samples_irregular == expected_samples);

	return 0;
}

static int
test_tx_decode_roundtrip(void)
{
	static int16_t pcm[TEST_TX_PCM_MAX];
	uint8_t frame[KILOTNC_AX25_MAX_FRAME];
	struct afsk1200_rx_frame rx_frames[1];
	struct afsk1200_rx_stats rx_stats;
	struct afsk1200_stream stream;
	struct afsk1200_stream_frame stream_frames[1];
	struct afsk1200_stream_stats stream_stats;
	struct afsk1200_tx tx;
	struct afsk1200_tx_config config;
	size_t frame_len;
	size_t samples;
	size_t out_count;
	size_t emitted;
	size_t off;
	size_t take;
	enum afsk1200_rx_result rxres;
	enum afsk1200_stream_result sres;

	CHECK(test_make_frame("roundtrip", frame, sizeof(frame),
	    &frame_len) == 0);
	config.txdelay_flags = 4U;
	config.txtail_flags = 2U;
	config.amplitude = AFSK1200_PCM_AMPLITUDE;
	CHECK(afsk1200_tx_init(&tx, &config) == AFSK1200_TX_OK);
	CHECK(afsk1200_tx_start_frame(&tx, frame, frame_len) == AFSK1200_TX_OK);
	CHECK(test_collect_tx(&tx, 23U, pcm, sizeof(pcm) / sizeof(pcm[0]),
	    &samples) == 0);

	rxres = afsk1200_rx_decode_frames(pcm, samples, rx_frames, 1U,
	    &out_count, &rx_stats);
	CHECK(rxres == AFSK1200_RX_OK);
	CHECK(out_count == 1U);
	CHECK(rx_frames[0].len == frame_len);
	CHECK(memcmp(rx_frames[0].data, frame, frame_len) == 0);

	CHECK(afsk1200_stream_init(&stream) == AFSK1200_STREAM_OK);
	off = 0U;
	out_count = 0U;
	while (off < samples) {
		take = 17U;
		if (take > samples - off)
			take = samples - off;
		sres = afsk1200_stream_process(&stream, &pcm[off], take,
		    &stream_frames[out_count], 1U - out_count, &emitted);
		CHECK(sres == AFSK1200_STREAM_OK);
		out_count += emitted;
		off += take;
	}
	CHECK(afsk1200_stream_flush(&stream, stream_frames, 1U, &emitted) ==
	    AFSK1200_STREAM_OK);
	CHECK(emitted == 0U);
	CHECK(out_count == 1U);
	CHECK(stream_frames[0].len == frame_len);
	CHECK(memcmp(stream_frames[0].data, frame, frame_len) == 0);
	CHECK(afsk1200_stream_stats(&stream, &stream_stats) ==
	    AFSK1200_STREAM_OK);
	CHECK(stream_stats.frames_ok == 1U);

	return 0;
}

static int
test_tx_init_and_args(void)
{
	uint8_t frame[KILOTNC_AX25_MAX_FRAME];
	int16_t pcm[AFSK1200_SAMPLES_PER_BIT];
	struct afsk1200_tx tx;
	struct afsk1200_tx_config config;
	struct afsk1200_tx_stats stats;
	size_t frame_len;
	size_t emitted;
	int active;

	CHECK(test_make_frame("args", frame, sizeof(frame), &frame_len) == 0);
	CHECK(afsk1200_tx_init(NULL, NULL) == AFSK1200_TX_ERR_ARG);
	CHECK(afsk1200_tx_init(&tx, NULL) == AFSK1200_TX_OK);
	CHECK(tx.config.txdelay_flags == AFSK1200_TX_DEFAULT_TXDELAY_FLAGS);
	CHECK(tx.config.txtail_flags == AFSK1200_TX_DEFAULT_TXTAIL_FLAGS);
	CHECK(tx.config.amplitude == AFSK1200_PCM_AMPLITUDE);

	config.txdelay_flags = 0U;
	config.txtail_flags = 0U;
	config.amplitude = 6000;
	CHECK(afsk1200_tx_init(&tx, &config) == AFSK1200_TX_OK);
	CHECK(tx.config.txdelay_flags == 0U);
	CHECK(tx.config.txtail_flags == 0U);
	CHECK(tx.config.amplitude == 6000);

	config.amplitude = 0;
	CHECK(afsk1200_tx_init(&tx, &config) == AFSK1200_TX_ERR_ARG);
	config.amplitude = -1;
	CHECK(afsk1200_tx_init(&tx, &config) == AFSK1200_TX_ERR_ARG);

	CHECK(afsk1200_tx_abort(NULL) == AFSK1200_TX_ERR_ARG);
	CHECK(afsk1200_tx_is_active(NULL, &active) == AFSK1200_TX_ERR_ARG);
	CHECK(afsk1200_tx_is_active(&tx, NULL) == AFSK1200_TX_ERR_ARG);
	CHECK(afsk1200_tx_stats(NULL, &stats) == AFSK1200_TX_ERR_ARG);
	CHECK(afsk1200_tx_stats(&tx, NULL) == AFSK1200_TX_ERR_ARG);
	CHECK(afsk1200_tx_process(NULL, pcm, sizeof(pcm) / sizeof(pcm[0]),
	    &emitted) == AFSK1200_TX_ERR_ARG);
	CHECK(afsk1200_tx_process(&tx, NULL, sizeof(pcm) / sizeof(pcm[0]),
	    &emitted) == AFSK1200_TX_ERR_ARG);
	CHECK(afsk1200_tx_process(&tx, pcm, sizeof(pcm) / sizeof(pcm[0]),
	    NULL) == AFSK1200_TX_ERR_ARG);
	CHECK(afsk1200_tx_start_frame(NULL, frame, frame_len) ==
	    AFSK1200_TX_ERR_ARG);
	CHECK(afsk1200_tx_start_frame(&tx, NULL, frame_len) ==
	    AFSK1200_TX_ERR_ARG);

	CHECK(afsk1200_tx_start_frame(&tx, frame, frame_len) == AFSK1200_TX_OK);
	CHECK(afsk1200_tx_process(&tx, pcm, 0U, &emitted) ==
	    AFSK1200_TX_ERR_SMALL);
	CHECK(afsk1200_tx_stats(&tx, &stats) == AFSK1200_TX_OK);
	CHECK(stats.underruns == 1U);

	return 0;
}

static int
test_tx_validation(void)
{
	uint8_t frame[KILOTNC_AX25_MAX_FRAME];
	uint8_t oversize[KILOTNC_AX25_MAX_FRAME + 1U];
	int16_t pcm[AFSK1200_SAMPLES_PER_BIT];
	struct afsk1200_tx tx;
	struct afsk1200_tx_stats stats;
	size_t frame_len;
	size_t emitted;

	CHECK(test_make_frame("valid", frame, sizeof(frame), &frame_len) == 0);
	CHECK(afsk1200_tx_init(&tx, NULL) == AFSK1200_TX_OK);
	CHECK(afsk1200_tx_start_frame(&tx, frame, 0U) ==
	    AFSK1200_TX_ERR_BAD_FRAME);
	CHECK(afsk1200_tx_start_frame(&tx, oversize, sizeof(oversize)) ==
	    AFSK1200_TX_ERR_BAD_FRAME);
	frame[frame_len - 1U] ^= 0x01U;
	CHECK(afsk1200_tx_start_frame(&tx, frame, frame_len) ==
	    AFSK1200_TX_ERR_BAD_FRAME);
	CHECK(afsk1200_tx_stats(&tx, &stats) == AFSK1200_TX_OK);
	CHECK(stats.frames_rejected == 3U);

	CHECK(test_make_frame("valid", frame, sizeof(frame), &frame_len) == 0);
	CHECK(afsk1200_tx_start_frame(&tx, frame, frame_len) == AFSK1200_TX_OK);
	CHECK(afsk1200_tx_start_frame(&tx, frame, frame_len) ==
	    AFSK1200_TX_ERR_BUSY);
	CHECK(afsk1200_tx_process(&tx, pcm, AFSK1200_SAMPLES_PER_BIT,
	    &emitted) == AFSK1200_TX_OK);
	CHECK(emitted == AFSK1200_SAMPLES_PER_BIT);

	return 0;
}
