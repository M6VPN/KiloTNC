/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/tests/test_target_metadata.c */

#include <sys/types.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "kilotnc_board.h"
#include "target.h"
#include "target_config.h"
#include "target_resources.h"

static int test_target_config_absent_assignments(void);
static int test_target_config_absent_features(void);
static int test_target_config_feature_flags(void);
static int test_target_resource_flags(void);
static int test_target_config_strings(void);
static int test_target_resource_strings(void);

static int
test_target_config_absent_assignments(void)
{
	if (KILOTNC_STM32H753_PIN_ASSIGNMENTS_VERIFIED != 0)
		return __LINE__;
	if (KILOTNC_STM32H753_USB_PIN_ASSIGNMENTS_VERIFIED != 0)
		return __LINE__;
	if (KILOTNC_STM32H753_PTT_PIN_ASSIGNMENTS_VERIFIED != 0)
		return __LINE__;
	if (KILOTNC_STM32H753_AUDIO_PIN_ASSIGNMENTS_VERIFIED != 0)
		return __LINE__;

	return 0;
}

static int
test_target_config_absent_features(void)
{
	if (KILOTNC_TARGET_STM32H753_NUCLEO_HAS_LINKER_SCRIPT != 0)
		return __LINE__;
	if (KILOTNC_TARGET_STM32H753_NUCLEO_HAS_STARTUP != 0)
		return __LINE__;
	if (KILOTNC_TARGET_STM32H753_NUCLEO_HAS_HAL != 0)
		return __LINE__;
	if (KILOTNC_TARGET_STM32H753_NUCLEO_HAS_USB_STACK != 0)
		return __LINE__;
	if (KILOTNC_TARGET_STM32H753_NUCLEO_HAS_PINOUT != 0)
		return __LINE__;
	if (KILOTNC_TARGET_STM32H753_NUCLEO_HAS_FLASH_IMAGE != 0)
		return __LINE__;

	return 0;
}

static int
test_target_config_feature_flags(void)
{
	uint32_t features;

	features = KILOTNC_TARGET_FEATURES;
	if ((features & KILOTNC_BOARD_FEATURE_USB_DEVICE_PLANNED) == 0U)
		return __LINE__;
	if ((features & KILOTNC_BOARD_FEATURE_WATCHDOG_PLANNED) == 0U)
		return __LINE__;
	if ((features & KILOTNC_BOARD_FEATURE_SAI_I2S_PLANNED) == 0U)
		return __LINE__;
	if ((features & KILOTNC_BOARD_FEATURE_GPIO_TEST_PTT_PLANNED) == 0U)
		return __LINE__;

	return 0;
}

static int
test_target_resource_flags(void)
{
	if (KILOTNC_STM32H753_USB_DEVICE_PLANNED != 1)
		return __LINE__;
	if (KILOTNC_STM32H753_TEST_PTT_GPIO_PLANNED != 1)
		return __LINE__;
	if (KILOTNC_STM32H753_AUDIO_SAI_PLANNED != 1)
		return __LINE__;
	if (KILOTNC_STM32H753_AUDIO_I2S_PLANNED != 1)
		return __LINE__;
	if (KILOTNC_STM32H753_DIAGNOSTIC_LED_PLANNED != 1)
		return __LINE__;
	if (KILOTNC_STM32H753_WATCHDOG_PLANNED != 1)
		return __LINE__;
	if (KILOTNC_STM32H753_RESET_CAUSE_PLANNED != 1)
		return __LINE__;

	return 0;
}

static int
test_target_config_strings(void)
{
	if (strcmp(KILOTNC_TARGET_NAME, "stm32h753-nucleo") != 0)
		return __LINE__;
	if (strcmp(KILOTNC_TARGET_MCU_FAMILY, "STM32H753") != 0)
		return __LINE__;
	if (strcmp(KILOTNC_TARGET_BOARD_CLASS,
	    "NUCLEO-H753ZI Nucleo-144") != 0)
		return __LINE__;

	return 0;
}

static int
test_target_resource_strings(void)
{
	if (strcmp(KILOTNC_STM32H753_RESOURCE_TARGET_NAME,
	    "stm32h753-nucleo") != 0)
		return __LINE__;
	if (strcmp(KILOTNC_STM32H753_TEST_PTT_GPIO_PORT, "TBD") != 0)
		return __LINE__;
	if (strcmp(KILOTNC_STM32H753_TEST_PTT_GPIO_PIN, "TBD") != 0)
		return __LINE__;
	if (strcmp(KILOTNC_STM32H753_AUDIO_RESOURCE, "TBD") != 0)
		return __LINE__;
	if (strcmp(KILOTNC_STM32H753_USB_RESOURCE, "TBD") != 0)
		return __LINE__;

	return 0;
}

int
test_target_metadata(void)
{
	int line;

	line = test_target_config_strings();
	if (line != 0)
		goto fail;
	line = test_target_config_feature_flags();
	if (line != 0)
		goto fail;
	line = test_target_config_absent_features();
	if (line != 0)
		goto fail;
	line = test_target_resource_flags();
	if (line != 0)
		goto fail;
	line = test_target_config_absent_assignments();
	if (line != 0)
		goto fail;
	line = test_target_resource_strings();
	if (line != 0)
		goto fail;

	(void)printf("ok target_metadata\n");
	return 0;

fail:
	(void)printf("not ok target_metadata line %d\n", line);
	return 1;
}
