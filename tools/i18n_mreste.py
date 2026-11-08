# -*- coding: utf-8 -*-
"""Oberflaechentexte von MRESTE (08.11.: Reste des Modell-Blocks vor rc611 - Blender-Modus in allen Stufen, Chamfer mit
Segmenten, ...) fuer tools/gen_i18n.py und die weiteren Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    # Punkt 1: Blender-Modus - Modellieren (km_edit_mesh mesh.select_linked Strg+L)
    ("MrVerbunden", "Select linked", "Verbundene wählen", "选择相连", "リンク選択", "Seleccionar vinculado", "Vybrat propojené"),
    ("MrVerbundenHint", "Grows the selection to everything connected to it by edges (whole elements), at the current level.",
     "Erweitert die Auswahl auf alles, was über Kanten daran hängt (ganze Elemente), in der aktuellen Ebene.",
     "将选择扩展到通过边与其相连的所有部分(整个元素),保持当前层级。",
     "辺でつながっているすべて(要素全体)に選択を広げます。現在のレベルのままです。",
     "Amplía la selección a todo lo que está conectado por aristas (elementos completos), en el nivel actual.",
     "Rozšíří výběr na vše, co je s ním spojeno hranami (celé prvky), v aktuální úrovni."),
    ("MrNichtsGewaehlt", "Select at least one vertex, edge or polygon first.", "Erst mindestens einen Punkt, eine Kante oder ein Polygon wählen.",
     "请先选择至少一个顶点、边或多边形。", "先に頂点、辺、ポリゴンのいずれかを 1 つ以上選択してください。",
     "Selecciona primero al menos un vértice, una arista o un polígono.", "Nejdřív vyberte aspoň jeden vrchol, hranu nebo polygon."),
    # Punkt 1: Blender-Modus - UV (km_uv_editor IMAGE_MT_uvs_align Umschalt+W)
    ("MrUvAusrichten", "Align", "Ausrichten", "对齐", "整列", "Alinear", "Zarovnat"),
    ("MrUvAusrichtenHint", "Opens the Align menu with Straighten and Align (Blender Shift+W).",
     "Öffnet das Menü Ausrichten mit Begradigen und Ausrichten (Blender Umschalt+W).",
     "打开对齐菜单,其中有拉直和对齐(Blender Shift+W)。", "「まっすぐにする」と「整列」がある整列メニューを開きます(Blender Shift+W)。",
     "Abre el menú Alinear con Enderezar y Alinear (Blender Mayús+W).", "Otevře nabídku Zarovnat s položkami Narovnat a Zarovnat (Blender Shift+W)."),
    # Punkt 1: Blender-Modus - Weights (km_weight_paint)
    ("MrGwAufnehmen", "Sample weight", "Gewicht aufnehmen", "采样权重", "ウェイトを取得", "Muestrear peso", "Odebrat váhu"),
    ("MrGwAufnehmenHint", "Takes the weight of the selected bone at the vertex under the mouse as the brush weight (Blender Shift+X).",
     "Nimmt das Gewicht des gewählten Knochens an der Ecke unter der Maus als Gewicht des Pinsels (Blender Umschalt+X).",
     "将鼠标下顶点处所选骨骼的权重用作画笔权重(Blender Shift+X)。",
     "マウスの下の頂点での選択ボーンのウェイトをブラシのウェイトにします(Blender Shift+X)。",
     "Toma el peso del hueso seleccionado en el vértice bajo el ratón como peso del pincel (Blender Mayús+X).",
     "Vezme váhu vybrané kosti ve vrcholu pod myší jako váhu štětce (Blender Shift+X)."),
    ("MrGwAufnehmenKlick", "Click the model to sample the weight.", "Auf das Modell klicken, um das Gewicht aufzunehmen.", "单击模型以采样权重。",
     "モデルをクリックしてウェイトを取得します。", "Haz clic en el modelo para muestrear el peso.", "Klepnutím na model odeberete váhu."),
    ("MrGwAufgenommen", "Weight sampled: %.3f", "Gewicht aufgenommen: %.3f", "已采样权重: %.3f", "ウェイトを取得しました: %.3f", "Peso muestreado: %.3f",
     "Váha odebrána: %.3f"),
    ("MrRadialHinweis", "Move the mouse left or right. Click or Enter confirms, Esc or right-click cancels.",
     "Maus nach links oder rechts bewegen. Klick oder Enter übernimmt, Esc oder Rechtsklick bricht ab.",
     "向左或向右移动鼠标。单击或 Enter 确认,Esc 或右键取消。", "マウスを左右に動かします。クリックまたは Enter で確定、Esc または右クリックで取り消します。",
     "Mueve el ratón a la izquierda o a la derecha. Clic o Intro confirma, Esc o clic derecho cancela.",
     "Pohybujte myší doleva nebo doprava. Klepnutí nebo Enter potvrdí, Esc nebo pravé tlačítko zruší."),
    ("MrGwTastenBlender",
     "Left mouse paints, with Ctrl inverted, with Shift smoothing. Shift+X samples the weight, Ctrl+X sets it on the selection. F radius, "
     "Shift+F strength, Ctrl+F weight. Ctrl+Z undoes a stroke, Ctrl+Tab switches the mode.",
     "Linke Maustaste malt, mit Strg umgekehrt, mit Umschalt glättend. Umschalt+X nimmt das Gewicht auf, Strg+X setzt es auf die Auswahl. "
     "F Radius, Umschalt+F Stärke, Strg+F Gewicht. Strg+Z nimmt einen Strich zurück, Strg+Tab wechselt den Modus.",
     "鼠标左键绘制,按住 Ctrl 反向,按住 Shift 平滑。Shift+X 采样权重,Ctrl+X 将其设置到所选内容。F 半径,Shift+F 强度,Ctrl+F 权重。"
     "Ctrl+Z 撤销一笔,Ctrl+Tab 切换模式。",
     "左ボタンで描画、Ctrl で反転、Shift でスムーズ。Shift+X でウェイトを取得、Ctrl+X で選択に設定。F 半径、Shift+F 強さ、Ctrl+F ウェイト。"
     "Ctrl+Z で 1 ストローク戻し、Ctrl+Tab でモードを切り替えます。",
     "El botón izquierdo pinta, con Ctrl invertido, con Mayús suavizando. Mayús+X muestrea el peso, Ctrl+X lo aplica a la selección. "
     "F radio, Mayús+F intensidad, Ctrl+F peso. Ctrl+Z deshace un trazo, Ctrl+Tab cambia el modo.",
     "Levé tlačítko maluje, s Ctrl obráceně, se Shift vyhlazuje. Shift+X odebere váhu, Ctrl+X ji nastaví na výběr. F poloměr, "
     "Shift+F síla, Ctrl+F váha. Ctrl+Z vrátí jeden tah, Ctrl+Tab přepne režim."),
    # Punkt 3: Chamfer mit Segmenten
    ("MrProfilMax", "Tension", "Spannung", "张力", "テンション", "Tensión", "Napětí"),
    ("MrProfilBlender", "Shape", "Form", "形状", "シェイプ", "Forma", "Tvar"),
    ("MrProfilHint", "Shape of the rounding with more than one segment: 0.5 round, 0.25 straight, towards 1 square, below 0.25 hollow.",
     "Form der Rundung bei mehr als einem Segment: 0,5 rund, 0,25 gerade, gegen 1 eckig, unter 0,25 hohl.",
     "多于一个分段时圆角的形状: 0.5 圆, 0.25 直, 接近 1 为方角, 低于 0.25 为内凹。",
     "セグメントが 2 つ以上のときの丸みの形: 0.5 で円、0.25 で直線、1 に近いほど角、0.25 未満でくぼみ。",
     "Forma del redondeo con más de un segmento: 0,5 redondo, 0,25 recto, hacia 1 en ángulo, por debajo de 0,25 hueco.",
     "Tvar zaoblení u více než jednoho segmentu: 0,5 kulaté, 0,25 rovné, k 1 hranaté, pod 0,25 duté."),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
