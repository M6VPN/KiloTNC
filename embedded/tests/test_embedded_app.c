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
static int test_app_watchdog_fault_forces_ptt_off(void);
static int test_stub_diag_write(void);
static int test_stub_null_args(void);
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
	if (status.state != EMBEDDED_APP_FAULT)
		return __LINE__;
	if (status.faulted != 1)
		return __LINE__;
	if (status.ptt_state != KILOTNC_GPIO_LOW)
		return __LINE__;
	if (stub.ptt_state != KILOTNC_GPIO_LOW)
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
	if (status.state != EMBEDDED_APP_RUNNING)
		return __LINE__;
	if (status.ptt_state != KILOTNC_GPIO_LOW)
		return __LINE__;
	if (status.reset_cause != KILOTNC_RESET_POWER_ON)
		return __LINE__;
	if (stub.ptt_state != KILOTNC_GPIO_LOW)
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
	if (embedded_app_audio_bridge(NULL, NULL) != EMBEDDED_APP_ERR_ARG)
		return __LINE__;
	if (embedded_app_usb_bridge(NULL, NULL) != EMBEDDED_APP_ERR_ARG)
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
	if (status.state != EMBEDDED_APP_STOPPED)
		return __LINE__;
	if (status.ptt_state != KILOTNC_GPIO_LOW)
		return __LINE__;
	if (stub.ptt_state != KILOTNC_GPIO_LOW)
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
	if (embedded_app_step(&app) != EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_app_status(&app, &status) != EMBEDDED_APP_OK)
		return __LINE__;
	if (status.tick_ms != 20u)
		return __LINE__;
	if (status.control_ticks_10ms != 2u)
		return __LINE__;
	if (status.steps != 2u)
		return __LINE__;
	if (status.watchdog_kicks != 2u)
		return __LINE__;
	if (stub.monotonic_ms != 20u)
		return __LINE__;
	if (stub.control_ticks_10ms != 2u)
		return __LINE__;
	if (stub.watchdog_kicks != 2u)
		return __LINE__;

	return 0;
}

static int
test_app_watchdog_fault_forces_ptt_off(void)
{
	struct platform_stub stub;
	struct embedded_app app;
	struct embedded_app_status status;
	const struct kilotnc_platform *platform;

	platform_stub_init(&stub);
	platform = platform_stub_platform(&stub);
	if (embedded_app_init(&app, platform) != EMBEDDED_APP_OK)
		return __LINE__;
	if (platform->ptt_set(platform->ctx, KILOTNC_GPIO_HIGH) !=
	    KILOTNC_PLATFORM_OK)
		return __LINE__;
	if (platform_stub_simulate_watchdog_fault(&stub) !=
	    KILOTNC_PLATFORM_OK)
		return __LINE__;
	if (embedded_app_step(&app) != EMBEDDED_APP_ERR_FAULT)
		return __LINE__;
	if (embedded_app_status(&app, &status) != EMBEDDED_APP_OK)
		return __LINE__;
	if (status.state != EMBEDDED_APP_FAULT)
		return __LINE__;
	if (status.reset_cause != KILOTNC_RESET_WATCHDOG)
		return __LINE__;
	if (status.ptt_state != KILOTNC_GPIO_LOW)
		return __LINE__;
	if (status.fault_count != 1u)
		return __LINE__;
	if (stub.ptt_state != KILOTNC_GPIO_LOW)
		return __LINE__;

	return 0;
}

static int
test_stub_diag_write(void)
{
	struct platform_stub stub;
	const struct kilotnc_platform *platform;
	size_t count;

	platform_stub_init(&stub);
	platform = platform_stub_platform(&stub);
	if (platform == NULL)
		return __LINE__;
	if (platform->diag_count(platform->ctx, &count) !=
	    KILOTNC_PLATFORM_OK)
		return __LINE__;
	if (count != 0u)
		return __LINE__;
	if (platform->diag_write(platform->ctx, "embedded diag") !=
	    KILOTNC_PLATFORM_OK)
		return __LINE__;
	if (platform->diag_count(platform->ctx, &count) !=
	    KILOTNC_PLATFORM_OK)
		return __LINE__;
	if (count != 1u)
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
test_stub_null_args(void)
{
	struct platform_stub stub;
	const struct kilotnc_platform *platform;
	enum kilotnc_gpio_state ptt_state;
	enum kilotnc_reset_cause reset_cause;
	uint32_t tick;
	size_t count;
	int faulted;

	platform_stub_init(&stub);
	platform = platform_stub_platform(&stub);
	if (platform_stub_platform(NULL) != NULL)
		return __LINE__;
	if (platform_stub_advance_ms(NULL, 10u) !=
	    KILOTNC_PLATFORM_ERR_ARG)
		return __LINE__;
	if (platform_stub_ptt_state(NULL, &ptt_state) !=
	    KILOTNC_PLATFORM_ERR_ARG)
		return __LINE__;
	if (platform_stub_ptt_state(&stub, NULL) !=
	    KILOTNC_PLATFORM_ERR_ARG)
		return __LINE__;
	if (platform_stub_simulate_watchdog_fault(NULL) !=
	    KILOTNC_PLATFORM_ERR_ARG)
		return __LINE__;
	if (platform->monotonic_ms(NULL, &tick) !=
	    KILOTNC_PLATFORM_ERR_ARG)
		return __LINE__;
	if (platform->tick_10ms(platform->ctx, NULL) !=
	    KILOTNC_PLATFORM_ERR_ARG)
		return __LINE__;
	if (platform->watchdog_faulted(platform->ctx, NULL) !=
	    KILOTNC_PLATFORM_ERR_ARG)
		return __LINE__;
	if (platform->reset_cause(platform->ctx, NULL) !=
	    KILOTNC_PLATFORM_ERR_ARG)
		return __LINE__;
	if (platform->ptt_get(platform->ctx, NULL) !=
	    KILOTNC_PLATFORM_ERR_ARG)
		return __LINE__;
	if (platform->diag_count(platform->ctx, NULL) !=
	    KILOTNC_PLATFORM_ERR_ARG)
		return __LINE__;
	if (platform->fault_count(platform->ctx, NULL) !=
	    KILOTNC_PLATFORM_ERR_ARG)
		return __LINE__;
	if (platform->diag_write(platform->ctx, NULL) !=
	    KILOTNC_PLATFORM_ERR_ARG)
		return __LINE__;
	if (platform->monotonic_ms(platform->ctx, &tick) !=
	    KILOTNC_PLATFORM_OK)
		return __LINE__;
	if (platform->watchdog_faulted(platform->ctx, &faulted) !=
	    KILOTNC_PLATFORM_OK)
		return __LINE__;
	if (platform->reset_cause(platform->ctx, &reset_cause) !=
	    KILOTNC_PLATFORM_OK)
		return __LINE__;
	if (platform->fault_count(platform->ctx, &count) !=
	    KILOTNC_PLATFORM_OK)
		return __LINE__;

	return 0;
}

static int
test_target_metadata(void)
{
	uint32_t features;

	features = KILOTNC_TARGET_FEATURES;
	if (strcmp(KILOTNC_EMBEDDED_STAGE,
	    "M2.5 embedded audio loopback path") !=
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
test_embedded_app(void)
{
	int line;

	line = test_app_init_forces_ptt_off();
	if (line != 0)
		goto fail;
	line = test_app_step_ticks_and_kicks_watchdog();
	if (line != 0)
		goto fail;
	line = test_app_watchdog_fault_forces_ptt_off();
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
	line = test_stub_null_args();
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
