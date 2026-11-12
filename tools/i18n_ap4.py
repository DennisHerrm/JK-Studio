# -*- coding: utf-8 -*-
"""Oberflaechentexte von AP4 (MBENTWURF Teil B): der Model-Tab im Bedienmodus wie Blender nach dem freigegebenen Entwurf -
Werkzeugleiste (T) links mit den Werkzeugen des Bereichs, Outliner oben rechts ueber dem Eigenschaften-Editor, Seitenleiste (N),
der Modus im Kopf der Ansicht, das Werkzeug Messen - fuer tools/gen_i18n.py und die weiteren Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    # --- Werkzeugleiste (T) und Seitenleiste (N) ---
    ("BzWerkzeugleiste", "Toolbar", "Werkzeugleiste", "工具栏", "ツールバー", "Barra de herramientas", "Panel nástrojů"),
    ("BzWerkzeugleisteHint", "The tools of the area on the left of the view, like the toolbar of Blender. T shows or hides it.",
     "Die Werkzeuge des Bereichs links neben der Ansicht, wie die Werkzeugleiste von Blender. T blendet sie ein oder aus.",
     "视图左侧显示该区域的工具, 与 Blender 的工具栏相同。按 T 显示或隐藏。",
     "ビューの左にこのエリアのツールを表示します (Blender のツールバーと同じ)。T で表示/非表示を切り替えます。",
     "Las herramientas del área a la izquierda de la vista, como la barra de herramientas de Blender. T la muestra u oculta.",
     "Nástroje oblasti vlevo od pohledu, jako panel nástrojů v Blenderu. Klávesa T ho zobrazí nebo skryje."),
    ("BzLeisteAufHint", "Show the toolbar (T)", "Werkzeugleiste zeigen (T)", "显示工具栏 (T)", "ツールバーを表示 (T)", "Mostrar la barra de herramientas (T)",
     "Zobrazit panel nástrojů (T)"),
    ("BzSeitenleiste", "Sidebar", "Seitenleiste", "侧栏", "サイドバー", "Barra lateral", "Postranní panel"),
    ("BzSeitenleisteHint", "Values of the selection and of the view on the right of the view, like the sidebar of Blender. N shows or hides it.",
     "Werte der Auswahl und der Ansicht rechts in der Ansicht, wie die Seitenleiste von Blender. N blendet sie ein oder aus.",
     "在视图右侧显示所选内容和视图的数值, 与 Blender 的侧栏相同。按 N 显示或隐藏。",
     "ビューの右に選択とビューの値を表示します (Blender のサイドバーと同じ)。N で表示/非表示を切り替えます。",
     "Los valores de la selección y de la vista a la derecha de la vista, como la barra lateral de Blender. N la muestra u oculta.",
     "Hodnoty výběru a pohledu vpravo v pohledu, jako postranní panel v Blenderu. Klávesa N ho zobrazí nebo skryje."),
    ("BzHinweisAnsicht", "N Sidebar · T Tools", "N Seitenleiste · T Werkzeuge", "N 侧栏 · T 工具", "N サイドバー · T ツール", "N Barra lateral · T Herramientas",
     "N Postranní panel · T Nástroje"),
    ("BzTrennerHint", "Drag to share the height between the Outliner and the Properties.",
     "Ziehen teilt die Höhe zwischen Outliner und Eigenschaften auf.", "拖动可在大纲视图和属性之间分配高度。",
     "ドラッグしてアウトライナーとプロパティの高さを配分します。", "Arrastra para repartir la altura entre el Outliner y las Propiedades.",
     "Tažením rozdělíte výšku mezi Outliner a Vlastnosti."),
    # --- Werkzeuge der Leiste ---
    ("BzAuswaehlen", "Select", "Auswählen", "选择", "選択", "Seleccionar", "Vybrat"),
    ("BzAuswaehlenHint", "Select with a click or by dragging a frame.", "Wählt mit einem Klick oder mit einem gezogenen Rahmen.",
     "单击或拖出选框进行选择。", "クリックまたはドラッグした枠で選択します。", "Selecciona con un clic o arrastrando un marco.",
     "Vybírá kliknutím nebo taženým rámečkem."),
    ("BzBewegen", "Move", "Bewegen", "移动", "移動", "Mover", "Posunout"),
    ("BzBewegenHint", "Shows the move gizmo at the selection; G moves without it.", "Zeigt das Gizmo zum Bewegen an der Auswahl; G bewegt ohne Gizmo.",
     "在所选内容处显示移动操纵器; 按 G 无需操纵器即可移动。", "選択に移動ギズモを表示します。G でギズモなしで移動します。",
     "Muestra el gizmo de mover en la selección; G mueve sin él.", "Ukáže u výběru manipulátor pro posun; G posouvá bez něj."),
    ("BzDrehen", "Rotate", "Drehen", "旋转", "回転", "Rotar", "Otočit"),
    ("BzDrehenHint", "Shows the rotate gizmo at the selection; R rotates without it.", "Zeigt das Gizmo zum Drehen an der Auswahl; R dreht ohne Gizmo.",
     "在所选内容处显示旋转操纵器; 按 R 无需操纵器即可旋转。", "選択に回転ギズモを表示します。R でギズモなしで回転します。",
     "Muestra el gizmo de rotar en la selección; R rota sin él.", "Ukáže u výběru manipulátor pro otáčení; R otáčí bez něj."),
    ("BzSkalieren", "Scale", "Skalieren", "缩放", "拡大縮小", "Escalar", "Změnit měřítko"),
    ("BzSkalierenHint", "Shows the scale gizmo at the selection; S scales without it.", "Zeigt das Gizmo zum Skalieren an der Auswahl; S skaliert ohne Gizmo.",
     "在所选内容处显示缩放操纵器; 按 S 无需操纵器即可缩放。", "選択に拡大縮小ギズモを表示します。S でギズモなしで拡大縮小します。",
     "Muestra el gizmo de escalar en la selección; S escala sin él.", "Ukáže u výběru manipulátor pro měřítko; S mění měřítko bez něj."),
    ("BzFase", "Bevel", "Fase", "倒角", "ベベル", "Bisel", "Zkosení"),
    ("BzMesser", "Knife", "Messer", "切刀", "ナイフ", "Cuchillo", "Nůž"),
    ("BzMessen", "Measure", "Messen", "测量", "メジャー", "Medir", "Měřit"),
    ("BzMessenHint", "Click two points on the model, or drag from one to the other: the distance in game units appears in the view and in the sidebar (N).",
     "Zwei Punkte auf dem Modell anklicken oder von einem zum anderen ziehen: der Abstand in Spieleinheiten steht in der Ansicht und in der Seitenleiste (N).",
     "在模型上单击两个点, 或从一点拖到另一点: 以游戏单位表示的距离显示在视图和侧栏 (N) 中。",
     "モデル上の 2 点をクリックするか、一方から他方へドラッグします。ゲーム単位の距離がビューとサイドバー (N) に表示されます。",
     "Haz clic en dos puntos del modelo o arrastra de uno a otro: la distancia en unidades del juego aparece en la vista y en la barra lateral (N).",
     "Klikněte na dva body modelu nebo táhněte z jednoho do druhého: vzdálenost v herních jednotkách se ukáže v pohledu a v postranním panelu (N)."),
    ("BzMessenMax", "Measure distance", "Abstand messen", "测量距离", "距離を測定", "Medir distancia", "Změřit vzdálenost"),
    ("BzFlaecheWaehlen", "Select surface", "Fläche wählen", "选择面", "サーフェスを選択", "Seleccionar superficie", "Vybrat plochu"),
    ("BzFlaecheWaehlenHint", "A click in the view selects the surface under the mouse (as in ModView).",
     "Ein Klick in die Ansicht wählt die Fläche unter der Maus (wie in ModView).", "在视图中单击即可选择鼠标下的面 (与 ModView 相同)。",
     "ビューをクリックするとマウスの下のサーフェスを選択します (ModView と同じ)。", "Un clic en la vista selecciona la superficie bajo el ratón (como en ModView).",
     "Kliknutí do pohledu vybere plochu pod myší (jako v ModView)."),
    ("BzKnochenWaehlen", "Select bone", "Knochen wählen", "选择骨骼", "ボーンを選択", "Seleccionar hueso", "Vybrat kost"),
    ("BzKnochenWaehlenHint", "Shows the bones; a click near a joint selects its bone (it is selected in the Outliner as well).",
     "Zeigt die Knochen; ein Klick nahe an einem Gelenk wählt seinen Knochen (auch im Outliner).", "显示骨骼; 在关节附近单击即可选择其骨骼 (大纲视图中也会选中)。",
     "ボーンを表示します。関節の近くをクリックするとそのボーンを選択します (アウトライナーでも選択されます)。",
     "Muestra los huesos; un clic cerca de una articulación selecciona su hueso (también en el Outliner).",
     "Ukáže kosti; kliknutí blízko kloubu vybere jeho kost (i v Outlineru)."),
    ("BzMessenErster", "Measure: click the first point on the model", "Messen: den ersten Punkt auf dem Modell anklicken", "测量: 单击模型上的第一个点",
     "メジャー: モデル上の最初の点をクリック", "Medir: haz clic en el primer punto del modelo", "Měření: klikněte na první bod modelu"),
    ("BzMessenZweiter", "Measure: click the second point", "Messen: den zweiten Punkt anklicken", "测量: 单击第二个点", "メジャー: 2 番目の点をクリック",
     "Medir: haz clic en el segundo punto", "Měření: klikněte na druhý bod"),
    ("BzDaneben", "Not on the model - click on a surface.", "Nicht auf dem Modell - auf eine Fläche klicken.", "不在模型上 - 请单击一个面。",
     "モデル上ではありません - サーフェスをクリックしてください。", "No está sobre el modelo: haz clic en una superficie.", "Není na modelu - klikněte na plochu."),
    ("BzEinheiten", "%.1f units", "%.1f Einheiten", "%.1f 单位", "%.1f ユニット", "%.1f unidades", "%.1f jednotek"),
    # --- Modus im Kopf der Ansicht (wie Blender: Object Mode, Edit Mode, Weight Paint, Texture Paint, Pose Mode) ---
    ("BzModusWeight", "Weight Paint", "Weight Paint", "权重绘制", "ウェイトペイント", "Pintura de pesos", "Malování vah"),
    ("BzModusTexture", "Texture Paint", "Texture Paint", "纹理绘制", "テクスチャペイント", "Pintura de texturas", "Malování textur"),
    ("BzModusPose", "Pose Mode", "Pose-Modus", "姿态模式", "ポーズモード", "Modo Pose", "Režim pózy"),
    ("BzModusHint", "The mode of the view, like Blender (Ctrl+Tab): it chooses the area - Object Mode View/Check, Edit Mode Modeling (UV in the area UV), Weight Paint, Texture Paint, Pose Mode Animation.",
     "Der Modus der Ansicht wie Blender (Strg+Tab): er wählt den Bereich - Objektmodus Ansehen/Prüfen, Bearbeitungsmodus Modellieren (UV im Bereich UV), Weight Paint, Texture Paint, Pose-Modus Animation.",
     "视图的模式, 与 Blender 相同 (Ctrl+Tab): 它决定区域 - 物体模式为查看/检查, 编辑模式为建模 (UV 区域中为 UV), 权重绘制、纹理绘制, 姿态模式为动画。",
     "Blender と同じビューのモード (Ctrl+Tab): エリアを選びます - オブジェクトモードは表示/チェック、編集モードはモデリング (UV エリアでは UV)、ウェイトペイント、テクスチャペイント、ポーズモードはアニメーション。",
     "El modo de la vista, como Blender (Ctrl+Tab): elige el área - Modo Objeto Ver/Comprobar, Modo Edición Modelado (UV en el área UV), Pintura de pesos, Pintura de texturas, Modo Pose Animación.",
     "Režim pohledu jako v Blenderu (Ctrl+Tab): volí oblast - Objektový režim Prohlížet/Kontrola, Editační režim Modelování (UV v oblasti UV), Malování vah, Malování textur, Režim pózy Animace."),
    # --- Seitenleiste ---
    ("BzReiterObjekt", "Item", "Objekt", "项", "アイテム", "Elemento", "Položka"),
    ("BzReiterAnsicht", "View", "Ansicht", "视图", "ビュー", "Vista", "Pohled"),
    ("BzTransform", "Transform", "Transformieren", "变换", "トランスフォーム", "Transformar", "Transformace"),
    ("BzOrt", "Location", "Position", "位置", "位置", "Ubicación", "Umístění"),
    ("BzDrehung", "Rotation", "Drehung", "旋转", "回転", "Rotación", "Rotace"),
    ("BzMasse", "Dimensions", "Abmessungen", "尺寸", "寸法", "Dimensiones", "Rozměry"),
    ("BzGewaehlt", "Selected", "Gewählt", "已选择", "選択中", "Seleccionado", "Vybráno"),
    ("BzNichtsGewaehlt", "nothing selected", "nichts gewählt", "未选择任何内容", "選択なし", "nada seleccionado", "nic nevybráno"),
    ("BzFlaechen", "%d {surface|surfaces}", "%d {Fläche|Flächen}", "%d 个面", "サーフェス %d", "%d {superficie|superficies}", "%d {plocha|plochy|ploch}"),
    ("BzKnochen", "Bone %s", "Knochen %s", "骨骼 %s", "ボーン %s", "Hueso %s", "Kost %s"),
    ("BzGriff", "Control %s", "Griff %s", "控制器 %s", "コントロール %s", "Control %s", "Ovladač %s"),
    ("BzAbstand", "Distance", "Abstand", "距离", "距離", "Distancia", "Vzdálenost"),
    ("BzKeineMessung", "Measure with the tool in the toolbar (View/Check).", "Mit dem Werkzeug in der Werkzeugleiste messen (Ansehen/Prüfen).",
     "使用工具栏中的工具进行测量 (查看/检查)。", "ツールバーのツールで測定します (表示/チェック)。", "Mide con la herramienta de la barra de herramientas (Ver/Comprobar).",
     "Měřte nástrojem v panelu nástrojů (Prohlížet/Kontrola)."),
    ("BzSichtfeld", "Field of view", "Sichtfeld", "视野", "視野角", "Campo de visión", "Zorné pole"),
    ("BzAnsichtArt", "Projection", "Projektion", "投影", "投影", "Proyección", "Projekce"),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
