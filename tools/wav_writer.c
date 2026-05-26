/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/tools/wav_writer.c */

#include <sys/types.h>

#include <stdint.h>
#include <stdio.h>

#include "wav_writer.h"

#define WAV_PCM_FORMAT		1U
#define WAV_MONO_CHANNELS	1U
#define WAV_BITS_PER_SAMPLE	16U
#define WAV_BLOCK_ALIGN		2U
#define WAV_HEADER_EXTRA	36U

static int wav_write_bytes(FILE *, const char *, size_t);
static int wav_write_le16(FILE *, uint16_t);
static int wav_write_le32(FILE *, uint32_t);
static uint16_t wav_sample_to_u16(int16_t);

/*
 * Write mono 16-bit PCM samples as a minimal RIFF/WAVE file.
 * Returns WAV_WRITER_OK on success.
 */
enum wav_writer_result
wav_writer_write_pcm16_mono(const char *path, const int16_t *pcm,
	size_t samples, uint32_t sample_rate)
{
	FILE *fp;
	uint32_t data_bytes;
	uint32_t riff_size;
	uint32_t byte_rate;
	size_t i;

	if (path == NULL || pcm == NULL || sample_rate == 0U)
		return WAV_WRITER_ERR_ARG;
	if (samples > (size_t)(UINT32_MAX / WAV_BLOCK_ALIGN))
		return WAV_WRITER_ERR_RANGE;
	data_bytes = (uint32_t)(samples * WAV_BLOCK_ALIGN);
	if (data_bytes > UINT32_MAX - WAV_HEADER_EXTRA)
		return WAV_WRITER_ERR_RANGE;
	if (sample_rate > UINT32_MAX / WAV_BLOCK_ALIGN)
		return WAV_WRITER_ERR_RANGE;
	riff_size = data_bytes + WAV_HEADER_EXTRA;
	byte_rate = sample_rate * WAV_BLOCK_ALIGN;

	fp = fopen(path, "wb");
	if (fp == NULL)
		return WAV_WRITER_ERR_IO;
	if (wav_write_bytes(fp, "RIFF", 4U) != 0 ||
	    wav_write_le32(fp, riff_size) != 0 ||
	    wav_write_bytes(fp, "WAVE", 4U) != 0 ||
	    wav_write_bytes(fp, "fmt ", 4U) != 0 ||
	    wav_write_le32(fp, 16U) != 0 ||
	    wav_write_le16(fp, WAV_PCM_FORMAT) != 0 ||
	    wav_write_le16(fp, WAV_MONO_CHANNELS) != 0 ||
	    wav_write_le32(fp, sample_rate) != 0 ||
	    wav_write_le32(fp, byte_rate) != 0 ||
	    wav_write_le16(fp, WAV_BLOCK_ALIGN) != 0 ||
	    wav_write_le16(fp, WAV_BITS_PER_SAMPLE) != 0 ||
	    wav_write_bytes(fp, "data", 4U) != 0 ||
	    wav_write_le32(fp, data_bytes) != 0) {
		(void)fclose(fp);
		return WAV_WRITER_ERR_IO;
	}
	for (i = 0U; i < samples; i++) {
		if (wav_write_le16(fp, wav_sample_to_u16(pcm[i])) != 0) {
			(void)fclose(fp);
			return WAV_WRITER_ERR_IO;
		}
	}
	if (fclose(fp) != 0)
		return WAV_WRITER_ERR_IO;

	return WAV_WRITER_OK;
}

static int
wav_write_bytes(FILE *fp, const char *buf, size_t len)
{
	if (fwrite(buf, 1U, len, fp) != len)
		return -1;
	return 0;
}

static int
wav_write_le16(FILE *fp, uint16_t value)
{
	if (fputc((int)(value & 0xFFU), fp) == EOF ||
	    fputc((int)((value >> 8U) & 0xFFU), fp) == EOF)
		return -1;
	return 0;
}

static int
wav_write_le32(FILE *fp, uint32_t value)
{
	if (fputc((int)(value & 0xFFU), fp) == EOF ||
	    fputc((int)((value >> 8U) & 0xFFU), fp) == EOF ||
	    fputc((int)((value >> 16U) & 0xFFU), fp) == EOF ||
	    fputc((int)((value >> 24U) & 0xFFU), fp) == EOF)
		return -1;
	return 0;
}

static uint16_t
wav_sample_to_u16(int16_t sample)
{
	int32_t value;

	value = sample;
	if (value < 0)
		return (uint16_t)(uint32_t)(value + 65536);
	return (uint16_t)value;
}
