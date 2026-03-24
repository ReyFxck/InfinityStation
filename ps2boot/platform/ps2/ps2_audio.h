#ifndef PS2_AUDIO_H
#define PS2_AUDIO_H

#include <stddef.h>
#include <stdint.h>

int ps2_audio_init_once(void);
void ps2_audio_shutdown(void);
size_t ps2_audio_push_samples(const int16_t *data, size_t frames);

#endif
