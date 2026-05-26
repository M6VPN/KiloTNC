/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/tools/kilotnc_cli.c */

#include <sys/types.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "afsk1200.h"
#include "ax25.h"
#include "kiss.h"
#include "tnc1200.h"
#include "tnc_diag.h"
#include "tnc_mode.h"
#include "wav_writer.h"

#define CLI_KISS_MAX_BYTES	(KILOTNC_KISS_MAX_FRAME * 8U)
#define CLI_PCM_MAX_SAMPLES \
	((AFSK1200_MAX_TEST_BITS + (80U * 8U)) * AFSK1200_SAMPLES_PER_BIT)
#define CLI_DIAG_MAX		2048U
#define CLI_PATH_MAX		512U
#define CLI_TX_CHUNK		257U
#define CLI_DEFAULT_DST		"APZKTN"
#define CLI_DEFAULT_SRC		"M6VPN"
#define CLI_DEFAULT_INFO	"KiloTNC test"

static uint8_t cli_kiss_in[CLI_KISS_MAX_BYTES];
static uint8_t cli_kiss_out[CLI_KISS_MAX_BYTES];
static int16_t cli_pcm[CLI_PCM_MAX_SAMPLES];
static char cli_diag_buf[CLI_DIAG_MAX];
static char cli_path_kiss[CLI_PATH_MAX];
static char cli_path_pcm[CLI_PATH_MAX];
static char cli_path_wav[CLI_PATH_MAX];
static char cli_path_out_kiss[CLI_PATH_MAX];
static char cli_path_diag[CLI_PATH_MAX];

struct cli_args {
	const char *in_path;
	const char *out_path;
	const char *mode_text;
	const char *dst;
	const char *src;
	const char *info;
	const char *prefix;
};

static int cli_apply_mode(struct tnc1200 *, enum tnc_mode_id, int);
static int cli_build_path(char *, size_t, const char *, const char *);
static int cli_build_vector_kiss(const struct cli_args *, uint8_t *,
	size_t, size_t *);
static int cli_collect_tx(struct tnc1200 *, int16_t *, size_t, size_t *);
static int cli_command_diag(const struct cli_args *);
static int cli_command_generate_kiss(const struct cli_args *);
static int cli_command_generate_pcm(const struct cli_args *);
static int cli_command_generate_wav(const struct cli_args *);
static int cli_command_kiss_to_pcm(const struct cli_args *);
static int cli_command_loopback(const struct cli_args *);
static int cli_command_mode(const struct cli_args *);
static int cli_command_pcm_to_kiss(const struct cli_args *);
static int cli_command_vector_loopback(const struct cli_args *);
static int cli_emit_diag(const char *, const struct tnc1200 *);
static int cli_format_diag_line(const char *, const struct tnc1200 *, char *,
	size_t, size_t *);
static int cli_generate_pcm_from_kiss(const struct cli_args *,
	const uint8_t *, size_t, int16_t *, size_t, size_t *,
	struct tnc1200 *);
static int cli_init_vector_paths(const char *);
static int cli_mode_from_args(const struct cli_args *, enum tnc_mode_id *,
	int *);
static int cli_parse_args(int, char **, struct cli_args *);
static int cli_set_call(char *, size_t, const char *);
static const char *cli_support_name(enum tnc_mode_support);
static int cli_read_bytes(const char *, uint8_t *, size_t, size_t *);
static int cli_read_pcm_le(const char *, int16_t *, size_t, size_t *);
static int cli_require_implemented(enum tnc_mode_id);
static void cli_usage(void);
static int cli_write_bytes(const char *, const uint8_t *, size_t);
static int cli_write_diag_file(const char *, const struct tnc1200 *,
	const struct tnc1200 *);
static int cli_write_pcm_le(const char *, const int16_t *, size_t);

int
main(int argc, char *argv[])
{
	struct cli_args args;

	if (argc < 2) {
		cli_usage();
		return 1;
	}
	if (cli_parse_args(argc - 2, &argv[2], &args) != 0) {
		cli_usage();
		return 1;
	}

	if (strcmp(argv[1], "mode") == 0)
		return cli_command_mode(&args);
	if (strcmp(argv[1], "inspect-mode") == 0)
		return cli_command_mode(&args);
	if (strcmp(argv[1], "generate-kiss") == 0)
		return cli_command_generate_kiss(&args);
	if (strcmp(argv[1], "generate-pcm") == 0)
		return cli_command_generate_pcm(&args);
	if (strcmp(argv[1], "generate-wav") == 0)
		return cli_command_generate_wav(&args);
	if (strcmp(argv[1], "vector-loopback") == 0)
		return cli_command_vector_loopback(&args);
	if (strcmp(argv[1], "kiss-to-pcm") == 0)
		return cli_command_kiss_to_pcm(&args);
	if (strcmp(argv[1], "pcm-to-kiss") == 0)
		return cli_command_pcm_to_kiss(&args);
	if (strcmp(argv[1], "loopback") == 0)
		return cli_command_loopback(&args);
	if (strcmp(argv[1], "diag") == 0)
		return cli_command_diag(&args);

	cli_usage();
	return 1;
}

static int
cli_apply_mode(struct tnc1200 *tnc, enum tnc_mode_id mode, int temporary)
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
cli_build_path(char *out, size_t out_cap, const char *prefix,
	const char *suffix)
{
	int n;

	if (out == NULL || prefix == NULL || suffix == NULL || out_cap == 0U)
		return -1;
	n = snprintf(out, out_cap, "%s%s", prefix, suffix);
	if (n < 0 || (size_t)n >= out_cap)
		return -1;

	return 0;
}

static int
cli_build_vector_kiss(const struct cli_args *args, uint8_t *out,
	size_t out_cap, size_t *out_len)
{
	struct ax25_frame frame;
	uint8_t ax25[KILOTNC_AX25_MAX_FRAME];
	const char *dst;
	const char *src;
	const char *info;
	size_t ax25_len;
	size_t info_len;

	if (args == NULL || out == NULL || out_len == NULL)
		return -1;

	dst = args->dst == NULL ? CLI_DEFAULT_DST : args->dst;
	src = args->src == NULL ? CLI_DEFAULT_SRC : args->src;
	info = args->info == NULL ? CLI_DEFAULT_INFO : args->info;

	(void)memset(&frame, 0, sizeof(frame));
	if (cli_set_call(frame.dst.callsign, sizeof(frame.dst.callsign),
	    dst) != 0 ||
	    cli_set_call(frame.src.callsign, sizeof(frame.src.callsign),
	    src) != 0) {
		(void)fprintf(stderr, "invalid callsign\n");
		return -1;
	}
	frame.dst.ssid = 0U;
	frame.src.ssid = 0U;
	frame.pid = AX25_PID_NONE;
	info_len = strlen(info);
	if (info_len > sizeof(frame.info)) {
		(void)fprintf(stderr, "info too large\n");
		return -1;
	}
	(void)memcpy(frame.info, info, info_len);
	frame.info_len = info_len;

	if (!ax25_is_valid_addr(&frame.dst) ||
	    !ax25_is_valid_addr(&frame.src)) {
		(void)fprintf(stderr, "invalid callsign\n");
		return -1;
	}
	if (ax25_encode_ui_fcs(&frame, ax25, sizeof(ax25), &ax25_len) !=
	    AX25_OK)
		return -1;
	if (kiss_encode_frame(0U, KISS_CMD_DATA, ax25, ax25_len, out,
	    out_cap, out_len) != KISS_OK)
		return -1;

	return 0;
}

static int
cli_collect_tx(struct tnc1200 *tnc, int16_t *pcm, size_t pcm_cap,
	size_t *sample_count)
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
		if (chunk > CLI_TX_CHUNK)
			chunk = CLI_TX_CHUNK;
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
	*sample_count = off;

	return 0;
}

static int
cli_command_diag(const struct cli_args *args)
{
	struct tnc1200 tnc;
	enum tnc_mode_id mode;
	int temporary;

	if (cli_mode_from_args(args, &mode, &temporary) != 0)
		return 1;
	if (tnc1200_init(&tnc, NULL) != TNC1200_OK)
		return 1;
	if (cli_apply_mode(&tnc, mode, temporary) != 0)
		return 1;
	if (cli_emit_diag("diag", &tnc) != 0)
		return 1;

	return 0;
}

static int
cli_command_generate_kiss(const struct cli_args *args)
{
	size_t kiss_len;

	if (args->out_path == NULL) {
		cli_usage();
		return 1;
	}
	if (cli_build_vector_kiss(args, cli_kiss_in, sizeof(cli_kiss_in),
	    &kiss_len) != 0)
		return 1;
	if (cli_write_bytes(args->out_path, cli_kiss_in, kiss_len) != 0)
		return 1;
	(void)printf("generated kiss bytes=%lu\n",
	    (unsigned long)kiss_len);

	return 0;
}

static int
cli_command_generate_pcm(const struct cli_args *args)
{
	struct tnc1200 tnc;
	size_t kiss_len;
	size_t samples;

	if (args->out_path == NULL) {
		cli_usage();
		return 1;
	}
	if (cli_build_vector_kiss(args, cli_kiss_in, sizeof(cli_kiss_in),
	    &kiss_len) != 0)
		return 1;
	if (cli_generate_pcm_from_kiss(args, cli_kiss_in, kiss_len, cli_pcm,
	    sizeof(cli_pcm) / sizeof(cli_pcm[0]), &samples, &tnc) != 0)
		return 1;
	if (cli_write_pcm_le(args->out_path, cli_pcm, samples) != 0)
		return 1;
	if (cli_emit_diag("diag", &tnc) != 0)
		return 1;
	(void)printf("generated pcm samples=%lu\n",
	    (unsigned long)samples);

	return 0;
}

static int
cli_command_generate_wav(const struct cli_args *args)
{
	struct tnc1200 tnc;
	size_t kiss_len;
	size_t samples;

	if (args->out_path == NULL) {
		cli_usage();
		return 1;
	}
	if (cli_build_vector_kiss(args, cli_kiss_in, sizeof(cli_kiss_in),
	    &kiss_len) != 0)
		return 1;
	if (cli_generate_pcm_from_kiss(args, cli_kiss_in, kiss_len, cli_pcm,
	    sizeof(cli_pcm) / sizeof(cli_pcm[0]), &samples, &tnc) != 0)
		return 1;
	if (wav_writer_write_pcm16_mono(args->out_path, cli_pcm, samples,
	    AFSK1200_SAMPLE_RATE) != WAV_WRITER_OK)
		return 1;
	if (cli_emit_diag("diag", &tnc) != 0)
		return 1;
	(void)printf("generated wav samples=%lu\n",
	    (unsigned long)samples);

	return 0;
}

static int
cli_command_kiss_to_pcm(const struct cli_args *args)
{
	struct tnc1200 tnc;
	enum tnc_mode_id mode;
	size_t kiss_len;
	size_t samples;
	int temporary;

	if (args->in_path == NULL || args->out_path == NULL) {
		cli_usage();
		return 1;
	}
	if (cli_mode_from_args(args, &mode, &temporary) != 0)
		return 1;
	if (cli_require_implemented(mode) != 0)
		return 1;
	if (cli_read_bytes(args->in_path, cli_kiss_in, sizeof(cli_kiss_in),
	    &kiss_len) != 0)
		return 1;
	if (tnc1200_init(&tnc, NULL) != TNC1200_OK)
		return 1;
	if (cli_apply_mode(&tnc, mode, temporary) != 0)
		return 1;
	if (tnc1200_host_input(&tnc, cli_kiss_in, kiss_len) != TNC1200_OK)
		return 1;
	if (cli_collect_tx(&tnc, cli_pcm,
	    sizeof(cli_pcm) / sizeof(cli_pcm[0]), &samples) != 0)
		return 1;
	if (cli_write_pcm_le(args->out_path, cli_pcm, samples) != 0)
		return 1;
	if (cli_emit_diag("diag", &tnc) != 0)
		return 1;

	return 0;
}

static int
cli_command_vector_loopback(const struct cli_args *args)
{
	struct tnc1200 rx_tnc;
	struct tnc1200 tx_tnc;
	size_t kiss_len;
	size_t out_len;
	size_t samples;

	if (args->prefix == NULL) {
		cli_usage();
		return 1;
	}
	if (cli_init_vector_paths(args->prefix) != 0)
		return 1;
	if (cli_build_vector_kiss(args, cli_kiss_in, sizeof(cli_kiss_in),
	    &kiss_len) != 0)
		return 1;
	if (cli_generate_pcm_from_kiss(args, cli_kiss_in, kiss_len, cli_pcm,
	    sizeof(cli_pcm) / sizeof(cli_pcm[0]), &samples, &tx_tnc) != 0)
		return 1;
	if (tnc1200_init(&rx_tnc, NULL) != TNC1200_OK)
		return 1;
	if (tnc1200_rx_process(&rx_tnc, cli_pcm, samples, cli_kiss_out,
	    sizeof(cli_kiss_out), &out_len) != TNC1200_OK)
		return 1;
	if (cli_write_bytes(cli_path_kiss, cli_kiss_in, kiss_len) != 0 ||
	    cli_write_pcm_le(cli_path_pcm, cli_pcm, samples) != 0 ||
	    wav_writer_write_pcm16_mono(cli_path_wav, cli_pcm, samples,
	    AFSK1200_SAMPLE_RATE) != WAV_WRITER_OK ||
	    cli_write_bytes(cli_path_out_kiss, cli_kiss_out, out_len) != 0 ||
	    cli_write_diag_file(cli_path_diag, &tx_tnc, &rx_tnc) != 0)
		return 1;
	(void)printf("vector-loopback ok kiss=%lu pcm_samples=%lu "
	    "out_kiss=%lu\n", (unsigned long)kiss_len,
	    (unsigned long)samples, (unsigned long)out_len);

	return 0;
}

static int
cli_command_loopback(const struct cli_args *args)
{
	struct tnc1200 rx_tnc;
	struct tnc1200 tx_tnc;
	enum tnc_mode_id mode;
	size_t kiss_len;
	size_t out_len;
	size_t samples;
	int temporary;

	if (args->in_path == NULL || args->out_path == NULL) {
		cli_usage();
		return 1;
	}
	if (cli_mode_from_args(args, &mode, &temporary) != 0)
		return 1;
	if (cli_require_implemented(mode) != 0)
		return 1;
	if (cli_read_bytes(args->in_path, cli_kiss_in, sizeof(cli_kiss_in),
	    &kiss_len) != 0)
		return 1;
	if (tnc1200_init(&tx_tnc, NULL) != TNC1200_OK)
		return 1;
	if (tnc1200_init(&rx_tnc, NULL) != TNC1200_OK)
		return 1;
	if (cli_apply_mode(&tx_tnc, mode, temporary) != 0 ||
	    cli_apply_mode(&rx_tnc, mode, temporary) != 0)
		return 1;
	if (tnc1200_host_input(&tx_tnc, cli_kiss_in, kiss_len) != TNC1200_OK)
		return 1;
	if (cli_collect_tx(&tx_tnc, cli_pcm,
	    sizeof(cli_pcm) / sizeof(cli_pcm[0]), &samples) != 0)
		return 1;
	if (tnc1200_rx_process(&rx_tnc, cli_pcm, samples, cli_kiss_out,
	    sizeof(cli_kiss_out), &out_len) != TNC1200_OK)
		return 1;
	if (cli_write_bytes(args->out_path, cli_kiss_out, out_len) != 0)
		return 1;
	if (cli_emit_diag("tx_diag", &tx_tnc) != 0)
		return 1;
	if (cli_emit_diag("rx_diag", &rx_tnc) != 0)
		return 1;

	return 0;
}

static int
cli_command_mode(const struct cli_args *args)
{
	const struct tnc_mode_desc *desc;
	enum tnc_mode_id mode;
	int temporary;

	if (args->mode_text == NULL) {
		cli_usage();
		return 1;
	}
	if (tnc_mode_parse_option(args->mode_text, &mode, &temporary) !=
	    TNC_MODE_OK) {
		(void)fprintf(stderr, "invalid mode: %s\n", args->mode_text);
		return 1;
	}
	if (tnc_mode_get(mode, &desc) != TNC_MODE_OK)
		return 1;

	(void)printf("name=%s\n", desc->name);
	(void)printf("id=%u\n", (unsigned int)desc->id);
	(void)printf("support=%s\n", cli_support_name(desc->support));
	(void)printf("nino_switch=%u\n",
	    (unsigned int)desc->nino_switch_mode);
	(void)printf("nino_sethw=%u\n",
	    (unsigned int)desc->nino_sethw_persistent);
	(void)printf("nino_sethw_temp=%u\n",
	    (unsigned int)desc->nino_sethw_temporary);
	(void)printf("temporary=%d\n", temporary);

	return 0;
}

static int
cli_command_pcm_to_kiss(const struct cli_args *args)
{
	struct tnc1200 tnc;
	enum tnc_mode_id mode;
	size_t out_len;
	size_t samples;
	int temporary;

	if (args->in_path == NULL || args->out_path == NULL) {
		cli_usage();
		return 1;
	}
	if (cli_mode_from_args(args, &mode, &temporary) != 0)
		return 1;
	if (cli_require_implemented(mode) != 0)
		return 1;
	if (cli_read_pcm_le(args->in_path, cli_pcm,
	    sizeof(cli_pcm) / sizeof(cli_pcm[0]), &samples) != 0)
		return 1;
	if (tnc1200_init(&tnc, NULL) != TNC1200_OK)
		return 1;
	if (cli_apply_mode(&tnc, mode, temporary) != 0)
		return 1;
	if (tnc1200_rx_process(&tnc, cli_pcm, samples, cli_kiss_out,
	    sizeof(cli_kiss_out), &out_len) != TNC1200_OK)
		return 1;
	if (cli_write_bytes(args->out_path, cli_kiss_out, out_len) != 0)
		return 1;
	if (cli_emit_diag("diag", &tnc) != 0)
		return 1;

	return 0;
}

static int
cli_emit_diag(const char *prefix, const struct tnc1200 *tnc)
{
	size_t len;

	if (cli_format_diag_line(prefix, tnc, cli_diag_buf,
	    sizeof(cli_diag_buf), &len) != 0)
		return -1;
	(void)printf("%s", cli_diag_buf);

	return 0;
}

static int
cli_format_diag_line(const char *prefix, const struct tnc1200 *tnc,
	char *buf, size_t buf_cap, size_t *len)
{
	struct tnc_diag diag;
	struct tnc_diag_snapshot snapshot;
	char snapshot_buf[CLI_DIAG_MAX];
	size_t snapshot_len;
	int n;

	if (prefix == NULL || tnc == NULL || buf == NULL || len == NULL ||
	    buf_cap == 0U)
		return -1;
	if (tnc_diag_init(&diag) != TNC_DIAG_OK)
		return -1;
	if (tnc_diag_capture_tnc1200(&diag, tnc) != TNC_DIAG_OK)
		return -1;
	if (tnc_diag_snapshot(&diag, &snapshot) != TNC_DIAG_OK)
		return -1;
	if (tnc_diag_format_snapshot(&snapshot, snapshot_buf,
	    sizeof(snapshot_buf), &snapshot_len) != TNC_DIAG_OK)
		return -1;
	n = snprintf(buf, buf_cap, "%s %s\n", prefix, snapshot_buf);
	if (n < 0 || (size_t)n >= buf_cap)
		return -1;
	*len = (size_t)n;

	return 0;
}

static int
cli_generate_pcm_from_kiss(const struct cli_args *args, const uint8_t *kiss,
	size_t kiss_len, int16_t *pcm, size_t pcm_cap, size_t *samples,
	struct tnc1200 *tnc)
{
	enum tnc_mode_id mode;
	int temporary;

	if (args == NULL || kiss == NULL || pcm == NULL || samples == NULL ||
	    tnc == NULL)
		return -1;
	if (cli_mode_from_args(args, &mode, &temporary) != 0)
		return -1;
	if (cli_require_implemented(mode) != 0)
		return -1;
	if (tnc1200_init(tnc, NULL) != TNC1200_OK)
		return -1;
	if (cli_apply_mode(tnc, mode, temporary) != 0)
		return -1;
	if (tnc1200_host_input(tnc, kiss, kiss_len) != TNC1200_OK)
		return -1;
	if (cli_collect_tx(tnc, pcm, pcm_cap, samples) != 0)
		return -1;

	return 0;
}

static int
cli_init_vector_paths(const char *prefix)
{
	if (cli_build_path(cli_path_kiss, sizeof(cli_path_kiss), prefix,
	    ".kiss") != 0 ||
	    cli_build_path(cli_path_pcm, sizeof(cli_path_pcm), prefix,
	    ".pcm") != 0 ||
	    cli_build_path(cli_path_wav, sizeof(cli_path_wav), prefix,
	    ".wav") != 0 ||
	    cli_build_path(cli_path_out_kiss, sizeof(cli_path_out_kiss),
	    prefix, ".out.kiss") != 0 ||
	    cli_build_path(cli_path_diag, sizeof(cli_path_diag), prefix,
	    ".diag.txt") != 0) {
		(void)fprintf(stderr, "prefix path too long\n");
		return -1;
	}

	return 0;
}

static int
cli_mode_from_args(const struct cli_args *args, enum tnc_mode_id *mode,
	int *temporary)
{
	if (args->mode_text == NULL) {
		*temporary = 0;
		return tnc_mode_default(mode) == TNC_MODE_OK ? 0 : -1;
	}
	if (tnc_mode_parse_option(args->mode_text, mode, temporary) !=
	    TNC_MODE_OK) {
		(void)fprintf(stderr, "invalid mode: %s\n", args->mode_text);
		return -1;
	}

	return 0;
}

static int
cli_parse_args(int argc, char **argv, struct cli_args *args)
{
	int i;

	(void)memset(args, 0, sizeof(*args));
	for (i = 0; i < argc; i++) {
		if (strcmp(argv[i], "--in") == 0) {
			if (i + 1 >= argc)
				return -1;
			args->in_path = argv[++i];
		} else if (strcmp(argv[i], "--out") == 0) {
			if (i + 1 >= argc)
				return -1;
			args->out_path = argv[++i];
		} else if (strcmp(argv[i], "--mode") == 0) {
			if (i + 1 >= argc)
				return -1;
			args->mode_text = argv[++i];
		} else if (strcmp(argv[i], "--dst") == 0) {
			if (i + 1 >= argc)
				return -1;
			args->dst = argv[++i];
		} else if (strcmp(argv[i], "--src") == 0) {
			if (i + 1 >= argc)
				return -1;
			args->src = argv[++i];
		} else if (strcmp(argv[i], "--info") == 0) {
			if (i + 1 >= argc)
				return -1;
			args->info = argv[++i];
		} else if (strcmp(argv[i], "--prefix") == 0) {
			if (i + 1 >= argc)
				return -1;
			args->prefix = argv[++i];
		} else {
			return -1;
		}
	}

	return 0;
}

static int
cli_set_call(char *dst, size_t dst_cap, const char *src)
{
	size_t len;

	if (dst == NULL || src == NULL || dst_cap == 0U)
		return -1;
	len = strlen(src);
	if (len == 0U || len >= dst_cap ||
	    len > KILOTNC_AX25_MAX_CALLSIGN)
		return -1;
	(void)memcpy(dst, src, len);
	dst[len] = '\0';

	return 0;
}

static const char *
cli_support_name(enum tnc_mode_support support)
{
	if (support == TNC_MODE_SUPPORT_IMPLEMENTED)
		return "implemented";
	if (support == TNC_MODE_SUPPORT_PLANNED)
		return "planned";
	if (support == TNC_MODE_SUPPORT_RESEARCH)
		return "research";
	return "unsupported";
}

static int
cli_read_bytes(const char *path, uint8_t *buf, size_t cap, size_t *len)
{
	FILE *fp;
	int extra;

	fp = fopen(path, "rb");
	if (fp == NULL) {
		(void)fprintf(stderr, "open failed: %s\n", path);
		return -1;
	}
	*len = fread(buf, 1U, cap, fp);
	if (ferror(fp)) {
		(void)fclose(fp);
		return -1;
	}
	extra = fgetc(fp);
	(void)fclose(fp);
	if (extra != EOF) {
		(void)fprintf(stderr, "input too large: %s\n", path);
		return -1;
	}

	return 0;
}

static int
cli_read_pcm_le(const char *path, int16_t *pcm, size_t cap,
	size_t *samples)
{
	FILE *fp;
	int lo;
	int hi;
	uint16_t raw;

	fp = fopen(path, "rb");
	if (fp == NULL) {
		(void)fprintf(stderr, "open failed: %s\n", path);
		return -1;
	}
	*samples = 0U;
	for (;;) {
		lo = fgetc(fp);
		if (lo == EOF)
			break;
		hi = fgetc(fp);
		if (hi == EOF) {
			(void)fclose(fp);
			(void)fprintf(stderr, "odd pcm byte count: %s\n", path);
			return -1;
		}
		if (*samples >= cap) {
			(void)fclose(fp);
			(void)fprintf(stderr, "pcm input too large: %s\n", path);
			return -1;
		}
		raw = (uint16_t)((uint16_t)(uint8_t)lo |
		    (uint16_t)((uint16_t)(uint8_t)hi << 8U));
		if (raw <= (uint16_t)INT16_MAX)
			pcm[*samples] = (int16_t)raw;
		else
			pcm[*samples] = (int16_t)(-1 -
			    (int16_t)(UINT16_MAX - raw));
		(*samples)++;
	}
	if (ferror(fp)) {
		(void)fclose(fp);
		return -1;
	}
	(void)fclose(fp);

	return 0;
}

static int
cli_require_implemented(enum tnc_mode_id mode)
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

static void
cli_usage(void)
{
	(void)fprintf(stderr,
	    "usage:\n"
	    "  kilotnc_cli mode --mode NINO_MODE=6\n"
	    "  kilotnc_cli inspect-mode --mode NINO_MODE=22\n"
	    "  kilotnc_cli generate-kiss --out frame.kiss --dst APZKTN "
	    "--src M6VPN --info \"KiloTNC test\"\n"
	    "  kilotnc_cli generate-pcm --out tx.pcm --dst APZKTN "
	    "--src M6VPN --info \"KiloTNC test\" --mode NINO_MODE=6\n"
	    "  kilotnc_cli generate-wav --out tx.wav --dst APZKTN "
	    "--src M6VPN --info \"KiloTNC test\" --mode NINO_MODE=6\n"
	    "  kilotnc_cli vector-loopback --prefix build/vectors/kilotnc "
	    "--mode NINO_MODE=6\n"
	    "  kilotnc_cli kiss-to-pcm --in frame.kiss --out tx.pcm "
	    "--mode NINO_MODE=6\n"
	    "  kilotnc_cli pcm-to-kiss --in rx.pcm --out frame.kiss "
	    "--mode NINO_MODE=6\n"
	    "  kilotnc_cli loopback --in frame.kiss --out out.kiss "
	    "--mode NINO_MODE=6\n"
	    "  kilotnc_cli diag --mode NINO_MODE=6\n");
}

static int
cli_write_bytes(const char *path, const uint8_t *buf, size_t len)
{
	FILE *fp;

	fp = fopen(path, "wb");
	if (fp == NULL) {
		(void)fprintf(stderr, "open failed: %s\n", path);
		return -1;
	}
	if (fwrite(buf, 1U, len, fp) != len) {
		(void)fclose(fp);
		return -1;
	}
	if (fclose(fp) != 0)
		return -1;

	return 0;
}

static int
cli_write_diag_file(const char *path, const struct tnc1200 *tx_tnc,
	const struct tnc1200 *rx_tnc)
{
	FILE *fp;
	size_t len;

	if (path == NULL || tx_tnc == NULL || rx_tnc == NULL)
		return -1;
	fp = fopen(path, "wb");
	if (fp == NULL) {
		(void)fprintf(stderr, "open failed: %s\n", path);
		return -1;
	}
	if (cli_format_diag_line("tx_diag", tx_tnc, cli_diag_buf,
	    sizeof(cli_diag_buf), &len) != 0 ||
	    fwrite(cli_diag_buf, 1U, len, fp) != len ||
	    cli_format_diag_line("rx_diag", rx_tnc, cli_diag_buf,
	    sizeof(cli_diag_buf), &len) != 0 ||
	    fwrite(cli_diag_buf, 1U, len, fp) != len) {
		(void)fclose(fp);
		return -1;
	}
	if (fclose(fp) != 0)
		return -1;

	return 0;
}

static int
cli_write_pcm_le(const char *path, const int16_t *pcm, size_t samples)
{
	FILE *fp;
	size_t i;
	uint16_t raw;

	fp = fopen(path, "wb");
	if (fp == NULL) {
		(void)fprintf(stderr, "open failed: %s\n", path);
		return -1;
	}
	for (i = 0U; i < samples; i++) {
		if (pcm[i] < 0)
			raw = (uint16_t)(32768U +
			    (uint16_t)(pcm[i] - INT16_MIN));
		else
			raw = (uint16_t)pcm[i];
		if (fputc((int)(raw & 0xFFU), fp) == EOF ||
		    fputc((int)((raw >> 8U) & 0xFFU), fp) == EOF) {
			(void)fclose(fp);
			return -1;
		}
	}
	if (fclose(fp) != 0)
		return -1;

	return 0;
}
