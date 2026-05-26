#!/usr/bin/env bash
# KiloTNC - Developed by M6VPN (M6VPN@tuta.com)
# KiloTNC/embedded/targets/stm32h753-nucleo/check_target_compile.sh

set -eu

cc="${ARM_NONE_EABI_CC:-arm-none-eabi-gcc}"
build_dir="build/embedded-target"
makefile="embedded/targets/stm32h753-nucleo/target_build.mk"

if ! command -v "${cc}" >/dev/null 2>&1; then
	printf '%s\n' 'skip: arm-none-eabi-gcc not found'
	exit 0
fi

mkdir -p "${build_dir}"

printf '%s\n' 'checking stm32h753-nucleo target skeleton objects'
make -f "${makefile}" \
	ARM_NONE_EABI_CC="${cc}" \
	KILOTNC_STM32H753_BUILD_DIR="${build_dir}" \
	embedded-target-object-check
