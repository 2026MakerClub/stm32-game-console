# STM32 Arcade carrier PCB

A handheld-shaped carrier board for the course **mcudev STM32F4XX_Pro
(STM32F407ZGT6, "DevEBox")** and its 3.2" TFT. The DEB (with the TFT still
plugged into its New‑TFT header) pushes into two female headers on the
**back** of the carrier, and the TFT shows through a window. Pull it straight
out and the DEB is a plain dev board again.

```
 front ─┬─ TFT glass (sticks ~3 mm out of the window)
        ├─ carrier PCB 1.6 mm   ← this board: buttons, stick, buzzer on the front
        │    └─ 2× 2x30 female header, 8.5 mm (soldered on the BACK)
        ├─ ~11 mm gap           (DEB male pin plastic 2.5 + socket 8.5)
 back  ─┴─ DEB, component side facing the carrier; optional M3×11 standoffs
```

Pin map is the one in `fw_console/PINMAP.md`, so **no firmware change is needed**:

| Function | DEB pin | Carrier part |
|---|---|---|
| UP / DOWN / LEFT / RIGHT | PE1 / PE2 / PE3 / PE4 (J1 pins 1, 4, 3, 6) | SW1–SW4 (D-pad) |
| Stick X / Y | PC0 / PC1 (J1 19, 22) | U1 pots, powered from **3V3** |
| Stick push | PC4 (J1 31) | U1 switch |
| A (confirm) | PC5 (J1 34), the firmware's "stick 2 push" | SW5 |
| BACK | PB8 (J2 2), parallel with the DEB's KEY1 | SW6 |
| Sound | PA6 / TIM3_CH1 (J1 29) | Q1 S8050 → BZ1 buzzer and/or J3 speaker |
| Power | 3V3 (J1 59/60), 5V (J2 59), GND (J1 55, J2 58/60) | — |

## Files

| Path | What |
|---|---|
| `stm32_console_carrier/stm32_console_carrier.kicad_pro` | Open this in KiCad 10 |
| `…/stm32_console_carrier_gerbers.zip` | Gerbers and drill files. Upload as-is to JLCPCB/PCBWay |
| `…/stm32_console_carrier_schematic.pdf` | Schematic |
| `…/bom.csv` | Bill of materials |
| `…/render_top.png`, `render_bottom.png` | 3D renders |
| `…/fit_1to1.pdf` | 1:1 print for a paper test-fit |
| `…/erc.rpt`, `drc.rpt` | KiCad's own checks |
| `gen/` | The generator. Edit `gen/design.py`, then rerun |

Regenerate everything (about 5 minutes, mostly the autorouter):

```
"C:\Program Files\KiCad\10.0\bin\python.exe" hw\gen\build.py
```

Current check status: **ERC 0 errors, 0 warnings. DRC: 0 errors, 0
unconnected pads, schematic and PCB in sync.** One warning remains: one
thermal spoke on R2's GND pad is starved on F.Cu, but the pad is fully
connected on B.Cu.

## Measure before ordering

These numbers came from photos and from the stm32-base.org board page. **The
TFT size is a placeholder.** All of them are at the top of `gen/design.py`.

| # | Measure | Parameter | Current value |
|---|---|---|---|
| 1 | TFT module **PCB** (not the glass): length × width | `TFT_PCB_W`, `TFT_PCB_H` | 92 × 57 mm (guess) |
| 2 | With the TFT plugged in: TFT PCB centre vs DEB centre | `TFT_DX`, `TFT_DY` | 0, 0 |
| 3 | Outer pin row of header 1 to outer pin row of header 2, centre to centre | `HDR_OUTER_FROM_EDGE` = (74 − this) / 2 | 71.12 mm |
| 4 | Do the two headers' columns line up? (If not, the offset) | `HDR2_COL_SHIFT` | 0 |
| 5 | DEB centre to pin 1's column | `HDR_COL1_FROM_CENTRE` | 36.83 (header centred) |
| 6 | M3 hole centres, inset from the DEB corners | `DEB_HOLE_INSET_X/Y` | 3.5 mm |
| 7 | RST button centre from the DEB's left (USB) and top (header 1) edges | `DEB_RST_U/V` | 99.1, 7.9 mm |
| 8 | DEB top surface to TFT PCB underside (≈ carrier height) | (check only) | ~11 mm |

**The TFT must fit between the two header rows**, i.e. less than about 62 mm
across. If yours is wider, the carrier can't sit there. Tell me and the
mechanical concept changes.

Also check before soldering:

- **Joystick (U1):** the footprint is SparkFun's PS2 thumbstick pattern. Pots
  are on a 2.54 mm pitch, 10.16 mm from the centre, and the mounting tabs are at
  ±7.62 / ±6.67 mm. Lay your bare stick on a 1:1 print (see below) first.
- **Buzzer (BZ1):** pin 2 is a slot, so both 6.5 mm and 7.6 mm pitches fit.
  Use a **passive** buzzer, so melodies work.
- **Q1:** S8050 pinout is **E‑B‑C** with the flat side facing you. 2N2222A
  is **E‑B‑C** too in TO‑92, but check your batch.

**Cheapest sanity check:** print `stm32_console_carrier/fit_1to1.pdf` at **100 %**
(A4 landscape). Poke the DEB's pins through the paper and lay the TFT and the bare
stick on it, then confirm both header rows, the window and the stick pads line up.

## Assembly notes

1. Solder the **two 2x30 female headers on the BACK** (the side with the
   white DEB outline). Plug them onto the DEB first and solder with the DEB
   in place, so they come out perfectly aligned. Then unplug the DEB.
2. Front side: Q1, R1–R4, D1 (band = cathode = pin 1), C1 (+ marked), C2,
   buzzer (+ marked), switches, joystick.
3. **Trim all leads flush on the back of the left (D-pad) area.** The DEB's
   20‑pin JTAG/IDC plug and mini‑USB plugs come in underneath it. The IDC
   plug leaves only ~1.5 mm under the carrier.
4. Optional speaker: wire an 8 Ω 0.5 W speaker to J3 (`+` marked on the back)
   and glue it to the back, right side. Buzzer and speaker can both be fitted.
5. Optional: four M3 × 11 mm standoffs between the DEB's corner holes and
   H1–H4.

Bring-up: same stages as `fw_console/PINMAP.md` §5. The **I/O TEST** screen
shows every button, both stick axes and the sound. If a stick axis is
reversed, flip `JOY_INVERT_X/Y` in `fw_console/App/input.c`.

Things the carrier covers on the DEB: its RST button (poke it through the
notch at the window's top-right corner), K0/K1/WK_UP (BACK replaces K1), the
LEDs, the micro‑SD slot and the RTC battery.

## Ordering

2 layers, 1.6 mm FR‑4, 210 × 96 mm, min track/space 0.25/0.2 mm, min drill
0.3 mm, HASL. The TFT window is a normal routed internal cutout, so no
special option is needed. JLCPCB will price it as a >100 mm board.
