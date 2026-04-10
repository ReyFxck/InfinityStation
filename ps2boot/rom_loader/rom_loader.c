#include "rom_loader.h"
#include "rom_zip.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

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

static char to_lower_ascii(char c)
{
    if (c >= 'A' && c <= 'Z')
        return (char)(c - 'A' + 'a');
    return c;
}

static int is_cdrom_path(const char *path)
{
    return path && !strncmp(path, "cdrom0:", 7);
}

static int ext_equals(const char *path, const char *ext)
{
    const char *dot;
    size_t i;

    if (!path || !ext)
        return 0;

    dot = strrchr(path, '.');
    if (!dot)
        return 0;

    for (i = 0; dot[i] && ext[i]; i++) {
        if (to_lower_ascii(dot[i]) != to_lower_ascii(ext[i]))
            return 0;
    }

    if (ext[i] != '\0')
        return 0;

    if (dot[i] == '\0')
        return 1;

    if (dot[i] == ';') /* aceita ISO9660: .SFC;1 / .ZIP;1 */
        return 1;

    return 0;
}

static const char *base_name_only(const char *path)
{
    const char *a;
    const char *b;
    const char *p;

    if (!path)
        return "";

    a = strrchr(path, '/');
    b = strrchr(path, '\\');
    p = a;
    if (!p || (b && b > p))
        p = b;

    return p ? (p + 1) : path;
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

    printf("[DBG] read_disc_file_once: open('%s')\n", path ? path : "");
    fflush(stdout);

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        printf("[DBG] open() falhou para '%s' fd=%d\n", path ? path : "", fd);
        fflush(stdout);
        return 0;
    }

    file_size = lseek(fd, 0, SEEK_END);
    if (file_size <= 0) {
        printf("[DBG] lseek(SEEK_END) retornou %d para '%s'\n", file_size, path ? path : "");
        fflush(stdout);
        close(fd);
        return 0;
    }

    if (lseek(fd, 0, SEEK_SET) < 0) {
        printf("[DBG] lseek(SEEK_SET) falhou para '%s'\n", path ? path : "");
        fflush(stdout);
        close(fd);
        return 0;
    }

    buf = (unsigned char *)rom_loader_acquire_buffer((size_t)file_size);
    if (!buf) {
        printf("[DBG] rom_loader_acquire_buffer(%d) falhou para '%s'\n", file_size, path ? path : "");
        fflush(stdout);
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
            printf("[DBG] read() falhou/encerrou em %d de %d para '%s' got=%d\n",
                   total, file_size, path ? path : "", got);
            fflush(stdout);
            close(fd);
            return 0;
        }

        total += got;
    }

    close(fd);

    *out_data = buf;
    *out_size = (size_t)file_size;

    printf("[DBG] read_disc_file_once OK: size=%u path='%s'\n",
           (unsigned)*out_size, path ? path : "");
    fflush(stdout);
    return 1;
}

static int load_plain_file_stdio(const char *path, void **out_data, size_t *out_size)
{
    FILE *fp;
    long file_size;
    size_t read_bytes;
    void *buf;

    printf("[DBG] load_plain_file_stdio: fopen('%s')\n", path ? path : "");
    fflush(stdout);

    fp = fopen(path, "rb");
    if (!fp) {
        printf("[DBG] fopen() falhou para '%s'\n", path ? path : "");
        fflush(stdout);
        return 0;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        printf("[DBG] fseek(SEEK_END) falhou para '%s'\n", path ? path : "");
        fflush(stdout);
        fclose(fp);
        return 0;
    }

    file_size = ftell(fp);
    if (file_size <= 0) {
        printf("[DBG] ftell() retornou %ld para '%s'\n", file_size, path ? path : "");
        fflush(stdout);
        fclose(fp);
        return 0;
    }

    if (fseek(fp, 0, SEEK_SET) != 0) {
        printf("[DBG] fseek(SEEK_SET) falhou para '%s'\n", path ? path : "");
        fflush(stdout);
        fclose(fp);
        return 0;
    }

    buf = rom_loader_acquire_buffer((size_t)file_size);
    if (!buf) {
        printf("[DBG] rom_loader_acquire_buffer(%ld) falhou para '%s'\n", file_size, path ? path : "");
        fflush(stdout);
        fclose(fp);
        return 0;
    }

    read_bytes = fread(buf, 1, (size_t)file_size, fp);
    fclose(fp);

    if (read_bytes != (size_t)file_size) {
        printf("[DBG] fread curto: leu=%u esperado=%u path='%s'\n",
               (unsigned)read_bytes, (unsigned)file_size, path ? path : "");
        fflush(stdout);
        return 0;
    }

    *out_data = buf;
    *out_size = (size_t)file_size;

    printf("[DBG] load_plain_file_stdio OK: size=%u path='%s'\n",
           (unsigned)*out_size, path ? path : "");
    fflush(stdout);
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

    printf("[DBG] tentativa host candidate='%s'\n", candidate);
    fflush(stdout);
    return load_plain_file_once(candidate, out_data, out_size);
}

static int load_host_file_with_fallbacks(const char *path, void **out_data, size_t *out_size)
{
    char rel[512];
    char base[256];
    char c1[576], c2[576], c3[576], c4[576], c5[576], c6[576];
    const char *rp;

    rp = host_rel_path(path);
    if (!rp[0])
        return 0;

    snprintf(rel, sizeof(rel), "%s", rp);
    snprintf(base, sizeof(base), "%s", base_name_only(rp));

    snprintf(c1, sizeof(c1), "host:./%s", rel);
    snprintf(c2, sizeof(c2), "host:%s", rel);
    snprintf(c3, sizeof(c3), "./%s", rel);
    snprintf(c4, sizeof(c4), "%s", rel);
    snprintf(c5, sizeof(c5), "host:./%s", base);
    snprintf(c6, sizeof(c6), "%s", base);

    if (try_candidate(c1, out_data, out_size)) return 1;
    if (strcmp(c2, c1) && try_candidate(c2, out_data, out_size)) return 1;
    if (strcmp(c3, c1) && strcmp(c3, c2) && try_candidate(c3, out_data, out_size)) return 1;
    if (strcmp(c4, c1) && strcmp(c4, c2) && strcmp(c4, c3) && try_candidate(c4, out_data, out_size)) return 1;
    if (strcmp(c5, c1) && strcmp(c5, c2) && strcmp(c5, c3) && strcmp(c5, c4) && try_candidate(c5, out_data, out_size)) return 1;
    if (strcmp(c6, c1) && strcmp(c6, c2) && strcmp(c6, c3) && strcmp(c6, c4) && strcmp(c6, c5) && try_candidate(c6, out_data, out_size)) return 1;

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
        printf("[DBG] rom_loader_load: argumentos invalidos\n");
        fflush(stdout);
        return 0;
    }

    *out_data = NULL;
    *out_size = 0;
    if (out_name && out_name_size > 0)
        out_name[0] = '\0';

    printf("[DBG] rom_loader_load: path='%s'\n", path);
    fflush(stdout);

    if (ext_equals(path, ".zip"))
        return rom_zip_load(path, out_data, out_size, out_name, out_name_size);

    if (!rom_loader_is_supported(path)) {
        printf("[DBG] rom_loader_load: extensao nao suportada '%s'\n", path);
        fflush(stdout);
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
        printf("[DBG] rom_loader_load: out_name='%s'\n", out_name);
        fflush(stdout);
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
