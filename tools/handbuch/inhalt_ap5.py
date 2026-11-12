"""AP5 (MBENTWURF Teil B): die Seite "Model-Tab: Ansehen und Pruefen" - alles, was ModView kann, im Startbereich des Model-Tabs:
Gruppen Modell (Importieren), Auswahl, Detailstufe und Skin, Boden und Kamera, Anbauten, Pruefung fuer das Spiel, Szene (.mvs); die
Pruefanzeigen von ModView in den Menues der Kopfzeile; der Hinweis "LOD - Dreiecke" in der Ansicht; vier Ansichten wie 3ds Max;
Overlays und Schattierung wie Blender.

Eigene Quelle des Pakets; AP14 haengt die Seite ins Handbuch (inhalt.py: `inhalt_ap5.seite()` hinter modell-bedienung, ES/CS in
inhalt_es.py / inhalt_cs.py, die Texte von i18n_ap5 in gen_handbuch.py `_lade_ui_texte`) - wie AP4, AP6-AP10. Die Bilder h_ap5_max,
h_ap5_blender und h_ap5_pruefen nimmt der Selbsttest auf (BHED_EDITORTEST=handbuch, BHED_HB_NUR=ap5). Jeder Text einmal in allen
sechs Sprachen (en, de, zh, ja, es, cs); {{Kennung}} setzt den Text der Oberflaeche ein.
"""

X = {
    "titel": ("Model tab: view and check", "Model-Tab: Ansehen und Prüfen", "模型选项卡: 查看与检查", "モデルタブ: 表示とチェック",
              "Pestaña Model: ver y comprobar", "Karta Model: prohlížení a kontrola"),
    "kurz": (
        "The area View/Check is where a model opens: everything ModView can do - levels of detail, skins, attachments, floor and camera, the check against the limits of the game and scenes as ModView scripts - drawn as in the game.",
        "Der Bereich Ansehen/Prüfen ist der Startbereich eines Modells: alles, was ModView kann - Detailstufen, Skins, Anbauten, Boden und Kamera, die Prüfung gegen die Grenzen des Spiels und Szenen als ModView-Skript - gezeichnet wie im Spiel.",
        "“查看/检查”区域是打开模型时的起始区域: ModView 能做的一切 - 细节级别、皮肤、附件、地面和相机、按游戏上限检查, 以及以 ModView 脚本保存的场景 - 均按游戏中的方式绘制。",
        "表示/チェックはモデルを開いたときの開始エリアです。ModView でできること (詳細レベル、スキン、アタッチメント、床とカメラ、ゲームの上限によるチェック、ModView スクリプトとしてのシーン) をゲームと同じ描画で行えます。",
        "El área Ver/Comprobar es donde se abre un modelo: todo lo que hace ModView - niveles de detalle, skins, acoplados, suelo y cámara, la comprobación frente a los límites del juego y escenas como scripts de ModView - dibujado como en el juego.",
        "Oblast Prohlížení/Kontrola je výchozí oblastí modelu: vše, co umí ModView - úrovně detailu, skiny, připojené modely, podlaha a kamera, kontrola proti limitům hry a scény jako skripty ModView - vykresleno jako ve hře."),
    "bild_max": (
        "Like 3ds Max: the command panel on the tab Display holds the groups Model, Selection, Level of detail and skin, Floor and camera, Attachments, Check for the game and Scene; bottom right in the view the level of detail and the triangles.",
        "Wie 3ds Max: das Befehlsfeld im Reiter Anzeige mit den Gruppen Modell, Auswahl, Detailstufe und Skin, Boden und Kamera, Anbauten, Prüfung für das Spiel und Szene; unten rechts in der Ansicht Detailstufe und Dreiecke.",
        "像 3ds Max: “显示”选项卡中的命令面板含模型、所选内容、细节级别和皮肤、地面和相机、附件、针对游戏检查和场景各组; 视图右下角显示细节级别和三角形数。",
        "3ds Max 風: 表示タブのコマンドパネルにモデル、選択、詳細レベルとスキン、床とカメラ、アタッチメント、ゲーム向けチェック、シーンのグループ。ビューの右下に詳細レベルと三角形数。",
        "Como 3ds Max: el panel de comandos en la pestaña Mostrar con los grupos Modelo, Selección, Nivel de detalle y skin, Suelo y cámara, Acoplados, Comprobar para el juego y Escena; abajo a la derecha en la vista el nivel de detalle y los triángulos.",
        "Jako 3ds Max: příkazový panel na kartě Zobrazení se skupinami Model, Výběr, Úroveň detailu a skin, Podlaha a kamera, Připojené modely, Kontrola pro hru a Scéna; vpravo dole v pohledu úroveň detailu a trojúhelníky."),
    "bild_blender": (
        "The same area like Blender: the same groups in the properties editor (tab Object) below the outliner, the toolbar with Measure, Select surface, Select bone and Photo on the left.",
        "Derselbe Bereich wie Blender: dieselben Gruppen im Eigenschaften-Editor (Reiter Objekt) unter dem Outliner, links die Werkzeugleiste mit Messen, Fläche wählen, Knochen wählen und Foto.",
        "同一区域像 Blender 时: 相同的各组位于大纲视图下方的属性编辑器 (物体选项卡) 中, 左侧是含测量、选择面、选择骨骼和照片的工具栏。",
        "同じエリアの Blender 風: アウトライナーの下のプロパティエディター (オブジェクトタブ) に同じグループ、左にメジャー、サーフェスを選択、ボーンを選択、写真のツールバー。",
        "La misma área como Blender: los mismos grupos en el editor de propiedades (pestaña Objeto) bajo el Outliner; a la izquierda la barra de herramientas con Medir, Seleccionar superficie, Seleccionar hueso y Foto.",
        "Stejná oblast jako Blender: stejné skupiny v editoru vlastností (karta Objekt) pod Outlinerem, vlevo panel nástrojů s Měřit, Vybrat plochu, Vybrat kost a Fotka."),
    "h_gruppen": ("The groups", "Die Gruppen", "各组", "グループ", "Los grupos", "Skupiny"),
    "gruppen": (
        "**{{MbRModell}}** shows file and skeleton and opens a model with **{{PqImport}}** (from the disk or from the .pk3 of the game); **{{MbTexNeu}}** has Alt+R in both control modes. "
        "**{{PqRAuswahl}}** lists the values of the selected surface, tag or bone (vertices, triangles, bone references, shader, parent, state). "
        "**{{MbRLod}}**: all ten levels can be chosen like r_lodbias in the game - a level above the last one of the model shows the last one, and the sentence below says so; the skins stand in two columns. "
        "**{{PqRBoden}}**: floor at -24 or under the lowest point of the pose, field of view 10°, 80° or 90°, a plain background, the weight threshold, **{{PqAnsichtZurueck}}** and **{{PqFoto}}**.",
        "**{{MbRModell}}** zeigt Datei und Skelett und öffnet mit **{{PqImport}}** ein Modell (von der Platte oder aus den .pk3 des Spiels); **{{MbTexNeu}}** hat in beiden Bedienmodi Alt+R. "
        "**{{PqRAuswahl}}** nennt die Werte der gewählten Fläche, des Tags oder Knochens (Ecken, Dreiecke, Knochenbezüge, Shader, Eltern, Status). "
        "**{{MbRLod}}**: alle zehn Stufen sind wählbar wie r_lodbias im Spiel - eine Stufe über der letzten des Modells zeigt die letzte, und der Satz darunter sagt es; die Skins stehen in zwei Spalten. "
        "**{{PqRBoden}}**: Boden auf -24 oder unter dem tiefsten Punkt der Pose, Sichtfeld 10°, 80° oder 90°, ein einfarbiger Hintergrund, die Gewicht-Schwelle, **{{PqAnsichtZurueck}}** und **{{PqFoto}}**.",
        "**{{MbRModell}}** 显示文件和骨骼, 并可用 **{{PqImport}}** 打开模型 (从磁盘或游戏的 .pk3); **{{MbTexNeu}}** 在两种操作模式下都可按 Alt+R。"
        "**{{PqRAuswahl}}** 列出所选面、标签或骨骼的数值 (顶点、三角形、骨骼引用、着色器、父级、状态)。"
        "**{{MbRLod}}**: 十个级别都可选择, 与游戏中的 r_lodbias 相同 - 高于模型最后一级的级别显示最后一级, 下方的句子会说明; 皮肤分两列显示。"
        "**{{PqRBoden}}**: 地面位于 -24 或姿势最低点之下, 视野 10°、80° 或 90°, 纯色背景, 权重阈值, **{{PqAnsichtZurueck}}** 和 **{{PqFoto}}**。",
        "**{{MbRModell}}** はファイルと骨格を表示し、**{{PqImport}}** でモデルを開きます (ディスクから、またはゲームの .pk3 から)。**{{MbTexNeu}}** はどちらの操作モードでも Alt+R です。"
        "**{{PqRAuswahl}}** は選択したサーフェス、タグ、ボーンの値 (頂点、三角形、ボーン参照、シェーダー、親、状態) を示します。"
        "**{{MbRLod}}**: ゲームの r_lodbias と同じく 10 段階すべてを選べます。モデルの最後のレベルより上を選ぶと最後のレベルを表示し、下の文がそれを示します。スキンは 2 列に並びます。"
        "**{{PqRBoden}}**: 床を -24 またはポーズの最低点の下に、視野角 10°/80°/90°、単色の背景、ウェイトしきい値、**{{PqAnsichtZurueck}}** と **{{PqFoto}}**。",
        "**{{MbRModell}}** muestra el archivo y el esqueleto y abre un modelo con **{{PqImport}}** (desde el disco o desde los .pk3 del juego); **{{MbTexNeu}}** tiene Alt+R en ambos modos de control. "
        "**{{PqRAuswahl}}** enumera los valores de la superficie, el tag o el hueso elegido (vértices, triángulos, referencias de huesos, shader, padre, estado). "
        "**{{MbRLod}}**: los diez niveles se pueden elegir como r_lodbias en el juego: un nivel por encima del último del modelo muestra el último, y la frase de abajo lo dice; los skins están en dos columnas. "
        "**{{PqRBoden}}**: suelo a -24 o bajo el punto más bajo de la pose, campo de visión de 10°, 80° o 90°, un fondo liso, el umbral de peso, **{{PqAnsichtZurueck}}** y **{{PqFoto}}**.",
        "**{{MbRModell}}** ukazuje soubor a kostru a tlačítkem **{{PqImport}}** otevře model (z disku nebo z .pk3 hry); **{{MbTexNeu}}** má v obou režimech ovládání Alt+R. "
        "**{{PqRAuswahl}}** uvádí hodnoty vybrané plochy, tagu nebo kosti (vrcholy, trojúhelníky, odkazy kostí, shader, rodič, stav). "
        "**{{MbRLod}}**: všech deset úrovní lze zvolit jako r_lodbias ve hře - úroveň nad poslední úrovní modelu ukazuje tu poslední a věta pod tím to říká; skiny jsou ve dvou sloupcích. "
        "**{{PqRBoden}}**: podlaha na -24 nebo pod nejnižším bodem pózy, zorné pole 10°, 80° nebo 90°, jednobarevné pozadí, práh váhy, **{{PqAnsichtZurueck}}** a **{{PqFoto}}**."),
    "bild_pruefen": (
        "The check for the game: bars of the selected surface against the limits of the target game, the rows of the design; in the view red dashes on surfaces without their own shadow and the highlighted surface with its bone references.",
        "Die Prüfung für das Spiel: Balken der gewählten Fläche gegen die Grenzen des Zielspiels, die Zeilen des Entwurfs; in der Ansicht rote Striche auf Flächen ohne eigenen Schatten und die hervorgehobene Fläche mit ihren Knochenbezügen.",
        "针对游戏的检查: 所选面相对于目标游戏上限的进度条以及设计中的各行; 视图中无自身阴影的面显示红色虚线, 高亮的面显示其骨骼引用。",
        "ゲーム向けチェック: 選択したサーフェスの対象ゲーム上限に対するバーとデザインの各行。ビューでは自前の影を落とさないサーフェスに赤い破線、強調したサーフェスとそのボーン参照。",
        "La comprobación para el juego: barras de la superficie elegida frente a los límites del juego de destino y las filas del diseño; en la vista, trazos rojos en superficies sin sombra propia y la superficie resaltada con sus referencias de huesos.",
        "Kontrola pro hru: pruhy vybrané plochy proti limitům cílové hry a řádky návrhu; v pohledu červené čárky na plochách bez vlastního stínu a zvýrazněná plocha s odkazy kostí."),
    "h_pruefen": ("Check for the game", "Prüfung für das Spiel", "针对游戏检查", "ゲーム向けチェック", "Comprobar para el juego", "Kontrola pro hru"),
    "pruefen": (
        "**{{MbRPruefen}}** measures the selected surface (without a selection the one closest to a limit) against the target game: vertices and bones per surface, weights per vertex. "
        "Below stand **{{PqPrFlaechen}}**, **{{PqPrSequenzen}}** (sequences of the animation.cfg that end after the GLA), **{{PqPrTags}}** (the tags the player code bolts to - SP only for a standard skeleton), "
        "**{{PqPrShader}}**, **{{PqPrLods}}** and **{{PqPrSchatten}}**: a surface with half of SHADER_MAX_VERTEXES or more vertices does not cast its own stencil shadow; the game takes the same surface of the last LOD.",
        "**{{MbRPruefen}}** misst die gewählte Fläche (ohne Auswahl die, die einer Grenze am nächsten ist) gegen das Zielspiel: Ecken und Knochen je Fläche, Gewichte je Ecke. "
        "Darunter stehen **{{PqPrFlaechen}}**, **{{PqPrSequenzen}}** (Sequenzen der animation.cfg, die hinter der GLA enden), **{{PqPrTags}}** (die Tags, an die der Spielercode anbaut - im SP nur für ein Standard-Skelett), "
        "**{{PqPrShader}}**, **{{PqPrLods}}** und **{{PqPrSchatten}}**: eine Fläche mit der Hälfte von SHADER_MAX_VERTEXES oder mehr Ecken wirft keinen eigenen Stencil-Schatten; das Spiel nimmt dieselbe Fläche der letzten LOD.",
        "**{{MbRPruefen}}** 按目标游戏测量所选面 (未选择时取最接近上限的面): 每个面的顶点和骨骼, 每个顶点的权重。"
        "下方是 **{{PqPrFlaechen}}**、**{{PqPrSequenzen}}** (animation.cfg 中结束于 GLA 之后的序列)、**{{PqPrTags}}** (玩家代码挂载的标签 - 单人模式仅限标准骨骼)、"
        "**{{PqPrShader}}**、**{{PqPrLods}}** 和 **{{PqPrSchatten}}**: 顶点数达到 SHADER_MAX_VERTEXES 一半或更多的面不投射自身的模板阴影; 游戏改用最后一级 LOD 的同一面。",
        "**{{MbRPruefen}}** は選択したサーフェス (未選択なら上限に最も近いもの) を対象ゲームで測ります: サーフェスごとの頂点とボーン、頂点ごとのウェイト。"
        "その下に **{{PqPrFlaechen}}**、**{{PqPrSequenzen}}** (GLA より後で終わる animation.cfg のシーケンス)、**{{PqPrTags}}** (プレイヤーコードが取り付けるタグ。SP では標準骨格のみ)、"
        "**{{PqPrShader}}**、**{{PqPrLods}}**、**{{PqPrSchatten}}** があります。頂点が SHADER_MAX_VERTEXES の半分以上のサーフェスは自前のステンシル影を落とさず、ゲームは最後の LOD の同じサーフェスを使います。",
        "**{{MbRPruefen}}** mide la superficie elegida (sin selección, la más cercana a un límite) frente al juego de destino: vértices y huesos por superficie, pesos por vértice. "
        "Debajo están **{{PqPrFlaechen}}**, **{{PqPrSequenzen}}** (secuencias de animation.cfg que terminan después de la GLA), **{{PqPrTags}}** (los tags en los que el código del jugador acopla - en SP solo para un esqueleto estándar), "
        "**{{PqPrShader}}**, **{{PqPrLods}}** y **{{PqPrSchatten}}**: una superficie con la mitad de SHADER_MAX_VERTEXES o más vértices no proyecta su propia sombra de stencil; el juego usa la misma superficie del último LOD.",
        "**{{MbRPruefen}}** měří vybranou plochu (bez výběru tu nejblíž limitu) proti cílové hře: vrcholy a kosti na plochu, váhy na vrchol. "
        "Pod tím jsou **{{PqPrFlaechen}}**, **{{PqPrSequenzen}}** (sekvence animation.cfg, které končí za GLA), **{{PqPrTags}}** (tagy, na které kód hráče připojuje - v SP jen pro standardní kostru), "
        "**{{PqPrShader}}**, **{{PqPrLods}}** a **{{PqPrSchatten}}**: plocha s polovinou SHADER_MAX_VERTEXES nebo více vrcholy nevrhá vlastní stencilový stín; hra vezme stejnou plochu poslední LOD."),
    "h_anzeige": ("Check displays of ModView", "Prüfanzeigen von ModView", "ModView 的检查显示", "ModView のチェック表示", "Indicadores de ModView", "Kontrolní zobrazení ModView"),
    "anzeige": (
        "All display switches of ModView stand in the menus of the header of the view (like 3ds Max [Standard] and [+] > Bones and checks, like Blender Viewport Shading and Overlays), in Search all actions and in the context menu: "
        "**{{PqAlpha}}** off draws everything opaque, **{{PqFilter}}** off shows every pixel of the textures, **{{PqSchwelle}}** leaves out small weights when posing, **{{PqHervor}}** outlines the selected surface in yellow and names the bones it references, "
        "**{{PqSchatten}}** draws red dashes on surfaces without their own shadow. Like Blender Z chooses the shading and Shift+Alt+Z hides all overlays.",
        "Alle Anzeige-Schalter von ModView stehen in den Menüs der Kopfzeile der Ansicht (wie 3ds Max [Standard] und [+] > Knochen und Prüfen, wie Blender Viewport Shading und Overlays), in Alle Aktionen suchen und im Rechtsklick: "
        "**{{PqAlpha}}** aus zeichnet alles deckend, **{{PqFilter}}** aus zeigt jeden Bildpunkt der Texturen, **{{PqSchwelle}}** lässt beim Posieren kleine Gewichte weg, **{{PqHervor}}** umrandet die gewählte Fläche gelb und nennt die Knochen, auf die sie sich bezieht, "
        "**{{PqSchatten}}** zeichnet rote Striche auf Flächen ohne eigenen Schatten. Wie Blender wählt Z die Schattierung, und Umschalt+Alt+Z blendet alle Overlays aus.",
        "ModView 的所有显示开关都位于视图标题栏的菜单中 (像 3ds Max 的 [标准] 和 [+] > 骨骼与检查, 像 Blender 的视图着色和叠加层), 也在“搜索所有操作”和右键菜单中: "
        "关闭 **{{PqAlpha}}** 时全部不透明绘制, 关闭 **{{PqFilter}}** 时可看到纹理的每个像素, **{{PqSchwelle}}** 在摆姿势时略去小权重, **{{PqHervor}}** 用黄色勾勒所选面并标出其引用的骨骼, "
        "**{{PqSchatten}}** 在无自身阴影的面上画红色虚线。像 Blender 一样, Z 选择着色, Shift+Alt+Z 隐藏所有叠加层。",
        "ModView のすべての表示スイッチは、ビューのヘッダーのメニュー (3ds Max 風は [標準] と [+] > ボーンとチェック、Blender 風はビューポートシェーディングとオーバーレイ)、「すべてのアクションを検索」、右クリックメニューにあります: "
        "**{{PqAlpha}}** をオフにするとすべて不透明、**{{PqFilter}}** をオフにするとテクスチャのピクセルが見え、**{{PqSchwelle}}** はポーズ時に小さなウェイトを省き、**{{PqHervor}}** は選択したサーフェスを黄色で囲み参照ボーンを示し、"
        "**{{PqSchatten}}** は自前の影を落とさないサーフェスに赤い破線を描きます。Blender と同じく Z でシェーディングを選び、Shift+Alt+Z ですべてのオーバーレイを隠します。",
        "Todos los interruptores de visualización de ModView están en los menús de la cabecera de la vista (como 3ds Max [Estándar] y [+] > Huesos y comprobación, como Blender Sombreado de la vista y Superposiciones), en Buscar todas las acciones y en el menú contextual: "
        "**{{PqAlpha}}** desactivado dibuja todo opaco, **{{PqFilter}}** desactivado muestra cada píxel de las texturas, **{{PqSchwelle}}** omite pesos pequeños al posar, **{{PqHervor}}** rodea de amarillo la superficie elegida y nombra los huesos a los que hace referencia, "
        "**{{PqSchatten}}** dibuja trazos rojos en superficies sin sombra propia. Como Blender, Z elige el sombreado y Mayús+Alt+Z oculta todas las superposiciones.",
        "Všechny přepínače zobrazení ModView jsou v nabídkách záhlaví pohledu (jako 3ds Max [Standard] a [+] > Kosti a kontrola, jako Blender Stínování pohledu a Překryvy), ve Vyhledat všechny akce a v místní nabídce: "
        "vypnuté **{{PqAlpha}}** kreslí vše neprůhledně, vypnuté **{{PqFilter}}** ukazuje každý pixel textur, **{{PqSchwelle}}** při pózování vynechá malé váhy, **{{PqHervor}}** žlutě orámuje vybranou plochu a pojmenuje kosti, na které odkazuje, "
        "**{{PqSchatten}}** kreslí červené čárky na plochách bez vlastního stínu. Jako v Blenderu volí Z stínování a Shift+Alt+Z skryje všechny překryvy."),
    "h_szene": ("Scene and views", "Szene und Ansichten", "场景与视图", "シーンとビュー", "Escena y vistas", "Scéna a pohledy"),
    "szene": (
        "**{{PqSzeneSpeichern}}** writes model, skin, attachments, primary and secondary sequence, surfaces on and off, root, level of detail and view as a ModView script (.mvs) - ModView reads it too; **{{PqMvsLesen}}** reads such a script and names what it does not take over. "
        "Like 3ds Max the areas Modeling, Weights, Painting and Animation start with four views (Top, Front, Left, Perspective); Alt+W maximizes, and every area keeps its choice. A .glm always opens in View/Check.",
        "**{{PqSzeneSpeichern}}** schreibt Modell, Skin, Anbauten, Primär- und Sekundärsequenz, Flächen an und aus, Wurzel, Detailstufe und Ansicht als ModView-Skript (.mvs) - ModView liest es auch; **{{PqMvsLesen}}** liest ein solches Skript und nennt, was es nicht übernimmt. "
        "Wie 3ds Max beginnen die Bereiche Modellieren, Weights, Painting und Animation mit vier Ansichten (Oben, Vorne, Links, Perspektive); Alt+W maximiert, und jeder Bereich behält seine Wahl. Eine .glm öffnet immer in Ansehen/Prüfen.",
        "**{{PqSzeneSpeichern}}** 将模型、皮肤、附件、主/次序列、面的开关、根、细节级别和视图写成 ModView 脚本 (.mvs) - ModView 也能读取; **{{PqMvsLesen}}** 读取这样的脚本, 并列出未采用的内容。"
        "像 3ds Max 一样, 建模、权重、绘制和动画区域以四个视图 (顶、前、左、透视) 开始; Alt+W 最大化, 每个区域会记住自己的选择。打开 .glm 时总是进入“查看/检查”。",
        "**{{PqSzeneSpeichern}}** はモデル、スキン、アタッチメント、プライマリ/セカンダリシーケンス、サーフェスのオン/オフ、ルート、詳細レベル、ビューを ModView スクリプト (.mvs) として書き出します (ModView でも読めます)。**{{PqMvsLesen}}** はそのスクリプトを読み、取り込まなかったものを示します。"
        "3ds Max と同じく、モデリング、ウェイト、ペイント、アニメーションのエリアは 4 つのビュー (上、前、左、パース) で始まります。Alt+W で最大化し、各エリアが選択を記憶します。.glm は常に表示/チェックで開きます。",
        "**{{PqSzeneSpeichern}}** escribe modelo, skin, acoplados, secuencia primaria y secundaria, superficies encendidas y apagadas, raíz, nivel de detalle y vista como script de ModView (.mvs) - ModView también lo lee; **{{PqMvsLesen}}** lee un script así y nombra lo que no toma. "
        "Como 3ds Max, las áreas Modelado, Weights, Painting y Animación empiezan con cuatro vistas (Arriba, Frente, Izquierda, Perspectiva); Alt+W maximiza y cada área conserva su elección. Un .glm siempre se abre en Ver/Comprobar.",
        "**{{PqSzeneSpeichern}}** zapíše model, skin, připojené modely, primární a sekundární sekvenci, plochy zapnuté a vypnuté, kořen, úroveň detailu a pohled jako skript ModView (.mvs) - ModView ho přečte také; **{{PqMvsLesen}}** takový skript načte a uvede, co nepřevzal. "
        "Jako 3ds Max začínají oblasti Modelování, Weights, Painting a Animace se čtyřmi pohledy (Shora, Zepředu, Zleva, Perspektiva); Alt+W maximalizuje a každá oblast si pamatuje svou volbu. Soubor .glm se vždy otevře v Prohlížení/Kontrola."),
}

ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}


def _t(schluessel):
    from inhalt_basis import T
    z = X[schluessel]
    return T(z[0], z[1], z[2], z[3])


def seite():
    """Die Seite modell-pruefen (Gruppe Missionen, hinter modell-bedienung)."""
    return {
        "id": "modell-pruefen",
        "gruppe": "mission",
        "titel": _t("titel"),
        "kurz": _t("kurz"),
        "bloecke": [
            ("img", "h_ap5_max", _t("bild_max"), "breit"),
            ("img", "h_ap5_blender", _t("bild_blender"), "breit"),
            ("h2", "gruppen", _t("h_gruppen")),
            ("p", _t("gruppen")),
            ("img", "h_ap5_pruefen", _t("bild_pruefen"), "breit"),
            ("h2", "pruefen", _t("h_pruefen")),
            ("p", _t("pruefen")),
            ("h2", "anzeige", _t("h_anzeige")),
            ("p", _t("anzeige")),
            ("h2", "szene", _t("h_szene")),
            ("p", _t("szene")),
        ],
    }
