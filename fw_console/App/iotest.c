/* ============================================================
 * iotest.c - live wiring diagnostic
 *
 * Solder one thing, come here, see it move.  That is the whole
 * point: it turns "the game does not respond" into "pin PE3 is
 * dead", which is a question you can actually answer.
 *
 * What each row tells you
 *   raw 0..4095    the ADC count for that joystick axis
 *   a centred stick should read near the boot centre shown in
 *   brackets; ~0 or ~4095 at rest means the axis is shorted to
 *   GND or 3V3, i.e. the wire is on the wrong pin
 *   a button box lights up while that button is held
 * ============================================================ */
#include "console.h"
#include "input.h"
#include "sound.h"
#include <stdio.h>

/* BEEP now runs a 2.5 s sweep from 400 Hz to 4 kHz instead of a
 * short jingle.  Two reasons:
 *   - a small speaker is far louder at its resonance than away
 *     from it, and a sweep is guaranteed to cross it
 *   - it holds the pin toggling long enough to put a multimeter
 *     on PA6.  A 50% square wave at 3.3 V averages about 1.65 V,
 *     so a DC reading near half rail proves the timer is running
 *     and moves the blame to the speaker side.
 */
#define DC_MS      1500u        /* phase 1: steady DC, for an active buzzer */
#define SWEEP_MS   2500u        /* phase 2: sweep, for passive / speaker     */
#define SWEEP_LO    400u
#define SWEEP_HI   4000u

/* Which half of the speaker test is running, shown on the SPK line. */
static const char *s_phase = "IDLE ";

#define BG      0x0000
#define FG      0xFFFF
#define DIM     0x6B4D
#define BAR_BG  0x2965
#define BAR_FG  0x07FF
#define HOT     0x07E0

#define BAR_X   112
#define BAR_W   118
#define BAR_H    12

#define BTN_Y   150
#define BTN_W    40
#define BTN_H    26
#define BTN_GAP  44

#define BACK_X  250
#define BACK_Y    2
#define BACK_W   66
#define BACK_H   24

#define CAL_X    10
#define CAL_Y   186
#define CAL_W   130
#define CAL_H    26

#define BEEP_X  156
#define BEEP_Y  186
#define BEEP_W   90
#define BEEP_H   26

#define FIND_X  252
#define FIND_Y  186
#define FIND_W   62
#define FIND_H   26

static const char *AXIS_NAME[4] = { "J1  X", "J1  Y", "J2  X", "J2  Y" };
static const uint8_t AXIS_CH[4] = { 10, 11, 12, 13 };

static const char *BTN_NAME[7] = { "UP", "DN", "LF", "RT", "J1", "J2", "K1" };
static const uint16_t BTN_BIT[7] = {
    BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT, BTN_J1SW, BTN_J2SW, BTN_BACK
};

static uint16_t axis_y(uint8_t i) { return (uint16_t)(34 + i * 20); }

static void draw_frame(void)
{
    uint8_t i;

    LCD_SetTextColor(BG);
    LCD_FillRect(0, 0, SCR_W, SCR_H);

    LCD_SetFont(&Font16);
    LCD_SetColors(BAR_FG, BG);
    LCD_DisplayStringAt(8, 4, "I/O TEST", LEFT_MODE);

    UI_Button(BACK_X, BACK_Y, BACK_W, BACK_H, "BACK", 0x39C7, FG, &Font12);
    UI_Button(CAL_X,  CAL_Y,  CAL_W,  CAL_H,  "CENTRE STICKS", 0x8400, FG, &Font12);
    UI_Button(BEEP_X, BEEP_Y, BEEP_W, BEEP_H, "BEEP", 0x0410, FG, &Font12);
    UI_Button(FIND_X, FIND_Y, FIND_W, FIND_H, "FIND", 0x8400, FG, &Font12);

    LCD_SetFont(&Font12);
    for (i = 0; i < 4; i++) {
        LCD_SetColors(DIM, BG);
        LCD_DisplayStringAt(8, (uint16_t)(axis_y(i) + 1), (char *)AXIS_NAME[i],
                            LEFT_MODE);
    }

    for (i = 0; i < 7; i++) {
        UI_Button((uint16_t)(6 + i * BTN_GAP), BTN_Y, BTN_W, BTN_H,
                  BTN_NAME[i], BAR_BG, DIM, &Font12);
    }

}

/* ------------------------------------------------------------
 * The line that answers "which pin is this button actually on?"
 *
 * Press a button and read the pin name off the screen.  If the
 * name is not the one you soldered to, you are off by that many
 * positions on the header - fix the wire, or fix the #define at
 * the top of input.c.  Either is fine; the firmware does not
 * care which pin does what.
 * ---------------------------------------------------------- */
static void draw_scan(void)
{
    char     buf[40];
    char    *p = buf;
    uint16_t lowE, lowC;
    uint8_t  i, n = 0;

    p += sprintf(p, (Input_AdcFaults() == 0) ? "ADC OK  LOW " : "ADC FAIL LOW ");

    Input_ScanLow(&lowE, &lowC);
    for (i = 0; i <= 6 && n < 4; i++)
        if (lowE & (1u << i)) { p += sprintf(p, "PE%u ", i); n++; }
    for (i = 4; i <= 12 && n < 4; i++)
        if (lowC & (1u << i)) { p += sprintf(p, "PC%u ", i); n++; }
    if (n == 0) p += sprintf(p, "-");

    /* pad so the previous, longer line is fully overwritten */
    while ((p - buf) < 34) *p++ = ' ';
    *p = 0;

    LCD_SetFont(&Font12);
    LCD_SetColors(Input_AdcFaults() ? 0xF800 : DIM, BG);
    LCD_DisplayStringAt(4, 222, buf, LEFT_MODE);
}

static void draw_axis(uint8_t i, uint16_t raw, uint16_t centre)
{
    uint16_t w = (uint16_t)(((uint32_t)raw * BAR_W) / 4095u);
    uint16_t y = axis_y(i);
    char buf[16];

    LCD_SetTextColor(BAR_FG);
    if (w) LCD_FillRect(BAR_X, y, w, BAR_H);
    LCD_SetTextColor(BAR_BG);
    if (w < BAR_W) LCD_FillRect((uint16_t)(BAR_X + w), y,
                                (uint16_t)(BAR_W - w), BAR_H);

    /* a tick at the measured resting point */
    LCD_SetTextColor(0xFD20);
    LCD_FillRect((uint16_t)(BAR_X + ((uint32_t)centre * BAR_W) / 4095u), y,
                 2, BAR_H);

    sprintf(buf, "%4u", raw);
    LCD_SetFont(&Font12);
    LCD_SetColors(FG, BG);
    LCD_DisplayStringAt((uint16_t)(BAR_X + BAR_W + 6), (uint16_t)(y + 1),
                        buf, LEFT_MODE);
}

static void draw_buttons(uint16_t held, uint16_t shown)
{
    uint8_t i;
    for (i = 0; i < 7; i++) {
        uint16_t bit = BTN_BIT[i];
        if ((held & bit) == (shown & bit)) continue;   /* unchanged */
        UI_Button((uint16_t)(6 + i * BTN_GAP), BTN_Y, BTN_W, BTN_H,
                  BTN_NAME[i], (held & bit) ? HOT : BAR_BG,
                  (held & bit) ? BG : DIM, &Font12);
    }
}

static void draw_dirs(uint16_t held)
{
    char buf[34];

    sprintf(buf, "J1 %c%c%c%c   J2 %c%c%c%c",
            (held & J1_UP)    ? 'U' : '-', (held & J1_DOWN)  ? 'D' : '-',
            (held & J1_LEFT)  ? 'L' : '-', (held & J1_RIGHT) ? 'R' : '-',
            (held & J2_UP)    ? 'U' : '-', (held & J2_DOWN)  ? 'D' : '-',
            (held & J2_LEFT)  ? 'L' : '-', (held & J2_RIGHT) ? 'R' : '-');

    LCD_SetFont(&Font12);
    LCD_SetColors(BAR_FG, BG);
    LCD_DisplayStringAt(8, 124, buf, LEFT_MODE);
}

/* One line of hard numbers about the sound path. */
static void draw_sound(void)
{
    char     buf[36];
    uint32_t clk;
    uint16_t psc, arr, f;

    Sound_Debug(&clk, &psc, &arr, &f);
    (void)psc;
    sprintf(buf, "SPK %s %luk ARR%u %uHz    ",
            s_phase, (unsigned long)(clk / 1000u), arr, f);

    LCD_SetFont(&Font12);
    LCD_SetColors((clk > 1000000u) ? DIM : 0xF800, BG);
    LCD_DisplayStringAt(4, 108, buf, LEFT_MODE);
}

static void draw_touch(uint16_t x, uint16_t y, uint8_t down)
{
    char buf[26];
    sprintf(buf, down ? "TOUCH x=%3u y=%3u" : "TOUCH   --    --  ", x, y);
    LCD_SetFont(&Font12);
    LCD_SetColors(down ? HOT : DIM, BG);
    LCD_DisplayStringAt(180, 124, buf, LEFT_MODE);
}

/* ------------------------------------------------------------
 * FIND: which pin is the buzzer wire actually on?
 *
 * Walks PA1..PA8, squaring each one at roughly 3 kHz for 600 ms
 * and naming it on screen.  Clip the buzzer to the wire you
 * soldered and listen: the pin that makes a noise is the pin you
 * are really connected to.  It bit-bangs rather than using TIM3,
 * so it works on every pin regardless of which timer that pin
 * belongs to, and it drives both buzzer types.
 *
 * PA0 is skipped (often a wake button tied to 3V3), as are
 * PA9..PA12 (USART/USB) and PA13/PA14, which are the SWD debug
 * pins - driving those would drop the ST-Link mid-session.
 * ---------------------------------------------------------- */
static void pin_warble(uint8_t pin, uint32_t ms)
{
    uint32_t t0 = msTick;
    while (msTick - t0 < ms) {
        volatile uint32_t d;
        GPIOA->BSRR = (1UL << pin);
        for (d = 4000; d; d--) { }
        GPIOA->BSRR = (1UL << (pin + 16));
        for (d = 4000; d; d--) { }
    }
}

static void find_pin(void)
{
    uint8_t pin;

    RCC->AHB1ENR |= (1UL << 0);                 /* GPIOA */
    Sound_Stop();

    for (pin = 1; pin <= 8; pin++) {
        char buf[20];
        uint32_t saveMode = GPIOA->MODER;

        sprintf(buf, "TESTING  PA%u", pin);
        LCD_SetTextColor(BG);
        LCD_FillRect(0, 56, SCR_W, 40);
        UI_TextCentred(SCR_W / 2, 62, buf, HOT, BG, &Font16);

        GPIOA->MODER   = (GPIOA->MODER & ~(3UL << (pin * 2)))
                       | (1UL << (pin * 2));    /* 01 = output */
        GPIOA->OTYPER &= ~(1UL << pin);         /* push-pull   */

        pin_warble(pin, 600);

        GPIOA->BSRR  = (1UL << (pin + 16));     /* park low    */
        GPIOA->MODER = saveMode;                /* put it back */
    }

    Sound_Init();        /* FIND left PA6 as a plain GPIO; restore the timer */
}

void IoTest_Run(void)
{
    TS_StateTypeDef ts;
    uint16_t centre[4];
    uint16_t shownBtn = 0;
    uint16_t tx = 0, ty = 0;
    uint8_t  tdown = 0, tshown = 0xFF;
    uint16_t backHold = 0;
    uint32_t sweepEnd = 0, dcEnd = 0;

    draw_frame();
    Input_DiagInit();          /* pull up every candidate button pin */
    Input_JoyCentre(&centre[0], &centre[1], &centre[2], &centre[3]);
    Input_Flush();
    WaitForTouchRelease(5);

    for (;;) {
        uint32_t t0 = msTick;
        uint16_t held, pressed;
        uint8_t  i;

        Input_Poll();
        Sound_Update();

        held    = Input_Held();
        pressed = Input_Pressed();

        /* every button press also proves the speaker works */
        if (pressed & (BTN_UP | BTN_DOWN | BTN_LEFT | BTN_RIGHT |
                       BTN_J1SW | BTN_J2SW))
            Sound_Tone(NOTE_G5, 60);

        /* KEY1 is one of the inputs under test, so a tap must stay a
         * tap.  Holding it for 1.5 s is the way out when the touch
         * panel is the thing that is broken.                       */
        backHold = (uint16_t)((held & BTN_BACK) ? (backHold + 1) : 0);
        if (backHold > 30) return;

        for (i = 0; i < 4; i++)
            draw_axis(i, Input_AdcRaw(AXIS_CH[i]), centre[i]);

        /* ---- speaker test: DC first, then a swept tone ----
         * One press covers both device types.  An ACTIVE buzzer
         * sounds during the DC half and goes quiet for the sweep;
         * a PASSIVE buzzer or a speaker does the opposite.  So
         * whichever half makes a noise also tells you which kind
         * of part is plugged in.                                */
        if (dcEnd) {
            if ((int32_t)(msTick - dcEnd) < 0) {
                Sound_DC(1);
                s_phase = "DC-ON";
            } else {
                Sound_DC(0);
                dcEnd = 0;
                sweepEnd = msTick + SWEEP_MS;
            }
        } else if (sweepEnd) {
            if ((int32_t)(msTick - sweepEnd) < 0) {
                uint32_t done = SWEEP_MS - (sweepEnd - msTick);
                uint16_t f = (uint16_t)(SWEEP_LO +
                             (done * (SWEEP_HI - SWEEP_LO)) / SWEEP_MS);
                Sound_Tone(f, 200);      /* re-armed every frame = continuous */
                s_phase = "SWEEP";
            } else {
                sweepEnd = 0;
                Sound_Stop();
                s_phase = "IDLE ";
            }
        }

        draw_dirs(held);
        draw_sound();
        draw_scan();
        draw_buttons(held, shownBtn);
        shownBtn = held;

        TS_GetState(&ts);
        tdown = (uint8_t)(ts.TouchDetected ? 1 : 0);
        if (tdown) { tx = ts.x; ty = ts.y; }
        if (tdown != tshown || tdown) { draw_touch(tx, ty, tdown); tshown = tdown; }

        if (tdown) {
            if (UI_HitBox(ts.x, ts.y, BACK_X, BACK_Y, BACK_W, BACK_H)) {
                WaitForTouchRelease(5);
                return;
            }
            if (UI_HitBox(ts.x, ts.y, CAL_X, CAL_Y, CAL_W, CAL_H)) {
                Input_Recalibrate();
                Input_JoyCentre(&centre[0], &centre[1], &centre[2], &centre[3]);
                SFX_Good();
                WaitForTouchRelease(5);
            }
            if (UI_HitBox(ts.x, ts.y, FIND_X, FIND_Y, FIND_W, FIND_H)) {
                WaitForTouchRelease(5);
                find_pin();
                draw_frame();
                shownBtn = 0;
                tshown   = 0xFF;
                dcEnd = sweepEnd = 0;
                s_phase = "IDLE ";
            }
            if (UI_HitBox(ts.x, ts.y, BEEP_X, BEEP_Y, BEEP_W, BEEP_H)) {
                dcEnd    = msTick + DC_MS;   /* phase 1; phase 2 follows */
                sweepEnd = 0;
                WaitForTouchRelease(5);
            }
        }

        while (msTick - t0 < 50u) { }
    }
}
