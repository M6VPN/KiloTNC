/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/include/kilotnc_embedded_loopback.h */

#ifndef KILOTNC_EMBEDDED_LOOPBACK_H
#define KILOTNC_EMBEDDED_LOOPBACK_H

#include <sys/types.h>

#include <stdint.h>

#include "kilotnc_embedded_diag.h"

#define EMBEDDED_LOOPBACK_DEFAULT_MAX_ITERATIONS 4096U
#define EMBEDDED_LOOPBACK_DEFAULT_AUDIO_COPY 64U
#define EMBEDDED_LOOPBACK_AUDIO_COPY_MAX 64U

enum embedded_loopback_result {
	EMBEDDED_LOOPBACK_OK = 0,
	EMBEDDED_LOOPBACK_ERR_ARG,
	EMBEDDED_LOOPBACK_ERR_SMALL,
	EMBEDDED_LOOPBACK_ERR_TIMEOUT,
	EMBEDDED_LOOPBACK_ERR_MISMATCH,
	EMBEDDED_LOOPBACK_ERR_FAULT
};

struct embedded_loopback_config {
	size_t max_iterations;
	size_t audio_copy_chunk;
	int simulate_watchdog_fault;
	size_t watchdog_fault_iteration;
};

struct embedded_loopback_stats {
	size_t iterations;
	size_t usb_rx_bytes;
	size_t usb_tx_bytes;
	size_t audio_tx_samples;
	size_t audio_rx_samples;
	size_t audio_copied_samples;
	size_t modem_tx_frames;
	size_t modem_tx_rejected;
	size_t modem_rx_frames;
	size_t kiss_frames_out;
	size_t watchdog_kicks;
	uint8_t ptt_state;
	uint8_t timeout;
	uint8_t faulted;
	uint8_t last_result;
	struct embedded_diag_snapshot diag;
};

enum embedded_loopback_result embedded_loopback_run_once(
	const uint8_t *, size_t, uint8_t *, size_t, size_t *,
	struct embedded_loopback_stats *);
enum embedded_loopback_result embedded_loopback_run_once_config(
	const uint8_t *, size_t, uint8_t *, size_t, size_t *,
	const struct embedded_loopback_config *,
	struct embedded_loopback_stats *);

#endif
