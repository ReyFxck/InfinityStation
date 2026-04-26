#include "browser_internal.h"

#include <strings.h>
#include <stdlib.h>

static int launcher_browser_entry_compare(const void *a, const void *b)
{
    const launcher_browser_entry_t *ea = (const launcher_browser_entry_t *)a;
    const launcher_browser_entry_t *eb = (const launcher_browser_entry_t *)b;

    if (ea->is_dir != eb->is_dir)
        return eb->is_dir - ea->is_dir; /* diretorios primeiro */

    return strcasecmp(ea->name, eb->name);
}

void launcher_browser_sort_entries(void)
{
    launcher_browser_state_t *state = launcher_browser_state_mut();

    if (!state->entries || state->entry_count <= 1)
        return;

    qsort(state->entries,
          (size_t)state->entry_count,
          sizeof(*state->entries),
          launcher_browser_entry_compare);
}
