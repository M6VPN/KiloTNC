/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/targets/stm32h753-nucleo/target_reset.h */

#ifndef TARGET_RESET_STM32H753_NUCLEO_H
#define TARGET_RESET_STM32H753_NUCLEO_H

/*
 * M2.15 metadata only. Future reset-cause capture must happen early in
 * boot and must not read reset registers until the target hardware adapter
 * is explicitly implemented.
 */
#define KILOTNC_TARGET_RESET_CAUSE_FINALIZED		0U
#define KILOTNC_TARGET_RESET_CAUSE_CAPTURE_PLANNED	1U
#define KILOTNC_TARGET_RESET_CAUSE_DIAG_PLANNED		1U
#define KILOTNC_TARGET_RESET_POWER_ON_PLANNED		1U
#define KILOTNC_TARGET_RESET_SOFTWARE_PLANNED		1U
#define KILOTNC_TARGET_RESET_WATCHDOG_PLANNED		1U
#define KILOTNC_TARGET_RESET_BROWNOUT_PLANNED		1U
#define KILOTNC_TARGET_RESET_UNKNOWN_FALLBACK		1U

#define KILOTNC_TARGET_RESET_REGISTER_PLAN		"TBD"

#endif
