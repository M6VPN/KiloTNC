/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/tests/test_usb_cdc_stub.c */

#include <sys/types.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "usb_cdc_stub.h"

static int test_usb_stub_connected_state(void);
static int test_usb_stub_disconnect_blocks_io(void);
static int test_usb_stub_null_args(void);
static int test_usb_stub_rx_overflow(void);
static int test_usb_stub_rx_read(void);
static int test_usb_stub_tx_overflow(void);
static int test_usb_stub_tx_write_take(void);

static int
test_usb_stub_connected_state(void)
{
	struct usb_cdc_stub stub;
	const struct kilotnc_usb_cdc *usb;
	int connected;

	usb_cdc_stub_init(&stub);
	usb = usb_cdc_stub_usb(&stub);
	if (usb == NULL)
		return __LINE__;
	if (usb->connected(usb->ctx, &connected) != KILOTNC_USB_OK)
		return __LINE__;
	if (connected != 0)
		return __LINE__;
	if (usb_cdc_stub_set_connected(&stub, 1) != KILOTNC_USB_OK)
		return __LINE__;
	if (usb->connected(usb->ctx, &connected) != KILOTNC_USB_OK)
		return __LINE__;
	if (connected != 1)
		return __LINE__;

	return 0;
}

static int
test_usb_stub_disconnect_blocks_io(void)
{
	struct usb_cdc_stub stub;
	const struct kilotnc_usb_cdc *usb;
	uint8_t buf[4];
	size_t len;

	usb_cdc_stub_init(&stub);
	usb = usb_cdc_stub_usb(&stub);
	if (usb_cdc_stub_inject_rx(&stub, (const uint8_t *)"abc", 3) !=
	    KILOTNC_USB_OK)
		return __LINE__;
	if (usb->read(usb->ctx, buf, sizeof(buf), &len) !=
	    KILOTNC_USB_ERR_WOULD_BLOCK)
		return __LINE__;
	if (usb->write(usb->ctx, (const uint8_t *)"x", 1, &len) !=
	    KILOTNC_USB_ERR_WOULD_BLOCK)
		return __LINE__;

	return 0;
}

static int
test_usb_stub_null_args(void)
{
	struct usb_cdc_stub stub;
	const struct kilotnc_usb_cdc *usb;
	uint8_t buf[4];
	size_t len;
	int connected;

	usb_cdc_stub_init(&stub);
	usb = usb_cdc_stub_usb(&stub);
	if (usb_cdc_stub_usb(NULL) != NULL)
		return __LINE__;
	if (usb_cdc_stub_inject_rx(NULL, buf, sizeof(buf)) !=
	    KILOTNC_USB_ERR_ARG)
		return __LINE__;
	if (usb_cdc_stub_inject_rx(&stub, NULL, sizeof(buf)) !=
	    KILOTNC_USB_ERR_ARG)
		return __LINE__;
	if (usb_cdc_stub_set_connected(NULL, 1) != KILOTNC_USB_ERR_ARG)
		return __LINE__;
	if (usb_cdc_stub_take_tx(NULL, buf, sizeof(buf), &len) !=
	    KILOTNC_USB_ERR_ARG)
		return __LINE__;
	if (usb_cdc_stub_take_tx(&stub, NULL, sizeof(buf), &len) !=
	    KILOTNC_USB_ERR_ARG)
		return __LINE__;
	if (usb_cdc_stub_take_tx(&stub, buf, sizeof(buf), NULL) !=
	    KILOTNC_USB_ERR_ARG)
		return __LINE__;
	if (usb->connected(NULL, &connected) != KILOTNC_USB_ERR_ARG)
		return __LINE__;
	if (usb->connected(usb->ctx, NULL) != KILOTNC_USB_ERR_ARG)
		return __LINE__;
	if (usb->read(NULL, buf, sizeof(buf), &len) != KILOTNC_USB_ERR_ARG)
		return __LINE__;
	if (usb->read(usb->ctx, NULL, sizeof(buf), &len) !=
	    KILOTNC_USB_ERR_ARG)
		return __LINE__;
	if (usb->read(usb->ctx, buf, sizeof(buf), NULL) !=
	    KILOTNC_USB_ERR_ARG)
		return __LINE__;
	if (usb->write(NULL, buf, sizeof(buf), &len) != KILOTNC_USB_ERR_ARG)
		return __LINE__;
	if (usb->write(usb->ctx, NULL, sizeof(buf), &len) !=
	    KILOTNC_USB_ERR_ARG)
		return __LINE__;
	if (usb->write(usb->ctx, buf, sizeof(buf), NULL) !=
	    KILOTNC_USB_ERR_ARG)
		return __LINE__;

	return 0;
}

static int
test_usb_stub_rx_overflow(void)
{
	struct usb_cdc_stub stub;
	uint8_t data[USB_CDC_STUB_BUFFER_MAX + 1U];
	size_t i;

	usb_cdc_stub_init(&stub);
	for (i = 0; i < sizeof(data); i++)
		data[i] = (uint8_t)i;
	if (usb_cdc_stub_inject_rx(&stub, data, sizeof(data)) !=
	    KILOTNC_USB_ERR_SMALL)
		return __LINE__;
	if (stub.rx_overflows != 1u)
		return __LINE__;

	return 0;
}

static int
test_usb_stub_rx_read(void)
{
	struct usb_cdc_stub stub;
	const struct kilotnc_usb_cdc *usb;
	uint8_t buf[8];
	size_t len;

	usb_cdc_stub_init(&stub);
	usb = usb_cdc_stub_usb(&stub);
	if (usb_cdc_stub_set_connected(&stub, 1) != KILOTNC_USB_OK)
		return __LINE__;
	if (usb_cdc_stub_inject_rx(&stub, (const uint8_t *)"abc", 3) !=
	    KILOTNC_USB_OK)
		return __LINE__;
	if (usb->read(usb->ctx, buf, sizeof(buf), &len) != KILOTNC_USB_OK)
		return __LINE__;
	if (len != 3u)
		return __LINE__;
	if (memcmp(buf, "abc", 3) != 0)
		return __LINE__;
	if (usb->read(usb->ctx, buf, sizeof(buf), &len) !=
	    KILOTNC_USB_ERR_WOULD_BLOCK)
		return __LINE__;

	return 0;
}

static int
test_usb_stub_tx_overflow(void)
{
	struct usb_cdc_stub stub;
	const struct kilotnc_usb_cdc *usb;
	uint8_t data[USB_CDC_STUB_BUFFER_MAX + 1U];
	size_t len;
	size_t i;

	usb_cdc_stub_init(&stub);
	usb = usb_cdc_stub_usb(&stub);
	if (usb_cdc_stub_set_connected(&stub, 1) != KILOTNC_USB_OK)
		return __LINE__;
	for (i = 0; i < sizeof(data); i++)
		data[i] = (uint8_t)i;
	if (usb->write(usb->ctx, data, sizeof(data), &len) !=
	    KILOTNC_USB_ERR_SMALL)
		return __LINE__;
	if (stub.tx_overflows != 1u)
		return __LINE__;

	return 0;
}

static int
test_usb_stub_tx_write_take(void)
{
	struct usb_cdc_stub stub;
	const struct kilotnc_usb_cdc *usb;
	uint8_t out[8];
	size_t len;

	usb_cdc_stub_init(&stub);
	usb = usb_cdc_stub_usb(&stub);
	if (usb_cdc_stub_set_connected(&stub, 1) != KILOTNC_USB_OK)
		return __LINE__;
	if (usb->write(usb->ctx, (const uint8_t *)"xyz", 3, &len) !=
	    KILOTNC_USB_OK)
		return __LINE__;
	if (len != 3u)
		return __LINE__;
	if (usb_cdc_stub_take_tx(&stub, out, sizeof(out), &len) !=
	    KILOTNC_USB_OK)
		return __LINE__;
	if (len != 3u)
		return __LINE__;
	if (memcmp(out, "xyz", 3) != 0)
		return __LINE__;

	return 0;
}

int
test_usb_cdc_stub(void)
{
	int line;

	line = test_usb_stub_connected_state();
	if (line != 0)
		goto fail;
	line = test_usb_stub_disconnect_blocks_io();
	if (line != 0)
		goto fail;
	line = test_usb_stub_rx_read();
	if (line != 0)
		goto fail;
	line = test_usb_stub_tx_write_take();
	if (line != 0)
		goto fail;
	line = test_usb_stub_rx_overflow();
	if (line != 0)
		goto fail;
	line = test_usb_stub_tx_overflow();
	if (line != 0)
		goto fail;
	line = test_usb_stub_null_args();
	if (line != 0)
		goto fail;

	(void)printf("ok usb_cdc_stub\n");
	return 0;

fail:
	(void)printf("not ok usb_cdc_stub line %d\n", line);
	return 1;
}
