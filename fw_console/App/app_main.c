/* ============================================================
 * app_main.c - power on, show the menu, run the chosen game
 *
 * BOOT ORDER MATTERS
 *   1. stm32f4_Hardware_Init() sets the PLL, SysTick and the
 *      PCLK globals.  Everything below depends on it.
 *   2. LCD_Init() brings up the FSMC and the ILI9341, and is
 *      what makes LCD_Pixel_Width() meaningful.
 *   3. Default_Calibration() loads the touch coefficients, so it
 *      must come after LCD_Init().
 *   4. Driver_GPIO() configures the SPI touch pins and KEY1.
 *   5. Input_Init() adds our buttons and the joystick ADC, and
 *      measures where the sticks are resting - so do not touch
 *      the sticks while the console boots.
 *   6. Sound_Init() needs PCLK1_Freq from step 1.
 * ============================================================ */
#include "console.h"
#include "input.h"
#include "sound.h"
#include "gfx.h"
#include <stdio.h>

void stm32f4_Hardware_Init(void);
void Default_Calibration(void);
void Driver_GPIO(void);

#define SPLASH_BG  0x18E3
#define SPLASH_FG  0x07FF

/* How long the opening screen stays up, in ms.  Any button press
 * after the first second skips it, so it does not get in the way
 * while you are reflashing. */
#define SPLASH_MS  4500

static void splash(void)
{
    char     buf[36];
    uint32_t t0;
    uint16_t lastW = 0;

    LCD_SetTextColor(SPLASH_BG);
    LCD_FillRect(0, 0, SCR_W, SCR_H);

    Gfx_Sprite((SCR_W - SPR_DINO_IDLE.w) / 2, 76, &SPR_DINO_IDLE,
               SPLASH_FG, SPLASH_BG);

    UI_TextCentred(SCR_W / 2, 26, "STM32 ARCADE", SPLASH_FG, SPLASH_BG, &Font16);
    LCD_SetTextColor(SPLASH_FG);
    LCD_FillRect(96, 48, 128, 2);
    UI_TextCentred(SCR_W / 2, 126, "NYCU Maker Club", 0x8410, SPLASH_BG, &Font12);

    /* progress bar track */
    LCD_SetTextColor(0x2124);
    LCD_FillRect(60, 160, 200, 6);

    /* Two polls then a flush, so a button that is stuck LOW
     * because of a wiring fault cannot look like a fresh press
     * and skip the screen instantly. */
    Input_Poll();
    Input_Poll();
    Input_Flush();

    t0 = msTick;
    while (msTick - t0 < SPLASH_MS) {
        uint32_t e = msTick - t0;
        uint16_t w = (uint16_t)((e * 200u) / SPLASH_MS);

        Input_Poll();

        if (w > lastW) {
            LCD_SetTextColor(SPLASH_FG);
            LCD_FillRect((uint16_t)(60 + lastW), 160, (uint16_t)(w - lastW), 6);
            lastW = w;
        }

        /* Live joystick readings, so a dead axis is visible before
         * you even reach the menu.  A stick that is wired and
         * powered rests somewhere near 2000; a reading pinned at 0
         * means that axis has no supply or no wire.            */
        sprintf(buf, "JOY %4u %4u %4u %4u",
                Input_AdcRaw(10), Input_AdcRaw(11),
                Input_AdcRaw(12), Input_AdcRaw(13));
        UI_TextCentred(SCR_W / 2, 186, buf, 0x6B4D, SPLASH_BG, &Font12);

        if (Input_AdcFaults())
            UI_TextCentred(SCR_W / 2, 206, "ADC FAULT", 0xF800, SPLASH_BG, &Font12);

        if (e > 1000u && Input_Pressed()) break;

        delay_ms(40);
    }
}

int main(void)
{
    AppScreen screen;

    stm32f4_Hardware_Init();

    LCD_Init();
    LCD_DisplayOn();
    Default_Calibration();
    Driver_GPIO();

    Input_Init();
    Sound_Init();

    splash();

    for (;;) {
        screen = Menu_Run();

        switch (screen) {
        case APP_DINO: Dino_Run();     break;
        case APP_2048: Game2048_Run(); break;
        case APP_GATE: Gate_Run();     break;
        case APP_IOTEST: IoTest_Run(); break;
        default:                       break;
        }

        Sound_Stop();
    }
}
