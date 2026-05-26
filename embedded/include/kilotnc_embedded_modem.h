/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/include/kilotnc_embedded_modem.h */

#ifndef KILOTNC_EMBEDDED_MODEM_H
#define KILOTNC_EMBEDDED_MODEM_H

#include <sys/types.h>

#include <stdint.h>

#include "afsk1200_tx.h"
#include "kilotnc_audio.h"
#include "tnc_mode.h"

#define EMBEDDED_MODEM_TX_CHUNK_MAX 64U

enum embedded_modem_result {
	EMBEDDED_MODEM_OK = 0,
	EMBEDDED_MODEM_DONE,
	EMBEDDED_MODEM_ERR_ARG,
	EMBEDDED_MODEM_ERR_BUSY,
	EMBEDDED_MODEM_ERR_SMALL,
	EMBEDDED_MODEM_ERR_UNSUPPORTED,
	EMBEDDED_MODEM_ERR_AUDIO
};

struct embedded_modem_status {
	uint8_t tx_active;
	uint8_t tx_done;
	enum tnc_mode_id current_mode;
	size_t tx_frames_started;
	size_t tx_frames_rejected;
	size_t tx_frames_done;
	size_t tx_samples_generated;
	size_t tx_audio_errors;
	size_t tx_audio_overflows;
	size_t tx_underflows;
	size_t aborts;
};

struct embedded_modem {
	struct afsk1200_tx tx;
	struct embedded_modem_status status;
};

enum embedded_modem_result embedded_modem_abort(struct embedded_modem *);
enum embedded_modem_result embedded_modem_init(struct embedded_modem *);
enum embedded_modem_result embedded_modem_process_tx(struct embedded_modem *,
	const struct kilotnc_audio *, size_t);
enum embedded_modem_result embedded_modem_start_ax25(struct embedded_modem *,
	const uint8_t *, size_t, enum tnc_mode_id);
enum embedded_modem_result embedded_modem_status(const struct embedded_modem *,
	struct embedded_modem_status *);

#endif
