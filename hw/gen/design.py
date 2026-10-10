"""
design.py - single source of truth for the STM32 console carrier board.

Everything the schematic and the PCB generator need lives here: board
geometry, where the STM32 board (DEB) and its TFT sit, every part, and
which net every pin is on.  Edit the MEASURED block when you have caliper
numbers and re-run build.py; nothing else hard-codes a dimension.

Coordinates: millimetres, origin at the centre of the carrier, +x to the
right, +y DOWN (KiCad convention), looking at the FRONT of the carrier
(the side with the buttons, the side you hold).

Stack-up, front to back:

    TFT glass  ~2.8 mm  (pokes ~3 mm out of the window)
    carrier PCB 1.6 mm  <- this board; its female headers hang off the back
    ~11 mm gap          (male pin plastic 2.5 + female socket body 8.5)
    DEB (STM32F4XX_Pro), component side facing the carrier

The TFT plugs into the DEB's New-TFT header exactly as it does today, which
puts the TFT's own PCB at the same height as the carrier.  So the carrier
has a window the whole TFT module fits through.
"""

# ---------------------------------------------------------------------------
# MEASURED  - values to confirm with calipers (see hw/README.md, section 2)
# ---------------------------------------------------------------------------

# DEB = mcudev STM32F4XX_Pro (STM32F407ZGT6), seen from its component side,
# USB/JTAG edge on the LEFT, New-TFT edge on the RIGHT.
DEB_W = 102.0           # long edge                         (stm32-base.org)
DEB_H = 74.0            # short edge                        (stm32-base.org)

# Pin headers.  Both 2x30 headers sit on the 0.1" grid; outer rows are
# 71.12 mm apart (28 x 2.54).  Measured from photos - confirm.
HDR_OUTER_FROM_EDGE = 1.44     # long edge -> outer pin row centre
HDR_COL1_FROM_CENTRE = 36.83   # DEB centre -> column 1 (pin 1/2), toward New-TFT.
                               # 36.83 = header centred on the board (29*2.54/2)
HDR2_COL_SHIFT = 0.0           # extra x shift of header 2 vs header 1 (0 = aligned)

# M3 mounting holes of the DEB, centre inset from each corner.
DEB_HOLE_INSET_X = 3.5
DEB_HOLE_INSET_Y = 3.5

# The DEB's RST push button (top-right corner), from the DEB's left/top edge.
DEB_RST_U = 99.1
DEB_RST_V = 7.9

# TFT module PCB outline (NOT the glass) and its centre relative to the DEB
# centre when plugged into New-TFT.  PLACEHOLDER until measured.
TFT_PCB_W = 92.0
TFT_PCB_H = 57.0
TFT_DX = 0.0
TFT_DY = 0.0

# ---------------------------------------------------------------------------
# Carrier geometry
# ---------------------------------------------------------------------------

BOARD_W = 210.0                 # stadium ("pill") outline, Switch-Lite sized
BOARD_H = 96.0
BOARD_R = BOARD_H / 2           # fully rounded ends

DEB_X = 0.0                     # DEB centre on the carrier
DEB_Y = 0.0

WINDOW_CLEAR = 1.0              # gap between TFT PCB and window edge, per side
WINDOW_CORNER_R = 1.5

RST_HOLE_D = 3.0                # poke-hole above the DEB's RST button
M3_HOLE_D = 3.2

# PCB generator origin on the KiCad page
PAGE_OX = 150.0
PAGE_OY = 110.0

# Design rules (JLCPCB/PCBWay standard 2-layer is well inside these)
TRACK_SIG = 0.25
TRACK_3V3 = 0.5
TRACK_PWR = 0.6
CLEARANCE = 0.2
EDGE_CLEAR = 0.35
VIA_D = 0.6
VIA_DRILL = 0.3

# ---------------------------------------------------------------------------
# DEB pin map  (stm32-base.org "STM32F4XX Pro", checked against the board
# silkscreen in photos).  Column k (1..30) holds pins 2k-1 and 2k; column 1
# is at the New-TFT end.  Odd pins are on the row nearer header 1 (the top).
# ---------------------------------------------------------------------------

H1_NAMES = {
    1: "PE1", 2: "PE0", 3: "PE3", 4: "PE2", 5: "PE5", 6: "PE4", 7: "PC13", 8: "PE6",
    9: "PF1", 10: "PF0", 11: "PF3", 12: "PF2", 13: "PF5", 14: "PF4", 15: "PF7",
    16: "PF6", 17: "PF9", 18: "PF8", 19: "PC0", 20: "PF10", 21: "PC2", 22: "PC1",
    23: "PA0", 24: "PC3", 25: "PA2", 26: "PA1", 27: "PA4", 28: "PA3", 29: "PA6",
    30: "PA5", 31: "PC4", 32: "PA7", 33: "PB0", 34: "PC5", 35: "PB2", 36: "PB1",
    37: "PF12", 38: "PF11", 39: "PF14", 40: "PF13", 41: "PG0", 42: "PF15",
    43: "PE7", 44: "PG1", 45: "PE9", 46: "PE8", 47: "PE11", 48: "PE10",
    49: "PE13", 50: "PE12", 51: "PE15", 52: "PE14", 53: "PB11", 54: "PB10",
    55: "GND", 56: "VREF+", 57: "BOOT0", 58: "BOOT1", 59: "3V3", 60: "3V3",
}
H2_NAMES = {
    1: "PB9", 2: "PB8", 3: "PB7", 4: "PB6", 5: "PB5", 6: "PB4", 7: "PB3", 8: "PG15",
    9: "PG14", 10: "PG13", 11: "PG12", 12: "PG11", 13: "PG10", 14: "PG9", 15: "PD7",
    16: "PD6", 17: "PD5", 18: "PD4", 19: "PD3", 20: "PD2", 21: "PD1", 22: "PD0",
    23: "PC12", 24: "PC11", 25: "PC10", 26: "PA15", 27: "PA14", 28: "PA13",
    29: "PA12", 30: "PA11", 31: "PA10", 32: "PA9", 33: "PA8", 34: "PC9", 35: "PC8",
    36: "PC7", 37: "PC6", 38: "PG8", 39: "PG7", 40: "PG6", 41: "PG5", 42: "PG4",
    43: "PG3", 44: "PG2", 45: "PD15", 46: "PD14", 47: "PD13", 48: "PD12",
    49: "PD11", 50: "PD10", 51: "PD9", 52: "PD8", 53: "PB15", 54: "PB14",
    55: "PB13", 56: "PB12", 57: "3V3", 58: "GND", 59: "5V", 60: "GND",
}


def deb_to_board(u, v):
    """DEB-local (u from its left/USB edge, v from its top/header-1 edge)."""
    return (DEB_X - DEB_W / 2 + u, DEB_Y - DEB_H / 2 + v)


def header_pin_xy(header, pin):
    """Board position of DEB header pin (header 1 = top edge, 2 = bottom)."""
    col = (pin + 1) // 2
    odd = pin % 2 == 1
    u = DEB_W / 2 + HDR_COL1_FROM_CENTRE - (col - 1) * 2.54
    if header == 1:
        v = HDR_OUTER_FROM_EDGE if odd else HDR_OUTER_FROM_EDGE + 2.54
    else:
        u += HDR2_COL_SHIFT
        v = DEB_H - HDR_OUTER_FROM_EDGE - 2.54 if odd else DEB_H - HDR_OUTER_FROM_EDGE
    return deb_to_board(u, v)


def window_rect():
    """TFT window (x0, y0, x1, y1) on the carrier."""
    cx, cy = DEB_X + TFT_DX, DEB_Y + TFT_DY
    hw = TFT_PCB_W / 2 + WINDOW_CLEAR
    hh = TFT_PCB_H / 2 + WINDOW_CLEAR
    return (cx - hw, cy - hh, cx + hw, cy + hh)


def deb_holes():
    xs = (DEB_HOLE_INSET_X, DEB_W - DEB_HOLE_INSET_X)
    ys = (DEB_HOLE_INSET_Y, DEB_H - DEB_HOLE_INSET_Y)
    return [deb_to_board(u, v) for v in ys for u in xs]


def rst_hole():
    return deb_to_board(DEB_RST_U, DEB_RST_V)


# ---------------------------------------------------------------------------
# Parts
#
# Each part: ref, value, symbol lib_id, footprint lib_id, board position
# (centre of the part body), rotation (deg, CCW as KiCad shows it), side,
# pins {pin_number: net}, plus a short description used for the BOM.
# ---------------------------------------------------------------------------

DPAD_X, DPAD_Y, DPAD_S = -79.0, 3.0, 15.0
JOY_X, JOY_Y = 79.0, -12.0

H1_NETS = {
    1: "BTN_UP",      # PE1
    3: "BTN_LEFT",    # PE3
    4: "BTN_DOWN",    # PE2
    6: "BTN_RIGHT",   # PE4
    19: "JOY_X",      # PC0  ADC1_IN10
    22: "JOY_Y",      # PC1  ADC1_IN11
    29: "SND",        # PA6  TIM3_CH1
    31: "JOY_SW",     # PC4
    34: "BTN_A",      # PC5  (stick-2 push input in firmware)
    55: "GND",
    59: "+3V3",
    60: "+3V3",
}
H2_NETS = {
    2: "BTN_BACK",    # PB8  (same as the DEB's KEY1)
    58: "GND",
    59: "+5V",
    60: "GND",
}

PARTS = []


def part(ref, value, symbol, footprint, xy, rot=0, side="F", pins=None,
         desc="", dnp=False, mpn=""):
    PARTS.append(dict(ref=ref, value=value, symbol=symbol, footprint=footprint,
                      xy=xy, rot=rot, side=side, pins=pins or {}, desc=desc,
                      dnp=dnp, mpn=mpn))


# --- DEB sockets (on the BACK of the carrier) ------------------------------
part("J1", "DEB_H1", "Connector_Generic:Conn_02x30_Odd_Even",
     "console:PinSocket_2x30_P2.54mm_DEB",
     None, side="B", pins=H1_NETS,
     desc="2x30 female header 2.54 mm, 8.5 mm tall (DEB header 1, PE1...3V3)")
part("J2", "DEB_H2", "Connector_Generic:Conn_02x30_Odd_Even",
     "console:PinSocket_2x30_P2.54mm_DEB",
     None, side="B", pins=H2_NETS,
     desc="2x30 female header 2.54 mm, 8.5 mm tall (DEB header 2, PB9...GND)")

# --- D-pad (left thumb) -----------------------------------------------------
SW12 = "Button_Switch_THT:SW_PUSH-12mm_Wuerth-430476085716"
SWDESC = "Tactile switch 12x12x7.3 mm THT + round cap"
part("SW1", "UP", "Switch:SW_Push", SW12, (DPAD_X, DPAD_Y - DPAD_S),
     pins={1: "BTN_UP", 2: "GND"}, desc=SWDESC)
part("SW2", "DOWN", "Switch:SW_Push", SW12, (DPAD_X, DPAD_Y + DPAD_S),
     pins={1: "BTN_DOWN", 2: "GND"}, desc=SWDESC)
part("SW3", "LEFT", "Switch:SW_Push", SW12, (DPAD_X - DPAD_S, DPAD_Y),
     pins={1: "BTN_LEFT", 2: "GND"}, desc=SWDESC)
part("SW4", "RIGHT", "Switch:SW_Push", SW12, (DPAD_X + DPAD_S, DPAD_Y),
     pins={1: "BTN_RIGHT", 2: "GND"}, desc=SWDESC)

# --- right thumb: stick + A / BACK -----------------------------------------
part("U1", "Joystick", "console:Joystick_2Axis_SW",
     "console:Joystick_PS2_Thumb", (JOY_X, JOY_Y),
     pins={1: "+3V3", 2: "JOY_X", 3: "GND",
           4: "+3V3", 5: "JOY_Y", 6: "GND",
           7: "JOY_SW", 8: "GND"},
     desc="PS2-style 2-axis thumb joystick, 10k pots + push switch "
          "(same part as on a KY-023 module)")
part("SW5", "A", "Switch:SW_Push", SW12, (88.0, 17.0),
     pins={1: "BTN_A", 2: "GND"}, desc=SWDESC)
part("SW6", "BACK", "Switch:SW_Push", SW12, (69.0, 28.0),
     pins={1: "BTN_BACK", 2: "GND"}, desc=SWDESC)

# --- sound: PA6 -> S8050 low-side switch -> buzzer (front) / speaker (back) -
part("BZ1", "Buzzer_passive", "Device:Buzzer",
     "console:Buzzer_12mm_P6.5-7.6mm", (-62.0, -34.0),
     pins={1: "BZ_P", 2: "SPK_DRV"},
     desc="12 mm PASSIVE magnetic buzzer, 3-5 V (pitch 6.5 or 7.6 mm both fit)")
part("Q1", "S8050", "Transistor_BJT:Q_NPN_EBC",
     "Package_TO_SOT_THT:TO-92_Inline", (-82.5, -25.0),
     pins={1: "GND", 2: "Q_BASE", 3: "SPK_DRV"},
     desc="S8050 NPN, TO-92, pinout E-B-C (2N2222A/SS8050 also fine if E-B-C)")
part("R1", "1k", "Device:R",
     "Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P2.54mm_Vertical", (-89.0, -25.0),
     rot=90, pins={1: "SND", 2: "Q_BASE"}, desc="Resistor 1 kOhm 1/4 W")
part("R2", "10k", "Device:R",
     "Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P2.54mm_Vertical", (-76.5, -25.0),
     rot=90, pins={1: "Q_BASE", 2: "GND"}, desc="Resistor 10 kOhm 1/4 W")
part("D1", "1N4148", "Device:D",
     "Diode_THT:D_DO-35_SOD27_P2.54mm_Vertical_AnodeUp", (-71.0, -26.0),
     rot=90, pins={1: "+5V", 2: "SPK_DRV"}, desc="1N4148 flyback diode")
part("R3", "22R 1/2W", "Device:R",
     "Resistor_THT:R_Axial_DIN0309_L9.0mm_D3.2mm_P2.54mm_Vertical", (-87.0, -32.5),
     rot=90, pins={1: "+5V", 2: "BZ_P"},
     desc="Resistor 22 Ohm 1/2 W (buzzer current limit)")
part("C1", "100uF", "Device:C_Polarized",
     "Capacitor_THT:CP_Radial_D6.3mm_P2.50mm", (-73.5, -37.5),
     pins={1: "+5V", 2: "GND"}, desc="Electrolytic 100 uF 16 V, 6.3 mm")
part("C2", "100nF", "Device:C",
     "Capacitor_THT:C_Disc_D5.0mm_W2.5mm_P2.50mm", (-80.0, -32.0),
     pins={1: "+5V", 2: "GND"}, desc="Ceramic 100 nF")
part("R4", "33R 1/2W", "Device:R",
     "Resistor_THT:R_Axial_DIN0309_L9.0mm_D3.2mm_P2.54mm_Vertical", (97.5, 4.0),
     rot=90, pins={1: "+5V", 2: "SPK_P"},
     desc="Resistor 33 Ohm 1/2 W (speaker current limit)")
part("J3", "SPEAKER", "Connector_Generic:Conn_01x02",
     "Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical", (100.5, -4.0),
     pins={1: "SPK_P", 2: "SPK_DRV"},
     desc="Speaker pads: solder an 8 Ohm 0.5 W speaker's wires here (optional)")

# --- mechanical -------------------------------------------------------------
for i, xy in enumerate(deb_holes()):
    part(f"H{i + 1}", "M3", "Mechanical:MountingHole",
         "MountingHole:MountingHole_3.2mm_M3", xy,
         desc="M3 hole for an 11 mm M3 standoff to the DEB")

# Nets that get routed, in routing order, with their track width.
ROUTE_NETS = [
    ("+5V", TRACK_PWR),
    ("SPK_DRV", TRACK_PWR),
    ("+3V3", TRACK_3V3),
    ("BTN_UP", TRACK_SIG),
    ("BTN_LEFT", TRACK_SIG),
    ("BTN_DOWN", TRACK_SIG),
    ("BTN_RIGHT", TRACK_SIG),
    ("SND", TRACK_SIG),
    ("JOY_X", TRACK_SIG),
    ("JOY_Y", TRACK_SIG),
    ("JOY_SW", TRACK_SIG),
    ("BTN_A", TRACK_SIG),
    ("BTN_BACK", TRACK_SIG),
    ("BZ_P", TRACK_PWR),
    ("SPK_P", TRACK_PWR),
    ("Q_BASE", TRACK_SIG),
]


def all_nets():
    nets = set()
    for p in PARTS:
        nets.update(p["pins"].values())
    return sorted(nets)
