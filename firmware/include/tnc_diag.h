/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/include/tnc_diag.h */

#ifndef TNC_DIAG_H
#define TNC_DIAG_H

#include <sys/types.h>

#include <stdint.h>

#include "tnc1200.h"

#define TNC_DIAG_FAULT_RING	16U

enum tnc_diag_result {
	TNC_DIAG_OK = 0,
	TNC_DIAG_ERR_ARG,
	TNC_DIAG_ERR_SMALL,
	TNC_DIAG_ERR_RANGE
};

enum tnc_diag_fault {
	TNC_DIAG_FAULT_NONE = 0,
	TNC_DIAG_FAULT_KISS_PARSE,
	TNC_DIAG_FAULT_KISS_OVERLENGTH,
	TNC_DIAG_FAULT_TX_BUSY_DROP,
	TNC_DIAG_FAULT_RX_BAD_FCS,
	TNC_DIAG_FAULT_RX_MALFORMED,
	TNC_DIAG_FAULT_RX_OVERSIZE,
	TNC_DIAG_FAULT_RX_OUTPUT_DROP,
	TNC_DIAG_FAULT_TX_TIMEOUT,
	TNC_DIAG_FAULT_TX_ABORT,
	TNC_DIAG_FAULT_AUDIO_UNDERRUN,
	TNC_DIAG_FAULT_AUDIO_OVERRUN
};

struct tnc_diag_snapshot {
	size_t kiss_frames_in;
	size_t kiss_frames_out;
	size_t kiss_parse_errors;
	size_t kiss_ignored_commands;
	size_t tx_frames_started;
	size_t tx_frames_done;
	size_t tx_frames_rejected;
	size_t tx_samples_out;
	size_t rx_frames_ok;
	size_t rx_frames_bad_fcs;
	size_t rx_frames_malformed;
	size_t rx_frames_dropped;
	size_t rx_samples_in;
	size_t channel_tx_requests;
	size_t channel_tx_grants;
	size_t channel_tx_denied_busy;
	size_t channel_tx_persistence_deferrals;
	size_t channel_tx_timeouts;
	size_t channel_tx_aborts;
	size_t ptt_on_events;
	size_t ptt_off_events;
	uint16_t rx_dcd_score;
	uint16_t rx_confidence_avg;
	uint8_t p;
	uint8_t slottime_10ms;
	uint8_t fullduplex;
	uint8_t ptt_state;
	uint8_t tx_active;
	uint8_t audio_ready;
	uint8_t dcd_busy;
	enum tnc_diag_fault last_fault;
};

struct tnc_diag {
	struct tnc_diag_snapshot snapshot;
	enum tnc_diag_fault fault_ring[TNC_DIAG_FAULT_RING];
	size_t fault_head;
	size_t fault_count;
};

enum tnc_diag_result tnc_diag_capture_tnc1200(struct tnc_diag *,
	const struct tnc1200 *);
enum tnc_diag_result tnc_diag_faults(const struct tnc_diag *,
	enum tnc_diag_fault *, size_t, size_t *);
enum tnc_diag_result tnc_diag_format_snapshot(
	const struct tnc_diag_snapshot *, char *, size_t, size_t *);
enum tnc_diag_result tnc_diag_init(struct tnc_diag *);
enum tnc_diag_result tnc_diag_record_fault(struct tnc_diag *,
	enum tnc_diag_fault);
enum tnc_diag_result tnc_diag_snapshot(const struct tnc_diag *,
	struct tnc_diag_snapshot *);

#endif
