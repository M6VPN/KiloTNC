/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/tests/test_embedded_diag.c */

#include <sys/types.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "embedded_app.h"
#include "embedded_diag.h"
#include "embedded_usb_bridge.h"
#include "kiss.h"
#include "platform_stub.h"
#include "usb_cdc_stub.h"

static int diag_encode_kiss(uint8_t, const uint8_t *, size_t, uint8_t *,
	size_t, size_t *);
static int diag_make_app(struct platform_stub *, struct usb_cdc_stub *,
	struct embedded_usb_bridge *, struct embedded_app *);
static int test_diag_app_step_counters(void);
static int test_diag_format_small_buffer(void);
static int test_diag_format_stable_fields(void);
static int test_diag_fresh_app(void);
static int test_diag_malformed_kiss(void);
static int test_diag_null_args(void);
static int test_diag_unsupported_kiss_command(void);
static int test_diag_usb_kiss_counters(void);
static int test_diag_usb_overflows(void);
static int test_diag_watchdog_fault(void);

static int
diag_encode_kiss(uint8_t command, const uint8_t *payload, size_t payload_len,
	uint8_t *out, size_t out_cap, size_t *out_len)
{
	if (kiss_encode_frame(0, command, payload, payload_len, out, out_cap,
	    out_len) != KISS_OK)
		return __LINE__;

	return 0;
}

static int
diag_make_app(struct platform_stub *platform, struct usb_cdc_stub *usb_stub,
	struct embedded_usb_bridge *bridge, struct embedded_app *app)
{
	platform_stub_init(platform);
	usb_cdc_stub_init(usb_stub);
	if (usb_cdc_stub_set_connected(usb_stub, 1) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_usb_bridge_init(bridge, usb_cdc_stub_usb(usb_stub),
	    EMBEDDED_USB_BRIDGE_KISS_LOOPBACK) != EMBEDDED_USB_BRIDGE_OK)
		return __LINE__;
	if (embedded_app_init(app, platform_stub_platform(platform)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_app_usb_bridge(app, bridge) != EMBEDDED_APP_OK)
		return __LINE__;

	return 0;
}

static int
test_diag_app_step_counters(void)
{
	struct platform_stub platform;
	struct usb_cdc_stub usb_stub;
	struct embedded_usb_bridge bridge;
	struct embedded_app app;
	struct embedded_diag_snapshot snapshot;
	const struct kilotnc_platform *platform_if;

	if (diag_make_app(&platform, &usb_stub, &bridge, &app) != 0)
		return __LINE__;
	platform_if = platform_stub_platform(&platform);
	if (platform_if->diag_write(platform_if->ctx, "diag") !=
	    KILOTNC_PLATFORM_OK)
		return __LINE__;
	if (embedded_app_step(&app) != EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_diag_capture(&app, &snapshot) != EMBEDDED_DIAG_OK)
		return __LINE__;
	if (snapshot.app_steps != 1u)
		return __LINE__;
	if (snapshot.platform_ticks != 10u)
		return __LINE__;
	if (snapshot.watchdog_kicks != 1u)
		return __LINE__;
	if (snapshot.diagnostics_writes != 1u)
		return __LINE__;

	return 0;
}

static int
test_diag_format_small_buffer(void)
{
	struct embedded_diag_snapshot snapshot;
	char buf[8];
	size_t out_len;

	(void)memset(&snapshot, 0, sizeof(snapshot));
	if (embedded_diag_format(&snapshot, buf, sizeof(buf), &out_len) !=
	    EMBEDDED_DIAG_ERR_SMALL)
		return __LINE__;

	return 0;
}

static int
test_diag_format_stable_fields(void)
{
	struct embedded_diag_snapshot snapshot;
	char buf[4096];
	size_t out_len;

	(void)memset(&snapshot, 0, sizeof(snapshot));
	snapshot.app_steps = 1u;
	snapshot.ptt_state = KILOTNC_GPIO_LOW;
	if (embedded_diag_format(&snapshot, buf, sizeof(buf), &out_len) !=
	    EMBEDDED_DIAG_OK)
		return __LINE__;
	if (out_len == 0u)
		return __LINE__;
	if (strstr(buf, "app_steps=1 app_faults=0") == NULL)
		return __LINE__;
	if (strstr(buf, "kiss_parse_errors=0") == NULL)
		return __LINE__;
	if (strstr(buf, "audio_rx_samples=0 audio_tx_samples=0") == NULL)
		return __LINE__;
	if (strstr(buf, "tnc_mode=0 tnc_kiss_frames_in=0") == NULL)
		return __LINE__;
	if (strstr(buf, "ptt=0 usb_connected=0") == NULL)
		return __LINE__;

	return 0;
}

static int
test_diag_fresh_app(void)
{
	struct platform_stub platform;
	struct embedded_app app;
	struct embedded_diag_snapshot snapshot;

	platform_stub_init(&platform);
	if (embedded_app_init(&app, platform_stub_platform(&platform)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_diag_capture(&app, &snapshot) != EMBEDDED_DIAG_OK)
		return __LINE__;
	if (snapshot.ptt_state != KILOTNC_GPIO_LOW)
		return __LINE__;
	if (snapshot.reset_cause != KILOTNC_RESET_POWER_ON)
		return __LINE__;
	if (snapshot.app_state != EMBEDDED_APP_RUNNING)
		return __LINE__;

	return 0;
}

static int
test_diag_malformed_kiss(void)
{
	struct platform_stub platform;
	struct usb_cdc_stub usb_stub;
	struct embedded_usb_bridge bridge;
	struct embedded_app app;
	struct embedded_diag_snapshot snapshot;
	uint8_t data[6];

	data[0] = KISS_FEND;
	data[1] = KISS_CMD_DATA;
	data[2] = KISS_FESC;
	data[3] = 0x00u;
	data[4] = 'x';
	data[5] = KISS_FEND;
	if (diag_make_app(&platform, &usb_stub, &bridge, &app) != 0)
		return __LINE__;
	if (usb_cdc_stub_inject_rx(&usb_stub, data, sizeof(data)) !=
	    KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_app_step(&app) != EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_diag_capture(&app, &snapshot) != EMBEDDED_DIAG_OK)
		return __LINE__;
	if (snapshot.kiss_parse_errors != 1u)
		return __LINE__;
	if (snapshot.ptt_state != KILOTNC_GPIO_LOW)
		return __LINE__;

	return 0;
}

static int
test_diag_null_args(void)
{
	struct platform_stub platform;
	struct embedded_app app;
	struct embedded_diag_snapshot snapshot;
	char buf[64];
	size_t out_len;

	(void)memset(&snapshot, 0, sizeof(snapshot));
	platform_stub_init(&platform);
	if (embedded_app_init(&app, platform_stub_platform(&platform)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_diag_capture(NULL, &snapshot) != EMBEDDED_DIAG_ERR_ARG)
		return __LINE__;
	if (embedded_diag_capture(&app, NULL) != EMBEDDED_DIAG_ERR_ARG)
		return __LINE__;
	if (embedded_diag_format(NULL, buf, sizeof(buf), &out_len) !=
	    EMBEDDED_DIAG_ERR_ARG)
		return __LINE__;
	if (embedded_diag_format(&snapshot, NULL, sizeof(buf), &out_len) !=
	    EMBEDDED_DIAG_ERR_ARG)
		return __LINE__;
	if (embedded_diag_format(&snapshot, buf, sizeof(buf), NULL) !=
	    EMBEDDED_DIAG_ERR_ARG)
		return __LINE__;
	if (embedded_diag_format(&snapshot, buf, 0, &out_len) !=
	    EMBEDDED_DIAG_ERR_SMALL)
		return __LINE__;

	return 0;
}

static int
test_diag_unsupported_kiss_command(void)
{
	struct platform_stub platform;
	struct usb_cdc_stub usb_stub;
	struct embedded_usb_bridge bridge;
	struct embedded_app app;
	struct embedded_diag_snapshot snapshot;
	uint8_t kiss[32];
	size_t kiss_len;

	if (diag_encode_kiss(14, (const uint8_t *)"x", 1, kiss,
	    sizeof(kiss), &kiss_len) != 0)
		return __LINE__;
	if (diag_make_app(&platform, &usb_stub, &bridge, &app) != 0)
		return __LINE__;
	if (usb_cdc_stub_inject_rx(&usb_stub, kiss, kiss_len) !=
	    KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_app_step(&app) != EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_diag_capture(&app, &snapshot) != EMBEDDED_DIAG_OK)
		return __LINE__;
	if (snapshot.kiss_ignored_commands != 1u)
		return __LINE__;
	if (snapshot.kiss_frames_out != 0u)
		return __LINE__;

	return 0;
}

static int
test_diag_usb_kiss_counters(void)
{
	struct platform_stub platform;
	struct usb_cdc_stub usb_stub;
	struct embedded_usb_bridge bridge;
	struct embedded_app app;
	struct embedded_diag_snapshot snapshot;
	uint8_t kiss[32];
	size_t kiss_len;

	if (diag_encode_kiss(KISS_CMD_DATA, (const uint8_t *)"abc", 3, kiss,
	    sizeof(kiss), &kiss_len) != 0)
		return __LINE__;
	if (diag_make_app(&platform, &usb_stub, &bridge, &app) != 0)
		return __LINE__;
	if (usb_cdc_stub_inject_rx(&usb_stub, kiss, kiss_len) !=
	    KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_app_step(&app) != EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_diag_capture(&app, &snapshot) != EMBEDDED_DIAG_OK)
		return __LINE__;
	if (snapshot.usb_rx_bytes != kiss_len)
		return __LINE__;
	if (snapshot.usb_tx_bytes == 0u)
		return __LINE__;
	if (snapshot.usb_connected != 1u)
		return __LINE__;
	if (snapshot.kiss_frames_in != 1u)
		return __LINE__;
	if (snapshot.kiss_frames_out != 1u)
		return __LINE__;

	return 0;
}

static int
test_diag_usb_overflows(void)
{
	struct platform_stub platform;
	struct usb_cdc_stub usb_stub;
	struct embedded_usb_bridge bridge;
	struct embedded_app app;
	struct embedded_diag_snapshot snapshot;
	const struct kilotnc_usb_cdc *usb;
	uint8_t data[USB_CDC_STUB_BUFFER_MAX + 1U];
	size_t len;

	(void)memset(data, 0x55, sizeof(data));
	if (diag_make_app(&platform, &usb_stub, &bridge, &app) != 0)
		return __LINE__;
	usb = usb_cdc_stub_usb(&usb_stub);
	if (usb_cdc_stub_inject_rx(&usb_stub, data, sizeof(data)) !=
	    KILOTNC_USB_ERR_SMALL)
		return __LINE__;
	if (usb->write(usb->ctx, data, sizeof(data), &len) !=
	    KILOTNC_USB_ERR_SMALL)
		return __LINE__;
	if (embedded_diag_capture(&app, &snapshot) != EMBEDDED_DIAG_OK)
		return __LINE__;
	if (snapshot.usb_rx_overflows != 1u)
		return __LINE__;
	if (snapshot.usb_tx_overflows != 1u)
		return __LINE__;

	return 0;
}

static int
test_diag_watchdog_fault(void)
{
	struct platform_stub platform;
	struct embedded_app app;
	struct embedded_diag_snapshot snapshot;
	const struct kilotnc_platform *platform_if;

	platform_stub_init(&platform);
	platform_if = platform_stub_platform(&platform);
	if (embedded_app_init(&app, platform_if) != EMBEDDED_APP_OK)
		return __LINE__;
	if (platform_if->ptt_set(platform_if->ctx, KILOTNC_GPIO_HIGH) !=
	    KILOTNC_PLATFORM_OK)
		return __LINE__;
	if (platform_stub_simulate_watchdog_fault(&platform) !=
	    KILOTNC_PLATFORM_OK)
		return __LINE__;
	if (embedded_app_step(&app) != EMBEDDED_APP_ERR_FAULT)
		return __LINE__;
	if (embedded_diag_capture(&app, &snapshot) != EMBEDDED_DIAG_OK)
		return __LINE__;
	if (snapshot.app_state != EMBEDDED_APP_FAULT)
		return __LINE__;
	if (snapshot.reset_cause != KILOTNC_RESET_WATCHDOG)
		return __LINE__;
	if (snapshot.ptt_state != KILOTNC_GPIO_LOW)
		return __LINE__;
	if (snapshot.app_faults != 1u)
		return __LINE__;

	return 0;
}

int
test_embedded_diag(void)
{
	int line;

	line = test_diag_fresh_app();
	if (line != 0)
		goto fail;
	line = test_diag_app_step_counters();
	if (line != 0)
		goto fail;
	line = test_diag_usb_kiss_counters();
	if (line != 0)
		goto fail;
	line = test_diag_malformed_kiss();
	if (line != 0)
		goto fail;
	line = test_diag_unsupported_kiss_command();
	if (line != 0)
		goto fail;
	line = test_diag_usb_overflows();
	if (line != 0)
		goto fail;
	line = test_diag_watchdog_fault();
	if (line != 0)
		goto fail;
	line = test_diag_format_stable_fields();
	if (line != 0)
		goto fail;
	line = test_diag_format_small_buffer();
	if (line != 0)
		goto fail;
	line = test_diag_null_args();
	if (line != 0)
		goto fail;

	(void)printf("ok embedded_diag\n");
	return 0;

fail:
	(void)printf("not ok embedded_diag line %d\n", line);
	return 1;
}
