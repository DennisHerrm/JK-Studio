# -*- coding: utf-8 -*-
"""Oberflaechentexte von AP5 (MBENTWURF Teil B): der Bereich Ansehen/Pruefen des Model-Tabs vollstaendig nach dem freigegebenen
Entwurf - Gruppen Modell (Importieren), Detailstufe und Skin (LOD 1-10 wie im Spiel, Skins zweispaltig), Boden und Kamera, Auswahl,
Pruefung fuer das Spiel (Balken, gesuchte Tags, Flaechen, LODs), Szene (.mvs wie ModView), die fuenf fehlenden Anzeige-Schalter von
ModView, der Hinweis "LOD - Dreiecke" in der Ansicht, vier Ansichten wie 3ds Max, Tasten wie Blender (Z, Umschalt+Alt+Z) - fuer
tools/gen_i18n.py und die weiteren Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    # --- Gruppe Modell (P1) ---
    ("PqImport", "Import...", "Importieren...", "导入……", "インポート...", "Importar...", "Importovat..."),
    ("PqImportHint", "Open a model: .glm or .md3 from the disk, or any model of the game from its .pk3 archives. "
     "XSI, FBX, OBJ and glTF models cannot be read yet (the import of these formats is for animations).",
     "Ein Modell öffnen: .glm oder .md3 von der Platte oder jedes Modell des Spiels aus seinen .pk3-Archiven. "
     "Modelle aus XSI, FBX, OBJ und glTF lassen sich noch nicht lesen (der Import dieser Formate gilt Animationen).",
     "打开模型: 从磁盘打开 .glm 或 .md3, 或从游戏的 .pk3 存档打开任意模型。暂时还不能读取 XSI、FBX、OBJ 和 glTF 模型 (这些格式的导入用于动画)。",
     "モデルを開きます: ディスクの .glm / .md3、またはゲームの .pk3 アーカイブにある任意のモデル。XSI、FBX、OBJ、glTF のモデルはまだ読めません (これらの形式のインポートはアニメーション用です)。",
     "Abrir un modelo: .glm o .md3 del disco, o cualquier modelo del juego desde sus archivos .pk3. "
     "Los modelos XSI, FBX, OBJ y glTF aún no se pueden leer (la importación de esos formatos es para animaciones).",
     "Otevřít model: .glm nebo .md3 z disku, nebo libovolný model hry z jejích archivů .pk3. "
     "Modely XSI, FBX, OBJ a glTF zatím načíst nelze (import těchto formátů slouží animacím)."),
    ("PqImportDatei", "From the disk (.glm, .md3) ...", "Von der Platte (.glm, .md3) ...", "从磁盘 (.glm, .md3)……", "ディスクから (.glm, .md3)...",
     "Desde el disco (.glm, .md3)...", "Z disku (.glm, .md3)..."),
    ("PqImportPk3", "From the game (.pk3) ...", "Aus dem Spiel (.pk3) ...", "从游戏 (.pk3)……", "ゲームから (.pk3)...", "Desde el juego (.pk3)...", "Ze hry (.pk3)..."),
    # --- Gruppe Detailstufe und Skin (P2, Z20) ---
    ("PqLodZahl", "The model has %d {LOD|LODs}. %d {2:triangle|triangles}.", "Das Modell hat %d {LOD|LODs}. %d {2:Dreieck|Dreiecke}.",
     "该模型有 %d 个 LOD。%d 个三角形。", "モデルの LOD は %d 個です。三角形 %d。", "El modelo tiene %d {LOD|LOD}. %d {2:triángulo|triángulos}.",
     "Model má %d {LOD|LOD|LOD}. %d {2:trojúhelník|trojúhelníky|trojúhelníků}."),
    ("PqLodWieSpiel", "LOD %d shows LOD %d, as in the game.", "LOD %d zeigt LOD %d, wie im Spiel.", "LOD %d 显示 LOD %d, 与游戏中相同。",
     "LOD %d は LOD %d を表示します (ゲームと同じ)。", "LOD %d muestra LOD %d, como en el juego.", "LOD %d ukazuje LOD %d, jako ve hře."),
    ("PqLodStufeHint", "Level of detail like r_lodbias in the game: a level above the last one of the model shows the last one (G2_ComputeLOD).",
     "Detailstufe wie r_lodbias im Spiel: eine Stufe über der letzten des Modells zeigt die letzte (G2_ComputeLOD).",
     "细节级别与游戏中的 r_lodbias 相同: 高于模型最后一级的级别显示最后一级 (G2_ComputeLOD)。",
     "ゲームの r_lodbias と同じ詳細レベル: モデルの最後のレベルより上を選ぶと最後のレベルを表示します (G2_ComputeLOD)。",
     "Nivel de detalle como r_lodbias en el juego: un nivel por encima del último del modelo muestra el último (G2_ComputeLOD).",
     "Úroveň detailu jako r_lodbias ve hře: úroveň nad poslední úrovní modelu ukazuje tu poslední (G2_ComputeLOD)."),
    ("PqFlaechenliste", "Surface list to the detail log", "Flächenliste ins Detailprotokoll", "面列表写入详细日志", "サーフェス一覧を詳細ログへ",
     "Lista de superficies al registro detallado", "Seznam ploch do podrobného protokolu"),
    # --- Anzeige-Schalter von ModView (P3a) ---
    ("PqAlpha", "Alpha blending", "Alpha-Blending", "Alpha 混合", "アルファブレンド", "Mezcla alfa", "Alfa prolínání"),
    ("PqAlphaHint", "On: transparent parts blend as the shader of the game says (as in the game). Off: everything is drawn opaque, to see the shape "
     "of hair, glass and lenses (ModView: Alpha).",
     "An: durchsichtige Teile mischen, wie der Shader des Spiels es sagt (wie im Spiel). Aus: alles deckend, um die Form von Haaren, Glas und "
     "Linsen zu sehen (ModView: Alpha).",
     "开: 透明部分按游戏着色器的规定混合 (与游戏中相同)。关: 全部不透明绘制, 以便看清头发、玻璃和镜片的形状 (ModView: Alpha)。",
     "オン: 透明部分をゲームのシェーダーどおりにブレンドします (ゲームと同じ)。オフ: すべて不透明で描画し、髪・ガラス・レンズの形を確認できます (ModView: Alpha)。",
     "Activado: las partes transparentes se mezclan como dice el shader del juego (como en el juego). Desactivado: todo opaco, para ver la forma "
     "del pelo, el cristal y las lentes (ModView: Alpha).",
     "Zapnuto: průhledné části se prolínají podle shaderu hry (jako ve hře). Vypnuto: vše neprůhledně, aby byl vidět tvar vlasů, skla a čoček "
     "(ModView: Alpha)."),
    ("PqFilter", "Texture filter", "Texturfilter", "纹理过滤", "テクスチャフィルター", "Filtro de texturas", "Filtrování textur"),
    ("PqFilterHint", "On: smooth textures as in the game (r_ext_texture_filter_anisotropic 16). Off: the nearest texel, to see every pixel of the "
     "image (ModView: Bilinear filtering).",
     "An: glatte Texturen wie im Spiel (r_ext_texture_filter_anisotropic 16). Aus: der nächste Bildpunkt, um jeden Pixel des Bilds zu sehen "
     "(ModView: Bilinear filtering).",
     "开: 与游戏中相同的平滑纹理 (r_ext_texture_filter_anisotropic 16)。关: 取最近的纹素, 可看清图像的每个像素 (ModView: Bilinear filtering)。",
     "オン: ゲームと同じ滑らかなテクスチャ (r_ext_texture_filter_anisotropic 16)。オフ: 最も近いテクセルを使い、画像のピクセルを1つずつ確認できます (ModView: Bilinear filtering)。",
     "Activado: texturas suaves como en el juego (r_ext_texture_filter_anisotropic 16). Desactivado: el texel más cercano, para ver cada píxel de la "
     "imagen (ModView: Bilinear filtering).",
     "Zapnuto: hladké textury jako ve hře (r_ext_texture_filter_anisotropic 16). Vypnuto: nejbližší texel, aby byl vidět každý pixel obrázku "
     "(ModView: Bilinear filtering)."),
    ("PqSchwelle", "Bone weight threshold", "Knochengewicht-Schwelle", "骨骼权重阈值", "ボーンウェイトのしきい値", "Umbral de peso de hueso", "Práh váhy kosti"),
    ("PqSchwelleHint", "When the figure is posed, weights at or below the threshold are left out without giving their share to the others (ModView: "
     "Bone weight thresholding) - vertices that move away depend on small weights. Only the view changes, not the file.",
     "Beim Posieren fallen Gewichte bis zur Schwelle weg, ohne ihren Anteil an die anderen zu geben (ModView: Bone weight thresholding) - Ecken, "
     "die sich wegbewegen, hängen an kleinen Gewichten. Nur die Ansicht ändert sich, nicht die Datei.",
     "摆姿势时, 不超过阈值的权重被略去, 且不把其份额分给其他权重 (ModView: Bone weight thresholding) - 移开的顶点依赖于小权重。只改变视图, 不改变文件。",
     "ポーズ時にしきい値以下のウェイトを省き、その分を他に回しません (ModView: Bone weight thresholding)。動いてしまう頂点は小さなウェイトに依存しています。変わるのは表示だけで、ファイルは変わりません。",
     "Al posar, los pesos hasta el umbral se omiten sin repartir su parte a los demás (ModView: Bone weight thresholding): los vértices que se "
     "desplazan dependen de pesos pequeños. Solo cambia la vista, no el archivo.",
     "Při pózování se váhy do prahu vynechají, aniž by jejich podíl dostaly ostatní (ModView: Bone weight thresholding) - vrcholy, které se "
     "posunou, závisí na malých vahách. Mění se jen pohled, ne soubor."),
    ("PqSchwelleWert", "Weight threshold", "Gewicht-Schwelle", "权重阈值", "ウェイトしきい値", "Umbral de peso", "Práh váhy"),
    ("PqHervor", "Highlight surface (+ bone refs)", "Fläche hervorheben (+ Knochenbezüge)", "高亮面 (+ 骨骼引用)", "サーフェスを強調 (+ ボーン参照)",
     "Resaltar superficie (+ refs. de huesos)", "Zvýraznit plochu (+ odkazy kostí)"),
    ("PqHervorHint", "The selected surface is outlined in yellow, and every bone it references shows its origin, name and number "
     "(ModView: Surface highlight, shows bone weighting).",
     "Die gewählte Fläche ist gelb umrandet, und jeder Knochen, auf den sie sich bezieht, zeigt Ursprung, Name und Nummer "
     "(ModView: Surface highlight, shows bone weighting).",
     "所选面以黄色描边, 它引用的每块骨骼都显示原点、名称和编号 (ModView: Surface highlight, shows bone weighting)。",
     "選択したサーフェスを黄色で囲み、参照している各ボーンの原点・名前・番号を表示します (ModView: Surface highlight, shows bone weighting)。",
     "La superficie elegida se rodea de amarillo y cada hueso al que hace referencia muestra su origen, nombre y número "
     "(ModView: Surface highlight, shows bone weighting).",
     "Vybraná plocha je žlutě orámována a každá kost, na kterou odkazuje, ukazuje počátek, název a číslo "
     "(ModView: Surface highlight, shows bone weighting)."),
    ("PqSchatten", "Non-shadow-casting surfaces", "Nicht schattenwerfende Flächen", "不投射阴影的面", "影を落とさないサーフェス",
     "Superficies sin sombra propia", "Plochy bez vlastního stínu"),
    ("PqSchattenHint", "Red dashes on every surface with %d vertices or more (half of SHADER_MAX_VERTEXES of %s): the stencil shadow of the game "
     "(r_shadows 2) does not come from it but from the same surface of the last LOD (tr_ghoul2.cpp).",
     "Rote Striche auf jeder Fläche mit %d Ecken oder mehr (die Hälfte von SHADER_MAX_VERTEXES in %s): der Stencil-Schatten des Spiels "
     "(r_shadows 2) kommt nicht von ihr, sondern von derselben Fläche der letzten LOD (tr_ghoul2.cpp).",
     "在顶点数达到 %d 或更多 (%s 中 SHADER_MAX_VERTEXES 的一半) 的每个面上显示红色虚线: 游戏的模板阴影 (r_shadows 2) 不来自该面, 而来自最后一级 LOD 的同一面 (tr_ghoul2.cpp)。",
     "頂点が %d 以上 (%s の SHADER_MAX_VERTEXES の半分) のサーフェスに赤い破線を表示します。ゲームのステンシル影 (r_shadows 2) はそのサーフェスからではなく、最後の LOD の同じサーフェスから作られます (tr_ghoul2.cpp)。",
     "Trazos rojos en cada superficie con %d vértices o más (la mitad de SHADER_MAX_VERTEXES de %s): la sombra de stencil del juego "
     "(r_shadows 2) no sale de ella sino de la misma superficie del último LOD (tr_ghoul2.cpp).",
     "Červené čárky na každé ploše s %d vrcholy nebo více (polovina SHADER_MAX_VERTEXES v %s): stencilový stín hry (r_shadows 2) nevzniká z ní, "
     "ale ze stejné plochy poslední LOD (tr_ghoul2.cpp)."),
    ("PqSchattenMenueHint", "Red dashes on every surface with half of SHADER_MAX_VERTEXES of the target game or more vertices (JKA and Jedi Outcast 500, "
     "Movie Duels 2000): its stencil shadow (r_shadows 2) comes from the same surface of the last LOD (ModView: Unshadowable surfaces).",
     "Rote Striche auf jeder Fläche mit der Hälfte von SHADER_MAX_VERTEXES des Zielspiels oder mehr Ecken (JKA und Jedi Outcast 500, "
     "Movie Duels 2000): ihr Stencil-Schatten (r_shadows 2) kommt von derselben Fläche der letzten LOD (ModView: Unshadowable surfaces).",
     "在顶点数达到目标游戏 SHADER_MAX_VERTEXES 一半或更多的每个面上显示红色虚线 (JKA 和 Jedi Outcast 为 500, Movie Duels 为 2000): 其模板阴影 (r_shadows 2) 来自最后一级 LOD 的同一面 (ModView: Unshadowable surfaces)。",
     "頂点数が対象ゲームの SHADER_MAX_VERTEXES の半分以上 (JKA と Jedi Outcast は 500、Movie Duels は 2000) のサーフェスに赤い破線を表示します。そのステンシル影 (r_shadows 2) は最後の LOD の同じサーフェスから作られます (ModView: Unshadowable surfaces)。",
     "Trazos rojos en cada superficie con la mitad de SHADER_MAX_VERTEXES del juego de destino o más vértices (JKA y Jedi Outcast 500, "
     "Movie Duels 2000): su sombra de stencil (r_shadows 2) sale de la misma superficie del último LOD (ModView: Unshadowable surfaces).",
     "Červené čárky na každé ploše s polovinou SHADER_MAX_VERTEXES cílové hry nebo více vrcholy (JKA a Jedi Outcast 500, Movie Duels 2000): "
     "její stencilový stín (r_shadows 2) vzniká ze stejné plochy poslední LOD (ModView: Unshadowable surfaces)."),
    # --- Gruppe Boden und Kamera (P4) ---
    ("PqRBoden", "Floor and camera", "Boden und Kamera", "地面和相机", "床とカメラ", "Suelo y cámara", "Podlaha a kamera"),
    ("PqBoden", "Floor", "Boden", "地面", "床", "Suelo", "Podlaha"),
    ("PqBodenHint", "The grey floor at -24 as in ModView, or right under the lowest point of the figure in the pose.",
     "Der graue Boden auf -24 wie in ModView oder genau unter dem tiefsten Punkt der Figur in der Pose.",
     "灰色地面位于 -24 (与 ModView 相同), 或正好位于姿势中人物最低点之下。",
     "灰色の床を ModView と同じ -24 に、またはポーズ中の人物の最低点のすぐ下に置きます。",
     "El suelo gris a -24 como en ModView, o justo bajo el punto más bajo de la figura en la pose.",
     "Šedá podlaha na -24 jako v ModView, nebo přesně pod nejnižším bodem postavy v póze."),
    ("PqFovHint", "Field of view of the perspective in degrees (ModView: 10, 80 and 90).", "Blickwinkel der Perspektive in Grad (ModView: 10, 80 und 90).",
     "透视的视野角度 (ModView: 10、80 和 90)。", "パースの視野角 (度) (ModView: 10、80、90)。", "Campo de visión de la perspectiva en grados (ModView: 10, 80 y 90).",
     "Zorné pole perspektivy ve stupních (ModView: 10, 80 a 90)."),
    ("PqHintergrund", "Background", "Hintergrund", "背景", "背景", "Fondo", "Pozadí"),
    ("PqHintergrundHint", "A plain background colour like ModView (Clear colour); the box off shows the gradient again.",
     "Eine einfarbige Hintergrundfarbe wie in ModView (Clear colour); ohne Haken wieder der Verlauf.",
     "像 ModView 一样的纯色背景 (Clear colour); 取消勾选则恢复渐变。", "ModView と同じ単色の背景 (Clear colour)。チェックを外すとグラデーションに戻ります。",
     "Un color de fondo liso como en ModView (Clear colour); sin la casilla vuelve el degradado.",
     "Jednobarevné pozadí jako v ModView (Clear colour); bez zaškrtnutí opět přechod."),
    ("PqSchwelleWertHint", "Weights up to this value are left out when \"Bone weight threshold\" is on (0 to 1; ModView: 5 %).",
     "Gewichte bis zu diesem Wert fallen weg, wenn \"Knochengewicht-Schwelle\" an ist (0 bis 1; ModView: 5 %).",
     "启用“骨骼权重阈值”时, 不超过此值的权重被略去 (0 到 1; ModView: 5 %)。",
     "「ボーンウェイトのしきい値」がオンのとき、この値以下のウェイトを省きます (0〜1、ModView: 5 %)。",
     "Los pesos hasta este valor se omiten si \"Umbral de peso de hueso\" está activado (0 a 1; ModView: 5 %).",
     "Váhy do této hodnoty se vynechají, když je zapnutý \"Práh váhy kosti\" (0 až 1; ModView: 5 %)."),
    ("PqAnsichtZurueck", "Reset view", "Ansicht zurücksetzen", "重置视图", "ビューをリセット", "Restablecer vista", "Obnovit pohled"),
    ("PqAnsichtZurueckHint", "The whole figure from the front again, in all views (like loading it).", "Wieder die ganze Figur von vorn, in allen Ansichten (wie beim Laden).",
     "在所有视图中重新从正面显示整个人物 (与加载时相同)。", "すべてのビューで人物全体を再び正面から表示します (読み込み時と同じ)。",
     "Otra vez la figura entera de frente, en todas las vistas (como al cargarla).", "Znovu celá postava zepředu, ve všech pohledech (jako při načtení)."),
    ("PqFoto", "Photo (1920+)", "Foto (1920+)", "照片 (1920+)", "写真 (1920+)", "Foto (1920+)", "Fotka (1920+)"),
    # --- Gruppe Auswahl (X18) ---
    ("PqRAuswahl", "Selection", "Auswahl", "所选内容", "選択", "Selección", "Výběr"),
    ("PqAuswahlKeine", "Click a surface, tag or bone in the scene explorer or in the view - its values stand here.",
     "Eine Fläche, einen Tag oder Knochen im Szenen-Explorer oder in der Ansicht anklicken - hier stehen dann seine Werte.",
     "在场景资源管理器或视图中单击一个面、标签或骨骼 - 它的数值会显示在这里。",
     "シーンエクスプローラーかビューでサーフェス、タグ、ボーンをクリックすると、その値がここに表示されます。",
     "Pulsa una superficie, un tag o un hueso en el explorador de escena o en la vista: aquí aparecen sus valores.",
     "Klikněte na plochu, tag nebo kost v průzkumníku scény nebo v pohledu - tady pak budou její hodnoty."),
    ("PqAuswahlMehr", "%d more selected", "%d weitere gewählt", "另有 %d 个已选", "ほかに %d 個選択", "%d más seleccionadas", "dalších vybraných: %d"),
    # --- Gruppe Pruefung fuer das Spiel (P6) ---
    ("PqPrBalkenHint", "The selected surface (without a selection: the one closest to the limit) in the drawn LOD against the limit of the target game.",
     "Die gewählte Fläche (ohne Auswahl: die, die der Grenze am nächsten ist) in der gezeichneten LOD gegen die Grenze des Zielspiels.",
     "所选面 (未选择时: 最接近上限的面) 在当前绘制的 LOD 中与目标游戏上限的比较。",
     "選択したサーフェス (未選択なら上限に最も近いもの) の描画中の LOD を、対象ゲームの上限と比べます。",
     "La superficie elegida (sin selección: la más cercana al límite) en el LOD dibujado frente al límite del juego de destino.",
     "Vybraná plocha (bez výběru: ta nejblíž limitu) ve vykreslené LOD proti limitu cílové hry."),
    ("PqPrFlaechen", "Surfaces in the model", "Flächen im Modell", "模型中的面", "モデルのサーフェス", "Superficies en el modelo", "Plochy v modelu"),
    ("PqPrSequenzen", "Sequences within the GLA", "Sequenzen innerhalb der GLA", "GLA 范围内的序列", "GLA 内のシーケンス", "Secuencias dentro de la GLA", "Sekvence uvnitř GLA"),
    ("PqPrTags", "Tags the game code looks for", "Vom Spielcode gesuchte Tags", "游戏代码查找的标签", "ゲームコードが探すタグ", "Tags que busca el código del juego",
     "Tagy, které hledá kód hry"),
    ("PqPrTagsHint", "G2API_AddBolt with a fixed tag name for the player model (%s). Missing: %s",
     "G2API_AddBolt mit festem Tag-Namen für das Spielermodell (%s). Fehlt: %s", "对玩家模型使用固定标签名调用 G2API_AddBolt (%s)。缺少: %s",
     "プレイヤーモデルに固定タグ名で G2API_AddBolt (%s)。不足: %s", "G2API_AddBolt con nombre de tag fijo para el modelo del jugador (%s). Faltan: %s",
     "G2API_AddBolt s pevným názvem tagu pro model hráče (%s). Chybí: %s"),
    ("PqPrTagsKeine", "- (not a _humanoid skeleton)", "- (kein _humanoid-Skelett)", "- (不是 _humanoid 骨骼)", "- (_humanoid 骨格ではありません)",
     "- (no es un esqueleto _humanoid)", "- (není kostra _humanoid)"),
    ("PqPrShader", "Shaders without image", "Shader ohne Bild", "无图像的着色器", "画像のないシェーダー", "Shaders sin imagen", "Shadery bez obrázku"),
    ("PqPrLods", "LODs", "LODs", "LOD", "LOD", "LOD", "LOD"),
    ("PqPrLodsWert", "%d (the game picks the LOD by distance)", "%d (Spiel nimmt LOD nach Abstand)", "%d (游戏按距离选择 LOD)", "%d (ゲームは距離で LOD を選択)",
     "%d (el juego elige el LOD por distancia)", "%d (hra volí LOD podle vzdálenosti)"),
    ("PqPrSchatten", "Without own shadow", "Ohne eigenen Schatten", "无自身阴影", "自前の影なし", "Sin sombra propia", "Bez vlastního stínu"),
    # --- Gruppe Szene (P7) ---
    ("PqRSzene", "Scene", "Szene", "场景", "シーン", "Escena", "Scéna"),
    ("PqSzeneSpeichern", "Save scene...", "Szene speichern...", "保存场景……", "シーンを保存...", "Guardar escena...", "Uložit scénu..."),
    ("PqSzeneSpeichernHint", "Model, skin, attachments, primary and secondary sequence, surfaces on and off, root, level of detail and view as a "
     "ModView script (.mvs) - ModView reads it too.",
     "Modell, Skin, Anbauten, Primär- und Sekundärsequenz, Flächen an und aus, Wurzel, Detailstufe und Ansicht als ModView-Skript (.mvs) - "
     "ModView liest es auch.",
     "将模型、皮肤、附件、主/次序列、面的开关、根、细节级别和视图保存为 ModView 脚本 (.mvs) - ModView 也能读取。",
     "モデル、スキン、アタッチメント、プライマリ/セカンダリシーケンス、サーフェスのオン/オフ、ルート、詳細レベル、ビューを ModView スクリプト (.mvs) として保存します。ModView でも読めます。",
     "Modelo, skin, acoplados, secuencia primaria y secundaria, superficies encendidas y apagadas, raíz, nivel de detalle y vista como script de "
     "ModView (.mvs); ModView también lo lee.",
     "Model, skin, připojené modely, primární a sekundární sekvence, plochy zapnuté a vypnuté, kořen, úroveň detailu a pohled jako skript "
     "ModView (.mvs) - ModView ho přečte také."),
    ("PqMvsLesen", "Read ModView script...", "ModView-Skript lesen...", "读取 ModView 脚本……", "ModView スクリプトを読む...", "Leer script de ModView...",
     "Načíst skript ModView..."),
    ("PqMvsLesenHint", "A ModView script (.mvs): model, attachments on tags and bones, primary and secondary sequence, skin, surfaces on and off, root.",
     "Ein ModView-Skript (.mvs): Modell, Anbauten an Tags und Knochen, Primär- und Sekundärsequenz, Skin, Flächen an und aus, Wurzel.",
     "ModView 脚本 (.mvs): 模型、挂在标签和骨骼上的附件、主/次序列、皮肤、面的开关、根。",
     "ModView スクリプト (.mvs): モデル、タグやボーンへのアタッチメント、プライマリ/セカンダリシーケンス、スキン、サーフェスのオン/オフ、ルート。",
     "Un script de ModView (.mvs): modelo, acoplados en tags y huesos, secuencia primaria y secundaria, skin, superficies encendidas y apagadas, raíz.",
     "Skript ModView (.mvs): model, připojené modely na tagy a kosti, primární a sekundární sekvence, skin, plochy zapnuté a vypnuté, kořen."),
    ("PqNpc", "NPC browser", "NPC-Browser", "NPC 浏览器", "NPC ブラウザー", "Navegador de NPC", "Prohlížeč NPC"),
    ("PqSzeneGespeichert", "Scene saved: %s", "Szene gespeichert: %s", "场景已保存: %s", "シーンを保存しました: %s", "Escena guardada: %s", "Scéna uložena: %s"),
    ("PqSzeneGelesen", "Scene read: %s", "Szene gelesen: %s", "已读取场景: %s", "シーンを読み込みました: %s", "Escena leída: %s", "Scéna načtena: %s"),
    ("PqSzeneFehler", "Scene not read: %s", "Szene nicht gelesen: %s", "未能读取场景: %s", "シーンを読めません: %s", "Escena no leída: %s", "Scénu nelze načíst: %s"),
    ("PqSzeneOhne", "Not taken from the script: %s", "Aus dem Skript nicht übernommen: %s", "脚本中未采用: %s", "スクリプトから取り込まなかったもの: %s",
     "No se tomó del script: %s", "Ze skriptu nepřevzato: %s"),
    ("PqSzeneSpeichernFehler", "Scene not saved: %s", "Szene nicht gespeichert: %s", "场景未保存: %s", "シーンを保存できません: %s", "Escena no guardada: %s",
     "Scénu nelze uložit: %s"),
    # --- Ansicht (R10), Tasten wie Blender (P10) ---
    ("PqLodHinweis", "LOD %d · %d {2:triangle|triangles} · drawn as in the game (Ghoul2 from the Mission tab)",
     "LOD %d · %d {2:Dreieck|Dreiecke} · wie im Spiel gezeichnet (Ghoul2 aus dem Mission-Tab)", "LOD %d · %d 个三角形 · 与游戏中一样绘制 (来自任务选项卡的 Ghoul2)",
     "LOD %d · 三角形 %d · ゲームと同じ描画 (ミッションタブの Ghoul2)", "LOD %d · %d {2:triángulo|triángulos} · dibujado como en el juego (Ghoul2 de la pestaña Mission)",
     "LOD %d · %d {2:trojúhelník|trojúhelníky|trojúhelníků} · vykresleno jako ve hře (Ghoul2 z karty Mission)"),
    ("PqSchattierungMenue", "Shading (pie menu)", "Schattierung (Auswahl)", "着色 (饼菜单)", "シェーディング (パイメニュー)", "Sombreado (menú circular)",
     "Stínování (koláčové menu)"),
    ("PqOverlays", "Show overlays", "Überlagerungen zeigen", "显示叠加层", "オーバーレイを表示", "Mostrar superposiciones", "Zobrazit překryvy"),
    ("PqOverlaysHint", "Like Blender: grid, floor, axes and everything drawn over the model (bones, tags, numbers, selection) on or off; the header and "
     "the navigation gizmo stay.",
     "Wie Blender: Raster, Boden, Achsen und alles, was über dem Modell liegt (Knochen, Tags, Nummern, Auswahl), an oder aus; Kopfzeile und "
     "Navigations-Gizmo bleiben.",
     "与 Blender 相同: 网格、地面、坐标轴以及模型上方的所有内容 (骨骼、标签、编号、所选内容) 一起开关; 标题栏和导航小工具保留。",
     "Blender と同じ: グリッド、床、軸、モデルの上に重なるすべて (ボーン、タグ、番号、選択) をまとめてオン/オフします。ヘッダーとナビゲーションギズモは残ります。",
     "Como Blender: cuadrícula, suelo, ejes y todo lo que está sobre el modelo (huesos, tags, números, selección) activado o desactivado; la cabecera y "
     "el gizmo de navegación se mantienen.",
     "Jako Blender: mřížka, podlaha, osy a vše nad modelem (kosti, tagy, čísla, výběr) zapnuto nebo vypnuto; záhlaví a navigační gizmo zůstávají."),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
