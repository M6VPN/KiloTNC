#!/usr/bin/env bash
# KiloTNC - Developed by M6VPN (M6VPN@tuta.com)
# KiloTNC/interop/run_ninotnc_optional.sh

set -eu

mkdir -p build/interop

if [ -z "${NINOTNC_DEVICE:-}" ]; then
	printf '%s\n' 'skip: NINOTNC_DEVICE is not set'
	exit 0
fi

if [ "${KILOTNC_INTEROP_RUN:-0}" != '1' ]; then
	printf '%s\n' 'skip: set KILOTNC_INTEROP_RUN=1 to enable optional NinoTNC checks'
	printf 'planned device: %s\n' "${NINOTNC_DEVICE}"
	printf '%s\n' 'planned: SETHW mode 6/22 and KISS framing checks'
	exit 0
fi

printf '%s\n' 'skip: NinoTNC optional harness is a placeholder in M1.26'
printf 'device not opened: %s\n' "${NINOTNC_DEVICE}"
printf '%s\n' 'no RF transmit attempted'
