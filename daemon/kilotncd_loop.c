/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/daemon/kilotncd_loop.c */

#include <sys/types.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "afsk1200.h"
#include "kiss.h"
#include "kilotncd_loop.h"
#include "tnc_control.h"
#include "tnc_mode.h"

static enum kilotncd_loop_result kilotncd_loop_apply_mode(
	struct tnc1200 *, enum tnc_mode_id, int);
static enum kilotncd_loop_result kilotncd_loop_emit_diag(
	struct kilotncd_loop *, FILE *);
static enum kilotncd_loop_result kilotncd_loop_init_tnc(
	struct tnc1200 *, const struct kilotncd_config *);

enum kilotncd_loop_result
kilotncd_loop_init(struct kilotncd_loop *loop,
	const struct kilotncd_config *daemon_config,
	const struct kilotncd_loop_config *loop_config)
{
	if (loop == NULL || daemon_config == NULL || loop_config == NULL)
		return KILOTNCD_LOOP_ERR_ARG;
	if (loop_config->max_iterations == 0U || loop_config->tick_ms == 0U)
		return KILOTNCD_LOOP_ERR_CONFIG;

	(void)memset(loop, 0, sizeof(*loop));
	loop->loop_config = *loop_config;
	if (kilotncd_loop_init_tnc(&loop->tnc, daemon_config) !=
	    KILOTNCD_LOOP_OK)
		return KILOTNCD_LOOP_ERR_CONFIG;
	if (tnc_diag_init(&loop->diag) != TNC_DIAG_OK)
		return KILOTNCD_LOOP_ERR_CONFIG;
	loop->running = 1U;

	return KILOTNCD_LOOP_OK;
}

enum kilotncd_loop_result
kilotncd_loop_run(struct kilotncd_loop *loop, FILE *diag_fp)
{
	enum kilotncd_loop_result res;

	if (loop == NULL)
		return KILOTNCD_LOOP_ERR_ARG;
	for (;;) {
		res = kilotncd_loop_step(loop, diag_fp);
		if (res == KILOTNCD_LOOP_DONE)
			return KILOTNCD_LOOP_OK;
		if (res != KILOTNCD_LOOP_OK)
			return res;
	}
}

enum kilotncd_loop_result
kilotncd_loop_step(struct kilotncd_loop *loop, FILE *diag_fp)
{
	enum tnc_control_result cres;

	if (loop == NULL)
		return KILOTNCD_LOOP_ERR_ARG;
	if (loop->running == 0U)
		return KILOTNCD_LOOP_DONE;
	if (loop->iterations >= loop->loop_config.max_iterations) {
		loop->running = 0U;
		return KILOTNCD_LOOP_DONE;
	}

	cres = tnc_control_tick_10ms(&loop->tnc.control);
	if (cres == TNC_CONTROL_ERR_TIMEOUT) {
		(void)tnc1200_abort_tx(&loop->tnc);
		loop->running = 0U;
		return KILOTNCD_LOOP_ERR_TIMEOUT;
	}
	if (cres != TNC_CONTROL_OK && cres != TNC_CONTROL_ERR_DENIED)
		return KILOTNCD_LOOP_ERR_CONFIG;

	loop->iterations++;
	loop->diag_ticks++;
	if (loop->loop_config.diag_interval_ticks != 0U &&
	    loop->diag_ticks >= loop->loop_config.diag_interval_ticks) {
		if (kilotncd_loop_emit_diag(loop, diag_fp) !=
		    KILOTNCD_LOOP_OK)
			return KILOTNCD_LOOP_ERR_IO;
		loop->diag_ticks = 0U;
	}
	if (loop->iterations >= loop->loop_config.max_iterations) {
		loop->running = 0U;
		return KILOTNCD_LOOP_DONE;
	}

	return KILOTNCD_LOOP_OK;
}

enum kilotncd_loop_result
kilotncd_loop_status(const struct kilotncd_loop *loop,
	struct tnc_diag_snapshot *snapshot)
{
	struct tnc_diag diag;

	if (loop == NULL || snapshot == NULL)
		return KILOTNCD_LOOP_ERR_ARG;
	if (tnc_diag_init(&diag) != TNC_DIAG_OK ||
	    tnc_diag_capture_tnc1200(&diag, &loop->tnc) != TNC_DIAG_OK ||
	    tnc_diag_snapshot(&diag, snapshot) != TNC_DIAG_OK)
		return KILOTNCD_LOOP_ERR_CONFIG;

	return KILOTNCD_LOOP_OK;
}

enum kilotncd_loop_result
kilotncd_loop_stop(struct kilotncd_loop *loop)
{
	if (loop == NULL)
		return KILOTNCD_LOOP_ERR_ARG;
	if (tnc1200_abort_tx(&loop->tnc) != TNC1200_OK)
		return KILOTNCD_LOOP_ERR_CONFIG;
	loop->running = 0U;

	return KILOTNCD_LOOP_OK;
}

static enum kilotncd_loop_result
kilotncd_loop_apply_mode(struct tnc1200 *tnc, enum tnc_mode_id mode,
	int temporary)
{
	uint8_t payload[1];
	uint8_t frame[8];
	size_t frame_len;

	if (tnc_mode_to_nino_sethw(mode, temporary, &payload[0]) !=
	    TNC_MODE_OK)
		return KILOTNCD_LOOP_ERR_CONFIG;
	if (kiss_encode_frame(0U, KISS_CMD_SETHARDWARE, payload,
	    sizeof(payload), frame, sizeof(frame), &frame_len) != KISS_OK)
		return KILOTNCD_LOOP_ERR_CONFIG;
	if (tnc1200_host_input(tnc, frame, frame_len) != TNC1200_OK)
		return KILOTNCD_LOOP_ERR_CONFIG;

	return KILOTNCD_LOOP_OK;
}

static enum kilotncd_loop_result
kilotncd_loop_emit_diag(struct kilotncd_loop *loop, FILE *diag_fp)
{
	struct tnc_diag_snapshot snapshot;
	char buf[KILOTNCD_LOOP_DIAG_MAX];
	size_t len;

	if (diag_fp == NULL)
		return KILOTNCD_LOOP_OK;
	if (tnc_diag_capture_tnc1200(&loop->diag, &loop->tnc) !=
	    TNC_DIAG_OK ||
	    tnc_diag_snapshot(&loop->diag, &snapshot) != TNC_DIAG_OK ||
	    tnc_diag_format_snapshot(&snapshot, buf, sizeof(buf), &len) !=
	    TNC_DIAG_OK)
		return KILOTNCD_LOOP_ERR_IO;
	if (fprintf(diag_fp, "loop_diag iteration=%zu %s\n",
	    loop->iterations, buf) < 0)
		return KILOTNCD_LOOP_ERR_IO;

	return KILOTNCD_LOOP_OK;
}

static enum kilotncd_loop_result
kilotncd_loop_init_tnc(struct tnc1200 *tnc,
	const struct kilotncd_config *daemon_config)
{
	struct tnc1200_config tnc_config;

	(void)memset(&tnc_config, 0, sizeof(tnc_config));
	tnc_config.txdelay_flags = AFSK1200_TX_DEFAULT_TXDELAY_FLAGS;
	tnc_config.txtail_flags = AFSK1200_TX_DEFAULT_TXTAIL_FLAGS;
	tnc_config.amplitude = AFSK1200_PCM_AMPLITUDE;
	tnc_config.p = daemon_config->p;
	tnc_config.slottime_10ms = daemon_config->slottime_10ms;
	tnc_config.fullduplex = daemon_config->fullduplex;
	tnc_config.max_tx_ms = daemon_config->max_tx_ms;
	tnc_config.rng_seed = 1U;
	if (tnc1200_init(tnc, &tnc_config) != TNC1200_OK)
		return KILOTNCD_LOOP_ERR_CONFIG;
	if (kilotncd_loop_apply_mode(tnc, daemon_config->mode,
	    daemon_config->mode_temporary) != KILOTNCD_LOOP_OK)
		return KILOTNCD_LOOP_ERR_CONFIG;

	return KILOTNCD_LOOP_OK;
}
