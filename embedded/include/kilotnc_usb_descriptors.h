/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/include/kilotnc_usb_descriptors.h */

#ifndef KILOTNC_USB_DESCRIPTORS_H
#define KILOTNC_USB_DESCRIPTORS_H

#include <sys/types.h>

#include <stdint.h>

enum kilotnc_usb_desc_result {
	KILOTNC_USB_DESC_OK = 0,
	KILOTNC_USB_DESC_ERR_ARG,
	KILOTNC_USB_DESC_ERR_SMALL,
	KILOTNC_USB_DESC_ERR_UNSUPPORTED
};

enum kilotnc_usb_desc_profile {
	KILOTNC_USB_DESC_PROFILE_KISS_ONLY = 0,
	KILOTNC_USB_DESC_PROFILE_KISS_PLUS_DIAG
};

struct kilotnc_usb_desc_plan {
	enum kilotnc_usb_desc_profile profile;
	uint8_t cdc_interfaces;
	uint8_t data_endpoints;
	uint8_t notification_endpoints;
	uint8_t has_final_vid_pid;
	uint16_t vid;
	uint16_t pid;
	uint16_t control_packet_size;
	uint16_t data_packet_size;
	uint16_t notification_packet_size;
	const char *manufacturer;
	const char *product;
	const char *configuration;
	const char *serial;
	const char *stack_target;
};

enum kilotnc_usb_desc_result kilotnc_usb_desc_default_plan(
	struct kilotnc_usb_desc_plan *);
enum kilotnc_usb_desc_result kilotnc_usb_desc_diag_plan(
	struct kilotnc_usb_desc_plan *);
enum kilotnc_usb_desc_result kilotnc_usb_desc_format(
	const struct kilotnc_usb_desc_plan *, char *, size_t, size_t *);
enum kilotnc_usb_desc_result kilotnc_usb_desc_profile_format(
	enum kilotnc_usb_desc_profile, char *, size_t);
enum kilotnc_usb_desc_result kilotnc_usb_desc_profile_parse(const char *,
	enum kilotnc_usb_desc_profile *);
enum kilotnc_usb_desc_result kilotnc_usb_desc_validate(
	const struct kilotnc_usb_desc_plan *);

#endif
