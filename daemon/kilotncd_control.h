/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/daemon/kilotncd_control.h */

#ifndef KILOTNCD_CONTROL_H
#define KILOTNCD_CONTROL_H

#include <sys/types.h>

#include "tnc1200.h"
#include "tnc_diag.h"

#define KILOTNCD_CONTROL_COMMAND_MAX	128U
#define KILOTNCD_CONTROL_RESPONSE_MAX	2048U

enum kilotncd_control_result {
	KILOTNCD_CONTROL_OK = 0,
	KILOTNCD_CONTROL_ERR_ARG,
	KILOTNCD_CONTROL_ERR_SMALL,
	KILOTNCD_CONTROL_ERR_UNKNOWN,
	KILOTNCD_CONTROL_ERR_BAD_VALUE,
	KILOTNCD_CONTROL_ERR_UNSUPPORTED
};

struct kilotncd_control_context {
	struct tnc1200 *tnc;
	struct tnc_diag *diag;
};

enum kilotncd_control_result kilotncd_control_exec(
	struct kilotncd_control_context *, const char *, char *, size_t,
	size_t *);

#endif
