/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/test/test_main.c */

#include <sys/types.h>

#include <stdio.h>

int test_ax25(void);
int test_fcs(void);
int test_hdlc(void);
int test_kiss(void);

static int test_run(const char *, int (*)(void));

int
main(void)
{
	int failures;

	failures = 0;
	failures += test_run("fcs", test_fcs);
	failures += test_run("hdlc", test_hdlc);
	failures += test_run("ax25", test_ax25);
	failures += test_run("kiss", test_kiss);

	if (failures != 0) {
		(void)printf("failed: %d\n", failures);
		return 1;
	}

	(void)printf("all tests passed\n");
	return 0;
}

static int
test_run(const char *name, int (*fn)(void))
{
	int line;

	line = fn();
	if (line != 0) {
		(void)printf("not ok %s line %d\n", name, line);
		return 1;
	}

	(void)printf("ok %s\n", name);
	return 0;
}
