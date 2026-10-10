"""
make_pcb.py - builds stm32_console_carrier.kicad_pcb from design.py.

Run with KiCad's bundled Python (it needs the pcbnew module):
    "C:\\Program Files\\KiCad\\10.0\\bin\\python.exe" build.py
"""
import math
import os

import pcbnew

import design as D
from router import Router

KICAD_FP = r"C:\Program Files\KiCad\10.0\share\kicad\footprints"
MM = pcbnew.FromMM


def pcb_netname(n):
    return n if n in ("GND", "+3V3", "+5V") else "/" + n


def P(x, y):
    """design coords (board centre = 0,0) -> KiCad page coords, as VECTOR2I."""
    return pcbnew.VECTOR2I(MM(D.PAGE_OX + x), MM(D.PAGE_OY + y))


def page(x, y):
    return (D.PAGE_OX + x, D.PAGE_OY + y)


# --------------------------------------------------------------------------
# board outline, window, holes
# --------------------------------------------------------------------------

def stadium_sdf(X, Y):
    c = D.BOARD_W / 2 - D.BOARD_R
    x = X - D.PAGE_OX
    y = Y - D.PAGE_OY
    import numpy as np
    xc = np.clip(x, -c, c)
    return D.BOARD_R - np.hypot(x - xc, y)


def window_geometry():
    """Window rounded-rect plus, if the RST hole sits on/near it, a notch.

    Returns (rect, notch) where notch is None or (x1, y1, x2, y2, r) capsule,
    and rst_separate (bool) telling whether the RST hole is its own circle.
    """
    x0, y0, x1, y1 = D.window_rect()
    rx, ry = D.rst_hole()
    r = D.RST_HOLE_D / 2
    # distance from RST hole centre to the window rectangle
    dx = max(x0 - rx, 0, rx - x1)
    dy = max(y0 - ry, 0, ry - y1)
    d = math.hypot(dx, dy) - r
    if d >= 0.8:
        return (x0, y0, x1, y1), None, True
    # merge: capsule from the RST centre to the nearest point 1 mm inside
    nx = min(max(rx, x0 + 1), x1 - 1)
    ny = min(max(ry, y0 + 1), y1 - 1)
    return (x0, y0, x1, y1), (rx, ry, nx, ny, r), False


def edge_sdf_factory():
    import numpy as np
    (x0, y0, x1, y1), notch, rst_sep = window_geometry()
    cx, cy = (x0 + x1) / 2 + D.PAGE_OX, (y0 + y1) / 2 + D.PAGE_OY
    hw, hh, rr = (x1 - x0) / 2, (y1 - y0) / 2, D.WINDOW_CORNER_R
    holes = [(page(*xy), D.M3_HOLE_D / 2 + 0.15) for xy in D.deb_holes()]
    if rst_sep:
        holes.append((page(*D.rst_hole()), D.RST_HOLE_D / 2))

    def fn(X, Y):
        d = stadium_sdf(X, Y)
        qx = np.abs(X - cx) - (hw - rr)
        qy = np.abs(Y - cy) - (hh - rr)
        dw = (np.hypot(np.maximum(qx, 0), np.maximum(qy, 0))
              + np.minimum(np.maximum(qx, qy), 0) - rr)
        if notch:
            ax, ay = page(notch[0], notch[1])
            bx, by = page(notch[2], notch[3])
            vx, vy = bx - ax, by - ay
            L2 = vx * vx + vy * vy
            t = np.clip(((X - ax) * vx + (Y - ay) * vy) / L2, 0, 1)
            dn = np.hypot(X - (ax + t * vx), Y - (ay + t * vy)) - notch[4]
            dw = np.minimum(dw, dn)
        d = np.minimum(d, dw)
        for (hx, hy), hr in holes:
            d = np.minimum(d, np.hypot(X - hx, Y - hy) - hr)
        return d

    return fn


def add_seg(board, a, b, layer=pcbnew.Edge_Cuts, w=0.1):
    s = pcbnew.PCB_SHAPE(board)
    s.SetShape(pcbnew.SHAPE_T_SEGMENT)
    s.SetStart(P(*a))
    s.SetEnd(P(*b))
    s.SetLayer(layer)
    s.SetWidth(MM(w))
    board.Add(s)
    return s


def add_arc(board, start, mid, end, layer=pcbnew.Edge_Cuts, w=0.1):
    s = pcbnew.PCB_SHAPE(board)
    s.SetShape(pcbnew.SHAPE_T_ARC)
    s.SetArcGeometry(P(*start), P(*mid), P(*end))
    s.SetLayer(layer)
    s.SetWidth(MM(w))
    board.Add(s)


def add_circle(board, c, r, layer=pcbnew.Edge_Cuts, w=0.1, fill=False):
    s = pcbnew.PCB_SHAPE(board)
    s.SetShape(pcbnew.SHAPE_T_CIRCLE)
    s.SetCenter(P(*c))
    s.SetEnd(P(c[0] + r, c[1]))
    s.SetLayer(layer)
    s.SetWidth(MM(w))
    if fill:
        s.SetFilled(True)
    board.Add(s)


def add_rect(board, x0, y0, x1, y1, layer, w=0.15):
    for a, b in (((x0, y0), (x1, y0)), ((x1, y0), (x1, y1)),
                 ((x1, y1), (x0, y1)), ((x0, y1), (x0, y0))):
        add_seg(board, a, b, layer, w)


def add_poly(board, pts, layer=pcbnew.Edge_Cuts, w=0.1):
    s = pcbnew.PCB_SHAPE(board)
    s.SetShape(pcbnew.SHAPE_T_POLY)
    s.SetPolyPoints([P(x, y) for x, y in pts])
    s.SetLayer(layer)
    s.SetWidth(MM(w))
    s.SetFilled(False)
    board.Add(s)


def rounded_rect_pts(x0, y0, x1, y1, r, n=8):
    pts = []
    for cx, cy, a0 in ((x1 - r, y0 + r, -90), (x1 - r, y1 - r, 0),
                       (x0 + r, y1 - r, 90), (x0 + r, y0 + r, 180)):
        for k in range(n + 1):
            a = math.radians(a0 + 90 * k / n)
            pts.append((cx + r * math.cos(a), cy + r * math.sin(a)))
    return pts


def capsule_pts(x1, y1, x2, y2, r, n=12):
    ang = math.atan2(y2 - y1, x2 - x1)
    pts = []
    for k in range(n + 1):
        a = ang + math.pi / 2 + math.pi * k / n
        pts.append((x1 + r * math.cos(a), y1 + r * math.sin(a)))
    for k in range(n + 1):
        a = ang - math.pi / 2 + math.pi * k / n
        pts.append((x2 + r * math.cos(a), y2 + r * math.sin(a)))
    return pts


def draw_outline(board):
    c = D.BOARD_W / 2 - D.BOARD_R
    R = D.BOARD_R
    add_seg(board, (-c, -R), (c, -R))
    add_arc(board, (c, -R), (c + R, 0), (c, R))
    add_seg(board, (c, R), (-c, R))
    add_arc(board, (-c, R), (-c - R, 0), (-c, -R))

    (x0, y0, x1, y1), notch, rst_sep = window_geometry()
    wpts = rounded_rect_pts(x0, y0, x1, y1, D.WINDOW_CORNER_R)
    if notch:
        a = pcbnew.SHAPE_POLY_SET()
        a.NewOutline()
        for x, y in wpts:
            a.Append(MM(x), MM(y))
        b = pcbnew.SHAPE_POLY_SET()
        b.NewOutline()
        for x, y in capsule_pts(*notch):
            b.Append(MM(x), MM(y))
        a.BooleanAdd(b)
        a.Simplify()
        ol = a.Outline(0)
        wpts = [(pcbnew.ToMM(ol.CPoint(k).x), pcbnew.ToMM(ol.CPoint(k).y))
                for k in range(ol.PointCount())]
    add_poly(board, wpts)
    if rst_sep:
        add_circle(board, D.rst_hole(), D.RST_HOLE_D / 2)
    return rst_sep


# --------------------------------------------------------------------------
# footprints
# --------------------------------------------------------------------------

def load_fp(fpid, outdir):
    lib, name = fpid.split(":")
    path = os.path.join(outdir, "console.pretty") if lib == "console" else \
        os.path.join(KICAD_FP, lib + ".pretty")
    fp = pcbnew.FootprintLoad(path, name)
    if fp is None:
        raise RuntimeError("footprint not found: " + fpid)
    fp.SetFPID(pcbnew.LIB_ID(lib, name))
    return fp


def pads_centre(fp):
    xs = [p.GetPosition().x for p in fp.Pads() if p.GetNumber() != ""]
    ys = [p.GetPosition().y for p in fp.Pads() if p.GetNumber() != ""]
    return pcbnew.VECTOR2I((min(xs) + max(xs)) // 2, (min(ys) + max(ys)) // 2)


def place_socket(board, fp, header):
    """2x30 socket on the back; pad n ends up under DEB header pin n."""
    fp.SetPosition(pcbnew.VECTOR2I(0, 0))
    board.Add(fp)                      # Flip() needs the board's layer stack
    fp.Flip(pcbnew.VECTOR2I(0, 0), pcbnew.FLIP_DIRECTION_LEFT_RIGHT)
    want = {n: D.header_pin_xy(header, n) for n in range(1, 61)}
    for rot in (0, 90, 180, 270):
        fp.SetOrientationDegrees(rot)
        pos = {int(p.GetNumber()): (pcbnew.ToMM(p.GetPosition().x), pcbnew.ToMM(p.GetPosition().y))
               for p in fp.Pads()}
        ok = all(abs((pos[n][0] - pos[1][0]) - (want[n][0] - want[1][0])) < 0.01 and
                 abs((pos[n][1] - pos[1][1]) - (want[n][1] - want[1][1])) < 0.01
                 for n in range(1, 61))
        if ok:
            break
    else:
        raise RuntimeError("could not orient socket to match DEB pin map")
    tx = D.PAGE_OX + want[1][0] - pos[1][0]
    ty = D.PAGE_OY + want[1][1] - pos[1][1]
    fp.SetPosition(pcbnew.VECTOR2I(MM(tx), MM(ty)))
    for p in fp.Pads():
        n = int(p.GetNumber())
        x, y = pcbnew.ToMM(p.GetPosition().x), pcbnew.ToMM(p.GetPosition().y)
        assert abs(x - D.PAGE_OX - want[n][0]) < 0.005 and abs(y - D.PAGE_OY - want[n][1]) < 0.005


def build(outdir, name, sym_uuids, sch_file):
    pcb_path = os.path.join(outdir, name + ".kicad_pcb")
    board = pcbnew.NewBoard(pcb_path)

    ds = board.GetDesignSettings()
    nc = ds.m_NetSettings.GetDefaultNetclass()
    nc.SetClearance(MM(D.CLEARANCE))
    nc.SetTrackWidth(MM(D.TRACK_SIG))
    nc.SetViaDiameter(MM(D.VIA_D))
    nc.SetViaDrill(MM(D.VIA_DRILL))
    ds.m_CopperEdgeClearance = MM(0.3)
    ds.m_TrackMinWidth = MM(0.2)
    ds.m_MinThroughDrill = MM(0.3)
    ds.m_HoleToHoleMin = MM(0.25)
    ds.m_HoleClearance = MM(0.2)
    ds.m_ViasMinSize = MM(0.6)
    ds.m_ViasMinAnnularWidth = MM(0.13)
    ds.m_MinSilkTextHeight = MM(0.8)
    ds.m_MinSilkTextThickness = MM(0.12)
    ds.SetBoardThickness(MM(1.6))

    # nets
    nets = {}
    for n in D.all_nets():
        ni = pcbnew.NETINFO_ITEM(board, pcb_netname(n))
        board.Add(ni)
        nets[n] = ni
    for p in D.PARTS:
        if p["ref"] in ("J1", "J2"):
            for k in range(1, 61):
                if k not in p["pins"]:
                    key = f"unconnected-({p['ref']}-Pin_{k}-Pad{k})"
                    ni = pcbnew.NETINFO_ITEM(board, key)
                    board.Add(ni)
                    nets[key] = ni
    to_design = {pcb_netname(n): n for n in D.all_nets()}

    rst_sep = draw_outline(board)

    # footprints
    fps = {}
    for p in D.PARTS:
        fp = load_fp(p["footprint"], outdir)
        fp.SetReference(p["ref"])
        fp.SetValue(p["value"])
        if p["ref"] in ("J1", "J2"):
            place_socket(board, fp, int(p["ref"][1]))
        else:
            fp.SetOrientationDegrees(p["rot"])
            fp.SetPosition(pcbnew.VECTOR2I(0, 0))
            anchor = pcbnew.VECTOR2I(0, 0) if p["ref"].startswith(("U", "H")) else pads_centre(fp)
            tgt = P(*p["xy"])
            fp.SetPosition(pcbnew.VECTOR2I(tgt.x - anchor.x, tgt.y - anchor.y))
            board.Add(fp)
        uid = sym_uuids[p["ref"]]
        fp.SetPath(pcbnew.KIID_PATH("/" + uid))
        fp.SetSheetname("/")
        fp.SetSheetfile(sch_file)
        if p["dnp"]:
            fp.SetDNP(True)
        fp.SetField("Description", p["desc"])
        for pad in fp.Pads():
            num = pad.GetNumber()
            if num and num.isdigit() and int(num) in p["pins"]:
                pad.SetNet(nets[p["pins"][int(num)]])
            elif p["ref"] in ("J1", "J2") and num:
                pad.SetNet(nets[f"unconnected-({p['ref']}-Pin_{num}-Pad{num})"])
        fps[p["ref"]] = fp

    decorate(board, fps, rst_sep)

    # ---------------------------------------------------------------- route
    x0, y0 = D.PAGE_OX - D.BOARD_W / 2 - 1, D.PAGE_OY - D.BOARD_H / 2 - 1
    r = Router((x0, y0, x0 + D.BOARD_W + 2, y0 + D.BOARD_H + 2), 0.2,
               D.CLEARANCE, D.EDGE_CLEAR, D.VIA_D)
    r.set_edge_distance(edge_sdf_factory())
    netpads = {}
    for fp in fps.values():
        for pad in fp.Pads():
            if pad.GetAttribute() == pcbnew.PAD_ATTRIB_NPTH:
                continue          # holes are already in the edge map
            pn = pad.GetNetname()
            net = to_design.get(pn, pn) or None
            bb = pad.GetBoundingBox()
            cx, cy = pcbnew.ToMM(bb.GetCenter().x), pcbnew.ToMM(bb.GetCenter().y)
            hw, hh = pcbnew.ToMM(bb.GetWidth()) / 2, pcbnew.ToMM(bb.GetHeight()) / 2
            layers = (0, 1) if pad.GetAttribute() == pcbnew.PAD_ATTRIB_PTH else \
                ((0,) if pad.IsOnLayer(pcbnew.F_Cu) else (1,))
            shape = pad.GetShape(pcbnew.F_Cu)
            if shape == pcbnew.PAD_SHAPE_CIRCLE:
                r.add_circle(net, layers, cx, cy, hw)
            elif shape == pcbnew.PAD_SHAPE_OVAL:
                if hw > hh:
                    r.add_capsule(net, layers, cx - hw + hh, cy, cx + hw - hh, cy, hh)
                else:
                    r.add_capsule(net, layers, cx, cy - hh + hw, cx, cy + hh - hw, hw)
            else:
                r.add_rect(net, layers, cx, cy, hw, hh)
            if net:
                netpads.setdefault(net, []).append((cx, cy, min(hw, hh), layers))

    failed = []
    for net, w in D.ROUTE_NETS:
        pads = netpads.get(net, [])
        print(f"  routing {net:10s} {len(pads)} pads, {w} mm")
        if not r.route_net(net, w, pads):
            failed.append(net)

    for net, runs in r.paths.items():
        w = dict(D.ROUTE_NETS)[net]
        for L, pts in runs:
            for (ax, ay), (bx, by) in zip(pts, pts[1:]):
                t = pcbnew.PCB_TRACK(board)
                t.SetStart(pcbnew.VECTOR2I(MM(ax), MM(ay)))
                t.SetEnd(pcbnew.VECTOR2I(MM(bx), MM(by)))
                t.SetWidth(MM(w))
                t.SetLayer(pcbnew.F_Cu if L == 0 else pcbnew.B_Cu)
                t.SetNet(nets[net])
                board.Add(t)
        for vx, vy in r.vias.get(net, []):
            v = pcbnew.PCB_VIA(board)
            v.SetPosition(pcbnew.VECTOR2I(MM(vx), MM(vy)))
            v.SetDrill(MM(D.VIA_DRILL))
            v.SetWidth(MM(D.VIA_D))
            v.SetNet(nets[net])
            board.Add(v)

    # ------------------------------------------------- GND stitching vias
    bt, bv = r.blocked("__stitch__", 0.4)   # every pad is an obstacle, GND pads too
    import numpy as np
    pitch = 6.0
    nstitch = 0
    ys = np.arange(D.PAGE_OY - D.BOARD_H / 2 + 3, D.PAGE_OY + D.BOARD_H / 2 - 2, pitch)
    xs = np.arange(D.PAGE_OX - D.BOARD_W / 2 + 3, D.PAGE_OX + D.BOARD_W / 2 - 2, pitch)
    for yy in ys:
        for xx in xs:
            i, j = r.cell(xx, yy)
            if 0 <= i < r.nx and 0 <= j < r.ny and not bv[j, i] and not bt[0][j, i] and not bt[1][j, i]:
                # keep a little extra room so the pour can actually reach the via
                win = bv[max(0, j - 4):j + 5, max(0, i - 4):i + 5]
                if win.any():
                    continue
                v = pcbnew.PCB_VIA(board)
                v.SetPosition(pcbnew.VECTOR2I(MM(float(xx)), MM(float(yy))))
                v.SetDrill(MM(D.VIA_DRILL))
                v.SetWidth(MM(D.VIA_D))
                v.SetNet(nets["GND"])
                board.Add(v)
                nstitch += 1

    # --------------------------------------------------------- GND pours
    for layer in (pcbnew.F_Cu, pcbnew.B_Cu):
        z = pcbnew.ZONE(board)
        z.SetLayer(layer)
        z.SetNet(nets["GND"])
        z.SetLocalClearance(MM(0.3))
        z.SetMinThickness(MM(0.25))
        z.SetThermalReliefGap(MM(0.4))
        z.SetThermalReliefSpokeWidth(MM(0.45))
        z.SetPadConnection(pcbnew.ZONE_CONNECTION_THERMAL)
        z.SetIslandRemovalMode(pcbnew.ISLAND_REMOVAL_MODE_ALWAYS)
        ol = z.Outline()
        ol.NewOutline()
        bx0, by0 = D.PAGE_OX - D.BOARD_W / 2 - 1, D.PAGE_OY - D.BOARD_H / 2 - 1
        for px, py in ((bx0, by0), (bx0 + D.BOARD_W + 2, by0),
                       (bx0 + D.BOARD_W + 2, by0 + D.BOARD_H + 2), (bx0, by0 + D.BOARD_H + 2)):
            ol.Append(MM(px), MM(py))
        board.Add(z)
    pcbnew.ZONE_FILLER(board).Fill(board.Zones())

    pcbnew.SaveBoard(pcb_path, board)
    print(f"  stitching vias: {nstitch}; failed nets: {failed or 'none'}")
    return pcb_path, failed


# --------------------------------------------------------------------------
# silkscreen / fab artwork
# --------------------------------------------------------------------------

def text(board, s, xy, size=1.2, layer=pcbnew.F_SilkS, rot=0, bold=False, mirror=None):
    t = pcbnew.PCB_TEXT(board)
    t.SetText(s)
    t.SetPosition(P(*xy))
    t.SetLayer(layer)
    t.SetTextSize(pcbnew.VECTOR2I(MM(size), MM(size)))
    t.SetTextThickness(MM(size * (0.2 if bold else 0.15)))
    t.SetTextAngleDegrees(rot)
    if mirror if mirror is not None else layer in (pcbnew.B_SilkS, pcbnew.B_Fab):
        t.SetMirrored(True)
    board.Add(t)
    return t


def decorate(board, fps, rst_sep):
    F, B = pcbnew.F_SilkS, pcbnew.B_SilkS
    for ref, fp in fps.items():
        rt = fp.Reference()
        rt.SetTextSize(pcbnew.VECTOR2I(MM(0.9), MM(0.9)))
        rt.SetTextThickness(MM(0.14))
        if ref.startswith(("SW", "U", "H", "BZ", "J")):
            rt.SetVisible(False)      # function labels below say what these are
        fp.Value().SetVisible(False)

    def xy(ref):
        return next(p["xy"] for p in D.PARTS if p["ref"] == ref)

    # front: title + legends
    text(board, "STM32 ARCADE", (0, -44.0), 2.0, F, bold=True)
    text(board, "NYCU Maker  |  carrier for mcudev STM32F4XX_Pro  |  v0.1 2026-10",
         (0, 44.3), 1.0, F)
    dx, dy, s = D.DPAD_X, D.DPAD_Y, D.DPAD_S
    text(board, "UP", (dx - 9.2, dy - s), 1.2, F)
    text(board, "DOWN", (dx - 9.8, dy + s), 1.2, F)
    text(board, "LEFT", (dx - s, dy - 7.8), 1.2, F)
    text(board, "RIGHT", (dx + s, dy - 7.8), 1.2, F)
    ax, ay = xy("SW5")
    bx, by = xy("SW6")
    text(board, "A", (ax, ay + 8.2), 1.6, F, bold=True)
    text(board, "BACK", (bx, by + 8.2), 1.4, F, bold=True)
    zx, zy = xy("BZ1")
    text(board, "BUZZER", (zx, zy + 8.4), 1.0, F)
    text(board, "STICK (3V3!)", (D.JOY_X, D.JOY_Y + 15.6), 1.0, F)
    rx, ry = D.rst_hole()
    if rst_sep:
        text(board, "RST", (rx - 3.4, ry), 0.9, F, rot=90)
    else:
        text(board, "RST", (rx - 4.6, ry - 1.6), 0.9, F)
    jx, jy = xy("J3")
    for layer in (F, B):
        text(board, "SPK", (jx - 3.4, jy), 0.9, layer, rot=90)
    text(board, "+", (jx + 2.0, jy - 1.27), 1.0, B)
    text(board, "J3", (jx - 0.1, jy + 3.6), 0.8, F)

    # back: where the DEB goes (outline offset so it clears the socket silk)
    dx0, dy0 = D.deb_to_board(0, 0)
    dx1, dy1 = D.deb_to_board(D.DEB_W, D.DEB_H)
    add_rect(board, dx0 - 0.6, dy0 - 0.6, dx1 + 0.6, dy1 + 0.6, B, 0.15)
    text(board, "STM32F4XX_Pro (DevEBox) plugs in here, component side facing this board",
         (0, dy1 + 4.6), 1.0, B)
    text(board, "USB / JTAG side", (dx0 + 1.8, 0), 1.2, B, rot=90)
    text(board, "New-TFT side", (dx1 - 1.8, 0), 1.2, B, rot=90)
    x1, _ = D.header_pin_xy(1, 1)
    text(board, "<- pin 1   J1 = DEB header 1 (PE1 ... 3V3)", (x1 - 17.0, dy0 - 2.2), 0.9, B)
    text(board, "<- pin 1   J2 = DEB header 2 (PB9 ... GND)", (x1 - 17.0, dy1 + 2.2), 0.9, B)
    # fab layer: TFT module outline for reference
    cx, cy = D.DEB_X + D.TFT_DX, D.DEB_Y + D.TFT_DY
    add_rect(board, cx - D.TFT_PCB_W / 2, cy - D.TFT_PCB_H / 2,
             cx + D.TFT_PCB_W / 2, cy + D.TFT_PCB_H / 2, pcbnew.F_Fab, 0.1)
    text(board, "TFT module (placeholder size)", (cx, cy), 1.5, pcbnew.F_Fab)
