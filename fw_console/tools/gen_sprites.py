# Convert Chrome-Dino-Runner PNG assets into 1bpp packed C arrays for STM32.
from PIL import Image
import os

SRC = r"C:\Users\user\Downloads\2026_Fall\maker\stm32 game\Chrome-Dino-Runner\assets"
OUT = r"C:\Users\user\Downloads\2026_Fall\maker\stm32 game\fw_console\App\dino_sprites.h"

# (c_name, relative path, target height)  width scales proportionally
JOBS = [
    ("dino_run1",  r"Dino\DinoRun1.png",        34),
    ("dino_run2",  r"Dino\DinoRun2.png",        34),
    ("dino_jump",  r"Dino\DinoJump.png",        34),
    ("dino_dead",  r"Dino\DinoDead.png",        34),
    ("dino_duck1", r"Dino\DinoDuck1.png",       22),
    ("dino_duck2", r"Dino\DinoDuck2.png",       22),
    ("dino_idle",  r"Dino\DinoStart.png",       34),
    ("cac_s1",     r"Cactus\SmallCactus1.png",  26),
    ("cac_s2",     r"Cactus\SmallCactus2.png",  26),
    ("cac_s3",     r"Cactus\SmallCactus3.png",  26),
    ("cac_l1",     r"Cactus\LargeCactus1.png",  34),
    ("cac_l2",     r"Cactus\LargeCactus2.png",  34),
    ("bird1",      r"Bird\Bird1.png",           24),
    ("bird2",      r"Bird\Bird2.png",           24),
    ("cloud",      r"Other\Cloud.png",          18),
    ("gameover",   r"Other\GameOver.png",       14),
]

def convert(path, th):
    im = Image.open(path).convert("RGBA")
    w, h = im.size
    tw = max(1, round(w * th / h))
    im = im.resize((tw, th), Image.LANCZOS)
    px = im.load()
    stride = (tw + 7) // 8
    data = bytearray(stride * th)
    for y in range(th):
        for x in range(tw):
            r, g, b, a = px[x, y]
            # asset art is a dark silhouette on transparent background
            on = (a > 110) and ((r + g + b) // 3 < 200)
            if on:
                data[y * stride + (x >> 3)] |= 0x80 >> (x & 7)
    return tw, th, stride, data

lines = []
lines.append("/* ============================================================")
lines.append(" * dino_sprites.h - GENERATED, do not hand-edit.")
lines.append(" *")
lines.append(" * Source : Chrome-Dino-Runner assets (PNG)")
lines.append(" * Tool   : tools/gen_sprites.py")
lines.append(" * Format : 1 bit per pixel, MSB first, row-major.")
lines.append(" *          Each row is padded to a whole number of bytes,")
lines.append(" *          so one row occupies ((w + 7) / 8) bytes.")
lines.append(" *          bit = 1 -> draw foreground, bit = 0 -> background.")
lines.append(" * ============================================================ */")
lines.append("#ifndef __DINO_SPRITES_H")
lines.append("#define __DINO_SPRITES_H")
lines.append("")
lines.append("#include <stdint.h>")
lines.append("")
lines.append("typedef struct {")
lines.append("    uint8_t        w;      /* width  in pixels               */")
lines.append("    uint8_t        h;      /* height in pixels               */")
lines.append("    const uint8_t *bits;   /* (w+7)/8 bytes per row, h rows  */")
lines.append("} Sprite;")
lines.append("")

total = 0
decls = []
for name, rel, th in JOBS:
    tw, th2, stride, data = convert(os.path.join(SRC, rel), th)
    total += len(data)
    lines.append("/* %-10s %2d x %2d px, %4d bytes */" % (name, tw, th2, len(data)))
    lines.append("static const uint8_t %s_bits[] = {" % name)
    for y in range(th2):
        row = data[y * stride:(y + 1) * stride]
        lines.append("    " + ", ".join("0x%02X" % b for b in row) + ",")
    lines.append("};")
    lines.append("static const Sprite SPR_%s = { %d, %d, %s_bits };" % (name.upper(), tw, th2, name))
    lines.append("")
    decls.append((name, tw, th2, len(data)))

lines.append("#endif /* __DINO_SPRITES_H */")

with open(OUT, "w", encoding="utf-8", newline="\n") as f:
    f.write("\n".join(lines))

for n, w, h, b in decls:
    print("%-10s %3d x %3d  %5d B" % (n, w, h, b))
print("TOTAL %d bytes of flash" % total)
