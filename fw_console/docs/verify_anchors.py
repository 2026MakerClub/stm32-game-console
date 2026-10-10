# -*- coding: utf-8 -*-
"""把產生出來的頁面裡所有落點抽出來，回去對照真實檔案，確認沒有一個指到空行或超出檔尾。"""
import io, os, random, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
PAGE = os.path.join(ROOT, u"fw_console_程式教學.html")

h = io.open(PAGE, encoding="utf-8").read()

pairs = set()
# prose 裡的跳躍鈕（被 json.dumps 轉義過，所以引號前面有反斜線）
for m in re.finditer(r'data-f=\\"(.+?)\\" data-l=\\"(\d+)\\"', h):
    pairs.add((m.group(1), int(m.group(2))))
# 流程圖節點與章節初始焦點
for m in re.finditer(r'(?:tgt|focus):\{f:"(.+?)",l:(\d+)\}', h):
    pairs.add((m.group(1), int(m.group(2))))

print(u"頁面裡不同的落點：%d 處" % len(pairs))

cache = {}
def srcline(rel, n):
    if rel not in cache:
        p = os.path.join(ROOT, rel.replace("/", os.sep))
        with io.open(p, encoding="utf-8", errors="replace") as f:
            cache[rel] = f.read().replace("\r\n", "\n").replace("\r", "\n").split("\n")
    L = cache[rel]
    return L[n - 1] if 0 < n <= len(L) else None

bad = []
for rel, n in sorted(pairs):
    t = srcline(rel, n)
    if t is None:
        bad.append((rel, n, u"超出檔尾"))
    elif t.strip() in ("", "{", "}", "*/"):
        bad.append((rel, n, u"指到空行或只有括號：%r" % t.strip()))

print(u"指到空行、括號或檔尾外的落點：%d 處" % len(bad))
for rel, n, why in bad:
    print(u"   %s :%d  %s" % (rel, n, why))

random.seed(11)
print(u"\n--- 隨機抽 8 處回頭驗證 ---")
for rel, n in random.sample(sorted(pairs), min(8, len(pairs))):
    print(u"%-30s :%-5d %s" % (rel, n, srcline(rel, n).strip()[:76]))

sys.exit(1 if bad else 0)
