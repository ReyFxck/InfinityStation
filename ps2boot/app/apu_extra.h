#ifndef APP_APU_EXTRA_H
#define APP_APU_EXTRA_H

#include <stdbool.h>

/*
 * Prototipos de funcoes definidas no nucleo Blargg APU (apu_blargg.c)
 * que nao tem declaracao no header upstream (apu_blargg.h). Mantemos
 * eles aqui para nao patchar codigo de terceiros e ainda satisfazer
 * o '-Wmissing-prototypes' que aplicamos a libretro.c.
 */
void S9xSetSoundMute(bool mute);

#endif
