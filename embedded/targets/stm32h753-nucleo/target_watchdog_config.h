/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/targets/stm32h753-nucleo/target_watchdog_config.h */

#ifndef TARGET_WATCHDOG_CONFIG_STM32H753_NUCLEO_H
#define TARGET_WATCHDOG_CONFIG_STM32H753_NUCLEO_H

/*
 * M2.15 metadata only. Future watchdog setup must stay behind the platform
 * adapter and must not refresh the watchdog unless required tasks report
 * bounded progress.
 */
#define KILOTNC_TARGET_WATCHDOG_FINALIZED		0U
#define KILOTNC_TARGET_IWDG_POLICY_PLANNED		1U
#define KILOTNC_TARGET_WWDG_POLICY_PLANNED		0U
#define KILOTNC_TARGET_WATCHDOG_QUORUM_PLANNED		1U
#define KILOTNC_TARGET_WATCHDOG_SAFE_OFF_REQUIRED	1U
#define KILOTNC_TARGET_WATCHDOG_DIAG_REQUIRED		1U
#define KILOTNC_TARGET_WATCHDOG_ENABLE_AT_BOOT		0U
#define KILOTNC_TARGET_WATCHDOG_REGISTER_VALUES_FINALIZED 0U

#define KILOTNC_TARGET_WATCHDOG_POLICY_PLAN		"IWDG preferred"
#define KILOTNC_TARGET_WATCHDOG_QUORUM_PLAN \
	"main,usb,audio,ptt-control"

#endif
