/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/daemon/kilotncd_profile.h */

#ifndef KILOTNCD_PROFILE_H
#define KILOTNCD_PROFILE_H

#include <sys/types.h>

#define KILOTNCD_PROFILE_ERROR_MAX	160U

struct kilotncd_config;

enum kilotncd_profile {
	KILOTNCD_PROFILE_FILE_TX = 0,
	KILOTNCD_PROFILE_FILE_RX,
	KILOTNCD_PROFILE_FILE_LOOPBACK,
	KILOTNCD_PROFILE_STDIO_TX,
	KILOTNCD_PROFILE_STDIO_RX,
	KILOTNCD_PROFILE_TCP_KISS_ONCE,
	KILOTNCD_PROFILE_UNIX_KISS_ONCE,
	KILOTNCD_PROFILE_PTY_KISS_ONCE,
	KILOTNCD_PROFILE_STATUS,
	KILOTNCD_PROFILE_UNSET
};

enum kilotncd_profile_result {
	KILOTNCD_PROFILE_OK = 0,
	KILOTNCD_PROFILE_ERR_ARG,
	KILOTNCD_PROFILE_ERR_CONFLICT,
	KILOTNCD_PROFILE_ERR_MISSING,
	KILOTNCD_PROFILE_ERR_UNSUPPORTED,
	KILOTNCD_PROFILE_ERR_UNSAFE
};

enum kilotncd_profile_result kilotncd_profile_apply_defaults(
	struct kilotncd_config *);
enum kilotncd_profile_result kilotncd_profile_error(
	enum kilotncd_profile_result, enum kilotncd_profile,
	char *, size_t);
enum kilotncd_profile_result kilotncd_profile_format(
	enum kilotncd_profile, const char **);
enum kilotncd_profile_result kilotncd_profile_infer(
	const struct kilotncd_config *, enum kilotncd_profile *);
enum kilotncd_profile_result kilotncd_profile_parse(const char *,
	enum kilotncd_profile *);
enum kilotncd_profile_result kilotncd_profile_validate(
	const struct kilotncd_config *);

#endif
