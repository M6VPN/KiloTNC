/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/tests/test_embedded_app.c */

#include <sys/types.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "embedded_app.h"
#include "kilotnc_board.h"
#include "kilotnc_embedded.h"
#include "platform_stub.h"
#include "target.h"

static int test_app_fault_forces_ptt_off(void);
static int test_app_init_forces_ptt_off(void);
static int test_app_null_args(void);
static int test_app_shutdown_forces_ptt_off(void);
static int test_app_step_ticks_and_kicks_watchdog(void);
static int test_stub_diag_write(void);
static int test_target_metadata(void);

static int
test_app_fault_forces_ptt_off(void)
{
	struct platform_stub stub;
	struct embedded_app app;
	struct embedded_app_status status;

	platform_stub_init(&stub);
	if (embedded_app_init(&app, platform_stub_platform(&stub)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	stub.ptt_state = 1;
	app.status.ptt_state = 1;

	if (embedded_app_fault(&app) != EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_app_status(&app, &status) != EMBEDDED_APP_OK)
		return __LINE__;
	if (status.faulted != 1)
		return __LINE__;
	if (status.ptt_state != 0)
		return __LINE__;
	if (stub.ptt_state != 0)
		return __LINE__;
	if (embedded_app_step(&app) != EMBEDDED_APP_ERR_FAULT)
		return __LINE__;

	return 0;
}

static int
test_app_init_forces_ptt_off(void)
{
	struct platform_stub stub;
	struct embedded_app app;
	struct embedded_app_status status;

	platform_stub_init(&stub);
	stub.ptt_state = 1;
	if (embedded_app_init(&app, platform_stub_platform(&stub)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_app_status(&app, &status) != EMBEDDED_APP_OK)
		return __LINE__;
	if (status.ptt_state != 0)
		return __LINE__;
	if (stub.ptt_state != 0)
		return __LINE__;

	return 0;
}

static int
test_app_null_args(void)
{
	struct platform_stub stub;
	struct embedded_app app;
	struct embedded_app_status status;

	platform_stub_init(&stub);
	if (embedded_app_init(NULL, platform_stub_platform(&stub)) !=
	    EMBEDDED_APP_ERR_ARG)
		return __LINE__;
	if (embedded_app_init(&app, NULL) != EMBEDDED_APP_ERR_ARG)
		return __LINE__;
	if (embedded_app_step(NULL) != EMBEDDED_APP_ERR_ARG)
		return __LINE__;
	if (embedded_app_status(NULL, &status) != EMBEDDED_APP_ERR_ARG)
		return __LINE__;
	if (embedded_app_status(&app, NULL) != EMBEDDED_APP_ERR_ARG)
		return __LINE__;
	if (embedded_app_shutdown(NULL) != EMBEDDED_APP_ERR_ARG)
		return __LINE__;
	if (embedded_app_fault(NULL) != EMBEDDED_APP_ERR_ARG)
		return __LINE__;

	return 0;
}

static int
test_app_shutdown_forces_ptt_off(void)
{
	struct platform_stub stub;
	struct embedded_app app;
	struct embedded_app_status status;

	platform_stub_init(&stub);
	if (embedded_app_init(&app, platform_stub_platform(&stub)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	stub.ptt_state = 1;
	app.status.ptt_state = 1;

	if (embedded_app_shutdown(&app) != EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_app_status(&app, &status) != EMBEDDED_APP_OK)
		return __LINE__;
	if (status.shutdown_requested != 1)
		return __LINE__;
	if (status.ptt_state != 0)
		return __LINE__;
	if (stub.ptt_state != 0)
		return __LINE__;

	return 0;
}

static int
test_app_step_ticks_and_kicks_watchdog(void)
{
	struct platform_stub stub;
	struct embedded_app app;
	struct embedded_app_status status;

	platform_stub_init(&stub);
	if (embedded_app_init(&app, platform_stub_platform(&stub)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_app_step(&app) != EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_app_status(&app, &status) != EMBEDDED_APP_OK)
		return __LINE__;
	if (status.tick_ms != 10u)
		return __LINE__;
	if (status.steps != 1u)
		return __LINE__;
	if (status.watchdog_kicks != 1u)
		return __LINE__;
	if (stub.tick_ms != 10u)
		return __LINE__;
	if (stub.watchdog_kicks != 1u)
		return __LINE__;

	return 0;
}

static int
test_stub_diag_write(void)
{
	struct platform_stub stub;
	const struct kilotnc_platform *platform;

	platform_stub_init(&stub);
	platform = platform_stub_platform(&stub);
	if (platform == NULL)
		return __LINE__;
	if (platform->diag_write(platform->ctx, "embedded diag") !=
	    KILOTNC_PLATFORM_OK)
		return __LINE__;
	if (stub.diag_writes != 1u)
		return __LINE__;
	if (strcmp(stub.diag_last, "embedded diag") != 0)
		return __LINE__;
	if (platform->usb_poll(platform->ctx) !=
	    KILOTNC_PLATFORM_ERR_UNSUPPORTED)
		return __LINE__;
	if (platform->audio_poll(platform->ctx) !=
	    KILOTNC_PLATFORM_ERR_UNSUPPORTED)
		return __LINE__;

	return 0;
}

static int
test_target_metadata(void)
{
	uint32_t features;

	features = KILOTNC_TARGET_FEATURES;
	if (strcmp(KILOTNC_EMBEDDED_STAGE, "M2.1 compile-only skeleton") !=
	    0)
		return __LINE__;
	if (strcmp(KILOTNC_TARGET_NAME, "stm32h753-nucleo") != 0)
		return __LINE__;
	if (strcmp(KILOTNC_TARGET_MCU_FAMILY, "STM32H753") != 0)
		return __LINE__;
	if ((features & KILOTNC_BOARD_FEATURE_USB_DEVICE_PLANNED) == 0)
		return __LINE__;
	if ((features & KILOTNC_BOARD_FEATURE_WATCHDOG_PLANNED) == 0)
		return __LINE__;
	if ((features & KILOTNC_BOARD_FEATURE_SAI_I2S_PLANNED) == 0)
		return __LINE__;
	if ((features & KILOTNC_BOARD_FEATURE_GPIO_TEST_PTT_PLANNED) == 0)
		return __LINE__;

	return 0;
}

int
main(void)
{
	int line;

	line = test_app_init_forces_ptt_off();
	if (line != 0)
		goto fail;
	line = test_app_step_ticks_and_kicks_watchdog();
	if (line != 0)
		goto fail;
	line = test_app_shutdown_forces_ptt_off();
	if (line != 0)
		goto fail;
	line = test_app_fault_forces_ptt_off();
	if (line != 0)
		goto fail;
	line = test_app_null_args();
	if (line != 0)
		goto fail;
	line = test_stub_diag_write();
	if (line != 0)
		goto fail;
	line = test_target_metadata();
	if (line != 0)
		goto fail;

	(void)printf("ok embedded_app\n");
	return 0;

fail:
	(void)printf("not ok embedded_app line %d\n", line);
	return 1;
}
