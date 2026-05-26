/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/platform/audio_stub.c */

#include <sys/types.h>

#include <stdint.h>
#include <string.h>

#include "audio_stub.h"

static enum kilotnc_audio_result audio_stub_format(void *,
	struct kilotnc_audio_format *);
static enum kilotnc_audio_result audio_stub_read_rx(void *, int16_t *,
	size_t, size_t *);
static enum kilotnc_audio_result audio_stub_stats(void *,
	struct kilotnc_audio_stats *);
static enum kilotnc_audio_result audio_stub_write_tx(void *,
	const int16_t *, size_t, size_t *);

static enum kilotnc_audio_result
audio_stub_format(void *ctx, struct kilotnc_audio_format *format)
{
	if (ctx == NULL || format == NULL)
		return KILOTNC_AUDIO_ERR_ARG;

	format->sample_rate = KILOTNC_AUDIO_SAMPLE_RATE;
	format->channels = KILOTNC_AUDIO_CHANNELS;
	format->bits_per_sample = KILOTNC_AUDIO_BITS_PER_SAMPLE;
	return KILOTNC_AUDIO_OK;
}

static enum kilotnc_audio_result
audio_stub_read_rx(void *ctx, int16_t *samples, size_t sample_cap,
	size_t *out_len)
{
	struct audio_stub *stub;
	size_t available;
	size_t copy_len;

	if (ctx == NULL || samples == NULL || out_len == NULL)
		return KILOTNC_AUDIO_ERR_ARG;
	if (sample_cap == 0)
		return KILOTNC_AUDIO_ERR_SMALL;

	stub = ctx;
	*out_len = 0;
	if (stub->rx_pos >= stub->rx_len) {
		stub->rx_underflows++;
		return KILOTNC_AUDIO_ERR_UNDERFLOW;
	}

	available = stub->rx_len - stub->rx_pos;
	copy_len = available;
	if (copy_len > sample_cap)
		copy_len = sample_cap;
	(void)memcpy(samples, stub->rx_buf + stub->rx_pos,
	    copy_len * sizeof(samples[0]));
	stub->rx_pos += copy_len;
	stub->rx_samples_read += copy_len;
	*out_len = copy_len;
	if (stub->rx_pos == stub->rx_len) {
		stub->rx_pos = 0;
		stub->rx_len = 0;
	}

	return KILOTNC_AUDIO_OK;
}

static enum kilotnc_audio_result
audio_stub_stats(void *ctx, struct kilotnc_audio_stats *stats)
{
	struct audio_stub *stub;

	if (ctx == NULL || stats == NULL)
		return KILOTNC_AUDIO_ERR_ARG;

	stub = ctx;
	stats->rx_samples_read = stub->rx_samples_read;
	stats->tx_samples_written = stub->tx_samples_written;
	stats->rx_overflows = stub->rx_overflows;
	stats->tx_overflows = stub->tx_overflows;
	stats->rx_underflows = stub->rx_underflows;
	stats->tx_underflows = stub->tx_underflows;
	return KILOTNC_AUDIO_OK;
}

static enum kilotnc_audio_result
audio_stub_write_tx(void *ctx, const int16_t *samples, size_t sample_len,
	size_t *out_len)
{
	struct audio_stub *stub;

	if (ctx == NULL || samples == NULL || out_len == NULL)
		return KILOTNC_AUDIO_ERR_ARG;

	stub = ctx;
	*out_len = 0;
	if (sample_len > sizeof(stub->tx_buf) / sizeof(stub->tx_buf[0]) -
	    stub->tx_len) {
		stub->tx_overflows++;
		return KILOTNC_AUDIO_ERR_OVERFLOW;
	}

	(void)memcpy(stub->tx_buf + stub->tx_len, samples,
	    sample_len * sizeof(samples[0]));
	stub->tx_len += sample_len;
	stub->tx_samples_written += sample_len;
	*out_len = sample_len;
	return KILOTNC_AUDIO_OK;
}

enum kilotnc_audio_result
audio_stub_inject_rx(struct audio_stub *stub, const int16_t *samples,
	size_t sample_len)
{
	if (stub == NULL || samples == NULL)
		return KILOTNC_AUDIO_ERR_ARG;
	if (sample_len > sizeof(stub->rx_buf) / sizeof(stub->rx_buf[0]) -
	    stub->rx_len) {
		stub->rx_overflows++;
		return KILOTNC_AUDIO_ERR_OVERFLOW;
	}

	(void)memcpy(stub->rx_buf + stub->rx_len, samples,
	    sample_len * sizeof(samples[0]));
	stub->rx_len += sample_len;
	stub->rx_samples_injected += sample_len;
	return KILOTNC_AUDIO_OK;
}

void
audio_stub_init(struct audio_stub *stub)
{
	if (stub == NULL)
		return;

	(void)memset(stub, 0, sizeof(*stub));
	stub->audio.ctx = stub;
	stub->audio.read_rx = audio_stub_read_rx;
	stub->audio.write_tx = audio_stub_write_tx;
	stub->audio.format = audio_stub_format;
	stub->audio.stats = audio_stub_stats;
}

const struct kilotnc_audio *
audio_stub_audio(struct audio_stub *stub)
{
	if (stub == NULL)
		return NULL;

	return &stub->audio;
}

enum kilotnc_audio_result
audio_stub_take_tx(struct audio_stub *stub, int16_t *samples,
	size_t sample_cap, size_t *out_len)
{
	if (stub == NULL || samples == NULL || out_len == NULL)
		return KILOTNC_AUDIO_ERR_ARG;
	if (sample_cap < stub->tx_len)
		return KILOTNC_AUDIO_ERR_SMALL;
	if (stub->tx_len == 0) {
		*out_len = 0;
		stub->tx_underflows++;
		return KILOTNC_AUDIO_ERR_UNDERFLOW;
	}

	(void)memcpy(samples, stub->tx_buf,
	    stub->tx_len * sizeof(samples[0]));
	*out_len = stub->tx_len;
	stub->tx_len = 0;
	return KILOTNC_AUDIO_OK;
}
