/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/include/kilotnc_config.h */

#ifndef KILOTNC_CONFIG_H
#define KILOTNC_CONFIG_H

#include <sys/types.h>

#include <stdint.h>

#include "kilotnc_usb_descriptors.h"
#include "tnc_mode.h"

#define EMBEDDED_CONFIG_MAGIC		0x4b544346U
#define EMBEDDED_CONFIG_SCHEMA_VERSION	1U
#define EMBEDDED_CONFIG_TXDELAY_MAX	255U
#define EMBEDDED_CONFIG_TXTAIL_MAX	255U
#define EMBEDDED_CONFIG_P_MAX		255U
#define EMBEDDED_CONFIG_SLOTTIME_MAX	255U
#define EMBEDDED_CONFIG_DIAG_LEVEL_MAX	3U
#define EMBEDDED_CONFIG_DEFAULT_MAX_TX_MS 30000U

enum embedded_config_result {
	EMBEDDED_CONFIG_OK = 0,
	EMBEDDED_CONFIG_ERR_ARG,
	EMBEDDED_CONFIG_ERR_RANGE,
	EMBEDDED_CONFIG_ERR_UNSUPPORTED,
	EMBEDDED_CONFIG_ERR_CRC,
	EMBEDDED_CONFIG_ERR_NOT_IMPLEMENTED
};

struct embedded_config {
	uint32_t magic;
	uint16_t schema_version;
	uint16_t payload_len;
	enum tnc_mode_id mode_id;
	enum tnc_mode_id requested_mode_id;
	uint16_t txdelay;
	uint16_t p;
	uint16_t slottime;
	uint16_t txtail;
	uint32_t max_tx_ms;
	uint16_t safety_flags;
	uint8_t mode_temporary;
	uint8_t fullduplex;
	uint8_t usb_desc_profile;
	uint8_t diagnostics_level;
};

enum embedded_config_result embedded_config_apply_kiss_setting(
	struct embedded_config *, uint8_t, uint8_t);
enum embedded_config_result embedded_config_apply_nino_sethw(
	struct embedded_config *, uint8_t);
enum embedded_config_result embedded_config_defaults(struct embedded_config *);
enum embedded_config_result embedded_config_load(struct embedded_config *);
enum embedded_config_result embedded_config_persist(
	const struct embedded_config *);
enum embedded_config_result embedded_config_validate(
	const struct embedded_config *);

#endif
