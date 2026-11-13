"""Modell-Block 1a-1: die Seite "Model-Tab: Ansehen und Pruefen" (Dokument, Gestell, Bereich Ansehen/Pruefen wie ModView,
Bedienung wie 3ds Max).

Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_modobj.py: die ersten vier gehen in
T(...), Spanisch und Tschechisch nach inhalt_es.py / inhalt_cs.py (ES, CS).
"""

import inhalt_mbkopf  # noqa: E402  (MBKOPF: Kopfzeile der Ansicht, ViewCube und Gizmo)
import inhalt_mbraster  # noqa: E402  (MBRASTER: Boden-Raster wie 3ds Max / Blender)
import inhalt_mbnav  # noqa: E402  (MBNAV: Navigation in den Ansichten wie 3ds Max / Blender)

X = {
    "titel": (
        "Model tab: overview - views, explorer and panel",
        "Model-Tab: Überblick - Ansichten, Explorer und Feld",
        "模型选项卡: 概览 - 视图、资源管理器和面板",
        "モデルタブ: 概要 - ビュー、エクスプローラー、パネル",
        "Pestaña Model: vista general - vistas, explorador y panel",
        "Karta Model: přehled - pohledy, průzkumník a panel",
    ),
    "kurz": (
        "The Model tab at a glance: six areas for one model - viewing and checking, modelling, UVs, weights, texture painting and animation - with the same views, scene explorer, panel on the right and timeline in each of them. The model is drawn the way the game draws it; mouse, keys and layout follow 3ds Max or Blender, whichever you choose.",
        "Der Model-Tab im Überblick: sechs Bereiche für ein Modell - Ansehen und Prüfen, Modellieren, UVs, Gewichte, Texturen malen und Animation - mit denselben Ansichten, demselben Szenen-Explorer, dem Feld rechts und der Zeitleiste in jedem Bereich. Das Modell wird gezeichnet, wie das Spiel es zeichnet; Maus, Tasten und Aufbau folgen 3ds Max oder Blender, je nach deiner Wahl.",
        "模型选项卡概览: 一个模型的六个区域 - 查看与检查、建模、UV、权重、纹理绘制和动画 - 每个区域都有相同的视图、场景资源管理器、右侧面板和时间轴。模型按游戏的方式绘制; 鼠标、按键和布局按你的选择与 3ds Max 或 Blender 相同。",
        "モデルタブの概要: 1 つのモデルに 6 つのエリア - 表示とチェック、モデリング、UV、ウェイト、テクスチャペイント、アニメーション - があり、どのエリアにも同じビュー、シーンエクスプローラー、右側のパネル、タイムラインがあります。モデルはゲームと同じ方法で描画され、マウス、キー、レイアウトは選んだ操作モードに応じて 3ds Max または Blender と同じです。",
        "La pestaña Model de un vistazo: seis áreas para un modelo - ver y comprobar, modelar, UV, pesos, pintar texturas y animación - con las mismas vistas, el mismo explorador de escena, el panel de la derecha y la línea de tiempo en cada una. El modelo se dibuja como lo dibuja el juego; el ratón, las teclas y la disposición siguen a 3ds Max o a Blender, según elijas.",
        "Karta Model v kostce: šest oblastí pro jeden model - prohlížení a kontrola, modelování, UV, váhy, malování textur a animace - a v každé stejné pohledy, průzkumník scény, panel vpravo a časová osa. Model se kreslí tak, jak ho kreslí hra; myš, klávesy a rozvržení se řídí 3ds Max nebo Blenderem, podle vaší volby.",
    ),
    "bild": (
        "The stormtrooper with a saber at *r_hand: on the left the scene explorer, in the middle the view, on the right the command panel with its groups, below the timeline.",
        "Der Sturmtruppler mit einem Säbel an *r_hand: links der Szenen-Explorer, in der Mitte die Ansicht, rechts das Befehlsfeld mit seinen Gruppen, unten die Zeitleiste.",
        "手持光剑 (*r_hand) 的冲锋队员: 左侧是场景资源管理器, 中间是视图, 右侧是带各分组的命令面板, 下方是时间轴。",
        "*r_hand にセイバーを付けたストームトルーパー。左にシーンエクスプローラー、中央にビュー、右にグループの並ぶコマンドパネル、下にタイムライン。",
        "El soldado de asalto con un sable en *r_hand: a la izquierda el explorador de escena, en el centro la vista, a la derecha el panel de comandos con sus grupos, abajo la línea de tiempo.",
        "Stormtrooper s mečem na *r_hand: vlevo průzkumník scény, uprostřed pohled, vpravo panel příkazů se skupinami, dole časová osa.",
    ),
    "aufbau": (
        "Open a model as before ({{NpcBrowser}}, `.glm`, `.pk3`). The tab starts in **{{MbBereichAnsehen}}** ([[modell-pruefen]]); the row of areas above the views switches to **{{MbBereichModellieren}}** ([[modell-modellieren]]), **{{MbBereichUv}}** ([[modell-uv]]), **{{MbBereichWeights}}** ([[modell-weights]]), **{{MbBereichPainting}}** ([[modell-painting]]) and **{{MbBereichAnimation}}** ([[modell-animationsfeld]]). Views, scene explorer and timeline ([[modell-zeitleiste]]) are the same in every area, and the pose keeps playing while you work. The model is loaded into its own document with every level of detail; drawing an unchanged model gives exactly the same picture as before (checked with every model under `models/players` of Jedi Academy, Jedi Outcast and Movie Duels).",
        "Ein Modell öffnest du wie bisher ({{NpcBrowser}}, `.glm`, `.pk3`). Der Tab beginnt in **{{MbBereichAnsehen}}** ([[modell-pruefen]]); die Reihe der Bereiche über den Ansichten wechselt nach **{{MbBereichModellieren}}** ([[modell-modellieren]]), **{{MbBereichUv}}** ([[modell-uv]]), **{{MbBereichWeights}}** ([[modell-weights]]), **{{MbBereichPainting}}** ([[modell-painting]]) und **{{MbBereichAnimation}}** ([[modell-animationsfeld]]). Ansichten, Szenen-Explorer und Zeitleiste ([[modell-zeitleiste]]) sind in jedem Bereich dieselben, und die Pose läuft weiter, während du arbeitest. Das Modell kommt mit allen Detailstufen in ein eigenes Dokument; ein unverändertes Modell ergibt genau dasselbe Bild wie vorher (geprüft mit jedem Modell unter `models/players` von Jedi Academy, Jedi Outcast und Movie Duels).",
        "像以前一样打开模型({{NpcBrowser}}、`.glm`、`.pk3`)。选项卡从 **{{MbBereichAnsehen}}** 开始 ([[modell-pruefen]]); 视图上方的区域栏可切换到 **{{MbBereichModellieren}}** ([[modell-modellieren]])、**{{MbBereichUv}}** ([[modell-uv]])、**{{MbBereichWeights}}** ([[modell-weights]])、**{{MbBereichPainting}}** ([[modell-painting]]) 和 **{{MbBereichAnimation}}** ([[modell-animationsfeld]])。视图、场景资源管理器和时间轴 ([[modell-zeitleiste]]) 在每个区域都相同, 工作时姿势继续播放。模型连同所有细节级别载入自己的文档;未修改的模型绘制出的画面与以前完全相同(已用 Jedi Academy、Jedi Outcast 和 Movie Duels 中 `models/players` 下的每个模型检查)。",
        "モデルはこれまでどおり開きます ({{NpcBrowser}}、`.glm`、`.pk3`)。タブは **{{MbBereichAnsehen}}** ([[modell-pruefen]]) で始まります。ビューの上に並ぶエリアの行で **{{MbBereichModellieren}}** ([[modell-modellieren]])、**{{MbBereichUv}}** ([[modell-uv]])、**{{MbBereichWeights}}** ([[modell-weights]])、**{{MbBereichPainting}}** ([[modell-painting]])、**{{MbBereichAnimation}}** ([[modell-animationsfeld]]) に切り替えます。ビュー、シーンエクスプローラー、タイムライン ([[modell-zeitleiste]]) はどのエリアでも同じで、作業中もポーズは再生され続けます。モデルはすべての詳細レベルと一緒に専用のドキュメントに読み込まれ、変更していないモデルは以前とまったく同じ画像になります (Jedi Academy、Jedi Outcast、Movie Duels の `models/players` にある全モデルで確認)。",
        "Abre un modelo como antes ({{NpcBrowser}}, `.glm`, `.pk3`). La pestaña empieza en **{{MbBereichAnsehen}}** ([[modell-pruefen]]); la fila de áreas sobre las vistas cambia a **{{MbBereichModellieren}}** ([[modell-modellieren]]), **{{MbBereichUv}}** ([[modell-uv]]), **{{MbBereichWeights}}** ([[modell-weights]]), **{{MbBereichPainting}}** ([[modell-painting]]) y **{{MbBereichAnimation}}** ([[modell-animationsfeld]]). Las vistas, el explorador de escena y la línea de tiempo ([[modell-zeitleiste]]) son los mismos en cada área, y la pose sigue reproduciéndose mientras trabajas. El modelo se carga en su propio documento con todos los niveles de detalle; un modelo sin cambios da exactamente la misma imagen que antes (comprobado con cada modelo de `models/players` de Jedi Academy, Jedi Outcast y Movie Duels).",
        "Model otevřete jako dřív ({{NpcBrowser}}, `.glm`, `.pk3`). Karta začíná v **{{MbBereichAnsehen}}** ([[modell-pruefen]]); řada oblastí nad pohledy přepíná na **{{MbBereichModellieren}}** ([[modell-modellieren]]), **{{MbBereichUv}}** ([[modell-uv]]), **{{MbBereichWeights}}** ([[modell-weights]]), **{{MbBereichPainting}}** ([[modell-painting]]) a **{{MbBereichAnimation}}** ([[modell-animationsfeld]]). Pohledy, průzkumník scény a časová osa ([[modell-zeitleiste]]) jsou v každé oblasti stejné a póza běží dál, zatímco pracujete. Model se načte do vlastního dokumentu se všemi úrovněmi detailu; nezměněný model dává přesně stejný obraz jako dřív (ověřeno s každým modelem v `models/players` z Jedi Academy, Jedi Outcast a Movie Duels).",
    ),
    "h_explorer": ("Scene explorer", "Szenen-Explorer", "场景资源管理器", "シーンエクスプローラー", "Explorador de escena", "Průzkumník scény"),
    "explorer": (
        "The **{{MbExplorer}}** shows the surfaces (**{{MbNachHierarchie}}** or **{{MbNachEbenen}}**), the tags and the bones of the skeleton; "
        "**{{MbSuche}}** filters by name. Grey surfaces are not drawn. A right-click works like in ModView and like the game "
        "(`G2_SetSurfaceOnOff`, `G2_SetRootSurface`): **{{MbFlAus}}** hides the surface but keeps its children, **{{MbFlAusOhneKinder}}** hides "
        "the whole branch, **{{MbFlWurzel}}** draws only this branch, **{{MbFlUrsprung}}** undoes it all. A skin line `name,*off` switches a "
        "surface off the same way. Bones with an orange dot are looked up by the game code.",
        "Der **{{MbExplorer}}** zeigt die Flächen (**{{MbNachHierarchie}}** oder **{{MbNachEbenen}}**), die Tags und die Knochen des Skeletts; "
        "**{{MbSuche}}** filtert nach Namen. Graue Flächen werden nicht gezeichnet. Ein Rechtsklick wirkt wie in ModView und wie im Spiel "
        "(`G2_SetSurfaceOnOff`, `G2_SetRootSurface`): **{{MbFlAus}}** blendet die Fläche aus, ihre Kinder bleiben, **{{MbFlAusOhneKinder}}** "
        "blendet den ganzen Zweig aus, **{{MbFlWurzel}}** zeichnet nur diesen Zweig, **{{MbFlUrsprung}}** nimmt alles zurück. Eine Skin-Zeile "
        "`name,*off` schaltet eine Fläche genauso ab. Knochen mit orangem Punkt sucht der Spielcode.",
        "**{{MbExplorer}}** 显示面(**{{MbNachHierarchie}}** 或 **{{MbNachEbenen}}**)、标记和骨架的骨骼;**{{MbSuche}}** 按名称过滤。灰色的面不会"
        "被绘制。右键单击的作用与 ModView 和游戏相同 (`G2_SetSurfaceOnOff`、`G2_SetRootSurface`):**{{MbFlAus}}** 隐藏该面但保留其子级,"
        "**{{MbFlAusOhneKinder}}** 隐藏整个分支,**{{MbFlWurzel}}** 只绘制此分支,**{{MbFlUrsprung}}** 全部撤销。皮肤中的 `name,*off` 行也以同样方式"
        "关闭一个面。带橙色圆点的骨骼会被游戏代码查找。",
        "**{{MbExplorer}}** にはサーフェス (**{{MbNachHierarchie}}** または **{{MbNachEbenen}}**)、タグ、スケルトンのボーンが表示されます。"
        "**{{MbSuche}}** で名前を絞り込みます。灰色のサーフェスは描画されません。右クリックは ModView やゲームと同じように働きます "
        "(`G2_SetSurfaceOnOff`、`G2_SetRootSurface`)。**{{MbFlAus}}** はサーフェスを隠して子は残し、**{{MbFlAusOhneKinder}}** は枝全体を隠し、"
        "**{{MbFlWurzel}}** はこの枝だけを描画し、**{{MbFlUrsprung}}** ですべて元に戻します。スキンの `name,*off` 行も同じようにサーフェスを"
        "オフにします。オレンジの点が付いたボーンはゲームコードが参照します。",
        "El **{{MbExplorer}}** muestra las superficies (**{{MbNachHierarchie}}** o **{{MbNachEbenen}}**), los tags y los huesos del esqueleto; "
        "**{{MbSuche}}** filtra por nombre. Las superficies grises no se dibujan. El clic derecho funciona como en ModView y como en el juego "
        "(`G2_SetSurfaceOnOff`, `G2_SetRootSurface`): **{{MbFlAus}}** oculta la superficie pero mantiene sus hijas, **{{MbFlAusOhneKinder}}** "
        "oculta toda la rama, **{{MbFlWurzel}}** dibuja solo esa rama, **{{MbFlUrsprung}}** lo deshace todo. Una línea de skin `name,*off` "
        "apaga una superficie de la misma forma. Los huesos con un punto naranja los busca el código del juego.",
        "**{{MbExplorer}}** ukazuje povrchy (**{{MbNachHierarchie}}** nebo **{{MbNachEbenen}}**), tagy a kosti kostry; **{{MbSuche}}** filtruje "
        "podle názvu. Šedé povrchy se nekreslí. Pravé tlačítko funguje jako v ModView a jako ve hře (`G2_SetSurfaceOnOff`, `G2_SetRootSurface`): "
        "**{{MbFlAus}}** skryje povrch, jeho potomci zůstanou, **{{MbFlAusOhneKinder}}** skryje celou větev, **{{MbFlWurzel}}** kreslí jen tuto "
        "větev, **{{MbFlUrsprung}}** vše vrátí. Řádek skinu `name,*off` vypne povrch stejně. Kosti s oranžovou tečkou hledá kód hry."),
    "bild_menue": ("The right-click menu of a surface.", "Das Rechtsklick-Menü einer Fläche.", "面的右键菜单。", "サーフェスの右クリックメニュー。",
                   "El menú contextual de una superficie.", "Kontextová nabídka povrchu."),
    "anbauen": (
        "Right-click a tag (for example `*r_hand`) or a bone and choose **{{MbAnbauen}}** to attach a model from the game archives - a saber, a weapon, a head - like `G2API_AttachG2Model`: on a tag it follows the tag triangle, on a bone the bone. The group **{{MbRAnbauten}}** lists them and detaches them again.",
        "Rechtsklick auf einen Tag (etwa `*r_hand`) oder einen Knochen und **{{MbAnbauen}}** baut ein Modell aus den Spielarchiven an - einen Säbel, eine Waffe, einen Kopf - wie `G2API_AttachG2Model`: an einem Tag folgt es dem Tag-Dreieck, an einem Knochen dem Knochen. Die Gruppe **{{MbRAnbauten}}** listet sie und löst sie wieder.",
        "右键单击标记(例如 `*r_hand`)或骨骼并选择 **{{MbAnbauen}}**,即可从游戏存档附加模型——光剑、武器、头部——与 `G2API_AttachG2Model` 相同:附加在标记上时跟随标记三角形,附加在骨骼上时跟随骨骼。分组 **{{MbRAnbauten}}** 列出它们并可再次分离。",
        "タグ (たとえば `*r_hand`) かボーンを右クリックして **{{MbAnbauen}}** を選ぶと、ゲームアーカイブからモデル (セイバー、武器、頭など) を `G2API_AttachG2Model` と同じように取り付けます。タグに付けるとタグの三角形に、ボーンに付けるとボーンに従います。グループ **{{MbRAnbauten}}** に一覧が出て、取り外せます。",
        "Haz clic derecho en un tag (por ejemplo `*r_hand`) o un hueso y elige **{{MbAnbauen}}** para acoplar un modelo de los archivos del juego (un sable, un arma, una cabeza) como `G2API_AttachG2Model`: en un tag sigue el triángulo del tag, en un hueso sigue el hueso. El grupo **{{MbRAnbauten}}** los muestra y los vuelve a desacoplar.",
        "Pravým tlačítkem na tag (třeba `*r_hand`) nebo kost a **{{MbAnbauen}}** připojí model z archivů hry - meč, zbraň, hlavu - jako `G2API_AttachG2Model`: na tagu sleduje trojúhelník tagu, na kosti kost. Skupina **{{MbRAnbauten}}** je vypíše a znovu odpojí.",
    ),
    "h_befehl": ("Command panel", "Befehlsfeld", "命令面板", "コマンドパネル", "Panel de comandos", "Panel příkazů"),
    "sequenzen": (
        "The secondary sequence plays on the upper body from `lower_lumbar` up, like `SET_ANIM_UPPER` in the game (another start bone: right-click a bone). **{{MbInterpolation}}** blends between two frames like the game.",
        "Die sekundäre Sequenz spielt auf dem Oberkörper ab `lower_lumbar`, wie `SET_ANIM_UPPER` im Spiel (ein anderer Start-Knochen: Rechtsklick auf einen Knochen). **{{MbInterpolation}}** mischt zwischen zwei Bildern wie das Spiel.",
        "次要序列像游戏中的 `SET_ANIM_UPPER` 一样在 `lower_lumbar` 以上的上半身播放 (其他起始骨骼: 右键单击骨骼)。**{{MbInterpolation}}** 像游戏一样在两帧之间混合。",
        "セカンダリシーケンスは、ゲームの `SET_ANIM_UPPER` と同じく `lower_lumbar` から上の上半身で再生されます (別の開始ボーン: ボーンを右クリック)。**{{MbInterpolation}}** はゲームと同じく 2 つのフレームの間を補間します。",
        "La secuencia secundaria se reproduce en el torso desde `lower_lumbar`, como `SET_ANIM_UPPER` en el juego (otro hueso de inicio: clic derecho en un hueso). **{{MbInterpolation}}** mezcla entre dos fotogramas como el juego.",
        "Sekundární sekvence se přehrává na horní části těla od `lower_lumbar`, jako `SET_ANIM_UPPER` ve hře (jiná počáteční kost: pravé tlačítko na kost). **{{MbInterpolation}}** prolíná mezi dvěma snímky jako hra.",
    ),
    "auge": (
        "Every row of the scene explorer shows what it is: model, tag, bone, root bone, attachment, rig handle, sequence; a surface shows its state in the game (dot = on, ring = off, struck ring = off with its children). A click on the eye hides a row together with its children, Ctrl or Alt+click shows only this one; **{{MbAllesEinblenden}}** is in the right-click menu and in the action search. Hidden rows are greyed out; selecting, painting and the photo pass them by. Like Blender: H, Alt+H, Shift+H.",
        "Jede Zeile des Szenen-Explorers zeigt, was sie ist: Modell, Tag, Knochen, Wurzelknochen, Anbau, Rig-Griff, Sequenz; eine Fläche zeigt ihren Status im Spiel (Punkt = an, Ring = aus, durchgestrichener Ring = aus samt Kindern). Ein Klick auf das Auge blendet eine Zeile samt ihren Kindern aus, Strg- oder Alt+Klick zeigt nur diese; **{{MbAllesEinblenden}}** steht im Rechtsklick-Menü und in der Aktionssuche. Ausgeblendete Zeilen sind grau; Auswählen, Malen und das Foto gehen an ihnen vorbei. Wie Blender: H, Alt+H, Umschalt+H.",
        "场景资源管理器的每一行都显示其类型:模型、标记、骨骼、根骨骼、附件、绑定控制柄、序列;LOD 位于面的标题中。面前面的标记显示其在游戏中的状态 (圆点 = 开, 圆环 = 关, 带斜线的圆环 = 连同子级一起关)。单击眼睛会连同子级一起隐藏该行,Ctrl 或 Alt+单击只显示这一个;**{{MbAllesEinblenden}}** 在右键菜单和操作搜索中。隐藏的行显示为灰色;选择、绘制和照片会跳过它们。像 Blender 一样:H、Alt+H、Shift+H。",
        "シーンエクスプローラーの各行は種類を示します: モデル、タグ、ボーン、ルートボーン、アタッチ、リグのハンドル、シーケンス。LOD はサーフェスの見出しにあります。サーフェスの前の印はゲーム内の状態を示します (点 = オン、輪 = オフ、斜線付きの輪 = 子ごとオフ)。目をクリックすると行を子と一緒に非表示にし、Ctrl か Alt+クリックでその行だけを表示します。**{{MbAllesEinblenden}}** は右クリックメニューと操作検索にあります。非表示の行は灰色で、選択・ペイント・写真はそれを飛ばします。Blender と同じ: H、Alt+H、Shift+H。",
        "Cada fila del explorador de escena muestra lo que es: modelo, tag, hueso, hueso raíz, acoplado, control del rig, secuencia; una superficie muestra su estado en el juego (punto = activada, anillo = desactivada, anillo tachado = desactivada con sus hijas). Un clic en el ojo oculta una fila con sus hijos, Ctrl o Alt+clic muestra solo esa; **{{MbAllesEinblenden}}** está en el menú del clic derecho y en la búsqueda de acciones. Las filas ocultas aparecen en gris; seleccionar, pintar y la foto las saltan. Como Blender: H, Alt+H, Mayús+H.",
        "Každý řádek průzkumníku scény ukazuje, co je zač: model, tag, kost, kořenová kost, přídavek, úchyt rigu, sekvence; povrch ukazuje svůj stav ve hře (tečka = zapnuto, kroužek = vypnuto, přeškrtnutý kroužek = vypnuto i s potomky). Klik na oko skryje řádek i s potomky, Ctrl nebo Alt+klik ukáže jen tento; **{{MbAllesEinblenden}}** je v nabídce pravého tlačítka a ve hledání akcí. Skryté řádky jsou šedé; výběr, malování a fotka je přeskočí. Jako Blender: H, Alt+H, Shift+H.",
    ),
    "bild_auge": (
        "The scene explorer with its symbols; the head is hidden with the eye - in the game it stays.",
        "Der Szenen-Explorer mit seinen Symbolen; der Kopf ist mit dem Auge ausgeblendet - im Spiel bleibt er.",
        "带符号的场景资源管理器;头部用眼睛隐藏——在游戏中它仍然存在。",
        "記号付きのシーンエクスプローラー。頭は目で非表示 - ゲームでは残ります。",
        "El explorador de escena con sus símbolos; la cabeza está oculta con el ojo; en el juego sigue ahí.",
        "Průzkumník scény se symboly; hlava je skrytá okem - ve hře zůstává.",
    ),
}

ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}


def _t(schluessel):
    from inhalt_basis import T
    t = X[schluessel]
    return T(t[0], t[1], t[2], t[3])


def seite():
    """Die Seite modell-ansehen (Gruppe Missionen, nach "Modellansicht und Figuren")."""
    return {
        "id": "modell-ansehen",
        "gruppe": "mission",
        "titel": _t("titel"),
        "kurz": _t("kurz"),
        "bloecke": [
            ("img", "h_mb1_ansehen", _t("bild"), "breit"),
            ("p", _t("aufbau")),
            ("h2", "explorer", _t("h_explorer")),
            ("p", _t("explorer")),
            ("img", "h_mb1_menue", _t("bild_menue")),
            ("p", _t("anbauen")),
            # hier haengt inhalt_ap2 den Baum, die Zeile je Bedienmodus und die Ebenen ein (hinter "anbauen")
            ("p", _t("sequenzen")),
            ("p", _t("auge")),
            ("img", "h_mb2_auge", _t("bild_auge"), "breit"),
            ("h2", "befehlsfeld", _t("h_befehl")),
            # hier haengt inhalt_ap3 das rechte Feld ein; die Gruppen des Bereichs Ansehen/Pruefen und die Pruefung fuer das Spiel
            # stehen auf der Seite modell-pruefen (inhalt_ap5), die Tasten wie 3ds Max auf modell-bedienung (inhalt_mb1a2)
        ] + inhalt_mbkopf.bloecke() + inhalt_mbraster.bloecke() + inhalt_mbnav.bloecke(),
    }
