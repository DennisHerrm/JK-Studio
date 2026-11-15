# -*- coding: utf-8 -*-
"""Oberflaechentexte von CW-MODELL (Clone-Wars-Kette Teil 3, plaene/clonewars_kette.md): Detailstufen erzeugen und je Stufe
zaehlen, Bolts fuer Sitze und Muendungen, eigene Skelette und die animation.cfg fuer Fahrzeuge.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    # --- Detailstufen (Gruppe wie 3ds Max ProOptimizer / Blender Decimate) ---
    ("LodGruppeMax", "ProOptimizer - detail levels", "ProOptimizer - Detailstufen", "ProOptimizer - 细节级别", "ProOptimizer - 詳細レベル",
     "ProOptimizer - niveles de detalle", "ProOptimizer - úrovně detailu"),
    ("LodGruppeBl", "Decimate - detail levels", "Decimate - Detailstufen", "Decimate - 细节级别", "Decimate - 詳細レベル",
     "Decimate - niveles de detalle", "Decimate - úrovně detailu"),
    ("LodGruppeHint",
     "Generates the coarser detail levels of the whole model from LOD 1, like ProOptimizer in 3ds Max and Decimate (Collapse) in Blender: "
     "a .glm carries them in the file, a .md3 gets name_1.md3 and name_2.md3 next to it when saved. Every vertex of a coarser level is a "
     "vertex of LOD 1 (position, normal, UV, bones and weights stay as in the file, all frames of a .md3 stay). Borders between surfaces "
     "stay closed, tags, _off and _cap_ surfaces stay as they are. Nothing changes until you click Generate.",
     "Erzeugt die gröberen Detailstufen des ganzen Modells aus LOD 1, wie ProOptimizer in 3ds Max und Decimate (Collapse) in Blender: eine "
     ".glm trägt sie in der Datei, eine .md3 bekommt beim Speichern name_1.md3 und name_2.md3 daneben. Jede Ecke einer gröberen Stufe ist "
     "eine Ecke von LOD 1 (Ort, Normale, UV, Knochen und Gewichte bleiben wie in der Datei, alle Bilder einer .md3 bleiben). Ränder zwischen "
     "Flächen bleiben geschlossen, Tags, _off- und _cap_-Flächen bleiben, wie sie sind. Bis zum Klick auf Erzeugen ändert sich nichts.",
     "像 3ds Max 的 ProOptimizer 和 Blender 的 Decimate (Collapse) 一样, 从 LOD 1 生成整个模型的较粗细节级别: .glm 把它们存在文件中, .md3 在保存时"
     "旁边得到 name_1.md3 和 name_2.md3。较粗级别的每个顶点都是 LOD 1 的顶点(位置、法线、UV、骨骼和权重保持文件中的样子, .md3 的所有帧都保留)。"
     "表面之间的边界保持闭合, 标签、_off 和 _cap_ 表面保持原样。点击“生成”之前什么都不会改变。",
     "3ds Max の ProOptimizer や Blender の Decimate (Collapse) のように、LOD 1 からモデル全体の粗い詳細レベルを作ります。.glm はファイル内に、"
     ".md3 は保存時に横へ name_1.md3 と name_2.md3 を持ちます。粗いレベルの頂点はすべて LOD 1 の頂点です(位置・法線・UV・ボーン・ウェイトは"
     "ファイルのまま、.md3 の全フレームも残ります)。面と面の境界は閉じたまま、タグ・_off・_cap_ の面はそのままです。「生成」を押すまで何も"
     "変わりません。",
     "Genera los niveles de detalle más bastos de todo el modelo a partir de LOD 1, como ProOptimizer en 3ds Max y Decimate (Collapse) en "
     "Blender: un .glm los lleva en el archivo, un .md3 recibe al guardar name_1.md3 y name_2.md3 al lado. Cada vértice de un nivel más "
     "basto es un vértice de LOD 1 (posición, normal, UV, huesos y pesos como en el archivo; todos los fotogramas de un .md3 se conservan). "
     "Los bordes entre superficies quedan cerrados; las etiquetas y las superficies _off y _cap_ quedan como están. Nada cambia hasta pulsar "
     "Generar.",
     "Vytvoří hrubší úrovně detailu celého modelu z LOD 1, jako ProOptimizer v 3ds Max a Decimate (Collapse) v Blenderu: .glm je nese v "
     "souboru, .md3 dostane při uložení vedle sebe name_1.md3 a name_2.md3. Každý vrchol hrubší úrovně je vrcholem LOD 1 (poloha, normála, "
     "UV, kosti a váhy zůstanou jako v souboru, všechny snímky .md3 zůstanou). Hranice mezi plochami zůstanou uzavřené, tagy a plochy _off a "
     "_cap_ zůstanou, jak jsou. Do kliknutí na Vytvořit se nic nezmění."),
    ("LodKurzinfo", "%d LODs", "%d Stufen", "%d 级", "%d レベル", "%d niveles", "%d úrovní"),
    ("LodVorlage", "Template", "Vorlage", "模板", "テンプレート", "Plantilla", "Šablona"),
    ("LodVorlageHint",
     "How much each level keeps, measured on the game files of Jedi Academy (median of the models with detail levels). The fitting one "
     "is chosen from the path and the skeleton of the model.",
     "Wie viel jede Stufe behält, gemessen an den Spieldateien von Jedi Academy (Median der Modelle mit Detailstufen). Die passende wird nach "
     "Pfad und Skelett des Modells gewählt.",
     "每个级别保留多少, 按 Jedi Academy 游戏文件测得(带细节级别的模型的中位数)。根据模型的路径和骨架选择合适的一个。",
     "各レベルがどれだけ残すか。Jedi Academy のゲームファイルで測定(詳細レベルを持つモデルの中央値)。モデルのパスとスケルトンから合うものが"
     "選ばれます。",
     "Cuánto conserva cada nivel, medido en los archivos de Jedi Academy (mediana de los modelos con niveles de detalle). La adecuada se "
     "elige por la ruta y el esqueleto del modelo.",
     "Kolik si každá úroveň ponechá, změřeno na herních souborech Jedi Academy (medián modelů s úrovněmi detailu). Vhodná se zvolí podle "
     "cesty a kostry modelu."),
    ("LodVorFigur", "Figure", "Figur", "人物", "フィギュア", "Figura", "Postava"),
    ("LodVorFigurHint", "Figures on _humanoid: LOD 2 keeps 0.65, LOD 3 0.40 of the vertices (Jedi Academy: 0.67 / 0.41, 51 models).",
     "Figuren auf _humanoid: LOD 2 behält 0,65, LOD 3 0,40 der Ecken (Jedi Academy: 0,67 / 0,41, 51 Modelle).",
     "_humanoid 上的人物: LOD 2 保留 0.65 顶点, LOD 3 保留 0.40(Jedi Academy: 0.67 / 0.41, 51 个模型)。",
     "_humanoid のフィギュア: LOD 2 は頂点の 0.65、LOD 3 は 0.40 を残します(Jedi Academy: 0.67 / 0.41、51 モデル)。",
     "Figuras en _humanoid: LOD 2 conserva 0,65, LOD 3 0,40 de los vértices (Jedi Academy: 0,67 / 0,41, 51 modelos).",
     "Postavy na _humanoid: LOD 2 ponechá 0,65, LOD 3 0,40 vrcholů (Jedi Academy: 0,67 / 0,41, 51 modelů)."),
    ("LodVorAusruestung", "Weapon / equipment", "Waffe / Ausrüstung", "武器 / 装备", "武器 / 装備", "Arma / equipo", "Zbraň / výstroj"),
    ("LodVorAusruestungHint", "Weapons and equipment: LOD 2 keeps 0.55, LOD 3 0.30 (Jedi Academy weapons: 0.56 / 0.31).",
     "Waffen und Ausrüstung: LOD 2 behält 0,55, LOD 3 0,30 (Waffen von Jedi Academy: 0,56 / 0,31).",
     "武器和装备: LOD 2 保留 0.55, LOD 3 保留 0.30(Jedi Academy 武器: 0.56 / 0.31)。",
     "武器と装備: LOD 2 は 0.55、LOD 3 は 0.30 を残します(Jedi Academy の武器: 0.56 / 0.31)。",
     "Armas y equipo: LOD 2 conserva 0,55, LOD 3 0,30 (armas de Jedi Academy: 0,56 / 0,31).",
     "Zbraně a výstroj: LOD 2 ponechá 0,55, LOD 3 0,30 (zbraně Jedi Academy: 0,56 / 0,31)."),
    ("LodVorRequisit", "Prop", "Requisit", "道具", "小道具", "Accesorio", "Rekvizita"),
    ("LodVorRequisitHint", "Props (.md3): LOD 2 keeps 0.75, LOD 3 0.50 (Jedi Academy .md3 with name_1/_2: 0.75 / 0.49).",
     "Requisiten (.md3): LOD 2 behält 0,75, LOD 3 0,50 (.md3 von Jedi Academy mit name_1/_2: 0,75 / 0,49).",
     "道具(.md3): LOD 2 保留 0.75, LOD 3 保留 0.50(带 name_1/_2 的 Jedi Academy .md3: 0.75 / 0.49)。",
     "小道具(.md3): LOD 2 は 0.75、LOD 3 は 0.50 を残します(name_1/_2 を持つ Jedi Academy の .md3: 0.75 / 0.49)。",
     "Accesorios (.md3): LOD 2 conserva 0,75, LOD 3 0,50 (.md3 de Jedi Academy con name_1/_2: 0,75 / 0,49).",
     "Rekvizity (.md3): LOD 2 ponechá 0,75, LOD 3 0,50 (.md3 Jedi Academy s name_1/_2: 0,75 / 0,49)."),
    ("LodVorEigene", "Own", "Eigene", "自定义", "カスタム", "Propia", "Vlastní"),
    ("LodVorEigeneHint", "Your own shares - moving a slider chooses this.", "Eigene Anteile - ein Regler wählt das.", "自定义比例 - 拖动滑块即选择此项。",
     "自分の割合 - スライダーを動かすとこれになります。", "Proporciones propias: mover un control la elige.", "Vlastní podíly - posunutím jezdce se zvolí tato."),
    ("LodStufen", "Levels", "Stufen", "级别", "レベル", "Niveles", "Úrovně"),
    ("LodStufenHint", "Number of detail levels including LOD 1. A .md3 has at most 3 (MD3_MAX_LODS).",
     "Zahl der Detailstufen samt LOD 1. Eine .md3 hat höchstens 3 (MD3_MAX_LODS).", "包括 LOD 1 在内的细节级别数。.md3 最多 3 级(MD3_MAX_LODS)。",
     "LOD 1 を含む詳細レベルの数。.md3 は最大 3(MD3_MAX_LODS)。", "Número de niveles de detalle incluido LOD 1. Un .md3 tiene como máximo 3 (MD3_MAX_LODS).",
     "Počet úrovní detailu včetně LOD 1. .md3 má nejvýše 3 (MD3_MAX_LODS)."),
    ("LodStufen4Hint", "Four levels like some figures of Jedi Academy (0.74 / 0.43 / 0.28) - only for .glm.",
     "Vier Stufen wie manche Figuren von Jedi Academy (0,74 / 0,43 / 0,28) - nur für .glm.", "四个级别, 像 Jedi Academy 的部分人物(0.74 / 0.43 / 0.28)- 仅限 .glm。",
     "Jedi Academy の一部のフィギュアのように 4 レベル(0.74 / 0.43 / 0.28)- .glm のみ。",
     "Cuatro niveles como algunas figuras de Jedi Academy (0,74 / 0,43 / 0,28); solo para .glm.",
     "Čtyři úrovně jako některé postavy Jedi Academy (0,74 / 0,43 / 0,28) - jen pro .glm."),
    ("LodAnteil", "LOD %d", "LOD %d", "LOD %d", "LOD %d", "LOD %d", "LOD %d"),
    ("LodAnteilMaxHint", "Share of the vertices of LOD 1 that this level keeps (ProOptimizer: Vertex Percent). In brackets the target in vertices.",
     "Anteil der Ecken von LOD 1, den diese Stufe behält (ProOptimizer: Vertex Percent). In Klammern das Ziel in Ecken.",
     "此级别保留的 LOD 1 顶点比例(ProOptimizer: Vertex Percent)。括号中是目标顶点数。", "このレベルが残す LOD 1 の頂点の割合(ProOptimizer: Vertex Percent)。括弧内は目標頂点数。",
     "Proporción de vértices de LOD 1 que conserva este nivel (ProOptimizer: Vertex Percent). Entre paréntesis el objetivo en vértices.",
     "Podíl vrcholů LOD 1, který si tato úroveň ponechá (ProOptimizer: Vertex Percent). V závorce cíl ve vrcholech."),
    ("LodAnteilBlHint", "Share of the vertices of LOD 1 that this level keeps (Decimate: Ratio). In brackets the target in vertices.",
     "Anteil der Ecken von LOD 1, den diese Stufe behält (Decimate: Ratio). In Klammern das Ziel in Ecken.",
     "此级别保留的 LOD 1 顶点比例(Decimate: Ratio)。括号中是目标顶点数。", "このレベルが残す LOD 1 の頂点の割合(Decimate: Ratio)。括弧内は目標頂点数。",
     "Proporción de vértices de LOD 1 que conserva este nivel (Decimate: Ratio). Entre paréntesis el objetivo en vértices.",
     "Podíl vrcholů LOD 1, který si tato úroveň ponechá (Decimate: Ratio). V závorce cíl ve vrcholech."),
    ("LodRichtwert", "Clone Wars chain: lowest level at most %d vertices", "Clone-Wars-Kette: niedrigste Stufe höchstens %d Ecken",
     "Clone Wars 流程: 最低级别最多 %d 个顶点", "Clone Wars チェーン: 最低レベルは最大 %d 頂点", "Cadena Clone Wars: nivel más bajo como máximo %d vértices",
     "Řetězec Clone Wars: nejnižší úroveň nejvýše %d vrcholů"),
    ("LodRichtwertHint",
     "Only with the check profile Clone Wars chain: battles with 150-250 figures need cheap distant figures - the lowest level gets at most "
     "500 vertices drawn for a figure, 300 for weapons and equipment (as far as borders and seams allow).",
     "Nur mit dem Prüfprofil Clone-Wars-Kette: Schlachten mit 150-250 Figuren brauchen billige ferne Figuren - die niedrigste Stufe bekommt "
     "höchstens 500 gezeichnete Ecken je Figur, 300 für Waffen und Ausrüstung (soweit Ränder und Nähte es erlauben).",
     "仅在检查配置 Clone Wars 流程下: 150-250 个人物的战斗需要远处人物开销低 - 最低级别每个人物最多绘制 500 个顶点, 武器和装备 300 个(在边界和接缝允许的范围内)。",
     "チェックプロファイル Clone Wars チェーンのときだけ: 150-250 体の戦闘では遠くのフィギュアを軽くする必要があります - 最低レベルはフィギュア"
     "あたり最大 500、武器と装備は 300 頂点を描画します(境界と継ぎ目が許す範囲で)。",
     "Solo con el perfil de comprobación Cadena Clone Wars: las batallas con 150-250 figuras necesitan figuras lejanas baratas; el nivel "
     "más bajo dibuja como máximo 500 vértices por figura, 300 para armas y equipo (en la medida en que bordes y costuras lo permitan).",
     "Jen s profilem kontrol Řetězec Clone Wars: bitvy se 150-250 postavami potřebují levné vzdálené postavy - nejnižší úroveň vykreslí "
     "nejvýše 500 vrcholů na postavu, 300 pro zbraně a výstroj (pokud to hranice a švy dovolí)."),
    ("LodErweitert", "Advanced", "Erweitert", "高级", "詳細設定", "Avanzado", "Pokročilé"),
    ("LodGwNormale", "Keep normals", "Normalen bewahren", "保留法线", "法線を保持", "Conservar normales", "Zachovat normály"),
    ("LodGwNormaleHint", "How much a change of the normal costs against a change of the shape (0: only the shape counts).",
     "Wie viel eine Änderung der Normale gegen eine Änderung der Form kostet (0: nur die Form zählt).", "法线变化相对于形状变化的代价(0: 只计形状)。",
     "法線の変化が形の変化に対してどれだけ重いか(0: 形だけを数える)。", "Cuánto cuesta un cambio de la normal frente a un cambio de la forma (0: solo cuenta la forma).",
     "Kolik stojí změna normály oproti změně tvaru (0: počítá se jen tvar)."),
    ("LodGwUv", "Keep UVs", "UV bewahren", "保留 UV", "UV を保持", "Conservar UV", "Zachovat UV"),
    ("LodGwUvHint", "How much a stretched texture costs. UV seams stay in every case.", "Wie viel eine verzerrte Textur kostet. UV-Nähte bleiben in jedem Fall.",
     "纹理拉伸的代价。UV 接缝在任何情况下都保留。", "テクスチャの歪みのコスト。UV の継ぎ目はどの場合も残ります。",
     "Cuánto cuesta una textura deformada. Las costuras UV se conservan siempre.", "Kolik stojí zkreslená textura. UV švy zůstanou v každém případě."),
    ("LodGwKnochen", "Keep weights", "Gewichte bewahren", "保留权重", "ウェイトを保持", "Conservar pesos", "Zachovat váhy"),
    ("LodGwKnochenHint",
     "How much it costs to merge vertices that follow different bones (the vertices keep their weights from the file: at most 4, sum 1).",
     "Wie viel es kostet, Ecken zu verschmelzen, die anderen Knochen folgen (die Ecken behalten ihre Gewichte aus der Datei: höchstens 4, Summe 1).",
     "合并跟随不同骨骼的顶点的代价(顶点保留文件中的权重: 最多 4 个, 总和 1)。", "異なるボーンに従う頂点を統合するコスト(頂点はファイルのウェイトを保持: 最大 4、合計 1)。",
     "Cuánto cuesta fusionar vértices que siguen huesos distintos (los vértices conservan sus pesos del archivo: como máximo 4, suma 1).",
     "Kolik stojí sloučit vrcholy, které sledují jiné kosti (vrcholy si ponechají váhy ze souboru: nejvýše 4, součet 1)."),
    ("LodErzeugen", "Generate detail levels", "Detailstufen erzeugen", "生成细节级别", "詳細レベルを生成", "Generar niveles de detalle", "Vytvořit úrovně detailu"),
    ("LodErzeugenHint",
     "Replaces LOD 2 and further with new levels from LOD 1 (one undo step). The view then shows the lowest level; click a row to see "
     "another one.",
     "Ersetzt LOD 2 und weitere durch neue Stufen aus LOD 1 (ein Rückgängig-Schritt). Die Ansicht zeigt danach die niedrigste Stufe; ein Klick "
     "auf eine Zeile zeigt eine andere.",
     "用从 LOD 1 生成的新级别替换 LOD 2 及之后的级别(一步撤销)。之后视图显示最低级别; 点击一行可查看其他级别。",
     "LOD 2 以降を LOD 1 から作った新しいレベルで置き換えます(元に戻すは 1 回)。その後ビューは最低レベルを表示し、行をクリックすると別の"
     "レベルを表示します。",
     "Sustituye LOD 2 y siguientes por niveles nuevos a partir de LOD 1 (un paso de deshacer). Después la vista muestra el nivel más bajo; "
     "pulsa una fila para ver otro.",
     "Nahradí LOD 2 a další novými úrovněmi z LOD 1 (jeden krok zpět). Pohled pak ukáže nejnižší úroveň; kliknutím na řádek ukáže jinou."),
    ("LodEntfernen", "Remove detail levels", "Detailstufen entfernen", "移除细节级别", "詳細レベルを削除", "Quitar niveles de detalle", "Odebrat úrovně detailu"),
    ("LodEntfernenHint", "Keeps only LOD 1 (one undo step).", "Behält nur LOD 1 (ein Rückgängig-Schritt).", "只保留 LOD 1(一步撤销)。",
     "LOD 1 だけを残します(元に戻すは 1 回)。", "Conserva solo LOD 1 (un paso de deshacer).", "Ponechá jen LOD 1 (jeden krok zpět)."),
    ("LodSchritt", "Generate detail levels", "Detailstufen erzeugen", "生成细节级别", "詳細レベルを生成", "Generar niveles de detalle", "Vytvořit úrovně detailu"),
    ("LodEntfernenSchritt", "Remove detail levels", "Detailstufen entfernen", "移除细节级别", "詳細レベルを削除", "Quitar niveles de detalle", "Odebrat úrovně detailu"),
    ("LodErzeugt", "%d detail levels: %d -> %d vertices drawn, largest deviation %.2f units.",
     "%d Detailstufen: %d -> %d gezeichnete Ecken, größte Abweichung %.2f Einheiten.", "%d 个细节级别: 绘制顶点 %d -> %d, 最大偏差 %.2f 单位。",
     "詳細レベル %d: 描画頂点 %d -> %d、最大のずれ %.2f 単位。", "%d niveles de detalle: %d -> %d vértices dibujados, desviación máxima %.2f unidades.",
     "%d úrovní detailu: %d -> %d vykreslených vrcholů, největší odchylka %.2f jednotek."),
    ("LodVerfehlt", "%d targets not reached (borders and seams keep vertices).", "%d Ziele nicht erreicht (Ränder und Nähte halten Ecken).",
     "%d 个目标未达到(边界和接缝保留了顶点)。", "%d 個の目標に届きませんでした(境界と継ぎ目が頂点を保持)。", "%d objetivos no alcanzados (bordes y costuras retienen vértices).",
     "%d cílů nedosaženo (hranice a švy drží vrcholy)."),
    ("LodEntfernt", "Detail levels removed - only LOD 1.", "Detailstufen entfernt - nur LOD 1.", "已移除细节级别 - 只有 LOD 1。", "詳細レベルを削除しました - LOD 1 のみ。",
     "Niveles de detalle quitados: solo LOD 1.", "Úrovně detailu odebrány - jen LOD 1."),
    ("LodSpStufe", "Level", "Stufe", "级别", "レベル", "Nivel", "Úroveň"),
    ("LodSpEcken", "Vertices drawn", "Ecken gezeichnet", "绘制的顶点", "描画頂点", "Vértices dibujados", "Vykreslené vrcholy"),
    ("LodSpDreiecke", "Triangles drawn", "Dreiecke gezeichnet", "绘制的三角形", "描画三角形", "Triángulos dibujados", "Vykreslené trojúhelníky"),
    ("LodSpHint", "Counted as the game draws the model without a skin: without tags and surfaces switched off by default (_off, caps). "
     "Surfaces: all but tags.",
     "Gezählt, wie das Spiel das Modell ohne Haut zeichnet: ohne Tags und ab Werk abgeschaltete Flächen (_off, Kappen). Teilflächen: alle außer Tags.",
     "按游戏在没有皮肤时绘制模型的方式计数: 不含标签和默认关闭的表面(_off、端盖)。表面: 除标签外的全部。",
     "スキンなしでゲームがモデルを描画するとおりに数えます: タグと既定でオフの面(_off、キャップ)を除きます。サーフェス: タグ以外すべて。",
     "Contado como el juego dibuja el modelo sin skin: sin etiquetas ni superficies apagadas por defecto (_off, tapas). Superficies: todas menos etiquetas.",
     "Počítáno, jak hra kreslí model bez skinu: bez tagů a ploch ve výchozím stavu vypnutých (_off, krytky). Plochy: všechny kromě tagů."),
    ("LodSpFlaechen", "Surfaces", "Teilflächen", "表面", "サーフェス", "Superficies", "Plochy"),
    ("LodZeileHint", "Click: show this level in the view (like LOD 1-10 above).", "Klick: diese Stufe in der Ansicht zeigen (wie LOD 1-10 oben).",
     "点击: 在视图中显示此级别(同上方的 LOD 1-10)。", "クリック: このレベルをビューに表示(上の LOD 1-10 と同じ)。",
     "Clic: mostrar este nivel en la vista (como LOD 1-10 arriba).", "Klepnutí: ukázat tuto úroveň v pohledu (jako LOD 1-10 nahoře)."),
    ("LodGroesste", "Largest surface: %s (%d vertices)", "Größte Teilfläche: %s (%d Ecken)", "最大表面: %s(%d 个顶点)", "最大のサーフェス: %s(%d 頂点)",
     "Superficie más grande: %s (%d vértices)", "Největší plocha: %s (%d vrcholů)"),
    ("LodAbweichung", "Largest deviation from LOD 1: %.2f units.", "Größte Abweichung von LOD 1: %.2f Einheiten.", "与 LOD 1 的最大偏差: %.2f 单位。",
     "LOD 1 からの最大のずれ: %.2f 単位。", "Desviación máxima respecto a LOD 1: %.2f unidades.", "Největší odchylka od LOD 1: %.2f jednotek."),
    ("LodMd3Speichern", "Saving writes the levels as name_1.md3 and name_2.md3 next to the .md3.",
     "Speichern schreibt die Stufen als name_1.md3 und name_2.md3 neben die .md3.", "保存时将这些级别写成 .md3 旁边的 name_1.md3 和 name_2.md3。",
     "保存すると、レベルが .md3 の横に name_1.md3 と name_2.md3 として書かれます。", "Guardar escribe los niveles como name_1.md3 y name_2.md3 junto al .md3.",
     "Uložení zapíše úrovně jako name_1.md3 a name_2.md3 vedle .md3."),
    ("LodMd3Geschrieben", "Detail levels: %s.", "Detailstufen: %s.", "细节级别: %s。", "詳細レベル: %s。", "Niveles de detalle: %s.", "Úrovně detailu: %s."),
    ("LodMd3Fehler", "%s not written: %s", "%s nicht geschrieben: %s", "%s 未写入: %s", "%s を書けませんでした: %s", "%s no escrito: %s", "%s nezapsáno: %s"),
    ("LodMd3Alt", "(%s still lies next to it - the game loads it as a further level)", "(%s liegt noch daneben - das Spiel lädt sie als weitere Stufe)",
     "(%s 仍在旁边 - 游戏会把它作为另一级别加载)", "(%s がまだ横にあります - ゲームは追加のレベルとして読み込みます)",
     "(%s sigue al lado: el juego lo carga como otro nivel)", "(%s stále leží vedle - hra ho načte jako další úroveň)"),
    ("LodUeberschreibenFrage", "Saving writes the generated detail levels next to the .md3 and overwrites these files:",
     "Speichern schreibt die erzeugten Detailstufen neben die .md3 und überschreibt diese Dateien:", "保存时会把生成的细节级别写到 .md3 旁边, 并覆盖以下文件:",
     "保存すると生成した詳細レベルを .md3 の横に書き、次のファイルを上書きします:", "Guardar escribe los niveles de detalle generados junto al .md3 y sobrescribe estos archivos:",
     "Uložení zapíše vytvořené úrovně detailu vedle .md3 a přepíše tyto soubory:"),
    ("LodUeberschreibtZeile", "%s lies there already and is overwritten", "%s liegt schon da und wird überschrieben", "%s 已存在, 将被覆盖",
     "%s はすでにあり、上書きされます", "%s ya existe y se sobrescribe", "%s už existuje a bude přepsán"),
    ("LodZumWerkzeug", "Generate detail levels ...", "Detailstufen erzeugen ...", "生成细节级别 ...", "詳細レベルを生成 ...", "Generar niveles de detalle ...",
     "Vytvořit úrovně detailu ..."),
    ("LodZumWerkzeugHint", "Opens the group Detail levels in the area Modelling (tab Modify / Modifiers).",
     "Öffnet die Gruppe Detailstufen im Bereich Modellieren (Reiter Ändern / Modifier).", "在“建模”区域打开“细节级别”组(修改 / 修改器 选项卡)。",
     "モデリング領域の詳細レベルのグループを開きます(修正 / モディファイアー タブ)。", "Abre el grupo Niveles de detalle en el área Modelado (pestaña Modificar / Modificadores).",
     "Otevře skupinu Úrovně detailu v oblasti Modelování (karta Upravit / Modifikátory)."),
    # --- Befunde je Stufe ---
    ("LodBfEcken", "%s, LOD %d: %d vertices - %s draws at most %d per surface (ERR_DROP above).",
     "%s, LOD %d: %d Ecken - %s zeichnet höchstens %d je Teilfläche (darüber ERR_DROP).", "%s, LOD %d: %d 个顶点 - %s 每个表面最多绘制 %d 个(超过则 ERR_DROP)。",
     "%s、LOD %d: %d 頂点 - %s はサーフェスあたり最大 %d を描画します(超えると ERR_DROP)。",
     "%s, LOD %d: %d vértices; %s dibuja como máximo %d por superficie (por encima ERR_DROP).",
     "%s, LOD %d: %d vrcholů - %s vykreslí nejvýše %d na plochu (nad tím ERR_DROP)."),
    ("LodBfSchatten", "%s, LOD %d (lowest): %d vertices - the stencil shadow of a large surface uses this level, at most %d (SHADER_MAX_VERTEXES / 2).",
     "%s, LOD %d (niedrigste): %d Ecken - der Stencil-Schatten einer großen Teilfläche nimmt diese Stufe, höchstens %d (SHADER_MAX_VERTEXES / 2).",
     "%s, LOD %d(最低): %d 个顶点 - 大表面的模板阴影使用此级别, 最多 %d(SHADER_MAX_VERTEXES / 2)。",
     "%s、LOD %d(最低): %d 頂点 - 大きなサーフェスのステンシル影はこのレベルを使います、最大 %d(SHADER_MAX_VERTEXES / 2)。",
     "%s, LOD %d (más bajo): %d vértices; la sombra stencil de una superficie grande usa este nivel, como máximo %d (SHADER_MAX_VERTEXES / 2).",
     "%s, LOD %d (nejnižší): %d vrcholů - stencil stín velké plochy použije tuto úroveň, nejvýše %d (SHADER_MAX_VERTEXES / 2)."),
    ("LodBfMd3Ecken", "%s, LOD %d: %d vertices - more than MD3_MAX_VERTS %d.", "%s, LOD %d: %d Ecken - mehr als MD3_MAX_VERTS %d.",
     "%s, LOD %d: %d 个顶点 - 超过 MD3_MAX_VERTS %d。", "%s、LOD %d: %d 頂点 - MD3_MAX_VERTS %d を超えています。",
     "%s, LOD %d: %d vértices; más que MD3_MAX_VERTS %d.", "%s, LOD %d: %d vrcholů - více než MD3_MAX_VERTS %d."),
    ("LodBfMd3Flaechen", "LOD %d: %d surfaces - more than MD3_MAX_SURFACES %d.", "LOD %d: %d Teilflächen - mehr als MD3_MAX_SURFACES %d.",
     "LOD %d: %d 个表面 - 超过 MD3_MAX_SURFACES %d。", "LOD %d: %d サーフェス - MD3_MAX_SURFACES %d を超えています。",
     "LOD %d: %d superficies; más que MD3_MAX_SURFACES %d.", "LOD %d: %d ploch - více než MD3_MAX_SURFACES %d."),
    ("LodBfRichtwert", "LOD %d (lowest): %d vertices drawn - the Clone Wars chain wants at most %d.",
     "LOD %d (niedrigste): %d gezeichnete Ecken - die Clone-Wars-Kette will höchstens %d.", "LOD %d(最低): 绘制 %d 个顶点 - Clone Wars 流程要求最多 %d 个。",
     "LOD %d(最低): 描画頂点 %d - Clone Wars チェーンは最大 %d を求めます。", "LOD %d (más bajo): %d vértices dibujados; la Cadena Clone Wars quiere como máximo %d.",
     "LOD %d (nejnižší): %d vykreslených vrcholů - řetězec Clone Wars chce nejvýše %d."),
    ("LodBfKeineStufen", "Only one detail level - the Clone Wars chain wants three for figures, weapons and equipment.",
     "Nur eine Detailstufe - die Clone-Wars-Kette will drei für Figuren, Waffen und Ausrüstung.", "只有一个细节级别 - Clone Wars 流程要求人物、武器和装备有三个。",
     "詳細レベルが 1 つだけ - Clone Wars チェーンはフィギュア・武器・装備に 3 つを求めます。",
     "Solo un nivel de detalle; la Cadena Clone Wars quiere tres para figuras, armas y equipo.",
     "Jen jedna úroveň detailu - řetězec Clone Wars chce tři pro postavy, zbraně a výstroj."),
    # --- Optimierer (Pruefprofil Clone-Wars-Kette): Detailstufen der Laufzeit-Modelle ---
    ("KeOptPrModellStufen", "MD3 detail level missing", "MD3-Detailstufe fehlt", "缺少 MD3 细节级别", "MD3 の詳細レベルが欠けています",
     "Falta un nivel de detalle MD3", "Chybí úroveň detailu MD3"),
    ("KeOptPrModellStufenE",
     "Next to the .md3 lies a name_2.md3 but no name_1.md3. RE_RegisterModel loads the levels backwards (name_2, name_1, name) and stops "
     "with ERR_DROP \"has LOD 2 but is missing LOD 1\" (tr_model.cpp:650-653). Fix: save the model again in the model tab with its "
     "detail levels, or remove name_2.md3.",
     "Neben der .md3 liegt eine name_2.md3, aber keine name_1.md3. RE_RegisterModel lädt die Stufen rückwärts (name_2, name_1, name) und "
     "bricht mit ERR_DROP \"has LOD 2 but is missing LOD 1\" ab (tr_model.cpp:650-653). Abhilfe: das Modell im Model-Tab mit seinen "
     "Detailstufen neu speichern oder name_2.md3 entfernen.",
     ".md3 旁边有 name_2.md3 但没有 name_1.md3。RE_RegisterModel 倒序加载各级别(name_2、name_1、name), 并以 ERR_DROP \"has LOD 2 but is "
     "missing LOD 1\" 终止(tr_model.cpp:650-653)。解决: 在 Model 选项卡中连同细节级别重新保存模型, 或删除 name_2.md3。",
     ".md3 の横に name_2.md3 はあるのに name_1.md3 がありません。RE_RegisterModel はレベルを逆順に読み込み(name_2、name_1、name)、ERR_DROP "
     "\"has LOD 2 but is missing LOD 1\" で止まります(tr_model.cpp:650-653)。対処: Model タブで詳細レベルごと保存し直すか、name_2.md3 を削除します。",
     "Junto al .md3 hay un name_2.md3 pero ningún name_1.md3. RE_RegisterModel carga los niveles al revés (name_2, name_1, name) y se "
     "detiene con ERR_DROP \"has LOD 2 but is missing LOD 1\" (tr_model.cpp:650-653). Solución: guarda el modelo de nuevo en la pestaña "
     "Model con sus niveles de detalle, o quita name_2.md3.",
     "Vedle .md3 leží name_2.md3, ale žádný name_1.md3. RE_RegisterModel načítá úrovně pozpátku (name_2, name_1, name) a skončí s ERR_DROP "
     "\"has LOD 2 but is missing LOD 1\" (tr_model.cpp:650-653). Náprava: model znovu uložte na kartě Model i s úrovněmi detailu, nebo "
     "odstraňte name_2.md3."),
    ("KeOptPrModellDetail", "Large model without detail levels", "Großes Modell ohne Detailstufen", "大模型没有细节级别", "詳細レベルのない大きなモデル",
     "Modelo grande sin niveles de detalle", "Velký model bez úrovní detailu"),
    ("KeOptPrModellDetailE",
     "A model the game loads at run time (misc_model_static, misc_model_breakable) has more vertices than one surface of the target game "
     "carries, but no name_1.md3 / name_2.md3. Far away it is drawn in full. The requirements of the Clone Wars chain: large models should "
     "have detail levels. Fix: open it in the model tab, Generate detail levels, save.",
     "Ein Modell, das das Spiel zur Laufzeit lädt (misc_model_static, misc_model_breakable), hat mehr Ecken, als eine Fläche des Zielspiels "
     "trägt, aber keine name_1.md3 / name_2.md3. Auch in der Ferne wird es ganz gezeichnet. Die Anforderungen der Clone-Wars-Kette: große "
     "Modelle sollten Detailstufen haben. Abhilfe: im Model-Tab öffnen, Detailstufen erzeugen, speichern.",
     "游戏在运行时加载的模型(misc_model_static、misc_model_breakable)顶点数超过目标游戏一个表面所能承载的数量, 但没有 name_1.md3 / name_2.md3。"
     "在远处也会完整绘制。Clone Wars 流程的要求: 大模型应有细节级别。解决: 在 Model 选项卡中打开, 生成细节级别, 保存。",
     "ゲームが実行時に読み込むモデル(misc_model_static、misc_model_breakable)の頂点数がターゲットゲームの 1 面の上限より多いのに、name_1.md3 / "
     "name_2.md3 がありません。遠くでも全部描画されます。Clone Wars チェーンの要件: 大きなモデルは詳細レベルを持つべきです。対処: Model タブで"
     "開き、詳細レベルを生成して保存します。",
     "Un modelo que el juego carga en ejecución (misc_model_static, misc_model_breakable) tiene más vértices de los que lleva una superficie "
     "del juego, pero no tiene name_1.md3 / name_2.md3. También de lejos se dibuja entero. Los requisitos de la Cadena Clone Wars: los modelos "
     "grandes deberían tener niveles de detalle. Solución: ábrelo en la pestaña Model, genera los niveles de detalle y guarda.",
     "Model, který hra načítá za běhu (misc_model_static, misc_model_breakable), má víc vrcholů, než unese jedna plocha cílové hry, ale nemá "
     "name_1.md3 / name_2.md3. I v dálce se kreslí celý. Požadavky řetězce Clone Wars: velké modely by měly mít úrovně detailu. Náprava: "
     "otevřete ho na kartě Model, vytvořte úrovně detailu a uložte."),
    ("KeOptD55", "%s .md3 {file|files} with a _2.md3 but without _1.md3 (the game stops with ERR_DROP)",
     "%s {.md3-Datei|.md3-Dateien} mit _2.md3, aber ohne _1.md3 (das Spiel bricht mit ERR_DROP ab)",
     "%s 个 .md3 文件有 _2.md3 但没有 _1.md3(游戏以 ERR_DROP 终止)", "_2.md3 はあるが _1.md3 がない .md3 ファイル %s 個(ゲームは ERR_DROP で止まります)",
     "%s {archivo .md3|archivos .md3} con _2.md3 pero sin _1.md3 (el juego se detiene con ERR_DROP)",
     "%s {soubor .md3|soubory .md3|souborů .md3} s _2.md3, ale bez _1.md3 (hra skončí s ERR_DROP)"),
    ("KeOptD56", "%s run-time .md3 {model|models} with more than %s vertices and no detail levels (_1.md3, _2.md3)",
     "%s {Laufzeit-.md3|Laufzeit-.md3} mit mehr als %s Ecken und ohne Detailstufen (_1.md3, _2.md3)",
     "%s 个运行时 .md3 模型超过 %s 个顶点且没有细节级别(_1.md3、_2.md3)", "実行時 .md3 モデル %s 個: 頂点が %s を超え、詳細レベル(_1.md3、_2.md3)がありません",
     "%s {modelo .md3|modelos .md3} de ejecución con más de %s vértices y sin niveles de detalle (_1.md3, _2.md3)",
     "%s {běhový model .md3|běhové modely .md3|běhových modelů .md3} s více než %s vrcholy a bez úrovní detailu (_1.md3, _2.md3)"),
    # --- Bolts und eigenes Skelett (gui/app_modelltab_cwskelett.cpp) ---
    ('CwKeinKnochen', '(none - a root)', '(keiner - eine Wurzel)', '(无 - 根)', '(なし - ルート)', '(ninguno - una raíz)', '(žádná - kořen)'),
    ('CwBoltGruppeMax', 'Bolts (point helpers)', 'Bolts (Point-Helfer)', '挂点 (Point 辅助对象)', 'ボルト (Point ヘルパー)', 'Bolts (ayudantes Point)', 'Bolty (pomocníci Point)'),
    ('CwBoltGruppeBl', 'Bolts (empties on bones)', 'Bolts (Empties an Knochen)', '挂点 (骨骼上的空物体)', 'ボルト (ボーンのエンプティ)', 'Bolts (empties en huesos)', 'Bolty (prázdné objekty na kostech)'),
    ('CwBoltGruppeHint', 'Bolt surfaces (*name) where the game attaches something: seats for driver and gunners, muzzles, exhausts, weapons. Like a point helper linked to a bone in 3ds Max or an empty with a parent bone in Blender. Saved in the .glm as a surface with one triangle.', 'Bolt-Flächen (*name), an die das Spiel etwas hängt: Sitze für Fahrer und Schützen, Mündungen, Auspuffe, Waffen. Wie ein Point-Helfer, an einen Knochen gelinkt, in 3ds Max oder ein Empty mit Eltern-Knochen in Blender. In der .glm als Fläche mit einem Dreieck gespeichert.', '游戏挂接东西的挂点表面 (*name): 驾驶员和炮手的座位、枪口、排气口、武器。如同 3ds Max 中链接到骨骼的 Point 辅助对象, 或 Blender 中带父骨骼的空物体。在 .glm 中保存为带一个三角形的表面。', 'ゲームが何かを取り付けるボルト面 (*name): 運転手と砲手の座席、銃口、排気口、武器。3ds Max でボーンにリンクした Point ヘルパー、Blender で親ボーンを持つエンプティと同じです。.glm に三角形 1 つの面として保存されます。', 'Superficies bolt (*nombre) donde el juego engancha algo: asientos de conductor y artilleros, bocas de fuego, escapes, armas. Como un ayudante Point enlazado a un hueso en 3ds Max o un empty con hueso padre en Blender. Se guarda en el .glm como superficie con un triángulo.', 'Plochy boltů (*jméno), na které hra něco připojí: sedadla řidiče a střelců, ústí hlavní, výfuky, zbraně. Jako pomocník Point napojený na kost v 3ds Max nebo prázdný objekt s rodičovskou kostí v Blenderu. V .glm se uloží jako plocha s jedním trojúhelníkem.'),
    ('CwBoltVorlage', 'Name the game looks for', 'Name, den das Spiel sucht', '游戏查找的名称', 'ゲームが探す名前', 'Nombre que busca el juego', 'Jméno, které hra hledá'),
    ('CwBoltVorlageHint', 'The names the game code asks for with G2API_AddBolt (vehicles: *driver, *droidunit, *muzzle1.., *exhaust1..; emplaced guns: *seat, *flash01; weapons: *flash; figures). The tooltip of each name gives the file and line in OpenJK or Movie Duels.', 'Die Namen, nach denen der Spielcode mit G2API_AddBolt fragt (Fahrzeuge: *driver, *droidunit, *muzzle1.., *exhaust1..; Geschütze: *seat, *flash01; Waffen: *flash; Figuren). Der Tooltip jedes Namens nennt Datei und Zeile in OpenJK bzw. Movie Duels.', '游戏代码用 G2API_AddBolt 查询的名称(载具: *driver、*droidunit、*muzzle1..、*exhaust1..; 固定炮: *seat、*flash01; 武器: *flash; 人物)。每个名称的提示给出 OpenJK 或 Movie Duels 中的文件和行号。', 'ゲームコードが G2API_AddBolt で問い合わせる名前(乗り物: *driver、*droidunit、*muzzle1..、*exhaust1..; 据え置き砲: *seat、*flash01; 武器: *flash; フィギュア)。各名前のツールチップに OpenJK または Movie Duels のファイルと行があります。', 'Los nombres que el código del juego pide con G2API_AddBolt (vehículos: *driver, *droidunit, *muzzle1.., *exhaust1..; cañones fijos: *seat, *flash01; armas: *flash; figuras). El tooltip de cada nombre indica el archivo y la línea en OpenJK o Movie Duels.', 'Jména, na která se kód hry ptá pomocí G2API_AddBolt (vozidla: *driver, *droidunit, *muzzle1.., *exhaust1..; pevná děla: *seat, *flash01; zbraně: *flash; postavy). Tooltip každého jména uvádí soubor a řádek v OpenJK nebo Movie Duels.'),
    ('CwGruppeFahrzeug', 'Vehicle', 'Fahrzeug', '载具', '乗り物', 'Vehículo', 'Vozidlo'),
    ('CwGruppeGeschuetz', 'Emplaced gun', 'Geschütz', '固定炮', '据え置き砲', 'Cañón fijo', 'Pevné dělo'),
    ('CwGruppeWaffe', 'Weapon', 'Waffe', '武器', '武器', 'Arma', 'Zbraň'),
    ('CwGruppeFigur', 'Figure', 'Figur', '人物', 'フィギュア', 'Figura', 'Postava'),
    ('CwBoltName', 'Name', 'Name', '名称', '名前', 'Nombre', 'Jméno'),
    ('CwBoltNameHint', 'Starts with *, at most 63 characters, no other surface may have it (upper/lower case does not count, like the game).', 'Beginnt mit *, höchstens 63 Zeichen, keine andere Fläche darf ihn haben (Groß-/Kleinschreibung zählt nicht, wie im Spiel).', '以 * 开头, 最多 63 个字符, 不能与其他表面重名(与游戏一样不区分大小写)。', '* で始まり最大 63 文字、他の面と同じ名前は不可(ゲームと同じく大文字小文字は区別しません)。', 'Empieza con *, como máximo 63 caracteres, ninguna otra superficie puede tenerlo (mayúsculas y minúsculas dan igual, como en el juego).', 'Začíná *, nejvýše 63 znaků, žádná jiná plocha ho nesmí mít (velikost písmen nehraje roli, jako ve hře).'),
    ('CwBoltKnochenMax', 'Linked to', 'Gelinkt an', '链接到', 'リンク先', 'Enlazado a', 'Napojeno na'),
    ('CwBoltKnochenBl', 'Parent bone', 'Eltern-Knochen', '父骨骼', '親ボーン', 'Hueso padre', 'Rodičovská kost'),
    ('CwBoltKnochenHint', 'The bolt moves with this bone (all three corners weight 1).', 'Der Bolt geht mit diesem Knochen (alle drei Ecken Gewicht 1).', '挂点随此骨骼移动(三个角的权重都为 1)。', 'ボルトはこのボーンと一緒に動きます(3 つの頂点すべてウェイト 1)。', 'El bolt se mueve con este hueso (las tres esquinas con peso 1).', 'Bolt se pohybuje s touto kostí (všechny tři rohy mají váhu 1).'),
    ('CwBoltOrt', 'Position', 'Ort', '位置', '位置', 'Posición', 'Poloha'),
    ('CwBoltOrtHint', 'Origin of the bolt in model space (bind pose), in game units.', 'Ursprung des Bolts im Raum des Modells (Grundstellung), in Spieleinheiten.', '挂点在模型空间(绑定姿势)中的原点, 单位为游戏单位。', 'モデル空間(バインドポーズ)でのボルトの原点、ゲーム単位。', 'Origen del bolt en el espacio del modelo (pose base), en unidades del juego.', 'Počátek boltu v prostoru modelu (základní póza), v herních jednotkách.'),
    ('CwBoltWinkel', 'Rotation X Y Z', 'Drehung X Y Z', '旋转 X Y Z', '回転 X Y Z', 'Rotación X Y Z', 'Otočení X Y Z'),
    ('CwBoltWinkelHint', 'Euler XYZ in degrees like 3ds Max: what hangs on the bolt points along its X axis.', 'Euler XYZ in Grad wie 3ds Max: was am Bolt hängt, zeigt entlang seiner X-Achse.', '与 3ds Max 相同的欧拉 XYZ(度): 挂在挂点上的东西沿其 X 轴指向。', '3ds Max と同じオイラー XYZ(度): ボルトに付くものはその X 軸方向を向きます。', 'Euler XYZ en grados como 3ds Max: lo que cuelga del bolt apunta a lo largo de su eje X.', 'Euler XYZ ve stupních jako 3ds Max: co visí na boltu, míří podél jeho osy X.'),
    ('CwBoltAnlegen', 'Add bolt', 'Bolt anlegen', '添加挂点', 'ボルトを追加', 'Añadir bolt', 'Přidat bolt'),
    ('CwBoltAnlegenHint', 'Adds the bolt with name, bone, position and rotation above (one undo step).', 'Legt den Bolt mit Name, Knochen, Ort und Drehung von oben an (ein Rückgängig-Schritt).', '用上面的名称、骨骼、位置和旋转添加挂点(一步撤销)。', '上の名前・ボーン・位置・回転でボルトを追加します(元に戻すは 1 回)。', 'Añade el bolt con el nombre, hueso, posición y rotación de arriba (un paso de deshacer).', 'Přidá bolt se jménem, kostí, polohou a otočením výše (jeden krok zpět).'),
    ('CwBoltAnKnochen', 'To the bone', 'An den Knochen', '移到骨骼', 'ボーンへ', 'Al hueso', 'Ke kosti'),
    ('CwBoltAnKnochenHint', 'Puts the position on the joint of the chosen bone.', 'Setzt den Ort auf das Gelenk des gewählten Knochens.', '把位置放到所选骨骼的关节上。', '位置を選んだボーンの関節に置きます。', 'Pone la posición en la articulación del hueso elegido.', 'Nastaví polohu na kloub zvolené kosti.'),
    ('CwBoltUebernehmen', 'Apply', 'Übernehmen', '应用', '適用', 'Aplicar', 'Použít'),
    ('CwBoltUebernehmenHint', 'Gives the chosen bolt the name, bone, position and rotation above (one undo step).', 'Gibt dem gewählten Bolt Name, Knochen, Ort und Drehung von oben (ein Rückgängig-Schritt).', '把上面的名称、骨骼、位置和旋转赋给所选挂点(一步撤销)。', '選んだボルトに上の名前・ボーン・位置・回転を与えます(元に戻すは 1 回)。', 'Da al bolt elegido el nombre, hueso, posición y rotación de arriba (un paso de deshacer).', 'Dá zvolenému boltu jméno, kost, polohu a otočení výše (jeden krok zpět).'),
    ('CwBoltEntfernen', 'Remove bolt', 'Bolt entfernen', '移除挂点', 'ボルトを削除', 'Quitar bolt', 'Odebrat bolt'),
    ('CwBoltEntfernenHint', 'Removes the chosen bolt (one undo step).', 'Entfernt den gewählten Bolt (ein Rückgängig-Schritt).', '移除所选挂点(一步撤销)。', '選んだボルトを削除します(元に戻すは 1 回)。', 'Quita el bolt elegido (un paso de deshacer).', 'Odebere zvolený bolt (jeden krok zpět).'),
    ('CwBoltSchritt', 'Add bolt', 'Bolt anlegen', '添加挂点', 'ボルトを追加', 'Añadir bolt', 'Přidat bolt'),
    ('CwBoltAendernSchritt', 'Change bolt', 'Bolt ändern', '修改挂点', 'ボルトを変更', 'Cambiar bolt', 'Změnit bolt'),
    ('CwBoltEntfernenSchritt', 'Remove bolt', 'Bolt entfernen', '移除挂点', 'ボルトを削除', 'Quitar bolt', 'Odebrat bolt'),
    ('CwBoltNameBelegt', '%s: the name is taken or not a bolt name (* at the front, at most 63 characters).', '%s: der Name ist vergeben oder kein Bolt-Name (* vorn, höchstens 63 Zeichen).', '%s: 名称已被占用或不是挂点名称(开头为 *, 最多 63 个字符)。', '%s: 名前が使用済みか、ボルト名ではありません(先頭に *、最大 63 文字)。', '%s: el nombre está ocupado o no es un nombre de bolt (* delante, como máximo 63 caracteres).', '%s: jméno je obsazené nebo to není jméno boltu (* na začátku, nejvýše 63 znaků).'),
    ('CwBoltAngelegt', 'Bolt %s on bone %s.', 'Bolt %s am Knochen %s.', '挂点 %s 位于骨骼 %s。', 'ボルト %s をボーン %s に。', 'Bolt %s en el hueso %s.', 'Bolt %s na kosti %s.'),
    ('CwBoltKeinFahrer', 'Vehicle without *driver: G2API_AddBolt finds no seat, the driver sits at the origin of the model.', 'Fahrzeug ohne *driver: G2API_AddBolt findet keinen Sitz, der Fahrer sitzt im Ursprung des Modells.', '载具没有 *driver: G2API_AddBolt 找不到座位, 驾驶员坐在模型原点。', '*driver のない乗り物: G2API_AddBolt は座席を見つけられず、運転手はモデルの原点に座ります。', 'Vehículo sin *driver: G2API_AddBolt no encuentra asiento, el conductor se sienta en el origen del modelo.', 'Vozidlo bez *driver: G2API_AddBolt nenajde sedadlo, řidič sedí v počátku modelu.'),
    ('CwBoltKeineMuendung', 'Vehicle without muzzle (*muzzle1.. or *flash1..): its weapons fire from the origin of the model.', 'Fahrzeug ohne Mündung (*muzzle1.. oder *flash1..): seine Waffen schießen aus dem Ursprung des Modells.', '载具没有枪口(*muzzle1.. 或 *flash1..): 其武器从模型原点射击。', '銃口(*muzzle1.. または *flash1..)のない乗り物: 武器はモデルの原点から撃ちます。', 'Vehículo sin boca de fuego (*muzzle1.. o *flash1..): sus armas disparan desde el origen del modelo.', 'Vozidlo bez ústí (*muzzle1.. nebo *flash1..): jeho zbraně střílejí z počátku modelu.'),
    ('CwFahrzeug', 'Check as vehicle', 'Als Fahrzeug prüfen', '作为载具检查', '乗り物としてチェック', 'Comprobar como vehículo', 'Kontrolovat jako vozidlo'),
    ('CwFahrzeugHint', 'Warns when the seat of the driver or a muzzle is missing. With the check profile Clone Wars chain always on.', 'Warnt, wenn der Sitz des Fahrers oder eine Mündung fehlt. Mit dem Prüfprofil Clone-Wars-Kette immer an.', '缺少驾驶员座位或枪口时发出警告。使用检查配置 Clone Wars 流程时始终开启。', '運転手の座席や銃口がないと警告します。チェックプロファイル Clone Wars チェーンでは常にオン。', 'Avisa cuando falta el asiento del conductor o una boca de fuego. Con el perfil de comprobación Cadena Clone Wars siempre activo.', 'Varuje, když chybí sedadlo řidiče nebo ústí. S profilem kontrol Řetězec Clone Wars vždy zapnuto.'),
    ('CwSkelettGruppeMax', 'Bone tools (own skeleton)', 'Bone Tools (eigenes Skelett)', '骨骼工具(自定义骨架)', 'ボーンツール(独自スケルトン)', 'Herramientas de huesos (esqueleto propio)', 'Nástroje kostí (vlastní kostra)'),
    ('CwSkelettGruppeBl', 'Armature (own skeleton)', 'Armature (eigenes Skelett)', '骨架(自定义骨架)', 'アーマチュア(独自スケルトン)', 'Armadura (esqueleto propio)', 'Armatura (vlastní kostra)'),
    ('CwSkelettGruppeHint', 'An own skeleton for vehicles, walkers and droids: add bones, bind surfaces rigidly, add sequences, key them with the control rig, save .gla, .glm and animation.cfg - like Bone Tools / CAT in 3ds Max and an armature in Blender. A skeleton from a file (_humanoid) stays as it is.', 'Ein eigenes Skelett für Fahrzeuge, Läufer und Droiden: Knochen anlegen, Flächen starr binden, Sequenzen anlegen, mit dem Control Rig keyen, .gla, .glm und animation.cfg speichern - wie Bone Tools / CAT in 3ds Max und eine Armature in Blender. Ein Skelett aus einer Datei (_humanoid) bleibt, wie es ist.', '为载具、步行机和机器人建立自定义骨架: 添加骨骼、刚性绑定表面、添加序列、用控制绑定打关键帧, 保存 .gla、.glm 和 animation.cfg - 如同 3ds Max 的骨骼工具 / CAT 和 Blender 的骨架。来自文件的骨架(_humanoid)保持原样。', '乗り物・歩行機・ドロイド用の独自スケルトン: ボーンを追加し、面をリジッドに結び付け、シーケンスを追加し、コントロールリグでキーを打ち、.gla・.glm・animation.cfg を保存します - 3ds Max のボーンツール / CAT、Blender のアーマチュアと同じです。ファイルのスケルトン(_humanoid)はそのままです。', 'Un esqueleto propio para vehículos, caminantes y droides: añade huesos, une superficies de forma rígida, añade secuencias, ponles claves con el control rig y guarda .gla, .glm y animation.cfg, como Bone Tools / CAT en 3ds Max y una armadura en Blender. Un esqueleto de un archivo (_humanoid) queda como está.', 'Vlastní kostra pro vozidla, chodce a droidy: přidejte kosti, pevně navažte plochy, přidejte sekvence, klíčujte je control rigem a uložte .gla, .glm a animation.cfg - jako Bone Tools / CAT v 3ds Max a armatura v Blenderu. Kostra ze souboru (_humanoid) zůstane, jak je.'),
    ('CwSkelettAusDatei', 'Skeleton from the file: %s (%d bones) - it stays as it is.', 'Skelett aus der Datei: %s (%d Knochen) - es bleibt, wie es ist.', '来自文件的骨架: %s(%d 根骨骼)- 保持原样。', 'ファイルのスケルトン: %s(ボーン %d)- そのままです。', 'Esqueleto del archivo: %s (%d huesos); queda como está.', 'Kostra ze souboru: %s (%d kostí) - zůstane, jak je.'),
    ('CwSkelettName', 'Folder name', 'Ordnername', '文件夹名', 'フォルダー名', 'Nombre de carpeta', 'Název složky'),
    ('CwSkelettNameHint', 'The skeleton becomes models/players/<name>/<name>.gla - the name in its header must be the folder (G_ParseAnimFileSet).', 'Das Skelett wird models/players/<Name>/<Name>.gla - der Name im Kopf muss der Ordner sein (G_ParseAnimFileSet).', '骨架将成为 models/players/<名称>/<名称>.gla - 文件头中的名称必须是文件夹名(G_ParseAnimFileSet)。', 'スケルトンは models/players/<名前>/<名前>.gla になります - ヘッダーの名前はフォルダーでなければなりません(G_ParseAnimFileSet)。', 'El esqueleto será models/players/<nombre>/<nombre>.gla; el nombre de su cabecera debe ser la carpeta (G_ParseAnimFileSet).', 'Kostra bude models/players/<jméno>/<jméno>.gla - jméno v hlavičce musí být složka (G_ParseAnimFileSet).'),
    ('CwSkelettNeu', 'New skeleton', 'Neues Skelett', '新建骨架', '新しいスケルトン', 'Esqueleto nuevo', 'Nová kostra'),
    ('CwSkelettNeuHint', 'Creates an own skeleton with the bone model_root and puts every surface on it (one undo step).', 'Legt ein eigenes Skelett mit dem Knochen model_root an und hängt jede Fläche daran (ein Rückgängig-Schritt).', '创建带骨骼 model_root 的自定义骨架, 并把每个表面挂到它上面(一步撤销)。', 'ボーン model_root を持つ独自スケルトンを作り、すべての面をそれに付けます(元に戻すは 1 回)。', 'Crea un esqueleto propio con el hueso model_root y pone en él todas las superficies (un paso de deshacer).', 'Vytvoří vlastní kostru s kostí model_root a navěsí na ni každou plochu (jeden krok zpět).'),
    ('CwSkelettNeuSchritt', 'New skeleton', 'Neues Skelett', '新建骨架', '新しいスケルトン', 'Esqueleto nuevo', 'Nová kostra'),
    ('CwSkelettNameUngueltig', 'The folder name must not be empty and must not contain a slash, a dot or spaces.', 'Der Ordnername darf nicht leer sein und keinen Schrägstrich, Punkt oder Leerzeichen enthalten.', '文件夹名不能为空, 也不能包含斜杠、点或空格。', 'フォルダー名は空にできず、スラッシュ・ドット・空白を含められません。', 'El nombre de carpeta no puede estar vacío ni contener barras, puntos o espacios.', 'Název složky nesmí být prázdný a nesmí obsahovat lomítko, tečku ani mezery.'),
    ('CwSkelettAngelegt', 'Own skeleton %s: model_root, all surfaces on it.', 'Eigenes Skelett %s: model_root, alle Flächen daran.', '自定义骨架 %s: model_root, 所有表面挂在其上。', '独自スケルトン %s: model_root、すべての面がそれに付きました。', 'Esqueleto propio %s: model_root, todas las superficies en él.', 'Vlastní kostra %s: model_root, všechny plochy na ní.'),
    ('CwSkelettNurEigenes', 'Bones can only be changed on an own skeleton (New skeleton).', 'Knochen lassen sich nur an einem eigenen Skelett ändern (Neues Skelett).', '只能在自定义骨架上修改骨骼(新建骨架)。', 'ボーンは独自スケルトンでのみ変更できます(新しいスケルトン)。', 'Los huesos solo se cambian en un esqueleto propio (Esqueleto nuevo).', 'Kosti lze měnit jen na vlastní kostře (Nová kostra).'),
    ('CwSkelettEigen', 'Own skeleton %s: %d bones, %d frames.', 'Eigenes Skelett %s: %d Knochen, %d Bilder.', '自定义骨架 %s: %d 根骨骼, %d 帧。', '独自スケルトン %s: ボーン %d、フレーム %d。', 'Esqueleto propio %s: %d huesos, %d fotogramas.', 'Vlastní kostra %s: %d kostí, %d snímků.'),
    ('CwKnochenName', 'Bone name', 'Knochenname', '骨骼名称', 'ボーン名', 'Nombre del hueso', 'Jméno kosti'),
    ('CwKnochenNameHint', 'At most 63 characters, unique in the skeleton.', 'Höchstens 63 Zeichen, einmalig im Skelett.', '最多 63 个字符, 在骨架中唯一。', '最大 63 文字、スケルトン内で一意。', 'Como máximo 63 caracteres, único en el esqueleto.', 'Nejvýše 63 znaků, jedinečné v kostře.'),
    ('CwKnochenEltern', 'Parent', 'Eltern', '父级', '親', 'Padre', 'Rodič'),
    ('CwKnochenElternHint', 'The bone hangs under this one and moves with it.', 'Der Knochen hängt unter diesem und geht mit ihm.', '骨骼挂在它下面并随它移动。', 'ボーンはこの下に付き、一緒に動きます。', 'El hueso cuelga de este y se mueve con él.', 'Kost visí pod touto a pohybuje se s ní.'),
    ('CwKnochenOrt', 'Joint', 'Gelenk', '关节', '関節', 'Articulación', 'Kloub'),
    ('CwKnochenOrtHint', 'Position of the joint in model space (bind pose).', 'Ort des Gelenks im Raum des Modells (Grundstellung).', '关节在模型空间(绑定姿势)中的位置。', 'モデル空間(バインドポーズ)での関節の位置。', 'Posición de la articulación en el espacio del modelo (pose base).', 'Poloha kloubu v prostoru modelu (základní póza).'),
    ('CwKnochenAnlegenMax', 'Create bone', 'Bone erstellen', '创建骨骼', 'ボーンを作成', 'Crear hueso', 'Vytvořit kost'),
    ('CwKnochenAnlegenBl', 'Add bone', 'Knochen hinzufügen', '添加骨骼', 'ボーンを追加', 'Añadir hueso', 'Přidat kost'),
    ('CwKnochenAnlegenHint', 'Adds a bone with name, parent and joint above; in every frame it moves rigidly with its parent (one undo step).', 'Legt einen Knochen mit Name, Eltern und Gelenk von oben an; in jedem Bild geht er starr mit seinem Eltern (ein Rückgängig-Schritt).', '用上面的名称、父级和关节添加骨骼; 在每一帧中它都随父级刚性移动(一步撤销)。', '上の名前・親・関節でボーンを追加します。どのフレームでも親と一緒にリジッドに動きます(元に戻すは 1 回)。', 'Añade un hueso con el nombre, padre y articulación de arriba; en cada fotograma se mueve rígido con su padre (un paso de deshacer).', 'Přidá kost se jménem, rodičem a kloubem výše; v každém snímku se pohybuje pevně s rodičem (jeden krok zpět).'),
    ('CwKnochenUebernehmen', 'Apply to bone', 'Auf Knochen anwenden', '应用到骨骼', 'ボーンに適用', 'Aplicar al hueso', 'Použít na kost'),
    ('CwKnochenUebernehmenHint', 'Renames, re-parents and moves the chosen bone (one undo step).', 'Benennt den gewählten Knochen um, hängt ihn um und verschiebt ihn (ein Rückgängig-Schritt).', '重命名、改变父级并移动所选骨骼(一步撤销)。', '選んだボーンの名前変更・親の変更・移動を行います(元に戻すは 1 回)。', 'Renombra, cambia de padre y mueve el hueso elegido (un paso de deshacer).', 'Přejmenuje, přepojí a posune zvolenou kost (jeden krok zpět).'),
    ('CwKnochenEntfernen', 'Remove bone', 'Knochen entfernen', '移除骨骼', 'ボーンを削除', 'Quitar hueso', 'Odebrat kost'),
    ('CwKnochenEntfernenHint', 'Only a bone without children; its vertices go to its parent (one undo step).', 'Nur ein Knochen ohne Kinder; seine Ecken gehen an seinen Eltern (ein Rückgängig-Schritt).', '只能移除没有子级的骨骼; 其顶点归父级(一步撤销)。', '子のないボーンのみ。その頂点は親へ移ります(元に戻すは 1 回)。', 'Solo un hueso sin hijos; sus vértices pasan a su padre (un paso de deshacer).', 'Jen kost bez potomků; její vrcholy přejdou na rodiče (jeden krok zpět).'),
    ('CwFlaechenMax', 'Skin: selection to bone', 'Skin: Auswahl an Knochen', '蒙皮: 选择项到骨骼', 'スキン: 選択をボーンへ', 'Skin: selección al hueso', 'Skin: výběr na kost'),
    ('CwFlaechenBl', 'Assign selection to bone', 'Auswahl dem Knochen zuweisen', '将选择项指定给骨骼', '選択をボーンに割り当て', 'Asignar selección al hueso', 'Přiřadit výběr kosti'),
    ('CwFlaechenHint', 'The selected surfaces follow the chosen bone rigidly (every vertex weight 1, all detail levels) - as a vehicle part does.', 'Die gewählten Flächen folgen dem gewählten Knochen starr (jede Ecke Gewicht 1, alle Detailstufen) - wie ein Teil eines Fahrzeugs.', '所选表面刚性跟随所选骨骼(每个顶点权重 1, 所有细节级别)- 就像载具的部件。', '選んだ面は選んだボーンにリジッドに従います(各頂点ウェイト 1、全詳細レベル)- 乗り物の部品のように。', 'Las superficies elegidas siguen al hueso elegido de forma rígida (cada vértice peso 1, todos los niveles de detalle), como una pieza de vehículo.', 'Zvolené plochy pevně sledují zvolenou kost (každý vrchol váha 1, všechny úrovně detailu) - jako díl vozidla.'),
    ('CwFlaechenKeine', 'Select the surfaces first (scene explorer or the view).', 'Zuerst die Flächen wählen (Szenen-Explorer oder Ansicht).', '先选择表面(场景资源管理器或视图)。', '先に面を選んでください(シーンエクスプローラーまたはビュー)。', 'Primero elige las superficies (explorador de escena o vista).', 'Nejdřív vyberte plochy (průzkumník scény nebo pohled).'),
    ('CwFlaechenSchritt', 'Surfaces to bone', 'Flächen an Knochen', '表面到骨骼', '面をボーンへ', 'Superficies al hueso', 'Plochy na kost'),
    ('CwFlaechenGebunden', '%d surfaces rigidly on %s.', '%d Flächen starr an %s.', '%d 个表面刚性绑定到 %s。', '%d 面を %s にリジッドに。', '%d superficies rígidas en %s.', '%d ploch pevně na %s.'),
    ('CwKnochenSchritt', 'Add bone', 'Knochen anlegen', '添加骨骼', 'ボーンを追加', 'Añadir hueso', 'Přidat kost'),
    ('CwKnochenAendernSchritt', 'Change bone', 'Knochen ändern', '修改骨骼', 'ボーンを変更', 'Cambiar hueso', 'Změnit kost'),
    ('CwKnochenEntfernenSchritt', 'Remove bone', 'Knochen entfernen', '移除骨骼', 'ボーンを削除', 'Quitar hueso', 'Odebrat kost'),
    ('CwKnochenNameBelegt', '%s: the name is taken or invalid.', '%s: der Name ist vergeben oder ungültig.', '%s: 名称已被占用或无效。', '%s: 名前が使用済みか無効です。', '%s: el nombre está ocupado o no es válido.', '%s: jméno je obsazené nebo neplatné.'),
    ('CwKnochenAngelegt', 'Bone %s added.', 'Knochen %s angelegt.', '已添加骨骼 %s。', 'ボーン %s を追加しました。', 'Hueso %s añadido.', 'Kost %s přidána.'),
    ('CwSequenz', 'Sequence', 'Sequenz', '序列', 'シーケンス', 'Secuencia', 'Sekvence'),
    ('CwSequenzHint', 'Name of the line in the animation.cfg - only names from the animation table of the game count.', 'Name der Zeile in der animation.cfg - es zählen nur Namen aus der Animationstabelle des Spiels.', 'animation.cfg 中该行的名称 - 只有游戏动画表中的名称有效。', 'animation.cfg の行の名前 - ゲームのアニメーション表にある名前だけが有効です。', 'Nombre de la línea en el animation.cfg; solo cuentan los nombres de la tabla de animaciones del juego.', 'Jméno řádku v animation.cfg - platí jen jména z tabulky animací hry.'),
    ('CwSequenzUnbekannt', '%s is not in the animation table of %s - the game skips this line of the animation.cfg.', '%s steht nicht in der Animationstabelle von %s - das Spiel übergeht diese Zeile der animation.cfg.', '%s 不在 %s 的动画表中 - 游戏会跳过 animation.cfg 的这一行。', '%s は %s のアニメーション表にありません - ゲームは animation.cfg のこの行を飛ばします。', '%s no está en la tabla de animaciones de %s; el juego salta esta línea del animation.cfg.', '%s není v tabulce animací %s - hra tento řádek animation.cfg přeskočí.'),
    ('CwSequenzBilder', 'Frames', 'Bilder', '帧数', 'フレーム', 'Fotogramas', 'Snímky'),
    ('CwSequenzBilderHint', 'So many frames are added to the skeleton (in the bind pose, ready for keying).', 'So viele Bilder kommen an das Skelett (in der Grundstellung, bereit zum Keyen).', '向骨架添加这么多帧(处于绑定姿势, 可打关键帧)。', 'これだけのフレームをスケルトンに追加します(バインドポーズ、キーを打てる状態)。', 'Se añaden tantos fotogramas al esqueleto (en la pose base, listos para claves).', 'Tolik snímků se přidá ke kostře (v základní póze, připravené ke klíčování).'),
    ('CwSequenzFps', 'Frames per second', 'Bilder je Sekunde', '每秒帧数', '毎秒フレーム', 'Fotogramas por segundo', 'Snímků za sekundu'),
    ('CwSequenzFpsHint', 'The last number of the line (frameLerp = 1000 / fps).', 'Die letzte Zahl der Zeile (frameLerp = 1000 / fps).', '该行的最后一个数字(frameLerp = 1000 / fps)。', '行の最後の数字(frameLerp = 1000 / fps)。', 'El último número de la línea (frameLerp = 1000 / fps).', 'Poslední číslo řádku (frameLerp = 1000 / fps).'),
    ('CwSequenzSchleife', 'Loop', 'Schleife', '循环', 'ループ', 'Bucle', 'Smyčka'),
    ('CwSequenzSchleifeHint', 'loopFrames 0 (repeats from the first frame) instead of -1 (holds the last frame).', 'loopFrames 0 (wiederholt ab dem ersten Bild) statt -1 (hält das letzte Bild).', 'loopFrames 0(从第一帧重复)而不是 -1(停在最后一帧)。', 'loopFrames 0(最初のフレームから繰り返す)、-1(最後のフレームで止まる)ではなく。', 'loopFrames 0 (repite desde el primer fotograma) en lugar de -1 (mantiene el último).', 'loopFrames 0 (opakuje od prvního snímku) místo -1 (drží poslední snímek).'),
    ('CwSequenzAnlegen', 'Add sequence', 'Sequenz anlegen', '添加序列', 'シーケンスを追加', 'Añadir secuencia', 'Přidat sekvenci'),
    ('CwSequenzAnlegenHint', 'Adds the frames to the skeleton and the line to the animation.cfg (one undo step); then pose and key them in the area Animation.', 'Hängt die Bilder an das Skelett und die Zeile an die animation.cfg (ein Rückgängig-Schritt); danach im Bereich Animation stellen und keyen.', '把帧加到骨架、把该行加到 animation.cfg(一步撤销); 然后在“动画”区域摆姿势并打关键帧。', 'フレームをスケルトンに、行を animation.cfg に追加します(元に戻すは 1 回)。その後アニメーション領域でポーズを付けてキーを打ちます。', 'Añade los fotogramas al esqueleto y la línea al animation.cfg (un paso de deshacer); después posa y pon claves en el área Animación.', 'Přidá snímky ke kostře a řádek do animation.cfg (jeden krok zpět); pak je v oblasti Animace napózujte a klíčujte.'),
    ('CwSequenzSchritt', 'Add sequence', 'Sequenz anlegen', '添加序列', 'シーケンスを追加', 'Añadir secuencia', 'Přidat sekvenci'),
    ('CwSequenzBelegt', '%s: the sequence exists already.', '%s: die Sequenz gibt es schon.', '%s: 该序列已存在。', '%s: このシーケンスはすでにあります。', '%s: la secuencia ya existe.', '%s: sekvence už existuje.'),
    ('CwSequenzAngelegt', 'Sequence %s: from frame %d, %d frames at %d fps - now pose and key it.', 'Sequenz %s: ab Bild %d, %d Bilder mit %d fps - jetzt stellen und keyen.', '序列 %s: 从第 %d 帧起, 共 %d 帧, %d fps - 现在摆姿势并打关键帧。', 'シーケンス %s: フレーム %d から %d フレーム、%d fps - ポーズを付けてキーを打ってください。', 'Secuencia %s: desde el fotograma %d, %d fotogramas a %d fps; ahora posa y pon claves.', 'Sekvence %s: od snímku %d, %d snímků při %d fps - teď napózujte a klíčujte.'),
    ('CwSkelettTipp', 'Order: New skeleton -> bones -> surfaces to bones -> sequences -> pose and key in the control rig -> save GLA (Animation > Save GLA), GLM (Ctrl+S) and animation.cfg. An own skeleton loads only in single player.', 'Reihenfolge: Neues Skelett -> Knochen -> Flächen an Knochen -> Sequenzen -> im Control Rig stellen und keyen -> GLA speichern (Animation > GLA speichern), GLM (Strg+S) und animation.cfg. Ein eigenes Skelett lädt nur der Einzelspieler.', '顺序: 新建骨架 -> 骨骼 -> 表面到骨骼 -> 序列 -> 在控制绑定中摆姿势并打关键帧 -> 保存 GLA(动画 > 保存 GLA)、GLM(Ctrl+S)和 animation.cfg。自定义骨架只在单人模式中加载。', '順序: 新しいスケルトン -> ボーン -> 面をボーンへ -> シーケンス -> コントロールリグでポーズとキー -> GLA(アニメーション > GLA を保存)、GLM(Ctrl+S)、animation.cfg を保存。独自スケルトンはシングルプレイでのみ読み込まれます。', 'Orden: Esqueleto nuevo -> huesos -> superficies a huesos -> secuencias -> posar y poner claves en el control rig -> guardar GLA (Animación > Guardar GLA), GLM (Ctrl+S) y animation.cfg. Un esqueleto propio solo se carga en un jugador.', 'Pořadí: Nová kostra -> kosti -> plochy na kosti -> sekvence -> v control rigu napózovat a klíčovat -> uložit GLA (Animace > Uložit GLA), GLM (Ctrl+S) a animation.cfg. Vlastní kostru načte jen hra jednoho hráče.'),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
