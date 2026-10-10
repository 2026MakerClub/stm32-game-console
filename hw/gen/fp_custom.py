"""
fp_custom.py - writes the project-local footprint and symbol libraries.

  console.pretty/Joystick_PS2_Thumb.kicad_mod
      PS2-style thumbstick (the stick on a KY-023 module).  Pad pattern from
      SparkFun's proven "JOYSTICK" package (COM-09032), converted to KiCad
      (y flipped).  Switch pins: only the two SparkFun actually used carry a
      net, the other two are mechanical, so a wrong guess about which pins
      are bridged inside the switch can never short JOY_SW to GND.

  console.pretty/Buzzer_12mm_P6.5-7.6mm.kicad_mod
      12 mm buzzer; pin 2 is a plated slot so both the 6.5 mm (most passive
      12x8.5) and 7.6 mm (12x9.5) pitches drop in.

  console.kicad_sym : Joystick_2Axis_SW symbol.
"""
import os

KICAD_FP = r"C:\Program Files\KiCad\10.0\share\kicad\footprints"


def _line(x1, y1, x2, y2, layer, w=0.12):
    return (f'  (fp_line (start {x1:.3f} {y1:.3f}) (end {x2:.3f} {y2:.3f}) '
            f'(stroke (width {w}) (type solid)) (layer "{layer}"))\n')


def _circle(cx, cy, r, layer, w=0.12):
    return (f'  (fp_circle (center {cx:.3f} {cy:.3f}) (end {cx + r:.3f} {cy:.3f}) '
            f'(stroke (width {w}) (type solid)) (fill none) (layer "{layer}"))\n')


def _rect(x1, y1, x2, y2, layer, w=0.05):
    return (f'  (fp_rect (start {x1:.3f} {y1:.3f}) (end {x2:.3f} {y2:.3f}) '
            f'(stroke (width {w}) (type solid)) (fill none) (layer "{layer}"))\n')


def _text(kind, txt, x, y, layer, size=1.0, hide=False):
    h = " (hide yes)" if hide else ""
    if kind in ("Reference", "Value"):
        return (f'  (property "{kind}" "{txt}" (at {x:.3f} {y:.3f} 0) (layer "{layer}"){h}\n'
                f'    (effects (font (size {size} {size}) (thickness {size * 0.15:.3f}))))\n')
    return (f'  (fp_text user "{txt}" (at {x:.3f} {y:.3f} 0) (layer "{layer}")\n'
            f'    (effects (font (size {size} {size}) (thickness {size * 0.15:.3f}))))\n')


def _pad(num, x, y, size, drill, shape="circle", drill_oval=None, size_xy=None):
    sx, sy = size_xy if size_xy else (size, size)
    d = f"(drill oval {drill_oval[0]} {drill_oval[1]})" if drill_oval else f"(drill {drill})"
    return (f'  (pad "{num}" thru_hole {shape} (at {x:.3f} {y:.3f}) (size {sx} {sy}) {d} '
            f'(layers "*.Cu" "*.Mask"))\n')


def joystick_fp():
    s = ('(footprint "Joystick_PS2_Thumb" (version 20240108) (generator "console_gen")\n'
         '  (layer "F.Cu")\n'
         '  (descr "PS2-style 2-axis thumb joystick (KY-023 stick), pad pattern after '
         'SparkFun JOYSTICK / COM-09032. Pots 2.54 mm pitch, 10.16 mm from centre.")\n'
         '  (tags "joystick thumbstick PS2 KY-023")\n'
         '  (attr through_hole)\n')
    s += _text("Reference", "REF**", -13.2, 0, "F.SilkS")
    s += _text("Value", "Joystick", 0, 14.5, "F.Fab")
    # X pot (horizontal axis) - pins along the bottom
    for n, x in ((1, -2.54), (2, 0.0), (3, 2.54)):
        s += _pad(n, x, 10.16, 1.8, 0.9, "rect" if n == 1 else "circle")
    # Y pot (vertical axis) - pins down the left side
    for n, y in ((4, -2.54), (5, 0.0), (6, 2.54)):
        s += _pad(n, -10.16, y, 1.8, 0.9, "rect" if n == 4 else "circle")
    # push switch: 7 = SW (SparkFun B1A), 8 = SW return (B2A); B1B/B2B mechanical
    s += _pad(7, -3.175, -12.7, 1.8, 0.9)
    s += _pad(8, -3.175, -7.62, 1.8, 0.9)
    s += _pad("", 3.175, -12.7, 1.8, 0.9)
    s += _pad("", 3.175, -7.62, 1.8, 0.9)
    # four metal mounting tabs
    for x in (-7.62, 7.62):
        for y in (-6.6675, 6.6675):
            s += _pad("", x, y, 2.3, 1.4)
    # silkscreen: corner brackets of the stick base, clear of every pad
    e, k = 9.9, 6.0
    for sx in (-1, 1):
        for sy in (-1, 1):
            s += _line(sx * e, sy * e, sx * k, sy * e, "F.SilkS")
            s += _line(sx * e, sy * e, sx * e, sy * k, "F.SilkS")
    s += _circle(0, 0, 0.635, "F.SilkS")
    s += _circle(0, 0, 12.7, "F.Fab", 0.1)            # stick travel
    s += _text("user", "X", 0, 12.6, "F.Fab", 0.8)
    s += _text("user", "Y", -12.4, 0, "F.Fab", 0.8)
    s += _rect(-12.0, -14.5, 10.1, 12.0, "F.CrtYd")
    s += ')\n'
    return s


def buzzer_fp():
    s = ('(footprint "Buzzer_12mm_P6.5-7.6mm" (version 20240108) (generator "console_gen")\n'
         '  (layer "F.Cu")\n'
         '  (descr "12 mm round buzzer, pin 2 slotted to accept 6.5 or 7.6 mm pitch")\n'
         '  (tags "buzzer 12mm passive")\n'
         '  (attr through_hole)\n')
    s += _text("Reference", "REF**", 3.5, -7.3, "F.SilkS")
    s += _text("Value", "Buzzer", 3.5, 7.3, "F.Fab")
    s += _pad(1, 0, 0, 1.8, 0.9, "rect")
    s += _pad(2, 7.05, 0, 0, 0, "oval", drill_oval=(2.1, 0.9), size_xy=(3.0, 1.8))
    s += _circle(3.5, 0, 6.1, "F.SilkS")
    s += _circle(3.5, 0, 6.0, "F.Fab", 0.1)
    s += _text("user", "+", -0.2, -2.6, "F.SilkS", 1.2)
    s += _circle(3.5, 0, 6.6, "F.CrtYd", 0.05)
    s += ')\n'
    return s


def joystick_sym():
    def pin(num, name, x, y, ang):
        return (f'      (pin passive line (at {x} {y} {ang}) (length 2.54)\n'
                f'        (name "{name}" (effects (font (size 1.27 1.27))))\n'
                f'        (number "{num}" (effects (font (size 1.27 1.27)))))\n')

    def prop(k, v, y, hide=False):
        h = " (hide yes)" if hide else ""
        return (f'    (property "{k}" "{v}" (at 0 {y} 0){h}\n'
                f'      (effects (font (size 1.27 1.27))))\n')

    s = ('(kicad_symbol_lib (version 20241209) (generator "console_gen")\n'
         '  (symbol "Joystick_2Axis_SW" (pin_names (offset 1.016)) (exclude_from_sim no)\n'
         '    (in_bom yes) (on_board yes)\n')
    s += prop("Reference", "U", 13.97)
    s += prop("Value", "Joystick_2Axis_SW", -13.97)
    s += prop("Footprint", "console:Joystick_PS2_Thumb", -16.51, True)
    s += prop("Datasheet", "", -19.05, True)
    s += prop("Description", "2-axis analog thumb joystick, two 10k pots + push switch", -21.59, True)
    s += ('    (symbol "Joystick_2Axis_SW_0_1"\n'
          '      (rectangle (start -7.62 11.43) (end 7.62 -11.43)\n'
          '        (stroke (width 0.254) (type default)) (fill (type background))))\n'
          '    (symbol "Joystick_2Axis_SW_1_1"\n')
    s += pin(1, "X+", -10.16, 8.89, 0)
    s += pin(2, "X", -10.16, 6.35, 0)
    s += pin(3, "X-", -10.16, 3.81, 0)
    s += pin(4, "Y+", -10.16, -3.81, 0)
    s += pin(5, "Y", -10.16, -6.35, 0)
    s += pin(6, "Y-", -10.16, -8.89, 0)
    s += pin(7, "SW", 10.16, 2.54, 180)
    s += pin(8, "SW_GND", 10.16, -2.54, 180)
    s += '    )\n    (embedded_fonts no)\n  )\n)\n'
    return s


def socket_fp(pretty):
    """KiCad's 2x30 socket with 1.5 mm round pads, so a 0.25 mm track fits
    between two pins.  Saved under its own name so DRC's library check is happy."""
    import pcbnew
    src = os.path.join(KICAD_FP, "Connector_PinSocket_2.54mm.pretty")
    fp = pcbnew.FootprintLoad(src, "PinSocket_2x30_P2.54mm_Vertical")
    for pad in fp.Pads():
        pad.SetSize(pcbnew.F_Cu, pcbnew.VECTOR2I(pcbnew.FromMM(1.5), pcbnew.FromMM(1.5)))
        if pad.GetNumber() != "1":
            pad.SetShape(pcbnew.F_Cu, pcbnew.PAD_SHAPE_CIRCLE)
    fp.SetFPID(pcbnew.LIB_ID("console", "PinSocket_2x30_P2.54mm_DEB"))
    fp.SetLibDescription("2x30 2.54 mm female header for the STM32F4XX_Pro (DEB), 1.5 mm pads")
    path = os.path.join(pretty, "PinSocket_2x30_P2.54mm_DEB.kicad_mod")
    if os.path.exists(path):
        os.remove(path)
    pcbnew.FootprintSave(pretty, fp)


def write_libs(outdir):
    pretty = os.path.join(outdir, "console.pretty")
    os.makedirs(pretty, exist_ok=True)
    socket_fp(pretty)
    with open(os.path.join(pretty, "Joystick_PS2_Thumb.kicad_mod"), "w", newline="\n") as f:
        f.write(joystick_fp())
    with open(os.path.join(pretty, "Buzzer_12mm_P6.5-7.6mm.kicad_mod"), "w", newline="\n") as f:
        f.write(buzzer_fp())
    with open(os.path.join(outdir, "console.kicad_sym"), "w", newline="\n") as f:
        f.write(joystick_sym())
    with open(os.path.join(outdir, "fp-lib-table"), "w", newline="\n") as f:
        f.write('(fp_lib_table\n  (version 7)\n'
                '  (lib (name "console")(type "KiCad")(uri "${KIPRJMOD}/console.pretty")'
                '(options "")(descr "STM32 console carrier parts"))\n)\n')
    with open(os.path.join(outdir, "sym-lib-table"), "w", newline="\n") as f:
        f.write('(sym_lib_table\n  (version 7)\n'
                '  (lib (name "console")(type "KiCad")(uri "${KIPRJMOD}/console.kicad_sym")'
                '(options "")(descr "STM32 console carrier parts"))\n)\n')
    return pretty
