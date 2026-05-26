/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/daemon/kilotncd_kiss_test.c */

#include <sys/types.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ax25.h"
#include "kiss.h"
#include "kilotncd_kiss_test.h"

#define KISS_TEST_BUF_MAX	8192U
#define KISS_TEST_FRAME_MAX	16U
#define KISS_TEST_PATH_MAX	512U

static const uint8_t kiss_test_plain_info[] = "KiloTNC plain";
static const uint8_t kiss_test_escaped_info[] = {
	'K', 'i', 'l', 'o', 'T', 'N', 'C', ' ',
	KISS_FEND, ' ', KISS_FESC, ' ', 'e', 's', 'c'
};

static uint8_t kiss_test_buf[KISS_TEST_BUF_MAX];
static uint8_t kiss_test_tmp[KISS_TEST_BUF_MAX];

static int kiss_test_append_data(uint8_t *, size_t, size_t *,
	const uint8_t *, size_t);
static int kiss_test_append_frame(uint8_t *, size_t, size_t *, uint8_t,
	const uint8_t *, size_t);
static int kiss_test_build_ax25(const uint8_t *, size_t, uint8_t *, size_t,
	size_t *);
static int kiss_test_build_commands(uint8_t *, size_t, size_t *);
static int kiss_test_build_escaped(uint8_t *, size_t, size_t *);
static int kiss_test_build_malformed(uint8_t *, size_t, size_t *);
static int kiss_test_build_multi(uint8_t *, size_t, size_t *);
static int kiss_test_build_plain(uint8_t *, size_t, size_t *);
static int kiss_test_build_repeated_fend(uint8_t *, size_t, size_t *);
static int kiss_test_copy_call(char *, size_t, const char *);
static int kiss_test_expect_frames(const struct kiss_frame *, size_t,
	const char *);
static int kiss_test_frame_matches(const struct kiss_frame *,
	const uint8_t *, size_t);
static int kiss_test_join_path(const char *, const char *, char *, size_t);
static int kiss_test_read_file(const char *, uint8_t *, size_t, size_t *);
static int kiss_test_write_file(const char *, const uint8_t *, size_t);
static void kiss_test_usage(void);

int
kilotncd_kiss_test_expect_file(const char *path, const char *kind)
{
	struct kiss_parser parser;
	struct kiss_frame frames[KISS_TEST_FRAME_MAX];
	size_t frame_count;
	size_t len;

	if (path == NULL || kind == NULL)
		return -1;
	if (kiss_test_read_file(path, kiss_test_buf, sizeof(kiss_test_buf),
	    &len) != 0)
		return -1;
	kiss_parser_init(&parser);
	if (kiss_parse_bytes(&parser, kiss_test_buf, len, frames,
	    sizeof(frames) / sizeof(frames[0]), &frame_count) != KISS_OK)
		return -1;

	return kiss_test_expect_frames(frames, frame_count, kind);
}

int
kilotncd_kiss_test_generate_suite(const char *dir)
{
	char path[KISS_TEST_PATH_MAX];
	size_t len;

	if (dir == NULL)
		return -1;

	if (kiss_test_build_plain(kiss_test_buf, sizeof(kiss_test_buf),
	    &len) != 0)
		return -1;
	if (kiss_test_join_path(dir, "plain.kiss", path, sizeof(path)) != 0 ||
	    kiss_test_write_file(path, kiss_test_buf, len) != 0)
		return -1;

	if (kiss_test_build_escaped(kiss_test_buf, sizeof(kiss_test_buf),
	    &len) != 0)
		return -1;
	if (kiss_test_join_path(dir, "escaped.kiss", path, sizeof(path)) != 0 ||
	    kiss_test_write_file(path, kiss_test_buf, len) != 0)
		return -1;

	if (kiss_test_build_multi(kiss_test_buf, sizeof(kiss_test_buf),
	    &len) != 0)
		return -1;
	if (kiss_test_join_path(dir, "multi.kiss", path, sizeof(path)) != 0 ||
	    kiss_test_write_file(path, kiss_test_buf, len) != 0)
		return -1;

	if (kiss_test_build_commands(kiss_test_buf, sizeof(kiss_test_buf),
	    &len) != 0)
		return -1;
	if (kiss_test_join_path(dir, "commands.kiss", path,
	    sizeof(path)) != 0 ||
	    kiss_test_write_file(path, kiss_test_buf, len) != 0)
		return -1;

	if (kiss_test_build_malformed(kiss_test_buf, sizeof(kiss_test_buf),
	    &len) != 0)
		return -1;
	if (kiss_test_join_path(dir, "malformed.kiss", path,
	    sizeof(path)) != 0 ||
	    kiss_test_write_file(path, kiss_test_buf, len) != 0)
		return -1;

	if (kiss_test_build_repeated_fend(kiss_test_buf, sizeof(kiss_test_buf),
	    &len) != 0)
		return -1;
	if (kiss_test_join_path(dir, "repeated-fend.kiss", path,
	    sizeof(path)) != 0 ||
	    kiss_test_write_file(path, kiss_test_buf, len) != 0)
		return -1;

	return 0;
}

int
main(int argc, char *argv[])
{
	if (argc == 3 && strcmp(argv[1], "generate") == 0)
		return kilotncd_kiss_test_generate_suite(argv[2]) == 0 ? 0 : 1;
	if (argc == 4 && strcmp(argv[1], "expect") == 0)
		return kilotncd_kiss_test_expect_file(argv[2], argv[3]) == 0 ?
		    0 : 1;

	kiss_test_usage();
	return 1;
}

static int
kiss_test_append_data(uint8_t *out, size_t out_cap, size_t *out_len,
	const uint8_t *info, size_t info_len)
{
	size_t ax25_len;

	if (kiss_test_build_ax25(info, info_len, kiss_test_tmp,
	    sizeof(kiss_test_tmp), &ax25_len) != 0)
		return -1;

	return kiss_test_append_frame(out, out_cap, out_len, KISS_CMD_DATA,
	    kiss_test_tmp, ax25_len);
}

static int
kiss_test_append_frame(uint8_t *out, size_t out_cap, size_t *out_len,
	uint8_t command, const uint8_t *data, size_t data_len)
{
	size_t encoded_len;

	if (*out_len > out_cap)
		return -1;
	if (kiss_encode_frame(0U, command, data, data_len, &out[*out_len],
	    out_cap - *out_len, &encoded_len) != KISS_OK)
		return -1;
	*out_len += encoded_len;

	return 0;
}

static int
kiss_test_build_ax25(const uint8_t *info, size_t info_len, uint8_t *out,
	size_t out_cap, size_t *out_len)
{
	struct ax25_frame frame;

	(void)memset(&frame, 0, sizeof(frame));
	if (kiss_test_copy_call(frame.dst.callsign, sizeof(frame.dst.callsign),
	    "APZKTN") != 0)
		return -1;
	if (kiss_test_copy_call(frame.src.callsign, sizeof(frame.src.callsign),
	    "M6VPN") != 0)
		return -1;
	frame.pid = AX25_PID_NONE;
	frame.info_len = info_len;
	if (info_len > sizeof(frame.info))
		return -1;
	if (info_len != 0U)
		(void)memcpy(frame.info, info, info_len);
	if (ax25_encode_ui_fcs(&frame, out, out_cap, out_len) != AX25_OK)
		return -1;

	return 0;
}

static int
kiss_test_build_commands(uint8_t *out, size_t out_cap, size_t *out_len)
{
	uint8_t value;

	*out_len = 0U;
	value = 12U;
	if (kiss_test_append_frame(out, out_cap, out_len, KISS_CMD_TXDELAY,
	    &value, 1U) != 0)
		return -1;
	value = 255U;
	if (kiss_test_append_frame(out, out_cap, out_len, KISS_CMD_P,
	    &value, 1U) != 0)
		return -1;
	value = 10U;
	if (kiss_test_append_frame(out, out_cap, out_len, KISS_CMD_SLOTTIME,
	    &value, 1U) != 0)
		return -1;
	value = 4U;
	if (kiss_test_append_frame(out, out_cap, out_len, KISS_CMD_TXTAIL,
	    &value, 1U) != 0)
		return -1;
	value = 1U;
	if (kiss_test_append_frame(out, out_cap, out_len, KISS_CMD_FULLDUPLEX,
	    &value, 1U) != 0)
		return -1;
	value = 6U;
	if (kiss_test_append_frame(out, out_cap, out_len, KISS_CMD_SETHARDWARE,
	    &value, 1U) != 0)
		return -1;
	value = 22U;
	if (kiss_test_append_frame(out, out_cap, out_len, KISS_CMD_SETHARDWARE,
	    &value, 1U) != 0)
		return -1;
	value = 0xAAU;
	if (kiss_test_append_frame(out, out_cap, out_len, 7U, &value,
	    1U) != 0)
		return -1;

	return kiss_test_append_data(out, out_cap, out_len,
	    kiss_test_plain_info, sizeof(kiss_test_plain_info) - 1U);
}

static int
kiss_test_build_escaped(uint8_t *out, size_t out_cap, size_t *out_len)
{
	*out_len = 0U;
	return kiss_test_append_data(out, out_cap, out_len,
	    kiss_test_escaped_info, sizeof(kiss_test_escaped_info));
}

static int
kiss_test_build_malformed(uint8_t *out, size_t out_cap, size_t *out_len)
{
	if (out_cap < 5U)
		return -1;
	*out_len = 0U;
	out[(*out_len)++] = KISS_FEND;
	out[(*out_len)++] = KISS_CMD_DATA;
	out[(*out_len)++] = KISS_FESC;
	out[(*out_len)++] = 0x00U;
	out[(*out_len)++] = KISS_FEND;

	return kiss_test_append_data(out, out_cap, out_len,
	    kiss_test_plain_info, sizeof(kiss_test_plain_info) - 1U);
}

static int
kiss_test_build_multi(uint8_t *out, size_t out_cap, size_t *out_len)
{
	*out_len = 0U;
	if (kiss_test_append_data(out, out_cap, out_len, kiss_test_plain_info,
	    sizeof(kiss_test_plain_info) - 1U) != 0)
		return -1;

	return kiss_test_append_data(out, out_cap, out_len,
	    kiss_test_escaped_info, sizeof(kiss_test_escaped_info));
}

static int
kiss_test_build_plain(uint8_t *out, size_t out_cap, size_t *out_len)
{
	*out_len = 0U;
	return kiss_test_append_data(out, out_cap, out_len,
	    kiss_test_plain_info, sizeof(kiss_test_plain_info) - 1U);
}

static int
kiss_test_build_repeated_fend(uint8_t *out, size_t out_cap, size_t *out_len)
{
	size_t i;

	if (out_cap < 4U)
		return -1;
	*out_len = 0U;
	for (i = 0U; i < 4U; i++)
		out[(*out_len)++] = KISS_FEND;

	return kiss_test_append_data(out, out_cap, out_len,
	    kiss_test_plain_info, sizeof(kiss_test_plain_info) - 1U);
}

static int
kiss_test_copy_call(char *dst, size_t cap, const char *src)
{
	size_t len;

	len = strlen(src);
	if (cap == 0U || len >= cap)
		return -1;
	(void)memcpy(dst, src, len + 1U);

	return 0;
}

static int
kiss_test_expect_frames(const struct kiss_frame *frames, size_t frame_count,
	const char *kind)
{
	size_t i;
	int plain_seen;
	int escaped_seen;

	plain_seen = 0;
	escaped_seen = 0;
	for (i = 0U; i < frame_count; i++) {
		if (!plain_seen && kiss_test_frame_matches(&frames[i],
		    kiss_test_plain_info, sizeof(kiss_test_plain_info) - 1U))
			plain_seen = 1;
		else if (kiss_test_frame_matches(&frames[i],
		    kiss_test_escaped_info, sizeof(kiss_test_escaped_info)))
			escaped_seen = 1;
	}

	if (strcmp(kind, "plain") == 0)
		return plain_seen ? 0 : -1;
	if (strcmp(kind, "escaped") == 0)
		return escaped_seen ? 0 : -1;
	if (strcmp(kind, "multi") == 0)
		return plain_seen && escaped_seen ? 0 : -1;

	return -1;
}

static int
kiss_test_frame_matches(const struct kiss_frame *frame, const uint8_t *info,
	size_t info_len)
{
	struct ax25_frame decoded;

	if (frame->command != KISS_CMD_DATA)
		return 0;
	if (ax25_decode_ui_fcs(frame->data, frame->len, &decoded) != AX25_OK)
		return 0;
	if (decoded.info_len != info_len)
		return 0;
	if (info_len != 0U && memcmp(decoded.info, info, info_len) != 0)
		return 0;

	return 1;
}

static int
kiss_test_join_path(const char *dir, const char *name, char *path, size_t cap)
{
	int n;

	if (cap == 0U)
		return -1;
	n = snprintf(path, cap, "%s/%s", dir, name);
	if (n < 0 || (size_t)n >= cap)
		return -1;

	return 0;
}

static int
kiss_test_read_file(const char *path, uint8_t *buf, size_t cap, size_t *len)
{
	FILE *fp;
	int extra;

	fp = fopen(path, "rb");
	if (fp == NULL)
		return -1;
	*len = fread(buf, 1U, cap, fp);
	if (ferror(fp)) {
		(void)fclose(fp);
		return -1;
	}
	extra = fgetc(fp);
	if (fclose(fp) != 0)
		return -1;
	if (extra != EOF)
		return -1;

	return 0;
}

static int
kiss_test_write_file(const char *path, const uint8_t *buf, size_t len)
{
	FILE *fp;

	fp = fopen(path, "wb");
	if (fp == NULL)
		return -1;
	if (fwrite(buf, 1U, len, fp) != len) {
		(void)fclose(fp);
		return -1;
	}
	if (fclose(fp) != 0)
		return -1;

	return 0;
}

static void
kiss_test_usage(void)
{
	(void)fprintf(stderr,
	    "usage: kilotncd_kiss_test generate dir\n"
	    "       kilotncd_kiss_test expect file.kiss plain|escaped|multi\n");
}
