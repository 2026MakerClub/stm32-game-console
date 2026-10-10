/* ============================================================
 * gfx.c - compose-then-blit sprite renderer
 * ============================================================ */
#include "stm324xg_lcd_sklin.h"
#include "gfx.h"

static uint16_t s_buf[GFX_BUF_PIX];
static uint16_t s_w, s_h;

void Gfx_BandBegin(uint16_t w, uint16_t h, uint16_t bg)
{
    uint32_t n, i;

    if ((uint32_t)w * h > GFX_BUF_PIX) {   /* refuse rather than corrupt RAM */
        s_w = s_h = 0;
        return;
    }
    s_w = w;
    s_h = h;
    n   = (uint32_t)w * h;
    for (i = 0; i < n; i++) s_buf[i] = bg;
}

void Gfx_BandRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t colour)
{
    int16_t xx, yy;

    if (s_w == 0) return;

    for (yy = y; yy < y + h; yy++) {
        if (yy < 0 || yy >= (int16_t)s_h) continue;
        for (xx = x; xx < x + w; xx++) {
            if (xx < 0 || xx >= (int16_t)s_w) continue;
            s_buf[(uint32_t)yy * s_w + xx] = colour;
        }
    }
}

void Gfx_BandHLine(int16_t y, uint16_t colour, uint8_t thickness)
{
    Gfx_BandRect(0, y, (int16_t)s_w, (int16_t)thickness, colour);
}

void Gfx_BandSprite(const Sprite *s, int16_t x, int16_t y, uint16_t ink)
{
    uint16_t stride;
    int16_t  sx, sy;

    if (s_w == 0 || s == 0) return;

    stride = (uint16_t)((s->w + 7) / 8);

    for (sy = 0; sy < (int16_t)s->h; sy++) {
        int16_t dy = y + sy;
        const uint8_t *row;

        if (dy < 0 || dy >= (int16_t)s_h) continue;
        row = s->bits + (uint32_t)sy * stride;

        for (sx = 0; sx < (int16_t)s->w; sx++) {
            int16_t dx = x + sx;
            if (dx < 0 || dx >= (int16_t)s_w) continue;
            if (row[sx >> 3] & (0x80u >> (sx & 7)))
                s_buf[(uint32_t)dy * s_w + dx] = ink;
        }
    }
}

void Gfx_BandEnd(int16_t screenX, int16_t screenY)
{
    if (s_w == 0) return;
    /* The caller is responsible for keeping the band on screen;
     * LCD_DrawRGBImage clips the window but not the source
     * pointer, so a partly off-screen band would shear.        */
    LCD_DrawRGBImage((uint16_t)screenX, (uint16_t)screenY,
                     s_w, s_h, (uint8_t *)s_buf);
    s_w = s_h = 0;
}

void Gfx_Sprite(int16_t x, int16_t y, const Sprite *s,
                uint16_t ink, uint16_t bg)
{
    if (!s) return;
    Gfx_BandBegin(s->w, s->h, bg);
    Gfx_BandSprite(s, 0, 0, ink);
    Gfx_BandEnd(x, y);
}
