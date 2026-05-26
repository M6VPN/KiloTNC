/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/daemon/kilotncd_tcp.h */

#ifndef KILOTNCD_TCP_H
#define KILOTNCD_TCP_H

#include <sys/types.h>

#include <stdint.h>

#define KILOTNCD_TCP_HOST_MAX	64U

enum kilotncd_tcp_result {
	KILOTNCD_TCP_OK = 0,
	KILOTNCD_TCP_ERR_ARG,
	KILOTNCD_TCP_ERR_PARSE,
	KILOTNCD_TCP_ERR_BIND,
	KILOTNCD_TCP_ERR_IO,
	KILOTNCD_TCP_ERR_RANGE
};

struct kilotncd_tcp_addr {
	char host[KILOTNCD_TCP_HOST_MAX];
	uint16_t port;
};

typedef size_t (*kilotncd_tcp_handler)(const uint8_t *, size_t, uint8_t *,
	size_t, void *);

enum kilotncd_tcp_result kilotncd_tcp_parse_listen(const char *,
	struct kilotncd_tcp_addr *);
enum kilotncd_tcp_result kilotncd_tcp_reject_nonlocal(
	const struct kilotncd_tcp_addr *, int);
enum kilotncd_tcp_result kilotncd_tcp_server_once(
	const struct kilotncd_tcp_addr *, uint8_t *, size_t, size_t *,
	const uint8_t *, size_t);
enum kilotncd_tcp_result kilotncd_tcp_server_transaction(
	const struct kilotncd_tcp_addr *, uint8_t *, size_t, size_t *,
	uint8_t *, size_t, kilotncd_tcp_handler, void *);

#endif
