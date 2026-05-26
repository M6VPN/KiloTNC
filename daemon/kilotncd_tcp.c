/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/daemon/kilotncd_tcp.c */

#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "kilotncd_tcp.h"

#define KILOTNCD_TCP_BACKLOG	1

static enum kilotncd_tcp_result kilotncd_tcp_copy_host(char *, size_t,
	const char *, size_t);
static int kilotncd_tcp_is_localhost(const char *);
static enum kilotncd_tcp_result kilotncd_tcp_read_client(int, uint8_t *,
	size_t, size_t *);
static enum kilotncd_tcp_result kilotncd_tcp_write_all(int, const uint8_t *,
	size_t);

enum kilotncd_tcp_result
kilotncd_tcp_parse_listen(const char *text, struct kilotncd_tcp_addr *addr)
{
	const char *colon;
	const char *host;
	const char *port_text;
	char *end;
	unsigned long port;
	size_t host_len;

	if (text == NULL || addr == NULL)
		return KILOTNCD_TCP_ERR_ARG;
	(void)memset(addr, 0, sizeof(*addr));
	colon = strrchr(text, ':');
	if (colon == NULL) {
		host = "127.0.0.1";
		host_len = strlen(host);
		port_text = text;
	} else {
		host = text;
		host_len = (size_t)(colon - text);
		port_text = colon + 1;
	}
	if (host_len == 0U || port_text[0] == '\0')
		return KILOTNCD_TCP_ERR_PARSE;
	if (kilotncd_tcp_copy_host(addr->host, sizeof(addr->host), host,
	    host_len) != KILOTNCD_TCP_OK)
		return KILOTNCD_TCP_ERR_RANGE;
	errno = 0;
	port = strtoul(port_text, &end, 10);
	if (errno != 0 || end == port_text || *end != '\0' || port == 0UL ||
	    port > UINT16_MAX)
		return KILOTNCD_TCP_ERR_PARSE;
	addr->port = (uint16_t)port;

	return KILOTNCD_TCP_OK;
}

enum kilotncd_tcp_result
kilotncd_tcp_reject_nonlocal(const struct kilotncd_tcp_addr *addr,
	int allow_nonlocal)
{
	if (addr == NULL)
		return KILOTNCD_TCP_ERR_ARG;
	if (kilotncd_tcp_is_localhost(addr->host))
		return KILOTNCD_TCP_OK;
	if (allow_nonlocal)
		return KILOTNCD_TCP_OK;
	return KILOTNCD_TCP_ERR_BIND;
}

enum kilotncd_tcp_result
kilotncd_tcp_server_once(const struct kilotncd_tcp_addr *addr,
	uint8_t *rx, size_t rx_cap, size_t *rx_len, const uint8_t *tx,
	size_t tx_len)
{
	struct sockaddr_in sin;
	int server_fd;
	int client_fd;
	int opt;
	enum kilotncd_tcp_result res;

	if (addr == NULL || rx == NULL || rx_len == NULL ||
	    (tx == NULL && tx_len != 0U))
		return KILOTNCD_TCP_ERR_ARG;
	*rx_len = 0U;
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1)
		return KILOTNCD_TCP_ERR_IO;
	opt = 1;
	(void)setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt,
	    sizeof(opt));
	(void)memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(addr->port);
	if (inet_pton(AF_INET, addr->host, &sin.sin_addr) != 1) {
		(void)close(server_fd);
		return KILOTNCD_TCP_ERR_PARSE;
	}
	if (bind(server_fd, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
		(void)close(server_fd);
		return KILOTNCD_TCP_ERR_BIND;
	}
	if (listen(server_fd, KILOTNCD_TCP_BACKLOG) == -1) {
		(void)close(server_fd);
		return KILOTNCD_TCP_ERR_IO;
	}
	client_fd = accept(server_fd, NULL, NULL);
	(void)close(server_fd);
	if (client_fd == -1)
		return KILOTNCD_TCP_ERR_IO;

	res = kilotncd_tcp_read_client(client_fd, rx, rx_cap, rx_len);
	if (res == KILOTNCD_TCP_OK && tx_len != 0U)
		res = kilotncd_tcp_write_all(client_fd, tx, tx_len);
	if (close(client_fd) != 0 && res == KILOTNCD_TCP_OK)
		res = KILOTNCD_TCP_ERR_IO;

	return res;
}

enum kilotncd_tcp_result
kilotncd_tcp_server_transaction(const struct kilotncd_tcp_addr *addr,
	uint8_t *rx, size_t rx_cap, size_t *rx_len, uint8_t *tx,
	size_t tx_cap, kilotncd_tcp_handler handler, void *arg)
{
	struct sockaddr_in sin;
	int server_fd;
	int client_fd;
	int opt;
	size_t tx_len;
	enum kilotncd_tcp_result res;

	if (handler == NULL || tx == NULL)
		return KILOTNCD_TCP_ERR_ARG;
	if (addr == NULL || rx == NULL || rx_len == NULL)
		return KILOTNCD_TCP_ERR_ARG;
	*rx_len = 0U;
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1)
		return KILOTNCD_TCP_ERR_IO;
	opt = 1;
	(void)setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt,
	    sizeof(opt));
	(void)memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(addr->port);
	if (inet_pton(AF_INET, addr->host, &sin.sin_addr) != 1) {
		(void)close(server_fd);
		return KILOTNCD_TCP_ERR_PARSE;
	}
	if (bind(server_fd, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
		(void)close(server_fd);
		return KILOTNCD_TCP_ERR_BIND;
	}
	if (listen(server_fd, KILOTNCD_TCP_BACKLOG) == -1) {
		(void)close(server_fd);
		return KILOTNCD_TCP_ERR_IO;
	}
	client_fd = accept(server_fd, NULL, NULL);
	(void)close(server_fd);
	if (client_fd == -1)
		return KILOTNCD_TCP_ERR_IO;

	res = kilotncd_tcp_read_client(client_fd, rx, rx_cap, rx_len);
	if (res == KILOTNCD_TCP_OK) {
		tx_len = handler(rx, *rx_len, tx, tx_cap, arg);
		if (tx_len == (size_t)-1)
			res = KILOTNCD_TCP_ERR_IO;
		else if (tx_len > tx_cap)
			res = KILOTNCD_TCP_ERR_RANGE;
		else
			res = kilotncd_tcp_write_all(client_fd, tx, tx_len);
	}
	if (close(client_fd) != 0 && res == KILOTNCD_TCP_OK)
		res = KILOTNCD_TCP_ERR_IO;

	return res;
}

static enum kilotncd_tcp_result
kilotncd_tcp_copy_host(char *dst, size_t dst_cap, const char *src,
	size_t src_len)
{
	if (dst == NULL || src == NULL || dst_cap == 0U)
		return KILOTNCD_TCP_ERR_ARG;
	if (src_len == 0U || src_len >= dst_cap)
		return KILOTNCD_TCP_ERR_RANGE;
	(void)memcpy(dst, src, src_len);
	dst[src_len] = '\0';

	return KILOTNCD_TCP_OK;
}

static int
kilotncd_tcp_is_localhost(const char *host)
{
	return strcmp(host, "127.0.0.1") == 0 || strcmp(host, "localhost") == 0;
}

static enum kilotncd_tcp_result
kilotncd_tcp_read_client(int fd, uint8_t *rx, size_t rx_cap, size_t *rx_len)
{
	ssize_t n;

	for (;;) {
		if (*rx_len >= rx_cap)
			return KILOTNCD_TCP_ERR_RANGE;
		n = recv(fd, &rx[*rx_len], rx_cap - *rx_len, 0);
		if (n == 0)
			return KILOTNCD_TCP_OK;
		if (n < 0)
			return KILOTNCD_TCP_ERR_IO;
		*rx_len += (size_t)n;
	}
}

static enum kilotncd_tcp_result
kilotncd_tcp_write_all(int fd, const uint8_t *tx, size_t tx_len)
{
	size_t off;
	ssize_t n;

	off = 0U;
	while (off < tx_len) {
		n = send(fd, &tx[off], tx_len - off, 0);
		if (n <= 0)
			return KILOTNCD_TCP_ERR_IO;
		off += (size_t)n;
	}

	return KILOTNCD_TCP_OK;
}
