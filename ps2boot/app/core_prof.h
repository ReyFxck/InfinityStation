#ifndef APP_CORE_PROF_H
#define APP_CORE_PROF_H

/*
 * Linhas de profile do core (preenchidas em retro_run via
 * app_core_prof_commit). Expostas como API publica para um
 * overlay de debug poder ler. Atualmente nao consumidas.
 */
const char *app_core_prof_get_line1(void);
const char *app_core_prof_get_line2(void);
const char *app_core_prof_get_line3(void);
const char *app_core_prof_get_line4(void);

#endif
