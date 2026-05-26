/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/include/kilotnc_usb_stack_boundary.h */

#ifndef KILOTNC_USB_STACK_BOUNDARY_H
#define KILOTNC_USB_STACK_BOUNDARY_H

#include <sys/types.h>

enum kilotnc_usb_stack {
	KILOTNC_USB_STACK_STUB = 0,
	KILOTNC_USB_STACK_TINYUSB,
	KILOTNC_USB_STACK_STM32CUBE,
	KILOTNC_USB_STACK_CUSTOM
};

enum kilotnc_usb_stack_result {
	KILOTNC_USB_STACK_OK = 0,
	KILOTNC_USB_STACK_ERR_ARG,
	KILOTNC_USB_STACK_ERR_UNSUPPORTED,
	KILOTNC_USB_STACK_ERR_SMALL
};

enum kilotnc_usb_stack_result kilotnc_usb_stack_format(
	enum kilotnc_usb_stack, char *, size_t);
enum kilotnc_usb_stack_result kilotnc_usb_stack_parse(const char *,
	enum kilotnc_usb_stack *);
enum kilotnc_usb_stack_result kilotnc_usb_stack_support(
	enum kilotnc_usb_stack);

#endif
