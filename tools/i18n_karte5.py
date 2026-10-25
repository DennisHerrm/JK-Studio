# -*- coding: utf-8 -*-
"""Oberflaechentexte des Karten-Editors, Stufe 5 (Optimierer und Grenzen),
fuer tools/gen_i18n.py.

NRC hat keinen Optimierer - die englische Spalte ist darum kein NRC-Wortlaut,
sondern die Fachbegriffe der Q3/JKA-Mapper (brush, patch, lightmap, ...), wie
sie mit "Begriffe wie NetRadiant" erscheinen.
"""

# (Kennung, English, Deutsch, 中文, 日本語)
KARTE5 = [
    # --- Arten der Grenzen ---------------------------------------------------
    ("KeGrArtBsp", "BSP lump", "BSP-Lump", "BSP 数据块", "BSP ランプ"),
    ("KeGrArtLaufzeit", "Runtime", "Laufzeit", "运行时", "実行時"),
    ("KeGrArtRenderer", "Renderer", "Renderer", "渲染器", "レンダラー"),
    ("KeGrArtKollision", "Collision", "Kollision", "碰撞", "衝突判定"),
    ("KeGrArtSpawn", "Spawn", "Spawn", "生成", "スポーン"),
    ("KeGrArtCompiler", "Compiler (q3map2)", "Compiler (q3map2)", "编译器 (q3map2)", "コンパイラ (q3map2)"),
    # --- Grenzen: BSP ----------------------------------------------------------
    ("KeGrBspEntitaeten", "Entities in the BSP", "Entities in der BSP", "BSP 中的实体", "BSP 内のエンティティ"),
    ("KeGrBspEnttext", "Entity text (bytes)", "Entity-Text (Bytes)", "实体文本(字节)", "エンティティ文字列(バイト)"),
    ("KeGrBspShader", "Shaders in the BSP", "Shader in der BSP", "BSP 中的着色器", "BSP 内のシェーダー"),
    ("KeGrBspModelle", "Models in the BSP", "Modelle in der BSP", "BSP 中的模型", "BSP 内のモデル"),
    ("KeGrBspPinsel", "Brushes", "Pinsel", "笔刷", "ブラシ"),
    ("KeGrBspPinselseiten", "Brush sides", "Pinselseiten", "笔刷面", "ブラシ面"),
    ("KeGrBspEbenen", "Planes", "Ebenen", "平面", "平面"),
    ("KeGrBspKnoten", "BSP nodes", "BSP-Knoten", "BSP 节点", "BSP ノード"),
    ("KeGrBspBlaetter", "Leafs", "Blätter", "叶节点", "リーフ"),
    ("KeGrBspBlattflaechen", "Leaf surfaces", "Blattflächen", "叶表面", "リーフ面"),
    ("KeGrBspBlattpinsel", "Leaf brushes", "Blattpinsel", "叶笔刷", "リーフブラシ"),
    ("KeGrBspFlaechen", "Draw surfaces", "Zeichenflächen", "绘制表面", "描画サーフェス"),
    ("KeGrBspPunkte", "Draw vertices", "Zeichenpunkte", "绘制顶点", "描画頂点"),
    ("KeGrBspIndizes", "Draw indexes", "Zeichenindizes", "绘制索引", "描画インデックス"),
    ("KeGrBspNebel", "Fogs in the BSP", "Nebel in der BSP", "BSP 中的雾", "BSP 内のフォグ"),
    ("KeGrBspLicht", "Lightmap data (bytes)", "Lightmap-Daten (Bytes)", "光照贴图数据(字节)", "ライトマップデータ(バイト)"),
    ("KeGrBspSicht", "Visibility data (bytes)", "Sichtdaten (Bytes)", "可见性数据(字节)", "可視性データ(バイト)"),
    ("KeGrBspPortale", "Portals", "Portale", "门户", "ポータル"),
    ("KeGrBspLichtgitter", "Light grid points", "Lichtgitter-Punkte", "光照网格点", "ライトグリッドの点"),
    ("KeGrBspLichtgitterfeld", "Light grid cells", "Lichtgitter-Zellen", "光照网格单元", "ライトグリッドのセル"),
    ("KeGrBspAreas", "Areas", "Areas", "区域", "エリア"),
    # --- Grenzen: Renderer -------------------------------------------------------
    ("KeGrRenFlaechenpunkte", "Vertices per surface", "Punkte je Fläche", "每个表面的顶点", "サーフェスごとの頂点"),
    ("KeGrRenFlaechenindizes", "Indexes per surface", "Indizes je Fläche", "每个表面的索引", "サーフェスごとのインデックス"),
    ("KeGrRenLightmaps", "Lightmaps", "Lightmaps", "光照贴图", "ライトマップ"),
    ("KeGrRenNebel", "Fogs (sort bits)", "Nebel (Sortierbits)", "雾(排序位)", "フォグ(ソートビット)"),
    ("KeGrRenPatchgroesse", "Patch size (control points)", "Patch-Größe (Kontrollpunkte)", "曲面片大小(控制点)", "パッチサイズ(制御点)"),
    ("KeGrRenDrawsurfs", "Surfaces per view (estimate)", "Flächen je Ansicht (Schätzung)", "每个视图的表面(估计)", "ビューごとのサーフェス(推定)"),
    ("KeGrRenShader", "Shaders loaded", "Geladene Shader", "已加载的着色器", "読み込まれたシェーダー"),
    ("KeGrRenSkins", "Skins loaded", "Geladene Skins", "已加载的皮肤", "読み込まれたスキン"),
    # --- Grenzen: Kollision --------------------------------------------------------
    ("KeGrKolSubmodelle", "Brush models (incl. world)", "Brush-Modelle (mit Welt)", "笔刷模型(含世界)", "ブラシモデル(ワールド含む)"),
    ("KeGrKolPatchebenen", "Collision planes per patch", "Kollisionsebenen je Patch", "每个曲面片的碰撞平面", "パッチごとの衝突平面"),
    ("KeGrKolPatchfacetten", "Collision facets per patch", "Kollisionsfacetten je Patch", "每个曲面片的碰撞面片", "パッチごとの衝突ファセット"),
    ("KeGrKolPatchpunkte", "Control points per patch", "Kontrollpunkte je Patch", "每个曲面片的控制点", "パッチごとの制御点"),
    ("KeGrKolWeltgrenze", "World size (largest coordinate)", "Weltgröße (größte Koordinate)", "世界大小(最大坐标)", "ワールドの大きさ(最大座標)"),
    # --- Grenzen: Laufzeit -----------------------------------------------------------
    ("KeGrLzEntitaeten", "Entities after spawn", "Entities nach dem Spawn", "生成后的实体", "スポーン後のエンティティ"),
    ("KeGrLzModelle", "Models (configstrings)", "Modelle (Configstrings)", "模型(配置字符串)", "モデル(コンフィグ文字列)"),
    ("KeGrLzKlaenge", "Sounds (configstrings)", "Klänge (Configstrings)", "声音(配置字符串)", "サウンド(コンフィグ文字列)"),
    ("KeGrLzEffekte", "Effects (configstrings)", "Effekte (Configstrings)", "特效(配置字符串)", "エフェクト(コンフィグ文字列)"),
    ("KeGrLzWelteffekte", "World effects (rain, snow, wind)", "Welteffekte (Regen, Schnee, Wind)", "世界特效(雨、雪、风)", "ワールドエフェクト(雨・雪・風)"),
    ("KeGrLzGamestate", "Configstring text (lower bound)", "Configstring-Text (Untergrenze)", "配置字符串文本(下限)", "コンフィグ文字列(下限)"),
    ("KeGrLzRoffs", "ROFF files", "ROFF-Dateien", "ROFF 文件", "ROFF ファイル"),
    ("KeGrLzNpcModelle", "NPC models per level", "NPC-Modelle je Level", "每关的 NPC 模型", "レベルごとの NPC モデル"),
    ("KeGrLzSubBsp", "Sub-BSPs (misc_bsp)", "Unter-BSPs (misc_bsp)", "子 BSP (misc_bsp)", "サブ BSP (misc_bsp)"),
    # --- Grenzen: Spawn --------------------------------------------------------------
    ("KeGrSpVariablen", "Keys per entity", "Schlüssel je Entity", "每个实体的键", "エンティティごとのキー"),
    ("KeGrSpZeichen", "Key text per entity", "Schlüsseltext je Entity", "每个实体的键文本", "エンティティごとのキー文字列"),
    # --- Grenzen: Compiler -------------------------------------------------------------
    ("KeGrCoLichtschalter", "Switchable light styles", "Schaltbare Lichtstile", "可切换的光照样式", "切り替え可能なライトスタイル"),
    ("KeGrCoSicht", "Visibility data (q3map2)", "Sichtdaten (q3map2)", "可见性数据 (q3map2)", "可視性データ (q3map2)"),
]
