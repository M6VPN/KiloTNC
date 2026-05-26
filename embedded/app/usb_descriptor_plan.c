/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/app/usb_descriptor_plan.c */

#include <sys/types.h>

#include <stdio.h>
#include <string.h>

#include "usb_descriptor_plan.h"

#define USB_DESC_PACKET_SIZE_TBD 0U

static enum kilotnc_usb_desc_result usb_desc_fill(
	struct kilotnc_usb_desc_plan *, enum kilotnc_usb_desc_profile);
static const char *usb_desc_profile_name(enum kilotnc_usb_desc_profile);

static enum kilotnc_usb_desc_result
usb_desc_fill(struct kilotnc_usb_desc_plan *plan,
    enum kilotnc_usb_desc_profile profile)
{
	if (plan == NULL)
		return KILOTNC_USB_DESC_ERR_ARG;

	(void)memset(plan, 0, sizeof(*plan));
	plan->profile = profile;
	plan->has_final_vid_pid = 0U;
	plan->vid = 0U;
	plan->pid = 0U;
	plan->control_packet_size = USB_DESC_PACKET_SIZE_TBD;
	plan->data_packet_size = USB_DESC_PACKET_SIZE_TBD;
	plan->notification_packet_size = USB_DESC_PACKET_SIZE_TBD;
	plan->manufacturer = "M6VPN";
	plan->product = "KiloTNC USB CDC KISS";
	plan->configuration = "KiloTNC configuration";
	plan->serial = "TBD";
	plan->stack_target = "stub";

	switch (profile) {
	case KILOTNC_USB_DESC_PROFILE_KISS_ONLY:
		plan->cdc_interfaces = 1U;
		plan->data_endpoints = 2U;
		plan->notification_endpoints = 1U;
		return KILOTNC_USB_DESC_OK;
	case KILOTNC_USB_DESC_PROFILE_KISS_PLUS_DIAG:
		plan->cdc_interfaces = 2U;
		plan->data_endpoints = 4U;
		plan->notification_endpoints = 2U;
		return KILOTNC_USB_DESC_OK;
	default:
		return KILOTNC_USB_DESC_ERR_UNSUPPORTED;
	}
}

static const char *
usb_desc_profile_name(enum kilotnc_usb_desc_profile profile)
{
	switch (profile) {
	case KILOTNC_USB_DESC_PROFILE_KISS_ONLY:
		return "kiss-only";
	case KILOTNC_USB_DESC_PROFILE_KISS_PLUS_DIAG:
		return "kiss-plus-diag";
	default:
		return NULL;
	}
}

enum kilotnc_usb_desc_result
kilotnc_usb_desc_default_plan(struct kilotnc_usb_desc_plan *plan)
{
	return usb_desc_fill(plan, KILOTNC_USB_DESC_PROFILE_KISS_ONLY);
}

enum kilotnc_usb_desc_result
kilotnc_usb_desc_diag_plan(struct kilotnc_usb_desc_plan *plan)
{
	return usb_desc_fill(plan, KILOTNC_USB_DESC_PROFILE_KISS_PLUS_DIAG);
}

enum kilotnc_usb_desc_result
kilotnc_usb_desc_format(const struct kilotnc_usb_desc_plan *plan, char *buf,
    size_t bufsiz, size_t *used)
{
	const char *profile_name;
	int n;

	if (plan == NULL || buf == NULL || bufsiz == 0U || used == NULL)
		return KILOTNC_USB_DESC_ERR_ARG;
	if (kilotnc_usb_desc_validate(plan) != KILOTNC_USB_DESC_OK)
		return KILOTNC_USB_DESC_ERR_UNSUPPORTED;

	profile_name = usb_desc_profile_name(plan->profile);
	if (profile_name == NULL)
		return KILOTNC_USB_DESC_ERR_UNSUPPORTED;

	n = snprintf(buf, bufsiz,
	    "profile=%s cdc=%u data_ep=%u notify_ep=%u final_vid_pid=%u "
	    "vid=0x%04x pid=0x%04x manufacturer=\"%s\" product=\"%s\" "
	    "configuration=\"%s\" serial=\"%s\" stack=\"%s\"",
	    profile_name, (unsigned int)plan->cdc_interfaces,
	    (unsigned int)plan->data_endpoints,
	    (unsigned int)plan->notification_endpoints,
	    (unsigned int)plan->has_final_vid_pid,
	    (unsigned int)plan->vid, (unsigned int)plan->pid,
	    plan->manufacturer, plan->product, plan->configuration,
	    plan->serial, plan->stack_target);
	if (n < 0 || (size_t)n >= bufsiz)
		return KILOTNC_USB_DESC_ERR_SMALL;

	*used = (size_t)n;
	return KILOTNC_USB_DESC_OK;
}

enum kilotnc_usb_desc_result
kilotnc_usb_desc_profile_format(enum kilotnc_usb_desc_profile profile,
    char *buf, size_t bufsiz)
{
	const char *name;
	int n;

	if (buf == NULL || bufsiz == 0U)
		return KILOTNC_USB_DESC_ERR_ARG;

	name = usb_desc_profile_name(profile);
	if (name == NULL)
		return KILOTNC_USB_DESC_ERR_UNSUPPORTED;

	n = snprintf(buf, bufsiz, "%s", name);
	if (n < 0 || (size_t)n >= bufsiz)
		return KILOTNC_USB_DESC_ERR_SMALL;

	return KILOTNC_USB_DESC_OK;
}

enum kilotnc_usb_desc_result
kilotnc_usb_desc_profile_parse(const char *name,
    enum kilotnc_usb_desc_profile *profile)
{
	if (name == NULL || profile == NULL)
		return KILOTNC_USB_DESC_ERR_ARG;

	if (strcmp(name, "kiss-only") == 0) {
		*profile = KILOTNC_USB_DESC_PROFILE_KISS_ONLY;
		return KILOTNC_USB_DESC_OK;
	}
	if (strcmp(name, "kiss-plus-diag") == 0) {
		*profile = KILOTNC_USB_DESC_PROFILE_KISS_PLUS_DIAG;
		return KILOTNC_USB_DESC_OK;
	}

	return KILOTNC_USB_DESC_ERR_UNSUPPORTED;
}

enum kilotnc_usb_desc_result
kilotnc_usb_desc_validate(const struct kilotnc_usb_desc_plan *plan)
{
	if (plan == NULL)
		return KILOTNC_USB_DESC_ERR_ARG;
	if (plan->has_final_vid_pid != 0U)
		return KILOTNC_USB_DESC_ERR_UNSUPPORTED;
	if (plan->vid != 0U || plan->pid != 0U)
		return KILOTNC_USB_DESC_ERR_UNSUPPORTED;
	if (plan->manufacturer == NULL || plan->product == NULL ||
	    plan->configuration == NULL || plan->serial == NULL ||
	    plan->stack_target == NULL)
		return KILOTNC_USB_DESC_ERR_ARG;

	switch (plan->profile) {
	case KILOTNC_USB_DESC_PROFILE_KISS_ONLY:
		if (plan->cdc_interfaces != 1U ||
		    plan->data_endpoints != 2U ||
		    plan->notification_endpoints != 1U)
			return KILOTNC_USB_DESC_ERR_UNSUPPORTED;
		break;
	case KILOTNC_USB_DESC_PROFILE_KISS_PLUS_DIAG:
		if (plan->cdc_interfaces != 2U ||
		    plan->data_endpoints != 4U ||
		    plan->notification_endpoints != 2U)
			return KILOTNC_USB_DESC_ERR_UNSUPPORTED;
		break;
	default:
		return KILOTNC_USB_DESC_ERR_UNSUPPORTED;
	}

	return KILOTNC_USB_DESC_OK;
}
