# -*- coding: utf-8 -*-
"""Oberflaechentexte von ENTSCHEID1 (rc612, Entscheidungen von shank 11./12.11.): die Werkzeugnamen der Werkzeugleiste wie Blender,
Werkzeuggruppen (langer Klick), "Werkzeugnamen zeigen" und die eingebbare Seitenleiste (N) - fuer tools/gen_i18n.py und die
weiteren Sprachen.

Massstab der Uebersetzungen ist Blenders eigene (Blender 5.0, datafiles/locale/<sprache>/LC_MESSAGES/blender.mo, nur gelesen mit
Y:/werkzeuge/ent1/mo_lesen.py). Werkzeugnamen uebersetzt Blender im Kontext "Operator" (space_toolsystem_common.py:
iface_(item.label, i18n_contexts.operator_default)); fehlt dort ein Eintrag, zeigt Blender den englischen Namen - dann bleibt unser
Text. Offensichtlich falsche Eintraege von Blender (anderer Begriff) sind nicht uebernommen, sie stehen in
docs/intern/NOTIZEN_ent1_1211.md.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    # --- Werkzeuge der Leiste (Blender 5.0 space_toolsystem_toolbar.py: label=...) ---
    ("BeExtrude", "Extrude Region", "Region extrudieren", "挤出选区", "押し出し（領域）", "Extruir región", "Vysunutí oblasti"),
    ("BeExtrudeNormalen", "Extrude Along Normals", "Entlang der Normalen extrudieren", "沿法向挤出", "押し出し（法線方向）", "Extruir según normales",
     "Vysunutí Podél Normál"),
    ("BeExtrudeEinzeln", "Extrude Individual", "Individuell extrudieren", "挤出各个面", "押し出し（個別）", "Extruir individualmente", "Vysunout Jednotlivé"),
    ("BeInset", "Inset Faces", "Einfügen (Inset)", "内插面", "面を差し込む", "Incrustar caras", "Vsazení ploch"),
    ("BeLoopCut", "Loop Cut", "Schleife schneiden", "环切", "ループカット", "Cortar bucle", "Výřez smyčkou"),
    ("BeHalbieren", "Bisect", "Halbieren", "切分", "二分割", "Biseccionar", "Půlit"),
    ("BeNaht", "Mark Seam", "Naht markieren", "标记缝合边", "シームをマーク", "Marcar como costura", "Označit šev"),
    ("BePacken", "Pack Islands", "Packen", "拼排孤岛", "アイランドを梱包", "Empacar islas", "Zabalit"),
    ("BeEntspannen", "Relax", "Entspannen", "松弛", "リラックス", "Distender", "Uvolnit"),
    ("BeVerlauf", "Gradient", "Verlauf", "梯度渐变", "グラデーション", "Gradiente", "Přechod"),
    ("BeGewichtHolen", "Sample Weight", "Beispieleinfluss", "采样权重", "ウェイトを採取", "Tomar muestra (influencia)", "Odebrat váhu"),
    ("BeFuellen", "Fill", "Füllen", "填充", "フィル", "Rellenar", "Vyplnit"),
    ("BeWischen", "Smear", "Verwischen", "涂抹", "スミア", "Manchar", "Rozmazat"),
    ("BeMaske", "Mask", "Maskieren", "遮罩", "マスク", "Máscara", "Maska"),
    ("BeKopieren", "Copy Pose", "Pose kopieren", "复制姿态", "ポーズをコピー", "Copiar pose", "Kopírovat pózu"),
    ("BeGespiegelt", "Paste X-Flipped Pose", "Gespiegelt einfügen", "粘贴 X 轴向翻转姿态", "X軸反転ポーズを貼り付け", "Pegar pose (invertida en X)",
     "Vložit zrcadlově"),
    # --- Werkzeuggruppen und Werkzeugnamen ---
    ("BeGruppeHint", "Hold or drag: the other tools of this group.", "Gedrückt halten oder ziehen: die anderen Werkzeuge dieser Gruppe.",
     "按住或拖动: 此组中的其他工具。", "長押しまたはドラッグ: このグループのほかのツール。", "Mantén pulsado o arrastra: las demás herramientas de este grupo.",
     "Podržte nebo táhněte: další nástroje této skupiny."),
    ("BeExtrudeNormalenHint", "Extrudes each face along its own normal (like the Local Normal extrusion of 3ds Max).",
     "Extrudiert jede Fläche entlang ihrer eigenen Normalen (wie die Extrusion Local Normal von 3ds Max).",
     "沿每个面自身的法线挤出 (与 3ds Max 的 Local Normal 挤出相同)。", "各面をそれぞれの法線方向に押し出します (3ds Max の Local Normal と同じ)。",
     "Extruye cada cara según su propia normal (como la extrusión Local Normal de 3ds Max).",
     "Vysune každou plochu podél její vlastní normály (jako vysunutí Local Normal ve 3ds Max)."),
    ("BeExtrudeEinzelnHint", "Extrudes every face on its own (like the By Polygon extrusion of 3ds Max).",
     "Extrudiert jede Fläche für sich (wie die Extrusion By Polygon von 3ds Max).", "单独挤出每个面 (与 3ds Max 的 By Polygon 挤出相同)。",
     "各面を個別に押し出します (3ds Max の By Polygon と同じ)。", "Extruye cada cara por separado (como la extrusión By Polygon de 3ds Max).",
     "Vysune každou plochu zvlášť (jako vysunutí By Polygon ve 3ds Max)."),
    ("BeWerkzeugnamen", "Show Tool Names", "Werkzeugnamen zeigen", "显示工具名称", "ツール名を表示", "Mostrar nombres de herramientas", "Zobrazit názvy nástrojů"),
    ("BeWerkzeugnamenHint", "A wider toolbar with the name next to each tool. The program remembers the choice.",
     "Eine breitere Werkzeugleiste mit dem Namen neben jedem Werkzeug. Das Programm merkt sich die Wahl.",
     "更宽的工具栏, 每个工具旁显示名称。程序会记住此选择。", "各ツールの横に名前を表示する幅の広いツールバー。選択は記憶されます。",
     "Una barra de herramientas más ancha con el nombre junto a cada herramienta. El programa recuerda la elección.",
     "Širší panel nástrojů s názvem vedle každého nástroje. Program si volbu zapamatuje."),
    # --- Seitenleiste (N) eingebbar ---
    ("BeFeldHint", "Drag left or right to change, click to type a value (Enter takes it, Esc cancels). Ctrl+Z undoes the input.",
     "Nach links oder rechts ziehen ändert den Wert, ein Klick tippt ihn ein (Enter übernimmt, Esc bricht ab). Strg+Z nimmt die Eingabe zurück.",
     "左右拖动可更改数值, 单击可输入数值 (Enter 确认, Esc 取消)。Ctrl+Z 撤销此输入。",
     "左右にドラッグで値を変更、クリックで値を入力します (Enter で確定、Esc で取り消し)。Ctrl+Z で入力を元に戻します。",
     "Arrastra a izquierda o derecha para cambiar, haz clic para escribir un valor (Enter lo acepta, Esc cancela). Ctrl+Z deshace la entrada.",
     "Tažením doleva nebo doprava hodnotu změníte, kliknutím ji napíšete (Enter potvrdí, Esc zruší). Ctrl+Z vrátí zadání zpět."),
    ("BeFeldNurPose", "Only readable here: like Blender, the bone is posed in the Pose Mode (area Animation, with the rig).",
     "Hier nur zum Ablesen: wie in Blender stellt man den Knochen im Posenmodus (Bereich Animation, mit dem Rig).",
     "此处仅供查看: 与 Blender 相同, 骨骼在姿态模式 (动画区域, 使用绑定) 中摆姿势。",
     "ここでは表示のみ: Blender と同じく、ボーンはポーズモード (アニメーションエリア、リグ使用) でポーズを付けます。",
     "Aquí solo para leer: como en Blender, el hueso se posa en el Modo Pose (área Animación, con el rig).",
     "Zde jen ke čtení: jako v Blenderu se kost napózuje v režimu pózy (oblast Animace, s rigem)."),
    ("BeFeldGesperrt", "This control cannot do that - the message under the view says why.",
     "Dieser Griff kann das nicht - die Meldung unter der Ansicht sagt, warum.", "此控制器无法执行此操作 - 视图下方的消息会说明原因。",
     "このコントロールではできません - 理由はビューの下のメッセージに表示されます。", "Este control no puede hacerlo: el mensaje bajo la vista dice por qué.",
     "Tento ovladač to neumí - zpráva pod pohledem řekne proč."),
    ("BeUndoSeite", "Sidebar: %s", "Seitenleiste: %s", "侧栏: %s", "サイドバー: %s", "Barra lateral: %s", "Postranní panel: %s"),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
