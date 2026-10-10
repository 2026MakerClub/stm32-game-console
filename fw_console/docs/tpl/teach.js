/* =====================================================================
   互動式程式架構導覽 —— 邏輯層
   SRC / CHAPTERS 由 build_teach.py 在上方注入
   ===================================================================== */
const $ = (s, r) => (r || document).querySelector(s);
const el = (t, c, h) => { const n = document.createElement(t);
  if (c) n.className = c; if (h != null) n.innerHTML = h; return n; };

const KEY = "fwconsole.teach.v1";
let SAVE = {done:{}, ex:{}};
try { const r = localStorage.getItem(KEY); if (r) SAVE = Object.assign(SAVE, JSON.parse(r)); } catch(e){}
const save = () => { try { localStorage.setItem(KEY, JSON.stringify(SAVE)); } catch(e){} };

let cur = 0, curFile = null;

/* ---------------------------------------------------------------
   深入連結。HW_SITE 任一項留空就渲染成灰色的佔位，不會變成死連結。
   --------------------------------------------------------------- */
function deepLink(key, label){
  const u = (HW_SITE && HW_SITE[key]) ? String(HW_SITE[key]).trim() : "";
  const txt = label || "想更深入";
  if (!u) return `<span class="deep off" title="在檔案開頭的 HW_SITE 填上網址就會變成連結">${txt}（待填）</span>`;
  return `<a class="deep" href="${u}" target="_blank" rel="noopener">${txt}</a>`;
}

/* ===============================================================
   程式碼面板
   =============================================================== */
function renderTabs(ch){
  const t = $("#tabs"); t.innerHTML = "";
  ch.files.forEach(f => {
    const b = el("button", f === curFile ? "cur" : "", f.split("/").pop());
    b.title = f;
    b.onclick = () => showFile(f);
    t.appendChild(b);
  });
}
function showFile(f, line){
  curFile = f;
  const src = SRC[f];
  const wrap = $("#codewrap");
  if (!src){ wrap.innerHTML = '<pre style="padding:16px">找不到 ' + f + '</pre>'; return; }
  const pre = el("pre");
  let html = "";
  for (let i = 0; i < src.length; i++){
    html += '<div class="cl" id="L' + (i+1) + '"><i>' + (i+1) + '</i><span>' + src[i] + '</span></div>';
  }
  pre.innerHTML = html;
  wrap.innerHTML = "";
  wrap.appendChild(pre);
  $$("#tabs button").forEach(b => b.classList.toggle("cur", b.title === f));
  if (line) markLine(line);
}
const $$ = (s, r) => Array.from((r || document).querySelectorAll(s));

function markLine(line){
  const wrap = $("#codewrap");
  $$(".cl.marked", wrap).forEach(n => n.classList.remove("marked","flash"));
  const n = $("#L" + line, wrap);
  if (!n) return;
  /* 先閃兩下，動畫結束後留下固定的標示 */
  n.classList.add("flash");
  n.addEventListener("animationend", () => {
    n.classList.remove("flash"); n.classList.add("marked");
  }, {once:true});
  n.classList.add("marked");
  const top = n.offsetTop - wrap.clientHeight / 2 + n.offsetHeight / 2;
  wrap.scrollTo({top: Math.max(0, top), behavior:"smooth"});
}
function jumpTo(f, line, why){
  const ch = CHAPTERS[cur];
  if (!ch.files.includes(f)) ch.files = ch.files.concat([f]);
  renderTabs(ch);
  if (f !== curFile) showFile(f, line); else markLine(line);
  if (why) $("#fcap").innerHTML = why;
  const wb = $("#wb");
  const r = wb.getBoundingClientRect();
  if (r.top < 0 || r.top > window.innerHeight * 0.6)
    wb.scrollIntoView({behavior:"smooth", block:"start"});
}

/* ===============================================================
   流程圖：絕對定位的節點 + SVG 連線
   =============================================================== */
function renderFlow(ch){
  const host = $("#canv");
  host.innerHTML = "";
  const fl = ch.flow;
  host.style.width = fl.w + "px";
  host.style.height = fl.h + "px";

  const NS = "http://www.w3.org/2000/svg";
  const svg = document.createElementNS(NS, "svg");
  svg.setAttribute("width", fl.w); svg.setAttribute("height", fl.h);
  const defs = document.createElementNS(NS, "defs");
  defs.innerHTML = '<marker id="ah" viewBox="0 0 10 10" refX="9" refY="5" ' +
    'markerWidth="7" markerHeight="7" orient="auto-start-reverse">' +
    '<path d="M0,0 L10,5 L0,10 z" fill="currentColor"/></marker>';
  svg.appendChild(defs);
  host.appendChild(svg);

  const byId = {};
  fl.nodes.forEach(n => {
    byId[n.id] = n;
    const d = el("div", "nd" + (n.cls ? " " + n.cls : ""));
    d.style.left = n.x + "px"; d.style.top = n.y + "px";
    d.style.width = n.w + "px"; d.style.height = n.h + "px";
    d.innerHTML = "<b>" + n.t + "</b>" + (n.sub ? "<em>" + n.sub + "</em>" : "");
    d.tabIndex = 0;
    d.dataset.id = n.id;
    const pick = () => {
      $$(".nd", host).forEach(x => x.classList.remove("sel"));
      d.classList.add("sel");
      $$(".edge", svg).forEach(e => e.classList.toggle("hi",
        e.dataset.a === n.id || e.dataset.b === n.id));
      $("#fcap").innerHTML = (n.cap || "") +
        (n.tgt ? ' <button class="jump" data-f="' + n.tgt.f + '" data-l="' + n.tgt.l +
          '">' + n.tgt.f.split("/").pop() + ":" + n.tgt.l + '</button>' +
          ' <span class="hint">（雙擊節點也可以）</span>' : "");
      bindJumps($("#fcap"));
    };
    d.onclick = pick;
    d.ondblclick = () => { pick(); if (n.tgt) jumpTo(n.tgt.f, n.tgt.l, n.cap || ""); };
    d.onkeydown = ev => {
      if (ev.key === "Enter"){ pick(); if (n.tgt) jumpTo(n.tgt.f, n.tgt.l, n.cap || ""); }
    };
    host.appendChild(d);
  });

  /* 連線 */
  fl.edges.forEach(e => {
    const a = byId[e.a], b = byId[e.b];
    if (!a || !b) return;
    const p = document.createElementNS(NS, "path");
    p.setAttribute("class", "edge");
    p.dataset.a = e.a; p.dataset.b = e.b;
    p.setAttribute("marker-end", "url(#ah)");
    p.setAttribute("d", route(a, b, e.side));
    svg.appendChild(p);
    if (e.lab){
      const tx = document.createElementNS(NS, "text");
      const m = midOf(a, b, e.side);
      tx.setAttribute("class","elab");
      tx.setAttribute("x", m[0]); tx.setAttribute("y", m[1]);
      tx.textContent = e.lab;
      svg.appendChild(tx);
    }
  });
}
function cx(n){ return n.x + n.w/2; }
function cy(n){ return n.y + n.h/2; }
function route(a, b, side){
  if (side === "r"){                                   /* a 右 → b 左 */
    const x1 = a.x + a.w, y1 = cy(a), x2 = b.x, y2 = cy(b);
    const mx = (x1 + x2) / 2;
    return `M${x1},${y1} L${mx},${y1} L${mx},${y2} L${x2},${y2}`;
  }
  if (side === "l"){                                   /* a 左 → b 右 */
    const x1 = a.x, y1 = cy(a), x2 = b.x + b.w, y2 = cy(b);
    const mx = (x1 + x2) / 2;
    return `M${x1},${y1} L${mx},${y1} L${mx},${y2} L${x2},${y2}`;
  }
  if (side === "back"){                                /* 迴圈：繞左邊回去 */
    const x1 = a.x, y1 = cy(a), x2 = b.x, y2 = cy(b);
    const off = Math.min(a.x, b.x) - 26;
    return `M${x1},${y1} L${off},${y1} L${off},${y2} L${x2},${y2}`;
  }
  if (side === "backr"){                               /* 迴圈：繞右邊回去 */
    const x1 = a.x + a.w, y1 = cy(a), x2 = b.x + b.w, y2 = cy(b);
    const off = Math.max(a.x + a.w, b.x + b.w) + 26;
    return `M${x1},${y1} L${off},${y1} L${off},${y2} L${x2},${y2}`;
  }
  /* 預設：a 下 → b 上 */
  const x1 = cx(a), y1 = a.y + a.h, x2 = cx(b), y2 = b.y;
  if (Math.abs(x1 - x2) < 2) return `M${x1},${y1} L${x2},${y2}`;
  const my = (y1 + y2) / 2;
  return `M${x1},${y1} L${x1},${my} L${x2},${my} L${x2},${y2}`;
}
function midOf(a, b, side){
  if (side === "r") return [(a.x + a.w + b.x)/2 + 4, (cy(a) + cy(b))/2 - 5];
  if (side === "l") return [(a.x + b.x + b.w)/2 + 4, (cy(a) + cy(b))/2 - 5];
  if (side === "back" || side === "backr") return [Math.min(a.x, b.x) - 22, (cy(a)+cy(b))/2];
  return [cx(a) + 7, (a.y + a.h + b.y)/2 + 3];
}

/* 縮放 */
let zoom = 1;
function setZoom(z){
  zoom = Math.max(0.5, Math.min(1.4, z));
  $("#canv").style.transform = "scale(" + zoom + ")";
  $("#zlab").textContent = Math.round(zoom * 100) + "%";
}

/* ===============================================================
   拖曳分隔線
   =============================================================== */
function initGrip(){
  const grip = $("#grip"), pane = $("#cpane"), split = $("#split");
  let on = false;
  const move = ev => {
    if (!on) return;
    const r = split.getBoundingClientRect();
    const x = (ev.touches ? ev.touches[0].clientX : ev.clientX) - r.left;
    const pct = Math.max(22, Math.min(78, x / r.width * 100));
    pane.style.flex = "0 0 " + pct + "%";
    ev.preventDefault();
  };
  const up = () => { on = false; grip.classList.remove("on");
    document.body.style.userSelect = ""; };
  grip.onmousedown = grip.ontouchstart = ev => {
    on = true; grip.classList.add("on");
    document.body.style.userSelect = "none"; ev.preventDefault();
  };
  window.addEventListener("mousemove", move, {passive:false});
  window.addEventListener("touchmove", move, {passive:false});
  window.addEventListener("mouseup", up);
  window.addEventListener("touchend", up);
  grip.ondblclick = () => { pane.style.flex = "0 0 52%"; };
}

/* ===============================================================
   章節
   =============================================================== */
function bindJumps(root){
  $$(".jump", root).forEach(b => {
    if (b.dataset.bound) return;
    b.dataset.bound = "1";
    b.onclick = () => jumpTo(b.dataset.f, +b.dataset.l, b.dataset.why || "");
  });
}
function chapterDone(ch){
  if (SAVE.done[ch.n]) return true;
  const ex = ch.ex || [];
  return ex.length > 0 && ex.every(e => SAVE.ex[e.id] === true);
}
function showChapter(i){
  cur = i;
  const ch = CHAPTERS[i];
  const done = CHAPTERS.filter(chapterDone).length;

  $$("#chapnav button").forEach((b, k) => {
    if (k === i) b.setAttribute("aria-current","page"); else b.removeAttribute("aria-current");
  });

  $("#intro").innerHTML =
    '<div class="eyebrow">第 ' + ch.n + ' 章 / ' + CHAPTERS.length + '　·　已完成 ' +
    done + ' / ' + CHAPTERS.length + '</div>' +
    '<h2>' + ch.title + '</h2>' +
    '<div class="problem"><h4>這一章在解決什麼問題</h4><p>' + ch.problem + '</p></div>';

  $("#flowname").textContent = ch.flowName || "流程";
  curFile = ch.files[0];
  renderTabs(ch);
  renderFlow(ch);
  showFile(ch.focus.f, ch.focus.l);
  $("#fcap").innerHTML = ch.flowHint ||
    "點節點看說明，<b>雙擊</b>節點會讓左邊跳到對應的程式行並標示起來。";

  $("#prose").innerHTML = ch.html;
  bindJumps($("#prose"));
  $$(".deepslot", $("#prose")).forEach(s => { s.outerHTML = deepLink(s.dataset.k, s.dataset.l); });

  const dh = $("#demos"); dh.innerHTML = "";
  (ch.demos || []).forEach(d => { if (DEMOS[d]) DEMOS[d](dh); });

  const eh = $("#exs"); eh.innerHTML = "";
  renderEx(ch, eh);

  const nf = $("#navfoot"); nf.innerHTML = "";
  const prev = el("button","b", i > 0 ? "← 第 " + CHAPTERS[i-1].n + " 章　" + CHAPTERS[i-1].title : "← 第一章");
  prev.onclick = () => showChapter(Math.max(0, i-1));
  const mid = el("button","b" + (chapterDone(ch) ? " done" : ""),
    chapterDone(ch) ? "✓ 已完成這一章" : "標記這一章完成");
  mid.onclick = () => { SAVE.done[ch.n] = !SAVE.done[ch.n]; save(); showChapter(i); };
  const next = el("button","b", i < CHAPTERS.length-1
    ? "第 " + CHAPTERS[i+1].n + " 章　" + CHAPTERS[i+1].title + " →" : "最後一章");
  next.onclick = () => showChapter(Math.min(CHAPTERS.length-1, i+1));
  nf.append(prev, mid, next);

  $("#pb").style.width = (done / CHAPTERS.length * 100) + "%";
  try { history.replaceState(null,"","#c" + ch.n); } catch(e){}
  window.scrollTo({top:0});
}

/* ===============================================================
   練習
   =============================================================== */
function renderEx(ch, host){
  const ex = ch.ex || [];
  if (!ex.length) return;
  host.appendChild(el("h3", null, "練習"));
  ex.forEach(e => {
    const box = el("div","ex");
    if (SAVE.ex[e.id] === true) box.dataset.s = "ok";
    else if (SAVE.ex[e.id] === false) box.dataset.s = "no";
    box.appendChild(el("span","exn",
      (e.type === "mc" ? "問答" : e.type === "fill" ? "填空" : "排序") + " · " + e.id));
    box.appendChild(el("p","exq", e.q));
    const fb = el("div","fb"); fb.hidden = true;
    const done = ok => { SAVE.ex[e.id] = ok; save(); box.dataset.s = ok ? "ok" : "no";
      $("#pb").style.width = (CHAPTERS.filter(chapterDone).length / CHAPTERS.length * 100) + "%"; };

    if (e.type === "mc"){
      const opts = el("div","opts");
      e.opts.forEach((o, oi) => {
        const l = el("label","opt");
        l.innerHTML = '<input type="radio" name="' + e.id + '" value="' + oi + '"><span>' + o + '</span>';
        opts.appendChild(l);
      });
      box.appendChild(opts);
      const b = el("button","b","送出");
      b.style.background = "var(--tx)"; b.style.color = "var(--bg)"; b.style.borderColor = "var(--tx)";
      b.onclick = () => {
        const s = box.querySelector('input[name="' + e.id + '"]:checked');
        if (!s){ fb.hidden = false; fb.className = "fb no"; fb.innerHTML = "<b>先選一個</b>"; return; }
        const ok = +s.value === e.ans;
        $$(".opt", opts).forEach((l, li) => {
          l.classList.remove("r","w");
          if (li === e.ans) l.classList.add("r"); else if (li === +s.value) l.classList.add("w");
        });
        fb.hidden = false; fb.className = "fb " + (ok ? "ok" : "no");
        fb.innerHTML = "<b>" + (ok ? "對" : "再想一次") + "</b>" + e.fb;
        bindJumps(fb); done(ok);
      };
      box.appendChild(b);
    } else if (e.type === "fill"){
      const ta = e.multi ? el("textarea","exin") : el("input","exin");
      if (!e.multi) ta.type = "text";
      ta.placeholder = e.ph || ""; box.appendChild(ta);
      const row = el("div","drow"); row.style.marginTop = "10px";
      const b = el("button","b","批改");
      b.style.background = "var(--tx)"; b.style.color = "var(--bg)"; b.style.borderColor = "var(--tx)";
      const bs = el("button","b","看答案");
      bs.style.background = "var(--card)"; bs.style.color = "var(--tx)"; bs.style.borderColor = "var(--ln)";
      row.append(b, bs); box.appendChild(row);
      b.onclick = () => {
        const t = (ta.value || "").replace(/\s+/g," ").trim();
        if (!t){ fb.hidden = false; fb.className = "fb no"; fb.innerHTML = "<b>還沒寫</b>"; return; }
        const hay = [t, t.toUpperCase(), t.toLowerCase()];
        let miss = null;
        for (const [re, hint] of e.need){ if (!hay.some(x => re.test(x))){ miss = hint; break; } }
        const ok = !miss;
        fb.hidden = false; fb.className = "fb " + (ok ? "ok" : "no");
        fb.innerHTML = ok ? "<b>過了</b>" + e.fb : "<b>還差一點</b>" + miss;
        bindJumps(fb); done(ok);
      };
      bs.onclick = () => { fb.hidden = false; fb.className = "fb";
        fb.innerHTML = "<b>參考答案</b><pre>" + e.ans + "</pre><div style='margin-top:.6em'>" + e.fb + "</div>";
        bindJumps(fb); };
    } else { /* sort */
      const ul = el("ul","sortl");
      const order = e.items.map((_, i) => i);
      for (let i = order.length - 1; i > 0; i--){
        const j = (i * 7 + 3) % (i + 1);           /* 固定打亂，重載後一樣 */
        [order[i], order[j]] = [order[j], order[i]];
      }
      order.forEach(ix => {
        const li = el("li"); li.draggable = true; li.dataset.ix = ix;
        li.innerHTML = '<span class="h">≡</span><span class="t">' + e.items[ix] + '</span>';
        li.ondragstart = ev => { li.classList.add("drag");
          ev.dataTransfer.setData("text/plain", ix); };
        li.ondragend = () => li.classList.remove("drag");
        li.ondragover = ev => {
          ev.preventDefault();
          const d = $(".drag", ul); if (!d || d === li) return;
          const r = li.getBoundingClientRect();
          ul.insertBefore(d, (ev.clientY - r.top) / r.height > .5 ? li.nextSibling : li);
        };
        ul.appendChild(li);
      });
      ul.ondragover = ev => ev.preventDefault();
      box.appendChild(ul);
      const b = el("button","b","檢查順序");
      b.style.background = "var(--tx)"; b.style.color = "var(--bg)"; b.style.borderColor = "var(--tx)";
      b.onclick = () => {
        const got = $$("li", ul).map(li => +li.dataset.ix);
        const ok = got.every((v, i) => v === i);
        fb.hidden = false; fb.className = "fb " + (ok ? "ok" : "no");
        fb.innerHTML = ok ? "<b>順序正確</b>" + e.fb
          : "<b>順序還不對</b>" + (e.wrongFb || "") + "<br>目前第一個是「"
            + e.items[got[0]].replace(/<[^>]+>/g,"") + "」。" + e.hintFirst;
        bindJumps(fb); done(ok);
      };
      box.appendChild(b);
    }
    box.appendChild(fb);
    host.appendChild(box);
  });
  host.appendChild(el("p","hint",
    "批改是關鍵字比對，寫法不同但意思對的可能被判不過，按「看答案」自己核對。紀錄存在這個瀏覽器。"));
}

/* ===============================================================
   啟動
   =============================================================== */
const nav = $("#chapnav");
CHAPTERS.forEach((ch, i) => {
  const b = el("button", null, String(ch.n).padStart(2,"0") + " " + ch.title);
  b.onclick = () => showChapter(i);
  nav.appendChild(b);
});
initGrip();
$("#zin").onclick = () => setZoom(zoom + .1);
$("#zout").onclick = () => setZoom(zoom - .1);
$("#zrst").onclick = () => setZoom(1);

const tb = $("#themeBtn");
const tlab = () => tb.textContent =
  document.documentElement.getAttribute("data-theme") === "dark" ? "淺色" : "深色";
tb.onclick = () => {
  const d = document.documentElement.getAttribute("data-theme") === "dark";
  document.documentElement.setAttribute("data-theme", d ? "light" : "dark");
  try { localStorage.setItem("fwconsole.theme", d ? "light" : "dark"); } catch(e){}
  tlab();
};
try { const t = localStorage.getItem("fwconsole.theme");
  if (t) document.documentElement.setAttribute("data-theme", t); } catch(e){}
tlab();

(function(){
  const m = /^#c(\d{1,2})$/.exec(location.hash || "");
  const i = m ? CHAPTERS.findIndex(c => c.n === +m[1]) : 0;
  showChapter(i >= 0 ? i : 0);
})();
