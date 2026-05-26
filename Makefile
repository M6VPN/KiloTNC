# KiloTNC - Developed by M6VPN (M6VPN@tuta.com)
# KiloTNC/Makefile

CC	?= cc
CFLAGS	+= -std=c99 -Wall -Wextra -Wconversion -Wsign-conversion -Werror
CFLAGS	+= -I firmware/include
SANFLAGS ?= -fsanitize=address,undefined -fno-omit-frame-pointer

BUILD	= build
TESTBIN	= ${BUILD}/kilotnc_tests
SANBIN	= ${BUILD}/kilotnc_tests_sanitize
TOOLBIN	= ${BUILD}/kilotnc_cli
DAEMONBIN = ${BUILD}/kilotncd
TCPCLIENTBIN = ${BUILD}/kilotncd_tcp_client
UNIXCLIENTBIN = ${BUILD}/kilotncd_unix_client
PTYCLIENTBIN = ${BUILD}/kilotncd_pty_client
TOOL_SRCS = tools/kilotnc_cli.c \
	  tools/wav_writer.c
DAEMON_SRCS = daemon/kilotncd.c \
	  daemon/kilotncd_config.c \
	  daemon/kilotncd_file.c \
	  daemon/kilotncd_pty.c \
	  daemon/kilotncd_tcp.c \
	  daemon/kilotncd_unix.c

CORE_SRCS = firmware/src/afsk1200.c \
	  firmware/src/afsk1200_rx.c \
	  firmware/src/afsk1200_stream.c \
	  firmware/src/afsk1200_tx.c \
	  firmware/src/ax25.c \
	  firmware/src/fcs.c \
	  firmware/src/hdlc.c \
	  firmware/src/kiss.c \
	  firmware/src/tnc_control.c \
	  firmware/src/tnc_diag.c \
	  firmware/src/tnc_mode.c \
	  firmware/src/tnc1200.c

TEST_SRCS = firmware/test/test_afsk1200.c \
	  firmware/test/test_afsk1200_rx.c \
	  firmware/test/test_afsk1200_stream.c \
	  firmware/test/test_afsk1200_tx.c \
	  firmware/test/test_ax25.c \
	  firmware/test/test_fcs.c \
	  firmware/test/test_fuzz.c \
	  firmware/test/test_hdlc.c \
	  firmware/test/test_kiss.c \
	  firmware/test/test_tnc_control.c \
	  firmware/test/test_tnc_diag.c \
	  firmware/test/test_tnc_mode.c \
	  firmware/test/test_tnc1200.c \
	  firmware/test/test_main.c

SRCS	= ${CORE_SRCS} ${TEST_SRCS}

all: ${TESTBIN}

${TESTBIN}: ${SRCS}
	mkdir -p ${BUILD}
	${CC} ${CFLAGS} -o $@ ${SRCS}

test: ${TESTBIN}
	./${TESTBIN}

sanitize: ${SANBIN}
	./${SANBIN}

tools: ${TOOLBIN}

tool-test: ${TOOLBIN}
	mkdir -p ${BUILD}/vectors
	./${TOOLBIN} inspect-mode --mode NINO_MODE=6
	./${TOOLBIN} inspect-mode --mode NINO_MODE=22
	./${TOOLBIN} generate-kiss --out ${BUILD}/vectors/kilotnc.kiss --dst APZKTN --src M6VPN --info "KiloTNC test"
	./${TOOLBIN} generate-pcm --out ${BUILD}/vectors/kilotnc.pcm --dst APZKTN --src M6VPN --info "KiloTNC test" --mode NINO_MODE=6
	./${TOOLBIN} generate-wav --out ${BUILD}/vectors/kilotnc.wav --dst APZKTN --src M6VPN --info "KiloTNC test" --mode NINO_MODE=22
	./${TOOLBIN} vector-loopback --prefix ${BUILD}/vectors/kilotnc --mode NINO_MODE=6
	test -s ${BUILD}/vectors/kilotnc.kiss
	test -s ${BUILD}/vectors/kilotnc.pcm
	test -s ${BUILD}/vectors/kilotnc.wav
	test -s ${BUILD}/vectors/kilotnc.out.kiss
	test -s ${BUILD}/vectors/kilotnc.diag.txt
	test "$$(od -An -tx1 -N4 ${BUILD}/vectors/kilotnc.wav | tr -d ' \n')" = "52494646"
	test "$$(od -An -tx1 -j8 -N4 ${BUILD}/vectors/kilotnc.wav | tr -d ' \n')" = "57415645"
	test "$$(od -An -tx1 -j20 -N2 ${BUILD}/vectors/kilotnc.wav | tr -d ' \n')" = "0100"
	test "$$(od -An -tx1 -j22 -N2 ${BUILD}/vectors/kilotnc.wav | tr -d ' \n')" = "0100"
	test "$$(od -An -tx1 -j24 -N4 ${BUILD}/vectors/kilotnc.wav | tr -d ' \n')" = "80bb0000"
	test "$$(od -An -tx1 -j34 -N2 ${BUILD}/vectors/kilotnc.wav | tr -d ' \n')" = "1000"
	if ./${TOOLBIN} generate-pcm --out ${BUILD}/vectors/bad.pcm --mode NINO_MODE=0; then exit 1; else exit 0; fi

daemon: ${DAEMONBIN} ${TCPCLIENTBIN} ${UNIXCLIENTBIN} ${PTYCLIENTBIN}

daemon-test: ${DAEMONBIN} ${TCPCLIENTBIN} ${UNIXCLIENTBIN} ${PTYCLIENTBIN} ${TOOLBIN}
	mkdir -p ${BUILD}/daemon
	./${TOOLBIN} generate-kiss --out ${BUILD}/daemon/input.kiss --dst APZKTN --src M6VPN --info "KiloTNC daemon"
	./${TOOLBIN} generate-pcm --out ${BUILD}/daemon/input.pcm --dst APZKTN --src M6VPN --info "KiloTNC daemon" --mode NINO_MODE=6
	./${DAEMONBIN} --status
	./${DAEMONBIN} --status --mode NINO_MODE=6
	./${DAEMONBIN} --status --mode NINO_MODE=22
	./${DAEMONBIN} --mode NINO_MODE=6 --kiss-in ${BUILD}/daemon/input.kiss --pcm-out ${BUILD}/daemon/tx.pcm --once
	./${DAEMONBIN} --mode NINO_MODE=6 --pcm-in ${BUILD}/daemon/input.pcm --kiss-out ${BUILD}/daemon/rx.kiss --once
	./${DAEMONBIN} --mode NINO_MODE=6 --kiss-in ${BUILD}/daemon/input.kiss --kiss-out ${BUILD}/daemon/loop.kiss --loopback-once
	./${DAEMONBIN} --config daemon/example.conf --once
	./${DAEMONBIN} --mode NINO_MODE=6 --kiss-in - --pcm-out ${BUILD}/daemon/stdin-tx.pcm --once < ${BUILD}/daemon/input.kiss
	./${DAEMONBIN} --mode NINO_MODE=6 --kiss-in ${BUILD}/daemon/input.kiss --pcm-out - --once > ${BUILD}/daemon/stdout-tx.pcm 2> ${BUILD}/daemon/stdout-tx.diag
	./${DAEMONBIN} --mode NINO_MODE=6 --pcm-in ${BUILD}/daemon/input.pcm --kiss-out - --once > ${BUILD}/daemon/stdout-rx.kiss 2> ${BUILD}/daemon/stdout-rx.diag
	if ./${DAEMONBIN} --kiss-in - --pcm-in - --pcm-out ${BUILD}/daemon/ambiguous.pcm --once < ${BUILD}/daemon/input.kiss; then exit 1; else exit 0; fi
	./${DAEMONBIN} --kiss-tcp-listen 127.0.0.1:18015 --kiss-tcp-once --pcm-out ${BUILD}/daemon/tcp-tx.pcm --once > ${BUILD}/daemon/tcp.log 2>&1 & pid=$$!; sleep 1; ./${TCPCLIENTBIN} 127.0.0.1 18015 ${BUILD}/daemon/input.kiss; wait $$pid
	./${DAEMONBIN} --kiss-unix-listen ${BUILD}/daemon/kilotnc.sock --kiss-unix-once --pcm-out ${BUILD}/daemon/unix-tx.pcm --once > ${BUILD}/daemon/unix.log 2>&1 & pid=$$!; sleep 1; ./${UNIXCLIENTBIN} ${BUILD}/daemon/kilotnc.sock ${BUILD}/daemon/input.kiss; wait $$pid
	test ! -e ${BUILD}/daemon/kilotnc.sock
	./${DAEMONBIN} --kiss-pty --kiss-pty-once --pty-path-out ${BUILD}/daemon/kilotnc.pty --pcm-out ${BUILD}/daemon/pty-tx.pcm --once > ${BUILD}/daemon/pty.log 2> ${BUILD}/daemon/pty.err & pid=$$!; sleep 1; test -s ${BUILD}/daemon/kilotnc.pty; ./${PTYCLIENTBIN} ${BUILD}/daemon/kilotnc.pty ${BUILD}/daemon/input.kiss; wait $$pid
	printf 'unknown=1\n' > ${BUILD}/daemon/invalid.conf
	if ./${DAEMONBIN} --config ${BUILD}/daemon/invalid.conf --status; then exit 1; else exit 0; fi
	if ./${DAEMONBIN} --mode NINO_MODE=0 --kiss-in ${BUILD}/daemon/input.kiss --pcm-out ${BUILD}/daemon/bad.pcm --once; then exit 1; else exit 0; fi
	if ./${DAEMONBIN} --kiss-tcp-listen 0.0.0.0:18015 --kiss-tcp-once --pcm-out ${BUILD}/daemon/nonlocal.pcm --once; then exit 1; else exit 0; fi
	printf 'mode=NINO_MODE=6\nkiss_tcp_listen=127.0.0.1:18016\nkiss_tcp_once=1\nallow_nonlocal_bind=0\npcm_out=${BUILD}/daemon/config-tcp.pcm\n' > ${BUILD}/daemon/tcp.conf
	./${DAEMONBIN} --config ${BUILD}/daemon/tcp.conf --once > ${BUILD}/daemon/config-tcp.log 2>&1 & pid=$$!; sleep 1; ./${TCPCLIENTBIN} 127.0.0.1 18016 ${BUILD}/daemon/input.kiss; wait $$pid
	printf 'mode=NINO_MODE=6\nkiss_unix_listen=${BUILD}/daemon/config.sock\nkiss_unix_once=1\nunlink_stale_socket=0\npcm_out=${BUILD}/daemon/config-unix.pcm\n' > ${BUILD}/daemon/unix.conf
	./${DAEMONBIN} --config ${BUILD}/daemon/unix.conf --once > ${BUILD}/daemon/config-unix.log 2>&1 & pid=$$!; sleep 1; ./${UNIXCLIENTBIN} ${BUILD}/daemon/config.sock ${BUILD}/daemon/input.kiss; wait $$pid
	printf 'mode=NINO_MODE=6\nkiss_pty=1\nkiss_pty_once=1\npty_path_out=${BUILD}/daemon/config.pty\npcm_out=${BUILD}/daemon/config-pty.pcm\n' > ${BUILD}/daemon/pty.conf
	./${DAEMONBIN} --config ${BUILD}/daemon/pty.conf --once > ${BUILD}/daemon/config-pty.log 2> ${BUILD}/daemon/config-pty.err & pid=$$!; sleep 1; test -s ${BUILD}/daemon/config.pty; ./${PTYCLIENTBIN} ${BUILD}/daemon/config.pty ${BUILD}/daemon/input.kiss; wait $$pid
	if ./${DAEMONBIN} --kiss-tcp-listen 127.0.0.1:18017 --kiss-unix-listen ${BUILD}/daemon/conflict.sock --kiss-tcp-once --kiss-unix-once --pcm-out ${BUILD}/daemon/conflict.pcm --once; then exit 1; else exit 0; fi
	if ./${DAEMONBIN} --kiss-tcp-listen 127.0.0.1:18018 --kiss-tcp-once --kiss-pty --kiss-pty-once --pcm-out ${BUILD}/daemon/conflict-pty.pcm --once; then exit 1; else exit 0; fi
	if ./${DAEMONBIN} --kiss-unix-listen ${BUILD}/daemon/conflict.sock --kiss-unix-once --kiss-pty --kiss-pty-once --pcm-out ${BUILD}/daemon/conflict-unix-pty.pcm --once; then exit 1; else exit 0; fi
	test -s ${BUILD}/daemon/input.kiss
	test -s ${BUILD}/daemon/input.pcm
	test -s ${BUILD}/daemon/tx.pcm
	test -s ${BUILD}/daemon/rx.kiss
	test -s ${BUILD}/daemon/loop.kiss
	test -s ${BUILD}/daemon/config-tx.pcm
	test -s ${BUILD}/daemon/stdin-tx.pcm
	test -s ${BUILD}/daemon/stdout-tx.pcm
	test -s ${BUILD}/daemon/stdout-tx.diag
	test -s ${BUILD}/daemon/stdout-rx.kiss
	test -s ${BUILD}/daemon/stdout-rx.diag
	test -s ${BUILD}/daemon/tcp-tx.pcm
	test -s ${BUILD}/daemon/config-tcp.pcm
	test -s ${BUILD}/daemon/unix-tx.pcm
	test -s ${BUILD}/daemon/config-unix.pcm
	test -s ${BUILD}/daemon/kilotnc.pty
	test -s ${BUILD}/daemon/pty-tx.pcm
	test -s ${BUILD}/daemon/config.pty
	test -s ${BUILD}/daemon/config-pty.pcm

${SANBIN}: ${SRCS}
	mkdir -p ${BUILD}
	${CC} ${CFLAGS} ${SANFLAGS} -o $@ ${SRCS}

${TOOLBIN}: ${CORE_SRCS} ${TOOL_SRCS}
	mkdir -p ${BUILD}
	${CC} ${CFLAGS} -o $@ ${CORE_SRCS} ${TOOL_SRCS}

${DAEMONBIN}: ${CORE_SRCS} ${DAEMON_SRCS}
	mkdir -p ${BUILD}
	${CC} ${CFLAGS} -o $@ ${CORE_SRCS} ${DAEMON_SRCS}

${TCPCLIENTBIN}: daemon/kilotncd_tcp_client.c
	mkdir -p ${BUILD}
	${CC} ${CFLAGS} -o $@ daemon/kilotncd_tcp_client.c

${UNIXCLIENTBIN}: daemon/kilotncd_unix_client.c
	mkdir -p ${BUILD}
	${CC} ${CFLAGS} -o $@ daemon/kilotncd_unix_client.c

${PTYCLIENTBIN}: daemon/kilotncd_pty_client.c
	mkdir -p ${BUILD}
	${CC} ${CFLAGS} -o $@ daemon/kilotncd_pty_client.c

clean:
	rm -rf ${BUILD}

.PHONY: all test sanitize tools tool-test daemon daemon-test clean
