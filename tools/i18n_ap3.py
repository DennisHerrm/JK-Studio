# -*- coding: utf-8 -*-
"""Oberflaechentexte von AP3 (MBENTWURF Teil B): das rechte Feld des Model-Tabs - Befehlsfeld mit Reitern (wie 3ds Max),
Eigenschaften-Editor mit senkrechter Reiterspalte (wie Blender), Kurzinfos und Werte der Gruppen von Ansehen/Pruefen,
Hinweis neben den Bereichsreitern - fuer tools/gen_i18n.py und die weiteren Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    # --- Titel des Felds ---
    ("MfEigenschaften", "Properties", "Eigenschaften", "属性", "プロパティ", "Propiedades", "Vlastnosti"),
    # --- Reiter wie 3ds Max (Befehlsfeld) ---
    ("MfErstellen", "Create", "Erstellen", "创建", "作成", "Crear", "Vytvořit"),
    ("MfAendern", "Modify", "Ändern", "修改", "修正", "Modificar", "Upravit"),
    ("MfHierarchie", "Hierarchy", "Hierarchie", "层次", "階層", "Jerarquía", "Hierarchie"),
    ("MfBewegung", "Motion", "Bewegung", "运动", "モーション", "Movimiento", "Pohyb"),
    ("MfAnzeige", "Display", "Anzeige", "显示", "表示", "Presentación", "Zobrazení"),
    ("MfDienste", "Utilities", "Dienstprogramme", "实用程序", "ユーティリティ", "Utilidades", "Nástroje"),
    ("MfErstellenHint", "New things: a new prop, primitives, import.", "Neues: neues Requisit, Grundkörper, Importieren.", "新建: 新道具、基本体、导入。",
     "新規: 新しい小道具、プリミティブ、インポート。", "Lo nuevo: nuevo objeto, primitivas, importar.", "Nové: nová rekvizita, základní tělesa, import."),
    ("MfAendernHint", "The tools of this area for the selection.", "Die Werkzeuge dieses Bereichs für die Auswahl.", "此区域用于所选内容的工具。", "このエリアの選択用ツール。",
     "Las herramientas de esta área para la selección.", "Nástroje této oblasti pro výběr."),
    ("MfHierarchieHint", "What hangs on what: root surface, attached models, start of the secondary sequence, bones of the rig.",
     "Was woran hängt: Wurzelfläche, angebaute Modelle, Start der Sekundärsequenz, Knochen des Rigs.", "连接关系: 根面、挂接的模型、次级序列的起点、绑定的骨骼。",
     "つながり: ルートサーフェス、取り付けたモデル、セカンダリシーケンスの開始、リグのボーン。",
     "Qué cuelga de qué: superficie raíz, modelos acoplados, inicio de la secuencia secundaria, huesos del rig.",
     "Co na čem visí: kořenový povrch, připojené modely, začátek sekundární sekvence, kosti rigu."),
    ("MfBewegungHint", "Animation: rig, IK/FK, physics, transfer, presets, GLA and animation.cfg.", "Animation: Rig, IK/FK, Physik, Übertragen, Presets, GLA und animation.cfg.",
     "动画: 绑定、IK/FK、物理、重定向、预设、GLA 和 animation.cfg。", "アニメーション: リグ、IK/FK、物理、リターゲット、プリセット、GLA と animation.cfg。",
     "Animación: rig, IK/FK, física, transferencia, preajustes, GLA y animation.cfg.", "Animace: rig, IK/FK, fyzika, přenos, předvolby, GLA a animation.cfg."),
    ("MfAnzeigeHint", "What you see: the model, level of detail and skin, floor and camera.", "Was man sieht: Modell, Detailstufe und Skin, Boden und Kamera.",
     "所见内容: 模型、细节级别和皮肤、地面和摄像机。", "見えるもの: モデル、詳細レベルとスキン、床とカメラ。",
     "Lo que se ve: el modelo, nivel de detalle y skin, suelo y cámara.", "Co je vidět: model, úroveň detailu a skin, podlaha a kamera."),
    ("MfDiensteHint", "Check for the game, scene, photo.", "Prüfung für das Spiel, Szene, Foto.", "针对游戏的检查、场景、照片。", "ゲーム向けチェック、シーン、写真。",
     "Comprobación para el juego, escena, foto.", "Kontrola pro hru, scéna, fotka."),
    # --- Reiter wie Blender (Eigenschaften-Editor) ---
    ("MfWerkzeug", "Tool", "Werkzeug", "工具", "ツール", "Herramienta", "Nástroj"),
    ("MfObjekt", "Object", "Objekt", "物体", "オブジェクト", "Objeto", "Objekt"),
    ("MfModifier", "Modifiers", "Modifikatoren", "修改器", "モディファイアー", "Modificadores", "Modifikátory"),
    ("MfDaten", "Data", "Daten", "数据", "データ", "Datos", "Data"),
    ("MfKnochen", "Bone", "Knochen", "骨骼", "ボーン", "Hueso", "Kost"),
    ("MfConstraints", "Constraints", "Beschränkungen", "约束", "コンストレイント", "Restricciones", "Vazby"),
    ("MfMaterial", "Material", "Material", "材质", "マテリアル", "Material", "Materiál"),
    ("MfWerkzeugHint", "The settings of the active tool (brush).", "Die Einstellungen des aktiven Werkzeugs (Pinsel).", "活动工具 (笔刷) 的设置。", "アクティブなツール (ブラシ) の設定。",
     "Los ajustes de la herramienta activa (pincel).", "Nastavení aktivního nástroje (štětce)."),
    ("MfObjektHint", "The model as a whole: file, level of detail, attachments, check for the game.", "Das Modell als Ganzes: Datei, Detailstufe, Anbauten, Prüfung für das Spiel.",
     "整个模型: 文件、细节级别、挂接物、针对游戏的检查。", "モデル全体: ファイル、詳細レベル、取り付け、ゲーム向けチェック。",
     "El modelo completo: archivo, nivel de detalle, acoples, comprobación para el juego.", "Model jako celek: soubor, úroveň detailu, připojené modely, kontrola pro hru."),
    ("MfModifierHint", "The modifier stack and the tools of modeling.", "Der Modifier-Stapel und die Werkzeuge des Modellierens.", "修改器堆栈和建模工具。", "モディファイアースタックとモデリングのツール。",
     "La pila de modificadores y las herramientas de modelado.", "Zásobník modifikátorů a nástroje modelování."),
    ("MfDatenHint", "The data of the mesh: UV, weights (vertex groups), levels of detail.", "Die Daten des Netzes: UV, Gewichte (Vertexgruppen), Detailstufen.",
     "网格数据: UV、权重 (顶点组)、细节级别。", "メッシュのデータ: UV、ウェイト (頂点グループ)、詳細レベル。",
     "Los datos de la malla: UV, pesos (grupos de vértices), niveles de detalle.", "Data sítě: UV, váhy (skupiny vrcholů), úrovně detailu."),
    ("MfKnochenHint", "Bones, rig and animation.", "Knochen, Rig und Animation.", "骨骼、绑定和动画。", "ボーン、リグ、アニメーション。", "Huesos, rig y animación.", "Kosti, rig a animace."),
    ("MfConstraintsHint", "What follows what: IK/FK, models attached to tags and bones.", "Was wem folgt: IK/FK, an Tags und Knochen angebaute Modelle.",
     "跟随关系: IK/FK、挂接到标签和骨骼的模型。", "追従関係: IK/FK、タグやボーンに取り付けたモデル。", "Qué sigue a qué: IK/FK, modelos acoplados a tags y huesos.",
     "Co co následuje: IK/FK, modely připojené k tagům a kostem."),
    ("MfMaterialHint", "Skins, shaders and textures.", "Skins, Shader und Texturen.", "皮肤、着色器和纹理。", "スキン、シェーダー、テクスチャ。", "Skins, shaders y texturas.",
     "Skiny, shadery a textury."),
    ("MfReiterLeer", "Nothing on this tab in this area.", "In diesem Bereich steht hier nichts.", "此区域在该选项卡中没有内容。", "このエリアではこのタブに項目はありません。",
     "En esta área no hay nada en esta pestaña.", "V této oblasti na této kartě nic není."),
    ("MfBreiteHint", "Drag: width of the panel. Double-click: default width.", "Ziehen: Breite des Felds. Doppelklick: Vorgabe.", "拖动: 面板宽度。双击: 默认宽度。",
     "ドラッグ: パネルの幅。ダブルクリック: 既定の幅。", "Arrastrar: ancho del panel. Doble clic: ancho predeterminado.", "Tažení: šířka panelu. Dvojklik: výchozí šířka."),
    # --- Hinweis neben den Bereichsreitern (je Bedienmodus und Bereich; die Tasten des Bereichs stehen in der Statuszeile) ---
    ("MfTippMaxAnsehen", "Q Select · F3 Wireframe · Alt+W 4 views · Z Zoom extents", "Q Auswählen · F3 Drahtgitter · Alt+W 4 Ansichten · Z Zoom Grenzen",
     "Q 选择 · F3 线框 · Alt+W 4 视图 · Z 最大化显示", "Q 選択 · F3 ワイヤーフレーム · Alt+W 4 ビュー · Z 全体表示", "Q Seleccionar · F3 Alambre · Alt+W 4 vistas · Z Zoom extensión",
     "Q Výběr · F3 Drátový model · Alt+W 4 pohledy · Z Zoom na vše"),
    ("MfTippMaxBearbeiten", "1-5 sub-object levels, 6 = object · W/E/R Move/Rotate/Scale · / Play",
     "1-5 Unterebenen, 6 = Objekt · W/E/R Verschieben/Drehen/Skalieren · / Abspielen", "1-5 子对象层级, 6 = 对象 · W/E/R 移动/旋转/缩放 · / 播放",
     "1-5 サブオブジェクト レベル、6 = オブジェクト · W/E/R 移動/回転/スケール · / 再生", "1-5 niveles de subobjeto, 6 = objeto · W/E/R Mover/Rotar/Escalar · / Reproducir",
     "1-5 úrovně podobjektů, 6 = objekt · W/E/R Posun/Otočení/Měřítko · / Přehrát"),
    ("MfTippMaxAnim", "W/E/R Move/Rotate/Scale · N Auto Key · K Set Key · / Play", "W/E/R Verschieben/Drehen/Skalieren · N Auto Key · K Set Key · / Abspielen",
     "W/E/R 移动/旋转/缩放 · N Auto Key · K Set Key · / 播放", "W/E/R 移動/回転/スケール · N Auto Key · K Set Key · / 再生",
     "W/E/R Mover/Rotar/Escalar · N Auto Key · K Set Key · / Reproducir", "W/E/R Posun/Otočení/Měřítko · N Auto Key · K Set Key · / Přehrát"),
    ("MfTippBlAnsehen", "Shift+Z Wireframe · Numpad views · Home Frame all · Space Play", "Umschalt+Z Drahtgitter · Ziffernblock-Ansichten · Pos1 Alles zeigen · Leertaste Abspielen",
     "Shift+Z 线框 · 小键盘视图 · Home 显示全部 · 空格 播放", "Shift+Z ワイヤーフレーム · テンキーでビュー · Home 全体表示 · Space 再生",
     "Mayús+Z Alambre · Vistas con el teclado numérico · Inicio Ver todo · Espacio Reproducir", "Shift+Z Drátový model · Pohledy na numerické klávesnici · Home Zobrazit vše · Mezerník Přehrát"),
    ("MfTippBlBearbeiten", "Tab Edit · Ctrl+Tab Mode · 1/2/3 Vertex/Edge/Face · G/R/S Grab/Rotate/Scale", "Tab Bearbeiten · Strg+Tab Modus · 1/2/3 Ecke/Kante/Fläche · G/R/S Greifen/Drehen/Skalieren",
     "Tab 编辑 · Ctrl+Tab 模式 · 1/2/3 顶点/边/面 · G/R/S 移动/旋转/缩放", "Tab 編集 · Ctrl+Tab モード · 1/2/3 頂点/エッジ/面 · G/R/S 移動/回転/スケール",
     "Tab Edición · Ctrl+Tab Modo · 1/2/3 Vértice/Arista/Cara · G/R/S Mover/Rotar/Escalar", "Tab Editace · Ctrl+Tab Režim · 1/2/3 Vrchol/Hrana/Plocha · G/R/S Posun/Otočení/Měřítko"),
    ("MfTippBlAnim", "Ctrl+Tab Mode · G/R/S Grab/Rotate/Scale · I Key · Space Play", "Strg+Tab Modus · G/R/S Greifen/Drehen/Skalieren · I Schlüssel · Leertaste Abspielen",
     "Ctrl+Tab 模式 · G/R/S 移动/旋转/缩放 · I 关键帧 · 空格 播放", "Ctrl+Tab モード · G/R/S 移動/回転/スケール · I キー · Space 再生",
     "Ctrl+Tab Modo · G/R/S Mover/Rotar/Escalar · I Clave · Espacio Reproducir", "Ctrl+Tab Režim · G/R/S Posun/Otočení/Měřítko · I Klíč · Mezerník Přehrát"),
    # --- Gruppen von Ansehen/Pruefen in der Form des Entwurfs: Kurzinfos und Werte ---
    ("MfInfoDreiecke", "%d {triangle|triangles}", "%d {Dreieck|Dreiecke}", "%d 个三角形", "三角形 %d", "%d {triángulo|triángulos}", "%d {trojúhelník|trojúhelníky|trojúhelníků}"),
    ("MfInfoLod", "LOD %d", "LOD %d", "LOD %d", "LOD %d", "LOD %d", "LOD %d"),
    ("MfInfoFehler", "%d {finding|findings}", "%d {Befund|Befunde}", "%d 项问题", "指摘 %d 件", "%d {aviso|avisos}", "%d {nález|nálezy|nálezů}"),
    ("MfInfoGut", "all good", "alles gut", "一切正常", "問題なし", "todo bien", "vše v pořádku"),
    ("MfDreiecke", "Triangles", "Dreiecke", "三角形", "三角形", "Triángulos", "Trojúhelníky"),
    ("MfEcken", "Vertices", "Ecken", "顶点", "頂点", "Vértices", "Vrcholy"),
    ("MfAnbauTipp", "Attach: right-click a tag or a bone in the tree.", "Anbauen: Rechtsklick auf einen Tag oder Knochen im Baum.", "挂接: 在树中右键单击标签或骨骼。",
     "取り付け: ツリーでタグまたはボーンを右クリック。", "Acoplar: clic derecho en un tag o un hueso del árbol.", "Připojení: klikněte pravým tlačítkem na tag nebo kost ve stromu."),
    ("MfPrEcken", "Vertices per surface", "Ecken je Fläche", "每个面的顶点数", "サーフェスごとの頂点", "Vértices por superficie", "Vrcholy na povrch"),
    ("MfPrDreiecke", "Triangles per surface", "Dreiecke je Fläche", "每个面的三角形数", "サーフェスごとの三角形", "Triángulos por superficie", "Trojúhelníky na povrch"),
    ("MfPrKnochen", "Bones per surface", "Knochen je Fläche", "每个面的骨骼数", "サーフェスごとのボーン", "Huesos por superficie", "Kosti na povrch"),
    ("MfPrGewichte", "Weights per vertex", "Gewichte je Ecke", "每个顶点的权重数", "頂点ごとのウェイト", "Pesos por vértice", "Váhy na vrchol"),
    ("MfPrInstanzen", "Ghoul2 instances in this view", "Ghoul2-Instanzen in dieser Ansicht", "此视图中的 Ghoul2 实例", "このビューの Ghoul2 インスタンス", "Instancias Ghoul2 en esta vista",
     "Instance Ghoul2 v tomto pohledu"),
    ("MfPrUnter", "below %d", "unter %d", "低于 %d", "%d 未満", "menos de %d", "méně než %d"),
    ("MfPrHoechstens", "at most %d", "höchstens %d", "最多 %d", "最大 %d", "como máximo %d", "nejvýše %d"),
    ("MfPrGrenzenVon", "Limits of %s", "Grenzen von %s", "%s 的限制", "%s の制限", "Límites de %s", "Limity %s"),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
