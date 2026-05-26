/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/tests/test_embedded_modem.c */

#include <sys/types.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ax25.h"
#include "embedded_app.h"
#include "embedded_diag.h"
#include "embedded_modem.h"
#include "embedded_tnc.h"
#include "kiss.h"
#include "platform_stub.h"
#include "audio_stub.h"
#include "usb_cdc_stub.h"

#define TEST_MODEM_PCM_MAX 24000U

static int build_ax25_frame(uint8_t *, size_t, size_t *);
static int build_kiss_frame(uint8_t *, size_t, size_t *);
static int build_pcm_frame(int16_t *, size_t, size_t *);
static int build_sethardware_frame(uint8_t, uint8_t *, size_t, size_t *);
static int check_decoded_frame(const uint8_t *, size_t);
static int check_kiss_output(const uint8_t *, size_t);
static int drain_modem(struct embedded_modem *, struct audio_stub *,
	size_t *);
static int feed_rx_pcm(struct embedded_modem *, struct audio_stub *,
	const int16_t *, size_t, size_t, struct embedded_modem_rx_frame *,
	size_t, size_t *);
static int test_embedded_modem_abort(void);
static int test_embedded_modem_app_step(void);
static int test_embedded_modem_busy(void);
static int test_embedded_modem_diag(void);
static int test_embedded_modem_init(void);
static int test_embedded_modem_null_args(void);
static int test_embedded_modem_process_tx(void);
static int test_embedded_modem_rejects_bad_frame(void);
static int test_embedded_modem_rejects_unsupported_mode(void);
static int test_embedded_modem_rx_decodes_frame(void);
static int test_embedded_modem_rx_diag(void);
static int test_embedded_modem_rx_empty_underflows(void);
static int test_embedded_modem_rx_noise_safe(void);
static int test_embedded_modem_rx_output_small(void);
static int test_embedded_modem_rx_reset(void);
static int test_embedded_modem_rx_small_chunks(void);
static int test_embedded_modem_shutdown_aborts(void);
static int test_embedded_modem_tnc_rx_disabled(void);
static int test_embedded_modem_tnc_rx_emits_kiss(void);
static int test_embedded_modem_tnc_rx_overflow_drop(void);
static int test_embedded_modem_tnc_disabled(void);
static int test_embedded_modem_tnc_malformed(void);
static int test_embedded_modem_tnc_starts_tx(void);
static int test_embedded_modem_tnc_unsupported_mode_blocks_tx(void);
static int test_embedded_modem_tnc_watchdog_blocks_rx(void);
static int test_embedded_modem_watchdog_aborts(void);

static int
build_ax25_frame(uint8_t *out, size_t out_cap, size_t *out_len)
{
	struct ax25_frame frame;

	(void)memset(&frame, 0, sizeof(frame));
	(void)memcpy(frame.dst.callsign, "APZKTN", 6);
	(void)memcpy(frame.src.callsign, "M6VPN", 5);
	frame.pid = AX25_PID_NONE;
	(void)memcpy(frame.info, "embedded modem", 14);
	frame.info_len = 14U;
	if (ax25_encode_ui_fcs(&frame, out, out_cap, out_len) != AX25_OK)
		return __LINE__;

	return 0;
}

static int
build_kiss_frame(uint8_t *out, size_t out_cap, size_t *out_len)
{
	uint8_t ax25[KILOTNC_AX25_MAX_FRAME];
	size_t ax25_len;
	int line;

	line = build_ax25_frame(ax25, sizeof(ax25), &ax25_len);
	if (line != 0)
		return line;
	if (kiss_encode_frame(0, KISS_CMD_DATA, ax25, ax25_len, out,
	    out_cap, out_len) != KISS_OK)
		return __LINE__;

	return 0;
}

static int
build_pcm_frame(int16_t *pcm, size_t pcm_cap, size_t *pcm_len)
{
	struct embedded_modem modem;
	struct audio_stub audio;
	int16_t chunk[AUDIO_STUB_BUFFER_MAX];
	uint8_t frame[KILOTNC_AX25_MAX_FRAME];
	enum embedded_modem_result result;
	size_t frame_len;
	size_t out_len;
	size_t loops;
	int line;

	if (pcm == NULL || pcm_len == NULL)
		return __LINE__;

	*pcm_len = 0U;
	line = build_ax25_frame(frame, sizeof(frame), &frame_len);
	if (line != 0)
		return line;
	audio_stub_init(&audio);
	if (embedded_modem_init(&modem) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (embedded_modem_start_ax25(&modem, frame, frame_len,
	    TNC_MODE_1200_AFSK_AX25) != EMBEDDED_MODEM_OK)
		return __LINE__;
	for (loops = 0U; loops < 4096U; loops++) {
		result = embedded_modem_process_tx(&modem,
		    audio_stub_audio(&audio), EMBEDDED_MODEM_TX_CHUNK_MAX);
		if (result != EMBEDDED_MODEM_OK &&
		    result != EMBEDDED_MODEM_DONE)
			return __LINE__;
		if (audio_stub_take_tx(&audio, chunk, sizeof(chunk) /
		    sizeof(chunk[0]), &out_len) == KILOTNC_AUDIO_OK) {
			if (out_len > pcm_cap - *pcm_len)
				return __LINE__;
			(void)memcpy(pcm + *pcm_len, chunk,
			    out_len * sizeof(chunk[0]));
			*pcm_len += out_len;
		}
		if (result == EMBEDDED_MODEM_DONE)
			return 0;
	}

	return __LINE__;
}

static int
build_sethardware_frame(uint8_t value, uint8_t *out, size_t out_cap,
	size_t *out_len)
{
	if (kiss_encode_frame(0, KISS_CMD_SETHARDWARE, &value, 1U, out,
	    out_cap, out_len) != KISS_OK)
		return __LINE__;

	return 0;
}

static int
check_decoded_frame(const uint8_t *frame, size_t frame_len)
{
	struct ax25_frame decoded;

	if (ax25_decode_ui_fcs(frame, frame_len, &decoded) != AX25_OK)
		return __LINE__;
	if (strcmp(decoded.dst.callsign, "APZKTN") != 0)
		return __LINE__;
	if (strcmp(decoded.src.callsign, "M6VPN") != 0)
		return __LINE__;
	if (decoded.info_len != 14U ||
	    memcmp(decoded.info, "embedded modem", 14U) != 0)
		return __LINE__;

	return 0;
}

static int
check_kiss_output(const uint8_t *buf, size_t len)
{
	struct kiss_parser parser;
	struct kiss_frame frames[1];
	size_t frame_count;
	int line;

	kiss_parser_init(&parser);
	if (kiss_parse_bytes(&parser, buf, len, frames,
	    sizeof(frames) / sizeof(frames[0]), &frame_count) != KISS_OK)
		return __LINE__;
	if (frame_count != 1U || frames[0].command != KISS_CMD_DATA)
		return __LINE__;
	line = check_decoded_frame(frames[0].data, frames[0].len);
	if (line != 0)
		return line;

	return 0;
}

static int
drain_modem(struct embedded_modem *modem, struct audio_stub *stub,
	size_t *total_samples)
{
	int16_t samples[AUDIO_STUB_BUFFER_MAX];
	enum embedded_modem_result result;
	size_t out_len;
	size_t loops;

	*total_samples = 0U;
	for (loops = 0U; loops < 4096U; loops++) {
		result = embedded_modem_process_tx(modem,
		    audio_stub_audio(stub), 16U);
		if (result != EMBEDDED_MODEM_OK &&
		    result != EMBEDDED_MODEM_DONE)
			return __LINE__;
		if (audio_stub_take_tx(stub, samples, sizeof(samples) /
		    sizeof(samples[0]), &out_len) == KILOTNC_AUDIO_OK)
			*total_samples += out_len;
		if (result == EMBEDDED_MODEM_DONE)
			return 0;
	}

	return __LINE__;
}

static int
feed_rx_pcm(struct embedded_modem *modem, struct audio_stub *audio,
	const int16_t *pcm, size_t pcm_len, size_t chunk_len,
	struct embedded_modem_rx_frame *frames, size_t frame_cap,
	size_t *frame_count)
{
	struct embedded_modem_rx_frame local[EMBEDDED_MODEM_RX_FRAME_CAP];
	enum embedded_modem_result result;
	size_t copied;
	size_t offset;
	size_t out_count;
	size_t take;
	size_t i;

	*frame_count = 0U;
	offset = 0U;
	while (offset < pcm_len) {
		take = pcm_len - offset;
		if (take > chunk_len)
			take = chunk_len;
		if (audio_stub_inject_rx(audio, pcm + offset, take) !=
		    KILOTNC_AUDIO_OK)
			return __LINE__;
		result = embedded_modem_process_rx(modem,
		    audio_stub_audio(audio), local,
		    EMBEDDED_MODEM_RX_FRAME_CAP, &out_count);
		if (result != EMBEDDED_MODEM_OK &&
		    result != EMBEDDED_MODEM_ERR_SMALL)
			return __LINE__;
		for (i = 0U; i < out_count; i++) {
			if (*frame_count >= frame_cap)
				return __LINE__;
			copied = local[i].len;
			(void)memcpy(frames[*frame_count].data, local[i].data,
			    copied);
			frames[*frame_count].len = copied;
			(*frame_count)++;
		}
		offset += take;
	}

	return 0;
}

static int
test_embedded_modem_abort(void)
{
	struct embedded_modem modem;
	struct embedded_modem_status status;
	uint8_t frame[KILOTNC_AX25_MAX_FRAME];
	size_t frame_len;
	int line;

	line = build_ax25_frame(frame, sizeof(frame), &frame_len);
	if (line != 0)
		return line;
	if (embedded_modem_init(&modem) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (embedded_modem_start_ax25(&modem, frame, frame_len,
	    TNC_MODE_1200_AFSK_AX25) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (embedded_modem_abort(&modem) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (embedded_modem_status(&modem, &status) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (status.tx_active != 0U || status.aborts != 1U)
		return __LINE__;

	return 0;
}

static int
test_embedded_modem_app_step(void)
{
	struct platform_stub platform;
	struct audio_stub audio;
	struct usb_cdc_stub usb;
	struct embedded_modem modem;
	struct embedded_tnc tnc;
	struct embedded_app app;
	struct embedded_app_status app_status;
	enum kilotnc_gpio_state ptt;
	uint8_t kiss[128];
	int16_t samples[AUDIO_STUB_BUFFER_MAX];
	size_t kiss_len;
	size_t out_len;
	int line;

	line = build_kiss_frame(kiss, sizeof(kiss), &kiss_len);
	if (line != 0)
		return line;

	platform_stub_init(&platform);
	audio_stub_init(&audio);
	usb_cdc_stub_init(&usb);
	if (usb_cdc_stub_set_connected(&usb, 1) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_modem_init(&modem) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (embedded_tnc_init(&tnc) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (embedded_tnc_modem(&tnc, &modem) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (embedded_tnc_set_modem_tx(&tnc, 1) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (embedded_app_init(&app, platform_stub_platform(&platform)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_app_tnc(&app, &tnc, usb_cdc_stub_usb(&usb)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_app_modem(&app, &modem, audio_stub_audio(&audio)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	if (usb_cdc_stub_inject_rx(&usb, kiss, kiss_len) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_app_step(&app) != EMBEDDED_APP_OK)
		return __LINE__;
	if (audio_stub_take_tx(&audio, samples, sizeof(samples) /
	    sizeof(samples[0]), &out_len) != KILOTNC_AUDIO_OK)
		return __LINE__;
	if (out_len == 0U)
		return __LINE__;
	if (embedded_app_status(&app, &app_status) != EMBEDDED_APP_OK)
		return __LINE__;
	if (app_status.watchdog_kicks != 1U)
		return __LINE__;
	if (platform_stub_ptt_state(&platform, &ptt) != KILOTNC_PLATFORM_OK)
		return __LINE__;
	if (ptt != KILOTNC_GPIO_LOW)
		return __LINE__;

	return 0;
}

static int
test_embedded_modem_busy(void)
{
	struct embedded_modem modem;
	uint8_t frame[KILOTNC_AX25_MAX_FRAME];
	size_t frame_len;
	int line;

	line = build_ax25_frame(frame, sizeof(frame), &frame_len);
	if (line != 0)
		return line;
	if (embedded_modem_init(&modem) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (embedded_modem_start_ax25(&modem, frame, frame_len,
	    TNC_MODE_1200_AFSK_AX25) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (embedded_modem_start_ax25(&modem, frame, frame_len,
	    TNC_MODE_1200_AFSK_AX25) != EMBEDDED_MODEM_ERR_BUSY)
		return __LINE__;

	return 0;
}

static int
test_embedded_modem_diag(void)
{
	struct platform_stub platform;
	struct audio_stub audio;
	struct usb_cdc_stub usb;
	struct embedded_modem modem;
	struct embedded_tnc tnc;
	struct embedded_app app;
	struct embedded_diag_snapshot snapshot;
	char formatted[1536];
	uint8_t kiss[128];
	size_t formatted_len;
	size_t kiss_len;
	int line;

	line = build_kiss_frame(kiss, sizeof(kiss), &kiss_len);
	if (line != 0)
		return line;

	platform_stub_init(&platform);
	audio_stub_init(&audio);
	usb_cdc_stub_init(&usb);
	if (usb_cdc_stub_set_connected(&usb, 1) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_modem_init(&modem) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (embedded_tnc_init(&tnc) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (embedded_tnc_modem(&tnc, &modem) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (embedded_tnc_set_modem_tx(&tnc, 1) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (embedded_app_init(&app, platform_stub_platform(&platform)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_app_tnc(&app, &tnc, usb_cdc_stub_usb(&usb)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_app_modem(&app, &modem, audio_stub_audio(&audio)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	if (usb_cdc_stub_inject_rx(&usb, kiss, kiss_len) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_app_step(&app) != EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_diag_capture(&app, &snapshot) != EMBEDDED_DIAG_OK)
		return __LINE__;
	if (snapshot.tnc_modem_tx_accepted != 1U ||
	    snapshot.modem_tx_samples_generated == 0U)
		return __LINE__;
	if (embedded_diag_format(&snapshot, formatted, sizeof(formatted),
	    &formatted_len) != EMBEDDED_DIAG_OK)
		return __LINE__;
	if (strstr(formatted, "modem_tx_frames_started=1") == NULL)
		return __LINE__;

	return 0;
}

static int
test_embedded_modem_init(void)
{
	struct embedded_modem modem;
	struct embedded_modem_status status;

	if (embedded_modem_init(&modem) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (embedded_modem_status(&modem, &status) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (status.tx_active != 0U || status.tx_done != 1U)
		return __LINE__;

	return 0;
}

static int
test_embedded_modem_null_args(void)
{
	struct embedded_modem modem;
	struct embedded_modem_rx_frame rx_frame;
	struct embedded_modem_status status;
	struct audio_stub audio;
	uint8_t frame[KILOTNC_AX25_MAX_FRAME];
	size_t frame_len;
	size_t frame_count;
	int line;

	line = build_ax25_frame(frame, sizeof(frame), &frame_len);
	if (line != 0)
		return line;
	audio_stub_init(&audio);
	if (embedded_modem_init(NULL) != EMBEDDED_MODEM_ERR_ARG)
		return __LINE__;
	if (embedded_modem_init(&modem) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (embedded_modem_start_ax25(NULL, frame, frame_len,
	    TNC_MODE_1200_AFSK_AX25) != EMBEDDED_MODEM_ERR_ARG)
		return __LINE__;
	if (embedded_modem_start_ax25(&modem, NULL, frame_len,
	    TNC_MODE_1200_AFSK_AX25) != EMBEDDED_MODEM_ERR_ARG)
		return __LINE__;
	if (embedded_modem_process_tx(NULL, audio_stub_audio(&audio), 16U) !=
	    EMBEDDED_MODEM_ERR_ARG)
		return __LINE__;
	if (embedded_modem_process_tx(&modem, NULL, 16U) !=
	    EMBEDDED_MODEM_ERR_ARG)
		return __LINE__;
	if (embedded_modem_process_rx(NULL, audio_stub_audio(&audio),
	    &rx_frame, 1U, &frame_count) != EMBEDDED_MODEM_ERR_ARG)
		return __LINE__;
	if (embedded_modem_process_rx(&modem, NULL, &rx_frame, 1U,
	    &frame_count) != EMBEDDED_MODEM_ERR_ARG)
		return __LINE__;
	if (embedded_modem_process_rx(&modem, audio_stub_audio(&audio), NULL,
	    1U, &frame_count) != EMBEDDED_MODEM_ERR_ARG)
		return __LINE__;
	if (embedded_modem_process_rx(&modem, audio_stub_audio(&audio),
	    &rx_frame, 1U, NULL) != EMBEDDED_MODEM_ERR_ARG)
		return __LINE__;
	if (embedded_modem_abort(NULL) != EMBEDDED_MODEM_ERR_ARG)
		return __LINE__;
	if (embedded_modem_rx_reset(NULL) != EMBEDDED_MODEM_ERR_ARG)
		return __LINE__;
	if (embedded_modem_status(NULL, &status) != EMBEDDED_MODEM_ERR_ARG)
		return __LINE__;
	if (embedded_modem_status(&modem, NULL) != EMBEDDED_MODEM_ERR_ARG)
		return __LINE__;

	return 0;
}

static int
test_embedded_modem_process_tx(void)
{
	struct embedded_modem modem;
	struct embedded_modem_status status;
	struct audio_stub audio;
	uint8_t frame[KILOTNC_AX25_MAX_FRAME];
	size_t frame_len;
	size_t total_samples;
	int line;

	line = build_ax25_frame(frame, sizeof(frame), &frame_len);
	if (line != 0)
		return line;
	audio_stub_init(&audio);
	if (embedded_modem_init(&modem) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (embedded_modem_start_ax25(&modem, frame, frame_len,
	    TNC_MODE_1200_AFSK_AX25) != EMBEDDED_MODEM_OK)
		return __LINE__;
	line = drain_modem(&modem, &audio, &total_samples);
	if (line != 0)
		return line;
	if (total_samples == 0U)
		return __LINE__;
	if (embedded_modem_status(&modem, &status) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (status.tx_active != 0U || status.tx_frames_done != 1U ||
	    status.tx_samples_generated == 0U)
		return __LINE__;

	return 0;
}

static int
test_embedded_modem_rejects_bad_frame(void)
{
	struct embedded_modem modem;
	uint8_t frame[3];

	frame[0] = 0U;
	frame[1] = 1U;
	frame[2] = 2U;
	if (embedded_modem_init(&modem) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (embedded_modem_start_ax25(&modem, frame, sizeof(frame),
	    TNC_MODE_1200_AFSK_AX25) != EMBEDDED_MODEM_ERR_SMALL)
		return __LINE__;

	return 0;
}

static int
test_embedded_modem_rejects_unsupported_mode(void)
{
	struct embedded_modem modem;
	uint8_t frame[KILOTNC_AX25_MAX_FRAME];
	size_t frame_len;
	int line;

	line = build_ax25_frame(frame, sizeof(frame), &frame_len);
	if (line != 0)
		return line;
	if (embedded_modem_init(&modem) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (embedded_modem_start_ax25(&modem, frame, frame_len,
	    TNC_MODE_9600_GFSK_AX25) != EMBEDDED_MODEM_ERR_UNSUPPORTED)
		return __LINE__;

	return 0;
}

static int
test_embedded_modem_rx_decodes_frame(void)
{
	struct audio_stub audio;
	struct embedded_modem modem;
	struct embedded_modem_rx_frame frames[2];
	struct embedded_modem_status status;
	int16_t pcm[TEST_MODEM_PCM_MAX];
	size_t frame_count;
	size_t pcm_len;
	int line;

	line = build_pcm_frame(pcm, sizeof(pcm) / sizeof(pcm[0]), &pcm_len);
	if (line != 0)
		return line;
	audio_stub_init(&audio);
	if (embedded_modem_init(&modem) != EMBEDDED_MODEM_OK)
		return __LINE__;
	line = feed_rx_pcm(&modem, &audio, pcm, pcm_len, 64U, frames,
	    sizeof(frames) / sizeof(frames[0]), &frame_count);
	if (line != 0)
		return line;
	if (frame_count != 1U)
		return __LINE__;
	line = check_decoded_frame(frames[0].data, frames[0].len);
	if (line != 0)
		return line;
	if (embedded_modem_status(&modem, &status) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (status.rx_frames_ok != 1U || status.rx_samples_consumed == 0U)
		return __LINE__;

	return 0;
}

static int
test_embedded_modem_rx_diag(void)
{
	struct platform_stub platform;
	struct audio_stub audio;
	struct usb_cdc_stub usb;
	struct embedded_modem modem;
	struct embedded_tnc tnc;
	struct embedded_app app;
	struct embedded_diag_snapshot snapshot;
	char formatted[1536];
	int16_t pcm[TEST_MODEM_PCM_MAX];
	size_t formatted_len;
	size_t offset;
	size_t pcm_len;
	size_t take;
	int line;

	line = build_pcm_frame(pcm, sizeof(pcm) / sizeof(pcm[0]), &pcm_len);
	if (line != 0)
		return line;
	platform_stub_init(&platform);
	audio_stub_init(&audio);
	usb_cdc_stub_init(&usb);
	if (usb_cdc_stub_set_connected(&usb, 1) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_modem_init(&modem) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (embedded_tnc_init(&tnc) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (embedded_tnc_modem(&tnc, &modem) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (embedded_tnc_set_modem_rx(&tnc, 1) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (embedded_app_init(&app, platform_stub_platform(&platform)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_app_tnc(&app, &tnc, usb_cdc_stub_usb(&usb)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_app_modem(&app, &modem, audio_stub_audio(&audio)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	for (offset = 0U; offset < pcm_len; offset += take) {
		take = pcm_len - offset;
		if (take > 64U)
			take = 64U;
		if (audio_stub_inject_rx(&audio, pcm + offset, take) !=
		    KILOTNC_AUDIO_OK)
			return __LINE__;
		if (embedded_app_step(&app) != EMBEDDED_APP_OK)
			return __LINE__;
	}
	if (embedded_diag_capture(&app, &snapshot) != EMBEDDED_DIAG_OK)
		return __LINE__;
	if (snapshot.modem_rx_frames_ok != 1U ||
	    snapshot.tnc_modem_rx_kiss_frames != 1U ||
	    snapshot.modem_rx_samples_consumed == 0U)
		return __LINE__;
	if (embedded_diag_format(&snapshot, formatted, sizeof(formatted),
	    &formatted_len) != EMBEDDED_DIAG_OK)
		return __LINE__;
	if (strstr(formatted, "modem_rx_frames_ok=1") == NULL ||
	    strstr(formatted, "tnc_modem_rx_kiss_frames=1") == NULL)
		return __LINE__;

	return 0;
}

static int
test_embedded_modem_rx_empty_underflows(void)
{
	struct audio_stub audio;
	struct embedded_modem modem;
	struct embedded_modem_rx_frame frame;
	struct embedded_modem_status status;
	size_t frame_count;

	audio_stub_init(&audio);
	if (embedded_modem_init(&modem) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (embedded_modem_process_rx(&modem, audio_stub_audio(&audio),
	    &frame, 1U, &frame_count) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (frame_count != 0U)
		return __LINE__;
	if (embedded_modem_status(&modem, &status) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (status.rx_audio_underflows != 1U)
		return __LINE__;

	return 0;
}

static int
test_embedded_modem_rx_noise_safe(void)
{
	struct audio_stub audio;
	struct embedded_modem modem;
	struct embedded_modem_rx_frame frames[2];
	struct embedded_modem_status status;
	int16_t noise[128];
	size_t frame_count;
	size_t out_count;
	size_t i;

	for (i = 0U; i < sizeof(noise) / sizeof(noise[0]); i++)
		noise[i] = (int16_t)((i % 2U) == 0U ? 1 : -1);
	audio_stub_init(&audio);
	if (audio_stub_inject_rx(&audio, noise,
	    sizeof(noise) / sizeof(noise[0])) != KILOTNC_AUDIO_OK)
		return __LINE__;
	if (embedded_modem_init(&modem) != EMBEDDED_MODEM_OK)
		return __LINE__;
	frame_count = 0U;
	for (i = 0U; i < 2U; i++) {
		if (embedded_modem_process_rx(&modem,
		    audio_stub_audio(&audio), frames,
		    sizeof(frames) / sizeof(frames[0]), &out_count) !=
		    EMBEDDED_MODEM_OK)
			return __LINE__;
		frame_count += out_count;
	}
	if (frame_count != 0U)
		return __LINE__;
	if (embedded_modem_status(&modem, &status) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (status.rx_frames_ok != 0U)
		return __LINE__;

	return 0;
}

static int
test_embedded_modem_rx_output_small(void)
{
	struct audio_stub audio;
	struct embedded_modem modem;
	int16_t pcm[TEST_MODEM_PCM_MAX];
	size_t frame_count;
	size_t offset;
	size_t pcm_len;
	size_t take;
	int got_small;
	int line;

	line = build_pcm_frame(pcm, sizeof(pcm) / sizeof(pcm[0]), &pcm_len);
	if (line != 0)
		return line;
	audio_stub_init(&audio);
	if (embedded_modem_init(&modem) != EMBEDDED_MODEM_OK)
		return __LINE__;
	got_small = 0;
	for (offset = 0U; offset < pcm_len; offset += take) {
		take = pcm_len - offset;
		if (take > 64U)
			take = 64U;
		if (audio_stub_inject_rx(&audio, pcm + offset, take) !=
		    KILOTNC_AUDIO_OK)
			return __LINE__;
		if (embedded_modem_process_rx(&modem,
		    audio_stub_audio(&audio), NULL, 0U, &frame_count) ==
		    EMBEDDED_MODEM_ERR_SMALL)
			got_small = 1;
	}
	if (got_small == 0)
		return __LINE__;

	return 0;
}

static int
test_embedded_modem_rx_reset(void)
{
	struct audio_stub audio;
	struct embedded_modem modem;
	struct embedded_modem_rx_frame frame;
	struct embedded_modem_status status;
	size_t frame_count;

	audio_stub_init(&audio);
	if (embedded_modem_init(&modem) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (embedded_modem_process_rx(&modem, audio_stub_audio(&audio),
	    &frame, 1U, &frame_count) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (embedded_modem_rx_reset(&modem) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (embedded_modem_status(&modem, &status) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (status.rx_active != 1U || status.rx_audio_underflows != 0U ||
	    status.rx_frames_ok != 0U)
		return __LINE__;

	return 0;
}

static int
test_embedded_modem_rx_small_chunks(void)
{
	struct audio_stub audio;
	struct embedded_modem modem;
	struct embedded_modem_rx_frame frames[2];
	int16_t pcm[TEST_MODEM_PCM_MAX];
	size_t frame_count;
	size_t pcm_len;
	int line;

	line = build_pcm_frame(pcm, sizeof(pcm) / sizeof(pcm[0]), &pcm_len);
	if (line != 0)
		return line;
	audio_stub_init(&audio);
	if (embedded_modem_init(&modem) != EMBEDDED_MODEM_OK)
		return __LINE__;
	line = feed_rx_pcm(&modem, &audio, pcm, pcm_len, 7U, frames,
	    sizeof(frames) / sizeof(frames[0]), &frame_count);
	if (line != 0)
		return line;
	if (frame_count != 1U)
		return __LINE__;

	return check_decoded_frame(frames[0].data, frames[0].len);
}

static int
test_embedded_modem_shutdown_aborts(void)
{
	struct platform_stub platform;
	struct audio_stub audio;
	struct embedded_modem modem;
	struct embedded_app app;
	struct embedded_modem_status status;
	enum kilotnc_gpio_state ptt;
	uint8_t frame[KILOTNC_AX25_MAX_FRAME];
	size_t frame_len;
	int line;

	line = build_ax25_frame(frame, sizeof(frame), &frame_len);
	if (line != 0)
		return line;
	platform_stub_init(&platform);
	audio_stub_init(&audio);
	if (embedded_modem_init(&modem) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (embedded_modem_start_ax25(&modem, frame, frame_len,
	    TNC_MODE_1200_AFSK_AX25) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (embedded_app_init(&app, platform_stub_platform(&platform)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_app_modem(&app, &modem, audio_stub_audio(&audio)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_app_shutdown(&app) != EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_modem_status(&modem, &status) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (status.tx_active != 0U || status.aborts != 1U)
		return __LINE__;
	if (platform_stub_ptt_state(&platform, &ptt) != KILOTNC_PLATFORM_OK)
		return __LINE__;
	if (ptt != KILOTNC_GPIO_LOW)
		return __LINE__;

	return 0;
}

static int
test_embedded_modem_tnc_disabled(void)
{
	struct usb_cdc_stub usb;
	struct embedded_modem modem;
	struct embedded_tnc tnc;
	struct embedded_tnc_status status;
	uint8_t kiss[128];
	size_t kiss_len;
	int line;

	line = build_kiss_frame(kiss, sizeof(kiss), &kiss_len);
	if (line != 0)
		return line;
	usb_cdc_stub_init(&usb);
	if (usb_cdc_stub_set_connected(&usb, 1) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_modem_init(&modem) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (embedded_tnc_init(&tnc) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (embedded_tnc_modem(&tnc, &modem) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (usb_cdc_stub_inject_rx(&usb, kiss, kiss_len) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_tnc_process_usb(&tnc, usb_cdc_stub_usb(&usb)) !=
	    EMBEDDED_TNC_OK)
		return __LINE__;
	if (embedded_tnc_status(&tnc, &status) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (status.modem_tx_requests != 0U)
		return __LINE__;

	return 0;
}

static int
test_embedded_modem_tnc_malformed(void)
{
	struct usb_cdc_stub usb;
	struct embedded_modem modem;
	struct embedded_tnc tnc;
	struct embedded_tnc_status status;
	uint8_t bad[6];

	bad[0] = KISS_FEND;
	bad[1] = KISS_CMD_DATA;
	bad[2] = KISS_FESC;
	bad[3] = 0U;
	bad[4] = 1U;
	bad[5] = KISS_FEND;
	usb_cdc_stub_init(&usb);
	if (usb_cdc_stub_set_connected(&usb, 1) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_modem_init(&modem) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (embedded_tnc_init(&tnc) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (embedded_tnc_modem(&tnc, &modem) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (embedded_tnc_set_modem_tx(&tnc, 1) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (usb_cdc_stub_inject_rx(&usb, bad, sizeof(bad)) !=
	    KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_tnc_process_usb(&tnc, usb_cdc_stub_usb(&usb)) !=
	    EMBEDDED_TNC_OK)
		return __LINE__;
	if (embedded_tnc_status(&tnc, &status) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (status.modem_tx_requests != 1U ||
	    status.modem_tx_accepted != 0U)
		return __LINE__;

	return 0;
}

static int
test_embedded_modem_tnc_rx_disabled(void)
{
	struct platform_stub platform;
	struct audio_stub audio;
	struct usb_cdc_stub usb;
	struct embedded_modem modem;
	struct embedded_tnc tnc;
	struct embedded_app app;
	uint8_t out[USB_CDC_STUB_BUFFER_MAX];
	int16_t pcm[TEST_MODEM_PCM_MAX];
	size_t out_len;
	size_t pcm_len;
	size_t take;
	int line;

	line = build_pcm_frame(pcm, sizeof(pcm) / sizeof(pcm[0]), &pcm_len);
	if (line != 0)
		return line;
	platform_stub_init(&platform);
	audio_stub_init(&audio);
	usb_cdc_stub_init(&usb);
	if (usb_cdc_stub_set_connected(&usb, 1) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_modem_init(&modem) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (embedded_tnc_init(&tnc) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (embedded_app_init(&app, platform_stub_platform(&platform)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_app_tnc(&app, &tnc, usb_cdc_stub_usb(&usb)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_app_modem(&app, &modem, audio_stub_audio(&audio)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	take = pcm_len;
	if (take > 64U)
		take = 64U;
	if (audio_stub_inject_rx(&audio, pcm, take) != KILOTNC_AUDIO_OK)
		return __LINE__;
	if (embedded_app_step(&app) != EMBEDDED_APP_OK)
		return __LINE__;
	if (usb_cdc_stub_take_tx(&usb, out, sizeof(out), &out_len) !=
	    KILOTNC_USB_OK)
		return __LINE__;
	if (out_len != 0U)
		return __LINE__;

	return 0;
}

static int
test_embedded_modem_tnc_rx_emits_kiss(void)
{
	struct platform_stub platform;
	struct audio_stub audio;
	struct usb_cdc_stub usb;
	struct embedded_modem modem;
	struct embedded_tnc tnc;
	struct embedded_app app;
	struct embedded_app_status app_status;
	enum kilotnc_gpio_state ptt;
	uint8_t out[USB_CDC_STUB_BUFFER_MAX];
	int16_t pcm[TEST_MODEM_PCM_MAX];
	size_t offset;
	size_t out_len;
	size_t pcm_len;
	size_t take;
	int line;

	line = build_pcm_frame(pcm, sizeof(pcm) / sizeof(pcm[0]), &pcm_len);
	if (line != 0)
		return line;
	platform_stub_init(&platform);
	audio_stub_init(&audio);
	usb_cdc_stub_init(&usb);
	if (usb_cdc_stub_set_connected(&usb, 1) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_modem_init(&modem) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (embedded_tnc_init(&tnc) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (embedded_tnc_modem(&tnc, &modem) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (embedded_tnc_set_modem_rx(&tnc, 1) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (embedded_app_init(&app, platform_stub_platform(&platform)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_app_tnc(&app, &tnc, usb_cdc_stub_usb(&usb)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_app_modem(&app, &modem, audio_stub_audio(&audio)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	for (offset = 0U; offset < pcm_len; offset += take) {
		take = pcm_len - offset;
		if (take > 64U)
			take = 64U;
		if (audio_stub_inject_rx(&audio, pcm + offset, take) !=
		    KILOTNC_AUDIO_OK)
			return __LINE__;
		if (embedded_app_step(&app) != EMBEDDED_APP_OK)
			return __LINE__;
	}
	if (usb_cdc_stub_take_tx(&usb, out, sizeof(out), &out_len) !=
	    KILOTNC_USB_OK)
		return __LINE__;
	if (out_len == 0U)
		return __LINE__;
	line = check_kiss_output(out, out_len);
	if (line != 0)
		return line;
	if (embedded_app_status(&app, &app_status) != EMBEDDED_APP_OK)
		return __LINE__;
	if (app_status.watchdog_kicks == 0U)
		return __LINE__;
	if (platform_stub_ptt_state(&platform, &ptt) != KILOTNC_PLATFORM_OK)
		return __LINE__;
	if (ptt != KILOTNC_GPIO_LOW)
		return __LINE__;

	return 0;
}

static int
test_embedded_modem_tnc_rx_overflow_drop(void)
{
	struct platform_stub platform;
	struct audio_stub audio;
	struct usb_cdc_stub usb;
	struct embedded_modem modem;
	struct embedded_tnc tnc;
	struct embedded_tnc_status status;
	struct embedded_app app;
	int16_t pcm[TEST_MODEM_PCM_MAX];
	size_t offset;
	size_t pcm_len;
	size_t take;
	int line;

	line = build_pcm_frame(pcm, sizeof(pcm) / sizeof(pcm[0]), &pcm_len);
	if (line != 0)
		return line;
	platform_stub_init(&platform);
	audio_stub_init(&audio);
	usb_cdc_stub_init(&usb);
	if (usb_cdc_stub_set_connected(&usb, 1) != KILOTNC_USB_OK)
		return __LINE__;
	usb.tx_len = USB_CDC_STUB_BUFFER_MAX - 1U;
	if (embedded_modem_init(&modem) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (embedded_tnc_init(&tnc) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (embedded_tnc_set_modem_rx(&tnc, 1) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (embedded_app_init(&app, platform_stub_platform(&platform)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_app_tnc(&app, &tnc, usb_cdc_stub_usb(&usb)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_app_modem(&app, &modem, audio_stub_audio(&audio)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	for (offset = 0U; offset < pcm_len; offset += take) {
		take = pcm_len - offset;
		if (take > 64U)
			take = 64U;
		if (audio_stub_inject_rx(&audio, pcm + offset, take) !=
		    KILOTNC_AUDIO_OK)
			return __LINE__;
		if (embedded_app_step(&app) != EMBEDDED_APP_OK)
			return __LINE__;
	}
	if (embedded_tnc_status(&tnc, &status) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (status.modem_rx_output_drops == 0U)
		return __LINE__;

	return 0;
}

static int
test_embedded_modem_tnc_starts_tx(void)
{
	struct usb_cdc_stub usb;
	struct embedded_modem modem;
	struct embedded_modem_status modem_status;
	struct embedded_tnc tnc;
	struct embedded_tnc_status tnc_status;
	uint8_t kiss[128];
	size_t kiss_len;
	int line;

	line = build_kiss_frame(kiss, sizeof(kiss), &kiss_len);
	if (line != 0)
		return line;
	usb_cdc_stub_init(&usb);
	if (usb_cdc_stub_set_connected(&usb, 1) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_modem_init(&modem) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (embedded_tnc_init(&tnc) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (embedded_tnc_modem(&tnc, &modem) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (embedded_tnc_set_modem_tx(&tnc, 1) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (usb_cdc_stub_inject_rx(&usb, kiss, kiss_len) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_tnc_process_usb(&tnc, usb_cdc_stub_usb(&usb)) !=
	    EMBEDDED_TNC_OK)
		return __LINE__;
	if (embedded_tnc_status(&tnc, &tnc_status) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (embedded_modem_status(&modem, &modem_status) !=
	    EMBEDDED_MODEM_OK)
		return __LINE__;
	if (tnc_status.modem_tx_accepted != 1U ||
	    modem_status.tx_active != 1U)
		return __LINE__;

	return 0;
}

static int
test_embedded_modem_tnc_unsupported_mode_blocks_tx(void)
{
	struct usb_cdc_stub usb;
	struct embedded_modem modem;
	struct embedded_modem_status modem_status;
	struct embedded_tnc tnc;
	struct embedded_tnc_status tnc_status;
	uint8_t kiss[128];
	uint8_t sethw[16];
	size_t kiss_len;
	size_t sethw_len;
	int line;

	line = build_sethardware_frame(0U, sethw, sizeof(sethw), &sethw_len);
	if (line != 0)
		return line;
	line = build_kiss_frame(kiss, sizeof(kiss), &kiss_len);
	if (line != 0)
		return line;
	usb_cdc_stub_init(&usb);
	if (usb_cdc_stub_set_connected(&usb, 1) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_modem_init(&modem) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (embedded_tnc_init(&tnc) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (embedded_tnc_modem(&tnc, &modem) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (embedded_tnc_set_modem_tx(&tnc, 1) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (usb_cdc_stub_inject_rx(&usb, sethw, sethw_len) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_tnc_process_usb(&tnc, usb_cdc_stub_usb(&usb)) !=
	    EMBEDDED_TNC_OK)
		return __LINE__;
	if (usb_cdc_stub_inject_rx(&usb, kiss, kiss_len) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_tnc_process_usb(&tnc, usb_cdc_stub_usb(&usb)) !=
	    EMBEDDED_TNC_OK)
		return __LINE__;
	if (embedded_tnc_status(&tnc, &tnc_status) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (embedded_modem_status(&modem, &modem_status) !=
	    EMBEDDED_MODEM_OK)
		return __LINE__;
	if (tnc_status.unsupported_mode_requests != 1U ||
	    tnc_status.modem_tx_requests != 1U ||
	    tnc_status.modem_tx_rejected != 1U ||
	    modem_status.tx_active != 0U)
		return __LINE__;

	return 0;
}

static int
test_embedded_modem_tnc_watchdog_blocks_rx(void)
{
	struct platform_stub platform;
	struct audio_stub audio;
	struct usb_cdc_stub usb;
	struct embedded_modem modem;
	struct embedded_tnc tnc;
	struct embedded_app app;
	enum kilotnc_gpio_state ptt;
	uint8_t out[USB_CDC_STUB_BUFFER_MAX];
	int16_t pcm[TEST_MODEM_PCM_MAX];
	size_t out_len;
	size_t pcm_len;
	size_t take;
	int line;

	line = build_pcm_frame(pcm, sizeof(pcm) / sizeof(pcm[0]), &pcm_len);
	if (line != 0)
		return line;
	platform_stub_init(&platform);
	audio_stub_init(&audio);
	usb_cdc_stub_init(&usb);
	if (usb_cdc_stub_set_connected(&usb, 1) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_modem_init(&modem) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (embedded_tnc_init(&tnc) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (embedded_tnc_set_modem_rx(&tnc, 1) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (embedded_app_init(&app, platform_stub_platform(&platform)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_app_tnc(&app, &tnc, usb_cdc_stub_usb(&usb)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_app_modem(&app, &modem, audio_stub_audio(&audio)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	take = pcm_len;
	if (take > 64U)
		take = 64U;
	if (audio_stub_inject_rx(&audio, pcm, take) != KILOTNC_AUDIO_OK)
		return __LINE__;
	if (platform_stub_simulate_watchdog_fault(&platform) !=
	    KILOTNC_PLATFORM_OK)
		return __LINE__;
	if (embedded_app_step(&app) != EMBEDDED_APP_ERR_FAULT)
		return __LINE__;
	if (usb_cdc_stub_take_tx(&usb, out, sizeof(out), &out_len) !=
	    KILOTNC_USB_OK)
		return __LINE__;
	if (out_len != 0U)
		return __LINE__;
	if (platform_stub_ptt_state(&platform, &ptt) != KILOTNC_PLATFORM_OK)
		return __LINE__;
	if (ptt != KILOTNC_GPIO_LOW)
		return __LINE__;

	return 0;
}

static int
test_embedded_modem_watchdog_aborts(void)
{
	struct platform_stub platform;
	struct audio_stub audio;
	struct embedded_modem modem;
	struct embedded_app app;
	struct embedded_modem_status status;
	enum kilotnc_gpio_state ptt;
	uint8_t frame[KILOTNC_AX25_MAX_FRAME];
	size_t frame_len;
	int line;

	line = build_ax25_frame(frame, sizeof(frame), &frame_len);
	if (line != 0)
		return line;
	platform_stub_init(&platform);
	audio_stub_init(&audio);
	if (embedded_modem_init(&modem) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (embedded_modem_start_ax25(&modem, frame, frame_len,
	    TNC_MODE_1200_AFSK_AX25) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (embedded_app_init(&app, platform_stub_platform(&platform)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_app_modem(&app, &modem, audio_stub_audio(&audio)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	if (platform_stub_simulate_watchdog_fault(&platform) !=
	    KILOTNC_PLATFORM_OK)
		return __LINE__;
	if (embedded_app_step(&app) != EMBEDDED_APP_ERR_FAULT)
		return __LINE__;
	if (embedded_modem_status(&modem, &status) != EMBEDDED_MODEM_OK)
		return __LINE__;
	if (status.tx_active != 0U || status.aborts != 1U)
		return __LINE__;
	if (platform_stub_ptt_state(&platform, &ptt) != KILOTNC_PLATFORM_OK)
		return __LINE__;
	if (ptt != KILOTNC_GPIO_LOW)
		return __LINE__;

	return 0;
}

int
test_embedded_modem(void)
{
	int line;

	line = test_embedded_modem_init();
	if (line != 0)
		goto fail;
	line = test_embedded_modem_rejects_unsupported_mode();
	if (line != 0)
		goto fail;
	line = test_embedded_modem_rejects_bad_frame();
	if (line != 0)
		goto fail;
	line = test_embedded_modem_busy();
	if (line != 0)
		goto fail;
	line = test_embedded_modem_process_tx();
	if (line != 0)
		goto fail;
	line = test_embedded_modem_abort();
	if (line != 0)
		goto fail;
	line = test_embedded_modem_rx_empty_underflows();
	if (line != 0)
		goto fail;
	line = test_embedded_modem_rx_reset();
	if (line != 0)
		goto fail;
	line = test_embedded_modem_rx_decodes_frame();
	if (line != 0)
		goto fail;
	line = test_embedded_modem_rx_small_chunks();
	if (line != 0)
		goto fail;
	line = test_embedded_modem_rx_noise_safe();
	if (line != 0)
		goto fail;
	line = test_embedded_modem_rx_output_small();
	if (line != 0)
		goto fail;
	line = test_embedded_modem_tnc_disabled();
	if (line != 0)
		goto fail;
	line = test_embedded_modem_tnc_starts_tx();
	if (line != 0)
		goto fail;
	line = test_embedded_modem_tnc_malformed();
	if (line != 0)
		goto fail;
	line = test_embedded_modem_tnc_unsupported_mode_blocks_tx();
	if (line != 0)
		goto fail;
	line = test_embedded_modem_tnc_rx_disabled();
	if (line != 0)
		goto fail;
	line = test_embedded_modem_tnc_rx_emits_kiss();
	if (line != 0)
		goto fail;
	line = test_embedded_modem_tnc_rx_overflow_drop();
	if (line != 0)
		goto fail;
	line = test_embedded_modem_app_step();
	if (line != 0)
		goto fail;
	line = test_embedded_modem_watchdog_aborts();
	if (line != 0)
		goto fail;
	line = test_embedded_modem_tnc_watchdog_blocks_rx();
	if (line != 0)
		goto fail;
	line = test_embedded_modem_shutdown_aborts();
	if (line != 0)
		goto fail;
	line = test_embedded_modem_diag();
	if (line != 0)
		goto fail;
	line = test_embedded_modem_rx_diag();
	if (line != 0)
		goto fail;
	line = test_embedded_modem_null_args();
	if (line != 0)
		goto fail;

	(void)printf("ok embedded_modem\n");
	return 0;

fail:
	(void)printf("not ok embedded_modem line %d\n", line);
	return 1;
}
