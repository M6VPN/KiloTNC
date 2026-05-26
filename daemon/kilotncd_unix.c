/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/daemon/kilotncd_unix.c */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "kilotncd_unix.h"

#define KILOTNCD_UNIX_BACKLOG	1

static int kilotncd_unix_path_safe_build(const char *);
static enum kilotncd_unix_result kilotncd_unix_prepare_path(const char *,
	int);
static enum kilotncd_unix_result kilotncd_unix_read_client(int, uint8_t *,
	size_t, size_t *);
static enum kilotncd_unix_result kilotncd_unix_write_all(int,
	const uint8_t *, size_t);

enum kilotncd_unix_result
kilotncd_unix_server_once(const char *path, int unlink_stale,
	uint8_t *rx, size_t rx_cap, size_t *rx_len, const uint8_t *tx,
	size_t tx_len)
{
	struct sockaddr_un sun;
	int server_fd;
	int client_fd;
	enum kilotncd_unix_result res;

	if (path == NULL || rx == NULL || rx_len == NULL ||
	    (tx == NULL && tx_len != 0U))
		return KILOTNCD_UNIX_ERR_ARG;
	*rx_len = 0U;
	if (strlen(path) >= sizeof(sun.sun_path))
		return KILOTNCD_UNIX_ERR_PATH;
	res = kilotncd_unix_prepare_path(path, unlink_stale);
	if (res != KILOTNCD_UNIX_OK)
		return res;

	server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (server_fd == -1)
		return KILOTNCD_UNIX_ERR_IO;
	(void)memset(&sun, 0, sizeof(sun));
	sun.sun_family = AF_UNIX;
	(void)strncpy(sun.sun_path, path, sizeof(sun.sun_path) - 1U);
	if (bind(server_fd, (struct sockaddr *)&sun, sizeof(sun)) == -1) {
		(void)close(server_fd);
		return KILOTNCD_UNIX_ERR_BIND;
	}
	if (listen(server_fd, KILOTNCD_UNIX_BACKLOG) == -1) {
		(void)close(server_fd);
		(void)unlink(path);
		return KILOTNCD_UNIX_ERR_IO;
	}
	client_fd = accept(server_fd, NULL, NULL);
	(void)close(server_fd);
	if (client_fd == -1) {
		(void)unlink(path);
		return KILOTNCD_UNIX_ERR_IO;
	}

	res = kilotncd_unix_read_client(client_fd, rx, rx_cap, rx_len);
	if (res == KILOTNCD_UNIX_OK && tx_len != 0U)
		res = kilotncd_unix_write_all(client_fd, tx, tx_len);
	if (close(client_fd) != 0 && res == KILOTNCD_UNIX_OK)
		res = KILOTNCD_UNIX_ERR_IO;
	if (unlink(path) != 0 && res == KILOTNCD_UNIX_OK)
		res = KILOTNCD_UNIX_ERR_IO;

	return res;
}

static int
kilotncd_unix_path_safe_build(const char *path)
{
	return strncmp(path, "build/", 6U) == 0;
}

static enum kilotncd_unix_result
kilotncd_unix_prepare_path(const char *path, int unlink_stale)
{
	if (access(path, F_OK) == -1) {
		if (errno == ENOENT)
			return KILOTNCD_UNIX_OK;
		return KILOTNCD_UNIX_ERR_IO;
	}
	if (!unlink_stale && !kilotncd_unix_path_safe_build(path))
		return KILOTNCD_UNIX_ERR_PATH;
	if (unlink(path) != 0)
		return KILOTNCD_UNIX_ERR_IO;

	return KILOTNCD_UNIX_OK;
}

static enum kilotncd_unix_result
kilotncd_unix_read_client(int fd, uint8_t *rx, size_t rx_cap,
	size_t *rx_len)
{
	ssize_t n;

	for (;;) {
		if (*rx_len >= rx_cap)
			return KILOTNCD_UNIX_ERR_RANGE;
		n = recv(fd, &rx[*rx_len], rx_cap - *rx_len, 0);
		if (n == 0)
			return KILOTNCD_UNIX_OK;
		if (n < 0)
			return KILOTNCD_UNIX_ERR_IO;
		*rx_len += (size_t)n;
	}
}

static enum kilotncd_unix_result
kilotncd_unix_write_all(int fd, const uint8_t *tx, size_t tx_len)
{
	size_t off;
	ssize_t n;

	off = 0U;
	while (off < tx_len) {
		n = send(fd, &tx[off], tx_len - off, 0);
		if (n <= 0)
			return KILOTNCD_UNIX_ERR_IO;
		off += (size_t)n;
	}

	return KILOTNCD_UNIX_OK;
}
