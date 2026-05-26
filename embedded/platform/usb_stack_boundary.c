/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/platform/usb_stack_boundary.c */

#include <sys/types.h>

#include <stdio.h>
#include <string.h>

#include "usb_stack_boundary.h"

static const char *usb_stack_name(enum kilotnc_usb_stack);

static const char *
usb_stack_name(enum kilotnc_usb_stack stack)
{
	switch (stack) {
	case KILOTNC_USB_STACK_STUB:
		return "stub";
	case KILOTNC_USB_STACK_TINYUSB:
		return "tinyusb";
	case KILOTNC_USB_STACK_STM32CUBE:
		return "stm32cube";
	case KILOTNC_USB_STACK_CUSTOM:
		return "custom";
	default:
		return NULL;
	}
}

enum kilotnc_usb_stack_result
kilotnc_usb_stack_format(enum kilotnc_usb_stack stack, char *buf,
    size_t bufsiz)
{
	const char *name;
	int n;

	if (buf == NULL || bufsiz == 0U)
		return KILOTNC_USB_STACK_ERR_ARG;

	name = usb_stack_name(stack);
	if (name == NULL)
		return KILOTNC_USB_STACK_ERR_ARG;

	n = snprintf(buf, bufsiz, "%s", name);
	if (n < 0 || (size_t)n >= bufsiz)
		return KILOTNC_USB_STACK_ERR_SMALL;

	return KILOTNC_USB_STACK_OK;
}

enum kilotnc_usb_stack_result
kilotnc_usb_stack_parse(const char *name, enum kilotnc_usb_stack *stack)
{
	if (name == NULL || stack == NULL)
		return KILOTNC_USB_STACK_ERR_ARG;

	if (strcmp(name, "stub") == 0) {
		*stack = KILOTNC_USB_STACK_STUB;
		return KILOTNC_USB_STACK_OK;
	}
	if (strcmp(name, "tinyusb") == 0) {
		*stack = KILOTNC_USB_STACK_TINYUSB;
		return KILOTNC_USB_STACK_OK;
	}
	if (strcmp(name, "stm32cube") == 0) {
		*stack = KILOTNC_USB_STACK_STM32CUBE;
		return KILOTNC_USB_STACK_OK;
	}
	if (strcmp(name, "custom") == 0) {
		*stack = KILOTNC_USB_STACK_CUSTOM;
		return KILOTNC_USB_STACK_OK;
	}

	return KILOTNC_USB_STACK_ERR_UNSUPPORTED;
}

enum kilotnc_usb_stack_result
kilotnc_usb_stack_support(enum kilotnc_usb_stack stack)
{
	switch (stack) {
	case KILOTNC_USB_STACK_STUB:
		return KILOTNC_USB_STACK_OK;
	case KILOTNC_USB_STACK_TINYUSB:
	case KILOTNC_USB_STACK_STM32CUBE:
	case KILOTNC_USB_STACK_CUSTOM:
		return KILOTNC_USB_STACK_ERR_UNSUPPORTED;
	default:
		return KILOTNC_USB_STACK_ERR_ARG;
	}
}
