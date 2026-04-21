#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#include <sifrpc.h>
#include <loadfile.h>
#include <libmc.h>

#include "app_game.h"
#include "frontend_config.h"
#include "app_core_options.h"
#include "rom_loader/rom_loader.h"
#include "common/inf_paths.h"

#include "libretro.h"
#include "ui/launcher/launcher.h"

static void *g_loaded_rom_data = NULL;
static size_t g_loaded_rom_size = 0;
static char g_loaded_rom_name[256];
static char g_loaded_rom_identity_path[INF_PATH_MAX];

static int g_mc_init_attempted = 0;
static int g_mc_initialized = 0;
static int g_mc_type = MC_TYPE_MC;
static int g_mc_port = -1;
static int g_mc_slot = 0;

#define APP_GAME_MC_DIR      "InfinityStation"
#define APP_GAME_MC_SAVE_DIR "InfinityStation/srm"
#define APP_GAME_SAVE_EXT    ".srm"

static int app_game_ext_equals(const char *path, const char *ext)
{
    const char *dot;
    size_t i;

    if (!path || !ext)
        return 0;

    dot = strrchr(path, '.');
    if (!dot)
        return 0;

    for (i = 0; dot[i] && ext[i]; i++) {
        char a = dot[i];
        char b = ext[i];

        if (a >= 'A' && a <= 'Z')
            a = (char)(a - 'A' + 'a');
        if (b >= 'A' && b <= 'Z')
            b = (char)(b - 'A' + 'a');

        if (a != b)
            return 0;
    }

    if (ext[i] != '\0')
        return 0;

    return (dot[i] == '\0' || dot[i] == ';');
}

static int app_game_should_preload(const char *path)
{
    if (!path || !path[0])
        return 0;

#ifdef LOAD_FROM_MEMORY
    return 1;
#else
    if (!strncmp(path, "host:", 5))
        return 1;

    return 0;
#endif
}

static void app_game_build_identity_path(const char *selected_path,
                                         const char *loaded_name,
                                         char *out,
                                         size_t out_size)
{
    const char *a;
    const char *b;
    const char *slash;
    size_t dir_len;

    if (!out || out_size == 0)
        return;

    out[0] = '\0';

    if (!selected_path || !selected_path[0])
        return;

    if (!app_game_ext_equals(selected_path, ".zip") || !loaded_name || !loaded_name[0]) {
        snprintf(out, out_size, "%s", selected_path);
        return;
    }

    a = strrchr(selected_path, '/');
    b = strrchr(selected_path, '\\');
    slash = a;
    if (!slash || (b && b > slash))
        slash = b;

    if (!slash) {
        snprintf(out, out_size, "%s", loaded_name);
        return;
    }

    dir_len = (size_t)(slash - selected_path + 1);
    if (dir_len >= out_size) {
        snprintf(out, out_size, "%s", loaded_name);
        return;
    }

    memcpy(out, selected_path, dir_len);
    out[dir_len] = '\0';
    snprintf(out + dir_len, out_size - dir_len, "%s", loaded_name);
}

const char *app_game_loaded_identity_path(void)
{
    return g_loaded_rom_identity_path[0] ? g_loaded_rom_identity_path : NULL;
}

static int app_game_is_path_sep(char c)
{
    return c == '/' || c == '\\' || c == ':';
}

static void app_game_build_save_stem(const char *identity_path,
                                     char *out,
                                     size_t out_size)
{
    const char *base;
    const char *p;
    const char *end;
    size_t len = 0;

    if (!out || out_size == 0)
        return;

    out[0] = '\0';

    if (!identity_path || !identity_path[0]) {
        snprintf(out, out_size, "save");
        return;
    }

    base = identity_path;
    for (p = identity_path; *p; p++) {
        if (app_game_is_path_sep(*p))
            base = p + 1;
    }

    end = base + strlen(base);

    for (p = base; *p; p++) {
        if (*p == ';') {
            end = p;
            break;
        }
    }

    for (p = end; p > base; ) {
        p--;
        if (*p == '.') {
            end = p;
            break;
        }
    }

    while (base < end && len + 1 < out_size) {
        char c = *base++;

        if (c == '<' || c == '>' || c == '"' || c == '|' ||
            c == '?' || c == '*' || c == ';' || c == '/' || c == '\\' || c == ':')
            c = '_';

        out[len++] = c;
    }

    out[len] = '\0';

    if (!out[0])
        snprintf(out, out_size, "save");
}

static void app_game_build_mc_save_path(char *out, size_t out_size)
{
    char stem[256];

    if (!out || out_size == 0)
        return;

    out[0] = '\0';

    if (!g_loaded_rom_identity_path[0])
        return;

    app_game_build_save_stem(g_loaded_rom_identity_path, stem, sizeof(stem));
    if (!stem[0])
        return;

    snprintf(out, out_size, APP_GAME_MC_SAVE_DIR "/%s" APP_GAME_SAVE_EXT, stem);
}

static int app_game_mc_wait(const char *tag)
{
    int cmd = 0;
    int result = 0;
    int sync_res = mcSync(0, &cmd, &result);

    if (sync_res < 0) {
        printf("[DBG] %s: mcSync failed sync_res=%d\n", tag, sync_res);
        fflush(stdout);
        return sync_res;
    }

    return result;
}

static void app_game_mc_load_stack(void)
{
    int ret_sio2man;
    int ret_mcman;
    int ret_mcserv;
    int ret_xmcman;
    int ret_xmcserv;

    ret_sio2man = SifLoadModule("rom0:SIO2MAN", 0, NULL);
    ret_mcman   = SifLoadModule("rom0:MCMAN", 0, NULL);
    ret_mcserv  = SifLoadModule("rom0:MCSERV", 0, NULL);
    ret_xmcman  = SifLoadModule("rom0:XMCMAN", 0, NULL);
    ret_xmcserv = SifLoadModule("rom0:XMCSERV", 0, NULL);

    printf("[DBG] app_game_mc_load_stack: SIO2MAN=%d MCMAN=%d MCSERV=%d XMCMAN=%d XMCSERV=%d\n",
           ret_sio2man, ret_mcman, ret_mcserv, ret_xmcman, ret_xmcserv);
    fflush(stdout);
}

static int app_game_mc_try_init_type(int type, const char *tag)
{
    int init_res;

    mcReset();
    init_res = mcInit(type);

    printf("[DBG] app_game_mc_try_init_type(%s): type=%d res=%d\n",
           tag ? tag : "", type, init_res);
    fflush(stdout);

    if (init_res < 0)
        return 0;

    g_mc_type = type;
    g_mc_initialized = 1;
    return 1;
}

static int app_game_mc_probe_port(int port, int slot)
{
    int submit_res;
    int sync_state;
    int cmd = 0;
    int result = 0;
    int type = 0;
    int free_clusters = 0;
    int format = 0;
    int formatted = 0;

    submit_res = mcGetInfo(port, slot, &type, &free_clusters, &format);
    if (submit_res < 0) {
        printf("[DBG] app_game_mc_probe_port: mcGetInfo submit failed port=%d slot=%d res=%d\n",
               port, slot, submit_res);
        fflush(stdout);
        return 0;
    }

    sync_state = mcSync(0, &cmd, &result);
    if (sync_state < 0) {
        printf("[DBG] app_game_mc_probe_port: mcSync failed port=%d slot=%d sync=%d\n",
               port, slot, sync_state);
        fflush(stdout);
        return 0;
    }

    formatted = (result == 0 || result == -1);
    if (!formatted && format > 0)
        formatted = 1;

    printf("[DBG] app_game_mc_probe_port: port=%d slot=%d type=%d free=%d format=%d cmd=%d result=%d formatted=%d\n",
           port, slot, type, free_clusters, format, cmd, result, formatted);
    fflush(stdout);

    if (type <= 0)
        return 0;
    if (result < -2)
        return 0;
    if (!formatted)
        return 0;

    g_mc_port = port;
    g_mc_slot = slot;
    return 1;
}

void app_game_init_storage(void)
{
    if (g_mc_initialized && g_mc_port >= 0)
        return;

    g_mc_init_attempted = 1;
    g_mc_initialized = 0;
    g_mc_port = -1;
    g_mc_slot = 0;
    g_mc_type = MC_TYPE_MC;

    /* Try the current environment first. */
    if (!app_game_mc_try_init_type(MC_TYPE_MC, "initial-mc")) {
        if (!app_game_mc_try_init_type(MC_TYPE_XMC, "initial-xmc")) {
            app_game_mc_load_stack();

            if (!app_game_mc_try_init_type(MC_TYPE_MC, "after-load-mc")) {
                if (!app_game_mc_try_init_type(MC_TYPE_XMC, "after-load-xmc")) {
                    printf("[DBG] app_game_init_storage: all mcInit attempts failed\n");
                    fflush(stdout);
                    g_mc_init_attempted = 0;
                    return;
                }
            }
        }
    }

    if (app_game_mc_probe_port(0, 0) || app_game_mc_probe_port(1, 0)) {
        printf("[DBG] app_game_init_storage: ready port=%d slot=%d type=%d\n",
               g_mc_port, g_mc_slot, g_mc_type);
        fflush(stdout);
        return;
    }

    /* Keep the library initialized, but allow a later retry if no usable card was found yet. */
    printf("[DBG] app_game_init_storage: no usable formatted memory card found\n");
    fflush(stdout);
    g_mc_init_attempted = 0;
}

static void app_game_mc_ensure_dirs(int port)
{
    int call_res;

    if (!g_mc_initialized)
        return;

    call_res = mcMkDir(port, 0, APP_GAME_MC_DIR);
    if (call_res >= 0)
        (void)app_game_mc_wait("mcMkDir InfinityStation");

    call_res = mcMkDir(port, 0, APP_GAME_MC_SAVE_DIR);
    if (call_res >= 0)
        (void)app_game_mc_wait("mcMkDir InfinityStation/srm");
}

int app_game_sram_autoload(void)
{
    char mc_path[INF_PATH_MAX];
    void *save_data;
    size_t save_size;
    int port;

    if (!g_mc_init_attempted)
        app_game_init_storage();

    if (!g_loaded_rom_identity_path[0]) {
        printf("[DBG] app_game_sram_autoload: no loaded identity path\n");
        fflush(stdout);
        return 0;
    }

    if (!g_mc_initialized) {
        printf("[DBG] app_game_sram_autoload: mc not initialized\n");
        fflush(stdout);
        return 0;
    }

    save_size = retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);
    save_data = retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);

    if (!save_data || save_size == 0) {
        printf("[DBG] app_game_sram_autoload: core exposed no SRAM\n");
        fflush(stdout);
        return 0;
    }

    app_game_build_mc_save_path(mc_path, sizeof(mc_path));
    if (!mc_path[0]) {
        printf("[DBG] app_game_sram_autoload: failed to build mc path\n");
        fflush(stdout);
        return 0;
    }

    memset(save_data, 0, save_size);

    for (port = 0; port <= 1; port++) {
        int submit_res;
        int fd;
        int rd;

        submit_res = mcOpen(port, 0, mc_path, O_RDONLY);
        if (submit_res < 0)
            continue;

        fd = app_game_mc_wait("mcOpen load");
        if (fd < 0)
            continue;

        submit_res = mcRead(fd, save_data, (int)save_size);
        if (submit_res < 0) {
            (void)mcClose(fd);
            (void)app_game_mc_wait("mcClose load");
            continue;
        }

        rd = app_game_mc_wait("mcRead");
        (void)mcClose(fd);
        (void)app_game_mc_wait("mcClose load");

        if (rd < 0)
            continue;

        if ((size_t)rd < save_size)
            memset((unsigned char *)save_data + rd, 0, save_size - (size_t)rd);

        printf("[DBG] app_game_sram_autoload: loaded port=%d path='%s' bytes=%d\n",
               port, mc_path, rd);
        fflush(stdout);
        return 1;
    }

    printf("[DBG] app_game_sram_autoload: save not found path='%s'\n", mc_path);
    fflush(stdout);
    return 0;
}

int app_game_sram_autosave(void)
{
    char mc_path[INF_PATH_MAX];
    void *save_data;
    size_t save_size;
    int port;

    if (!g_mc_init_attempted)
        app_game_init_storage();

    if (!g_loaded_rom_identity_path[0]) {
        printf("[DBG] app_game_sram_autosave: no loaded identity path\n");
        fflush(stdout);
        return 0;
    }

    if (!g_mc_initialized) {
        printf("[DBG] app_game_sram_autosave: mc not initialized\n");
        fflush(stdout);
        return 0;
    }

    save_size = retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);
    save_data = retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);

    if (!save_data || save_size == 0) {
        printf("[DBG] app_game_sram_autosave: core exposed no SRAM\n");
        fflush(stdout);
        return 0;
    }

    app_game_build_mc_save_path(mc_path, sizeof(mc_path));
    if (!mc_path[0]) {
        printf("[DBG] app_game_sram_autosave: failed to build mc path\n");
        fflush(stdout);
        return 0;
    }

    for (port = 0; port <= 1; port++) {
        int submit_res;
        int fd;
        int wr;

        app_game_mc_ensure_dirs(port);

        submit_res = mcOpen(port, 0, mc_path, O_WRONLY | O_CREAT | O_TRUNC);
        if (submit_res < 0)
            continue;

        fd = app_game_mc_wait("mcOpen save");
        if (fd < 0)
            continue;

        submit_res = mcWrite(fd, save_data, (int)save_size);
        if (submit_res < 0) {
            (void)mcClose(fd);
            (void)app_game_mc_wait("mcClose save");
            continue;
        }

        wr = app_game_mc_wait("mcWrite");
        if (wr >= 0) {
            submit_res = mcFlush(fd);
            if (submit_res >= 0)
                (void)app_game_mc_wait("mcFlush");
        }

        (void)mcClose(fd);
        (void)app_game_mc_wait("mcClose save");

        if (wr == (int)save_size) {
            printf("[DBG] app_game_sram_autosave: saved port=%d path='%s' bytes=%d\n",
                   port, mc_path, wr);
            fflush(stdout);
            return 1;
        }
    }

    printf("[DBG] app_game_sram_autosave: failed path='%s'\n", mc_path);
    fflush(stdout);
    return 0;
}

void app_game_unload_loaded(void)
{
    if (g_loaded_rom_data) {
        rom_loader_free(&g_loaded_rom_data, &g_loaded_rom_size);
        g_loaded_rom_data = NULL;
        g_loaded_rom_size = 0;
    }

    g_loaded_rom_name[0] = '\0';
    g_loaded_rom_identity_path[0] = '\0';
}

int app_game_load_selected(void)
{
    struct retro_game_info game;
    const frontend_config_t *cfg = frontend_config_get();
    const char *path = launcher_selected_path();
    int preload = 0;
    const char *load_mode = "path";

    memset(&game, 0, sizeof(game));

    printf("[DBG] app_game_load_selected: selected_path='%s'\n",
           (path && path[0]) ? path : "");
    fflush(stdout);

    if (!(path && path[0])) {
        printf("[DBG] nenhuma ROM selecionada\n");
        fflush(stdout);
        return 0;
    }

    app_game_unload_loaded();

    preload = app_game_should_preload(path);

    if (preload) {
        if (!rom_loader_load(path,
                             &g_loaded_rom_data,
                             &g_loaded_rom_size,
                             g_loaded_rom_name,
                             sizeof(g_loaded_rom_name))) {
            printf("[DBG] rom_loader_load() FALHOU: path='%s'\n", path);
            fflush(stdout);
            return 0;
        }

        app_game_build_identity_path(path,
                                     g_loaded_rom_name,
                                     g_loaded_rom_identity_path,
                                     sizeof(g_loaded_rom_identity_path));

        game.path = g_loaded_rom_identity_path[0] ? g_loaded_rom_identity_path : path;
        game.data = g_loaded_rom_data;
        game.size = g_loaded_rom_size;
        load_mode = app_game_ext_equals(path, ".zip") ? "zip-preload" : "preload";
    } else {
        snprintf(g_loaded_rom_identity_path,
                 sizeof(g_loaded_rom_identity_path),
                 "%s",
                 path);
        g_loaded_rom_name[0] = '\0';
        game.path = path;
        game.data = NULL;
        game.size = 0;
        load_mode = "path";
    }

    game.meta = NULL;

    printf("[DBG] app_game_load_selected mode=%s raw_path='%s'\n",
           load_mode, path);
    printf("[DBG] retro_game_info.path='%s'\n",
           game.path ? game.path : "");
    printf("[DBG] retro_game_info.size=%u\n", (unsigned)game.size);
    fflush(stdout);

    if (retro_load_game(&game)) {
        app_core_apply_runtime_options(cfg->game_reduce_slowdown,
                                       cfg->game_reduce_flicker,
                                       cfg->game_frameskip_mode,
                                       cfg->game_frameskip_threshold);
        app_game_sram_autoload();
        printf("[DBG] retro_load_game() OK\n");
        fflush(stdout);

#ifdef LOAD_FROM_MEMORY
        if (g_loaded_rom_data) {
            rom_loader_free(&g_loaded_rom_data, &g_loaded_rom_size);
            g_loaded_rom_data = NULL;
            g_loaded_rom_size = 0;
            g_loaded_rom_name[0] = '\0';
        }
#endif
        return 1;
    }

    printf("[DBG] retro_load_game() FALHOU: raw_path='%s' final_path='%s' size=%u\n",
           path,
           game.path ? game.path : "",
           (unsigned)game.size);
    fflush(stdout);

    app_game_unload_loaded();
    return 0;
}
