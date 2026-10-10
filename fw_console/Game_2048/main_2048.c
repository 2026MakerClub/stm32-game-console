/* ============================================================
 * main_2048.c  ─  2048 Sliding Tile Game
 * Platform : STM32F407  +  ILI9341 LCD (320 × 240, landscape)
 *            Resistive touch screen via SPI
 *
 * How to build:
 *   1. Copy (or rename) an existing course Keil project
 *      (e.g. STOP_STANDBY_9).
 *   2. Remove main_PWR.c from the "flash" build target.
 *   3. Add THIS file to the build target instead.
 *   4. All other files (HardwareSystem_Ini.c, GLCD driver,
 *      touch_module.c, user_defined.c …) remain unchanged.
 *   5. Rebuild All → Download.
 * ============================================================ */

#include "stm32f4xx.h"
#include "stm324xg_lcd_sklin.h"   /* LCD_Init, LCD_Clear, LCD_SetColors … */
#include "user_defined.h"          /* msTick, delay_ms, Bit() …            */
#include "touch_module.h"          /* TS_GetState, TS_StateTypeDef …       */
#include <stdio.h>                 /* sprintf                               */
#include <string.h>                /* strlen                                */

/* ─────────────────────────────────────────────────────────────
 * Section 1 : Hardware initialiser declaration
 * (implemented in User_common/HardwareSystem_Ini.c)
 * ───────────────────────────────────────────────────────────── */
void stm32f4_Hardware_Init(void);
void Default_Calibration(void);   /* loads touch-screen cal coefficients (touch_module.c) */
void Driver_GPIO(void);           /* configures touch SPI GPIO pins     (Driver_SPI.c)    */

/* ─────────────────────────────────────────────────────────────
 * Section 2 : Game-board geometry constants
 *
 *  Screen is 320 (wide) × 240 (tall) pixels.
 *  We leave the top 40 px for score / controls.
 *  The 4×4 grid occupies a 201 × 201 px square, horizontally
 *  centred in the remaining 200 px height.
 *
 *  Cell layout:
 *    x of cell (r,c) = GRID_X + CELL_GAP + c*(CELL_SIZE+CELL_GAP)
 *    y of cell (r,c) = GRID_Y + CELL_GAP + r*(CELL_SIZE+CELL_GAP)
 * ───────────────────────────────────────────────────────────── */
#define BOARD_N      4             /* grid dimension (4 × 4)               */
#define CELL_SIZE    44            /* pixels per cell (square)              */
#define CELL_GAP     5             /* gap between cells and outer border    */
/* total grid side = 4*44 + 5*5 = 201 px */
#define GRID_X       59            /* leftmost pixel of grid (centred: (320-201)/2=59) */
#define GRID_Y       39            /* topmost pixel of grid                 */
#define GRID_SIDE   ((BOARD_N)*(CELL_SIZE)+(BOARD_N+1)*(CELL_GAP))  /* = 201 */

/* "New Game" button in top-right corner */
#define BTN_X        220
#define BTN_Y        4
#define BTN_W        95
#define BTN_H        32

/* Swipe threshold: ignore movements shorter than this */
#define SWIPE_MIN    28            /* pixels                                */

/* ─────────────────────────────────────────────────────────────
 * Section 3 : Tile colour table (RGB565)
 *
 *  Index  = log2(tile_value).  Index 0 = empty cell.
 *  e.g. tile=2   → index 1
 *       tile=4   → index 2
 *       tile=2048 → index 11
 * ───────────────────────────────────────────────────────────── */
static const uint16_t TILE_BG_COLOR[] = {
    0x7BEF,   /* idx 0  : empty  – medium grey                         */
    0xEF5D,   /* idx 1  : 2      – warm light grey                     */
    0xEDB6,   /* idx 2  : 4      – warm tan                            */
    0xFD20,   /* idx 3  : 8      – orange         (= ORANGE)           */
    0xFC80,   /* idx 4  : 16     – deep orange                         */
    0xF800,   /* idx 5  : 32     – red             (= RED)             */
    0xF9E0,   /* idx 6  : 64     – red-orange                          */
    0xFFE0,   /* idx 7  : 128    – yellow          (= YELLOW)          */
    0xFFE0,   /* idx 8  : 256    – yellow                              */
    0x87E0,   /* idx 9  : 512    – light green                         */
    0x07E0,   /* idx 10 : 1024   – green           (= GREEN)           */
    0x051F,   /* idx 11 : 2048   – blue            (= BLUE2)           */
    0x8010,   /* idx 12 : >2048  – magenta                             */
};
#define TILE_COLOR_COUNT  (sizeof(TILE_BG_COLOR)/sizeof(TILE_BG_COLOR[0]))

/* ─────────────────────────────────────────────────────────────
 * Section 4 : Game state variables
 * ───────────────────────────────────────────────────────────── */
static uint16_t g_board[BOARD_N][BOARD_N]; /* tile values; 0 = empty      */
static uint32_t g_score;                   /* current round score          */
static uint32_t g_best;                    /* all-time best (RAM only)     */
static uint8_t  g_game_over;               /* 1 after no valid move exists */
static uint8_t  g_game_won;                /* 1 once a 2048 tile is formed */

/* ─────────────────────────────────────────────────────────────
 * Section 5 : Touch / swipe tracking state
 * ───────────────────────────────────────────────────────────── */
static uint16_t g_sx, g_sy;               /* swipe start (finger down)    */
static uint16_t g_ex, g_ey;               /* swipe end   (last known pos) */
static uint8_t  g_was_touching;           /* previous-frame touch state   */

/* ─────────────────────────────────────────────────────────────
 * Section 6 : Function prototypes
 * ───────────────────────────────────────────────────────────── */
static void     Game_NewRound(void);
static void     Add_RandomTile(void);
static uint8_t  Move_Left(void);
static uint8_t  Move_Right(void);
static uint8_t  Move_Up(void);
static uint8_t  Move_Down(void);
static uint8_t  Check_GameOver(void);
static uint8_t  Check_Win(void);
static void     Draw_Background(void);
static void     Draw_Board(void);
static void     Draw_Tile(uint8_t row, uint8_t col);
static void     Draw_Score(void);
static void     Draw_NewGameBtn(uint8_t pressed);
static void     Draw_Message(const char *msg, uint16_t bg);
static void     Handle_Swipe(int16_t dx, int16_t dy);
static uint8_t  Log2Idx(uint16_t val);
static uint32_t Rand_LCG(void);

/* ============================================================
 * MAIN
 * ============================================================ */
int main(void)
{
    TS_StateTypeDef ts;

    /* ── Hardware + LCD init ── */
    stm32f4_Hardware_Init();
    LCD_Init();
    LCD_DisplayOn();
    Default_Calibration();        /* must be after LCD_Init so LCD_Pixel_Width() is valid */
    Driver_GPIO();                /* enable GPIOB/C/F clocks + configure SPI touch pins   */
    LCD_SetFont(&Font20);

    /* ── Start first game ── */
    Game_NewRound();

    /* ── Main event loop ── */
    /*
     * Why debounce is needed
     * ──────────────────────
     * The resistive touch IRQ pin (PB1) is polled raw by TS_GetState().
     * Without a loop delay the loop runs ~168 000 000 / loop-body cycles
     * per second.  A single noisy sample that returns "not touching" while
     * the finger is still down would:
     *   (a) fire Handle_Swipe immediately with a near-zero delta → ignored,
     *       but g_was_touching resets to 0.
     *   (b) restart g_sx/g_sy from the wrong mid-swipe position on the
     *       very next frame → wrong direction / double-fire.
     *
     * Fix:
     *   1. Enforce a 10 ms loop period so each sample is 10 ms apart.
     *   2. Require REL_DEBOUNCE consecutive no-touch samples before
     *      treating it as a real release (30 ms stable gap).
     *   3. After evaluating the swipe, call WaitForTouchRelease() so the
     *      board-redraw time cannot be mis-read as a new swipe start.
     */
#define REL_DEBOUNCE  3u    /* × 10 ms = 30 ms stable no-touch required */
    uint8_t s_rel_cnt = 0;  /* consecutive no-touch frame counter         */

    while (1)
    {
        uint32_t t0 = msTick;       /* remember loop-start time             */

        TS_GetState(&ts);

        if (ts.TouchDetected)
        {
            s_rel_cnt = 0;           /* any touch resets release debounce    */

            if (!g_was_touching)
            {
                /* ─ Finger just touched down ─ */
                g_sx = ts.x;
                g_sy = ts.y;
                g_was_touching = 1;

                /* Check "New Game" button tap (register on press, not release) */
                if (ts.x >= BTN_X && ts.x <= (BTN_X + BTN_W) &&
                    ts.y >= BTN_Y && ts.y <= (BTN_Y + BTN_H))
                {
                    Draw_NewGameBtn(1);           /* visual feedback: pressed */
                    delay_ms(120);
                    Game_NewRound();
                }
            }
            /* Keep tracking where the finger is */
            g_ex = ts.x;
            g_ey = ts.y;
        }
        else
        {
            if (g_was_touching)
            {
                if (++s_rel_cnt >= REL_DEBOUNCE)
                {
                    /* ─ Release confirmed after REL_DEBOUNCE stable frames ─ */
                    g_was_touching = 0;
                    s_rel_cnt      = 0;
                    Draw_NewGameBtn(0);

                    int16_t dx = (int16_t)g_ex - (int16_t)g_sx;
                    int16_t dy = (int16_t)g_ey - (int16_t)g_sy;

                    if (!g_game_over && !g_game_won)
                        Handle_Swipe(dx, dy);

                    /* Wait until finger is fully gone before next gesture.
                     * Prevents the board-redraw window from being treated
                     * as a new swipe start.                               */
                    WaitForTouchRelease(5);
                }
                /* else: still counting — keep g_was_touching = 1         */
            }
        }

        /* Enforce 10 ms per loop tick (100 Hz touch polling).
         * unsigned subtraction handles msTick wrap correctly.             */
        while (msTick - t0 < 10u);
    }
}

/* ============================================================
 * Section 7 : Game logic
 * ============================================================ */

/*
 * Game_NewRound()
 * ──────────────
 * Resets the board, score and status flags, places two starter
 * tiles, then redraws everything from scratch.
 */
static void Game_NewRound(void)
{
    uint8_t r, c;
    for (r = 0; r < BOARD_N; r++)
        for (c = 0; c < BOARD_N; c++)
            g_board[r][c] = 0;

    g_score     = 0;
    g_game_over = 0;
    g_game_won  = 0;

    Add_RandomTile();
    Add_RandomTile();

    Draw_Background();
    Draw_Board();
    Draw_Score();
    Draw_NewGameBtn(0);
}

/*
 * Add_RandomTile()
 * ────────────────
 * Picks a random empty cell and places a 2 (90 % chance) or
 * a 4 (10 % chance) there.
 */
static void Add_RandomTile(void)
{
    /* Collect empty-cell coordinates */
    uint8_t pos[BOARD_N * BOARD_N][2];  /* [i][0]=row, [i][1]=col */
    uint8_t count = 0, r, c;

    for (r = 0; r < BOARD_N; r++)
        for (c = 0; c < BOARD_N; c++)
            if (g_board[r][c] == 0) {
                pos[count][0] = r;
                pos[count][1] = c;
                count++;
            }

    if (count == 0) return;  /* full board – should not happen here */

    uint8_t idx = (uint8_t)(Rand_LCG() % count);
    g_board[pos[idx][0]][pos[idx][1]] = (Rand_LCG() % 10 < 9) ? 2 : 4;
}

/*
 * Move_Left() / Move_Right() / Move_Up() / Move_Down()
 * ──────────────────────────────────────────────────────
 * Each returns 1 if at least one tile changed position or merged,
 * 0 if the board was already in that shifted state (no move made).
 *
 * Algorithm for Move_Left (others are rotations of the same idea):
 *   For each row:
 *   1. Compact: slide all non-zero tiles to the left.
 *   2. Merge  : if two adjacent tiles are equal, double the left
 *               one and zero the right one, add value to score.
 *   3. Compact again (fill gaps left by merge).
 *   4. Detect if anything changed.
 */
static uint8_t Move_Left(void)
{
    uint8_t moved = 0;
    uint8_t r, c, k;

    for (r = 0; r < BOARD_N; r++)
    {
        uint16_t tmp[BOARD_N] = {0};

        /* Step 1 – compact */
        k = 0;
        for (c = 0; c < BOARD_N; c++)
            if (g_board[r][c]) tmp[k++] = g_board[r][c];

        /* Step 2 – merge */
        for (c = 0; c < BOARD_N - 1; c++) {
            if (tmp[c] && tmp[c] == tmp[c+1]) {
                tmp[c] <<= 1;           /* double (same as *= 2)  */
                g_score += tmp[c];
                if (g_score > g_best) g_best = g_score;
                tmp[c+1] = 0;
            }
        }

        /* Step 3 – compact again */
        uint16_t result[BOARD_N] = {0};
        k = 0;
        for (c = 0; c < BOARD_N; c++)
            if (tmp[c]) result[k++] = tmp[c];

        /* Step 4 – write back & detect change */
        for (c = 0; c < BOARD_N; c++) {
            if (g_board[r][c] != result[c]) moved = 1;
            g_board[r][c] = result[c];
        }
    }
    return moved;
}

static uint8_t Move_Right(void)
{
    uint8_t moved = 0;
    uint8_t r, c, k;

    for (r = 0; r < BOARD_N; r++)
    {
        uint16_t tmp[BOARD_N] = {0};
        k = BOARD_N - 1;
        for (c = BOARD_N; c-- > 0; )           /* iterate right→left */
            if (g_board[r][c]) tmp[k--] = g_board[r][c];

        for (c = BOARD_N - 1; c > 0; c--) {
            if (tmp[c] && tmp[c] == tmp[c-1]) {
                tmp[c] <<= 1;
                g_score += tmp[c];
                if (g_score > g_best) g_best = g_score;
                tmp[c-1] = 0;
            }
        }

        uint16_t result[BOARD_N] = {0};
        k = BOARD_N - 1;
        for (c = BOARD_N; c-- > 0; )
            if (tmp[c]) result[k--] = tmp[c];

        for (c = 0; c < BOARD_N; c++) {
            if (g_board[r][c] != result[c]) moved = 1;
            g_board[r][c] = result[c];
        }
    }
    return moved;
}

static uint8_t Move_Up(void)
{
    uint8_t moved = 0;
    uint8_t r, c, k;

    for (c = 0; c < BOARD_N; c++)
    {
        uint16_t tmp[BOARD_N] = {0};
        k = 0;
        for (r = 0; r < BOARD_N; r++)
            if (g_board[r][c]) tmp[k++] = g_board[r][c];

        for (r = 0; r < BOARD_N - 1; r++) {
            if (tmp[r] && tmp[r] == tmp[r+1]) {
                tmp[r] <<= 1;
                g_score += tmp[r];
                if (g_score > g_best) g_best = g_score;
                tmp[r+1] = 0;
            }
        }

        uint16_t result[BOARD_N] = {0};
        k = 0;
        for (r = 0; r < BOARD_N; r++)
            if (tmp[r]) result[k++] = tmp[r];

        for (r = 0; r < BOARD_N; r++) {
            if (g_board[r][c] != result[r]) moved = 1;
            g_board[r][c] = result[r];
        }
    }
    return moved;
}

static uint8_t Move_Down(void)
{
    uint8_t moved = 0;
    uint8_t r, c, k;

    for (c = 0; c < BOARD_N; c++)
    {
        uint16_t tmp[BOARD_N] = {0};
        k = BOARD_N - 1;
        for (r = BOARD_N; r-- > 0; )
            if (g_board[r][c]) tmp[k--] = g_board[r][c];

        for (r = BOARD_N - 1; r > 0; r--) {
            if (tmp[r] && tmp[r] == tmp[r-1]) {
                tmp[r] <<= 1;
                g_score += tmp[r];
                if (g_score > g_best) g_best = g_score;
                tmp[r-1] = 0;
            }
        }

        uint16_t result[BOARD_N] = {0};
        k = BOARD_N - 1;
        for (r = BOARD_N; r-- > 0; )
            if (tmp[r]) result[k--] = tmp[r];

        for (r = 0; r < BOARD_N; r++) {
            if (g_board[r][c] != result[r]) moved = 1;
            g_board[r][c] = result[r];
        }
    }
    return moved;
}

/*
 * Check_GameOver()
 * ────────────────
 * Returns 1 if no horizontal or vertical adjacent pair is equal
 * AND no empty cell exists (both necessary conditions for a move).
 */
static uint8_t Check_GameOver(void)
{
    uint8_t r, c;
    for (r = 0; r < BOARD_N; r++)
        for (c = 0; c < BOARD_N; c++) {
            if (g_board[r][c] == 0) return 0;
            if (c+1 < BOARD_N && g_board[r][c] == g_board[r][c+1]) return 0;
            if (r+1 < BOARD_N && g_board[r][c] == g_board[r+1][c]) return 0;
        }
    return 1;
}

/*
 * Check_Win()
 * ───────────
 * Returns 1 the first time a cell reaches 2048.
 */
static uint8_t Check_Win(void)
{
    uint8_t r, c;
    for (r = 0; r < BOARD_N; r++)
        for (c = 0; c < BOARD_N; c++)
            if (g_board[r][c] >= 2048) return 1;
    return 0;
}

/* ============================================================
 * Section 8 : Rendering
 * ============================================================ */

/*
 * Draw_Background()
 * ─────────────────
 * Fills the whole screen, then draws the grey grid frame.
 */
static void Draw_Background(void)
{
    LCD_SetTextColor(0x5ACB);         /* screen background – dark slate   */
    LCD_FillRect(0, 0, 320, 240);

    /* Grid frame (one rectangle per cell-gap slot, including outer border) */
    LCD_SetTextColor(0x94B2);         /* grid colour – medium grey-brown  */
    LCD_FillRect(GRID_X, GRID_Y, GRID_SIDE, GRID_SIDE);
}

/*
 * Draw_Board()
 * ────────────
 * Redraws every tile.  Called after each successful move.
 */
static void Draw_Board(void)
{
    uint8_t r, c;
    for (r = 0; r < BOARD_N; r++)
        for (c = 0; c < BOARD_N; c++)
            Draw_Tile(r, c);
}

/*
 * Draw_Tile()
 * ───────────
 * Draws a single cell at grid position (row, col):
 *   1. Fill the cell rectangle with the colour for its value.
 *   2. If the cell is non-empty, centre the number string inside it.
 */
static void Draw_Tile(uint8_t row, uint8_t col)
{
    uint16_t x    = GRID_X + CELL_GAP + col * (CELL_SIZE + CELL_GAP);
    uint16_t y    = GRID_Y + CELL_GAP + row * (CELL_SIZE + CELL_GAP);
    uint16_t val  = g_board[row][col];
    uint8_t  cidx = Log2Idx(val);
    if (cidx >= TILE_COLOR_COUNT) cidx = TILE_COLOR_COUNT - 1;

    uint16_t bg  = TILE_BG_COLOR[cidx];
    uint16_t fg  = (val <= 4) ? DARKGRAY : WHITE;

    /* Fill tile */
    LCD_SetTextColor(bg);
    LCD_FillRect(x, y, CELL_SIZE, CELL_SIZE);

    /* Draw centred number */
    if (val > 0) {
        char buf[8];
        sprintf(buf, "%u", (unsigned)val);
        uint16_t txt_w = (uint16_t)(strlen(buf) * Font20.Width);
        uint16_t txt_x = x + (CELL_SIZE - txt_w)   / 2;
        uint16_t txt_y = y + (CELL_SIZE - Font20.Height) / 2;
        LCD_SetColors(fg, bg);
        LCD_DisplayStringAt(txt_x, txt_y, buf, LEFT_MODE);
    }
}

/*
 * Draw_Score()
 * ────────────
 * Refreshes the score and best-score text in the top bar.
 */
static void Draw_Score(void)
{
    char buf[32];
    LCD_SetColors(WHITE, 0x5ACB);
    sprintf(buf, "Score:%5lu  Best:%5lu", (unsigned long)g_score,
                                          (unsigned long)g_best);
    LCD_DisplayStringAt(2, 10, buf, LEFT_MODE);
}

/*
 * Draw_NewGameBtn()
 * ─────────────────
 * pressed = 0 : normal state (green button)
 * pressed = 1 : highlighted state (dark green, brief visual feedback)
 */
static void Draw_NewGameBtn(uint8_t pressed)
{
    uint16_t bg = pressed ? DARKGREEN : GREEN;
    LCD_SetColors(WHITE, bg);
    LCD_FillRect(BTN_X, BTN_Y, BTN_W, BTN_H);
    LCD_DisplayStringAt(BTN_X + 4, BTN_Y + 6, "New Game", LEFT_MODE);
}

/*
 * Draw_Message()
 * ──────────────
 * Draws a centred message overlay (Game Over / You Win!).
 * Displayed inside the grid area.
 */
static void Draw_Message(const char *msg, uint16_t bg)
{
    uint16_t msg_w = (uint16_t)(strlen(msg) * Font20.Width);
    uint16_t box_x = GRID_X + (GRID_SIDE - msg_w - 16) / 2;
    uint16_t box_y = GRID_Y + (GRID_SIDE - 40)          / 2;

    LCD_SetTextColor(bg);
    LCD_FillRect(box_x, box_y, msg_w + 16, 40);
    LCD_SetColors(WHITE, bg);
    LCD_DisplayStringAt(box_x + 8, box_y + 10, (char *)msg, LEFT_MODE);
}

/* ============================================================
 * Section 9 : Input handling
 * ============================================================ */

/*
 * Handle_Swipe()
 * ──────────────
 * dx = end_x - start_x  (positive = rightward swipe)
 * dy = end_y - start_y  (positive = downward  swipe)
 *
 * The dominant axis (larger absolute delta) determines the direction.
 * Ignored if the largest delta is below SWIPE_MIN.
 */
static void Handle_Swipe(int16_t dx, int16_t dy)
{
    int16_t adx = dx < 0 ? -dx : dx;
    int16_t ady = dy < 0 ? -dy : dy;

    if (adx < SWIPE_MIN && ady < SWIPE_MIN) return;  /* too short */

    uint8_t moved = 0;

    if (adx >= ady) {
        moved = (dx > 0) ? Move_Right() : Move_Left();
    } else {
        moved = (dy > 0) ? Move_Down()  : Move_Up();
    }

    if (moved) {
        Add_RandomTile();
        Draw_Board();
        Draw_Score();

        if (!g_game_won && Check_Win()) {
            g_game_won = 1;
            Draw_Message("  You Win! ", 0x051F);   /* blue overlay */
        }
        if (!g_game_won && Check_GameOver()) {      /* only show GO if not won */
            g_game_over = 1;
            Draw_Message(" Game Over ", DARKRED);
        }
    }
}

/* ============================================================
 * Section 10 : Utility functions
 * ============================================================ */

/*
 * Log2Idx()
 * ─────────
 * Converts a tile value to its colour-table index.
 *   0    → 0
 *   2    → 1  (2^1)
 *   4    → 2  (2^2)
 *   2048 → 11 (2^11)
 */
static uint8_t Log2Idx(uint16_t val)
{
    if (val == 0) return 0;
    uint8_t  idx = 0;
    uint16_t v   = val;
    while (v > 1) { v >>= 1; idx++; }
    return idx;
}

/*
 * Rand_LCG()
 * ──────────
 * Linear-congruential pseudo-random generator.
 * Seeds itself from msTick and g_score so the sequence changes
 * every game, removing any repeating pattern.
 */
static uint32_t g_rand_state = 42;

static uint32_t Rand_LCG(void)
{
    g_rand_state ^= (msTick * 2654435761UL) ^ ((uint32_t)g_score << 7);
    g_rand_state  = g_rand_state * 1664525UL + 1013904223UL;
    return g_rand_state;
}
