/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/tests/test_usb_descriptor_plan.c */

#include <sys/types.h>

#include <stdio.h>
#include <string.h>

#include "usb_descriptor_plan.h"

static int test_usb_desc_default_plan(void);
static int test_usb_desc_diag_plan(void);
static int test_usb_desc_format(void);
static int test_usb_desc_null_args(void);
static int test_usb_desc_parse_format(void);
static int test_usb_desc_validate(void);

static int
test_usb_desc_default_plan(void)
{
	struct kilotnc_usb_desc_plan plan;

	if (kilotnc_usb_desc_default_plan(&plan) != KILOTNC_USB_DESC_OK)
		return __LINE__;
	if (plan.profile != KILOTNC_USB_DESC_PROFILE_KISS_ONLY)
		return __LINE__;
	if (plan.cdc_interfaces != 1U)
		return __LINE__;
	if (plan.data_endpoints != 2U)
		return __LINE__;
	if (plan.notification_endpoints != 1U)
		return __LINE__;
	if (plan.has_final_vid_pid != 0U)
		return __LINE__;
	if (plan.vid != 0U || plan.pid != 0U)
		return __LINE__;
	if (strcmp(plan.stack_target, "stub") != 0)
		return __LINE__;

	return 0;
}

static int
test_usb_desc_diag_plan(void)
{
	struct kilotnc_usb_desc_plan plan;

	if (kilotnc_usb_desc_diag_plan(&plan) != KILOTNC_USB_DESC_OK)
		return __LINE__;
	if (plan.profile != KILOTNC_USB_DESC_PROFILE_KISS_PLUS_DIAG)
		return __LINE__;
	if (plan.cdc_interfaces != 2U)
		return __LINE__;
	if (plan.data_endpoints != 4U)
		return __LINE__;
	if (plan.notification_endpoints != 2U)
		return __LINE__;
	if (plan.has_final_vid_pid != 0U)
		return __LINE__;

	return 0;
}

static int
test_usb_desc_format(void)
{
	struct kilotnc_usb_desc_plan plan;
	char buf[256];
	size_t used;

	if (kilotnc_usb_desc_default_plan(&plan) != KILOTNC_USB_DESC_OK)
		return __LINE__;
	if (kilotnc_usb_desc_format(&plan, buf, sizeof(buf), &used) !=
	    KILOTNC_USB_DESC_OK)
		return __LINE__;
	if (used == 0U)
		return __LINE__;
	if (strstr(buf, "profile=kiss-only") == NULL)
		return __LINE__;
	if (strstr(buf, "final_vid_pid=0") == NULL)
		return __LINE__;
	if (kilotnc_usb_desc_format(&plan, buf, 8U, &used) !=
	    KILOTNC_USB_DESC_ERR_SMALL)
		return __LINE__;

	return 0;
}

static int
test_usb_desc_null_args(void)
{
	struct kilotnc_usb_desc_plan plan;
	enum kilotnc_usb_desc_profile profile;
	char buf[32];
	size_t used;

	if (kilotnc_usb_desc_default_plan(NULL) != KILOTNC_USB_DESC_ERR_ARG)
		return __LINE__;
	if (kilotnc_usb_desc_diag_plan(NULL) != KILOTNC_USB_DESC_ERR_ARG)
		return __LINE__;
	if (kilotnc_usb_desc_format(NULL, buf, sizeof(buf), &used) !=
	    KILOTNC_USB_DESC_ERR_ARG)
		return __LINE__;
	if (kilotnc_usb_desc_default_plan(&plan) != KILOTNC_USB_DESC_OK)
		return __LINE__;
	if (kilotnc_usb_desc_format(&plan, NULL, sizeof(buf), &used) !=
	    KILOTNC_USB_DESC_ERR_ARG)
		return __LINE__;
	if (kilotnc_usb_desc_format(&plan, buf, 0U, &used) !=
	    KILOTNC_USB_DESC_ERR_ARG)
		return __LINE__;
	if (kilotnc_usb_desc_format(&plan, buf, sizeof(buf), NULL) !=
	    KILOTNC_USB_DESC_ERR_ARG)
		return __LINE__;
	if (kilotnc_usb_desc_profile_parse(NULL, &profile) !=
	    KILOTNC_USB_DESC_ERR_ARG)
		return __LINE__;
	if (kilotnc_usb_desc_profile_parse("kiss-only", NULL) !=
	    KILOTNC_USB_DESC_ERR_ARG)
		return __LINE__;
	if (kilotnc_usb_desc_profile_format(
	    KILOTNC_USB_DESC_PROFILE_KISS_ONLY, NULL, sizeof(buf)) !=
	    KILOTNC_USB_DESC_ERR_ARG)
		return __LINE__;
	if (kilotnc_usb_desc_validate(NULL) != KILOTNC_USB_DESC_ERR_ARG)
		return __LINE__;

	return 0;
}

static int
test_usb_desc_parse_format(void)
{
	enum kilotnc_usb_desc_profile profile;
	char buf[16];

	if (kilotnc_usb_desc_profile_parse("kiss-only", &profile) !=
	    KILOTNC_USB_DESC_OK)
		return __LINE__;
	if (profile != KILOTNC_USB_DESC_PROFILE_KISS_ONLY)
		return __LINE__;
	if (kilotnc_usb_desc_profile_parse("kiss-plus-diag", &profile) !=
	    KILOTNC_USB_DESC_OK)
		return __LINE__;
	if (profile != KILOTNC_USB_DESC_PROFILE_KISS_PLUS_DIAG)
		return __LINE__;
	if (kilotnc_usb_desc_profile_parse("bad", &profile) !=
	    KILOTNC_USB_DESC_ERR_UNSUPPORTED)
		return __LINE__;
	if (kilotnc_usb_desc_profile_format(
	    KILOTNC_USB_DESC_PROFILE_KISS_ONLY, buf, sizeof(buf)) !=
	    KILOTNC_USB_DESC_OK)
		return __LINE__;
	if (strcmp(buf, "kiss-only") != 0)
		return __LINE__;
	if (kilotnc_usb_desc_profile_format(
	    KILOTNC_USB_DESC_PROFILE_KISS_PLUS_DIAG, buf, sizeof(buf)) !=
	    KILOTNC_USB_DESC_OK)
		return __LINE__;
	if (strcmp(buf, "kiss-plus-diag") != 0)
		return __LINE__;
	if (kilotnc_usb_desc_profile_format(
	    KILOTNC_USB_DESC_PROFILE_KISS_PLUS_DIAG, buf, 4U) !=
	    KILOTNC_USB_DESC_ERR_SMALL)
		return __LINE__;

	return 0;
}

static int
test_usb_desc_validate(void)
{
	struct kilotnc_usb_desc_plan plan;

	if (kilotnc_usb_desc_default_plan(&plan) != KILOTNC_USB_DESC_OK)
		return __LINE__;
	if (kilotnc_usb_desc_validate(&plan) != KILOTNC_USB_DESC_OK)
		return __LINE__;
	plan.has_final_vid_pid = 1U;
	if (kilotnc_usb_desc_validate(&plan) !=
	    KILOTNC_USB_DESC_ERR_UNSUPPORTED)
		return __LINE__;
	if (kilotnc_usb_desc_default_plan(&plan) != KILOTNC_USB_DESC_OK)
		return __LINE__;
	plan.manufacturer = NULL;
	if (kilotnc_usb_desc_validate(&plan) != KILOTNC_USB_DESC_ERR_ARG)
		return __LINE__;
	if (kilotnc_usb_desc_default_plan(&plan) != KILOTNC_USB_DESC_OK)
		return __LINE__;
	plan.cdc_interfaces = 2U;
	if (kilotnc_usb_desc_validate(&plan) !=
	    KILOTNC_USB_DESC_ERR_UNSUPPORTED)
		return __LINE__;

	return 0;
}

int
test_usb_descriptor_plan(void)
{
	int line;

	line = test_usb_desc_default_plan();
	if (line != 0)
		goto fail;
	line = test_usb_desc_diag_plan();
	if (line != 0)
		goto fail;
	line = test_usb_desc_parse_format();
	if (line != 0)
		goto fail;
	line = test_usb_desc_format();
	if (line != 0)
		goto fail;
	line = test_usb_desc_validate();
	if (line != 0)
		goto fail;
	line = test_usb_desc_null_args();
	if (line != 0)
		goto fail;

	(void)printf("ok usb_descriptor_plan\n");
	return 0;

fail:
	(void)printf("not ok usb_descriptor_plan line %d\n", line);
	return 1;
}
