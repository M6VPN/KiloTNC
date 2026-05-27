/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/tests/test_budget_metadata.c */

#include <sys/types.h>

#include <stdio.h>

#include "kilotnc_budget.h"

static int test_budget_flags(void);
static int test_budget_rates(void);
static int test_budget_sizes(void);

static int
test_budget_flags(void)
{
	if (KILOTNC_BUDGET_FINALIZED != 0U)
		return __LINE__;
	if (KILOTNC_BUDGET_LINKER_MAP_AVAILABLE != 0U)
		return __LINE__;
	if (KILOTNC_BUDGET_STACK_HIGH_WATER_MEASURED != 0U)
		return __LINE__;
	if (KILOTNC_BUDGET_CPU_CYCLES_MEASURED != 0U)
		return __LINE__;
	if (KILOTNC_BUDGET_HEAP_ALLOWED_RT_PATH != 0U)
		return __LINE__;

	return 0;
}

static int
test_budget_rates(void)
{
	if (KILOTNC_BUDGET_AUDIO_SAMPLE_RATE_HZ != 48000U)
		return __LINE__;
	if (KILOTNC_BUDGET_CONTROL_TICK_MS != 10U)
		return __LINE__;

	return 0;
}

static int
test_budget_sizes(void)
{
	if (KILOTNC_BUDGET_AX25_MAX_FRAME_BYTES == 0U ||
	    KILOTNC_BUDGET_AX25_MAX_FRAME_BYTES > 2048U)
		return __LINE__;
	if (KILOTNC_BUDGET_KISS_MAX_FRAME_BYTES == 0U ||
	    KILOTNC_BUDGET_KISS_MAX_FRAME_BYTES > 2048U)
		return __LINE__;
	if (KILOTNC_BUDGET_DIAG_FAULT_RING_ENTRIES == 0U ||
	    KILOTNC_BUDGET_DIAG_FAULT_RING_ENTRIES > 64U)
		return __LINE__;
	if (KILOTNC_BUDGET_USB_RX_QUEUE_BYTES == 0U ||
	    KILOTNC_BUDGET_USB_RX_QUEUE_BYTES > 4096U)
		return __LINE__;
	if (KILOTNC_BUDGET_USB_TX_QUEUE_BYTES == 0U ||
	    KILOTNC_BUDGET_USB_TX_QUEUE_BYTES > 4096U)
		return __LINE__;
	if (KILOTNC_BUDGET_AUDIO_RX_STUB_SAMPLES == 0U ||
	    KILOTNC_BUDGET_AUDIO_RX_STUB_SAMPLES > 4096U)
		return __LINE__;
	if (KILOTNC_BUDGET_AUDIO_TX_STUB_SAMPLES == 0U ||
	    KILOTNC_BUDGET_AUDIO_TX_STUB_SAMPLES > 4096U)
		return __LINE__;
	if (KILOTNC_BUDGET_AUDIO_BLOCK_SAMPLES == 0U ||
	    KILOTNC_BUDGET_AUDIO_BLOCK_SAMPLES > 1024U)
		return __LINE__;
	if (KILOTNC_BUDGET_MODEM_TX_CHUNK_SAMPLES == 0U ||
	    KILOTNC_BUDGET_MODEM_TX_CHUNK_SAMPLES > 1024U)
		return __LINE__;
	if (KILOTNC_BUDGET_MODEM_RX_CHUNK_SAMPLES == 0U ||
	    KILOTNC_BUDGET_MODEM_RX_CHUNK_SAMPLES > 1024U)
		return __LINE__;
	if (KILOTNC_BUDGET_TNC_USB_READ_BYTES == 0U ||
	    KILOTNC_BUDGET_TNC_USB_READ_BYTES > 1024U)
		return __LINE__;

	return 0;
}

int
test_budget_metadata(void)
{
	int line;

	line = test_budget_flags();
	if (line != 0)
		goto fail;
	line = test_budget_rates();
	if (line != 0)
		goto fail;
	line = test_budget_sizes();
	if (line != 0)
		goto fail;

	(void)printf("ok budget_metadata\n");
	return 0;

fail:
	(void)printf("not ok budget_metadata line %d\n", line);
	return 1;
}
