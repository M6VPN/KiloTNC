/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/platform/usb_cdc_stub.h */

#ifndef USB_CDC_STUB_H
#define USB_CDC_STUB_H

#include <sys/types.h>

#include <stdint.h>

#include "kilotnc_usb_cdc.h"

#define USB_CDC_STUB_BUFFER_MAX 512

struct usb_cdc_stub {
	struct kilotnc_usb_cdc usb;
	uint8_t rx_buf[USB_CDC_STUB_BUFFER_MAX];
	uint8_t tx_buf[USB_CDC_STUB_BUFFER_MAX];
	size_t rx_len;
	size_t rx_pos;
	size_t tx_len;
	size_t read_calls;
	size_t write_calls;
	size_t connected_calls;
	size_t rx_injected;
	size_t tx_written;
	size_t rx_overflows;
	size_t tx_overflows;
	int connected;
};

enum kilotnc_usb_result usb_cdc_stub_inject_rx(struct usb_cdc_stub *,
	const uint8_t *, size_t);
void usb_cdc_stub_init(struct usb_cdc_stub *);
enum kilotnc_usb_result usb_cdc_stub_set_connected(struct usb_cdc_stub *,
	int);
enum kilotnc_usb_result usb_cdc_stub_take_tx(struct usb_cdc_stub *,
	uint8_t *, size_t, size_t *);
const struct kilotnc_usb_cdc *usb_cdc_stub_usb(struct usb_cdc_stub *);

#endif
