/* =====================================================================
   互動示範。重現的是邏輯，不是畫面；行為以真實硬體為準。
   ===================================================================== */
function dshell(title, note){
  const w = el("div","demo");
  const h = el("div","dhead");
  h.appendChild(el("h4", null, title));
  h.appendChild(el("span","sim", note || "瀏覽器示意"));
  w.appendChild(h);
  const b = el("div","dbody");
  w.appendChild(b);
  return {w:w, b:b};
}

const DEMOS = {};

/* ---------------------------------------------------------------
   1. 碰撞框視覺化
   --------------------------------------------------------------- */
DEMOS.collide = function(host){
  const d = dshell("碰撞框視覺化 · dino_hits()", "示意：可拖動，判定式與真實程式相同");
  const W = 520, H = 190;
  const cv = el("canvas"); cv.width = W; cv.height = H;
  cv.style.width = "100%"; cv.style.cursor = "grab";
  d.b.appendChild(cv);
  const out = el("div","dout"); out.style.marginTop = "12px"; d.b.appendChild(out);
  const note = el("div","dnote"); d.b.appendChild(note);
  const ctx = cv.getContext("2d");

  /* 數字取自 game_dino.c：站立的恐龍框 22×28，仙人掌框比 sprite 內縮 3 px */
  const A = {x:40, y:96, w:22, h:28, n:"恐龍"};
  const B = {x:250, y:100, w:9, h:20, n:"仙人掌"};
  let drag = null, off = [0,0];

  function overlap(){
    const c1 = (A.x + A.w <= B.x), c2 = (B.x + B.w <= A.x);
    const c3 = (A.y + A.h <= B.y), c4 = (B.y + B.h <= A.y);
    return {hit: !(c1 || c2 || c3 || c4), c1, c2, c3, c4};
  }
  function draw(){
    const r = overlap();
    ctx.fillStyle = "#0A0E0C"; ctx.fillRect(0,0,W,H);
    ctx.strokeStyle = "#1E2A24"; ctx.beginPath();
    ctx.moveTo(0, 124.5); ctx.lineTo(W, 124.5); ctx.stroke();
    ctx.fillStyle = "#5A6B76"; ctx.font = "11px monospace";
    ctx.fillText("GROUND_Y", 6, 138);

    [[A, r.hit ? "#E0553A" : "#7FD98C"], [B, r.hit ? "#E0553A" : "#3FC4D0"]].forEach(([o,c]) => {
      ctx.fillStyle = c + "33"; ctx.fillRect(o.x, o.y, o.w, o.h);
      ctx.strokeStyle = c; ctx.lineWidth = 1.5;
      ctx.strokeRect(o.x + .5, o.y + .5, o.w - 1, o.h - 1);
      ctx.fillStyle = c; ctx.font = "11px sans-serif";
      ctx.fillText(o.n, o.x, o.y - 5);
      ctx.fillStyle = "#5A6B76"; ctx.font = "10px monospace";
      ctx.fillText(o.x + "," + o.y + " " + o.w + "×" + o.h, o.x, o.y + o.h + 13);
    });
    const T = v => v ? '<span style="color:#7FD98C">true </span>' : '<span style="color:#5A6B76">false</span>';
    out.innerHTML =
      "ax+aw &lt;= bx  → " + T(r.c1) + "　|　bx+bw &lt;= ax  → " + T(r.c2) + "\n" +
      "ay+ah &lt;= by  → " + T(r.c3) + "　|　by+bh &lt;= ay  → " + T(r.c4) + "\n" +
      '<span style="color:' + (r.hit ? "#E0553A" : "#7FD98C") + '">→ ' +
      (r.hit ? "重疊，撞到了（return 1）" : "沒有重疊（return 0）") + "</span>";
    note.innerHTML = r.hit
      ? "四個條件都不成立，表示兩個矩形在 X 和 Y 上都有交集。程式的寫法是<b>反過來想</b>：只要找到一條分離的軸就立刻回傳沒撞到，比直接證明重疊好寫得多。"
      : "只要<b>任何一個</b>條件成立，就代表存在一條軸讓兩個矩形分開，可以馬上回傳 0。" +
        "注意用的是 <code>&lt;=</code> 而不是 <code>&lt;</code>：剛好貼齊不算撞到。";
  }
  function pos(ev){
    const r = cv.getBoundingClientRect();
    const s = W / r.width;
    const p = ev.touches ? ev.touches[0] : ev;
    return [(p.clientX - r.left) * s, (p.clientY - r.top) * s];
  }
  function hitTest(x, y){
    if (x >= B.x-4 && x <= B.x+B.w+4 && y >= B.y-4 && y <= B.y+B.h+4) return B;
    if (x >= A.x-4 && x <= A.x+A.w+4 && y >= A.y-4 && y <= A.y+A.h+4) return A;
    return null;
  }
  cv.onmousedown = cv.ontouchstart = ev => {
    const [x,y] = pos(ev); const o = hitTest(x,y);
    if (o){ drag = o; off = [x - o.x, y - o.y]; cv.style.cursor = "grabbing"; ev.preventDefault(); }
  };
  window.addEventListener("mousemove", ev => {
    if (!drag) return;
    const [x,y] = pos(ev);
    drag.x = Math.max(0, Math.min(W - drag.w, x - off[0]));
    drag.y = Math.max(0, Math.min(H - drag.h - 18, y - off[1]));
    draw();
  });
  cv.addEventListener("touchmove", ev => {
    if (!drag) return;
    const [x,y] = pos(ev);
    drag.x = Math.max(0, Math.min(W - drag.w, x - off[0]));
    drag.y = Math.max(0, Math.min(H - drag.h - 18, y - off[1]));
    draw(); ev.preventDefault();
  }, {passive:false});
  const up = () => { drag = null; cv.style.cursor = "grab"; };
  window.addEventListener("mouseup", up); window.addEventListener("touchend", up);
  draw();
  host.appendChild(d.w);
};

/* ---------------------------------------------------------------
   2. 2048 的一次移動，單步執行
   --------------------------------------------------------------- */
DEMOS.move2048 = function(host){
  const d = dshell("Move_Left 單步執行", "示意：演算法與 game_2048.c 相同");
  const presets = [
    [2,2,4,0], [0,2,0,2], [4,4,4,4], [2,0,2,4], [0,0,0,8], [2,4,2,4]
  ];
  let row = presets[0].slice(), stage = 0, tmp = [], res = [], gained = 0;

  const ctl = el("div","drow");
  const sel = el("select");
  presets.forEach((p,i) => sel.appendChild(el("option", null, "[ " + p.join(", ") + " ]")));
  const bStep = el("button","b am","下一步");
  const bRst = el("button","b","重來");
  const dd = el("div"); dd.appendChild(el("label",null,"起始的一列")); dd.appendChild(sel);
  ctl.append(dd, bStep, bRst);
  d.b.appendChild(ctl);

  const W = 520, H = 210;
  const cv = el("canvas"); cv.width = W; cv.height = H; cv.style.width = "100%";
  cv.style.marginTop = "13px";
  d.b.appendChild(cv);
  const out = el("div","dout"); out.style.marginTop = "11px"; d.b.appendChild(out);
  const note = el("div","dnote"); d.b.appendChild(note);
  const ctx = cv.getContext("2d");

  const COL = {0:"#141A17",2:"#2A3730",4:"#3A4A3E",8:"#8A5A22",16:"#95521E",
    32:"#A83A2A",64:"#B04030",128:"#A88A28",256:"#A88A28",512:"#4A8A40",
    1024:"#3A8A50",2048:"#2A6AA8"};
  function tiles(arr, y, lab){
    ctx.fillStyle = "#5A6B76"; ctx.font = "11px monospace";
    ctx.fillText(lab, 8, y + 26);
    arr.forEach((v, i) => {
      const x = 118 + i * 92;
      ctx.fillStyle = COL[v] || "#2A6AA8";
      ctx.fillRect(x, y, 80, 42);
      ctx.strokeStyle = "#1E2A24"; ctx.strokeRect(x+.5, y+.5, 79, 41);
      if (v){ ctx.fillStyle = v <= 4 ? "#A8B8AE" : "#FFFFFF";
        ctx.font = "600 17px monospace"; ctx.textAlign = "center";
        ctx.fillText(String(v), x + 40, y + 27); ctx.textAlign = "left"; }
    });
  }
  function draw(){
    ctx.fillStyle = "#0A0E0C"; ctx.fillRect(0,0,W,H);
    tiles(row, 12, "原始");
    if (stage >= 1) tiles(tmp, 68, "① 壓縮");
    if (stage >= 2) tiles(tmp, 124, "② 合併");
    if (stage >= 3) tiles(res, 180 - 12, "③ 再壓縮");
  }
  function step(){
    if (stage === 0){
      tmp = []; row.forEach(v => { if (v) tmp.push(v); });
      while (tmp.length < 4) tmp.push(0);
      stage = 1;
      out.innerHTML = "① <b>壓縮</b>：把非零的磁磚依序搬到最左邊，中間的空格被擠掉。\n"
        + "for (c = 0; c &lt; 4; c++) if (g_board[r][c]) tmp[k++] = g_board[r][c];";
      note.innerHTML = "這一步只搬動、不合併。先壓縮的理由是：合併只看「相鄰」的兩格，"
        + "中間夾著空格的 <code>[2,0,2,0]</code> 必須先變成 <code>[2,2,0,0]</code> 才看得出可以合。";
    } else if (stage === 1){
      gained = 0;
      for (let c = 0; c < 3; c++){
        if (tmp[c] && tmp[c] === tmp[c+1]){ tmp[c] *= 2; gained += tmp[c]; tmp[c+1] = 0; }
      }
      stage = 2;
      out.innerHTML = "② <b>合併</b>：從左往右掃，相鄰相等就把左邊那格加倍、右邊清零。得分 +"
        + gained + "\n"
        + "if (tmp[c] &amp;&amp; tmp[c] == tmp[c+1]) { tmp[c] &lt;&lt;= 1; score += tmp[c]; tmp[c+1] = 0; }";
      note.innerHTML = "迴圈從左往右且<b>不回頭</b>，所以 <code>[4,4,4,4]</code> 會變成 "
        + "<code>[8,0,8,0]</code> 而不是 <code>[16,0,0,0]</code>——一顆磁磚在同一次移動裡只能合一次，"
        + "這是 2048 的規則，也是迴圈上限寫 <code>c &lt; BOARD_N - 1</code> 而不是巢狀迴圈的原因。";
    } else if (stage === 2){
      res = []; tmp.forEach(v => { if (v) res.push(v); });
      while (res.length < 4) res.push(0);
      stage = 3;
      const moved = res.some((v,i) => v !== row[i]);
      out.innerHTML = "③ <b>再壓縮</b>：合併在中間留下了空格，再擠一次。\n"
        + "最後比對前後是否不同 → moved = " + (moved ? "1（算一次有效移動）" : "0（盤面沒變，不算）");
      note.innerHTML = "為什麼要第三步：② 把 <code>tmp[c+1]</code> 清成 0，"
        + "於是 <code>[2,2,4,0]</code> 在合併後是 <code>[4,0,4,0]</code>，中間破了一個洞。"
        + "<b>moved 這個回傳值很重要</b>：只有真的動過才會生出新磁磚，"
        + "否則玩家亂滑一個沒有效果的方向也會被塞磁磚。";
    } else {
      stage = 0; tmp = []; res = [];
      out.innerHTML = "回到起點，再按一次「下一步」。";
      note.innerHTML = "四個方向各寫一份函式，而不是轉置陣列再呼叫 Move_Left。"
        + "程式碼是多了三倍，但每一份都只有一層索引、沒有座標轉換，"
        + "在沒有除錯器的環境裡遠比「聰明」的寫法好查。";
    }
    draw();
  }
  sel.onchange = () => { row = presets[sel.selectedIndex].slice(); stage = 0;
    tmp = []; res = []; draw();
    out.innerHTML = "按「下一步」開始。"; note.innerHTML = "&nbsp;"; };
  bStep.onclick = step;
  bRst.onclick = () => { stage = 0; tmp = []; res = []; draw();
    out.innerHTML = "按「下一步」開始。"; note.innerHTML = "&nbsp;"; };
  draw();
  out.innerHTML = "按「下一步」開始，會依序跑壓縮、合併、再壓縮三個階段。";
  note.innerHTML = "&nbsp;";
  host.appendChild(d.w);
};

/* ---------------------------------------------------------------
   3. 四通道開火與冷卻
   --------------------------------------------------------------- */
DEMOS.lanes = function(host){
  const d = dshell("四通道開火 · 260 ms 冷卻", "示意：冷卻邏輯與 game_gate.c 相同");
  const NAMES = ["UP","DOWN","LEFT","RIGHT"];
  const COOL = 260;
  const ready = [0,0,0,0];
  let now = 0, fired = 0, wasted = 0, t0 = performance.now();

  const ctl = el("div","drow");
  NAMES.forEach((n, i) => {
    const b = el("button","b", n);
    b.onclick = () => fire(i);
    ctl.appendChild(b);
  });
  const bRst = el("button","b","歸零");
  bRst.onclick = () => { fired = wasted = 0; ready.fill(0); };
  ctl.appendChild(bRst);
  d.b.appendChild(ctl);

  const W = 520, H = 170;
  const cv = el("canvas"); cv.width = W; cv.height = H; cv.style.width = "100%";
  cv.style.marginTop = "13px"; d.b.appendChild(cv);
  const out = el("div","dout"); out.style.marginTop = "11px"; d.b.appendChild(out);
  const note = el("div","dnote"); d.b.appendChild(note);
  const ctx = cv.getContext("2d");

  function fire(i){
    if (ready[i] > now){ wasted++; return; }   /* 冷卻中，這次按鍵沒有作用 */
    ready[i] = now + COOL;
    fired++;
  }
  function loop(){
    now = performance.now() - t0;
    ctx.fillStyle = "#0A0E0C"; ctx.fillRect(0,0,W,H);
    NAMES.forEach((n, i) => {
      const y = 14 + i * 38, rem = Math.max(0, ready[i] - now);
      const pct = rem / COOL;
      ctx.fillStyle = "#5A6B76"; ctx.font = "11px monospace";
      ctx.fillText(n, 8, y + 18);
      ctx.fillStyle = "#141A17"; ctx.fillRect(62, y, 400, 24);
      ctx.fillStyle = pct > 0 ? "#3A2A14" : "#17291B";
      ctx.fillRect(62, y, 400, 24);
      if (pct > 0){
        ctx.fillStyle = "#E8A93C"; ctx.fillRect(62, y, 400 * pct, 24);
        ctx.fillStyle = "#241700"; ctx.font = "600 11px monospace";
        ctx.fillText("冷卻 " + Math.ceil(rem) + " ms", 70, y + 16);
      } else {
        ctx.fillStyle = "#7FD98C"; ctx.font = "600 11px monospace";
        ctx.fillText("READY", 70, y + 16);
      }
      ctx.strokeStyle = "#1E2A24"; ctx.strokeRect(62.5, y+.5, 399, 23);
    });
    out.innerHTML = "成功開火 " + fired + " 次　|　冷卻中被吃掉 " + wasted + " 次"
      + (wasted > 0 ? '\n<span style="color:#E8A93C">按太快的那幾下完全沒有效果</span>' : "");
    requestAnimationFrame(loop);
  }
  note.innerHTML = "試著瘋狂連打同一顆：成功次數不會變多，只是「被吃掉」的計數往上跳。"
    + "判斷只有一行 <code>if (s_pulse[lane].readyAt &gt; msTick) return;</code>，"
    + "但它決定了這個遊戲考的是<b>時機</b>而不是手速。"
    + "四條通道各有自己的冷卻，所以同時照顧四邊是可行的——難的是分配注意力。";
  loop();
  host.appendChild(d.w);
};

/* ---------------------------------------------------------------
   4. 固定步長迴圈怎麼補時間
   --------------------------------------------------------------- */
DEMOS.frame = function(host){
  const d = dshell("固定步長迴圈 · 補時間", "示意：時間軸是畫出來的，不是量測結果");
  let workMs = 6, tick = 20;

  const ctl = el("div","drow");
  const dw = el("div"); dw.style.flex = "1 1 240px";
  dw.appendChild(el("label",null,"這一幀的工作花了 6 ms"));
  const sw = el("input"); sw.type = "range"; sw.min = "1"; sw.max = "40"; sw.value = "6";
  dw.appendChild(sw);
  const dt = el("div");
  dt.appendChild(el("label",null,"TICK_MS"));
  const st = el("select");
  [["20","20　Dino"],["25","25　Gate Keeper"],["10","10　2048 與選單"]].forEach(([v,t]) => {
    const o = el("option", null, t); o.value = v; st.appendChild(o); });
  dt.appendChild(st);
  ctl.append(dw, dt);
  d.b.appendChild(ctl);

  const W = 520, H = 180;
  const cv = el("canvas"); cv.width = W; cv.height = H; cv.style.width = "100%";
  cv.style.marginTop = "13px"; d.b.appendChild(cv);
  const out = el("div","dout"); out.style.marginTop = "11px"; d.b.appendChild(out);
  const note = el("div","dnote"); d.b.appendChild(note);
  const ctx = cv.getContext("2d");

  function draw(){
    const px = 9;                                    /* 1 ms = 9 px */
    ctx.fillStyle = "#0A0E0C"; ctx.fillRect(0,0,W,H);
    const over = workMs > tick;

    for (let f = 0; f < 3; f++){
      const y = 22 + f * 48;
      const x0 = 46 + f * (Math.max(workMs, tick) * px + 6);
      ctx.fillStyle = "#5A6B76"; ctx.font = "11px monospace";
      if (f === 0) ctx.fillText("幀", 8, y + 16);
      ctx.fillText("#" + (f+1), 8, y + 16);
      /* 工作 */
      ctx.fillStyle = over ? "#7A2A1E" : "#1E6A8A";
      ctx.fillRect(x0, y, workMs * px, 22);
      ctx.fillStyle = "#CFE0D6"; ctx.font = "600 10px monospace";
      if (workMs * px > 34) ctx.fillText(workMs + "ms 工作", x0 + 4, y + 15);
      /* 補時間 */
      if (!over){
        ctx.fillStyle = "#17291B";
        ctx.fillRect(x0 + workMs * px, y, (tick - workMs) * px, 22);
        ctx.strokeStyle = "#2C4A33";
        ctx.strokeRect(x0 + workMs*px + .5, y + .5, (tick-workMs)*px - 1, 21);
        ctx.fillStyle = "#7FD98C"; ctx.font = "10px monospace";
        if ((tick-workMs)*px > 30) ctx.fillText("等 " + (tick-workMs) + "ms", x0 + workMs*px + 4, y + 15);
      }
      /* 幀界線 */
      const end = x0 + Math.max(workMs, tick) * px;
      ctx.strokeStyle = over ? "#E0553A" : "#3FC4D0";
      ctx.setLineDash([3,3]); ctx.beginPath();
      ctx.moveTo(end + .5, y - 6); ctx.lineTo(end + .5, y + 30); ctx.stroke();
      ctx.setLineDash([]);
    }
    out.innerHTML = over
      ? 'TICK_MS = ' + tick + '　工作 = ' + workMs + ' ms\n'
        + '<span style="color:#E0553A">工作超過一幀的預算，while 迴圈的條件一開始就不成立，'
        + '完全不等 → 這一幀變成 ' + workMs + ' ms，遊戲會變慢</span>'
      : 'TICK_MS = ' + tick + '　工作 = ' + workMs + ' ms　補 ' + (tick - workMs) + ' ms\n'
        + '<span style="color:#7FD98C">每一幀都剛好 ' + tick + ' ms，速度穩定</span>';
    note.innerHTML = over
      ? "這就是「畫面越複雜遊戲越慢」的來源。修法不是把 TICK_MS 加大，而是讓繪製變便宜——"
        + "第 5 章的只重畫變動區塊就是在做這件事。"
      : "工作發生在等待<b>之前</b>，所以某一幀畫得久一點，等待就自動變短，總長不變。"
        + "換成 <code>delay_ms(20)</code> 的話總長會變成「工作 + 20」，速度就跟著畫面複雜度飄。";
    draw2();
  }
  function draw2(){
    dw.firstChild.textContent = "這一幀的工作花了 " + workMs + " ms";
  }
  sw.oninput = () => { workMs = +sw.value; draw(); };
  st.onchange = () => { tick = +st.value; draw(); };
  draw();
  host.appendChild(d.w);
};
