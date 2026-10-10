/* ============================================================
 * game_gate.c - GATE KEEPER
 *
 * You are a core in the middle of four lanes.  Things walk down
 * the lanes towards you.  Tap the button for that lane to fire a
 * pulse up it.  Meanwhile a collector you steer with stick 1
 * has to pick up drops before they expire.
 *
 * So: the four buttons are reflex, the stick is navigation, and
 * the game is about not being able to give either your full
 * attention.
 *
 *          UP
 *           |
 *    LEFT --O-- RIGHT        O = the core
 *           |
 *         DOWN
 *
 * WHY THE GEOMETRY LOOKS ODD IN THE CODE
 *   The screen is 320 x 240, so the left/right lanes are 152 px
 *   long and the up/down lanes only 100 px.  If everything moved
 *   at the same pixel speed, the short lanes would be brutally
 *   harder.  So every speed is scaled by that lane's length: an
 *   enemy always takes the same *time* to walk in, whichever
 *   lane it came from.  See lane_scale().
 * ============================================================ */
#include "console.h"
#include "input.h"
#include "sound.h"
#include "gfx.h"
#include <stdio.h>

/* ---- 電競風 palette: neon on black ------------------------- */
#define C_BG        0x0000      /* black                        */
#define C_GRID      0x2124      /* dim blue-grey dots           */
#define C_RAIL      0x18E3      /* lane rail, idle              */
#define C_READY     0x045F      /* lane rail, charged           */
#define C_CORE      0x07FF      /* cyan                         */
#define C_CORE2     0x03BF
#define C_CORE_HIT  0xF800      /* red flash                    */
#define C_PULSE     0xFFFF      /* white                        */
#define C_PULSE2    0x07FF
#define C_EN        0xF81F      /* magenta                      */
#define C_EN2       0xB00F
#define C_FAST      0xFC00      /* orange-red                   */
#define C_FAST2     0xA000
#define C_COLL      0x07E0      /* lime                         */
#define C_HEAL      0x07E0
#define C_COIN      0xFFE0      /* yellow                       */
#define C_NUKE      0x981F      /* electric purple              */
#define C_TEXT      0x07FF
#define C_TEXT2     0x5AEB
#define C_HOT       0xF81F

/* ---- arena ------------------------------------------------- */
#define HUD_H        30
#define CX          160
#define CY          134
#define CORE_R       20
#define LANE_V       92         /* up/down lane length, px       */
#define LANE_H      148         /* left/right lane length, px    */
#define LANE_PAD     11         /* band overhang, so a diamond at
                                 * the spawn point is not clipped */
#define GRID_S       32
#define LANE_W       26         /* lane strip width, px          */

enum { LN_UP = 0, LN_DOWN, LN_LEFT, LN_RIGHT, LN_N };

typedef struct { int16_t bx, by; uint16_t bw, bh; } Band;

/* The strip of screen each lane owns.  Every one of these stops
 * short of the core box, so redrawing a lane can never erase the
 * core and the two never fight over a pixel. */
static const Band LB[LN_N] = {
    { CX - LANE_W / 2,       CY - LANE_V - LANE_PAD, LANE_W, LANE_V - CORE_R + LANE_PAD },
    { CX - LANE_W / 2,       CY + CORE_R + 1,        LANE_W, LANE_V - CORE_R + LANE_PAD },
    { CX - LANE_H - LANE_PAD, CY - LANE_W / 2,       LANE_H - CORE_R + LANE_PAD, LANE_W },
    { CX + CORE_R + 1,        CY - LANE_W / 2,       LANE_H - CORE_R + LANE_PAD, LANE_W },
};

static int16_t lane_len(uint8_t l) { return (l < LN_LEFT) ? LANE_V : LANE_H; }

/* speed multiplier so a short lane is not a harder lane */
static int16_t lane_scale(uint8_t l, int16_t v)
{
    return (l < LN_LEFT) ? (int16_t)((int32_t)v * LANE_V / LANE_H) : v;
}

/* ---- entities ---------------------------------------------- */
#define MAX_EN 14
typedef struct {
    uint8_t active, lane, kind;   /* kind 0 = normal, 1 = fast   */
    int16_t d16, spd16;           /* distance from core, 1/16 px */
} Enemy;

typedef struct {
    uint8_t  active;
    int16_t  d16, spd16;
    uint32_t readyAt;             /* msTick when the lane recharges */
} Pulse;

#define MAX_PU 2
enum { PU_COIN = 0, PU_HEAL, PU_NUKE };
typedef struct {
    uint8_t  active, kind;
    int16_t  x, y;
    uint32_t dieAt;
} Pickup;

#define COOLDOWN_MS 260
#define PULSE_SPD16 128           /* 8 px/frame on a long lane    */
#define KILL_PX       9
#define MAX_HP        5
#define TICK_MS      25

#define COLL_R        8           /* collector half-size          */

/* ---- state ------------------------------------------------- */
static Enemy   s_en[MAX_EN];
static Pulse   s_pulse[LN_N];
static Pickup  s_pu[MAX_PU];
static int16_t s_cx, s_cy, s_ocx, s_ocy;      /* collector, now / previous */
static uint8_t s_hp, s_wave;
static uint16_t s_left;                        /* enemies still to spawn   */
static uint32_t s_score, s_best, s_nextSpawn, s_nextPu, s_coreFlash;
static uint16_t s_combo, s_bestCombo;
static uint32_t s_rng = 0xA5A5F00D;

/* what the HUD is currently showing, so we only repaint changes */
static uint32_t s_hudScore;
static uint16_t s_hudCombo;
static uint8_t  s_hudHp, s_hudWave;

static uint32_t rnd(void)
{
    s_rng ^= (msTick * 2654435761UL);
    s_rng  = s_rng * 1664525UL + 1013904223UL;
    return s_rng;
}

/* ============================================================
 * drawing helpers - all of these compose into the current band
 * ========================================================== */
static void band_grid(const Band *b)
{
    int16_t gx, gy;
    int16_t y0 = (int16_t)(((b->by - HUD_H + GRID_S - 1) / GRID_S) * GRID_S + HUD_H);
    int16_t x0 = (int16_t)(((b->bx + GRID_S - 1) / GRID_S) * GRID_S);

    for (gy = y0; gy < b->by + (int16_t)b->bh; gy = (int16_t)(gy + GRID_S))
        for (gx = x0; gx < b->bx + (int16_t)b->bw; gx = (int16_t)(gx + GRID_S))
            Gfx_BandRect((int16_t)(gx - b->bx), (int16_t)(gy - b->by), 2, 2, C_GRID);
}

/* A filled diamond, drawn row by row.  Diamonds read as "hostile"
 * far better than squares at this size. */
static void band_diamond(int16_t cx, int16_t cy, int8_t r, uint16_t col)
{
    int8_t i;
    for (i = (int8_t)-r; i <= r; i++) {
        int8_t a = (int8_t)(i < 0 ? -i : i);
        int8_t w = (int8_t)(r - a);
        Gfx_BandRect((int16_t)(cx - w), (int16_t)(cy + i),
                     (int16_t)(2 * w + 1), 1, col);
    }
}

/* Where does distance d sit inside this lane's band? */
static void lane_band_xy(uint8_t l, int16_t d, int16_t *bx, int16_t *by)
{
    switch (l) {
    case LN_UP:    *bx = LANE_W / 2;              *by = (int16_t)(CY - d - LB[l].by); break;
    case LN_DOWN:  *bx = LANE_W / 2;              *by = (int16_t)(CY + d - LB[l].by); break;
    case LN_LEFT:  *bx = (int16_t)(CX - d - LB[l].bx); *by = LANE_W / 2;              break;
    default:       *bx = (int16_t)(CX + d - LB[l].bx); *by = LANE_W / 2;              break;
    }
}

static void draw_lane(uint8_t l)
{
    const Band *b = &LB[l];
    uint8_t  vertical = (uint8_t)(l < LN_LEFT);
    uint16_t rail = (s_pulse[l].readyAt <= msTick) ? C_READY : C_RAIL;
    uint8_t  i;

    Gfx_BandBegin(b->bw, b->bh, C_BG);
    band_grid(b);

    /* the rail: a thin line down the middle of the lane */
    if (vertical) Gfx_BandRect(LANE_W / 2 - 1, 0, 2, (int16_t)b->bh, rail);
    else          Gfx_BandRect(0, LANE_W / 2 - 1, (int16_t)b->bw, 2, rail);

    /* the pulse: a bright bar across the lane, with a dimmer tail */
    if (s_pulse[l].active) {
        int16_t px, py;
        lane_band_xy(l, (int16_t)(s_pulse[l].d16 / 16), &px, &py);
        if (vertical) {
            Gfx_BandRect(2, (int16_t)(py - 2), LANE_W - 4, 5, C_PULSE);
            Gfx_BandRect(4, (int16_t)(py + 3), LANE_W - 8, 3, C_PULSE2);
            Gfx_BandRect(6, (int16_t)(py + 6), LANE_W - 12, 2, C_CORE2);
        } else {
            Gfx_BandRect((int16_t)(px - 2), 2, 5, LANE_W - 4, C_PULSE);
            Gfx_BandRect((int16_t)(px + 3), 4, 3, LANE_W - 8, C_PULSE2);
            Gfx_BandRect((int16_t)(px + 6), 6, 2, LANE_W - 12, C_CORE2);
        }
    }

    for (i = 0; i < MAX_EN; i++) {
        int16_t ex, ey;
        if (!s_en[i].active || s_en[i].lane != l) continue;
        lane_band_xy(l, (int16_t)(s_en[i].d16 / 16), &ex, &ey);
        band_diamond(ex, ey, 9, s_en[i].kind ? C_FAST2 : C_EN2);
        band_diamond(ex, ey, 6, s_en[i].kind ? C_FAST  : C_EN);
        band_diamond(ex, ey, 2, 0xFFFF);
    }

    Gfx_BandEnd(b->bx, b->by);
}

static void draw_core(void)
{
    uint16_t a = C_CORE, bcol = C_CORE2;
    Band b;

    if (msTick < s_coreFlash) { a = C_CORE_HIT; bcol = 0x8000; }

    b.bx = CX - CORE_R; b.by = CY - CORE_R;
    b.bw = CORE_R * 2;  b.bh = CORE_R * 2;

    Gfx_BandBegin(b.bw, b.bh, C_BG);
    band_grid(&b);
    band_diamond(CORE_R, CORE_R, (int8_t)(CORE_R - 2), bcol);
    band_diamond(CORE_R, CORE_R, (int8_t)(CORE_R - 7), a);
    band_diamond(CORE_R, CORE_R, 4, 0xFFFF);
    Gfx_BandEnd(b.bx, b.by);
}

static uint16_t pu_colour(uint8_t k)
{
    return (k == PU_HEAL) ? C_HEAL : (k == PU_NUKE) ? C_NUKE : C_COIN;
}

static void draw_pickup(const Pickup *p)
{
    Band b;
    uint16_t col = pu_colour(p->kind);

    /* blink once it is nearly out of time */
    if ((p->dieAt - msTick) < 1500u && ((msTick / 150) & 1)) col = C_RAIL;

    b.bx = (int16_t)(p->x - 8); b.by = (int16_t)(p->y - 8);
    b.bw = 16; b.bh = 16;

    Gfx_BandBegin(b.bw, b.bh, C_BG);
    band_grid(&b);
    band_diamond(8, 8, 7, col);
    band_diamond(8, 8, 3, C_BG);
    if (p->kind == PU_HEAL) {           /* a plus sign */
        Gfx_BandRect(7, 4, 2, 8, col);
        Gfx_BandRect(4, 7, 8, 2, col);
    }
    Gfx_BandEnd(b.bx, b.by);
}

/* Erase a rectangle back to arena background. */
static void wipe(int16_t x, int16_t y, uint16_t w, uint16_t h)
{
    Band b;
    if (x < 0) { w = (uint16_t)(w + x); x = 0; }
    if (y < HUD_H) { h = (uint16_t)(h + y - HUD_H); y = HUD_H; }
    if (x + (int16_t)w > SCR_W) w = (uint16_t)(SCR_W - x);
    if (y + (int16_t)h > SCR_H) h = (uint16_t)(SCR_H - y);
    if ((int16_t)w <= 0 || (int16_t)h <= 0) return;

    b.bx = x; b.by = y; b.bw = w; b.bh = h;
    Gfx_BandBegin(w, h, C_BG);
    band_grid(&b);
    Gfx_BandEnd(x, y);
}

static void draw_collector(void)
{
    Band b;
    b.bx = (int16_t)(s_cx - COLL_R); b.by = (int16_t)(s_cy - COLL_R);
    b.bw = COLL_R * 2; b.bh = COLL_R * 2;

    Gfx_BandBegin(b.bw, b.bh, C_BG);
    band_grid(&b);
    /* an open ring, so it hides as little of the arena as possible */
    Gfx_BandRect(0, 0, COLL_R * 2, 2, C_COLL);
    Gfx_BandRect(0, COLL_R * 2 - 2, COLL_R * 2, 2, C_COLL);
    Gfx_BandRect(0, 0, 2, COLL_R * 2, C_COLL);
    Gfx_BandRect(COLL_R * 2 - 2, 0, 2, COLL_R * 2, C_COLL);
    Gfx_BandRect(COLL_R - 1, COLL_R - 1, 2, 2, C_COLL);
    Gfx_BandEnd(b.bx, b.by);
}

/* ============================================================
 * HUD
 * ========================================================== */
static void hud_frame(void)
{
    LCD_SetTextColor(0x1082);
    LCD_FillRect(0, 0, SCR_W, HUD_H);
    LCD_SetTextColor(C_CORE2);
    LCD_FillRect(0, HUD_H - 2, SCR_W, 1);
    s_hudScore = 0xFFFFFFFF; s_hudCombo = 0xFFFF;
    s_hudHp = 0xFF; s_hudWave = 0xFF;
}

static void hud_update(void)
{
    char buf[16];

    if (s_hudWave != s_wave) {
        s_hudWave = s_wave;
        sprintf(buf, "WAVE %02u", s_wave);
        LCD_SetFont(&Font12);
        LCD_SetColors(C_TEXT2, 0x1082);
        LCD_DisplayStringAt(6, 9, buf, LEFT_MODE);
    }

    if (s_hudHp != s_hp) {
        uint8_t i;
        s_hudHp = s_hp;
        for (i = 0; i < MAX_HP; i++) {
            LCD_SetTextColor(i < s_hp ? C_CORE : 0x3186);
            LCD_FillRect((uint16_t)(70 + i * 13), 9, 10, 12);
        }
    }

    if (s_hudScore != s_score) {
        s_hudScore = s_score;
        sprintf(buf, "%06lu", (unsigned long)(s_score > 999999UL ? 999999UL : s_score));
        LCD_SetFont(&Font16);
        LCD_SetColors(C_TEXT, 0x1082);
        LCD_DisplayStringAt(150, 6, buf, LEFT_MODE);
    }

    if (s_hudCombo != s_combo) {
        s_hudCombo = s_combo;
        sprintf(buf, "x%-3u", s_combo);
        LCD_SetFont(&Font12);
        LCD_SetColors(s_combo >= 5 ? C_HOT : C_TEXT2, 0x1082);
        LCD_DisplayStringAt(286, 9, buf, LEFT_MODE);
    }
}

/* A message strip across the middle, then put the arena back. */
static void flash_msg(const char *msg, uint16_t col)
{
    LCD_SetTextColor(0x1082);
    LCD_FillRect(0, 100, SCR_W, 34);
    UI_TextCentred(SCR_W / 2, 108, msg, col, 0x1082, &Font16);
}

/* The whole arena is far larger than one compose buffer, so this
 * one paints straight to the panel instead of going through
 * Gfx_Band*.  It runs between waves, not per frame. */
static void arena_clear(void)
{
    int16_t gx, gy;

    LCD_SetTextColor(C_BG);
    LCD_FillRect(0, HUD_H, SCR_W, SCR_H - HUD_H);

    LCD_SetTextColor(C_GRID);
    for (gy = HUD_H; gy < SCR_H; gy = (int16_t)(gy + GRID_S))
        for (gx = 0; gx < SCR_W; gx = (int16_t)(gx + GRID_S))
            LCD_FillRect((uint16_t)gx, (uint16_t)gy, 2, 2);
}

/* ============================================================
 * game logic
 * ========================================================== */
static void fire(uint8_t lane)
{
    if (s_pulse[lane].readyAt > msTick) return;      /* still charging */

    s_pulse[lane].active  = 1;
    s_pulse[lane].d16     = (int16_t)(CORE_R * 16);
    s_pulse[lane].spd16   = lane_scale(lane, PULSE_SPD16);
    s_pulse[lane].readyAt = msTick + COOLDOWN_MS;
    Sound_Tone((uint16_t)(700 + lane * 60), 40);
}

static void spawn_enemy(void)
{
    uint8_t  i, lane = (uint8_t)(rnd() & 3);
    uint8_t  kind = (uint8_t)((s_wave >= 3 && (rnd() % 100) < 22) ? 1 : 0);
    int16_t  base = (int16_t)(17 + 2 * (s_wave - 1));

    if (base > 45) base = 45;
    if (kind) base = (int16_t)((base * 16) / 10);

    /* do not stack two enemies on top of each other at the mouth */
    for (i = 0; i < MAX_EN; i++)
        if (s_en[i].active && s_en[i].lane == lane &&
            s_en[i].d16 > (int16_t)((lane_len(lane) - 24) * 16))
            return;

    for (i = 0; i < MAX_EN; i++) {
        if (s_en[i].active) continue;
        s_en[i].active = 1;
        s_en[i].lane   = lane;
        s_en[i].kind   = kind;
        s_en[i].d16    = (int16_t)(lane_len(lane) * 16);
        s_en[i].spd16  = lane_scale(lane, base);
        s_left--;
        return;
    }
}

static void spawn_pickup(void)
{
    uint8_t i;
    for (i = 0; i < MAX_PU; i++) {
        int16_t x, y, dx, dy;
        if (s_pu[i].active) continue;

        /* somewhere in the arena, but not sitting on the core */
        do {
            x = (int16_t)(20 + (rnd() % (SCR_W - 40)));
            y = (int16_t)(HUD_H + 16 + (rnd() % (SCR_H - HUD_H - 32)));
            dx = (int16_t)(x - CX); if (dx < 0) dx = (int16_t)-dx;
            dy = (int16_t)(y - CY); if (dy < 0) dy = (int16_t)-dy;
        } while (dx < 46 && dy < 46);

        s_pu[i].active = 1;
        s_pu[i].x = x;
        s_pu[i].y = y;
        s_pu[i].dieAt = msTick + 6500u;
        {
            uint32_t r = rnd() % 100;
            s_pu[i].kind = (uint8_t)((r < 58) ? PU_COIN
                                   : (r < 88) ? PU_HEAL : PU_NUKE);
        }
        return;
    }
}

static void kill_enemy(uint8_t i)
{
    s_en[i].active = 0;
    s_combo++;
    if (s_combo > s_bestCombo) s_bestCombo = s_combo;
    s_score += 10u + (s_combo > 20u ? 20u : s_combo);
    Sound_Tone((uint16_t)(900 + (s_combo < 24 ? s_combo * 25 : 600)), 35);
}

static void core_hit(uint8_t i)
{
    s_en[i].active = 0;
    if (s_hp) s_hp--;
    s_combo     = 0;
    s_coreFlash = msTick + 300u;
    SFX_Hit();
}

/* ============================================================
 * one simulation step
 * ========================================================== */
static void step_world(void)
{
    uint8_t l, i;

    /* pulses travel outward */
    for (l = 0; l < LN_N; l++) {
        if (!s_pulse[l].active) continue;
        s_pulse[l].d16 = (int16_t)(s_pulse[l].d16 + s_pulse[l].spd16);
        if (s_pulse[l].d16 > (int16_t)((lane_len(l) + 6) * 16))
            s_pulse[l].active = 0;
    }

    /* enemies walk inward */
    for (i = 0; i < MAX_EN; i++) {
        if (!s_en[i].active) continue;
        s_en[i].d16 = (int16_t)(s_en[i].d16 - s_en[i].spd16);
        if (s_en[i].d16 <= (int16_t)(CORE_R * 16)) { core_hit(i); continue; }

        l = s_en[i].lane;
        if (s_pulse[l].active) {
            int16_t gap = (int16_t)((s_en[i].d16 - s_pulse[l].d16) / 16);
            if (gap < 0) gap = (int16_t)-gap;
            if (gap <= KILL_PX) kill_enemy(i);
        }
    }

    /* pickups time out */
    for (i = 0; i < MAX_PU; i++) {
        if (!s_pu[i].active) continue;
        if ((int32_t)(msTick - s_pu[i].dieAt) >= 0) {
            s_pu[i].active = 0;
            wipe((int16_t)(s_pu[i].x - 8), (int16_t)(s_pu[i].y - 8), 16, 16);
        }
    }
}

static void move_collector(void)
{
    int16_t ax = Input_JoyAxis(0);
    int16_t ay = Input_JoyAxis(1);
    uint8_t i;

    s_ocx = s_cx;
    s_ocy = s_cy;

    /* /110 gives a top speed of about 4.6 px per 25 ms frame */
    s_cx = (int16_t)(s_cx + ax / 110);
    s_cy = (int16_t)(s_cy - ay / 110);     /* stick up = screen up */

    if (s_cx < COLL_R)              s_cx = COLL_R;
    if (s_cx > SCR_W - COLL_R)      s_cx = SCR_W - COLL_R;
    if (s_cy < HUD_H + COLL_R)      s_cy = HUD_H + COLL_R;
    if (s_cy > SCR_H - COLL_R)      s_cy = SCR_H - COLL_R;

    for (i = 0; i < MAX_PU; i++) {
        int16_t dx, dy;
        if (!s_pu[i].active) continue;
        dx = (int16_t)(s_pu[i].x - s_cx); if (dx < 0) dx = (int16_t)-dx;
        dy = (int16_t)(s_pu[i].y - s_cy); if (dy < 0) dy = (int16_t)-dy;
        if (dx > COLL_R + 6 || dy > COLL_R + 6) continue;

        switch (s_pu[i].kind) {
        case PU_HEAL:
            if (s_hp < MAX_HP) s_hp++;
            SFX_Good();
            break;
        case PU_NUKE: {
            uint8_t k;
            for (k = 0; k < MAX_EN; k++)
                if (s_en[k].active) { s_en[k].active = 0; s_score += 15; }
            SFX_Start();
            break;
        }
        default:
            s_score += 150;
            SFX_Merge();
            break;
        }
        s_pu[i].active = 0;
        wipe((int16_t)(s_pu[i].x - 8), (int16_t)(s_pu[i].y - 8), 16, 16);
    }
}

static void render(void)
{
    uint8_t i;

    /* erase the collector's old footprint first, so the lane and
     * core redraws below put back anything it was covering */
    if (s_ocx != s_cx || s_ocy != s_cy)
        wipe((int16_t)(s_ocx - COLL_R), (int16_t)(s_ocy - COLL_R),
             COLL_R * 2, COLL_R * 2);

    for (i = 0; i < LN_N; i++) draw_lane(i);
    draw_core();
    for (i = 0; i < MAX_PU; i++) if (s_pu[i].active) draw_pickup(&s_pu[i]);
    draw_collector();
    hud_update();
}

static void reset_round(void)
{
    uint8_t i;
    for (i = 0; i < MAX_EN; i++) s_en[i].active = 0;
    for (i = 0; i < LN_N; i++)   { s_pulse[i].active = 0; s_pulse[i].readyAt = 0; }
    for (i = 0; i < MAX_PU; i++) s_pu[i].active = 0;

    s_cx = s_ocx = CX;
    s_cy = s_ocy = CY + 64;
    s_hp = MAX_HP;
    s_wave = 0;
    s_left = 0;
    s_score = 0;
    s_combo = 0;
    s_bestCombo = 0;
    s_coreFlash = 0;
    s_nextPu = msTick + 4000u;
}

static uint16_t wave_size(uint8_t w) { return (uint16_t)(6 + 2 * w); }

static uint32_t spawn_gap(uint8_t w)
{
    int32_t g = 1150 - 60 * (int32_t)w;
    return (uint32_t)(g < 340 ? 340 : g);
}

/* ============================================================
 * tutorial - three steps, same shape as the other two games
 * ========================================================== */
static uint8_t gate_tutorial(void)      /* 1 = play, 0 = menu */
{
    static const uint16_t WANT[2] = { BTN_LEFT, BTN_UP };
    static const uint8_t  LANE[2] = { LN_LEFT,  LN_UP  };
    static const char *L1[3] = { "ENEMY ON THE",   "NOW FROM",      "A DROP APPEARED" };
    static const char *L2[3] = { "LEFT - PRESS LEFT", "ABOVE - PRESS UP",
                                 "STICK 1 TO GRAB IT" };
    uint8_t step = 0;

    reset_round();
    hud_frame();
    arena_clear();
    Input_Flush();
    WaitForTouchRelease(5);

    for (;;) {
        uint32_t t0 = msTick;
        uint16_t p;
        uint8_t  i, busy;
        TS_StateTypeDef ts;

        Input_Poll();
        Sound_Update();
        p = Input_Pressed();
        if (p & BTN_BACK) return 0;

        TS_GetState(&ts);
        if (ts.TouchDetected) {
            uint8_t hit = UI_HitBox(ts.x, ts.y, 232, 214, 80, 22);
            WaitForTouchRelease(5);
            if (hit) return 1;
        }

        if (step < 2) {
            /* keep exactly one slow enemy walking down the demo lane */
            busy = 0;
            for (i = 0; i < MAX_EN; i++) if (s_en[i].active) busy = 1;
            if (!busy) {
                s_en[0].active = 1;
                s_en[0].lane   = LANE[step];
                s_en[0].kind   = 0;
                s_en[0].d16    = (int16_t)(lane_len(LANE[step]) * 16);
                s_en[0].spd16  = lane_scale(LANE[step], 10);
            }
            if (p & WANT[step]) fire(LANE[step]);
        } else {
            busy = 0;
            for (i = 0; i < MAX_PU; i++) if (s_pu[i].active) busy = 1;
            if (!busy && s_score == 0) {
                spawn_pickup();
                /* a HEAL drop scores nothing, and the step below
                 * watches the score - so force a coin here */
                for (i = 0; i < MAX_PU; i++)
                    if (s_pu[i].active) s_pu[i].kind = PU_COIN;
            }
            move_collector();
        }

        step_world();
        s_hp = MAX_HP;          /* the tutorial never actually hurts you */
        render();

        /* Prompt and SKIP are painted after render(), because the
         * lane strips and the collector wipe would otherwise erase
         * them.  Repainting text over itself does not flicker: the
         * font routine writes foreground and background together,
         * so there is never a blank frame in between.            */
        LCD_SetFont(&Font12);
        LCD_SetColors(C_TEXT, C_BG);
        LCD_DisplayStringAt(6, 44, (char *)L1[step], LEFT_MODE);
        LCD_SetColors(step == 2 ? C_COLL : C_TEXT2, C_BG);
        LCD_DisplayStringAt(6, 58, (char *)L2[step], LEFT_MODE);
        UI_Button(232, 214, 80, 22, "SKIP", 0x39C7, 0xFFFF, &Font12);

        if (step < 2) {
            busy = 0;
            for (i = 0; i < MAX_EN; i++) if (s_en[i].active) busy = 1;
            if (!busy && s_score > 0) {              /* shot it, not missed */
                s_score = 0; s_combo = 0;
                SFX_Good();
                flash_msg("GOOD", C_COLL);
                App_Wait(700);
                arena_clear();
                step++;
                Input_Flush();
            }
        } else if (s_score > 0) {
            SFX_Good();
            flash_msg("READY", C_CORE);
            App_Wait(900);
            return 1;
        }

        while (msTick - t0 < TICK_MS) { }
    }
}

/* ============================================================
 * the round
 * ========================================================== */
static uint8_t gate_play(void)      /* 1 = core destroyed, 0 = quit */
{
    reset_round();
    hud_frame();
    arena_clear();
    hud_update();
    Input_Flush();

    for (;;) {
        uint32_t t0 = msTick;
        uint16_t p;

        Input_Poll();
        Sound_Update();

        p = Input_Pressed();
        if (p & BTN_BACK) { if (s_score > s_best) s_best = s_score; return 0; }

        if (p & BTN_UP)    fire(LN_UP);
        if (p & BTN_DOWN)  fire(LN_DOWN);
        if (p & BTN_LEFT)  fire(LN_LEFT);
        if (p & BTN_RIGHT) fire(LN_RIGHT);

        /* ---- waves ---- */
        if (s_left == 0) {
            uint8_t i, clear = 1;
            for (i = 0; i < MAX_EN; i++) if (s_en[i].active) clear = 0;
            if (clear) {
                s_wave++;
                s_left = wave_size(s_wave);
                s_nextSpawn = msTick + 700u;
                s_score += 50u * s_wave;
                SFX_Start();
                flash_msg("WAVE UP", C_HOT);
                App_Wait(900);
                arena_clear();
            }
        } else if ((int32_t)(msTick - s_nextSpawn) >= 0) {
            spawn_enemy();
            s_nextSpawn = msTick + spawn_gap(s_wave);
        }

        if ((int32_t)(msTick - s_nextPu) >= 0) {
            spawn_pickup();
            s_nextPu = msTick + 5200u + (rnd() % 3000u);
        }

        move_collector();
        step_world();
        render();

        if (s_hp == 0) {
            if (s_score > s_best) s_best = s_score;
            return 1;
        }

        while (msTick - t0 < TICK_MS) { }
    }
}

/* ============================================================
 * game over
 * ========================================================== */
static uint8_t gate_over(void)      /* 1 = retry, 0 = menu */
{
    TS_StateTypeDef ts;
    char buf[32];

    LCD_SetTextColor(0x1082);
    LCD_FillRect(20, 76, 280, 92);
    LCD_SetTextColor(C_CORE_HIT);
    LCD_FillRect(20, 76, 280, 2);

    UI_TextCentred(SCR_W / 2, 84, "CORE BREACHED", C_HOT, 0x1082, &Font16);
    sprintf(buf, "SCORE %06lu", (unsigned long)s_score);
    UI_TextCentred(SCR_W / 2, 110, buf, C_TEXT, 0x1082, &Font16);
    sprintf(buf, "BEST %06lu   COMBO x%u",
            (unsigned long)s_best, s_bestCombo);
    UI_TextCentred(SCR_W / 2, 130, buf, C_TEXT2, 0x1082, &Font12);

    UI_Button(58, 146, 90, 18, "RETRY", C_CORE2, 0x0000, &Font12);
    UI_Button(172, 146, 90, 18, "MENU", 0x39C7, 0xFFFF, &Font12);

    Input_Flush();
    WaitForTouchRelease(5);

    for (;;) {
        uint32_t t0 = msTick;
        uint16_t p;

        Input_Poll();
        Sound_Update();
        p = Input_Pressed();

        if (p & BTN_BACK) return 0;
        if (p & (BTN_UP | BTN_J1SW | BTN_J2SW)) return 1;

        TS_GetState(&ts);
        if (ts.TouchDetected) {
            WaitForTouchRelease(5);
            if (UI_HitBox(ts.x, ts.y, 172, 146, 90, 18)) return 0;
            if (UI_HitBox(ts.x, ts.y, 58, 146, 90, 18))  return 1;
        }

        while (msTick - t0 < 10u) { }
    }
}

/* ============================================================
 * entry point
 * ========================================================== */
void Gate_Run(void)
{
    if (!gate_tutorial()) return;

    for (;;) {
        if (!gate_play()) return;
        if (!gate_over()) return;
    }
}
