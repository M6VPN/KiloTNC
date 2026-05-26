/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/tests/test_audio_stub.c */

#include <sys/types.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "audio_stub.h"

static int test_audio_stub_format(void);
static int test_audio_stub_null_args(void);
static int test_audio_stub_rx_overflow(void);
static int test_audio_stub_rx_read(void);
static int test_audio_stub_rx_underflow(void);
static int test_audio_stub_tx_overflow(void);
static int test_audio_stub_tx_underflow(void);
static int test_audio_stub_tx_write_take(void);

static int
test_audio_stub_format(void)
{
	struct audio_stub stub;
	const struct kilotnc_audio *audio;
	struct kilotnc_audio_format format;

	audio_stub_init(&stub);
	audio = audio_stub_audio(&stub);
	if (audio == NULL)
		return __LINE__;
	if (audio->format(audio->ctx, &format) != KILOTNC_AUDIO_OK)
		return __LINE__;
	if (format.sample_rate != KILOTNC_AUDIO_SAMPLE_RATE)
		return __LINE__;
	if (format.channels != KILOTNC_AUDIO_CHANNELS)
		return __LINE__;
	if (format.bits_per_sample != KILOTNC_AUDIO_BITS_PER_SAMPLE)
		return __LINE__;

	return 0;
}

static int
test_audio_stub_null_args(void)
{
	struct audio_stub stub;
	const struct kilotnc_audio *audio;
	struct kilotnc_audio_format format;
	struct kilotnc_audio_stats stats;
	int16_t samples[4];
	size_t len;

	audio_stub_init(&stub);
	audio = audio_stub_audio(&stub);
	if (audio_stub_audio(NULL) != NULL)
		return __LINE__;
	if (audio_stub_inject_rx(NULL, samples, 1) !=
	    KILOTNC_AUDIO_ERR_ARG)
		return __LINE__;
	if (audio_stub_inject_rx(&stub, NULL, 1) !=
	    KILOTNC_AUDIO_ERR_ARG)
		return __LINE__;
	if (audio_stub_take_tx(NULL, samples, 4, &len) !=
	    KILOTNC_AUDIO_ERR_ARG)
		return __LINE__;
	if (audio_stub_take_tx(&stub, NULL, 4, &len) !=
	    KILOTNC_AUDIO_ERR_ARG)
		return __LINE__;
	if (audio_stub_take_tx(&stub, samples, 4, NULL) !=
	    KILOTNC_AUDIO_ERR_ARG)
		return __LINE__;
	if (audio->read_rx(NULL, samples, 4, &len) !=
	    KILOTNC_AUDIO_ERR_ARG)
		return __LINE__;
	if (audio->read_rx(audio->ctx, NULL, 4, &len) !=
	    KILOTNC_AUDIO_ERR_ARG)
		return __LINE__;
	if (audio->read_rx(audio->ctx, samples, 4, NULL) !=
	    KILOTNC_AUDIO_ERR_ARG)
		return __LINE__;
	if (audio->write_tx(NULL, samples, 4, &len) !=
	    KILOTNC_AUDIO_ERR_ARG)
		return __LINE__;
	if (audio->write_tx(audio->ctx, NULL, 4, &len) !=
	    KILOTNC_AUDIO_ERR_ARG)
		return __LINE__;
	if (audio->write_tx(audio->ctx, samples, 4, NULL) !=
	    KILOTNC_AUDIO_ERR_ARG)
		return __LINE__;
	if (audio->format(NULL, &format) != KILOTNC_AUDIO_ERR_ARG)
		return __LINE__;
	if (audio->format(audio->ctx, NULL) != KILOTNC_AUDIO_ERR_ARG)
		return __LINE__;
	if (audio->stats(NULL, &stats) != KILOTNC_AUDIO_ERR_ARG)
		return __LINE__;
	if (audio->stats(audio->ctx, NULL) != KILOTNC_AUDIO_ERR_ARG)
		return __LINE__;

	return 0;
}

static int
test_audio_stub_rx_overflow(void)
{
	struct audio_stub stub;
	int16_t samples[AUDIO_STUB_BUFFER_MAX + 1U];

	audio_stub_init(&stub);
	(void)memset(samples, 0, sizeof(samples));
	if (audio_stub_inject_rx(&stub, samples, AUDIO_STUB_BUFFER_MAX + 1U) !=
	    KILOTNC_AUDIO_ERR_OVERFLOW)
		return __LINE__;
	if (stub.rx_overflows != 1u)
		return __LINE__;

	return 0;
}

static int
test_audio_stub_rx_read(void)
{
	struct audio_stub stub;
	const struct kilotnc_audio *audio;
	int16_t input[3];
	int16_t output[4];
	size_t len;

	input[0] = 11;
	input[1] = -22;
	input[2] = 33;
	audio_stub_init(&stub);
	audio = audio_stub_audio(&stub);
	if (audio_stub_inject_rx(&stub, input, 3) != KILOTNC_AUDIO_OK)
		return __LINE__;
	if (audio->read_rx(audio->ctx, output, 4, &len) !=
	    KILOTNC_AUDIO_OK)
		return __LINE__;
	if (len != 3u)
		return __LINE__;
	if (memcmp(input, output, sizeof(input)) != 0)
		return __LINE__;
	if (stub.rx_samples_read != 3u)
		return __LINE__;

	return 0;
}

static int
test_audio_stub_rx_underflow(void)
{
	struct audio_stub stub;
	const struct kilotnc_audio *audio;
	int16_t samples[2];
	size_t len;

	audio_stub_init(&stub);
	audio = audio_stub_audio(&stub);
	if (audio->read_rx(audio->ctx, samples, 2, &len) !=
	    KILOTNC_AUDIO_ERR_UNDERFLOW)
		return __LINE__;
	if (stub.rx_underflows != 1u)
		return __LINE__;

	return 0;
}

static int
test_audio_stub_tx_overflow(void)
{
	struct audio_stub stub;
	const struct kilotnc_audio *audio;
	int16_t samples[AUDIO_STUB_BUFFER_MAX + 1U];
	size_t len;

	audio_stub_init(&stub);
	audio = audio_stub_audio(&stub);
	(void)memset(samples, 0, sizeof(samples));
	if (audio->write_tx(audio->ctx, samples, AUDIO_STUB_BUFFER_MAX + 1U,
	    &len) != KILOTNC_AUDIO_ERR_OVERFLOW)
		return __LINE__;
	if (stub.tx_overflows != 1u)
		return __LINE__;

	return 0;
}

static int
test_audio_stub_tx_underflow(void)
{
	struct audio_stub stub;
	int16_t samples[2];
	size_t len;

	audio_stub_init(&stub);
	if (audio_stub_take_tx(&stub, samples, 2, &len) !=
	    KILOTNC_AUDIO_ERR_UNDERFLOW)
		return __LINE__;
	if (stub.tx_underflows != 1u)
		return __LINE__;

	return 0;
}

static int
test_audio_stub_tx_write_take(void)
{
	struct audio_stub stub;
	const struct kilotnc_audio *audio;
	int16_t input[3];
	int16_t output[4];
	size_t len;

	input[0] = 44;
	input[1] = -55;
	input[2] = 66;
	audio_stub_init(&stub);
	audio = audio_stub_audio(&stub);
	if (audio->write_tx(audio->ctx, input, 3, &len) !=
	    KILOTNC_AUDIO_OK)
		return __LINE__;
	if (len != 3u)
		return __LINE__;
	if (audio_stub_take_tx(&stub, output, 4, &len) !=
	    KILOTNC_AUDIO_OK)
		return __LINE__;
	if (len != 3u)
		return __LINE__;
	if (memcmp(input, output, sizeof(input)) != 0)
		return __LINE__;

	return 0;
}

int
test_audio_stub(void)
{
	int line;

	line = test_audio_stub_format();
	if (line != 0)
		goto fail;
	line = test_audio_stub_rx_read();
	if (line != 0)
		goto fail;
	line = test_audio_stub_rx_underflow();
	if (line != 0)
		goto fail;
	line = test_audio_stub_tx_write_take();
	if (line != 0)
		goto fail;
	line = test_audio_stub_tx_underflow();
	if (line != 0)
		goto fail;
	line = test_audio_stub_rx_overflow();
	if (line != 0)
		goto fail;
	line = test_audio_stub_tx_overflow();
	if (line != 0)
		goto fail;
	line = test_audio_stub_null_args();
	if (line != 0)
		goto fail;

	(void)printf("ok audio_stub\n");
	return 0;

fail:
	(void)printf("not ok audio_stub line %d\n", line);
	return 1;
}
