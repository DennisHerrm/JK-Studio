"""ENTSCHEID3A (12.11., Entscheidungen B7-B13 und C4 aus plaene/entscheidungen_1211.md): Handbuch des Model-Tabs.

Bausteine hinter Absaetzen anderer Quellen (Register von AP14, tools/handbuch/inhalt_modell.py, je eine Zeile in QUELLEN):
    Hinter("inhalt_ent3a", "bloecke_explorer", "modell-ansehen", "inhalt_ap2", "ebenen"),     # B7 eingefrorene Zeile waehlbar
    Hinter("inhalt_ent3a", "bloecke_bedienung", "modell-bedienung", "inhalt_mb1a2", "b_text"), # C4 Esc schliesst das Rechtsklick-Menue
    Hinter("inhalt_ent3a", "bloecke_leiste", "modell-blender", "inhalt_ap4", "gruppen"),       # B8 Leiste breiter ziehen
    Hinter("inhalt_ent3a", "bloecke_seite", "modell-blender", "inhalt_ap4", "seite"),          # B9 Seitenleiste bei Knochen
    Hinter("inhalt_ent3a", "bloecke_stapel", "modell-modellieren", "inhalt_ap13s", "bedienen"),# B11, B12, B13 im Modifier-Stapel
B10 (Strg+Umschalt+Z) steht im Absatz "m5" von inhalt_mbnav.py (dort berichtigt). Die Oberflaechentexte ({{Ea...}}) kommen aus
tools/i18n_ent3a.py (in tools/gen_i18n.py eingetragen). Jeder Text in sechs Sprachen (en, de, zh, ja, es, cs).
Bilder: handbuch/bilder/<sprache>/h_ent3a_explorer.webp, h_ent3a_leiste.webp, h_ent3a_seite.webp, h_ent3a_stapel.webp
(Selbsttest BHED_EDITORTEST=modellblock, BHED_MODELL_NUR=ent3ahb, je Sprache; Y:\\werkzeuge\\ent3a\\hb.sh).
"""

X = {
    # --- B7 ---
    "explorer": (
        "**Frozen rows in the Scene Explorer (3ds Max mode):** a click on a frozen row selects it, as in the Scene Explorer of 3ds Max - "
        "for example to unfreeze it with **{{EaAuswahlAuftauen}}** in the right-click menu of the row or in Search all actions. In the "
        "view a click still goes through a frozen surface and selects the one behind it. In Blender mode a row whose arrow "
        "\"selectable\" is off stays unselectable, as before.",
        "**Eingefrorene Zeilen im Szenen-Explorer (3ds-Max-Modus):** ein Klick auf eine eingefrorene Zeile wählt sie, wie im Scene "
        "Explorer von 3ds Max - etwa um sie mit **{{EaAuswahlAuftauen}}** im Rechtsklick-Menü der Zeile oder in „Alle Aktionen suchen“ "
        "aufzutauen. In der Ansicht geht ein Klick weiter durch eine eingefrorene Fläche hindurch und wählt die dahinter. Im "
        "Blender-Modus bleibt eine Zeile, deren Pfeil „wählbar“ aus ist, wie bisher nicht wählbar.",
        "**场景资源管理器中的冻结行 (3ds Max 模式):** 单击冻结的行即可选择它, 与 3ds Max 的场景资源管理器相同 - 例如随后在该行的右键菜单或“搜索所有操作”中用 "
        "**{{EaAuswahlAuftauen}}** 解冻。在视图中单击仍会穿过冻结的面, 选中其后面的面。Blender 模式下, 箭头“可选择”关闭的行仍和以前一样不可选择。",
        "**シーン エクスプローラのフリーズした行 (3ds Max 風):** フリーズした行をクリックすると、3ds Max のシーン エクスプローラと同じく選択できます。"
        "たとえば行の右クリック メニューや「すべての操作を検索」の **{{EaAuswahlAuftauen}}** でフリーズを解除できます。ビューでのクリックは引き続き"
        "フリーズしたサーフェスを通り抜け、その後ろのものを選択します。Blender 風では「選択可能」の矢印がオフの行はこれまでどおり選択できません。",
        "**Filas congeladas en el Explorador de escena (modo 3ds Max):** un clic en una fila congelada la selecciona, como en el Scene "
        "Explorer de 3ds Max; por ejemplo, para descongelarla con **{{EaAuswahlAuftauen}}** en el menú contextual de la fila o en "
        "«Buscar todas las acciones». En la vista, un clic sigue atravesando una superficie congelada y selecciona la de detrás. En modo "
        "Blender, una fila con la flecha «seleccionable» desactivada sigue sin poder seleccionarse, como antes.",
        "**Zmrazené řádky v Průzkumníku scény (režim 3ds Max):** kliknutí na zmrazený řádek jej vybere jako ve Scene Exploreru 3ds Max - "
        "třeba pro rozmrazení pomocí **{{EaAuswahlAuftauen}}** v nabídce pravého tlačítka řádku nebo v „Hledat všechny akce“. V pohledu "
        "kliknutí dál prochází zmrazenou plochou a vybere plochu za ní. V režimu Blender zůstává řádek s vypnutou šipkou „vybratelné“ "
        "nevybratelný jako dosud."),
    "bild_explorer": (
        "3ds Max mode: the frozen surface torso selected in the Scene Explorer, right-click with Unfreeze selection",
        "3ds-Max-Modus: die eingefrorene Fläche torso im Szenen-Explorer gewählt, Rechtsklick mit „Auswahl auftauen“",
        "3ds Max 模式: 在场景资源管理器中选择了冻结的面 torso, 右键菜单中有“解冻所选内容”",
        "3ds Max 風: シーン エクスプローラでフリーズしたサーフェス torso を選択、右クリックに「選択をフリーズ解除」",
        "Modo 3ds Max: la superficie congelada torso seleccionada en el Explorador de escena, menú contextual con «Descongelar selección»",
        "Režim 3ds Max: zmrazená plocha torso vybraná v Průzkumníku scény, nabídka pravého tlačítka s „Rozmrazit výběr“"),
    # --- C4 ---
    "esc": (
        "<kbd>Esc</kbd> closes the right-click menu of the view, also from an open submenu, as in 3ds Max and Blender; nothing else "
        "happens with that key press.",
        "<kbd>Esc</kbd> schließt das Rechtsklick-Menü der Ansicht, auch aus einem offenen Untermenü, wie in 3ds Max und Blender; sonst "
        "geschieht mit diesem Tastendruck nichts.",
        "<kbd>Esc</kbd> 关闭视图的右键菜单 (包括打开的子菜单), 与 3ds Max 和 Blender 相同; 这次按键不会触发其他操作。",
        "<kbd>Esc</kbd> はビューの右クリック メニューを閉じます (開いているサブメニューからでも)。3ds Max や Blender と同じで、このキー操作で"
        "ほかには何も起きません。",
        "<kbd>Esc</kbd> cierra el menú contextual de la vista, también desde un submenú abierto, como en 3ds Max y Blender; esa pulsación "
        "no hace nada más.",
        "<kbd>Esc</kbd> zavře nabídku pravého tlačítka v pohledu, i z otevřené podnabídky, jako ve 3ds Max a Blenderu; nic jiného se "
        "tímto stiskem nestane."),
    # --- B8 ---
    "leiste": (
        "Like in Blender the toolbar can be dragged wider at its right edge: up to 120 points it shows only the symbols, wider it "
        "shows the tool names next to them. The width stays for the next start; a double-click on the edge brings back the narrow "
        "toolbar. **{{BeWerkzeugnamen}}** in the right-click menu still shows the names at any width.",
        "Wie in Blender lässt sich die Werkzeugleiste an ihrer rechten Kante breiter ziehen: bis 120 Punkte zeigt sie nur die Symbole, "
        "breiter die Werkzeugnamen daneben. Die Breite bleibt für den nächsten Start; ein Doppelklick auf die Kante bringt die schmale "
        "Leiste zurück. **{{BeWerkzeugnamen}}** im Rechtsklick-Menü zeigt die Namen weiter in jeder Breite.",
        "与 Blender 一样, 可以在工具栏右边缘拖动将其加宽: 宽度不超过 120 点时只显示图标, 更宽时在图标旁显示工具名称。宽度会保留到下次启动; 双击边缘恢复窄"
        "工具栏。右键菜单中的 **{{BeWerkzeugnamen}}** 仍可在任何宽度下显示名称。",
        "Blender と同じく、ツールバーは右端をドラッグして広げられます。120 ポイントまではアイコンだけ、それより広いとツール名を隣に表示します。"
        "幅は次回の起動まで保たれ、端をダブルクリックすると細いツールバーに戻ります。右クリック メニューの **{{BeWerkzeugnamen}}** は"
        "どの幅でも名前を表示します。",
        "Como en Blender, la barra de herramientas se puede ensanchar arrastrando su borde derecho: hasta 120 puntos solo muestra los "
        "símbolos; más ancha, muestra los nombres de las herramientas al lado. El ancho se conserva para el próximo inicio; un doble "
        "clic en el borde vuelve a la barra estrecha. **{{BeWerkzeugnamen}}** en el menú contextual sigue mostrando los nombres con "
        "cualquier ancho.",
        "Jako v Blenderu lze panel nástrojů rozšířit tažením za pravý okraj: do 120 bodů ukazuje jen symboly, širší i názvy nástrojů "
        "vedle nich. Šířka zůstane do dalšího spuštění; dvojklik na okraj vrátí úzký panel. **{{BeWerkzeugnamen}}** v nabídce pravého "
        "tlačítka dál ukazuje názvy v jakékoli šířce."),
    "bild_leiste": (
        "Blender mode: the toolbar dragged to 160 points shows the tool names",
        "Blender-Modus: die auf 160 Punkte gezogene Werkzeugleiste zeigt die Werkzeugnamen",
        "Blender 模式: 拖到 160 点宽的工具栏显示工具名称",
        "Blender 風: 160 ポイントに広げたツールバーはツール名を表示します",
        "Modo Blender: la barra ensanchada a 160 puntos muestra los nombres de las herramientas",
        "Režim Blender: panel nástrojů roztažený na 160 bodů ukazuje názvy nástrojů"),
    # --- B9 ---
    "seite": (
        "**Bones in the sidebar:** for a bone or a rig control the sidebar shows local values like Blender - location, rotation and "
        "scale relative to the rest position of the bone (the base pose of the .gla), in its own axes. In the rest position they "
        "are 0, a rotation without turn and 1. The rotation appears in the rotation mode of the bone: **{{EaModusQuat}}** (Blender's "
        "default), one of the six Euler orders or **{{EaModusAchse}}**; the field **{{EaDrehModus}}** below changes it for this bone. "
        "The mode changes only what you see and type - the game stores the pose as matrices. Scale is only shown: the rig does not "
        "scale bones.",
        "**Knochen in der Seitenleiste:** für einen Knochen oder einen Griff des Rigs zeigt die Seitenleiste lokale Werte wie Blender - "
        "Position, Rotation und Skalierung bezogen auf die Ruhelage des Knochens (die Grundpose der .gla), in seinen eigenen Achsen. In "
        "der Ruhelage sind sie 0, eine Drehung ohne Drehung und 1. Die Rotation steht im Drehmodus des Knochens: **{{EaModusQuat}}** "
        "(Blenders Vorgabe), eine der sechs Euler-Reihenfolgen oder **{{EaModusAchse}}**; das Feld **{{EaDrehModus}}** darunter stellt ihn "
        "für diesen Knochen um. Der Modus ändert nur, was man sieht und eintippt - das Spiel speichert die Pose als Matrizen. Die "
        "Skalierung steht nur zum Ablesen da: das Rig skaliert Knochen nicht.",
        "**侧栏中的骨骼:** 对于骨骼或绑定控制器, 侧栏像 Blender 一样显示局部值 - 相对于骨骼静止位置 (.gla 的基础姿态) 的位置、旋转和缩放, 使用骨骼自身的轴。"
        "在静止位置它们为 0、无旋转和 1。旋转按骨骼的旋转模式显示: **{{EaModusQuat}}** (Blender 的默认值)、六种欧拉顺序之一或 **{{EaModusAchse}}**; "
        "下方的 **{{EaDrehModus}}** 字段可为此骨骼切换模式。模式只改变显示和输入 - 游戏以矩阵保存姿态。缩放仅供查看: 绑定不缩放骨骼。",
        "**サイドバーのボーン:** ボーンやリグのコントロールについて、サイドバーは Blender と同じくローカル値を表示します - ボーンのレスト位置 "
        "(.gla の基本ポーズ) を基準にした位置・回転・スケールで、ボーン自身の軸で表します。レスト位置では 0、回転なし、1 です。回転はボーンの"
        "回転モードで表示されます: **{{EaModusQuat}}** (Blender の既定)、6 つのオイラー順序のいずれか、または **{{EaModusAchse}}**。下の "
        "**{{EaDrehModus}}** フィールドでこのボーンのモードを切り替えます。モードは表示と入力だけを変え、ゲームはポーズを行列で保存します。"
        "スケールは表示のみです: リグはボーンを拡大縮小しません。",
        "**Huesos en la barra lateral:** para un hueso o un control del rig, la barra lateral muestra valores locales como Blender: "
        "posición, rotación y escala respecto a la posición de reposo del hueso (la pose base del .gla), en sus propios ejes. En reposo "
        "son 0, una rotación sin giro y 1. La rotación aparece en el modo de rotación del hueso: **{{EaModusQuat}}** (el valor por "
        "defecto de Blender), uno de los seis órdenes de Euler o **{{EaModusAchse}}**; el campo **{{EaDrehModus}}** de debajo lo cambia "
        "para este hueso. El modo solo cambia lo que se ve y se escribe; el juego guarda la pose como matrices. La escala solo se "
        "muestra: el rig no escala huesos.",
        "**Kosti v postranním panelu:** pro kost nebo ovladač rigu ukazuje postranní panel místní hodnoty jako Blender - polohu, rotaci "
        "a měřítko vztažené ke klidové poloze kosti (základní póze .gla), v jejích vlastních osách. V klidové poloze jsou 0, rotace bez "
        "otočení a 1. Rotace je v režimu rotace kosti: **{{EaModusQuat}}** (výchozí v Blenderu), jedno ze šesti pořadí Eulerových úhlů "
        "nebo **{{EaModusAchse}}**; pole **{{EaDrehModus}}** pod nimi ho pro tuto kost přepne. Režim mění jen to, co vidíte a zadáváte - "
        "hra ukládá pózu jako matice. Měřítko je jen ke čtení: rig kosti neškáluje."),
    "bild_seite": (
        "Blender mode: the sidebar for the pelvis control - local location, rotation as quaternion, rotation mode and scale",
        "Blender-Modus: die Seitenleiste für den Griff Becken - lokale Position, Rotation als Quaternion, Rotationsmodus und Skalierung",
        "Blender 模式: 骨盆控制器的侧栏 - 局部位置、四元数旋转、旋转模式和缩放",
        "Blender 風: 骨盤コントロールのサイドバー - ローカル位置、クォータニオンの回転、回転モード、スケール",
        "Modo Blender: la barra lateral del control de la pelvis: posición local, rotación como cuaternio, modo de rotación y escala",
        "Režim Blender: postranní panel ovladače pánve - místní poloha, rotace jako quaternion, režim rotace a měřítko"),
    # --- B11, B12, B13 ---
    "augen": (
        "In 3ds Max mode **{{MgStapelPoly}}** and **{{MgStapelSkin}}** have an eye as well. **{{MgStapelPoly}}** off shows the surface as "
        "it was loaded, without the changes made since; **{{MgStapelSkin}}** off shows the whole figure in its bind pose while the "
        "bones keep their pose, as in 3ds Max (all surfaces hang on the same skeleton). Both change only the view: no undo step, the "
        "model and saving stay the same. Above **{{MgStapelPoly}}** stands **{{EaUnwrap}}** as in 3ds Max - the UVs of the surface; a "
        "click switches to the UV area.",
        "Im 3ds-Max-Modus haben auch **{{MgStapelPoly}}** und **{{MgStapelSkin}}** ein Auge. **{{MgStapelPoly}}** aus zeigt die Fläche, "
        "wie sie geladen wurde, ohne die Änderungen seither; **{{MgStapelSkin}}** aus zeigt die ganze Figur in ihrer Bindepose, die "
        "Knochen behalten ihre Pose, wie in 3ds Max (alle Flächen hängen am selben Skelett). Beides ändert nur die Anzeige: kein "
        "Rückgängig-Schritt, Modell und Speichern bleiben gleich. Über **{{MgStapelPoly}}** steht wie in 3ds Max **{{EaUnwrap}}** - die "
        "UVs der Fläche; ein Klick wechselt in den Bereich UV.",
        "在 3ds Max 模式下, **{{MgStapelPoly}}** 和 **{{MgStapelSkin}}** 也有眼睛图标。关闭 **{{MgStapelPoly}}** 时按加载时的样子显示该面, 不含此后的"
        "更改; 关闭 **{{MgStapelSkin}}** 时以绑定姿态显示整个角色, 骨骼保持其姿态, 与 3ds Max 相同 (所有面都挂在同一骨架上)。两者都只改变显示: 没有撤销"
        "步骤, 模型和保存不变。在 **{{MgStapelPoly}}** 上方, 与 3ds Max 一样有 **{{EaUnwrap}}** - 面的 UV; 单击可切换到 UV 区域。",
        "3ds Max 風では **{{MgStapelPoly}}** と **{{MgStapelSkin}}** にも目のアイコンがあります。**{{MgStapelPoly}}** をオフにするとサーフェスを"
        "読み込んだときの状態 (その後の変更なし) で表示し、**{{MgStapelSkin}}** をオフにするとフィギュア全体をバインドポーズで表示します。ボーンは"
        "ポーズを保ちます (3ds Max と同じ。すべてのサーフェスが同じスケルトンに付いています)。どちらも表示だけを変え、元に戻すステップはなく、"
        "モデルと保存は変わりません。**{{MgStapelPoly}}** の上には 3ds Max と同じく **{{EaUnwrap}}** (サーフェスの UV) があり、クリックすると "
        "UV エリアに切り替わります。",
        "En modo 3ds Max, **{{MgStapelPoly}}** y **{{MgStapelSkin}}** también tienen un ojo. Con **{{MgStapelPoly}}** desactivado se ve la "
        "superficie tal como se cargó, sin los cambios posteriores; con **{{MgStapelSkin}}** desactivado se ve toda la figura en su pose "
        "de enlace y los huesos conservan su pose, como en 3ds Max (todas las superficies cuelgan del mismo esqueleto). Ambos solo "
        "cambian la vista: no hay paso de deshacer y el modelo y el guardado no cambian. Encima de **{{MgStapelPoly}}** está "
        "**{{EaUnwrap}}**, como en 3ds Max: las UV de la superficie; un clic cambia al área UV.",
        "V režimu 3ds Max mají oko i **{{MgStapelPoly}}** a **{{MgStapelSkin}}**. Vypnutý **{{MgStapelPoly}}** ukazuje plochu tak, jak byla "
        "načtena, bez pozdějších změn; vypnutý **{{MgStapelSkin}}** ukazuje celou postavu ve vazební póze a kosti si drží svou pózu, jako "
        "ve 3ds Max (všechny plochy visí na stejné kostře). Obojí mění jen zobrazení: žádný krok Zpět, model i ukládání zůstávají stejné. "
        "Nad **{{MgStapelPoly}}** je jako ve 3ds Max **{{EaUnwrap}}** - UV souřadnice plochy; kliknutí přepne do oblasti UV."),
    "bild_stapel": (
        "3ds Max mode: the stack of torso with TurboSmooth, Unwrap UVW and the eyes of Edit Poly and Skin (Skin off: the figure in its bind pose)",
        "3ds-Max-Modus: der Stapel von torso mit TurboSmooth, Unwrap UVW und den Augen von Edit Poly und Skin (Skin aus: die Figur in ihrer Bindepose)",
        "3ds Max 模式: torso 的堆栈, 含 TurboSmooth、Unwrap UVW 以及 Edit Poly 和 Skin 的眼睛图标 (Skin 关闭: 角色处于绑定姿态)",
        "3ds Max 風: TurboSmooth、Unwrap UVW、Edit Poly と Skin の目のアイコンがある torso のスタック (Skin オフ: フィギュアはバインドポーズ)",
        "Modo 3ds Max: la pila de torso con TurboSmooth, Unwrap UVW y los ojos de Edit Poly y Skin (Skin desactivado: la figura en su pose de enlace)",
        "Režim 3ds Max: zásobník plochy torso s TurboSmooth, Unwrap UVW a okem u Edit Poly a Skin (Skin vypnutý: postava ve vazební póze)"),
    "begleit": (
        "The stack stays when the model is closed: saving writes it next to the model into **model.glm.stack**. The .glm itself is "
        "written exactly as before, and the game never reads the .stack file. Opening the .glm again brings the modifiers back; if "
        "the .stack file cannot be read (damaged, or the .glm was changed outside JK Studio), the model opens without them and the "
        "status line says why. Saving a model without modifiers removes an old .stack file of its own. The same holds for an .md3 requisite (model.md3.stack). The file also keeps the rotation mode of each bone that is not Quaternion (sidebar like Blender); an older .stack file without rotation modes still opens.",
        "Der Stapel bleibt, wenn das Modell geschlossen wird: Speichern schreibt ihn neben das Modell in **model.glm.stack**. Die .glm "
        "selbst wird genau wie bisher geschrieben, und das Spiel liest die .stack-Datei nie. Öffnet man die .glm wieder, sind die "
        "Modifier zurück; lässt sich die .stack-Datei nicht lesen (beschädigt, oder die .glm wurde außerhalb von JK Studio geändert), "
        "öffnet sich das Modell ohne sie, und die Statuszeile sagt warum. Speichern eines Modells ohne Modifier entfernt eine alte "
        "eigene .stack-Datei. Dasselbe gilt für ein .md3-Requisit (model.md3.stack). Die Datei behält auch den Rotationsmodus jedes Knochens, der nicht Quaternion ist (Seitenleiste wie Blender); eine ältere .stack-Datei ohne Rotationsmodi öffnet sich weiter.",
        "关闭模型后堆栈仍会保留: 保存时把它写到模型旁的 **model.glm.stack** 中。.glm 本身与以前完全一样地写入, 游戏从不读取 .stack 文件。再次打开 .glm "
        "时修改器会恢复; 如果无法读取 .stack 文件 (已损坏, 或 .glm 在 JK Studio 之外被修改), 模型会在没有修改器的情况下打开, 状态栏会说明原因。保存没有修改器的"
        "模型时会删除其旧的 .stack 文件。.md3 道具同样适用 (model.md3.stack)。该文件还会保存每个非四元数骨骼的旋转模式 (像 Blender 的侧栏); 没有旋转模式的旧 .stack 文件仍可打开。",
        "モデルを閉じてもスタックは残ります: 保存するとモデルの隣の **model.glm.stack** に書き込まれます。.glm 自体はこれまでとまったく同じに"
        "書き込まれ、ゲームは .stack ファイルを読みません。.glm を開き直すとモディファイアが戻ります。.stack ファイルが読めない場合 (壊れている、"
        "または JK Studio 以外で .glm が変更された) は、モデルはモディファイアなしで開き、ステータス行に理由が表示されます。モディファイアのない"
        "モデルを保存すると、古い自分の .stack ファイルは削除されます。.md3 の小道具も同じです (model.md3.stack)。このファイルはクォータニオン以外のボーンの回転モードも保存します (Blender 風のサイドバー)。回転モードのない古い .stack ファイルも引き続き開けます。",
        "La pila se mantiene al cerrar el modelo: guardar la escribe junto al modelo en **model.glm.stack**. El .glm se escribe "
        "exactamente igual que antes y el juego nunca lee el archivo .stack. Al volver a abrir el .glm vuelven los modificadores; si el "
        "archivo .stack no se puede leer (dañado, o el .glm se cambió fuera de JK Studio), el modelo se abre sin ellos y la línea de "
        "estado dice por qué. Guardar un modelo sin modificadores elimina su antiguo archivo .stack. Lo mismo vale para un accesorio .md3 (model.md3.stack). El archivo también guarda el modo de rotación de cada hueso que no sea cuaternio (barra lateral como Blender); un archivo .stack antiguo sin modos de rotación se sigue abriendo.",
        "Zásobník zůstane i po zavření modelu: uložení ho zapíše vedle modelu do **model.glm.stack**. Samotný .glm se zapíše přesně jako "
        "dosud a hra soubor .stack nikdy nečte. Po novém otevření .glm se modifikátory vrátí; pokud soubor .stack nejde přečíst "
        "(poškozený, nebo byl .glm změněn mimo JK Studio), model se otevře bez nich a stavový řádek řekne proč. Uložení modelu bez "
        "modifikátorů odstraní jeho starý soubor .stack. Totéž platí pro rekvizitu .md3 (model.md3.stack). Soubor uchovává i režim rotace každé kosti, který není Quaternion (postranní panel jako Blender); starší soubor .stack bez režimů rotace se dál otevře."),
}

ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}


def _t(schluessel):
    from inhalt_basis import T
    t = X[schluessel]
    return T(t[0], t[1], t[2], t[3])


def bloecke_explorer():
    """B7: hinter den Absatz "ebenen" von inhalt_ap2 (Einfrieren) (Szenen-Explorer, Seite modell-ansehen)."""
    return [("p", _t("explorer")), ("img", "h_ent3a_explorer", _t("bild_explorer"), "breit")]


def bloecke_bedienung():
    """C4: hinter den Absatz "b_text" von inhalt_mb1a2 (Seite modell-bedienung)."""
    return [("p", _t("esc"))]


def bloecke_leiste():
    """B8: hinter den Absatz "gruppen" von inhalt_ap4 (Seite modell-blender)."""
    return [("p", _t("leiste")), ("img", "h_ent3a_leiste", _t("bild_leiste"), "breit")]


def bloecke_seite():
    """B9: hinter den Absatz "seite" von inhalt_ap4 (Seite modell-blender)."""
    return [("p", _t("seite")), ("img", "h_ent3a_seite", _t("bild_seite"), "breit")]


def bloecke_stapel():
    """B11, B12, B13: hinter den Absatz "bedienen" von inhalt_ap13s (Abschnitt Modifier-Stapel, Seite modell-modellieren)."""
    return [("p", _t("augen")), ("img", "h_ent3a_stapel", _t("bild_stapel"), "breit"), ("note", _t("begleit"))]
