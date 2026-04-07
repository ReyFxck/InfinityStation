#include "rom_zip.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
