# -*- coding: utf-8 -*-
"""第 7 到 12 章。"""

AM="App/app_main.c"; CH="App/console.h"; MN="App/menu.c"; UI="App/ui.c"
IN="App/input.c"; IH="App/input.h"; GF="App/gfx.c"; GH="App/gfx.h"
SC="App/sound.c"; SH="App/sound.h"; DN="App/game_dino.c"; TW="App/game_2048.c"
GT="App/game_gate.c"; IO="App/iotest.c"; UD="User_common/user_defined.c"
DS="SPI_touch/Driver_SPI.c"

CHAPTERS_B = [

# ===================================================================== 7
{
"n":7, "title":"遊戲迴圈骨架",
"problem":"三款遊戲的主迴圈長得一樣，因為它們解的是同一個問題：讓世界以固定速度前進，而且不管這一幀畫得多複雜都一樣快。這一章也講那個到處出現的 (int32_t)(msTick - deadline) 寫法為什麼必要。",
"files":[DN,GT,UD],
"focus":(DN,"s_score = s_dist / 8;"),
"flowName":"固定步長的一幀",
"flowHint":"注意等待在最後。順序換了就不是固定步長了。",
"flow":{"w":600,"h":600,"nodes":[
 {"id":"t0","x":190,"y":10,"w":220,"h":42,"cls":"ev","t":"t0 = msTick","sub":"記住這一幀的起點",
  "cap":"先取時間，之後所有工作都算在這一幀裡。"},
 {"id":"in","x":190,"y":80,"w":220,"h":46,"cls":"io","t":"Input_Poll()","sub":"取樣一次",
  "tgt":(IN,"void Input_Poll(void)"),
  "cap":"第 4 章。一定在最前面，因為後面的邏輯要用它的結果。"},
 {"id":"su","x":190,"y":152,"w":220,"h":46,"t":"Sound_Update()","sub":"推進音符佇列",
  "tgt":(SC,"void Sound_Update(void)"),
  "cap":"第 6 章。放這裡是因為它便宜且與其他事無關。"},
 {"id":"pr","x":190,"y":224,"w":220,"h":46,"cls":"dec","t":"p = Input_Pressed()","sub":"取出邊緣，讀走即清",
  "tgt":(IN,"uint16_t Input_Pressed(void)"),
  "cap":"一幀只取一次，避免同一個按下被兩段程式各吃一次。"},
 {"id":"bk","x":190,"y":296,"w":220,"h":46,"cls":"dec","t":"KEY1 → return","sub":"最早的出口",
  "tgt":(GT,"if (p & BTN_BACK) { if (s_score > s_best) s_best = s_score; return 0; }"),
  "cap":"先處理退出，省掉後面所有工作。"},
 {"id":"w","x":190,"y":368,"w":220,"h":46,"t":"更新世界","sub":"物理 · 生成 · 碰撞",
  "tgt":(GT,"static void step_world(void)"),
  "cap":"純邏輯，完全不碰畫面。"},
 {"id":"rd","x":190,"y":440,"w":220,"h":46,"t":"render()","sub":"只重畫變動的區塊",
  "tgt":(GT,"static void render(void)"),
  "cap":"第 5 章。畫面永遠是世界狀態的函數，不是反過來。"},
 {"id":"wt","x":170,"y":512,"w":260,"h":54,"cls":"dec","t":"while (msTick - t0 < TICK_MS)","sub":"把剩下的時間補滿",
  "cap":"<b>一定在最後。</b>工作久一點，等待就自動短一點，一幀總長不變。"},
],"edges":[
 {"a":"t0","b":"in"},{"a":"in","b":"su"},{"a":"su","b":"pr"},{"a":"pr","b":"bk"},
 {"a":"bk","b":"w"},{"a":"w","b":"rd"},{"a":"rd","b":"wt"},
 {"a":"wt","b":"t0","side":"back","lab":"下一幀"},
]},
"html":"""
<h3>先做事，最後才等</h3>
<p>三款遊戲的主迴圈都是這個形狀：取 <code>t0</code>、讀輸入、更新世界、繪製、然後等到這一幀的時間額度用完。</p>
<p>關鍵是<b>等待在最後</b>。如果某一幀因為障礙物多而畫得久，等待自動變短，一幀的總長仍然是 <code>TICK_MS</code>。換成「先 <code>delay_ms(20)</code> 再做事」的話，總長就變成「工作 + 20」，畫面越複雜遊戲越慢。</p>
<p>這也讓「每幀移動幾個像素」這種說法有意義。Dino 的速度常數從 3.0 爬到 8.0 像素／幀，配 20 毫秒一幀，就是 150 到 400 像素／秒。沒有固定步長的話這些數字什麼都不代表。</p>

<h3>順序不是隨便排的</h3>
<ul>
<li><b>輸入在最前</b>，因為後面每一段都要用它的結果。</li>
<li><b>邊緣只取一次</b>。<code>Input_Pressed()</code> 讀走就清，所以一幀裡只能呼叫一次，結果存進區域變數給後面用。呼叫兩次的話第二次會拿到空的。</li>
<li><b>退出判斷很早</b>，省掉後面所有工作。</li>
<li><b>更新世界完全不碰畫面</b>，繪製完全不改世界。這個分界讓「畫面是世界狀態的函數」成立，除錯時只要盯狀態就好。</li>
</ul>

<h3>那個到處出現的相減寫法</h3>
<p><code>msTick</code> 是 32 位元的毫秒計數器，約 49.7 天後會繞回 0。程式裡所有時間比較都寫成相減：</p>
<pre class="note" style="font-family:var(--sm);font-size:12.5px;white-space:pre">while (msTick - t0 &lt; TICK_MS) { }                   /* 等一幀       */
if ((int32_t)(msTick - s_noteEnd) &gt;= 0) { ... }     /* 音符時間到了 */
if (s_pulse[lane].readyAt &gt; msTick) return;         /* 還在冷卻     */</pre>
<p>為什麼不寫 <code>msTick &gt; t0 + TICK_MS</code>：假設 <code>t0</code> 是 0xFFFFFFF8、<code>TICK_MS</code> 是 20，那麼 <code>t0 + 20</code> 會溢位成 12。條件 <code>msTick &gt; 12</code> 在 <code>msTick</code> 還是 0xFFFFFFFA 的時候就成立了，於是那一幀完全不等。</p>
<p>改成相減就沒事：<code>0xFFFFFFFA - 0xFFFFFFF8</code> 在無號數運算下正好是 2，完全正確。<b>無號數的減法在模 2³² 下是封閉的</b>，差值永遠是真實的間隔，只要間隔小於 2³¹。</p>
<p>需要判斷「某個時刻到了沒有」而不是「過了多久」時，要多一個轉型：<code>(int32_t)(msTick - deadline) &gt;= 0</code>。把差值當成有號數，於是「還沒到」是負數、「到了」是零或正數，同樣能跨越繞回點。</p>
<div class="note"><b>49.7 天聽起來不重要，</b>但這個寫法不是為了那一天。它的價值是「不需要想」——所有時間比較都用同一個形狀，就不會有某一處剛好寫錯。<span class="deepslot" data-k="systick" data-l="msTick 是怎麼來的"></span></div>
""",
"demos":["frame"],
"ex":[
 {"id":"c7a","type":"mc",
  "q":"把主迴圈結尾的 <code>while (msTick - t0 &lt; TICK_MS) { }</code> 換成開頭的 <code>delay_ms(20);</code>，其他不動。症狀是？",
  "opts":["恐龍不動","閃爍變嚴重","幀率不再固定：障礙物多的時候那一幀變長，遊戲速度跟著飄",
          "按鍵反應變快"],
  "ans":2,
  "fb":"原本是「補滿剩下的時間」，總長固定。換成固定延遲後總長變成「工作 + 20」。上面的示範把滑桿拉過 TICK_MS 就能看到這個效果。"},
 {"id":"c7b","type":"mc",
  "q":"某人把冷卻判斷寫成 <code>if (msTick &lt; s_ready) return;</code>，而 <code>s_ready = msTick + 260</code>。這在什麼情況下會出錯？",
  "opts":["永遠不會出錯","<code>msTick + 260</code> 溢位繞回 0 附近時，比較會得到相反的結果",
          "260 太小","只有在 TICK_MS 是 25 的時候"],
  "ans":1,
  "fb":"<code>s_ready</code> 溢位成一個很小的數，於是 <code>msTick &lt; s_ready</code> 立刻不成立，冷卻被跳過。寫成 <code>(int32_t)(msTick - s_ready) &lt; 0</code> 就沒有這個問題。專案裡這一處恰好是 <code>readyAt &gt; msTick</code>，也有同樣的風險，只是要跑 49.7 天才會遇到一次。"},
 {"id":"c7c","type":"fill",
  "q":"寫出「距離 <code>last</code> 已經超過 <code>PERIOD</code> 毫秒」的判斷，要能扛住溢位。",
  "ph":"if ( ... ) { ... }",
  "need":[[r"/msTick/","要用 msTick"],
          [r"/msTick\s*-\s*last/","關鍵是用減法 msTick - last"],
          [r"/PERIOD/","要和 PERIOD 比較"],
          [r"/>=|>/","用大於或大於等於"]],
  "ans":"if (msTick - last &gt;= PERIOD) { ... }",
  "fb":"減法在無號數下跨越繞回點仍然正確。寫成 <code>msTick &gt;= last + PERIOD</code> 就會在 <code>last + PERIOD</code> 溢位時失效。"},
]},

# ===================================================================== 8
{
"n":8, "title":"Dino Run",
"problem":"一個只有跳和蹲的遊戲，難的不是操作而是手感：跳躍弧線要好看、障礙物間距要過得去、速度要越來越快但不能忽然變不可能。這一章看這些數字怎麼被挑出來，以及那個五步教學關怎麼做到邊教邊讓恐龍是活的。",
"files":[DN,GF],
"focus":(DN,"#define GRAVITY16"),
"flowName":"一幀的 Dino",
"flowHint":"物理、生成、碰撞、繪製四段。雙擊節點看程式。",
"flow":{"w":640,"h":620,"nodes":[
 {"id":"st","x":200,"y":10,"w":230,"h":50,"cls":"dec","t":"dino_step(held, pressed)","sub":"跳 / 蹲 / 重力，回傳做了什麼",
  "tgt":(DN,"static uint8_t dino_step(uint16_t held, uint16_t pressed)"),
  "cap":"教學關和正式遊戲共用這一段，所以恐龍在教學時是活的。"},
 {"id":"gr","x":30,"y":90,"w":180,"h":54,"t":"在地面上？","sub":"pressed 有 UP → 起跳",
  "tgt":(DN,"s_vy16     = JUMP_V16;"),
  "cap":"起跳給一個負的垂直速度；蹲下只有在地面才成立。"},
 {"id":"ai","x":420,"y":90,"w":190,"h":54,"t":"在空中","sub":"vy += 重力，y += vy",
  "tgt":(DN,"s_vy16 += GRAVITY16;"),
  "cap":"全部用 1/16 像素的整數，沒有浮點。"},
 {"id":"sc","x":205,"y":166,"w":220,"h":46,"t":"捲動距離 step","sub":"速度累加，取整數部分",
  "tgt":(DN,"s_scrollRem += s_speed16;"),
  "cap":"速度是 1/16 像素，餘數留到下一幀，所以慢速也能平滑。"},
 {"id":"sp","x":205,"y":238,"w":220,"h":50,"cls":"dec","t":"obst_maybe_spawn()","sub":"間距夠了才生成",
  "tgt":(DN,"static void obst_maybe_spawn(void)"),
  "cap":"間距和速度成正比，否則變快之後會變成不可能。"},
 {"id":"cl","x":30,"y":318,"w":180,"h":46,"t":"draw_clouds(step)","sub":"雲以 1/3 速度飄",
  "tgt":(DN,"static void draw_clouds(uint8_t step)"),
  "cap":"視差：遠景比近景慢，畫面才有深度。"},
 {"id":"gd","x":225,"y":318,"w":180,"h":46,"t":"draw_ground(step)","sub":"地面虛線捲動",
  "tgt":(DN,"static void draw_ground(uint8_t step)"),
  "cap":"用一個 0..23 的偏移量做出無限捲動。"},
 {"id":"dd","x":425,"y":318,"w":185,"h":46,"t":"draw_dino()","sub":"整條垂直帶一次送出",
  "tgt":(DN,"static void draw_dino(void)"),
  "cap":"帶寬 44、帶高 100，就是 GFX_BUF_PIX 的由來。"},
 {"id":"mv","x":205,"y":394,"w":220,"h":46,"t":"障礙物往左移","sub":"x -= step，逐一繪製",
  "tgt":(DN,"static void draw_obst(Obst *o, uint8_t step)"),
  "cap":"區塊寬 = sprite 寬 + step，順手擦掉尾巴。"},
 {"id":"hit","x":205,"y":466,"w":220,"h":50,"cls":"dec","t":"dino_hits()","sub":"兩個矩形重疊嗎",
  "tgt":(DN,"static uint8_t dino_hits(const Obst *o)"),
  "cap":"框比 sprite 內縮幾個像素，讓差一點過的情況真的算過。"},
 {"id":"scr","x":205,"y":542,"w":220,"h":50,"cls":"io","t":"分數 = 距離 / 8","sub":"每 100 分一個音",
  "tgt":(DN,"s_score = s_dist / 8;"),
  "cap":"高分存在 RAM，關機就沒了。"},
],"edges":[
 {"a":"st","b":"gr"},{"a":"st","b":"ai","side":"r"},
 {"a":"gr","b":"sc"},{"a":"ai","b":"sc","side":"l"},
 {"a":"sc","b":"sp"},{"a":"sp","b":"cl"},{"a":"sp","b":"gd"},{"a":"sp","b":"dd"},
 {"a":"gd","b":"mv"},{"a":"mv","b":"hit"},{"a":"hit","b":"scr"},
]},
"html":"""
<h3>跳躍弧線：四個整數決定手感</h3>
<p>{{J:App/game_dino.c::#define GRAVITY16::四個常數}}把整個物理定完了：</p>
<pre class="note" style="font-family:var(--sm);font-size:12.5px;white-space:pre">#define GRAVITY16      5        /* 每幀速度加 5/16 像素   */
#define JUMP_V16    (-102)      /* 起跳 −102/16 像素/幀   */
#define SPEED_MIN16   48        /* 3.0 像素/幀            */
#define SPEED_MAX16  128        /* 8.0 像素/幀            */</pre>
<p>單位是 1/16 像素。變數名稱結尾的 <code>16</code> 就是提醒。要換成真正的像素就除以 16，而 16 是 2 的次方，編譯器用一個位移指令解決。</p>
<p>算一下這組數字給出什麼：起跳速度 −102、每幀加 5，20 幀後速度歸零。上升的總距離是 102+97+…+2 除以 16，約 62 像素。再 20 幀落地，所以滯空 40 幀 = 0.8 秒。恐龍高 34 像素，最高的仙人掌 34 像素——跳躍高度大約是障礙物高度的兩倍，時機有容錯空間但不會太寬鬆。</p>
<p>{{J:App/game_dino.c::s_vy16 += GRAVITY16;::物理更新}}是兩行整數加法。另外還有一個細節：在空中按下時會額外多加一次重力，讓玩家可以「快速下墜」，這在原版 Chrome Dino 裡也有。</p>

<h3>捲動用餘數累加</h3>
<p>速度是 1/16 像素，但螢幕只能整數像素。{{J:App/game_dino.c::s_scrollRem += s_speed16;::做法}}是把速度累加進一個餘數，每幀取出整數部分，餘數留給下一幀：</p>
<pre class="note" style="font-family:var(--sm);font-size:12.5px;white-space:pre">s_scrollRem += s_speed16;
step = (uint8_t)(s_scrollRem / 16);
s_scrollRem %= 16;</pre>
<p>於是 3.0 像素／幀真的是平均 3.0，而不是被截成 3 或 4。這是定點數最實用的地方。</p>

<h3>生成規則：間距要跟著速度長大</h3>
<p>{{J:App/game_dino.c::static void obst_maybe_spawn(void)::obst_maybe_spawn()}} 的第一件事是算出這次需要的間距，而間距和速度成正比：</p>
<pre class="note" style="font-family:var(--sm);font-size:12.5px;white-space:pre">int16_t gapMin = (int16_t)(110 + (s_speed16 * 12) / 16);
int16_t gap    = (int16_t)(gapMin + (int16_t)(rnd() % 120));

if (rightmost_obst() &gt; (int16_t)(SCR_W - gap)) return;</pre>
<p>如果間距固定，速度從 150 爬到 400 像素／秒之後，反應時間就只剩三分之一，遊戲會從「難」變成「不可能」。把間距綁上速度，反應時間大致守住。</p>
<p>{{J:App/game_dino.c::static int16_t rightmost_obst(void)::rightmost_obst()}} 回傳最右側障礙物的右緣，沒有障礙物時回傳 −1000。這個哨兵值讓「畫面上沒東西就一定生成」不需要特別寫成一個分支。</p>
<p>障礙物用固定大小的陣列加 <code>active</code> 旗標，最多三個同時存在。沒有動態配置——理由在第 10 章會講。</p>

<h3>碰撞：反過來想比較好寫</h3>
<p>{{J:App/game_dino.c::static uint8_t dino_hits(const Obst *o)::dino_hits()}} 呼叫一個通用的矩形重疊判斷，而它的寫法是<b>找分離軸</b>：</p>
<pre class="note" style="font-family:var(--sm);font-size:12.5px;white-space:pre">if (ax + aw &lt;= bx || bx + bw &lt;= ax) return 0;   /* X 上分開 */
if (ay + ah &lt;= by || by + bh &lt;= ay) return 0;   /* Y 上分開 */
return 1;</pre>
<p>直接證明「重疊」要考慮好幾種相對位置；證明「分開」只要找到一條軸。下面可以拖動兩個框，看四個比較分別成立不成立。</p>
<p>另外框比 sprite 內縮幾個像素（站立時 22×28，而 sprite 是 31×34）。這讓視覺上「差一點擦過去」真的算過去。純幾何正確的碰撞框在遊戲裡通常感覺很爛。</p>

<h3>教學關：共用同一段物理，所以恐龍是活的</h3>
<p>{{J:App/game_dino.c::static uint8_t dino_tutorial(void)::dino_tutorial()}} 不是一串靜態說明。它有自己的主迴圈，而且呼叫的是<b>同一個</b> <code>dino_step()</code>。所以教學進行中恐龍完全可以操作——玩家在被教的同時已經在練。</p>
<p>{{J:App/game_dino.c::static const TutStep TUT[] = {::步驟表}}是一個陣列，每一項有兩行文字和一個「要等什麼輸入」的遮罩：</p>
<pre class="note" style="font-family:var(--sm);font-size:12.5px;white-space:pre">{ "PRESS  UP",         "the dino jumps",         BTN_UP,            0 },
{ "HOLD  DOWN",        "the dino ducks",         BTN_DOWN,          0 },
{ "PUSH A STICK UP",   "either stick jumps too", J1_UP | J2_UP,     0 },
{ "PULL A STICK DOWN", "either stick ducks too", J1_DOWN | J2_DOWN, 0 },
{ "JUMP THE CACTUS",   "wait for it, then press UP", 0,             1 },</pre>
<p>前四步只要對應的位元出現在 <code>pressed</code> 裡就過關。第五步的 <code>cactus</code> 旗標是 1，改成等「障礙物通過」這個事件。</p>
<p>這裡有一個曾經寫錯的地方值得看：一開始「通過」和「跳躍」共用同一個回傳位元，結果第五步在玩家<b>按下跳躍鍵的那一刻</b>就過關了，仙人掌還沒到。修法是把「清掉了」用一個獨立的區域變數 <code>cleared</code> 記，和「玩家做了什麼」分開。<b>兩個語意不同的事件不要擠進同一個旗標</b>。</p>
""",
"demos":["collide"],
"ex":[
 {"id":"c8a","type":"fill",
  "q":"補完矩形重疊判斷裡「X 軸上分開」那一行的條件（a 和 b 兩個框，欄位是 x/y/w/h）。",
  "ph":"if ( ... ) return 0;",
  "need":[[r"/ax\s*\+\s*aw/","一邊是 ax + aw"],
          [r"/bx\s*\+\s*bw/","另一邊是 bx + bw"],
          [r"/\|\|/","兩種情況要用 || 串起來"],
          [r"/<=/","用 &lt;= 而不是 &lt;：剛好貼齊不算撞到"]],
  "ans":"if (ax + aw &lt;= bx || bx + bw &lt;= ax) return 0;",
  "fb":"兩種分開的情況：a 整個在 b 左邊，或 b 整個在 a 左邊。用 <code>&lt;=</code> 讓剛好貼齊算成沒撞到。上面的示範可以把兩個框推到剛好相鄰驗證。"},
 {"id":"c8b","type":"mc",
  "q":"如果障礙物間距寫成固定值而不是跟著速度長大，會發生什麼？",
  "opts":["障礙物會重疊","速度爬到最高之後反應時間只剩三分之一，遊戲變成不可能",
          "分數會算錯","雲會停止移動"],
  "ans":1,
  "fb":"速度從 150 爬到 400 像素／秒，間距不變的話，兩個障礙物之間的<b>時間</b>就縮成不到三分之一。把間距綁上 <code>s_speed16</code> 讓反應時間大致守住，遊戲才是「越來越難」而不是「突然不可能」。"},
 {"id":"c8c","type":"mc",
  "q":"教學第五步原本在玩家按下跳躍鍵的瞬間就過關，而不是等仙人掌通過。根本原因是？",
  "opts":["按鍵去彈跳沒做","「玩家跳了」和「障礙物清掉了」共用同一個回傳位元",
          "TICK_MS 太小","仙人掌生成得太慢"],
  "ans":1,
  "fb":"<code>dino_step()</code> 的回傳值用 bit 0 表示「這一幀起跳了」，而通過檢查也往同一個變數塞 bit 0。兩個語意不同的事件擠在一個旗標裡，就分不出來了。修法是用一個獨立的 <code>cleared</code> 變數。"},
]},

# ===================================================================== 9
{
"n":9, "title":"2048",
"problem":"盤面邏輯是這台機器唯一「純演算法」的部分，沒有時間、沒有硬體，只有一個 4×4 陣列和一次移動要怎麼變換它。這一章把那個三段式拆開，並解釋為什麼四個方向各寫一份反而是對的選擇。",
"files":[TW],
"focus":(TW,"static uint8_t Move_Left(void)"),
"flowName":"一次滑動的完整路徑",
"flowHint":"注意 moved 這個回傳值控制了後面所有事。",
"flow":{"w":620,"h":620,"nodes":[
 {"id":"tp","x":200,"y":10,"w":220,"h":46,"cls":"io","t":"poll_touch()","sub":"按下 → 追蹤 → 放開",
  "tgt":(TW,"static SwipeDir poll_touch(uint8_t *tapped)"),
  "cap":"放開也要去彈跳：連續三次沒摸到才算真的放手。"},
 {"id":"sd","x":200,"y":82,"w":220,"h":50,"cls":"dec","t":"swipe_dir(dx, dy)","sub":"主軸決定方向，太短就忽略",
  "tgt":(TW,"static SwipeDir swipe_dir(int16_t dx, int16_t dy)"),
  "cap":"位移不足 SWIPE_MIN（28 像素）回傳 SW_NONE，避免點一下被當成滑動。"},
 {"id":"ml","x":200,"y":158,"w":220,"h":46,"t":"Move_Left() 等四個","sub":"回傳 moved",
  "tgt":(TW,"static uint8_t Move_Left(void)"),
  "cap":"四個方向各一份，各自只有一層索引。"},
 {"id":"s1","x":25,"y":232,"w":175,"h":50,"t":"① 壓縮","sub":"非零的往左靠",
  "cap":"先擠掉空格，合併才看得出相鄰。"},
 {"id":"s2","x":222,"y":232,"w":175,"h":50,"cls":"dec","t":"② 合併","sub":"相鄰相等就加倍",
  "cap":"從左到右不回頭，所以一顆磁磚一次只合一次。"},
 {"id":"s3","x":419,"y":232,"w":175,"h":50,"t":"③ 再壓縮","sub":"填掉合併留下的洞",
  "cap":"② 把右邊那格清成 0，中間會破洞。"},
 {"id":"mv","x":200,"y":310,"w":220,"h":50,"cls":"dec","t":"moved？","sub":"盤面真的變了嗎",
  "cap":"<b>只有真的動過才繼續</b>。否則玩家亂滑無效方向也會被塞磁磚。"},
 {"id":"ar","x":200,"y":388,"w":220,"h":46,"t":"Add_RandomTile()","sub":"隨機空格放 2 或 4",
  "tgt":(TW,"static void Add_RandomTile(void)"),
  "cap":"先收集所有空格再抽一個，避免「一直抽到滿格」的迴圈。"},
 {"id":"dr","x":200,"y":460,"w":220,"h":46,"t":"Draw_Board()","sub":"顏色用 log2 當索引",
  "tgt":(TW,"static void Draw_Tile(uint8_t row, uint8_t col)"),
  "cap":"字型大小也跟著位數選，1024 才不會撐破格子。"},
 {"id":"ck","x":200,"y":532,"w":220,"h":54,"cls":"dec","t":"Check_Win / Check_GameOver","sub":"贏了？還是沒路了？",
  "tgt":(TW,"static uint8_t Check_GameOver(void)"),
  "cap":"「沒路」= 沒有空格<b>而且</b>沒有相鄰同值。"},
],"edges":[
 {"a":"tp","b":"sd"},{"a":"sd","b":"ml"},
 {"a":"ml","b":"s1"},{"a":"s1","b":"s2","side":"r"},{"a":"s2","b":"s3","side":"r"},
 {"a":"s2","b":"mv"},
 {"a":"mv","b":"ar","lab":"是"},{"a":"ar","b":"dr"},{"a":"dr","b":"ck"},
 {"a":"mv","b":"tp","side":"back","lab":"沒動：什麼都不做"},
]},
"html":"""
<h3>三段式：壓縮、合併、再壓縮</h3>
<p>{{J:App/game_2048.c::static uint8_t Move_Left(void)::Move_Left()}} 對每一列做三件事。順序有嚴格的理由，下面的示範可以單步跑一遍。</p>
<p><b>① 壓縮。</b>把非零的磁磚依序搬到最左邊。這一步只搬不合。理由是合併只檢查「相鄰」兩格，而 <code>[2, 0, 2, 0]</code> 中間夾著空格，不先壓縮就看不出可以合。</p>
<p><b>② 合併。</b>從左往右掃，相鄰相等就把左邊加倍、右邊清零、加分。迴圈上限是 <code>c &lt; BOARD_N - 1</code>，而且<b>不回頭</b>。這讓 <code>[4,4,4,4]</code> 變成 <code>[8,0,8,0]</code> 而不是 <code>[16,0,0,0]</code>——一顆磁磚在同一次移動裡只能合一次，這是 2048 的規則。</p>
<p><b>③ 再壓縮。</b>② 在中間留下了 0，要再擠一次。<code>[2,2,4,0]</code> 合併後是 <code>[4,0,4,0]</code>，第三步才變成 <code>[4,4,0,0]</code>。</p>

<h3>moved 這個回傳值撐住了整個遊戲規則</h3>
<p>三段做完之後，函式比對前後盤面，回傳「有沒有真的變化」。這個值決定後面所有事：<b>只有 moved 是 1 才生成新磁磚</b>。</p>
<p>少了它，玩家往一個已經靠底的方向亂滑也會被塞磁磚，盤面很快填滿，遊戲壞掉。一個 <code>uint8_t</code> 回傳值就是規則本身。</p>

<h3>為什麼四個方向各寫一份</h3>
<p>「聰明」的寫法是只寫 <code>Move_Left()</code>，其他三個方向先把陣列轉置或翻轉、呼叫它、再轉回來。程式碼少四分之三。</p>
<p>這個專案沒有這樣做，四個方向各一份完整實作。理由是可讀性和可除錯性：</p>
<ul>
<li>每一份都只有一層索引，沒有座標轉換。<code>Move_Up()</code> 就是外圈跑 <code>c</code>、內圈跑 <code>r</code>，一眼看得出在做什麼。</li>
<li>轉置版本一旦有 bug，你要同時懷疑「演算法錯了」和「座標轉錯了」。在沒有除錯器、只能靠 LCD 印字的環境裡，這個差別很大。</li>
<li>多出來的程式碼不到 100 行，而這台機器的瓶頸是 32 KB 的<b>整體</b>，不是這一個函式。</li>
</ul>
<p>值得注意的是 <code>Move_Right()</code> 和 <code>Move_Down()</code> 的迴圈用了 <code>for (c = BOARD_N; c-- &gt; 0; )</code> 這種倒數寫法，因為索引是無號數，寫 <code>c &gt;= 0</code> 會永遠成立。</p>

<h3>顏色用 log2 當索引</h3>
<p>磁磚的值是 2 的次方，所以 {{J:App/game_2048.c::static uint8_t Log2Idx(uint16_t val)::Log2Idx()}} 用一個 while 迴圈數出它是 2 的幾次方，直接當顏色表的索引：</p>
<pre class="note" style="font-family:var(--sm);font-size:12.5px;white-space:pre">while (v &gt; 1) { v &gt;&gt;= 1; idx++; }      /* 2→1, 4→2, 2048→11 */</pre>
<p>13 個顏色 26 位元組，換到的是「顏色可以任意挑」和「每次只要一次陣列索引」。沒有用公式算色相，因為 2048 的配色本來就是設計出來的，不是算出來的。</p>
<p>{{J:App/game_2048.c::static void Draw_Tile(uint8_t row, uint8_t col)::Draw_Tile()}} 還做了一件小事：依位數選字型。四位數用 Font16（4 × 11 = 44，剛好塞滿 44 像素的格子），五位數就換 Font12。原本的課程版本全部用 Font20，於是 <code>1024</code> 是 4 × 14 = 56 像素，撐破格子。</p>

<h3>滑動：主軸決定方向</h3>
<p>{{J:App/game_2048.c::static SwipeDir swipe_dir(int16_t dx, int16_t dy)::swipe_dir()}} 比較 dx 和 dy 的絕對值，大的那個決定方向。低於 <code>SWIPE_MIN</code>（28 像素）就回傳「沒有滑動」，避免點一下被誤判。</p>
<p>「放開」也需要去彈跳，理由和按鍵一樣：面板讀數偶爾會有一兩次假的「沒人摸」。{{J:App/game_2048.c::static SwipeDir poll_touch(uint8_t *tapped)::poll_touch()}} 要求連續三次取樣都沒摸到才承認放手，在 10 毫秒的迴圈上是 30 毫秒。少了這個，一次滑動會被切成兩次，方向還可能相反。</p>
""",
"demos":["move2048"],
"ex":[
 {"id":"c9a","type":"fill",
  "q":"補完 <code>Move_Left()</code> 合併階段的條件：<code>tmp[]</code> 是壓縮後的一列，要判斷第 c 格能不能和右邊合併。",
  "ph":"if ( ... ) { tmp[c] <<= 1; ... }",
  "need":[[r"/tmp\[c\]/","要先確認 tmp[c] 不是 0"],
          [r"/tmp\[c\s*\+\s*1\]/","要和 tmp[c+1] 比較"],
          [r"/==/","相等判斷用 =="],
          [r"/&&/","兩個條件用 && 串起來"]],
  "ans":"if (tmp[c] &amp;&amp; tmp[c] == tmp[c + 1]) { tmp[c] &lt;&lt;= 1; g_score += tmp[c]; tmp[c + 1] = 0; }",
  "fb":"<code>tmp[c]</code> 必須先判非零，否則兩個空格 <code>0 == 0</code> 也會「合併」成 0 並加 0 分。這是這個演算法最容易漏的一個條件。"},
 {"id":"c9b","type":"mc",
  "q":"為什麼合併之後還要再壓縮一次？",
  "opts":["為了重新計算分數","合併把右邊那格清成 0，中間會出現空格",
          "為了偵測遊戲結束","為了讓動畫比較順"],
  "ans":1,
  "fb":"<code>[2,2,4,0]</code> 壓縮後是 <code>[2,2,4,0]</code>，合併後是 <code>[4,0,4,0]</code> —— 中間破了一個洞。第三步把它擠成 <code>[4,4,0,0]</code>。上面的示範可以單步看這三個階段。"},
 {"id":"c9c","type":"mc",
  "q":"<code>Move_Left()</code> 回傳的 <code>moved</code> 如果永遠回傳 1，遊戲會怎麼壞？",
  "opts":["分數會算錯","玩家往已經靠底的方向亂滑也會生成新磁磚，盤面很快被填滿",
          "顏色會錯","觸控會失效"],
  "ans":1,
  "fb":"生成新磁磚的前提是「這次移動真的改變了盤面」。少了這個判斷，無效的滑動也會被獎勵一顆新磁磚，遊戲的節奏整個壞掉。一個回傳值撐住了規則。"},
]},

# ===================================================================== 10
{
"n":10, "title":"Gate Keeper",
"problem":"四條通道、四顆按鍵、一支搖桿要同時顧。這一章講那組看起來很怪的通道座標表為什麼要這樣寫、冷卻時間怎麼把遊戲從比手速變成比時機，以及為什麼所有實體都用固定陣列而不是動態配置。",
"files":[GT],
"focus":(GT,"static const Band LB[LN_N] = {"),
"flowName":"一幀的 Gate Keeper",
"flowHint":"注意 render() 裡「先擦後畫」的順序。",
"flow":{"w":640,"h":640,"nodes":[
 {"id":"fi","x":195,"y":10,"w":240,"h":50,"cls":"dec","t":"四顆按鍵 → fire(lane)","sub":"冷卻中就直接 return",
  "tgt":(GT,"static void fire(uint8_t lane)"),
  "cap":"一行判斷決定了這個遊戲考時機而不是手速。"},
 {"id":"wv","x":195,"y":82,"w":240,"h":50,"t":"波次控制","sub":"清空了才進下一波",
  "tgt":(GT,"static uint32_t spawn_gap(uint8_t w)"),
  "cap":"波次越高，數量越多、間隔越短、速度越快。"},
 {"id":"se","x":195,"y":154,"w":240,"h":50,"t":"spawn_enemy()","sub":"隨機通道，檢查入口沒塞住",
  "tgt":(GT,"static void spawn_enemy(void)"),
  "cap":"同一條通道入口附近已經有敵人就放棄這次生成。"},
 {"id":"mc","x":30,"y":232,"w":200,"h":50,"cls":"io","t":"move_collector()","sub":"搖桿類比值 → 位置",
  "tgt":(GT,"static void move_collector(void)"),
  "cap":"用 Input_JoyAxis() 除以 110，最高約 4.6 像素/幀。"},
 {"id":"sw","x":400,"y":232,"w":210,"h":50,"cls":"dec","t":"step_world()","sub":"脈衝外推 · 敵人內推 · 判定",
  "tgt":(GT,"static void step_world(void)"),
  "cap":"一個函式做完所有世界更新，完全不碰畫面。"},
 {"id":"ki","x":400,"y":306,"w":210,"h":46,"t":"距離差 ≤ 9 就擊殺","sub":"同一條通道才比",
  "cap":"脈衝和敵人都用「離核心多遠」表示，判定變成一維。"},
 {"id":"wi","x":30,"y":306,"w":200,"h":46,"t":"撿到掉落物","sub":"分數 / 補血 / 清場",
  "cap":"兩軸距離都小於半徑加 6 就算撿到。"},
 {"id":"rd","x":195,"y":382,"w":240,"h":46,"cls":"dec","t":"render()","sub":"順序 = 圖層",
  "tgt":(GT,"static void render(void)"),
  "cap":"先擦收集器舊位置，再畫車道、核心、掉落物，最後畫收集器。"},
 {"id":"e1","x":25,"y":454,"w":180,"h":44,"t":"① 擦舊收集器","sub":"下面的重畫會補回來",
  "cap":"先擦的理由：擦出來的洞會被後面的車道重繪填掉。"},
 {"id":"e2","x":222,"y":454,"w":180,"h":44,"t":"② 四條車道","sub":"各自一個 band",
  "tgt":(GT,"static void draw_lane(uint8_t l)"),
  "cap":"每條車道的 band 都停在核心外面，所以不會互相覆蓋。"},
 {"id":"e3","x":419,"y":454,"w":190,"h":44,"t":"③ 核心 ④ 掉落物 ⑤ 收集器","sub":"最後畫的在最上層",
  "cap":"收集器是開口的方框，遮住的畫面最少。"},
 {"id":"hu","x":195,"y":528,"w":240,"h":50,"cls":"io","t":"hud_update()","sub":"只有值變了才重畫",
  "tgt":(GT,"static void hud_update(void)"),
  "cap":"記住目前顯示的數字，一樣就跳過。"},
 {"id":"hp","x":195,"y":598,"w":240,"h":38,"cls":"dec","t":"HP 歸零 → 結束",
  "cap":"被突破一次扣一血，連段歸零。"},
],"edges":[
 {"a":"fi","b":"wv"},{"a":"wv","b":"se"},
 {"a":"se","b":"mc"},{"a":"se","b":"sw"},
 {"a":"sw","b":"ki"},{"a":"mc","b":"wi"},
 {"a":"ki","b":"rd"},{"a":"wi","b":"rd"},
 {"a":"rd","b":"e1"},{"a":"e1","b":"e2","side":"r"},{"a":"e2","b":"e3","side":"r"},
 {"a":"e2","b":"hu"},{"a":"hu","b":"hp"},
]},
"html":"""
<h3>那組看起來很怪的通道座標</h3>
<p>README 特別註明這一段會讓人困惑，值得拆開。問題出在螢幕是 320 × 240 而不是正方形：<b>左右通道 148 像素，上下只有 92</b>。</p>
<p>如果所有敵人都用同樣的像素速度，上下通道的敵人只給你 62% 的反應時間。玩家不會知道為什麼，只會覺得這遊戲怪。</p>
<p>解法是 {{J:App/game_gate.c::static int16_t lane_scale(uint8_t l, int16_t v)::lane_scale()}}：</p>
<pre class="note" style="font-family:var(--sm);font-size:12.5px;white-space:pre">static int16_t lane_scale(uint8_t l, int16_t v)
{
    return (l &lt; LN_LEFT) ? (int16_t)((int32_t)v * LANE_V / LANE_H) : v;
}</pre>
<p>所有速度——敵人、脈衝——在生成時都先過這個函式。於是每個敵人走進來的<b>時間</b>相同，而像素速度不同。玩家完全察覺不到，但少了它遊戲就不公平。</p>
<p>另一半是 {{J:App/game_gate.c::static const Band LB[LN_N] = {::LB[] 這張表}}。四條通道各有一個矩形，用同一組參數算出來：</p>
<pre class="note" style="font-family:var(--sm);font-size:12.5px;white-space:pre">{ CX - LANE_W/2,        CY - LANE_V - LANE_PAD, LANE_W, LANE_V - CORE_R + LANE_PAD },
{ CX - LANE_W/2,        CY + CORE_R + 1,        LANE_W, LANE_V - CORE_R + LANE_PAD },
{ CX - LANE_H - LANE_PAD, CY - LANE_W/2,       LANE_H - CORE_R + LANE_PAD, LANE_W },
{ CX + CORE_R + 1,        CY - LANE_W/2,       LANE_H - CORE_R + LANE_PAD, LANE_W },</pre>
<p>看懂的關鍵是三件事：每個矩形都<b>停在核心外面</b>（所以重畫車道永遠不會擦到核心，兩者不會搶同一個像素）；<code>LANE_PAD</code> 是往外多留的餘裕，讓剛生成的敵人整個菱形都在矩形內，不會被切一半；而實體位置一律用「離核心多遠」這個一維距離表示，畫的時候才換成螢幕座標。</p>
<p>一維距離帶來一個很大的好處：擊殺判定變成 <code>|敵人距離 − 脈衝距離| ≤ 9</code>，一條通道一個減法。不需要二維碰撞。</p>

<h3>260 毫秒的冷卻</h3>
<p>{{J:App/game_gate.c::static void fire(uint8_t lane)::fire()}} 的第一行就是全部：</p>
<pre class="note" style="font-family:var(--sm);font-size:12.5px;white-space:pre">if (s_pulse[lane].readyAt &gt; msTick) return;      /* 還在充能 */</pre>
<p>沒有冷卻的話，玩家只要四顆鍵一起亂打就能通關，反應時機完全不重要，遊戲就沒有內容了。四條通道<b>各有自己的冷卻</b>，所以同時照顧四邊是可行的——難的是分配注意力，而那正是這個遊戲想考的東西。下面可以試著連打。</p>

<h3>固定陣列加 active 旗標</h3>
<p>敵人最多 14 個、掉落物最多 2 個，都是固定大小的陣列加一個 <code>active</code> 旗標。沒有 <code>malloc</code>。</p>
<p>理由有三層。最實際的是<b>沒有堆積可以碎裂</b>：這支韌體要連續跑幾小時，動態配置在沒有作業系統的環境裡遲早會碎成一堆無法使用的空隙，而那個故障會在展示的第二十分鐘才出現。其次是<b>記憶體用量在編譯時就確定</b>，不會有「玩到某個波次突然配置失敗」這種事。第三是<b>生成變成一次線性掃描</b>：</p>
<pre class="note" style="font-family:var(--sm);font-size:12.5px;white-space:pre">for (i = 0; i &lt; MAX_EN; i++) {
    if (s_en[i].active) continue;
    ... 填好這一格 ...
    return;
}
/* 掃完沒有空位就放棄這次生成 */</pre>
<p>「陣列滿了」變成「這一幀不生成」，一個很自然的降級，不需要錯誤處理。<code>MAX_EN</code> 設 14 是因為四條通道各三四個就已經很難了，滿了本身就代表玩家快死。</p>

<h3>世界更新一步做了哪些事</h3>
<p>{{J:App/game_gate.c::static void step_world(void)::step_world()}} 依序做三件事：脈衝往外推、敵人往內推並檢查是否碰到核心、以及檢查脈衝和敵人的距離差。掉落物的逾時也在這裡。</p>
<p>整個函式<b>完全不碰畫面</b>。這讓「畫面是世界狀態的函數」成立，也讓 <code>render()</code> 可以自由決定重畫哪些區塊而不影響邏輯。</p>

<h3>難度曲線只有兩個函式</h3>
<p>波次大小是 <code>6 + 2 × 波次</code>，生成間隔是 {{J:App/game_gate.c::static uint32_t spawn_gap(uint8_t w)::spawn_gap()}} 的 <code>1150 − 60 × 波次</code>（下限 340 毫秒），敵人速度是 <code>17 + 2 × (波次−1)</code>（上限 45）。三條直線，各有一個下限或上限。</p>
<p>沒有難度表、沒有曲線擬合。要調整手感就改這幾個係數，而且每個係數的意義都很明確——這比一張二十筆的表好調得多。</p>

<h3>HUD 只擦變動的那一小塊</h3>
<p>{{J:App/game_gate.c::static void hud_update(void)::hud_update()}} 為每個欄位記住「目前畫上去的值」，一樣就跳過：</p>
<pre class="note" style="font-family:var(--sm);font-size:12.5px;white-space:pre">if (s_hudScore != s_score) {
    s_hudScore = s_score;
    ... 只畫分數那一小塊 ...
}</pre>
<p>分數每幀都重畫會閃（文字繪製是先填背景再畫字），而且浪費時間。血條同理：只有扣血那一幀才重畫五個方塊。這是第 5 章那個原則在文字上的版本。</p>
""",
"demos":["lanes"],
"ex":[
 {"id":"c10a","type":"fill",
  "q":"補完「這條通道的冷卻時間到了沒有」的判斷（<code>s_pulse[lane].readyAt</code> 是可以再開火的時刻）。",
  "ph":"if ( ... ) return;   /* 還在冷卻 */",
  "need":[[r"/readyAt/","要看 readyAt"],
          [r"/msTick/","要和 msTick 比較"],
          [r"/>|</","需要一個大小比較"]],
  "ans":"if (s_pulse[lane].readyAt &gt; msTick) return;",
  "fb":"開火成功時設 <code>readyAt = msTick + COOLDOWN_MS</code>，所以「還沒到」就是 <code>readyAt &gt; msTick</code>。更耐溢位的寫法是 <code>(int32_t)(msTick - readyAt) &lt; 0</code>，兩者在 49.7 天內等價。"},
 {"id":"c10b","type":"mc",
  "q":"<code>lane_scale()</code> 把上下通道的速度乘上 92/148。為什麼不乾脆把四條通道畫成一樣長？",
  "opts":["因為那樣程式比較難寫",
          "螢幕是 320×240，上下方向的空間本來就比左右少，畫成一樣長會浪費兩側的畫面",
          "因為 ADC 的取樣時間不同",
          "因為 band 緩衝區不夠大"],
  "ans":1,
  "fb":"要讓四條一樣長，就得把長度都縮到 92，左右兩側各空出 56 像素。與其浪費畫面，不如讓長度不同而把<b>時間</b>對齊——玩家感受到的是時間，不是像素。"},
 {"id":"c10c","type":"mc",
  "q":"敵人用固定 14 格的陣列加 active 旗標，而不是動態配置。在這個環境裡最實際的理由是？",
  "opts":["固定陣列比較快",
          "沒有作業系統的長時間執行下，堆積碎裂會在展示中途才發作，而且很難重現",
          "malloc 在 Keil 裡不能用",
          "這樣才能用 for 迴圈"],
  "ans":1,
  "fb":"速度差異其實不大。真正的理由是可預測性：記憶體用量在編譯時就確定，不會有「玩到第二十分鐘突然配置失敗」這種只在展示時出現的故障。附帶好處是「陣列滿了」變成「這一幀不生成」，一個不需要錯誤處理的自然降級。"},
]},

# ===================================================================== 11
{
"n":11, "title":"I/O TEST 與硬體補課",
"problem":"前面十章都把硬體擋在下面。這一章是唯一要打開那層蓋子的地方，而且只講讀懂這支程式所需要的最小集合。I/O TEST 剛好是最好的教材：它存在的目的就是把硬體的狀態攤出來給人看。",
"files":[IO,IN,SC,UD],
"focus":(IO,"void IoTest_Run(void)"),
"flowName":"I/O TEST 一輪顯示",
"flowHint":"紅色邊框的節點是直接碰硬體的地方。",
"flow":{"w":640,"h":600,"nodes":[
 {"id":"lp","x":215,"y":10,"w":210,"h":40,"cls":"ev","t":"50 ms 一輪","sub":"不需要遊戲的幀率",
  "tgt":(IO,"void IoTest_Run(void)"),
  "cap":"診斷畫面只要人眼跟得上。"},
 {"id":"ip","x":215,"y":74,"w":210,"h":44,"cls":"io","t":"Input_Poll()","sub":"和遊戲用同一套",
  "tgt":(IN,"void Input_Poll(void)"),
  "cap":"刻意共用，這樣測到的就是遊戲會看到的。"},
 {"id":"ad","x":20,"y":142,"w":200,"h":54,"cls":"hw","t":"四軸長條圖","sub":"12 位元 → 0..4095",
  "tgt":(IO,"static void draw_axis(uint8_t i, uint16_t raw, uint16_t centre)"),
  "cap":"橘色刻度是開機量到的中心。長條停在刻度上才正常。"},
 {"id":"bt","x":420,"y":142,"w":200,"h":54,"cls":"hw","t":"按鍵方格","sub":"按著就變綠",
  "tgt":(IO,"static void draw_buttons(uint16_t held, uint16_t shown)"),
  "cap":"用 Held 而不是 Pressed，因為要看的是持續狀態。"},
 {"id":"sn","x":215,"y":218,"w":210,"h":54,"cls":"hw","t":"LOW 掃腳位","sub":"哪一支腳正被拉低",
  "tgt":(IN,"void Input_ScanLow(uint16_t *portE, uint16_t *portC)"),
  "cap":"回答「我的按鍵到底接在哪支腳」。只動上拉設定，不動方向。"},
 {"id":"df","x":215,"y":296,"w":210,"h":50,"cls":"hw","t":"ADC OK / FAIL","sub":"轉換有沒有完成過",
  "tgt":(IN,"uint32_t Input_AdcFaults(void)"),
  "cap":"把「韌體壞了」和「線沒接」分開的關鍵。"},
 {"id":"sp","x":215,"y":372,"w":210,"h":54,"cls":"hw","t":"SPK 那一行","sub":"時脈 · ARR · 頻率",
  "tgt":(IO,"static void draw_sound(void)"),
  "cap":"84000k 代表時脈算對了。這一行救過一次真實的故障。"},
 {"id":"dc","x":20,"y":452,"w":200,"h":54,"t":"DC 階段","sub":"純拉高，有源蜂鳴器會叫",
  "tgt":(SC,"void Sound_DC(uint8_t on)"),
  "cap":"完全不經過計時器，是「腳位能不能驅動東西」最簡單的證明。"},
 {"id":"sw","x":420,"y":452,"w":200,"h":54,"t":"SWEEP 階段","sub":"400 Hz → 4 kHz",
  "tgt":(IO,"#define SWEEP_MS"),
  "cap":"掃頻一定會經過蜂鳴器的共振點，比單一頻率好聽得出來。"},
 {"id":"fp","x":215,"y":532,"w":210,"h":54,"t":"FIND：逐腳測試","sub":"自己翻腳位，不用 TIM3",
  "tgt":(IO,"static void pin_warble(uint8_t pin, uint32_t ms)"),
  "cap":"PA1 到 PA8 各叫 600 毫秒，聽到哪一支就是接在哪一支。"},
],"edges":[
 {"a":"lp","b":"ip"},{"a":"ip","b":"ad"},{"a":"ip","b":"bt"},
 {"a":"ad","b":"sn"},{"a":"bt","b":"sn"},
 {"a":"sn","b":"df"},{"a":"df","b":"sp"},
 {"a":"sp","b":"dc"},{"a":"sp","b":"sw"},
 {"a":"dc","b":"fp"},{"a":"sw","b":"fp"},
]},
"html":"""
<div class="note">這一章是全站唯一講硬體的地方，而且只講「不懂就看不懂這支程式」的部分。每個概念旁邊的連結會帶到另一個專門教 STM32 的網站。</div>

<h3>按鍵：上拉與 active low</h3>
<p>一支設成輸入的腳如果什麼都沒接，電位由周圍雜訊決定，讀回來是隨機的。解法是打開晶片內部的上拉電阻把它輕輕拉到 3.3 V，按鈕的另一端接地——按下就把它硬拉到 0 V。</p>
<p>所以<b>按下讀 0、放開讀 1</b>，判斷式要取反。{{J:App/input.c::raw |= BTN_UP;::input.c 讀按鍵那幾行}}都是 <code>if (!(e &amp; bit))</code> 的形狀，那個驚嘆號就是 active low。</p>
<p>設定一支這樣的腳只要兩件事：方向設成輸入、上拉打開。{{J:App/input.c::static void gpio_input_pullup(GPIO_TypeDef *port, uint8_t pin)::gpio_input_pullup()}} 就是這兩行。附帶好處是按鍵<b>不需要外接電阻</b>。
<span class="deepslot" data-k="gpio" data-l="GPIO 模式與上拉的完整說明"></span></p>

<h3>ADC：12 位元的數字是什麼意思</h3>
<p>搖桿是兩個電位器分壓。ADC 把那個電壓換成整數，本專案用 12 位元，所以<b>0 V 對到 0、3.3 V 對到 4095</b>，一個計數約 0.806 mV。</p>
<p>{{J:App/input.c::uint16_t Input_AdcRaw(uint8_t ch)::Input_AdcRaw()}} 的流程是：指定要量哪個通道、清掉上次的完成旗標、按下開始、等完成旗標、讀結果。四步，六微秒。</p>
<p>值得看的是那個 <code>guard</code> 計數器。一次轉換只要幾百個 CPU 週期，正常永遠用不到它。它的作用是：<b>如果 ADC 設定錯了導致完成旗標永遠不來，整台機器不要當掉</b>，而是讓搖桿讀 0 並把錯誤記下來。</p>
<p>{{J:App/input.c::uint32_t Input_AdcFaults(void)::那個計數器}}被顯示在 I/O TEST 的 <code>ADC OK</code> / <code>ADC FAIL</code>。這一個設計把「韌體錯了」和「線沒接」切開：<code>ADC OK</code> 但長條停在 0，就一定是硬體那邊的事。<b>讓故障變成可見的資訊，而不是一片死寂。</b>
<span class="deepslot" data-k="adc" data-l="ADC 取樣與設定的完整說明"></span></p>

<h3>TIM3 的 PWM 怎麼變成聲音</h3>
<p>計時器有一個計數器，每個時脈加一，數到 <code>ARR</code> 就歸零重來。<code>PSC</code> 先把時脈除慢。所以一個週期是：</p>
<pre class="note" style="font-family:var(--sm);font-size:12.5px;white-space:pre">f = 計時器時脈 / ((PSC + 1) × (ARR + 1))</pre>
<p>把這個週期性的輸出接到腳位，就是方波；方波送進蜂鳴器就是聲音。工作週期固定在一半，因為<b>音高只由週期決定</b>——PWM 在這裡不是用來調音量。</p>
<p>{{J:App/sound.c::static void tone_on(uint16_t freq)::tone_on()}} 從頻率反算兩個數字。<code>ARR</code> 只有 16 位元，所以要先用 <code>PSC</code> 把總計數壓進 65536 以內：</p>
<pre class="note" style="font-family:var(--sm);font-size:12.5px;white-space:pre">total = s_timClk / freq;
psc   = (total + 65535UL) / 65536UL;   /* 最小的 PSC+1 使 ARR 放得下 */
arr   = total / psc;</pre>
<p>計時器時脈要從 {{J:App/sound.c::static uint32_t timer_clock(void)::timer_clock()}} 問，因為 APB1 上的計時器有一個容易踩的規則：<b>除頻器不是 1 的時候，計時器拿到的是匯流排時脈的兩倍</b>。課程板是 84 MHz，也就是 I/O TEST 上顯示的 <code>84000k</code>。</p>
<p>這一行顯示救過一次真實故障：蜂鳴器完全沒聲音，但 <code>84000k</code> 正確、<code>ARR</code> 正確、頻率也在跳——於是可以確定韌體沒問題，最後查出是杜邦線整排錯一格。<b>把計算結果顯示出來，就把問題切成兩半。</b>
<span class="deepslot" data-k="tim" data-l="計時器與 PWM 的完整說明"></span></p>

<h3>兩個測試階段對應兩種蜂鳴器</h3>
<p>按一次 BEEP 會跑兩段：先純 DC 拉高 1.5 秒，再掃頻 2.5 秒。這不是多餘——它同時測兩種零件。</p>
<p><b>有源蜂鳴器</b>內建振盪器，給它電壓就叫，波形無所謂。所以它在 DC 階段會響、掃頻階段安靜。{{J:App/sound.c::void Sound_DC(uint8_t on)::Sound_DC()}} 把腳位從計時器收回來當普通輸出，直接拉高——<b>完全不經過計時器</b>，是「這支腳能不能驅動任何東西」最簡單的證明。</p>
<p><b>無源蜂鳴器</b>沒有振盪器，要餵方波才會叫，所以它剛好相反。掃頻而不是單一頻率的理由是：蜂鳴器在共振頻率附近大聲得多，掃過去一定會經過。</p>
<p>哪一段有聲音，就知道插的是哪一種零件；兩段都沒聲音，問題在接線。</p>

<h3>FIND：讓板子自己說出腳位</h3>
<p>{{J:App/iotest.c::static void pin_warble(uint8_t pin, uint32_t ms)::pin_warble()}} 自己用迴圈翻腳位，<b>不用 TIM3</b>。這樣它可以測任何一支腳，不管那支腳屬於哪個計時器。</p>
<p>{{J:App/iotest.c::static void find_pin(void)::find_pin()}} 依序把 PA1 到 PA8 各叫 600 毫秒並在畫面上印出腳位名稱。蜂鳴器夾在你焊的那條線上，聽到哪一支就知道實際接在哪一支。它會存下每支腳原本的設定再還原，並刻意跳過 PA13／PA14——那是除錯器的線，動了會讓 ST-Link 斷線。</p>
<p>同樣的想法也用在按鍵上：{{J:App/input.c::void Input_DiagInit(void)::Input_DiagInit()}} 把所有候選腳的上拉打開（只動上拉，不動方向，而且跳過已經被別人設成輸出的腳），然後 <code>Input_ScanLow()</code> 回報哪幾支正被拉低。按一顆鍵，畫面就告訴你它在哪支腳。這兩個功能各抓到一次真實的接線錯誤。</p>

<h3>msTick 從哪裡來</h3>
<p>{{J:User_common/user_defined.c::void SysTick_Handler(void)::SysTick_Handler()}} 是一個每毫秒被硬體呼叫一次的函式，它只做一件事：把 <code>msTick</code> 加一。全機所有計時都建立在這個變數上。</p>
<p>它宣告成 <code>volatile</code>，這一點不能省：告訴編譯器「這個變數會被你看不到的程式改動」，否則 <code>while (msTick - t0 &lt; 20)</code> 裡的讀取會被最佳化成只讀一次，迴圈變成永遠成立。
<span class="deepslot" data-k="systick" data-l="SysTick 的設定與中斷"></span></p>

<h3>FSMC：為什麼 LCD 看起來像一塊記憶體</h3>
<p>這塊板子的螢幕不走 SPI，而是接在 FSMC 上——一個本來用來外接 SRAM 的並列匯流排。ILI9341 的並列介面長得很像 SRAM：一組資料線、一支寫入腳、一支「這次寫的是指令還是資料」的腳。</p>
<p>訣竅是把那支腳<b>接到一條位址線</b>（本專案用 A6）。於是驅動可以宣告一個兩欄的結構，兩欄的位址剛好差那一條線：寫第一欄就是送指令，寫第二欄就是送資料。「傳一個像素給螢幕」在 C 裡變成一次結構欄位賦值。</p>
<p>這就是 <code>GLCD/stm324xg_lcd_ILI9341_Pro.c</code> 第 5 到 7 行那個 <code>LCD_BASE</code> 被算成 <code>0x6C00007E</code> 的由來，也是第 5 章「一次寫入一整塊」之所以便宜的原因——連續寫入同一個位址，硬體自己把像素排進面板。
<span class="deepslot" data-k="fsmc" data-l="FSMC 與 ILI9341 的完整說明"></span></p>
""",
"demos":[],
"ex":[
 {"id":"c11a","type":"mc",
  "q":"I/O TEST 顯示 <code>ADC OK</code>，但四條長條全部停在 0。最可能的原因是？",
  "opts":["ADC 設定錯了","搖桿模組的 3.3 V 沒接上，或 VRx/VRy 沒接到對應的腳",
          "取樣時間太短","msTick 沒有在跑"],
  "ans":1,
  "fb":"<code>ADC OK</code> 表示轉換確實完成了，讀到 0 就是輸入電壓真的是 0。搖桿的兩軸是分壓電路，沒有電源就沒有輸出——而搖桿上的按鍵只是對地開關，沒電源也照樣能按。「按得到但推不動」幾乎就是電源線沒接。"},
 {"id":"c11b","type":"mc",
  "q":"按 BEEP 之後，DC 階段有聲音、掃頻階段安靜。插的是哪一種蜂鳴器？",
  "opts":["無源蜂鳴器","有源蜂鳴器","兩種都可能","蜂鳴器壞了"],
  "ans":1,
  "fb":"有源蜂鳴器內建振盪器，有電壓就叫，方波對它沒有額外意義（掃頻時腳位有一半時間是低的，反而變小聲或不叫）。無源的剛好相反：DC 只會「咔」一聲然後安靜，要方波才會發聲。所以哪一段有聲音就知道插的是哪一種。"},
 {"id":"c11c","type":"mc",
  "q":"<code>Input_AdcRaw()</code> 裡的 <code>guard</code> 計數器，真正的用途是什麼？",
  "opts":["控制取樣時間","限制 ADC 的轉換速度",
          "讓設定錯誤的 ADC 造成「搖桿讀 0」而不是整台機器卡死",
          "校正中心值"],
  "ans":2,
  "fb":"正常情況下永遠用不到它。它是一個失效模式的設計：把「無限迴圈等一個不會來的旗標」換成「回傳 0 並把錯誤記下來」。那個計數器再被顯示成 <code>ADC OK</code> / <code>ADC FAIL</code>，於是故障變成可以讀的資訊。"},
]},

# ===================================================================== 12
{
"n":12, "title":"收尾",
"problem":"32 KB 的連結上限逼出了這支韌體很多寫法。這一章把那些取捨整理出來，並列出「如果要加第四款遊戲」要動哪幾個檔案。",
"files":[GH,DN,AM,CH],
"focus":(GH,"#define GFX_BUF_PIX"),
"flowName":"加一款新遊戲要改哪裡",
"flowHint":"五個落點。雙擊節點跳到要改的位置。",
"flow":{"w":600,"h":520,"nodes":[
 {"id":"n","x":190,"y":10,"w":220,"h":44,"cls":"ev","t":"新檔案 game_xxx.c","sub":"提供 void Xxx_Run(void)",
  "cap":"照第 7 章的骨架寫主迴圈就好。遊戲之間不共用狀態。"},
 {"id":"e","x":190,"y":80,"w":220,"h":48,"t":"① console.h 的 AppScreen","sub":"加一個列舉值",
  "tgt":(CH,"} AppScreen;"),
  "cap":"順序要和選單項目表一致，因為選單用 APP_DINO + i 回傳。"},
 {"id":"d","x":190,"y":154,"w":220,"h":48,"t":"② console.h 加宣告","sub":"void Xxx_Run(void);",
  "tgt":(CH,"void      Gate_Run(void);"),
  "cap":"讓 app_main.c 看得到。"},
 {"id":"s","x":190,"y":228,"w":220,"h":48,"t":"③ app_main.c 的 switch","sub":"加一個 case",
  "tgt":(AM,"case APP_GATE: Gate_Run();     break;"),
  "cap":"狀態機本體。"},
 {"id":"m","x":190,"y":302,"w":220,"h":54,"t":"④ menu.c 的項目表","sub":"名稱 · 副標 · ITEM_N",
  "tgt":(MN,"static const char *ITEM_NAME[ITEM_N] = { \"DINO RUN\""),
  "cap":"三個陣列都要加，<code>ITEM_N</code> 也要加一。順序必須和列舉一致。"},
 {"id":"p","x":190,"y":382,"w":220,"h":48,"cls":"dec","t":"⑤ 專案檔加入編譯","sub":"Keil 的 App 群組",
  "cap":"用 Keil 介面加，或直接改 .uvprojx 的 App 群組。"},
 {"id":"b","x":190,"y":456,"w":220,"h":48,"cls":"hw","t":"檢查還剩多少空間","sub":"目前剩 3020 bytes",
  "cap":"Code + RO-data + RW-data 要小於 32768。超過連結器會報 L6047U。"},
],"edges":[
 {"a":"n","b":"e"},{"a":"e","b":"d"},{"a":"d","b":"s"},{"a":"s","b":"m"},
 {"a":"m","b":"p"},{"a":"p","b":"b"},
]},
"html":"""
<h3>目前的預算</h3>
<pre class="note" style="font-family:var(--sm);font-size:12.5px;white-space:pre">Code      21428 bytes   程式碼
RO-data    8280 bytes   常數：字型表、sprite、文字
RW-data      40 bytes
                        ----------
影像合計   29748 bytes   上限 32768，剩 3020

ZI-data   11752 bytes   RAM，其中 9600 是 gfx 的組圖緩衝區</pre>

<h3>32 KB 逼出來的五個決定</h3>
<p><b>一、不用浮點數。</b>F407 其實有硬體浮點單元，但一旦用了 <code>float</code>，連結器會拉進浮點的格式化與函式庫程式碼。改用 {{J:App/game_dino.c::#define GRAVITY16::1/16 像素的定點數}}，運算全是整數，而除以 16 只是一個位移指令。</p>
<p><b>二、sprite 用 1 位元一像素。</b>16 張圖共 2050 位元組。存成 RGB565 的話，光恐龍六個姿勢就超過 12 KB。代價是一張圖只能一個顏色，而 Chrome Dino 本來就是單色的。</p>
<p><b>三、砍掉兩個字型。</b>這是最大的一刀。點陣字型是純資料，一個字元佔「寬度位元組 × 高度」：Font24 是 6840 位元組、Font20 是 3800。第一次加完 Gate Keeper 時影像是 32488，只剩 280 位元組可用。試過的便宜手段都沒效——<code>-ffunction-sections</code> 沒有變化（編譯器預設就做了），移掉沒用到的檔案只省 208 位元組。最後改驅動裡兩行字型選擇，換來 10.6 KB。代價是標題變小，而且所有依賴 14 像素字寬的版面都要重算。<b>四款遊戲，或是大字型，選一個。</b></p>
<p><b>四、查表，但不是每次都查。</b>2048 的 13 個顏色用表（26 位元組），Gate Keeper 的車道幾何用表。但音高<b>沒有</b>用表——<code>tone_on()</code> 每次現算，因為要支援 400 Hz 到 4 kHz 的連續掃頻，一張涵蓋所有頻率的表會比那幾行除法大得多。判斷標準一直是「表的大小 vs 它省下的東西」。</p>
<p><b>五、組圖緩衝區的大小由最大的使用者決定。</b>{{J:App/gfx.h::#define GFX_BUF_PIX::GFX_BUF_PIX}} 是 4800 個像素、9600 位元組，占了 RAM 用量的八成。它不是隨便取的：Dino 的恐龍區塊要 44 × 100 = 4400，取 4800 留餘裕。這是 RAM 和「一次能畫多大一塊」之間的直接交換。</p>

<h3>如果要加第四款遊戲</h3>
<p>右邊的流程圖是完整清單，五個地方：</p>
<ol>
<li>寫一個新的 <code>.c</code>，提供一個 <code>void Xxx_Run(void)</code>，內部照第 7 章的骨架跑主迴圈，看到 KEY1 就 <code>return</code>。</li>
<li><code>console.h</code> 的 <code>AppScreen</code> 加一個列舉值，並加上函式宣告。</li>
<li><code>app_main.c</code> 的 switch 加一個 case。</li>
<li><code>menu.c</code> 的三個項目陣列各加一行，<code>ITEM_N</code> 加一。</li>
<li>把新檔案加進 Keil 專案的 App 群組。</li>
</ol>
<div class="warn"><h4>兩個容易忘的地方</h4>
<ul>
<li><b>列舉順序要和選單項目表一致</b>，因為選單是用 <code>APP_DINO + i</code> 回傳的。只改一邊會進錯遊戲。</li>
<li><b>主迴圈裡要呼叫 <code>Sound_Update()</code></b>，否則第一個音效會一直響。</li>
</ul></div>
<p>空間只剩 3020 位元組，所以第四款遊戲得很省，或者先砍掉一些東西。下一刀的候選：<code>gameover</code> 那張 sprite（238 位元組，可以用文字代替）、Font16（3040 位元組，但整站版面要重算）、或在 <code>tools/gen_sprites.py</code> 裡把 sprite 改小。</p>

<h3>刻意沒做的事</h3>
<ul>
<li><b>按鍵用輪詢而不是中斷。</b>50 到 100 Hz 的輪詢配兩次取樣去彈跳，對遊戲迴圈來說是更穩的選擇，而且不會和既有的中斷處理衝突。</li>
<li><b>高分只存在 RAM</b>，關機就沒了。寫進內部 Flash 的函式在 <code>user_defined.c</code> 裡已經有了，是一個明確的下一步。</li>
<li><b>第二支搖桿只有被讀取，沒有遊戲用它。</b>它是雙人模式最明顯的掛勾。</li>
</ul>
""",
"demos":[],
"ex":[
 {"id":"c12a","type":"mc",
  "q":"你加了第四款遊戲，列舉加在 <code>APP_GATE</code> 和 <code>APP_IOTEST</code> 之間，但選單項目表把新項目加在最後。結果是？",
  "opts":["編譯失敗","選單顯示正常，但選新遊戲會進 I/O TEST，選 I/O TEST 會進新遊戲",
          "新遊戲永遠進不去","選單只會顯示三個項目"],
  "ans":1,
  "fb":"選單回傳的是 <code>APP_DINO + i</code>，也就是用「第幾個項目」直接當列舉的偏移。兩邊順序一旦不一致，對應就錯位。這是一個為了省掉對應表而付出的代價，加東西時要特別注意。"},
 {"id":"c12b","type":"mc",
  "q":"為什麼 <code>GFX_BUF_PIX</code> 是 4800 而不是更大或更小？",
  "opts":["因為 4800 是 2 的次方","因為最大的使用者是 Dino 的恐龍區塊 44×100 = 4400，取 4800 留餘裕",
          "因為 ILI9341 一次最多接受 4800 個像素","因為 RAM 只有 9600 位元組"],
  "ans":1,
  "fb":"它由需求反推：恐龍區塊要蓋住從最高跳躍點到地面的整條垂直帶，寬度要容納最寬的蹲下姿勢。橫向車道是 139×26 = 3614。取 4800 同時滿足兩者。這個數字是 RAM 和「一次能畫多大一塊」的直接交換。"},
]},
]
