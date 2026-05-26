/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/include/tnc1200.h */

#ifndef TNC1200_H
#define TNC1200_H

#include <sys/types.h>

#include <stdint.h>

#include "afsk1200_stream.h"
#include "afsk1200_tx.h"
#include "kiss.h"

enum tnc1200_result {
	TNC1200_OK = 0,
	TNC1200_ERR_ARG,
	TNC1200_ERR_SMALL,
	TNC1200_ERR_BUSY,
	TNC1200_ERR_NO_DATA,
	TNC1200_ERR_FRAME_DROPPED
};

struct tnc1200_config {
	size_t txdelay_flags;
	size_t txtail_flags;
	int16_t amplitude;
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
};

struct tnc1200 {
	struct kiss_parser kiss;
	struct afsk1200_tx tx;
	struct afsk1200_stream rx;
	struct afsk1200_tx_config tx_config;
	struct tnc1200_stats stats;
	uint8_t p;
	uint8_t slottime;
	uint8_t fullduplex;
	uint8_t sethw[KILOTNC_SETHW_MAX_PAYLOAD];
	size_t sethw_len;
};

enum tnc1200_result tnc1200_abort_tx(struct tnc1200 *);
enum tnc1200_result tnc1200_host_input(struct tnc1200 *, const uint8_t *,
	size_t);
enum tnc1200_result tnc1200_init(struct tnc1200 *,
	const struct tnc1200_config *);
enum tnc1200_result tnc1200_rx_process(struct tnc1200 *, const int16_t *,
	size_t, uint8_t *, size_t, size_t *);
enum tnc1200_result tnc1200_stats(const struct tnc1200 *,
	struct tnc1200_stats *);
enum tnc1200_result tnc1200_tx_process(struct tnc1200 *, int16_t *, size_t,
	size_t *);

#endif
