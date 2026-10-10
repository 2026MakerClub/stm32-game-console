/* ============================================================
 * game_dino.c - Chrome dinosaur runner
 *
 * Sprites are the real Chrome-Dino-Runner artwork, converted to
 * 1-bit-per-pixel masks by scratchpad/gen_sprites.py.
 *
 * SCREEN (320 x 240 landscape)
 *
 *    y=0    +--------------------------------------+
 *           |  HI 00000                   00000    |  score strip
 *    y=56   |       (cloud)        (cloud)         |
 *    y=96   +--- dino band (x 30..74) -------------+
 *           |    |>|                    |cactus|   |
 *    y=196  +--------------------------------------+  ground line
 *           |  - -   - -   - -   - -   - -   - -   |  scrolling dashes
 *    y=240  +--------------------------------------+
 *
 * CONTROLS
 *    UP button / stick up      jump
 *    DOWN button / stick down  duck
 *    KEY1 (PB8)                back to menu
 * ============================================================ */
#include "console.h"
#include "input.h"
#include "sound.h"
#include "gfx.h"
#include <stdio.h>
#include <string.h>

/* ---- palette ------------------------------------------------ */
#define SKY        0xF7BE          /* very light grey, like the browser page */
#define INK        0x528A          /* Chrome's #535353                       */
#define DIMINK     0xB596
#define OKBG       0x0480          /* dark green, for the tutorial "GOOD!"   */
#define BADBG      0x9000          /* dark red                               */

/* ---- world geometry ----------------------------------------- */
#define GROUND_Y     196           /* y of the ground line                   */
#define DINO_X        30
#define DINO_BAND_W   44           /* widest dino pose (duck) + 1            */
#define DINO_BAND_TOP 96
#define DINO_BAND_H  (GROUND_Y - DINO_BAND_TOP)      /* 100 -> 4400 px       */

#define STAND_H       34
#define DUCK_H        22
#define STAND_TOP    (GROUND_Y - STAND_H)            /* 162 */
#define DUCK_TOP     (GROUND_Y - DUCK_H)             /* 174 */

#define CLOUD_Y       56

/* ---- physics, in 1/16 pixel ---------------------------------
 * A jump impulse of -102 against a gravity of +5 per frame peaks
 * after about 20 frames at roughly 64 px, and lands after 40
 * frames.  At a 20 ms tick that is a 0.8 s hop, which feels
 * right next to obstacles moving at 150-400 px/s.
 * ------------------------------------------------------------ */
#define GRAVITY16      5
#define JUMP_V16    (-102)

#define SPEED_MIN16   48           /* 3.0 px per frame */
#define SPEED_MAX16  128           /* 8.0 px per frame */

#define TICK_MS       20

/* ---- obstacle catalogue ------------------------------------- */
typedef struct {
    const Sprite *spr;
    int16_t       top;             /* absolute y of the sprite top */
} ObstDef;

enum { O_CS1 = 0, O_CS2, O_CS3, O_CL1, O_CL2, O_BIRD_MID, O_BIRD_LOW, O_COUNT };

static const ObstDef OBST[O_COUNT] = {
    { &SPR_CAC_S1, GROUND_Y - 26 },
    { &SPR_CAC_S2, GROUND_Y - 26 },
    { &SPR_CAC_S3, GROUND_Y - 26 },
    { &SPR_CAC_L1, GROUND_Y - 34 },
    { &SPR_CAC_L2, GROUND_Y - 34 },
    /* 150..174: clears a ducking dino (174..196), hits a standing one */
    { &SPR_BIRD1,  GROUND_Y - 46 },
    /* 170..194: must be jumped */
    { &SPR_BIRD2,  GROUND_Y - 26 },
};

#define MAX_OBST 3
typedef struct {
    int16_t x;
    uint8_t kind;
    uint8_t active;
    uint8_t passed;                /* already counted for the score */
} Obst;

/* ---- module state ------------------------------------------- */
static int32_t  s_y16, s_vy16;     /* dino vertical position / velocity */
static uint8_t  s_ducking;
static uint8_t  s_onGround;
static uint8_t  s_dead;
static uint8_t  s_animCnt, s_animFrame;

static Obst     s_obst[MAX_OBST];
static int32_t  s_speed16;
static int32_t  s_scrollRem;       /* sub-pixel carry for the scroll */
static uint16_t s_groundOfs;
static uint32_t s_dist;
static uint32_t s_score, s_shown;
static uint32_t s_hi;              /* survives until power-off */
static uint32_t s_rng = 0x1234567;

static int16_t  s_cloudX[2];
static int32_t  s_cloudRem;

/* ============================================================
 * helpers
 * ========================================================== */
static uint32_t rnd(void)
{
    s_rng ^= (msTick * 2654435761UL);
    s_rng  = s_rng * 1664525UL + 1013904223UL;
    return s_rng;
}

static int16_t dino_top(void)
{
    return s_ducking ? DUCK_TOP : (int16_t)(s_y16 / 16);
}

static const Sprite *dino_sprite(void)
{
    if (s_dead)     return &SPR_DINO_DEAD;
    if (!s_onGround) return &SPR_DINO_JUMP;
    if (s_ducking)  return s_animFrame ? &SPR_DINO_DUCK2 : &SPR_DINO_DUCK1;
    return s_animFrame ? &SPR_DINO_RUN2 : &SPR_DINO_RUN1;
}

/* Axis-aligned overlap test, with both boxes shrunk a little so
 * a near-miss reads as a near-miss rather than a hit. */
static uint8_t boxes_overlap(int16_t ax, int16_t ay, int16_t aw, int16_t ah,
                             int16_t bx, int16_t by, int16_t bw, int16_t bh)
{
    if (ax + aw <= bx || bx + bw <= ax) return 0;
    if (ay + ah <= by || by + bh <= ay) return 0;
    return 1;
}

static uint8_t dino_hits(const Obst *o)
{
    const Sprite *s = OBST[o->kind].spr;
    int16_t dx, dy, dw, dh;

    if (s_ducking) { dx = DINO_X + 6;  dy = DUCK_TOP + 2;   dw = 34; dh = 18; }
    else           { dx = DINO_X + 5;  dy = dino_top() + 4; dw = 22; dh = 28; }

    return boxes_overlap(dx, dy, dw, dh,
                         (int16_t)(o->x + 3), (int16_t)(OBST[o->kind].top + 3),
                         (int16_t)(s->w - 6), (int16_t)(s->h - 6));
}

/* ============================================================
 * drawing
 * ========================================================== */
static void draw_static_bg(void)
{
    LCD_SetTextColor(SKY);
    LCD_FillRect(0, 0, SCR_W, SCR_H);
}

static void draw_ground(uint8_t step)
{
    int16_t x;

    s_groundOfs = (uint16_t)((s_groundOfs + step) % 24);

    Gfx_BandBegin(SCR_W, 6, SKY);
    Gfx_BandHLine(0, INK, 2);
    for (x = (int16_t)(-(int16_t)s_groundOfs); x < SCR_W; x = (int16_t)(x + 24)) {
        Gfx_BandRect(x, 4, 7, 1, DIMINK);
        Gfx_BandRect((int16_t)(x + 12), 3, 4, 1, DIMINK);
    }
    Gfx_BandEnd(0, GROUND_Y);
}

static void draw_dino(void)
{
    Gfx_BandBegin(DINO_BAND_W, DINO_BAND_H, SKY);
    Gfx_BandSprite(dino_sprite(), 0,
                   (int16_t)(dino_top() - DINO_BAND_TOP), INK);
    Gfx_BandEnd(DINO_X, DINO_BAND_TOP);
}

/* Draw one obstacle and, in the same write, wipe the strip it
 * just vacated on its right-hand side. */
static void draw_obst(Obst *o, uint8_t step)
{
    const Sprite *s = OBST[o->kind].spr;
    int16_t bx = o->x;
    int16_t bw = (int16_t)(s->w + step);
    int16_t sx = 0;

    /* Obstacles are spawned just off the right edge, so "not on
     * screen yet" must not be confused with "scrolled away". */
    if (bx >= SCR_W) return;
    if (bx + bw <= 0) { o->active = 0; return; }

    if (bx < 0) { sx = bx; bw = (int16_t)(bw + bx); bx = 0; }
    if (bx + bw > SCR_W) bw = (int16_t)(SCR_W - bx);
    if (bw <= 0) return;

    Gfx_BandBegin((uint16_t)bw, s->h, SKY);
    Gfx_BandSprite(s, sx, 0, INK);
    Gfx_BandEnd(bx, OBST[o->kind].top);
}

static void draw_clouds(uint8_t step)
{
    uint8_t i;

    s_cloudRem += (int32_t)step * 6;          /* clouds drift at ~1/3 speed */
    if (s_cloudRem < 16) return;
    {
        uint8_t cstep = (uint8_t)(s_cloudRem / 16);
        s_cloudRem %= 16;

        for (i = 0; i < 2; i++) {
            int16_t bx, bw, sx = 0;
            s_cloudX[i] = (int16_t)(s_cloudX[i] - cstep);
            if (s_cloudX[i] + SPR_CLOUD.w + cstep <= 0)
                s_cloudX[i] = (int16_t)(SCR_W + (int16_t)(rnd() % 90));

            bx = s_cloudX[i];
            bw = (int16_t)(SPR_CLOUD.w + cstep);
            if (bx < 0) { sx = bx; bw = (int16_t)(bw + bx); bx = 0; }
            if (bx + bw > SCR_W) bw = (int16_t)(SCR_W - bx);
            if (bw <= 0) continue;

            Gfx_BandBegin((uint16_t)bw, SPR_CLOUD.h, SKY);
            Gfx_BandSprite(&SPR_CLOUD, sx, 0, DIMINK);
            Gfx_BandEnd(bx, CLOUD_Y);
        }
    }
}

static void draw_score(void)
{
    char buf[26];

    if (s_score == s_shown) return;
    s_shown = s_score;

    sprintf(buf, "HI %05lu   %05lu",
            (unsigned long)s_hi, (unsigned long)s_score);
    LCD_SetFont(&Font12);
    LCD_SetColors(INK, SKY);
    LCD_DisplayStringAt(174, 8, buf, LEFT_MODE);
}

/* ============================================================
 * dino physics, shared by the tutorial and the real game
 * ========================================================== */
static void dino_reset(void)
{
    s_y16      = (int32_t)STAND_TOP * 16;
    s_vy16     = 0;
    s_ducking  = 0;
    s_onGround = 1;
    s_dead     = 0;
    s_animCnt  = 0;
    s_animFrame = 0;
}

/* Returns a bit mask of what the player just did, so the
 * tutorial can check it off:  1 = jumped, 2 = ducked. */
static uint8_t dino_step(uint16_t held, uint16_t pressed)
{
    uint8_t did = 0;

    if (s_onGround) {
        if (pressed & ANY_UP) {
            s_vy16     = JUMP_V16;
            s_onGround = 0;
            s_ducking  = 0;
            did |= 1;
            SFX_Jump();
        } else {
            uint8_t wantDuck = (held & ANY_DOWN) ? 1 : 0;
            if (wantDuck && !s_ducking) { did |= 2; SFX_Duck(); }
            s_ducking = wantDuck;
        }
    }

    if (!s_onGround) {
        s_vy16 += GRAVITY16;
        s_y16  += s_vy16;
        if (s_y16 >= (int32_t)STAND_TOP * 16) {
            s_y16      = (int32_t)STAND_TOP * 16;
            s_vy16     = 0;
            s_onGround = 1;
        }
        /* pulling down in mid-air drops you faster, as in the original */
        if ((held & ANY_DOWN) && s_vy16 < 40) s_vy16 += GRAVITY16;
    }

    if (++s_animCnt >= 6) { s_animCnt = 0; s_animFrame ^= 1; }
    return did;
}

/* ============================================================
 * obstacles
 * ========================================================== */
static void obst_clear(void)
{
    uint8_t i;
    for (i = 0; i < MAX_OBST; i++) s_obst[i].active = 0;
}

static int16_t rightmost_obst(void)
{
    int16_t best = -1000;
    uint8_t i;
    for (i = 0; i < MAX_OBST; i++)
        if (s_obst[i].active) {
            int16_t r = (int16_t)(s_obst[i].x + OBST[s_obst[i].kind].spr->w);
            if (r > best) best = r;
        }
    return best;
}

static void obst_spawn(uint8_t kind, int16_t x)
{
    uint8_t i;
    for (i = 0; i < MAX_OBST; i++)
        if (!s_obst[i].active) {
            s_obst[i].x      = x;
            s_obst[i].kind   = kind;
            s_obst[i].active = 1;
            s_obst[i].passed = 0;
            return;
        }
}

static void obst_maybe_spawn(void)
{
    /* Keep the gap proportional to the speed, otherwise the game
     * becomes impossible rather than merely hard. */
    int16_t gapMin = (int16_t)(110 + (s_speed16 * 12) / 16);
    int16_t gap    = (int16_t)(gapMin + (int16_t)(rnd() % 120));

    if (rightmost_obst() > (int16_t)(SCR_W - gap)) return;

    {
        uint32_t r = rnd() % 100;
        uint8_t  kind;
        if (s_score < 150)      kind = (uint8_t)(r % 3);            /* small cacti  */
        else if (s_score < 400) kind = (uint8_t)(r % 5);            /* + large cacti*/
        else if (r < 70)        kind = (uint8_t)(r % 5);
        else                    kind = (uint8_t)(O_BIRD_MID + (r & 1));
        obst_spawn(kind, SCR_W + 4);
    }
}

/* ============================================================
 * the tutorial
 *
 * Every step is "do the thing and watch the dino answer".  The
 * dino stays fully interactive the whole time, so the player is
 * already practising before the run starts.
 * ========================================================== */
typedef struct {
    const char *l1;
    const char *l2;
    uint16_t    want;        /* input bits that complete the step */
    uint8_t     cactus;      /* 1 = the step is "clear an obstacle" */
} TutStep;

static const TutStep TUT[] = {
    { "PRESS  UP",          "the dino jumps",            BTN_UP,            0 },
    { "HOLD  DOWN",         "the dino ducks",            BTN_DOWN,          0 },
    { "PUSH A STICK UP",    "either stick jumps too",    J1_UP | J2_UP,     0 },
    { "PULL A STICK DOWN",  "either stick ducks too",    J1_DOWN | J2_DOWN, 0 },
    { "JUMP THE CACTUS",    "wait for it, then press UP", 0,                1 },
};
#define TUT_N (sizeof(TUT) / sizeof(TUT[0]))

/* Returns 1 to continue into the game, 0 if the player backed out. */
static uint8_t dino_tutorial(void)
{
    uint8_t  step = 0;
    uint32_t okUntil = 0;             /* while non-zero, "GOOD!" is showing */
    uint32_t t0;

    draw_static_bg();
    dino_reset();
    obst_clear();
    s_speed16   = SPEED_MIN16;
    s_groundOfs = 0;
    s_shown     = 0xFFFFFFFF;
    s_score     = 0;

    UI_Banner(TUT[0].l1, TUT[0].l2, INK, SKY);
    UI_Button(232, 210, 80, 24, "SKIP", DIMINK, SKY, &Font12);
    UI_TextCentred(96, 216, "KEY1 = MENU", DIMINK, SKY, &Font12);
    draw_ground(0);
    draw_dino();
    Input_Flush();
    WaitForTouchRelease(5);

    for (;;) {
        uint16_t held, pressed;
        uint8_t  cleared = 0, scroll;
        TS_StateTypeDef ts;

        t0 = msTick;
        Input_Poll();
        Sound_Update();

        held    = Input_Held();
        pressed = Input_Pressed();

        if (pressed & BTN_BACK) return 0;

        /* tapping SKIP jumps straight into the game */
        TS_GetState(&ts);
        if (ts.TouchDetected) {
            uint8_t hit = UI_HitBox(ts.x, ts.y, 232, 210, 80, 24);
            WaitForTouchRelease(5);
            if (hit) return 1;
        }

        dino_step(held, pressed);

        /* the world only scrolls during the last step */
        scroll = (uint8_t)(TUT[step].cactus ? 3 : 1);
        draw_ground(scroll);
        draw_dino();

        if (TUT[step].cactus) {
            uint8_t i;
            if (!s_obst[0].active && okUntil == 0) obst_spawn(O_CS1, SCR_W + 4);
            for (i = 0; i < MAX_OBST; i++) {
                if (!s_obst[i].active) continue;
                s_obst[i].x = (int16_t)(s_obst[i].x - scroll);
                draw_obst(&s_obst[i], scroll);
                if (!s_obst[i].active) continue;

                if (dino_hits(&s_obst[i])) {
                    SFX_Hit();
                    UI_Banner("OUCH - TRY AGAIN", "jump a little earlier",
                              BADBG, 0xFFFF);
                    App_Wait(900);
                    UI_Banner(TUT[step].l1, TUT[step].l2, INK, SKY);
                    s_obst[i].active = 0;
                    /* wipe whatever the cactus left behind */
                    LCD_SetTextColor(SKY);
                    LCD_FillRect(0, GROUND_Y - 40, SCR_W, 40);
                    dino_reset();
                    Input_Flush();
                } else if (!s_obst[i].passed &&
                           s_obst[i].x + OBST[s_obst[i].kind].spr->w < DINO_X) {
                    /* Cleared it.  This is tracked separately from
                     * "the player pressed jump", otherwise the step
                     * would tick itself off the moment UP is pressed,
                     * before the cactus has actually gone past.      */
                    s_obst[i].passed = 1;
                    cleared = 1;
                }
            }
        }

        /* ---- step bookkeeping ---- */
        if (okUntil) {
            if ((int32_t)(msTick - okUntil) >= 0) {
                okUntil = 0;
                step++;
                if (step >= TUT_N) break;
                UI_Banner(TUT[step].l1, TUT[step].l2, INK, SKY);
                obst_clear();
                LCD_SetTextColor(SKY);
                LCD_FillRect(0, GROUND_Y - 40, SCR_W, 40);
                Input_Flush();
            }
        } else {
            uint8_t done = 0;
            if (TUT[step].cactus) done = cleared;
            else if (pressed & TUT[step].want) done = 1;
            else if ((held & TUT[step].want) && (TUT[step].want & ANY_DOWN))
                done = 1;             /* "hold down" is satisfied by holding */

            if (done) {
                SFX_Good();
                UI_Banner("GOOD!", "", OKBG, 0xFFFF);
                okUntil = msTick + 800;
            }
        }

        while (msTick - t0 < TICK_MS) { }
    }

    /* ---- 3-2-1 ---- */
    {
        const char *cd[3] = { "3", "2", "1" };
        uint8_t i;
        for (i = 0; i < 3; i++) {
            UI_Banner(cd[i], "get ready", INK, SKY);
            Sound_Tone(NOTE_C5, 120);
            App_Wait(500);
        }
    }
    return 1;
}

/* ============================================================
 * the run
 * ========================================================== */
static uint8_t dino_play(void)      /* 0 = back to menu, 1 = player died */
{
    uint32_t t0;
    uint16_t framesSinceSpeedUp = 0;

    draw_static_bg();
    dino_reset();
    obst_clear();
    s_speed16   = SPEED_MIN16;
    s_scrollRem = 0;
    s_dist      = 0;
    s_score     = 0;
    s_shown     = 0xFFFFFFFF;
    s_cloudX[0] = 140;
    s_cloudX[1] = 250;
    s_cloudRem  = 0;

    draw_ground(0);
    draw_score();
    Input_Flush();
    SFX_Start();

    for (;;) {
        uint16_t held, pressed;
        uint8_t  step, i;

        t0 = msTick;
        Input_Poll();
        Sound_Update();

        held    = Input_Held();
        pressed = Input_Pressed();
        if (pressed & BTN_BACK) {
            if (s_score > s_hi) s_hi = s_score;
            return 0;
        }

        dino_step(held, pressed);

        /* ---- how far does the world move this frame? ---- */
        s_scrollRem += s_speed16;
        step = (uint8_t)(s_scrollRem / 16);
        s_scrollRem %= 16;

        if (++framesSinceSpeedUp >= 45 && s_speed16 < SPEED_MAX16) {
            framesSinceSpeedUp = 0;
            s_speed16++;
        }

        obst_maybe_spawn();

        draw_clouds(step);
        draw_ground(step);
        draw_dino();

        for (i = 0; i < MAX_OBST; i++) {
            if (!s_obst[i].active) continue;
            s_obst[i].x = (int16_t)(s_obst[i].x - step);
            draw_obst(&s_obst[i], step);
            if (!s_obst[i].active) continue;

            if (dino_hits(&s_obst[i])) {
                s_dead = 1;
                draw_dino();
                SFX_Hit();
                if (s_score > s_hi) s_hi = s_score;
                return 1;
            }
        }

        /* ---- score ---- */
        s_dist += step;
        s_score = s_dist / 8;
        if (s_score / 100 != s_shown / 100 && s_score >= 100) SFX_Point();
        draw_score();

        while (msTick - t0 < TICK_MS) { }
    }
}

/* ============================================================
 * game-over screen
 * ========================================================== */
static uint8_t dino_gameover(void)   /* 1 = retry, 0 = menu */
{
    TS_StateTypeDef ts;
    char buf[32];

    LCD_SetTextColor(SKY);
    LCD_FillRect(30, 70, 260, 70);

    Gfx_Sprite((SCR_W - SPR_GAMEOVER.w) / 2, 78, &SPR_GAMEOVER, INK, SKY);

    sprintf(buf, "SCORE %05lu    HI %05lu",
            (unsigned long)s_score, (unsigned long)s_hi);
    UI_TextCentred(SCR_W / 2, 104, buf, INK, SKY, &Font12);

    UI_Button(60, 124, 90, 26, "RETRY", INK, SKY, &Font12);
    UI_Button(172, 124, 90, 26, "MENU", DIMINK, SKY, &Font12);
    UI_TextCentred(SCR_W / 2, 214, "UP = RETRY    KEY1 = MENU",
                   DIMINK, SKY, &Font12);

    Input_Flush();
    WaitForTouchRelease(5);

    for (;;) {
        uint32_t t0 = msTick;
        uint16_t p;

        Input_Poll();
        Sound_Update();
        p = Input_Pressed();

        if (p & BTN_BACK) return 0;
        if (p & (ANY_UP | BTN_J1SW | BTN_J2SW)) return 1;

        TS_GetState(&ts);
        if (ts.TouchDetected) {
            WaitForTouchRelease(5);
            if (UI_HitBox(ts.x, ts.y, 172, 124, 90, 26)) return 0;
            if (UI_HitBox(ts.x, ts.y, 60, 124, 90, 26))  return 1;
        }

        while (msTick - t0 < 10u) { }
    }
}

/* ============================================================
 * entry point
 * ========================================================== */
void Dino_Run(void)
{
    if (!dino_tutorial()) return;

    for (;;) {
        if (!dino_play()) return;          /* KEY1 during the run */
        if (!dino_gameover()) return;      /* chose MENU          */
    }
}
