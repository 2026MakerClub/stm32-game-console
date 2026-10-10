# STM32 Arcade — working firmware

A complete, building handheld-console firmware for the course STM32F407ZGT6
board: touch menu, Dino Run with an interactive tutorial, 2048 with an
interactive tutorial, Gate Keeper, and a live I/O test screen.

Open **`stm32_arcade.uvprojx`** in Keil, Rebuild, Download.

Last verified build: **0 errors, 0 warnings**, image **28,528 bytes**
(Code 20,212 + RO-data 8,280 + RW-data 36), against the 32,768-byte MDK-Lite
linker limit — **4,240 bytes spare**. RAM: ZI-data 11,748 bytes.

---

## What it does

**Power on** → splash (shows the four joystick centre readings, so a
mis-wired stick is visible before you even reach the menu) → **menu**.

**Menu** — four entries. Tap one, or move with UP/DOWN and confirm with a
joystick push-switch.

1. **DINO RUN** — tutorial, then the game.
2. **2048** — tutorial, then the game.
3. **GATE KEEPER** — tutorial, then the game.
4. **I/O TEST** — live wiring diagnostic. See `PINMAP.md` §5.

**KEY1 (PB8) always goes back.**

### Dino tutorial

Five steps, each of which you have to actually perform. The dino is live the
whole time, so it reacts while you are being taught:

1. Press **UP** → it jumps
2. Hold **DOWN** → it ducks
3. Push **a stick up** → it jumps
4. Pull **a stick down** → it ducks
5. **Clear one cactus** — miss it and you get "OUCH, TRY AGAIN" and another go

Then 3-2-1 and the run starts. Cacti and birds scroll in, speed ramps from
150 px/s to 400 px/s, score and high score across the top. The high-height
bird has to be ducked under, the low one jumped. A **SKIP** button at the
bottom right jumps straight into the game on repeat plays.

### Gate Keeper

You are a core in the middle of four lanes. Things walk down the lanes towards
you; tap that lane's button to fire a pulse up it. Meanwhile drops appear
around the arena and a collector you steer with **stick 1** has to reach them
before they expire. The four buttons are reflex, the stick is navigation, and
the game is about not being able to give either your full attention.

```
            UP
             |
      LEFT --O-- RIGHT
             |
           DOWN
```

- Each lane has a **260 ms cooldown**, so mashing does not work.
- One pulse kills everything within 9 px of it — line two up for the combo.
- Drops: **yellow** +150, **green** repairs the core, **purple** clears the board.
- Waves get bigger and faster; from wave 3 some enemies are orange and quick.
- Core has 5 HP. A breach also resets your combo to zero.

Three tutorial steps: kill one on the left, kill one above, grab a drop.

**One geometry note worth knowing**, because it looks strange in the code:
the screen is 320 × 240, so the left/right lanes are 148 px long and the
up/down lanes only 92. If everything moved at the same pixel speed, the short
lanes would be brutally harder. So every speed is scaled by its lane's length
(`lane_scale()`) — an enemy always takes the same *time* to walk in, whichever
lane it came from.

### 2048 tutorial

Five steps, with a big arrow showing the direction:
swipe **LEFT**, **RIGHT**, **UP**, **DOWN**, then a rigged board where two 2s
sit side by side so you can watch them become a 4 and the score go up. Swipe
the wrong way and it says "OTHER DIRECTION" rather than punishing you. The
right-hand bar button reads **SKIP** during the tour.

---

## The 2048 overlap bug you asked about — what it was

The original `MCU2026/2048/Game_2048/main_2048.c` drew the top bar as one
string:

```c
sprintf(buf, "Score:%5lu  Best:%5lu", ...);
LCD_DisplayStringAt(2, 10, buf, LEFT_MODE);   /* Font20 = 14 px per char */
```

That string is 23 characters. 23 × 14 = **322 px**, starting at x=2, on a
**320 px** screen — so it ran off the right edge *and* straight through the
"New Game" button, which starts at x=220.

The top bar is now four fixed, non-overlapping boxes
(`App/game_2048.c`, "top bar" section):

```
 x:    6..61       96..151      196..252    258..314
     [ SCORE  ]  [  BEST   ]   [  NEW   ]  [  MENU  ]
       label Font12 (7 px)  +  value Font16 (11 px), 5 digits = 55 px
```

Same fix applied to the tiles: the old code drew every number in Font20, so
`1024` and `2048` were 4 × 14 = 56 px inside a 44 px cell and spilled over
their own edges. `Draw_Tile()` now picks the largest font that fits.

---

## Where things live

```
App/
  app_main.c       boot order, splash, menu -> game loop
  console.h        screen constants, shared UI helpers
  menu.c           home screen
  ui.c             buttons, centred text, banners, App_Wait()
  gfx.c/.h         compose-then-blit sprite renderer (flicker-free)
  dino_sprites.h   GENERATED - 2,050 bytes of 1bpp sprites
  input.c/.h       buttons + joystick ADC, all raw registers
  sound.c/.h       TIM3_CH1 PWM square wave + melody queue
  game_dino.c      tutorial + run + game over
  game_2048.c      tutorial + board logic + fixed layout
  game_gate.c      Gate Keeper: tutorial + waves + game over
  iotest.c         live wiring diagnostic
PINMAP.md          wiring, safety limits, verification checklist
```

Sprites are regenerated from the real Chrome-Dino-Runner PNGs with
`tools/gen_sprites.py`. Edit the `JOBS` table there to change sizes.

---

## Two changes I had to make to the shared code

1. **`GLCD/stm324xg_lcd_ILI9341_Pro.c`: `__Fonts_select` 0x1E → 0x06,
   `__default_Font` 3 → 2.** That drops **Font24** (~6.8 KB of tables) and
   **Font20** (3.8 KB), leaving Font12 and Font16. Without this the firmware
   does not fit: the first build was 32,848 bytes against a 32,768 limit, and
   Gate Keeper added another 4 KB on top. Everything now uses Font16 for
   headings and Font12 for labels. If you ever get a full MDK licence, set it
   back to `0x1E` / `3` and both fonts return.

2. **`main_2048.c` is no longer in the build.** Its `main()` would clash with
   the new one. The file is still on disk under `Game_2048/` for reference,
   along with its teaching document.

3. **The `PWR_group` files are no longer in the build** (`myPower.c`,
   `myRTC.c`, `NVIC_priorities.c`, `stm32f4_adc_Wizard.c`). Nothing in the
   app used them, and `myRTC.c` defines interrupt vectors, so the linker was
   keeping part of it alive. The files are still on disk.

---

## Known limits — deliberate, not oversights

- **Buttons are polled, not EXTI.** Polling at 50–100 Hz with a two-sample
  debounce is the robust choice for a game loop, and it cannot fight the
  EXTI handlers already defined in `User_common/user_defined.c`. Converting
  one button to EXTI is a good exercise (TA notes Unit 02) — the wiring
  already supports it.
- **The high score lives in RAM**, so it resets at power-off. Writing it to
  internal Flash is TA notes Unit 04, and `User_FLASH_SectorErase()` /
  `User_FLASH_beforeWRITE()` in `user_defined.c` are already there for it.
- **Only stick 1 is used in gameplay.** Stick 2 is read, debounced and shown
  in I/O TEST, but no game needs it yet. It is the obvious hook for a
  two-player mode.
- **Joystick axis direction is untested on real hardware.** If up/down or
  left/right feel inverted, flip `JOY_INVERT_Y` / `JOY_INVERT_X` at the top
  of `App/input.c`. That is a one-character change.
- **None of this has run on a board yet** — it builds clean and the geometry
  and collision maths are checked on paper, but the first flash is the first
  real test. Start with I/O TEST.

---

## If something does not work

| Symptom | Look at |
|---|---|
| Blank screen | LCD is untouched from the working 2048 project — suspect power or the ST-Link, not the code |
| Menu appears, buttons dead | I/O TEST → are the boxes lighting? If not, check the GND rail first |
| Stick drifts / auto-scrolls | Do not hold the sticks during boot; the centre is measured then. Or press CENTRE STICKS in I/O TEST |
| Stick is backwards | `JOY_INVERT_X` / `JOY_INVERT_Y` in `App/input.c` |
| No sound | Is the 220 Ω actually in series? Is the speaker on PA6 and GND? Press BEEP in I/O TEST |
| Touch taps land in the wrong place | Touch calibration — run the course's `Touch_CAL` project once |
| Linker "exceeds the maximum" | You are over 32 KB again. Drop Font20 next, or shrink the sprites in `gen_sprites.py` |
