"""AP6 (MBENTWURF Teil B): das rechte Feld im Bereich Modellieren des Model-Tabs nach dem freigegebenen Entwurf.

Haengt Absaetze und zwei Bilder an die Seite modell-modellieren (inhalt_md1d.py), Abschnitt "Der Bereich Modellieren", hinter
das Bild h_md_modell und ersetzt im ersten Absatz den Satz ueber die Rollouts "Edit Poly" (bis rc611). Jeder Text einmal in allen
sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_ap2.py.

Eingehaengt im Register tools/handbuch/inhalt_modell.py (Zeile `Anhang("inhalt_ap6", "modell-modellieren")`).
Bilder: handbuch/bilder/<sprache>/h_ap6_max.webp, h_ap6_blender.webp (Selbsttest BHED_MODELL_NUR=ap6hb, je Sprache).
"""
import inhalt_md1d

# Der Satz des ersten Absatzes, der bis rc611 die Rollouts "Edit Poly" / "Edit Mode" nannte, und sein Ersatz
_SATZ_ALT = (
    "The rollouts **{{MdRolloutMax}}** (Blender: **{{MdRolloutBlender}}**) list the tools of the level; the same tools are in the menu ",
    "Die Rollouts **{{MdRolloutMax}}** (Blender: **{{MdRolloutBlender}}**) zeigen die Werkzeuge der Ebene; dieselben stehen im Menü ",
    "卷展栏 **{{MdRolloutMax}}** (Blender: **{{MdRolloutBlender}}**) 列出该层级的工具; 同样的工具在菜单 ",
    "ロールアウト **{{MdRolloutMax}}** (Blender: **{{MdRolloutBlender}}**) にレベルのツールがあり、同じものがメニュー ",
    "Los paneles **{{MdRolloutMax}}** (Blender: **{{MdRolloutBlender}}**) muestran las herramientas del nivel; las mismas están en el menú ",
    "Rollouty **{{MdRolloutMax}}** (Blender: **{{MdRolloutBlender}}**) ukazují nástroje úrovně; stejné jsou v menu ",
)
_SATZ_NEU = (
    "The group **{{Ap6Werkzeuge}}** on the right lists the tools of the level; the same tools are in the menu ",
    "Die Gruppe **{{Ap6Werkzeuge}}** rechts zeigt die Werkzeuge der Ebene; dieselben stehen im Menü ",
    "右侧的 **{{Ap6Werkzeuge}}** 组列出该层级的工具; 同样的工具在菜单 ",
    "右側のグループ **{{Ap6Werkzeuge}}** にレベルのツールがあり、同じものがメニュー ",
    "El grupo **{{Ap6Werkzeuge}}** de la derecha muestra las herramientas del nivel; las mismas están en el menú ",
    "Skupina **{{Ap6Werkzeuge}}** vpravo ukazuje nástroje úrovně; stejné jsou v menu ",
)


def _wo():
    """Der erste Absatz der Seite mit dem neuen Satz (die Quelltexte von inhalt_md1d.py setzen die Zeilen ohne Leerzeichen
    aneinander - der alte Satz wird ueber seine Zeilengrenzen hinweg gesucht)."""
    alt = inhalt_md1d.X["wo"]
    neu = []
    for i in range(6):
        if _SATZ_ALT[i] not in alt[i]:
            raise SystemExit("inhalt_ap6: der Satz ueber die Rollouts fehlt im Absatz 'wo' von inhalt_md1d.py (Sprache %d)" % i)
        neu.append(alt[i].replace(_SATZ_ALT[i], _SATZ_NEU[i]))
    return tuple(neu)


X = {
    "wo": _wo(),
    "feld": (
        "**The panel on the right** is built like the approved design: three groups under one another. **{{MdGrAuswahl}}** holds the levels "
        "as one row of fields with their key in front (3ds Max mode 1 to 5, Blender mode 1 {{Ap6SegEcke}}, 2 {{MdEbeneKante}}, 3 "
        "{{Ap6SegFlaeche}}), below it **{{MdRing}}**, **{{MdLoop}}**, **{{MdAlles}}**, **{{MdNichts}}** and the counts of the mesh with "
        "the number selected. **{{Ap6Werkzeuge}}** shows the six main tools as buttons of equal width with a symbol and the key of the "
        "control mode - **{{MdExtrude}}**, **{{MdInset}}**, **{{MdChamfer}}**, **{{MdLoopCut}}**, **{{MdCut}}**, **{{MdMerge}}** - then "
        "the further tools of the level under its name from 3ds Max (**{{MdGrPunkte}}**, **{{MdGrKanten}}**, **{{MdGrRaender}}**, "
        "**{{MdGrVielecke}}**) and, folded, **{{MdGrGeometrie}}**. A tool that does not work on the current level is greyed out; its "
        "tooltip names the levels it works on. **{{MdGrGrenzen}}** shows bars for the selected surface - vertices, bones and weights "
        "against the limit of the game, yellow from 90 %, red above - and says in its heading what is closest to a limit; "
        "**{{Ap6FlaechenImModell}}** counts the surfaces, and the list of all surfaces opens with a click.",
        "**Das Feld rechts** ist aufgebaut wie der freigegebene Entwurf: drei Gruppen untereinander. **{{MdGrAuswahl}}** enthält die Ebenen "
        "als eine Reihe von Feldern mit ihrer Taste davor (3ds-Max-Modus 1 bis 5, Blender-Modus 1 {{Ap6SegEcke}}, 2 {{MdEbeneKante}}, 3 "
        "{{Ap6SegFlaeche}}), darunter **{{MdRing}}**, **{{MdLoop}}**, **{{MdAlles}}**, **{{MdNichts}}** und die Zähler des Netzes mit der "
        "Zahl der gewählten. **{{Ap6Werkzeuge}}** zeigt die sechs Hauptwerkzeuge als gleich breite Knöpfe mit Symbol und der Taste des "
        "Bedienmodus - **{{MdExtrude}}**, **{{MdInset}}**, **{{MdChamfer}}**, **{{MdLoopCut}}**, **{{MdCut}}**, **{{MdMerge}}** -, danach "
        "die weiteren Werkzeuge der Ebene unter ihrem Namen aus 3ds Max (**{{MdGrPunkte}}**, **{{MdGrKanten}}**, **{{MdGrRaender}}**, "
        "**{{MdGrVielecke}}**) und zugeklappt **{{MdGrGeometrie}}**. Ein Werkzeug, das auf der laufenden Ebene nicht arbeitet, ist "
        "ausgegraut; sein Tooltip nennt die Ebenen, auf denen es arbeitet. **{{MdGrGrenzen}}** zeigt Balken für die gewählte Fläche - "
        "Ecken, Knochen und Gewichte gegen die Grenze des Spiels, gelb ab 90 %, rot darüber - und nennt in der Überschrift, was einer "
        "Grenze am nächsten kommt; **{{Ap6FlaechenImModell}}** zählt die Flächen, und die Liste aller Flächen klappt mit einem Klick auf.",
        "**右侧面板** 按已批准的设计构建: 三个组上下排列。**{{MdGrAuswahl}}** 把各层级放在一行格子中, 前面是按键 (3ds Max 模式 1 到 5, Blender 模式 "
        "1 {{Ap6SegEcke}}、2 {{MdEbeneKante}}、3 {{Ap6SegFlaeche}}), 下面是 **{{MdRing}}**、**{{MdLoop}}**、**{{MdAlles}}**、**{{MdNichts}}** "
        "以及网格的计数和已选数量。**{{Ap6Werkzeuge}}** 把六个主要工具显示为等宽按钮, 带符号和当前操作模式的按键 - **{{MdExtrude}}**、**{{MdInset}}**、"
        "**{{MdChamfer}}**、**{{MdLoopCut}}**、**{{MdCut}}**、**{{MdMerge}}** - 其后是该层级的其他工具, 标题沿用 3ds Max 的名称 (**{{MdGrPunkte}}**、"
        "**{{MdGrKanten}}**、**{{MdGrRaender}}**、**{{MdGrVielecke}}**), 以及折叠的 **{{MdGrGeometrie}}**。在当前层级不起作用的工具显示为灰色, "
        "其提示写明它适用的层级。**{{MdGrGrenzen}}** 为所选面显示进度条 - 顶点、骨骼和权重对照游戏的限制, 90 % 起为黄色, 超出为红色 - 并在标题中写明"
        "最接近限制的一项; **{{Ap6FlaechenImModell}}** 统计面数, 单击即可展开全部面的列表。",
        "**右側のパネル** は承認済みのデザインどおりで、3 つのグループが縦に並びます。**{{MdGrAuswahl}}** はレベルを 1 列のマスで示し、前にキーが"
        "付きます (3ds Max 風は 1〜5、Blender 風は 1 {{Ap6SegEcke}}、2 {{MdEbeneKante}}、3 {{Ap6SegFlaeche}})。その下に **{{MdRing}}**、"
        "**{{MdLoop}}**、**{{MdAlles}}**、**{{MdNichts}}** と、メッシュの数および選択数があります。**{{Ap6Werkzeuge}}** は 6 つの主要ツールを、記号と"
        "操作モードのキー付きの同じ幅のボタンで示します - **{{MdExtrude}}**、**{{MdInset}}**、**{{MdChamfer}}**、**{{MdLoopCut}}**、**{{MdCut}}**、"
        "**{{MdMerge}}**。続いてレベルのその他のツールが 3ds Max の名前 (**{{MdGrPunkte}}**、**{{MdGrKanten}}**、**{{MdGrRaender}}**、"
        "**{{MdGrVielecke}}**) の下に並び、**{{MdGrGeometrie}}** は折りたたまれています。現在のレベルで使えないツールは灰色になり、ツールチップに"
        "使えるレベルが書かれます。**{{MdGrGrenzen}}** は選択したサーフェスのバーを示します - 頂点・ボーン・ウェイトをゲームの上限と比べ、90 % から黄色、"
        "超えると赤 - 見出しには上限に最も近いものが出ます。**{{Ap6FlaechenImModell}}** はサーフェス数で、全サーフェスの一覧はクリックで開きます。",
        "**El panel de la derecha** sigue el diseño aprobado: tres grupos uno bajo otro. **{{MdGrAuswahl}}** contiene los niveles como una fila "
        "de casillas con su tecla delante (modo 3ds Max 1 a 5, modo Blender 1 {{Ap6SegEcke}}, 2 {{MdEbeneKante}}, 3 {{Ap6SegFlaeche}}), "
        "debajo **{{MdRing}}**, **{{MdLoop}}**, **{{MdAlles}}**, **{{MdNichts}}** y los contadores de la malla con el número seleccionado. "
        "**{{Ap6Werkzeuge}}** muestra las seis herramientas principales como botones de igual ancho con símbolo y la tecla del modo de "
        "control - **{{MdExtrude}}**, **{{MdInset}}**, **{{MdChamfer}}**, **{{MdLoopCut}}**, **{{MdCut}}**, **{{MdMerge}}** -, después las "
        "demás herramientas del nivel bajo su nombre de 3ds Max (**{{MdGrPunkte}}**, **{{MdGrKanten}}**, **{{MdGrRaender}}**, "
        "**{{MdGrVielecke}}**) y, plegado, **{{MdGrGeometrie}}**. Una herramienta que no funciona en el nivel actual aparece atenuada; su "
        "ayuda nombra los niveles en los que funciona. **{{MdGrGrenzen}}** muestra barras para la superficie seleccionada - vértices, "
        "huesos y pesos frente al límite del juego, amarillo desde el 90 %, rojo por encima - y dice en su título qué está más cerca de un "
        "límite; **{{Ap6FlaechenImModell}}** cuenta las superficies, y la lista de todas se abre con un clic.",
        "**Panel vpravo** je postaven podle schváleného návrhu: tři skupiny pod sebou. **{{MdGrAuswahl}}** obsahuje úrovně jako řadu polí "
        "s klávesou vpředu (režim 3ds Max 1 až 5, režim Blender 1 {{Ap6SegEcke}}, 2 {{MdEbeneKante}}, 3 {{Ap6SegFlaeche}}), pod nimi "
        "**{{MdRing}}**, **{{MdLoop}}**, **{{MdAlles}}**, **{{MdNichts}}** a počty sítě s počtem vybraných. **{{Ap6Werkzeuge}}** ukazuje "
        "šest hlavních nástrojů jako stejně široká tlačítka se symbolem a klávesou režimu ovládání - **{{MdExtrude}}**, **{{MdInset}}**, "
        "**{{MdChamfer}}**, **{{MdLoopCut}}**, **{{MdCut}}**, **{{MdMerge}}** -, potom další nástroje úrovně pod jejím názvem z 3ds Max "
        "(**{{MdGrPunkte}}**, **{{MdGrKanten}}**, **{{MdGrRaender}}**, **{{MdGrVielecke}}**) a sbalenou **{{MdGrGeometrie}}**. Nástroj, "
        "který na aktuální úrovni nepracuje, je zašedlý; jeho nápověda jmenuje úrovně, na kterých pracuje. **{{MdGrGrenzen}}** ukazuje "
        "pruhy pro vybraný povrch - vrcholy, kosti a váhy proti limitu hry, žlutě od 90 %, červeně nad ním - a v nadpisu říká, co je "
        "limitu nejblíž; **{{Ap6FlaechenImModell}}** počítá povrchy a seznam všech povrchů se otevře kliknutím."),
    "bild_max": (
        "Like 3ds Max: the command panel on the tab Modify with the groups Selection, Tools and JKA limits.",
        "Wie 3ds Max: das Befehlsfeld im Reiter Ändern mit den Gruppen Auswahl, Werkzeuge und JKA-Grenzen.",
        "与 3ds Max 相同: 命令面板的“修改”选项卡, 含“选择”“工具”和“JKA 限制”三个组。",
        "3ds Max 風: コマンドパネルの「修正」タブに、選択・ツール・JKA の上限の各グループ。",
        "Como 3ds Max: el panel de comandos en la pestaña Modificar con los grupos Selección, Herramientas y Límites de JKA.",
        "Jako 3ds Max: příkazový panel na kartě Upravit se skupinami Výběr, Nástroje a Limity JKA."),
    "reiter": (
        "**The other tabs** of the panel hold what does not belong to changing the mesh. Primitives and props stand where the model "
        "comes from: in 3ds Max mode on the tab **{{MfErstellen}}**, in Blender mode on the tab **{{MfObjekt}}** - the group "
        "**{{Ap6Grundkoerper}}** with one button per shape and **{{MdGrRequisit}}** with the name, **{{MdNeuesRequisit}}** and "
        "**{{MdInDieKarte}}**. While a prop is open, **{{MdGrRequisit}}** also stays on the tab of the area, so that **{{MdInDieKarte}}** "
        "remains one click; an empty prop shows **{{Ap6Grundkoerper}}** there at the top. **{{MdGrGrenzen}}** is also on "
        "**{{MfDienste}}** (Blender mode: **{{MfDaten}}**), and in Blender mode the tab **{{MfWerkzeug}}** shows selection and tools alone.",
        "**Die anderen Reiter** des Felds enthalten, was nicht zum Ändern des Netzes gehört. Grundkörper und Requisiten stehen dort, wo "
        "das Modell entsteht: im 3ds-Max-Modus im Reiter **{{MfErstellen}}**, im Blender-Modus im Reiter **{{MfObjekt}}** - die Gruppe "
        "**{{Ap6Grundkoerper}}** mit einem Knopf je Form und **{{MdGrRequisit}}** mit dem Namen, **{{MdNeuesRequisit}}** und "
        "**{{MdInDieKarte}}**. Solange ein Requisit offen ist, bleibt **{{MdGrRequisit}}** auch im Reiter des Bereichs, damit "
        "**{{MdInDieKarte}}** ein Klick bleibt; ein leeres Requisit zeigt dort ganz oben **{{Ap6Grundkoerper}}**. **{{MdGrGrenzen}}** "
        "steht außerdem unter **{{MfDienste}}** (Blender-Modus: **{{MfDaten}}**), und im Blender-Modus zeigt der Reiter **{{MfWerkzeug}}** "
        "Auswahl und Werkzeuge allein.",
        "面板的 **其他选项卡** 放置与修改网格无关的内容。基本体和道具位于模型产生的地方: 3ds Max 模式在 **{{MfErstellen}}** 选项卡, Blender 模式在 "
        "**{{MfObjekt}}** 选项卡 - **{{Ap6Grundkoerper}}** 组每种形状一个按钮, **{{MdGrRequisit}}** 组包含名称、**{{MdNeuesRequisit}}** 和 "
        "**{{MdInDieKarte}}**。只要打开的是道具, **{{MdGrRequisit}}** 也保留在本区域的选项卡中, 使 **{{MdInDieKarte}}** 仍然只需单击一次; 空道具会在"
        "那里最上方显示 **{{Ap6Grundkoerper}}**。**{{MdGrGrenzen}}** 也在 **{{MfDienste}}** 下 (Blender 模式: **{{MfDaten}}**), Blender 模式的 "
        "**{{MfWerkzeug}}** 选项卡只显示选择和工具。",
        "パネルの **ほかのタブ** には、メッシュの変更に属さないものがあります。プリミティブと小道具はモデルを作る場所にあります: 3ds Max 風は "
        "**{{MfErstellen}}** タブ、Blender 風は **{{MfObjekt}}** タブ - 形ごとにボタンがある **{{Ap6Grundkoerper}}** グループと、名前・"
        "**{{MdNeuesRequisit}}**・**{{MdInDieKarte}}** のある **{{MdGrRequisit}}** です。小道具を開いている間は **{{MdGrRequisit}}** がエリアのタブ"
        "にも残り、**{{MdInDieKarte}}** はワンクリックのままです。空の小道具ではそこに **{{Ap6Grundkoerper}}** が一番上に出ます。"
        "**{{MdGrGrenzen}}** は **{{MfDienste}}** (Blender 風: **{{MfDaten}}**) にもあり、Blender 風の **{{MfWerkzeug}}** タブは選択とツールだけを示します。",
        "**Las demás pestañas** del panel contienen lo que no es cambiar la malla. Las primitivas y los accesorios están donde nace el "
        "modelo: en modo 3ds Max en la pestaña **{{MfErstellen}}**, en modo Blender en la pestaña **{{MfObjekt}}** - el grupo "
        "**{{Ap6Grundkoerper}}** con un botón por forma y **{{MdGrRequisit}}** con el nombre, **{{MdNeuesRequisit}}** y "
        "**{{MdInDieKarte}}**. Mientras haya un accesorio abierto, **{{MdGrRequisit}}** sigue también en la pestaña del área, para que "
        "**{{MdInDieKarte}}** siga siendo un clic; un accesorio vacío muestra allí arriba **{{Ap6Grundkoerper}}**. **{{MdGrGrenzen}}** "
        "está además en **{{MfDienste}}** (modo Blender: **{{MfDaten}}**), y en modo Blender la pestaña **{{MfWerkzeug}}** muestra solo "
        "selección y herramientas.",
        "**Ostatní karty** panelu obsahují to, co nepatří ke změně sítě. Základní tělesa a rekvizity jsou tam, kde model vzniká: v režimu "
        "3ds Max na kartě **{{MfErstellen}}**, v režimu Blender na kartě **{{MfObjekt}}** - skupina **{{Ap6Grundkoerper}}** s tlačítkem "
        "pro každý tvar a **{{MdGrRequisit}}** s názvem, **{{MdNeuesRequisit}}** a **{{MdInDieKarte}}**. Dokud je otevřená rekvizita, "
        "zůstává **{{MdGrRequisit}}** i na kartě oblasti, aby **{{MdInDieKarte}}** zůstalo jedním kliknutím; prázdná rekvizita tam nahoře "
        "ukáže **{{Ap6Grundkoerper}}**. **{{MdGrGrenzen}}** je také pod **{{MfDienste}}** (režim Blender: **{{MfDaten}}**) a v režimu "
        "Blender karta **{{MfWerkzeug}}** ukazuje jen výběr a nástroje."),
    "bild_blender": (
        "Like Blender: the properties editor on the tab Modifiers; the six main tools carry the keys E, I, Ctrl+B, Ctrl+R, K and M.",
        "Wie Blender: der Eigenschaften-Editor im Reiter Modifier; die sechs Hauptwerkzeuge tragen die Tasten E, I, Strg+B, Strg+R, K und M.",
        "与 Blender 相同: 属性编辑器的“修改器”选项卡; 六个主要工具带有按键 E、I、Ctrl+B、Ctrl+R、K 和 M。",
        "Blender 風: プロパティエディターの「モディファイアー」タブ。6 つの主要ツールにはキー E、I、Ctrl+B、Ctrl+R、K、M が付きます。",
        "Como Blender: el editor de propiedades en la pestaña Modificadores; las seis herramientas principales llevan las teclas E, I, Ctrl+B, Ctrl+R, K y M.",
        "Jako Blender: editor vlastností na kartě Modifikátory; šest hlavních nástrojů nese klávesy E, I, Ctrl+B, Ctrl+R, K a M."),
    "tasten": (
        "Every tool first shows its preview in the tool caddy: <kbd>Enter</kbd> applies, <kbd>Ctrl</kbd>+<kbd>Enter</kbd> applies and "
        "starts the same tool again on the result (the button **+**), <kbd>Esc</kbd> cancels. In Blender mode <kbd>E</kbd> extrudes "
        "as in Blender (<kbd>Alt</kbd>+<kbd>E</kbd> still works; the kinds of Blender's extrude menu are in the caddy), and "
        "<kbd>Shift</kbd>+<kbd>A</kbd> opens the menu **{{Ap6Hinzufuegen}}** with the primitives.",
        "Jedes Werkzeug zeigt erst seine Vorschau im Werkzeugkästchen: <kbd>Enter</kbd> übernimmt, <kbd>Strg</kbd>+<kbd>Enter</kbd> "
        "übernimmt und beginnt dasselbe Werkzeug noch einmal auf dem Ergebnis (der Knopf **+**), <kbd>Esc</kbd> bricht ab. Im "
        "Blender-Modus extrudiert <kbd>E</kbd> wie in Blender (<kbd>Alt</kbd>+<kbd>E</kbd> geht weiterhin; die Arten aus Blenders Menü "
        "Extrudieren stehen im Kästchen), und <kbd>Umschalt</kbd>+<kbd>A</kbd> öffnet das Menü **{{Ap6Hinzufuegen}}** mit den Grundkörpern.",
        "每个工具先在工具框中显示预览: <kbd>Enter</kbd> 应用, <kbd>Ctrl</kbd>+<kbd>Enter</kbd> 应用并在结果上再次启动同一工具 (按钮 **+**), "
        "<kbd>Esc</kbd> 取消。Blender 模式下 <kbd>E</kbd> 与 Blender 一样执行挤出 (<kbd>Alt</kbd>+<kbd>E</kbd> 仍然可用; Blender 挤出菜单中的"
        "各种方式在工具框中), <kbd>Shift</kbd>+<kbd>A</kbd> 打开含基本体的 **{{Ap6Hinzufuegen}}** 菜单。",
        "どのツールもまずツールボックスにプレビューを表示します: <kbd>Enter</kbd> で適用、<kbd>Ctrl</kbd>+<kbd>Enter</kbd> で適用して同じツールを結果に"
        "もう一度 (ボタン **+**)、<kbd>Esc</kbd> でキャンセル。Blender 風では <kbd>E</kbd> が Blender と同じく押し出しです "
        "(<kbd>Alt</kbd>+<kbd>E</kbd> も引き続き使えます。Blender の押し出しメニューの種類はボックス内にあります)。<kbd>Shift</kbd>+<kbd>A</kbd> は"
        "プリミティブのメニュー **{{Ap6Hinzufuegen}}** を開きます。",
        "Cada herramienta muestra primero su vista previa en la caja de herramienta: <kbd>Intro</kbd> aplica, "
        "<kbd>Ctrl</kbd>+<kbd>Intro</kbd> aplica y vuelve a empezar la misma herramienta sobre el resultado (el botón **+**), "
        "<kbd>Esc</kbd> cancela. En modo Blender <kbd>E</kbd> extruye como en Blender (<kbd>Alt</kbd>+<kbd>E</kbd> sigue funcionando; "
        "los tipos del menú de extrusión de Blender están en la caja), y <kbd>Mayús</kbd>+<kbd>A</kbd> abre el menú "
        "**{{Ap6Hinzufuegen}}** con las primitivas.",
        "Každý nástroj nejdřív ukáže náhled v okénku nástroje: <kbd>Enter</kbd> použije, <kbd>Ctrl</kbd>+<kbd>Enter</kbd> použije a "
        "spustí stejný nástroj znovu na výsledku (tlačítko **+**), <kbd>Esc</kbd> zruší. V režimu Blender <kbd>E</kbd> vysouvá jako "
        "v Blenderu (<kbd>Alt</kbd>+<kbd>E</kbd> funguje dál; druhy z nabídky vysunutí Blenderu jsou v okénku) a "
        "<kbd>Shift</kbd>+<kbd>A</kbd> otevře nabídku **{{Ap6Hinzufuegen}}** se základními tělesy."),
}

ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}


def _t(schluessel):
    from inhalt_basis import T
    t = X[schluessel]
    return T(t[0], t[1], t[2], t[3])


def einhaengen(modellieren):
    """Den ersten Absatz der Seite modell-modellieren ersetzen und Absaetze und Bilder hinter das Bild h_md_modell setzen."""
    b = modellieren["bloecke"]
    wo = next(n for n, x in enumerate(b) if x[0] == "h2" and x[1] == "wo")
    if b[wo + 1][0] != "p":
        raise SystemExit("inhalt_ap6: hinter der Ueberschrift 'wo' steht kein Absatz")
    b[wo + 1] = ("p", _t("wo"))
    i = next(n for n, x in enumerate(b) if x[0] == "img" and x[1] == "h_md_modell") + 1
    b[i:i] = [
        ("p", _t("feld")),
        ("img", "h_ap6_max", _t("bild_max"), "breit"),
        ("p", _t("reiter")),
        ("img", "h_ap6_blender", _t("bild_blender"), "breit"),
        ("note", _t("tasten")),
    ]
