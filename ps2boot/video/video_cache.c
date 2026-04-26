#include "video_internal.h"

static int g_last_frame_256_valid = 0;
static unsigned g_last_frame_256_height = 0;
static uint16_t g_last_frame_256[PS2_VIDEO_UPLOAD_256_WIDTH * PS2_VIDEO_TEX_HEIGHT] __attribute__((aligned(64)));

int ps2_video_cache_can_reuse_256(const uint16_t *src, unsigned width, unsigned height)
{
    size_t bytes;
    size_t count;
    unsigned i;

    if (!src || !ps2_video_packets_last_upload_valid() || !g_last_frame_256_valid)
        return 0;

    if (width != PS2_VIDEO_UPLOAD_256_WIDTH)
        return 0;

    if (height != g_last_frame_256_height)
        return 0;

    count = (size_t)width * (size_t)height;
    if (count == 0)
        return 0;

    /*
     * Cheap sample precheck:
     * avoid a full-frame memcmp on active gameplay frames that obviously changed.
     * Keep the final memcmp so static screens still reuse the already uploaded texture safely.
     */
    if (src[0] != g_last_frame_256[0] ||
        src[count - 1u] != g_last_frame_256[count - 1u])
        return 0;

    for (i = 1; i < 8; i++) {
        size_t idx = ((count - 1u) * i) / 8u;
        if (src[idx] != g_last_frame_256[idx])
            return 0;
    }

    bytes = count * sizeof(uint16_t);
    return memcmp(src, g_last_frame_256, bytes) == 0;
}

void ps2_video_cache_store_256(const uint16_t *src, unsigned width, unsigned height)
{
    size_t bytes;

    if (!src || width != PS2_VIDEO_UPLOAD_256_WIDTH || height == 0 || height > PS2_VIDEO_TEX_HEIGHT) {
        g_last_frame_256_valid = 0;
        g_last_frame_256_height = 0;
        return;
    }

    bytes = (size_t)width * (size_t)height * sizeof(uint16_t);
    memcpy(g_last_frame_256, src, bytes);
    g_last_frame_256_height = height;
    g_last_frame_256_valid = 1;
}

void ps2_video_cache_invalidate(void)
{
    g_last_frame_256_valid = 0;
    g_last_frame_256_height = 0;
}
