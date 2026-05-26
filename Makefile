# KiloTNC - Developed by M6VPN (M6VPN@tuta.com)
# KiloTNC/Makefile

CC	?= cc
CFLAGS	+= -std=c99 -Wall -Wextra -Wconversion -Wsign-conversion -Werror
CFLAGS	+= -I firmware/include -I daemon
SANFLAGS ?= -fsanitize=address,undefined -fno-omit-frame-pointer
ENABLE_ALSA ?= 0
ENABLE_SNDIO ?= 0
ENABLE_OSS ?= 0
ARM_NONE_EABI_CC ?= arm-none-eabi-gcc

BUILD	= build
TESTBIN	= ${BUILD}/kilotnc_tests
SANBIN	= ${BUILD}/kilotnc_tests_sanitize
TOOLBIN	= ${BUILD}/kilotnc_cli
DAEMONBIN = ${BUILD}/kilotncd
TCPCLIENTBIN = ${BUILD}/kilotncd_tcp_client
UNIXCLIENTBIN = ${BUILD}/kilotncd_unix_client
PTYCLIENTBIN = ${BUILD}/kilotncd_pty_client
KISSTESTBIN = ${BUILD}/kilotncd_kiss_test
EMBEDBIN = ${BUILD}/kilotnc_embedded_tests
EMBED_CFLAGS = ${CFLAGS} -I embedded/include -I embedded/app \
	  -I embedded/platform -I embedded/targets/stm32h753-nucleo
EMBED_TARGET_CFLAGS = -std=c99 -Wall -Wextra -Wconversion \
	  -Wsign-conversion -Werror -DKILOTNC_TARGET_STM32H753_NUCLEO \
	  -I embedded/include -I embedded/app -I embedded/platform \
	  -I embedded/targets/stm32h753-nucleo
TOOL_SRCS = tools/kilotnc_cli.c \
	  daemon/kilotncd_control.c \
	  tools/wav_writer.c
DAEMON_SRCS = daemon/kilotncd.c \
	  daemon/kilotncd_audio.c \
	  daemon/kilotncd_audio_alsa.c \
	  daemon/kilotncd_audio_oss.c \
	  daemon/kilotncd_audio_raw.c \
	  daemon/kilotncd_audio_sndio.c \
	  daemon/kilotncd_config.c \
	  daemon/kilotncd_control.c \
	  daemon/kilotncd_file.c \
	  daemon/kilotncd_loop.c \
	  daemon/kilotncd_profile.c \
	  daemon/kilotncd_pty.c \
	  daemon/kilotncd_radio.c \
	  daemon/kilotncd_radio_log.c \
	  daemon/kilotncd_radio_none.c \
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
	  firmware/test/test_kilotncd_control.c \
	  firmware/test/test_kiss.c \
	  firmware/test/test_tnc_control.c \
	  firmware/test/test_tnc_diag.c \
	  firmware/test/test_tnc_mode.c \
	  firmware/test/test_tnc1200.c \
	  firmware/test/test_main.c

SRCS	= ${CORE_SRCS} ${TEST_SRCS} daemon/kilotncd_control.c

all: ${TESTBIN}

help:
	@printf '%s\n' 'KiloTNC targets:'
	@printf '%s\n' '  test              build and run host tests'
	@printf '%s\n' '  sanitize          run sanitizer host tests'
	@printf '%s\n' '  tools             build host CLI tools'
	@printf '%s\n' '  tool-test         run deterministic CLI checks'
	@printf '%s\n' '  daemon            build kilotncd and local test clients'
	@printf '%s\n' '  daemon-test       run deterministic daemon checks'
	@printf '%s\n' '  kiss-compat-test  run local KISS compatibility checks'
	@printf '%s\n' '  embedded-test     build and run host-native embedded skeleton tests'
	@printf '%s\n' '  embedded-target-help show opt-in STM32H753 target guidance'
	@printf '%s\n' '  embedded-target-check check opt-in STM32H753 skeleton syntax'
	@printf '%s\n' '  interop-help      show optional interop wrapper guidance'
	@printf '%s\n' '  embedded-help     show M2 embedded skeleton guidance'
	@printf '%s\n' '  clean             remove build outputs'

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
	./${TOOLBIN} control --cmd status
	./${TOOLBIN} control --cmd "mode NINO_MODE=6"
	./${TOOLBIN} control --cmd "dcd 1"
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

interop-help:
	@printf '%s\n' 'Optional black-box interoperability helpers:'
	@printf '%s\n' '  interop/run_direwolf_optional.sh'
	@printf '%s\n' '  interop/run_ninotnc_optional.sh'
	@printf '%s\n' '  interop/run_ax25_optional.sh'
	@printf '%s\n' ''
	@printf '%s\n' 'These helpers are not part of CI.'
	@printf '%s\n' 'They skip safely unless prerequisites are present.'
	@printf '%s\n' 'Set KILOTNC_INTEROP_RUN=1 only for explicit local tests.'

embedded-help:
	@printf '%s\n' 'M2.10 embedded status: compile-gated STM32H753 target skeleton.'
	@printf '%s\n' 'Run make embedded-test for the skeleton test.'
	@printf '%s\n' 'No ARM toolchain is required for normal CI.'
	@printf '%s\n' 'Planned target: stm32h753-nucleo'
	@printf '%s\n' 'Reserved future environment variables:'
	@printf '%s\n' '  KILOTNC_EMBEDDED_TARGET=stm32h753-nucleo'
	@printf '%s\n' '  STM32CUBE_PATH=/path/to/STM32Cube...'
	@printf '%s\n' '  ARM_NONE_EABI_CC=arm-none-eabi-gcc'

embedded-target-help:
	@printf '%s\n' 'KiloTNC opt-in embedded target skeleton:'
	@printf '%s\n' '  target: stm32h753-nucleo'
	@printf '%s\n' '  board path: NUCLEO-H753ZI or current equivalent STM32H753 Nucleo-144 board'
	@printf '%s\n' '  normal CI target: make embedded-test'
	@printf '%s\n' '  opt-in syntax check: make embedded-target-check'
	@printf '%s\n' ''
	@printf '%s\n' 'Reserved future environment variables:'
	@printf '%s\n' '  KILOTNC_EMBEDDED_TARGET=stm32h753-nucleo'
	@printf '%s\n' '  ARM_NONE_EABI_CC=arm-none-eabi-gcc'
	@printf '%s\n' '  STM32CUBE_PATH=/path/to/STM32Cube...'
	@printf '%s\n' ''
	@printf '%s\n' 'No STM32Cube, CMSIS, TinyUSB, HAL, linker script, startup code, or flashable image is committed.'

embedded-target-check:
	@if command -v ${ARM_NONE_EABI_CC} >/dev/null 2>&1; then \
		printf '%s\n' 'checking stm32h753-nucleo target skeleton syntax'; \
		${ARM_NONE_EABI_CC} ${EMBED_TARGET_CFLAGS} -fsyntax-only \
			embedded/targets/stm32h753-nucleo/target_main.c \
			embedded/targets/stm32h753-nucleo/target_platform.c; \
	else \
		printf '%s\n' 'skip: arm-none-eabi-gcc not found; target skeleton syntax check not run'; \
		printf '%s\n' 'set ARM_NONE_EABI_CC=/path/to/arm-none-eabi-gcc to opt in'; \
	fi

embedded-test: ${EMBEDBIN}
	./${EMBEDBIN}

kiss-compat-test: ${DAEMONBIN} ${TCPCLIENTBIN} ${UNIXCLIENTBIN} ${PTYCLIENTBIN} ${TOOLBIN} ${KISSTESTBIN}
	mkdir -p ${BUILD}/kiss-compat
	./${KISSTESTBIN} generate ${BUILD}/kiss-compat
	./${KISSTESTBIN} expect ${BUILD}/kiss-compat/plain.kiss plain
	./${KISSTESTBIN} expect ${BUILD}/kiss-compat/escaped.kiss escaped
	./${KISSTESTBIN} expect ${BUILD}/kiss-compat/multi.kiss multi
	./${TOOLBIN} loopback --in ${BUILD}/kiss-compat/plain.kiss --out ${BUILD}/kiss-compat/cli-plain.out.kiss --mode NINO_MODE=6 > ${BUILD}/kiss-compat/cli-plain.diag
	./${KISSTESTBIN} expect ${BUILD}/kiss-compat/cli-plain.out.kiss plain
	./${TOOLBIN} loopback --in ${BUILD}/kiss-compat/escaped.kiss --out ${BUILD}/kiss-compat/cli-escaped.out.kiss --mode NINO_MODE=6 > ${BUILD}/kiss-compat/cli-escaped.diag
	./${KISSTESTBIN} expect ${BUILD}/kiss-compat/cli-escaped.out.kiss escaped
	./${TOOLBIN} vector-loopback --prefix ${BUILD}/kiss-compat/vector --mode NINO_MODE=6 > ${BUILD}/kiss-compat/vector.log
	test -s ${BUILD}/kiss-compat/vector.out.kiss
	./${DAEMONBIN} --mode NINO_MODE=6 --kiss-in ${BUILD}/kiss-compat/plain.kiss --kiss-out ${BUILD}/kiss-compat/file-plain.out.kiss --loopback-once > ${BUILD}/kiss-compat/file-plain.diag 2>&1
	./${KISSTESTBIN} expect ${BUILD}/kiss-compat/file-plain.out.kiss plain
	./${DAEMONBIN} --mode NINO_MODE=6 --kiss-in ${BUILD}/kiss-compat/escaped.kiss --kiss-out ${BUILD}/kiss-compat/file-escaped.out.kiss --loopback-once > ${BUILD}/kiss-compat/file-escaped.diag 2>&1
	./${KISSTESTBIN} expect ${BUILD}/kiss-compat/file-escaped.out.kiss escaped
	./${DAEMONBIN} --mode NINO_MODE=6 --kiss-in ${BUILD}/kiss-compat/plain.kiss --pcm-out ${BUILD}/kiss-compat/file-plain.pcm --once > ${BUILD}/kiss-compat/file-tx.diag 2>&1
	test -s ${BUILD}/kiss-compat/file-plain.pcm
	./${DAEMONBIN} --mode NINO_MODE=6 --pcm-in ${BUILD}/kiss-compat/file-plain.pcm --kiss-out - --once > ${BUILD}/kiss-compat/stdout-rx.out.kiss 2> ${BUILD}/kiss-compat/stdout-rx.diag
	./${KISSTESTBIN} expect ${BUILD}/kiss-compat/stdout-rx.out.kiss plain
	./${DAEMONBIN} --mode NINO_MODE=6 --kiss-in - --pcm-out ${BUILD}/kiss-compat/stdin-escaped.pcm --once < ${BUILD}/kiss-compat/escaped.kiss > ${BUILD}/kiss-compat/stdin-escaped.diag 2>&1
	test -s ${BUILD}/kiss-compat/stdin-escaped.pcm
	./${DAEMONBIN} --mode NINO_MODE=6 --kiss-in ${BUILD}/kiss-compat/commands.kiss --pcm-out ${BUILD}/kiss-compat/commands.pcm --once > ${BUILD}/kiss-compat/commands.diag 2>&1
	test -s ${BUILD}/kiss-compat/commands.pcm
	grep -q "kiss_ignored=1" ${BUILD}/kiss-compat/commands.diag
	grep -q "last_nino=22" ${BUILD}/kiss-compat/commands.diag
	grep -q "mode_temp=1" ${BUILD}/kiss-compat/commands.diag
	./${DAEMONBIN} --mode NINO_MODE=6 --kiss-in ${BUILD}/kiss-compat/malformed.kiss --pcm-out ${BUILD}/kiss-compat/malformed.pcm --once > ${BUILD}/kiss-compat/malformed.diag 2>&1
	test -s ${BUILD}/kiss-compat/malformed.pcm
	grep -q "kiss_parse_errors=1" ${BUILD}/kiss-compat/malformed.diag
	./${DAEMONBIN} --mode NINO_MODE=6 --kiss-in ${BUILD}/kiss-compat/repeated-fend.kiss --pcm-out ${BUILD}/kiss-compat/repeated-fend.pcm --once > ${BUILD}/kiss-compat/repeated-fend.diag 2>&1
	test -s ${BUILD}/kiss-compat/repeated-fend.pcm
	./${DAEMONBIN} --kiss-tcp-listen 127.0.0.1:18125 --kiss-tcp-once --pcm-out ${BUILD}/kiss-compat/tcp-plain.pcm --once > ${BUILD}/kiss-compat/tcp-plain.diag 2>&1 & pid=$$!; sleep 1; ./${TCPCLIENTBIN} 127.0.0.1 18125 ${BUILD}/kiss-compat/plain.kiss; wait $$pid
	test -s ${BUILD}/kiss-compat/tcp-plain.pcm
	./${DAEMONBIN} --kiss-tcp-listen 127.0.0.1:18126 --kiss-tcp-once --pcm-out ${BUILD}/kiss-compat/tcp-escaped.pcm --once > ${BUILD}/kiss-compat/tcp-escaped.diag 2>&1 & pid=$$!; sleep 1; ./${TCPCLIENTBIN} 127.0.0.1 18126 ${BUILD}/kiss-compat/escaped.kiss; wait $$pid
	test -s ${BUILD}/kiss-compat/tcp-escaped.pcm
	./${DAEMONBIN} --kiss-tcp-listen 127.0.0.1:18127 --kiss-tcp-once --pcm-out ${BUILD}/kiss-compat/tcp-commands.pcm --once > ${BUILD}/kiss-compat/tcp-commands.diag 2>&1 & pid=$$!; sleep 1; ./${TCPCLIENTBIN} 127.0.0.1 18127 ${BUILD}/kiss-compat/commands.kiss; wait $$pid
	test -s ${BUILD}/kiss-compat/tcp-commands.pcm
	grep -q "kiss_ignored=1" ${BUILD}/kiss-compat/tcp-commands.diag
	grep -q "last_nino=22" ${BUILD}/kiss-compat/tcp-commands.diag
	./${DAEMONBIN} --kiss-unix-listen ${BUILD}/kiss-compat/plain.sock --kiss-unix-once --pcm-out ${BUILD}/kiss-compat/unix-plain.pcm --once > ${BUILD}/kiss-compat/unix-plain.diag 2>&1 & pid=$$!; sleep 1; ./${UNIXCLIENTBIN} ${BUILD}/kiss-compat/plain.sock ${BUILD}/kiss-compat/plain.kiss; wait $$pid
	test -s ${BUILD}/kiss-compat/unix-plain.pcm
	./${DAEMONBIN} --kiss-unix-listen ${BUILD}/kiss-compat/escaped.sock --kiss-unix-once --pcm-out ${BUILD}/kiss-compat/unix-escaped.pcm --once > ${BUILD}/kiss-compat/unix-escaped.diag 2>&1 & pid=$$!; sleep 1; ./${UNIXCLIENTBIN} ${BUILD}/kiss-compat/escaped.sock ${BUILD}/kiss-compat/escaped.kiss; wait $$pid
	test -s ${BUILD}/kiss-compat/unix-escaped.pcm
	./${DAEMONBIN} --kiss-unix-listen ${BUILD}/kiss-compat/malformed.sock --kiss-unix-once --pcm-out ${BUILD}/kiss-compat/unix-malformed.pcm --once > ${BUILD}/kiss-compat/unix-malformed.diag 2>&1 & pid=$$!; sleep 1; ./${UNIXCLIENTBIN} ${BUILD}/kiss-compat/malformed.sock ${BUILD}/kiss-compat/malformed.kiss; wait $$pid
	test -s ${BUILD}/kiss-compat/unix-malformed.pcm
	grep -q "kiss_parse_errors=1" ${BUILD}/kiss-compat/unix-malformed.diag
	./${DAEMONBIN} --kiss-pty --kiss-pty-once --pty-path-out ${BUILD}/kiss-compat/plain.pty --pcm-out ${BUILD}/kiss-compat/pty-plain.pcm --once > ${BUILD}/kiss-compat/pty-plain.diag 2> ${BUILD}/kiss-compat/pty-plain.err & pid=$$!; sleep 1; ./${PTYCLIENTBIN} ${BUILD}/kiss-compat/plain.pty ${BUILD}/kiss-compat/plain.kiss; wait $$pid
	test -s ${BUILD}/kiss-compat/pty-plain.pcm
	./${DAEMONBIN} --kiss-pty --kiss-pty-once --pty-path-out ${BUILD}/kiss-compat/escaped.pty --pcm-out ${BUILD}/kiss-compat/pty-escaped.pcm --once > ${BUILD}/kiss-compat/pty-escaped.diag 2> ${BUILD}/kiss-compat/pty-escaped.err & pid=$$!; sleep 1; ./${PTYCLIENTBIN} ${BUILD}/kiss-compat/escaped.pty ${BUILD}/kiss-compat/escaped.kiss; wait $$pid
	test -s ${BUILD}/kiss-compat/pty-escaped.pcm
	./${DAEMONBIN} --kiss-pty --kiss-pty-once --pty-path-out ${BUILD}/kiss-compat/repeated-fend.pty --pcm-out ${BUILD}/kiss-compat/pty-repeated-fend.pcm --once > ${BUILD}/kiss-compat/pty-repeated-fend.diag 2> ${BUILD}/kiss-compat/pty-repeated-fend.err & pid=$$!; sleep 1; ./${PTYCLIENTBIN} ${BUILD}/kiss-compat/repeated-fend.pty ${BUILD}/kiss-compat/repeated-fend.kiss; wait $$pid
	test -s ${BUILD}/kiss-compat/pty-repeated-fend.pcm

daemon-test: ${DAEMONBIN} ${TCPCLIENTBIN} ${UNIXCLIENTBIN} ${PTYCLIENTBIN} ${TOOLBIN}
	mkdir -p ${BUILD}/daemon
	./${TOOLBIN} generate-kiss --out ${BUILD}/daemon/input.kiss --dst APZKTN --src M6VPN --info "KiloTNC daemon"
	./${TOOLBIN} generate-pcm --out ${BUILD}/daemon/input.pcm --dst APZKTN --src M6VPN --info "KiloTNC daemon" --mode NINO_MODE=6
	./${DAEMONBIN} --status
	./${DAEMONBIN} --control status
	./${DAEMONBIN} --control diag
	./${DAEMONBIN} --control "mode NINO_MODE=6"
	./${DAEMONBIN} --control "mode NINO_MODE=22"
	if ./${DAEMONBIN} --control "mode NINO_MODE=0"; then exit 1; else exit 0; fi
	./${DAEMONBIN} --foreground --dry-run --max-iterations 1
	./${DAEMONBIN} --foreground --dry-run --max-iterations 3 --diag-interval 1 2> ${BUILD}/daemon/foreground-diag.log
	test -s ${BUILD}/daemon/foreground-diag.log
	if ./${DAEMONBIN} --foreground --dry-run --max-iterations 0; then exit 1; else exit 0; fi
	if ./${DAEMONBIN} --foreground --dry-run --max-iterations 1 --diag-interval bad; then exit 1; else exit 0; fi
	./${DAEMONBIN} --profile status
	./${DAEMONBIN} --status --mode NINO_MODE=6
	./${DAEMONBIN} --status --mode NINO_MODE=22
	./${DAEMONBIN} --profile status --mode NINO_MODE=0
	./${DAEMONBIN} --status --radio-backend none
	printf 'radio_backend=none\n' > ${BUILD}/daemon/radio-none.conf
	./${DAEMONBIN} --config ${BUILD}/daemon/radio-none.conf --status
	./${DAEMONBIN} --config daemon/examples/status.conf
	./${DAEMONBIN} --profile file-tx --mode NINO_MODE=6 --kiss-in ${BUILD}/daemon/input.kiss --pcm-out ${BUILD}/daemon/profile-file-tx.pcm
	./${DAEMONBIN} --profile file-rx --mode NINO_MODE=6 --pcm-in ${BUILD}/daemon/input.pcm --kiss-out ${BUILD}/daemon/profile-file-rx.kiss
	./${DAEMONBIN} --profile file-loopback --mode NINO_MODE=6 --kiss-in ${BUILD}/daemon/input.kiss --kiss-out ${BUILD}/daemon/profile-file-loopback.kiss
	./${DAEMONBIN} --mode NINO_MODE=6 --kiss-in ${BUILD}/daemon/input.kiss --pcm-out ${BUILD}/daemon/tx.pcm --once
	./${DAEMONBIN} --mode NINO_MODE=6 --radio-backend log --radio-log ${BUILD}/daemon/ptt.log --kiss-in ${BUILD}/daemon/input.kiss --pcm-out ${BUILD}/daemon/radio-log-tx.pcm --once
	./${DAEMONBIN} --mode NINO_MODE=6 --pcm-in ${BUILD}/daemon/input.pcm --kiss-out ${BUILD}/daemon/rx.kiss --once
	printf 'mode=NINO_MODE=6\naudio_backend=raw\naudio_sample_rate=48000\naudio_channels=1\naudio_bits=16\nkiss_in=${BUILD}/daemon/input.kiss\npcm_out=${BUILD}/daemon/audio-raw.pcm\n' > ${BUILD}/daemon/audio-raw.conf
	./${DAEMONBIN} --config ${BUILD}/daemon/audio-raw.conf --once
	./${DAEMONBIN} --mode NINO_MODE=6 --kiss-in ${BUILD}/daemon/input.kiss --kiss-out ${BUILD}/daemon/loop.kiss --loopback-once
	./${DAEMONBIN} --config daemon/example.conf --once
	./${DAEMONBIN} --config daemon/examples/file-tx.conf
	./${DAEMONBIN} --config daemon/examples/file-rx.conf
	./${DAEMONBIN} --config daemon/examples/file-loopback.conf
	./${DAEMONBIN} --config daemon/examples/foreground-dry-run.conf
	./${DAEMONBIN} --foreground --profile file-loopback --max-iterations 1 --mode NINO_MODE=6 --kiss-in ${BUILD}/daemon/input.kiss --kiss-out ${BUILD}/daemon/foreground-loopback.kiss
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
	printf 'audio_backend=alsa\n' > ${BUILD}/daemon/audio-bad-backend.conf
	if ./${DAEMONBIN} --config ${BUILD}/daemon/audio-bad-backend.conf --status; then exit 1; else exit 0; fi
	printf 'mode=NINO_MODE=6\naudio_backend=alsa\nkiss_in=${BUILD}/daemon/input.kiss\npcm_out=${BUILD}/daemon/alsa-stub.pcm\n' > ${BUILD}/daemon/audio-alsa-tx.conf
	if ./${DAEMONBIN} --config ${BUILD}/daemon/audio-alsa-tx.conf --once; then exit 1; else exit 0; fi
	printf 'audio_backend=sndio\n' > ${BUILD}/daemon/audio-sndio-status.conf
	if ./${DAEMONBIN} --config ${BUILD}/daemon/audio-sndio-status.conf --status; then exit 1; else exit 0; fi
	printf 'mode=NINO_MODE=6\naudio_backend=sndio\nkiss_in=${BUILD}/daemon/input.kiss\npcm_out=${BUILD}/daemon/sndio-stub.pcm\n' > ${BUILD}/daemon/audio-sndio-tx.conf
	if ./${DAEMONBIN} --config ${BUILD}/daemon/audio-sndio-tx.conf --once; then exit 1; else exit 0; fi
	printf 'audio_backend=oss\n' > ${BUILD}/daemon/audio-oss-status.conf
	if ./${DAEMONBIN} --config ${BUILD}/daemon/audio-oss-status.conf --status; then exit 1; else exit 0; fi
	printf 'mode=NINO_MODE=6\naudio_backend=oss\nkiss_in=${BUILD}/daemon/input.kiss\npcm_out=${BUILD}/daemon/oss-stub.pcm\n' > ${BUILD}/daemon/audio-oss-tx.conf
	if ./${DAEMONBIN} --config ${BUILD}/daemon/audio-oss-tx.conf --once; then exit 1; else exit 0; fi
	printf 'audio_sample_rate=44100\n' > ${BUILD}/daemon/audio-bad-rate.conf
	if ./${DAEMONBIN} --config ${BUILD}/daemon/audio-bad-rate.conf --status; then exit 1; else exit 0; fi
	printf 'audio_channels=2\n' > ${BUILD}/daemon/audio-bad-channels.conf
	if ./${DAEMONBIN} --config ${BUILD}/daemon/audio-bad-channels.conf --status; then exit 1; else exit 0; fi
	printf 'audio_bits=24\n' > ${BUILD}/daemon/audio-bad-bits.conf
	if ./${DAEMONBIN} --config ${BUILD}/daemon/audio-bad-bits.conf --status; then exit 1; else exit 0; fi
	if ./${DAEMONBIN} --radio-backend serial-rts --status; then exit 1; else exit 0; fi
	if ./${DAEMONBIN} --radio-backend bad-radio --status; then exit 1; else exit 0; fi
	if ./${DAEMONBIN} --profile bad-profile; then exit 1; else exit 0; fi
	if ./${DAEMONBIN} --profile file-tx; then exit 1; else exit 0; fi
	if ./${DAEMONBIN} --mode NINO_MODE=0 --kiss-in ${BUILD}/daemon/input.kiss --pcm-out ${BUILD}/daemon/bad.pcm --once; then exit 1; else exit 0; fi
	if ./${DAEMONBIN} --kiss-tcp-listen 0.0.0.0:18015 --kiss-tcp-once --pcm-out ${BUILD}/daemon/nonlocal.pcm --once; then exit 1; else exit 0; fi
	if ./${DAEMONBIN} --profile tcp-kiss-once --kiss-tcp-listen 127.0.0.1:18020 --kiss-tcp-once --kiss-in ${BUILD}/daemon/input.kiss --pcm-out ${BUILD}/daemon/tcp-kiss-conflict.pcm; then exit 1; else exit 0; fi
	printf 'mode=NINO_MODE=6\nkiss_tcp_listen=127.0.0.1:18016\nkiss_tcp_once=1\nallow_nonlocal_bind=0\npcm_out=${BUILD}/daemon/config-tcp.pcm\n' > ${BUILD}/daemon/tcp.conf
	./${DAEMONBIN} --config ${BUILD}/daemon/tcp.conf --once > ${BUILD}/daemon/config-tcp.log 2>&1 & pid=$$!; sleep 1; ./${TCPCLIENTBIN} 127.0.0.1 18016 ${BUILD}/daemon/input.kiss; wait $$pid
	./${DAEMONBIN} --config daemon/examples/tcp-kiss-once.conf > ${BUILD}/daemon/example-tcp.log 2>&1 & pid=$$!; sleep 1; ./${TCPCLIENTBIN} 127.0.0.1 18019 ${BUILD}/daemon/input.kiss; wait $$pid
	printf 'mode=NINO_MODE=6\nkiss_unix_listen=${BUILD}/daemon/config.sock\nkiss_unix_once=1\nunlink_stale_socket=0\npcm_out=${BUILD}/daemon/config-unix.pcm\n' > ${BUILD}/daemon/unix.conf
	./${DAEMONBIN} --config ${BUILD}/daemon/unix.conf --once > ${BUILD}/daemon/config-unix.log 2>&1 & pid=$$!; sleep 1; ./${UNIXCLIENTBIN} ${BUILD}/daemon/config.sock ${BUILD}/daemon/input.kiss; wait $$pid
	./${DAEMONBIN} --config daemon/examples/unix-kiss-once.conf > ${BUILD}/daemon/example-unix.log 2>&1 & pid=$$!; sleep 1; ./${UNIXCLIENTBIN} ${BUILD}/daemon/example.sock ${BUILD}/daemon/input.kiss; wait $$pid
	printf 'mode=NINO_MODE=6\nkiss_pty=1\nkiss_pty_once=1\npty_path_out=${BUILD}/daemon/config.pty\npcm_out=${BUILD}/daemon/config-pty.pcm\n' > ${BUILD}/daemon/pty.conf
	./${DAEMONBIN} --config ${BUILD}/daemon/pty.conf --once > ${BUILD}/daemon/config-pty.log 2> ${BUILD}/daemon/config-pty.err & pid=$$!; sleep 1; test -s ${BUILD}/daemon/config.pty; ./${PTYCLIENTBIN} ${BUILD}/daemon/config.pty ${BUILD}/daemon/input.kiss; wait $$pid
	./${DAEMONBIN} --config daemon/examples/pty-kiss-once.conf > ${BUILD}/daemon/example-pty.log 2> ${BUILD}/daemon/example-pty.err & pid=$$!; sleep 1; test -s ${BUILD}/daemon/example.pty; ./${PTYCLIENTBIN} ${BUILD}/daemon/example.pty ${BUILD}/daemon/input.kiss; wait $$pid
	if ./${DAEMONBIN} --kiss-tcp-listen 127.0.0.1:18017 --kiss-unix-listen ${BUILD}/daemon/conflict.sock --kiss-tcp-once --kiss-unix-once --pcm-out ${BUILD}/daemon/conflict.pcm --once; then exit 1; else exit 0; fi
	if ./${DAEMONBIN} --kiss-tcp-listen 127.0.0.1:18018 --kiss-tcp-once --kiss-pty --kiss-pty-once --pcm-out ${BUILD}/daemon/conflict-pty.pcm --once; then exit 1; else exit 0; fi
	if ./${DAEMONBIN} --kiss-unix-listen ${BUILD}/daemon/conflict.sock --kiss-unix-once --kiss-pty --kiss-pty-once --pcm-out ${BUILD}/daemon/conflict-unix-pty.pcm --once; then exit 1; else exit 0; fi
	test -s ${BUILD}/daemon/input.kiss
	test -s ${BUILD}/daemon/input.pcm
	test -s ${BUILD}/daemon/profile-file-tx.pcm
	test -s ${BUILD}/daemon/profile-file-rx.kiss
	test -s ${BUILD}/daemon/profile-file-loopback.kiss
	test -s ${BUILD}/daemon/tx.pcm
	test -s ${BUILD}/daemon/radio-log-tx.pcm
	test -s ${BUILD}/daemon/ptt.log
	awk '/^ptt=on$$/ { found=1 } END { exit found ? 0 : 1 }' ${BUILD}/daemon/ptt.log
	awk '/^ptt=off$$/ { found=1 } END { exit found ? 0 : 1 }' ${BUILD}/daemon/ptt.log
	test -s ${BUILD}/daemon/rx.kiss
	test -s ${BUILD}/daemon/audio-raw.pcm
	test -s ${BUILD}/daemon/loop.kiss
	test -s ${BUILD}/daemon/config-tx.pcm
	test -s ${BUILD}/daemon/example-file-tx.pcm
	test -s ${BUILD}/daemon/example-file-rx.kiss
	test -s ${BUILD}/daemon/example-file-loopback.kiss
	test -s ${BUILD}/daemon/foreground-loopback.kiss
	test -s ${BUILD}/daemon/stdin-tx.pcm
	test -s ${BUILD}/daemon/stdout-tx.pcm
	test -s ${BUILD}/daemon/stdout-tx.diag
	test -s ${BUILD}/daemon/stdout-rx.kiss
	test -s ${BUILD}/daemon/stdout-rx.diag
	test -s ${BUILD}/daemon/tcp-tx.pcm
	test -s ${BUILD}/daemon/config-tcp.pcm
	test -s ${BUILD}/daemon/example-tcp.pcm
	test -s ${BUILD}/daemon/unix-tx.pcm
	test -s ${BUILD}/daemon/config-unix.pcm
	test -s ${BUILD}/daemon/example-unix.pcm
	test -s ${BUILD}/daemon/kilotnc.pty
	test -s ${BUILD}/daemon/pty-tx.pcm
	test -s ${BUILD}/daemon/config.pty
	test -s ${BUILD}/daemon/config-pty.pcm
	test -s ${BUILD}/daemon/example.pty
	test -s ${BUILD}/daemon/example-pty.pcm

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

${KISSTESTBIN}: ${CORE_SRCS} daemon/kilotncd_kiss_test.c
	mkdir -p ${BUILD}
	${CC} ${CFLAGS} -o $@ ${CORE_SRCS} daemon/kilotncd_kiss_test.c

${EMBEDBIN}: embedded/app/embedded_app.c \
	  embedded/app/embedded_audio.c \
	  embedded/app/embedded_diag.c \
	  embedded/app/embedded_loopback.c \
	  embedded/app/embedded_modem.c \
	  embedded/app/embedded_tnc.c \
	  embedded/app/embedded_usb_bridge.c \
	  embedded/platform/audio_stub.c \
	  embedded/platform/platform_stub.c \
	  embedded/platform/usb_cdc_stub.c \
	  embedded/tests/test_audio_stub.c \
	  embedded/tests/test_embedded_app.c \
	  embedded/tests/test_embedded_audio.c \
	  embedded/tests/test_embedded_diag.c \
	  embedded/tests/test_embedded_loopback.c \
	  embedded/tests/test_embedded_main.c \
	  embedded/tests/test_embedded_modem.c \
	  embedded/tests/test_embedded_tnc.c \
	  embedded/tests/test_embedded_usb_bridge.c \
	  embedded/tests/test_target_metadata.c \
	  embedded/tests/test_usb_cdc_stub.c \
	firmware/src/afsk1200.c \
	firmware/src/afsk1200_stream.c \
	firmware/src/afsk1200_tx.c \
	  firmware/src/ax25.c \
	  firmware/src/fcs.c \
	  firmware/src/hdlc.c \
	  firmware/src/kiss.c \
	  firmware/src/tnc_control.c \
	  firmware/src/tnc_mode.c
	mkdir -p ${BUILD}
	${CC} ${EMBED_CFLAGS} -o $@ embedded/app/embedded_app.c \
		embedded/app/embedded_audio.c \
		embedded/app/embedded_diag.c \
		embedded/app/embedded_loopback.c \
		embedded/app/embedded_modem.c \
		embedded/app/embedded_tnc.c \
		embedded/app/embedded_usb_bridge.c \
		embedded/platform/audio_stub.c \
		embedded/platform/platform_stub.c \
		embedded/platform/usb_cdc_stub.c \
		embedded/tests/test_audio_stub.c \
		embedded/tests/test_embedded_app.c \
		embedded/tests/test_embedded_audio.c \
		embedded/tests/test_embedded_diag.c \
		embedded/tests/test_embedded_loopback.c \
		embedded/tests/test_embedded_main.c \
		embedded/tests/test_embedded_modem.c \
		embedded/tests/test_embedded_tnc.c \
		embedded/tests/test_embedded_usb_bridge.c \
		embedded/tests/test_target_metadata.c \
		embedded/tests/test_usb_cdc_stub.c \
		firmware/src/afsk1200.c \
		firmware/src/afsk1200_stream.c \
		firmware/src/afsk1200_tx.c \
		firmware/src/ax25.c \
		firmware/src/fcs.c \
		firmware/src/hdlc.c \
		firmware/src/kiss.c \
		firmware/src/tnc_control.c \
		firmware/src/tnc_mode.c

clean:
	rm -rf ${BUILD}

.PHONY: all clean daemon daemon-test embedded-help embedded-target-check embedded-target-help embedded-test help interop-help kiss-compat-test sanitize test tools tool-test
