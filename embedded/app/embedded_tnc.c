/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/app/embedded_tnc.c */

#include <sys/types.h>

#include <stdint.h>
#include <string.h>

#include "embedded_modem.h"
#include "embedded_tnc.h"

static enum embedded_tnc_result embedded_tnc_apply_commands(
	struct embedded_tnc *);
static enum embedded_tnc_result embedded_tnc_apply_mode(
	struct embedded_tnc *, uint8_t);
static enum embedded_tnc_result embedded_tnc_loopback_frame(
	struct embedded_tnc *, const struct kilotnc_usb_cdc *,
	const struct kiss_frame *);
static enum embedded_tnc_result embedded_tnc_modem_frame(
	struct embedded_tnc *, const struct kiss_frame *);
static enum embedded_tnc_result embedded_tnc_parse_input(
	struct embedded_tnc *, const struct kilotnc_usb_cdc *,
	const uint8_t *, size_t);
static void embedded_tnc_sync_kiss_stats(struct embedded_tnc *);
static int embedded_tnc_usb_ready(const struct kilotnc_usb_cdc *);

static enum embedded_tnc_result
embedded_tnc_apply_commands(struct embedded_tnc *tnc)
{
	if (tnc->parser.txdelay != tnc->status.txdelay) {
		tnc->status.txdelay = tnc->parser.txdelay;
		tnc->control.config.txdelay_ms =
		    (uint32_t)tnc->status.txdelay * 10U;
	}
	if (tnc->parser.p != tnc->status.p) {
		tnc->status.p = tnc->parser.p;
		tnc->control.config.p = tnc->status.p;
	}
	if (tnc->parser.slottime != tnc->status.slottime) {
		tnc->status.slottime = tnc->parser.slottime;
		tnc->control.config.slottime_10ms = tnc->status.slottime;
	}
	if (tnc->parser.txtail != tnc->status.txtail) {
		tnc->status.txtail = tnc->parser.txtail;
		tnc->control.config.txtail_ms =
		    (uint32_t)tnc->status.txtail * 10U;
	}
	if ((uint8_t)(tnc->parser.fullduplex != 0) !=
	    tnc->status.fullduplex) {
		tnc->status.fullduplex =
		    (uint8_t)(tnc->parser.fullduplex != 0);
		tnc->control.config.fullduplex = tnc->status.fullduplex;
	}
	if (tnc->parser.sethw_len != 0U) {
		if (embedded_tnc_apply_mode(tnc, tnc->parser.sethw[0]) !=
		    EMBEDDED_TNC_OK)
			return EMBEDDED_TNC_ERR_MODE;
		(void)memcpy(tnc->sethw, tnc->parser.sethw,
		    tnc->parser.sethw_len);
		tnc->sethw_len = tnc->parser.sethw_len;
		tnc->parser.sethw_len = 0U;
	}

	return EMBEDDED_TNC_OK;
}

static enum embedded_tnc_result
embedded_tnc_apply_mode(struct embedded_tnc *tnc, uint8_t value)
{
	const struct tnc_mode_desc *desc;
	enum tnc_mode_id requested;
	enum tnc_mode_result mode_result;
	int temporary;

	tnc->status.mode_set_requests++;
	tnc->status.last_nino_sethw = value;
	mode_result = tnc_mode_from_nino_sethw(value, &requested, &temporary);
	if (mode_result != TNC_MODE_OK) {
		tnc->status.invalid_mode_requests++;
		tnc->status.modem_tx_inhibited = 1U;
		tnc->status.modem_rx_inhibited = 1U;
		return EMBEDDED_TNC_OK;
	}

	tnc->status.last_requested_mode = requested;
	tnc->status.last_mode_temporary = temporary != 0;
	if (tnc_mode_get(requested, &desc) != TNC_MODE_OK ||
	    desc->support != TNC_MODE_SUPPORT_IMPLEMENTED) {
		tnc->status.unsupported_mode_requests++;
		tnc->status.modem_tx_inhibited = 1U;
		tnc->status.modem_rx_inhibited = 1U;
		return EMBEDDED_TNC_OK;
	}

	tnc->status.current_mode = requested;
	tnc->status.modem_tx_inhibited = 0U;
	tnc->status.modem_rx_inhibited = 0U;
	return EMBEDDED_TNC_OK;
}

static enum embedded_tnc_result
embedded_tnc_loopback_frame(struct embedded_tnc *tnc,
	const struct kilotnc_usb_cdc *usb, const struct kiss_frame *frame)
{
	uint8_t encoded[EMBEDDED_TNC_ENCODE_MAX];
	enum kilotnc_usb_result usb_result;
	size_t encoded_len;
	size_t written;

	if (kiss_encode_frame(frame->port, KISS_CMD_DATA, frame->data,
	    frame->len, encoded, sizeof(encoded), &encoded_len) != KISS_OK)
		return EMBEDDED_TNC_ERR_SMALL;

	usb_result = usb->write(usb->ctx, encoded, encoded_len, &written);
	if (usb_result == KILOTNC_USB_ERR_WOULD_BLOCK) {
		tnc->status.usb_would_block++;
		return EMBEDDED_TNC_OK;
	}
	if (usb_result != KILOTNC_USB_OK || written != encoded_len) {
		tnc->status.usb_errors++;
		return EMBEDDED_TNC_ERR_USB;
	}

	tnc->status.usb_bytes_out += written;
	tnc->status.kiss_frames_out++;
	return EMBEDDED_TNC_OK;
}

static enum embedded_tnc_result
embedded_tnc_modem_frame(struct embedded_tnc *tnc,
	const struct kiss_frame *frame)
{
	enum embedded_modem_result result;

	if (tnc->status.modem_tx_enabled == 0U)
		return EMBEDDED_TNC_OK;

	tnc->status.modem_tx_requests++;
	if (tnc->status.modem_tx_inhibited != 0U) {
		tnc->status.modem_tx_rejected++;
		return EMBEDDED_TNC_OK;
	}
	if (tnc->modem == NULL) {
		tnc->status.modem_tx_rejected++;
		return EMBEDDED_TNC_OK;
	}

	result = embedded_modem_start_ax25(tnc->modem, frame->data,
	    frame->len, tnc->status.current_mode);
	if (result == EMBEDDED_MODEM_OK) {
		tnc->status.modem_tx_accepted++;
		return EMBEDDED_TNC_OK;
	}

	tnc->status.modem_tx_rejected++;
	return EMBEDDED_TNC_OK;
}

static enum embedded_tnc_result
embedded_tnc_parse_input(struct embedded_tnc *tnc,
	const struct kilotnc_usb_cdc *usb, const uint8_t *buf, size_t len)
{
	struct kiss_frame frames[EMBEDDED_TNC_FRAME_CAP];
	size_t frame_count;
	size_t i;

	if (kiss_parse_bytes(&tnc->parser, buf, len, frames,
	    EMBEDDED_TNC_FRAME_CAP, &frame_count) != KISS_OK) {
		embedded_tnc_sync_kiss_stats(tnc);
		return EMBEDDED_TNC_ERR_SMALL;
	}
	if (embedded_tnc_apply_commands(tnc) != EMBEDDED_TNC_OK)
		return EMBEDDED_TNC_ERR_MODE;

	embedded_tnc_sync_kiss_stats(tnc);

	for (i = 0U; i < frame_count; i++) {
		if (embedded_tnc_modem_frame(tnc, &frames[i]) !=
		    EMBEDDED_TNC_OK)
			return EMBEDDED_TNC_ERR_MODE;
		if (tnc->status.loopback_enabled != 0U &&
		    embedded_tnc_loopback_frame(tnc, usb, &frames[i]) !=
		    EMBEDDED_TNC_OK)
			return EMBEDDED_TNC_ERR_USB;
	}

	return EMBEDDED_TNC_OK;
}

static void
embedded_tnc_sync_kiss_stats(struct embedded_tnc *tnc)
{
	tnc->status.kiss_frames_in = tnc->parser.counters.decoded_frames;
	tnc->status.kiss_parse_errors = tnc->parser.counters.parse_errors;
	tnc->status.kiss_overlength_frames =
	    tnc->parser.counters.overlength_frames;
	tnc->status.kiss_ignored_commands =
	    tnc->parser.counters.ignored_commands;
}

static int
embedded_tnc_usb_ready(const struct kilotnc_usb_cdc *usb)
{
	if (usb == NULL)
		return 0;
	if (usb->read == NULL)
		return 0;
	if (usb->write == NULL)
		return 0;
	if (usb->connected == NULL)
		return 0;

	return 1;
}

enum embedded_tnc_result
embedded_tnc_emit_modem_rx(struct embedded_tnc *tnc,
	const struct kilotnc_usb_cdc *usb,
	const struct embedded_modem_rx_frame *frames, size_t frame_count)
{
	uint8_t encoded[EMBEDDED_TNC_ENCODE_MAX];
	enum kilotnc_usb_result usb_result;
	size_t encoded_len;
	size_t written;
	size_t i;

	if (tnc == NULL || !embedded_tnc_usb_ready(usb) || frames == NULL)
		return EMBEDDED_TNC_ERR_ARG;
	if (tnc->status.modem_rx_enabled == 0U ||
	    tnc->status.modem_rx_inhibited != 0U)
		return EMBEDDED_TNC_OK;

	for (i = 0U; i < frame_count; i++) {
		if (kiss_encode_frame(0, KISS_CMD_DATA, frames[i].data,
		    frames[i].len, encoded, sizeof(encoded), &encoded_len) !=
		    KISS_OK) {
			tnc->status.modem_rx_output_drops++;
			continue;
		}
		usb_result = usb->write(usb->ctx, encoded, encoded_len,
		    &written);
		if (usb_result == KILOTNC_USB_ERR_WOULD_BLOCK ||
		    usb_result == KILOTNC_USB_ERR_SMALL) {
			tnc->status.modem_rx_output_drops++;
			continue;
		}
		if (usb_result != KILOTNC_USB_OK || written != encoded_len) {
			tnc->status.usb_errors++;
			tnc->status.modem_rx_output_drops++;
			continue;
		}
		tnc->status.usb_bytes_out += written;
		tnc->status.modem_rx_kiss_frames++;
	}

	return EMBEDDED_TNC_OK;
}

enum embedded_tnc_result
embedded_tnc_init(struct embedded_tnc *tnc)
{
	struct tnc_control_config config;
	enum tnc_mode_id mode;

	if (tnc == NULL)
		return EMBEDDED_TNC_ERR_ARG;

	(void)memset(tnc, 0, sizeof(*tnc));
	kiss_parser_init(&tnc->parser);
	(void)memset(&config, 0, sizeof(config));
	config.p = tnc->parser.p;
	config.slottime_10ms = tnc->parser.slottime;
	config.txdelay_ms = (uint32_t)tnc->parser.txdelay * 10U;
	config.txtail_ms = (uint32_t)tnc->parser.txtail * 10U;
	config.max_tx_ms = 30000U;
	config.rng_seed = 1U;
	if (tnc_control_init(&tnc->control, &config) != TNC_CONTROL_OK)
		return EMBEDDED_TNC_ERR_ARG;
	if (tnc_mode_default(&mode) != TNC_MODE_OK)
		return EMBEDDED_TNC_ERR_MODE;

	tnc->status.current_mode = mode;
	tnc->status.last_requested_mode = mode;
	tnc->status.last_nino_sethw = TNC_MODE_NINO_NONE;
	tnc->status.txdelay = tnc->parser.txdelay;
	tnc->status.p = tnc->parser.p;
	tnc->status.slottime = tnc->parser.slottime;
	tnc->status.txtail = tnc->parser.txtail;
	tnc->status.fullduplex = (uint8_t)(tnc->parser.fullduplex != 0);
	tnc->status.ptt_state = TNC_CONTROL_PTT_OFF;
	return EMBEDDED_TNC_OK;
}

enum embedded_tnc_result
embedded_tnc_modem(struct embedded_tnc *tnc, struct embedded_modem *modem)
{
	if (tnc == NULL || modem == NULL)
		return EMBEDDED_TNC_ERR_ARG;

	tnc->modem = modem;
	return EMBEDDED_TNC_OK;
}

enum embedded_tnc_result
embedded_tnc_process_usb(struct embedded_tnc *tnc,
	const struct kilotnc_usb_cdc *usb)
{
	uint8_t buf[EMBEDDED_TNC_USB_READ_MAX];
	enum kilotnc_usb_result usb_result;
	size_t read_len;
	int connected;

	if (tnc == NULL || !embedded_tnc_usb_ready(usb))
		return EMBEDDED_TNC_ERR_ARG;

	if (usb->connected(usb->ctx, &connected) != KILOTNC_USB_OK) {
		tnc->status.usb_errors++;
		return EMBEDDED_TNC_ERR_USB;
	}
	if (connected == 0) {
		tnc->status.usb_would_block++;
		return EMBEDDED_TNC_OK;
	}

	usb_result = usb->read(usb->ctx, buf, sizeof(buf), &read_len);
	if (usb_result == KILOTNC_USB_ERR_WOULD_BLOCK) {
		tnc->status.usb_would_block++;
		return EMBEDDED_TNC_OK;
	}
	if (usb_result != KILOTNC_USB_OK) {
		tnc->status.usb_errors++;
		return EMBEDDED_TNC_ERR_USB;
	}

	tnc->status.usb_bytes_in += read_len;
	return embedded_tnc_parse_input(tnc, usb, buf, read_len);
}

enum embedded_tnc_result
embedded_tnc_set_loopback(struct embedded_tnc *tnc, int enabled)
{
	if (tnc == NULL)
		return EMBEDDED_TNC_ERR_ARG;

	tnc->status.loopback_enabled = enabled != 0;
	return EMBEDDED_TNC_OK;
}

enum embedded_tnc_result
embedded_tnc_set_modem_rx(struct embedded_tnc *tnc, int enabled)
{
	if (tnc == NULL)
		return EMBEDDED_TNC_ERR_ARG;

	tnc->status.modem_rx_enabled = enabled != 0;
	if (enabled != 0)
		tnc->status.modem_rx_inhibited = 0U;
	return EMBEDDED_TNC_OK;
}

enum embedded_tnc_result
embedded_tnc_set_modem_tx(struct embedded_tnc *tnc, int enabled)
{
	if (tnc == NULL)
		return EMBEDDED_TNC_ERR_ARG;

	tnc->status.modem_tx_enabled = enabled != 0;
	if (enabled != 0)
		tnc->status.modem_tx_inhibited = 0U;
	return EMBEDDED_TNC_OK;
}

enum embedded_tnc_result
embedded_tnc_status(const struct embedded_tnc *tnc,
	struct embedded_tnc_status *status)
{
	if (tnc == NULL || status == NULL)
		return EMBEDDED_TNC_ERR_ARG;

	*status = tnc->status;
	return EMBEDDED_TNC_OK;
}
