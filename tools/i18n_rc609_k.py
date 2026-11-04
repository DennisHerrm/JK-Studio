# -*- coding: utf-8 -*-
"""Oberflaechentexte von rc609 K (Texturen der Karte: Uebersicht und Ersetzen im Karten-Editor) fuer tools/gen_i18n.py und die weiteren Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("TxlPanel", "Map textures", "Texturen der Karte", "地图纹理", "マップのテクスチャ", "Texturas del mapa", "Textury mapy"),
    ("TxlMenue", "Map textures...", "Texturen der Karte...", "地图纹理...", "マップのテクスチャ...", "Texturas del mapa...", "Textury mapy..."),
    ("TxlMenueTip", "Every shader and texture the map uses, how often, and replacing them - also whole folders - on brushes and patches in one undo step.",
     "Jeder Shader und jede Textur der Karte, wie oft, und Ersetzen - auch ganzer Ordner - auf Brushes und Patches in einem Rückgängig-Schritt.",
     "地图使用的每个着色器和纹理、使用次数,以及在一个撤销步骤内替换画刷和面片上的纹理(也可替换整个文件夹)。",
     "マップで使われているすべてのシェーダーとテクスチャ、その使用数、そしてブラシとパッチ上での置き換え (フォルダーごとも可) を 1 回の元に戻す操作で行います。",
     "Cada shader y textura que usa el mapa, cuántas veces, y su sustitución (también carpetas enteras) en brushes y patches en un solo paso de deshacer.",
     "Každý shader a textura mapy, jak často, a jejich nahrazení - i celých složek - na brushích a patchích v jednom kroku zpět."),
    ("TxlUmfang", "Scope:", "Umfang:", "范围:", "範囲:", "Ámbito:", "Rozsah:"),
    ("TxlAlle", "Whole map", "Ganze Karte", "整个地图", "マップ全体", "Mapa entero", "Celá mapa"),
    ("TxlAuswahl", "Selection only", "Nur Auswahl", "仅选中", "選択のみ", "Solo la selección", "Jen výběr"),
    ("TxlSichtbar", "Visible only", "Nur sichtbare", "仅可见", "表示中のみ", "Solo lo visible", "Jen viditelné"),
    ("TxlFilter", "Filter by name", "Nach Namen filtern", "按名称筛选", "名前で絞り込み", "Filtrar por nombre", "Filtrovat podle názvu"),
    ("TxlSpBild", "Image", "Bild", "图像", "画像", "Imagen", "Obrázek"),
    ("TxlSpShader", "Shader", "Shader", "着色器", "シェーダー", "Shader", "Shader"),
    ("TxlSpSeiten", "Brush sides", "Brush-Seiten", "画刷面", "ブラシ面", "Caras de brush", "Strany brushů"),
    ("TxlSpBrushes", "Brushes", "Brushes", "画刷", "ブラシ", "Brushes", "Brushe"),
    ("TxlSpPatches", "Patches", "Patches", "面片", "パッチ", "Patches", "Patche"),
    ("TxlSpGroesse", "Size", "Größe", "尺寸", "サイズ", "Tamaño", "Velikost"),
    ("TxlKlickHint", "Click: select every brush side and patch with this texture in the scope. Ctrl+click: add them to the selection.",
     "Klick: jede Brush-Seite und jeden Patch mit dieser Textur im Umfang wählen. Strg+Klick: zur Auswahl hinzufügen.",
     "单击:选择范围内使用此纹理的所有画刷面和面片。Ctrl+单击:添加到选择中。",
     "クリック: 範囲内でこのテクスチャを使うすべてのブラシ面とパッチを選択します。Ctrl+クリック: 選択に追加します。",
     "Clic: selecciona cada cara de brush y cada patch con esta textura en el ámbito. Ctrl+clic: los añade a la selección.",
     "Klik: vybere každou stranu brushe a každý patch s touto texturou v rozsahu. Ctrl+klik: přidá je k výběru."),
    ("TxlBildFehlt", "(image missing)", "(Bild fehlt)", "(缺少图像)", "(画像なし)", "(falta la imagen)", "(chybí obrázek)"),
    ("TxlLeer", "No textures in this scope.", "Keine Texturen in diesem Umfang.", "此范围内没有纹理。", "この範囲にテクスチャはありません。",
     "No hay texturas en este ámbito.", "V tomto rozsahu nejsou žádné textury."),
    ("TxlKeineZeile", "Click a texture in the list to replace it.", "Eine Textur in der Liste anklicken, um sie zu ersetzen.",
     "单击列表中的纹理以替换它。", "置き換えるには一覧のテクスチャをクリックしてください。", "Haz clic en una textura de la lista para sustituirla.",
     "Klikněte na texturu v seznamu, chcete-li ji nahradit."),
    ("TxlGewaehlt", "%s: %d {brush side|brush sides}, %d {2:patch|patches}", "%s: %d {Brush-Seite|Brush-Seiten}, %d {2:Patch|Patches}", "%s:%d 个画刷面,%d 个面片",
     "%s: ブラシ面 %d、パッチ %d", "%s: %d {cara de brush|caras de brush}, %d {2:patch|patches}", "%s: %d {strana brushe|strany brushů|stran brushů}, %d {2:patch|patche|patchů}"),
    ("TxlGewaehltN", "Selected: %d {brush side|brush sides}, %d {2:patch|patches}.", "Gewählt: %d {Brush-Seite|Brush-Seiten}, %d {2:Patch|Patches}.", "已选择:%d 个画刷面,%d 个面片。",
     "選択: ブラシ面 %d、パッチ %d。", "Seleccionado: %d {cara de brush|caras de brush}, %d {2:patch|patches}.", "Vybráno: %d {strana brushe|strany brushů|stran brushů}, %d {2:patch|patche|patchů}."),
    ("TxlErsetzenDurch", "Replace with", "Ersetzen durch", "替换为", "置き換え先", "Sustituir por", "Nahradit"),
    ("TxlAusBrowser", "From texture browser", "Aus dem Texturbrowser", "取自纹理浏览器", "テクスチャブラウザーから", "Del explorador de texturas",
     "Z prohlížeče textur"),
    ("TxlAusBrowserHint", "Takes the texture selected in the texture browser - from any folder.",
     "Nimmt die im Texturbrowser gewählte Textur - aus einem beliebigen Ordner.", "使用纹理浏览器中选中的纹理(可来自任意文件夹)。",
     "テクスチャブラウザーで選んだテクスチャを使います (どのフォルダーからでも)。", "Toma la textura elegida en el explorador de texturas, de cualquier carpeta.",
     "Vezme texturu vybranou v prohlížeči textur - z libovolné složky."),
    ("TxlWeltgroesse", "Keep world size", "Gleiche Weltgröße", "保持世界尺寸", "ワールドサイズを保つ", "Mantener el tamaño en el mundo",
     "Zachovat velikost ve světě"),
    ("TxlWeltgroesseHint",
     "Rescales the brush sides so that one repeat of the new texture covers the same area as the old one, even if its image is larger or smaller. Off: the texture definition stays as it is, as in Find / Replace. Patches keep their texture coordinates either way.",
     "Skaliert die Brush-Seiten so, dass eine Wiederholung der neuen Textur dieselbe Fläche deckt wie die alte, auch wenn ihr Bild größer oder kleiner ist. Aus: die Texturdefinition bleibt, wie bei Find / Replace. Patches behalten ihre Texturkoordinaten in beiden Fällen.",
     "重新缩放画刷面,使新纹理的一次重复覆盖与旧纹理相同的面积,即使其图像更大或更小。关闭:纹理定义保持不变,与 Find / Replace 相同。面片在两种情况下都保留其纹理坐标。",
     "新しいテクスチャの 1 回の繰り返しが、画像の大小にかかわらず古いテクスチャと同じ面積を覆うようにブラシ面を拡大縮小します。オフ: Find / Replace と同じく、テクスチャ定義はそのままです。パッチはどちらの場合もテクスチャ座標を保ちます。",
     "Reescala las caras de brush para que una repetición de la textura nueva cubra la misma área que la antigua, aunque su imagen sea mayor o menor. Desactivado: la definición de textura se queda como está, igual que en Find / Replace. Los patches conservan sus coordenadas de textura en ambos casos.",
     "Přeškáluje strany brushů tak, aby jedno opakování nové textury pokrylo stejnou plochu jako stará, i když je její obrázek větší nebo menší. Vypnuto: definice textury zůstane, jako u Find / Replace. Patche si v obou případech ponechají souřadnice textury."),
    ("TxlFehltNeu", "%s is not among the game's shaders and textures.", "%s gibt es nicht unter den Shadern und Texturen des Spiels.",
     "游戏的着色器和纹理中没有 %s。", "%s はゲームのシェーダーとテクスチャにありません。", "%s no está entre los shaders y texturas del juego.",
     "%s mezi shadery a texturami hry není."),
    ("TxlErsetzen", "Replace", "Ersetzen", "替换", "置き換え", "Sustituir", "Nahradit"),
    ("TxlUngueltig", "Invalid texture name: %s", "Ungültiger Texturname: %s", "无效的纹理名称:%s", "無効なテクスチャ名: %s",
     "Nombre de textura no válido: %s", "Neplatný název textury: %s"),
    ("TxlOrdner", "Replace folder", "Ordner ersetzen", "替换文件夹", "フォルダーを置き換え", "Sustituir carpeta", "Nahradit složku"),
    ("TxlOrdnerAlt", "Old folder", "Alter Ordner", "旧文件夹", "元のフォルダー", "Carpeta antigua", "Stará složka"),
    ("TxlOrdnerNeu", "New folder", "Neuer Ordner", "新文件夹", "新しいフォルダー", "Carpeta nueva", "Nová složka"),
    ("TxlOrdnerBrowser", "Folder of the texture browser", "Ordner des Texturbrowsers", "纹理浏览器的文件夹", "テクスチャブラウザーのフォルダー",
     "Carpeta del explorador de texturas", "Složka prohlížeče textur"),
    ("TxlOrdnerBrowserHint", "Takes the folder shown in the texture browser as the new folder.",
     "Nimmt den Ordner, den der Texturbrowser zeigt, als neuen Ordner.", "将纹理浏览器当前显示的文件夹用作新文件夹。",
     "テクスチャブラウザーに表示中のフォルダーを新しいフォルダーにします。", "Usa la carpeta que muestra el explorador de texturas como carpeta nueva.",
     "Použije složku zobrazenou v prohlížeči textur jako novou složku."),
    ("TxlZuordnung", "Show mapping", "Zuordnung zeigen", "显示对应关系", "対応を表示", "Mostrar correspondencia", "Zobrazit přiřazení"),
    ("TxlZuordnungZahl", "%d {texture|textures} of the old folder, %d {2:gap|gaps} (missing in the new folder - they stay).",
     "%d {Textur|Texturen} des alten Ordners, %d {2:Lücke|Lücken} (fehlen im neuen Ordner - sie bleiben).", "旧文件夹中有 %d 个纹理,%d 个缺口(新文件夹中缺少,保持不变)。",
     "元のフォルダーのテクスチャ %d 個、欠け %d 個 (新しいフォルダーに無いもので、そのまま残ります)。",
     "%d {textura|texturas} de la carpeta antigua, %d {2:hueco|huecos} (faltan en la carpeta nueva; se quedan).",
     "%d {textura|textury|textur} staré složky, %d {2:mezera|mezery|mezer} (v nové složce chybí - zůstanou)."),
    ("TxlLuecke", "missing", "fehlt", "缺少", "なし", "falta", "chybí"),
    ("TxlOrdnerErsetzen", "Replace %d {texture|textures}", "%d {Textur|Texturen} ersetzen", "替换 %d 个纹理", "%d 個のテクスチャを置き換え", "Sustituir %d {textura|texturas}",
     "Nahradit %d {texturu|textury|textur}"),
    ("TxlNichts", "Nothing to replace in this scope.", "In diesem Umfang gibt es nichts zu ersetzen.", "此范围内没有可替换的内容。",
     "この範囲に置き換えるものはありません。", "No hay nada que sustituir en este ámbito.", "V tomto rozsahu není co nahradit."),
    ("TxlErsetzt", "Replaced on %d {brush side|brush sides} and %d {2:patch|patches} (one undo step).", "Ersetzt auf %d {Brush-Seite|Brush-Seiten} und %d {2:Patch|Patches} (ein Rückgängig-Schritt).",
     "已在 %d 个画刷面和 %d 个面片上替换(一个撤销步骤)。", "ブラシ面 %d とパッチ %d で置き換えました (元に戻す 1 回分)。",
     "Sustituido en %d {cara de brush|caras de brush} y %d {2:patch|patches} (un paso de deshacer).", "Nahrazeno na %d {straně brushe|stranách brushů|stranách brushů} a %d {2:patchi|patchích|patchích} (jeden krok zpět)."),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
