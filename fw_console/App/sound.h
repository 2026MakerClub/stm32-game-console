/* ============================================================
 * sound.h - square-wave sound effects on a passive speaker
 *
 * WIRING
 *   PA6 (TIM3_CH1) ---[ 220 ohm ]--- speaker + ... speaker - --- GND
 *
 *   The 220 ohm resistor is NOT optional.  A 4 ohm speaker wired
 *   straight to a GPIO pin would ask for 3.3 V / 4 ohm = 825 mA;
 *   an STM32 pin is rated for 25 mA.  220 ohm limits the current
 *   to about 15 mA, which is safe and still clearly audible.
 *   Do not lower it.
 *
 * HOW IT MAKES A NOTE
 *   TIM3 is run as a PWM generator at the note frequency with a
 *   50% duty cycle, i.e. a square wave.  Pitch comes from the
 *   timer period, not the duty cycle.
 * ============================================================ */
#ifndef __SOUND_H
#define __SOUND_H

#include <stdint.h>

/* note frequencies, Hz (equal temperament, A4 = 440) */
#define NOTE_C4   262
#define NOTE_D4   294
#define NOTE_E4   330
#define NOTE_F4   349
#define NOTE_G4   392
#define NOTE_A4   440
#define NOTE_B4   494
#define NOTE_C5   523
#define NOTE_D5   587
#define NOTE_E5   659
#define NOTE_G5   784
#define NOTE_C6   1047
#define NOTE_E6   1319
#define NOTE_G6   1568
#define NOTE_REST 0

typedef struct {
    uint16_t freq;   /* Hz, or NOTE_REST for silence */
    uint16_t ms;     /* duration                      */
} Note;

void Sound_Init(void);

/* Start one note and let it run for ms; non-blocking.
 * Sound_Update() turns it off when the time is up.   */
void Sound_Tone(uint16_t freq, uint16_t ms);

/* Queue a short melody; non-blocking, same rule.      */
void Sound_Melody(const Note *notes, uint8_t count);

/* Call once per game tick.                            */
void Sound_Update(void);

void Sound_Stop(void);

/* Drive the speaker pin as a plain DC output, bypassing the timer.
 *
 * This is what an ACTIVE buzzer (有源蜂鳴器) wants: it contains its
 * own oscillator, so it sounds whenever it has voltage and ignores
 * any waveform you send it.  It is also the simplest possible proof
 * that PA6 can drive anything at all.
 *
 * A PASSIVE buzzer (無源蜂鳴器) or a speaker will click once and
 * then sit silent on DC - those need Sound_Tone().               */
void Sound_DC(uint8_t on);
void Sound_Mute(uint8_t on);
uint8_t Sound_IsMuted(void);

/* What the timer is actually doing right now.  If timClk is not
 * about 84000000 on the standard course clock tree, the problem
 * is the clock setup, not the speaker.  If it is right and the
 * pin still does nothing, the fault is on the board. */
void Sound_Debug(uint32_t *timClk, uint16_t *psc, uint16_t *arr, uint16_t *freq);

/* ready-made effects */
void SFX_Jump(void);
void SFX_Duck(void);
void SFX_Point(void);
void SFX_Merge(void);
void SFX_Good(void);
void SFX_Hit(void);
void SFX_Select(void);
void SFX_Start(void);

#endif /* __SOUND_H */
