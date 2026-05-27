/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/targets/stm32h753-nucleo/target_clock.h */

#ifndef TARGET_CLOCK_STM32H753_NUCLEO_H
#define TARGET_CLOCK_STM32H753_NUCLEO_H

/*
 * M2.15 metadata only. Future firmware must verify PLL, RCC, USB clock,
 * timer clock, and audio clock values against RM0433, the STM32H753ZI
 * datasheet, and the selected Nucleo board clock source before use.
 */
#define KILOTNC_TARGET_CONTROL_TICK_MS			10U
#define KILOTNC_TARGET_TIMEBASE_TICK_MS			1U
#define KILOTNC_TARGET_AUDIO_SAMPLE_RATE_HZ		48000U
#define KILOTNC_TARGET_CLOCK_TREE_FINALIZED		0U
#define KILOTNC_TARGET_PLL_VALUES_FINALIZED		0U
#define KILOTNC_TARGET_USB_CLOCK_FINALIZED		0U
#define KILOTNC_TARGET_AUDIO_CLOCK_FINALIZED		0U
#define KILOTNC_TARGET_TIMER_CLOCK_FINALIZED		0U
#define KILOTNC_TARGET_HSE_SOURCE_FINALIZED		0U

#define KILOTNC_TARGET_HSE_SOURCE_PLAN			"TBD"
#define KILOTNC_TARGET_USB_CLOCK_PLAN			"TBD"
#define KILOTNC_TARGET_AUDIO_CLOCK_PLAN			"TBD"

#endif
