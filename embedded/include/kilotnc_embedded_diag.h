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
	uint8_t tnc_current_mode;
	uint8_t tnc_txdelay;
	uint8_t tnc_p;
	uint8_t tnc_slottime;
	uint8_t tnc_txtail;
	uint8_t tnc_fullduplex;
	uint8_t tnc_loopback_enabled;
	uint8_t app_state;
	uint8_t reset_cause;
	uint8_t ptt_state;
	uint8_t usb_connected;
};

enum embedded_diag_result embedded_diag_capture(const struct embedded_app *,
	struct embedded_diag_snapshot *);
enum embedded_diag_result embedded_diag_format(
	const struct embedded_diag_snapshot *, char *, size_t, size_t *);

#endif
