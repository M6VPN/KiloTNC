/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/daemon/kilotncd_pty_client.c */

#include <sys/types.h>

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define CLIENT_BUF_MAX	8192U
#define CLIENT_PATH_MAX	256U

static uint8_t client_in[CLIENT_BUF_MAX];
static char client_path[CLIENT_PATH_MAX];

static int client_open_pty(const char *);
static int client_read_file(const char *, uint8_t *, size_t, size_t *);
static int client_read_path(const char *, char *, size_t);
static int client_set_raw(int);
static int client_write_all(int, const uint8_t *, size_t);
static void client_usage(void);

int
main(int argc, char *argv[])
{
	const char *path_file;
	const char *in_path;
	size_t in_len;
	int fd;

	if (argc != 3) {
		client_usage();
		return 1;
	}
	path_file = argv[1];
	in_path = argv[2];

	if (client_read_path(path_file, client_path, sizeof(client_path)) != 0)
		return 1;
	if (client_read_file(in_path, client_in, sizeof(client_in),
	    &in_len) != 0)
		return 1;
	fd = client_open_pty(client_path);
	if (fd == -1)
		return 1;
	if (client_write_all(fd, client_in, in_len) != 0) {
		(void)close(fd);
		return 1;
	}
	(void)tcdrain(fd);
	if (close(fd) != 0)
		return 1;

	return 0;
}

static int
client_open_pty(const char *path)
{
	int fd;

	fd = open(path, O_RDWR | O_NOCTTY);
	if (fd == -1)
		return -1;
	if (client_set_raw(fd) != 0) {
		(void)close(fd);
		return -1;
	}

	return fd;
}

static int
client_read_file(const char *path, uint8_t *buf, size_t cap, size_t *len)
{
	FILE *fp;
	int extra;

	fp = fopen(path, "rb");
	if (fp == NULL)
		return -1;
	*len = fread(buf, 1U, cap, fp);
	if (ferror(fp)) {
		(void)fclose(fp);
		return -1;
	}
	extra = fgetc(fp);
	if (fclose(fp) != 0)
		return -1;
	if (extra != EOF)
		return -1;

	return 0;
}

static int
client_read_path(const char *path, char *buf, size_t cap)
{
	FILE *fp;
	size_t len;

	if (cap == 0U)
		return -1;
	fp = fopen(path, "r");
	if (fp == NULL)
		return -1;
	if (fgets(buf, (int)cap, fp) == NULL) {
		(void)fclose(fp);
		return -1;
	}
	if (fclose(fp) != 0)
		return -1;
	len = strlen(buf);
	while (len > 0U && (buf[len - 1U] == '\n' ||
	    buf[len - 1U] == '\r')) {
		len--;
		buf[len] = '\0';
	}
	if (len == 0U || len >= cap - 1U)
		return -1;

	return 0;
}

static int
client_set_raw(int fd)
{
	struct termios tio;

	if (tcgetattr(fd, &tio) != 0)
		return -1;
	tio.c_iflag &= (tcflag_t)~(IGNBRK | BRKINT | PARMRK | ISTRIP |
	    INLCR | IGNCR | ICRNL | IXON);
	tio.c_oflag &= (tcflag_t)~OPOST;
	tio.c_lflag &= (tcflag_t)~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	tio.c_cflag &= (tcflag_t)~(CSIZE | PARENB);
	tio.c_cflag |= CS8;
	tio.c_cc[VMIN] = 1U;
	tio.c_cc[VTIME] = 0U;
	if (tcsetattr(fd, TCSANOW, &tio) != 0)
		return -1;

	return 0;
}

static int
client_write_all(int fd, const uint8_t *buf, size_t len)
{
	size_t off;
	ssize_t n;

	off = 0U;
	while (off < len) {
		n = write(fd, &buf[off], len - off);
		if (n <= 0)
			return -1;
		off += (size_t)n;
	}

	return 0;
}

static void
client_usage(void)
{
	(void)fprintf(stderr,
	    "usage: kilotncd_pty_client pty-path-file in.kiss\n");
}
