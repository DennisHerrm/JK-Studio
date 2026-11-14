# -*- coding: utf-8 -*-
"""Oberflaechentexte von TEST1411 (14.11.: Testrueckmeldungen von shank zu rc612).

I: im Bereich Animation ist ein Knochen von Anfang an waehlbar (wie 3ds Max), auch ohne Control Rig.
Begriffe wie die vorhandenen Texte (AP4 "Knochen waehlen"/"Select bone", AP10 Control Rig, ENT2 IK/FK).
Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    # --- I: Knochen im Bereich Animation ---
    ("T1KnochenGewaehlt",
     "Bone %s selected - E rotates it, K sets a key (no control rig needed).",
     "Knochen %s gewählt - E dreht ihn, K setzt einen Schlüssel (ohne Control Rig).",
     "已选择骨骼 %s - E 旋转, K 设置关键帧 (无需控制绑定)。",
     "ボーン %s を選択しました - E で回転、K でキーを設定 (コントロールリグは不要)。",
     "Hueso %s seleccionado: E lo gira, K pone una clave (sin control rig).",
     "Kost %s vybrána - E ji otáčí, K nastaví klíč (bez control rigu)."),
    # --- H: Knochen als Kette je Glied ---
    ("T1Hilfsknochen", "Show helper bones", "Hilfsknochen zeigen", "显示辅助骨骼", "補助ボーンを表示", "Mostrar huesos auxiliares", "Zobrazit pomocné kosti"),
    ("T1HilfsknochenHint",
     "Twist helpers (e.g. lradiusX: same axis as the parent, in front of a sibling, no chain of their own) as a box instead of a small point. "
     "Every other bone is one box from its joint along its own axis to the next joint of the chain, like 3ds Max bones.",
     "Drehhilfen (z. B. lradiusX: gleiche Achse wie der Elternknochen, vor einem Geschwister, ohne eigene Kette) als Kasten statt als kleiner Punkt. "
     "Jeder andere Knochen ist ein Kasten von seinem Gelenk entlang seiner Achse bis zum nächsten Gelenk der Kette, wie Bones in 3ds Max.",
     "将扭转辅助骨骼 (例如 lradiusX: 与父骨骼同轴、位于同级骨骼之前、没有自己的骨骼链) 显示为方框而不是小点。"
     "其他每根骨骼都是一个方框, 从其关节沿自身轴线延伸到骨骼链的下一个关节, 与 3ds Max 的骨骼相同。",
     "ねじれ補助ボーン (例: lradiusX - 親と同じ軸、兄弟ボーンの手前、独自のチェーンなし) を小さな点ではなくボックスで表示します。"
     "他のボーンはすべて、関節から自身の軸に沿ってチェーンの次の関節まで伸びる 1 つのボックスです (3ds Max のボーンと同じ)。",
     "Ayudantes de torsión (p. ej. lradiusX: mismo eje que el padre, delante de un hermano, sin cadena propia) como caja en lugar de un punto pequeño. "
     "Cada uno de los demás huesos es una caja desde su articulación a lo largo de su eje hasta la siguiente articulación de la cadena, como los huesos de 3ds Max.",
     "Pomocné kosti pro natočení (např. lradiusX: stejná osa jako rodič, před sourozencem, bez vlastního řetězce) jako kvádr místo malého bodu. "
     "Každá jiná kost je jeden kvádr od svého kloubu podél vlastní osy k dalšímu kloubu řetězce, jako kosti v 3ds Max."),
    # --- G: Zeitleiste und Statuszeile wie 3ds Max ---
    ("T1SchieberHint", "Time slider: drag the handle to scrub, click the bar to jump there, < and > step one frame.",
     "Zeitschieber: den Griff ziehen spult, ein Klick in die Leiste springt dorthin, < und > gehen ein Bild weiter.",
     "时间滑块: 拖动滑块可拖拽播放, 单击滑轨跳到该处, < 和 > 前后移动一帧。",
     "タイムスライダー: つまみをドラッグしてスクラブ、バーをクリックでその位置へ、< と > で 1 フレーム移動します。",
     "Control de tiempo: arrastre el tirador para recorrer, un clic en la barra salta allí, < y > avanzan un fotograma.",
     "Časový posuvník: tažením úchytu se převíjí, klik do lišty skočí tam, < a > posunou o jeden snímek."),
    ("T1MiniKurve", "Mini Curve Editor", "Mini-Kurveneditor", "迷你曲线编辑器", "ミニカーブエディタ", "Mini editor de curvas", "Mini editor křivek"),
    ("T1MiniKurveHint", "Opens the dope sheet and the curve editor above the track bar, as in 3ds Max - the track bar stays the same.",
     "Öffnet Dope Sheet und Kurveneditor über der Spurleiste wie in 3ds Max - die Spurleiste bleibt dieselbe.",
     "像 3ds Max 一样在轨迹栏上方打开摄影表和曲线编辑器 - 轨迹栏保持不变。",
     "3ds Max と同じくトラックバーの上にドープシートとカーブエディタを開きます - トラックバーはそのままです。",
     "Abre la hoja de claves y el editor de curvas sobre la barra de pistas, como en 3ds Max; la barra de pistas sigue igual.",
     "Otevře tabulku klíčů a editor křivek nad stopovou lištou jako v 3ds Max - stopová lišta zůstává stejná."),
    ("T1BildfeldHint", "Current frame: type a number and press Enter to jump there (, and . step one frame).",
     "Aktuelles Bild: eine Zahl eintippen und Enter springt dorthin (, und . gehen ein Bild weiter).",
     "当前帧: 输入数字并按 Enter 跳到该帧 (, 和 . 前后移动一帧)。",
     "現在のフレーム: 数値を入力して Enter でそこへ移動します (, と . で 1 フレーム移動)。",
     "Fotograma actual: escriba un número y pulse Intro para saltar allí (, y . avanzan un fotograma).",
     "Aktuální snímek: napište číslo a Enter tam skočí (, a . posunou o jeden snímek)."),
    ("T1KeineAuswahl", "None Selected", "Nichts ausgewählt", "未选择", "選択なし", "Nada seleccionado", "Nic nevybráno"),
    ("T1AuswahlFlaechen", "%d Surfaces Selected", "%d Flächen ausgewählt", "已选择 %d 个表面", "%d 個のサーフェスを選択", "%d superficies seleccionadas", "Vybráno ploch: %d"),
    ("T1AuswahlEcken", "%d Vertices Selected", "%d Ecken ausgewählt", "已选择 %d 个顶点", "%d 個の頂点を選択", "%d vértices seleccionados", "Vybráno vrcholů: %d"),
    ("T1AuswahlKnochen", "Bone %s Selected", "Knochen %s ausgewählt", "已选择骨骼 %s", "ボーン %s を選択", "Hueso %s seleccionado", "Vybrána kost %s"),
    ("T1HinweisWaehlen", "Click or click-and-drag to select objects", "Klicken oder Rahmen ziehen, um Objekte auszuwählen", "单击或拖动以选择对象",
     "クリックまたはドラッグしてオブジェクトを選択", "Haga clic o arrastre para seleccionar objetos", "Klepnutím nebo tažením vyberte objekty"),
    ("T1HinweisBewegen", "Click and drag to select and move objects", "Klicken und ziehen, um Objekte auszuwählen und zu verschieben", "单击并拖动以选择并移动对象",
     "クリックしてドラッグし、オブジェクトを選択して移動", "Haga clic y arrastre para seleccionar y mover objetos", "Klepnutím a tažením vyberte a posuňte objekty"),
    ("T1HinweisDrehen", "Click and drag to select and rotate objects", "Klicken und ziehen, um Objekte auszuwählen und zu drehen", "单击并拖动以选择并旋转对象",
     "クリックしてドラッグし、オブジェクトを選択して回転", "Haga clic y arrastre para seleccionar y rotar objetos", "Klepnutím a tažením vyberte a otočte objekty"),
    ("T1HinweisSkalieren", "Click and drag to select and scale objects", "Klicken und ziehen, um Objekte auszuwählen und zu skalieren",
     "单击并拖动以选择并缩放对象", "クリックしてドラッグし、オブジェクトを選択して拡大縮小", "Haga clic y arrastre para seleccionar y escalar objetos",
     "Klepnutím a tažením vyberte a změňte měřítko objektů"),
    ("T1Raster", "Grid = %g", "Raster = %g", "栅格 = %g", "グリッド = %g", "Rejilla = %g", "Mřížka = %g"),
    ("T1Tastenzeile", "Key row under the timeline", "Tastenzeile unter der Zeitleiste", "时间轴下方的快捷键行", "タイムライン下のキー行",
     "Fila de teclas bajo la línea de tiempo", "Řádek kláves pod časovou osou"),
    ("T1TastenzeileHint", "Shows the keys of the area in a row under the status bar (off by default, as in 3ds Max).",
     "Zeigt die Tasten des Bereichs in einer Zeile unter der Statuszeile (ab Werk aus, wie in 3ds Max).",
     "在状态栏下方的一行中显示该区域的快捷键 (默认关闭, 与 3ds Max 相同)。",
     "ステータスバーの下の行にこのエリアのキーを表示します (既定はオフ、3ds Max と同じ)。",
     "Muestra las teclas del área en una fila bajo la barra de estado (desactivado de fábrica, como en 3ds Max).",
     "Zobrazí klávesy oblasti v řádku pod stavovým řádkem (ve výchozím stavu vypnuto, jako v 3ds Max)."),
    # --- J: Quad-Menue wie 3ds Max ---
    ("T1QuadTransform", "transform", "Transformieren", "变换", "変換", "transformar", "transformace"),
    ("T1QuadDisplay", "display", "Anzeige", "显示", "表示", "visualización", "zobrazení"),
    ("T1QuadTools1", "tools 1", "Werkzeuge 1", "工具 1", "ツール 1", "herramientas 1", "nástroje 1"),
    ("T1QuadTools2", "tools 2", "Werkzeuge 2", "工具 2", "ツール 2", "herramientas 2", "nástroje 2"),
    ("T1QuadMove", "Move", "Verschieben", "移动", "移動", "Mover", "Posunout"),
    ("T1QuadRotate", "Rotate", "Drehen", "旋转", "回転", "Rotar", "Otočit"),
    ("T1QuadScale", "Scale", "Skalieren", "缩放", "スケール", "Escalar", "Měřítko"),
    ("T1QuadSelect", "Select", "Auswählen", "选择", "選択", "Seleccionar", "Vybrat"),
    ("T1QuadCurveEditor", "Curve Editor...", "Kurveneditor...", "曲线编辑器...", "カーブエディタ...", "Editor de curvas...", "Editor křivek..."),
    ("T1QuadDopeSheet", "Dope Sheet...", "Dope Sheet...", "摄影表...", "ドープシート...", "Hoja de claves...", "Tabulka klíčů..."),
    ("T1QuadIsolate", "Isolate Selection", "Auswahl isolieren", "孤立当前选择", "選択を分離", "Aislar selección", "Izolovat výběr"),
    ("T1QuadEndIsolate", "End Isolate", "Isolieren beenden", "结束隔离", "分離を終了", "Finalizar aislamiento", "Ukončit izolaci"),
    ("T1QuadUnfreezeAll", "Unfreeze All", "Alle Fixierungen aufheben", "全部解冻", "すべてフリーズ解除", "Descongelar todo", "Uvolnit vše"),
    ("T1QuadFreezeSel", "Freeze Selection", "Auswahl fixieren", "冻结当前选择", "選択をフリーズ", "Congelar selección", "Zmrazit výběr"),
    ("T1QuadUnhideAll", "Unhide All", "Alle einblenden", "全部取消隐藏", "すべて表示", "Mostrar todo", "Zobrazit vše"),
    ("T1QuadHideUnsel", "Hide Unselected", "Nicht Ausgewähltes ausblenden", "隐藏未选定对象", "非選択を非表示", "Ocultar no seleccionado", "Skrýt nevybrané"),
    ("T1QuadHideSel", "Hide Selection", "Auswahl ausblenden", "隐藏选定对象", "選択を非表示", "Ocultar selección", "Skrýt výběr"),
    ("T1BlTransform", "Transform", "Transformieren", "变换", "トランスフォーム", "Transformar", "Transformace"),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
