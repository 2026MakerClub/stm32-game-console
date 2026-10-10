/* ============================================================
 * menu.c - the console's home screen
 *
 * Two ways to choose a game, on purpose:
 *   - tap it on the touch panel
 *   - or move with UP/DOWN (button or either stick) and confirm
 *     with a joystick push-switch or the RIGHT button
 * The second path is what saves you when the touch calibration
 * is off, which is the single most common demo-day failure.
 * ============================================================ */
#include "console.h"
#include "input.h"
#include "sound.h"
#include "gfx.h"

#define BG        0x18E3      /* near-black slate          */
#define PANEL     0x2965
#define ACCENT    0x07FF      /* cyan                      */
#define DIM       0x6B4D
#define INK       0xFFFF

#define ITEM_X     26
#define ITEM_W    268
#define ITEM_H     36
#define ITEM_Y0    58
#define ITEM_GAP   42
#define ITEM_N      4

static const char *ITEM_NAME[ITEM_N] = { "DINO RUN", "2048", "GATE KEEPER", "I/O TEST" };
static const char *ITEM_SUB [ITEM_N] = { "jump the cactus",
                                         "slide and merge",
                                         "hold the four lanes",
                                         "check your wiring" };
static const uint8_t ITEM_ON[ITEM_N] = { 1, 1, 1, 1 };

static uint16_t item_y(uint8_t i) { return (uint16_t)(ITEM_Y0 + i * ITEM_GAP); }

static void draw_item(uint8_t i, uint8_t selected)
{
    uint16_t y  = item_y(i);
    uint16_t bg = selected ? ACCENT : PANEL;
    uint16_t fg = selected ? 0x0000 : (ITEM_ON[i] ? INK : DIM);

    LCD_SetTextColor(bg);
    LCD_FillRect(ITEM_X, y, ITEM_W, ITEM_H);

    LCD_SetFont(&Font16);
    LCD_SetColors(fg, bg);
    LCD_DisplayStringAt(ITEM_X + 14, y + 2, (char *)ITEM_NAME[i], LEFT_MODE);

    LCD_SetFont(&Font12);
    LCD_SetColors(selected ? 0x2124 : DIM, bg);
    LCD_DisplayStringAt(ITEM_X + 16, y + 20, (char *)ITEM_SUB[i], LEFT_MODE);
}

static void draw_all(uint8_t sel)
{
    uint8_t i;

    LCD_SetTextColor(BG);
    LCD_FillRect(0, 0, SCR_W, SCR_H);

    /* title */
    UI_TextCentred(SCR_W / 2, 10, "STM32 ARCADE", ACCENT, BG, &Font16);
    LCD_SetTextColor(ACCENT);
    LCD_FillRect(90, 40, 140, 2);

    /* a small mascot in the corner so the screen is not all text */
    Gfx_Sprite(272, 6, &SPR_DINO_IDLE, DIM, BG);

    for (i = 0; i < ITEM_N; i++) draw_item(i, (uint8_t)(i == sel));

    UI_TextCentred(SCR_W / 2, 226, "TAP A GAME  -  KEY1 GOES BACK",
                   DIM, BG, &Font12);
}

AppScreen Menu_Run(void)
{
    TS_StateTypeDef ts;
    uint8_t  sel = 0;
    uint32_t t0;

    draw_all(sel);
    Input_Flush();
    WaitForTouchRelease(5);

    for (;;) {
        t0 = msTick;

        Input_Poll();
        Sound_Update();

        /* ---- keys / sticks ---- */
        {
            uint16_t p = Input_Pressed();
            uint8_t  old = sel;

            if (p & ANY_DOWN)  sel = (uint8_t)((sel + 1) % ITEM_N);
            if (p & ANY_UP)    sel = (uint8_t)((sel + ITEM_N - 1) % ITEM_N);

            if (sel != old) {
                SFX_Select();
                draw_item(old, 0);
                draw_item(sel, 1);
            }

            if (p & (BTN_J1SW | BTN_J2SW | ANY_RIGHT)) {
                SFX_Start();
                return (AppScreen)(APP_DINO + sel);
            }
        }

        /* ---- touch ---- */
        TS_GetState(&ts);
        if (ts.TouchDetected) {
            uint8_t i;
            for (i = 0; i < ITEM_N; i++) {
                if (UI_HitBox(ts.x, ts.y, ITEM_X, item_y(i), ITEM_W, ITEM_H)) {
                    draw_item(i, 1);
                    SFX_Start();
                    WaitForTouchRelease(5);
                    return (AppScreen)(APP_DINO + i);
                }
            }
            WaitForTouchRelease(5);
        }

        while (msTick - t0 < 10u) { }
    }
}
