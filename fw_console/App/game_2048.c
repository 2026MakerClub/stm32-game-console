/* ============================================================
 * game_2048.c - 2048 sliding-tile puzzle
 *
 * Board logic is the course project's original (MCU2026/2048),
 * with three changes:
 *
 *   1. LAYOUT FIX.  The old top bar printed one long string,
 *      "Score:%5lu  Best:%5lu", at x=2 in Font20 (14 px per
 *      character).  23 characters x 14 px = 322 px, on a 320 px
 *      screen, so it ran straight through the New Game button at
 *      x=220.  The bar is now four separate fields with fixed,
 *      non-overlapping boxes.
 *   2. Tile numbers pick a font that actually fits the 44 px
 *      cell, so 1024 and 2048 no longer spill over the edges.
 *   3. A swipe tutorial runs before the first game.
 *
 * SCREEN (320 x 240)
 *   y 0..37   top bar: SCORE | BEST | NEW | MENU
 *   y 39..239 the 4x4 board, 201 px square, centred at x=59
 * ============================================================ */
#include "console.h"
#include "input.h"
#include "sound.h"
#include "gfx.h"
#include <stdio.h>
#include <string.h>

/* ---- board geometry (unchanged from the original) ---------- */
#define BOARD_N      4
#define CELL_SIZE    44
#define CELL_GAP     5
#define GRID_X       59
#define GRID_Y       39
#define GRID_SIDE   ((BOARD_N) * (CELL_SIZE) + (BOARD_N + 1) * (CELL_GAP))  /* 201 */

/* ---- top bar: every field gets its own box, and they do not
 *      overlap.  Check the arithmetic in the comments before
 *      changing a font here.                                   */
#define BAR_H        38
#define BAR_BG       0x5ACB

#define SCORE_X       6      /* label Font12 5x7=35 px; value Font16 5x11=55 px */
#define BEST_X       96      /*                        -> 6..61 and 96..151     */
#define LBL_Y         3
#define VAL_Y        18

#define BTN_NEW_X   196
#define BTN_MENU_X  258
#define BTN_Y         4
#define BTN_W        56
#define BTN_H        30

#define SWIPE_MIN    28      /* ignore finger movements shorter than this */

/* ---- tile colours, index = log2(value) --------------------- */
static const uint16_t TILE_BG_COLOR[] = {
    0x7BEF, 0xEF5D, 0xEDB6, 0xFD20, 0xFC80, 0xF800, 0xF9E0,
    0xFFE0, 0xFFE0, 0x87E0, 0x07E0, 0x051F, 0x8010,
};
#define TILE_COLOR_COUNT (sizeof(TILE_BG_COLOR) / sizeof(TILE_BG_COLOR[0]))

/* ---- state ------------------------------------------------- */
static uint16_t g_board[BOARD_N][BOARD_N];
static uint32_t g_score;
static uint32_t g_best;
static uint8_t  g_game_over;
static uint8_t  g_game_won;
static uint32_t g_rand_state = 42;

/* swipe tracking */
static uint16_t g_sx, g_sy, g_ex, g_ey;
static uint8_t  g_was_touching;

typedef enum { SW_NONE = 0, SW_LEFT, SW_RIGHT, SW_UP, SW_DOWN } SwipeDir;

/* ============================================================
 * utilities
 * ========================================================== */
static uint32_t Rand_LCG(void)
{
    g_rand_state ^= (msTick * 2654435761UL) ^ ((uint32_t)g_score << 7);
    g_rand_state  = g_rand_state * 1664525UL + 1013904223UL;
    return g_rand_state;
}

static uint8_t Log2Idx(uint16_t val)
{
    uint8_t  idx = 0;
    uint16_t v   = val;
    if (val == 0) return 0;
    while (v > 1) { v >>= 1; idx++; }
    return idx;
}

/* ============================================================
 * board logic (unchanged algorithm, see the original teaching
 * document 教學文件_2048.md section 6)
 * ========================================================== */
static void Add_RandomTile(void)
{
    uint8_t pos[BOARD_N * BOARD_N][2];
    uint8_t count = 0, r, c, idx;

    for (r = 0; r < BOARD_N; r++)
        for (c = 0; c < BOARD_N; c++)
            if (g_board[r][c] == 0) { pos[count][0] = r; pos[count][1] = c; count++; }

    if (count == 0) return;
    idx = (uint8_t)(Rand_LCG() % count);
    g_board[pos[idx][0]][pos[idx][1]] = (uint16_t)((Rand_LCG() % 10 < 9) ? 2 : 4);
}

static void score_add(uint16_t v)
{
    g_score += v;
    if (g_score > g_best) g_best = g_score;
}

static uint8_t Move_Left(void)
{
    uint8_t moved = 0, r, c, k;
    for (r = 0; r < BOARD_N; r++) {
        uint16_t tmp[BOARD_N] = {0}, result[BOARD_N] = {0};
        k = 0;
        for (c = 0; c < BOARD_N; c++) if (g_board[r][c]) tmp[k++] = g_board[r][c];
        for (c = 0; c < BOARD_N - 1; c++)
            if (tmp[c] && tmp[c] == tmp[c + 1]) {
                tmp[c] = (uint16_t)(tmp[c] << 1); score_add(tmp[c]); tmp[c + 1] = 0;
            }
        k = 0;
        for (c = 0; c < BOARD_N; c++) if (tmp[c]) result[k++] = tmp[c];
        for (c = 0; c < BOARD_N; c++) {
            if (g_board[r][c] != result[c]) moved = 1;
            g_board[r][c] = result[c];
        }
    }
    return moved;
}

static uint8_t Move_Right(void)
{
    uint8_t moved = 0, r, c, k;
    for (r = 0; r < BOARD_N; r++) {
        uint16_t tmp[BOARD_N] = {0}, result[BOARD_N] = {0};
        k = BOARD_N - 1;
        for (c = BOARD_N; c-- > 0; ) if (g_board[r][c]) tmp[k--] = g_board[r][c];
        for (c = BOARD_N - 1; c > 0; c--)
            if (tmp[c] && tmp[c] == tmp[c - 1]) {
                tmp[c] = (uint16_t)(tmp[c] << 1); score_add(tmp[c]); tmp[c - 1] = 0;
            }
        k = BOARD_N - 1;
        for (c = BOARD_N; c-- > 0; ) if (tmp[c]) result[k--] = tmp[c];
        for (c = 0; c < BOARD_N; c++) {
            if (g_board[r][c] != result[c]) moved = 1;
            g_board[r][c] = result[c];
        }
    }
    return moved;
}

static uint8_t Move_Up(void)
{
    uint8_t moved = 0, r, c, k;
    for (c = 0; c < BOARD_N; c++) {
        uint16_t tmp[BOARD_N] = {0}, result[BOARD_N] = {0};
        k = 0;
        for (r = 0; r < BOARD_N; r++) if (g_board[r][c]) tmp[k++] = g_board[r][c];
        for (r = 0; r < BOARD_N - 1; r++)
            if (tmp[r] && tmp[r] == tmp[r + 1]) {
                tmp[r] = (uint16_t)(tmp[r] << 1); score_add(tmp[r]); tmp[r + 1] = 0;
            }
        k = 0;
        for (r = 0; r < BOARD_N; r++) if (tmp[r]) result[k++] = tmp[r];
        for (r = 0; r < BOARD_N; r++) {
            if (g_board[r][c] != result[r]) moved = 1;
            g_board[r][c] = result[r];
        }
    }
    return moved;
}

static uint8_t Move_Down(void)
{
    uint8_t moved = 0, r, c, k;
    for (c = 0; c < BOARD_N; c++) {
        uint16_t tmp[BOARD_N] = {0}, result[BOARD_N] = {0};
        k = BOARD_N - 1;
        for (r = BOARD_N; r-- > 0; ) if (g_board[r][c]) tmp[k--] = g_board[r][c];
        for (r = BOARD_N - 1; r > 0; r--)
            if (tmp[r] && tmp[r] == tmp[r - 1]) {
                tmp[r] = (uint16_t)(tmp[r] << 1); score_add(tmp[r]); tmp[r - 1] = 0;
            }
        k = BOARD_N - 1;
        for (r = BOARD_N; r-- > 0; ) if (tmp[r]) result[k--] = tmp[r];
        for (r = 0; r < BOARD_N; r++) {
            if (g_board[r][c] != result[r]) moved = 1;
            g_board[r][c] = result[r];
        }
    }
    return moved;
}

static uint8_t Check_GameOver(void)
{
    uint8_t r, c;
    for (r = 0; r < BOARD_N; r++)
        for (c = 0; c < BOARD_N; c++) {
            if (g_board[r][c] == 0) return 0;
            if (c + 1 < BOARD_N && g_board[r][c] == g_board[r][c + 1]) return 0;
            if (r + 1 < BOARD_N && g_board[r][c] == g_board[r + 1][c]) return 0;
        }
    return 1;
}

static uint8_t Check_Win(void)
{
    uint8_t r, c;
    for (r = 0; r < BOARD_N; r++)
        for (c = 0; c < BOARD_N; c++)
            if (g_board[r][c] >= 2048) return 1;
    return 0;
}

/* ============================================================
 * rendering
 * ========================================================== */
static void Draw_Tile(uint8_t row, uint8_t col)
{
    uint16_t x    = (uint16_t)(GRID_X + CELL_GAP + col * (CELL_SIZE + CELL_GAP));
    uint16_t y    = (uint16_t)(GRID_Y + CELL_GAP + row * (CELL_SIZE + CELL_GAP));
    uint16_t val  = g_board[row][col];
    uint8_t  cidx = Log2Idx(val);
    uint16_t bg, fg;

    if (cidx >= TILE_COLOR_COUNT) cidx = (uint8_t)(TILE_COLOR_COUNT - 1);
    bg = TILE_BG_COLOR[cidx];
    fg = (val <= 4) ? DARKGRAY : WHITE;

    LCD_SetTextColor(bg);
    LCD_FillRect(x, y, CELL_SIZE, CELL_SIZE);

    if (val > 0) {
        char   buf[8];
        sFONT *f;
        uint16_t tw, tx, ty;

        sprintf(buf, "%u", (unsigned)val);

        /* pick the biggest font whose string still fits the 44 px cell */
        if (strlen(buf) <= 4) f = &Font16;   /* 4 x 11 = 44, exactly fits */
        else                  f = &Font12;   /* 5 x  7 = 35              */

        LCD_SetFont(f);
        tw = (uint16_t)(strlen(buf) * f->Width);
        tx = (uint16_t)(x + (CELL_SIZE - tw) / 2);
        ty = (uint16_t)(y + (CELL_SIZE - f->Height) / 2);
        LCD_SetColors(fg, bg);
        LCD_DisplayStringAt(tx, ty, buf, LEFT_MODE);
    }
}

static void Draw_Board(void)
{
    uint8_t r, c;
    for (r = 0; r < BOARD_N; r++)
        for (c = 0; c < BOARD_N; c++)
            Draw_Tile(r, c);
}

static void Draw_Buttons(uint8_t newPressed)
{
    UI_Button(BTN_NEW_X,  BTN_Y, BTN_W, BTN_H, "NEW",
              newPressed ? DARKGREEN : GREEN, WHITE, &Font12);
    UI_Button(BTN_MENU_X, BTN_Y, BTN_W, BTN_H, "MENU", 0x39C7, WHITE, &Font12);
}

/* Only the two numbers are redrawn during play; the labels and
 * the buttons are painted once by Draw_Background(). */
static void Draw_Values(void)
{
    char buf[8];

    LCD_SetFont(&Font16);
    LCD_SetColors(WHITE, BAR_BG);

    sprintf(buf, "%5lu", (unsigned long)(g_score > 99999UL ? 99999UL : g_score));
    LCD_DisplayStringAt(SCORE_X, VAL_Y, buf, LEFT_MODE);

    sprintf(buf, "%5lu", (unsigned long)(g_best > 99999UL ? 99999UL : g_best));
    LCD_DisplayStringAt(BEST_X, VAL_Y, buf, LEFT_MODE);
}

static void Draw_Background(void)
{
    LCD_SetTextColor(0x39C7);
    LCD_FillRect(0, 0, SCR_W, SCR_H);

    LCD_SetTextColor(BAR_BG);
    LCD_FillRect(0, 0, SCR_W, BAR_H);

    LCD_SetFont(&Font12);
    LCD_SetColors(0xCE79, BAR_BG);
    LCD_DisplayStringAt(SCORE_X, LBL_Y, "SCORE", LEFT_MODE);
    LCD_DisplayStringAt(BEST_X,  LBL_Y, "BEST",  LEFT_MODE);

    Draw_Buttons(0);
    Draw_Values();

    LCD_SetTextColor(0x94B2);
    LCD_FillRect(GRID_X, GRID_Y, GRID_SIDE, GRID_SIDE);
}

static void Draw_Message(const char *msg, uint16_t bg)
{
    uint16_t msg_w = (uint16_t)(strlen(msg) * Font16.Width);
    uint16_t box_x = (uint16_t)(GRID_X + (GRID_SIDE - msg_w - 16) / 2);
    uint16_t box_y = (uint16_t)(GRID_Y + (GRID_SIDE - 40) / 2);

    LCD_SetTextColor(bg);
    LCD_FillRect(box_x, box_y, (uint16_t)(msg_w + 16), 40);
    LCD_SetFont(&Font16);
    LCD_SetColors(WHITE, bg);
    LCD_DisplayStringAt((uint16_t)(box_x + 8), (uint16_t)(box_y + 10),
                        (char *)msg, LEFT_MODE);
}

/* A chunky arrow, composed off-screen and blitted in one go. */
static void Draw_Arrow(uint16_t x, uint16_t y, SwipeDir d,
                       uint16_t col, uint16_t bg)
{
#define AR 28
    int16_t i;

    Gfx_BandBegin(AR, AR, bg);
    switch (d) {
    case SW_UP:
        for (i = 0; i < 12; i++) Gfx_BandRect((int16_t)(13 - i), i, (int16_t)(1 + 2 * i), 1, col);
        Gfx_BandRect(10, 12, 8, 14, col);
        break;
    case SW_DOWN:
        for (i = 0; i < 12; i++) Gfx_BandRect((int16_t)(13 - i), (int16_t)(27 - i), (int16_t)(1 + 2 * i), 1, col);
        Gfx_BandRect(10, 2, 8, 14, col);
        break;
    case SW_LEFT:
        for (i = 0; i < 12; i++) Gfx_BandRect(i, (int16_t)(13 - i), 1, (int16_t)(1 + 2 * i), col);
        Gfx_BandRect(12, 10, 14, 8, col);
        break;
    case SW_RIGHT:
        for (i = 0; i < 12; i++) Gfx_BandRect((int16_t)(27 - i), (int16_t)(13 - i), 1, (int16_t)(1 + 2 * i), col);
        Gfx_BandRect(2, 10, 14, 8, col);
        break;
    default: break;
    }
    Gfx_BandEnd((int16_t)x, (int16_t)y);
#undef AR
}

/* ============================================================
 * input
 * ========================================================== */
static SwipeDir swipe_dir(int16_t dx, int16_t dy)
{
    int16_t adx = dx < 0 ? (int16_t)-dx : dx;
    int16_t ady = dy < 0 ? (int16_t)-dy : dy;

    if (adx < SWIPE_MIN && ady < SWIPE_MIN) return SW_NONE;
    if (adx >= ady) return (dx > 0) ? SW_RIGHT : SW_LEFT;
    return (dy > 0) ? SW_DOWN : SW_UP;
}

static uint8_t apply_move(SwipeDir d)
{
    switch (d) {
    case SW_LEFT:  return Move_Left();
    case SW_RIGHT: return Move_Right();
    case SW_UP:    return Move_Up();
    case SW_DOWN:  return Move_Down();
    default:       return 0;
    }
}

/* One pass of the touch state machine.
 * Returns the swipe that was just completed, or SW_NONE.
 * *tapped is set to 1..2 when the NEW / MENU button was hit.  */
#define REL_DEBOUNCE 3u
static SwipeDir poll_touch(uint8_t *tapped)
{
    static uint8_t relCnt = 0;
    TS_StateTypeDef ts;

    *tapped = 0;
    TS_GetState(&ts);

    if (ts.TouchDetected) {
        relCnt = 0;
        if (!g_was_touching) {
            g_sx = ts.x; g_sy = ts.y; g_was_touching = 1;
            if (UI_HitBox(ts.x, ts.y, BTN_NEW_X, BTN_Y, BTN_W, BTN_H))  *tapped = 1;
            if (UI_HitBox(ts.x, ts.y, BTN_MENU_X, BTN_Y, BTN_W, BTN_H)) *tapped = 2;
        }
        g_ex = ts.x; g_ey = ts.y;
        return SW_NONE;
    }

    if (g_was_touching && ++relCnt >= REL_DEBOUNCE) {
        SwipeDir d;
        g_was_touching = 0;
        relCnt = 0;
        d = swipe_dir((int16_t)((int16_t)g_ex - (int16_t)g_sx),
                      (int16_t)((int16_t)g_ey - (int16_t)g_sy));
        WaitForTouchRelease(5);
        return d;
    }
    return SW_NONE;
}

/* ============================================================
 * the tutorial
 * ========================================================== */
typedef struct {
    const char *l1;
    const char *l2;
    SwipeDir    want;
    uint8_t     preset;        /* 1 = load the merge demo board */
} T2Step;

static const T2Step T2[] = {
    { "SWIPE  LEFT",  "drag your finger across",  SW_LEFT,  0 },
    { "SWIPE  RIGHT", "tiles slide all the way",  SW_RIGHT, 0 },
    { "SWIPE  UP",    "up and down work too",     SW_UP,    0 },
    { "SWIPE  DOWN",  "almost there",             SW_DOWN,  0 },
    { "MERGE 2 AND 2","equal tiles join and score", SW_LEFT, 1 },
};
#define T2_N (sizeof(T2) / sizeof(T2[0]))

static void tut_bar(const T2Step *s, uint16_t bg)
{
    LCD_SetTextColor(bg);
    LCD_FillRect(0, 0, SCR_W, BAR_H);

    Draw_Arrow(6, 5, s->want, WHITE, bg);

    LCD_SetFont(&Font16);
    LCD_SetColors(WHITE, bg);
    LCD_DisplayStringAt(42, 2, (char *)s->l1, LEFT_MODE);
    LCD_SetFont(&Font12);
    LCD_SetColors(0xCE79, bg);
    LCD_DisplayStringAt(42, 22, (char *)s->l2, LEFT_MODE);

    UI_Button(BTN_MENU_X, BTN_Y, BTN_W, BTN_H, "SKIP", 0x39C7, WHITE, &Font12);
}

static void tut_board(uint8_t preset)
{
    uint8_t r, c;
    for (r = 0; r < BOARD_N; r++)
        for (c = 0; c < BOARD_N; c++) g_board[r][c] = 0;

    if (preset) {
        g_board[1][1] = 2;
        g_board[1][2] = 2;
        g_board[2][0] = 4;
    } else {
        g_board[1][1] = 2;
        g_board[2][3] = 4;
    }
    Draw_Board();
}

/* 1 = go on to the real game, 0 = back to the menu */
static uint8_t tutorial_2048(void)
{
    uint8_t step = 0;

    g_score = 0;
    g_was_touching = 0;

    LCD_SetTextColor(0x39C7);
    LCD_FillRect(0, 0, SCR_W, SCR_H);
    LCD_SetTextColor(0x94B2);
    LCD_FillRect(GRID_X, GRID_Y, GRID_SIDE, GRID_SIDE);

    tut_bar(&T2[0], BAR_BG);
    tut_board(T2[0].preset);
    Input_Flush();
    WaitForTouchRelease(5);

    for (;;) {
        uint32_t t0 = msTick;
        uint8_t  tapped;
        SwipeDir d;

        Input_Poll();
        Sound_Update();
        if (Input_Pressed() & BTN_BACK) return 0;

        d = poll_touch(&tapped);
        /* the bar's right-hand button reads SKIP during the tour:
         * it jumps into the game.  KEY1 still leaves for the menu. */
        if (tapped == 2) { WaitForTouchRelease(5); return 1; }

        if (d != SW_NONE) {
            if (d == T2[step].want) {
                uint32_t before = g_score;
                apply_move(d);
                Draw_Board();
                if (T2[step].preset && g_score > before) {
                    char b[24];
                    SFX_Merge();
                    sprintf(b, "2 + 2 = 4   SCORE +%lu",
                            (unsigned long)(g_score - before));
                    LCD_SetTextColor(0x0480);
                    LCD_FillRect(0, 0, SCR_W, BAR_H);
                    UI_TextCentred(SCR_W / 2, 12, b, WHITE, 0x0480, &Font12);
                    App_Wait(1400);
                } else {
                    SFX_Good();
                    LCD_SetTextColor(0x0480);
                    LCD_FillRect(0, 0, SCR_W, BAR_H);
                    UI_TextCentred(SCR_W / 2, 12, "GOOD!", WHITE, 0x0480, &Font16);
                    App_Wait(700);
                }

                step++;
                if (step >= T2_N) break;
                tut_bar(&T2[step], BAR_BG);
                tut_board(T2[step].preset);
            } else {
                /* wrong direction: nudge, do not punish */
                LCD_SetTextColor(0x9000);
                LCD_FillRect(0, 0, SCR_W, BAR_H);
                UI_TextCentred(SCR_W / 2, 12, "OTHER DIRECTION", WHITE, 0x9000, &Font16);
                App_Wait(700);
                tut_bar(&T2[step], BAR_BG);
            }
        }

        while (msTick - t0 < 10u) { }
    }

    LCD_SetTextColor(0x0480);
    LCD_FillRect(0, 0, SCR_W, BAR_H);
    UI_TextCentred(SCR_W / 2, 10, "NOW PLAY - GOAL 2048", WHITE, 0x0480, &Font16);
    SFX_Start();
    App_Wait(1300);
    return 1;
}

/* ============================================================
 * a fresh round
 * ========================================================== */
static void Game_NewRound(void)
{
    uint8_t r, c;
    for (r = 0; r < BOARD_N; r++)
        for (c = 0; c < BOARD_N; c++) g_board[r][c] = 0;

    g_score     = 0;
    g_game_over = 0;
    g_game_won  = 0;

    Add_RandomTile();
    Add_RandomTile();

    Draw_Background();
    Draw_Board();
}

/* ============================================================
 * entry point
 * ========================================================== */
void Game2048_Run(void)
{
    if (!tutorial_2048()) return;

    Game_NewRound();
    Input_Flush();
    WaitForTouchRelease(5);

    for (;;) {
        uint32_t t0 = msTick;
        uint8_t  tapped;
        SwipeDir d;

        Input_Poll();
        Sound_Update();
        if (Input_Pressed() & BTN_BACK) return;

        d = poll_touch(&tapped);

        if (tapped == 1) {
            Draw_Buttons(1);
            SFX_Select();
            App_Wait(120);
            Game_NewRound();
            WaitForTouchRelease(5);
            continue;
        }
        if (tapped == 2) { WaitForTouchRelease(5); return; }

        if (d != SW_NONE && !g_game_over && !g_game_won) {
            uint32_t before = g_score;
            if (apply_move(d)) {
                Add_RandomTile();
                Draw_Board();
                Draw_Values();
                if (g_score > before) SFX_Merge(); else SFX_Select();

                if (!g_game_won && Check_Win()) {
                    g_game_won = 1;
                    SFX_Good();
                    Draw_Message("  You Win! ", 0x051F);
                } else if (Check_GameOver()) {
                    g_game_over = 1;
                    SFX_Hit();
                    Draw_Message(" Game Over ", DARKRED);
                }
            }
        }

        while (msTick - t0 < 10u) { }
    }
}
