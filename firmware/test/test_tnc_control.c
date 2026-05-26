/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/test/test_tnc_control.c */

#include <sys/types.h>

#include <string.h>

#include "tnc_control.h"

#define CHECK(expr) do { if (!(expr)) return __LINE__; } while (0)

static void test_default_config(struct tnc_control_config *);
static int test_tnc_control_access(void);
static int test_tnc_control_init_args(void);
static int test_tnc_control_persistence(void);
static int test_tnc_control_timing(void);
static int test_tnc_control_timeout_abort_stats(void);

int
test_tnc_control(void)
{
	int subres;

	subres = test_tnc_control_init_args();
	if (subres != 0)
		return subres;
	subres = test_tnc_control_access();
	if (subres != 0)
		return subres;
	subres = test_tnc_control_persistence();
	if (subres != 0)
		return subres;
	subres = test_tnc_control_timing();
	if (subres != 0)
		return subres;
	subres = test_tnc_control_timeout_abort_stats();
	if (subres != 0)
		return subres;

	return 0;
}

static void
test_default_config(struct tnc_control_config *config)
{
	(void)memset(config, 0, sizeof(*config));
	config->p = 255U;
	config->slottime_10ms = 1U;
	config->fullduplex = 0U;
	config->txdelay_ms = 0U;
	config->txtail_ms = 0U;
	config->max_tx_ms = 30000U;
	config->rng_seed = 1U;
}

static int
test_tnc_control_access(void)
{
	struct tnc_control ctl;
	struct tnc_control_config config;
	struct tnc_control_stats stats;
	enum tnc_control_ptt ptt;
	int can_emit;

	test_default_config(&config);
	CHECK(tnc_control_init(&ctl, &config) == TNC_CONTROL_OK);
	CHECK(tnc_control_set_dcd(&ctl, 1) == TNC_CONTROL_OK);
	CHECK(tnc_control_request_tx(&ctl) == TNC_CONTROL_ERR_DENIED);
	CHECK(tnc_control_ptt_state(&ctl, &ptt) == TNC_CONTROL_OK);
	CHECK(ptt == TNC_CONTROL_PTT_OFF);
	CHECK(tnc_control_stats(&ctl, &stats) == TNC_CONTROL_OK);
	CHECK(stats.tx_requests == 1U);
	CHECK(stats.tx_denied_busy == 1U);

	test_default_config(&config);
	config.fullduplex = 1U;
	CHECK(tnc_control_init(&ctl, &config) == TNC_CONTROL_OK);
	CHECK(tnc_control_set_dcd(&ctl, 1) == TNC_CONTROL_OK);
	CHECK(tnc_control_request_tx(&ctl) == TNC_CONTROL_OK);
	CHECK(tnc_control_can_emit_audio(&ctl, &can_emit) == TNC_CONTROL_OK);
	CHECK(can_emit == 1);
	CHECK(tnc_control_ptt_state(&ctl, &ptt) == TNC_CONTROL_OK);
	CHECK(ptt == TNC_CONTROL_PTT_ON);
	CHECK(tnc_control_stats(&ctl, &stats) == TNC_CONTROL_OK);
	CHECK(stats.tx_grants == 1U);
	CHECK(stats.ptt_on_events == 1U);

	return 0;
}

static int
test_tnc_control_init_args(void)
{
	struct tnc_control ctl;
	struct tnc_control_config config;
	struct tnc_control_stats stats;
	enum tnc_control_ptt ptt;
	int can_emit;

	CHECK(tnc_control_init(NULL, NULL) == TNC_CONTROL_ERR_ARG);
	CHECK(tnc_control_init(&ctl, NULL) == TNC_CONTROL_OK);
	CHECK(tnc_control_ptt_state(&ctl, &ptt) == TNC_CONTROL_OK);
	CHECK(ptt == TNC_CONTROL_PTT_OFF);
	CHECK(tnc_control_can_emit_audio(&ctl, &can_emit) == TNC_CONTROL_OK);
	CHECK(can_emit == 0);

	test_default_config(&config);
	config.txdelay_ms = 20U;
	config.txtail_ms = 10U;
	config.max_tx_ms = 100U;
	config.rng_seed = 42U;
	CHECK(tnc_control_init(&ctl, &config) == TNC_CONTROL_OK);
	CHECK(ctl.config.txdelay_ms == 20U);
	CHECK(ctl.config.txtail_ms == 10U);
	CHECK(ctl.config.max_tx_ms == 100U);
	CHECK(ctl.rng_state == 42U);

	CHECK(tnc_control_abort_tx(NULL) == TNC_CONTROL_ERR_ARG);
	CHECK(tnc_control_can_emit_audio(NULL, &can_emit) ==
	    TNC_CONTROL_ERR_ARG);
	CHECK(tnc_control_can_emit_audio(&ctl, NULL) == TNC_CONTROL_ERR_ARG);
	CHECK(tnc_control_complete_tx(NULL) == TNC_CONTROL_ERR_ARG);
	CHECK(tnc_control_ptt_state(NULL, &ptt) == TNC_CONTROL_ERR_ARG);
	CHECK(tnc_control_ptt_state(&ctl, NULL) == TNC_CONTROL_ERR_ARG);
	CHECK(tnc_control_request_tx(NULL) == TNC_CONTROL_ERR_ARG);
	CHECK(tnc_control_set_dcd(NULL, 1) == TNC_CONTROL_ERR_ARG);
	CHECK(tnc_control_stats(NULL, &stats) == TNC_CONTROL_ERR_ARG);
	CHECK(tnc_control_stats(&ctl, NULL) == TNC_CONTROL_ERR_ARG);
	CHECK(tnc_control_tick_10ms(NULL) == TNC_CONTROL_ERR_ARG);

	return 0;
}

static int
test_tnc_control_persistence(void)
{
	struct tnc_control ctl_a;
	struct tnc_control ctl_b;
	struct tnc_control_config config;
	struct tnc_control_stats stats_a;
	struct tnc_control_stats stats_b;
	size_t i;
	enum tnc_control_result res_a;
	enum tnc_control_result res_b;

	test_default_config(&config);
	config.p = 255U;
	CHECK(tnc_control_init(&ctl_a, &config) == TNC_CONTROL_OK);
	CHECK(tnc_control_request_tx(&ctl_a) == TNC_CONTROL_OK);

	test_default_config(&config);
	config.p = 0U;
	config.slottime_10ms = 2U;
	CHECK(tnc_control_init(&ctl_a, &config) == TNC_CONTROL_OK);
	CHECK(tnc_control_request_tx(&ctl_a) == TNC_CONTROL_ERR_DENIED);
	CHECK(tnc_control_tick_10ms(&ctl_a) == TNC_CONTROL_OK);
	CHECK(tnc_control_tick_10ms(&ctl_a) == TNC_CONTROL_ERR_DENIED);
	CHECK(tnc_control_stats(&ctl_a, &stats_a) == TNC_CONTROL_OK);
	CHECK(stats_a.tx_persistence_deferrals == 2U);

	test_default_config(&config);
	config.p = 128U;
	config.slottime_10ms = 1U;
	config.rng_seed = 123U;
	CHECK(tnc_control_init(&ctl_a, &config) == TNC_CONTROL_OK);
	CHECK(tnc_control_init(&ctl_b, &config) == TNC_CONTROL_OK);
	res_a = tnc_control_request_tx(&ctl_a);
	res_b = tnc_control_request_tx(&ctl_b);
	CHECK(res_a == res_b);
	for (i = 0U; i < 12U; i++) {
		res_a = tnc_control_tick_10ms(&ctl_a);
		res_b = tnc_control_tick_10ms(&ctl_b);
		CHECK(res_a == res_b);
	}
	CHECK(tnc_control_stats(&ctl_a, &stats_a) == TNC_CONTROL_OK);
	CHECK(tnc_control_stats(&ctl_b, &stats_b) == TNC_CONTROL_OK);
	CHECK(stats_a.tx_grants == stats_b.tx_grants);
	CHECK(stats_a.tx_persistence_deferrals ==
	    stats_b.tx_persistence_deferrals);

	return 0;
}

static int
test_tnc_control_timing(void)
{
	struct tnc_control ctl;
	struct tnc_control_config config;
	struct tnc_control_stats stats;
	enum tnc_control_ptt ptt;
	int can_emit;

	test_default_config(&config);
	config.txdelay_ms = 20U;
	config.txtail_ms = 20U;
	CHECK(tnc_control_init(&ctl, &config) == TNC_CONTROL_OK);
	CHECK(tnc_control_request_tx(&ctl) == TNC_CONTROL_OK);
	CHECK(tnc_control_ptt_state(&ctl, &ptt) == TNC_CONTROL_OK);
	CHECK(ptt == TNC_CONTROL_PTT_ON);
	CHECK(tnc_control_can_emit_audio(&ctl, &can_emit) == TNC_CONTROL_OK);
	CHECK(can_emit == 0);
	CHECK(tnc_control_tick_10ms(&ctl) == TNC_CONTROL_OK);
	CHECK(tnc_control_can_emit_audio(&ctl, &can_emit) == TNC_CONTROL_OK);
	CHECK(can_emit == 0);
	CHECK(tnc_control_tick_10ms(&ctl) == TNC_CONTROL_OK);
	CHECK(tnc_control_can_emit_audio(&ctl, &can_emit) == TNC_CONTROL_OK);
	CHECK(can_emit == 1);
	CHECK(tnc_control_complete_tx(&ctl) == TNC_CONTROL_OK);
	CHECK(tnc_control_tick_10ms(&ctl) == TNC_CONTROL_OK);
	CHECK(tnc_control_ptt_state(&ctl, &ptt) == TNC_CONTROL_OK);
	CHECK(ptt == TNC_CONTROL_PTT_ON);
	CHECK(tnc_control_tick_10ms(&ctl) == TNC_CONTROL_OK);
	CHECK(tnc_control_ptt_state(&ctl, &ptt) == TNC_CONTROL_OK);
	CHECK(ptt == TNC_CONTROL_PTT_OFF);
	CHECK(tnc_control_stats(&ctl, &stats) == TNC_CONTROL_OK);
	CHECK(stats.ptt_on_events == 1U);
	CHECK(stats.ptt_off_events == 1U);

	return 0;
}

static int
test_tnc_control_timeout_abort_stats(void)
{
	struct tnc_control ctl;
	struct tnc_control_config config;
	struct tnc_control_stats stats;
	enum tnc_control_ptt ptt;

	test_default_config(&config);
	config.max_tx_ms = 10U;
	CHECK(tnc_control_init(&ctl, &config) == TNC_CONTROL_OK);
	CHECK(tnc_control_request_tx(&ctl) == TNC_CONTROL_OK);
	CHECK(tnc_control_tick_10ms(&ctl) == TNC_CONTROL_ERR_TIMEOUT);
	CHECK(tnc_control_ptt_state(&ctl, &ptt) == TNC_CONTROL_OK);
	CHECK(ptt == TNC_CONTROL_PTT_OFF);
	CHECK(tnc_control_stats(&ctl, &stats) == TNC_CONTROL_OK);
	CHECK(stats.tx_timeouts == 1U);
	CHECK(stats.ptt_off_events == 1U);

	test_default_config(&config);
	config.txdelay_ms = 50U;
	CHECK(tnc_control_init(&ctl, &config) == TNC_CONTROL_OK);
	CHECK(tnc_control_request_tx(&ctl) == TNC_CONTROL_OK);
	CHECK(tnc_control_abort_tx(&ctl) == TNC_CONTROL_OK);
	CHECK(tnc_control_ptt_state(&ctl, &ptt) == TNC_CONTROL_OK);
	CHECK(ptt == TNC_CONTROL_PTT_OFF);
	CHECK(tnc_control_stats(&ctl, &stats) == TNC_CONTROL_OK);
	CHECK(stats.tx_aborts == 1U);
	CHECK(stats.ptt_on_events == 1U);
	CHECK(stats.ptt_off_events == 1U);

	return 0;
}
