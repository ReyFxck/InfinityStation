#include "launcher_browser_internal.h"

#include <stdlib.h>
#include <string.h>

static char to_lower_ascii(char c)
{
    if (c >= 'A' && c <= 'Z')
        return (char)(c - 'A' + 'a');
    return c;
}

int launcher_browser_name_cmp(const char *a, const char *b)
{
    size_t i = 0;

    if (!a)
        a = "";
    if (!b)
        b = "";

    while (a[i] && b[i]) {
        char ca = to_lower_ascii(a[i]);
        char cb = to_lower_ascii(b[i]);

        if (ca < cb)
            return -1;
        if (ca > cb)
            return 1;

        i++;
    }

    if (!a[i] && !b[i])
        return strcmp(a, b);

    return a[i] ? 1 : -1;
}

int launcher_browser_entry_cmp(const launcher_browser_entry_t *a, const launcher_browser_entry_t *b)
{
    if (!a && !b)
        return 0;
    if (!a)
        return 1;
    if (!b)
        return -1;

    if (a->is_dir != b->is_dir)
        return a->is_dir ? -1 : 1;

    return launcher_browser_name_cmp(a->name, b->name);
}

static int qsort_entry_cmp(const void *pa, const void *pb)
{
    const launcher_browser_entry_t *a = (const launcher_browser_entry_t *)pa;
    const launcher_browser_entry_t *b = (const launcher_browser_entry_t *)pb;

    return launcher_browser_entry_cmp(a, b);
}

void launcher_browser_sort_entries(void)
{
    if (!g_entries || g_entry_count <= 1)
        return;

    qsort(g_entries, (size_t)g_entry_count, sizeof(*g_entries), qsort_entry_cmp);
}
