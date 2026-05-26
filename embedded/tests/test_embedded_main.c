/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/tests/test_embedded_main.c */

#include <sys/types.h>

int test_embedded_app(void);
int test_audio_stub(void);
int test_embedded_audio(void);
int test_embedded_diag(void);
int test_embedded_usb_bridge(void);
int test_usb_cdc_stub(void);

int
main(void)
{
	if (test_embedded_app() != 0)
		return 1;
	if (test_audio_stub() != 0)
		return 1;
	if (test_embedded_audio() != 0)
		return 1;
	if (test_usb_cdc_stub() != 0)
		return 1;
	if (test_embedded_usb_bridge() != 0)
		return 1;
	if (test_embedded_diag() != 0)
		return 1;

	return 0;
}
