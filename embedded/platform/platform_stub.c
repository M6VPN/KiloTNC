/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/platform/platform_stub.c */

#include <sys/types.h>

#include <stdint.h>
#include <string.h>

#include "platform_stub.h"

static enum kilotnc_platform_result platform_stub_audio_poll(void *);
static enum kilotnc_platform_result platform_stub_diag_count(void *,
	size_t *);
static enum kilotnc_platform_result platform_stub_diag_write(void *,
	const char *);
static enum kilotnc_platform_result platform_stub_fault_count(void *,
	size_t *);
static enum kilotnc_platform_result platform_stub_monotonic_ms(void *,
	uint32_t *);
static enum kilotnc_platform_result platform_stub_ptt_get(void *,
	enum kilotnc_gpio_state *);
static enum kilotnc_platform_result platform_stub_ptt_set(void *,
	enum kilotnc_gpio_state);
static enum kilotnc_platform_result platform_stub_reset_cause(void *,
	enum kilotnc_reset_cause *);
static enum kilotnc_platform_result platform_stub_tick_10ms(void *,
	uint32_t *);
static enum kilotnc_platform_result platform_stub_usb_poll(void *);
static enum kilotnc_platform_result platform_stub_watchdog_faulted(void *,
	int *);
static enum kilotnc_platform_result platform_stub_watchdog_kick(void *);

static enum kilotnc_platform_result
platform_stub_audio_poll(void *ctx)
{
	if (ctx == NULL)
		return KILOTNC_PLATFORM_ERR_ARG;

	return KILOTNC_PLATFORM_ERR_UNSUPPORTED;
}

static enum kilotnc_platform_result
platform_stub_diag_count(void *ctx, size_t *count)
{
	struct platform_stub *stub;

	if (ctx == NULL || count == NULL)
		return KILOTNC_PLATFORM_ERR_ARG;

	stub = ctx;
	*count = stub->diag_writes;
	return KILOTNC_PLATFORM_OK;
}

static enum kilotnc_platform_result
platform_stub_diag_write(void *ctx, const char *message)
{
	struct platform_stub *stub;
	size_t i;

	if (ctx == NULL || message == NULL)
		return KILOTNC_PLATFORM_ERR_ARG;

	stub = ctx;
	stub->diag_writes++;
	for (i = 0; i + 1 < sizeof(stub->diag_last) && message[i] != '\0';
	    i++)
		stub->diag_last[i] = message[i];
	stub->diag_last[i] = '\0';

	return KILOTNC_PLATFORM_OK;
}

static enum kilotnc_platform_result
platform_stub_fault_count(void *ctx, size_t *count)
{
	struct platform_stub *stub;

	if (ctx == NULL || count == NULL)
		return KILOTNC_PLATFORM_ERR_ARG;

	stub = ctx;
	*count = stub->platform_faults;
	return KILOTNC_PLATFORM_OK;
}

static enum kilotnc_platform_result
platform_stub_monotonic_ms(void *ctx, uint32_t *tick_ms)
{
	struct platform_stub *stub;

	if (ctx == NULL || tick_ms == NULL)
		return KILOTNC_PLATFORM_ERR_ARG;

	stub = ctx;
	*tick_ms = stub->monotonic_ms;
	return KILOTNC_PLATFORM_OK;
}

static enum kilotnc_platform_result
platform_stub_ptt_get(void *ctx, enum kilotnc_gpio_state *state)
{
	struct platform_stub *stub;

	if (ctx == NULL || state == NULL)
		return KILOTNC_PLATFORM_ERR_ARG;

	stub = ctx;
	*state = stub->ptt_state;
	return KILOTNC_PLATFORM_OK;
}

static enum kilotnc_platform_result
platform_stub_ptt_set(void *ctx, enum kilotnc_gpio_state state)
{
	struct platform_stub *stub;

	if (ctx == NULL)
		return KILOTNC_PLATFORM_ERR_ARG;
	if (state != KILOTNC_GPIO_LOW && state != KILOTNC_GPIO_HIGH)
		return KILOTNC_PLATFORM_ERR_ARG;

	stub = ctx;
	stub->ptt_state = state;
	return KILOTNC_PLATFORM_OK;
}

static enum kilotnc_platform_result
platform_stub_reset_cause(void *ctx, enum kilotnc_reset_cause *cause)
{
	struct platform_stub *stub;

	if (ctx == NULL || cause == NULL)
		return KILOTNC_PLATFORM_ERR_ARG;

	stub = ctx;
	*cause = stub->reset_cause;
	return KILOTNC_PLATFORM_OK;
}

static enum kilotnc_platform_result
platform_stub_tick_10ms(void *ctx, uint32_t *ticks)
{
	struct platform_stub *stub;

	if (ctx == NULL || ticks == NULL)
		return KILOTNC_PLATFORM_ERR_ARG;

	stub = ctx;
	if (stub->watchdog_faulted != 0)
		return KILOTNC_PLATFORM_ERR_FAULT;

	stub->monotonic_ms += 10u;
	stub->control_ticks_10ms++;
	*ticks = stub->control_ticks_10ms;
	return KILOTNC_PLATFORM_OK;
}

static enum kilotnc_platform_result
platform_stub_usb_poll(void *ctx)
{
	if (ctx == NULL)
		return KILOTNC_PLATFORM_ERR_ARG;

	return KILOTNC_PLATFORM_ERR_UNSUPPORTED;
}

static enum kilotnc_platform_result
platform_stub_watchdog_faulted(void *ctx, int *faulted)
{
	struct platform_stub *stub;

	if (ctx == NULL || faulted == NULL)
		return KILOTNC_PLATFORM_ERR_ARG;

	stub = ctx;
	*faulted = stub->watchdog_faulted;
	return KILOTNC_PLATFORM_OK;
}

static enum kilotnc_platform_result
platform_stub_watchdog_kick(void *ctx)
{
	struct platform_stub *stub;

	if (ctx == NULL)
		return KILOTNC_PLATFORM_ERR_ARG;

	stub = ctx;
	if (stub->watchdog_faulted != 0)
		return KILOTNC_PLATFORM_ERR_FAULT;

	stub->watchdog_kicks++;
	return KILOTNC_PLATFORM_OK;
}

enum kilotnc_platform_result
platform_stub_advance_ms(struct platform_stub *stub, uint32_t delta_ms)
{
	if (stub == NULL)
		return KILOTNC_PLATFORM_ERR_ARG;

	stub->monotonic_ms += delta_ms;
	return KILOTNC_PLATFORM_OK;
}

enum kilotnc_platform_result
platform_stub_ptt_state(const struct platform_stub *stub,
	enum kilotnc_gpio_state *state)
{
	if (stub == NULL || state == NULL)
		return KILOTNC_PLATFORM_ERR_ARG;

	*state = stub->ptt_state;
	return KILOTNC_PLATFORM_OK;
}

const struct kilotnc_platform *
platform_stub_platform(struct platform_stub *stub)
{
	if (stub == NULL)
		return NULL;

	return &stub->platform;
}

void
platform_stub_init(struct platform_stub *stub)
{
	if (stub == NULL)
		return;

	(void)memset(stub, 0, sizeof(*stub));
	stub->reset_cause = KILOTNC_RESET_POWER_ON;
	stub->ptt_state = KILOTNC_GPIO_LOW;
	stub->platform.ctx = stub;
	stub->platform.monotonic_ms = platform_stub_monotonic_ms;
	stub->platform.tick_10ms = platform_stub_tick_10ms;
	stub->platform.watchdog_kick = platform_stub_watchdog_kick;
	stub->platform.watchdog_faulted = platform_stub_watchdog_faulted;
	stub->platform.reset_cause = platform_stub_reset_cause;
	stub->platform.ptt_set = platform_stub_ptt_set;
	stub->platform.ptt_get = platform_stub_ptt_get;
	stub->platform.diag_write = platform_stub_diag_write;
	stub->platform.diag_count = platform_stub_diag_count;
	stub->platform.fault_count = platform_stub_fault_count;
	stub->platform.usb_poll = platform_stub_usb_poll;
	stub->platform.audio_poll = platform_stub_audio_poll;
}

enum kilotnc_platform_result
platform_stub_simulate_watchdog_fault(struct platform_stub *stub)
{
	if (stub == NULL)
		return KILOTNC_PLATFORM_ERR_ARG;

	stub->watchdog_faulted = 1;
	stub->reset_cause = KILOTNC_RESET_WATCHDOG;
	stub->ptt_state = KILOTNC_GPIO_LOW;
	stub->platform_faults++;
	return KILOTNC_PLATFORM_OK;
}
