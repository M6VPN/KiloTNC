/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/daemon/kilotncd_pty.c */

#define _XOPEN_SOURCE 600

#include <sys/types.h>

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "kilotncd_pty.h"

#define KILOTNCD_PTY_FIRST_TIMEOUT_MS	10000
#define KILOTNCD_PTY_IDLE_TIMEOUT_MS	200
#define KILOTNCD_PTY_PATH_MAX		256U

static enum kilotncd_pty_result kilotncd_pty_read_master(int, uint8_t *,
	size_t, size_t *);
static enum kilotncd_pty_result kilotncd_pty_set_raw(int);
static enum kilotncd_pty_result kilotncd_pty_write_all(int,
	const uint8_t *, size_t);
static enum kilotncd_pty_result kilotncd_pty_write_path(const char *,
	const char *);

enum kilotncd_pty_result
kilotncd_pty_server_once(const char *path_out, uint8_t *rx, size_t rx_cap,
	size_t *rx_len, const uint8_t *tx, size_t tx_len)
{
	char slave_path[KILOTNCD_PTY_PATH_MAX];
	const char *name;
	size_t name_len;
	int master_fd;
	int slave_fd;
	enum kilotncd_pty_result res;

	if (rx == NULL || rx_len == NULL || (tx == NULL && tx_len != 0U))
		return KILOTNCD_PTY_ERR_ARG;
	*rx_len = 0U;

	master_fd = posix_openpt(O_RDWR | O_NOCTTY);
	if (master_fd == -1)
		return KILOTNCD_PTY_ERR_OPEN;
	if (grantpt(master_fd) == -1 || unlockpt(master_fd) == -1) {
		(void)close(master_fd);
		return KILOTNCD_PTY_ERR_IO;
	}
	name = ptsname(master_fd);
	if (name == NULL) {
		(void)close(master_fd);
		return KILOTNCD_PTY_ERR_IO;
	}
	name_len = strlen(name);
	if (name_len == 0U || name_len >= sizeof(slave_path)) {
		(void)close(master_fd);
		return KILOTNCD_PTY_ERR_RANGE;
	}
	(void)memcpy(slave_path, name, name_len);
	slave_path[name_len] = '\0';

	slave_fd = open(slave_path, O_RDWR | O_NOCTTY);
	if (slave_fd == -1) {
		(void)close(master_fd);
		return KILOTNCD_PTY_ERR_OPEN;
	}
	if (kilotncd_pty_set_raw(slave_fd) != KILOTNCD_PTY_OK ||
	    kilotncd_pty_write_path(path_out, slave_path) !=
	    KILOTNCD_PTY_OK) {
		(void)close(slave_fd);
		(void)close(master_fd);
		return KILOTNCD_PTY_ERR_IO;
	}
	(void)fprintf(stderr, "pty=%s\n", slave_path);

	res = kilotncd_pty_read_master(master_fd, rx, rx_cap, rx_len);
	if (res == KILOTNCD_PTY_OK && tx_len != 0U)
		res = kilotncd_pty_write_all(master_fd, tx, tx_len);
	if (close(slave_fd) != 0 && res == KILOTNCD_PTY_OK)
		res = KILOTNCD_PTY_ERR_IO;
	if (close(master_fd) != 0 && res == KILOTNCD_PTY_OK)
		res = KILOTNCD_PTY_ERR_IO;

	return res;
}

static enum kilotncd_pty_result
kilotncd_pty_read_master(int fd, uint8_t *rx, size_t rx_cap, size_t *rx_len)
{
	struct pollfd pfd;
	ssize_t n;
	int timeout;
	int pres;

	for (;;) {
		timeout = *rx_len == 0U ? KILOTNCD_PTY_FIRST_TIMEOUT_MS :
		    KILOTNCD_PTY_IDLE_TIMEOUT_MS;
		pfd.fd = fd;
		pfd.events = (short)POLLIN;
		pfd.revents = 0;
		pres = poll(&pfd, (nfds_t)1, timeout);
		if (pres == 0)
			return *rx_len == 0U ? KILOTNCD_PTY_ERR_IO :
			    KILOTNCD_PTY_OK;
		if (pres < 0) {
			if (errno == EINTR)
				continue;
			return KILOTNCD_PTY_ERR_IO;
		}
		if ((pfd.revents & POLLIN) == 0)
			return *rx_len == 0U ? KILOTNCD_PTY_ERR_IO :
			    KILOTNCD_PTY_OK;
		if (*rx_len >= rx_cap)
			return KILOTNCD_PTY_ERR_RANGE;
		n = read(fd, &rx[*rx_len], rx_cap - *rx_len);
		if (n == 0)
			return KILOTNCD_PTY_OK;
		if (n < 0) {
			if (errno == EINTR)
				continue;
			if (errno == EIO && *rx_len != 0U)
				return KILOTNCD_PTY_OK;
			return KILOTNCD_PTY_ERR_IO;
		}
		*rx_len += (size_t)n;
	}
}

static enum kilotncd_pty_result
kilotncd_pty_set_raw(int fd)
{
	struct termios tio;

	if (tcgetattr(fd, &tio) != 0)
		return KILOTNCD_PTY_ERR_IO;
	tio.c_iflag &= (tcflag_t)~(IGNBRK | BRKINT | PARMRK | ISTRIP |
	    INLCR | IGNCR | ICRNL | IXON);
	tio.c_oflag &= (tcflag_t)~OPOST;
	tio.c_lflag &= (tcflag_t)~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	tio.c_cflag &= (tcflag_t)~(CSIZE | PARENB);
	tio.c_cflag |= CS8;
	tio.c_cc[VMIN] = 1U;
	tio.c_cc[VTIME] = 0U;
	if (tcsetattr(fd, TCSANOW, &tio) != 0)
		return KILOTNCD_PTY_ERR_IO;

	return KILOTNCD_PTY_OK;
}

static enum kilotncd_pty_result
kilotncd_pty_write_all(int fd, const uint8_t *tx, size_t tx_len)
{
	size_t off;
	ssize_t n;

	off = 0U;
	while (off < tx_len) {
		n = write(fd, &tx[off], tx_len - off);
		if (n < 0) {
			if (errno == EINTR)
				continue;
			return KILOTNCD_PTY_ERR_IO;
		}
		if (n == 0)
			return KILOTNCD_PTY_ERR_IO;
		off += (size_t)n;
	}

	return KILOTNCD_PTY_OK;
}

static enum kilotncd_pty_result
kilotncd_pty_write_path(const char *path_out, const char *slave_path)
{
	FILE *fp;

	if (path_out == NULL)
		return KILOTNCD_PTY_OK;
	fp = fopen(path_out, "w");
	if (fp == NULL)
		return KILOTNCD_PTY_ERR_IO;
	if (fprintf(fp, "%s\n", slave_path) < 0) {
		(void)fclose(fp);
		return KILOTNCD_PTY_ERR_IO;
	}
	if (fclose(fp) != 0)
		return KILOTNCD_PTY_ERR_IO;

	return KILOTNCD_PTY_OK;
}
