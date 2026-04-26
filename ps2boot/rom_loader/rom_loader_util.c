#include "rom_loader_util.h"

#include <string.h>

char rom_loader_util_to_lower_ascii(char c)
{
    if (c >= 'A' && c <= 'Z')
        return (char)(c - 'A' + 'a');
    return c;
}

int rom_loader_util_is_cdrom_path(const char *path)
{
    return path && !strncmp(path, "cdrom0:", 7);
}

int rom_loader_util_ext_equals(const char *path, const char *ext)
{
    const char *dot;
    size_t i;

    if (!path || !ext)
        return 0;

    dot = strrchr(path, '.');
    if (!dot)
        return 0;

    for (i = 0; dot[i] && ext[i]; i++) {
        if (rom_loader_util_to_lower_ascii(dot[i]) !=
            rom_loader_util_to_lower_ascii(ext[i]))
            return 0;
    }

    if (ext[i] != '\0')
        return 0;

    if (dot[i] == '\0')
        return 1;

    /* Accept ISO9660 file-version suffix: ".SFC;1" / ".ZIP;1". */
    if (dot[i] == ';')
        return 1;

    return 0;
}

const char *rom_loader_util_base_name_only(const char *path)
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
