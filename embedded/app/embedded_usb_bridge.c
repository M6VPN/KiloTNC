/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/app/embedded_usb_bridge.c */

#include <sys/types.h>

#include <stdint.h>
#include <string.h>

#include "embedded_usb_bridge.h"

#define EMBEDDED_USB_BRIDGE_READ_MAX 64U
#define EMBEDDED_USB_BRIDGE_FRAME_CAP 4U
#define EMBEDDED_USB_BRIDGE_ENCODE_MAX \
	((KILOTNC_KISS_MAX_FRAME * 2U) + 2U)

static enum embedded_usb_bridge_result embedded_usb_bridge_kiss_loopback(
	struct embedded_usb_bridge *, const uint8_t *, size_t);
static enum embedded_usb_bridge_result embedded_usb_bridge_write(
	struct embedded_usb_bridge *, const uint8_t *, size_t);
static int embedded_usb_bridge_usb_ready(const struct kilotnc_usb_cdc *);
static void embedded_usb_bridge_update_kiss_stats(
	struct embedded_usb_bridge *);

static enum embedded_usb_bridge_result
embedded_usb_bridge_kiss_loopback(struct embedded_usb_bridge *bridge,
	const uint8_t *buf, size_t len)
{
	struct kiss_frame frames[EMBEDDED_USB_BRIDGE_FRAME_CAP];
	uint8_t encoded[EMBEDDED_USB_BRIDGE_ENCODE_MAX];
	size_t frame_count;
	size_t out_len;
	size_t i;

	if (kiss_parse_bytes(&bridge->parser, buf, len, frames,
	    EMBEDDED_USB_BRIDGE_FRAME_CAP, &frame_count) != KISS_OK) {
		bridge->stats.usb_errors++;
		embedded_usb_bridge_update_kiss_stats(bridge);
		return EMBEDDED_USB_BRIDGE_ERR_SMALL;
	}
	embedded_usb_bridge_update_kiss_stats(bridge);

	for (i = 0; i < frame_count; i++) {
		if (kiss_encode_frame(frames[i].port, KISS_CMD_DATA,
		    frames[i].data, frames[i].len, encoded, sizeof(encoded),
		    &out_len) != KISS_OK) {
			bridge->stats.usb_errors++;
			return EMBEDDED_USB_BRIDGE_ERR_SMALL;
		}
		if (embedded_usb_bridge_write(bridge, encoded, out_len) !=
		    EMBEDDED_USB_BRIDGE_OK)
			return EMBEDDED_USB_BRIDGE_ERR_USB;
		bridge->stats.kiss_frames_out++;
	}

	return EMBEDDED_USB_BRIDGE_OK;
}

static enum embedded_usb_bridge_result
embedded_usb_bridge_write(struct embedded_usb_bridge *bridge,
	const uint8_t *buf, size_t len)
{
	enum kilotnc_usb_result result;
	size_t written;

	result = bridge->usb->write(bridge->usb->ctx, buf, len, &written);
	if (result == KILOTNC_USB_ERR_WOULD_BLOCK) {
		bridge->stats.usb_would_block++;
		return EMBEDDED_USB_BRIDGE_OK;
	}
	if (result != KILOTNC_USB_OK || written != len) {
		bridge->stats.usb_errors++;
		return EMBEDDED_USB_BRIDGE_ERR_USB;
	}

	bridge->stats.bytes_out += written;
	return EMBEDDED_USB_BRIDGE_OK;
}

static int
embedded_usb_bridge_usb_ready(const struct kilotnc_usb_cdc *usb)
{
	if (usb == NULL)
		return 0;
	if (usb->read == NULL)
		return 0;
	if (usb->write == NULL)
		return 0;
	if (usb->connected == NULL)
		return 0;
	if (usb->stats == NULL)
		return 0;

	return 1;
}

static void
embedded_usb_bridge_update_kiss_stats(struct embedded_usb_bridge *bridge)
{
	bridge->stats.kiss_frames_in = bridge->parser.counters.decoded_frames;
	bridge->stats.kiss_parse_errors = bridge->parser.counters.parse_errors;
	bridge->stats.kiss_overlength = bridge->parser.counters.overlength_frames;
	bridge->stats.kiss_ignored_commands =
	    bridge->parser.counters.ignored_commands;
}

enum embedded_usb_bridge_result
embedded_usb_bridge_init(struct embedded_usb_bridge *bridge,
	const struct kilotnc_usb_cdc *usb, enum embedded_usb_bridge_mode mode)
{
	if (bridge == NULL || !embedded_usb_bridge_usb_ready(usb))
		return EMBEDDED_USB_BRIDGE_ERR_ARG;
	if (mode != EMBEDDED_USB_BRIDGE_ECHO &&
	    mode != EMBEDDED_USB_BRIDGE_KISS_LOOPBACK)
		return EMBEDDED_USB_BRIDGE_ERR_ARG;

	(void)memset(bridge, 0, sizeof(*bridge));
	bridge->usb = usb;
	bridge->mode = mode;
	kiss_parser_init(&bridge->parser);
	return EMBEDDED_USB_BRIDGE_OK;
}

enum embedded_usb_bridge_result
embedded_usb_bridge_service(struct embedded_usb_bridge *bridge)
{
	uint8_t buf[EMBEDDED_USB_BRIDGE_READ_MAX];
	enum kilotnc_usb_result usb_result;
	size_t read_len;
	int connected;

	if (bridge == NULL || !embedded_usb_bridge_usb_ready(bridge->usb))
		return EMBEDDED_USB_BRIDGE_ERR_ARG;

	if (bridge->usb->connected(bridge->usb->ctx, &connected) !=
	    KILOTNC_USB_OK) {
		bridge->stats.usb_errors++;
		return EMBEDDED_USB_BRIDGE_ERR_USB;
	}
	if (connected == 0) {
		bridge->stats.usb_would_block++;
		return EMBEDDED_USB_BRIDGE_OK;
	}

	usb_result = bridge->usb->read(bridge->usb->ctx, buf, sizeof(buf),
	    &read_len);
	if (usb_result == KILOTNC_USB_ERR_WOULD_BLOCK) {
		bridge->stats.usb_would_block++;
		return EMBEDDED_USB_BRIDGE_OK;
	}
	if (usb_result != KILOTNC_USB_OK) {
		bridge->stats.usb_errors++;
		return EMBEDDED_USB_BRIDGE_ERR_USB;
	}

	bridge->stats.bytes_in += read_len;
	if (bridge->mode == EMBEDDED_USB_BRIDGE_ECHO) {
		bridge->stats.echo_chunks++;
		return embedded_usb_bridge_write(bridge, buf, read_len);
	}

	return embedded_usb_bridge_kiss_loopback(bridge, buf, read_len);
}

enum embedded_usb_bridge_result
embedded_usb_bridge_stats(const struct embedded_usb_bridge *bridge,
	struct embedded_usb_bridge_stats *stats)
{
	if (bridge == NULL || stats == NULL)
		return EMBEDDED_USB_BRIDGE_ERR_ARG;

	*stats = bridge->stats;
	return EMBEDDED_USB_BRIDGE_OK;
}
