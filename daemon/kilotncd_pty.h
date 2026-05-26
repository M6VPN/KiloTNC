/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/daemon/kilotncd_pty.h */

#ifndef KILOTNCD_PTY_H
#define KILOTNCD_PTY_H

#include <sys/types.h>

#include <stdint.h>

enum kilotncd_pty_result {
	KILOTNCD_PTY_OK = 0,
	KILOTNCD_PTY_ERR_ARG,
	KILOTNCD_PTY_ERR_OPEN,
	KILOTNCD_PTY_ERR_IO,
	KILOTNCD_PTY_ERR_RANGE
};

enum kilotncd_pty_result kilotncd_pty_server_once(const char *,
	uint8_t *, size_t, size_t *, const uint8_t *, size_t);

#endif
