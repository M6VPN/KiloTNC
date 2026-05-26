#!/usr/bin/env bash
# KiloTNC - Developed by M6VPN (M6VPN@tuta.com)
# KiloTNC/interop/run_ax25_optional.sh

set -eu

mkdir -p build/interop

missing=0
for tool in kissattach kissparms; do
	if ! command -v "${tool}" >/dev/null 2>&1; then
		printf 'skip: %s not found\n' "${tool}"
		missing=1
	fi
done

if [ "${missing}" = '1' ]; then
	exit 0
fi

if [ "${KILOTNC_INTEROP_RUN:-0}" != '1' ]; then
	printf '%s\n' 'skip: set KILOTNC_INTEROP_RUN=1 to enable optional Linux AX.25 checks'
	printf '%s\n' 'planned: local KISS client checks through PTY or TCP adapters'
	exit 0
fi

printf '%s\n' 'skip: Linux AX.25 optional harness is a placeholder in M1.26'
printf '%s\n' 'no kernel AX.25 setup changed'
printf '%s\n' 'no RF transmit attempted'
