/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/platform/usb_cdc_stub.c */

#include <sys/types.h>

#include <stdint.h>
#include <string.h>

#include "usb_cdc_stub.h"

static enum kilotnc_usb_result usb_cdc_stub_connected(void *, int *);
static enum kilotnc_usb_result usb_cdc_stub_read(void *, uint8_t *,
	size_t, size_t *);
static enum kilotnc_usb_result usb_cdc_stub_stats(void *,
	struct kilotnc_usb_cdc_stats *);
static enum kilotnc_usb_result usb_cdc_stub_write(void *, const uint8_t *,
	size_t, size_t *);

static enum kilotnc_usb_result
usb_cdc_stub_connected(void *ctx, int *connected)
{
	struct usb_cdc_stub *stub;

	if (ctx == NULL || connected == NULL)
		return KILOTNC_USB_ERR_ARG;

	stub = ctx;
	stub->connected_calls++;
	*connected = stub->connected;
	return KILOTNC_USB_OK;
}

static enum kilotnc_usb_result
usb_cdc_stub_read(void *ctx, uint8_t *buf, size_t buflen, size_t *out_len)
{
	struct usb_cdc_stub *stub;
	size_t available;
	size_t copy_len;

	if (ctx == NULL || buf == NULL || out_len == NULL)
		return KILOTNC_USB_ERR_ARG;
	if (buflen == 0)
		return KILOTNC_USB_ERR_SMALL;

	stub = ctx;
	*out_len = 0;
	stub->read_calls++;
	if (stub->connected == 0)
		return KILOTNC_USB_ERR_WOULD_BLOCK;
	if (stub->rx_pos >= stub->rx_len)
		return KILOTNC_USB_ERR_WOULD_BLOCK;

	available = stub->rx_len - stub->rx_pos;
	copy_len = available;
	if (copy_len > buflen)
		copy_len = buflen;
	(void)memcpy(buf, stub->rx_buf + stub->rx_pos, copy_len);
	stub->rx_pos += copy_len;
	*out_len = copy_len;
	if (stub->rx_pos == stub->rx_len) {
		stub->rx_pos = 0;
		stub->rx_len = 0;
	}

	return KILOTNC_USB_OK;
}

static enum kilotnc_usb_result
usb_cdc_stub_stats(void *ctx, struct kilotnc_usb_cdc_stats *stats)
{
	struct usb_cdc_stub *stub;

	if (ctx == NULL || stats == NULL)
		return KILOTNC_USB_ERR_ARG;

	stub = ctx;
	stats->rx_bytes = stub->rx_injected;
	stats->tx_bytes = stub->tx_written;
	stats->rx_overflows = stub->rx_overflows;
	stats->tx_overflows = stub->tx_overflows;
	stats->connected = stub->connected;
	return KILOTNC_USB_OK;
}

static enum kilotnc_usb_result
usb_cdc_stub_write(void *ctx, const uint8_t *buf, size_t len, size_t *out_len)
{
	struct usb_cdc_stub *stub;

	if (ctx == NULL || buf == NULL || out_len == NULL)
		return KILOTNC_USB_ERR_ARG;

	stub = ctx;
	*out_len = 0;
	stub->write_calls++;
	if (stub->connected == 0)
		return KILOTNC_USB_ERR_WOULD_BLOCK;
	if (len > sizeof(stub->tx_buf) - stub->tx_len) {
		stub->tx_overflows++;
		return KILOTNC_USB_ERR_SMALL;
	}

	(void)memcpy(stub->tx_buf + stub->tx_len, buf, len);
	stub->tx_len += len;
	stub->tx_written += len;
	*out_len = len;
	return KILOTNC_USB_OK;
}

enum kilotnc_usb_result
usb_cdc_stub_inject_rx(struct usb_cdc_stub *stub, const uint8_t *buf,
	size_t len)
{
	if (stub == NULL || buf == NULL)
		return KILOTNC_USB_ERR_ARG;
	if (len > sizeof(stub->rx_buf) - stub->rx_len) {
		stub->rx_overflows++;
		return KILOTNC_USB_ERR_SMALL;
	}

	(void)memcpy(stub->rx_buf + stub->rx_len, buf, len);
	stub->rx_len += len;
	stub->rx_injected += len;
	return KILOTNC_USB_OK;
}

void
usb_cdc_stub_init(struct usb_cdc_stub *stub)
{
	if (stub == NULL)
		return;

	(void)memset(stub, 0, sizeof(*stub));
	stub->usb.ctx = stub;
	stub->usb.read = usb_cdc_stub_read;
	stub->usb.write = usb_cdc_stub_write;
	stub->usb.connected = usb_cdc_stub_connected;
	stub->usb.stats = usb_cdc_stub_stats;
}

enum kilotnc_usb_result
usb_cdc_stub_set_connected(struct usb_cdc_stub *stub, int connected)
{
	if (stub == NULL)
		return KILOTNC_USB_ERR_ARG;

	stub->connected = connected != 0;
	return KILOTNC_USB_OK;
}

enum kilotnc_usb_result
usb_cdc_stub_take_tx(struct usb_cdc_stub *stub, uint8_t *buf, size_t buflen,
	size_t *out_len)
{
	if (stub == NULL || buf == NULL || out_len == NULL)
		return KILOTNC_USB_ERR_ARG;
	if (buflen < stub->tx_len)
		return KILOTNC_USB_ERR_SMALL;

	(void)memcpy(buf, stub->tx_buf, stub->tx_len);
	*out_len = stub->tx_len;
	stub->tx_len = 0;
	return KILOTNC_USB_OK;
}

const struct kilotnc_usb_cdc *
usb_cdc_stub_usb(struct usb_cdc_stub *stub)
{
	if (stub == NULL)
		return NULL;

	return &stub->usb;
}
