/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/daemon/kilotncd_audio.h */

#ifndef KILOTNCD_AUDIO_H
#define KILOTNCD_AUDIO_H

#include <sys/types.h>

#include <stdint.h>

#define KILOTNCD_AUDIO_PATH_MAX	512U

enum kilotncd_audio_result {
	KILOTNCD_AUDIO_OK = 0,
	KILOTNCD_AUDIO_ERR_ARG,
	KILOTNCD_AUDIO_ERR_IO,
	KILOTNCD_AUDIO_ERR_SMALL,
	KILOTNCD_AUDIO_ERR_UNSUPPORTED,
	KILOTNCD_AUDIO_EOF
};

enum kilotncd_audio_backend {
	KILOTNCD_AUDIO_BACKEND_RAW_FILE = 0,
	KILOTNCD_AUDIO_BACKEND_ALSA,
	KILOTNCD_AUDIO_BACKEND_SNDIO,
	KILOTNCD_AUDIO_BACKEND_OSS
};

struct kilotncd_audio_format {
	uint32_t sample_rate;
	uint8_t channels;
	uint8_t bits_per_sample;
	uint8_t little_endian;
};

struct kilotncd_audio_config {
	enum kilotncd_audio_backend backend;
	char path[KILOTNCD_AUDIO_PATH_MAX];
	struct kilotncd_audio_format format;
};

struct kilotncd_audio {
	struct kilotncd_audio_config config;
	int output;
	int opened;
};

enum kilotncd_audio_result kilotncd_audio_backend_name(
	enum kilotncd_audio_backend, const char **);
enum kilotncd_audio_result kilotncd_audio_close(struct kilotncd_audio *);
void kilotncd_audio_default_format(struct kilotncd_audio_format *);
enum kilotncd_audio_result kilotncd_audio_open_input(struct kilotncd_audio *,
	const struct kilotncd_audio_config *);
enum kilotncd_audio_result kilotncd_audio_open_output(struct kilotncd_audio *,
	const struct kilotncd_audio_config *);
enum kilotncd_audio_result kilotncd_audio_parse_backend(const char *,
	enum kilotncd_audio_backend *);
enum kilotncd_audio_result kilotncd_audio_read(struct kilotncd_audio *,
	int16_t *, size_t, size_t *);
enum kilotncd_audio_result kilotncd_audio_validate_format(
	const struct kilotncd_audio_format *);
enum kilotncd_audio_result kilotncd_audio_write(struct kilotncd_audio *,
	const int16_t *, size_t);

#endif
