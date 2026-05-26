/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/app/embedded_usb_bridge.h */

#ifndef EMBEDDED_USB_BRIDGE_H
#define EMBEDDED_USB_BRIDGE_H

#include <sys/types.h>

#include <stdint.h>

#include "kilotnc_usb_cdc.h"
#include "kiss.h"

enum embedded_usb_bridge_mode {
	EMBEDDED_USB_BRIDGE_ECHO = 0,
	EMBEDDED_USB_BRIDGE_KISS_LOOPBACK
};

enum embedded_usb_bridge_result {
	EMBEDDED_USB_BRIDGE_OK = 0,
	EMBEDDED_USB_BRIDGE_ERR_ARG,
	EMBEDDED_USB_BRIDGE_ERR_USB,
	EMBEDDED_USB_BRIDGE_ERR_SMALL
};

struct embedded_usb_bridge_stats {
	size_t bytes_in;
	size_t bytes_out;
	size_t echo_chunks;
	size_t kiss_frames_in;
	size_t kiss_frames_out;
	size_t kiss_parse_errors;
	size_t kiss_overlength;
	size_t kiss_ignored_commands;
	size_t usb_would_block;
	size_t usb_errors;
};

struct embedded_usb_bridge {
	const struct kilotnc_usb_cdc *usb;
	enum embedded_usb_bridge_mode mode;
	struct kiss_parser parser;
	struct embedded_usb_bridge_stats stats;
};

enum embedded_usb_bridge_result embedded_usb_bridge_init(
	struct embedded_usb_bridge *, const struct kilotnc_usb_cdc *,
	enum embedded_usb_bridge_mode);
enum embedded_usb_bridge_result embedded_usb_bridge_service(
	struct embedded_usb_bridge *);
enum embedded_usb_bridge_result embedded_usb_bridge_stats(
	const struct embedded_usb_bridge *, struct embedded_usb_bridge_stats *);

#endif
