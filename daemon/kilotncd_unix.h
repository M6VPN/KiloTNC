/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/daemon/kilotncd_unix.h */

#ifndef KILOTNCD_UNIX_H
#define KILOTNCD_UNIX_H

#include <sys/types.h>

#include <stdint.h>

enum kilotncd_unix_result {
	KILOTNCD_UNIX_OK = 0,
	KILOTNCD_UNIX_ERR_ARG,
	KILOTNCD_UNIX_ERR_PATH,
	KILOTNCD_UNIX_ERR_BIND,
	KILOTNCD_UNIX_ERR_IO,
	KILOTNCD_UNIX_ERR_RANGE
};

enum kilotncd_unix_result kilotncd_unix_server_once(const char *,
	int, uint8_t *, size_t, size_t *, const uint8_t *, size_t);

#endif
