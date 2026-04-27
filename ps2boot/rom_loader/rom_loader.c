#include "rom_loader.h"
#include "rom_loader_util.h"
#include "rom_zip.h"
#include "common/inf_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

/* Short aliases for shared helpers to keep call sites compact. */
#define is_cdrom_path  rom_loader_util_is_cdrom_path
#define ext_equals     rom_loader_util_ext_equals
#define base_name_only rom_loader_util_base_name_only

#define DISC_READ_CHUNK (128 * 1024)

static unsigned char *g_rom_loader_buffer = NULL;
static size_t g_rom_loader_buffer_size = 0;

static void *rom_loader_acquire_buffer(size_t size)
{
    unsigned char *new_buf;

    if (size == 0)
        return NULL;

    if (g_rom_loader_buffer && g_rom_loader_buffer_size >= size)
        return g_rom_loader_buffer;

    new_buf = (unsigned char *)realloc(g_rom_loader_buffer, size);
    if (!new_buf)
        return NULL;

    g_rom_loader_buffer = new_buf;
    g_rom_loader_buffer_size = size;
    return g_rom_loader_buffer;
}

static int rom_loader_uses_persistent_buffer(const void *ptr)
{
    return ptr && ptr == g_rom_loader_buffer;
}

static const char *host_rel_path(const char *path)
{
    if (!path || strncmp(path, "host:", 5) != 0)
        return "";

    path += 5;
    while (*path == '/')
        path++;

    return path;
}

static int read_disc_file_once(const char *path, void **out_data, size_t *out_size)
{
    int fd;
    int file_size;
    unsigned char *buf;
    int total;

    total = 0;

    INF_LOG_DBG("read_disc_file_once: open('%s')\n", path ? path : "");

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        INF_LOG_DBG("open() falhou para '%s' fd=%d\n", path ? path : "", fd);
        return 0;
    }

    file_size = lseek(fd, 0, SEEK_END);
    if (file_size <= 0) {
        INF_LOG_DBG("lseek(SEEK_END) retornou %d para '%s'\n", file_size, path ? path : "");
        close(fd);
        return 0;
    }

    if (lseek(fd, 0, SEEK_SET) < 0) {
        INF_LOG_DBG("lseek(SEEK_SET) falhou para '%s'\n", path ? path : "");
        close(fd);
        return 0;
    }

    buf = (unsigned char *)rom_loader_acquire_buffer((size_t)file_size);
    if (!buf) {
        INF_LOG_DBG("rom_loader_acquire_buffer(%d) falhou para '%s'\n", file_size, path ? path : "");
        close(fd);
        return 0;
    }

    while (total < file_size) {
        int want;
        int got;

        want = file_size - total;
        if (want > DISC_READ_CHUNK)
            want = DISC_READ_CHUNK;

        got = read(fd, buf + total, want);
        if (got <= 0) {
            INF_LOG_DBG("read() falhou/encerrou em %d de %d para '%s' got=%d\n",
                   total, file_size, path ? path : "", got);
            close(fd);
            return 0;
        }

        total += got;
    }

    close(fd);

    *out_data = buf;
    *out_size = (size_t)file_size;

    INF_LOG_DBG("read_disc_file_once OK: size=%u path='%s'\n",
           (unsigned)*out_size, path ? path : "");
    return 1;
}

static int load_plain_file_stdio(const char *path, void **out_data, size_t *out_size)
{
    FILE *fp;
    long file_size;
    size_t read_bytes;
    void *buf;

    INF_LOG_DBG("load_plain_file_stdio: fopen('%s')\n", path ? path : "");

    if (!path)
        return 0;

    fp = fopen(path, "rb");
    if (!fp) {
        INF_LOG_DBG("fopen() falhou para '%s'\n", path ? path : "");
        return 0;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        INF_LOG_DBG("fseek(SEEK_END) falhou para '%s'\n", path ? path : "");
        fclose(fp);
        return 0;
    }

    file_size = ftell(fp);
    if (file_size <= 0) {
        INF_LOG_DBG("ftell() retornou %ld para '%s'\n", file_size, path ? path : "");
        fclose(fp);
        return 0;
    }

    if (fseek(fp, 0, SEEK_SET) != 0) {
        INF_LOG_DBG("fseek(SEEK_SET) falhou para '%s'\n", path ? path : "");
        fclose(fp);
        return 0;
    }

    buf = rom_loader_acquire_buffer((size_t)file_size);
    if (!buf) {
        INF_LOG_DBG("rom_loader_acquire_buffer(%ld) falhou para '%s'\n", file_size, path ? path : "");
        fclose(fp);
        return 0;
    }

    read_bytes = fread(buf, 1, (size_t)file_size, fp);
    fclose(fp);

    if (read_bytes != (size_t)file_size) {
        INF_LOG_DBG("fread curto: leu=%u esperado=%u path='%s'\n",
               (unsigned)read_bytes, (unsigned)file_size, path ? path : "");
        return 0;
    }

    *out_data = buf;
    *out_size = (size_t)file_size;

    INF_LOG_DBG("load_plain_file_stdio OK: size=%u path='%s'\n",
           (unsigned)*out_size, path ? path : "");
    return 1;
}

static int load_plain_file_once(const char *path, void **out_data, size_t *out_size)
{
    if (is_cdrom_path(path))
        return read_disc_file_once(path, out_data, out_size);

    return load_plain_file_stdio(path, out_data, out_size);
}

static int try_candidate(const char *candidate, void **out_data, size_t *out_size)
{
    if (!candidate || !candidate[0])
        return 0;

    INF_LOG_DBG("tentativa host candidate='%s'\n", candidate);
    return load_plain_file_once(candidate, out_data, out_size);
}

/* Tabela de candidatos para 'host:' do PS2: (prefixo, fonte).
 * 'fonte' eh 0 = path relativo completo (ex.: 'roms/sub/x.sfc'),
 * 1 = apenas o nome do arquivo (ex.: 'x.sfc'). A ordem importa:
 * tentamos do mais especifico (host:./<rel>) ao mais permissivo
 * (basename direto), parando no primeiro que abrir.
 *
 * Adicionar um candidato novo agora eh acrescentar uma linha aqui
 * (em vez de criar mais uma cascata de strcmp). */
static const struct {
    const char *prefix;
    unsigned    use_base;  /* 0 = rel, 1 = basename */
} g_host_candidate_table[] = {
    { "host:./", 0 },
    { "host:",   0 },
    { "./",      0 },
    { "",        0 },
    { "host:./", 1 },
    { "",        1 },
};

#define HOST_CANDIDATE_COUNT \
    (sizeof(g_host_candidate_table) / sizeof(g_host_candidate_table[0]))

#define HOST_CANDIDATE_BUF_SIZE 576

static int load_host_file_with_fallbacks(const char *path, void **out_data, size_t *out_size)
{
    char rel[512];
    char base[256];
    char tried[HOST_CANDIDATE_COUNT][HOST_CANDIDATE_BUF_SIZE];
    char buf[HOST_CANDIDATE_BUF_SIZE];
    const char *rp;
    size_t i, j;
    unsigned n_tried = 0;

    rp = host_rel_path(path);
    if (!rp[0])
        return 0;

    /* Se o path nao cabe no buffer de trabalho nao podemos confiar
     * em nenhum dos candidatos: cada um produziria uma string
     * truncada que poderia coincidir com um arquivo existente
     * (ou pior, ler outro). Aborta limpo. */
    if (!INF_SNPRINTF_OK(rel, sizeof(rel), "%s", rp))
        return 0;
    if (!INF_SNPRINTF_OK(base, sizeof(base), "%s", base_name_only(rp)))
        return 0;

    for (i = 0; i < HOST_CANDIDATE_COUNT; i++) {
        const char *src = g_host_candidate_table[i].use_base ? base : rel;
        unsigned is_dup = 0;

        if (!INF_SNPRINTF_OK(buf, sizeof(buf), "%s%s",
                             g_host_candidate_table[i].prefix, src))
            continue; /* truncated -> skip silently */

        for (j = 0; j < n_tried; j++) {
            if (!strcmp(buf, tried[j])) {
                is_dup = 1;
                break;
            }
        }
        if (is_dup)
            continue;

        if (try_candidate(buf, out_data, out_size))
            return 1;

        /* Lembra que tentamos esse buffer para evitar duplicatas
         * nas iteracoes seguintes (alguns prefixos coincidem com
         * 'rel'/'base' ja absolutos). */
        memcpy(tried[n_tried], buf, sizeof(tried[n_tried]));
        n_tried++;
    }

    return 0;
}

int rom_loader_is_supported(const char *path)
{
    if (!path || !path[0])
        return 0;

    if (ext_equals(path, ".smc")) return 1;
    if (ext_equals(path, ".sfc")) return 1;
    if (ext_equals(path, ".swc")) return 1;
    if (ext_equals(path, ".fig")) return 1;
    if (ext_equals(path, ".zip")) return 1;

    return 0;
}

int rom_loader_load(const char *path, void **out_data, size_t *out_size,
                    char *out_name, size_t out_name_size)
{
    if (!path || !path[0] || !out_data || !out_size) {
        INF_LOG_DBG("rom_loader_load: argumentos invalidos\n");
        return 0;
    }

    *out_data = NULL;
    *out_size = 0;
    if (out_name && out_name_size > 0)
        out_name[0] = '\0';

    INF_LOG_DBG("rom_loader_load: path='%s'\n", path);

    if (ext_equals(path, ".zip"))
        return rom_zip_load(path, out_data, out_size, out_name, out_name_size);

    if (!rom_loader_is_supported(path)) {
        INF_LOG_DBG("rom_loader_load: extensao nao suportada '%s'\n", path);
        return 0;
    }

    if (!strncmp(path, "host:", 5)) {
        if (!load_host_file_with_fallbacks(path, out_data, out_size))
            return 0;
    } else {
        if (!load_plain_file_once(path, out_data, out_size))
            return 0;
    }

    if (out_name && out_name_size > 0) {
        char *semi;
        strncpy(out_name, base_name_only(path), out_name_size - 1);
        out_name[out_name_size - 1] = '\0';
        semi = strrchr(out_name, ';');
        if (semi)
            *semi = '\0';
        INF_LOG_DBG("rom_loader_load: out_name='%s'\n", out_name);
    }

    return 1;
}

void rom_loader_free(void **data, size_t *size)
{
    if (data && *data) {
        if (rom_loader_uses_persistent_buffer(*data)) {
            free(g_rom_loader_buffer);
            g_rom_loader_buffer = NULL;
            g_rom_loader_buffer_size = 0;
        } else {
            free(*data);
        }
        *data = NULL;
    }

    if (size)
        *size = 0;
}
