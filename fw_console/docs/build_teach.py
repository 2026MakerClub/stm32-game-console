# -*- coding: utf-8 -*-
"""
產生 fw_console_程式教學.html

設計重點：所有行號都是拿一段真實的程式碼字串去檔案裡找出來的，沒有一個是寫死的。
需要的字串（needle）在原始碼裡找不到時，這支程式會直接失敗並告訴你哪一個，
所以程式一改動、教材的落點就會立刻被抓出來，不會悄悄指到錯的地方。

用法：  python docs/build_teach.py
"""
import io, json, os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)                       # fw_console/
sys.path.insert(0, os.path.join(HERE, "tpl"))

from chapters_a import CHAPTERS_A                  # noqa: E402
from chapters_b import CHAPTERS_B                  # noqa: E402

CHAPTERS = CHAPTERS_A + CHAPTERS_B

# 要嵌進頁面的原始碼。刻意不含 dino_sprites.h（541 行十六進位，讀它沒有意義）
# 也不含 3739 行的 LCD 驅動，那部分在第 11 章用文字說明。
FILES = [
    "App/app_main.c", "App/console.h", "App/menu.c", "App/ui.c",
    "App/gfx.c", "App/gfx.h", "App/input.c", "App/input.h",
    "App/sound.c", "App/sound.h",
    "App/game_dino.c", "App/game_2048.c", "App/game_gate.c", "App/iotest.c",
    "User_common/user_defined.c", "SPI_touch/Driver_SPI.c",
]

# ----------------------------------------------------------------------
# C 語法上色。在這裡做完，頁面就不用帶一個上色函式庫。
# ----------------------------------------------------------------------
KW = set("""if else for while do switch case default break continue return goto
sizeof typedef struct union enum static const volatile extern inline register
__inline __weak __IO""".split())
TY = set("""void char short int long float double signed unsigned
uint8_t uint16_t uint32_t uint64_t int8_t int16_t int32_t int64_t
size_t bool""".split())

def esc(s):
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")

TOKEN = re.compile(r"""
    (?P<bc>/\*)                      |
    (?P<lc>//)                       |
    (?P<str>"(?:\\.|[^"\\])*")       |
    (?P<chr>'(?:\\.|[^'\\])*')       |
    (?P<num>\b0[xX][0-9a-fA-F]+[uUlL]*\b|\b\d+[uUlL]*\b) |
    (?P<id>[A-Za-z_]\w*)
""", re.X)

def hl_line(line, in_bc):
    """回傳 (上色後的 HTML, 離開這一行時是否還在區塊註解裡)"""
    out, i, n = [], 0, len(line)

    if in_bc:
        end = line.find("*/")
        if end < 0:
            return '<span class="tk-c">%s</span>' % esc(line), True
        out.append('<span class="tk-c">%s</span>' % esc(line[:end + 2]))
        i = end + 2
        in_bc = False

    # 前置處理器指令：整行的第一個非空白是 #
    stripped = line[i:].lstrip()
    if stripped.startswith("#"):
        lead = line[i:len(line) - len(line[i:].lstrip())]
        m = re.match(r"(#\s*\w+)(.*)$", stripped, re.S)
        if m:
            rest, rest_bc = hl_line(m.group(2), False)
            return ("".join(out) + esc(lead) +
                    '<span class="tk-p">%s</span>' % esc(m.group(1)) + rest), rest_bc

    while i < n:
        m = TOKEN.search(line, i)
        if not m:
            out.append(esc(line[i:]))
            break
        if m.start() > i:
            out.append(esc(line[i:m.start()]))
        k = m.lastgroup
        t = m.group()
        if k == "bc":
            end = line.find("*/", m.end())
            if end < 0:
                out.append('<span class="tk-c">%s</span>' % esc(line[m.start():]))
                return "".join(out), True
            out.append('<span class="tk-c">%s</span>' % esc(line[m.start():end + 2]))
            i = end + 2
            continue
        if k == "lc":
            out.append('<span class="tk-c">%s</span>' % esc(line[m.start():]))
            break
        if k in ("str", "chr"):
            out.append('<span class="tk-s">%s</span>' % esc(t))
        elif k == "num":
            out.append('<span class="tk-n">%s</span>' % esc(t))
        else:
            if t in KW:
                out.append('<span class="tk-k">%s</span>' % esc(t))
            elif t in TY:
                out.append('<span class="tk-t">%s</span>' % esc(t))
            elif re.match(r"^[A-Z][A-Za-z0-9_]*$", t) and t.isupper():
                out.append('<span class="tk-n">%s</span>' % esc(t))
            elif line[m.end():m.end() + 1] == "(":
                out.append('<span class="tk-f">%s</span>' % esc(t))
            else:
                out.append(esc(t))
        i = m.end()
    return "".join(out), in_bc

# ----------------------------------------------------------------------
# 讀檔並建立 needle → 行號 的查找
# ----------------------------------------------------------------------
def decode(raw):
    u"""課程提供的檔案不一定是 UTF-8：Driver_SPI.c 的註解裡就有一個 cp1252 的省略號。
    用 errors="replace" 會把它換成 U+FFFD，所以這裡依序試幾種編碼。"""
    for enc in ("utf-8", "cp950", "cp1252", "latin-1"):
        try:
            return raw.decode(enc)
        except UnicodeDecodeError:
            continue
    return raw.decode("utf-8", errors="replace")

RAW, SRC = {}, {}
for rel in FILES:
    p = os.path.join(ROOT, rel.replace("/", os.sep))
    with io.open(p, "rb") as f:
        text = decode(f.read())
    assert u"�" not in text, u"%s 解碼之後還有 U+FFFD" % rel
    lines = text.replace("\r\n", "\n").replace("\r", "\n").split("\n")
    if lines and lines[-1] == "":
        lines.pop()
    RAW[rel] = lines
    hl, bc = [], False
    for ln in lines:
        h, bc = hl_line(ln, bc)
        hl.append(h)
    SRC[rel] = hl

RESOLVED, MISSES, AMBIG = [], [], []

def line_of(rel, needle):
    """用去掉前後空白的字串比對，所以縮排改了也還找得到。"""
    if rel not in RAW:
        MISSES.append((rel, needle, "這個檔案沒有被嵌進頁面"))
        return 1
    key = needle.strip()
    hits = [i + 1 for i, ln in enumerate(RAW[rel]) if key in ln]
    if not hits:
        MISSES.append((rel, needle, "找不到"))
        return 1
    if len(hits) > 1:
        AMBIG.append((rel, needle, hits))
    RESOLVED.append((rel, hits[0], key))
    return hits[0]

JUMP = re.compile(r"\{\{J:([^:]+(?:::)??[^:]*?)::(.+?)::(.+?)\}\}", re.S)
JUMP2 = re.compile(r"\{\{J:(.+?)::(.+?)::(.+?)\}\}", re.S)

def expand(html):
    def sub(m):
        rel, needle, label = m.group(1), m.group(2), m.group(3)
        ln = line_of(rel, needle)
        return ('<button class="jump" data-f="%s" data-l="%d">%s</button>'
                % (rel, ln, label))
    return JUMP2.sub(sub, html)

# ----------------------------------------------------------------------
# 組出 JS
# ----------------------------------------------------------------------
def js(o):
    return json.dumps(o, ensure_ascii=False)

def emit_ex(ex):
    parts = []
    for e in ex:
        f = ['id:%s' % js(e["id"]), 'type:%s' % js(e["type"]), 'q:%s' % js(e["q"])]
        if e["type"] == "mc":
            f.append('opts:[%s]' % ",".join(js(o) for o in e["opts"]))
            f.append('ans:%d' % e["ans"])
        elif e["type"] == "fill":
            if e.get("multi"): f.append('multi:true')
            f.append('ph:%s' % js(e.get("ph", "")))
            f.append('need:[%s]' % ",".join(
                "[%s,%s]" % (rx, js(hint)) for rx, hint in e["need"]))
            f.append('ans:%s' % js(e["ans"]))
        else:
            f.append('items:[%s]' % ",".join(js(i) for i in e["items"]))
            f.append('hintFirst:%s' % js(e.get("hintFirst", "")))
            f.append('wrongFb:%s' % js(e.get("wrongFb", "")))
        f.append('fb:%s' % js(e["fb"]))
        parts.append("{%s}" % ",".join(f))
    return "[%s]" % ",".join(parts)

def emit_flow(fl):
    nodes = []
    for nd in fl["nodes"]:
        f = ['id:%s' % js(nd["id"]), 'x:%d' % nd["x"], 'y:%d' % nd["y"],
             'w:%d' % nd["w"], 'h:%d' % nd["h"], 't:%s' % js(nd["t"])]
        if nd.get("cls"): f.append('cls:%s' % js(nd["cls"]))
        if nd.get("sub"): f.append('sub:%s' % js(nd["sub"]))
        if nd.get("cap"): f.append('cap:%s' % js(nd["cap"]))
        if nd.get("tgt"):
            rel, needle = nd["tgt"]
            f.append('tgt:{f:%s,l:%d}' % (js(rel), line_of(rel, needle)))
        nodes.append("{%s}" % ",".join(f))
    edges = []
    for e in fl["edges"]:
        f = ['a:%s' % js(e["a"]), 'b:%s' % js(e["b"])]
        if e.get("side"): f.append('side:%s' % js(e["side"]))
        if e.get("lab"):  f.append('lab:%s' % js(e["lab"]))
        edges.append("{%s}" % ",".join(f))
    return "{w:%d,h:%d,nodes:[%s],edges:[%s]}" % (
        fl["w"], fl["h"], ",".join(nodes), ",".join(edges))

chunks = []
for ch in CHAPTERS:
    rel, needle = ch["focus"]
    f = ['n:%d' % ch["n"], 'title:%s' % js(ch["title"]),
         'problem:%s' % js(ch["problem"]),
         'files:[%s]' % ",".join(js(x) for x in ch["files"]),
         'focus:{f:%s,l:%d}' % (js(rel), line_of(rel, needle)),
         'flowName:%s' % js(ch.get("flowName", "流程")),
         'flowHint:%s' % js(ch.get("flowHint", "")),
         'flow:%s' % emit_flow(ch["flow"]),
         'html:%s' % js(expand(ch["html"])),
         'demos:[%s]' % ",".join(js(d) for d in ch.get("demos", [])),
         'ex:%s' % emit_ex(ch.get("ex", []))]
    chunks.append("{%s}" % ",".join(f))
CH_JS = "const CHAPTERS=[%s];" % ",".join(chunks)

SRC_JS = "const SRC={%s};" % ",".join(
    "%s:[%s]" % (js(k), ",".join(js(x) for x in v)) for k, v in SRC.items())

def read(p):
    with io.open(os.path.join(HERE, "tpl", p), encoding="utf-8") as f:
        return f.read()

CSS = read("teach.css")
DEMOS = read("demos.js")
LOGIC = read("teach.js")

HEAD = u"""<title>掌機韌體怎麼寫的</title>
<script>
/* ====================================================================
   想更深入的連結都放在這裡。把你那個 STM32 硬體教學站的網址填進來，
   頁面上對應的「想更深入」就會從灰色佔位變成可以點的連結。
   留空的項目不會變成死連結，只會顯示成「（待填）」。
   ==================================================================== */
const HW_SITE = {
  base:    "",   /* 網站首頁，目前沒有用到，留著備用 */
  gpio:    "",   /* GPIO 模式、上拉下拉、active low        → 第 4、11 章 */
  adc:     "",   /* ADC 取樣、解析度、通道設定              → 第 11 章   */
  tim:     "",   /* 計時器、PWM、PSC 與 ARR                → 第 6、11 章 */
  fsmc:    "",   /* FSMC 並列匯流排與 ILI9341              → 第 11 章   */
  systick: ""    /* SysTick 與中斷                         → 第 7、11 章 */
};
</script>
<link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Noto+Serif+TC:wght@500;700&family=Noto+Sans+TC:wght@400;500;700&family=JetBrains+Mono:wght@400;500;600&display=swap">
<style>
%s
</style>

<div id="bar"><div class="barin">
  <span class="brand">掌機韌體<b>怎麼寫的</b></span>
  <nav id="chapnav" aria-label="章節"></nav>
  <button id="themeBtn" class="tbtn" type="button">深色</button>
</div></div>

<main>
  <div class="pbar"><i id="pb" style="width:0%%"></i></div>
  <div class="col" id="intro"></div>

  <div class="wb" id="wb">
    <div class="wbhead">
      <h4>程式 ↔ 流程對照</h4>
      <span class="hint">拖中間的把手可以調寬窄　·　<b>雙擊</b>右邊的節點會讓左邊跳到那一行</span>
    </div>
    <div class="split" id="split">
      <div class="cpane" id="cpane">
        <div class="tabs" id="tabs"></div>
        <div class="codewrap" id="codewrap"></div>
      </div>
      <div class="grip" id="grip" title="拖曳調整，雙擊還原"></div>
      <div class="fpane">
        <div class="fbar">
          <span id="flowname">流程</span>
          <span style="margin-left:auto"></span>
          <button class="tbtn" id="zout" type="button">−</button>
          <span id="zlab">100%%</span>
          <button class="tbtn" id="zin" type="button">+</button>
          <button class="tbtn" id="zrst" type="button">重設</button>
        </div>
        <div class="fscroll"><div class="canv" id="canv"></div></div>
        <div class="fcap" id="fcap"></div>
      </div>
    </div>
  </div>

  <div class="col" id="prose"></div>
  <div id="demos"></div>
  <div class="col" id="exs"></div>
  <div class="col navfoot" id="navfoot"></div>
</main>

<script>
""" % CSS

OUT = HEAD + SRC_JS + "\n" + CH_JS + "\n" + DEMOS + "\n" + LOGIC + "\n</script>\n"

dest = os.path.join(ROOT, u"fw_console_程式教學.html")
with io.open(dest, "w", encoding="utf-8", newline="\n") as f:
    f.write(OUT)

# ----------------------------------------------------------------------
# 回報
# ----------------------------------------------------------------------
print("嵌入原始碼 %d 個檔案，共 %d 行" % (len(SRC), sum(len(v) for v in SRC.values())))
print("章節 %d 章，流程圖 %d 張，節點 %d 個，連線 %d 條" % (
    len(CHAPTERS), len(CHAPTERS),
    sum(len(c["flow"]["nodes"]) for c in CHAPTERS),
    sum(len(c["flow"]["edges"]) for c in CHAPTERS)))
print("互動示範 %d 種，練習 %d 題" % (
    len(set(d for c in CHAPTERS for d in c.get("demos", []))),
    sum(len(c.get("ex", [])) for c in CHAPTERS)))
print("解析落點 %d 處" % len(RESOLVED))
print("輸出 %s（%.0f KB）" % (dest, os.path.getsize(dest) / 1024.0))

if AMBIG:
    print("\n--- 以下 needle 命中多行，用了第一個（可以接受，但確認一下） ---")
    for rel, nd, hits in AMBIG:
        print("  %s  %r  → %s" % (rel, nd[:56], hits[:6]))
if MISSES:
    print("\n*** 失敗：以下 needle 找不到，落點會是錯的 ***")
    for rel, nd, why in MISSES:
        print("  %s  (%s)  %r" % (rel, why, nd[:72]))
    sys.exit(1)
print("\n全部落點都對上了。")
