# KiloTNC - Developed by M6VPN (M6VPN@tuta.com)
# KiloTNC/Makefile

CC	?= cc
CFLAGS	+= -std=c99 -Wall -Wextra -Wconversion -Wsign-conversion -Werror
CFLAGS	+= -I firmware/include
SANFLAGS ?= -fsanitize=address,undefined -fno-omit-frame-pointer

BUILD	= build
TESTBIN	= ${BUILD}/kilotnc_tests
SANBIN	= ${BUILD}/kilotnc_tests_sanitize

SRCS	= firmware/src/afsk1200.c \
	  firmware/src/afsk1200_rx.c \
	  firmware/src/ax25.c \
	  firmware/src/fcs.c \
	  firmware/src/hdlc.c \
	  firmware/src/kiss.c \
	  firmware/test/test_afsk1200.c \
	  firmware/test/test_afsk1200_rx.c \
	  firmware/test/test_ax25.c \
	  firmware/test/test_fcs.c \
	  firmware/test/test_fuzz.c \
	  firmware/test/test_hdlc.c \
	  firmware/test/test_kiss.c \
	  firmware/test/test_main.c

all: ${TESTBIN}

${TESTBIN}: ${SRCS}
	mkdir -p ${BUILD}
	${CC} ${CFLAGS} -o $@ ${SRCS}

test: ${TESTBIN}
	./${TESTBIN}

sanitize: ${SANBIN}
	./${SANBIN}

${SANBIN}: ${SRCS}
	mkdir -p ${BUILD}
	${CC} ${CFLAGS} ${SANFLAGS} -o $@ ${SRCS}

clean:
	rm -rf ${BUILD}

.PHONY: all test sanitize clean
