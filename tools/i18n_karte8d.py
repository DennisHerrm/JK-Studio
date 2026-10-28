# -*- coding: utf-8 -*-
"""Oberflaechentexte des Karten-Editors, Stufe 8d Punkt 1b (GPU bake: Auto), fuer tools/gen_i18n.py.

Build > "GPU bake" ist ein Untermenue mit Auto / Ein / Aus (Vorgabe Auto,
kGpuBackenVorgabe in include/bhed/karte/editordaten.h). Auto: bhed_q3map2 waehlt mit
-bhedgpubakeauto die schnellere Variante, die GPU nur bitgleich (bhed/karte/gpuwahl.h).
Die Gruende kommen als Kurzname aus der Ausgabezeile ("BehavEd GPU bake auto: ...").
"""

# (Kennung, English, Deutsch, 中文, 日本語)
KARTE8D = [
    ("KeGpuAuto", "Auto (the faster one, bit-identical)", "Auto (das Schnellere, bitgleich)",
     "自动(更快的一方,逐位相同)", "自動(速い方、ビット単位で同一)"),
    ("KeGpuEin", "On (graphics card)", "Ein (Grafikkarte)", "开(显卡)", "オン(グラフィックカード)"),
    ("KeGpuAus", "Off (processor)", "Aus (Prozessor)", "关(处理器)", "オフ(プロセッサー)"),
    ("KeGpuAutoTip",
     "bhed_q3map2 decides for every -light step: the graphics card only if it computes with double precision (then the .bsp is byte-identical) and is expected to be faster - large maps and bounce rounds; small maps and -samples 4 on large maps stay on the processor. It remembers how long the last processor and graphics card run of each map took and from then on takes the faster one. The console shows what was chosen and why. Also for the quick bake",
     "bhed_q3map2 entscheidet bei jedem -light-Schritt: die Grafikkarte nur, wenn sie mit double rechnet (dann ist die .bsp Byte für Byte gleich) und voraussichtlich schneller ist - große Karten und Bounce-Runden; kleine Karten und -samples 4 auf großen Karten bleiben auf dem Prozessor. Es merkt sich, wie lange der letzte Lauf jeder Karte auf dem Prozessor und auf der Grafikkarte gedauert hat, und nimmt danach das Schnellere. Die Konsole zeigt, was gewählt wurde und warum. Auch für das Schnell-Backen",
     "bhed_q3map2 在每个 -light 步骤中决定:只有当显卡支持双精度计算(此时 .bsp 逐字节相同)且预计更快时才使用显卡——大地图和反弹轮次;小地图以及大地图上的 -samples 4 留在处理器上。它会记住每张地图上次在处理器和显卡上运行所用的时间,之后选择更快的一方。控制台会显示选择了什么以及原因。快速烘焙也适用",
     "bhed_q3map2 は各 -light ステップで判断します。グラフィックカードを使うのは、倍精度で計算でき(その場合 .bsp はバイト単位で同一)、速いと見込まれる場合だけです(大きなマップやバウンス)。小さなマップや大きなマップでの -samples 4 はプロセッサーで計算します。各マップの前回のプロセッサーとグラフィックカードでの所要時間を覚え、以後は速い方を選びます。何を選んだか、その理由はコンソールに表示されます。クイックベイクにも使われます"),
    ("KeGpuWahl", "GPU bake (Auto): %s - %s", "GPU-Backen (Auto): %s - %s", "GPU 烘焙(自动):%s - %s", "GPU ベイク(自動):%s - %s"),
    ("KeGpuWahlGpu", "graphics card", "Grafikkarte", "显卡", "グラフィックカード"),
    ("KeGpuWahlCpu", "processor", "Prozessor", "处理器", "プロセッサー"),
    ("KeGpuGrundNogpu", "no usable graphics card", "keine nutzbare Grafikkarte", "没有可用的显卡", "使えるグラフィックカードがありません"),
    ("KeGpuGrundNodouble", "the graphics card has no double precision - its result would not be bit-identical",
     "die Grafikkarte kann kein double - ihr Ergebnis wäre nicht bitgleich",
     "显卡不支持双精度——其结果不会逐位相同",
     "グラフィックカードが倍精度に対応していません(結果がビット単位で同一になりません)"),
    ("KeGpuGrundLearned", "faster last time on this map (processor %s s, graphics card %s s)",
     "beim letzten Mal auf dieser Karte schneller (Prozessor %s s, Grafikkarte %s s)",
     "上次在此地图上更快(处理器 %s 秒,显卡 %s 秒)",
     "このマップで前回速かった方(プロセッサー %s 秒、グラフィックカード %s 秒)"),
    ("KeGpuGrundSmall", "little light work (%s super luxels) - the graphics card would gain almost nothing",
     "wenig Lichtarbeit (%s Super-Luxel) - die Grafikkarte gewänne fast nichts",
     "光照计算量小(%s 个超级光照像素)——显卡几乎没有优势",
     "光の計算が少ない(スーパールクセル %s 個)ため、グラフィックカードではほとんど速くなりません"),
    ("KeGpuGrundSamples", "-samples 4 or more on a large map is usually faster on the processor",
     "-samples 4 und mehr auf einer großen Karte ist meist auf dem Prozessor schneller",
     "大地图上 -samples 4 及以上通常在处理器上更快",
     "大きなマップでの -samples 4 以上は、たいていプロセッサーの方が速いです"),
    ("KeGpuGrundMeasure", "measuring the graphics card once (the processor took %s s)",
     "die Grafikkarte wird einmal gemessen (der Prozessor brauchte %s s)",
     "测量一次显卡(处理器用了 %s 秒)",
     "グラフィックカードを一度計測します(プロセッサーは %s 秒かかりました)"),
    ("KeGpuGrundLarge", "large map (%s super luxels) - the graphics card is expected to be faster",
     "große Karte (%s Super-Luxel) - die Grafikkarte ist voraussichtlich schneller",
     "大地图(%s 个超级光照像素)——预计显卡更快",
     "大きなマップ(スーパールクセル %s 個)なので、グラフィックカードの方が速いと見込まれます"),
]
