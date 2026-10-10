# Pin map and wiring

## 1. Pins the board already owns — do not touch these

These are not guesses. They are read straight out of the driver sources in
this project:

| Source | Line | Pins |
|---|---|---|
| `GLCD/stm324xg_lcd_ILI9341_Pro.c` | 25–33 | `PNr_FSMC_set1 = 3 (GPIOD), 0xC733` → **PD0, PD1, PD4, PD5, PD8, PD9, PD10, PD14, PD15** |
| | | `PNr_FSMC_set2 = 4 (GPIOE), 0xFF80` → **PE7 … PE15** |
| | | `PNr_FSMC_set3 = 6 (GPIOG), 0x1000` → **PG12** (FSMC_NE4) |
| | | `PNr_FSMC_set4 = 5 (GPIOF), 0x1000` → **PF12** (FSMC_A6, used as the LCD's RS line) |
| | 20–21 | `PNr_backlight = 1 (GPIOB), PIN_bk = 15` → **PB15** (LCD backlight) |
| `SPI_touch/Driver_SPI.c` | 14–22 | **PB0** SCK, **PB1** TP_INT, **PB2** MISO, **PF11** MOSI, **PC13** CS |
| | 30–37 | **PB8** on-board KEY1 |

The touch panel talks to its own SPI controller chip, not to the STM32's ADC,
so there is **no conflict** between the touch screen and the joysticks.

## 2. What this firmware adds

| Function | Pin | Peripheral | Notes |
|---|---|---|---|
| UP button | **PE1** | GPIO in, pull-up | active LOW |
| DOWN button | **PE2** | GPIO in, pull-up | active LOW |
| LEFT button | **PE3** | GPIO in, pull-up | active LOW |
| RIGHT button | **PE4** | GPIO in, pull-up | active LOW |
| Joystick 1 VRx | **PC0** | ADC1_IN10 | analog |
| Joystick 1 VRy | **PC1** | ADC1_IN11 | analog |
| Joystick 2 VRx | **PC2** | ADC1_IN12 | analog |
| Joystick 2 VRy | **PC3** | ADC1_IN13 | analog |
| Joystick 1 SW | **PC4** | GPIO in, pull-up | active LOW |
| Joystick 2 SW | **PC5** | GPIO in, pull-up | active LOW |
| Speaker | **PA6** | TIM3_CH1, AF2 | through 220 Ω |
| Back / menu | PB8 | already wired | on-board KEY1 |

All of these live in one block at the top of `App/input.c` (and `PIN_SPK` in
`App/sound.c`). If a pin turns out to be taken on your particular board,
change it there — nothing else in the firmware hard-codes a pin number.

> **PE2 / PE3 / PE4 caveat.** On some F407ZGT6 dev boards (the Alientek
> Explorer family in particular) those three pins are the on-board KEY0/KEY1/
> KEY2 switches, already pulled up and already switched to ground. If that is
> your board, wiring your own buttons in parallel is harmless — both work. But
> **check before you solder**: open the **I/O TEST** screen and press the
> on-board keys. If the UP/DN/LF/RT boxes light up on their own, the pins are
> shared, and you can either use the on-board keys or move to PE0/PE5/PE6 and
> one more free pin.

## 3. Two rules that protect the hardware

**Power the joysticks from 3V3, not 5V.** A joystick module is two
potentiometers wired as voltage dividers. Feed it 5 V and the wiper output
swings to 5 V, straight into an ADC pin whose absolute maximum is VDDA
(3.3 V). This is the fastest way to kill a pin on this board.

**Use a buzzer on PA6, not a bare speaker.** A 4 ohm speaker wired straight to
a GPIO would ask for 3.3 V / 4 ohm = **825 mA**; an STM32 pin is rated for
**25 mA**. Limiting it with 220 ohm is safe but delivers only about 0.2 mW to
the cone, roughly 45 dB at a metre - audible with your ear next to it and not
much further. Buzzers are resonant and perhaps 20-30 dB louder for the same
current, so they are the right part here.

Which buzzer changes the wiring and what the firmware can do with it:

| | 有源 active | 無源 passive |
|---|---|---|
| Built-in oscillator | yes | no |
| Drive it with | DC, on/off only | a square wave at the note pitch |
| Can play melodies | **no** | **yes** |
| Firmware call | `Sound_DC()` | `Sound_Tone()`, `Sound_Melody()` |

```
active, or piezo passive, or any 3-pin module:
    PA6 ------------------ (+) BUZZER (-) ------- GND

magnetic passive buzzer (16-42 ohm coil):
    PA6 ---[ 100-220R ]--- (+) BUZZER (-) ------- GND
```

A piezo passive buzzer is nearly capacitive and draws about 1 mA, so it needs
no resistor and is the loudest option per milliamp. A magnetic one is a low
resistance coil and does need the series resistor. A 3-pin module has its own
driver transistor on board, so give it VCC, GND and the signal, and no
resistor.

Mount the buzzer on a **2-pin female header** rather than soldering it down, so
active and passive can be swapped while testing.

If a 4 ohm speaker is what you want in the end, it needs a transistor or a
small amp module between the pin and the coil. That is a separate build.

## 4. Suggested build, given you want the parts reusable

You said buttons can be soldered down permanently but the joysticks and
speaker need to come back off. So:

- One small piece of perfboard is the **hub**.
- Solder the **4 tactile buttons** directly onto it, in a + layout.
- Run a **GND rail** and a **3V3 rail** along one edge of the perfboard.
- Solder **one leg of every button to the GND rail**; the other leg gets its
  own signal wire.
- Solder **female pin headers** (or dupont pigtails) for joystick 1,
  joystick 2 and the speaker, so those three plug in and pull out.
- Put the **220 Ω on the perfboard**, in the speaker header's signal leg, so
  the resistor can never be forgotten when the speaker is re-plugged.
- One ribbon of **13 wires** leaves the perfboard for the STM32 headers:

```
  3V3, GND,                        (2)
  PE1, PE2, PE3, PE4,              (4)  buttons
  PC0, PC1, PC2, PC3,              (4)  stick axes
  PC4, PC5,                        (2)  stick push-switches
  PA6                              (1)  speaker
```

Buttons need **no external resistor** — the internal pull-ups are switched on
in `Input_Init()`. Keep your 220 Ω resistors for the speaker.

## 5. Bring-up order — wire one stage, test it, then the next

Do NOT wire all 13 conductors and then power up. Every stage below ends in a
test, so a fault can only be in the thing you just soldered.

**Stage 0 — nothing wired yet.** Flash the firmware. You should get the
splash, then the menu. The LCD and the touch panel need none of your wiring,
so if this fails it is the board or the ST-Link, not you. On the splash, the
four JOY numbers will be garbage or 0 — that is correct with nothing attached.

**Stage 1 — power rails.** GND and 3V3 from the board headers to the two rails
on your perfboard. Check with a multimeter that you read 3.3 V between the
rails *before* anything else touches them. Getting these two backwards is the
one mistake that damages parts.

**Stage 2 — the four buttons.** Common leg of all four to the GND rail, the
other leg of each to PE1/PE2/PE3/PE4. Open **I/O TEST**: each box lights
green while held. No resistors needed — the pull-ups are internal.

**Stage 3 — the speaker.** PA6 through the 220 Ω to speaker +, speaker − to
GND. Back in **I/O TEST**, every button press now also beeps, and the **BEEP**
button plays a short tune. This is the fastest stage to verify, which is why
it comes before the fiddly analog ones.

**Stage 4 — joystick 1.** GND, 3V3 (**not** 5 V), VRx→PC0, VRy→PC1, SW→PC4.
Power-cycle so the centre is measured with the stick attached. The top two
bars in I/O TEST should sit near mid-scale and sweep the full width when you
push the stick. `J1 ----` becomes `J1 U---` etc.

**Stage 5 — joystick 2.** Same again: VRx→PC2, VRy→PC3, SW→PC5. Bottom two
bars.

Then play Gate Keeper — it is the only game that exercises all four buttons
and an analog stick at once, so it is the best end-to-end check.

## 6. Verifying it

Flash the firmware, pick **I/O TEST** in the menu, and check, in this order:

1. **Four bars move** when you waggle the sticks. A bar stuck at 0 or 4095
   means that axis is shorted to GND or 3V3 — the wire is on the wrong pin.
2. **The orange tick** on each bar is where the axis sat at boot. The bar
   should rest on the tick. If it does not, press **CENTRE STICKS**.
3. **`J1 ----` turns into `J1 U---`** etc. when you push a stick far enough.
4. **Each button box lights green** while held, and you hear a beep.
5. **TOUCH x= y=** follows your finger.
6. **BEEP** runs a 2.5 s sweep from 400 Hz to 4 kHz, and the **SPK** line
   shows `SPK 84000k PSC.. ARR.. ..Hz` with the frequency climbing as it goes.

   If those numbers move, the timer is running and any silence is on the board
   side. Confirm with a meter: **PA6 to GND, DC volts, during the sweep, reads
   about 1.6 V** — half of 3.3 V, because that is the average of a 50% square
   wave. A steady 0 V or 3.3 V means the pin is not toggling.

   Then check the speaker end: **resistance across the speaker terminals should
   be 3–4 Ω**. Open circuit means a bad joint or a dead speaker. And
   **measure the series resistor** — 220 Ω is red-red-brown. Red-red-red is
   2.2 kΩ and red-red-orange is 22 kΩ, either of which makes it inaudible.

Hold **KEY1 for 1.5 s** to leave, in case the touch panel is the broken thing.

## 7. Which game uses what

| | buttons | stick 1 | stick 2 | touch |
|---|---|---|---|---|
| Menu | up/down to move | up/down to move | up/down to move | tap an entry |
| Dino Run | UP jump, DOWN duck | up/down also works | up/down also works | SKIP button |
| 2048 | — | — | — | swipe, NEW, MENU |
| Gate Keeper | all four, one per lane | moves the collector | — | SKIP button |
| I/O TEST | all shown live | both axes shown | both axes shown | coordinates shown |

KEY1 (PB8) goes back from anywhere.
