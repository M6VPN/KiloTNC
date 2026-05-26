/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/daemon/kilotncd_file.c */

#include <sys/types.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "kilotncd_file.h"

static FILE *kilotncd_file_open_read(const char *, int *);
static FILE *kilotncd_file_open_write(const char *, int *);
static uint16_t kilotncd_file_sample_to_u16(int16_t);
static int16_t kilotncd_file_u16_to_sample(uint16_t);

enum kilotncd_file_result
kilotncd_file_read_bytes(const char *path, uint8_t *buf, size_t cap,
	size_t *len)
{
	FILE *fp;
	int close_file;
	int extra;

	if (path == NULL || buf == NULL || len == NULL)
		return KILOTNCD_FILE_ERR_ARG;
	fp = kilotncd_file_open_read(path, &close_file);
	if (fp == NULL)
		return KILOTNCD_FILE_ERR_IO;
	*len = fread(buf, 1U, cap, fp);
	if (ferror(fp)) {
		if (close_file)
			(void)fclose(fp);
		return KILOTNCD_FILE_ERR_IO;
	}
	extra = fgetc(fp);
	if (close_file && fclose(fp) != 0)
		return KILOTNCD_FILE_ERR_IO;
	if (extra != EOF)
		return KILOTNCD_FILE_ERR_RANGE;

	return KILOTNCD_FILE_OK;
}

enum kilotncd_file_result
kilotncd_file_read_pcm16(const char *path, int16_t *pcm, size_t cap,
	size_t *samples)
{
	FILE *fp;
	int close_file;
	int lo;
	int hi;
	uint16_t raw;

	if (path == NULL || pcm == NULL || samples == NULL)
		return KILOTNCD_FILE_ERR_ARG;
	fp = kilotncd_file_open_read(path, &close_file);
	if (fp == NULL)
		return KILOTNCD_FILE_ERR_IO;
	*samples = 0U;
	for (;;) {
		lo = fgetc(fp);
		if (lo == EOF)
			break;
		hi = fgetc(fp);
		if (hi == EOF) {
			if (close_file)
				(void)fclose(fp);
			return KILOTNCD_FILE_ERR_FORMAT;
		}
		if (*samples >= cap) {
			if (close_file)
				(void)fclose(fp);
			return KILOTNCD_FILE_ERR_RANGE;
		}
		raw = (uint16_t)((uint16_t)(uint8_t)lo |
		    (uint16_t)((uint16_t)(uint8_t)hi << 8U));
		pcm[*samples] = kilotncd_file_u16_to_sample(raw);
		(*samples)++;
	}
	if (ferror(fp)) {
		if (close_file)
			(void)fclose(fp);
		return KILOTNCD_FILE_ERR_IO;
	}
	if (close_file && fclose(fp) != 0)
		return KILOTNCD_FILE_ERR_IO;

	return KILOTNCD_FILE_OK;
}

enum kilotncd_file_result
kilotncd_file_write_bytes(const char *path, const uint8_t *buf, size_t len)
{
	FILE *fp;
	int close_file;

	if (path == NULL || (buf == NULL && len != 0U))
		return KILOTNCD_FILE_ERR_ARG;
	fp = kilotncd_file_open_write(path, &close_file);
	if (fp == NULL)
		return KILOTNCD_FILE_ERR_IO;
	if (fwrite(buf, 1U, len, fp) != len) {
		if (close_file)
			(void)fclose(fp);
		return KILOTNCD_FILE_ERR_IO;
	}
	if (close_file && fclose(fp) != 0)
		return KILOTNCD_FILE_ERR_IO;

	return KILOTNCD_FILE_OK;
}

enum kilotncd_file_result
kilotncd_file_write_pcm16(const char *path, const int16_t *pcm,
	size_t samples)
{
	FILE *fp;
	int close_file;
	size_t i;
	uint16_t raw;

	if (path == NULL || (pcm == NULL && samples != 0U))
		return KILOTNCD_FILE_ERR_ARG;
	fp = kilotncd_file_open_write(path, &close_file);
	if (fp == NULL)
		return KILOTNCD_FILE_ERR_IO;
	for (i = 0U; i < samples; i++) {
		raw = kilotncd_file_sample_to_u16(pcm[i]);
		if (fputc((int)(raw & 0xFFU), fp) == EOF ||
		    fputc((int)((raw >> 8U) & 0xFFU), fp) == EOF) {
			if (close_file)
				(void)fclose(fp);
			return KILOTNCD_FILE_ERR_IO;
		}
	}
	if (close_file && fclose(fp) != 0)
		return KILOTNCD_FILE_ERR_IO;

	return KILOTNCD_FILE_OK;
}

static FILE *
kilotncd_file_open_read(const char *path, int *close_file)
{
	if (strcmp(path, "-") == 0) {
		*close_file = 0;
		return stdin;
	}
	*close_file = 1;
	return fopen(path, "rb");
}

static FILE *
kilotncd_file_open_write(const char *path, int *close_file)
{
	if (strcmp(path, "-") == 0) {
		*close_file = 0;
		return stdout;
	}
	*close_file = 1;
	return fopen(path, "wb");
}

static uint16_t
kilotncd_file_sample_to_u16(int16_t sample)
{
	int32_t value;

	value = sample;
	if (value < 0)
		return (uint16_t)(uint32_t)(value + 65536);
	return (uint16_t)value;
}

static int16_t
kilotncd_file_u16_to_sample(uint16_t raw)
{
	if (raw <= (uint16_t)INT16_MAX)
		return (int16_t)raw;
	return (int16_t)(-1 - (int16_t)(UINT16_MAX - raw));
}
