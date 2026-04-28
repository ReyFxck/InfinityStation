#ifndef PS2_AUDIO_INTERNAL_H
#define PS2_AUDIO_INTERNAL_H

#include "audio.h"

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include "audio_backend.h"

#define QUIET_PS2AUDIO_LOGS 1
#if QUIET_PS2AUDIO_LOGS
#define PS2AUDIO_LOG(...) ((void)0)
#else
#define PS2AUDIO_LOG(...) printf(__VA_ARGS__)
#endif

#define PS2_AUDIO_RATE              PS2_CORE_AUDIO_RATE
#define PS2_AUDIO_CHANNELS          2
#define PS2_AUDIO_BYTES_PER_SAMPLE  2
#define PS2_AUDIO_FRAME_BYTES       (PS2_AUDIO_CHANNELS * PS2_AUDIO_BYTES_PER_SAMPLE)
#define PS2_AUDIO_BACKEND_VOLUME    100
#define PS2_AUDIO_FADE_STEPS       6
#define PS2_AUDIO_FADE_DELAY_US    750
#define AUDIO_UNDERRUN_HYSTERESIS_LOOPS  16

#define SOUND_BLOCK_COUNT 8
#define SOUND_BUFFER_CHUNK          1024
#define SOUND_TOTAL_FRAMES          (SOUND_BLOCK_COUNT * SOUND_BUFFER_CHUNK)
#define SOUND_TOTAL_BYTES           (SOUND_TOTAL_FRAMES * PS2_AUDIO_FRAME_BYTES)

#define BACKEND_FEED_FRAMES         512
#define BACKEND_FEED_BYTES          (BACKEND_FEED_FRAMES * PS2_AUDIO_FRAME_BYTES)
#define BACKEND_QUEUE_TARGET_FRAMES (BACKEND_FEED_FRAMES * 4)
#define BACKEND_QUEUE_TARGET_BYTES  (BACKEND_QUEUE_TARGET_FRAMES * PS2_AUDIO_FRAME_BYTES)
#define BACKEND_RESUME_UNMUTE_FRAMES (BACKEND_FEED_FRAMES * 4)
#define BACKEND_REAL_TARGET_FRAMES  (BACKEND_FEED_FRAMES * 3)
#define BACKEND_REAL_TARGET_BYTES   (BACKEND_REAL_TARGET_FRAMES * PS2_AUDIO_FRAME_BYTES)

/*
 * Threshold de "underrun iminente" reportado pro libretro (frameskip).
 *
 * Em steady state, o sound thread mantem o backend em torno de
 * BACKEND_QUEUE_TARGET_FRAMES (2048 frames = ~64 ms @ 32 kHz). O
 * heuristico antigo flegava underrun quando total < 1536 frames
 * (BACKEND_REAL_TARGET_FRAMES); como o steady state oscila perto desse
 * valor, frameskip auto disparava em quase toda janela retro_run, mesmo
 * sem real risco de underrun. Resultado: FPS observado caia pra ~36 (60
 * * 0.6) em jogos leves apesar do EE estar a 30 % de carga.
 *
 * O underrun real so' acontece quando a fila combinada (ring + backend)
 * fica abaixo de ~1 vsync de audio (~533 frames @ 32 kHz NTSC). Setamos
 * o threshold em 256 frames (~8 ms) -- isto e', metade de um vsync, com
 * margem confortavel pro audio thread reabastecer o backend antes que o
 * SPU2 fique sem amostras. Frameskip auto so' fira quando o EE esta'
 * realmente atras da curva (cena pesada de SA-1, DSP, etc.), nao em
 * jogos onde o EE tem folga sobrando depois das otimizacoes de video
 * dos PRs #36-#39.
 */
#define BACKEND_UNDERRUN_REPORT_FRAMES 256u

#define SOUND_THREAD_PRIO           64
#define SOUND_THREAD_STACK          0x2000

#endif
