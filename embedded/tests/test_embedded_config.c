/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/tests/test_embedded_config.c */

#include <sys/types.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "embedded_app.h"
#include "embedded_config.h"
#include "embedded_diag.h"
#include "embedded_tnc.h"
#include "kiss.h"
#include "platform_stub.h"
#include "tnc_mode.h"
#include "usb_cdc_stub.h"

static int config_encode_command(uint8_t, uint8_t, uint8_t *, size_t,
	size_t *);
static int config_process_command(struct embedded_tnc *,
	struct usb_cdc_stub *, uint8_t, uint8_t);
static int test_config_defaults(void);
static int test_config_diag_and_ptt(void);
static int test_config_invalid_schema(void);
static int test_config_kiss_settings(void);
static int test_config_load_persist(void);
static int test_config_null_args(void);
static int test_config_ranges(void);
static int test_config_sethw(void);
static int test_config_tnc_defaults(void);

static int
config_encode_command(uint8_t command, uint8_t value, uint8_t *out,
	size_t out_cap, size_t *out_len)
{
	if (kiss_encode_frame(0, command, &value, 1, out, out_cap,
	    out_len) != KISS_OK)
		return __LINE__;

	return 0;
}

static int
config_process_command(struct embedded_tnc *tnc, struct usb_cdc_stub *usb,
	uint8_t command, uint8_t value)
{
	uint8_t kiss[32];
	size_t kiss_len;
	int line;

	line = config_encode_command(command, value, kiss, sizeof(kiss),
	    &kiss_len);
	if (line != 0)
		return line;
	if (usb_cdc_stub_inject_rx(usb, kiss, kiss_len) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_tnc_process_usb(tnc, usb_cdc_stub_usb(usb)) !=
	    EMBEDDED_TNC_OK)
		return __LINE__;

	return 0;
}

static int
test_config_defaults(void)
{
	struct embedded_config config;

	if (embedded_config_defaults(&config) != EMBEDDED_CONFIG_OK)
		return __LINE__;
	if (embedded_config_validate(&config) != EMBEDDED_CONFIG_OK)
		return __LINE__;
	if (config.magic != EMBEDDED_CONFIG_MAGIC)
		return __LINE__;
	if (config.schema_version != EMBEDDED_CONFIG_SCHEMA_VERSION)
		return __LINE__;
	if (config.mode_id != TNC_MODE_1200_AFSK_AX25)
		return __LINE__;
	if (config.requested_mode_id != TNC_MODE_1200_AFSK_AX25)
		return __LINE__;
	if (config.max_tx_ms == 0U)
		return __LINE__;
	if (config.txdelay != 50U || config.p != 63U ||
	    config.slottime != 10U || config.txtail != 0U)
		return __LINE__;

	return 0;
}

static int
test_config_diag_and_ptt(void)
{
	struct embedded_app app;
	struct embedded_diag_snapshot snapshot;
	struct embedded_tnc tnc;
	struct platform_stub platform;
	struct usb_cdc_stub usb;
	char formatted[2048];
	enum kilotnc_gpio_state ptt;
	size_t formatted_len;

	platform_stub_init(&platform);
	usb_cdc_stub_init(&usb);
	if (usb_cdc_stub_set_connected(&usb, 1) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_tnc_init(&tnc) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (embedded_app_init(&app, platform_stub_platform(&platform)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_app_tnc(&app, &tnc, usb_cdc_stub_usb(&usb)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	if (config_process_command(&tnc, &usb, KISS_CMD_TXDELAY, 12U) != 0)
		return __LINE__;
	if (config_process_command(&tnc, &usb, KISS_CMD_SETHARDWARE, 22U) !=
	    0)
		return __LINE__;
	if (embedded_diag_capture(&app, &snapshot) != EMBEDDED_DIAG_OK)
		return __LINE__;
	if (snapshot.config_schema_version !=
	    EMBEDDED_CONFIG_SCHEMA_VERSION)
		return __LINE__;
	if (snapshot.config_requested_mode != TNC_MODE_1200_AFSK_AX25)
		return __LINE__;
	if (snapshot.config_temporary != 1U)
		return __LINE__;
	if (snapshot.config_max_tx_ms !=
	    EMBEDDED_CONFIG_DEFAULT_MAX_TX_MS)
		return __LINE__;
	if (snapshot.tnc_txdelay != 12U)
		return __LINE__;
	if (embedded_diag_format(&snapshot, formatted, sizeof(formatted),
	    &formatted_len) != EMBEDDED_DIAG_OK)
		return __LINE__;
	if (strstr(formatted, "config_schema=1") == NULL)
		return __LINE__;
	if (strstr(formatted, "config_temporary=1") == NULL)
		return __LINE__;
	if (platform_stub_ptt_state(&platform, &ptt) != KILOTNC_PLATFORM_OK)
		return __LINE__;
	if (ptt != KILOTNC_GPIO_LOW)
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

static int
test_config_invalid_schema(void)
{
	struct embedded_config config;

	if (embedded_config_defaults(&config) != EMBEDDED_CONFIG_OK)
		return __LINE__;
	config.schema_version++;
	if (embedded_config_validate(&config) !=
	    EMBEDDED_CONFIG_ERR_RANGE)
		return __LINE__;
	if (embedded_config_defaults(&config) != EMBEDDED_CONFIG_OK)
		return __LINE__;
	config.magic = 0U;
	if (embedded_config_validate(&config) !=
	    EMBEDDED_CONFIG_ERR_RANGE)
		return __LINE__;

	return 0;
}

static int
test_config_kiss_settings(void)
{
	struct embedded_tnc tnc;
	struct embedded_tnc_status status;
	struct usb_cdc_stub usb;

	usb_cdc_stub_init(&usb);
	if (usb_cdc_stub_set_connected(&usb, 1) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_tnc_init(&tnc) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (config_process_command(&tnc, &usb, KISS_CMD_TXDELAY, 11U) != 0)
		return __LINE__;
	if (config_process_command(&tnc, &usb, KISS_CMD_P, 255U) != 0)
		return __LINE__;
	if (config_process_command(&tnc, &usb, KISS_CMD_SLOTTIME, 7U) != 0)
		return __LINE__;
	if (config_process_command(&tnc, &usb, KISS_CMD_TXTAIL, 3U) != 0)
		return __LINE__;
	if (config_process_command(&tnc, &usb, KISS_CMD_FULLDUPLEX, 1U) !=
	    0)
		return __LINE__;
	if (embedded_tnc_status(&tnc, &status) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (status.txdelay != 11U || status.p != 255U ||
	    status.slottime != 7U || status.txtail != 3U ||
	    status.fullduplex != 1U)
		return __LINE__;
	if (status.config_validation_errors != 0U)
		return __LINE__;

	return 0;
}

static int
test_config_load_persist(void)
{
	struct embedded_config config;

	if (embedded_config_defaults(&config) != EMBEDDED_CONFIG_OK)
		return __LINE__;
	if (embedded_config_persist(&config) !=
	    EMBEDDED_CONFIG_ERR_NOT_IMPLEMENTED)
		return __LINE__;
	if (embedded_config_load(&config) !=
	    EMBEDDED_CONFIG_ERR_NOT_IMPLEMENTED)
		return __LINE__;

	return 0;
}

static int
test_config_null_args(void)
{
	struct embedded_config config;

	if (embedded_config_defaults(NULL) != EMBEDDED_CONFIG_ERR_ARG)
		return __LINE__;
	if (embedded_config_validate(NULL) != EMBEDDED_CONFIG_ERR_ARG)
		return __LINE__;
	if (embedded_config_apply_nino_sethw(NULL, 6U) !=
	    EMBEDDED_CONFIG_ERR_ARG)
		return __LINE__;
	if (embedded_config_apply_kiss_setting(NULL, KISS_CMD_P, 1U) !=
	    EMBEDDED_CONFIG_ERR_ARG)
		return __LINE__;
	if (embedded_config_persist(NULL) != EMBEDDED_CONFIG_ERR_ARG)
		return __LINE__;
	if (embedded_config_load(NULL) != EMBEDDED_CONFIG_ERR_ARG)
		return __LINE__;
	if (embedded_config_defaults(&config) != EMBEDDED_CONFIG_OK)
		return __LINE__;
	if (embedded_config_apply_kiss_setting(&config, 99U, 1U) !=
	    EMBEDDED_CONFIG_ERR_UNSUPPORTED)
		return __LINE__;

	return 0;
}

static int
test_config_ranges(void)
{
	struct embedded_config config;

	if (embedded_config_defaults(&config) != EMBEDDED_CONFIG_OK)
		return __LINE__;
	config.p = 256U;
	if (embedded_config_validate(&config) !=
	    EMBEDDED_CONFIG_ERR_RANGE)
		return __LINE__;
	if (embedded_config_defaults(&config) != EMBEDDED_CONFIG_OK)
		return __LINE__;
	config.slottime = 256U;
	if (embedded_config_validate(&config) !=
	    EMBEDDED_CONFIG_ERR_RANGE)
		return __LINE__;
	if (embedded_config_defaults(&config) != EMBEDDED_CONFIG_OK)
		return __LINE__;
	config.txdelay = 256U;
	if (embedded_config_validate(&config) !=
	    EMBEDDED_CONFIG_ERR_RANGE)
		return __LINE__;
	if (embedded_config_defaults(&config) != EMBEDDED_CONFIG_OK)
		return __LINE__;
	config.txtail = 256U;
	if (embedded_config_validate(&config) !=
	    EMBEDDED_CONFIG_ERR_RANGE)
		return __LINE__;
	if (embedded_config_defaults(&config) != EMBEDDED_CONFIG_OK)
		return __LINE__;
	config.fullduplex = 2U;
	if (embedded_config_validate(&config) !=
	    EMBEDDED_CONFIG_ERR_RANGE)
		return __LINE__;
	if (embedded_config_defaults(&config) != EMBEDDED_CONFIG_OK)
		return __LINE__;
	config.max_tx_ms = 0U;
	if (embedded_config_validate(&config) !=
	    EMBEDDED_CONFIG_ERR_RANGE)
		return __LINE__;

	return 0;
}

static int
test_config_sethw(void)
{
	struct embedded_config config;

	if (embedded_config_defaults(&config) != EMBEDDED_CONFIG_OK)
		return __LINE__;
	if (embedded_config_apply_nino_sethw(&config, 6U) !=
	    EMBEDDED_CONFIG_OK)
		return __LINE__;
	if (config.mode_id != TNC_MODE_1200_AFSK_AX25 ||
	    config.requested_mode_id != TNC_MODE_1200_AFSK_AX25 ||
	    config.mode_temporary != 0U)
		return __LINE__;
	if (embedded_config_apply_nino_sethw(&config, 22U) !=
	    EMBEDDED_CONFIG_OK)
		return __LINE__;
	if (config.mode_id != TNC_MODE_1200_AFSK_AX25 ||
	    config.requested_mode_id != TNC_MODE_1200_AFSK_AX25 ||
	    config.mode_temporary != 1U)
		return __LINE__;
	if (embedded_config_apply_nino_sethw(&config, 0U) !=
	    EMBEDDED_CONFIG_ERR_UNSUPPORTED)
		return __LINE__;
	if (config.mode_id != TNC_MODE_1200_AFSK_AX25)
		return __LINE__;
	if (embedded_config_apply_nino_sethw(&config, 31U) !=
	    EMBEDDED_CONFIG_ERR_RANGE)
		return __LINE__;
	if (config.mode_id != TNC_MODE_1200_AFSK_AX25)
		return __LINE__;

	return 0;
}

static int
test_config_tnc_defaults(void)
{
	struct embedded_tnc tnc;
	struct embedded_tnc_status status;

	if (embedded_tnc_init(&tnc) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (embedded_tnc_status(&tnc, &status) != EMBEDDED_TNC_OK)
		return __LINE__;
	if (status.current_mode != TNC_MODE_1200_AFSK_AX25)
		return __LINE__;
	if (status.config_schema_version != EMBEDDED_CONFIG_SCHEMA_VERSION)
		return __LINE__;
	if (status.max_tx_ms != EMBEDDED_CONFIG_DEFAULT_MAX_TX_MS)
		return __LINE__;
	if (status.ptt_state != TNC_CONTROL_PTT_OFF)
		return __LINE__;

	return 0;
}

int
test_embedded_config(void)
{
	int line;

	line = test_config_defaults();
	if (line != 0)
		goto fail;
	line = test_config_ranges();
	if (line != 0)
		goto fail;
	line = test_config_sethw();
	if (line != 0)
		goto fail;
	line = test_config_load_persist();
	if (line != 0)
		goto fail;
	line = test_config_invalid_schema();
	if (line != 0)
		goto fail;
	line = test_config_tnc_defaults();
	if (line != 0)
		goto fail;
	line = test_config_kiss_settings();
	if (line != 0)
		goto fail;
	line = test_config_diag_and_ptt();
	if (line != 0)
		goto fail;
	line = test_config_null_args();
	if (line != 0)
		goto fail;

	(void)printf("ok embedded_config\n");
	return 0;

fail:
	(void)printf("not ok embedded_config line %d\n", line);
	return 1;
}
