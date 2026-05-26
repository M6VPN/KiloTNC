/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/tests/test_embedded_usb_bridge.c */

#include <sys/types.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "embedded_app.h"
#include "embedded_usb_bridge.h"
#include "kiss.h"
#include "platform_stub.h"
#include "usb_cdc_stub.h"

static int decode_single_kiss(const uint8_t *, size_t,
	struct kiss_frame *);
static int encode_kiss_data(const uint8_t *, size_t, uint8_t *, size_t,
	size_t *);
static int test_usb_bridge_app_step_services_bridge(void);
static int test_usb_bridge_disconnected(void);
static int test_usb_bridge_echo(void);
static int test_usb_bridge_escaped_kiss_loopback(void);
static int test_usb_bridge_kiss_loopback(void);
static int test_usb_bridge_malformed_kiss(void);
static int test_usb_bridge_null_args(void);
static int test_usb_bridge_repeated_fend(void);
static int test_usb_bridge_unsupported_command(void);

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
	if (count != 1u)
		return __LINE__;

	*frame = frames[0];
	return 0;
}

static int
encode_kiss_data(const uint8_t *payload, size_t payload_len, uint8_t *out,
	size_t out_cap, size_t *out_len)
{
	if (kiss_encode_frame(0, KISS_CMD_DATA, payload, payload_len, out,
	    out_cap, out_len) != KISS_OK)
		return __LINE__;

	return 0;
}

static int
test_usb_bridge_app_step_services_bridge(void)
{
	struct platform_stub platform;
	struct usb_cdc_stub usb_stub;
	struct embedded_usb_bridge bridge;
	struct embedded_app app;
	struct embedded_app_status status;
	enum kilotnc_gpio_state ptt;
	uint8_t out[16];
	size_t out_len;

	platform_stub_init(&platform);
	usb_cdc_stub_init(&usb_stub);
	if (usb_cdc_stub_set_connected(&usb_stub, 1) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_usb_bridge_init(&bridge, usb_cdc_stub_usb(&usb_stub),
	    EMBEDDED_USB_BRIDGE_ECHO) != EMBEDDED_USB_BRIDGE_OK)
		return __LINE__;
	if (embedded_app_init(&app, platform_stub_platform(&platform)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_app_usb_bridge(&app, &bridge) != EMBEDDED_APP_OK)
		return __LINE__;
	if (usb_cdc_stub_inject_rx(&usb_stub, (const uint8_t *)"hi", 2) !=
	    KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_app_step(&app) != EMBEDDED_APP_OK)
		return __LINE__;
	if (usb_cdc_stub_take_tx(&usb_stub, out, sizeof(out), &out_len) !=
	    KILOTNC_USB_OK)
		return __LINE__;
	if (out_len != 2u || memcmp(out, "hi", 2) != 0)
		return __LINE__;
	if (embedded_app_status(&app, &status) != EMBEDDED_APP_OK)
		return __LINE__;
	if (status.watchdog_kicks != 1u)
		return __LINE__;
	if (platform_stub_ptt_state(&platform, &ptt) != KILOTNC_PLATFORM_OK)
		return __LINE__;
	if (ptt != KILOTNC_GPIO_LOW)
		return __LINE__;

	return 0;
}

static int
test_usb_bridge_disconnected(void)
{
	struct usb_cdc_stub usb_stub;
	struct embedded_usb_bridge bridge;
	struct embedded_usb_bridge_stats stats;

	usb_cdc_stub_init(&usb_stub);
	if (embedded_usb_bridge_init(&bridge, usb_cdc_stub_usb(&usb_stub),
	    EMBEDDED_USB_BRIDGE_ECHO) != EMBEDDED_USB_BRIDGE_OK)
		return __LINE__;
	if (embedded_usb_bridge_service(&bridge) != EMBEDDED_USB_BRIDGE_OK)
		return __LINE__;
	if (embedded_usb_bridge_stats(&bridge, &stats) !=
	    EMBEDDED_USB_BRIDGE_OK)
		return __LINE__;
	if (stats.usb_would_block != 1u)
		return __LINE__;

	return 0;
}

static int
test_usb_bridge_echo(void)
{
	struct usb_cdc_stub usb_stub;
	struct embedded_usb_bridge bridge;
	uint8_t out[16];
	size_t out_len;

	usb_cdc_stub_init(&usb_stub);
	if (usb_cdc_stub_set_connected(&usb_stub, 1) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_usb_bridge_init(&bridge, usb_cdc_stub_usb(&usb_stub),
	    EMBEDDED_USB_BRIDGE_ECHO) != EMBEDDED_USB_BRIDGE_OK)
		return __LINE__;
	if (usb_cdc_stub_inject_rx(&usb_stub, (const uint8_t *)"echo", 4) !=
	    KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_usb_bridge_service(&bridge) != EMBEDDED_USB_BRIDGE_OK)
		return __LINE__;
	if (usb_cdc_stub_take_tx(&usb_stub, out, sizeof(out), &out_len) !=
	    KILOTNC_USB_OK)
		return __LINE__;
	if (out_len != 4u || memcmp(out, "echo", 4) != 0)
		return __LINE__;

	return 0;
}

static int
test_usb_bridge_escaped_kiss_loopback(void)
{
	struct usb_cdc_stub usb_stub;
	struct embedded_usb_bridge bridge;
	struct kiss_frame frame;
	uint8_t payload[4];
	uint8_t kiss[32];
	uint8_t out[32];
	size_t kiss_len;
	size_t out_len;
	int line;

	payload[0] = 0x11u;
	payload[1] = KISS_FEND;
	payload[2] = KISS_FESC;
	payload[3] = 0x22u;
	line = encode_kiss_data(payload, sizeof(payload), kiss, sizeof(kiss),
	    &kiss_len);
	if (line != 0)
		return line;

	usb_cdc_stub_init(&usb_stub);
	if (usb_cdc_stub_set_connected(&usb_stub, 1) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_usb_bridge_init(&bridge, usb_cdc_stub_usb(&usb_stub),
	    EMBEDDED_USB_BRIDGE_KISS_LOOPBACK) != EMBEDDED_USB_BRIDGE_OK)
		return __LINE__;
	if (usb_cdc_stub_inject_rx(&usb_stub, kiss, kiss_len) !=
	    KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_usb_bridge_service(&bridge) != EMBEDDED_USB_BRIDGE_OK)
		return __LINE__;
	if (usb_cdc_stub_take_tx(&usb_stub, out, sizeof(out), &out_len) !=
	    KILOTNC_USB_OK)
		return __LINE__;
	line = decode_single_kiss(out, out_len, &frame);
	if (line != 0)
		return line;
	if (frame.len != sizeof(payload))
		return __LINE__;
	if (memcmp(frame.data, payload, sizeof(payload)) != 0)
		return __LINE__;

	return 0;
}

static int
test_usb_bridge_kiss_loopback(void)
{
	struct usb_cdc_stub usb_stub;
	struct embedded_usb_bridge bridge;
	struct embedded_usb_bridge_stats stats;
	struct kiss_frame frame;
	uint8_t kiss[32];
	uint8_t out[32];
	size_t kiss_len;
	size_t out_len;
	int line;

	line = encode_kiss_data((const uint8_t *)"abc", 3, kiss,
	    sizeof(kiss), &kiss_len);
	if (line != 0)
		return line;

	usb_cdc_stub_init(&usb_stub);
	if (usb_cdc_stub_set_connected(&usb_stub, 1) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_usb_bridge_init(&bridge, usb_cdc_stub_usb(&usb_stub),
	    EMBEDDED_USB_BRIDGE_KISS_LOOPBACK) != EMBEDDED_USB_BRIDGE_OK)
		return __LINE__;
	if (usb_cdc_stub_inject_rx(&usb_stub, kiss, kiss_len) !=
	    KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_usb_bridge_service(&bridge) != EMBEDDED_USB_BRIDGE_OK)
		return __LINE__;
	if (usb_cdc_stub_take_tx(&usb_stub, out, sizeof(out), &out_len) !=
	    KILOTNC_USB_OK)
		return __LINE__;
	line = decode_single_kiss(out, out_len, &frame);
	if (line != 0)
		return line;
	if (frame.len != 3u || memcmp(frame.data, "abc", 3) != 0)
		return __LINE__;
	if (embedded_usb_bridge_stats(&bridge, &stats) !=
	    EMBEDDED_USB_BRIDGE_OK)
		return __LINE__;
	if (stats.kiss_frames_in != 1u || stats.kiss_frames_out != 1u)
		return __LINE__;

	return 0;
}

static int
test_usb_bridge_malformed_kiss(void)
{
	struct usb_cdc_stub usb_stub;
	struct embedded_usb_bridge bridge;
	struct embedded_usb_bridge_stats stats;
	uint8_t data[6];

	data[0] = KISS_FEND;
	data[1] = KISS_CMD_DATA;
	data[2] = KISS_FESC;
	data[3] = 0x00u;
	data[4] = 'x';
	data[5] = KISS_FEND;

	usb_cdc_stub_init(&usb_stub);
	if (usb_cdc_stub_set_connected(&usb_stub, 1) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_usb_bridge_init(&bridge, usb_cdc_stub_usb(&usb_stub),
	    EMBEDDED_USB_BRIDGE_KISS_LOOPBACK) != EMBEDDED_USB_BRIDGE_OK)
		return __LINE__;
	if (usb_cdc_stub_inject_rx(&usb_stub, data, sizeof(data)) !=
	    KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_usb_bridge_service(&bridge) != EMBEDDED_USB_BRIDGE_OK)
		return __LINE__;
	if (embedded_usb_bridge_stats(&bridge, &stats) !=
	    EMBEDDED_USB_BRIDGE_OK)
		return __LINE__;
	if (stats.kiss_parse_errors != 1u)
		return __LINE__;

	return 0;
}

static int
test_usb_bridge_null_args(void)
{
	struct usb_cdc_stub usb_stub;
	struct embedded_usb_bridge bridge;
	struct embedded_usb_bridge_stats stats;

	usb_cdc_stub_init(&usb_stub);
	if (embedded_usb_bridge_init(NULL, usb_cdc_stub_usb(&usb_stub),
	    EMBEDDED_USB_BRIDGE_ECHO) != EMBEDDED_USB_BRIDGE_ERR_ARG)
		return __LINE__;
	if (embedded_usb_bridge_init(&bridge, NULL,
	    EMBEDDED_USB_BRIDGE_ECHO) != EMBEDDED_USB_BRIDGE_ERR_ARG)
		return __LINE__;
	if (embedded_usb_bridge_init(&bridge, usb_cdc_stub_usb(&usb_stub),
	    (enum embedded_usb_bridge_mode)99) !=
	    EMBEDDED_USB_BRIDGE_ERR_ARG)
		return __LINE__;
	if (embedded_usb_bridge_service(NULL) != EMBEDDED_USB_BRIDGE_ERR_ARG)
		return __LINE__;
	if (embedded_usb_bridge_stats(NULL, &stats) !=
	    EMBEDDED_USB_BRIDGE_ERR_ARG)
		return __LINE__;
	if (embedded_usb_bridge_stats(&bridge, NULL) !=
	    EMBEDDED_USB_BRIDGE_ERR_ARG)
		return __LINE__;

	return 0;
}

static int
test_usb_bridge_repeated_fend(void)
{
	struct usb_cdc_stub usb_stub;
	struct embedded_usb_bridge bridge;
	struct embedded_usb_bridge_stats stats;
	uint8_t data[4];

	data[0] = KISS_FEND;
	data[1] = KISS_FEND;
	data[2] = KISS_FEND;
	data[3] = KISS_FEND;

	usb_cdc_stub_init(&usb_stub);
	if (usb_cdc_stub_set_connected(&usb_stub, 1) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_usb_bridge_init(&bridge, usb_cdc_stub_usb(&usb_stub),
	    EMBEDDED_USB_BRIDGE_KISS_LOOPBACK) != EMBEDDED_USB_BRIDGE_OK)
		return __LINE__;
	if (usb_cdc_stub_inject_rx(&usb_stub, data, sizeof(data)) !=
	    KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_usb_bridge_service(&bridge) != EMBEDDED_USB_BRIDGE_OK)
		return __LINE__;
	if (usb_stub.tx_len != 0u)
		return __LINE__;
	if (embedded_usb_bridge_stats(&bridge, &stats) !=
	    EMBEDDED_USB_BRIDGE_OK)
		return __LINE__;
	if (stats.kiss_frames_in != 0u || stats.kiss_frames_out != 0u)
		return __LINE__;

	return 0;
}

static int
test_usb_bridge_unsupported_command(void)
{
	struct usb_cdc_stub usb_stub;
	struct embedded_usb_bridge bridge;
	struct embedded_usb_bridge_stats stats;
	uint8_t kiss[32];
	size_t kiss_len;

	if (kiss_encode_frame(0, 14, (const uint8_t *)"x", 1, kiss,
	    sizeof(kiss), &kiss_len) != KISS_OK)
		return __LINE__;

	usb_cdc_stub_init(&usb_stub);
	if (usb_cdc_stub_set_connected(&usb_stub, 1) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_usb_bridge_init(&bridge, usb_cdc_stub_usb(&usb_stub),
	    EMBEDDED_USB_BRIDGE_KISS_LOOPBACK) != EMBEDDED_USB_BRIDGE_OK)
		return __LINE__;
	if (usb_cdc_stub_inject_rx(&usb_stub, kiss, kiss_len) !=
	    KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_usb_bridge_service(&bridge) != EMBEDDED_USB_BRIDGE_OK)
		return __LINE__;
	if (embedded_usb_bridge_stats(&bridge, &stats) !=
	    EMBEDDED_USB_BRIDGE_OK)
		return __LINE__;
	if (stats.kiss_ignored_commands != 1u)
		return __LINE__;
	if (usb_stub.tx_len != 0u)
		return __LINE__;

	return 0;
}

int
test_embedded_usb_bridge(void)
{
	int line;

	line = test_usb_bridge_disconnected();
	if (line != 0)
		goto fail;
	line = test_usb_bridge_echo();
	if (line != 0)
		goto fail;
	line = test_usb_bridge_kiss_loopback();
	if (line != 0)
		goto fail;
	line = test_usb_bridge_escaped_kiss_loopback();
	if (line != 0)
		goto fail;
	line = test_usb_bridge_repeated_fend();
	if (line != 0)
		goto fail;
	line = test_usb_bridge_malformed_kiss();
	if (line != 0)
		goto fail;
	line = test_usb_bridge_unsupported_command();
	if (line != 0)
		goto fail;
	line = test_usb_bridge_app_step_services_bridge();
	if (line != 0)
		goto fail;
	line = test_usb_bridge_null_args();
	if (line != 0)
		goto fail;

	(void)printf("ok embedded_usb_bridge\n");
	return 0;

fail:
	(void)printf("not ok embedded_usb_bridge line %d\n", line);
	return 1;
}
