/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/include/kilotnc_embedded_diag.h */

#ifndef KILOTNC_EMBEDDED_DIAG_H
#define KILOTNC_EMBEDDED_DIAG_H

#include <sys/types.h>

#include <stdint.h>

struct embedded_app;

enum embedded_diag_result {
	EMBEDDED_DIAG_OK = 0,
	EMBEDDED_DIAG_ERR_ARG,
	EMBEDDED_DIAG_ERR_SMALL
};

struct embedded_diag_snapshot {
	uint32_t app_steps;
	uint32_t app_faults;
	uint32_t platform_ticks;
	uint32_t watchdog_kicks;
	uint32_t diagnostics_writes;
	uint32_t scheduler_cycles;
	uint32_t scheduler_faults;
	uint32_t scheduler_enabled_mask;
	uint32_t scheduler_required_mask;
	uint32_t scheduler_progress_mask;
	uint32_t usb_rx_bytes;
	uint32_t usb_tx_bytes;
	uint32_t usb_rx_overflows;
	uint32_t usb_tx_overflows;
	uint32_t kiss_frames_in;
	uint32_t kiss_frames_out;
	uint32_t kiss_parse_errors;
	uint32_t kiss_ignored_commands;
	uint32_t kiss_overlength_frames;
	uint32_t audio_rx_samples;
	uint32_t audio_tx_samples;
	uint32_t audio_rx_overflows;
	uint32_t audio_tx_overflows;
	uint32_t audio_rx_underflows;
	uint32_t audio_tx_underflows;
	uint32_t audio_loopback_blocks;
	uint32_t tnc_kiss_frames_in;
	uint32_t tnc_kiss_frames_out;
	uint32_t tnc_kiss_parse_errors;
	uint32_t tnc_kiss_ignored_commands;
	uint32_t tnc_mode_set_requests;
	uint32_t tnc_mode_unsupported;
	uint32_t tnc_mode_invalid;
	uint32_t config_schema_version;
	uint32_t config_requested_mode;
	uint32_t config_max_tx_ms;
	uint32_t config_validation_errors;
	uint32_t config_persistence_unsupported;
	uint32_t tnc_modem_tx_requests;
	uint32_t tnc_modem_tx_accepted;
	uint32_t tnc_modem_tx_rejected;
	uint32_t tnc_modem_rx_kiss_frames;
	uint32_t tnc_modem_rx_output_drops;
	uint32_t modem_tx_frames_started;
	uint32_t modem_tx_frames_rejected;
	uint32_t modem_tx_frames_done;
	uint32_t modem_tx_samples_generated;
	uint32_t modem_tx_audio_errors;
	uint32_t modem_rx_frames_ok;
	uint32_t modem_rx_bad_fcs;
	uint32_t modem_rx_malformed;
	uint32_t modem_rx_dropped;
	uint32_t modem_rx_samples_consumed;
	uint32_t modem_rx_audio_errors;
	uint32_t modem_rx_audio_underflows;
	uint32_t modem_rx_audio_overflows;
	uint32_t modem_aborts;
	uint8_t tnc_current_mode;
	uint8_t tnc_txdelay;
	uint8_t tnc_p;
	uint8_t tnc_slottime;
	uint8_t tnc_txtail;
	uint8_t tnc_fullduplex;
	uint8_t tnc_loopback_enabled;
	uint8_t tnc_modem_tx_enabled;
	uint8_t tnc_modem_rx_enabled;
	uint8_t config_temporary;
	uint8_t modem_tx_active;
	uint8_t modem_rx_active;
	uint8_t modem_current_mode;
	uint8_t app_state;
	uint8_t reset_cause;
	uint8_t ptt_state;
	uint8_t scheduler_watchdog_allowed;
	uint8_t scheduler_last_failed_task;
	uint8_t usb_connected;
};

enum embedded_diag_result embedded_diag_capture(const struct embedded_app *,
	struct embedded_diag_snapshot *);
enum embedded_diag_result embedded_diag_format(
	const struct embedded_diag_snapshot *, char *, size_t, size_t *);

#endif
