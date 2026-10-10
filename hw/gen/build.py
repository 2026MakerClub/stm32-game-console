"""
build.py - regenerate the whole carrier project and its fab outputs.

    "C:\\Program Files\\KiCad\\10.0\\bin\\python.exe" hw\\gen\\build.py

Outputs go to hw/stm32_console_carrier/:
    *.kicad_pro / *.kicad_sch / *.kicad_pcb   open these in KiCad 10
    fab/   Gerbers + drill, zipped for JLCPCB/PCBWay
    erc.rpt, drc.rpt, bom.csv, render_*.png
"""
import json
import os
import shutil
import subprocess
import sys
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import design as D            # noqa: E402
import fp_custom              # noqa: E402
import make_sch               # noqa: E402
import make_pcb               # noqa: E402

NAME = "stm32_console_carrier"
OUT = os.path.join(os.path.dirname(HERE), NAME)
CLI = r"C:\Program Files\KiCad\10.0\bin\kicad-cli.exe"


def cli(*args, check=False):
    r = subprocess.run([CLI, *args], capture_output=True, text=True,
                       encoding="utf-8", errors="replace")
    if r.returncode and check:
        print(r.stdout, r.stderr)
        raise SystemExit("kicad-cli failed: " + " ".join(args))
    return r


def write_project():
    pro = os.path.join(OUT, NAME + ".kicad_pro")
    data = {}
    if os.path.exists(pro):
        with open(pro, encoding="utf-8") as f:
            data = json.load(f)
    data.setdefault("meta", {"filename": NAME + ".kicad_pro", "version": 3})
    nc = data.setdefault("net_settings", {}).setdefault("classes", [{}])
    nc[0].update({"name": "Default", "clearance": D.CLEARANCE, "track_width": D.TRACK_SIG,
                  "via_diameter": D.VIA_D, "via_drill": D.VIA_DRILL})
    with open(pro, "w", encoding="utf-8", newline="\n") as f:
        json.dump(data, f, indent=2)


def main():
    os.makedirs(OUT, exist_ok=True)
    for stale in (NAME + ".kicad_pcb", NAME + ".kicad_sch"):
        p = os.path.join(OUT, stale)
        if os.path.exists(p):
            os.remove(p)
    print("[1/5] libraries")
    fp_custom.write_libs(OUT)
    print("[2/5] schematic")
    sch, uuids = make_sch.build(OUT, NAME)
    print("[3/5] pcb")
    pcb, failed = make_pcb.build(OUT, NAME, uuids, NAME + ".kicad_sch")
    write_project()

    print("[4/5] checks")
    cli("sch", "erc", "--severity-all", "-o", os.path.join(OUT, "erc.rpt"), sch)
    cli("pcb", "drc", "--schematic-parity", "--severity-all", "--refill-zones",
        "-o", os.path.join(OUT, "drc.rpt"), pcb)
    for rpt in ("erc.rpt", "drc.rpt"):
        with open(os.path.join(OUT, rpt), encoding="utf-8") as f:
            lines = [l.rstrip() for l in f if l.startswith("**") or l.startswith("[")]
        print(f"  {rpt}:")
        for l in lines[:40]:
            print("   ", l)

    print("[5/5] fab outputs")
    fab = os.path.join(OUT, "fab")
    shutil.rmtree(fab, ignore_errors=True)
    os.makedirs(fab)
    cli("pcb", "export", "gerbers", "--no-protel-ext", "-l",
        "F.Cu,B.Cu,F.Paste,B.Paste,F.SilkS,B.SilkS,F.Mask,B.Mask,Edge.Cuts",
        "--subtract-soldermask", "-o", fab + os.sep, pcb, check=True)
    cli("pcb", "export", "drill", "--format", "excellon", "--excellon-separate-th",
        "--generate-map", "--map-format", "pdf", "-o", fab + os.sep, pcb, check=True)
    zpath = os.path.join(OUT, NAME + "_gerbers.zip")
    with zipfile.ZipFile(zpath, "w", zipfile.ZIP_DEFLATED) as z:
        for fn in sorted(os.listdir(fab)):
            z.write(os.path.join(fab, fn), fn)
    cli("sch", "export", "bom", "--fields",
        "Reference,Value,Footprint,Description,${QUANTITY}", "--labels",
        "Refs,Value,Footprint,Description,Qty", "--group-by", "Value,Footprint",
        "--exclude-dnp", "-o", os.path.join(OUT, "bom.csv"), sch)
    cli("sch", "export", "pdf", "-o", os.path.join(OUT, NAME + "_schematic.pdf"), sch)
    cli("pcb", "export", "pdf", "--layers", "Edge.Cuts,F.SilkS,F.Cu",
        "-o", os.path.join(OUT, "fit_1to1.pdf"), pcb)
    for side in ("top", "bottom"):
        cli("pcb", "render", "--side", side, "--width", "2000", "--height", "1100",
            "--quality", "high", "--background", "opaque", "--zoom", "1.9",
            "-o", os.path.join(OUT, f"render_{side}.png"), pcb)
    print("done ->", OUT, "| unrouted:", failed or "none")


if __name__ == "__main__":
    main()
