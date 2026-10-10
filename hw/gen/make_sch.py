"""
make_sch.py - writes stm32_console_carrier.kicad_sch from design.py.

Symbols are copied out of KiCad's own libraries (plus console.kicad_sym)
and embedded, the way Eeschema stores them.  Every used pin gets a short
wire and a net label (or a power symbol), every unused DEB pin a
no-connect flag, so ERC and "update PCB from schematic" both work.
"""
import os
import uuid

import design as D

KICAD_SYM = r"C:\Program Files\KiCad\10.0\share\kicad\symbols"


# ------------------------------------------------------------- s-expressions
class Q(str):
    """A quoted string atom."""


def parse(text):
    i, n = 0, len(text)
    stack = [[]]
    while i < n:
        c = text[i]
        if c == "(":
            stack.append([])
            i += 1
        elif c == ")":
            done = stack.pop()
            stack[-1].append(done)
            i += 1
        elif c == '"':
            j = i + 1
            buf = []
            while text[j] != '"':
                if text[j] == "\\":
                    buf.append(text[j:j + 2])
                    j += 2
                else:
                    buf.append(text[j])
                    j += 1
            stack[-1].append(Q("".join(buf)))
            i = j + 1
        elif c.isspace():
            i += 1
        else:
            j = i
            while j < n and not text[j].isspace() and text[j] not in '()"':
                j += 1
            stack[-1].append(text[i:j])
            i = j
    return stack[0]


def dump(x, ind=1):
    if isinstance(x, Q):
        return '"' + x + '"'
    if isinstance(x, str):
        return x
    if all(not isinstance(e, list) for e in x):
        return "(" + " ".join(dump(e) for e in x) + ")"
    pad = "\t" * ind
    out = "(" + dump(x[0])
    for e in x[1:]:
        if isinstance(e, list):
            out += "\n" + pad + dump(e, ind + 1)
        else:
            out += " " + dump(e)
    return out + "\n" + "\t" * (ind - 1) + ")"


def find(lst, key):
    return [e for e in lst if isinstance(e, list) and e and e[0] == key]


_libcache = {}


def lib_symbol(lib_id):
    lib, name = lib_id.split(":")
    if lib == "console":
        path = os.path.join(_OUT, "console.kicad_sym")
    else:
        path = os.path.join(KICAD_SYM, lib + ".kicad_sym")
    if path not in _libcache:
        with open(path, encoding="utf-8") as f:
            _libcache[path] = parse(f.read())[0]
    tree = _libcache[path]
    for s in find(tree, "symbol"):
        if s[1] == name:
            if find(s, "extends"):
                raise RuntimeError(f"{lib_id} uses 'extends'; pick the base symbol")
            sym = [e for e in s]           # shallow copy is enough, we only rename
            sym[1] = Q(lib_id)
            return sym
    raise RuntimeError("symbol not found: " + lib_id)


def sym_pins(sym):
    pins = {}
    for sub in find(sym, "symbol"):
        for p in find(sub, "pin"):
            at = find(p, "at")[0]
            num = find(p, "number")[0][1]
            name = find(p, "name")[0][1]
            pins[str(num)] = (float(at[1]), float(at[2]), float(at[3]) if len(at) > 3 else 0.0, str(name))
    return pins


# ------------------------------------------------------------------ writer
def U():
    return str(uuid.uuid4())


def fmt(v):
    s = f"{v:.4f}".rstrip("0").rstrip(".")
    return "0" if s == "-0" else s


class Sheet:
    def __init__(self, project, root_uuid):
        self.project = project
        self.root = root_uuid
        self.items = []
        self.lib = {}
        self.pwr_n = 0

    def symbol(self, lib_id, ref, value, x, y, rot=0, footprint="", props=None,
               uid=None, in_bom=True, on_board=True, ref_xy=None, val_xy=None, hide_val=False):
        sym = lib_symbol(lib_id)
        self.lib[lib_id] = sym
        uid = uid or U()
        pins = sym_pins(sym)
        rx, ry = ref_xy or (x, y - 6)
        vx, vy = val_xy or (x, y + 6)
        s = (f'\t(symbol\n\t\t(lib_id "{lib_id}")\n\t\t(at {fmt(x)} {fmt(y)} {rot})\n'
             f'\t\t(unit 1)\n\t\t(exclude_from_sim no)\n'
             f'\t\t(in_bom {"yes" if in_bom else "no"})\n\t\t(on_board {"yes" if on_board else "no"})\n'
             f'\t\t(dnp no)\n\t\t(uuid "{uid}")\n')
        hidden_ref = ref.startswith("#")

        def prop(k, v, px, py, hide):
            h = "\t\t\t(hide yes)\n" if hide else ""
            return (f'\t\t(property "{k}" "{v}"\n\t\t\t(at {fmt(px)} {fmt(py)} 0)\n{h}'
                    f'\t\t\t(effects\n\t\t\t\t(font\n\t\t\t\t\t(size 1.27 1.27)\n\t\t\t\t)\n\t\t\t)\n\t\t)\n')
        s += prop("Reference", ref, rx, ry, hidden_ref)
        s += prop("Value", value, vx, vy, hide_val)
        s += prop("Footprint", footprint, x, y, True)
        s += prop("Datasheet", "", x, y, True)
        for k, v in (props or {}).items():
            s += prop(k, v, x, y, True)
        for num in pins:
            s += f'\t\t(pin "{num}"\n\t\t\t(uuid "{U()}")\n\t\t)\n'
        s += (f'\t\t(instances\n\t\t\t(project "{self.project}"\n'
              f'\t\t\t\t(path "/{self.root}"\n\t\t\t\t\t(reference "{ref}")\n\t\t\t\t\t(unit 1)\n'
              f'\t\t\t\t)\n\t\t\t)\n\t\t)\n\t)\n')
        self.items.append(s)
        return uid, pins

    def wire(self, x1, y1, x2, y2):
        self.items.append(
            f'\t(wire\n\t\t(pts\n\t\t\t(xy {fmt(x1)} {fmt(y1)}) (xy {fmt(x2)} {fmt(y2)})\n\t\t)\n'
            f'\t\t(stroke\n\t\t\t(width 0)\n\t\t\t(type default)\n\t\t)\n\t\t(uuid "{U()}")\n\t)\n')

    def label(self, name, x, y, ang):
        just = {0: "left bottom", 90: "left bottom", 180: "right bottom", 270: "right bottom"}[ang]
        self.items.append(
            f'\t(label "{name}"\n\t\t(at {fmt(x)} {fmt(y)} {ang})\n\t\t(fields_autoplaced yes)\n'
            f'\t\t(effects\n\t\t\t(font\n\t\t\t\t(size 1.27 1.27)\n\t\t\t)\n\t\t\t(justify {just})\n\t\t)\n'
            f'\t\t(uuid "{U()}")\n\t)\n')

    def noconnect(self, x, y):
        self.items.append(f'\t(no_connect\n\t\t(at {fmt(x)} {fmt(y)})\n\t\t(uuid "{U()}")\n\t)\n')

    def note(self, txt, x, y, size=1.5):
        txt = txt.replace('"', "'").replace(chr(10), chr(92) + "n")
        self.items.append(
            f'\t(text "{txt}"\n\t\t(exclude_from_sim no)\n\t\t(at {fmt(x)} {fmt(y)} 0)\n'
            f'\t\t(effects\n\t\t\t(font\n\t\t\t\t(size {size} {size})\n\t\t\t)\n\t\t\t(justify left top)\n\t\t)\n'
            f'\t\t(uuid "{U()}")\n\t)\n')

    def power(self, net, x, y, out_dir):
        """power symbol whose pin sits at (x, y); glyph points along out_dir."""
        self.pwr_n += 1
        lib_id = {"GND": "power:GND", "+3V3": "power:+3V3", "+5V": "power:+5V"}[net]
        if net == "GND":
            rot = {(0, 1): 0, (1, 0): 90, (0, -1): 180, (-1, 0): 270}[out_dir]
        else:
            rot = {(0, -1): 0, (-1, 0): 90, (0, 1): 180, (1, 0): 270}[out_dir]
        ox, oy = out_dir
        self.symbol(lib_id, f"#PWR{self.pwr_n:03d}", net, x, y, rot,
                    ref_xy=(x, y), val_xy=(x + ox * 5.5, y + oy * 5.5), in_bom=False, on_board=False)

    def connect_pins(self, x, y, pins, nets, stub=2.54, nc_unused=True):
        """Attach labels / power / no-connects to a symbol placed at (x, y), rot 0."""
        for num, (px, py, ang, _name) in pins.items():
            cx, cy = x + px, y - py            # lib y-up -> sheet y-down
            # outward direction (away from the body)
            ox, oy = {0: (-1, 0), 180: (1, 0), 90: (0, 1), 270: (0, -1)}[int(ang) % 360]
            net = nets.get(num)
            if net is None:
                if nc_unused:
                    self.noconnect(cx, cy)
                continue
            ex, ey = cx + ox * stub, cy + oy * stub
            self.wire(cx, cy, ex, ey)
            if net in ("GND", "+3V3", "+5V"):
                self.power(net, ex, ey, (ox, oy))
            else:
                ang_l = {(1, 0): 0, (-1, 0): 180, (0, -1): 90, (0, 1): 270}[(ox, oy)]
                self.label(net, ex, ey, ang_l)

    def write(self, path, title):
        libs = "".join("\t\t" + dump(s, 3) + "\n" for s in self.lib.values())
        with open(path, "w", encoding="utf-8", newline="\n") as f:
            f.write("(kicad_sch\n\t(version 20250114)\n\t(generator \"console_gen\")\n"
                    "\t(generator_version \"10.0\")\n"
                    f"\t(uuid \"{self.root}\")\n\t(paper \"A3\")\n"
                    f"\t(title_block\n\t\t(title \"{title}\")\n\t\t(date \"2026-10-10\")\n"
                    "\t\t(rev \"0.1\")\n\t\t(company \"NYCU Maker Club\")\n"
                    "\t\t(comment 1 \"Carrier for mcudev STM32F4XX_Pro (STM32F407ZGT6) + 3.2in TFT\")\n"
                    "\t\t(comment 2 \"Pin map = fw_console/PINMAP.md (PE1-4 d-pad, PC0/PC1/PC4 stick, PC5 A, PB8 BACK, PA6 sound)\")\n"
                    "\t)\n")
            f.write("\t(lib_symbols\n" + libs + "\t)\n")
            f.write("".join(self.items))
            f.write("\t(sheet_instances\n\t\t(path \"/\"\n\t\t\t(page \"1\")\n\t\t)\n\t)\n"
                    "\t(embedded_fonts no)\n)\n")


_OUT = None


def build(outdir, name):
    global _OUT
    _OUT = outdir
    root = U()
    sh = Sheet(name, root)
    uuids = {}

    parts = {p["ref"]: p for p in D.PARTS}

    def place(ref, x, y, **kw):
        p = parts[ref]
        props = {"Description": p["desc"]}
        uid, pins = sh.symbol(p["symbol"], ref, p["value"], x, y, footprint=p["footprint"],
                              props=props, in_bom=not ref.startswith("H"), **kw)
        nets = {str(k): v for k, v in p["pins"].items()}
        sh.connect_pins(x, y, pins, nets, nc_unused=ref in ("J1", "J2"))
        uuids[ref] = uid
        return pins

    # DEB sockets - big, on the left
    place("J1", 55.88, 99.06, ref_xy=(55.88, 57.0), val_xy=(55.88, 141.0))
    sh.note("J1 = DEB header 1 (top edge)\nodd pins = outer row, pin 1 at New-TFT end\n"
            "unused pins are left open (X)", 30.48, 144.78, 1.27)
    place("J2", 55.88, 210.82, ref_xy=(55.88, 169.0), val_xy=(55.88, 253.0))
    sh.note("J2 = DEB header 2 (bottom edge)\nodd pins = inner row, pin 1 at New-TFT end",
            30.48, 256.54, 1.27)

    # D-pad + A + BACK
    y = 50.8
    for ref in ("SW1", "SW2", "SW3", "SW4", "SW5", "SW6"):
        place(ref, 139.7, y, ref_xy=(139.7, y - 4.5), val_xy=(139.7, y + 3.5))
        y += 17.78
    sh.note("Buttons: active LOW, no resistors -\nthe STM32 internal pull-ups are enabled\n"
            "in Input_Init().  BACK (PB8) is in parallel\nwith the DEB's own KEY1.",
            121.92, 160.02, 1.27)

    # joystick
    place("U1", 215.9, 63.5, ref_xy=(215.9, 49.0), val_xy=(215.9, 78.0))
    sh.note("Stick pots run from 3V3, NOT 5V\n(ADC pins max out at VDDA = 3.3 V).\n"
            "If an axis is reversed, flip JOY_INVERT_X/Y\nin fw_console/App/input.c.",
            198.12, 83.82, 1.27)

    # sound
    place("R1", 200.66, 139.7, ref_xy=(204.0, 138.4), val_xy=(204.0, 141.0))
    place("R2", 200.66, 160.02, ref_xy=(204.0, 158.7), val_xy=(204.0, 161.3))
    place("Q1", 226.06, 139.7, ref_xy=(232.0, 138.4), val_xy=(232.0, 141.0))
    place("D1", 254.0, 124.46, ref_xy=(254.0, 120.0), val_xy=(254.0, 129.0))
    place("BZ1", 289.56, 132.08, ref_xy=(296.0, 129.0), val_xy=(296.0, 135.0))
    place("R3", 289.56, 114.3, ref_xy=(293.0, 113.0), val_xy=(293.0, 115.6))
    place("R4", 320.04, 114.3, ref_xy=(323.4, 113.0), val_xy=(323.4, 115.6))
    place("J3", 325.12, 139.7, ref_xy=(325.12, 133.0), val_xy=(325.12, 146.0))
    place("C1", 345.44, 162.56, ref_xy=(349.0, 161.3), val_xy=(349.0, 163.8))
    place("C2", 365.76, 162.56, ref_xy=(369.0, 161.3), val_xy=(369.0, 163.8))
    sh.note("Sound: PA6 (TIM3_CH1 PWM) -> S8050 low-side switch on +5V.\n"
            "Front: 12 mm PASSIVE buzzer through R3.  Back: optional 8 ohm speaker on J3\n"
            "through R4.  Fit either or both.  D1 catches the coil kick.\n"
            "Firmware Sound_Stop() parks PA6 low, so the transistor idles OFF.",
            198.12, 172.72, 1.27)

    # mounting holes
    x = 345.44
    for ref in ("H1", "H2", "H3", "H4"):
        place(ref, x, 210.82, ref_xy=(x, 205.0), val_xy=(x, 216.0))
        x += 12.7
    sh.note("M3 holes line up with the DEB's corner holes:\n11 mm M3 standoffs, DEB to carrier.",
            340.36, 220.98, 1.27)

    # power flags
    for i, (net, d) in enumerate((("GND", (0, 1)), ("+3V3", (0, -1)), ("+5V", (0, -1)))):
        px, py = 139.7 + i * 15.24, 228.6
        sh.symbol("power:PWR_FLAG", f"#FLG{i + 1:02d}", "PWR_FLAG", px, py, 0,
                  ref_xy=(px, py - 3), val_xy=(px, py - 4.5), in_bom=False, on_board=False)
        sh.power(net, px, py, d)
    sh.note("Power comes from the DEB: its USB 5 V and its AMS1117 3.3 V.\n"
            "The carrier draws < 0.3 A from 5 V (sound only) and ~1 mA from 3V3.",
            134.62, 241.3, 1.27)

    path = os.path.join(outdir, name + ".kicad_sch")
    sh.write(path, "STM32 Arcade - console carrier")
    return path, uuids
