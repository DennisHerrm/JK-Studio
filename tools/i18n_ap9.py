# -*- coding: utf-8 -*-
"""Oberflaechentexte von AP9 (MBENTWURF Teil B): das rechte Feld des Bereichs Painting nach dem freigegebenen Entwurf -
Gruppen Ziel, Werkzeuge, Pinsel, Ebenen, Fuer das Spiel (Baustein gui/ui_rollout.h), Tasten [ und ] fuer die Pinselgroesse -
fuer tools/gen_i18n.py und die weiteren Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
Die uebrigen Texte des Bereichs stehen in tools/i18n_paint.py (Modell-Block 1c).
"""

ZEILEN = [
    # --- Gruppe Ziel (Entwurf: Shader | Bild) ---
    ("PtGrZiel", "Target", "Ziel", "目标", "ターゲット", "Destino", "Cíl"),
    ("PtZielHint", "What you paint on: the image behind the shader of the surfaces. Size and format stay as in the original.",
     "Worauf du malst: das Bild hinter dem Shader der Flächen. Größe und Format bleiben wie im Original.",
     "绘制的对象: 表面着色器背后的图像。大小和格式与原图保持一致。", "塗る対象: サーフェスのシェーダーの背後にある画像。サイズと形式はオリジナルのままです。",
     "Sobre qué pintas: la imagen detrás del shader de las superficies. El tamaño y el formato se mantienen como en el original.",
     "Na co malujete: obrázek za shaderem ploch. Velikost a formát zůstávají jako v originálu."),
    ("PtZielShader", "Shader", "Shader", "着色器", "シェーダー", "Shader", "Shader"),
    ("PtZielStufe", "Stage", "Stufe", "阶段", "ステージ", "Etapa", "Fáze"),
    ("PtZielBildWert", "%s · %d × %d (stays)", "%s · %d × %d (bleibt so)", "%s · %d × %d (保持不变)", "%s · %d × %d (そのまま)", "%s · %d × %d (se mantiene)",
     "%s · %d × %d (zůstává)"),
    ("PtZielFormat", "Format", "Format", "格式", "形式", "Formato", "Formát"),
    ("PtZielFormatWert", "%s, %d bit", "%s, %d Bit", "%s, %d 位", "%s、%d ビット", "%s, %d bits", "%s, %d bitů"),
    # --- Gruppe Werkzeuge ---
    ("PtWerkzeugeHint", "The painting tools - what Viewport Canvas is in 3ds Max and Texture Paint in Blender. They work in the view and in the 2D image.",
     "Die Malwerkzeuge - was in 3ds Max Viewport Canvas und in Blender Texture Paint ist. Sie arbeiten in der Ansicht und im 2D-Bild.",
     "绘制工具 - 相当于 3ds Max 的 Viewport Canvas 和 Blender 的 Texture Paint。可在视图和 2D 图像中使用。",
     "ペイントツール - 3ds Max の Viewport Canvas、Blender の Texture Paint にあたります。ビューと 2D 画像で使えます。",
     "Las herramientas de pintura: lo que en 3ds Max es Viewport Canvas y en Blender Texture Paint. Funcionan en la vista y en la imagen 2D.",
     "Malířské nástroje - to, co je ve 3ds Max Viewport Canvas a v Blenderu Texture Paint. Fungují v pohledu i ve 2D obrázku."),
    # --- Gruppe Pinsel: Kurzinfo je Bedienmodus (Entwurf), Beschriftungen der Regler ---
    ("PtInfoMax", "[ ] Size", "[ ] Größe", "[ ] 大小", "[ ] サイズ", "[ ] Tamaño", "[ ] Velikost"),
    ("PtInfoBlender", "F Radius · Shift+F Strength", "F Radius · Umschalt+F Stärke", "F 半径 · Shift+F 强度", "F 半径 · Shift+F 強さ", "F Radio · Mayús+F Fuerza",
     "F Poloměr · Shift+F Síla"),
    ("PtPinselHint", "Colour and shape of the brush - they apply to every tool.", "Farbe und Form des Pinsels - sie gelten für jedes Werkzeug.", "画笔的颜色和形状 - 适用于每种工具。",
     "ブラシの色と形 - すべてのツールに適用されます。", "Color y forma del pincel: valen para todas las herramientas.", "Barva a tvar štětce - platí pro každý nástroj."),
    ("PtFarbe", "Colour", "Farbe", "颜色", "カラー", "Color", "Barva"),
    ("PtRadius", "Radius", "Radius", "半径", "半径", "Radio", "Poloměr"),
    ("PtStaerke", "Strength", "Stärke", "强度", "強さ", "Fuerza", "Síla"),
    ("PtHaerte", "Hardness", "Härte", "硬度", "硬さ", "Dureza", "Tvrdost"),
    ("PtAbstand", "Spacing", "Abstand", "间距", "間隔", "Espaciado", "Rozestup"),
    ("PtToleranz", "Tolerance", "Toleranz", "容差", "許容値", "Tolerancia", "Tolerance"),
    ("PtRadiusTasten", "[ and ]: one step smaller or larger.", "[ und ]: einen Schritt kleiner oder größer.", "[ 和 ]: 缩小或放大一级。", "[ と ]: 1 段階小さく / 大きく。",
     "[ y ]: un paso más pequeño o más grande.", "[ a ]: o krok menší nebo větší."),
    ("PtErweitert", "Advanced", "Erweitert", "高级", "詳細設定", "Avanzado", "Rozšířené"),
    ("PtAchse", "Axis", "Achse", "轴", "軸", "Eje", "Osa"),
    # --- Tasten [ und ] (Entwurf: wie 3ds Max "[ ] Groesse"; Blender-Handbuch Brush Settings: "The size can be decreased/increased using [ and ]") ---
    ("PtPinselKleiner", "Brush smaller", "Pinsel kleiner", "缩小画笔", "ブラシを小さく", "Pincel más pequeño", "Menší štětec"),
    ("PtPinselKleinerHint", "Makes the brush radius one step smaller (to nine tenths). Hold the key to go on.",
     "Macht den Pinselradius einen Schritt kleiner (auf neun Zehntel). Taste halten: weiter.", "将画笔半径缩小一级 (变为十分之九)。按住按键可连续缩小。",
     "ブラシ半径を 1 段階小さくします (10 分の 9 に)。キーを押し続けると連続します。", "Reduce el radio del pincel un paso (a nueve décimos). Mantén la tecla para seguir.",
     "Zmenší poloměr štětce o krok (na devět desetin). Držením klávesy pokračujete."),
    ("PtPinselGroesser", "Brush larger", "Pinsel größer", "放大画笔", "ブラシを大きく", "Pincel más grande", "Větší štětec"),
    ("PtPinselGroesserHint", "Makes the brush radius one step larger (by a ninth). Hold the key to go on.",
     "Macht den Pinselradius einen Schritt größer (um ein Neuntel). Taste halten: weiter.", "将画笔半径放大一级 (增加九分之一)。按住按键可连续放大。",
     "ブラシ半径を 1 段階大きくします (9 分の 1 増)。キーを押し続けると連続します。", "Aumenta el radio del pincel un paso (un noveno más). Mantén la tecla para seguir.",
     "Zvětší poloměr štětce o krok (o devítinu). Držením klávesy pokračujete."),
    # --- Gruppe Ebenen ---
    ("PtEbenenHint", "The layers of this image, the topmost first. Click a row to paint on that layer.",
     "Die Ebenen dieses Bilds, die oberste zuerst. Klick auf eine Zeile: auf dieser Ebene malen.", "此图像的图层, 最上层在前。单击一行即可在该图层上绘制。",
     "この画像のレイヤー (上のレイヤーが先頭)。行をクリックするとそのレイヤーに塗ります。", "Las capas de esta imagen, la superior primero. Haz clic en una fila para pintar en esa capa.",
     "Vrstvy tohoto obrázku, nejvyšší první. Kliknutím na řádek malujete na tuto vrstvu."),
    ("PtDeckkraft", "Opacity", "Deckkraft", "不透明度", "不透明度", "Opacidad", "Krytí"),
    ("PtEbenenTipp", "The original always stays the bottom layer and is never changed. The game gets the result of all visible layers, in the size and format of the original.",
     "Das Original bleibt immer die unterste Ebene und wird nie verändert. Das Spiel bekommt das Ergebnis aller sichtbaren Ebenen, in Größe und Format des Originals.",
     "原图始终是最底层, 永远不会被更改。游戏得到的是所有可见图层的合成结果, 大小和格式与原图相同。",
     "オリジナルは常に最下層のレイヤーで、変更されることはありません。ゲームには表示中の全レイヤーの結果が、オリジナルのサイズと形式で渡されます。",
     "El original es siempre la capa inferior y nunca se modifica. El juego recibe el resultado de todas las capas visibles, con el tamaño y formato del original.",
     "Originál zůstává vždy spodní vrstvou a nikdy se nemění. Hra dostane výsledek všech viditelných vrstev ve velikosti a formátu originálu."),
    # --- Gruppe Fuer das Spiel ---
    ("PtAusgabeHint", "Writes the painted images for the game - into a mod folder, never into the game folder.",
     "Schreibt die bemalten Bilder für das Spiel - in einen Mod-Ordner, nie in den Spielordner.", "为游戏写出已绘制的图像 - 写入模组文件夹, 绝不写入游戏文件夹。",
     "塗った画像をゲーム用に書き出します - MOD フォルダーへ。ゲームフォルダーには書き込みません。",
     "Escribe las imágenes pintadas para el juego: en una carpeta de mod, nunca en la carpeta del juego.",
     "Zapíše namalované obrázky pro hru - do složky modu, nikdy do složky hry."),
    ("PtBemalt", "Painted images", "Bemalte Bilder", "已绘制图像", "塗った画像", "Imágenes pintadas", "Namalované obrázky"),
    ("PtInfoBemalt", "%d painted", "%d bemalt", "已绘制 %d 张", "塗装済み %d 枚", "%d {pintada|pintadas}", "%d {namalovaný|namalované|namalovaných}"),
    ("PtOrdnerWaehlen", "Choose folder", "Ordner wählen", "选择文件夹", "フォルダーを選択", "Elegir carpeta", "Zvolit složku"),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
