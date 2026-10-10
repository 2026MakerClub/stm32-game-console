/* ============================================================
 * console.h - shared declarations for the STM32 handheld console
 *
 * Board  : STM32F407ZGT6 course board
 * Display: on-board ILI9341 via FSMC, 320 x 240 landscape
 * Input  : 4 tactile buttons, 2 analog joysticks, resistive touch
 * Audio  : 4 ohm speaker on TIM3_CH1 (PA6) through a 220 ohm resistor
 * ============================================================ */
#ifndef __CONSOLE_H
#define __CONSOLE_H

#include "stm32f4xx.h"
#include "stm324xg_lcd_sklin.h"
#include "user_defined.h"
#include "touch_module.h"
#include <stdint.h>

/* ---- screen geometry ------------------------------------- */
#define SCR_W   320
#define SCR_H   240

/* ---- which screen the console is currently showing -------- */
typedef enum {
    APP_MENU = 0,
    APP_DINO,
    APP_2048,
    APP_GATE,
    APP_IOTEST
} AppScreen;

/* Each of these runs until the player backs out, then returns. */
AppScreen Menu_Run(void);
void      Dino_Run(void);
void      Game2048_Run(void);
void      Gate_Run(void);
void      IoTest_Run(void);

/* ---- small shared helpers (ui.c) -------------------------- */
void UI_Button(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
               const char *label, uint16_t bg, uint16_t fg, sFONT *font);
uint8_t UI_HitBox(uint16_t tx, uint16_t ty,
                  uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void UI_TextCentred(uint16_t cx, uint16_t y, const char *s,
                    uint16_t fg, uint16_t bg, sFONT *font);
void UI_Banner(const char *line1, const char *line2,
               uint16_t bg, uint16_t fg);

/* Blocking wait that keeps the sound engine ticking.  Always use
 * this instead of delay_ms() inside a game, otherwise a note that
 * should have ended keeps ringing for the whole delay.          */
void App_Wait(uint32_t ms);

#endif /* __CONSOLE_H */
