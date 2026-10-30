# -*- coding: utf-8 -*-
"""Oberflaechentexte des Auftrags vis1 (schnelleres Kompilieren), fuer tools/gen_i18n.py.

Build-Menue: dasselbe VIS in jedem Bau (-bhedvisexact).
vis4: GPU-Backen Auto mit dem dritten Kandidaten "Grafikkarte mit Nachproben" (bhed/gpuwahl.h).
"""

# (Kennung, English, Deutsch, 中文, 日本語)
VIS1 = [
    ('KeVisExakt',
     "Same VIS in every build (not in NetRadiant-Custom)",
     "Dasselbe VIS in jedem Bau (nicht in NetRadiant-Custom)",
     "每次编译得到相同的 VIS(NetRadiant-Custom 中没有)",
     "毎回のビルドで同じ VIS(NetRadiant-Custom にはありません)"),
    ('KeVisExaktTip',
     "Every -vis step gets -bhedvisexact. With several threads a full VIS can come out slightly different from build to build - as with NetRadiant-Custom's q3map2, a portal uses whatever other portals happen to be finished; every result is valid. With this switch bhed_q3map2 computes with all threads exactly what one thread computes, byte for byte. Needed for reproducible builds: the same .map gives the same .bsp (comparing builds, checksums, a team building the same map). It waits where a portal needs an unfinished one - on closed maps about as fast, on open maps much slower (duel_mos_espa: 5 instead of 1.5 minutes). Off: the faster way.",
     "Jeder -vis-Schritt bekommt -bhedvisexact. Mit mehreren Threads kann ein volles VIS von Bau zu Bau leicht anders ausfallen - wie beim q3map2 von NetRadiant-Custom nimmt ein Portal die anderen Portale, die gerade fertig sind; jedes Ergebnis ist gültig. Mit diesem Schalter rechnet bhed_q3map2 mit allen Threads genau das, was ein Thread rechnet, Byte für Byte. Nötig für reproduzierbare Bauten: dieselbe .map gibt dieselbe .bsp (Bauten vergleichen, Prüfsummen, ein Team baut dieselbe Karte). Er wartet, wo ein Portal ein noch nicht fertiges braucht - auf geschlossenen Karten etwa gleich schnell, auf offenen deutlich langsamer (duel_mos_espa: 5 statt 1,5 Minuten). Aus: der schnellere Weg.",
     "每个 -vis 步骤都会加上 -bhedvisexact。使用多个线程时,完整 VIS 的结果在每次编译之间可能略有不同——与 NetRadiant-Custom 的 q3map2 一样,一个门户会使用当时恰好已完成的其他门户;每种结果都有效。打开此开关后,bhed_q3map2 用全部线程精确计算出与单线程相同的结果,逐字节一致。可重复编译需要它:同一个 .map 得到同一个 .bsp(比较编译结果、校验和、团队编译同一张地图)。当某个门户需要一个尚未完成的门户时它会等待——在封闭的地图上速度相当,在开阔的地图上会慢很多(duel_mos_espa:5 分钟而不是 1.5 分钟)。关闭:更快的方式。",
     "各 -vis ステップに -bhedvisexact が付きます。複数のスレッドでは、完全な VIS の結果がビルドごとにわずかに異なることがあります。NetRadiant-Custom の q3map2 と同じく、ポータルはその時点で完了しているほかのポータルを使うためで、どの結果も有効です。このスイッチをオンにすると、bhed_q3map2 はすべてのスレッドで 1 スレッドとバイト単位で同じ結果を計算します。再現可能なビルドに必要です:同じ .map から同じ .bsp ができます(ビルドの比較、チェックサム、チームで同じマップをビルドする場合)。まだ完了していないポータルが必要なところでは待つため、閉じたマップではほぼ同じ速さ、開けたマップではかなり遅くなります(duel_mos_espa:1.5 分ではなく 5 分)。オフ:速い方法。"),
    # vis4: Auto mit dem dritten Kandidaten (bhed/gpuwahl.h, Regeln 3a/3b)
    ('KeGpuWahlGpuZ',
     "graphics card with random subsamples",
     "Grafikkarte mit Nachproben",
     "显卡(含随机子采样)",
     "グラフィックカード(ランダムなサブサンプル込み)"),
    ('KeGpuGrundLearnedZ',
     "fastest last time on this map (processor %s s, graphics card %s s, with random subsamples %s s)",
     "beim letzten Mal auf dieser Karte am schnellsten (Prozessor %s s, Grafikkarte %s s, mit Nachproben %s s)",
     "上次在此地图上最快(处理器 %s 秒,显卡 %s 秒,含随机子采样 %s 秒)",
     "このマップで前回いちばん速かった方(プロセッサー %s 秒、グラフィックカード %s 秒、ランダムなサブサンプル込み %s 秒)"),
    ('KeGpuGrundMeasureZ',
     "the graphics card was faster (%s s against %s s on the processor) - measuring it once with the random subsamples too",
     "die Grafikkarte war schneller (%s s gegen %s s auf dem Prozessor) - sie wird einmal auch mit den Nachproben gemessen",
     "显卡更快(%s 秒,处理器 %s 秒)——再测量一次显卡同时计算随机子采样",
     "グラフィックカードの方が速かったため(%s 秒、プロセッサーは %s 秒)、ランダムなサブサンプルも含めて一度計測します"),
]
