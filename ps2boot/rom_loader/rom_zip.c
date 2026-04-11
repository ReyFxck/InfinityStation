#include "rom_zip.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>

#include "miniz/miniz.h"

#define DISC_READ_CHUNK (128 * 1024)

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

    if (dot[i] == ';')
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

static int zip_name_is_rom(const char *name)
{
    if (ext_equals(name, ".smc"))
        return 1;
    if (ext_equals(name, ".sfc"))
        return 1;
    if (ext_equals(name, ".swc"))
        return 1;
    if (ext_equals(name, ".fig"))
        return 1;
    return 0;
}

static int read_disc_file_once(const char *path, void **out_data, size_t *out_size)
{
    int fd;
    int file_size;
    unsigned char *buf;
    int total = 0;

    fd = open(path, O_RDONLY);
    if (fd < 0)
        return 0;

    file_size = lseek(fd, 0, SEEK_END);
    if (file_size <= 0) {
        close(fd);
        return 0;
    }

    if (lseek(fd, 0, SEEK_SET) < 0) {
        close(fd);
        return 0;
    }

    buf = (unsigned char *)malloc((size_t)file_size);
    if (!buf) {
        close(fd);
        return 0;
    }

    while (total < file_size) {
        int want = file_size - total;
        int got;

        if (want > DISC_READ_CHUNK)
            want = DISC_READ_CHUNK;

        got = read(fd, buf + total, want);
        if (got <= 0) {
            free(buf);
            close(fd);
            return 0;
        }

        total += got;
    }

    close(fd);

    *out_data = buf;
    *out_size = (size_t)file_size;
    return 1;
}

static int read_stdio_file_once(const char *path, void **out_data, size_t *out_size)
{
    FILE *fp;
    long file_size;
    size_t read_bytes;
    void *buf;

    fp = fopen(path, "rb");
    if (!fp)
        return 0;

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return 0;
    }

    file_size = ftell(fp);
    if (file_size <= 0) {
        fclose(fp);
        return 0;
    }

    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return 0;
    }

    buf = malloc((size_t)file_size);
    if (!buf) {
        fclose(fp);
        return 0;
    }

    read_bytes = fread(buf, 1, (size_t)file_size, fp);
    fclose(fp);

    if (read_bytes != (size_t)file_size) {
        free(buf);
        return 0;
    }

    *out_data = buf;
    *out_size = (size_t)file_size;
    return 1;
}

static int read_path_fully(const char *path, void **out_data, size_t *out_size)
{
    if (is_cdrom_path(path))
        return read_disc_file_once(path, out_data, out_size);
    return read_stdio_file_once(path, out_data, out_size);
}

static int zip_reader_init_path(mz_zip_archive *zip, const char *zip_path,
                                void **zip_data, size_t *zip_size)
{
    if (!zip || !zip_path || !zip_data || !zip_size)
        return 0;

    *zip_data = NULL;
    *zip_size = 0;

#ifndef MINIZ_NO_STDIO
    if (!is_cdrom_path(zip_path)) {
        if (mz_zip_reader_init_file(zip, zip_path, 0))
            return 1;
    }
#endif

    if (!read_path_fully(zip_path, zip_data, zip_size))
        return 0;

    if (!mz_zip_reader_init_mem(zip, *zip_data, *zip_size, 0)) {
        free(*zip_data);
        *zip_data = NULL;
        *zip_size = 0;
        return 0;
    }

    return 1;
}

static size_t path_dir_prefix_len(const char *path)
{
    const char *a;
    const char *b;
    const char *p;
    const char *colon;

    if (!path)
        return 0;

    a = strrchr(path, '/');
    b = strrchr(path, '\\');
    p = a;

    if (!p || (b && b > p))
        p = b;

    if (p)
        return (size_t)(p - path + 1);

    colon = strchr(path, ':');
    if (colon)
        return (size_t)(colon - path + 1);

    return 0;
}

static void sanitize_file_component(const char *src, char *dst, size_t dst_size)
{
    size_t n = 0;
    size_t i;

    if (!dst || dst_size == 0)
        return;

    if (!src)
        src = "";

    for (i = 0; src[i] && n + 1 < dst_size; i++) {
        char c = src[i];

        if (c == '/' || c == '\\' || c == ':' ||
            c == '*' || c == '?' || c == '"' ||
            c == '<' || c == '>' || c == '|')
            c = '_';

        dst[n++] = c;
    }

    dst[n] = '\0';

    if (dst[0] == '\0') {
        strncpy(dst, "rom.sfc", dst_size - 1);
        dst[dst_size - 1] = '\0';
    }
}

static int build_temp_output_path(const char *zip_path,
                                  const char *rom_name,
                                  char *out_path,
                                  size_t out_path_size)
{
    size_t dir_len;
    size_t pos = 0;
    int need_slash = 0;
    char safe_name[256];
    int written;

    if (!zip_path || !out_path || out_path_size == 0)
        return 0;

    sanitize_file_component(base_name_only(rom_name), safe_name, sizeof(safe_name));
    dir_len = path_dir_prefix_len(zip_path);

    if (dir_len > 0) {
        if (dir_len >= out_path_size)
            return 0;

        memcpy(out_path, zip_path, dir_len);
        pos = dir_len;
        out_path[pos] = '\0';

        if (out_path[pos - 1] != '/' && out_path[pos - 1] != '\\')
            need_slash = 1;
    }

    written = snprintf(out_path + pos,
                       out_path_size - pos,
                       "%s_is_tmp_%s",
                       need_slash ? "/" : "",
                       safe_name);

    if (written <= 0 || (size_t)written >= (out_path_size - pos))
        return 0;

    return 1;
}

typedef struct zip_extract_file_ctx_
{
    FILE *fp;
    mz_uint64 expected_ofs;
    int failed;
} zip_extract_file_ctx_t;

static size_t zip_extract_write_cb(void *opaque,
                                   mz_uint64 file_ofs,
                                   const void *pBuf,
                                   size_t n)
{
    zip_extract_file_ctx_t *ctx = (zip_extract_file_ctx_t *)opaque;
    size_t written;

    if (!ctx || !ctx->fp || ctx->failed)
        return 0;

    if (file_ofs > (mz_uint64)LONG_MAX) {
        ctx->failed = 1;
        return 0;
    }

    if (ctx->expected_ofs != file_ofs) {
        if (fseek(ctx->fp, (long)file_ofs, SEEK_SET) != 0) {
            ctx->failed = 1;
            return 0;
        }
    }

    written = fwrite(pBuf, 1, n, ctx->fp);
    if (written != n)
        ctx->failed = 1;

    ctx->expected_ofs = file_ofs + written;
    return written;
}

static int extract_current_file_to_path(mz_zip_archive *zip,
                                        mz_uint file_index,
                                        const char *dst_path)
{
    FILE *fp;
    mz_bool ok;
    zip_extract_file_ctx_t ctx;

    if (!zip || !dst_path || !dst_path[0])
        return 0;

    remove(dst_path);

    fp = fopen(dst_path, "wb");
    if (!fp)
        return 0;

    memset(&ctx, 0, sizeof(ctx));
    ctx.fp = fp;

    ok = mz_zip_reader_extract_to_callback(zip, file_index,
                                           zip_extract_write_cb,
                                           &ctx,
                                           0);

    if (fflush(fp) != 0)
        ok = MZ_FALSE;

    if (fclose(fp) != 0)
        ok = MZ_FALSE;

    if (!ok || ctx.failed) {
        remove(dst_path);
        return 0;
    }

    return 1;
}

int rom_zip_load(const char *zip_path, void **out_data, size_t *out_size,
                 char *out_name, size_t out_name_size)
{
    mz_zip_archive zip;
    mz_uint i;
    mz_uint file_count;
    void *zip_data = NULL;
    size_t zip_size = 0;
    int used_mem_reader = 0;

    if (!zip_path || !out_data || !out_size)
        return 0;

    *out_data = NULL;
    *out_size = 0;

    if (out_name && out_name_size > 0)
        out_name[0] = '\0';

    memset(&zip, 0, sizeof(zip));

    if (!zip_reader_init_path(&zip, zip_path, &zip_data, &zip_size)) {
        printf("[DBG] rom_zip_load: nao conseguiu abrir zip '%s'\n",
               zip_path ? zip_path : "");
        fflush(stdout);
        return 0;
    }

    used_mem_reader = (zip_data != NULL);
    file_count = mz_zip_reader_get_num_files(&zip);

    for (i = 0; i < file_count; i++) {
        mz_zip_archive_file_stat st;
        void *buf;

        memset(&st, 0, sizeof(st));

        if (!mz_zip_reader_file_stat(&zip, i, &st))
            continue;
        if (mz_zip_reader_is_file_a_directory(&zip, i))
            continue;
        if (!zip_name_is_rom(st.m_filename))
            continue;
        if (!st.m_uncomp_size)
            continue;
        if (st.m_uncomp_size > (mz_uint64)((size_t)-1))
            continue;

        buf = malloc((size_t)st.m_uncomp_size);
        if (!buf)
            break;

        if (!mz_zip_reader_extract_to_mem(&zip, i, buf, (size_t)st.m_uncomp_size, 0)) {
            free(buf);
            continue;
        }

        *out_data = buf;
        *out_size = (size_t)st.m_uncomp_size;

        if (out_name && out_name_size > 0) {
            strncpy(out_name, base_name_only(st.m_filename), out_name_size - 1);
            out_name[out_name_size - 1] = '\0';
        }

        mz_zip_reader_end(&zip);

        if (used_mem_reader)
            free(zip_data);

        printf("[DBG] rom_zip_load OK: rom='%s' size=%u zip='%s'%s\n",
               out_name ? out_name : "",
               (unsigned)*out_size,
               zip_path ? zip_path : "",
               used_mem_reader ? " [mem-reader]" : " [file-reader]");
        fflush(stdout);
        return 1;
    }

    mz_zip_reader_end(&zip);

    if (used_mem_reader)
        free(zip_data);

    printf("[DBG] rom_zip_load: nenhuma ROM valida em '%s'\n",
           zip_path ? zip_path : "");
    fflush(stdout);
    return 0;
}

int rom_zip_extract_to_temp_file(const char *zip_path,
                                 char *out_path,
                                 size_t out_path_size,
                                 char *out_name,
                                 size_t out_name_size)
{
    mz_zip_archive zip;
    mz_uint i;
    mz_uint file_count;
    void *zip_data = NULL;
    size_t zip_size = 0;
    int used_mem_reader = 0;

    if (!zip_path || !zip_path[0] || !out_path || out_path_size == 0)
        return 0;

    out_path[0] = '\0';

    if (out_name && out_name_size > 0)
        out_name[0] = '\0';

    memset(&zip, 0, sizeof(zip));

    if (!zip_reader_init_path(&zip, zip_path, &zip_data, &zip_size)) {
        printf("[DBG] rom_zip_extract_to_temp_file: nao conseguiu abrir zip '%s'\n",
               zip_path ? zip_path : "");
        fflush(stdout);
        return 0;
    }

    used_mem_reader = (zip_data != NULL);
    file_count = mz_zip_reader_get_num_files(&zip);

    for (i = 0; i < file_count; i++) {
        mz_zip_archive_file_stat st;

        memset(&st, 0, sizeof(st));

        if (!mz_zip_reader_file_stat(&zip, i, &st))
            continue;
        if (mz_zip_reader_is_file_a_directory(&zip, i))
            continue;
        if (!zip_name_is_rom(st.m_filename))
            continue;
        if (!st.m_uncomp_size)
            continue;
        if (st.m_uncomp_size > (mz_uint64)((size_t)-1))
            continue;

        if (!build_temp_output_path(zip_path,
                                    base_name_only(st.m_filename),
                                    out_path,
                                    out_path_size))
            continue;

        if (!extract_current_file_to_path(&zip, i, out_path))
            continue;

        if (out_name && out_name_size > 0) {
            strncpy(out_name, base_name_only(st.m_filename), out_name_size - 1);
            out_name[out_name_size - 1] = '\0';
        }

        mz_zip_reader_end(&zip);

        if (used_mem_reader)
            free(zip_data);

        printf("[DBG] rom_zip_extract_to_temp_file OK: rom='%s' zip='%s' temp='%s'%s\n",
               out_name ? out_name : "",
               zip_path ? zip_path : "",
               out_path,
               used_mem_reader ? " [mem-reader]" : " [file-reader]");
        fflush(stdout);
        return 1;
    }

    mz_zip_reader_end(&zip);

    if (used_mem_reader)
        free(zip_data);

    out_path[0] = '\0';

    printf("[DBG] rom_zip_extract_to_temp_file: nenhuma ROM extraida de '%s'\n",
           zip_path ? zip_path : "");
    fflush(stdout);
    return 0;
}
