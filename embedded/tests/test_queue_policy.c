/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/tests/test_queue_policy.c */

#include <sys/types.h>

#include <stdio.h>
#include <string.h>

#include "kilotnc_queue_policy.h"

static int test_queue_policy_all_entries(void);
static int test_queue_policy_finalized_flag(void);
static int test_queue_policy_format(void);
static int test_queue_policy_null_args(void);
static int test_queue_policy_safety_rules(void);

static int
test_queue_policy_all_entries(void)
{
	const struct kilotnc_queue_policy *policy;
	enum kilotnc_queue_id id;

	if (sizeof(kilotnc_queue_policies) /
	    sizeof(kilotnc_queue_policies[0]) != KILOTNC_QUEUE_COUNT)
		return __LINE__;

	for (id = KILOTNC_QUEUE_USB_RX; id < KILOTNC_QUEUE_COUNT; id++) {
		policy = kilotnc_queue_policy_get(id);
		if (policy == NULL)
			return __LINE__;
		if (policy->id != id)
			return __LINE__;
		if (policy->name == NULL || policy->name[0] == '\0')
			return __LINE__;
		if (policy->capacity == 0U)
			return __LINE__;
		if (kilotnc_queue_overflow_name(policy->overflow_policy) ==
		    NULL)
			return __LINE__;
	}

	return 0;
}

static int
test_queue_policy_finalized_flag(void)
{
	if (KILOTNC_QUEUE_POLICY_FINALIZED != 0U)
		return __LINE__;

	return 0;
}

static int
test_queue_policy_format(void)
{
	char buf[96];

	if (kilotnc_queue_policy_format(KILOTNC_QUEUE_USB_RX, buf,
	    sizeof(buf)) != KILOTNC_QUEUE_POLICY_OK)
		return __LINE__;
	if (strstr(buf, "queue=usb_rx") == NULL)
		return __LINE__;
	if (strstr(buf, "overflow=drop_newest") == NULL)
		return __LINE__;
	if (kilotnc_queue_policy_format(KILOTNC_QUEUE_CONTROL_EVENT, buf,
	    sizeof(buf)) != KILOTNC_QUEUE_POLICY_OK)
		return __LINE__;
	if (strstr(buf, "queue=control_event") == NULL)
		return __LINE__;
	if (strstr(buf, "overflow=safety_fault") == NULL)
		return __LINE__;
	if (kilotnc_queue_policy_format(KILOTNC_QUEUE_CONTROL_EVENT, buf,
	    8U) != KILOTNC_QUEUE_POLICY_ERR_SMALL)
		return __LINE__;

	return 0;
}

static int
test_queue_policy_null_args(void)
{
	char buf[64];

	if (kilotnc_queue_policy_get(KILOTNC_QUEUE_COUNT) != NULL)
		return __LINE__;
	if (kilotnc_queue_policy_format(KILOTNC_QUEUE_USB_RX, NULL,
	    sizeof(buf)) != KILOTNC_QUEUE_POLICY_ERR_ARG)
		return __LINE__;
	if (kilotnc_queue_policy_format(KILOTNC_QUEUE_USB_RX, buf, 0U) !=
	    KILOTNC_QUEUE_POLICY_ERR_ARG)
		return __LINE__;
	if (kilotnc_queue_policy_format(KILOTNC_QUEUE_COUNT, buf,
	    sizeof(buf)) != KILOTNC_QUEUE_POLICY_ERR_ARG)
		return __LINE__;
	if (kilotnc_queue_overflow_name(
	    (enum kilotnc_queue_overflow_policy)99) != NULL)
		return __LINE__;

	return 0;
}

static int
test_queue_policy_safety_rules(void)
{
	const struct kilotnc_queue_policy *audio_rx;
	const struct kilotnc_queue_policy *audio_tx;
	const struct kilotnc_queue_policy *control;
	const struct kilotnc_queue_policy *diag;
	const struct kilotnc_queue_policy *usb_rx;
	const struct kilotnc_queue_policy *usb_tx;

	usb_rx = kilotnc_queue_policy_get(KILOTNC_QUEUE_USB_RX);
	usb_tx = kilotnc_queue_policy_get(KILOTNC_QUEUE_USB_TX);
	audio_rx = kilotnc_queue_policy_get(KILOTNC_QUEUE_AUDIO_RX_SAMPLE);
	audio_tx = kilotnc_queue_policy_get(KILOTNC_QUEUE_AUDIO_TX_SAMPLE);
	diag = kilotnc_queue_policy_get(KILOTNC_QUEUE_DIAG_EVENT);
	control = kilotnc_queue_policy_get(KILOTNC_QUEUE_CONTROL_EVENT);
	if (usb_rx == NULL || usb_tx == NULL || audio_rx == NULL ||
	    audio_tx == NULL || diag == NULL || control == NULL)
		return __LINE__;
	if (usb_rx->overflow_policy != KILOTNC_QUEUE_DROP_NEWEST)
		return __LINE__;
	if (usb_tx->overflow_policy != KILOTNC_QUEUE_DROP_NEWEST)
		return __LINE__;
	if (audio_rx->overflow_policy != KILOTNC_QUEUE_DROP_NEWEST)
		return __LINE__;
	if (audio_tx->overflow_policy != KILOTNC_QUEUE_DROP_NEWEST)
		return __LINE__;
	if (diag->overflow_policy != KILOTNC_QUEUE_DROP_OLDEST)
		return __LINE__;
	if (control->safety_critical == 0U)
		return __LINE__;
	if (control->overflow_policy == KILOTNC_QUEUE_DROP_NEWEST ||
	    control->overflow_policy == KILOTNC_QUEUE_DROP_OLDEST)
		return __LINE__;
	if (control->overflow_policy != KILOTNC_QUEUE_SAFETY_FAULT)
		return __LINE__;

	return 0;
}

int
test_queue_policy(void)
{
	int line;

	line = test_queue_policy_all_entries();
	if (line != 0)
		goto fail;
	line = test_queue_policy_safety_rules();
	if (line != 0)
		goto fail;
	line = test_queue_policy_format();
	if (line != 0)
		goto fail;
	line = test_queue_policy_null_args();
	if (line != 0)
		goto fail;
	line = test_queue_policy_finalized_flag();
	if (line != 0)
		goto fail;

	(void)printf("ok queue_policy\n");
	return 0;

fail:
	(void)printf("not ok queue_policy line %d\n", line);
	return 1;
}
