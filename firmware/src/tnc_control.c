/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/src/tnc_control.c */

#include <sys/types.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "tnc_control.h"

#define TNC_CONTROL_DEFAULT_P		255U
#define TNC_CONTROL_DEFAULT_SLOTTIME	10U
#define TNC_CONTROL_DEFAULT_MAX_TX_MS	30000U
#define TNC_CONTROL_DEFAULT_RNG_SEED	1U

static enum tnc_control_result tnc_control_attempt_grant(
	struct tnc_control *);
static uint8_t tnc_control_next_random(struct tnc_control *);
static void tnc_control_ptt_off(struct tnc_control *);
static void tnc_control_ptt_on(struct tnc_control *);
static uint32_t tnc_control_slot_ticks(const struct tnc_control *);
static enum tnc_control_result tnc_control_start_grant(
	struct tnc_control *);
static uint32_t tnc_control_ticks_from_ms(uint32_t);
static enum tnc_control_result tnc_control_timeout(struct tnc_control *);

enum tnc_control_result
tnc_control_abort_tx(struct tnc_control *ctl)
{
	if (ctl == NULL)
		return TNC_CONTROL_ERR_ARG;

	ctl->stats.tx_aborts++;
	ctl->state = TNC_CONTROL_IDLE;
	ctl->slot_wait_ticks = 0U;
	ctl->txdelay_ticks = 0U;
	ctl->txtail_ticks = 0U;
	ctl->max_tx_ticks = 0U;
	tnc_control_ptt_off(ctl);

	return TNC_CONTROL_OK;
}

enum tnc_control_result
tnc_control_can_emit_audio(const struct tnc_control *ctl, int *can_emit)
{
	if (ctl == NULL || can_emit == NULL)
		return TNC_CONTROL_ERR_ARG;

	*can_emit = ctl->state == TNC_CONTROL_TX_ACTIVE;
	return TNC_CONTROL_OK;
}

enum tnc_control_result
tnc_control_complete_tx(struct tnc_control *ctl)
{
	if (ctl == NULL)
		return TNC_CONTROL_ERR_ARG;
	if (ctl->state != TNC_CONTROL_TX_ACTIVE)
		return TNC_CONTROL_ERR_DENIED;

	ctl->txtail_ticks = tnc_control_ticks_from_ms(ctl->config.txtail_ms);
	if (ctl->txtail_ticks == 0U) {
		ctl->state = TNC_CONTROL_IDLE;
		ctl->max_tx_ticks = 0U;
		tnc_control_ptt_off(ctl);
	} else {
		ctl->state = TNC_CONTROL_TXTAIL;
	}

	return TNC_CONTROL_OK;
}

enum tnc_control_result
tnc_control_init(struct tnc_control *ctl,
	const struct tnc_control_config *config)
{
	struct tnc_control_config defaults;

	if (ctl == NULL)
		return TNC_CONTROL_ERR_ARG;

	defaults.p = TNC_CONTROL_DEFAULT_P;
	defaults.slottime_10ms = TNC_CONTROL_DEFAULT_SLOTTIME;
	defaults.fullduplex = 0U;
	defaults.txdelay_ms = 0U;
	defaults.txtail_ms = 0U;
	defaults.max_tx_ms = TNC_CONTROL_DEFAULT_MAX_TX_MS;
	defaults.rng_seed = TNC_CONTROL_DEFAULT_RNG_SEED;
	if (config == NULL)
		config = &defaults;

	(void)memset(ctl, 0, sizeof(*ctl));
	ctl->config = *config;
	if (ctl->config.max_tx_ms == 0U)
		ctl->config.max_tx_ms = TNC_CONTROL_DEFAULT_MAX_TX_MS;
	if (ctl->config.rng_seed == 0U)
		ctl->config.rng_seed = TNC_CONTROL_DEFAULT_RNG_SEED;
	ctl->rng_state = ctl->config.rng_seed;
	ctl->state = TNC_CONTROL_IDLE;
	ctl->ptt = TNC_CONTROL_PTT_OFF;

	return TNC_CONTROL_OK;
}

enum tnc_control_result
tnc_control_ptt_state(const struct tnc_control *ctl,
	enum tnc_control_ptt *ptt)
{
	if (ctl == NULL || ptt == NULL)
		return TNC_CONTROL_ERR_ARG;

	*ptt = ctl->ptt;
	return TNC_CONTROL_OK;
}

enum tnc_control_result
tnc_control_request_tx(struct tnc_control *ctl)
{
	enum tnc_control_result res;

	if (ctl == NULL)
		return TNC_CONTROL_ERR_ARG;
	if (ctl->state != TNC_CONTROL_IDLE)
		return TNC_CONTROL_ERR_BUSY;

	ctl->stats.tx_requests++;
	res = tnc_control_attempt_grant(ctl);
	if (res == TNC_CONTROL_ERR_DENIED) {
		ctl->state = TNC_CONTROL_WAIT_SLOT;
		ctl->slot_wait_ticks = tnc_control_slot_ticks(ctl);
	}

	return res;
}

enum tnc_control_result
tnc_control_set_dcd(struct tnc_control *ctl, int busy)
{
	if (ctl == NULL)
		return TNC_CONTROL_ERR_ARG;

	ctl->dcd_busy = busy != 0;
	return TNC_CONTROL_OK;
}

enum tnc_control_result
tnc_control_stats(const struct tnc_control *ctl,
	struct tnc_control_stats *stats)
{
	if (ctl == NULL || stats == NULL)
		return TNC_CONTROL_ERR_ARG;

	*stats = ctl->stats;
	return TNC_CONTROL_OK;
}

enum tnc_control_result
tnc_control_tick_10ms(struct tnc_control *ctl)
{
	enum tnc_control_result res;

	if (ctl == NULL)
		return TNC_CONTROL_ERR_ARG;

	if (ctl->state == TNC_CONTROL_WAIT_SLOT) {
		if (ctl->slot_wait_ticks > 0U)
			ctl->slot_wait_ticks--;
		if (ctl->slot_wait_ticks == 0U) {
			res = tnc_control_attempt_grant(ctl);
			if (res == TNC_CONTROL_ERR_DENIED)
				ctl->slot_wait_ticks = tnc_control_slot_ticks(ctl);
			return res;
		}
		return TNC_CONTROL_OK;
	}

	if (ctl->state == TNC_CONTROL_TXDELAY ||
	    ctl->state == TNC_CONTROL_TX_ACTIVE ||
	    ctl->state == TNC_CONTROL_TXTAIL) {
		if (ctl->max_tx_ticks > 0U)
			ctl->max_tx_ticks--;
		if (ctl->max_tx_ticks == 0U)
			return tnc_control_timeout(ctl);
	}

	if (ctl->state == TNC_CONTROL_TXDELAY) {
		if (ctl->txdelay_ticks > 0U)
			ctl->txdelay_ticks--;
		if (ctl->txdelay_ticks == 0U)
			ctl->state = TNC_CONTROL_TX_ACTIVE;
	}
	if (ctl->state == TNC_CONTROL_TXTAIL) {
		if (ctl->txtail_ticks > 0U)
			ctl->txtail_ticks--;
		if (ctl->txtail_ticks == 0U) {
			ctl->state = TNC_CONTROL_IDLE;
			ctl->max_tx_ticks = 0U;
			tnc_control_ptt_off(ctl);
		}
	}

	return TNC_CONTROL_OK;
}

static enum tnc_control_result
tnc_control_attempt_grant(struct tnc_control *ctl)
{
	uint8_t sample;

	if (ctl->config.fullduplex == 0U && ctl->dcd_busy) {
		ctl->stats.tx_denied_busy++;
		return TNC_CONTROL_ERR_DENIED;
	}
	if (ctl->config.p == 0U) {
		ctl->stats.tx_persistence_deferrals++;
		return TNC_CONTROL_ERR_DENIED;
	}
	if (ctl->config.p != 255U) {
		sample = tnc_control_next_random(ctl);
		if (sample >= ctl->config.p) {
			ctl->stats.tx_persistence_deferrals++;
			return TNC_CONTROL_ERR_DENIED;
		}
	}

	return tnc_control_start_grant(ctl);
}

static uint8_t
tnc_control_next_random(struct tnc_control *ctl)
{
	ctl->rng_state = (ctl->rng_state * 1103515245U) + 12345U;
	return (uint8_t)((ctl->rng_state >> 16U) & 0xFFU);
}

static void
tnc_control_ptt_off(struct tnc_control *ctl)
{
	if (ctl->ptt == TNC_CONTROL_PTT_ON) {
		ctl->ptt = TNC_CONTROL_PTT_OFF;
		ctl->stats.ptt_off_events++;
	}
}

static void
tnc_control_ptt_on(struct tnc_control *ctl)
{
	if (ctl->ptt == TNC_CONTROL_PTT_OFF) {
		ctl->ptt = TNC_CONTROL_PTT_ON;
		ctl->stats.ptt_on_events++;
	}
}

static uint32_t
tnc_control_slot_ticks(const struct tnc_control *ctl)
{
	return ctl->config.slottime_10ms == 0U ? 1U :
	    ctl->config.slottime_10ms;
}

static enum tnc_control_result
tnc_control_start_grant(struct tnc_control *ctl)
{
	ctl->stats.tx_grants++;
	ctl->max_tx_ticks = tnc_control_ticks_from_ms(ctl->config.max_tx_ms);
	if (ctl->max_tx_ticks == 0U)
		ctl->max_tx_ticks =
		    tnc_control_ticks_from_ms(TNC_CONTROL_DEFAULT_MAX_TX_MS);
	tnc_control_ptt_on(ctl);
	ctl->txdelay_ticks = tnc_control_ticks_from_ms(ctl->config.txdelay_ms);
	if (ctl->txdelay_ticks == 0U)
		ctl->state = TNC_CONTROL_TX_ACTIVE;
	else
		ctl->state = TNC_CONTROL_TXDELAY;

	return TNC_CONTROL_OK;
}

static uint32_t
tnc_control_ticks_from_ms(uint32_t ms)
{
	return (ms + 9U) / 10U;
}

static enum tnc_control_result
tnc_control_timeout(struct tnc_control *ctl)
{
	ctl->stats.tx_timeouts++;
	ctl->state = TNC_CONTROL_IDLE;
	ctl->slot_wait_ticks = 0U;
	ctl->txdelay_ticks = 0U;
	ctl->txtail_ticks = 0U;
	tnc_control_ptt_off(ctl);

	return TNC_CONTROL_ERR_TIMEOUT;
}
