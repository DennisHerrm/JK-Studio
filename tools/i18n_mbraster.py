# -*- coding: utf-8 -*-
"""Oberflaechentexte von MBRASTER (rc612, 10.11.): das Boden-Raster des Model-Tabs wie 3ds Max (Home Grid) bzw. Blender
(Overlays > Guides: Grid, Floor, Axes X Y Z, Scale, Subdivisions), ab Werk an.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
Namen der Vorbilder: 3ds Max (Home Grid, Grid and Snap Settings: Grid Spacing, Major Lines every Nth Grid Line, Perspective
View Grid Extent), Blender (Overlays: Grid, Floor, Axes, Scale, Subdivisions).
"""

ZEILEN = [
    # --- 3ds Max: [+] > Raster (G) und die Rastereinstellungen ---
    ("MrRasterHint",
     "Home grid like 3ds Max (G, this view): thin lines at the grid spacing, darker major lines, the world axes in black - on the floor in the perspective view, in the view plane in the orthographic views. 3ds Max puts it at Z = 0; a JKA figure has its origin at the hips (feet at -24), so here it lies at the floor height under the feet.",
     "Home Grid wie 3ds Max (G, diese Ansicht): feine Linien im Rasterabstand, dunklere Hauptlinien, die Weltachsen schwarz - in der Perspektive auf dem Boden, in den Orthoansichten in der Bildebene. 3ds Max legt es auf Z = 0; eine JKA-Figur hat ihren Ursprung in der Hüfte (Füße bei -24), darum liegt es hier in Bodenhöhe unter den Füßen.",
     "如 3ds Max 的主栅格 (G, 此视图): 按栅格间距的细线、较深的主线、黑色的世界坐标轴——透视图中位于地面, 正交视图中位于视图平面。3ds Max 将其放在 Z = 0; JKA 角色的原点在髋部 (脚在 -24), 因此这里它位于脚下的地面高度。",
     "3ds Max と同じホームグリッド (G、このビュー): グリッド間隔の細い線、濃いメジャーライン、黒いワールド軸 - パースでは床の上、平行投影ビューではビュー平面に。3ds Max は Z = 0 に置きます。JKA のフィギュアは原点が腰にある (足は -24) ので、ここでは足の下の床の高さにあります。",
     "Rejilla de inicio como en 3ds Max (G, esta vista): líneas finas a la distancia de la rejilla, líneas principales más oscuras, los ejes del mundo en negro; en perspectiva sobre el suelo, en las vistas ortográficas en el plano de la vista. 3ds Max la pone en Z = 0; una figura de JKA tiene su origen en la cadera (pies en -24), por eso aquí queda a la altura del suelo bajo los pies.",
     "Domovská mřížka jako v 3ds Max (G, tento pohled): tenké čáry po rozteči mřížky, tmavší hlavní čáry, osy světa černě - v perspektivě na podlaze, v ortografických pohledech v rovině pohledu. 3ds Max ji dává na Z = 0; postava JKA má počátek v bocích (chodidla na -24), proto zde leží ve výšce podlahy pod chodidly."),
    ("MrRasterEinst", "Grid settings", "Rastereinstellungen", "栅格设置", "グリッド設定", "Ajustes de la rejilla", "Nastavení mřížky"),
    ("MrRasterEinstHint",
     "Spacing, major lines, size in the perspective view and floor height of the home grid (3ds Max: Grid and Snap Settings > Home Grid).",
     "Abstand, Hauptlinien, Größe in der Perspektive und Bodenhöhe des Home Grid (3ds Max: Raster- und Fangeinstellungen > Home Grid).",
     "主栅格的间距、主线、透视图中的大小和地面高度 (3ds Max: 栅格和捕捉设置 > 主栅格)。",
     "ホームグリッドの間隔、メジャーライン、パースでの大きさ、床の高さ (3ds Max: グリッドとスナップの設定 > ホームグリッド)。",
     "Separación, líneas principales, tamaño en perspectiva y altura del suelo de la rejilla de inicio (3ds Max: Grid and Snap Settings > Home Grid).",
     "Rozteč, hlavní čáry, velikost v perspektivě a výška podlahy domovské mřížky (3ds Max: Grid and Snap Settings > Home Grid)."),
    ("MrAbstand", "Grid spacing", "Rasterabstand", "栅格间距", "グリッド間隔", "Separación de la rejilla", "Rozteč mřížky"),
    ("MrAbstandHint",
     "Spacing of the grid lines in JKA units (3ds Max: Grid Spacing). Default 8, like the grid of the map editor.",
     "Abstand der Rasterlinien in JKA-Einheiten (3ds Max: Grid Spacing). Ab Werk 8 wie das Raster des Karten-Editors.",
     "栅格线的间距, 以 JKA 单位计 (3ds Max: 栅格间距)。默认 8, 与地图编辑器的栅格相同。",
     "グリッド線の間隔 (JKA 単位、3ds Max: グリッド間隔)。既定は 8 で、マップエディタのグリッドと同じ。",
     "Separación de las líneas de la rejilla en unidades de JKA (3ds Max: Grid Spacing). Por defecto 8, como la rejilla del editor de mapas.",
     "Rozteč čar mřížky v jednotkách JKA (3ds Max: Grid Spacing). Výchozí 8 jako mřížka editoru map."),
    ("MrHaupt", "Major lines every Nth grid line", "Hauptlinien alle N Rasterlinien", "每 N 条栅格线一条主线", "N 本ごとにメジャーライン", "Líneas principales cada N líneas", "Hlavní čára každých N čar"),
    ("MrHauptHint",
     "3ds Max: Major Lines every Nth Grid Line. Default 8: a major line every 64 units - the height of a player (-24 to +40).",
     "3ds Max: Major Lines every Nth Grid Line. Ab Werk 8: eine Hauptlinie alle 64 Einheiten - die Höhe eines Spielers (-24 bis +40).",
     "3ds Max: 每 N 条栅格线一条主线。默认 8: 每 64 单位一条主线——玩家的高度 (-24 到 +40)。",
     "3ds Max: Major Lines every Nth Grid Line。既定は 8: 64 単位ごとにメジャーライン - プレイヤーの高さ (-24 から +40)。",
     "3ds Max: Major Lines every Nth Grid Line. Por defecto 8: una línea principal cada 64 unidades, la altura de un jugador (-24 a +40).",
     "3ds Max: Major Lines every Nth Grid Line. Výchozí 8: hlavní čára každých 64 jednotek - výška hráče (-24 až +40)."),
    ("MrAusdehnung", "Perspective view grid extent", "Rastergröße in der Perspektive", "透视图栅格范围", "パースのグリッド範囲", "Extensión de la rejilla en perspectiva", "Rozsah mřížky v perspektivě"),
    ("MrAusdehnungHint",
     "Half the size of the home grid in the perspective view, in grid lines (3ds Max: Perspective View Grid Extent, default 7). The orthographic views always fill the whole view.",
     "Halbe Größe des Home Grid in der Perspektive, in Rasterlinien (3ds Max: Perspective View Grid Extent, ab Werk 7). Die Orthoansichten füllt es immer ganz.",
     "透视图中主栅格的一半大小, 以栅格线计 (3ds Max: 透视视图栅格范围, 默认 7)。正交视图中始终填满整个视图。",
     "パースでのホームグリッドの半分の大きさ (グリッド線の本数、3ds Max: Perspective View Grid Extent、既定 7)。平行投影ビューでは常にビュー全体を埋めます。",
     "Mitad del tamaño de la rejilla de inicio en perspectiva, en líneas (3ds Max: Perspective View Grid Extent, por defecto 7). En las vistas ortográficas siempre llena toda la vista.",
     "Poloviční velikost domovské mřížky v perspektivě, v čarách mřížky (3ds Max: Perspective View Grid Extent, výchozí 7). Ortografické pohledy vždy vyplní celé."),
    ("MrBodenZHint",
     "Height of the floor grid and the grey floor. Default -24: the feet of a JKA figure, whose origin is at the hips (DEFAULT_MINS_2). 3ds Max and Blender put their grid at Z = 0.",
     "Höhe des Bodenrasters und des grauen Bodens. Ab Werk -24: die Füße einer JKA-Figur, deren Ursprung in der Hüfte liegt (DEFAULT_MINS_2). 3ds Max und Blender legen ihr Raster auf Z = 0.",
     "地面栅格和灰色地面的高度。默认 -24: JKA 角色的脚, 其原点在髋部 (DEFAULT_MINS_2)。3ds Max 和 Blender 将栅格放在 Z = 0。",
     "床グリッドと灰色の床の高さ。既定は -24: 原点が腰にある JKA フィギュアの足 (DEFAULT_MINS_2)。3ds Max と Blender はグリッドを Z = 0 に置きます。",
     "Altura de la rejilla del suelo y del suelo gris. Por defecto -24: los pies de una figura de JKA, cuyo origen está en la cadera (DEFAULT_MINS_2). 3ds Max y Blender ponen su rejilla en Z = 0.",
     "Výška podlahové mřížky a šedé podlahy. Výchozí -24: chodidla postavy JKA, jejíž počátek je v bocích (DEFAULT_MINS_2). 3ds Max a Blender dávají mřížku na Z = 0."),
    ("MrTiefster", "Lowest point", "Tiefster Punkt", "最低点", "最低点", "Punto más bajo", "Nejnižší bod"),
    ("MrTiefsterHint",
     "Put the floor under the lowest point of the figure in the current pose (for props whose origin is on the floor: 0).",
     "Den Boden unter den tiefsten Punkt der Figur in der aktuellen Pose legen (bei Requisiten mit Ursprung am Boden: 0).",
     "将地面放到角色在当前姿势中的最低点之下 (原点在地面上的道具: 0)。",
     "現在のポーズでのフィギュアの最低点の下に床を置く (原点が床にある小道具なら 0)。",
     "Poner el suelo bajo el punto más bajo de la figura en la pose actual (en accesorios con el origen en el suelo: 0).",
     "Položit podlahu pod nejnižší bod postavy v aktuální póze (u rekvizit s počátkem na podlaze: 0)."),
    ("MrGrauBoden", "Grey floor (ModView)", "Grauer Boden (ModView)", "灰色地面 (ModView)", "灰色の床 (ModView)", "Suelo gris (ModView)", "Šedá podlaha (ModView)"),
    # --- Blender: Overlays > Guides ---
    ("MrHilfslinien", "Guides", "Hilfslinien", "参考线", "ガイド", "Guías", "Vodítka"),
    ("MrRasterHintBl",
     "Grid in the orthographic side views (Blender Overlays > Grid): subdivisions that change with the zoom, the axes in their colours. In this view.",
     "Raster in den Orthoansichten von vorne, oben, seitlich (Blender Overlays > Grid): Unterteilungen, die mit dem Zoom wechseln, die Achsen in ihren Farben. In dieser Ansicht.",
     "正交侧视图中的栅格 (Blender 叠加层 > 栅格): 细分随缩放变化, 坐标轴以其颜色显示。仅此视图。",
     "平行投影の側面ビューのグリッド (Blender オーバーレイ > グリッド): ズームで変わる細分、軸はそれぞれの色。このビュー。",
     "Rejilla en las vistas ortográficas laterales (Blender Overlays > Grid): subdivisiones que cambian con el zoom, los ejes en sus colores. En esta vista.",
     "Mřížka v ortografických bočních pohledech (Blender Overlays > Grid): dělení, které se mění s přiblížením, osy ve svých barvách. V tomto pohledu."),
    ("MrBoden", "Floor", "Boden", "地面", "床", "Suelo", "Podlaha"),
    ("MrBodenHint",
     "Floor grid in the perspective and the free orthographic view (Blender Overlays > Floor): it fades out towards the horizon. At the floor height under the feet (Blender: Z = 0; a JKA figure has its origin at the hips). In this view.",
     "Bodenraster in der Perspektive und der freien Orthoansicht (Blender Overlays > Floor): es blendet zum Horizont aus. In Bodenhöhe unter den Füßen (Blender: Z = 0; eine JKA-Figur hat ihren Ursprung in der Hüfte). In dieser Ansicht.",
     "透视图和自由正交视图中的地面栅格 (Blender 叠加层 > 地面): 向地平线淡出。位于脚下的地面高度 (Blender: Z = 0; JKA 角色的原点在髋部)。仅此视图。",
     "パースと自由な平行投影ビューの床グリッド (Blender オーバーレイ > 床): 地平線に向かってフェードアウト。足の下の床の高さ (Blender: Z = 0。JKA のフィギュアは原点が腰)。このビュー。",
     "Rejilla del suelo en perspectiva y en la vista ortográfica libre (Blender Overlays > Floor): se desvanece hacia el horizonte. A la altura del suelo bajo los pies (Blender: Z = 0; una figura de JKA tiene su origen en la cadera). En esta vista.",
     "Podlahová mřížka v perspektivě a volném ortografickém pohledu (Blender Overlays > Floor): k horizontu slábne. Ve výšce podlahy pod chodidly (Blender: Z = 0; postava JKA má počátek v bocích). V tomto pohledu."),
    ("MrAchsen", "Axes", "Achsen", "坐标轴", "軸", "Ejes", "Osy"),
    ("MrAchseX", "Axis X", "Achse X", "X 轴", "X 軸", "Eje X", "Osa X"),
    ("MrAchseY", "Axis Y", "Achse Y", "Y 轴", "Y 軸", "Eje Y", "Osa Y"),
    ("MrAchseZ", "Axis Z", "Achse Z", "Z 轴", "Z 軸", "Eje Z", "Osa Z"),
    ("MrAchsenHint",
     "Show the world axis as a coloured line: X red, Y green, Z blue (Blender Overlays > Axes; X and Y on by default, Z off). In this view.",
     "Die Weltachse als farbige Linie zeigen: X rot, Y grün, Z blau (Blender Overlays > Axes; ab Werk X und Y an, Z aus). In dieser Ansicht.",
     "将世界坐标轴显示为彩色线: X 红、Y 绿、Z 蓝 (Blender 叠加层 > 坐标轴; 默认 X 和 Y 开, Z 关)。仅此视图。",
     "ワールド軸を色付きの線で表示: X 赤、Y 緑、Z 青 (Blender オーバーレイ > 軸。既定は X と Y がオン、Z はオフ)。このビュー。",
     "Mostrar el eje del mundo como línea de color: X rojo, Y verde, Z azul (Blender Overlays > Axes; por defecto X e Y activados, Z no). En esta vista.",
     "Zobrazit osu světa jako barevnou čáru: X červeně, Y zeleně, Z modře (Blender Overlays > Axes; výchozí X a Y zapnuto, Z vypnuto). V tomto pohledu."),
    ("MrMassstab", "Scale", "Maßstab", "缩放", "スケール", "Escala", "Měřítko"),
    ("MrMassstabHint",
     "Spacing of the grid lines in JKA units (Blender Overlays > Scale). Default 8, like the grid of the map editor.",
     "Abstand der Rasterlinien in JKA-Einheiten (Blender Overlays > Scale). Ab Werk 8 wie das Raster des Karten-Editors.",
     "栅格线的间距, 以 JKA 单位计 (Blender 叠加层 > 缩放)。默认 8, 与地图编辑器的栅格相同。",
     "グリッド線の間隔 (JKA 単位、Blender オーバーレイ > スケール)。既定は 8 で、マップエディタのグリッドと同じ。",
     "Separación de las líneas de la rejilla en unidades de JKA (Blender Overlays > Scale). Por defecto 8, como la rejilla del editor de mapas.",
     "Rozteč čar mřížky v jednotkách JKA (Blender Overlays > Scale). Výchozí 8 jako mřížka editoru map."),
    ("MrUnterteil", "Subdivisions", "Unterteilungen", "细分", "細分化", "Subdivisiones", "Dělení"),
    ("MrUnterteilHint",
     "How many lines lie between two stronger lines (Blender Overlays > Subdivisions). Default 8: a stronger line every 64 units - the height of a player (-24 to +40).",
     "Wie viele Linien zwischen zwei kräftigeren liegen (Blender Overlays > Subdivisions). Ab Werk 8: eine kräftigere Linie alle 64 Einheiten - die Höhe eines Spielers (-24 bis +40).",
     "两条较强的线之间有多少条线 (Blender 叠加层 > 细分)。默认 8: 每 64 单位一条较强的线——玩家的高度 (-24 到 +40)。",
     "強い線と強い線の間に何本の線があるか (Blender オーバーレイ > 細分化)。既定は 8: 64 単位ごとに強い線 - プレイヤーの高さ (-24 から +40)。",
     "Cuántas líneas hay entre dos más marcadas (Blender Overlays > Subdivisions). Por defecto 8: una línea marcada cada 64 unidades, la altura de un jugador (-24 a +40).",
     "Kolik čar leží mezi dvěma silnějšími (Blender Overlays > Subdivisions). Výchozí 8: silnější čára každých 64 jednotek - výška hráče (-24 až +40)."),
    # --- Einstellungen ---
    ("MrEinstGruppe", "Model tab: grid", "Model-Tab: Raster", "模型选项卡: 栅格", "モデルタブ: グリッド", "Pestaña Model: rejilla", "Karta Model: mřížka"),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
