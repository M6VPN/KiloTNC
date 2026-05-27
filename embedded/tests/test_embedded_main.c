/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/tests/test_embedded_main.c */

#include <sys/types.h>

int test_embedded_app(void);
int test_audio_stub(void);
int test_budget_metadata(void);
int test_embedded_audio(void);
int test_embedded_diag(void);
int test_embedded_loopback(void);
int test_embedded_modem(void);
int test_embedded_tnc(void);
int test_embedded_usb_bridge(void);
int test_target_metadata(void);
int test_usb_cdc_stub(void);
int test_usb_descriptor_plan(void);
int test_usb_stack_boundary(void);

int
main(void)
{
	if (test_embedded_app() != 0)
		return 1;
	if (test_audio_stub() != 0)
		return 1;
	if (test_budget_metadata() != 0)
		return 1;
	if (test_embedded_audio() != 0)
		return 1;
	if (test_embedded_modem() != 0)
		return 1;
	if (test_usb_cdc_stub() != 0)
		return 1;
	if (test_embedded_usb_bridge() != 0)
		return 1;
	if (test_embedded_tnc() != 0)
		return 1;
	if (test_embedded_diag() != 0)
		return 1;
	if (test_embedded_loopback() != 0)
		return 1;
	if (test_target_metadata() != 0)
		return 1;
	if (test_usb_stack_boundary() != 0)
		return 1;
	if (test_usb_descriptor_plan() != 0)
		return 1;

	return 0;
}
