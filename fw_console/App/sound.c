/* ============================================================
 * sound.c - TIM3_CH1 (PA6) PWM square-wave generator
 * ============================================================ */
#include "stm32f4xx.h"
#include "user_defined.h"      /* msTick */
#include "sound.h"

#define PIN_SPK   6            /* PA6 = TIM3_CH1, alternate function 2 */
#define AF_TIM3   2

/* computed by stm32f4_Hardware_Init() in HardwareSystem_Ini.c */
extern uint32_t PCLK1_Freq;

static uint32_t s_timClk;          /* TIM3 counter clock, Hz            */
static uint8_t  s_muted;
static uint32_t s_noteEnd;         /* msTick value when the note stops  */
static const Note *s_seq;          /* melody being played, or 0         */
static uint8_t  s_seqLen, s_seqIdx;
static uint16_t s_lastFreq;        /* last pitch actually programmed */

/* ------------------------------------------------------------
 * How fast does TIM3 actually count?
 *
 * On the F4, an APB1 timer is fed PCLK1 when the APB1 prescaler
 * is 1, and 2 x PCLK1 for every other prescaler value.  On the
 * usual 168 MHz course clock tree that means 42 MHz x 2 = 84 MHz.
 * ---------------------------------------------------------- */
static uint32_t timer_clock(void)
{
    uint32_t ppre1 = (RCC->CFGR >> 10) & 0x7;   /* bits 12:10 */
    return (ppre1 < 4) ? PCLK1_Freq : (PCLK1_Freq * 2);
}

void Sound_Init(void)
{
    /* --- PA6 to alternate function 2 ---------------------------- */
    RCC->AHB1ENR |= (1UL << 0);                         /* GPIOA clock */

    GPIOA->MODER   = (GPIOA->MODER   & ~(3UL << (PIN_SPK * 2)))
                   | (2UL << (PIN_SPK * 2));            /* 10 = AF     */
    GPIOA->OTYPER &= ~(1UL << PIN_SPK);                 /* push-pull   */
    GPIOA->OSPEEDR = (GPIOA->OSPEEDR & ~(3UL << (PIN_SPK * 2)))
                   | (1UL << (PIN_SPK * 2));            /* medium      */
    GPIOA->PUPDR  &= ~(3UL << (PIN_SPK * 2));           /* no pull     */
    GPIOA->AFR[0]  = (GPIOA->AFR[0]  & ~(0xFUL << (PIN_SPK * 4)))
                   | ((uint32_t)AF_TIM3 << (PIN_SPK * 4));

    /* --- TIM3 --------------------------------------------------- */
    RCC->APB1ENR |= (1UL << 1);                         /* TIM3EN      */

    s_timClk = timer_clock();

    TIM3->CR1  = 0;
    TIM3->CCMR1 = (6UL << 4)      /* OC1M = 110 -> PWM mode 1          */
                | (1UL << 3);     /* OC1PE: preload CCR1               */
    TIM3->CCER  = 0;              /* channel stays off until a note    */
    TIM3->CR1  |= (1UL << 7);     /* ARPE: preload ARR                 */

    s_muted   = 0;
    s_seq     = 0;
    s_noteEnd = 0;
}

/* ------------------------------------------------------------
 * Pick a prescaler/reload pair that lands on the wanted pitch.
 *
 *   f = timClk / ((PSC + 1) * (ARR + 1))
 *
 * ARR is only 16 bits, so divide the total count down with PSC
 * until the remainder fits.
 * ---------------------------------------------------------- */
static void tone_on(uint16_t freq)
{
    uint32_t total, psc, arr;

    s_lastFreq = freq;
    if (freq == 0 || s_muted) { Sound_Stop(); return; }

    total = s_timClk / freq;
    if (total == 0) { Sound_Stop(); return; }

    psc = (total + 65535UL) / 65536UL;     /* smallest PSC+1 that fits  */
    if (psc == 0) psc = 1;
    arr = total / psc;
    if (arr == 0) arr = 1;
    if (arr > 65536UL) arr = 65536UL;

    TIM3->PSC  = (uint16_t)(psc - 1);
    TIM3->ARR  = (uint16_t)(arr - 1);
    TIM3->CCR1 = (uint16_t)(arr / 2);      /* 50% duty = square wave    */
    TIM3->EGR  = 1UL;                      /* UG: load the new values   */
    TIM3->CCER |= (1UL << 0);              /* CC1E: drive the pin       */
    TIM3->CR1  |= (1UL << 0);              /* CEN: run                  */

    /* hand PA6 back to the timer */
    GPIOA->MODER = (GPIOA->MODER & ~(3UL << (PIN_SPK * 2)))
                 | (2UL << (PIN_SPK * 2));           /* 10 = AF */
}

void Sound_Stop(void)
{
    TIM3->CCER &= ~(1UL << 0);             /* release the compare output */
    TIM3->CR1  &= ~(1UL << 0);             /* stop counting              */

    /* Take the pin back off the timer and hold it low.  Leaving it
     * on an idle alternate function can park it high, which would
     * push a steady 15 mA through the speaker for no sound at all. */
    GPIOA->MODER = (GPIOA->MODER & ~(3UL << (PIN_SPK * 2)))
                 | (1UL << (PIN_SPK * 2));           /* 01 = output */
    GPIOA->BSRR  = (1UL << (PIN_SPK + 16));          /* drive low   */
}

void Sound_DC(uint8_t on)
{
    TIM3->CCER &= ~(1UL << 0);          /* let go of the compare output */
    TIM3->CR1  &= ~(1UL << 0);          /* stop the timer               */

    GPIOA->MODER = (GPIOA->MODER & ~(3UL << (PIN_SPK * 2)))
                 | (1UL << (PIN_SPK * 2));            /* 01 = output    */
    GPIOA->BSRR  = on ? (1UL << PIN_SPK) : (1UL << (PIN_SPK + 16));
}

/* msTick is a free-running ms counter; a note end of exactly 0 is
 * used as the "nothing playing" marker, so nudge past it. */
static void set_end(uint32_t when)
{
    s_noteEnd = when ? when : 1u;
}

void Sound_Tone(uint16_t freq, uint16_t ms)
{
    s_seq = 0;                             /* a single tone cancels a melody */
    tone_on(freq);
    set_end(msTick + ms);
}

void Sound_Melody(const Note *notes, uint8_t count)
{
    if (!notes || count == 0) return;
    s_seq    = notes;
    s_seqLen = count;
    s_seqIdx = 0;
    tone_on(notes[0].freq);
    set_end(msTick + notes[0].ms);
}

void Sound_Update(void)
{
    if (s_noteEnd == 0) return;
    if ((int32_t)(msTick - s_noteEnd) < 0) return;   /* still playing */

    if (s_seq && (s_seqIdx + 1) < s_seqLen) {
        s_seqIdx++;
        tone_on(s_seq[s_seqIdx].freq);
        set_end(msTick + s_seq[s_seqIdx].ms);
    } else {
        Sound_Stop();
        s_seq     = 0;
        s_noteEnd = 0;
    }
}

void Sound_Mute(uint8_t on)
{
    s_muted = on ? 1 : 0;
    if (s_muted) { Sound_Stop(); s_seq = 0; s_noteEnd = 0; }
}

uint8_t Sound_IsMuted(void) { return s_muted; }

void Sound_Debug(uint32_t *timClk, uint16_t *psc, uint16_t *arr, uint16_t *freq)
{
    if (timClk) *timClk = s_timClk;
    if (psc)    *psc    = (uint16_t)TIM3->PSC;
    if (arr)    *arr    = (uint16_t)TIM3->ARR;
    if (freq)   *freq   = s_lastFreq;
}

/* ------------------------------------------------------------
 * Canned effects
 * ---------------------------------------------------------- */
static const Note MEL_JUMP[]   = { {NOTE_G5, 40}, {NOTE_C6, 55} };
static const Note MEL_DUCK[]   = { {NOTE_E4, 50}, {NOTE_C4, 55} };
static const Note MEL_POINT[]  = { {NOTE_E6, 35}, {NOTE_G6, 45} };
static const Note MEL_MERGE[]  = { {NOTE_C5, 40}, {NOTE_E5, 40}, {NOTE_G5, 60} };
static const Note MEL_GOOD[]   = { {NOTE_C5, 60}, {NOTE_E5, 60}, {NOTE_G5, 60},
                                   {NOTE_C6, 110} };
static const Note MEL_HIT[]    = { {NOTE_G4, 70}, {NOTE_E4, 70}, {NOTE_C4, 160} };
static const Note MEL_SELECT[] = { {NOTE_C5, 30}, {NOTE_G5, 45} };
static const Note MEL_START[]  = { {NOTE_C5, 90}, {NOTE_REST, 40}, {NOTE_C5, 90},
                                   {NOTE_REST, 40}, {NOTE_C6, 180} };

#define PLAY(m)  Sound_Melody((m), (uint8_t)(sizeof(m)/sizeof((m)[0])))

void SFX_Jump(void)   { PLAY(MEL_JUMP);   }
void SFX_Duck(void)   { PLAY(MEL_DUCK);   }
void SFX_Point(void)  { PLAY(MEL_POINT);  }
void SFX_Merge(void)  { PLAY(MEL_MERGE);  }
void SFX_Good(void)   { PLAY(MEL_GOOD);   }
void SFX_Hit(void)    { PLAY(MEL_HIT);    }
void SFX_Select(void) { PLAY(MEL_SELECT); }
void SFX_Start(void)  { PLAY(MEL_START);  }
