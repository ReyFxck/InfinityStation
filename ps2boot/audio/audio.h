#ifndef PS2_AUDIO_H
#define PS2_AUDIO_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/*
 * Sample rate de saida do snes9x e do audsrv.
 *
 * A APU do SNES (Blargg) gera amostras a 32040 Hz internamente. O
 * audsrv-IOP suporta nativamente (find_upsampler) 11025/22050/32000/
 * 44100/48000 e o SPU2 do PS2 reproduz a 48000 Hz. A pergunta e' que
 * taxa pedimos pro Blargg e pra audsrv.
 *
 * 32000 Hz e' o melhor ponto:
 *   - Resampler do Blargg vai de 32040 -> 32000, ~0.13% de drift,
 *     praticamente identity. Conteudo de frequencia preservado ate
 *     ~16 kHz (Nyquist), suficiente pra cobrir o espectro da APU.
 *   - audsrv usa um upsampler LUT 32000->48000 (custo desprezivel).
 *
 * Antes este define estava em 22050 com o argumento "alivia ~30% do
 * APU mixing". Mas o ganho de CPU vinha as custas de qualidade muito
 * audivel: 22050 corta tudo acima de ~11 kHz (chimbal/agudos). Os
 * PRs de video (#36 zero conv, #37 cleanup, #38 vsync reorder, #39
 * DMA-direct) liberaram folga de EE suficiente pra absorver os ~30%
 * extras de mixing.
 */
#define PS2_CORE_AUDIO_RATE 32000u

int ps2_audio_init_once(void);
void ps2_audio_shutdown(void);
size_t ps2_audio_push_samples(const int16_t *data, size_t frames);
void ps2_audio_pump(void);
void ps2_audio_pause(void);
void ps2_audio_resume(void);

void ps2_audio_set_iop_ready(int ready);

void ps2_audio_get_buffer_status(bool *active, unsigned *occupancy, bool *underrun_likely);
#endif
