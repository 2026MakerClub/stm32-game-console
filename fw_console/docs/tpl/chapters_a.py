# -*- coding: utf-8 -*-
"""第 1 到 6 章。行號一律用 needle 字串去真實檔案裡找，不寫死。"""

AM = "App/app_main.c"
CH = "App/console.h"
MN = "App/menu.c"
UI = "App/ui.c"
IN = "App/input.c"
IH = "App/input.h"
GF = "App/gfx.c"
GH = "App/gfx.h"
SC = "App/sound.c"
SH = "App/sound.h"

CHAPTERS_A = [

# ===================================================================== 1
{
"n": 1, "title": "開機",
"problem": "七個初始化呼叫，每一個都可能依賴前一個已經做完的事。順序排錯的症狀不是當機，而是「某個功能就是不會動」，而你找不到原因。這一章講清楚每一步在等什麼。",
"files": [AM, CH],
"focus": (AM, "    stm32f4_Hardware_Init();"),
"flowName": "main() 的啟動順序",
"flowHint": "橘色邊框的節點有前置依賴，順序不能換。雙擊任何節點跳到程式。",
"flow": {"w": 560, "h": 700, "nodes": [
  {"id":"pw","x":180,"y":8,"w":180,"h":40,"cls":"ev","t":"電源接上","sub":"CPU 從 Reset_Handler 開始",
   "cap":"啟動碼把 SRAM 清好、把初值搬進 RAM，然後跳進 <b>main()</b>。"},
  {"id":"hw","x":150,"y":72,"w":240,"h":54,"cls":"dec","t":"stm32f4_Hardware_Init()","sub":"PLL · SysTick · 算出四個匯流排頻率",
   "tgt":(AM,"    stm32f4_Hardware_Init();"),
   "cap":"<b>必須第一個。</b>它設好時脈樹並把 PCLK1_Freq 等四個值算出來，後面每一步都建立在這上面。"},
  {"id":"lcd","x":150,"y":150,"w":240,"h":48,"t":"LCD_Init()","sub":"FSMC + ILI9341 初始化序列",
   "tgt":(AM,"    LCD_Init();"),
   "cap":"設定 FSMC、把螢幕用到的腳位交給它、送出面板的初始化指令、開背光。"},
  {"id":"on","x":150,"y":222,"w":240,"h":40,"t":"LCD_DisplayOn()","sub":"打開顯示輸出",
   "tgt":(AM,"    LCD_DisplayOn();"),
   "cap":"到這裡畫面才真的會亮。"},
  {"id":"cal","x":150,"y":286,"w":240,"h":54,"cls":"dec","t":"Default_Calibration()","sub":"觸控座標係數",
   "tgt":(AM,"    Default_Calibration();"),
   "cap":"<b>必須在 LCD_Init() 之後。</b>它算係數時要問螢幕多寬，而那個值在面板初始化前沒有意義。"},
  {"id":"drv","x":150,"y":364,"w":240,"h":48,"t":"Driver_GPIO()","sub":"觸控的 SPI 腳位 + 板載 KEY1",
   "tgt":(AM,"    Driver_GPIO();"),
   "cap":"設定觸控那四條線，順手把 KEY1（PB8）設成輸入上拉。"},
  {"id":"inp","x":150,"y":436,"w":240,"h":54,"cls":"dec","t":"Input_Init()","sub":"按鍵 · 搖桿 ADC · 量中心值",
   "tgt":(AM,"    Input_Init();"),
   "cap":"<b>這一刻不要碰搖桿。</b>最後一步會把「現在的讀數」定義成中心，你推著它就會被記進去。"},
  {"id":"snd","x":150,"y":514,"w":240,"h":54,"cls":"dec","t":"Sound_Init()","sub":"PA6 交給 TIM3，設成 PWM",
   "tgt":(AM,"    Sound_Init();"),
   "cap":"<b>需要第一步算出的 PCLK1_Freq</b> 才能知道計時器跑多快，所以不能提前。"},
  {"id":"sp","x":150,"y":592,"w":240,"h":48,"cls":"io","t":"splash()","sub":"4.5 秒，顯示四軸即時讀數",
   "tgt":(AM,"#define SPLASH_MS"),
   "cap":"開機畫面兼第一道診斷：四個 ADC 數字在這裡就會告訴你搖桿有沒有接好。"},
  {"id":"lp","x":150,"y":656,"w":240,"h":38,"cls":"io","t":"for (;;) { Menu_Run(); … }","sub":"永遠不回傳",
   "tgt":(AM,"        screen = Menu_Run();"),
   "cap":"第 2 章的主題。"},
 ], "edges":[
  {"a":"pw","b":"hw"},{"a":"hw","b":"lcd"},{"a":"lcd","b":"on"},{"a":"on","b":"cal"},
  {"a":"cal","b":"drv"},{"a":"drv","b":"inp"},{"a":"inp","b":"snd"},{"a":"snd","b":"sp"},
  {"a":"sp","b":"lp"},
 ]},
"html": """
<h3>七步，三個真正的依賴</h3>
<p>整個 <code>main()</code> 只有十幾行，但檔案開頭那段註解把每一步為什麼排在那裡都列出來了。實際上只有三處是「換了就壞」：</p>
<ul>
<li><b>時脈要第一個。</b>{{J:App/app_main.c::    stm32f4_Hardware_Init();::app_main.c 的第一行呼叫}} 設好 PLL 並算出 <code>SYS_Freq</code>、<code>HCLK_Freq</code>、<code>PCLK1_Freq</code>、<code>PCLK2_Freq</code>。後面所有周邊的時間計算都讀這幾個值。</li>
<li><b>觸控校正要在螢幕之後。</b>{{J:App/app_main.c::    Default_Calibration();::Default_Calibration()}} 把觸控的原始讀數換算成像素座標，係數算的時候要知道螢幕多寬。在 <code>LCD_Init()</code> 之前問，答案是沒有意義的。</li>
<li><b>發聲要在時脈之後。</b>{{J:App/sound.c::static uint32_t timer_clock(void)::sound.c 的 timer_clock()}} 直接讀 <code>PCLK1_Freq</code>。第一步沒跑完，它會拿到 0，然後 <code>tone_on()</code> 裡的除法會算出 0 並直接靜音——而且不會有任何錯誤訊息。</li>
</ul>
<p>其餘四步的順序是為了讀起來合理，不是硬性要求。</p>

<h3>splash 為什麼要印四個數字</h3>
<p>開機畫面停 4.5 秒，中間一直更新一行 {{J:App/app_main.c::sprintf(buf, "JOY %4u::四軸的即時 ADC 讀數}}。這不是裝飾。</p>
<p>搖桿的兩軸是分壓電路，要有電源才會輸出電壓；而搖桿上的按鍵只是一個對地的開關，<b>沒有接電源也能正常工作</b>。所以「按下去有反應、推動沒反應」這個組合，幾乎一定是 3.3 V 那條線沒接上。把四個數字放在開機畫面，是讓這個判斷在進選單之前就能做完。</p>
<p>正常接好的搖桿靜止時會落在 2000 附近；讀到 0 表示那一軸沒有電源或沒有接線。</p>

<h3>那兩次看起來多餘的 Input_Poll</h3>
<p>{{J:App/app_main.c::    Input_Flush();::splash() 裡的三行}}長這樣：連續呼叫 <code>Input_Poll()</code> 兩次，然後 <code>Input_Flush()</code>。</p>
<p>原因是開機畫面允許「按任意鍵跳過」，而跳過的判斷用的是<b>邊緣</b>——「剛剛才按下去」。輸入層記錄上一次的狀態來算邊緣，而那個變數一開始是 0。如果有一顆按鍵因為接線錯誤一直被拉低，第一次 poll 就會產生一個假的「剛按下」，開機畫面瞬間閃過，而你完全不知道為什麼。</p>
<p>連 poll 兩次讓去彈跳濾波器有兩筆資料可以比對，把狀態填成真實值；<code>Input_Flush()</code> 再把累積的邊緣丟掉。之後出現的任何邊緣都是人真的按的。第 4 章會看去彈跳本身怎麼寫。</p>

<div class="note"><b>順帶一提：</b><code>main()</code> 結尾是 <code>for(;;)</code> 而不是 <code>return 0;</code>。沒有作業系統可以接收回傳值，跳出 <code>main()</code> 就無處可去。韌體的主迴圈本身就是程式的終點。</div>
""",
"demos": [],
"ex": [
 {"id":"c1a","type":"mc",
  "q":"如果把 <code>Sound_Init()</code> 搬到 <code>stm32f4_Hardware_Init()</code> 前面，會發生什麼？",
  "opts":["編譯失敗","程式當機在 Sound_Init 裡",
          "能跑但完全沒聲音，而且 TIM3 的暫存器值看起來都正常",
          "聲音會變成正確音高的兩倍"],
  "ans":2,
  "fb":"<code>timer_clock()</code> 會讀到還沒被算出來的 <code>PCLK1_Freq</code>（0），於是 <code>tone_on()</code> 裡 <code>total = 0 / freq</code> 得到 0，函式直接走進 <code>Sound_Stop()</code> 就回來了。沒有錯誤、沒有當機，只是永遠安靜——這種失敗最難查，也是為什麼那段註解要把依賴寫出來。"},
 {"id":"c1b","type":"mc",
  "q":"開機時你正好推著搖桿。之後放手，選單的選擇框自己往下跑。為什麼？",
  "opts":["搖桿壞了","去彈跳濾波器把搖桿當成按鍵",
          "<code>Input_Init()</code> 把你推著的位置記成中心，放手後真正的中心相對基準就有偏移",
          "splash 的 4.5 秒不夠久"],
  "ans":2,
  "fb":"中心值是開機那一刻量的。你推著的讀數變成基準，放手後偏移量超過門檻就一直被判定成有方向。I/O TEST 裡的 CENTRE STICKS 可以重新量。"},
]},

# ===================================================================== 2
{
"n": 2, "title": "主迴圈與畫面切換",
"problem": "一台機器有選單和三款遊戲，玩家要能進去、玩、退出、再進別的。這需要一個「現在在哪個畫面」的概念，以及一條乾淨的回家路。這裡用的是最樸素的做法，而它剛好也是最好維護的做法。",
"files": [AM, CH],
"focus": (AM, "        switch (screen) {"),
"flowName": "狀態機",
"flowHint": "四個遊戲都是「跑到玩家退出才 return」。雙擊節點看程式。",
"flow": {"w": 620, "h": 480, "nodes": [
  {"id":"m","x":215,"y":16,"w":190,"h":46,"cls":"io","t":"Menu_Run()","sub":"阻塞，直到玩家選了一個",
   "tgt":(MN,"AppScreen Menu_Run(void)"),
   "cap":"它自己有主迴圈，選到才回傳一個 <b>AppScreen</b>。"},
  {"id":"sw","x":235,"y":96,"w":150,"h":42,"cls":"dec","t":"switch (screen)","sub":"一個列舉，四條路",
   "tgt":(AM,"        switch (screen) {"),
   "cap":"整個狀態機就是這個 switch。加遊戲＝加一個 case。"},
  {"id":"d","x":18,"y":180,"w":140,"h":52,"t":"Dino_Run()","sub":"教學 → 跑酷 → 結束畫面",
   "tgt":("App/game_dino.c","void Dino_Run(void)"),
   "cap":"第 8 章。"},
  {"id":"t","x":172,"y":180,"w":140,"h":52,"t":"Game2048_Run()","sub":"教學 → 盤面",
   "tgt":("App/game_2048.c","void Game2048_Run(void)"),
   "cap":"第 9 章。"},
  {"id":"g","x":326,"y":180,"w":140,"h":52,"t":"Gate_Run()","sub":"教學 → 波次",
   "tgt":("App/game_gate.c","void Gate_Run(void)"),
   "cap":"第 10 章。"},
  {"id":"i","x":480,"y":180,"w":124,"h":52,"cls":"hw","t":"IoTest_Run()","sub":"接線診斷",
   "tgt":("App/iotest.c","void IoTest_Run(void)"),
   "cap":"第 11 章，也是唯一講硬體的一章。"},
  {"id":"k","x":205,"y":286,"w":210,"h":44,"cls":"ev","t":"玩家按 KEY1","sub":"每個 *_Run 內部都在看它",
   "cap":"KEY1（PB8）在任何畫面都是返回。遊戲的主迴圈看到它就 <code>return</code>。"},
  {"id":"s","x":220,"y":370,"w":180,"h":42,"t":"Sound_Stop()","sub":"離開遊戲一定關聲音",
   "tgt":(AM,"        Sound_Stop();"),
   "cap":"防止某個音符卡在播放狀態被帶回選單。"},
 ], "edges":[
  {"a":"m","b":"sw"},
  {"a":"sw","b":"d","side":"r"},{"a":"sw","b":"t"},{"a":"sw","b":"g","side":"r"},{"a":"sw","b":"i","side":"r"},
  {"a":"d","b":"k","side":"r"},{"a":"t","b":"k"},{"a":"g","b":"k","side":"l"},{"a":"i","b":"k","side":"l"},
  {"a":"k","b":"s"},
  {"a":"s","b":"m","side":"back","lab":"回選單"},
 ]},
"html": """
<h3>狀態機就是一個列舉加一個 switch</h3>
<p>{{J:App/console.h::} AppScreen;::console.h 的 AppScreen}} 列出所有畫面。<code>Menu_Run()</code> 回傳玩家選了哪一個，<code>main()</code> 的 switch 呼叫對應的函式。</p>
<p>沒有框架、沒有函式指標表、沒有註冊機制。三十行以內就講完，而且它有一個不明顯的好處：<b>每個遊戲完全獨立</b>。它們不共用全域狀態，不互相呼叫，也不需要知道彼此存在。</p>

<h3>「跑到玩家退出才 return」這個約定</h3>
<p>每個 <code>*_Run()</code> 內部有自己的主迴圈。它不是「更新一幀然後回來」，而是整場玩完才回來。第 7 章會看那個迴圈長什麼樣。</p>
<p>這個選擇的代價是每個遊戲要自己處理退出；好處是遊戲內部可以有自己的子狀態（教學、遊玩、結束畫面）而完全不用讓 <code>main()</code> 知道。<code>Dino_Run()</code> 的結構就是這樣：</p>
<pre class="note" style="font-family:var(--sm);font-size:12.5px;white-space:pre">void Dino_Run(void)
{
    if (!dino_tutorial()) return;          /* 教學裡按 KEY1 直接回選單 */

    for (;;) {
        if (!dino_play())     return;      /* 遊戲中按 KEY1            */
        if (!dino_gameover()) return;      /* 結束畫面選 MENU          */
    }                                      /* 選 RETRY 就再玩一場      */
}</pre>
<p>「重玩」在這裡只是一個 <code>for</code> 迴圈，不需要回到 <code>main()</code> 再進來一次。</p>

<h3>回傳值當作退出原因</h3>
<p>注意 <code>dino_play()</code> 回傳的是 <code>uint8_t</code>：1 代表「玩家死了」，0 代表「玩家按 KEY1 要走」。兩種結束要走不同的路——死了要看結束畫面，主動離開要直接回選單。用回傳值區分比用全域旗標乾淨。</p>

<div class="note"><b>如果要加第四款遊戲：</b>在 <code>AppScreen</code> 加一個列舉、在 switch 加一個 case、在 <code>menu.c</code> 的項目表加一行、寫一個新的 <code>.c</code> 檔提供 <code>Xxx_Run(void)</code>。第 12 章會把完整清單列出來。</div>
""",
"demos": [],
"ex": [
 {"id":"c2a","type":"mc",
  "q":"<code>dino_play()</code> 為什麼要回傳值，而不是直接 <code>void</code>？",
  "opts":["為了讓編譯器檢查","為了區分「玩家死了」和「玩家按 KEY1 要離開」，兩者接下來要做的事不同",
          "為了回傳分數","為了相容 AppScreen 列舉"],
  "ans":1,
  "fb":"死掉要顯示結束畫面並讓玩家選重玩或回選單；主動離開要立刻回選單。用回傳值把這個決定交給呼叫者，比在函式裡塞一個全域旗標好追。"},
]},

# ===================================================================== 3
{
"n": 3, "title": "選單",
"problem": "選單是全機唯一必須同時支援觸控和按鍵的畫面。原因很實際：觸控校正跑掉是展示場合最常見的故障，如果選單只能用觸控，機器就等於整台壞掉。",
"files": [MN, UI],
"focus": (MN, "AppScreen Menu_Run(void)"),
"flowName": "Menu_Run() 的一次輪詢",
"flowHint": "兩條輸入路徑並存。雙擊節點跳到程式。",
"flow": {"w": 620, "h": 520, "nodes": [
  {"id":"dr","x":210,"y":12,"w":200,"h":44,"t":"draw_all(sel)","sub":"整頁畫一次，只在進入時",
   "tgt":(MN,"static void draw_all(uint8_t sel)"),
   "cap":"標題、四個項目、底部提示。之後只重畫變動的項目。"},
  {"id":"t0","x":240,"y":78,"w":140,"h":38,"cls":"ev","t":"t0 = msTick","sub":"10 ms 一輪",
   "cap":"選單不需要 50 fps，10 毫秒一輪已經很流暢。"},
  {"id":"ip","x":210,"y":140,"w":200,"h":44,"cls":"io","t":"Input_Poll()","sub":"按鍵 + 搖桿",
   "tgt":(IN,"void Input_Poll(void)"),
   "cap":"第 4 章。這裡只取用它的結果。"},
  {"id":"pr","x":30,"y":210,"w":190,"h":50,"cls":"dec","t":"Input_Pressed()","sub":"上下移動 / 押下確認",
   "tgt":(MN,"            uint16_t p = Input_Pressed();"),
   "cap":"用<b>邊緣</b>而不是電位：按著上鍵不會一直往上跑。"},
  {"id":"ts","x":400,"y":210,"w":190,"h":50,"cls":"dec","t":"TS_GetState(&ts)","sub":"觸控座標",
   "tgt":(MN,"        TS_GetState(&ts);"),
   "cap":"有人在摸才回傳座標。"},
  {"id":"mv","x":30,"y":288,"w":190,"h":46,"t":"sel 改變 → 重畫兩項","sub":"舊的取消亮，新的亮",
   "tgt":(MN,"                draw_item(old, 0);"),
   "cap":"只重畫變動的兩個項目，不是整頁。這是全機通用的做法。"},
  {"id":"hb","x":400,"y":288,"w":190,"h":46,"t":"UI_HitBox()","sub":"點擊落在哪個矩形裡",
   "tgt":(UI,"uint8_t UI_HitBox(uint16_t tx, uint16_t ty,"),
   "cap":"四個邊界比較，回傳 0 或 1。"},
  {"id":"rt","x":215,"y":368,"w":190,"h":46,"cls":"io","t":"return APP_DINO + i","sub":"告訴 main() 選了誰",
   "tgt":(MN,"                    return (AppScreen)(APP_DINO + i);"),
   "cap":"列舉的順序和項目表的順序刻意對齊，所以可以直接相加。"},
  {"id":"wt","x":215,"y":446,"w":190,"h":40,"cls":"ev","t":"等到 10 ms","sub":"while (msTick - t0 < 10)",
   "cap":"第 7 章會講這個寫法為什麼能扛住溢位。"},
 ], "edges":[
  {"a":"dr","b":"t0"},{"a":"t0","b":"ip"},
  {"a":"ip","b":"pr"},{"a":"ip","b":"ts"},
  {"a":"pr","b":"mv"},{"a":"ts","b":"hb"},
  {"a":"mv","b":"rt"},{"a":"hb","b":"rt"},
  {"a":"rt","b":"wt","lab":"沒選就繼續"},
  {"a":"wt","b":"t0","side":"back","lab":"下一輪"},
 ]},
"html": """
<h3>命中判定只有兩行</h3>
<p>{{J:App/ui.c::uint8_t UI_HitBox(uint16_t tx, uint16_t ty,::UI_HitBox()}} 判斷一個座標有沒有落在矩形裡。它是四個邊界比較，寫成一個回傳 <code>uint8_t</code> 的小函式，然後全機所有可點的東西都用它——選單項目、2048 的 NEW 和 MENU 按鈕、教學畫面的 SKIP、I/O TEST 的每一顆按鈕。</p>
<p>值得注意的是它<b>沒有</b>被做成「按鈕物件」。沒有註冊、沒有回呼、沒有清單。每個畫面自己知道自己有哪些矩形，自己比對。畫面數量有限的時候，這樣比一套按鈕框架短得多，而且看一眼就知道會發生什麼。</p>

<h3>兩條輸入路徑寫在同一個迴圈裡</h3>
<p>{{J:App/menu.c::AppScreen Menu_Run(void)::Menu_Run()}} 的迴圈裡兩段並存：先看按鍵的邊緣、再看觸控。兩者都可能造成回傳，互不干擾。</p>
<p>按鍵那段用的是 {{J:App/menu.c::            uint16_t p = Input_Pressed();::Input_Pressed()}}，也就是<b>邊緣</b>。如果用 <code>Input_Held()</code>，按著上鍵選擇框會以每 10 毫秒一格的速度飛過去。這是「電位」和「邊緣」語意差別的第一個實例，第 4 章會講清楚。</p>

<h3>只重畫變動的部分</h3>
<p>選擇框移動時，程式重畫<b>兩個</b>項目——舊的那個取消高亮、新的那個加上高亮：</p>
<pre class="note" style="font-family:var(--sm);font-size:12.5px;white-space:pre">if (sel != old) {
    SFX_Select();
    draw_item(old, 0);
    draw_item(sel, 1);
}</pre>
<p>整頁重畫在這裡也不會慢到看得出來，但這個習慣在遊戲畫面裡是必要的。第 5 章會看到同樣的原則被推到極致。</p>

<h3>列舉順序和項目表順序刻意對齊</h3>
<p>{{J:App/menu.c::                    return (AppScreen)(APP_DINO + i);::回傳的那一行}}是 <code>APP_DINO + i</code>。這依賴一個約定：<code>AppScreen</code> 裡 <code>APP_DINO</code>、<code>APP_2048</code>、<code>APP_GATE</code>、<code>APP_IOTEST</code> 的順序，必須和 <code>ITEM_NAME[]</code> 的順序一致。</p>
<p>這是一個省掉對應表的小技巧，代價是<b>加遊戲時兩邊都要改，而且順序要一致</b>。如果只改一邊，選單會進錯遊戲。這種隱含約定值得在加東西時特別注意。</p>
""",
"demos": [],
"ex": [
 {"id":"c3a","type":"mc",
  "q":"選單的上下移動如果改用 <code>Input_Held()</code> 而不是 <code>Input_Pressed()</code>，會怎樣？",
  "opts":["完全一樣","按著方向鍵時選擇框會以每 10 毫秒一格的速度衝過去",
          "按鍵會失效","會需要重新校正觸控"],
  "ans":1,
  "fb":"<code>Held</code> 是電位：按著的每一輪都成立。<code>Pressed</code> 是邊緣：只在按下去那一輪成立。選單要的是後者。反過來說，Dino 的「蹲下」要的是 <code>Held</code>，因為那是持續狀態。"},
 {"id":"c3b","type":"fill",
  "q":"補完 <code>UI_HitBox()</code> 的判斷式：點 (tx, ty) 落在以 (x, y) 為左上角、寬 w 高 h 的矩形內。",
  "ph":"return (tx >= x && ...);",
  "need":[[r"/tx\s*>=\s*x/","要先判斷 tx 不小於 x"],
          [r"/tx\s*<\s*\(?\s*x\s*\+\s*w/","右邊界是 x + w，而且用 &lt; 不用 &lt;=（寬 w 的矩形佔 x 到 x+w-1）"],
          [r"/ty\s*>=\s*y/","y 方向也要下界"],
          [r"/ty\s*<\s*\(?\s*y\s*\+\s*h/","y 方向的上界是 y + h"]],
  "ans":"return (uint8_t)(tx &gt;= x &amp;&amp; tx &lt; (uint16_t)(x + w) &amp;&amp;\n                 ty &gt;= y &amp;&amp; ty &lt; (uint16_t)(y + h));",
  "fb":"四個比較，用 <code>&amp;&amp;</code> 串起來。右／下邊界用 <code>&lt;</code> 而不是 <code>&lt;=</code>，因為寬度 w 的矩形佔的是 x 到 x+w-1 這 w 個像素。"},
]},

# ===================================================================== 4
{
"n": 4, "title": "輸入層",
"problem": "遊戲程式需要知道「玩家要跳」，不需要知道那是哪一支腳位、按鍵接地還是接電源、有沒有彈跳。這一章講輸入層怎麼把這些擋在下面，以及它對外只留四個函式的理由。",
"files": [IN, IH, MN],
"focus": (IN, "void Input_Poll(void)"),
"flowName": "Input_Poll() 的一次取樣",
"flowHint": "左邊是四顆按鍵，右邊是四軸搖桿，最後合成同一個 16 位元遮罩。",
"flow": {"w": 640, "h": 560, "nodes": [
  {"id":"call","x":230,"y":10,"w":190,"h":40,"cls":"ev","t":"每一幀呼叫一次","sub":"遊戲迴圈的第一件事",
   "cap":"10 或 20 毫秒一次。取樣間隔決定了去彈跳的時間常數。"},
  {"id":"btn","x":20,"y":76,"w":200,"h":56,"cls":"hw","t":"讀四顆按鍵","sub":"active low：按下讀 0",
   "tgt":(IN,"    if (!(e & (1UL << PIN_BTN_UP)))    raw |= BTN_UP;"),
   "cap":"按鍵接地、內部上拉，所以判斷要取反。第 11 章補硬體細節。"},
  {"id":"adc","x":420,"y":76,"w":200,"h":56,"cls":"hw","t":"讀四軸 ADC","sub":"12 位元，0 到 4095",
   "tgt":(IN,"uint16_t Input_AdcRaw(uint8_t ch)"),
   "cap":"每軸一次轉換，約 6 微秒。第 11 章補硬體細節。"},
  {"id":"ax","x":420,"y":156,"w":200,"h":56,"cls":"dec","t":"axis_bits()","sub":"兩個門檻做遲滯",
   "tgt":(IN,"static uint16_t axis_bits(uint16_t raw, uint16_t centre,"),
   "cap":"已經推著的軸用比較寬鬆的門檻判斷是否還推著，避免在門檻附近抖動。"},
  {"id":"raw","x":225,"y":236,"w":200,"h":48,"t":"raw：這次的原始狀態","sub":"16 個位元，按鍵和搖桿混在一起",
   "tgt":(IH,"#define BTN_UP"),
   "cap":"按鍵和搖桿方向共用一個遮罩，遊戲因此可以寫 <code>pressed & ANY_UP</code> 一視同仁。"},
  {"id":"deb","x":225,"y":308,"w":200,"h":56,"cls":"dec","t":"去彈跳","sub":"兩次取樣一致才准改變",
   "tgt":(IN,"    agree     = (uint16_t)~(raw ^ s_lastRaw);"),
   "cap":"三行位元運算換掉一整組計時器。"},
  {"id":"held","x":80,"y":392,"w":180,"h":48,"cls":"io","t":"s_held","sub":"電位：現在按著嗎",
   "tgt":(IN,"uint16_t Input_Held(void)"),
   "cap":"用於持續狀態，例如 Dino 的蹲下。"},
  {"id":"edge","x":390,"y":392,"w":180,"h":48,"cls":"io","t":"s_edge","sub":"邊緣：剛剛才按下",
   "tgt":(IN,"    s_edge |= (uint16_t)(s_held & ~s_prev);"),
   "cap":"用於一次性動作，例如跳躍、開火、選單移動。讀走就清掉。"},
  {"id":"game","x":225,"y":470,"w":200,"h":48,"t":"遊戲只看這兩個","sub":"從不自己讀腳位",
   "cap":"這是分層的意義：遊戲邏輯不知道 active low、不知道 ADC、不知道腳位編號。"},
 ], "edges":[
  {"a":"call","b":"btn"},{"a":"call","b":"adc"},
  {"a":"adc","b":"ax"},
  {"a":"btn","b":"raw"},{"a":"ax","b":"raw"},
  {"a":"raw","b":"deb"},
  {"a":"deb","b":"held"},{"a":"deb","b":"edge"},
  {"a":"held","b":"game"},{"a":"edge","b":"game"},
 ]},
"html": """
<h3>對外只有四個函式</h3>
<p>整個輸入層對遊戲只露出四個名字：</p>
<ul>
<li><code>Input_Poll()</code> — 取樣一次。每幀呼叫，其他三個都只是讀它的結果。</li>
<li><code>Input_Held()</code> — <b>電位</b>。現在按著嗎？用於持續狀態。</li>
<li><code>Input_Pressed()</code> — <b>邊緣</b>。剛剛才按下嗎？<b>讀走就清掉</b>，所以每個按下只會被消費一次。</li>
<li><code>Input_Flush()</code> — 把累積的邊緣丟掉。切換畫面時用，避免上一個畫面殘留的按鍵被下一個畫面吃到。</li>
</ul>
<p>Dino 的兩個動作剛好示範了前兩者的差別：</p>
<pre class="note" style="font-family:var(--sm);font-size:12.5px;white-space:pre">if (pressed &amp; ANY_UP)  { ... 起跳 ... }      /* 邊緣：只在按下那一幀 */
s_ducking = (held &amp; ANY_DOWN) ? 1 : 0;      /* 電位：按著就蹲       */</pre>

<h3>去彈跳：三行，不用計時器</h3>
<p>機械接點按下的瞬間會連續通斷幾毫秒。常見做法是為每顆按鍵配一個計時器，這裡{{J:App/input.c::    agree     = (uint16_t)~(raw ^ s_lastRaw);::用三行做完}}：</p>
<pre class="note" style="font-family:var(--sm);font-size:12.5px;white-space:pre">agree     = (uint16_t)~(raw ^ s_lastRaw);
s_held    = (uint16_t)((s_held &amp; ~agree) | (raw &amp; agree));
s_lastRaw = raw;</pre>
<p><code>raw ^ s_lastRaw</code> 是「這次和上次不同的位元」，取反之後 <code>agree</code> 就是「這兩次一致的位元」。第二行的意思是：<b>只有連續兩次取樣一致的位元，才准改變 s_held</b>；其他位元沿用舊值。</p>
<p>迴圈 10 毫秒一輪，所以這是一個 20 毫秒的濾波器，比輕觸開關的彈跳時間長。而且它<b>一次處理 16 個位元</b>——四顆按鍵、兩個搖桿鍵、KEY1、八個搖桿方向，全部同時被過濾。這是把狀態擠進一個整數的回報。</p>

<h3>邊緣的累積與清空</h3>
<p>{{J:App/input.c::    s_edge |= (uint16_t)(s_held & ~s_prev);::邊緣計算}}用 <code>|=</code> 而不是 <code>=</code>，這一點很重要：如果遊戲某一幀特別忙、<code>Input_Poll()</code> 被呼叫了兩次才輪到 <code>Input_Pressed()</code>，中間那次的按下不會被蓋掉。按鍵不會漏。</p>
<p>反過來 <code>Input_Pressed()</code> 讀完就清零，所以同一個按下也不會被吃兩次。</p>

<h3>搖桿：開機量中心，兩個門檻做遲滯</h3>
<p>搖桿靜止時的讀數不是理論上的 2048。電位器有公差、電源有誤差，每一支都不一樣。所以 {{J:App/input.c::void Input_Recalibrate(void)::Input_Recalibrate()}} 把「現在的讀數」定義成中心，每軸平均 16 次。這直接推導出「開機不要碰搖桿」這條使用規則。</p>
<p>有了中心值，方向判定還有第二個問題：搖桿停在門檻附近時，雜訊會讓它反覆跨越，方向訊號一秒抖幾十次。{{J:App/input.c::static uint16_t axis_bits(uint16_t raw, uint16_t centre,::axis_bits()}} 的解法是進入和離開用不同的門檻：</p>
<pre class="note" style="font-family:var(--sm);font-size:12.5px;white-space:pre">if ((was &amp; posBit) &amp;&amp; d &gt;  JOY_OFF) out |= posBit;   /* 已經推著：用低門檻 */
else if (d &gt;  JOY_ON)               out |= posBit;   /* 還沒推：用高門檻   */</pre>
<p><code>was</code> 是上一幀的結果。{{J:App/input.c::#define JOY_ON::JOY_ON 是 700、JOY_OFF 是 450}}，中間那 250 個計數就是不會抖動的緩衝區。</p>

<h3>類比值走另一條路</h3>
<p>方向位元夠 Dino 用，但 Gate Keeper 的收集器要平滑移動，所以另外有 {{J:App/input.c::int16_t Input_JoyAxis(uint8_t idx)::Input_JoyAxis()}} 回傳 −512 到 +511。裡面有一個容易忽略的細節：死區是<b>減掉</b>而不是歸零。</p>
<pre class="note" style="font-family:var(--sm);font-size:12.5px;white-space:pre">if (d &gt; -JOY_DEAD &amp;&amp; d &lt; JOY_DEAD) return 0;
d = (d &gt; 0) ? (d - JOY_DEAD) : (d + JOY_DEAD);   /* 減掉，不是照原值 */</pre>
<p>如果死區外照原值回傳，搖桿一離開死區輸出就從 0 跳到 250，收集器會瞬間彈開。減掉之後，輸出從死區邊緣的 0 平滑爬升。</p>

<div class="note"><b>分層的價值：</b>遊戲程式裡沒有一行 <code>GPIOE-&gt;IDR</code>。按鍵改接哪支腳、搖桿反了要不要翻轉、去彈跳要幾毫秒，全部是 <code>input.c</code> 內部的事。<span class="deepslot" data-k="gpio" data-l="GPIO 與上拉的硬體細節"></span></div>
""",
"demos": [],
"ex": [
 {"id":"c4a","type":"mc",
  "q":"<code>s_edge |= (s_held &amp; ~s_prev);</code> 這一行用 <code>|=</code> 而不是 <code>=</code>。差別是什麼？",
  "opts":["沒有差別，每幀都會被讀走",
          "如果 Input_Poll() 在兩次 Input_Pressed() 之間跑了不只一次，用 = 會把中間那次的按下蓋掉",
          "|= 比較快","= 會讓邊緣永遠不清空"],
  "ans":1,
  "fb":"邊緣是「累積到被讀走為止」。用 <code>|=</code> 讓多次取樣之間的按下都保留下來，<code>Input_Pressed()</code> 讀完才清零。改成 <code>=</code> 的話，忙碌的那一幀裡玩家的按鍵可能直接消失。"},
 {"id":"c4b","type":"mc",
  "q":"某人在 Dino 的主迴圈裡直接寫 <code>if (!(GPIOE-&gt;IDR &amp; (1UL &lt;&lt; 1))) jump();</code>，繞過輸入層。除了破壞分層，最直接的功能問題是什麼？",
  "opts":["讀不到腳位","沒有去彈跳也沒有邊緣偵測，一次按下會觸發好幾次跳躍",
          "會讓 ADC 停止工作","編譯不過"],
  "ans":1,
  "fb":"腳位讀得到，但那是<b>瞬時電位</b>：彈跳期間會讀到好幾次 0，而且按著不放的每一幀都成立。跳躍會連發。輸入層存在的理由就是把這兩件事處理掉。"},
 {"id":"c4c","type":"fill",
  "q":"寫出 <code>axis_bits()</code> 裡「已經推著、判斷是否還推著」那一行的條件部分（正方向）。",
  "ph":"if ( ... ) out |= posBit;",
  "need":[[r"/was/","要看上一幀的狀態 was"],
          [r"/posBit/","要檢查的是 posBit 這一位"],
          [r"/JOY_OFF/","已經推著的時候用的是比較寬鬆的 JOY_OFF"],
          [r"/d\s*>/","比較的是偏移量 d 大於門檻"]],
  "ans":"if ((was &amp; posBit) &amp;&amp; d &gt; JOY_OFF) out |= posBit;",
  "fb":"關鍵是 <code>was &amp; posBit</code>：只有「上一幀就已經是這個方向」才走寬鬆門檻。還沒推的軸要跨過比較嚴格的 <code>JOY_ON</code>。兩個門檻之間的差距就是遲滯帶。"},
]},

# ===================================================================== 5
{
"n": 5, "title": "畫面層",
"problem": "把舊位置擦掉再畫新位置，在螢幕上是兩次可見的寫入，眼睛會看到中間那個空白的瞬間——這就是閃爍。gfx.h 開頭的註解把問題和解法都寫了，這一章把它拆開。",
"files": [GH, GF],
"focus": (GH, "#define GFX_BUF_PIX"),
"flowName": "一次 band 合成",
"flowHint": "三個呼叫要按順序。雙擊節點看程式。",
"flow": {"w": 600, "h": 560, "nodes": [
  {"id":"p","x":175,"y":10,"w":250,"h":50,"cls":"ev","t":"問題：擦掉再畫 = 兩次寫入","sub":"中間那一瞬間是空白的",
   "cap":"面板上的像素先變成背景色、再變成新顏色。眼睛抓到中間那格，就是閃爍。"},
  {"id":"b1","x":175,"y":88,"w":250,"h":52,"t":"Gfx_BandBegin(w, h, bg)","sub":"在 RAM 裡開一塊，填滿背景色",
   "tgt":(GF,"void Gfx_BandBegin(uint16_t w, uint16_t h, uint16_t bg)"),
   "cap":"w×h 超過 GFX_BUF_PIX 就<b>拒絕</b>並讓後續呼叫變成空操作，而不是寫壞別人的記憶體。"},
  {"id":"b2","x":30,"y":168,"w":230,"h":50,"t":"Gfx_BandRect(...)","sub":"地面線、格線、方塊",
   "tgt":(GF,"void Gfx_BandRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t colour)"),
   "cap":"全部畫在 RAM 裡，面板還沒看到任何東西。"},
  {"id":"b3","x":340,"y":168,"w":230,"h":50,"t":"Gfx_BandSprite(...)","sub":"1bpp 點陣圖 + 指定顏色",
   "tgt":(GF,"void Gfx_BandSprite(const Sprite *s, int16_t x, int16_t y, uint16_t ink)"),
   "cap":"位元是 1 才寫顏色，是 0 就留著背景——這就是透明。"},
  {"id":"b4","x":175,"y":248,"w":250,"h":52,"cls":"dec","t":"Gfx_BandEnd(x, y)","sub":"一次寫入面板",
   "tgt":(GF,"void Gfx_BandEnd(int16_t screenX, int16_t screenY)"),
   "cap":"開一個視窗，連續送出 w×h 個顏色。每個像素只被寫一次。"},
  {"id":"r","x":175,"y":330,"w":250,"h":46,"cls":"io","t":"面板上沒有中間狀態","sub":"所以沒有東西可以閃",
   "cap":"像素從舊值<b>直接</b>變成新值。"},
  {"id":"tr","x":175,"y":408,"w":250,"h":56,"t":"順手擦掉尾巴","sub":"區塊寬 = sprite 寬 + step",
   "tgt":("App/game_dino.c","    int16_t bw = (int16_t)(s->w + step);"),
   "cap":"障礙物往左移，多出來的 step 像素是背景色，剛好蓋掉剛剛讓出來的位置。一次寫入完成畫和擦。"},
  {"id":"o","x":175,"y":492,"w":250,"h":50,"t":"繪製順序 = 圖層","sub":"後畫的蓋住先畫的",
   "tgt":("App/game_gate.c","static void render(void)"),
   "cap":"Gate Keeper 的收集器最後畫，所以它在最上層。"},
 ], "edges":[
  {"a":"p","b":"b1"},{"a":"b1","b":"b2"},{"a":"b1","b":"b3"},
  {"a":"b2","b":"b4"},{"a":"b3","b":"b4"},
  {"a":"b4","b":"r"},{"a":"r","b":"tr"},{"a":"tr","b":"o"},
 ]},
"html": """
<h3>先在 RAM 裡組好，再一次推出去</h3>
<p>{{J:App/gfx.h::#define GFX_BUF_PIX::gfx.h}} 開頭的註解把這件事講完了：直覺的畫法是「用背景色把舊位置擦掉，再在新位置畫」，而那在螢幕上是兩次可見的寫入。</p>
<p>解法是把整塊區域——背景、地面線、角色，全部——畫進一塊 RAM 緩衝區，然後一次送到面板。像素從來沒有處在中間狀態，所以沒有東西可以閃。三個呼叫固定這個順序：</p>
<pre class="note" style="font-family:var(--sm);font-size:12.5px;white-space:pre">Gfx_BandBegin(w, h, 背景色);        /* 開一塊，填滿   */
Gfx_BandHLine(...);                 /* 想畫什麼就畫   */
Gfx_BandSprite(&amp;SPR_dino_run1, x, y, 前景色);
Gfx_BandEnd(螢幕x, 螢幕y);          /* 一次送出       */</pre>

<h3>GFX_BUF_PIX 4800 這個數字怎麼來的</h3>
<p>{{J:App/gfx.h::#define GFX_BUF_PIX::那一行}}是 4800 個像素，也就是 9600 位元組——整支韌體 RAM 用量最大的單一配置。</p>
<p>數字不是隨便挑的，它由<b>最大的使用者</b>決定。Dino 的恐龍區塊要蓋住從最高跳躍點到地面的整條垂直帶，寬度要容納最寬的姿勢（蹲下時 43 像素）：44 × 100 = 4400。Gate Keeper 最寬的橫向車道是 139 × 26 = 3614。取 4800 留一點餘裕。</p>
<p>更值得注意的是超過會怎樣：</p>
<pre class="note" style="font-family:var(--sm);font-size:12.5px;white-space:pre">if ((uint32_t)w * h &gt; GFX_BUF_PIX) {   /* 拒絕，而不是蓋掉別的 RAM */
    s_w = s_h = 0;
    return;
}</pre>
<p>它<b>拒絕</b>，並把尺寸設成 0，讓後續的 <code>BandRect</code>、<code>BandSprite</code>、<code>BandEnd</code> 全部變成空操作。症狀是「那一塊沒畫出來」，而不是「隨機的記憶體被寫壞、幾分鐘後在別的地方當掉」。前者十秒鐘就找到，後者可以耗掉一個晚上。</p>
<div class="warn"><h4>這個保護真的救過一次</h4><p>Gate Keeper 有一個函式想一次清掉整個競技場：320 × 210 = 67200 個像素，是緩衝區的十四倍。因為有這個檢查，症狀是「格線沒有畫出來」而不是記憶體損毀。後來改成直接呼叫面板的填色函式，不經過 band。</p></div>

<h3>sprite 怎麼存</h3>
<p>{{J:App/gfx.c::    stride = (uint16_t)((s->w + 7) / 8);::取位元那一段}}是整份程式最密的幾行：</p>
<pre class="note" style="font-family:var(--sm);font-size:12.5px;white-space:pre">stride = (uint16_t)((s-&gt;w + 7) / 8);        /* 一列佔幾個位元組 */
...
if (row[sx &gt;&gt; 3] &amp; (0x80u &gt;&gt; (sx &amp; 7)))    /* 第 sx 個位元是 1 嗎 */
    s_buf[(uint32_t)dy * s_w + dx] = ink;</pre>
<p>一個像素一個位元。<code>sx &gt;&gt; 3</code> 是「第幾個位元組」，<code>sx &amp; 7</code> 是「位元組裡的第幾位」。位元是 1 就寫前景色，是 0 就<b>什麼都不做</b>——背景色已經在那裡了，所以留白自動變成透明。</p>
<p>16 個 sprite 總共 2050 位元組。同樣的圖存成每像素 16 位元的話，光恐龍的六個姿勢就超過 12 KB。代價是一張圖只能一個顏色，而 Chrome Dino 本來就是單色的。</p>

<h3>繪製順序就是圖層</h3>
<p>沒有 z-order、沒有排序，後畫的就在上面。{{J:App/game_gate.c::static void render(void)::Gate Keeper 的 render()}} 因此有一個講究的順序：先擦掉收集器的舊位置，再畫四條車道、核心、掉落物，最後才畫收集器。</p>
<p>先擦的理由是：擦掉的動作可能會挖掉車道的一部分，而下面的車道重繪會把它補回來。如果反過來，挖出來的洞會留在畫面上。</p>
""",
"demos": [],
"ex": [
 {"id":"c5a","type":"sort",
  "q":"把一次 band 合成的步驟排成正確順序。",
  "items":["<code>Gfx_BandBegin(w, h, 背景色)</code> — 在 RAM 開一塊並填滿背景",
           "<code>Gfx_BandRect(...)</code> — 把地面線、格線畫進緩衝區",
           "<code>Gfx_BandSprite(...)</code> — 把角色疊上去",
           "<code>Gfx_BandEnd(x, y)</code> — 一次寫到面板"],
  "hintFirst":"想一想：在往緩衝區畫任何東西之前，必須先有緩衝區、而且它要已經填好背景色。",
  "wrongFb":"順序錯的後果各不相同：先 <code>BandEnd</code> 會送出一塊還沒畫東西的區域；<code>BandBegin</code> 放在中間會把先前畫的內容用背景色覆蓋掉；sprite 畫在 <code>BandRect</code> 之前則會被後畫的矩形蓋住。",
  "fb":"<code>BandBegin</code> 必須先跑，因為它決定尺寸並填滿背景；中間想畫幾層就畫幾層，後畫的蓋住先畫的；<code>BandEnd</code> 是唯一真正碰到面板的一步，所以一定在最後。"},
 {"id":"c5b","type":"mc",
  "q":"障礙物每幀往左移 <code>step</code> 像素，送出的區塊寬度取 <code>sprite 寬 + step</code>。多出來的那幾個像素是什麼用途？",
  "opts":["預留給下一個障礙物","填成背景色，用來擦掉障礙物剛剛讓出來的尾巴",
          "避免 sprite 被螢幕邊緣截掉","讓移動看起來更平滑"],
  "ans":1,
  "fb":"障礙物往左走，右邊會露出一條剛剛還被它蓋住的區域。把區塊加寬 <code>step</code> 並填背景色，等於在<b>同一次寫入</b>裡順手把尾巴擦掉——不需要第二次寫入，也就不會閃。"},
]},

# ===================================================================== 6
{
"n": 6, "title": "聲音層",
"problem": "音效不能讓遊戲停下來等它播完。這一章講一個非阻塞的音符佇列怎麼寫、為什麼每一幀都要呼叫它一次，以及為什麼遊戲裡的等待要用 App_Wait 而不是 delay_ms。",
"files": [SC, SH, UI],
"focus": (SC, "void Sound_Update(void)"),
"flowName": "旋律怎麼非阻塞地走完",
"flowHint": "Sound_Update() 每幀被呼叫一次，只在時間到的時候做事。",
"flow": {"w": 620, "h": 500, "nodes": [
  {"id":"sfx","x":210,"y":10,"w":200,"h":46,"cls":"ev","t":"SFX_Jump()","sub":"遊戲呼叫，立刻回來",
   "tgt":(SC,"void SFX_Jump(void)   { PLAY(MEL_JUMP);   }"),
   "cap":"八個現成音效都是 <code>Sound_Melody(表, 長度)</code> 的包裝。"},
  {"id":"mel","x":210,"y":86,"w":200,"h":52,"t":"Sound_Melody(notes, n)","sub":"記下表、播第一個音",
   "tgt":(SC,"void Sound_Melody(const Note *notes, uint8_t count)"),
   "cap":"設定 <code>s_noteEnd = msTick + 這個音的長度</code>，然後回傳。"},
  {"id":"tone","x":420,"y":160,"w":190,"h":52,"cls":"hw","t":"tone_on(freq)","sub":"算 PSC/ARR，啟動 TIM3",
   "tgt":(SC,"static void tone_on(uint16_t freq)"),
   "cap":"硬體自己產生方波，CPU 不用管。第 11 章補細節。"},
  {"id":"upd","x":210,"y":174,"w":190,"h":52,"cls":"dec","t":"Sound_Update()","sub":"每一幀呼叫一次",
   "tgt":(SC,"void Sound_Update(void)"),
   "cap":"整個佇列的引擎。不呼叫它，第一個音就會一直響下去。"},
  {"id":"chk","x":205,"y":252,"w":200,"h":52,"cls":"dec","t":"時間到了嗎","sub":"(int32_t)(msTick - end) >= 0",
   "tgt":(SC,"    if ((int32_t)(msTick - s_noteEnd) < 0) return;   /* still playing */"),
   "cap":"用相減再轉號誌，第 7 章會講為什麼這樣能扛住溢位。"},
  {"id":"nxt","x":40,"y":340,"w":180,"h":50,"t":"還有下一個音","sub":"tone_on 下一個，更新 end",
   "cap":"表往前走一格。"},
  {"id":"stp","x":400,"y":340,"w":180,"h":50,"t":"表播完了","sub":"Sound_Stop()，關掉腳位",
   "tgt":(SC,"void Sound_Stop(void)"),
   "cap":"把 PA6 從計時器收回來並拉低，確保安靜。"},
  {"id":"wait","x":205,"y":430,"w":200,"h":54,"cls":"io","t":"App_Wait(ms)","sub":"會繼續呼叫 Sound_Update 的等待",
   "tgt":(UI,"void App_Wait(uint32_t ms)"),
   "cap":"遊戲裡需要停一下時用它，不要用 delay_ms。"},
 ], "edges":[
  {"a":"sfx","b":"mel"},{"a":"mel","b":"upd"},{"a":"mel","b":"tone","side":"r"},
  {"a":"upd","b":"chk"},{"a":"chk","b":"nxt","lab":"是"},{"a":"chk","b":"stp","lab":"是，但沒了"},
  {"a":"nxt","b":"upd","side":"back","lab":"下一幀"},
  {"a":"stp","b":"wait"},
 ]},
"html": """
<h3>旋律是一張表</h3>
<p>{{J:App/sound.h::} Note;::Note 結構}}只有兩個欄位：頻率和長度。一段旋律就是一個陣列：</p>
<pre class="note" style="font-family:var(--sm);font-size:12.5px;white-space:pre">static const Note MEL_JUMP[]   = { {NOTE_G5, 40}, {NOTE_C6, 55} };
static const Note MEL_HIT[]    = { {NOTE_G4, 70}, {NOTE_E4, 70}, {NOTE_C4, 160} };
static const Note MEL_START[]  = { {NOTE_C5, 90}, {NOTE_REST, 40}, {NOTE_C5, 90},
                                   {NOTE_REST, 40}, {NOTE_C6, 180} };</pre>
<p>八個 <code>SFX_*</code> 函式都是同一個巨集的包裝，{{J:App/sound.c::static const Note MEL_JUMP[]::表都在 sound.c 下半}}。<code>NOTE_REST</code> 是 0，<code>tone_on()</code> 收到 0 就靜音——休止符不需要特別處理。</p>

<h3>為什麼每一幀都要呼叫 Sound_Update()</h3>
<p><code>Sound_Melody()</code> 只做兩件事：播第一個音、記下它該在什麼時候結束，然後<b>立刻回傳</b>。它不等。</p>
<p>所以一定要有人在後面推進這張表，那就是 {{J:App/sound.c::void Sound_Update(void)::Sound_Update()}}。它每幀被呼叫一次，絕大多數時候什麼都不做（時間還沒到就直接 return）；時間到了才換下一個音或關掉。</p>
<p>忘記呼叫的後果很好認：<b>第一個音會一直響下去</b>，因為沒有人去把它關掉。這是新增畫面時最容易漏的一行。</p>

<h3>App_Wait 和 delay_ms 的差別</h3>
<p>{{J:App/ui.c::void App_Wait(uint32_t ms)::App_Wait()}} 只有三行，而它存在的理由就是上面那個問題：</p>
<pre class="note" style="font-family:var(--sm);font-size:12.5px;white-space:pre">void App_Wait(uint32_t ms)
{
    uint32_t until = msTick + ms;
    while ((int32_t)(msTick - until) &lt; 0) Sound_Update();
}</pre>
<p>它一樣是阻塞的——遊戲在這段時間裡真的停住了。差別是它在等的時候<b>繼續推進音效引擎</b>。</p>
<p>具體場景：教學關答對時顯示「GOOD!」並停 700 毫秒。<code>SFX_Good()</code> 是一段四個音的旋律。如果用 <code>delay_ms(700)</code>，第一個音會卡住整整 700 毫秒然後突然跳到結束；用 <code>App_Wait(700)</code>，四個音正常播完。</p>
<div class="note"><b>什麼時候用哪個：</b>畫面切換、開機、結束畫面這種「本來就不需要反應」的地方用 <code>App_Wait</code>。<b>遊戲主迴圈裡兩個都不要用</b>——那裡要用第 7 章的固定步長寫法。</div>

<h3>硬體被包在最下面</h3>
<p>整個聲音層只有 {{J:App/sound.c::static void tone_on(uint16_t freq)::tone_on()}} 碰硬體。上面的旋律佇列完全是邏輯，連「頻率」以外的硬體概念都不知道。</p>
<p>這個分層有一個實際回報：診斷蜂鳴器不響的時候，可以加一個 {{J:App/sound.c::void Sound_DC(uint8_t on)::Sound_DC()}} 繞過整個計時器、直接把腳位拉高——因為呼叫它的人只需要「發出聲音」這個語意，不在乎底下怎麼做。第 11 章會用到它。<span class="deepslot" data-k="tim" data-l="計時器與 PWM 的硬體細節"></span></p>
""",
"demos": [],
"ex": [
 {"id":"c6a","type":"mc",
  "q":"你加了一個新畫面，忘記在迴圈裡呼叫 <code>Sound_Update()</code>。症狀是什麼？",
  "opts":["完全沒有聲音","第一個音一直響個不停",
          "聲音變成正確音高的一半","程式當在 Sound_Melody 裡"],
  "ans":1,
  "fb":"<code>Sound_Melody()</code> 已經把第一個音打開了，而關掉它和換下一個音都是 <code>Sound_Update()</code> 的工作。沒有人推進，那個音就沒有終點。"},
 {"id":"c6b","type":"mc",
  "q":"在遊戲主迴圈裡用 <code>App_Wait(200)</code> 做一個短暫停頓，會有什麼問題？",
  "opts":["沒問題，這就是它的用途","音效會斷掉",
          "它仍然是阻塞的：那 200 毫秒裡不讀輸入、不更新世界、不繪製，玩家的按鍵會消失",
          "會讓 msTick 停止累加"],
  "ans":2,
  "fb":"<code>App_Wait</code> 只解決「音效不要卡住」，沒有解決「遊戲不要停住」。遊戲迴圈裡需要延遲的話，正確做法是記一個截止時刻放進狀態、每幀檢查它有沒有到——Gate Keeper 的開火冷卻就是這樣做的。"},
]},
]
