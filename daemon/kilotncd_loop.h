/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/daemon/kilotncd_loop.h */

#ifndef KILOTNCD_LOOP_H
#define KILOTNCD_LOOP_H

#include <sys/types.h>

#include <stdint.h>
#include <stdio.h>

#include "kilotncd_config.h"
#include "tnc1200.h"
#include "tnc_diag.h"

#define KILOTNCD_LOOP_DIAG_MAX	2048U

enum kilotncd_loop_result {
	KILOTNCD_LOOP_OK = 0,
	KILOTNCD_LOOP_DONE,
	KILOTNCD_LOOP_ERR_ARG,
	KILOTNCD_LOOP_ERR_CONFIG,
	KILOTNCD_LOOP_ERR_IO,
	KILOTNCD_LOOP_ERR_TIMEOUT
};

struct kilotncd_loop_config {
	size_t max_iterations;
	size_t diag_interval_ticks;
	uint32_t tick_ms;
	uint8_t once_mode;
	uint8_t dry_run;
};

struct kilotncd_loop {
	struct tnc1200 tnc;
	struct tnc_diag diag;
	struct kilotncd_loop_config loop_config;
	size_t iterations;
	size_t diag_ticks;
	uint8_t running;
};

enum kilotncd_loop_result kilotncd_loop_init(struct kilotncd_loop *,
	const struct kilotncd_config *, const struct kilotncd_loop_config *);
enum kilotncd_loop_result kilotncd_loop_run(struct kilotncd_loop *,
	FILE *);
enum kilotncd_loop_result kilotncd_loop_step(struct kilotncd_loop *,
	FILE *);
enum kilotncd_loop_result kilotncd_loop_status(const struct kilotncd_loop *,
	struct tnc_diag_snapshot *);
enum kilotncd_loop_result kilotncd_loop_stop(struct kilotncd_loop *);

#endif
