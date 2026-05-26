/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/targets/stm32h753-nucleo/target_platform.c */

#ifndef KILOTNC_TARGET_STM32H753_NUCLEO
#error "Define KILOTNC_TARGET_STM32H753_NUCLEO for target skeleton builds."
#endif

#include <sys/types.h>

#include <stdint.h>
#include <string.h>

#include "target_platform.h"

static enum kilotnc_platform_result target_diag_count(void *, size_t *);
static enum kilotnc_platform_result target_diag_write(void *, const char *);
static enum kilotnc_platform_result target_fault_count(void *, size_t *);
static enum kilotnc_platform_result target_monotonic_ms(void *, uint32_t *);
static enum kilotnc_platform_result target_ptt_get(void *,
	enum kilotnc_gpio_state *);
static enum kilotnc_platform_result target_ptt_set(void *,
	enum kilotnc_gpio_state);
static enum kilotnc_platform_result target_reset_cause(void *,
	enum kilotnc_reset_cause *);
static enum kilotnc_platform_result target_tick_10ms(void *, uint32_t *);
static enum kilotnc_platform_result target_unsupported(void *);
static enum kilotnc_platform_result target_watchdog_faulted(void *, int *);
static enum kilotnc_platform_result target_watchdog_kick(void *);

static enum kilotnc_platform_result
target_diag_count(void *ctx, size_t *count)
{
	struct stm32h753_nucleo_platform *platform;

	if (ctx == NULL || count == NULL)
		return KILOTNC_PLATFORM_ERR_ARG;

	platform = ctx;
	*count = platform->diag_writes;
	return KILOTNC_PLATFORM_OK;
}

static enum kilotnc_platform_result
target_diag_write(void *ctx, const char *message)
{
	struct stm32h753_nucleo_platform *platform;

	if (ctx == NULL || message == NULL)
		return KILOTNC_PLATFORM_ERR_ARG;

	platform = ctx;
	platform->diag_writes++;
	return KILOTNC_PLATFORM_OK;
}

static enum kilotnc_platform_result
target_fault_count(void *ctx, size_t *count)
{
	if (ctx == NULL || count == NULL)
		return KILOTNC_PLATFORM_ERR_ARG;

	*count = 0U;
	return KILOTNC_PLATFORM_OK;
}

static enum kilotnc_platform_result
target_monotonic_ms(void *ctx, uint32_t *tick_ms)
{
	struct stm32h753_nucleo_platform *platform;

	if (ctx == NULL || tick_ms == NULL)
		return KILOTNC_PLATFORM_ERR_ARG;

	platform = ctx;
	*tick_ms = platform->tick_ms;
	return KILOTNC_PLATFORM_OK;
}

static enum kilotnc_platform_result
target_ptt_get(void *ctx, enum kilotnc_gpio_state *state)
{
	struct stm32h753_nucleo_platform *platform;

	if (ctx == NULL || state == NULL)
		return KILOTNC_PLATFORM_ERR_ARG;

	platform = ctx;
	*state = platform->ptt_state;
	return KILOTNC_PLATFORM_OK;
}

static enum kilotnc_platform_result
target_ptt_set(void *ctx, enum kilotnc_gpio_state state)
{
	struct stm32h753_nucleo_platform *platform;

	if (ctx == NULL)
		return KILOTNC_PLATFORM_ERR_ARG;
	if (state != KILOTNC_GPIO_LOW)
		return KILOTNC_PLATFORM_ERR_UNSUPPORTED;

	platform = ctx;
	platform->ptt_state = KILOTNC_GPIO_LOW;
	return KILOTNC_PLATFORM_OK;
}

static enum kilotnc_platform_result
target_reset_cause(void *ctx, enum kilotnc_reset_cause *cause)
{
	if (ctx == NULL || cause == NULL)
		return KILOTNC_PLATFORM_ERR_ARG;

	*cause = KILOTNC_RESET_UNKNOWN;
	return KILOTNC_PLATFORM_OK;
}

static enum kilotnc_platform_result
target_tick_10ms(void *ctx, uint32_t *ticks)
{
	struct stm32h753_nucleo_platform *platform;

	if (ctx == NULL || ticks == NULL)
		return KILOTNC_PLATFORM_ERR_ARG;

	platform = ctx;
	platform->tick_ms += 10U;
	*ticks = platform->tick_ms / 10U;
	return KILOTNC_PLATFORM_OK;
}

static enum kilotnc_platform_result
target_unsupported(void *ctx)
{
	if (ctx == NULL)
		return KILOTNC_PLATFORM_ERR_ARG;

	return KILOTNC_PLATFORM_ERR_UNSUPPORTED;
}

static enum kilotnc_platform_result
target_watchdog_faulted(void *ctx, int *faulted)
{
	if (ctx == NULL || faulted == NULL)
		return KILOTNC_PLATFORM_ERR_ARG;

	*faulted = 0;
	return KILOTNC_PLATFORM_OK;
}

static enum kilotnc_platform_result
target_watchdog_kick(void *ctx)
{
	if (ctx == NULL)
		return KILOTNC_PLATFORM_ERR_ARG;

	return KILOTNC_PLATFORM_OK;
}

enum kilotnc_platform_result
stm32h753_nucleo_platform_init(struct stm32h753_nucleo_platform *platform)
{
	if (platform == NULL)
		return KILOTNC_PLATFORM_ERR_ARG;

	(void)memset(platform, 0, sizeof(*platform));
	platform->ptt_state = KILOTNC_GPIO_LOW;
	platform->platform.ctx = platform;
	platform->platform.monotonic_ms = target_monotonic_ms;
	platform->platform.tick_10ms = target_tick_10ms;
	platform->platform.watchdog_kick = target_watchdog_kick;
	platform->platform.watchdog_faulted = target_watchdog_faulted;
	platform->platform.reset_cause = target_reset_cause;
	platform->platform.ptt_set = target_ptt_set;
	platform->platform.ptt_get = target_ptt_get;
	platform->platform.diag_write = target_diag_write;
	platform->platform.diag_count = target_diag_count;
	platform->platform.fault_count = target_fault_count;
	platform->platform.usb_poll = target_unsupported;
	platform->platform.audio_poll = target_unsupported;
	return KILOTNC_PLATFORM_OK;
}
