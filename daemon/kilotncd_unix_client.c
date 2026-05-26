/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/daemon/kilotncd_unix_client.c */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define CLIENT_BUF_MAX	8192U

static uint8_t client_in[CLIENT_BUF_MAX];
static uint8_t client_out[CLIENT_BUF_MAX];

static int client_connect(const char *);
static int client_read_file(const char *, uint8_t *, size_t, size_t *);
static int client_send_all(int, const uint8_t *, size_t);
static int client_write_file(const char *, const uint8_t *, size_t);
static void client_usage(void);

int
main(int argc, char *argv[])
{
	const char *path;
	const char *in_path;
	const char *out_path;
	size_t in_len;
	size_t out_len;
	int fd;
	int n;

	if (argc < 3) {
		client_usage();
		return 1;
	}
	path = argv[1];
	in_path = argv[2];
	out_path = argc > 3 ? argv[3] : NULL;

	if (client_read_file(in_path, client_in, sizeof(client_in),
	    &in_len) != 0)
		return 1;
	fd = client_connect(path);
	if (fd == -1)
		return 1;
	if (client_send_all(fd, client_in, in_len) != 0) {
		(void)close(fd);
		return 1;
	}
	(void)shutdown(fd, SHUT_WR);
	out_len = 0U;
	for (;;) {
		if (out_len >= sizeof(client_out)) {
			(void)close(fd);
			return 1;
		}
		n = (int)recv(fd, &client_out[out_len],
		    sizeof(client_out) - out_len, 0);
		if (n == 0)
			break;
		if (n < 0) {
			(void)close(fd);
			return 1;
		}
		out_len += (size_t)n;
	}
	if (close(fd) != 0)
		return 1;
	if (out_path != NULL && client_write_file(out_path, client_out,
	    out_len) != 0)
		return 1;

	return 0;
}

static int
client_connect(const char *path)
{
	struct sockaddr_un sun;
	int fd;

	if (strlen(path) >= sizeof(sun.sun_path))
		return -1;
	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd == -1)
		return -1;
	(void)memset(&sun, 0, sizeof(sun));
	sun.sun_family = AF_UNIX;
	(void)strncpy(sun.sun_path, path, sizeof(sun.sun_path) - 1U);
	if (connect(fd, (struct sockaddr *)&sun, sizeof(sun)) == -1) {
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
client_send_all(int fd, const uint8_t *buf, size_t len)
{
	size_t off;
	ssize_t n;

	off = 0U;
	while (off < len) {
		n = send(fd, &buf[off], len - off, 0);
		if (n <= 0)
			return -1;
		off += (size_t)n;
	}

	return 0;
}

static int
client_write_file(const char *path, const uint8_t *buf, size_t len)
{
	FILE *fp;

	fp = fopen(path, "wb");
	if (fp == NULL)
		return -1;
	if (fwrite(buf, 1U, len, fp) != len) {
		(void)fclose(fp);
		return -1;
	}
	if (fclose(fp) != 0)
		return -1;

	return 0;
}

static void
client_usage(void)
{
	(void)fprintf(stderr,
	    "usage: kilotncd_unix_client socket-path in.kiss [out.kiss]\n");
}
