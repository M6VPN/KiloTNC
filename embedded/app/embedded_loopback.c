/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/app/embedded_loopback.c */

#include <sys/types.h>

#include <stdint.h>
#include <string.h>

#include "audio_stub.h"
#include "embedded_app.h"
#include "embedded_diag.h"
#include "embedded_loopback.h"
#include "embedded_modem.h"
#include "embedded_tnc.h"
#include "platform_stub.h"
#include "usb_cdc_stub.h"

struct embedded_loopback_context {
	struct platform_stub platform;
	struct audio_stub audio;
	struct usb_cdc_stub usb;
	struct embedded_modem modem;
	struct embedded_tnc tnc;
	struct embedded_app app;
};

static enum embedded_loopback_result embedded_loopback_append_usb(
	struct embedded_loopback_context *, uint8_t *, size_t, size_t *);
static enum embedded_loopback_result embedded_loopback_copy_audio(
	struct embedded_loopback_context *,
	const struct embedded_loopback_config *, size_t *);
static void embedded_loopback_default_config(struct embedded_loopback_config *);
static enum embedded_loopback_result embedded_loopback_refresh_stats(
	struct embedded_loopback_context *, struct embedded_loopback_stats *,
	enum embedded_loopback_result);
static enum embedded_loopback_result embedded_loopback_setup(
	struct embedded_loopback_context *, const uint8_t *, size_t);

static enum embedded_loopback_result
embedded_loopback_append_usb(struct embedded_loopback_context *ctx,
	uint8_t *kiss_out, size_t kiss_out_cap, size_t *kiss_out_len)
{
	uint8_t chunk[USB_CDC_STUB_BUFFER_MAX];
	size_t chunk_len;

	if (usb_cdc_stub_take_tx(&ctx->usb, chunk, sizeof(chunk),
	    &chunk_len) != KILOTNC_USB_OK)
		return EMBEDDED_LOOPBACK_ERR_SMALL;
	if (chunk_len == 0U)
		return EMBEDDED_LOOPBACK_OK;
	if (chunk_len > kiss_out_cap - *kiss_out_len)
		return EMBEDDED_LOOPBACK_ERR_SMALL;

	(void)memcpy(kiss_out + *kiss_out_len, chunk, chunk_len);
	*kiss_out_len += chunk_len;
	return EMBEDDED_LOOPBACK_OK;
}

static enum embedded_loopback_result
embedded_loopback_copy_audio(struct embedded_loopback_context *ctx,
	const struct embedded_loopback_config *config, size_t *copied)
{
	int16_t samples[AUDIO_STUB_BUFFER_MAX];
	size_t offset;
	size_t take_len;
	size_t tx_len;

	*copied = 0U;
	if (audio_stub_take_tx(&ctx->audio, samples, sizeof(samples) /
	    sizeof(samples[0]), &tx_len) == KILOTNC_AUDIO_ERR_UNDERFLOW)
		return EMBEDDED_LOOPBACK_OK;
	if (tx_len == 0U)
		return EMBEDDED_LOOPBACK_OK;

	offset = 0U;
	while (offset < tx_len) {
		take_len = tx_len - offset;
		if (take_len > config->audio_copy_chunk)
			take_len = config->audio_copy_chunk;
		if (audio_stub_inject_rx(&ctx->audio, samples + offset,
		    take_len) != KILOTNC_AUDIO_OK)
			return EMBEDDED_LOOPBACK_ERR_FAULT;
		*copied += take_len;
		offset += take_len;
	}

	return EMBEDDED_LOOPBACK_OK;
}

static void
embedded_loopback_default_config(struct embedded_loopback_config *config)
{
	config->max_iterations = EMBEDDED_LOOPBACK_DEFAULT_MAX_ITERATIONS;
	config->audio_copy_chunk = EMBEDDED_LOOPBACK_DEFAULT_AUDIO_COPY;
	config->simulate_watchdog_fault = 0;
	config->watchdog_fault_iteration = 0U;
}

static enum embedded_loopback_result
embedded_loopback_refresh_stats(struct embedded_loopback_context *ctx,
	struct embedded_loopback_stats *stats,
	enum embedded_loopback_result result)
{
	struct embedded_modem_status modem_status;
	struct embedded_tnc_status tnc_status;
	enum kilotnc_gpio_state ptt;

	if (stats == NULL)
		return result;

	stats->usb_rx_bytes = ctx->usb.rx_injected;
	stats->usb_tx_bytes = ctx->usb.tx_written;
	stats->audio_tx_samples = ctx->audio.tx_samples_written;
	stats->audio_rx_samples = ctx->audio.rx_samples_injected;
	stats->watchdog_kicks = ctx->platform.watchdog_kicks;
	if (platform_stub_ptt_state(&ctx->platform, &ptt) ==
	    KILOTNC_PLATFORM_OK)
		stats->ptt_state = (uint8_t)ptt;
	if (embedded_tnc_status(&ctx->tnc, &tnc_status) ==
	    EMBEDDED_TNC_OK) {
		stats->kiss_frames_out = tnc_status.modem_rx_kiss_frames;
		stats->modem_tx_rejected = tnc_status.modem_tx_rejected;
	}
	if (embedded_modem_status(&ctx->modem, &modem_status) ==
	    EMBEDDED_MODEM_OK) {
		stats->modem_tx_frames = modem_status.tx_frames_started;
		stats->modem_rx_frames = modem_status.rx_frames_ok;
	}
	if (embedded_diag_capture(&ctx->app, &stats->diag) !=
	    EMBEDDED_DIAG_OK)
		stats->faulted = 1U;
	stats->last_result = (uint8_t)result;
	return result;
}

static enum embedded_loopback_result
embedded_loopback_setup(struct embedded_loopback_context *ctx,
	const uint8_t *kiss_in, size_t kiss_in_len)
{
	platform_stub_init(&ctx->platform);
	audio_stub_init(&ctx->audio);
	usb_cdc_stub_init(&ctx->usb);
	if (usb_cdc_stub_set_connected(&ctx->usb, 1) != KILOTNC_USB_OK)
		return EMBEDDED_LOOPBACK_ERR_FAULT;
	if (embedded_modem_init(&ctx->modem) != EMBEDDED_MODEM_OK)
		return EMBEDDED_LOOPBACK_ERR_FAULT;
	if (embedded_tnc_init(&ctx->tnc) != EMBEDDED_TNC_OK)
		return EMBEDDED_LOOPBACK_ERR_FAULT;
	if (embedded_tnc_modem(&ctx->tnc, &ctx->modem) != EMBEDDED_TNC_OK)
		return EMBEDDED_LOOPBACK_ERR_FAULT;
	if (embedded_tnc_set_modem_tx(&ctx->tnc, 1) != EMBEDDED_TNC_OK)
		return EMBEDDED_LOOPBACK_ERR_FAULT;
	if (embedded_tnc_set_modem_rx(&ctx->tnc, 1) != EMBEDDED_TNC_OK)
		return EMBEDDED_LOOPBACK_ERR_FAULT;
	if (embedded_app_init(&ctx->app,
	    platform_stub_platform(&ctx->platform)) != EMBEDDED_APP_OK)
		return EMBEDDED_LOOPBACK_ERR_FAULT;
	if (embedded_app_tnc(&ctx->app, &ctx->tnc,
	    usb_cdc_stub_usb(&ctx->usb)) != EMBEDDED_APP_OK)
		return EMBEDDED_LOOPBACK_ERR_FAULT;
	if (embedded_app_modem(&ctx->app, &ctx->modem,
	    audio_stub_audio(&ctx->audio)) != EMBEDDED_APP_OK)
		return EMBEDDED_LOOPBACK_ERR_FAULT;
	if (usb_cdc_stub_inject_rx(&ctx->usb, kiss_in, kiss_in_len) !=
	    KILOTNC_USB_OK)
		return EMBEDDED_LOOPBACK_ERR_SMALL;

	return EMBEDDED_LOOPBACK_OK;
}

enum embedded_loopback_result
embedded_loopback_run_once(const uint8_t *kiss_in, size_t kiss_in_len,
	uint8_t *kiss_out, size_t kiss_out_cap, size_t *kiss_out_len,
	struct embedded_loopback_stats *stats)
{
	struct embedded_loopback_config config;

	embedded_loopback_default_config(&config);
	return embedded_loopback_run_once_config(kiss_in, kiss_in_len,
	    kiss_out, kiss_out_cap, kiss_out_len, &config, stats);
}

enum embedded_loopback_result
embedded_loopback_run_once_config(const uint8_t *kiss_in, size_t kiss_in_len,
	uint8_t *kiss_out, size_t kiss_out_cap, size_t *kiss_out_len,
	const struct embedded_loopback_config *config,
	struct embedded_loopback_stats *stats)
{
	struct embedded_loopback_context ctx;
	struct embedded_loopback_config local_config;
	enum embedded_app_result app_result;
	enum embedded_loopback_result loop_result;
	size_t copied;
	size_t i;

	if (kiss_in == NULL || kiss_out == NULL || kiss_out_len == NULL ||
	    config == NULL || config->max_iterations == 0U ||
	    config->audio_copy_chunk == 0U ||
	    config->audio_copy_chunk > EMBEDDED_LOOPBACK_AUDIO_COPY_MAX)
		return EMBEDDED_LOOPBACK_ERR_ARG;
	if (kiss_out_cap == 0U)
		return EMBEDDED_LOOPBACK_ERR_SMALL;

	(void)memset(&ctx, 0, sizeof(ctx));
	if (stats != NULL)
		(void)memset(stats, 0, sizeof(*stats));
	*kiss_out_len = 0U;
	local_config = *config;
	loop_result = embedded_loopback_setup(&ctx, kiss_in, kiss_in_len);
	if (loop_result != EMBEDDED_LOOPBACK_OK)
		return embedded_loopback_refresh_stats(&ctx, stats,
		    loop_result);

	for (i = 0U; i < local_config.max_iterations; i++) {
		if (stats != NULL)
			stats->iterations = i + 1U;
		if (local_config.simulate_watchdog_fault != 0 &&
		    i == local_config.watchdog_fault_iteration) {
			if (platform_stub_simulate_watchdog_fault(
			    &ctx.platform) != KILOTNC_PLATFORM_OK)
				return embedded_loopback_refresh_stats(&ctx,
				    stats, EMBEDDED_LOOPBACK_ERR_FAULT);
		}
		app_result = embedded_app_step(&ctx.app);
		if (app_result == EMBEDDED_APP_ERR_FAULT) {
			if (stats != NULL)
				stats->faulted = 1U;
			return embedded_loopback_refresh_stats(&ctx, stats,
			    EMBEDDED_LOOPBACK_ERR_FAULT);
		}
		if (app_result != EMBEDDED_APP_OK)
			return embedded_loopback_refresh_stats(&ctx, stats,
			    EMBEDDED_LOOPBACK_ERR_FAULT);
		loop_result = embedded_loopback_copy_audio(&ctx,
		    &local_config, &copied);
		if (loop_result != EMBEDDED_LOOPBACK_OK)
			return embedded_loopback_refresh_stats(&ctx, stats,
			    loop_result);
		if (stats != NULL)
			stats->audio_copied_samples += copied;
		loop_result = embedded_loopback_append_usb(&ctx, kiss_out,
		    kiss_out_cap, kiss_out_len);
		if (loop_result != EMBEDDED_LOOPBACK_OK)
			return embedded_loopback_refresh_stats(&ctx, stats,
			    loop_result);
		if (*kiss_out_len != 0U)
			return embedded_loopback_refresh_stats(&ctx, stats,
			    EMBEDDED_LOOPBACK_OK);
	}

	if (stats != NULL)
		stats->timeout = 1U;
	return embedded_loopback_refresh_stats(&ctx, stats,
	    EMBEDDED_LOOPBACK_ERR_TIMEOUT);
}
