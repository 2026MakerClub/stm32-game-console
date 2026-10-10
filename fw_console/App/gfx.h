/* ============================================================
 * gfx.h - flicker-free sprite drawing
 *
 * THE PROBLEM
 *   The obvious way to animate is "fill the old rectangle with
 *   the background colour, then draw the sprite at its new
 *   position".  On an LCD that is two visible writes, and the
 *   eye catches the gap between them as flicker.
 *
 * THE FIX (the course calls it "only redraw the changed area")
 *   Compose one rectangle - background, ground line, sprite, all
 *   of it - in a small RAM buffer, then push that rectangle to
 *   the panel in a single windowed write.  The pixel is never
 *   shown in an intermediate state, so there is nothing to
 *   flicker.
 *
 * USAGE
 *   Gfx_BandBegin(w, h, skyColour);
 *   Gfx_BandHLine(yInBand, groundColour, thickness);
 *   Gfx_BandSprite(&SPR_dino_run1, xInBand, yInBand, inkColour);
 *   Gfx_BandEnd(screenX, screenY);
 * ============================================================ */
#ifndef __GFX_H
#define __GFX_H

#include <stdint.h>
#include "dino_sprites.h"

/* Largest rectangle we can compose at once, in pixels.
 * The widest user is the dino band: 44 px wide x 100 px tall. */
#define GFX_BUF_PIX  4800

void Gfx_BandBegin(uint16_t w, uint16_t h, uint16_t bg);
void Gfx_BandHLine(int16_t y, uint16_t colour, uint8_t thickness);
void Gfx_BandRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t colour);
void Gfx_BandSprite(const Sprite *s, int16_t x, int16_t y, uint16_t ink);
void Gfx_BandEnd(int16_t screenX, int16_t screenY);

/* One-shot sprite on a flat background, for static screens. */
void Gfx_Sprite(int16_t x, int16_t y, const Sprite *s,
                uint16_t ink, uint16_t bg);

#endif /* __GFX_H */
