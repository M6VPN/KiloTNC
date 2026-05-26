/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/firmware/include/afsk1200.h */

#ifndef AFSK1200_H
#define AFSK1200_H

#include <sys/types.h>

#include <stdint.h>

#include "kilotnc_limits.h"

#define AFSK1200_SAMPLE_RATE		48000U
#define AFSK1200_BAUD			1200U
#define AFSK1200_SAMPLES_PER_BIT	40U
#define AFSK1200_MARK_HZ		1200U
#define AFSK1200_SPACE_HZ		2200U
#define AFSK1200_PCM_AMPLITUDE		12000
#define AFSK1200_MAX_TEST_BITS \
	(((KILOTNC_AX25_MAX_FRAME * 8U) * 7U) / 5U)

enum afsk1200_result {
	AFSK1200_OK = 0,
	AFSK1200_ERR_ARG,
	AFSK1200_ERR_SMALL,
	AFSK1200_ERR_BAD_LEN,
	AFSK1200_ERR_BIT
};

enum afsk1200_result afsk1200_decode_pcm(const int16_t *, size_t,
	uint8_t *, size_t, size_t *);
enum afsk1200_result afsk1200_encode_pcm(const uint8_t *, size_t,
	int16_t *, size_t, size_t *);
enum afsk1200_result afsk1200_nrzi_decode(const uint8_t *, size_t,
	uint8_t *, size_t, size_t *);
enum afsk1200_result afsk1200_nrzi_encode(const uint8_t *, size_t,
	uint8_t *, size_t, size_t *);
enum afsk1200_result afsk1200_samples_for_bits(size_t, size_t *);

#endif
