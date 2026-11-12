"""AP13S (MBENTWURF Teil B): der Modifier-Stapel des Model-Tabs - Spiegeln und Glaetten als Eintraege je Flaeche, wie 3ds Max
die Gruppe "Modifier-Stapel" (Bereiche Modellieren und Weights), wie Blender die Gruppe "Modifier".

Haengt einen Abschnitt "Der Modifier-Stapel" mit drei Absaetzen, zwei Bildern und einer Notiz an die Seite modell-modellieren
(inhalt_md1d.py), vor den Abschnitt "grenzen". Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in
inhalt_ap6.py.

Einbinden (wie AP6-AP10 beim Zusammenfuehren des Handbuchs):
    tools/handbuch/inhalt.py        import inhalt_ap13s
                                    inhalt_ap13s.einhaengen(next(_s for _s in SEITEN if _s["id"] == "modell-modellieren"))
    tools/handbuch/inhalt_es.py     from inhalt_ap13s import ES as _AP13S ; TEXTE.update(_AP13S)
    tools/handbuch/inhalt_cs.py     from inhalt_ap13s import CS as _AP13S ; TEXTE.update(_AP13S)
    tools/handbuch/gen_handbuch.py  from i18n_ap13s import zeilen_vier as _ap13s (bei den anderen i18n_*-Listen; dazu i18n_ap3
                                    und i18n_ap8 fuer die Namen der Reiter und der Stufen)
Bilder: handbuch/bilder/<sprache>/h_ap13s_max.webp, h_ap13s_blender.webp (Selbsttest BHED_MODELL_NUR=ap13shb, je Sprache).
"""

X = {
    "h_stapel": (
        "The modifier stack",
        "Der Modifier-Stapel",
        "修改器堆栈",
        "モディファイアスタック",
        "La pila de modificadores",
        "Zásobník modifikátorů"),
    "stapel": (
        "**Modifiers** change a surface without touching its mesh: the surface underneath keeps what you modeled, and the view shows "
        "the result. Every surface has its own stack; the panel shows the one of the surface selected in the tree or in the view. In "
        "3ds Max mode the group **{{MgRStapel}}** stands at the top of the tab **{{MfAendern}}**, in the Modeling area and in the "
        "Weights area, and reads from the bottom up: the surface of the file, **{{MgStapelPoly}}**, the modifiers, **{{MgStapelSkin}}**. "
        "A click on **{{MgStapelPoly}}** or **{{MgStapelSkin}}** switches between Modeling and Weights. In Blender mode the group "
        "**{{StpGruppeBl}}** stands on the tab **{{MfModifier}}** and lists only the modifiers, from the top down. Selection, tools and "
        "the gizmo keep working on the mesh underneath.",
        "**Modifier** ändern eine Fläche, ohne ihr Netz anzufassen: die Fläche darunter behält, was modelliert wurde, und die Ansicht "
        "zeigt das Ergebnis. Jede Fläche hat ihren eigenen Stapel; das Feld zeigt den der Fläche, die im Baum oder in der Ansicht "
        "gewählt ist. Im 3ds-Max-Modus steht die Gruppe **{{MgRStapel}}** zuoberst im Reiter **{{MfAendern}}**, im Bereich Modellieren "
        "und im Bereich Weights, und liest sich von unten nach oben: die Fläche der Datei, **{{MgStapelPoly}}**, die Modifier, "
        "**{{MgStapelSkin}}**. Ein Klick auf **{{MgStapelPoly}}** oder **{{MgStapelSkin}}** wechselt zwischen Modellieren und Weights. "
        "Im Blender-Modus steht die Gruppe **{{StpGruppeBl}}** im Reiter **{{MfModifier}}** und nennt nur die Modifier, von oben nach "
        "unten. Auswahl, Werkzeuge und Gizmo arbeiten weiter auf dem Netz darunter.",
        "**修改器** 在不触碰网格的情况下改变一个面: 其下的面保留已建模的内容, 视图显示结果。每个面都有自己的堆栈; 面板显示在树中或视图中所选面的"
        "堆栈。3ds Max 模式下, **{{MgRStapel}}** 组位于 **{{MfAendern}}** 选项卡的最上方 (建模区域和 Weights 区域), 自下而上读: 文件中的面、"
        "**{{MgStapelPoly}}**、各修改器、**{{MgStapelSkin}}**。单击 **{{MgStapelPoly}}** 或 **{{MgStapelSkin}}** 可在建模与 Weights 之间切换。"
        "Blender 模式下, **{{StpGruppeBl}}** 组位于 **{{MfModifier}}** 选项卡, 只列出修改器, 自上而下。选择、工具和操纵器仍然作用于其下的网格。",
        "**モディファイア** はメッシュに触れずにサーフェスを変えます: 下のサーフェスはモデリングした内容を保ち、ビューには結果が表示されます。サーフェス"
        "ごとにスタックがあり、パネルにはツリーまたはビューで選択したサーフェスのスタックが表示されます。3ds Max 風では **{{MgRStapel}}** グループが "
        "**{{MfAendern}}** タブの一番上 (モデリングエリアと Weights エリア) にあり、下から上へ読みます: ファイルのサーフェス、**{{MgStapelPoly}}**、"
        "モディファイア、**{{MgStapelSkin}}**。**{{MgStapelPoly}}** または **{{MgStapelSkin}}** をクリックすると、モデリングと Weights が切り替わります。"
        "Blender 風では **{{StpGruppeBl}}** グループが **{{MfModifier}}** タブにあり、モディファイアだけを上から下へ並べます。選択、ツール、ギズモは"
        "引き続き下のメッシュに作用します。",
        "Los **modificadores** cambian una superficie sin tocar su malla: la superficie de debajo conserva lo modelado y la vista "
        "muestra el resultado. Cada superficie tiene su propia pila; el panel muestra la de la superficie seleccionada en el árbol o en "
        "la vista. En modo 3ds Max el grupo **{{MgRStapel}}** está arriba del todo en la pestaña **{{MfAendern}}**, en el área de "
        "modelado y en el área Weights, y se lee de abajo arriba: la superficie del archivo, **{{MgStapelPoly}}**, los modificadores, "
        "**{{MgStapelSkin}}**. Un clic en **{{MgStapelPoly}}** o **{{MgStapelSkin}}** cambia entre modelado y Weights. En modo Blender el "
        "grupo **{{StpGruppeBl}}** está en la pestaña **{{MfModifier}}** y solo enumera los modificadores, de arriba abajo. La selección, "
        "las herramientas y el gizmo siguen trabajando sobre la malla de debajo.",
        "**Modifikátory** mění povrch, aniž by se dotkly jeho sítě: povrch pod nimi si ponechá to, co bylo vymodelováno, a pohled ukazuje "
        "výsledek. Každý povrch má vlastní zásobník; panel ukazuje zásobník povrchu vybraného ve stromu nebo v pohledu. V režimu 3ds Max "
        "je skupina **{{MgRStapel}}** úplně nahoře na kartě **{{MfAendern}}**, v oblasti modelování i v oblasti Weights, a čte se zdola "
        "nahoru: povrch ze souboru, **{{MgStapelPoly}}**, modifikátory, **{{MgStapelSkin}}**. Kliknutí na **{{MgStapelPoly}}** nebo "
        "**{{MgStapelSkin}}** přepíná mezi modelováním a Weights. V režimu Blender je skupina **{{StpGruppeBl}}** na kartě "
        "**{{MfModifier}}** a uvádí jen modifikátory, shora dolů. Výběr, nástroje a gizmo dál pracují se sítí pod nimi."),
    "bild_max": (
        "Like 3ds Max: the modifier stack of the surface torso - Symmetry and TurboSmooth between Edit Poly and Skin, the settings of the selected modifier below.",
        "Wie 3ds Max: der Modifier-Stapel der Fläche torso - Symmetrie und TurboSmooth zwischen Edit Poly und Skin, darunter die Einstellungen des gewählten Modifiers.",
        "与 3ds Max 相同: 面 torso 的修改器堆栈 - 对称和涡轮平滑位于 Edit Poly 与 Skin 之间, 下方是所选修改器的设置。",
        "3ds Max 風: サーフェス torso のモディファイアスタック - Edit Poly と Skin の間にシンメトリとターボスムーズ、その下に選択したモディファイアの設定。",
        "Como 3ds Max: la pila de modificadores de la superficie torso; Simetría y TurboSmooth entre Edit Poly y Skin, y debajo los ajustes del modificador seleccionado.",
        "Jako 3ds Max: zásobník modifikátorů povrchu torso - Symetrie a TurboSmooth mezi Edit Poly a Skin, pod nimi nastavení vybraného modifikátoru."),
    "arten": (
        "**{{StpHinzuMax}}** (Blender mode: **{{StpHinzuBl}}**) adds one of two modifiers. **{{StpSpiegelnMax}}** (Blender mode: "
        "**{{StpSpiegelnBl}}**) cuts the surface at an axis plane through the origin of the model and mirrors one side onto the other: "
        "model one half and the other follows, and the bone weights move to the bones of the other side. **{{StpGlaettenMax}}** "
        "(Blender mode: **{{StpGlaettenBl}}**) subdivides the surface and rounds it, one to three times; **{{StpRaender}}** leaves open "
        "borders where they are, so the surface still meets its neighbors. Below the list stand the settings of the selected modifier "
        "and the vertices and triangles before and after it - yellow or red as soon as the result exceeds what the game allows per "
        "surface.",
        "**{{StpHinzuMax}}** (Blender-Modus: **{{StpHinzuBl}}**) fügt einen von zwei Modifiern hinzu. **{{StpSpiegelnMax}}** "
        "(Blender-Modus: **{{StpSpiegelnBl}}**) schneidet die Fläche an einer Achsenebene durch den Ursprung des Modells und spiegelt "
        "eine Seite auf die andere: eine Hälfte modellieren, die andere folgt, und die Knochengewichte gehen auf die Knochen der anderen "
        "Seite. **{{StpGlaettenMax}}** (Blender-Modus: **{{StpGlaettenBl}}**) unterteilt die Fläche und rundet sie, ein- bis dreimal; "
        "**{{StpRaender}}** lässt offene Ränder, wo sie sind, damit die Fläche weiter an ihre Nachbarn anschließt. Unter der Liste "
        "stehen die Einstellungen des gewählten Modifiers und die Ecken und Dreiecke vor und nach ihm - gelb oder rot, sobald das "
        "Ergebnis mehr hat, als das Spiel je Fläche erlaubt.",
        "**{{StpHinzuMax}}** (Blender 模式: **{{StpHinzuBl}}**) 可添加两种修改器之一。**{{StpSpiegelnMax}}** (Blender 模式: **{{StpSpiegelnBl}}**) "
        "在穿过模型原点的轴平面处切开面, 并把一侧镜像到另一侧: 只需建模一半, 另一半随之变化, 骨骼权重也转到另一侧的骨骼。**{{StpGlaettenMax}}** "
        "(Blender 模式: **{{StpGlaettenBl}}**) 细分并圆滑面, 一到三次; **{{StpRaender}}** 使开放边界保持原位, 面仍与相邻面相接。列表下方是所选修改器"
        "的设置以及它之前和之后的顶点数与三角形数 - 一旦结果超过游戏对每个面的限制, 就显示为黄色或红色。",
        "**{{StpHinzuMax}}** (Blender 風: **{{StpHinzuBl}}**) で 2 種類のモディファイアのどちらかを追加します。**{{StpSpiegelnMax}}** (Blender 風: "
        "**{{StpSpiegelnBl}}**) は、モデルの原点を通る軸平面でサーフェスを切り、片側をもう片側にミラーします: 半分をモデリングすれば残りが追従し、ボーン"
        "ウェイトも反対側のボーンに移ります。**{{StpGlaettenMax}}** (Blender 風: **{{StpGlaettenBl}}**) はサーフェスを 1〜3 回細分して丸めます。"
        "**{{StpRaender}}** は開いた境界をその場に残し、サーフェスが隣とつながったままになります。リストの下には、選択したモディファイアの設定と、その前後"
        "の頂点数・三角形数が表示されます - 結果がゲームの 1 サーフェスあたりの上限を超えると黄色または赤になります。",
        "**{{StpHinzuMax}}** (modo Blender: **{{StpHinzuBl}}**) añade uno de dos modificadores. **{{StpSpiegelnMax}}** (modo Blender: "
        "**{{StpSpiegelnBl}}**) corta la superficie en un plano de eje que pasa por el origen del modelo y refleja un lado sobre el otro: "
        "se modela una mitad y la otra la sigue, y los pesos de hueso pasan a los huesos del otro lado. **{{StpGlaettenMax}}** (modo "
        "Blender: **{{StpGlaettenBl}}**) subdivide la superficie y la redondea, de una a tres veces; **{{StpRaender}}** deja los bordes "
        "abiertos donde están, para que la superficie siga uniéndose a sus vecinas. Debajo de la lista están los ajustes del modificador "
        "seleccionado y los vértices y triángulos antes y después de él: en amarillo o rojo en cuanto el resultado supera lo que el juego "
        "permite por superficie.",
        "**{{StpHinzuMax}}** (režim Blender: **{{StpHinzuBl}}**) přidá jeden ze dvou modifikátorů. **{{StpSpiegelnMax}}** (režim Blender: "
        "**{{StpSpiegelnBl}}**) rozřízne povrch v osové rovině procházející počátkem modelu a zrcadlí jednu stranu na druhou: modelujete "
        "jednu polovinu a druhá ji následuje, váhy kostí přejdou na kosti druhé strany. **{{StpGlaettenMax}}** (režim Blender: "
        "**{{StpGlaettenBl}}**) povrch rozdělí a zaoblí, jednou až třikrát; **{{StpRaender}}** nechá otevřené okraje na místě, aby povrch "
        "dál navazoval na sousedy. Pod seznamem je nastavení vybraného modifikátoru a vrcholy a trojúhelníky před ním a po něm - žlutě "
        "nebo červeně, jakmile výsledek překročí, co hra na jeden povrch dovoluje."),
    "bedienen": (
        "The eye turns a modifier on and off. **{{StpDatei}}** decides whether saving and exporting write its result into the file: "
        "smoothing is a **preview only** at first, because a single step already makes six triangles out of one and a figure soon "
        "exceeds the limit of the game. **{{StpHoch}}** and **{{StpRunter}}** - or dragging a row - change the order, and the order is "
        "the order of computing. **{{StpAnwendenBl}}** (3ds Max mode: **{{StpAnwendenMax}}**, which takes every switched-on modifier up to "
        "the selected one) makes the result the real mesh of the surface in every LOD - exactly the mesh the view showed before. "
        "**{{StpEntfernen}}** takes a modifier off the stack. Each of these is one undo step. The same commands are in the right-click "
        "menu of a row, in the right-click menu of the view and in **{{MbAktionen}}**.",
        "Das Auge schaltet einen Modifier an und aus. **{{StpDatei}}** entscheidet, ob Speichern und Export sein Ergebnis in die Datei "
        "schreiben: Glätten ist zunächst **nur Vorschau**, weil schon ein Schritt aus einem Dreieck sechs macht und eine Figur die Grenze "
        "des Spiels schnell überschreitet. **{{StpHoch}}** und **{{StpRunter}}** - oder das Ziehen einer Zeile - ändern die Reihenfolge, "
        "und die Reihenfolge ist die des Rechnens. **{{StpAnwendenBl}}** (3ds-Max-Modus: **{{StpAnwendenMax}}**, das jeden "
        "eingeschalteten Modifier bis zum gewählten nimmt) macht das Ergebnis in jeder Detailstufe zum echten Netz der Fläche - genau "
        "das Netz, das die Ansicht vorher gezeigt hat. **{{StpEntfernen}}** nimmt einen Modifier aus dem Stapel. Jedes davon ist ein "
        "Rückgängig-Schritt. Dieselben Befehle stehen im Rechtsklick-Menü einer Zeile, im Rechtsklick-Menü der Ansicht und in "
        "**{{MbAktionen}}**.",
        "眼睛图标用于开关修改器。**{{StpDatei}}** 决定保存和导出时是否把它的结果写入文件: 平滑起初 **仅预览**, 因为仅一步就把一个三角形变成六个, 角色很快"
        "就会超过游戏的限制。**{{StpHoch}}** 和 **{{StpRunter}}** (或拖动某一行) 改变顺序, 顺序即计算顺序。**{{StpAnwendenBl}}** (3ds Max 模式: "
        "**{{StpAnwendenMax}}**, 它会处理到所选修改器为止所有已启用的修改器) 使结果在每个 LOD 中成为面的实际网格 - 正是视图此前显示的网格。"
        "**{{StpEntfernen}}** 把修改器从堆栈中移除。以上每一项都是一步撤销。同样的命令也在行的右键菜单、视图的右键菜单和 **{{MbAktionen}}** 中。",
        "目のアイコンでモディファイアをオン/オフします。**{{StpDatei}}** は、保存と書き出しで結果をファイルに書き込むかどうかを決めます: スムーズは最初 "
        "**プレビューのみ** です。1 ステップで三角形 1 つが 6 つになり、キャラクターはすぐにゲームの上限を超えるためです。**{{StpHoch}}** と "
        "**{{StpRunter}}** (または行のドラッグ) で順番を変えます。順番がそのまま計算の順です。**{{StpAnwendenBl}}** (3ds Max 風: "
        "**{{StpAnwendenMax}}**。選択したモディファイアまでのオンのものをすべて対象にします) は、結果をすべての LOD でサーフェスの実際のメッシュにします - "
        "それまでビューに表示されていたメッシュそのものです。**{{StpEntfernen}}** はモディファイアをスタックから外します。どれも元に戻す 1 ステップです。"
        "同じコマンドが、行の右クリックメニュー、ビューの右クリックメニュー、**{{MbAktionen}}** にもあります。",
        "El ojo activa y desactiva un modificador. **{{StpDatei}}** decide si guardar y exportar escriben su resultado en el archivo: el "
        "suavizado es al principio **solo vista previa**, porque un solo paso ya convierte un triángulo en seis y una figura supera "
        "pronto el límite del juego. **{{StpHoch}}** y **{{StpRunter}}**, o arrastrar una fila, cambian el orden, y el orden es el del "
        "cálculo. **{{StpAnwendenBl}}** (modo 3ds Max: **{{StpAnwendenMax}}**, que toma todos los modificadores activados hasta el "
        "seleccionado) convierte el resultado en la malla real de la superficie en cada LOD: exactamente la malla que la vista mostraba "
        "antes. **{{StpEntfernen}}** quita un modificador de la pila. Cada una de estas acciones es un paso de deshacer. Los mismos "
        "comandos están en el menú contextual de una fila, en el menú contextual de la vista y en **{{MbAktionen}}**.",
        "Oko zapíná a vypíná modifikátor. **{{StpDatei}}** rozhoduje, zda uložení a export zapíší jeho výsledek do souboru: vyhlazení je "
        "zpočátku **jen náhled**, protože už jeden krok udělá z jednoho trojúhelníku šest a postava rychle překročí limit hry. "
        "**{{StpHoch}}** a **{{StpRunter}}** - nebo tažení řádku - mění pořadí a pořadí je pořadím výpočtu. **{{StpAnwendenBl}}** (režim "
        "3ds Max: **{{StpAnwendenMax}}**, které vezme všechny zapnuté modifikátory až po vybraný) udělá z výsledku skutečnou síť povrchu "
        "v každém LOD - přesně tu síť, kterou pohled ukazoval předtím. **{{StpEntfernen}}** odebere modifikátor ze zásobníku. Každá z "
        "těchto akcí je jeden krok zpět. Stejné příkazy jsou v nabídce pravého tlačítka na řádku, v nabídce pravého tlačítka v pohledu a "
        "v **{{MbAktionen}}**."),
    "bild_blender": (
        "Like Blender: the group Modifiers with Mirror and Subdivision; each row has the eye and the switch for the file.",
        "Wie Blender: die Gruppe Modifier mit Spiegeln und Unterteilung; jede Zeile hat das Auge und den Schalter für die Datei.",
        "与 Blender 相同: “修改器”组, 含镜像和细分; 每一行都有眼睛图标和写入文件的开关。",
        "Blender 風: ミラーとサブディビジョンのあるモディファイアグループ。各行に目のアイコンとファイル用のスイッチがあります。",
        "Como Blender: el grupo Modificadores con Espejo y Subdivisión; cada fila tiene el ojo y el interruptor del archivo.",
        "Jako Blender: skupina Modifikátory se Zrcadlením a Dělením povrchu; každý řádek má oko a přepínač pro soubor."),
    "datei": (
        "As long as a surface has no modifier - or only modifiers marked preview only - saving writes exactly the same file as "
        "without the stack. The stack itself is not stored in the model file: after the model is closed the modifiers are gone, and "
        "what went into the file is ordinary mesh from then on.",
        "Solange eine Fläche keinen Modifier hat - oder nur solche mit „nur Vorschau“ -, schreibt Speichern genau dieselbe Datei wie "
        "ohne Stapel. Der Stapel selbst steht nicht in der Modelldatei: nach dem Schließen des Modells sind die Modifier weg, und was "
        "in die Datei ging, ist von da an gewöhnliches Netz.",
        "只要一个面没有修改器 (或只有标为“仅预览”的修改器), 保存时写出的文件就与没有堆栈时完全相同。堆栈本身不保存在模型文件中: 关闭模型后修改器即消失, "
        "已写入文件的部分此后就是普通网格。",
        "サーフェスにモディファイアがない (または「プレビューのみ」のものしかない) 間は、保存で書かれるファイルはスタックがない場合とまったく同じです。"
        "スタック自体はモデルファイルに保存されません: モデルを閉じるとモディファイアは消え、ファイルに書き込まれた分はそれ以降ふつうのメッシュになります。",
        "Mientras una superficie no tenga modificadores, o solo tenga los marcados como «solo vista previa», guardar escribe exactamente "
        "el mismo archivo que sin la pila. La pila en sí no se guarda en el archivo del modelo: al cerrar el modelo los modificadores "
        "desaparecen, y lo que pasó al archivo es desde entonces malla normal.",
        "Dokud povrch nemá žádný modifikátor - nebo jen takové, které jsou „jen náhled“ -, zapíše uložení přesně stejný soubor jako bez "
        "zásobníku. Samotný zásobník se do souboru modelu neukládá: po zavření modelu modifikátory zmizí a to, co šlo do souboru, je od "
        "té chvíle běžná síť."),
}

ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}


def _t(schluessel):
    from inhalt_basis import T
    t = X[schluessel]
    return T(t[0], t[1], t[2], t[3])


def einhaengen(modellieren):
    """Den Abschnitt "Der Modifier-Stapel" vor den Abschnitt "grenzen" der Seite modell-modellieren setzen."""
    b = modellieren["bloecke"]
    i = next(n for n, x in enumerate(b) if x[0] == "h2" and x[1] == "grenzen")
    b[i:i] = [
        ("h2", "stapel", _t("h_stapel")),
        ("p", _t("stapel")),
        ("img", "h_ap13s_max", _t("bild_max"), "breit"),
        ("p", _t("arten")),
        ("p", _t("bedienen")),
        ("img", "h_ap13s_blender", _t("bild_blender"), "breit"),
        ("note", _t("datei")),
    ]
