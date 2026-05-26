/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/daemon/kilotncd_control.c */

#include <sys/types.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "kiss.h"
#include "kilotncd_control.h"
#include "tnc_mode.h"

static enum kilotncd_control_result kilotncd_control_abort_tx(
	struct kilotncd_control_context *, char *, size_t, size_t *);
static enum kilotncd_control_result kilotncd_control_apply_mode(
	struct tnc1200 *, enum tnc_mode_id, int);
static enum kilotncd_control_result kilotncd_control_copy_command(
	const char *, char *, size_t);
static enum kilotncd_control_result kilotncd_control_diag(
	struct kilotncd_control_context *, char *, size_t, size_t *);
static enum kilotncd_control_result kilotncd_control_dcd(
	struct kilotncd_control_context *, const char *, char *, size_t,
	size_t *);
static enum kilotncd_control_result kilotncd_control_help(char *, size_t,
	size_t *);
static enum kilotncd_control_result kilotncd_control_mode(
	struct kilotncd_control_context *, const char *, char *, size_t,
	size_t *);
static enum kilotncd_control_result kilotncd_control_ptt(
	struct kilotncd_control_context *, char *, size_t, size_t *);
static enum kilotncd_control_result kilotncd_control_snapshot(
	struct kilotncd_control_context *, struct tnc_diag_snapshot *);
static enum kilotncd_control_result kilotncd_control_stats(
	struct kilotncd_control_context *, char *, size_t, size_t *);
static enum kilotncd_control_result kilotncd_control_status(
	struct kilotncd_control_context *, char *, size_t, size_t *);
static enum kilotncd_control_result kilotncd_control_write(size_t,
	size_t *, int);
static int kilotncd_control_space(char);
static const char *kilotncd_control_support_name(enum tnc_mode_support);
static char *kilotncd_control_next_arg(char *);

enum kilotncd_control_result
kilotncd_control_exec(struct kilotncd_control_context *ctx,
	const char *command, char *response, size_t response_cap,
	size_t *response_len)
{
	char buf[KILOTNCD_CONTROL_COMMAND_MAX];
	char *arg;

	if (ctx == NULL || ctx->tnc == NULL || command == NULL ||
	    response == NULL || response_cap == 0U || response_len == NULL)
		return KILOTNCD_CONTROL_ERR_ARG;
	response[0] = '\0';
	*response_len = 0U;
	if (kilotncd_control_copy_command(command, buf, sizeof(buf)) !=
	    KILOTNCD_CONTROL_OK)
		return KILOTNCD_CONTROL_ERR_BAD_VALUE;
	if (buf[0] == '\0')
		return KILOTNCD_CONTROL_ERR_UNKNOWN;
	arg = kilotncd_control_next_arg(buf);
	if (strcmp(buf, "status") == 0 && arg == NULL)
		return kilotncd_control_status(ctx, response, response_cap,
		    response_len);
	if (strcmp(buf, "diag") == 0 && arg == NULL)
		return kilotncd_control_diag(ctx, response, response_cap,
		    response_len);
	if (strcmp(buf, "stats") == 0 && arg == NULL)
		return kilotncd_control_stats(ctx, response, response_cap,
		    response_len);
	if (strcmp(buf, "mode") == 0)
		return kilotncd_control_mode(ctx, arg, response, response_cap,
		    response_len);
	if (strcmp(buf, "ptt") == 0 && arg == NULL)
		return kilotncd_control_ptt(ctx, response, response_cap,
		    response_len);
	if (strcmp(buf, "dcd") == 0)
		return kilotncd_control_dcd(ctx, arg, response, response_cap,
		    response_len);
	if (strcmp(buf, "abort-tx") == 0 && arg == NULL)
		return kilotncd_control_abort_tx(ctx, response, response_cap,
		    response_len);
	if (strcmp(buf, "help") == 0 && arg == NULL)
		return kilotncd_control_help(response, response_cap,
		    response_len);

	return KILOTNCD_CONTROL_ERR_UNKNOWN;
}

static enum kilotncd_control_result
kilotncd_control_abort_tx(struct kilotncd_control_context *ctx,
	char *response, size_t response_cap, size_t *response_len)
{
	enum tnc_control_ptt ptt;
	int n;

	if (tnc1200_abort_tx(ctx->tnc) != TNC1200_OK ||
	    tnc1200_ptt_state(ctx->tnc, &ptt) != TNC1200_OK)
		return KILOTNCD_CONTROL_ERR_BAD_VALUE;
	n = snprintf(response, response_cap, "abort-tx=ok ptt=%u\n",
	    (unsigned int)ptt);

	return kilotncd_control_write(response_cap, response_len, n);
}

static enum kilotncd_control_result
kilotncd_control_apply_mode(struct tnc1200 *tnc, enum tnc_mode_id mode,
	int temporary)
{
	uint8_t payload[1];
	uint8_t frame[8];
	size_t frame_len;

	if (tnc_mode_to_nino_sethw(mode, temporary, &payload[0]) !=
	    TNC_MODE_OK)
		return KILOTNCD_CONTROL_ERR_BAD_VALUE;
	if (kiss_encode_frame(0U, KISS_CMD_SETHARDWARE, payload,
	    sizeof(payload), frame, sizeof(frame), &frame_len) != KISS_OK)
		return KILOTNCD_CONTROL_ERR_BAD_VALUE;
	if (tnc1200_host_input(tnc, frame, frame_len) != TNC1200_OK)
		return KILOTNCD_CONTROL_ERR_BAD_VALUE;

	return KILOTNCD_CONTROL_OK;
}

static enum kilotncd_control_result
kilotncd_control_copy_command(const char *command, char *buf, size_t buf_cap)
{
	const char *start;
	const char *end;
	size_t len;

	start = command;
	while (*start != '\0' && kilotncd_control_space(*start))
		start++;
	end = start + strlen(start);
	while (end > start && kilotncd_control_space(end[-1]))
		end--;
	len = (size_t)(end - start);
	if (len >= buf_cap)
		return KILOTNCD_CONTROL_ERR_BAD_VALUE;
	(void)memcpy(buf, start, len);
	buf[len] = '\0';

	return KILOTNCD_CONTROL_OK;
}

static enum kilotncd_control_result
kilotncd_control_diag(struct kilotncd_control_context *ctx, char *response,
	size_t response_cap, size_t *response_len)
{
	struct tnc_diag_snapshot snapshot;
	int n;

	if (kilotncd_control_snapshot(ctx, &snapshot) != KILOTNCD_CONTROL_OK)
		return KILOTNCD_CONTROL_ERR_BAD_VALUE;
	n = snprintf(response, response_cap, "diag ");
	if (kilotncd_control_write(response_cap, response_len, n) !=
	    KILOTNCD_CONTROL_OK)
		return KILOTNCD_CONTROL_ERR_SMALL;
	if (tnc_diag_format_snapshot(&snapshot, response + *response_len,
	    response_cap - *response_len, response_len) != TNC_DIAG_OK)
		return KILOTNCD_CONTROL_ERR_SMALL;
	*response_len = strlen(response);
	if (*response_len + 1U >= response_cap)
		return KILOTNCD_CONTROL_ERR_SMALL;
	response[*response_len] = '\n';
	response[*response_len + 1U] = '\0';
	(*response_len)++;

	return KILOTNCD_CONTROL_OK;
}

static enum kilotncd_control_result
kilotncd_control_dcd(struct kilotncd_control_context *ctx, const char *arg,
	char *response, size_t response_cap, size_t *response_len)
{
	int dcd;
	int n;

	if (arg == NULL)
		return KILOTNCD_CONTROL_ERR_BAD_VALUE;
	if (strcmp(arg, "0") == 0)
		dcd = 0;
	else if (strcmp(arg, "1") == 0)
		dcd = 1;
	else
		return KILOTNCD_CONTROL_ERR_BAD_VALUE;
	if (tnc1200_set_dcd(ctx->tnc, dcd) != TNC1200_OK)
		return KILOTNCD_CONTROL_ERR_BAD_VALUE;
	n = snprintf(response, response_cap, "dcd=%d\n", dcd);

	return kilotncd_control_write(response_cap, response_len, n);
}

static enum kilotncd_control_result
kilotncd_control_help(char *response, size_t response_cap,
	size_t *response_len)
{
	int n;

	n = snprintf(response, response_cap,
	    "commands=status,mode,diag,stats,ptt,dcd,abort-tx,help\n");

	return kilotncd_control_write(response_cap, response_len, n);
}

static enum kilotncd_control_result
kilotncd_control_mode(struct kilotncd_control_context *ctx, const char *arg,
	char *response, size_t response_cap, size_t *response_len)
{
	const struct tnc_mode_desc *desc;
	struct tnc1200_status status;
	enum tnc_mode_id mode;
	int temporary;
	int n;

	if (arg == NULL) {
		if (tnc1200_status(ctx->tnc, &status) != TNC1200_OK ||
		    tnc_mode_get(status.current_mode, &desc) != TNC_MODE_OK)
			return KILOTNCD_CONTROL_ERR_BAD_VALUE;
		n = snprintf(response, response_cap,
		    "mode name=\"%s\" support=%s temporary=%u id=%u\n",
		    desc->name, kilotncd_control_support_name(desc->support),
		    (unsigned int)status.last_mode_temporary,
		    (unsigned int)desc->nino_sethw_persistent);
		return kilotncd_control_write(response_cap, response_len, n);
	}
	if (tnc_mode_parse_option(arg, &mode, &temporary) != TNC_MODE_OK)
		return KILOTNCD_CONTROL_ERR_BAD_VALUE;
	if (tnc_mode_get(mode, &desc) != TNC_MODE_OK)
		return KILOTNCD_CONTROL_ERR_BAD_VALUE;
	if (desc->support != TNC_MODE_SUPPORT_IMPLEMENTED)
		return KILOTNCD_CONTROL_ERR_UNSUPPORTED;
	if (kilotncd_control_apply_mode(ctx->tnc, mode, temporary) !=
	    KILOTNCD_CONTROL_OK)
		return KILOTNCD_CONTROL_ERR_BAD_VALUE;
	n = snprintf(response, response_cap,
	    "mode name=\"%s\" support=%s temporary=%d id=%u\n",
	    desc->name, kilotncd_control_support_name(desc->support),
	    temporary, (unsigned int)desc->nino_sethw_persistent);

	return kilotncd_control_write(response_cap, response_len, n);
}

static enum kilotncd_control_result
kilotncd_control_ptt(struct kilotncd_control_context *ctx, char *response,
	size_t response_cap, size_t *response_len)
{
	enum tnc_control_ptt ptt;
	int n;

	if (tnc1200_ptt_state(ctx->tnc, &ptt) != TNC1200_OK)
		return KILOTNCD_CONTROL_ERR_BAD_VALUE;
	n = snprintf(response, response_cap, "ptt=%u\n",
	    (unsigned int)ptt);

	return kilotncd_control_write(response_cap, response_len, n);
}

static enum kilotncd_control_result
kilotncd_control_snapshot(struct kilotncd_control_context *ctx,
	struct tnc_diag_snapshot *snapshot)
{
	struct tnc_diag local_diag;
	struct tnc_diag *diag;

	diag = ctx->diag;
	if (diag == NULL) {
		if (tnc_diag_init(&local_diag) != TNC_DIAG_OK)
			return KILOTNCD_CONTROL_ERR_BAD_VALUE;
		diag = &local_diag;
	}
	if (tnc_diag_capture_tnc1200(diag, ctx->tnc) != TNC_DIAG_OK ||
	    tnc_diag_snapshot(diag, snapshot) != TNC_DIAG_OK)
		return KILOTNCD_CONTROL_ERR_BAD_VALUE;

	return KILOTNCD_CONTROL_OK;
}

static enum kilotncd_control_result
kilotncd_control_stats(struct kilotncd_control_context *ctx, char *response,
	size_t response_cap, size_t *response_len)
{
	struct tnc1200_stats stats;
	int n;

	if (tnc1200_stats(ctx->tnc, &stats) != TNC1200_OK)
		return KILOTNCD_CONTROL_ERR_BAD_VALUE;
	n = snprintf(response, response_cap,
	    "stats kiss_in=%zu kiss_out=%zu tx_started=%zu tx_done=%zu "
	    "rx_ok=%zu rx_bad_fcs=%zu rx_malformed=%zu rx_dropped=%zu\n",
	    stats.kiss_frames_in, stats.kiss_frames_out,
	    stats.tx_frames_started, stats.tx_frames_done,
	    stats.rx_frames_ok, stats.rx_frames_bad_fcs,
	    stats.rx_frames_malformed, stats.rx_frames_dropped);

	return kilotncd_control_write(response_cap, response_len, n);
}

static enum kilotncd_control_result
kilotncd_control_status(struct kilotncd_control_context *ctx, char *response,
	size_t response_cap, size_t *response_len)
{
	const struct tnc_mode_desc *desc;
	struct tnc_diag_snapshot snapshot;
	int n;

	if (kilotncd_control_snapshot(ctx, &snapshot) != KILOTNCD_CONTROL_OK ||
	    tnc_mode_get(snapshot.current_mode, &desc) != TNC_MODE_OK)
		return KILOTNCD_CONTROL_ERR_BAD_VALUE;
	n = snprintf(response, response_cap,
	    "status mode=\"%s\" ptt=%u dcd=%u tx_active=%u "
	    "audio_ready=%u rx_ok=%zu tx_done=%zu last_fault=%u\n",
	    desc->name, (unsigned int)snapshot.ptt_state,
	    (unsigned int)snapshot.dcd_busy,
	    (unsigned int)snapshot.tx_active,
	    (unsigned int)snapshot.audio_ready, snapshot.rx_frames_ok,
	    snapshot.tx_frames_done, (unsigned int)snapshot.last_fault);

	return kilotncd_control_write(response_cap, response_len, n);
}

static enum kilotncd_control_result
kilotncd_control_write(size_t response_cap, size_t *response_len, int n)
{
	if (n < 0)
		return KILOTNCD_CONTROL_ERR_SMALL;
	if ((size_t)n >= response_cap)
		return KILOTNCD_CONTROL_ERR_SMALL;
	*response_len = (size_t)n;

	return KILOTNCD_CONTROL_OK;
}

static int
kilotncd_control_space(char c)
{
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static const char *
kilotncd_control_support_name(enum tnc_mode_support support)
{
	if (support == TNC_MODE_SUPPORT_IMPLEMENTED)
		return "implemented";
	if (support == TNC_MODE_SUPPORT_PLANNED)
		return "planned";
	if (support == TNC_MODE_SUPPORT_RESEARCH)
		return "research";
	return "unsupported";
}

static char *
kilotncd_control_next_arg(char *buf)
{
	char *p;

	p = buf;
	while (*p != '\0' && !kilotncd_control_space(*p))
		p++;
	if (*p == '\0')
		return NULL;
	*p++ = '\0';
	while (*p != '\0' && kilotncd_control_space(*p))
		p++;
	if (*p == '\0')
		return NULL;

	return p;
}
