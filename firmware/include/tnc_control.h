/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/include/tnc_control.h */

#ifndef TNC_CONTROL_H
#define TNC_CONTROL_H

#include <sys/types.h>

#include <stdbool.h>
#include <stdint.h>

enum tnc_control_result {
	TNC_CONTROL_OK = 0,
	TNC_CONTROL_ERR_ARG,
	TNC_CONTROL_ERR_BUSY,
	TNC_CONTROL_ERR_DENIED,
	TNC_CONTROL_ERR_TIMEOUT
};

enum tnc_control_ptt {
	TNC_CONTROL_PTT_OFF = 0,
	TNC_CONTROL_PTT_ON
};

enum tnc_control_state {
	TNC_CONTROL_IDLE = 0,
	TNC_CONTROL_WAIT_SLOT,
	TNC_CONTROL_TXDELAY,
	TNC_CONTROL_TX_ACTIVE,
	TNC_CONTROL_TXTAIL
};

struct tnc_control_config {
	uint8_t p;
	uint8_t slottime_10ms;
	uint8_t fullduplex;
	uint32_t txdelay_ms;
	uint32_t txtail_ms;
	uint32_t max_tx_ms;
	uint32_t rng_seed;
};

struct tnc_control_stats {
	size_t tx_requests;
	size_t tx_grants;
	size_t tx_denied_busy;
	size_t tx_persistence_deferrals;
	size_t tx_timeouts;
	size_t tx_aborts;
	size_t ptt_on_events;
	size_t ptt_off_events;
};

struct tnc_control {
	struct tnc_control_config config;
	struct tnc_control_stats stats;
	enum tnc_control_state state;
	enum tnc_control_ptt ptt;
	uint32_t rng_state;
	uint32_t slot_wait_ticks;
	uint32_t txdelay_ticks;
	uint32_t txtail_ticks;
	uint32_t max_tx_ticks;
	bool dcd_busy;
};

enum tnc_control_result tnc_control_abort_tx(struct tnc_control *);
enum tnc_control_result tnc_control_can_emit_audio(
	const struct tnc_control *, int *);
enum tnc_control_result tnc_control_complete_tx(struct tnc_control *);
enum tnc_control_result tnc_control_init(struct tnc_control *,
	const struct tnc_control_config *);
enum tnc_control_result tnc_control_ptt_state(const struct tnc_control *,
	enum tnc_control_ptt *);
enum tnc_control_result tnc_control_request_tx(struct tnc_control *);
enum tnc_control_result tnc_control_set_dcd(struct tnc_control *, int);
enum tnc_control_result tnc_control_stats(const struct tnc_control *,
	struct tnc_control_stats *);
enum tnc_control_result tnc_control_tick_10ms(struct tnc_control *);

#endif
