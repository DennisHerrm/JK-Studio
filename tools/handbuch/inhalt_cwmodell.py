"""CW-MODELL: Teil 3 der Clone-Wars-Kette (plaene/clonewars_kette.md) im Model-Tab - Detailstufen erzeugen und je Stufe zaehlen,
Bolts fuer Sitze und Muendungen, ein eigenes Skelett fuer Fahrzeuge mit eigener animation.cfg.

Eigene Seite modell-clonewars (Gruppe mission, nach modell-animimport; eingetragen in inhalt_modell.QUELLEN). Jeder Text einmal in
allen sechs Sprachen (en, de, zh, ja, es, cs). Bilder: handbuch/bilder/<sprache>/h_cwmodell_*.webp (Selbsttest BHED_MODELL_NUR=cwmodellhb).
"""

X = {
    "titel": (
        "Model tab: detail levels, bolts, own skeleton", "Model-Tab: Detailstufen, Bolts, eigenes Skelett", "Model 选项卡: 细节级别、挂点、自定义骨架",
        "Model タブ: 詳細レベル、ボルト、独自スケルトン", "Pestaña Model: niveles de detalle, bolts, esqueleto propio",
        "Karta Model: úrovně detailu, bolty, vlastní kostra"),
    "kurz": (
        "Generate LOD 2 and 3 like ProOptimizer or Decimate, see vertices per level, add bolts for seats and muzzles, build a skeleton for a vehicle.",
        "LOD 2 und 3 wie ProOptimizer oder Decimate erzeugen, Ecken je Stufe sehen, Bolts für Sitze und Mündungen anlegen, ein Skelett für ein Fahrzeug bauen.",
        "像 ProOptimizer 或 Decimate 一样生成 LOD 2 和 3, 查看每级顶点数, 为座位和枪口添加挂点, 为载具建立骨架。",
        "ProOptimizer や Decimate のように LOD 2 と 3 を生成し、レベルごとの頂点数を見て、座席と銃口のボルトを追加し、乗り物のスケルトンを作ります。",
        "Genera LOD 2 y 3 como ProOptimizer o Decimate, mira los vértices por nivel, añade bolts para asientos y bocas de fuego, crea un esqueleto para un vehículo.",
        "Vytvořte LOD 2 a 3 jako ProOptimizer nebo Decimate, podívejte se na vrcholy po úrovních, přidejte bolty pro sedadla a ústí, postavte kostru pro vozidlo."),
    "h_lod": ("Generate detail levels", "Detailstufen erzeugen", "生成细节级别", "詳細レベルを生成", "Generar niveles de detalle", "Vytvořit úrovně detailu"),
    "lod": (
        "The game draws a far model with a coarser detail level. In the area **{{MbBereichModellieren}}** the group **{{LodGruppeMax}}** "
        "(3ds Max mode, tab {{MfAendern}}) or **{{LodGruppeBl}}** (Blender mode, tab Modifiers) makes them from LOD 1. **{{LodVorlage}}** "
        "chooses how much each level keeps - measured on the files of Jedi Academy: figures 0.65 / 0.40, weapons and equipment 0.55 / 0.30, "
        "props 0.75 / 0.50. **{{LodStufen}}** is 2, 3 or 4 (a .md3 has at most 3), the sliders set the share per level with the target in "
        "vertices. **{{LodErzeugen}}** is one undo step; afterwards the view shows the lowest level and the table lists every level before "
        "and after - click a row to see it.",
        "Das Spiel zeichnet ein fernes Modell mit einer gröberen Detailstufe. Im Bereich **{{MbBereichModellieren}}** macht die Gruppe "
        "**{{LodGruppeMax}}** (Bedienung wie 3ds Max, Reiter {{MfAendern}}) bzw. **{{LodGruppeBl}}** (wie Blender, Reiter Modifier) sie aus "
        "LOD 1. **{{LodVorlage}}** wählt, wie viel jede Stufe behält - gemessen an den Dateien von Jedi Academy: Figuren 0,65 / 0,40, Waffen "
        "und Ausrüstung 0,55 / 0,30, Requisiten 0,75 / 0,50. **{{LodStufen}}** ist 2, 3 oder 4 (eine .md3 hat höchstens 3), die Regler "
        "stellen den Anteil je Stufe mit dem Ziel in Ecken. **{{LodErzeugen}}** ist ein Rückgängig-Schritt; danach zeigt die Ansicht die "
        "niedrigste Stufe und die Tabelle jede Stufe vorher und nachher - ein Klick auf eine Zeile zeigt sie.",
        "游戏用较粗的细节级别绘制远处的模型。在 **{{MbBereichModellieren}}** 区域, 组 **{{LodGruppeMax}}**(3ds Max 方式, {{MfAendern}} 选项卡)或 "
        "**{{LodGruppeBl}}**(Blender 方式, 修改器选项卡)从 LOD 1 生成它们。**{{LodVorlage}}** 选择每级保留多少 - 按 Jedi Academy 的文件测得: 人物 "
        "0.65 / 0.40, 武器和装备 0.55 / 0.30, 道具 0.75 / 0.50。**{{LodStufen}}** 为 2、3 或 4(.md3 最多 3), 滑块设置每级比例并显示目标顶点数。"
        "**{{LodErzeugen}}** 是一步撤销; 之后视图显示最低级别, 表格列出每一级前后的数字 - 点击一行即可查看。",
        "ゲームは遠くのモデルを粗い詳細レベルで描画します。**{{MbBereichModellieren}}** 領域のグループ **{{LodGruppeMax}}**(3ds Max 方式、"
        "{{MfAendern}} タブ)または **{{LodGruppeBl}}**(Blender 方式、モディファイアータブ)が LOD 1 からそれを作ります。**{{LodVorlage}}** は"
        "各レベルが残す量を選びます - Jedi Academy のファイルで測定: フィギュア 0.65 / 0.40、武器と装備 0.55 / 0.30、小道具 0.75 / 0.50。"
        "**{{LodStufen}}** は 2、3、4(.md3 は最大 3)、スライダーはレベルごとの割合を目標頂点数付きで設定します。**{{LodErzeugen}}** は元に戻す "
        "1 回分です。その後ビューは最低レベルを表示し、表は各レベルの前後を示します - 行をクリックするとそのレベルを表示します。",
        "El juego dibuja un modelo lejano con un nivel de detalle más basto. En el área **{{MbBereichModellieren}}** el grupo "
        "**{{LodGruppeMax}}** (modo 3ds Max, pestaña {{MfAendern}}) o **{{LodGruppeBl}}** (modo Blender, pestaña Modificadores) los crea a "
        "partir de LOD 1. **{{LodVorlage}}** elige cuánto conserva cada nivel, medido en los archivos de Jedi Academy: figuras 0,65 / 0,40, "
        "armas y equipo 0,55 / 0,30, accesorios 0,75 / 0,50. **{{LodStufen}}** es 2, 3 o 4 (un .md3 tiene como máximo 3); los controles "
        "fijan la proporción por nivel con el objetivo en vértices. **{{LodErzeugen}}** es un paso de deshacer; después la vista muestra el "
        "nivel más bajo y la tabla cada nivel antes y después: pulsa una fila para verlo.",
        "Hra kreslí vzdálený model hrubší úrovní detailu. V oblasti **{{MbBereichModellieren}}** je skupina **{{LodGruppeMax}}** (ovládání jako "
        "3ds Max, karta {{MfAendern}}) nebo **{{LodGruppeBl}}** (jako Blender, karta Modifikátory) vytvoří z LOD 1. **{{LodVorlage}}** volí, "
        "kolik si každá úroveň ponechá - změřeno na souborech Jedi Academy: postavy 0,65 / 0,40, zbraně a výstroj 0,55 / 0,30, rekvizity "
        "0,75 / 0,50. **{{LodStufen}}** je 2, 3 nebo 4 (.md3 má nejvýše 3), jezdce nastaví podíl každé úrovně s cílem ve vrcholech. "
        "**{{LodErzeugen}}** je jeden krok zpět; pak pohled ukáže nejnižší úroveň a tabulka každou úroveň před a po - kliknutím na řádek ji ukáže."),
    "bild_max": ("Detail levels in 3ds Max mode", "Detailstufen wie 3ds Max", "3ds Max 方式的细节级别", "3ds Max 方式の詳細レベル",
                 "Niveles de detalle en modo 3ds Max", "Úrovně detailu v režimu 3ds Max"),
    "regeln": (
        "Every vertex of a coarser level is a vertex of LOD 1: position, normal, UV, bones and weights stay exactly as in the file, so a "
        ".md3 keeps all its frames and a .glm at most four weights per vertex with sum 1. Surfaces are reduced together: where two surfaces "
        "meet, an edge collapses in both, the border stays closed. UV seams stay. Tags (*name), surfaces ending in _off and surfaces with "
        "_cap_ stay as they are in every level, so caps still close the cut. Each surface stays below SHADER_MAX_VERTEXES of the target "
        "game, the lowest level of a .glm below half of it (the stencil shadow of a large surface uses that level). A .glm carries the "
        "levels in the file; saving a .md3 writes name_1.md3 and name_2.md3 next to it - if such files are already there, the program asks "
        "before it overwrites them. Without Generate nothing changes, saving writes the same bytes as before and leaves name_1/name_2 alone.",
        "Jede Ecke einer gröberen Stufe ist eine Ecke von LOD 1: Ort, Normale, UV, Knochen und Gewichte bleiben genau wie in der Datei - eine "
        ".md3 behält so alle Bilder, eine .glm höchstens vier Gewichte je Ecke mit Summe 1. Die Flächen werden gemeinsam vereinfacht: wo zwei "
        "Flächen aneinanderstoßen, kollabiert eine Kante in beiden, der Rand bleibt geschlossen. UV-Nähte bleiben. Tags (*name), Flächen mit "
        "_off am Ende und Flächen mit _cap_ bleiben in jeder Stufe, wie sie sind - Kappen schließen die Schnittstelle weiter. Jede Fläche "
        "bleibt unter SHADER_MAX_VERTEXES des Zielspiels, die niedrigste Stufe einer .glm unter der Hälfte (der Stencil-Schatten einer großen "
        "Fläche nimmt diese Stufe). Eine .glm trägt die Stufen in der Datei; Speichern einer .md3 schreibt name_1.md3 und name_2.md3 daneben. "
        "Liegen solche Dateien schon da, fragt das Programm vor dem Überschreiben. Ohne Erzeugen ändert sich nichts, Speichern schreibt dieselben "
        "Bytes wie vorher und lässt name_1/name_2 unberührt.",
        "较粗级别的每个顶点都是 LOD 1 的顶点: 位置、法线、UV、骨骼和权重与文件中完全相同 - 因此 .md3 保留所有帧, .glm 每个顶点最多四个权重且总和为 1。"
        "表面一起简化: 两个表面相接处, 一条边在两侧同时折叠, 边界保持闭合。UV 接缝保留。标签(*name)、以 _off 结尾和含 _cap_ 的表面在每一级都保持原样 - "
        "端盖仍能封住切口。每个表面都低于目标游戏的 SHADER_MAX_VERTEXES, .glm 的最低级别低于其一半(大表面的模板阴影使用该级别)。.glm 把各级别存在文件中; "
        "保存 .md3 时在旁边写出 name_1.md3 和 name_2.md3 - 若这些文件已存在, 程序会在覆盖前询问。不点“生成”就什么都不变, 保存写出的字节与以前相同, 也不动 name_1/name_2。",
        "粗いレベルの頂点はすべて LOD 1 の頂点です: 位置・法線・UV・ボーン・ウェイトはファイルのまま - .md3 は全フレームを保ち、.glm は頂点あたり"
        "最大 4 ウェイト・合計 1 のままです。面はまとめて簡略化されます: 2 つの面が接する所では辺が両方で同時に縮み、境界は閉じたままです。UV の"
        "継ぎ目は残ります。タグ(*name)、末尾が _off の面、_cap_ を含む面はどのレベルでもそのまま - キャップは切り口を閉じ続けます。各面はターゲット"
        "ゲームの SHADER_MAX_VERTEXES 未満、.glm の最低レベルはその半分未満です(大きな面のステンシル影はそのレベルを使います)。.glm はレベルを"
        "ファイル内に持ち、.md3 を保存すると横に name_1.md3 と name_2.md3 を書きます - 既にあれば上書きの前にプログラムが確認します。生成しなければ何も変わらず、"
        "保存は以前と同じバイトを書き、name_1/name_2 には触れません。",
        "Cada vértice de un nivel más basto es un vértice de LOD 1: posición, normal, UV, huesos y pesos quedan exactamente como en el "
        "archivo, así un .md3 conserva todos sus fotogramas y un .glm como máximo cuatro pesos por vértice con suma 1. Las superficies se "
        "simplifican juntas: donde se tocan dos superficies, una arista colapsa en ambas y el borde queda cerrado. Las costuras UV se "
        "conservan. Las etiquetas (*nombre), las superficies que terminan en _off y las que contienen _cap_ quedan como están en cada nivel, "
        "así las tapas siguen cerrando el corte. Cada superficie queda por debajo de SHADER_MAX_VERTEXES del juego, el nivel más bajo de un "
        ".glm por debajo de la mitad (la sombra stencil de una superficie grande usa ese nivel). Un .glm lleva los niveles en el archivo; al "
        "guardar un .md3 se escriben name_1.md3 y name_2.md3 al lado; si ya existen, el programa pregunta antes de sobrescribirlos. Sin Generar "
        "nada cambia, guardar escribe los mismos bytes que antes y no toca name_1/name_2.",
        "Každý vrchol hrubší úrovně je vrcholem LOD 1: poloha, normála, UV, kosti a váhy zůstanou přesně jako v souboru - .md3 si tak ponechá "
        "všechny snímky a .glm nejvýše čtyři váhy na vrchol se součtem 1. Plochy se zjednodušují společně: kde se dvě plochy stýkají, "
        "zkolabuje hrana v obou a hranice zůstane uzavřená. UV švy zůstanou. Tagy (*jméno), plochy končící _off a plochy s _cap_ zůstanou v "
        "každé úrovni, jak jsou - krytky dál uzavírají řez. Každá plocha zůstane pod SHADER_MAX_VERTEXES cílové hry, nejnižší úroveň .glm "
        "pod polovinou (stencil stín velké plochy použije tuto úroveň). .glm nese úrovně v souboru; uložení .md3 zapíše vedle name_1.md3 a "
        "name_2.md3; pokud už existují, program se před přepsáním zeptá. Bez Vytvořit se nic nezmění, uložení zapíše tytéž bajty jako dřív a "
        "name_1/name_2 nechá být."),
    "bild_blender": ("Detail levels in Blender mode", "Detailstufen wie Blender", "Blender 方式的细节级别", "Blender 方式の詳細レベル",
                     "Niveles de detalle en modo Blender", "Úrovně detailu v režimu Blender"),
    "cw": (
        "**Only with the check profile {{KeCwProfilClonewars}}** (off by default): the switch **Clone Wars chain: lowest level at most ... "
        "vertices** caps the lowest level at 500 vertices drawn for a figure and 300 for weapons and equipment (battles with 150-250 figures "
        "need cheap distant figures), and the table warns above it or with only one level. Borders and seams can keep more.",
        "**Nur mit dem Prüfprofil {{KeCwProfilClonewars}}** (ab Werk aus): der Schalter **Clone-Wars-Kette: niedrigste Stufe höchstens ... "
        "Ecken** begrenzt die niedrigste Stufe auf 500 gezeichnete Ecken je Figur und 300 für Waffen und Ausrüstung (Schlachten mit 150-250 "
        "Figuren brauchen billige ferne Figuren), und die Tabelle warnt darüber oder bei nur einer Stufe. Ränder und Nähte können mehr halten.",
        "**仅在检查配置 {{KeCwProfilClonewars}} 下**(默认关闭): 开关 **Clone Wars 流程: 最低级别最多 ... 个顶点** 把最低级别限制为每个人物绘制 500 个顶点、"
        "武器和装备 300 个(150-250 个人物的战斗需要远处人物开销低), 超过时或只有一个级别时表格会警告。边界和接缝可能保留更多。",
        "**チェックプロファイル {{KeCwProfilClonewars}} のときだけ**(既定ではオフ): スイッチ **Clone Wars チェーン: 最低レベルは最大 ... 頂点** "
        "は最低レベルをフィギュアあたり描画 500 頂点、武器と装備は 300 に抑えます(150-250 体の戦闘では遠くのフィギュアを軽くする必要があります)。"
        "それを超えるときやレベルが 1 つだけのときは表が警告します。境界と継ぎ目はそれ以上を保つことがあります。",
        "**Solo con el perfil de comprobación {{KeCwProfilClonewars}}** (desactivado por defecto): el interruptor **Cadena Clone Wars: "
        "nivel más bajo como máximo ... vértices** limita el nivel más bajo a 500 vértices dibujados por figura y 300 para armas y equipo "
        "(las batallas con 150-250 figuras necesitan figuras lejanas baratas), y la tabla avisa por encima o con un solo nivel. Bordes y "
        "costuras pueden retener más.",
        "**Jen s profilem kontrol {{KeCwProfilClonewars}}** (ve výchozím stavu vypnuto): přepínač **Řetězec Clone Wars: nejnižší úroveň "
        "nejvýše ... vrcholů** omezí nejnižší úroveň na 500 vykreslených vrcholů na postavu a 300 pro zbraně a výstroj (bitvy se 150-250 "
        "postavami potřebují levné vzdálené postavy) a tabulka nad tím nebo při jediné úrovni varuje. Hranice a švy mohou držet víc."),
    "h_stufen": ("Vertices per level", "Ecken je Stufe", "每级顶点数", "レベルごとの頂点数", "Vértices por nivel", "Vrcholy po úrovních"),
    "stufen": (
        "In the area **{{MbBereichAnsehen}}** the group **{{MbRLod}}** lists every level with vertices, triangles and surfaces; the tooltip "
        "of a row names the largest surface and every limit it passes (red: over SHADER_MAX_VERTEXES, yellow: shadow limit of the lowest "
        "level, MD3_MAX_VERTS 4096, MD3_MAX_SURFACES 64). For a .md3 of the game the files name_1.md3 and name_2.md3 next to it are counted "
        "too. Without levels the button **{{LodZumWerkzeug}}** opens the group above. With the Clone Wars profile the optimizer of the map "
        "editor also reports run-time .md3 models with more vertices than one surface carries and no _1.md3, and a _2.md3 without _1.md3 "
        "(the game stops there).",
        "Im Bereich **{{MbBereichAnsehen}}** nennt die Gruppe **{{MbRLod}}** jede Stufe mit Ecken, Dreiecken und Teilflächen; der Tooltip "
        "einer Zeile nennt die größte Teilfläche und jede Grenze, die sie überschreitet (rot: über SHADER_MAX_VERTEXES, gelb: Schattengrenze "
        "der niedrigsten Stufe, MD3_MAX_VERTS 4096, MD3_MAX_SURFACES 64). Bei einer .md3 des Spiels zählen die Dateien name_1.md3 und "
        "name_2.md3 daneben mit. Ohne Stufen öffnet der Knopf **{{LodZumWerkzeug}}** die Gruppe oben. Mit dem Clone-Wars-Profil meldet der "
        "Optimierer des Karten-Editors außerdem Laufzeit-.md3 mit mehr Ecken, als eine Fläche trägt, ohne _1.md3, und eine _2.md3 ohne _1.md3 "
        "(das Spiel bricht dort ab).",
        "在 **{{MbBereichAnsehen}}** 区域, 组 **{{MbRLod}}** 列出每一级的顶点、三角形和表面数; 一行的提示给出最大的表面以及它超过的每项限制(红色: 超过 "
        "SHADER_MAX_VERTEXES, 黄色: 最低级别的阴影限制、MD3_MAX_VERTS 4096、MD3_MAX_SURFACES 64)。对于游戏的 .md3, 旁边的 name_1.md3 和 name_2.md3 "
        "也会计入。没有级别时, 按钮 **{{LodZumWerkzeug}}** 打开上面的组。使用 Clone Wars 配置时, 地图编辑器的优化器还会报告顶点数超过一个表面承载量但没有 "
        "_1.md3 的运行时 .md3, 以及有 _2.md3 却没有 _1.md3 的情况(游戏会在那里终止)。",
        "**{{MbBereichAnsehen}}** 領域のグループ **{{MbRLod}}** は各レベルの頂点・三角形・サーフェス数を示します。行のツールチップは最大の"
        "サーフェスと超えている上限を示します(赤: SHADER_MAX_VERTEXES 超過、黄: 最低レベルの影の上限、MD3_MAX_VERTS 4096、MD3_MAX_SURFACES 64)。"
        "ゲームの .md3 では横の name_1.md3 と name_2.md3 も数えます。レベルがないときはボタン **{{LodZumWerkzeug}}** が上のグループを開きます。"
        "Clone Wars プロファイルでは、マップエディターの最適化ツールも、1 面の上限より頂点が多く _1.md3 のない実行時 .md3 と、_1.md3 のない "
        "_2.md3(ゲームはそこで止まります)を報告します。",
        "En el área **{{MbBereichAnsehen}}** el grupo **{{MbRLod}}** lista cada nivel con vértices, triángulos y superficies; el tooltip de "
        "una fila nombra la superficie más grande y cada límite que supera (rojo: por encima de SHADER_MAX_VERTEXES, amarillo: límite de "
        "sombra del nivel más bajo, MD3_MAX_VERTS 4096, MD3_MAX_SURFACES 64). Para un .md3 del juego cuentan también name_1.md3 y name_2.md3 "
        "a su lado. Sin niveles, el botón **{{LodZumWerkzeug}}** abre el grupo de arriba. Con el perfil Clone Wars el optimizador del editor "
        "de mapas informa además de .md3 de ejecución con más vértices de los que lleva una superficie y sin _1.md3, y de un _2.md3 sin "
        "_1.md3 (el juego se detiene ahí).",
        "V oblasti **{{MbBereichAnsehen}}** uvádí skupina **{{MbRLod}}** každou úroveň s vrcholy, trojúhelníky a plochami; tooltip řádku "
        "jmenuje největší plochu a každý limit, který překračuje (červeně: nad SHADER_MAX_VERTEXES, žlutě: limit stínu nejnižší úrovně, "
        "MD3_MAX_VERTS 4096, MD3_MAX_SURFACES 64). U .md3 ze hry se započtou i soubory name_1.md3 a name_2.md3 vedle něj. Bez úrovní otevře "
        "tlačítko **{{LodZumWerkzeug}}** skupinu výše. S profilem Clone Wars hlásí optimalizátor editoru map navíc běhové .md3 s více vrcholy, "
        "než unese jedna plocha, bez _1.md3, a _2.md3 bez _1.md3 (tam hra skončí)."),
    "bild_stufen": ("Vertices per level", "Ecken je Stufe", "每级顶点数", "レベルごとの頂点数", "Vértices por nivel", "Vrcholy po úrovních"),
    "h_bolts": ("Bolts for seats and muzzles", "Bolts für Sitze und Mündungen", "座位和枪口的挂点", "座席と銃口のボルト", "Bolts para asientos y bocas de fuego",
                "Bolty pro sedadla a ústí"),
    "bolts": (
        "The group **{{CwBoltGruppeMax}}** (3ds Max mode) or **{{CwBoltGruppeBl}}** (Blender mode) in the area **{{MbBereichModellieren}}** "
        "adds bolt surfaces to a .glm: the name from the list of names the game code asks for (vehicles *driver, *droidunit, *muzzle1.., "
        "*exhaust1..; emplaced guns *seat, *flash01; weapons *flash; the tags of figures - the tooltip names file and line in OpenJK or "
        "Movie Duels) or your own, the bone it follows, position and rotation. **{{CwBoltAnlegen}}** is one undo step; a click on a bolt in "
        "the list fills the fields, **{{CwBoltUebernehmen}}** applies them. The bolt is saved as a surface with one triangle the game reads "
        "as exactly this position and rotation.",
        "Die Gruppe **{{CwBoltGruppeMax}}** (wie 3ds Max) bzw. **{{CwBoltGruppeBl}}** (wie Blender) im Bereich **{{MbBereichModellieren}}** "
        "legt Bolt-Flächen in einer .glm an: der Name aus der Liste der Namen, nach denen der Spielcode fragt (Fahrzeuge *driver, *droidunit, "
        "*muzzle1.., *exhaust1..; Geschütze *seat, *flash01; Waffen *flash; die Tags der Figuren - der Tooltip nennt Datei und Zeile in "
        "OpenJK bzw. Movie Duels) oder ein eigener, der Knochen, dem er folgt, Ort und Drehung. **{{CwBoltAnlegen}}** ist ein "
        "Rückgängig-Schritt; ein Klick auf einen Bolt der Liste füllt die Felder, **{{CwBoltUebernehmen}}** übernimmt sie. Gespeichert wird "
        "der Bolt als Fläche mit einem Dreieck, das das Spiel genau als diesen Ort und diese Drehung liest.",
        "**{{MbBereichModellieren}}** 区域中的组 **{{CwBoltGruppeMax}}**(3ds Max 方式)或 **{{CwBoltGruppeBl}}**(Blender 方式)在 .glm 中添加挂点表面: "
        "名称取自游戏代码查询的名称列表(载具 *driver、*droidunit、*muzzle1..、*exhaust1..; 固定炮 *seat、*flash01; 武器 *flash; 人物的标签 - 提示给出 "
        "OpenJK 或 Movie Duels 中的文件和行号)或自定义, 以及它跟随的骨骼、位置和旋转。**{{CwBoltAnlegen}}** 是一步撤销; 点击列表中的挂点会填入字段, "
        "**{{CwBoltUebernehmen}}** 应用它们。挂点保存为带一个三角形的表面, 游戏正好把它读作这个位置和旋转。",
        "**{{MbBereichModellieren}}** 領域のグループ **{{CwBoltGruppeMax}}**(3ds Max 方式)または **{{CwBoltGruppeBl}}**(Blender 方式)は .glm に"
        "ボルト面を追加します: 名前はゲームコードが問い合わせる名前の一覧から(乗り物 *driver、*droidunit、*muzzle1..、*exhaust1..; 据え置き砲 *seat、"
        "*flash01; 武器 *flash; フィギュアのタグ - ツールチップに OpenJK または Movie Duels のファイルと行)または独自のもの、従うボーン、位置と回転。"
        "**{{CwBoltAnlegen}}** は元に戻す 1 回分です。一覧のボルトをクリックすると欄が埋まり、**{{CwBoltUebernehmen}}** で適用します。ボルトは"
        "三角形 1 つの面として保存され、ゲームはそれをちょうどこの位置と回転として読みます。",
        "El grupo **{{CwBoltGruppeMax}}** (modo 3ds Max) o **{{CwBoltGruppeBl}}** (modo Blender) del área **{{MbBereichModellieren}}** añade "
        "superficies bolt a un .glm: el nombre de la lista de nombres que pide el código del juego (vehículos *driver, *droidunit, "
        "*muzzle1.., *exhaust1..; cañones fijos *seat, *flash01; armas *flash; las etiquetas de las figuras; el tooltip indica archivo y "
        "línea en OpenJK o Movie Duels) o uno propio, el hueso al que sigue, posición y rotación. **{{CwBoltAnlegen}}** es un paso de "
        "deshacer; pulsar un bolt de la lista rellena los campos y **{{CwBoltUebernehmen}}** los aplica. El bolt se guarda como superficie "
        "con un triángulo que el juego lee exactamente como esta posición y rotación.",
        "Skupina **{{CwBoltGruppeMax}}** (jako 3ds Max) nebo **{{CwBoltGruppeBl}}** (jako Blender) v oblasti **{{MbBereichModellieren}}** "
        "přidá do .glm plochy boltů: jméno ze seznamu jmen, na která se ptá kód hry (vozidla *driver, *droidunit, *muzzle1.., *exhaust1..; "
        "pevná děla *seat, *flash01; zbraně *flash; tagy postav - tooltip uvádí soubor a řádek v OpenJK nebo Movie Duels) nebo vlastní, "
        "kost, kterou sleduje, polohu a otočení. **{{CwBoltAnlegen}}** je jeden krok zpět; kliknutí na bolt v seznamu vyplní pole, "
        "**{{CwBoltUebernehmen}}** je použije. Bolt se uloží jako plocha s jedním trojúhelníkem, který hra čte přesně jako tuto polohu a otočení."),
    "bild_bolts": ("Bolts on a figure", "Bolts an einer Figur", "人物上的挂点", "フィギュアのボルト", "Bolts en una figura", "Bolty na postavě"),
    "bolts_pruef": (
        "With **{{CwFahrzeug}}** - or always with the check profile Clone Wars chain - the group warns when *driver is missing (the driver "
        "sits at the origin of the model) or every muzzle (*muzzle1.. or *flash1..: the weapons fire from the origin).",
        "Mit **{{CwFahrzeug}}** - oder mit dem Prüfprofil Clone-Wars-Kette immer - warnt die Gruppe, wenn *driver fehlt (der Fahrer sitzt im "
        "Ursprung des Modells) oder jede Mündung (*muzzle1.. oder *flash1..: die Waffen schießen aus dem Ursprung).",
        "勾选 **{{CwFahrzeug}}** 时 - 或使用检查配置 Clone Wars 流程时始终 - 如果缺少 *driver(驾驶员坐在模型原点)或缺少所有枪口(*muzzle1.. 或 "
        "*flash1..: 武器从原点射击), 该组会发出警告。",
        "**{{CwFahrzeug}}** をオンにすると - チェックプロファイル Clone Wars チェーンでは常に - *driver がない(運転手がモデルの原点に座る)か、"
        "銃口がひとつもない(*muzzle1.. または *flash1..: 武器が原点から撃つ)ときにグループが警告します。",
        "Con **{{CwFahrzeug}}** (o siempre con el perfil de comprobación Cadena Clone Wars) el grupo avisa cuando falta *driver (el "
        "conductor se sienta en el origen del modelo) o todas las bocas de fuego (*muzzle1.. o *flash1..: las armas disparan desde el origen).",
        "S **{{CwFahrzeug}}** - nebo s profilem kontrol Řetězec Clone Wars vždy - skupina varuje, když chybí *driver (řidič sedí v počátku "
        "modelu) nebo všechna ústí (*muzzle1.. nebo *flash1..: zbraně střílejí z počátku)."),
    "h_skelett": ("An own skeleton for vehicles", "Ein eigenes Skelett für Fahrzeuge", "载具的自定义骨架", "乗り物の独自スケルトン",
                  "Un esqueleto propio para vehículos", "Vlastní kostra pro vozidla"),
    "skelett": (
        "Walkers like AT-RT or AT-TE and droids need their own skeleton - figures stay on _humanoid.gla, whose bones and animations the game "
        "shares. In the area **{{MbBereichAnimation}}** the group **{{CwSkelettGruppeMax}}** (3ds Max mode, like Bone Tools) or "
        "**{{CwSkelettGruppeBl}}** (Blender mode, like an armature in edit mode) goes the whole way without another program:",
        "Läufer wie AT-RT oder AT-TE und Droiden brauchen ein eigenes Skelett - Figuren bleiben auf _humanoid.gla, dessen Knochen und "
        "Animationen das Spiel teilt. Im Bereich **{{MbBereichAnimation}}** geht die Gruppe **{{CwSkelettGruppeMax}}** (wie 3ds Max, wie "
        "Bone Tools) bzw. **{{CwSkelettGruppeBl}}** (wie Blender, wie eine Armature im Bearbeitungsmodus) den ganzen Weg ohne ein anderes Programm:",
        "AT-RT 或 AT-TE 这样的步行机以及机器人需要自定义骨架 - 人物保持在 _humanoid.gla 上, 游戏共用它的骨骼和动画。在 **{{MbBereichAnimation}}** "
        "区域, 组 **{{CwSkelettGruppeMax}}**(3ds Max 方式, 如骨骼工具)或 **{{CwSkelettGruppeBl}}**(Blender 方式, 如编辑模式下的骨架)无需其他程序即可走完全程:",
        "AT-RT や AT-TE のような歩行機やドロイドには独自スケルトンが必要です - フィギュアは _humanoid.gla のままで、そのボーンとアニメーションは"
        "ゲーム全体で共有されます。**{{MbBereichAnimation}}** 領域のグループ **{{CwSkelettGruppeMax}}**(3ds Max 方式、ボーンツールのように)または "
        "**{{CwSkelettGruppeBl}}**(Blender 方式、編集モードのアーマチュアのように)は、他のプログラムなしで全工程をこなします:",
        "Los caminantes como AT-RT o AT-TE y los droides necesitan un esqueleto propio; las figuras se quedan en _humanoid.gla, cuyos huesos "
        "y animaciones comparte el juego. En el área **{{MbBereichAnimation}}** el grupo **{{CwSkelettGruppeMax}}** (modo 3ds Max, como "
        "Bone Tools) o **{{CwSkelettGruppeBl}}** (modo Blender, como una armadura en modo edición) recorre todo el camino sin otro programa:",
        "Chodci jako AT-RT nebo AT-TE a droidi potřebují vlastní kostru - postavy zůstávají na _humanoid.gla, jejíž kosti a animace hra "
        "sdílí. V oblasti **{{MbBereichAnimation}}** projde skupina **{{CwSkelettGruppeMax}}** (jako 3ds Max, jako Bone Tools) nebo "
        "**{{CwSkelettGruppeBl}}** (jako Blender, jako armatura v režimu úprav) celou cestu bez jiného programu:"),
    "schritt1": (
        "**{{CwSkelettNeu}}** with a folder name creates models/players/<name>/<name> with the bone model_root and puts every surface on it.",
        "**{{CwSkelettNeu}}** mit einem Ordnernamen legt models/players/<Name>/<Name> mit dem Knochen model_root an und hängt jede Fläche daran.",
        "**{{CwSkelettNeu}}** 加文件夹名会创建带骨骼 model_root 的 models/players/<名称>/<名称>, 并把每个表面挂到它上面。",
        "フォルダー名を付けて **{{CwSkelettNeu}}** を押すと、ボーン model_root を持つ models/players/<名前>/<名前> ができ、すべての面がそれに付きます。",
        "**{{CwSkelettNeu}}** con un nombre de carpeta crea models/players/<nombre>/<nombre> con el hueso model_root y pone en él todas las superficies.",
        "**{{CwSkelettNeu}}** s názvem složky vytvoří models/players/<jméno>/<jméno> s kostí model_root a navěsí na ni každou plochu."),
    "schritt2": (
        "**{{CwKnochenAnlegenMax}}** adds a bone with name, parent and joint; a click on a bone in the list chooses it to move, rename or remove.",
        "**{{CwKnochenAnlegenMax}}** legt einen Knochen mit Name, Eltern und Gelenk an; ein Klick auf einen Knochen der Liste wählt ihn zum "
        "Verschieben, Umbenennen oder Entfernen.",
        "**{{CwKnochenAnlegenMax}}** 添加带名称、父级和关节的骨骼; 点击列表中的骨骼可选中它以移动、重命名或移除。",
        "**{{CwKnochenAnlegenMax}}** は名前・親・関節付きのボーンを追加します。一覧のボーンをクリックすると移動・名前変更・削除のために選べます。",
        "**{{CwKnochenAnlegenMax}}** añade un hueso con nombre, padre y articulación; pulsar un hueso de la lista lo elige para moverlo, "
        "renombrarlo o quitarlo.",
        "**{{CwKnochenAnlegenMax}}** přidá kost se jménem, rodičem a kloubem; kliknutí na kost v seznamu ji zvolí k posunutí, přejmenování nebo odebrání."),
    "schritt3": (
        "**{{CwFlaechenMax}}** puts the selected surfaces rigidly on the chosen bone - as a part of a vehicle moves. Finer weights are painted "
        "in the area Weights as for every figure.",
        "**{{CwFlaechenMax}}** hängt die gewählten Flächen starr an den gewählten Knochen - so bewegt sich ein Teil eines Fahrzeugs. Feinere "
        "Gewichte malt man im Bereich Weights wie bei jeder Figur.",
        "**{{CwFlaechenMax}}** 把所选表面刚性地挂到所选骨骼上 - 载具部件就是这样运动的。更细的权重像任何人物一样在 Weights 区域绘制。",
        "**{{CwFlaechenMax}}** は選んだ面を選んだボーンにリジッドに付けます - 乗り物の部品はそう動きます。細かいウェイトはどのフィギュアとも同じく "
        "Weights 領域で塗ります。",
        "**{{CwFlaechenMax}}** pone las superficies elegidas rígidas en el hueso elegido, como se mueve una pieza de un vehículo. Los pesos "
        "más finos se pintan en el área Weights como en cualquier figura.",
        "**{{CwFlaechenMax}}** pevně navěsí zvolené plochy na zvolenou kost - tak se pohybuje díl vozidla. Jemnější váhy se malují v "
        "oblasti Weights jako u každé postavy."),
    "schritt4": (
        "**{{CwSequenzAnlegen}}** adds frames to the skeleton and a line ANIM_NAME first count loop fps to the animation.cfg; pose and key "
        "them with the control rig like any animation, then save .gla, .glm and animation.cfg.",
        "**{{CwSequenzAnlegen}}** hängt Bilder an das Skelett und eine Zeile ANIM_NAME erstesBild anzahlBilder schleifenBilder bilderJeSekunde "
        "an die animation.cfg; gestellt und gekeyt wird mit dem Control Rig wie jede Animation, danach .gla, .glm und animation.cfg speichern.",
        "**{{CwSequenzAnlegen}}** 向骨架添加帧, 并向 animation.cfg 添加一行 ANIM_NAME 起始帧 帧数 循环帧 每秒帧数; 像任何动画一样用控制绑定摆姿势并打关键帧, "
        "然后保存 .gla、.glm 和 animation.cfg。",
        "**{{CwSequenzAnlegen}}** はスケルトンにフレームを、animation.cfg に ANIM_NAME 開始フレーム フレーム数 ループ fps の行を追加します。"
        "どのアニメーションとも同じくコントロールリグでポーズとキーを付け、.gla・.glm・animation.cfg を保存します。",
        "**{{CwSequenzAnlegen}}** añade fotogramas al esqueleto y una línea ANIM_NAME primero cantidad bucle fps al animation.cfg; se posa "
        "y se ponen claves con el control rig como cualquier animación, luego se guardan .gla, .glm y animation.cfg.",
        "**{{CwSequenzAnlegen}}** přidá ke kostře snímky a do animation.cfg řádek ANIM_NAME první počet smyčka fps; pózuje a klíčuje se "
        "control rigem jako každá animace, pak se uloží .gla, .glm a animation.cfg."),
    "bild_skelett": ("Own skeleton for a walker", "Eigenes Skelett für einen Läufer", "步行机的自定义骨架", "歩行機の独自スケルトン",
                     "Esqueleto propio para un caminante", "Vlastní kostra pro chodce"),
    "namen": (
        "The game reads only names from its animation table (OpenJK: 1544, Movie Duels Update 9: 3213); any other line it skips without a "
        "word. The group and the check of the animation.cfg warn about such names for the chosen target game. An own skeleton loads only "
        "in single player.",
        "Das Spiel liest nur Namen aus seiner Animationstabelle (OpenJK: 1544, Movie Duels Update 9: 3213); jede andere Zeile übergeht es "
        "wortlos. Die Gruppe und die Prüfung der animation.cfg warnen vor solchen Namen für das gewählte Zielspiel. Ein eigenes Skelett lädt "
        "nur der Einzelspieler.",
        "游戏只读取其动画表中的名称(OpenJK: 1544 个, Movie Duels Update 9: 3213 个); 其他任何行都会被默默跳过。该组和 animation.cfg 检查会针对所选目标游戏"
        "对此类名称发出警告。自定义骨架只在单人模式中加载。",
        "ゲームはアニメーション表にある名前だけを読みます(OpenJK: 1544、Movie Duels Update 9: 3213)。それ以外の行は黙って飛ばします。グループと "
        "animation.cfg のチェックは、選んだターゲットゲームについてそのような名前を警告します。独自スケルトンはシングルプレイでのみ読み込まれます。",
        "El juego solo lee nombres de su tabla de animaciones (OpenJK: 1544, Movie Duels Update 9: 3213); cualquier otra línea la salta "
        "sin aviso. El grupo y la comprobación del animation.cfg avisan de esos nombres para el juego elegido. Un esqueleto propio solo se "
        "carga en un jugador.",
        "Hra čte jen jména ze své tabulky animací (OpenJK: 1544, Movie Duels Update 9: 3213); každý jiný řádek beze slova přeskočí. "
        "Skupina a kontrola animation.cfg na taková jména pro zvolenou cílovou hru varují. Vlastní kostru načte jen hra jednoho hráče."),
}


def _t(schluessel):
    from inhalt_basis import T
    t = X[schluessel]
    return T(t[0], t[1], t[2], t[3])


def seite():
    """Die Seite modell-clonewars: Detailstufen, Ecken je Stufe, Bolts, eigenes Skelett."""
    return {
        "id": "modell-clonewars",
        "gruppe": "mission",
        "titel": _t("titel"),
        "kurz": _t("kurz"),
        "bloecke": [
            ("h2", "detailstufen", _t("h_lod")),
            ("p", _t("lod")),
            ("img", "h_cwmodell_lod_max", _t("bild_max"), "breit"),
            ("p", _t("regeln")),
            ("img", "h_cwmodell_lod_blender", _t("bild_blender"), "breit"),
            ("note", _t("cw")),
            ("h2", "stufen", _t("h_stufen")),
            ("p", _t("stufen")),
            ("img", "h_cwmodell_stufen", _t("bild_stufen"), "breit"),
            ("h2", "bolts", _t("h_bolts")),
            ("p", _t("bolts")),
            ("img", "h_cwmodell_bolts", _t("bild_bolts"), "breit"),
            ("p", _t("bolts_pruef")),
            ("h2", "skelett", _t("h_skelett")),
            ("p", _t("skelett")),
            ("list", [_t("schritt1"), _t("schritt2"), _t("schritt3"), _t("schritt4")]),
            ("img", "h_cwmodell_skelett", _t("bild_skelett"), "breit"),
            ("note", _t("namen")),
        ],
    }


ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}
