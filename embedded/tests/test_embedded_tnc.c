/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/tests/test_embedded_tnc.c */

#include <sys/types.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "embedded_app.h"
#include "embedded_diag.h"
#include "embedded_tnc.h"
#include "kiss.h"
#include "platform_stub.h"
#include "tnc_mode.h"
#include "usb_cdc_stub.h"

static int decode_single_kiss(const uint8_t *, size_t,
	struct kiss_frame *);
static int encode_command(uint8_t, const uint8_t *, size_t, uint8_t *,
	size_t, size_t *);
static int encode_data(const uint8_t *, size_t, uint8_t *, size_t,
	size_t *);
static int process_bytes(struct embedded_tnc *, struct usb_cdc_stub *,
	const uint8_t *, size_t);
static int test_embedded_tnc_app_step(void);
static int test_embedded_tnc_commands(void);
static int test_embedded_tnc_data_frame(void);
static int test_embedded_tnc_diag(void);
static int test_embedded_tnc_escaped_loopback(void);
static int test_embedded_tnc_init(void);
static int test_embedded_tnc_malformed(void);
static int test_embedded_tnc_modes(void);
static int test_embedded_tnc_null_args(void);
static int test_embedded_tnc_repeated_fend(void);
static int test_embedded_tnc_return_command(void);
static int test_embedded_tnc_unsupported_command(void);
static int test_embedded_tnc_watchdog_fault(void);

static int
decode_single_kiss(const uint8_t *buf, size_t len, struct kiss_frame *frame)
{
	struct kiss_parser parser;
	struct kiss_frame frames[1];
	size_t count;

	kiss_parser_init(&parser);
	if (kiss_parse_bytes(&parser, buf, len, frames, 1, &count) !=
	    KISS_OK)
		return __LINE__;
	if (count != 1U)
		return __LINE__;

	*frame = frames[0];
	return 0;
}

static int
encode_command(uint8_t command, const uint8_t *payload, size_t payload_len,
	uint8_t *out, size_t out_cap, size_t *out_len)
{
	if (kiss_encode_frame(0, command, payload, payload_len, out, out_cap,
	    out_len) != KISS_OK)
		return __LINE__;

	return 0;
}

static int
encode_data(const uint8_t *payload, size_t payload_len, uint8_t *out,
	size_t out_cap, size_t *out_len)
{
	return encode_command(KISS_CMD_DATA, payload, payload_len, out,
	    out_cap, out_len);
}

static int
process_bytes(struct embedded_tnc *tnc, struct usb_cdc_stub *usb_stub,
	const uint8_t *buf, size_t len)
{
	if (usb_cdc_stub_inject_rx(usb_stub, buf, len) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_tnc_process_usb(tnc, usb_cdc_stub_usb(usb_stub)) !=
	    EMBEDDED_TNC_OK)
		return __LINE__;

	return 0;
}

static int
test_embedded_tnc_app_step(void)
{
	struct platform_stub platform;
	struct usb_cdc_stub usb_stub;
	struct embedded_tnc tnc;
	struct embedded_app app;
	struct embedded_app_status app_status;
	struct embedded_tnc_status tnc_status;
	enum kilotnc_gpio_state ptt;
	uint8_t kiss[32];
	size_t kiss_len;
	int line;

	line = encode_data((const uint8_t *)"app", 3, kiss, sizeof(kiss),
	    &kiss_len);
	if (line != 0)
		return line;

	platform_stub_init(&platform);
	usb_cdc_stub_init(&usb_stub);
	if (usb_cdc_stub_set_connected(&usb_stub, 1) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_tnc_init(&tnc) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (embedded_app_init(&app, platform_stub_platform(&platform)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_app_tnc(&app, &tnc, usb_cdc_stub_usb(&usb_stub)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	if (usb_cdc_stub_inject_rx(&usb_stub, kiss, kiss_len) !=
	    KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_app_step(&app) != EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_app_status(&app, &app_status) != EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_tnc_status(&tnc, &tnc_status) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (app_status.watchdog_kicks != 1U || tnc_status.kiss_frames_in != 1U)
		return __LINE__;
	if (platform_stub_ptt_state(&platform, &ptt) != KILOTNC_PLATFORM_OK)
		return __LINE__;
	if (ptt != KILOTNC_GPIO_LOW)
		return __LINE__;

	return 0;
}

static int
test_embedded_tnc_commands(void)
{
	struct usb_cdc_stub usb_stub;
	struct embedded_tnc tnc;
	struct embedded_tnc_status status;
	uint8_t kiss[32];
	uint8_t value;
	size_t kiss_len;
	int line;

	usb_cdc_stub_init(&usb_stub);
	if (usb_cdc_stub_set_connected(&usb_stub, 1) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_tnc_init(&tnc) != EMBEDDED_TNC_OK)
		return __LINE__;

	value = 11U;
	line = encode_command(KISS_CMD_TXDELAY, &value, 1, kiss,
	    sizeof(kiss), &kiss_len);
	if (line != 0)
		return line;
	line = process_bytes(&tnc, &usb_stub, kiss, kiss_len);
	if (line != 0)
		return line;

	value = 255U;
	line = encode_command(KISS_CMD_P, &value, 1, kiss, sizeof(kiss),
	    &kiss_len);
	if (line != 0)
		return line;
	line = process_bytes(&tnc, &usb_stub, kiss, kiss_len);
	if (line != 0)
		return line;

	value = 7U;
	line = encode_command(KISS_CMD_SLOTTIME, &value, 1, kiss,
	    sizeof(kiss), &kiss_len);
	if (line != 0)
		return line;
	line = process_bytes(&tnc, &usb_stub, kiss, kiss_len);
	if (line != 0)
		return line;

	value = 3U;
	line = encode_command(KISS_CMD_TXTAIL, &value, 1, kiss, sizeof(kiss),
	    &kiss_len);
	if (line != 0)
		return line;
	line = process_bytes(&tnc, &usb_stub, kiss, kiss_len);
	if (line != 0)
		return line;

	value = 1U;
	line = encode_command(KISS_CMD_FULLDUPLEX, &value, 1, kiss,
	    sizeof(kiss), &kiss_len);
	if (line != 0)
		return line;
	line = process_bytes(&tnc, &usb_stub, kiss, kiss_len);
	if (line != 0)
		return line;

	if (embedded_tnc_status(&tnc, &status) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (status.txdelay != 11U || status.p != 255U ||
	    status.slottime != 7U || status.txtail != 3U ||
	    status.fullduplex != 1U)
		return __LINE__;

	return 0;
}

static int
test_embedded_tnc_data_frame(void)
{
	struct usb_cdc_stub usb_stub;
	struct embedded_tnc tnc;
	struct embedded_tnc_status status;
	uint8_t kiss[32];
	size_t kiss_len;
	int line;

	line = encode_data((const uint8_t *)"tnc", 3, kiss, sizeof(kiss),
	    &kiss_len);
	if (line != 0)
		return line;

	usb_cdc_stub_init(&usb_stub);
	if (usb_cdc_stub_set_connected(&usb_stub, 1) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_tnc_init(&tnc) != EMBEDDED_TNC_OK)
		return __LINE__;
	line = process_bytes(&tnc, &usb_stub, kiss, kiss_len);
	if (line != 0)
		return line;
	if (embedded_tnc_status(&tnc, &status) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (status.kiss_frames_in != 1U || status.kiss_frames_out != 0U)
		return __LINE__;

	return 0;
}

static int
test_embedded_tnc_diag(void)
{
	struct platform_stub platform;
	struct usb_cdc_stub usb_stub;
	struct embedded_tnc tnc;
	struct embedded_app app;
	struct embedded_diag_snapshot snapshot;
	char formatted[4096];
	uint8_t kiss[32];
	size_t formatted_len;
	size_t kiss_len;
	int line;

	line = encode_data((const uint8_t *)"diag", 4, kiss, sizeof(kiss),
	    &kiss_len);
	if (line != 0)
		return line;

	platform_stub_init(&platform);
	usb_cdc_stub_init(&usb_stub);
	if (usb_cdc_stub_set_connected(&usb_stub, 1) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_tnc_init(&tnc) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (embedded_app_init(&app, platform_stub_platform(&platform)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_app_tnc(&app, &tnc, usb_cdc_stub_usb(&usb_stub)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	if (usb_cdc_stub_inject_rx(&usb_stub, kiss, kiss_len) !=
	    KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_app_step(&app) != EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_diag_capture(&app, &snapshot) != EMBEDDED_DIAG_OK)
		return __LINE__;
	if (snapshot.tnc_current_mode != TNC_MODE_1200_AFSK_AX25 ||
	    snapshot.tnc_kiss_frames_in != 1U)
		return __LINE__;
	if (embedded_diag_format(&snapshot, formatted, sizeof(formatted),
	    &formatted_len) != EMBEDDED_DIAG_OK)
		return __LINE__;
	if (strstr(formatted, "tnc_mode=") == NULL ||
	    strstr(formatted, "tnc_kiss_frames_in=1") == NULL)
		return __LINE__;

	return 0;
}

static int
test_embedded_tnc_escaped_loopback(void)
{
	struct usb_cdc_stub usb_stub;
	struct embedded_tnc tnc;
	struct embedded_tnc_status status;
	struct kiss_frame frame;
	uint8_t kiss[32];
	uint8_t out[32];
	uint8_t payload[4];
	size_t kiss_len;
	size_t out_len;
	int line;

	payload[0] = 0x44U;
	payload[1] = KISS_FEND;
	payload[2] = KISS_FESC;
	payload[3] = 0x55U;
	line = encode_data(payload, sizeof(payload), kiss, sizeof(kiss),
	    &kiss_len);
	if (line != 0)
		return line;

	usb_cdc_stub_init(&usb_stub);
	if (usb_cdc_stub_set_connected(&usb_stub, 1) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_tnc_init(&tnc) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (embedded_tnc_set_loopback(&tnc, 1) != EMBEDDED_TNC_OK)
		return __LINE__;
	line = process_bytes(&tnc, &usb_stub, kiss, kiss_len);
	if (line != 0)
		return line;
	if (usb_cdc_stub_take_tx(&usb_stub, out, sizeof(out), &out_len) !=
	    KILOTNC_USB_OK)
		return __LINE__;
	line = decode_single_kiss(out, out_len, &frame);
	if (line != 0)
		return line;
	if (frame.len != sizeof(payload) ||
	    memcmp(frame.data, payload, sizeof(payload)) != 0)
		return __LINE__;
	if (embedded_tnc_status(&tnc, &status) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (status.kiss_frames_in != 1U || status.kiss_frames_out != 1U)
		return __LINE__;

	return 0;
}

static int
test_embedded_tnc_init(void)
{
	struct embedded_tnc tnc;
	struct embedded_tnc_status status;

	if (embedded_tnc_init(&tnc) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (embedded_tnc_status(&tnc, &status) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (status.current_mode != TNC_MODE_1200_AFSK_AX25)
		return __LINE__;
	if (status.ptt_state != TNC_CONTROL_PTT_OFF)
		return __LINE__;

	return 0;
}

static int
test_embedded_tnc_malformed(void)
{
	struct usb_cdc_stub usb_stub;
	struct embedded_tnc tnc;
	struct embedded_tnc_status status;
	uint8_t bad[6];
	uint8_t kiss[32];
	size_t kiss_len;
	int line;

	bad[0] = KISS_FEND;
	bad[1] = KISS_CMD_DATA;
	bad[2] = KISS_FESC;
	bad[3] = 0x00U;
	bad[4] = 'x';
	bad[5] = KISS_FEND;
	line = encode_data((const uint8_t *)"ok", 2, kiss, sizeof(kiss),
	    &kiss_len);
	if (line != 0)
		return line;

	usb_cdc_stub_init(&usb_stub);
	if (usb_cdc_stub_set_connected(&usb_stub, 1) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_tnc_init(&tnc) != EMBEDDED_TNC_OK)
		return __LINE__;
	line = process_bytes(&tnc, &usb_stub, bad, sizeof(bad));
	if (line != 0)
		return line;
	line = process_bytes(&tnc, &usb_stub, kiss, kiss_len);
	if (line != 0)
		return line;
	if (embedded_tnc_status(&tnc, &status) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (status.kiss_parse_errors != 1U || status.kiss_frames_in != 2U)
		return __LINE__;

	return 0;
}

static int
test_embedded_tnc_modes(void)
{
	struct usb_cdc_stub usb_stub;
	struct embedded_tnc tnc;
	struct embedded_tnc_status status;
	uint8_t kiss[32];
	uint8_t mode;
	size_t kiss_len;
	int line;

	usb_cdc_stub_init(&usb_stub);
	if (usb_cdc_stub_set_connected(&usb_stub, 1) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_tnc_init(&tnc) != EMBEDDED_TNC_OK)
		return __LINE__;

	mode = 6U;
	line = encode_command(KISS_CMD_SETHARDWARE, &mode, 1, kiss,
	    sizeof(kiss), &kiss_len);
	if (line != 0)
		return line;
	line = process_bytes(&tnc, &usb_stub, kiss, kiss_len);
	if (line != 0)
		return line;

	mode = 22U;
	line = encode_command(KISS_CMD_SETHARDWARE, &mode, 1, kiss,
	    sizeof(kiss), &kiss_len);
	if (line != 0)
		return line;
	line = process_bytes(&tnc, &usb_stub, kiss, kiss_len);
	if (line != 0)
		return line;

	mode = 0U;
	line = encode_command(KISS_CMD_SETHARDWARE, &mode, 1, kiss,
	    sizeof(kiss), &kiss_len);
	if (line != 0)
		return line;
	line = process_bytes(&tnc, &usb_stub, kiss, kiss_len);
	if (line != 0)
		return line;

	mode = 255U;
	line = encode_command(KISS_CMD_SETHARDWARE, &mode, 1, kiss,
	    sizeof(kiss), &kiss_len);
	if (line != 0)
		return line;
	line = process_bytes(&tnc, &usb_stub, kiss, kiss_len);
	if (line != 0)
		return line;

	if (embedded_tnc_status(&tnc, &status) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (status.current_mode != TNC_MODE_1200_AFSK_AX25)
		return __LINE__;
	if (status.mode_set_requests != 4U ||
	    status.unsupported_mode_requests != 1U ||
	    status.invalid_mode_requests != 1U)
		return __LINE__;
	if (status.last_nino_sethw != 255U)
		return __LINE__;

	return 0;
}

static int
test_embedded_tnc_null_args(void)
{
	struct embedded_tnc tnc;
	struct embedded_tnc_status status;
	struct usb_cdc_stub usb_stub;

	usb_cdc_stub_init(&usb_stub);
	if (embedded_tnc_init(NULL) != EMBEDDED_TNC_ERR_ARG)
		return __LINE__;
	if (embedded_tnc_init(&tnc) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (embedded_tnc_process_usb(NULL, usb_cdc_stub_usb(&usb_stub)) !=
	    EMBEDDED_TNC_ERR_ARG)
		return __LINE__;
	if (embedded_tnc_process_usb(&tnc, NULL) != EMBEDDED_TNC_ERR_ARG)
		return __LINE__;
	if (embedded_tnc_set_loopback(NULL, 1) != EMBEDDED_TNC_ERR_ARG)
		return __LINE__;
	if (embedded_tnc_set_modem_rx(NULL, 1) != EMBEDDED_TNC_ERR_ARG)
		return __LINE__;
	if (embedded_tnc_set_modem_tx(NULL, 1) != EMBEDDED_TNC_ERR_ARG)
		return __LINE__;
	if (embedded_tnc_emit_modem_rx(NULL, usb_cdc_stub_usb(&usb_stub),
	    NULL, 0U) != EMBEDDED_TNC_ERR_ARG)
		return __LINE__;
	if (embedded_tnc_status(NULL, &status) != EMBEDDED_TNC_ERR_ARG)
		return __LINE__;
	if (embedded_tnc_status(&tnc, NULL) != EMBEDDED_TNC_ERR_ARG)
		return __LINE__;

	return 0;
}

static int
test_embedded_tnc_repeated_fend(void)
{
	struct usb_cdc_stub usb_stub;
	struct embedded_tnc tnc;
	struct embedded_tnc_status status;
	uint8_t data[4];
	int line;

	data[0] = KISS_FEND;
	data[1] = KISS_FEND;
	data[2] = KISS_FEND;
	data[3] = KISS_FEND;

	usb_cdc_stub_init(&usb_stub);
	if (usb_cdc_stub_set_connected(&usb_stub, 1) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_tnc_init(&tnc) != EMBEDDED_TNC_OK)
		return __LINE__;
	line = process_bytes(&tnc, &usb_stub, data, sizeof(data));
	if (line != 0)
		return line;
	if (embedded_tnc_status(&tnc, &status) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (status.kiss_frames_in != 0U || status.kiss_frames_out != 0U)
		return __LINE__;

	return 0;
}

static int
test_embedded_tnc_return_command(void)
{
	struct usb_cdc_stub usb_stub;
	struct embedded_tnc tnc;
	struct embedded_tnc_status status;
	uint8_t kiss[32];
	size_t kiss_len;
	int line;

	line = encode_command(KISS_CMD_RETURN, NULL, 0, kiss, sizeof(kiss),
	    &kiss_len);
	if (line != 0)
		return line;

	usb_cdc_stub_init(&usb_stub);
	if (usb_cdc_stub_set_connected(&usb_stub, 1) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_tnc_init(&tnc) != EMBEDDED_TNC_OK)
		return __LINE__;
	line = process_bytes(&tnc, &usb_stub, kiss, kiss_len);
	if (line != 0)
		return line;
	if (embedded_tnc_status(&tnc, &status) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (status.kiss_ignored_commands != 0U ||
	    status.kiss_frames_in != 0U)
		return __LINE__;

	return 0;
}

static int
test_embedded_tnc_unsupported_command(void)
{
	struct usb_cdc_stub usb_stub;
	struct embedded_tnc tnc;
	struct embedded_tnc_status status;
	uint8_t kiss[32];
	size_t kiss_len;
	int line;

	line = encode_command(14U, (const uint8_t *)"x", 1, kiss,
	    sizeof(kiss), &kiss_len);
	if (line != 0)
		return line;

	usb_cdc_stub_init(&usb_stub);
	if (usb_cdc_stub_set_connected(&usb_stub, 1) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_tnc_init(&tnc) != EMBEDDED_TNC_OK)
		return __LINE__;
	line = process_bytes(&tnc, &usb_stub, kiss, kiss_len);
	if (line != 0)
		return line;
	if (embedded_tnc_status(&tnc, &status) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (status.kiss_ignored_commands != 1U)
		return __LINE__;

	return 0;
}

static int
test_embedded_tnc_watchdog_fault(void)
{
	struct platform_stub platform;
	struct usb_cdc_stub usb_stub;
	struct embedded_tnc tnc;
	struct embedded_app app;
	enum kilotnc_gpio_state ptt;

	platform_stub_init(&platform);
	usb_cdc_stub_init(&usb_stub);
	if (usb_cdc_stub_set_connected(&usb_stub, 1) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_tnc_init(&tnc) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (embedded_app_init(&app, platform_stub_platform(&platform)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_app_tnc(&app, &tnc, usb_cdc_stub_usb(&usb_stub)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	if (platform_stub_simulate_watchdog_fault(&platform) !=
	    KILOTNC_PLATFORM_OK)
		return __LINE__;
	if (embedded_app_step(&app) != EMBEDDED_APP_ERR_FAULT)
		return __LINE__;
	if (platform_stub_ptt_state(&platform, &ptt) != KILOTNC_PLATFORM_OK)
		return __LINE__;
	if (ptt != KILOTNC_GPIO_LOW)
		return __LINE__;

	return 0;
}

int
test_embedded_tnc(void)
{
	int line;

	line = test_embedded_tnc_init();
	if (line != 0)
		goto fail;
	line = test_embedded_tnc_data_frame();
	if (line != 0)
		goto fail;
	line = test_embedded_tnc_escaped_loopback();
	if (line != 0)
		goto fail;
	line = test_embedded_tnc_repeated_fend();
	if (line != 0)
		goto fail;
	line = test_embedded_tnc_malformed();
	if (line != 0)
		goto fail;
	line = test_embedded_tnc_unsupported_command();
	if (line != 0)
		goto fail;
	line = test_embedded_tnc_return_command();
	if (line != 0)
		goto fail;
	line = test_embedded_tnc_commands();
	if (line != 0)
		goto fail;
	line = test_embedded_tnc_modes();
	if (line != 0)
		goto fail;
	line = test_embedded_tnc_app_step();
	if (line != 0)
		goto fail;
	line = test_embedded_tnc_watchdog_fault();
	if (line != 0)
		goto fail;
	line = test_embedded_tnc_diag();
	if (line != 0)
		goto fail;
	line = test_embedded_tnc_null_args();
	if (line != 0)
		goto fail;

	(void)printf("ok embedded_tnc\n");
	return 0;

fail:
	(void)printf("not ok embedded_tnc line %d\n", line);
	return 1;
}
