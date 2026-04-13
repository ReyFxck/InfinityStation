#ifndef PS2_AUDIO_BACKEND_H
#define PS2_AUDIO_BACKEND_H

#include <stdint.h>

int ps2_audio_backend_reset_iop(void);
void ps2_audio_backend_set_iop_ready(int ready);
int ps2_audio_backend_iop_ready(void);

int ps2_backend_init(int rate, int channels, int bits);
int ps2_backend_queued_bytes(void);
int ps2_backend_available_bytes(void);
void ps2_backend_wait_audio(int bytes);
int ps2_backend_queue_audio(const int16_t *data, int bytes);
int ps2_backend_set_volume(int vol);
void ps2_backend_stop_audio(void);
int ps2_backend_reprime_audio(int rate, int bits, int channels);
void ps2_backend_shutdown(void);

#endif
