/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/include/kilotnc_usb_cdc.h */

#ifndef KILOTNC_USB_CDC_H
#define KILOTNC_USB_CDC_H

#include <sys/types.h>

#include <stdint.h>

enum kilotnc_usb_result {
	KILOTNC_USB_OK = 0,
	KILOTNC_USB_ERR_ARG,
	KILOTNC_USB_ERR_SMALL,
	KILOTNC_USB_ERR_WOULD_BLOCK,
	KILOTNC_USB_ERR_UNSUPPORTED
};

struct kilotnc_usb_cdc_stats {
	size_t rx_bytes;
	size_t tx_bytes;
	size_t rx_overflows;
	size_t tx_overflows;
	int connected;
};

struct kilotnc_usb_cdc {
	void *ctx;
	enum kilotnc_usb_result (*read)(void *, uint8_t *, size_t, size_t *);
	enum kilotnc_usb_result (*write)(void *, const uint8_t *, size_t,
	    size_t *);
	enum kilotnc_usb_result (*connected)(void *, int *);
	enum kilotnc_usb_result (*stats)(void *,
	    struct kilotnc_usb_cdc_stats *);
};

#endif
