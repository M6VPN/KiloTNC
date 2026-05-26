/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/include/tnc_mode.h */

#ifndef TNC_MODE_H
#define TNC_MODE_H

#include <sys/types.h>

#include <stdint.h>

#define TNC_MODE_NINO_NONE		255U
#define TNC_MODE_NINO_SET_FROM_KISS	15U

enum tnc_mode_result {
	TNC_MODE_OK = 0,
	TNC_MODE_ERR_ARG,
	TNC_MODE_ERR_RANGE,
	TNC_MODE_ERR_UNSUPPORTED,
	TNC_MODE_ERR_SMALL
};

enum tnc_mode_id {
	TNC_MODE_9600_GFSK_AX25 = 0,
	TNC_MODE_19200_4FSK_IL2PC,
	TNC_MODE_9600_GFSK_IL2PC,
	TNC_MODE_9600_4FSK_IL2PC,
	TNC_MODE_4800_GFSK_IL2PC,
	TNC_MODE_3600_QPSK_IL2PC,
	TNC_MODE_1200_AFSK_AX25,
	TNC_MODE_1200_AFSK_IL2PC,
	TNC_MODE_300_BPSK_IL2PC,
	TNC_MODE_600_QPSK_IL2PC,
	TNC_MODE_1200_BPSK_IL2PC,
	TNC_MODE_2400_QPSK_IL2PC,
	TNC_MODE_300_AFSK_AX25,
	TNC_MODE_300_AFSK_IL2P,
	TNC_MODE_300_AFSK_IL2PC,
	TNC_MODE_SET_FROM_KISS,
	TNC_MODE_UNSUPPORTED
};

enum tnc_mode_support {
	TNC_MODE_SUPPORT_IMPLEMENTED = 0,
	TNC_MODE_SUPPORT_PLANNED,
	TNC_MODE_SUPPORT_RESEARCH,
	TNC_MODE_SUPPORT_UNSUPPORTED
};

struct tnc_mode_desc {
	enum tnc_mode_id id;
	enum tnc_mode_support support;
	uint8_t nino_switch_mode;
	uint8_t nino_brd_swch_mod;
	uint8_t nino_sethw_persistent;
	uint8_t nino_sethw_temporary;
	uint32_t baud;
	uint32_t bps;
	const char *name;
	const char *modulation;
	const char *protocol;
	const char *usage;
};

enum tnc_mode_result tnc_mode_default(enum tnc_mode_id *);
enum tnc_mode_result tnc_mode_from_nino_sethw(uint8_t,
	enum tnc_mode_id *, int *);
enum tnc_mode_result tnc_mode_from_nino_switch(uint8_t,
	enum tnc_mode_id *);
enum tnc_mode_result tnc_mode_get(enum tnc_mode_id,
	const struct tnc_mode_desc **);
enum tnc_mode_result tnc_mode_name(enum tnc_mode_id, char *, size_t,
	size_t *);
enum tnc_mode_result tnc_mode_parse_option(const char *,
	enum tnc_mode_id *, int *);
enum tnc_mode_result tnc_mode_to_nino_sethw(enum tnc_mode_id, int,
	uint8_t *);

#endif
