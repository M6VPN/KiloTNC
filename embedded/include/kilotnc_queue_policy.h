/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/include/kilotnc_queue_policy.h */

#ifndef KILOTNC_QUEUE_POLICY_H
#define KILOTNC_QUEUE_POLICY_H

#include <sys/types.h>

#include <stdint.h>
#include <stdio.h>

#include "kilotnc_budget.h"

#define KILOTNC_QUEUE_POLICY_FINALIZED	0U

enum kilotnc_queue_policy_result {
	KILOTNC_QUEUE_POLICY_OK = 0,
	KILOTNC_QUEUE_POLICY_ERR_ARG,
	KILOTNC_QUEUE_POLICY_ERR_SMALL
};

enum kilotnc_queue_id {
	KILOTNC_QUEUE_USB_RX = 0,
	KILOTNC_QUEUE_USB_TX,
	KILOTNC_QUEUE_KISS_FRAME,
	KILOTNC_QUEUE_MODEM_TX_FRAME,
	KILOTNC_QUEUE_AUDIO_TX_SAMPLE,
	KILOTNC_QUEUE_AUDIO_RX_SAMPLE,
	KILOTNC_QUEUE_MODEM_RX_FRAME,
	KILOTNC_QUEUE_DIAG_EVENT,
	KILOTNC_QUEUE_CONTROL_EVENT,
	KILOTNC_QUEUE_COUNT
};

enum kilotnc_queue_overflow_policy {
	KILOTNC_QUEUE_DROP_NEWEST = 0,
	KILOTNC_QUEUE_DROP_OLDEST,
	KILOTNC_QUEUE_REJECT,
	KILOTNC_QUEUE_SAFETY_FAULT
};

struct kilotnc_queue_policy {
	enum kilotnc_queue_id id;
	enum kilotnc_queue_overflow_policy overflow_policy;
	const char *name;
	size_t capacity;
	uint8_t safety_critical;
};

static const struct kilotnc_queue_policy kilotnc_queue_policies[] = {
	{
		KILOTNC_QUEUE_USB_RX,
		KILOTNC_QUEUE_DROP_NEWEST,
		"usb_rx",
		KILOTNC_BUDGET_USB_RX_QUEUE_BYTES,
		0U
	},
	{
		KILOTNC_QUEUE_USB_TX,
		KILOTNC_QUEUE_DROP_NEWEST,
		"usb_tx",
		KILOTNC_BUDGET_USB_TX_QUEUE_BYTES,
		0U
	},
	{
		KILOTNC_QUEUE_KISS_FRAME,
		KILOTNC_QUEUE_REJECT,
		"kiss_frame",
		4U,
		0U
	},
	{
		KILOTNC_QUEUE_MODEM_TX_FRAME,
		KILOTNC_QUEUE_REJECT,
		"modem_tx_frame",
		2U,
		0U
	},
	{
		KILOTNC_QUEUE_AUDIO_TX_SAMPLE,
		KILOTNC_QUEUE_DROP_NEWEST,
		"audio_tx_sample",
		KILOTNC_BUDGET_AUDIO_TX_STUB_SAMPLES,
		0U
	},
	{
		KILOTNC_QUEUE_AUDIO_RX_SAMPLE,
		KILOTNC_QUEUE_DROP_NEWEST,
		"audio_rx_sample",
		KILOTNC_BUDGET_AUDIO_RX_STUB_SAMPLES,
		0U
	},
	{
		KILOTNC_QUEUE_MODEM_RX_FRAME,
		KILOTNC_QUEUE_DROP_NEWEST,
		"modem_rx_frame",
		2U,
		0U
	},
	{
		KILOTNC_QUEUE_DIAG_EVENT,
		KILOTNC_QUEUE_DROP_OLDEST,
		"diag_event",
		KILOTNC_BUDGET_DIAG_FAULT_RING_ENTRIES,
		0U
	},
	{
		KILOTNC_QUEUE_CONTROL_EVENT,
		KILOTNC_QUEUE_SAFETY_FAULT,
		"control_event",
		4U,
		1U
	}
};

static inline const struct kilotnc_queue_policy *
kilotnc_queue_policy_get(enum kilotnc_queue_id id)
{
	if (id < 0 || id >= KILOTNC_QUEUE_COUNT)
		return NULL;

	return &kilotnc_queue_policies[id];
}

static inline const char *
kilotnc_queue_overflow_name(enum kilotnc_queue_overflow_policy policy)
{
	switch (policy) {
	case KILOTNC_QUEUE_DROP_NEWEST:
		return "drop_newest";
	case KILOTNC_QUEUE_DROP_OLDEST:
		return "drop_oldest";
	case KILOTNC_QUEUE_REJECT:
		return "reject";
	case KILOTNC_QUEUE_SAFETY_FAULT:
		return "safety_fault";
	default:
		return NULL;
	}
}

static inline enum kilotnc_queue_policy_result
kilotnc_queue_policy_format(enum kilotnc_queue_id id, char *buf,
	size_t bufsiz)
{
	const struct kilotnc_queue_policy *policy;
	const char *overflow_name;
	int ret;

	if (buf == NULL || bufsiz == 0U)
		return KILOTNC_QUEUE_POLICY_ERR_ARG;

	policy = kilotnc_queue_policy_get(id);
	if (policy == NULL)
		return KILOTNC_QUEUE_POLICY_ERR_ARG;
	overflow_name = kilotnc_queue_overflow_name(policy->overflow_policy);
	if (overflow_name == NULL)
		return KILOTNC_QUEUE_POLICY_ERR_ARG;

	ret = snprintf(buf, bufsiz, "queue=%s capacity=%zu overflow=%s "
	    "safety=%u", policy->name, policy->capacity, overflow_name,
	    policy->safety_critical);
	if (ret < 0 || (size_t)ret >= bufsiz)
		return KILOTNC_QUEUE_POLICY_ERR_SMALL;

	return KILOTNC_QUEUE_POLICY_OK;
}

#endif
