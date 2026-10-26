# -*- coding: utf-8 -*-
"""Oberflaechentexte des Karten-Editors, Stufe 6 (Pruefstufe), fuer
tools/gen_i18n.py.

NRC#279 (ABSICHT: die Kappen gehen nur fuer bestimmte Patch-Groessen): NRC
schreibt nur die englische Konsolenzeile "cannot create cylinder-cap - neither
patch width, nor height is 9". Unter der unveraenderten englischen Zeile steht
ein Hinweis in der Programmsprache (die Begriffe wie im Kappen-Dialog).
"""

# (Kennung, English, Deutsch, 中文, 日本語)
KARTE6 = [
    # --- NRC#279: Kappen-Hinweise ------------------------------------------------
    ("KeCapHintBevel",
     "Bevel caps need a patch that is 3 or 5 control points wide or high.",
     "Eine Fase braucht einen Patch, der 3 oder 5 Kontrollpunkte breit oder hoch ist.",
     "斜面封盖需要宽度或高度为 3 或 5 个控制点的面片。",
     "ベベルには幅または高さが制御点 3 つか 5 つのパッチが必要です。"),
    ("KeCapHintEndcap",
     "Endcaps need a patch that is 5 control points wide or high.",
     "Eine Endkappe braucht einen Patch, der 5 Kontrollpunkte breit oder hoch ist.",
     "端盖需要宽度或高度为 5 个控制点的面片。",
     "エンドキャップには幅または高さが制御点 5 つのパッチが必要です。"),
    ("KeCapHintCylinder",
     "Cylinder caps need a patch that is 9 control points wide or high, such as Patch > Cylinder.",
     "Eine Zylinderkappe braucht einen Patch, der 9 Kontrollpunkte breit oder hoch ist, etwa Patch > Zylinder.",
     "圆柱封盖需要宽度或高度为 9 个控制点的面片,例如 Patch > 圆柱。",
     "円柱キャップには幅または高さが制御点 9 つのパッチ(例: Patch > 円柱)が必要です。"),
    # --- NRC#132: Hinweis vor dem Region-Bau --------------------------------------
    ("KeRegionLeckHinweis",
     "Region build: the camera is inside a brush and the region has no entity in an open space. The player start of the region map lies in solid, so q3map2 will report a leak. Move the camera into the open or include an entity in the region.",
     "Region-Bau: Die Kamera steht in einem Pinsel, und die Region hat keine Entity im Freien. Der Startpunkt der Region-Karte liegt im Festen, q3map2 meldet deshalb ein Leck. Die Kamera ins Freie stellen oder eine Entity in die Region nehmen.",
     "区域编译:摄像机位于笔刷内部,且区域中没有位于开放空间的实体。区域地图的玩家起点位于实体内部,因此 q3map2 会报告泄漏。请把摄像机移到开放空间,或在区域中包含一个实体。",
     "リージョンビルド:カメラがブラシの中にあり、リージョンに空間内のエンティティがありません。リージョンマップのプレイヤー開始点が固体内部になるため、q3map2 はリークを報告します。カメラを空間に移すか、リージョンにエンティティを含めてください。"),
    # --- Stufe 6 Punkt 5: die Angaben der Optimierer-Befunde (Vorlagen aus optimierer_pruefen.cpp) ---
    ("KeOptD01", '%s points in the leak line', '%s Punkte in der Leck-Linie', '泄漏线上有 %s 个点', 'リークライン上の点 %s 個'),
    ("KeOptD02", '%s detail brushes on the leak line', '%s Detail-Pinsel auf der Leck-Linie', '泄漏线上有 %s 个细节笔刷', 'リークライン上のディテールブラシ %s 個'),
    ("KeOptD03", '%s visible faces completely covered by an opaque face of another brush', '%s sichtbare Flächen ganz von einer deckenden Fläche eines anderen Pinsels verdeckt', '%s 个可见面被另一个笔刷的不透明面完全遮挡', '別のブラシの不透明な面に完全に覆われた可視面 %s 個'),
    ("KeOptD04", '%s brushes without a valid shape (fewer than 4 faces or no windings)', '%s Pinsel ohne gültige Form (weniger als 4 Flächen oder keine Umrisse)', '%s 个笔刷没有有效形状(少于 4 个面或没有轮廓)', '有効な形状のないブラシ %s 個(面が 4 未満または輪郭なし)'),
    ("KeOptD05", '%s brushes thinner than 1 unit', '%s Pinsel dünner als 1 Einheit', '%s 个笔刷厚度小于 1 个单位', '厚さ 1 ユニット未満のブラシ %s 個'),
    ("KeOptD06", '%s brushes with vertices off the integer grid (largest deviation %s)', '%s Pinsel mit Ecken neben dem ganzzahligen Raster (größte Abweichung %s)', '%s 个笔刷的顶点不在整数网格上(最大偏差 %s)', '整数グリッドから外れた頂点を持つブラシ %s 個(最大ずれ %s)'),
    ("KeOptD07", '%s detail brushes with hint or areaportal faces', '%s Detail-Pinsel mit Hint- oder Areaportal-Flächen', '%s 个细节笔刷带有 hint 或 areaportal 面', 'hint または areaportal の面を持つディテールブラシ %s 個'),
    ("KeOptD08", '%s areaportal brushes in entities', '%s Areaportal-Pinsel in Entities', '实体中有 %s 个 areaportal 笔刷', 'エンティティ内の areaportal ブラシ %s 個'),
    ("KeOptD09", '%s small, thin or angled structural brushes (suggestion: detail)', '%s kleine, dünne oder schräge strukturelle Pinsel (Vorschlag: detail)', '%s 个小、薄或倾斜的结构笔刷(建议:设为 detail)', '小さい・薄い・斜めの構造ブラシ %s 個(提案:detail)'),
    ("KeOptD10", '%s structural brushes reach less than 32 units across a %s block boundary', '%s strukturelle Pinsel ragen weniger als 32 Einheiten über eine %s-Blockgrenze', '%s 个结构笔刷越过 %s 区块边界不到 32 个单位', '構造ブラシ %s 個が %s ブロック境界を 32 ユニット未満だけ越えています'),
    ("KeOptD11", '%s pairs of opaque brushes overlapping by more than 1 unit', '%s Paare deckender Pinsel, die sich um mehr als 1 Einheit überlappen', '%s 对不透明笔刷重叠超过 1 个单位', '1 ユニット以上重なる不透明ブラシの組 %s 個'),
    ("KeOptD12", '%s brush vertices lie inside an edge of another brush (T-junctions)', '%s Pinselecken liegen auf einer Kante eines anderen Pinsels (T-Verbindungen)', '%s 个笔刷顶点位于另一个笔刷的边上(T 形接点)', '別のブラシの辺上にあるブラシ頂点 %s 個(T ジャンクション)'),
    ("KeOptD13", '%s patches near or over the collision limits (most planes: %s)', '%s Patches nahe an oder über den Kollisionsgrenzen (meiste Ebenen: %s)', '%s 个曲面片接近或超过碰撞限制(最多平面数:%s)', '衝突の上限に近いか超えるパッチ %s 個(最大平面数:%s)'),
    ("KeOptD14", '%s patches subdivide to more than 129 grid columns', '%s Patches werden in mehr als 129 Gitterspalten unterteilt', '%s 个曲面片细分后超过 129 个网格列', '129 列を超えて分割されるパッチ %s 個'),
    ("KeOptD15", '%s misc_model with clip flags', '%s misc_model mit Clip-Flags', '%s 个带裁剪标志的 misc_model', 'クリップフラグ付きの misc_model %s 個'),
    ("KeOptD16", '%s shaders without a shader script or image', '%s Shader ohne Shader-Skript oder Bild', '%s 个着色器没有着色器脚本或图像', 'シェーダースクリプトも画像もないシェーダー %s 個'),
    ("KeOptD17", '%s brush entities without brushes or patches', '%s Brush-Entities ohne Pinsel oder Patches', '%s 个笔刷实体没有笔刷或曲面片', 'ブラシもパッチもないブラシエンティティ %s 個'),
    ("KeOptD18", '%s light grid cells point behind the light grid (index 65535)', '%s Lichtgitterzellen zeigen hinter das Lichtgitter (Index 65535)', '%s 个光照网格单元指向网格之外(索引 65535)', 'ライトグリッドの外を指すグリッドセル %s 個(インデックス 65535)'),
    ("KeOptD19", 'light grid array has %s cells expected by the engine', 'das Lichtgitterfeld hat %s Zellen, die die Engine erwartet', '光照网格数组有 %s 个引擎所需的单元', 'ライトグリッド配列にエンジンが想定するセルが %s 個あります'),
    ("KeOptD20", '%s = %s (%s %)', '%s = %s (%s %)', '%s = %s (%s %)', '%s = %s (%s %)'),
    ("KeOptD21", '%s portals, %s clusters', '%s Portale, %s Cluster', '%s 个入口,%s 个簇', 'ポータル %s 個、クラスター %s 個'),
    ("KeOptD22", '%s world triangles in the PVS of cluster %s', '%s Welt-Dreiecke im PVS von Cluster %s', '%s 个世界三角形位于簇 %s 的 PVS 中', 'ワールド三角形 %s 個がクラスター %s の PVS 内にあります'),
    ("KeOptD23", '%s spawn points or NPC spawners in solid space or outside the map', '%s Startpunkte oder NPC-Spawner im Festen oder außerhalb der Karte', '%s 个出生点或 NPC 生成器位于实体内部或地图之外', '固体内部またはマップ外にあるスポーン地点・NPC スポナー %s 個'),
    ("KeOptD24", '%s misc_model_static whose cull point lies in solid space (the game never draws them), %s without a zoffset within 1024 units - move them', '%s misc_model_static mit Prüfpunkt im Festen (das Spiel zeichnet sie nie), %s ohne zoffset innerhalb von 1024 Einheiten - verschieben', '%s 个 misc_model_static 的剔除点位于实体内部(游戏永远不会绘制它们),其中 %s 个在 1024 单位内找不到 zoffset——请移动它们', 'カリング点が固体内部にある misc_model_static %s 個(ゲームは描画しません)、うち %s 個は 1024 ユニット以内に zoffset がありません。移動してください'),
    ("KeOptD25", '%s misc_model_static whose cull point lies in solid space (the game never draws them)', '%s misc_model_static mit Prüfpunkt im Festen (das Spiel zeichnet sie nie)', '%s 个 misc_model_static 的剔除点位于实体内部(游戏永远不会绘制它们)', 'カリング点が固体内部にある misc_model_static %s 個(ゲームは描画しません)'),
    ("KeOptD26", '%s brush entities in groups with identical keys (no targetname)', '%s Brush-Entities in Gruppen mit gleichen Schlüsseln (ohne targetname)', '%s 个笔刷实体位于键值相同的组中(无 targetname)', '同じキーを持つグループ内のブラシエンティティ %s 個(targetname なし)'),
    ("KeOptD27", '%s faces with textures/common/origin (no surfaceparm origin in this game)', '%s Flächen mit textures/common/origin (in diesem Spiel ohne surfaceparm origin)', '%s 个面使用 textures/common/origin(此游戏中没有 surfaceparm origin)', 'textures/common/origin を使う面 %s 個(このゲームには surfaceparm origin がありません)'),
    ("KeOptD28", '%s brushes with an edge of %s units or more', '%s Pinsel mit einer Kante von %s Einheiten oder mehr', '%s 个笔刷有达到或超过 %s 个单位的边', 'ブラシ %s 個に %s ユニット以上の辺があります'),
    ("KeOptD29", '%s surface light faces whose backsplash lies behind a solid brush', '%s Flächenlicht-Flächen, deren Backsplash hinter einem festen Pinsel liegt', '%s 个面光源的 backsplash 位于实体笔刷后面', 'backsplash が固体ブラシの向こうにある面光源 %s 個'),
    ("KeOptD30", '%s build menu commands with switches this game cannot use (the build menu refuses them)', '%s Befehle im Build-Menü mit Schaltern, die dieses Spiel nicht kann (das Build-Menü verweigert sie)', '%s 条编译菜单命令带有此游戏无法使用的开关(编译菜单会拒绝执行)', 'このゲームで使えないスイッチを含むビルドメニューのコマンド %s 個(ビルドメニューは実行しません)'),
    ("KeOptD31", "%s brushes or patches with q3map_lightmapSize: Jedi Outcast cannot load external lightmaps",
     "%s Pinsel oder Patches mit q3map_lightmapSize: Jedi Outcast kann externe Lightmaps nicht laden",
     "%s 个笔刷或曲面片使用 q3map_lightmapSize:Jedi Outcast 无法加载外部光照贴图",
     "q3map_lightmapSize を使うブラシ・パッチ %s 個:Jedi Outcast は外部ライトマップを読み込めません"),
    ("KeOptD32", "%s brushes or patches with q3map_lightmapSize next to light styles: external lightmaps lose the style stages",
     "%s Pinsel oder Patches mit q3map_lightmapSize neben Lichtstilen: externe Lightmaps verlieren die Stil-Stufen",
     "%s 个笔刷或曲面片在光照样式旁使用 q3map_lightmapSize:外部光照贴图会丢失样式阶段",
     "ライトスタイルのそばで q3map_lightmapSize を使うブラシ・パッチ %s 個:外部ライトマップではスタイルの段階が失われます"),
    # --- Stufe 6: Optimierer-Pruefung externe Lightmaps per Shader (NRC#48 / NRC#98) ---
    ("KeOptPrLightmapExtern", "External lightmaps from shaders", "Externe Lightmaps durch Shader", "着色器产生的外部光照贴图", "シェーダーによる外部ライトマップ"),
    ("KeOptPrLightmapExternE",
     "A shader with q3map_lightmapSize other than 128 gets an external lightmap. In Jedi Academy and Movie Duels light styles then lose their lightmap stages on these surfaces (NRC#48, a limit of q3map2 for RBSP); Jedi Outcast cannot load external lightmaps at all (NRC#98). Remove q3map_lightmapSize from the shader, or do not use light styles (switchable or flickering lights) near these surfaces.",
     "Ein Shader mit q3map_lightmapSize ungleich 128 bekommt eine externe Lightmap. In Jedi Academy und Movie Duels verlieren Lichtstile auf diesen Flächen dann ihre Lightmap-Stufen (NRC#48, eine Grenze von q3map2 bei RBSP); Jedi Outcast kann externe Lightmaps überhaupt nicht laden (NRC#98). q3map_lightmapSize aus dem Shader nehmen oder bei diesen Flächen keine Lichtstile (schaltbare oder flackernde Lichter) verwenden.",
     "q3map_lightmapSize 不为 128 的着色器会得到外部光照贴图。在 Jedi Academy 和 Movie Duels 中,这些面上的光照样式会因此失去光照贴图阶段(NRC#48,q3map2 在 RBSP 下的限制);Jedi Outcast 则完全无法加载外部光照贴图(NRC#98)。请从着色器中去掉 q3map_lightmapSize,或者不要在这些面附近使用光照样式(可开关或闪烁的灯光)。",
     "q3map_lightmapSize が 128 以外のシェーダーには外部ライトマップが割り当てられます。Jedi Academy と Movie Duels では、これらの面でライトスタイルのライトマップ段階が失われます(NRC#48、RBSP における q3map2 の制限)。Jedi Outcast は外部ライトマップをまったく読み込めません(NRC#98)。シェーダーから q3map_lightmapSize を外すか、これらの面の近くではライトスタイル(切り替え式や点滅するライト)を使わないでください。"),
    # --- JKHub: Lichtstile nach Entity-Aenderung (Update entities only) ---
    ("KeBmSchaltlichter",
     "The switchable lights (lights with a targetname) are not the same as in the original map. Their light is baked into the lightmaps per light style, and the styles are assigned in the order of these lights - updating only the entities would switch the wrong light or none. Build and light the map completely instead.",
     "Die schaltbaren Lichter (Lichter mit targetname) sind nicht mehr dieselben wie in der Originalkarte. Ihr Licht steckt je Lichtstil in den Lightmaps, und die Stile werden in der Reihenfolge dieser Lichter vergeben - nur die Entities zu erneuern schaltete das falsche oder gar kein Licht. Die Karte stattdessen vollständig bauen und belichten.",
     "可开关的灯光(带 targetname 的灯光)与原始地图中的不再相同。它们的光照按光照样式烘焙在光照贴图中,而样式是按这些灯光的顺序分配的——只更新实体会开关错误的灯光或根本没有灯光。请改为完整编译并计算光照。",
     "切り替え可能なライト(targetname を持つライト)がオリジナルのマップと同じではありません。その光はライトスタイルごとにライトマップへ焼き込まれ、スタイルはこれらのライトの順に割り当てられます。エンティティだけを更新すると、違うライトが切り替わるか、まったく切り替わりません。代わりにマップを完全にビルドしてライティングしてください。"),
]
