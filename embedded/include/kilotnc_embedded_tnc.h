/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/include/kilotnc_embedded_tnc.h */

#ifndef KILOTNC_EMBEDDED_TNC_H
#define KILOTNC_EMBEDDED_TNC_H

#include <sys/types.h>

#include <stdint.h>

#include "kiss.h"
#include "kilotnc_usb_cdc.h"
#include "tnc_control.h"
#include "tnc_mode.h"

#define EMBEDDED_TNC_USB_READ_MAX 64U
#define EMBEDDED_TNC_FRAME_CAP 4U
#define EMBEDDED_TNC_ENCODE_MAX ((KILOTNC_KISS_MAX_FRAME * 2U) + 2U)

enum embedded_tnc_result {
	EMBEDDED_TNC_OK = 0,
	EMBEDDED_TNC_ERR_ARG,
	EMBEDDED_TNC_ERR_SMALL,
	EMBEDDED_TNC_ERR_USB,
	EMBEDDED_TNC_ERR_MODE
};

struct embedded_tnc_status {
	enum tnc_mode_id current_mode;
	enum tnc_mode_id last_requested_mode;
	uint8_t last_nino_sethw;
	uint8_t last_mode_temporary;
	uint8_t txdelay;
	uint8_t p;
	uint8_t slottime;
	uint8_t txtail;
	uint8_t fullduplex;
	uint8_t loopback_enabled;
	uint8_t modem_tx_enabled;
	uint8_t modem_tx_inhibited;
	uint8_t ptt_state;
	size_t kiss_frames_in;
	size_t kiss_frames_out;
	size_t kiss_parse_errors;
	size_t kiss_overlength_frames;
	size_t kiss_ignored_commands;
	size_t mode_set_requests;
	size_t unsupported_mode_requests;
	size_t invalid_mode_requests;
	size_t modem_tx_requests;
	size_t modem_tx_accepted;
	size_t modem_tx_rejected;
	size_t usb_bytes_in;
	size_t usb_bytes_out;
	size_t usb_would_block;
	size_t usb_errors;
};

struct embedded_modem;

struct embedded_tnc {
	struct kiss_parser parser;
	struct tnc_control control;
	struct embedded_modem *modem;
	struct embedded_tnc_status status;
	uint8_t sethw[KILOTNC_SETHW_MAX_PAYLOAD];
	size_t sethw_len;
};

enum embedded_tnc_result embedded_tnc_init(struct embedded_tnc *);
enum embedded_tnc_result embedded_tnc_modem(struct embedded_tnc *,
	struct embedded_modem *);
enum embedded_tnc_result embedded_tnc_process_usb(struct embedded_tnc *,
	const struct kilotnc_usb_cdc *);
enum embedded_tnc_result embedded_tnc_set_loopback(struct embedded_tnc *,
	int);
enum embedded_tnc_result embedded_tnc_set_modem_tx(struct embedded_tnc *,
	int);
enum embedded_tnc_result embedded_tnc_status(const struct embedded_tnc *,
	struct embedded_tnc_status *);

#endif
