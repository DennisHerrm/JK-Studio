# -*- coding: utf-8 -*-
"""Oberflaechentexte des Karten-Editors, Stufe 4 (fertige Karten, Originallicht,
Plugins), fuer tools/gen_i18n.py.

Englische Spalte bei NRC-Beschriftungen wortgleich NRC (ohne '&'); bei den
eigenen Zusaetzen die Begriffe des Programms.
"""

# (Kennung, English, Deutsch, 中文, 日本語)
KARTE4 = [
    # --- Punkt 1: fertige Karten oeffnen (File > Open .bsp...) --------------
    ("KeLOpenBsp", "Open .bsp...", ".bsp öffnen...", "打开 .bsp...", ".bsp を開く..."),
    ("KeBspTitel", "Open compiled map", "Fertige Karte öffnen", "打开已编译的地图", "コンパイル済みマップを開く"),
    ("KeBspFilter", "Filter", "Filter", "筛选", "フィルター"),
    ("KeBspKarte", "Map", "Karte", "地图", "マップ"),
    ("KeBspQuelle", "From", "Aus", "来源", "場所"),
    ("KeBspDatei", "From file...", "Aus Datei...", "从文件...", "ファイルから..."),
    ("KeBspOeffnen", "Open", "Öffnen", "打开", "開く"),
    ("KeBspHinweis",
     "The map is decompiled with q3map2 -convert into a work folder - never into the game folder.",
     "Die Karte wird mit q3map2 -convert in einen Arbeitsordner umgewandelt - nie in den Spielordner.",
     "地图用 q3map2 -convert 反编译到工作文件夹中——绝不写入游戏文件夹。",
     "マップは q3map2 -convert で作業フォルダーに逆コンパイルされます。ゲームフォルダーには書き込みません。"),
    ("KeBspKeine", "No compiled maps in the game folders.", "Keine fertigen Karten in den Spielordnern.",
     "游戏文件夹中没有已编译的地图。", "ゲームフォルダーにコンパイル済みマップがありません。"),
    # --- Punkt 2: Originallicht behalten (Build > Update entities only) -----
    ("KeBmNurEntities", "Update entities only (keep original light)", "Nur Entities aktualisieren (Originallicht behalten)",
     "仅更新实体(保留原始光照)", "エンティティのみ更新 (元のライトを保持)"),
    ("KeBmNurEntitiesTip",
     "q3map2 -onlyents on a copy of the opened .bsp: geometry, lightmaps and light grid stay byte for byte as they are.",
     "q3map2 -onlyents auf eine Kopie der geöffneten .bsp: Geometrie, Lightmaps und Lichtgitter bleiben Byte für Byte, wie sie sind.",
     "对打开的 .bsp 的副本运行 q3map2 -onlyents:几何体、光照贴图和光照网格逐字节保持不变。",
     "開いた .bsp のコピーに q3map2 -onlyents を実行します。ジオメトリ、ライトマップ、ライトグリッドはバイト単位でそのまま残ります。"),
    ("KeBmNurEntitiesAus", "Only available after File > Open .bsp...", "Nur nach Datei > .bsp öffnen...",
     "仅在“文件 > 打开 .bsp...”之后可用", "「ファイル > .bsp を開く...」の後でのみ使えます"),
    ("KeBmGeometrieTitel", "Update entities only", "Nur Entities aktualisieren", "仅更新实体", "エンティティのみ更新"),
    ("KeBmGeometrie",
     "Brushes, patches or baked entities (misc_model) have changed since the map was opened.\n\n"
     "\"Update entities only\" keeps the original geometry and light of the .bsp, so these changes would be lost. "
     "Build the map again with BSP, VIS and LIGHT (Build menu) to include them - the light is then calculated anew.",
     "Seit dem Öffnen wurden Pinsel, Patches oder eingebackene Entities (misc_model) geändert.\n\n"
     "„Nur Entities aktualisieren“ behält Geometrie und Licht der .bsp, diese Änderungen gingen also verloren. "
     "Baue die Karte mit BSP, VIS und LIGHT neu (Menü Kompilieren), um sie aufzunehmen - das Licht wird dann neu berechnet.",
     "自打开地图以来,画刷、曲面片或烘焙进几何体的实体(misc_model)已被修改。\n\n"
     "“仅更新实体”保留 .bsp 的原始几何体和光照,因此这些修改会丢失。"
     "请用 BSP、VIS 和 LIGHT 重新编译地图(“编译”菜单)以包含它们——光照将重新计算。",
     "マップを開いてから、ブラシ、パッチ、またはジオメトリに焼き込まれるエンティティ (misc_model) が変更されました。\n\n"
     "「エンティティのみ更新」は .bsp の元のジオメトリとライトを保持するため、これらの変更は失われます。"
     "変更を含めるには、BSP、VIS、LIGHT でマップを再ビルドしてください (ビルドメニュー)。ライトは再計算されます。"),
    ("KeBmNurEntitiesFertig", "Entities updated: %s - every other lump byte for byte as in the original (%d of %d).",
     "Entities aktualisiert: %s - jeder andere Lump Byte für Byte wie im Original (%d von %d).",
     "实体已更新:%s——其他每个数据块都与原始文件逐字节相同(%d / %d)。",
     "エンティティを更新しました: %s - 他のすべてのランプは元のファイルとバイト単位で同じです (%d / %d)。"),
]
