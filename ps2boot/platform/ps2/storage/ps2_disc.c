#include "ps2_disc.h"

#include <sifrpc.h>
#include <loadfile.h>
#include <libcdvd.h>

static int g_disc_init_done = 0;
static int g_disc_ready = 0;

static int ps2_disc_type_supported(int type)
{
    switch (type) {
        case SCECdPS2CD:
        case SCECdPS2DVD:
        case SCECdDVDV:
            return 1;
        default:
            return 0;
    }
}

int ps2_disc_refresh(void)
{
    int type;

    if (!g_disc_init_done) {
        g_disc_ready = 0;
        return 0;
    }

    type = sceCdGetDiskType();
    g_disc_ready = ps2_disc_type_supported(type);
    return g_disc_ready;
}

int ps2_disc_init_once(void)
{
    if (g_disc_init_done)
        return g_disc_ready;

    g_disc_init_done = 1;

    SifLoadFileInit();

    if (!sceCdInit(SCECdINIT)) {
        g_disc_ready = 0;
        return 0;
    }

    return ps2_disc_refresh();
}

int ps2_disc_ready(void)
{
    return g_disc_ready;
}
