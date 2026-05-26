/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/tests/test_usb_stack_boundary.c */

#include <sys/types.h>

#include <stdio.h>
#include <string.h>

#include "usb_stack_boundary.h"

static int test_usb_stack_format(void);
static int test_usb_stack_null_args(void);
static int test_usb_stack_parse(void);
static int test_usb_stack_support(void);

static int
test_usb_stack_format(void)
{
	char buf[16];

	if (kilotnc_usb_stack_format(KILOTNC_USB_STACK_STUB, buf,
	    sizeof(buf)) != KILOTNC_USB_STACK_OK)
		return __LINE__;
	if (strcmp(buf, "stub") != 0)
		return __LINE__;
	if (kilotnc_usb_stack_format(KILOTNC_USB_STACK_TINYUSB, buf,
	    sizeof(buf)) != KILOTNC_USB_STACK_OK)
		return __LINE__;
	if (strcmp(buf, "tinyusb") != 0)
		return __LINE__;
	if (kilotnc_usb_stack_format(KILOTNC_USB_STACK_STM32CUBE, buf,
	    sizeof(buf)) != KILOTNC_USB_STACK_OK)
		return __LINE__;
	if (strcmp(buf, "stm32cube") != 0)
		return __LINE__;
	if (kilotnc_usb_stack_format(KILOTNC_USB_STACK_CUSTOM, buf,
	    sizeof(buf)) != KILOTNC_USB_STACK_OK)
		return __LINE__;
	if (strcmp(buf, "custom") != 0)
		return __LINE__;
	if (kilotnc_usb_stack_format(KILOTNC_USB_STACK_TINYUSB, buf,
	    4U) != KILOTNC_USB_STACK_ERR_SMALL)
		return __LINE__;

	return 0;
}

static int
test_usb_stack_null_args(void)
{
	enum kilotnc_usb_stack stack;
	char buf[8];

	if (kilotnc_usb_stack_parse(NULL, &stack) !=
	    KILOTNC_USB_STACK_ERR_ARG)
		return __LINE__;
	if (kilotnc_usb_stack_parse("stub", NULL) !=
	    KILOTNC_USB_STACK_ERR_ARG)
		return __LINE__;
	if (kilotnc_usb_stack_format(KILOTNC_USB_STACK_STUB, NULL,
	    sizeof(buf)) != KILOTNC_USB_STACK_ERR_ARG)
		return __LINE__;
	if (kilotnc_usb_stack_format(KILOTNC_USB_STACK_STUB, buf, 0U) !=
	    KILOTNC_USB_STACK_ERR_ARG)
		return __LINE__;
	if (kilotnc_usb_stack_format((enum kilotnc_usb_stack)99, buf,
	    sizeof(buf)) != KILOTNC_USB_STACK_ERR_ARG)
		return __LINE__;
	if (kilotnc_usb_stack_support((enum kilotnc_usb_stack)99) !=
	    KILOTNC_USB_STACK_ERR_ARG)
		return __LINE__;

	return 0;
}

static int
test_usb_stack_parse(void)
{
	enum kilotnc_usb_stack stack;

	if (kilotnc_usb_stack_parse("stub", &stack) !=
	    KILOTNC_USB_STACK_OK)
		return __LINE__;
	if (stack != KILOTNC_USB_STACK_STUB)
		return __LINE__;
	if (kilotnc_usb_stack_parse("tinyusb", &stack) !=
	    KILOTNC_USB_STACK_OK)
		return __LINE__;
	if (stack != KILOTNC_USB_STACK_TINYUSB)
		return __LINE__;
	if (kilotnc_usb_stack_parse("stm32cube", &stack) !=
	    KILOTNC_USB_STACK_OK)
		return __LINE__;
	if (stack != KILOTNC_USB_STACK_STM32CUBE)
		return __LINE__;
	if (kilotnc_usb_stack_parse("custom", &stack) !=
	    KILOTNC_USB_STACK_OK)
		return __LINE__;
	if (stack != KILOTNC_USB_STACK_CUSTOM)
		return __LINE__;
	if (kilotnc_usb_stack_parse("bad", &stack) !=
	    KILOTNC_USB_STACK_ERR_UNSUPPORTED)
		return __LINE__;

	return 0;
}

static int
test_usb_stack_support(void)
{
	if (kilotnc_usb_stack_support(KILOTNC_USB_STACK_STUB) !=
	    KILOTNC_USB_STACK_OK)
		return __LINE__;
	if (kilotnc_usb_stack_support(KILOTNC_USB_STACK_TINYUSB) !=
	    KILOTNC_USB_STACK_ERR_UNSUPPORTED)
		return __LINE__;
	if (kilotnc_usb_stack_support(KILOTNC_USB_STACK_STM32CUBE) !=
	    KILOTNC_USB_STACK_ERR_UNSUPPORTED)
		return __LINE__;
	if (kilotnc_usb_stack_support(KILOTNC_USB_STACK_CUSTOM) !=
	    KILOTNC_USB_STACK_ERR_UNSUPPORTED)
		return __LINE__;

	return 0;
}

int
test_usb_stack_boundary(void)
{
	int line;

	line = test_usb_stack_parse();
	if (line != 0)
		goto fail;
	line = test_usb_stack_support();
	if (line != 0)
		goto fail;
	line = test_usb_stack_format();
	if (line != 0)
		goto fail;
	line = test_usb_stack_null_args();
	if (line != 0)
		goto fail;

	(void)printf("ok usb_stack_boundary\n");
	return 0;

fail:
	(void)printf("not ok usb_stack_boundary line %d\n", line);
	return 1;
}
