/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/tests/test_embedded_audio.c */

#include <sys/types.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "audio_stub.h"
#include "embedded_app.h"
#include "embedded_audio.h"
#include "embedded_diag.h"
#include "platform_stub.h"

static int make_audio_app(struct platform_stub *, struct audio_stub *,
	struct embedded_audio *, struct embedded_app *);
static int test_embedded_audio_app_step(void);
static int test_embedded_audio_diag_counters(void);
static int test_embedded_audio_loopback(void);
static int test_embedded_audio_null_args(void);
static int test_embedded_audio_partial_buffer(void);
static int test_embedded_audio_ptt_off(void);
static int test_embedded_audio_rx_underflow(void);
static int test_embedded_audio_tx_overflow(void);
static int test_embedded_audio_watchdog_fault(void);

static int
make_audio_app(struct platform_stub *platform, struct audio_stub *audio_stub,
	struct embedded_audio *audio_bridge, struct embedded_app *app)
{
	platform_stub_init(platform);
	audio_stub_init(audio_stub);
	if (embedded_audio_init(audio_bridge, audio_stub_audio(audio_stub), 4) !=
	    EMBEDDED_AUDIO_OK)
		return __LINE__;
	if (embedded_app_init(app, platform_stub_platform(platform)) !=
	    EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_app_audio_bridge(app, audio_bridge) != EMBEDDED_APP_OK)
		return __LINE__;

	return 0;
}

static int
test_embedded_audio_app_step(void)
{
	struct platform_stub platform;
	struct audio_stub audio_stub;
	struct embedded_audio audio_bridge;
	struct embedded_app app;
	struct embedded_app_status status;
	int16_t samples[2];

	samples[0] = 10;
	samples[1] = 20;
	if (make_audio_app(&platform, &audio_stub, &audio_bridge, &app) != 0)
		return __LINE__;
	if (audio_stub_inject_rx(&audio_stub, samples, 2) !=
	    KILOTNC_AUDIO_OK)
		return __LINE__;
	if (embedded_app_step(&app) != EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_app_status(&app, &status) != EMBEDDED_APP_OK)
		return __LINE__;
	if (status.watchdog_kicks != 1u)
		return __LINE__;
	if (audio_bridge.stats.loopback_blocks != 1u)
		return __LINE__;

	return 0;
}

static int
test_embedded_audio_diag_counters(void)
{
	struct platform_stub platform;
	struct audio_stub audio_stub;
	struct embedded_audio audio_bridge;
	struct embedded_app app;
	struct embedded_diag_snapshot snapshot;
	int16_t samples[3];

	samples[0] = 1;
	samples[1] = 2;
	samples[2] = 3;
	if (make_audio_app(&platform, &audio_stub, &audio_bridge, &app) != 0)
		return __LINE__;
	if (audio_stub_inject_rx(&audio_stub, samples, 3) !=
	    KILOTNC_AUDIO_OK)
		return __LINE__;
	if (embedded_app_step(&app) != EMBEDDED_APP_OK)
		return __LINE__;
	if (embedded_diag_capture(&app, &snapshot) != EMBEDDED_DIAG_OK)
		return __LINE__;
	if (snapshot.audio_rx_samples != 3u)
		return __LINE__;
	if (snapshot.audio_tx_samples != 3u)
		return __LINE__;
	if (snapshot.audio_loopback_blocks != 1u)
		return __LINE__;

	return 0;
}

static int
test_embedded_audio_loopback(void)
{
	struct audio_stub audio_stub;
	struct embedded_audio bridge;
	int16_t input[4];
	int16_t output[4];
	size_t len;

	input[0] = 100;
	input[1] = -200;
	input[2] = 300;
	input[3] = -400;
	audio_stub_init(&audio_stub);
	if (embedded_audio_init(&bridge, audio_stub_audio(&audio_stub), 4) !=
	    EMBEDDED_AUDIO_OK)
		return __LINE__;
	if (audio_stub_inject_rx(&audio_stub, input, 4) != KILOTNC_AUDIO_OK)
		return __LINE__;
	if (embedded_audio_process(&bridge) != EMBEDDED_AUDIO_OK)
		return __LINE__;
	if (audio_stub_take_tx(&audio_stub, output, 4, &len) !=
	    KILOTNC_AUDIO_OK)
		return __LINE__;
	if (len != 4u)
		return __LINE__;
	if (memcmp(input, output, sizeof(input)) != 0)
		return __LINE__;
	if (bridge.stats.loopback_blocks != 1u)
		return __LINE__;

	return 0;
}

static int
test_embedded_audio_null_args(void)
{
	struct audio_stub audio_stub;
	struct embedded_audio bridge;
	struct embedded_audio_stats stats;

	audio_stub_init(&audio_stub);
	if (embedded_audio_init(NULL, audio_stub_audio(&audio_stub), 4) !=
	    EMBEDDED_AUDIO_ERR_ARG)
		return __LINE__;
	if (embedded_audio_init(&bridge, NULL, 4) !=
	    EMBEDDED_AUDIO_ERR_ARG)
		return __LINE__;
	if (embedded_audio_init(&bridge, audio_stub_audio(&audio_stub), 0) !=
	    EMBEDDED_AUDIO_ERR_ARG)
		return __LINE__;
	if (embedded_audio_init(&bridge, audio_stub_audio(&audio_stub),
	    EMBEDDED_AUDIO_BLOCK_MAX + 1U) != EMBEDDED_AUDIO_ERR_ARG)
		return __LINE__;
	if (embedded_audio_process(NULL) != EMBEDDED_AUDIO_ERR_ARG)
		return __LINE__;
	if (embedded_audio_stats(NULL, &stats) != EMBEDDED_AUDIO_ERR_ARG)
		return __LINE__;
	if (embedded_audio_stats(&bridge, NULL) != EMBEDDED_AUDIO_ERR_ARG)
		return __LINE__;

	return 0;
}

static int
test_embedded_audio_partial_buffer(void)
{
	struct audio_stub audio_stub;
	struct embedded_audio bridge;
	int16_t input[2];
	int16_t output[2];
	size_t len;

	input[0] = 7;
	input[1] = 8;
	audio_stub_init(&audio_stub);
	if (embedded_audio_init(&bridge, audio_stub_audio(&audio_stub), 4) !=
	    EMBEDDED_AUDIO_OK)
		return __LINE__;
	if (audio_stub_inject_rx(&audio_stub, input, 2) != KILOTNC_AUDIO_OK)
		return __LINE__;
	if (embedded_audio_process(&bridge) != EMBEDDED_AUDIO_OK)
		return __LINE__;
	if (audio_stub_take_tx(&audio_stub, output, 2, &len) !=
	    KILOTNC_AUDIO_OK)
		return __LINE__;
	if (len != 2u)
		return __LINE__;
	if (memcmp(input, output, sizeof(input)) != 0)
		return __LINE__;

	return 0;
}

static int
test_embedded_audio_ptt_off(void)
{
	struct platform_stub platform;
	struct audio_stub audio_stub;
	struct embedded_audio audio_bridge;
	struct embedded_app app;
	enum kilotnc_gpio_state ptt;
	int16_t sample;

	sample = 9;
	if (make_audio_app(&platform, &audio_stub, &audio_bridge, &app) != 0)
		return __LINE__;
	if (audio_stub_inject_rx(&audio_stub, &sample, 1) != KILOTNC_AUDIO_OK)
		return __LINE__;
	if (embedded_app_step(&app) != EMBEDDED_APP_OK)
		return __LINE__;
	if (platform_stub_ptt_state(&platform, &ptt) != KILOTNC_PLATFORM_OK)
		return __LINE__;
	if (ptt != KILOTNC_GPIO_LOW)
		return __LINE__;

	return 0;
}

static int
test_embedded_audio_rx_underflow(void)
{
	struct audio_stub audio_stub;
	struct embedded_audio bridge;

	audio_stub_init(&audio_stub);
	if (embedded_audio_init(&bridge, audio_stub_audio(&audio_stub), 4) !=
	    EMBEDDED_AUDIO_OK)
		return __LINE__;
	if (embedded_audio_process(&bridge) != EMBEDDED_AUDIO_OK)
		return __LINE__;
	if (bridge.stats.rx_underflows != 1u)
		return __LINE__;
	if (audio_stub.rx_underflows != 1u)
		return __LINE__;

	return 0;
}

static int
test_embedded_audio_tx_overflow(void)
{
	struct audio_stub audio_stub;
	struct embedded_audio bridge;
	int16_t fill[AUDIO_STUB_BUFFER_MAX];
	int16_t sample;
	size_t len;

	(void)memset(fill, 0, sizeof(fill));
	sample = 123;
	audio_stub_init(&audio_stub);
	if (embedded_audio_init(&bridge, audio_stub_audio(&audio_stub), 4) !=
	    EMBEDDED_AUDIO_OK)
		return __LINE__;
	if (audio_stub.audio.write_tx(audio_stub.audio.ctx, fill,
	    AUDIO_STUB_BUFFER_MAX, &len) != KILOTNC_AUDIO_OK)
		return __LINE__;
	if (audio_stub_inject_rx(&audio_stub, &sample, 1) !=
	    KILOTNC_AUDIO_OK)
		return __LINE__;
	if (embedded_audio_process(&bridge) != EMBEDDED_AUDIO_OK)
		return __LINE__;
	if (bridge.stats.tx_overflows != 1u)
		return __LINE__;
	if (audio_stub.tx_overflows != 1u)
		return __LINE__;

	return 0;
}

static int
test_embedded_audio_watchdog_fault(void)
{
	struct platform_stub platform;
	struct audio_stub audio_stub;
	struct embedded_audio audio_bridge;
	struct embedded_app app;
	struct embedded_app_status status;
	const struct kilotnc_platform *platform_if;

	if (make_audio_app(&platform, &audio_stub, &audio_bridge, &app) != 0)
		return __LINE__;
	platform_if = platform_stub_platform(&platform);
	if (platform_if->ptt_set(platform_if->ctx, KILOTNC_GPIO_HIGH) !=
	    KILOTNC_PLATFORM_OK)
		return __LINE__;
	if (platform_stub_simulate_watchdog_fault(&platform) !=
	    KILOTNC_PLATFORM_OK)
		return __LINE__;
	if (embedded_app_step(&app) != EMBEDDED_APP_ERR_FAULT)
		return __LINE__;
	if (embedded_app_status(&app, &status) != EMBEDDED_APP_OK)
		return __LINE__;
	if (status.state != EMBEDDED_APP_FAULT)
		return __LINE__;
	if (status.ptt_state != KILOTNC_GPIO_LOW)
		return __LINE__;

	return 0;
}

int
test_embedded_audio(void)
{
	int line;

	line = test_embedded_audio_loopback();
	if (line != 0)
		goto fail;
	line = test_embedded_audio_partial_buffer();
	if (line != 0)
		goto fail;
	line = test_embedded_audio_rx_underflow();
	if (line != 0)
		goto fail;
	line = test_embedded_audio_tx_overflow();
	if (line != 0)
		goto fail;
	line = test_embedded_audio_diag_counters();
	if (line != 0)
		goto fail;
	line = test_embedded_audio_app_step();
	if (line != 0)
		goto fail;
	line = test_embedded_audio_ptt_off();
	if (line != 0)
		goto fail;
	line = test_embedded_audio_watchdog_fault();
	if (line != 0)
		goto fail;
	line = test_embedded_audio_null_args();
	if (line != 0)
		goto fail;

	(void)printf("ok embedded_audio\n");
	return 0;

fail:
	(void)printf("not ok embedded_audio line %d\n", line);
	return 1;
}
