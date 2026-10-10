/* ============================================================
 * input.c - register-level buttons + joystick ADC
 *
 * Everything here is deliberately written with raw register
 * access (no HAL), to match the course style.  See the TA notes
 * Unit 01 (GPIO/RCC) and Unit 05 (ADC) for the theory.
 * ============================================================ */
#include "stm32f4xx.h"
#include "input.h"

/* ------------------------------------------------------------
 * 1. Pin numbers
 * ---------------------------------------------------------- */
#define PIN_BTN_UP      1   /* PE1 */
#define PIN_BTN_DOWN    2   /* PE2 */
#define PIN_BTN_LEFT    3   /* PE3 */
#define PIN_BTN_RIGHT   4   /* PE4 */
#define PIN_J1_SW       4   /* PC4 */
#define PIN_J2_SW       5   /* PC5 */
#define PIN_BACK        8   /* PB8, on-board KEY1 */

/* ADC1 regular channels used by the two sticks */
#define CH_J1X         10   /* PC0 */
#define CH_J1Y         11   /* PC1 */
#define CH_J2X         12   /* PC2 */
#define CH_J2Y         13   /* PC3 */

/* ------------------------------------------------------------
 * 2. Joystick thresholds
 *
 * The ADC is 12-bit, so a centred stick reads near 2048 - but
 * never exactly, which is why we measure the real centre at
 * boot instead of trusting the theoretical value.
 *
 * Two thresholds (not one) give hysteresis: once a direction is
 * active it stays active until the stick comes well back, so a
 * stick resting right on the edge cannot chatter.
 * ---------------------------------------------------------- */
#define JOY_ON     700      /* counts away from centre to activate    */
#define JOY_OFF    450      /* counts to fall back to before release  */
#define JOY_DEAD   250      /* analog dead zone, see Input_JoyAxis()   */

/* If an axis feels upside-down in game, flip the matching flag. */
#define JOY_INVERT_Y  0
#define JOY_INVERT_X  0

/* ------------------------------------------------------------
 * 3. Module state
 * ---------------------------------------------------------- */
static uint16_t s_held;         /* debounced level                 */
static uint16_t s_prev;         /* previous level, for edges       */
static uint16_t s_edge;         /* latched press edges             */
static uint16_t s_lastRaw;      /* previous raw sample             */
static uint16_t s_centre[4];    /* measured centre of each axis    */
static uint16_t s_raw[4];       /* last sample of each axis        */
static uint32_t s_adcFaults;    /* conversions that never finished */

/* ------------------------------------------------------------
 * 4. Low-level ADC
 * ---------------------------------------------------------- */
uint16_t Input_AdcRaw(uint8_t ch)
{
    /* A conversion takes (112 + 12) ADC cycles at 21 MHz, about 6 us,
     * i.e. a few hundred CPU cycles.  The bounded wait is not there
     * for timing - it is there so that a mis-configured ADC makes the
     * joystick read 0 instead of freezing the whole console.        */
    uint32_t guard = 200000;

    ADC1->SQR3 = ch;                 /* 1st (and only) conversion = ch  */
    ADC1->SR  &= ~(1UL << 1);        /* clear EOC                       */
    ADC1->CR2 |=  (1UL << 30);       /* SWSTART: begin conversion       */

    while ((ADC1->SR & (1UL << 1)) == 0) {
        if (--guard == 0) { s_adcFaults++; return 0; }
    }
    return (uint16_t)(ADC1->DR & 0x0FFF);
}

/* Average several samples - a single reading of a 10k pot is noisy. */
static uint16_t adc_avg(uint8_t ch, uint8_t n)
{
    uint32_t sum = 0;
    uint8_t  i;
    for (i = 0; i < n; i++) sum += Input_AdcRaw(ch);
    return (uint16_t)(sum / n);
}

/* ------------------------------------------------------------
 * 5. Initialisation
 * ---------------------------------------------------------- */
static void gpio_input_pullup(GPIO_TypeDef *port, uint8_t pin)
{
    port->MODER &= ~(3UL << (pin * 2));                  /* 00 = input   */
    port->PUPDR  = (port->PUPDR & ~(3UL << (pin * 2)))
                 | (1UL << (pin * 2));                   /* 01 = pull-up */
}

static void gpio_analog(GPIO_TypeDef *port, uint8_t pin)
{
    port->MODER |=  (3UL << (pin * 2));                  /* 11 = analog  */
    port->PUPDR &= ~(3UL << (pin * 2));                  /* 00 = no pull */
}

void Input_Init(void)
{
    /* --- clocks: GPIOB(1), GPIOC(2), GPIOE(4) on AHB1 ------------- */
    RCC->AHB1ENR |= (1UL << 1) | (1UL << 2) | (1UL << 4);

    /* --- four direction buttons on port E ------------------------- */
    gpio_input_pullup(GPIOE, PIN_BTN_UP);
    gpio_input_pullup(GPIOE, PIN_BTN_DOWN);
    gpio_input_pullup(GPIOE, PIN_BTN_LEFT);
    gpio_input_pullup(GPIOE, PIN_BTN_RIGHT);

    /* --- joystick push-switches ----------------------------------- */
    gpio_input_pullup(GPIOC, PIN_J1_SW);
    gpio_input_pullup(GPIOC, PIN_J2_SW);

    /* --- on-board KEY1 (also done by Driver_GPIO, harmless) ------- */
    gpio_input_pullup(GPIOB, PIN_BACK);

    /* --- joystick axes as analog inputs --------------------------- */
    gpio_analog(GPIOC, 0);
    gpio_analog(GPIOC, 1);
    gpio_analog(GPIOC, 2);
    gpio_analog(GPIOC, 3);

    /* --- ADC1 ------------------------------------------------------
     * APB2 runs at 84 MHz; the ADC must not exceed 36 MHz, so the
     * common prescaler divides by 4 -> 21 MHz.
     * A 10k potentiometer is a high-impedance source, so we also
     * pick a long sample time (112 cycles) to let the internal
     * sample-and-hold capacitor charge fully.
     * -------------------------------------------------------------- */
    RCC->APB2ENR |= (1UL << 8);            /* ADC1EN                    */

    ADC->CCR = (ADC->CCR & ~(3UL << 16))
             | (1UL << 16);                /* ADCPRE = 01 -> PCLK2 / 4  */

    /* SMPR1 holds channels 10..18, 3 bits each.
     * ch10 -> bits [2:0], ch11 -> [5:3], ch12 -> [8:6], ch13 -> [11:9]
     * value 101b = 112 cycles                                          */
    ADC1->SMPR1 = (ADC1->SMPR1 & ~0x00000FFFUL)
                | (5UL << 0) | (5UL << 3) | (5UL << 6) | (5UL << 9);

    ADC1->SQR1 = 0;          /* L = 0000 -> sequence length of 1        */
    ADC1->CR1  = 0;          /* 12-bit, no scan, no interrupts          */
    ADC1->CR2  = 0;
    ADC1->CR2 |= (1UL << 0); /* ADON: wake the ADC up                   */

    /* the analog block needs a moment to stabilise after ADON */
    { volatile uint32_t d = 20000; while (d--) { } }

    Input_Recalibrate();

    s_held = s_prev = s_edge = s_lastRaw = 0;
}

void Input_Recalibrate(void)
{
    /* Whatever the sticks read right now is defined as "centre".
     * Do not touch the sticks while the console boots.               */
    s_centre[0] = adc_avg(CH_J1X, 16);
    s_centre[1] = adc_avg(CH_J1Y, 16);
    s_centre[2] = adc_avg(CH_J2X, 16);
    s_centre[3] = adc_avg(CH_J2Y, 16);
}

void Input_JoyCentre(uint16_t *c0, uint16_t *c1, uint16_t *c2, uint16_t *c3)
{
    if (c0) *c0 = s_centre[0];
    if (c1) *c1 = s_centre[1];
    if (c2) *c2 = s_centre[2];
    if (c3) *c3 = s_centre[3];
}

/* ------------------------------------------------------------
 * 6. Per-axis decode with hysteresis
 *
 * was = the direction bits that were active last tick, so an
 * axis that is already deflected uses the lower release
 * threshold instead of the higher activate threshold.
 * ---------------------------------------------------------- */
static uint16_t axis_bits(uint16_t raw, uint16_t centre,
                          uint16_t negBit, uint16_t posBit,
                          uint16_t was, uint8_t invert)
{
    int32_t  d   = (int32_t)raw - (int32_t)centre;
    uint16_t out = 0;

    if (invert) d = -d;

    if ((was & posBit) && d >  JOY_OFF) out |= posBit;
    else if (d >  JOY_ON)               out |= posBit;

    if ((was & negBit) && d < -JOY_OFF) out |= negBit;
    else if (d < -JOY_ON)               out |= negBit;

    return out;
}

/* ------------------------------------------------------------
 * 7. Poll
 * ---------------------------------------------------------- */
void Input_Poll(void)
{
    uint16_t raw = 0;
    uint16_t agree;
    uint32_t e = GPIOE->IDR;
    uint32_t c = GPIOC->IDR;
    uint32_t b = GPIOB->IDR;

    /* buttons are wired to ground, so a pressed button reads 0 */
    if (!(e & (1UL << PIN_BTN_UP)))    raw |= BTN_UP;
    if (!(e & (1UL << PIN_BTN_DOWN)))  raw |= BTN_DOWN;
    if (!(e & (1UL << PIN_BTN_LEFT)))  raw |= BTN_LEFT;
    if (!(e & (1UL << PIN_BTN_RIGHT))) raw |= BTN_RIGHT;
    if (!(c & (1UL << PIN_J1_SW)))     raw |= BTN_J1SW;
    if (!(c & (1UL << PIN_J2_SW)))     raw |= BTN_J2SW;
    if (!(b & (1UL << PIN_BACK)))      raw |= BTN_BACK;

    /* --- sticks ------------------------------------------------- */
    s_raw[0] = Input_AdcRaw(CH_J1X);
    s_raw[1] = Input_AdcRaw(CH_J1Y);
    s_raw[2] = Input_AdcRaw(CH_J2X);
    s_raw[3] = Input_AdcRaw(CH_J2Y);

    raw |= axis_bits(s_raw[0], s_centre[0], J1_LEFT, J1_RIGHT, s_held, JOY_INVERT_X);
    raw |= axis_bits(s_raw[1], s_centre[1], J1_DOWN, J1_UP,    s_held, JOY_INVERT_Y);
    raw |= axis_bits(s_raw[2], s_centre[2], J2_LEFT, J2_RIGHT, s_held, JOY_INVERT_X);
    raw |= axis_bits(s_raw[3], s_centre[3], J2_DOWN, J2_UP,    s_held, JOY_INVERT_Y);

    /* --- debounce -----------------------------------------------
     * A bit may only change s_held if two consecutive samples
     * agreed about it.  At a 10 ms tick that is a 20 ms filter,
     * which is longer than tactile-switch bounce.
     * ------------------------------------------------------------ */
    agree     = (uint16_t)~(raw ^ s_lastRaw);
    s_held    = (uint16_t)((s_held & ~agree) | (raw & agree));
    s_lastRaw = raw;

    s_edge |= (uint16_t)(s_held & ~s_prev);
    s_prev  = s_held;
}

/* ------------------------------------------------------------
 * Analog axis
 *
 * Subtract the dead zone rather than just zeroing inside it,
 * otherwise the stick would jump from 0 to a large value the
 * instant it leaves the dead zone.  This way the output starts
 * from 0 at the edge of the dead zone and rises smoothly.
 * ---------------------------------------------------------- */
int16_t Input_JoyAxis(uint8_t idx)
{
    int32_t d;

    if (idx > 3) return 0;

    d = (int32_t)s_raw[idx] - (int32_t)s_centre[idx];
    if (idx & 1) { if (JOY_INVERT_Y) d = -d; }
    else         { if (JOY_INVERT_X) d = -d; }

    if (d > -JOY_DEAD && d < JOY_DEAD) return 0;
    d = (d > 0) ? (d - JOY_DEAD) : (d + JOY_DEAD);

    d = (d * 512) / (2048 - JOY_DEAD);
    if (d >  511) d =  511;
    if (d < -512) d = -512;
    return (int16_t)d;
}

uint16_t Input_Held(void) { return s_held; }

uint16_t Input_Pressed(void)
{
    uint16_t e = s_edge;
    s_edge = 0;
    return e;
}

void Input_Flush(void)
{
    s_edge = 0;
}

uint32_t Input_AdcFaults(void) { return s_adcFaults; }

/* ------------------------------------------------------------
 * Bring-up diagnostics
 * ---------------------------------------------------------- */
static void pullup_if_input(GPIO_TypeDef *port, uint8_t pin)
{
    if (((port->MODER >> (pin * 2)) & 3UL) != 0) return;   /* not a plain input */
    port->PUPDR = (port->PUPDR & ~(3UL << (pin * 2))) | (1UL << (pin * 2));
}

void Input_DiagInit(void)
{
    uint8_t i;

    RCC->AHB1ENR |= (1UL << 2) | (1UL << 4);      /* GPIOC, GPIOE */

    for (i = 0; i <= 6;  i++) pullup_if_input(GPIOE, i);
    for (i = 4; i <= 12; i++) pullup_if_input(GPIOC, i);
}

void Input_ScanLow(uint16_t *portE, uint16_t *portC)
{
    uint32_t e = GPIOE->IDR, c = GPIOC->IDR;

    /* a bit set here means "this pin is being pulled LOW" */
    if (portE) *portE = (uint16_t)((~e) & 0x007Fu);            /* PE0..PE6   */
    if (portC) *portC = (uint16_t)((~c) & 0x1FF0u);            /* PC4..PC12  */
}
