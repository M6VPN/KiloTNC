/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/daemon/kilotncd.c */

#include <sys/types.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "afsk1200.h"
#include "kiss.h"
#include "kilotncd_audio.h"
#include "kilotncd_config.h"
#include "kilotncd_file.h"
#include "kilotncd_pty.h"
#include "kilotncd_tcp.h"
#include "kilotncd_unix.h"
#include "tnc1200.h"
#include "tnc_diag.h"
#include "tnc_mode.h"

#define KILOTNCD_KISS_MAX_BYTES	(KILOTNC_KISS_MAX_FRAME * 8U)
#define KILOTNCD_PCM_MAX_SAMPLES \
	((AFSK1200_MAX_TEST_BITS + (80U * 8U)) * AFSK1200_SAMPLES_PER_BIT)
#define KILOTNCD_DIAG_MAX	2048U
#define KILOTNCD_TX_CHUNK	257U

enum kilotncd_operation {
	KILOTNCD_OP_NONE = 0,
	KILOTNCD_OP_ONCE,
	KILOTNCD_OP_LOOPBACK_ONCE,
	KILOTNCD_OP_STATUS
};

struct kilotncd_loopback_ctx {
	const struct kilotncd_config *config;
	struct tnc1200 tx_tnc;
	struct tnc1200 rx_tnc;
};

static uint8_t daemon_kiss_in[KILOTNCD_KISS_MAX_BYTES];
static uint8_t daemon_kiss_out[KILOTNCD_KISS_MAX_BYTES];
static int16_t daemon_pcm[KILOTNCD_PCM_MAX_SAMPLES];
static char daemon_diag[KILOTNCD_DIAG_MAX];

static int kilotncd_apply_mode(struct tnc1200 *, enum tnc_mode_id, int);
static int kilotncd_audio_config_path(const struct kilotncd_config *,
	const char *, struct kilotncd_audio_config *);
static int kilotncd_collect_tx(struct tnc1200 *, int16_t *, size_t,
	size_t *);
static int kilotncd_diag(FILE *, const char *, const struct tnc1200 *);
static int kilotncd_init_tnc(struct tnc1200 *,
	const struct kilotncd_config *);
static FILE *kilotncd_diag_stream(const char *, const char *);
static int kilotncd_load_config(int, char **, struct kilotncd_config *,
	enum kilotncd_operation *);
static size_t kilotncd_loopback_handler(const uint8_t *, size_t, uint8_t *,
	size_t, void *);
static int kilotncd_mode_implemented(enum tnc_mode_id);
static int kilotncd_parse_args(int, char **, struct kilotncd_config *,
	enum kilotncd_operation *);
static int kilotncd_read_pcm(const struct kilotncd_config *, const char *,
	int16_t *, size_t, size_t *);
static int kilotncd_run_loopback_once(const struct kilotncd_config *);
static int kilotncd_run_once(const struct kilotncd_config *);
static int kilotncd_run_pty_tx_once(const struct kilotncd_config *);
static int kilotncd_run_tcp_loopback_once(const struct kilotncd_config *);
static int kilotncd_run_tcp_tx_once(const struct kilotncd_config *);
static int kilotncd_run_unix_tx_once(const struct kilotncd_config *);
static int kilotncd_run_rx_once(const struct kilotncd_config *);
static int kilotncd_run_status(const struct kilotncd_config *);
static int kilotncd_run_tx_once(const struct kilotncd_config *);
static int kilotncd_pty_ready(const struct kilotncd_config *);
static int kilotncd_tcp_ready(const struct kilotncd_config *);
static int kilotncd_unix_ready(const struct kilotncd_config *);
static int kilotncd_validate_config(const struct kilotncd_config *);
static int kilotncd_write_pcm(const struct kilotncd_config *, const char *,
	const int16_t *, size_t);
static const char *kilotncd_support_name(enum tnc_mode_support);
static void kilotncd_usage(void);

int
main(int argc, char *argv[])
{
	struct kilotncd_config config;
	enum kilotncd_operation op;

	if (kilotncd_load_config(argc, argv, &config, &op) != 0) {
		kilotncd_usage();
		return 1;
	}

	if (op == KILOTNCD_OP_STATUS)
		return kilotncd_run_status(&config);
	if (op == KILOTNCD_OP_ONCE)
		return kilotncd_run_once(&config);
	if (op == KILOTNCD_OP_LOOPBACK_ONCE)
		return kilotncd_run_loopback_once(&config);

	kilotncd_usage();
	return 1;
}

static int
kilotncd_apply_mode(struct tnc1200 *tnc, enum tnc_mode_id mode,
	int temporary)
{
	uint8_t payload[1];
	uint8_t frame[8];
	size_t frame_len;

	if (tnc_mode_to_nino_sethw(mode, temporary, &payload[0]) !=
	    TNC_MODE_OK)
		return -1;
	if (kiss_encode_frame(0U, KISS_CMD_SETHARDWARE, payload,
	    sizeof(payload), frame, sizeof(frame), &frame_len) != KISS_OK)
		return -1;
	if (tnc1200_host_input(tnc, frame, frame_len) != TNC1200_OK)
		return -1;

	return 0;
}

static int
kilotncd_audio_config_path(const struct kilotncd_config *config,
	const char *path, struct kilotncd_audio_config *audio_config)
{
	size_t len;

	if (config == NULL || path == NULL || audio_config == NULL)
		return -1;
	len = strlen(path);
	if (len == 0U || len >= sizeof(audio_config->path))
		return -1;
	(void)memset(audio_config, 0, sizeof(*audio_config));
	audio_config->backend = config->audio_backend;
	audio_config->format = config->audio_format;
	(void)memcpy(audio_config->path, path, len);
	audio_config->path[len] = '\0';

	return 0;
}

static int
kilotncd_collect_tx(struct tnc1200 *tnc, int16_t *pcm, size_t pcm_cap,
	size_t *samples)
{
	size_t emitted;
	size_t idle_count;
	size_t chunk;
	size_t off;
	enum tnc1200_result res;

	idle_count = 0U;
	off = 0U;
	for (;;) {
		if (off >= pcm_cap)
			return -1;
		chunk = pcm_cap - off;
		if (chunk > KILOTNCD_TX_CHUNK)
			chunk = KILOTNCD_TX_CHUNK;
		res = tnc1200_tx_process(tnc, &pcm[off], chunk, &emitted);
		if (res == TNC1200_ERR_NO_DATA) {
			if (off != 0U)
				break;
			idle_count++;
			if (idle_count > 5000U)
				return -1;
			continue;
		}
		if (res != TNC1200_OK)
			return -1;
		off += emitted;
	}
	*samples = off;

	return 0;
}

static int
kilotncd_diag(FILE *fp, const char *prefix, const struct tnc1200 *tnc)
{
	struct tnc_diag diag;
	struct tnc_diag_snapshot snapshot;
	size_t len;

	if (fp == NULL || prefix == NULL || tnc == NULL)
		return -1;
	if (tnc_diag_init(&diag) != TNC_DIAG_OK ||
	    tnc_diag_capture_tnc1200(&diag, tnc) != TNC_DIAG_OK ||
	    tnc_diag_snapshot(&diag, &snapshot) != TNC_DIAG_OK ||
	    tnc_diag_format_snapshot(&snapshot, daemon_diag,
	    sizeof(daemon_diag), &len) != TNC_DIAG_OK)
		return -1;
	(void)fprintf(fp, "%s %s\n", prefix, daemon_diag);

	return 0;
}

static int
kilotncd_init_tnc(struct tnc1200 *tnc, const struct kilotncd_config *config)
{
	struct tnc1200_config tnc_config;

	if (tnc == NULL || config == NULL)
		return -1;
	(void)memset(&tnc_config, 0, sizeof(tnc_config));
	tnc_config.txdelay_flags = AFSK1200_TX_DEFAULT_TXDELAY_FLAGS;
	tnc_config.txtail_flags = AFSK1200_TX_DEFAULT_TXTAIL_FLAGS;
	tnc_config.amplitude = AFSK1200_PCM_AMPLITUDE;
	tnc_config.p = config->p;
	tnc_config.slottime_10ms = config->slottime_10ms;
	tnc_config.fullduplex = config->fullduplex;
	tnc_config.max_tx_ms = config->max_tx_ms;
	tnc_config.rng_seed = 1U;
	if (tnc1200_init(tnc, &tnc_config) != TNC1200_OK)
		return -1;
	if (kilotncd_apply_mode(tnc, config->mode,
	    config->mode_temporary) != 0)
		return -1;

	return 0;
}

static FILE *
kilotncd_diag_stream(const char *primary_out, const char *secondary_out)
{
	if ((primary_out != NULL && strcmp(primary_out, "-") == 0) ||
	    (secondary_out != NULL && strcmp(secondary_out, "-") == 0))
		return stderr;
	return stdout;
}

static int
kilotncd_load_config(int argc, char **argv, struct kilotncd_config *config,
	enum kilotncd_operation *op)
{
	const char *config_path;
	int i;

	if (kilotncd_config_init(config) != KILOTNCD_CONFIG_OK)
		return -1;
	*op = KILOTNCD_OP_NONE;
	config_path = NULL;
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--config") == 0) {
			if (i + 1 >= argc)
				return -1;
			config_path = argv[++i];
		}
	}
	if (config_path != NULL &&
	    kilotncd_config_load_file(config, config_path) !=
	    KILOTNCD_CONFIG_OK)
		return -1;
	if (kilotncd_parse_args(argc, argv, config, op) != 0)
		return -1;
	return kilotncd_validate_config(config);
}

static size_t
kilotncd_loopback_handler(const uint8_t *kiss, size_t kiss_len, uint8_t *out,
	size_t out_cap, void *arg)
{
	struct kilotncd_loopback_ctx *ctx;
	size_t out_len;
	size_t samples;

	ctx = arg;
	if (ctx == NULL || kiss == NULL || out == NULL)
		return (size_t)-1;
	if (kilotncd_init_tnc(&ctx->tx_tnc, ctx->config) != 0 ||
	    kilotncd_init_tnc(&ctx->rx_tnc, ctx->config) != 0)
		return (size_t)-1;
	if (tnc1200_host_input(&ctx->tx_tnc, kiss, kiss_len) != TNC1200_OK)
		return (size_t)-1;
	if (kilotncd_collect_tx(&ctx->tx_tnc, daemon_pcm,
	    sizeof(daemon_pcm) / sizeof(daemon_pcm[0]), &samples) != 0)
		return (size_t)-1;
	if (tnc1200_rx_process(&ctx->rx_tnc, daemon_pcm, samples, out,
	    out_cap, &out_len) != TNC1200_OK)
		return (size_t)-1;

	return out_len;
}

static int
kilotncd_mode_implemented(enum tnc_mode_id mode)
{
	const struct tnc_mode_desc *desc;

	if (tnc_mode_get(mode, &desc) != TNC_MODE_OK)
		return -1;
	if (desc->support != TNC_MODE_SUPPORT_IMPLEMENTED) {
		(void)fprintf(stderr, "mode not implemented: %s\n",
		    desc->name);
		return -1;
	}

	return 0;
}

static int
kilotncd_parse_args(int argc, char **argv, struct kilotncd_config *config,
	enum kilotncd_operation *op)
{
	int i;

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--config") == 0) {
			if (i + 1 >= argc)
				return -1;
			i++;
		} else if (strcmp(argv[i], "--mode") == 0) {
			if (i + 1 >= argc ||
			    kilotncd_config_apply_arg(config, "mode",
			    argv[++i]) != KILOTNCD_CONFIG_OK)
				return -1;
		} else if (strcmp(argv[i], "--kiss-in") == 0) {
			if (i + 1 >= argc ||
			    kilotncd_config_apply_arg(config, "kiss_in",
			    argv[++i]) != KILOTNCD_CONFIG_OK)
				return -1;
		} else if (strcmp(argv[i], "--kiss-out") == 0) {
			if (i + 1 >= argc ||
			    kilotncd_config_apply_arg(config, "kiss_out",
			    argv[++i]) != KILOTNCD_CONFIG_OK)
				return -1;
		} else if (strcmp(argv[i], "--pcm-in") == 0) {
			if (i + 1 >= argc ||
			    kilotncd_config_apply_arg(config, "pcm_in",
			    argv[++i]) != KILOTNCD_CONFIG_OK)
				return -1;
		} else if (strcmp(argv[i], "--pcm-out") == 0) {
			if (i + 1 >= argc ||
			    kilotncd_config_apply_arg(config, "pcm_out",
			    argv[++i]) != KILOTNCD_CONFIG_OK)
				return -1;
		} else if (strcmp(argv[i], "--kiss-tcp-listen") == 0) {
			if (i + 1 >= argc ||
			    kilotncd_config_apply_arg(config,
			    "kiss_tcp_listen", argv[++i]) !=
			    KILOTNCD_CONFIG_OK)
				return -1;
		} else if (strcmp(argv[i], "--kiss-tcp-once") == 0) {
			if (kilotncd_config_apply_arg(config, "kiss_tcp_once",
			    "1") != KILOTNCD_CONFIG_OK)
				return -1;
		} else if (strcmp(argv[i], "--allow-nonlocal-bind") == 0) {
			if (kilotncd_config_apply_arg(config,
			    "allow_nonlocal_bind", "1") !=
			    KILOTNCD_CONFIG_OK)
				return -1;
		} else if (strcmp(argv[i], "--kiss-unix-listen") == 0) {
			if (i + 1 >= argc ||
			    kilotncd_config_apply_arg(config,
			    "kiss_unix_listen", argv[++i]) !=
			    KILOTNCD_CONFIG_OK)
				return -1;
		} else if (strcmp(argv[i], "--kiss-unix-once") == 0) {
			if (kilotncd_config_apply_arg(config,
			    "kiss_unix_once", "1") != KILOTNCD_CONFIG_OK)
				return -1;
		} else if (strcmp(argv[i], "--unlink-stale-socket") == 0) {
			if (kilotncd_config_apply_arg(config,
			    "unlink_stale_socket", "1") !=
			    KILOTNCD_CONFIG_OK)
				return -1;
		} else if (strcmp(argv[i], "--kiss-pty") == 0) {
			if (kilotncd_config_apply_arg(config, "kiss_pty",
			    "1") != KILOTNCD_CONFIG_OK)
				return -1;
		} else if (strcmp(argv[i], "--kiss-pty-once") == 0) {
			if (kilotncd_config_apply_arg(config,
			    "kiss_pty_once", "1") != KILOTNCD_CONFIG_OK)
				return -1;
		} else if (strcmp(argv[i], "--pty-path-out") == 0) {
			if (i + 1 >= argc ||
			    kilotncd_config_apply_arg(config,
			    "pty_path_out", argv[++i]) !=
			    KILOTNCD_CONFIG_OK)
				return -1;
		} else if (strcmp(argv[i], "--once") == 0) {
			*op = KILOTNCD_OP_ONCE;
		} else if (strcmp(argv[i], "--loopback-once") == 0) {
			*op = KILOTNCD_OP_LOOPBACK_ONCE;
		} else if (strcmp(argv[i], "--status") == 0) {
			*op = KILOTNCD_OP_STATUS;
		} else {
			return -1;
		}
	}

	return 0;
}

static int
kilotncd_read_pcm(const struct kilotncd_config *config, const char *path,
	int16_t *pcm, size_t cap, size_t *samples)
{
	struct kilotncd_audio audio;
	struct kilotncd_audio_config audio_config;
	enum kilotncd_audio_result res;

	if (kilotncd_audio_config_path(config, path, &audio_config) != 0)
		return -1;
	res = kilotncd_audio_open_input(&audio, &audio_config);
	if (res != KILOTNCD_AUDIO_OK)
		return -1;
	res = kilotncd_audio_read(&audio, pcm, cap, samples);
	if (kilotncd_audio_close(&audio) != KILOTNCD_AUDIO_OK)
		return -1;
	if (res != KILOTNCD_AUDIO_OK)
		return -1;

	return 0;
}

static int
kilotncd_run_loopback_once(const struct kilotncd_config *config)
{
	struct tnc1200 tx_tnc;
	struct tnc1200 rx_tnc;
	FILE *diag_fp;
	size_t kiss_len;
	size_t out_len;
	size_t samples;

	if (config->have_kiss_tcp_listen)
		return kilotncd_run_tcp_loopback_once(config);
	if (config->have_kiss_unix_listen)
		return 1;
	if (config->kiss_pty != 0U)
		return 1;
	if (!config->have_kiss_in || !config->have_kiss_out)
		return 1;
	if (kilotncd_mode_implemented(config->mode) != 0)
		return 1;
	if (kilotncd_file_read_bytes(config->kiss_in, daemon_kiss_in,
	    sizeof(daemon_kiss_in), &kiss_len) != KILOTNCD_FILE_OK)
		return 1;
	if (kilotncd_init_tnc(&tx_tnc, config) != 0 ||
	    kilotncd_init_tnc(&rx_tnc, config) != 0)
		return 1;
	if (tnc1200_host_input(&tx_tnc, daemon_kiss_in, kiss_len) !=
	    TNC1200_OK)
		return 1;
	if (kilotncd_collect_tx(&tx_tnc, daemon_pcm,
	    sizeof(daemon_pcm) / sizeof(daemon_pcm[0]), &samples) != 0)
		return 1;
	if (tnc1200_rx_process(&rx_tnc, daemon_pcm, samples,
	    daemon_kiss_out, sizeof(daemon_kiss_out), &out_len) != TNC1200_OK)
		return 1;
	if (kilotncd_file_write_bytes(config->kiss_out, daemon_kiss_out,
	    out_len) != KILOTNCD_FILE_OK)
		return 1;
	diag_fp = kilotncd_diag_stream(config->kiss_out, NULL);
	if (kilotncd_diag(diag_fp, "tx_diag", &tx_tnc) != 0 ||
	    kilotncd_diag(diag_fp, "rx_diag", &rx_tnc) != 0)
		return 1;

	return 0;
}

static int
kilotncd_run_once(const struct kilotncd_config *config)
{
	if (config->have_kiss_tcp_listen && config->have_pcm_out &&
	    !config->have_pcm_in && !config->have_kiss_out &&
	    !config->have_kiss_in)
		return kilotncd_run_tcp_tx_once(config);
	if (config->have_kiss_unix_listen && config->have_pcm_out &&
	    !config->have_pcm_in && !config->have_kiss_out &&
	    !config->have_kiss_in)
		return kilotncd_run_unix_tx_once(config);
	if (config->kiss_pty != 0U && config->have_pcm_out &&
	    !config->have_pcm_in && !config->have_kiss_out &&
	    !config->have_kiss_in)
		return kilotncd_run_pty_tx_once(config);
	if (config->have_kiss_in && config->have_pcm_out &&
	    !config->have_pcm_in && !config->have_kiss_out &&
	    !config->have_kiss_tcp_listen && !config->have_kiss_unix_listen &&
	    config->kiss_pty == 0U)
		return kilotncd_run_tx_once(config);
	if (config->have_pcm_in && config->have_kiss_out &&
	    !config->have_kiss_in && !config->have_pcm_out &&
	    !config->have_kiss_tcp_listen && !config->have_kiss_unix_listen &&
	    config->kiss_pty == 0U)
		return kilotncd_run_rx_once(config);

	return 1;
}

static int
kilotncd_run_pty_tx_once(const struct kilotncd_config *config)
{
	struct tnc1200 tnc;
	FILE *diag_fp;
	const char *path_out;
	size_t kiss_len;
	size_t samples;

	if (kilotncd_pty_ready(config) != 0 ||
	    kilotncd_mode_implemented(config->mode) != 0)
		return 1;
	path_out = config->have_pty_path_out ? config->pty_path_out : NULL;
	if (kilotncd_pty_server_once(path_out, daemon_kiss_in,
	    sizeof(daemon_kiss_in), &kiss_len, NULL, 0U) !=
	    KILOTNCD_PTY_OK)
		return 1;
	if (kilotncd_init_tnc(&tnc, config) != 0)
		return 1;
	if (tnc1200_host_input(&tnc, daemon_kiss_in, kiss_len) != TNC1200_OK)
		return 1;
	if (kilotncd_collect_tx(&tnc, daemon_pcm,
	    sizeof(daemon_pcm) / sizeof(daemon_pcm[0]), &samples) != 0)
		return 1;
	if (kilotncd_write_pcm(config, config->pcm_out, daemon_pcm,
	    samples) != 0)
		return 1;
	diag_fp = kilotncd_diag_stream(config->pcm_out, NULL);
	if (kilotncd_diag(diag_fp, "diag", &tnc) != 0)
		return 1;

	return 0;
}

static int
kilotncd_run_tcp_loopback_once(const struct kilotncd_config *config)
{
	struct kilotncd_loopback_ctx ctx;
	size_t kiss_len;

	if (config->have_kiss_out || config->have_kiss_in ||
	    config->have_pcm_in || config->have_pcm_out)
		return 1;
	if (kilotncd_tcp_ready(config) != 0 ||
	    kilotncd_mode_implemented(config->mode) != 0)
		return 1;
	(void)memset(&ctx, 0, sizeof(ctx));
	ctx.config = config;
	if (kilotncd_tcp_server_transaction(&config->kiss_tcp_addr,
	    daemon_kiss_in, sizeof(daemon_kiss_in), &kiss_len,
	    daemon_kiss_out, sizeof(daemon_kiss_out),
	    kilotncd_loopback_handler, &ctx) != KILOTNCD_TCP_OK)
		return 1;
	if (kilotncd_diag(stdout, "tx_diag", &ctx.tx_tnc) != 0 ||
	    kilotncd_diag(stdout, "rx_diag", &ctx.rx_tnc) != 0)
		return 1;

	return 0;
}

static int
kilotncd_run_tcp_tx_once(const struct kilotncd_config *config)
{
	struct tnc1200 tnc;
	FILE *diag_fp;
	size_t kiss_len;
	size_t samples;

	if (kilotncd_tcp_ready(config) != 0 ||
	    kilotncd_mode_implemented(config->mode) != 0)
		return 1;
	if (kilotncd_tcp_server_once(&config->kiss_tcp_addr, daemon_kiss_in,
	    sizeof(daemon_kiss_in), &kiss_len, NULL, 0U) !=
	    KILOTNCD_TCP_OK)
		return 1;
	if (kilotncd_init_tnc(&tnc, config) != 0)
		return 1;
	if (tnc1200_host_input(&tnc, daemon_kiss_in, kiss_len) != TNC1200_OK)
		return 1;
	if (kilotncd_collect_tx(&tnc, daemon_pcm,
	    sizeof(daemon_pcm) / sizeof(daemon_pcm[0]), &samples) != 0)
		return 1;
	if (kilotncd_write_pcm(config, config->pcm_out, daemon_pcm,
	    samples) != 0)
		return 1;
	diag_fp = kilotncd_diag_stream(config->pcm_out, NULL);
	if (kilotncd_diag(diag_fp, "diag", &tnc) != 0)
		return 1;

	return 0;
}

static int
kilotncd_run_rx_once(const struct kilotncd_config *config)
{
	struct tnc1200 tnc;
	FILE *diag_fp;
	size_t out_len;
	size_t samples;

	if (kilotncd_mode_implemented(config->mode) != 0)
		return 1;
	if (kilotncd_read_pcm(config, config->pcm_in, daemon_pcm,
	    sizeof(daemon_pcm) / sizeof(daemon_pcm[0]), &samples) != 0)
		return 1;
	if (kilotncd_init_tnc(&tnc, config) != 0)
		return 1;
	if (tnc1200_rx_process(&tnc, daemon_pcm, samples, daemon_kiss_out,
	    sizeof(daemon_kiss_out), &out_len) != TNC1200_OK)
		return 1;
	if (kilotncd_file_write_bytes(config->kiss_out, daemon_kiss_out,
	    out_len) != KILOTNCD_FILE_OK)
		return 1;
	diag_fp = kilotncd_diag_stream(config->kiss_out, NULL);
	if (kilotncd_diag(diag_fp, "diag", &tnc) != 0)
		return 1;

	return 0;
}

static int
kilotncd_run_unix_tx_once(const struct kilotncd_config *config)
{
	struct tnc1200 tnc;
	FILE *diag_fp;
	size_t kiss_len;
	size_t samples;

	if (kilotncd_unix_ready(config) != 0 ||
	    kilotncd_mode_implemented(config->mode) != 0)
		return 1;
	if (kilotncd_unix_server_once(config->kiss_unix_listen,
	    config->unlink_stale_socket, daemon_kiss_in,
	    sizeof(daemon_kiss_in), &kiss_len, NULL, 0U) !=
	    KILOTNCD_UNIX_OK)
		return 1;
	if (kilotncd_init_tnc(&tnc, config) != 0)
		return 1;
	if (tnc1200_host_input(&tnc, daemon_kiss_in, kiss_len) != TNC1200_OK)
		return 1;
	if (kilotncd_collect_tx(&tnc, daemon_pcm,
	    sizeof(daemon_pcm) / sizeof(daemon_pcm[0]), &samples) != 0)
		return 1;
	if (kilotncd_write_pcm(config, config->pcm_out, daemon_pcm,
	    samples) != 0)
		return 1;
	diag_fp = kilotncd_diag_stream(config->pcm_out, NULL);
	if (kilotncd_diag(diag_fp, "diag", &tnc) != 0)
		return 1;

	return 0;
}

static int
kilotncd_run_status(const struct kilotncd_config *config)
{
	struct tnc1200 tnc;
	const struct tnc_mode_desc *desc;

	if (tnc_mode_get(config->mode, &desc) != TNC_MODE_OK)
		return 1;
	if (kilotncd_init_tnc(&tnc, config) != 0)
		return 1;
	(void)printf("mode=%s\n", desc->name);
	(void)printf("support=%s\n", kilotncd_support_name(desc->support));
	(void)printf("temporary=%d\n", config->mode_temporary);
	if (kilotncd_diag(stdout, "diag", &tnc) != 0)
		return 1;

	return 0;
}

static int
kilotncd_run_tx_once(const struct kilotncd_config *config)
{
	struct tnc1200 tnc;
	FILE *diag_fp;
	size_t kiss_len;
	size_t samples;

	if (kilotncd_mode_implemented(config->mode) != 0)
		return 1;
	if (kilotncd_file_read_bytes(config->kiss_in, daemon_kiss_in,
	    sizeof(daemon_kiss_in), &kiss_len) != KILOTNCD_FILE_OK)
		return 1;
	if (kilotncd_init_tnc(&tnc, config) != 0)
		return 1;
	if (tnc1200_host_input(&tnc, daemon_kiss_in, kiss_len) != TNC1200_OK)
		return 1;
	if (kilotncd_collect_tx(&tnc, daemon_pcm,
	    sizeof(daemon_pcm) / sizeof(daemon_pcm[0]), &samples) != 0)
		return 1;
	if (kilotncd_write_pcm(config, config->pcm_out, daemon_pcm,
	    samples) != 0)
		return 1;
	diag_fp = kilotncd_diag_stream(config->pcm_out, NULL);
	if (kilotncd_diag(diag_fp, "diag", &tnc) != 0)
		return 1;

	return 0;
}

static int
kilotncd_pty_ready(const struct kilotncd_config *config)
{
	if (config->kiss_pty == 0U || config->kiss_pty_once == 0U)
		return -1;
	return 0;
}

static int
kilotncd_tcp_ready(const struct kilotncd_config *config)
{
	if (!config->have_kiss_tcp_listen || config->kiss_tcp_once == 0U)
		return -1;
	if (kilotncd_tcp_reject_nonlocal(&config->kiss_tcp_addr,
	    config->allow_nonlocal_bind) != KILOTNCD_TCP_OK) {
		(void)fprintf(stderr, "nonlocal TCP bind rejected: %s\n",
		    config->kiss_tcp_addr.host);
		return -1;
	}
	if (config->allow_nonlocal_bind != 0U)
		(void)fprintf(stderr,
		    "warning: nonlocal TCP bind explicitly enabled\n");

	return 0;
}

static int
kilotncd_unix_ready(const struct kilotncd_config *config)
{
	if (!config->have_kiss_unix_listen || config->kiss_unix_once == 0U)
		return -1;
	return 0;
}

static int
kilotncd_validate_config(const struct kilotncd_config *config)
{
	if ((config->have_kiss_tcp_listen && config->have_kiss_unix_listen) ||
	    (config->have_kiss_tcp_listen && config->kiss_pty != 0U) ||
	    (config->have_kiss_unix_listen && config->kiss_pty != 0U))
		return -1;
	if ((config->have_kiss_tcp_listen || config->have_kiss_unix_listen ||
	    config->kiss_pty != 0U) && config->have_kiss_in)
		return -1;
	if (config->have_kiss_in && config->have_pcm_in &&
	    strcmp(config->kiss_in, "-") == 0 &&
	    strcmp(config->pcm_in, "-") == 0)
		return -1;
	if (config->have_kiss_out && config->have_pcm_out &&
	    strcmp(config->kiss_out, "-") == 0 &&
	    strcmp(config->pcm_out, "-") == 0)
		return -1;
	if (config->audio_backend != KILOTNCD_AUDIO_BACKEND_RAW_FILE)
		return -1;
	if (kilotncd_audio_validate_format(&config->audio_format) !=
	    KILOTNCD_AUDIO_OK)
		return -1;

	return 0;
}

static int
kilotncd_write_pcm(const struct kilotncd_config *config, const char *path,
	const int16_t *pcm, size_t samples)
{
	struct kilotncd_audio audio;
	struct kilotncd_audio_config audio_config;
	enum kilotncd_audio_result res;

	if (kilotncd_audio_config_path(config, path, &audio_config) != 0)
		return -1;
	res = kilotncd_audio_open_output(&audio, &audio_config);
	if (res != KILOTNCD_AUDIO_OK)
		return -1;
	res = kilotncd_audio_write(&audio, pcm, samples);
	if (kilotncd_audio_close(&audio) != KILOTNCD_AUDIO_OK)
		return -1;
	if (res != KILOTNCD_AUDIO_OK)
		return -1;

	return 0;
}

static const char *
kilotncd_support_name(enum tnc_mode_support support)
{
	if (support == TNC_MODE_SUPPORT_IMPLEMENTED)
		return "implemented";
	if (support == TNC_MODE_SUPPORT_PLANNED)
		return "planned";
	if (support == TNC_MODE_SUPPORT_RESEARCH)
		return "research";
	return "unsupported";
}

static void
kilotncd_usage(void)
{
	(void)fprintf(stderr,
	    "usage:\n"
	    "  kilotncd --status [--mode NINO_MODE=6]\n"
	    "  kilotncd --config daemon/example.conf --once\n"
	    "  kilotncd --mode NINO_MODE=6 --kiss-in frame.kiss "
	    "--pcm-out tx.pcm --once\n"
	    "  kilotncd --mode NINO_MODE=6 --pcm-in rx.pcm "
	    "--kiss-out out.kiss --once\n"
	    "  kilotncd --mode NINO_MODE=6 --kiss-in frame.kiss "
	    "--kiss-out out.kiss --loopback-once\n"
	    "  kilotncd --kiss-tcp-listen 127.0.0.1:8001 "
	    "--kiss-tcp-once --pcm-out tx.pcm --once\n"
	    "  kilotncd --kiss-unix-listen build/daemon/kilotnc.sock "
	    "--kiss-unix-once --pcm-out tx.pcm --once\n"
	    "  kilotncd --kiss-pty --kiss-pty-once "
	    "--pty-path-out build/daemon/kilotnc.pty "
	    "--pcm-out tx.pcm --once\n");
}
