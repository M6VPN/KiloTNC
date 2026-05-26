/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/daemon/kilotncd_file.h */

#ifndef KILOTNCD_FILE_H
#define KILOTNCD_FILE_H

#include <sys/types.h>

#include <stdint.h>

enum kilotncd_file_result {
	KILOTNCD_FILE_OK = 0,
	KILOTNCD_FILE_ERR_ARG,
	KILOTNCD_FILE_ERR_IO,
	KILOTNCD_FILE_ERR_RANGE,
	KILOTNCD_FILE_ERR_FORMAT
};

enum kilotncd_file_result kilotncd_file_read_bytes(const char *,
	uint8_t *, size_t, size_t *);
enum kilotncd_file_result kilotncd_file_read_pcm16(const char *,
	int16_t *, size_t, size_t *);
enum kilotncd_file_result kilotncd_file_write_bytes(const char *,
	const uint8_t *, size_t);
enum kilotncd_file_result kilotncd_file_write_pcm16(const char *,
	const int16_t *, size_t);

#endif
