/* ============================================================
 * ui.c - small shared drawing helpers
 * ============================================================ */
#include "console.h"
#include "sound.h"
#include <string.h>

void App_Wait(uint32_t ms)
{
    uint32_t until = msTick + ms;
    while ((int32_t)(msTick - until) < 0) Sound_Update();
}

void UI_TextCentred(uint16_t cx, uint16_t y, const char *s,
                    uint16_t fg, uint16_t bg, sFONT *font)
{
    uint16_t w;
    int16_t  x;

    if (!s) return;
    LCD_SetFont(font);
    w = (uint16_t)(strlen(s) * font->Width);
    x = (int16_t)cx - (int16_t)(w / 2);
    if (x < 0) x = 0;
    LCD_SetColors(fg, bg);
    LCD_DisplayStringAt((uint16_t)x, y, (char *)s, LEFT_MODE);
}

void UI_Button(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
               const char *label, uint16_t bg, uint16_t fg, sFONT *font)
{
    LCD_SetTextColor(bg);
    LCD_FillRect(x, y, w, h);

    if (label) {
        uint16_t ty = (uint16_t)(y + (h - font->Height) / 2);
        UI_TextCentred((uint16_t)(x + w / 2), ty, label, fg, bg, font);
    }
}

uint8_t UI_HitBox(uint16_t tx, uint16_t ty,
                  uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    return (uint8_t)(tx >= x && tx < (uint16_t)(x + w) &&
                     ty >= y && ty < (uint16_t)(y + h));
}

/* A two-line prompt strip across the top of the screen.
 * Used by both tutorials, so the layout stays consistent. */
void UI_Banner(const char *line1, const char *line2,
               uint16_t bg, uint16_t fg)
{
    LCD_SetTextColor(bg);
    LCD_FillRect(0, 0, SCR_W, 44);

    if (line1) UI_TextCentred(SCR_W / 2,  4, line1, fg, bg, &Font16);
    if (line2) UI_TextCentred(SCR_W / 2, 24, line2, fg, bg, &Font12);
}
