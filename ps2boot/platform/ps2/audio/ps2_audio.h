#ifndef PS2_AUDIO_H
#define PS2_AUDIO_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define PS2_CORE_AUDIO_RATE 32040u

int ps2_audio_init_once(void);
void ps2_audio_shutdown(void);
size_t ps2_audio_push_samples(const int16_t *data, size_t frames);
void ps2_audio_pump(void);
void ps2_audio_pause(void);
void ps2_audio_resume(void);

void ps2_audio_set_iop_ready(int ready);

void ps2_audio_get_buffer_status(bool *active, unsigned *occupancy, bool *underrun_likely);
#endif
