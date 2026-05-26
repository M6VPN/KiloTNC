/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/platform/audio_stub.h */

#ifndef AUDIO_STUB_H
#define AUDIO_STUB_H

#include <sys/types.h>

#include <stdint.h>

#include "kilotnc_audio.h"

#define AUDIO_STUB_BUFFER_MAX 256

struct audio_stub {
	struct kilotnc_audio audio;
	int16_t rx_buf[AUDIO_STUB_BUFFER_MAX];
	int16_t tx_buf[AUDIO_STUB_BUFFER_MAX];
	size_t rx_len;
	size_t rx_pos;
	size_t tx_len;
	size_t rx_samples_injected;
	size_t rx_samples_read;
	size_t tx_samples_written;
	size_t rx_overflows;
	size_t tx_overflows;
	size_t rx_underflows;
	size_t tx_underflows;
};

enum kilotnc_audio_result audio_stub_inject_rx(struct audio_stub *,
	const int16_t *, size_t);
void audio_stub_init(struct audio_stub *);
const struct kilotnc_audio *audio_stub_audio(struct audio_stub *);
enum kilotnc_audio_result audio_stub_take_tx(struct audio_stub *,
	int16_t *, size_t, size_t *);

#endif
