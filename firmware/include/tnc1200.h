/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/include/tnc1200.h */

#ifndef TNC1200_H
#define TNC1200_H

#include <sys/types.h>

#include <stdbool.h>
#include <stdint.h>

#include "afsk1200_stream.h"
#include "afsk1200_tx.h"
#include "kiss.h"
#include "tnc_control.h"
#include "tnc_mode.h"

enum tnc1200_result {
	TNC1200_OK = 0,
	TNC1200_ERR_ARG,
	TNC1200_ERR_SMALL,
	TNC1200_ERR_BUSY,
	TNC1200_ERR_NO_DATA,
	TNC1200_ERR_FRAME_DROPPED,
	TNC1200_ERR_TIMEOUT
};

struct tnc1200_config {
	size_t txdelay_flags;
	size_t txtail_flags;
	int16_t amplitude;
	uint8_t p;
	uint8_t slottime_10ms;
	uint8_t fullduplex;
	uint32_t max_tx_ms;
	uint32_t rng_seed;
};

struct tnc1200_stats {
	size_t kiss_frames_in;
	size_t kiss_frames_out;
	size_t kiss_parse_errors;
	size_t kiss_ignored_commands;
	size_t tx_frames_started;
	size_t tx_frames_done;
	size_t tx_frames_rejected;
	size_t rx_frames_ok;
	size_t rx_frames_bad_fcs;
	size_t rx_frames_malformed;
	size_t rx_frames_dropped;
	size_t pcm_samples_in;
	size_t pcm_samples_out;
	size_t channel_tx_requests;
	size_t channel_tx_grants;
	size_t channel_tx_denied_busy;
	size_t channel_tx_persistence_deferrals;
	size_t channel_tx_timeouts;
	size_t channel_tx_aborts;
	size_t ptt_on_events;
	size_t ptt_off_events;
	size_t mode_set_requests;
	size_t mode_set_unsupported;
	size_t mode_set_invalid;
};

struct tnc1200_status {
	uint16_t rx_dcd_score;
	uint16_t rx_confidence_avg;
	uint8_t p;
	uint8_t slottime_10ms;
	uint8_t fullduplex;
	uint8_t ptt_state;
	uint8_t tx_active;
	uint8_t audio_ready;
	uint8_t dcd_busy;
	uint8_t last_nino_sethw;
	uint8_t last_mode_temporary;
	enum tnc_mode_id current_mode;
	enum tnc_mode_id last_requested_mode;
};

struct tnc1200 {
	struct kiss_parser kiss;
	struct afsk1200_tx tx;
	struct afsk1200_stream rx;
	struct afsk1200_tx_config tx_config;
	struct tnc_control control;
	struct tnc_control_config control_config;
	struct tnc1200_stats stats;
	uint8_t pending_frame[KILOTNC_AX25_MAX_FRAME];
	size_t pending_frame_len;
	bool pending_frame_valid;
	bool tx_started;
	enum tnc_mode_id current_mode;
	enum tnc_mode_id last_requested_mode;
	uint8_t p;
	uint8_t slottime;
	uint8_t fullduplex;
	uint8_t last_nino_sethw;
	uint8_t last_mode_temporary;
	uint8_t sethw[KILOTNC_SETHW_MAX_PAYLOAD];
	size_t sethw_len;
};

enum tnc1200_result tnc1200_abort_tx(struct tnc1200 *);
enum tnc1200_result tnc1200_can_emit_audio(const struct tnc1200 *, int *);
enum tnc1200_result tnc1200_host_input(struct tnc1200 *, const uint8_t *,
	size_t);
enum tnc1200_result tnc1200_init(struct tnc1200 *,
	const struct tnc1200_config *);
enum tnc1200_result tnc1200_rx_process(struct tnc1200 *, const int16_t *,
	size_t, uint8_t *, size_t, size_t *);
enum tnc1200_result tnc1200_ptt_state(const struct tnc1200 *,
	enum tnc_control_ptt *);
enum tnc1200_result tnc1200_set_dcd(struct tnc1200 *, int);
enum tnc1200_result tnc1200_status(const struct tnc1200 *,
	struct tnc1200_status *);
enum tnc1200_result tnc1200_stats(const struct tnc1200 *,
	struct tnc1200_stats *);
enum tnc1200_result tnc1200_tx_process(struct tnc1200 *, int16_t *, size_t,
	size_t *);

#endif
