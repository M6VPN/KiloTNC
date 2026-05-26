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
TOOL_SRCS = tools/kilotnc_cli.c \
	  tools/wav_writer.c
DAEMON_SRCS = daemon/kilotncd.c \
	  daemon/kilotncd_config.c \
	  daemon/kilotncd_file.c \
	  daemon/kilotncd_tcp.c

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

daemon: ${DAEMONBIN} ${TCPCLIENTBIN}

daemon-test: ${DAEMONBIN} ${TCPCLIENTBIN} ${TOOLBIN}
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
	./${DAEMONBIN} --kiss-tcp-listen 127.0.0.1:18015 --kiss-tcp-once --pcm-out ${BUILD}/daemon/tcp-tx.pcm --once > ${BUILD}/daemon/tcp.log 2>&1 & pid=$$!; sleep 1; ./${TCPCLIENTBIN} 127.0.0.1 18015 ${BUILD}/daemon/input.kiss; wait $$pid
	printf 'unknown=1\n' > ${BUILD}/daemon/invalid.conf
	if ./${DAEMONBIN} --config ${BUILD}/daemon/invalid.conf --status; then exit 1; else exit 0; fi
	if ./${DAEMONBIN} --mode NINO_MODE=0 --kiss-in ${BUILD}/daemon/input.kiss --pcm-out ${BUILD}/daemon/bad.pcm --once; then exit 1; else exit 0; fi
	if ./${DAEMONBIN} --kiss-tcp-listen 0.0.0.0:18015 --kiss-tcp-once --pcm-out ${BUILD}/daemon/nonlocal.pcm --once; then exit 1; else exit 0; fi
	printf 'mode=NINO_MODE=6\nkiss_tcp_listen=127.0.0.1:18016\nkiss_tcp_once=1\nallow_nonlocal_bind=0\npcm_out=${BUILD}/daemon/config-tcp.pcm\n' > ${BUILD}/daemon/tcp.conf
	./${DAEMONBIN} --config ${BUILD}/daemon/tcp.conf --once > ${BUILD}/daemon/config-tcp.log 2>&1 & pid=$$!; sleep 1; ./${TCPCLIENTBIN} 127.0.0.1 18016 ${BUILD}/daemon/input.kiss; wait $$pid
	test -s ${BUILD}/daemon/input.kiss
	test -s ${BUILD}/daemon/input.pcm
	test -s ${BUILD}/daemon/tx.pcm
	test -s ${BUILD}/daemon/rx.kiss
	test -s ${BUILD}/daemon/loop.kiss
	test -s ${BUILD}/daemon/config-tx.pcm
	test -s ${BUILD}/daemon/tcp-tx.pcm
	test -s ${BUILD}/daemon/config-tcp.pcm

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

clean:
	rm -rf ${BUILD}

.PHONY: all test sanitize tools tool-test daemon daemon-test clean
