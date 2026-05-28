/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/app/embedded_config.c */

#include <sys/types.h>

#include <stdint.h>
#include <string.h>

#include "embedded_config.h"
#include "kiss.h"

static enum embedded_config_result embedded_config_mode_validate(
	enum tnc_mode_id);

static enum embedded_config_result
embedded_config_mode_validate(enum tnc_mode_id mode)
{
	const struct tnc_mode_desc *desc;

	if (tnc_mode_get(mode, &desc) != TNC_MODE_OK)
		return EMBEDDED_CONFIG_ERR_RANGE;
	if (desc->support != TNC_MODE_SUPPORT_IMPLEMENTED)
		return EMBEDDED_CONFIG_ERR_UNSUPPORTED;

	return EMBEDDED_CONFIG_OK;
}

enum embedded_config_result
embedded_config_apply_kiss_setting(struct embedded_config *config,
	uint8_t command, uint8_t value)
{
	if (config == NULL)
		return EMBEDDED_CONFIG_ERR_ARG;

	switch (command) {
	case KISS_CMD_TXDELAY:
		config->txdelay = value;
		break;
	case KISS_CMD_P:
		config->p = value;
		break;
	case KISS_CMD_SLOTTIME:
		config->slottime = value;
		break;
	case KISS_CMD_TXTAIL:
		config->txtail = value;
		break;
	case KISS_CMD_FULLDUPLEX:
		config->fullduplex = value != 0U;
		break;
	default:
		return EMBEDDED_CONFIG_ERR_UNSUPPORTED;
	}

	return embedded_config_validate(config);
}

enum embedded_config_result
embedded_config_apply_nino_sethw(struct embedded_config *config,
	uint8_t value)
{
	enum embedded_config_result validate_result;
	enum tnc_mode_id requested;
	enum tnc_mode_result mode_result;
	int temporary;

	if (config == NULL)
		return EMBEDDED_CONFIG_ERR_ARG;

	mode_result = tnc_mode_from_nino_sethw(value, &requested, &temporary);
	if (mode_result != TNC_MODE_OK)
		return EMBEDDED_CONFIG_ERR_RANGE;

	config->requested_mode_id = requested;
	config->mode_temporary = temporary != 0;
	validate_result = embedded_config_mode_validate(requested);
	if (validate_result != EMBEDDED_CONFIG_OK)
		return validate_result;

	config->mode_id = requested;
	return embedded_config_validate(config);
}

enum embedded_config_result
embedded_config_defaults(struct embedded_config *config)
{
	enum tnc_mode_id mode;

	if (config == NULL)
		return EMBEDDED_CONFIG_ERR_ARG;
	if (tnc_mode_default(&mode) != TNC_MODE_OK)
		return EMBEDDED_CONFIG_ERR_UNSUPPORTED;

	(void)memset(config, 0, sizeof(*config));
	config->magic = EMBEDDED_CONFIG_MAGIC;
	config->schema_version = EMBEDDED_CONFIG_SCHEMA_VERSION;
	config->payload_len = (uint16_t)sizeof(*config);
	config->mode_id = mode;
	config->requested_mode_id = mode;
	config->txdelay = 50U;
	config->p = 63U;
	config->slottime = 10U;
	config->txtail = 0U;
	config->max_tx_ms = EMBEDDED_CONFIG_DEFAULT_MAX_TX_MS;
	config->usb_desc_profile = KILOTNC_USB_DESC_PROFILE_KISS_ONLY;

	return embedded_config_validate(config);
}

enum embedded_config_result
embedded_config_load(struct embedded_config *config)
{
	if (config == NULL)
		return EMBEDDED_CONFIG_ERR_ARG;

	return EMBEDDED_CONFIG_ERR_NOT_IMPLEMENTED;
}

enum embedded_config_result
embedded_config_persist(const struct embedded_config *config)
{
	if (config == NULL)
		return EMBEDDED_CONFIG_ERR_ARG;

	return EMBEDDED_CONFIG_ERR_NOT_IMPLEMENTED;
}

enum embedded_config_result
embedded_config_validate(const struct embedded_config *config)
{
	const struct tnc_mode_desc *desc;
	enum embedded_config_result mode_result;

	if (config == NULL)
		return EMBEDDED_CONFIG_ERR_ARG;
	if (config->magic != EMBEDDED_CONFIG_MAGIC)
		return EMBEDDED_CONFIG_ERR_RANGE;
	if (config->schema_version != EMBEDDED_CONFIG_SCHEMA_VERSION)
		return EMBEDDED_CONFIG_ERR_RANGE;
	if (config->payload_len != sizeof(*config))
		return EMBEDDED_CONFIG_ERR_RANGE;
	if (config->txdelay > EMBEDDED_CONFIG_TXDELAY_MAX ||
	    config->txtail > EMBEDDED_CONFIG_TXTAIL_MAX ||
	    config->p > EMBEDDED_CONFIG_P_MAX ||
	    config->slottime > EMBEDDED_CONFIG_SLOTTIME_MAX)
		return EMBEDDED_CONFIG_ERR_RANGE;
	if (config->fullduplex > 1U || config->mode_temporary > 1U)
		return EMBEDDED_CONFIG_ERR_RANGE;
	if (config->max_tx_ms == 0U)
		return EMBEDDED_CONFIG_ERR_RANGE;
	if (config->diagnostics_level > EMBEDDED_CONFIG_DIAG_LEVEL_MAX)
		return EMBEDDED_CONFIG_ERR_RANGE;
	if (config->usb_desc_profile !=
	    KILOTNC_USB_DESC_PROFILE_KISS_ONLY &&
	    config->usb_desc_profile !=
	    KILOTNC_USB_DESC_PROFILE_KISS_PLUS_DIAG)
		return EMBEDDED_CONFIG_ERR_RANGE;

	mode_result = embedded_config_mode_validate(config->mode_id);
	if (mode_result != EMBEDDED_CONFIG_OK)
		return mode_result;
	if (tnc_mode_get(config->requested_mode_id, &desc) != TNC_MODE_OK)
		return EMBEDDED_CONFIG_ERR_RANGE;

	return EMBEDDED_CONFIG_OK;
}
