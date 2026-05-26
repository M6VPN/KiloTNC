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
TOOL_SRCS = tools/kilotnc_cli.c \
	  tools/wav_writer.c

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

${SANBIN}: ${SRCS}
	mkdir -p ${BUILD}
	${CC} ${CFLAGS} ${SANFLAGS} -o $@ ${SRCS}

${TOOLBIN}: ${CORE_SRCS} ${TOOL_SRCS}
	mkdir -p ${BUILD}
	${CC} ${CFLAGS} -o $@ ${CORE_SRCS} ${TOOL_SRCS}

clean:
	rm -rf ${BUILD}

.PHONY: all test sanitize tools tool-test clean
