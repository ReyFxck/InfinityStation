#include "background.h"

#include <stdint.h>

#include <draw.h>
#include <draw2d.h>
#include <draw_types.h>

#include "launcher_video.h"

/* Procedural starfield background.
 *
 * Stars are drawn as GS sprite primitives, emitted directly into the
 * launcher's GS DMA chain (see launcher_video.c). Drawing the stars
 * on the GS instead of software-rasterizing them into the upload
 * buffer:
 *   - moves the per-pixel work off the EE (saves ~ms per frame),
 *   - lets us layer them BEHIND the UI texture: the texture clears
 *     to RGB=0,A=0 (transparent) and only put_pixel'd UI elements
 *     have A=1, so the textured-rect alpha test reveals stars in
 *     untouched regions.
 *
 * Math is kept entirely in s32 fixed-point so the EE doesn't fall
 * back to its (slow) FPU emulation path. The deterministic LCG means
 * the field is identical across boots, no seeding needed.
 */

#define STAR_COUNT      256
#define STAR_FOCAL      256        /* projection focal length */
#define STAR_Z_MIN      32         /* respawn threshold */
#define STAR_Z_NEAR     256        /* below this stars become 2x2 */
#define STAR_Z_MAX      2048
#define STAR_XY_RANGE   512        /* stars live in [-512, 511] world */
#define STAR_DRIFT      2          /* z-units lost per frame; calm pace */
#define STAR_PALETTES   3          /* white / blue / yellow */

typedef struct star
{
    int32_t x;       /* world X, [-STAR_XY_RANGE, STAR_XY_RANGE) */
    int32_t y;       /* world Y, [-STAR_XY_RANGE, STAR_XY_RANGE) */
    int32_t z;       /* depth from camera, (STAR_Z_MIN, STAR_Z_MAX] */
    uint8_t palette; /* 0=white, 1=bluish, 2=yellowish */
} star_t;

typedef struct rgb8
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb8_t;

/* Tints kept subtle on purpose: at peak brightness all three look
 * roughly white, and only diverge as stars get further away. This
 * matches the "deep space, contemplative" vibe (vs. saturated arcade
 * starfield colors). */
static const rgb8_t g_star_tints[STAR_PALETTES] = {
    {255, 255, 255}, /* neutral white  */
    {180, 200, 255}, /* slightly blue  */
    {255, 240, 180}  /* slightly amber */
};

static star_t g_stars[STAR_COUNT];
static int    g_stars_initialized = 0;

/* 32-bit linear-congruential generator (Numerical Recipes constants).
 * Deterministic, no seed needed; produces a balanced distribution of
 * star positions and palette indices. */
static uint32_t g_lcg_state = 0xDEADBEEFu;

static uint32_t lcg_next(void)
{
    g_lcg_state = g_lcg_state * 1664525u + 1013904223u;
    return g_lcg_state;
}

static int32_t lcg_range_signed(int32_t span_half)
{
    /* returns a value in (-span_half, span_half) */
    uint32_t r = lcg_next();
    int32_t  v = (int32_t)(r % (uint32_t)(span_half * 2));
    return v - span_half;
}

static int32_t lcg_range_pos(int32_t lo, int32_t hi)
{
    /* returns a value in [lo, hi) */
    uint32_t r = lcg_next();
    int32_t  span = hi - lo;
    return lo + (int32_t)(r % (uint32_t)span);
}

static void star_respawn(star_t *s)
{
    s->x = lcg_range_signed(STAR_XY_RANGE);
    s->y = lcg_range_signed(STAR_XY_RANGE);
    /* New stars enter at the far end of the volume so the in/out
     * distribution stays roughly uniform: as one falls out near the
     * camera, another starts at the back. */
    s->z = lcg_range_pos(STAR_Z_NEAR, STAR_Z_MAX);
    s->palette = (uint8_t)(lcg_next() % STAR_PALETTES);
}

static void launcher_starfield_init(void)
{
    int i;

    for (i = 0; i < STAR_COUNT; i++) {
        g_stars[i].x = lcg_range_signed(STAR_XY_RANGE);
        g_stars[i].y = lcg_range_signed(STAR_XY_RANGE);
        /* On first init spread stars across the entire depth range so
         * the user sees a populated field on frame zero, instead of
         * having to wait for far stars to drift in. */
        g_stars[i].z = lcg_range_pos(STAR_Z_MIN + 1, STAR_Z_MAX);
        g_stars[i].palette = (uint8_t)(lcg_next() % STAR_PALETTES);
    }

    g_stars_initialized = 1;
}

static void star_color_rgb8(uint8_t palette,
                            uint32_t brightness_q8,
                            uint8_t *out_r,
                            uint8_t *out_g,
                            uint8_t *out_b)
{
    /* brightness_q8 is in [0, 256]; treats 256 as "full intensity". */
    rgb8_t   base;
    uint32_t r;
    uint32_t g;
    uint32_t b;

    if (palette >= STAR_PALETTES)
        palette = 0;

    base = g_star_tints[palette];

    if (brightness_q8 > 256u)
        brightness_q8 = 256u;

    r = ((uint32_t)base.r * brightness_q8) >> 8;
    g = ((uint32_t)base.g * brightness_q8) >> 8;
    b = ((uint32_t)base.b * brightness_q8) >> 8;

    if (r > 255u) r = 255u;
    if (g > 255u) g = 255u;
    if (b > 255u) b = 255u;

    *out_r = (uint8_t)r;
    *out_g = (uint8_t)g;
    *out_b = (uint8_t)b;
}

void launcher_background_advance(void)
{
    int i;

    if (!g_stars_initialized)
        launcher_starfield_init();

    for (i = 0; i < STAR_COUNT; i++) {
        star_t *s = &g_stars[i];

        s->z -= STAR_DRIFT;
        if (s->z <= STAR_Z_MIN)
            star_respawn(s);
    }
}

qword_t *launcher_background_emit_packet(qword_t *q)
{
    const int32_t cx = (int32_t)(PS2_LAUNCHER_WIDTH  / 2);
    const int32_t cy = (int32_t)(PS2_LAUNCHER_HEIGHT / 2);
    int i;

    if (!g_stars_initialized)
        launcher_starfield_init();

    for (i = 0; i < STAR_COUNT; i++) {
        star_t        *s = &g_stars[i];
        int32_t        z;
        int32_t        sx;
        int32_t        sy;
        uint32_t       brightness_q8;
        uint8_t        rr;
        uint8_t        gg;
        uint8_t        bb;
        int            big;
        rect_t         rect;

        z = s->z;
        if (z <= 0)
            continue;

        /* Pinhole projection: screen = center + world*focal/z. */
        sx = cx + (s->x * STAR_FOCAL) / z;
        sy = cy + (s->y * STAR_FOCAL) / z;

        if (sx < 0 || sy < 0)
            continue;
        if (sx >= (int32_t)PS2_LAUNCHER_WIDTH ||
            sy >= (int32_t)PS2_LAUNCHER_HEIGHT)
            continue;

        /* Brightness ramps from ~32 (far) to 256 (near). Mapping is
         * linear in 1/z which gives the "stars getting brighter as
         * they approach" feel without any FPU work. */
        if (z >= STAR_Z_MAX) {
            brightness_q8 = 32u;
        } else if (z <= STAR_Z_NEAR) {
            brightness_q8 = 256u;
        } else {
            uint32_t span = (uint32_t)(STAR_Z_MAX - STAR_Z_NEAR);
            uint32_t off  = (uint32_t)(STAR_Z_MAX - z);
            brightness_q8 = 32u + (off * (256u - 32u)) / span;
        }

        star_color_rgb8(s->palette, brightness_q8, &rr, &gg, &bb);
        big = (z <= STAR_Z_NEAR);

        rect.v0.x = (float)sx;
        rect.v0.y = (float)sy;
        rect.v0.z = 0;
        rect.v1.x = (float)(sx + (big ? 2 : 1));
        rect.v1.y = (float)(sy + (big ? 2 : 1));
        rect.v1.z = 0;
        rect.color.r = rr;
        rect.color.g = gg;
        rect.color.b = bb;
        /* Opaque: alpha test (set up by launcher_video for the UI
         * texture) uses NOTEQUAL 0, so non-zero alpha passes. */
        rect.color.a = 0x80;
        rect.color.q = 1.0f;

        q = draw_rect_filled(q, 0, &rect);
    }

    return q;
}
