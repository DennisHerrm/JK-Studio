# -*- coding: utf-8 -*-
"""Oberflaechentexte des Karten-Editors, Stufe 8g (GPU-Backen), fuer tools/gen_i18n.py.

Build > "GPU bake": jeder -light-Schritt bekommt -bhedgpubake - die ganze
Lichtrechnung (Lightmaps, Bounce-Runden, Lichtgitter) auf der Grafikkarte,
Byte fuer Byte gleich der CPU. Eigene Funktion (nicht in NetRadiant-Custom).
Vorgabe aus (kGpuBackenVorgabe in include/bhed/karte/editordaten.h).
"""

# (Kennung, English, Deutsch, 中文, 日本語)
KARTE8G = [
    ("KeGpuMenue", "GPU bake", "GPU-Backen", "GPU 烘焙", "GPU ベイク"),
    ("KeGpuMenueTip",
     "Every -light step gets -bhedgpubake: bhed_q3map2 computes the whole light on the graphics card - lightmaps with supersampling and filter, every bounce round and the light grid - with the same calculations in the same order as the CPU, so the .bsp is byte-identical. Every 64th lightmap is computed on the CPU too and compared (the report follows the build). Without a suitable graphics card, or when the card fails, the CPU computes as always. Also used by the quick bake of the real-time light (not in NetRadiant-Custom)",
     "Jeder -light-Schritt bekommt -bhedgpubake: bhed_q3map2 rechnet das ganze Licht auf der Grafikkarte - Lightmaps mit Supersampling und Filter, jede Bounce-Runde und das Lichtgitter - mit denselben Rechnungen in derselben Reihenfolge wie die CPU, die .bsp ist also Byte für Byte gleich. Jede 64. Lightmap rechnet zusätzlich die CPU und vergleicht (der Bericht folgt nach dem Bau). Ohne passende Grafikkarte oder wenn sie ausfällt, rechnet die CPU wie immer. Gilt auch für das Schnell-Backen des Echtzeit-Lichts (nicht in NetRadiant-Custom)",
     "每个 -light 步骤都会加上 -bhedgpubake:bhed_q3map2 在显卡上计算全部光照——带超采样和滤波的光照贴图、每一轮反弹以及光照网格——计算与 CPU 相同、顺序相同,因此 .bsp 逐字节相同。每第 64 张光照贴图还会由 CPU 再算一次并比较(报告在编译后显示)。没有合适的显卡或显卡出错时,CPU 照常计算。实时光照的快速烘焙也会使用(NetRadiant-Custom 中没有)",
     "各 -light ステップに -bhedgpubake が付きます。bhed_q3map2 は光の計算をすべてグラフィックカードで行います(スーパーサンプリングとフィルター付きのライトマップ、すべてのバウンス、ライトグリッド)。計算も順序も CPU と同じなので、.bsp はバイト単位で同じです。64 枚に 1 枚のライトマップは CPU でも計算して比較します(レポートはビルドの後に表示)。適したグラフィックカードがない場合やカードが失敗した場合は、CPU が通常どおり計算します。リアルタイムライトのクイックベイクにも使われます(NetRadiant-Custom にはありません)"),
    ("KeGpuMenueTipBericht", "Last GPU bake: %s", "Letztes GPU-Backen: %s", "上次 GPU 烘焙:%s", "前回の GPU ベイク:%s"),
    ("KeGpuStatus", "GPU: %s", "GPU: %s", "GPU:%s", "GPU:%s"),
    ("KeGpuFsAdapter", "GPU bake on %s", "GPU-Backen auf %s", "在 %s 上进行 GPU 烘焙", "%s で GPU ベイク"),
    ("KeGpuBericht",
     "GPU bake on %s: %s %% of the %s luxels checked on the CPU identical (%s lightmaps checked, %s with a difference); %s lightmaps on the GPU, %s on the CPU; light grid %s points on the GPU",
     "GPU-Backen auf %s: %s %% der %s auf der CPU nachgerechneten Luxel gleich (%s Lightmaps nachgerechnet, %s mit Abweichung); %s Lightmaps auf der GPU, %s auf der CPU; Lichtgitter %s Punkte auf der GPU",
     "在 %s 上进行 GPU 烘焙:%s %% 相同(CPU 复核了 %s 个光照像素、%s 张光照贴图,其中 %s 张有差异);%s 张光照贴图在 GPU 上,%s 张在 CPU 上;光照网格 %s 个点在 GPU 上",
     "%s で GPU ベイク:一致 %s %%(CPU で再計算:ルクセル %s 個、ライトマップ %s 枚、うち差分あり %s 枚)。ライトマップは GPU で %s 枚、CPU で %s 枚。ライトグリッドは GPU で %s 点"),
    ("KeGpuKeineGpu", "GPU bake: no usable graphics card - the CPU computed everything",
     "GPU-Backen: keine nutzbare Grafikkarte - die CPU hat alles gerechnet",
     "GPU 烘焙:没有可用的显卡——全部由 CPU 计算",
     "GPU ベイク:使えるグラフィックカードがありません。すべて CPU で計算しました"),
]
