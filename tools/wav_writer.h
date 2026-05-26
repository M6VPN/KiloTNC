/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/tools/wav_writer.h */

#ifndef WAV_WRITER_H
#define WAV_WRITER_H

#include <sys/types.h>

#include <stdint.h>

enum wav_writer_result {
	WAV_WRITER_OK = 0,
	WAV_WRITER_ERR_ARG,
	WAV_WRITER_ERR_RANGE,
	WAV_WRITER_ERR_IO
};

/*
 * Write mono 16-bit PCM samples as RIFF/WAVE.
 * path and pcm must be non-NULL. sample_rate must be non-zero.
 */
enum wav_writer_result wav_writer_write_pcm16_mono(const char *,
	const int16_t *, size_t, uint32_t);

#endif
