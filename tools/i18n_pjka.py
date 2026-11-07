# -*- coding: utf-8 -*-
"""Oberflaechentexte des Auftrags p_jka (Max-Tasten des Modellierens nach der Autodesk-Tabelle) fuer tools/gen_i18n.py und
die weiteren Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("MdWegHint", "No key - menu: %s", "Ohne Taste - Menü: %s", "无快捷键 - 菜单: %s", "キーなし - メニュー: %s",
     "Sin tecla - menú: %s", "Bez klávesy - nabídka: %s"),
    ("MdEben", "Make Planar", "Eben machen", "平面化", "平面化", "Hacer plano", "Zarovnat do roviny"),
    ("MdEbenHint", "Moves the selected vertices (or those of the selected edges and polygons) onto one plane: its normal is the average "
     "normal of the selection, it runs through their centre; UVs stay.",
     "Schiebt die gewählten Punkte (oder die der gewählten Kanten und Polygone) in eine Ebene: ihre Normale ist die gemittelte Normale der "
     "Auswahl, sie läuft durch ihre Mitte; die UV bleibt.",
     "将选中的顶点 (或选中边和多边形的顶点) 移到同一平面上: 平面法线为选择的平均法线, 并通过其中心; UV 保持不变。",
     "選択した頂点 (または選択したエッジやポリゴンの頂点) を一つの平面に移します。平面の法線は選択の平均法線で、その中心を通ります。UV はそのままです。",
     "Lleva los vértices elegidos (o los de las aristas y polígonos elegidos) a un plano: su normal es la normal media de la selección y pasa "
     "por su centro; las UV se mantienen.",
     "Přesune vybrané vrcholy (nebo vrcholy vybraných hran a polygonů) do jedné roviny: její normála je průměrná normála výběru a prochází "
     "jejich středem; UV zůstanou."),
    ("MdCollapse", "Collapse", "Kollabieren", "塌陷", "集約", "Colapsar", "Sloučit do bodu"),
    ("MdCollapseHint", "Each connected group of the selection becomes one vertex at its centre.",
     "Jede zusammenhängende Gruppe der Auswahl wird ein Punkt in ihrer Mitte.",
     "选择中每个相连的组变为其中心的一个顶点。", "選択のつながった各グループが、その中心の一つの頂点になります。",
     "Cada grupo conectado de la selección se convierte en un vértice en su centro.",
     "Každá souvislá skupina výběru se stane jedním vrcholem v jejím středu."),
    ("MdSlice", "Slice", "Schneiden (Ebene)", "切片", "スライス", "Cortar (plano)", "Řez rovinou"),
    ("MdSliceHint", "Cuts the selected polygons (at other levels the whole model) along a plane across an axis; the caddy sets axis "
     "and position, it starts in the middle.",
     "Schneidet die gewählten Polygone (auf anderen Ebenen das ganze Modell) an einer Ebene quer zu einer Achse; Achse und Lage im "
     "Kästchen, sie beginnt in der Mitte.",
     "沿垂直于某轴的平面切开选中的多边形 (其他层级为整个模型); 在工具框中设置轴和位置, 默认在中间。",
     "選択したポリゴン (他のレベルではモデル全体) を軸に垂直な平面で切ります。軸と位置はツールボックスで設定し、中央から始まります。",
     "Corta los polígonos elegidos (en otros niveles todo el modelo) por un plano perpendicular a un eje; eje y posición en el cuadro, "
     "empieza en el centro.",
     "Rozřízne vybrané polygony (na jiných úrovních celý model) rovinou kolmou k ose; osa a poloha v okénku, začíná uprostřed."),
    ("MdSchnittOrt", "Position", "Lage", "位置", "位置", "Posición", "Poloha"),
    ("MdQuickSlice", "QuickSlice", "Schnellschnitt", "快速切片", "クイックスライス", "Corte rápido", "Rychlý řez"),
    ("MdQuickSliceHint", "Two clicks in a view draw a line; the model (or the selected polygons) is cut along the plane through that "
     "line and the view direction.",
     "Zwei Klicks in einer Ansicht ziehen eine Linie; das Modell (oder die gewählten Polygone) wird an der Ebene durch diese Linie und "
     "die Blickrichtung geschnitten.",
     "在视图中点击两次画出一条线; 沿经过该线和视线方向的平面切开模型 (或选中的多边形)。",
     "ビューで 2 回クリックして線を引くと、その線と視線方向を含む平面でモデル (または選択ポリゴン) を切ります。",
     "Dos clics en una vista trazan una línea; el modelo (o los polígonos elegidos) se corta por el plano que pasa por esa línea y la "
     "dirección de la vista.",
     "Dvě kliknutí v pohledu nakreslí čáru; model (nebo vybrané polygony) se rozřízne rovinou procházející touto čárou a směrem pohledu."),
    ("MdQuickStart", "QuickSlice: click the start of the line (Esc ends).", "Schnellschnitt: den Anfang der Linie anklicken (Esc beendet).",
     "快速切片: 点击线的起点 (Esc 结束)。", "クイックスライス: 線の始点をクリック (Esc で終了)。",
     "Corte rápido: haz clic en el inicio de la línea (Esc termina).", "Rychlý řez: klikněte na začátek čáry (Esc ukončí)."),
    ("MdQuickWeiter", "QuickSlice: click the end of the line.", "Schnellschnitt: das Ende der Linie anklicken.", "快速切片: 点击线的终点。",
     "クイックスライス: 線の終点をクリック。", "Corte rápido: haz clic en el final de la línea.", "Rychlý řez: klikněte na konec čáry."),
    ("MdHinge", "Hinge", "Scharnier (Hinge)", "铰链", "ヒンジ", "Bisagra", "Pant"),
    ("MdHingeHint", "Swings the selected polygons about one of their border edges (the hinge), with walls in between; the caddy "
     "chooses the edge and the angle.",
     "Dreht die gewählten Polygone um eine ihrer Randkanten (das Scharnier), mit Wänden dazwischen; Kante und Winkel im Kästchen.",
     "将选中的多边形绕其一条边界边 (铰链) 旋转, 中间生成侧壁; 在工具框中选择边和角度。",
     "選択したポリゴンを境界エッジの一つ (ヒンジ) を軸に回し、間に壁を作ります。エッジと角度はツールボックスで選びます。",
     "Gira los polígonos elegidos alrededor de una de sus aristas de borde (la bisagra), con paredes entre medias; arista y ángulo en el "
     "cuadro.",
     "Otočí vybrané polygony kolem jedné z jejich okrajových hran (pantu) se stěnami mezi nimi; hranu a úhel zvolíte v okénku."),
    ("MdHingeKante", "Hinge edge", "Scharnierkante", "铰链边", "ヒンジエッジ", "Arista bisagra", "Hrana pantu"),
    ("MdWinkel", "Angle", "Winkel", "角度", "角度", "Ángulo", "Úhel"),
    ("MdAblSchnitt", "the plane crosses no polygon of the selection.", "die Ebene kreuzt kein Polygon der Auswahl.",
     "平面未穿过选择中的任何多边形。", "平面が選択のどのポリゴンとも交わりません。", "el plano no cruza ningún polígono de la selección.",
     "rovina neprotíná žádný polygon výběru."),
    ("MdAblHinge", "the hinge must be a border edge of the selected polygons.", "das Scharnier muss eine Randkante der gewählten Polygone sein.",
     "铰链必须是选中多边形的边界边。", "ヒンジは選択したポリゴンの境界エッジでなければなりません。",
     "la bisagra debe ser una arista de borde de los polígonos elegidos.", "pant musí být okrajovou hranou vybraných polygonů."),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
