/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/platform/platform_stub.c */

#include <sys/types.h>

#include <stdint.h>
#include <string.h>

#include "platform_stub.h"

static enum kilotnc_platform_result platform_stub_audio_poll(void *);
static enum kilotnc_platform_result platform_stub_diag_write(void *,
	const char *);
static enum kilotnc_platform_result platform_stub_ptt_set(void *, int);
static enum kilotnc_platform_result platform_stub_tick_ms(void *,
	uint32_t *);
static enum kilotnc_platform_result platform_stub_usb_poll(void *);
static enum kilotnc_platform_result platform_stub_watchdog_kick(void *);

static enum kilotnc_platform_result
platform_stub_audio_poll(void *ctx)
{
	if (ctx == NULL)
		return KILOTNC_PLATFORM_ERR_ARG;

	return KILOTNC_PLATFORM_ERR_UNSUPPORTED;
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
platform_stub_ptt_set(void *ctx, int enabled)
{
	struct platform_stub *stub;

	if (ctx == NULL)
		return KILOTNC_PLATFORM_ERR_ARG;

	stub = ctx;
	stub->ptt_state = enabled != 0 ? 1 : 0;
	return KILOTNC_PLATFORM_OK;
}

static enum kilotnc_platform_result
platform_stub_tick_ms(void *ctx, uint32_t *tick_ms)
{
	struct platform_stub *stub;

	if (ctx == NULL || tick_ms == NULL)
		return KILOTNC_PLATFORM_ERR_ARG;

	stub = ctx;
	stub->tick_ms += 10u;
	*tick_ms = stub->tick_ms;
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
platform_stub_watchdog_kick(void *ctx)
{
	struct platform_stub *stub;

	if (ctx == NULL)
		return KILOTNC_PLATFORM_ERR_ARG;

	stub = ctx;
	stub->watchdog_kicks++;
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
	stub->platform.ctx = stub;
	stub->platform.tick_ms = platform_stub_tick_ms;
	stub->platform.watchdog_kick = platform_stub_watchdog_kick;
	stub->platform.ptt_set = platform_stub_ptt_set;
	stub->platform.diag_write = platform_stub_diag_write;
	stub->platform.usb_poll = platform_stub_usb_poll;
	stub->platform.audio_poll = platform_stub_audio_poll;
}
