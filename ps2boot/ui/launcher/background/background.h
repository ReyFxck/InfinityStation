#ifndef LAUNCHER_BACKGROUND_H
#define LAUNCHER_BACKGROUND_H

#include <tamtypes.h>

/* Advance the procedural starfield simulation one frame.
 *
 * No drawing is done here: stars are rendered as GS sprite primitives
 * via launcher_background_emit_packet(), called from inside the
 * launcher's GS DMA chain. Keeping the step phase decoupled from the
 * emit phase lets the caller insert star draws at any point in the
 * draw packet (between clear and the UI texture sprite, in our case).
 */
void launcher_background_advance(void);

/* Emit GS sprite primitives for the visible stars into the launcher
 * draw packet. `q` is the current packet write pointer; returns the
 * new write pointer. This must be called inside an active GS context
 * (after draw_setup_environment) and produces filled, untextured
 * sprite primitives. */
qword_t *launcher_background_emit_packet(qword_t *q);

#endif
