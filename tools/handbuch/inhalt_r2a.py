"""rc610 R2a: Gras (surfaceSprites) im Echtzeit-Licht, Spiegel mit allen Stufen, Nachfragen beim Schliessen und beim Einfuegen.

Wunsch Tester/shank (05.11.): "sprites ... like when u are on Yavin, those long grasses on the grass textures" und der
Spiegel-Shader des Testers (zwei Stufen ueber dem Spiegelbild). Haengt Absaetze und zwei Bilder an die Seite karte_tempo
(inhalt_karte7.py, nach dem Bild h_karte_portale), eine Notiz an die Seite karteneditor (Schliessen) und eine an
karte_bspmerge (Lumps, die sich waehrend des Teil-Compiles aendern).

Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_modobj.py: die ersten vier gehen in
T(...), Spanisch und Tschechisch nach inhalt_es.py / inhalt_cs.py (ES, CS).
"""

X = {
    "gras": (
        "**Grass and other surface sprites:** a shader stage with `surfaceSprites` (grass, bushes, flowers, vines - for example "
        "textures/yavin/ground_grasssprite) is drawn in the real-time light as in the game: many small sprites on every brush face, patch and "
        "baked `misc_model` with that shader, with the same density, size, variance, distance fade (`ssFademax`, `ssFadescale`) and spread as "
        "**{{KeSpielbildMenue}}**. Each face keeps its sprites until the face or its shader changes, so moving the camera costs only the sprites "
        "in range and in view. The sprites take the real-time light of the face below them (in grey, like the game takes the blue vertex colour). "
        "Wind zones (fx_wind) need the built map, so the sprites stand still here unless the stage sways by itself (`ssWindIdle`). The switch "
        "for grass in the preview also switches them off here.",
        "**Gras und andere Surface-Sprites:** eine Shaderstufe mit `surfaceSprites` (Gras, Büsche, Blumen, Ranken - zum Beispiel "
        "textures/yavin/ground_grasssprite) zeichnet das Echtzeit-Licht wie das Spiel: viele kleine Halme auf jeder Brushseite, jedem Patch und "
        "jedem eingebackenen `misc_model` mit diesem Shader, mit derselben Dichte, Größe, Streuung, Ausblendung in der Ferne (`ssFademax`, "
        "`ssFadescale`) und Verteilung wie **{{KeSpielbildMenue}}**. Jede Fläche behält ihre Halme, bis sich die Fläche oder ihr Shader ändert; "
        "eine Kamerafahrt kostet nur die Halme in Reichweite und im Bild. Die Halme bekommen das Echtzeit-Licht der Fläche unter ihnen (grau, "
        "wie das Spiel die blaue Eckenfarbe nimmt). Windzonen (fx_wind) brauchen die gebaute Karte, hier stehen die Halme deshalb still, außer "
        "die Stufe schwingt von selbst (`ssWindIdle`). Der Schalter für Gras in der Vorschau schaltet sie auch hier ab.",
        "**草和其他表面精灵:**带 `surfaceSprites` 的着色器阶段(草、灌木、花、藤蔓——例如 textures/yavin/ground_grasssprite)在实时光照中按游戏方式绘制:"
        "在使用该着色器的每个画刷面、补丁和烘焙的 `misc_model` 上绘制许多小精灵,密度、大小、变化、远处淡出(`ssFademax`、`ssFadescale`)和分布与"
        "**{{KeSpielbildMenue}}**相同。每个面保留自己的精灵,直到该面或其着色器改变;移动摄像机只计算范围内和画面内的精灵。精灵使用其下方表面的实时光照"
        "(灰色,就像游戏取蓝色顶点颜色)。风区(fx_wind)需要编译后的地图,因此这里的精灵保持静止,除非该阶段自己摆动(`ssWindIdle`)。预览中的草开关在这里也会关闭它们。",
        "**草やその他のサーフェススプライト:**`surfaceSprites` を持つシェーダーステージ(草、茂み、花、つる。たとえば textures/yavin/ground_grasssprite)"
        "は、リアルタイムライトでもゲームと同じように描かれます。そのシェーダーを使うすべてのブラシ面、パッチ、焼き込まれた `misc_model` に小さなスプライトが"
        "多数並び、密度、大きさ、ばらつき、遠くでのフェード(`ssFademax`、`ssFadescale`)、配置は **{{KeSpielbildMenue}}** と同じです。各面はその面か"
        "シェーダーが変わるまでスプライトを保持するので、カメラを動かしても範囲内かつ画面内のスプライトだけが計算されます。スプライトは下の面のリアルタイムライトを"
        "受けます(ゲームが青の頂点カラーを使うのと同じくグレーで)。風ゾーン(fx_wind)にはビルド済みマップが必要なため、ここではステージ自体が揺れる"
        "(`ssWindIdle`)場合を除きスプライトは静止します。プレビューの草のスイッチはここでもオフにします。",
        "**Hierba y otros surface sprites:** una etapa de shader con `surfaceSprites` (hierba, arbustos, flores, enredaderas; por ejemplo "
        "textures/yavin/ground_grasssprite) se dibuja en la luz en tiempo real como en el juego: muchos sprites pequeños en cada cara de brush, "
        "parche y `misc_model` horneado con ese shader, con la misma densidad, tamaño, variación, desvanecimiento a distancia (`ssFademax`, "
        "`ssFadescale`) y reparto que **{{KeSpielbildMenue}}**. Cada cara conserva sus sprites hasta que cambian la cara o su shader; mover la "
        "cámara solo cuesta los sprites al alcance y en la imagen. Los sprites toman la luz en tiempo real de la cara de debajo (en gris, como el "
        "juego toma el color azul del vértice). Las zonas de viento (fx_wind) necesitan el mapa compilado, así que aquí los sprites no se mueven, "
        "salvo que la etapa oscile sola (`ssWindIdle`). El interruptor de hierba de la vista previa también los apaga aquí.",
        "**Tráva a další surface sprity:** fázi shaderu se `surfaceSprites` (tráva, keře, květiny, popínavé rostliny - například "
        "textures/yavin/ground_grasssprite) kreslí světlo v reálném čase jako hra: mnoho malých spritů na každé stěně brushe, patchi a zapečeném "
        "`misc_model` s tímto shaderem, se stejnou hustotou, velikostí, rozptylem, mizením v dálce (`ssFademax`, `ssFadescale`) a rozložením jako "
        "**{{KeSpielbildMenue}}**. Každá plocha si sprity drží, dokud se nezmění ona nebo její shader; pohyb kamery stojí jen sprity v dosahu a "
        "v obraze. Sprity dostávají světlo v reálném čase plochy pod nimi (šedě, jako hra bere modrou barvu vrcholu). Větrné zóny (fx_wind) "
        "potřebují sestavenou mapu, proto tu sprity stojí, pokud fáze nekývá sama (`ssWindIdle`). Přepínač trávy v náhledu je vypne i tady."),
    "gras_bild": (
        "A test room in real-time light: the floor with textures/yavin/ground_grasssprite and a patch on the stone block carry their grass sprites.",
        "Ein Testraum im Echtzeit-Licht: der Boden mit textures/yavin/ground_grasssprite und ein Patch auf dem Steinblock tragen ihre Halme.",
        "实时光照中的测试房间:使用 textures/yavin/ground_grasssprite 的地面和石块上的补丁带有草精灵。",
        "リアルタイムライトのテストルーム:textures/yavin/ground_grasssprite の床と石ブロック上のパッチに草のスプライトが生えています。",
        "Una sala de prueba con luz en tiempo real: el suelo con textures/yavin/ground_grasssprite y un parche sobre el bloque de piedra llevan su hierba.",
        "Testovací místnost ve světle v reálném čase: podlaha s textures/yavin/ground_grasssprite a patch na kamenném bloku nesou svou trávu."),
    "spiegel": (
        "A mirror or portal face shows **all** stages of its shader over the mirror view, like the game draws it (RB_StageIteratorGeneric) - for "
        "example a floor texture with an alpha channel above a tinted mirror. A second mirror counts as soon as it is on screen; with two on "
        "screen only one view is drawn, as in the game (\"only one mirror view at a time\"). The entity of a mirror is the first "
        "misc_portal_surface within 64 units of its plane, as in the game.",
        "Eine Spiegel- oder Portalfläche zeigt **alle** Stufen ihres Shaders über der Spiegelsicht, wie das Spiel sie zeichnet "
        "(RB_StageIteratorGeneric) - zum Beispiel eine Bodentextur mit Alphakanal über einem getönten Spiegel. Ein zweiter Spiegel zählt, sobald "
        "er im Bild ist; sind zwei im Bild, wird wie im Spiel nur eine Sicht gezeichnet („only one mirror view at a time“). Die Entity eines "
        "Spiegels ist wie im Spiel das erste misc_portal_surface höchstens 64 Einheiten von seiner Ebene.",
        "镜子或传送门面会在镜像视野之上显示其着色器的**所有**阶段,与游戏的绘制方式相同(RB_StageIteratorGeneric)——例如在着色的镜子上方带 alpha 通道的地面纹理。"
        "第二面镜子一出现在画面中就会生效;画面中有两面时,与游戏一样只绘制一个视野(“only one mirror view at a time”)。与游戏一样,镜子的实体是距其平面 64 单位以内的第一个 misc_portal_surface。",
        "ミラーやポータルの面は、ゲームと同じく(RB_StageIteratorGeneric)ミラー視界の上にシェーダーの**すべての**ステージを表示します。たとえば色付きの"
        "ミラーの上にアルファチャンネル付きの床テクスチャ。2 枚目のミラーは画面に入るとすぐ使われます。2 枚が画面にあるときは、ゲームと同じく視界は 1 つだけ"
        "です(\"only one mirror view at a time\")。ミラーのエンティティは、ゲームと同じく平面から 64 ユニット以内の最初の misc_portal_surface です。",
        "Una cara de espejo o portal muestra **todas** las etapas de su shader sobre la vista del espejo, como lo dibuja el juego "
        "(RB_StageIteratorGeneric); por ejemplo, una textura de suelo con canal alfa sobre un espejo teñido. Un segundo espejo cuenta en cuanto "
        "está en la imagen; con dos en la imagen solo se dibuja una vista, como en el juego («only one mirror view at a time»). La entidad de un "
        "espejo es, como en el juego, el primer misc_portal_surface a menos de 64 unidades de su plano.",
        "Plocha zrcadla nebo portálu ukazuje **všechny** fáze svého shaderu nad pohledem zrcadla, jak ji kreslí hra (RB_StageIteratorGeneric) - "
        "například texturu podlahy s alfa kanálem nad tónovaným zrcadlem. Druhé zrcadlo platí, jakmile je v obraze; jsou-li v obraze dvě, kreslí "
        "se jako ve hře jen jeden pohled („only one mirror view at a time“). Entitou zrcadla je jako ve hře první misc_portal_surface nejvýše 64 "
        "jednotek od jeho roviny."),
    "spiegel_bild": (
        "A mirror floor with two stages in real-time light: the tinted mirror (blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA) and above it a floor "
        "texture whose transparent half shows the reflection.",
        "Ein Spiegelboden mit zwei Stufen im Echtzeit-Licht: der getönte Spiegel (blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA) und darüber eine "
        "Bodentextur, deren durchsichtige Hälfte das Spiegelbild zeigt.",
        "实时光照中带两个阶段的镜面地板:着色的镜子(blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA),其上是一张地面纹理,其透明的一半显示倒影。",
        "リアルタイムライトの 2 ステージのミラー床:色付きのミラー(blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA)と、その上の床テクスチャ。床の透明な半分に映り込みが見えます。",
        "Un suelo espejo con dos etapas con luz en tiempo real: el espejo teñido (blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA) y encima una textura "
        "de suelo cuya mitad transparente muestra el reflejo.",
        "Zrcadlová podlaha se dvěma fázemi ve světle v reálném čase: tónované zrcadlo (blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA) a nad ním "
        "textura podlahy, jejíž průhledná polovina ukazuje odraz."),
    "schliessen": (
        "Closing the program - the cross of the main window, <kbd>Alt</kbd>+<kbd>F4</kbd> or **{{KeLFile}} > Exit** - asks about a map with "
        "unsaved changes, also when the map editor is in a window of its own; the question appears in the map editor. Closing only the map "
        "editor's own window puts the tab back into the main window and keeps the map.",
        "Das Programm schließen - das Kreuz des Hauptfensters, <kbd>Alt</kbd>+<kbd>F4</kbd> oder **{{KeLFile}} > Exit** - fragt nach einer Karte "
        "mit ungespeicherten Änderungen, auch wenn der Karten-Editor in einem eigenen Fenster steht; die Frage erscheint im Karten-Editor. Wer nur "
        "das eigene Fenster des Karten-Editors schließt, bringt den Tab ins Hauptfenster zurück und behält die Karte.",
        "关闭程序——主窗口的关闭按钮、<kbd>Alt</kbd>+<kbd>F4</kbd> 或 **{{KeLFile}} > Exit**——会询问有未保存更改的地图,即使地图编辑器在单独的窗口中;"
        "问题显示在地图编辑器中。只关闭地图编辑器自己的窗口会把选项卡放回主窗口并保留地图。",
        "プログラムを閉じるとき(メインウィンドウの × ボタン、<kbd>Alt</kbd>+<kbd>F4</kbd>、**{{KeLFile}} > Exit**)は、マップエディターが別ウィンドウに"
        "あっても、未保存の変更があるマップについて確認します。確認はマップエディターに表示されます。マップエディター自身のウィンドウだけを閉じると、タブは"
        "メインウィンドウに戻り、マップはそのまま残ります。",
        "Cerrar el programa (la cruz de la ventana principal, <kbd>Alt</kbd>+<kbd>F4</kbd> o **{{KeLFile}} > Exit**) pregunta por un mapa con "
        "cambios sin guardar, también si el editor de mapas está en su propia ventana; la pregunta aparece en el editor de mapas. Cerrar solo la "
        "ventana propia del editor de mapas devuelve la pestaña a la ventana principal y conserva el mapa.",
        "Zavření programu - křížek hlavního okna, <kbd>Alt</kbd>+<kbd>F4</kbd> nebo **{{KeLFile}} > Exit** - se zeptá na mapu s neuloženými "
        "změnami, i když je editor map ve vlastním okně; otázka se objeví v editoru map. Kdo zavře jen vlastní okno editoru map, vrátí kartu do "
        "hlavního okna a mapu si ponechá."),
    "lumps": (
        "If the window **Shaders and surfaces** gets changes while the part is compiling, the editor asks before inserting: **Keep** saves them "
        "first and inserts on top of them, **Replace** discards them, **Cancel** inserts nothing.",
        "Ändert sich im Fenster **Shader und Flächen** etwas, während der Teil gebaut wird, fragt der Editor vor dem Einfügen: **Behalten** "
        "speichert die Änderungen zuerst und fügt darauf ein, **Ersetzen** verwirft sie, **Abbrechen** fügt nichts ein.",
        "如果在编译该部分时**着色器和表面**窗口中有更改,编辑器会在插入前询问:**保留**先保存这些更改再在其上插入,**替换**丢弃它们,**取消**不插入任何内容。",
        "パーツのコンパイル中に**シェーダーとサーフェス**ウィンドウで変更があると、エディターは挿入の前に確認します。**残す**は変更を先に保存してその上に挿入し、"
        "**置き換える**は変更を破棄し、**キャンセル**は何も挿入しません。",
        "Si la ventana **Shaders y superficies** cambia mientras se compila la parte, el editor pregunta antes de insertar: **Conservar** guarda "
        "primero los cambios e inserta sobre ellos, **Sustituir** los descarta, **Cancelar** no inserta nada.",
        "Změní-li se během sestavování části něco v okně **Shadery a plochy**, zeptá se editor před vložením: **Ponechat** změny nejdřív uloží "
        "a vloží na ně, **Nahradit** je zahodí, **Zrušit** nevloží nic."),
}

ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}


def _t(schluessel):
    from inhalt_basis import T
    t = X[schluessel]
    return T(t[0], t[1], t[2], t[3])


def einhaengen(tempo, editor, bspmerge):
    """Gras und Spiegel nach dem Bild h_karte_portale (karte_tempo), Schliessen und Lumps als Notizen."""
    b = tempo["bloecke"]
    i = next(n for n, x in enumerate(b) if x[0] == "img" and x[1] == "h_karte_portale")
    b.insert(i + 1, ("p", _t("spiegel")))
    b.insert(i + 2, ("img", "h_r2a_spiegel", _t("spiegel_bild")))
    b.insert(i + 3, ("p", _t("gras")))
    b.insert(i + 4, ("img", "h_r2a_gras", _t("gras_bild")))
    e = editor["bloecke"]
    j = next(n for n, x in enumerate(e) if x[0] == "p")
    e.insert(j + 1, ("note", _t("schliessen")))
    bspmerge["bloecke"].append(("note", _t("lumps")))
