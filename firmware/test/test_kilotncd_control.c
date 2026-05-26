/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/test/test_kilotncd_control.c */

#include <sys/types.h>

#include <stdint.h>
#include <string.h>

#include "kilotncd_control.h"
#include "tnc1200.h"
#include "tnc_diag.h"

#define CHECK(expr) do { if (!(expr)) return __LINE__; } while (0)

static int test_kilotncd_control_basic(void);
static int test_kilotncd_control_dcd(void);
static int test_kilotncd_control_errors(void);
static int test_kilotncd_control_mode(void);

int
test_kilotncd_control(void)
{
	int subres;

	subres = test_kilotncd_control_basic();
	if (subres != 0)
		return subres;
	subres = test_kilotncd_control_mode();
	if (subres != 0)
		return subres;
	subres = test_kilotncd_control_dcd();
	if (subres != 0)
		return subres;
	subres = test_kilotncd_control_errors();
	if (subres != 0)
		return subres;

	return 0;
}

static int
test_kilotncd_control_basic(void)
{
	struct kilotncd_control_context ctx;
	struct tnc1200 tnc;
	struct tnc_diag diag;
	char response[KILOTNCD_CONTROL_RESPONSE_MAX];
	size_t len;

	CHECK(tnc1200_init(&tnc, NULL) == TNC1200_OK);
	CHECK(tnc_diag_init(&diag) == TNC_DIAG_OK);
	ctx.tnc = &tnc;
	ctx.diag = &diag;

	CHECK(kilotncd_control_exec(&ctx, "status", response,
	    sizeof(response), &len) == KILOTNCD_CONTROL_OK);
	CHECK(len != 0U);
	CHECK(strstr(response, "status mode=") != NULL);
	CHECK(kilotncd_control_exec(&ctx, "diag", response,
	    sizeof(response), &len) == KILOTNCD_CONTROL_OK);
	CHECK(strstr(response, "diag ") == response);
	CHECK(kilotncd_control_exec(&ctx, "stats", response,
	    sizeof(response), &len) == KILOTNCD_CONTROL_OK);
	CHECK(strstr(response, "stats kiss_in=") == response);
	CHECK(kilotncd_control_exec(&ctx, "mode", response,
	    sizeof(response), &len) == KILOTNCD_CONTROL_OK);
	CHECK(strstr(response, "support=implemented") != NULL);
	CHECK(kilotncd_control_exec(&ctx, "ptt", response,
	    sizeof(response), &len) == KILOTNCD_CONTROL_OK);
	CHECK(strstr(response, "ptt=0") == response);
	CHECK(kilotncd_control_exec(&ctx, "help", response,
	    sizeof(response), &len) == KILOTNCD_CONTROL_OK);
	CHECK(strstr(response, "commands=") == response);
	CHECK(kilotncd_control_exec(&ctx, "abort-tx", response,
	    sizeof(response), &len) == KILOTNCD_CONTROL_OK);
	CHECK(strstr(response, "abort-tx=ok") == response);

	return 0;
}

static int
test_kilotncd_control_dcd(void)
{
	struct kilotncd_control_context ctx;
	struct tnc1200_status status;
	struct tnc1200 tnc;
	char response[KILOTNCD_CONTROL_RESPONSE_MAX];
	size_t len;

	CHECK(tnc1200_init(&tnc, NULL) == TNC1200_OK);
	ctx.tnc = &tnc;
	ctx.diag = NULL;

	CHECK(kilotncd_control_exec(&ctx, "dcd 1", response,
	    sizeof(response), &len) == KILOTNCD_CONTROL_OK);
	CHECK(strstr(response, "dcd=1") == response);
	CHECK(tnc1200_status(&tnc, &status) == TNC1200_OK);
	CHECK(status.dcd_busy == 1U);
	CHECK(kilotncd_control_exec(&ctx, "dcd 0", response,
	    sizeof(response), &len) == KILOTNCD_CONTROL_OK);
	CHECK(tnc1200_status(&tnc, &status) == TNC1200_OK);
	CHECK(status.dcd_busy == 0U);
	CHECK(kilotncd_control_exec(&ctx, "dcd 2", response,
	    sizeof(response), &len) == KILOTNCD_CONTROL_ERR_BAD_VALUE);

	return 0;
}

static int
test_kilotncd_control_errors(void)
{
	struct kilotncd_control_context ctx;
	struct tnc1200 tnc;
	char command[KILOTNCD_CONTROL_COMMAND_MAX + 2U];
	char response[KILOTNCD_CONTROL_RESPONSE_MAX];
	size_t len;

	CHECK(tnc1200_init(&tnc, NULL) == TNC1200_OK);
	ctx.tnc = &tnc;
	ctx.diag = NULL;

	CHECK(kilotncd_control_exec(NULL, "status", response,
	    sizeof(response), &len) == KILOTNCD_CONTROL_ERR_ARG);
	CHECK(kilotncd_control_exec(&ctx, NULL, response,
	    sizeof(response), &len) == KILOTNCD_CONTROL_ERR_ARG);
	CHECK(kilotncd_control_exec(&ctx, "status", NULL,
	    sizeof(response), &len) == KILOTNCD_CONTROL_ERR_ARG);
	CHECK(kilotncd_control_exec(&ctx, "status", response,
	    0U, &len) == KILOTNCD_CONTROL_ERR_ARG);
	CHECK(kilotncd_control_exec(&ctx, "status", response,
	    sizeof(response), NULL) == KILOTNCD_CONTROL_ERR_ARG);
	ctx.tnc = NULL;
	CHECK(kilotncd_control_exec(&ctx, "status", response,
	    sizeof(response), &len) == KILOTNCD_CONTROL_ERR_ARG);
	ctx.tnc = &tnc;
	CHECK(kilotncd_control_exec(&ctx, "bad-command", response,
	    sizeof(response), &len) == KILOTNCD_CONTROL_ERR_UNKNOWN);
	CHECK(kilotncd_control_exec(&ctx, "status", response,
	    4U, &len) == KILOTNCD_CONTROL_ERR_SMALL);
	(void)memset(command, 'a', sizeof(command) - 1U);
	command[sizeof(command) - 1U] = '\0';
	CHECK(kilotncd_control_exec(&ctx, command, response,
	    sizeof(response), &len) == KILOTNCD_CONTROL_ERR_BAD_VALUE);

	return 0;
}

static int
test_kilotncd_control_mode(void)
{
	struct kilotncd_control_context ctx;
	struct tnc1200_status status;
	struct tnc1200 tnc;
	char response[KILOTNCD_CONTROL_RESPONSE_MAX];
	size_t len;

	CHECK(tnc1200_init(&tnc, NULL) == TNC1200_OK);
	ctx.tnc = &tnc;
	ctx.diag = NULL;

	CHECK(kilotncd_control_exec(&ctx, "mode NINO_MODE=6", response,
	    sizeof(response), &len) == KILOTNCD_CONTROL_OK);
	CHECK(strstr(response, "support=implemented") != NULL);
	CHECK(tnc1200_status(&tnc, &status) == TNC1200_OK);
	CHECK(status.current_mode == TNC_MODE_1200_AFSK_AX25);
	CHECK(status.last_mode_temporary == 0U);
	CHECK(kilotncd_control_exec(&ctx, "mode NINO_MODE=22", response,
	    sizeof(response), &len) == KILOTNCD_CONTROL_OK);
	CHECK(tnc1200_status(&tnc, &status) == TNC1200_OK);
	CHECK(status.current_mode == TNC_MODE_1200_AFSK_AX25);
	CHECK(status.last_mode_temporary == 1U);
	CHECK(kilotncd_control_exec(&ctx, "mode NINO_MODE=0", response,
	    sizeof(response), &len) == KILOTNCD_CONTROL_ERR_UNSUPPORTED);
	CHECK(kilotncd_control_exec(&ctx, "mode BAD", response,
	    sizeof(response), &len) == KILOTNCD_CONTROL_ERR_BAD_VALUE);

	return 0;
}
