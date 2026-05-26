/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/include/afsk1200_tx.h */

#ifndef AFSK1200_TX_H
#define AFSK1200_TX_H

#include <sys/types.h>

#include <stdbool.h>
#include <stdint.h>

#include "afsk1200.h"
#include "kilotnc_limits.h"

#define AFSK1200_TX_DEFAULT_TXDELAY_FLAGS	16U
#define AFSK1200_TX_DEFAULT_TXTAIL_FLAGS		2U

enum afsk1200_tx_result {
	AFSK1200_TX_OK = 0,
	AFSK1200_TX_ERR_ARG,
	AFSK1200_TX_ERR_SMALL,
	AFSK1200_TX_ERR_BUSY,
	AFSK1200_TX_ERR_BAD_FRAME,
	AFSK1200_TX_DONE
};

enum afsk1200_tx_state {
	AFSK1200_TX_IDLE = 0,
	AFSK1200_TX_PREAMBLE_FLAGS,
	AFSK1200_TX_FRAME_BITS,
	AFSK1200_TX_TAIL_FLAGS,
	AFSK1200_TX_DONE_STATE
};

struct afsk1200_tx_config {
	size_t txdelay_flags;
	size_t txtail_flags;
	int16_t amplitude;
};

struct afsk1200_tx_stats {
	size_t frames_queued;
	size_t frames_done;
	size_t frames_rejected;
	size_t bits_total;
	size_t samples_total;
	size_t chunks_emitted;
	size_t underruns;
};

struct afsk1200_tx {
	struct afsk1200_tx_config config;
	struct afsk1200_tx_stats stats;
	enum afsk1200_tx_state state;
	uint8_t frame_bits[AFSK1200_MAX_TEST_BITS];
	size_t frame_bits_count;
	size_t frame_bits_pos;
	size_t flag_bits_pos;
	size_t current_bit_sample;
	uint8_t nrzi_state;
	uint8_t current_tone;
	bool have_tone;
};

enum afsk1200_tx_result afsk1200_tx_abort(struct afsk1200_tx *);
enum afsk1200_tx_result afsk1200_tx_init(struct afsk1200_tx *,
	const struct afsk1200_tx_config *);
enum afsk1200_tx_result afsk1200_tx_is_active(const struct afsk1200_tx *,
	int *);
enum afsk1200_tx_result afsk1200_tx_process(struct afsk1200_tx *, int16_t *,
	size_t, size_t *);
enum afsk1200_tx_result afsk1200_tx_start_frame(struct afsk1200_tx *,
	const uint8_t *, size_t);
enum afsk1200_tx_result afsk1200_tx_stats(const struct afsk1200_tx *,
	struct afsk1200_tx_stats *);

#endif
