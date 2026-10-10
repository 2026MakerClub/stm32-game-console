/* ============================================================
 * input.h - buttons + analog joysticks
 *
 * PIN MAP  (verified free: not used by the FSMC LCD, the SPI
 *           touch panel, or the LCD backlight - see PINMAP.md)
 *
 *   UP     button    PE1   input, internal pull-up, active LOW
 *   DOWN   button    PE2
 *   LEFT   button    PE3
 *   RIGHT  button    PE4
 *   BACK             PB8   on-board KEY1 (already pulled up by Driver_GPIO)
 *
 *   Joystick 1  VRx  PC0   ADC1_IN10
 *               VRy  PC1   ADC1_IN11
 *               SW   PC4   input, internal pull-up, active LOW
 *   Joystick 2  VRx  PC2   ADC1_IN12
 *               VRy  PC3   ADC1_IN13
 *               SW   PC5   input, internal pull-up, active LOW
 *
 *   >>> Power the joystick modules from 3.3V, NOT 5V. <<<
 *   They are potentiometer dividers; on 5V the wiper would drive
 *   5V into an ADC pin whose absolute maximum is VDDA (3.3V).
 * ============================================================ */
#ifndef __INPUT_H
#define __INPUT_H

#include <stdint.h>

/* ---- bit mask returned by Input_Held() / Input_Pressed() ---- */
#define BTN_UP        0x0001u
#define BTN_DOWN      0x0002u
#define BTN_LEFT      0x0004u
#define BTN_RIGHT     0x0008u
#define BTN_J1SW      0x0010u
#define BTN_J2SW      0x0020u
#define BTN_BACK      0x0040u   /* on-board KEY1, PB8 */

#define J1_UP         0x0100u
#define J1_DOWN       0x0200u
#define J1_LEFT       0x0400u
#define J1_RIGHT      0x0800u
#define J2_UP         0x1000u
#define J2_DOWN       0x2000u
#define J2_LEFT       0x4000u
#define J2_RIGHT      0x8000u

/* convenience groups */
#define ANY_UP        (BTN_UP    | J1_UP    | J2_UP)
#define ANY_DOWN      (BTN_DOWN  | J1_DOWN  | J2_DOWN)
#define ANY_LEFT      (BTN_LEFT  | J1_LEFT  | J2_LEFT)
#define ANY_RIGHT     (BTN_RIGHT | J1_RIGHT | J2_RIGHT)

void     Input_Init(void);      /* GPIO + ADC + centre calibration      */
void     Input_Poll(void);      /* call once per ~10 ms game tick       */
uint16_t Input_Held(void);      /* level:  1 = currently down           */
uint16_t Input_Pressed(void);   /* edge:   1 = went down since last read*/
void     Input_Flush(void);     /* discard any pending edges            */

/* ------------------------------------------------------------
 * Analog read, for games that want more than four directions.
 *
 *   idx 0 = J1 X, 1 = J1 Y, 2 = J2 X, 3 = J2 Y
 *
 * Returns the deflection from the measured centre, scaled to
 * -512 .. +511, and exactly 0 inside the dead zone.  Sign follows
 * the direction bits: positive X is J*_RIGHT, positive Y is
 * J*_UP - so screen movement is  vx = +axis,  vy = -axis.
 *
 * Uses the sample taken by the last Input_Poll(), so it costs
 * nothing to call it several times per frame.
 * ---------------------------------------------------------- */
int16_t Input_JoyAxis(uint8_t idx);

/* ------------------------------------------------------------
 * Bring-up diagnostics (used by the I/O TEST screen)
 *
 * Input_DiagInit() switches on the internal pull-up of every
 * candidate button pin that is still a plain input, so a pin
 * with nothing attached reads HIGH instead of floating randomly.
 * After that, Input_ScanLow() reports which pins are being held
 * LOW - i.e. which pin your button is really on.
 *
 * It only ever touches PUPDR, never MODER, and skips any pin
 * something else has already claimed as an output or an
 * alternate function.  So it cannot fight the LCD or the touch
 * panel.
 *
 * Scanned: PE0..PE6 and PC4..PC12.  PE7+ is the FSMC LCD,
 * PC13 is the touch chip select, PC0..PC3 are the analog axes.
 * ---------------------------------------------------------- */
void Input_DiagInit(void);
void Input_ScanLow(uint16_t *portE, uint16_t *portC);

/* Number of ADC conversions that never raised EOC.  Anything
 * other than 0 means the ADC itself is misconfigured, as opposed
 * to the joystick simply not being wired up. */
uint32_t Input_AdcFaults(void);

/* raw helpers, handy while debugging the wiring */
uint16_t Input_AdcRaw(uint8_t ch);          /* ch = 10..13               */
void     Input_JoyCentre(uint16_t *c0, uint16_t *c1,
                         uint16_t *c2, uint16_t *c3);
void     Input_Recalibrate(void);

#endif /* __INPUT_H */
