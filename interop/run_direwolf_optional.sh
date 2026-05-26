#!/usr/bin/env bash
# KiloTNC - Developed by M6VPN (M6VPN@tuta.com)
# KiloTNC/interop/run_direwolf_optional.sh

set -eu

mkdir -p build/interop

if ! command -v direwolf >/dev/null 2>&1; then
	printf '%s\n' 'skip: direwolf not found'
	exit 0
fi

if [ "${KILOTNC_INTEROP_RUN:-0}" != '1' ]; then
	printf '%s\n' 'skip: set KILOTNC_INTEROP_RUN=1 to enable optional Dire Wolf checks'
	printf '%s\n' 'planned: KISS TCP/PTY framing and generated PCM/WAV decode checks'
	exit 0
fi

printf '%s\n' 'skip: Dire Wolf optional harness is a placeholder in M1.26'
printf '%s\n' 'no persistent service started'
printf '%s\n' 'no RF transmit attempted'
