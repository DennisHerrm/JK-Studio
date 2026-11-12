# -*- coding: utf-8 -*-
"""Oberflaechentexte von AP6 (MBENTWURF Teil B): das rechte Feld im Bereich Modellieren des Model-Tabs nach dem Entwurf -
Gruppen Auswahl, Werkzeuge, JKA-Grenzen, dazu Grundkoerper und Requisit im Reiter Erstellen (wie 3ds Max) bzw. Objekt
(wie Blender) - fuer tools/gen_i18n.py und die weiteren Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    # --- Gruppe Auswahl: die Ebenen als Segment (Entwurf: 1 Ecke, 2 Kante, 3 Rand, 4 Polygon, 5 Element; Blender 3 Flaeche) ---
    ("Ap6SegEcke", "Vertex", "Ecke", "顶点", "頂点", "Vértice", "Vrchol"),
    ("Ap6SegFlaeche", "Face", "Fläche", "面", "面", "Cara", "Plocha"),
    ("Ap6ZPunkte", "Vertices", "Punkte", "顶点", "頂点", "Vértices", "Vrcholy"),
    ("Ap6ZKanten", "Edges", "Kanten", "边", "エッジ", "Aristas", "Hrany"),
    ("Ap6ZVielecke", "Polygons", "Polygone", "多边形", "ポリゴン", "Polígonos", "Polygony"),
    ("Ap6ZFlaechen", "Faces", "Flächen", "面", "面", "Caras", "Plochy"),
    ("Ap6ZWert", "%d · selected %d", "%d · gewählt %d", "%d · 已选 %d", "%d · 選択 %d", "%d · seleccionados %d", "%d · vybráno %d"),
    # --- Gruppe Werkzeuge ---
    ("Ap6Werkzeuge", "Tools", "Werkzeuge", "工具", "ツール", "Herramientas", "Nástroje"),
    ("Ap6WerkzeugTipp", "Each tool first shows a preview: Apply (Enter), Apply and continue (Ctrl+Enter), Cancel (Esc).",
     "Jedes Werkzeug zeigt erst eine Vorschau: Übernehmen (Enter), Übernehmen und weiter (Strg+Enter), Abbrechen (Esc).",
     "每个工具先显示预览: 应用 (Enter)、应用并继续 (Ctrl+Enter)、取消 (Esc)。", "各ツールはまずプレビューを表示します: 適用 (Enter)、適用して続行 (Ctrl+Enter)、キャンセル (Esc)。",
     "Cada herramienta muestra primero una vista previa: Aplicar (Intro), Aplicar y continuar (Ctrl+Intro), Cancelar (Esc).",
     "Každý nástroj nejdřív ukáže náhled: Použít (Enter), Použít a pokračovat (Ctrl+Enter), Zrušit (Esc)."),
    ("Ap6NichtHier", "Not at this level. Works on: %s.", "Nicht auf dieser Ebene. Arbeitet auf: %s.", "此层级不可用。适用于: %s。", "このレベルでは使えません。対象: %s。",
     "No en este nivel. Funciona con: %s.", "Na této úrovni ne. Pracuje s: %s."),
    # --- Gruppe JKA-Grenzen ---
    ("Ap6InfoEcken", "%d / %d vertices", "%d / %d Ecken", "%d / %d 顶点", "頂点 %d / %d", "%d / %d vértices", "%d / %d vrcholů"),
    ("Ap6InfoKnochen", "%d / %d bones", "%d / %d Knochen", "%d / %d 骨骼", "ボーン %d / %d", "%d / %d huesos", "%d / %d kostí"),
    ("Ap6FlaechenImModell", "Surfaces in the model", "Flächen im Modell", "模型中的面", "モデル内のサーフェス", "Superficies del modelo", "Povrchy v modelu"),
    ("Ap6VolleFlaeche", "Nothing selected: this is the surface closest to a limit.", "Nichts gewählt: gezeigt ist die Fläche, die einer Grenze am nächsten kommt.",
     "未选择: 显示最接近限制的面。", "未選択: 上限に最も近いサーフェスを表示しています。", "Nada seleccionado: se muestra la superficie más cercana a un límite.",
     "Nic není vybráno: zobrazen je povrch nejblíže limitu."),
    ("Ap6AlleFlaechen", "All surfaces (%d)", "Alle Flächen (%d)", "全部面 (%d)", "すべてのサーフェス (%d)", "Todas las superficies (%d)", "Všechny povrchy (%d)"),
    ("Ap6AlleLegende", "Vertices · bones · weights per surface", "Ecken · Knochen · Gewichte je Fläche", "每个面的顶点 · 骨骼 · 权重", "サーフェスごとの頂点 · ボーン · ウェイト",
     "Vértices · huesos · pesos por superficie", "Vrcholy · kosti · váhy na povrch"),
    ("Ap6WeitereBefunde", "+%d more - see the list below.", "+%d weitere - siehe die Liste darunter.", "还有 %d 项 - 见下方列表。", "ほか %d 件 - 下のリストを参照。",
     "+%d más: véase la lista de abajo.", "+%d dalších - viz seznam níže."),
    # --- Reiter Erstellen (wie 3ds Max) / Objekt (wie Blender): Grundkoerper und Requisit ---
    ("Ap6Grundkoerper", "Primitives", "Grundkörper", "基本体", "プリミティブ", "Primitivas", "Základní tělesa"),
    ("Ap6ErstellenLeer", "The prop is empty: start with a primitive.", "Das Requisit ist leer: mit einem Grundkörper beginnen.", "道具为空: 从一个基本体开始。",
     "小道具は空です: プリミティブから始めましょう。", "El accesorio está vacío: empieza con una primitiva.", "Rekvizita je prázdná: začněte základním tělesem."),
    ("Ap6Name", "Name", "Name", "名称", "名前", "Nombre", "Název"),
    ("Ap6Hinzufuegen", "Add", "Hinzufügen", "添加", "追加", "Añadir", "Přidat"),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
