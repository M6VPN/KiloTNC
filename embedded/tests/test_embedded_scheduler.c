/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/tests/test_embedded_scheduler.c */

#include <sys/types.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ax25.h"
#include "audio_stub.h"
#include "embedded_app.h"
#include "embedded_diag.h"
#include "embedded_modem.h"
#include "embedded_scheduler.h"
#include "embedded_usb_bridge.h"
#include "kiss.h"
#include "platform_stub.h"
#include "usb_cdc_stub.h"

static int build_scheduler_ax25(uint8_t *, size_t, size_t *);
static int test_scheduler_app_abort_modem_on_fault(void);
static int test_scheduler_app_default_quorum(void);
static int test_scheduler_app_diag_fields(void);
static int test_scheduler_app_malformed_kiss_progress(void);
static int test_scheduler_app_missing_usb_fault(void);
static int test_scheduler_app_shutdown(void);
static int test_scheduler_basic_masks(void);
static int test_scheduler_cycle_fault(void);
static int test_scheduler_cycle_success(void);
static int test_scheduler_invalid_task(void);
static int test_scheduler_null_args(void);

static int
build_scheduler_ax25(uint8_t *out, size_t out_cap, size_t *out_len)
{
	struct ax25_frame frame;

	(void)memset(&frame, 0, sizeof(frame));
	(void)memcpy(frame.dst.callsign, "APZKTN", 6);
	(void)memcpy(frame.src.callsign, "M6VPN", 5);
	frame.pid = AX25_PID_NONE;
	(void)memcpy(frame.info, "sched", 5);
	frame.info_len = 5U;
	if (ax25_encode_ui_fcs(&frame, out, out_cap, out_len) != AX25_OK)
		return __LINE__;

	return 0;
}

static int
test_scheduler_app_abort_modem_on_fault(void)
{
	struct audio_stub audio;
	struct embedded_app app;
	struct embedded_modem modem;
	struct embedded_modem_status modem_status;
	struct platform_stub platform;
	uint8_t frame[KILOTNC_AX25_MAX_FRAME];
	size_t frame_len;
	int line;

	line = build_scheduler_ax25(frame, sizeof(frame), &frame_len);
	if (line != 0)
		return line;
	audio_stub_init(&audio);
	platform_stub_init(&platform);
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
	if (embedded_scheduler_require_task(&app.scheduler, EMBEDDED_TASK_USB,
	    1) != EMBEDDED_SCHEDULER_OK)
		return __LINE__;
	if (embedded_app_step(&app) != EMBEDDED_APP_ERR_FAULT)
		return __LINE__;
	if (embedded_modem_status(&modem, &modem_status) !=
	    EMBEDDED_MODEM_OK)
		return __LINE__;
	if (modem_status.tx_active != 0U)
		return __LINE__;
	if (modem_status.aborts == 0U)
		return __LINE__;
	if (platform.ptt_state != KILOTNC_GPIO_LOW)
		return __LINE__;

	return 0;
}

static int
test_scheduler_app_default_quorum(void)
{
	struct embedded_app app;
	struct embedded_app_status status;
	struct platform_stub platform;

	platform_stub_init(&platform);
	if (embedded_app_init(&app, platform_stub_platform(&platform)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_app_step(&app) != EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_app_status(&app, &status) != EMBEDDED_APP_OK)
		return __LINE__;
	if (platform.watchdog_kicks != 1U)
		return __LINE__;
	if (status.watchdog_kicks != 1U)
		return __LINE__;
	if (status.scheduler_cycles != 1U)
		return __LINE__;
	if (status.scheduler_faults != 0U)
		return __LINE__;
	if (status.scheduler_watchdog_allowed != 1)
		return __LINE__;

	return 0;
}

static int
test_scheduler_app_diag_fields(void)
{
	struct embedded_app app;
	struct embedded_diag_snapshot snapshot;
	struct platform_stub platform;
	char buf[2048];
	size_t out_len;

	platform_stub_init(&platform);
	if (embedded_app_init(&app, platform_stub_platform(&platform)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_app_step(&app) != EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_diag_capture(&app, &snapshot) != EMBEDDED_DIAG_OK)
		return __LINE__;
	if (snapshot.scheduler_cycles != 1U)
		return __LINE__;
	if (snapshot.scheduler_faults != 0U)
		return __LINE__;
	if (snapshot.scheduler_watchdog_allowed != 1U)
		return __LINE__;
	if ((snapshot.scheduler_required_mask &
	    (1U << EMBEDDED_TASK_CONTROL)) == 0U)
		return __LINE__;
	if (embedded_diag_format(&snapshot, buf, sizeof(buf), &out_len) !=
	    EMBEDDED_DIAG_OK)
		return __LINE__;
	if (strstr(buf, "scheduler_cycles=1") == NULL)
		return __LINE__;
	if (strstr(buf, "scheduler_watchdog_allowed=1") == NULL)
		return __LINE__;

	return 0;
}

static int
test_scheduler_app_malformed_kiss_progress(void)
{
	struct embedded_app app;
	struct embedded_app_status status;
	struct embedded_usb_bridge bridge;
	struct platform_stub platform;
	struct usb_cdc_stub usb;
	uint8_t malformed[5];

	malformed[0] = KISS_FEND;
	malformed[1] = KISS_CMD_DATA;
	malformed[2] = KISS_FESC;
	malformed[3] = 0U;
	malformed[4] = KISS_FEND;
	platform_stub_init(&platform);
	usb_cdc_stub_init(&usb);
	if (usb_cdc_stub_set_connected(&usb, 1) != KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_usb_bridge_init(&bridge, usb_cdc_stub_usb(&usb),
	    EMBEDDED_USB_BRIDGE_KISS_LOOPBACK) != EMBEDDED_USB_BRIDGE_OK)
		return __LINE__;
	if (embedded_app_init(&app, platform_stub_platform(&platform)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_app_usb_bridge(&app, &bridge) != EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_scheduler_require_task(&app.scheduler, EMBEDDED_TASK_USB,
	    1) != EMBEDDED_SCHEDULER_OK)
		return __LINE__;
	if (usb_cdc_stub_inject_rx(&usb, malformed, sizeof(malformed)) !=
	    KILOTNC_USB_OK)
		return __LINE__;
	if (embedded_app_step(&app) != EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_app_status(&app, &status) != EMBEDDED_APP_OK)
		return __LINE__;
	if (status.scheduler_cycles != 1U || status.scheduler_faults != 0U)
		return __LINE__;
	if (status.ptt_state != KILOTNC_GPIO_LOW)
		return __LINE__;

	return 0;
}

static int
test_scheduler_app_missing_usb_fault(void)
{
	struct embedded_app app;
	struct embedded_app_status status;
	struct platform_stub platform;

	platform_stub_init(&platform);
	if (embedded_app_init(&app, platform_stub_platform(&platform)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_scheduler_require_task(&app.scheduler, EMBEDDED_TASK_USB,
	    1) != EMBEDDED_SCHEDULER_OK)
		return __LINE__;
	platform.ptt_state = KILOTNC_GPIO_HIGH;
	if (embedded_app_step(&app) != EMBEDDED_APP_ERR_FAULT)
		return __LINE__;
	if (embedded_app_status(&app, &status) != EMBEDDED_APP_OK)
		return __LINE__;
	if (platform.watchdog_kicks != 0U || status.watchdog_kicks != 0U)
		return __LINE__;
	if (status.state != EMBEDDED_APP_FAULT)
		return __LINE__;
	if (status.scheduler_faults != 1U)
		return __LINE__;
	if (status.scheduler_last_failed_task != EMBEDDED_TASK_USB)
		return __LINE__;
	if (status.ptt_state != KILOTNC_GPIO_LOW)
		return __LINE__;

	return 0;
}

static int
test_scheduler_app_shutdown(void)
{
	struct embedded_app app;
	struct embedded_app_status status;
	struct platform_stub platform;

	platform_stub_init(&platform);
	if (embedded_app_init(&app, platform_stub_platform(&platform)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	platform.ptt_state = KILOTNC_GPIO_HIGH;
	if (embedded_app_shutdown(&app) != EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_app_status(&app, &status) != EMBEDDED_APP_OK)
		return __LINE__;
	if (status.state != EMBEDDED_APP_STOPPED)
		return __LINE__;
	if (status.ptt_state != KILOTNC_GPIO_LOW)
		return __LINE__;
	if (embedded_app_step(&app) != EMBEDDED_APP_ERR_FAULT)
		return __LINE__;

	return 0;
}

static int
test_scheduler_basic_masks(void)
{
	struct embedded_scheduler scheduler;
	struct embedded_scheduler_status status;

	if (embedded_scheduler_init(&scheduler) != EMBEDDED_SCHEDULER_OK)
		return __LINE__;
	if (embedded_scheduler_status(&scheduler, &status) !=
	    EMBEDDED_SCHEDULER_OK)
		return __LINE__;
	if ((status.required_mask & (1U << EMBEDDED_TASK_MAIN)) == 0U)
		return __LINE__;
	if ((status.required_mask & (1U << EMBEDDED_TASK_CONTROL)) == 0U)
		return __LINE__;
	if (embedded_scheduler_enable_task(&scheduler, EMBEDDED_TASK_USB, 1) !=
	    EMBEDDED_SCHEDULER_OK)
		return __LINE__;
	if (embedded_scheduler_require_task(&scheduler, EMBEDDED_TASK_USB, 1) !=
	    EMBEDDED_SCHEDULER_OK)
		return __LINE__;
	if (embedded_scheduler_require_task(&scheduler, EMBEDDED_TASK_USB, 0) !=
	    EMBEDDED_SCHEDULER_OK)
		return __LINE__;
	if (embedded_scheduler_enable_task(&scheduler, EMBEDDED_TASK_USB, 0) !=
	    EMBEDDED_SCHEDULER_OK)
		return __LINE__;
	if (embedded_scheduler_status(&scheduler, &status) !=
	    EMBEDDED_SCHEDULER_OK)
		return __LINE__;
	if ((status.enabled_mask & (1U << EMBEDDED_TASK_USB)) != 0U)
		return __LINE__;
	if ((status.required_mask & (1U << EMBEDDED_TASK_USB)) != 0U)
		return __LINE__;

	return 0;
}

static int
test_scheduler_cycle_fault(void)
{
	struct embedded_scheduler scheduler;
	struct embedded_scheduler_status status;
	int allowed;
	int faulted;

	if (embedded_scheduler_init(&scheduler) != EMBEDDED_SCHEDULER_OK)
		return __LINE__;
	if (embedded_scheduler_mark_progress(&scheduler,
	    EMBEDDED_TASK_MAIN) != EMBEDDED_SCHEDULER_OK)
		return __LINE__;
	if (embedded_scheduler_cycle_complete(&scheduler) !=
	    EMBEDDED_SCHEDULER_ERR_FAULT)
		return __LINE__;
	if (embedded_scheduler_watchdog_allowed(&scheduler, &allowed) !=
	    EMBEDDED_SCHEDULER_OK)
		return __LINE__;
	if (embedded_scheduler_fault(&scheduler, &faulted) !=
	    EMBEDDED_SCHEDULER_OK)
		return __LINE__;
	if (allowed != 0 || faulted != 1)
		return __LINE__;
	if (embedded_scheduler_status(&scheduler, &status) !=
	    EMBEDDED_SCHEDULER_OK)
		return __LINE__;
	if (status.last_failed_task != EMBEDDED_TASK_CONTROL)
		return __LINE__;

	return 0;
}

static int
test_scheduler_cycle_success(void)
{
	struct embedded_scheduler scheduler;
	struct embedded_scheduler_status status;
	int allowed;

	if (embedded_scheduler_init(&scheduler) != EMBEDDED_SCHEDULER_OK)
		return __LINE__;
	if (embedded_scheduler_mark_progress(&scheduler,
	    EMBEDDED_TASK_MAIN) != EMBEDDED_SCHEDULER_OK)
		return __LINE__;
	if (embedded_scheduler_mark_progress(&scheduler,
	    EMBEDDED_TASK_CONTROL) != EMBEDDED_SCHEDULER_OK)
		return __LINE__;
	if (embedded_scheduler_cycle_complete(&scheduler) !=
	    EMBEDDED_SCHEDULER_OK)
		return __LINE__;
	if (embedded_scheduler_watchdog_allowed(&scheduler, &allowed) !=
	    EMBEDDED_SCHEDULER_OK)
		return __LINE__;
	if (allowed != 1)
		return __LINE__;
	if (embedded_scheduler_status(&scheduler, &status) !=
	    EMBEDDED_SCHEDULER_OK)
		return __LINE__;
	if (status.cycles_completed != 1U || status.progress_mask != 0U)
		return __LINE__;

	return 0;
}

static int
test_scheduler_invalid_task(void)
{
	struct embedded_scheduler scheduler;
	enum embedded_task_id bad;

	bad = EMBEDDED_TASK_COUNT;
	if (embedded_scheduler_init(&scheduler) != EMBEDDED_SCHEDULER_OK)
		return __LINE__;
	if (embedded_scheduler_enable_task(&scheduler, bad, 1) !=
	    EMBEDDED_SCHEDULER_ERR_RANGE)
		return __LINE__;
	if (embedded_scheduler_require_task(&scheduler, bad, 1) !=
	    EMBEDDED_SCHEDULER_ERR_RANGE)
		return __LINE__;
	if (embedded_scheduler_mark_progress(&scheduler, bad) !=
	    EMBEDDED_SCHEDULER_ERR_RANGE)
		return __LINE__;
	if (embedded_scheduler_force_fault(&scheduler, bad) !=
	    EMBEDDED_SCHEDULER_ERR_RANGE)
		return __LINE__;

	return 0;
}

static int
test_scheduler_null_args(void)
{
	struct embedded_scheduler scheduler;
	struct embedded_scheduler_status status;
	int value;

	if (embedded_scheduler_init(NULL) != EMBEDDED_SCHEDULER_ERR_ARG)
		return __LINE__;
	if (embedded_scheduler_init(&scheduler) != EMBEDDED_SCHEDULER_OK)
		return __LINE__;
	if (embedded_scheduler_enable_task(NULL, EMBEDDED_TASK_USB, 1) !=
	    EMBEDDED_SCHEDULER_ERR_ARG)
		return __LINE__;
	if (embedded_scheduler_require_task(NULL, EMBEDDED_TASK_USB, 1) !=
	    EMBEDDED_SCHEDULER_ERR_ARG)
		return __LINE__;
	if (embedded_scheduler_mark_progress(NULL, EMBEDDED_TASK_USB) !=
	    EMBEDDED_SCHEDULER_ERR_ARG)
		return __LINE__;
	if (embedded_scheduler_cycle_complete(NULL) !=
	    EMBEDDED_SCHEDULER_ERR_ARG)
		return __LINE__;
	if (embedded_scheduler_force_fault(NULL, EMBEDDED_TASK_USB) !=
	    EMBEDDED_SCHEDULER_ERR_ARG)
		return __LINE__;
	if (embedded_scheduler_status(NULL, &status) !=
	    EMBEDDED_SCHEDULER_ERR_ARG)
		return __LINE__;
	if (embedded_scheduler_status(&scheduler, NULL) !=
	    EMBEDDED_SCHEDULER_ERR_ARG)
		return __LINE__;
	if (embedded_scheduler_watchdog_allowed(NULL, &value) !=
	    EMBEDDED_SCHEDULER_ERR_ARG)
		return __LINE__;
	if (embedded_scheduler_watchdog_allowed(&scheduler, NULL) !=
	    EMBEDDED_SCHEDULER_ERR_ARG)
		return __LINE__;
	if (embedded_scheduler_fault(NULL, &value) !=
	    EMBEDDED_SCHEDULER_ERR_ARG)
		return __LINE__;
	if (embedded_scheduler_fault(&scheduler, NULL) !=
	    EMBEDDED_SCHEDULER_ERR_ARG)
		return __LINE__;

	return 0;
}

int
test_embedded_scheduler(void)
{
	int line;

	line = test_scheduler_basic_masks();
	if (line != 0)
		goto fail;
	line = test_scheduler_cycle_success();
	if (line != 0)
		goto fail;
	line = test_scheduler_cycle_fault();
	if (line != 0)
		goto fail;
	line = test_scheduler_invalid_task();
	if (line != 0)
		goto fail;
	line = test_scheduler_null_args();
	if (line != 0)
		goto fail;
	line = test_scheduler_app_default_quorum();
	if (line != 0)
		goto fail;
	line = test_scheduler_app_missing_usb_fault();
	if (line != 0)
		goto fail;
	line = test_scheduler_app_abort_modem_on_fault();
	if (line != 0)
		goto fail;
	line = test_scheduler_app_diag_fields();
	if (line != 0)
		goto fail;
	line = test_scheduler_app_malformed_kiss_progress();
	if (line != 0)
		goto fail;
	line = test_scheduler_app_shutdown();
	if (line != 0)
		goto fail;

	(void)printf("ok embedded_scheduler\n");
	return 0;

fail:
	(void)printf("not ok embedded_scheduler line %d\n", line);
	return line;
}
