"""Karte: Tempo und Echtzeit-Licht (Stufe 7).

Echtzeit-Licht in der Kamera (Vorschau auf der Grafikkarte), Schnell-Backen im
Hintergrund (echte Lightmaps aus bhed_q3map2) und was schneller geworden ist -
gemessen vorher und nachher, mit gleichem Ergebnis.
"""
from inhalt_basis import T

import tempo7


def _z(text):
    return T(text, text, text, text)   # Zahlen und Kennungen: in jeder Sprache gleich


_KOPF = [T("Step", "Schritt", "步骤", "処理"),
         T("Map", "Karte", "地图", "マップ"),
         T("NetRadiant-Custom", "NetRadiant-Custom", "NetRadiant-Custom", "NetRadiant-Custom"),
         T("before (Stage 6)", "vorher (Stufe 6)", "之前(第 6 阶段)", "以前(ステージ 6)"),
         T("now", "jetzt", "现在", "現在")]

_EDITOR_KOPF = [T("Step", "Schritt", "步骤", "処理"),
                T("Map", "Karte", "地图", "マップ"),
                T("before (Stage 6)", "vorher (Stufe 6)", "之前(第 6 阶段)", "以前(ステージ 6)"),
                T("now", "jetzt", "现在", "現在")]

KARTE_TEMPO = {
    "id": "karte_tempo",
    "gruppe": "karte",
    "titel": T("Map editor: speed and real-time light", "Karten-Editor: Tempo und Echtzeit-Licht",
               "地图编辑器:速度与实时光照", "マップエディター:速度とリアルタイムライト"),
    "kurz": T(
        "See the light while you build: {{KeLichtMenue}} lights the camera on the graphics card, {{KeBackenMenue}} builds the map in the background and shows the real lightmaps. Building and editing got faster - with the same result as before.",
        "Licht sehen, während man baut: {{KeLichtMenue}} beleuchtet die Kamera auf der Grafikkarte, {{KeBackenMenue}} baut die Karte im Hintergrund und zeigt die echten Lightmaps. Bauen und Bearbeiten wurden schneller - mit demselben Ergebnis wie vorher.",
        "边搭建边看光照:{{KeLichtMenue}} 在显卡上照亮摄像机视图,{{KeBackenMenue}} 在后台编译地图并显示真实的光照贴图。编译和编辑都变快了——结果与之前完全相同。",
        "作りながら光を確認できます。{{KeLichtMenue}} はグラフィックカードでカメラを照らし、{{KeBackenMenue}} はバックグラウンドでマップをビルドして本物のライトマップを表示します。ビルドと編集は速くなりましたが、結果は以前と同じです。"),
    "bloecke": [
        ("h2", "echtlicht", T("Real-time light", "Echtzeit-Licht", "实时光照", "リアルタイムライト")),
        ("img", "h_karte7_menue", T(
            "{{KeLBuild}}: {{KeLichtMenue}} and {{KeBackenMenue}} at the end of the menu (not in NetRadiant-Custom).",
            "{{KeLBuild}}: {{KeLichtMenue}} und {{KeBackenMenue}} am Ende des Menüs (nicht in NetRadiant-Custom).",
            "{{KeLBuild}}:菜单末尾的 {{KeLichtMenue}} 和 {{KeBackenMenue}}(NetRadiant-Custom 中没有)。",
            "{{KeLBuild}}:メニューの最後にある {{KeLichtMenue}} と {{KeBackenMenue}}(NetRadiant-Custom にはありません)。")),
        ("steps", [
            T("Switch on **{{KeLBuild}} > {{KeLichtMenue}}**. The camera (render mode textured) is lit by the lights of the map - point lights, spotlights, the sun of the sky shader (`q3map_sun`) and surface lights (`q3map_surfacelight`) - with the formulas of q3map2 -light and with shadows.",
              "Schalte **{{KeLBuild}} > {{KeLichtMenue}}** ein. Die Kamera (Darstellung texturiert) wird von den Lichtern der Karte beleuchtet - Punktlichter, Spotlichter, die Sonne des Himmels-Shaders (`q3map_sun`) und Flächenlicht (`q3map_surfacelight`) - mit den Formeln von q3map2 -light und mit Schatten.",
              "打开 **{{KeLBuild}} > {{KeLichtMenue}}**。摄像机(纹理渲染模式)会被地图中的光源照亮——点光源、聚光灯、天空着色器的太阳(`q3map_sun`)和面光源(`q3map_surfacelight`)——使用 q3map2 -light 的公式,并带有阴影。",
              "**{{KeLBuild}} > {{KeLichtMenue}}** をオンにします。カメラ(テクスチャ表示)がマップのライト、つまりポイントライト・スポットライト・空シェーダーの太陽(`q3map_sun`)・面光源(`q3map_surfacelight`)で、q3map2 -light の式を使い影付きで照らされます。"),
            T("Move a light or change its keys: the camera follows at once. Up to 512 lights light each frame - every light whose reach touches the picture, the strongest first, also far away; the 128 most important cast shadows, the rest light without shadow.",
              "Verschiebe ein Licht oder ändere seine Schlüssel: die Kamera folgt sofort. Bis zu 512 Lichter beleuchten jedes Bild - jedes Licht, dessen Reichweite das Bild berührt, die stärksten zuerst, auch in der Ferne; die 128 wichtigsten werfen Schatten, die übrigen leuchten ohne Schatten.",
              "移动光源或修改其键值,摄像机会立即更新。每一帧最多由 512 个光源照亮——凡是照射范围触及画面的光源都算,包括远处的,最强的优先;其中最重要的 128 个投射阴影,其余不带阴影。",
              "ライトを動かしたりキーを変えたりすると、カメラはすぐに追従します。1 フレームあたり最大 512 個のライトが照らします。画面に届くライトはすべて対象で、遠くのものも含め強いものから選ばれます。最も重要な 128 個が影を落とし、残りは影なしで照らします。"),
        ]),
        ("img", "h_karte7_licht", T(
            "Real-time light in a closed room with one light and a wall. Top right the camera says that this is a preview.",
            "Echtzeit-Licht in einem geschlossenen Raum mit einem Licht und einer Wand. Oben rechts sagt die Kamera, dass es eine Vorschau ist.",
            "在一个有一盏灯和一面墙的封闭房间中的实时光照。摄像机右上角注明这是预览。",
            "ライト 1 個と壁のある閉じた部屋でのリアルタイムライト。カメラの右上に、これがプレビューであることが表示されます。")),
        ("note", T(
            "The real-time light is a **preview** and is never saved as a result: no bounce light, no filter, no supersampling, shadows from shadow maps instead of rays. The final light always comes from bhed_q3map2 - {{KeBackenMenue}} or a build from the {{KeLBuild}} menu.",
            "Das Echtzeit-Licht ist eine **Vorschau** und wird nie als Ergebnis gespeichert: kein Bounce-Licht, kein Filter, kein Supersampling, Schatten aus Schattenkarten statt aus Strahlen. Das endgültige Licht kommt immer aus bhed_q3map2 - {{KeBackenMenue}} oder ein Bau aus dem Menü {{KeLBuild}}.",
            "实时光照只是**预览**,永远不会作为结果保存:没有反弹光、没有过滤、没有超采样,阴影来自阴影贴图而不是光线。最终光照始终来自 bhed_q3map2——{{KeBackenMenue}} 或 {{KeLBuild}} 菜单中的编译。",
            "リアルタイムライトは**プレビュー**で、結果として保存されることはありません。バウンス光・フィルター・スーパーサンプリングはなく、影はレイではなくシャドウマップで作られます。最終的なライトは常に bhed_q3map2 から作られます({{KeBackenMenue}} または {{KeLBuild}} メニューのビルド)。")),
        ("h2", "wiespiel", T("Real-time light like in the game", "Echtzeit-Licht wie im Spiel", "与游戏一致的实时光照", "ゲームと同じリアルタイムライト")),
        ("p", T(
            "The preview follows the rules of q3map2 -light -fast, so that the picture matches the baked lightmaps (a real JKA mapper's test map was the yardstick):",
            "Die Vorschau folgt den Regeln von q3map2 -light -fast, damit das Bild zu den gebackenen Lightmaps passt (Maßstab war die Testkarte eines erfahrenen JKA-Mappers):",
            "预览遵循 q3map2 -light -fast 的规则,使画面与烘焙出的光照贴图一致(以一位资深 JKA 地图作者的测试地图为准):",
            "プレビューは q3map2 -light -fast の規則に従うため、画面は焼き込んだライトマップと一致します(経験豊富な JKA マッパーのテストマップを基準にしました):")),
        ("list", [
            T("**Sky and sun:** the ray to the sun ends at the sky, as in q3map2 - the sky brush itself (caulk on its other sides, a lid above) no longer shadows the sun. `sun`, `q3map_sun`, `q3map_sunExt` and `q3map_skyLight` (many weak suns over the sky) all count; the sun has two shadow cascades (whole map, around the camera).",
              "**Himmel und Sonne:** der Strahl zur Sonne endet am Himmel wie in q3map2 - der Himmels-Brush selbst (caulk auf den anderen Seiten, ein Deckel darüber) wirft keinen Schatten mehr auf die Sonne. `sun`, `q3map_sun`, `q3map_sunExt` und `q3map_skyLight` (viele schwache Sonnen über den Himmel) zählen alle; die Sonne hat zwei Schattenstufen (ganze Karte, um die Kamera).",
              "**天空和太阳:**与 q3map2 一样,射向太阳的光线在天空处结束——天空画刷本身(其他面为 caulk,上方再加一层盖子)不再遮挡阳光。`sun`、`q3map_sun`、`q3map_sunExt` 和 `q3map_skyLight`(天空上的许多弱太阳)都会计入;太阳有两级阴影(整张地图、摄像机周围)。",
              "**空と太陽:**q3map2 と同じく、太陽への光線は空で終わります。空のブラシ自体(ほかの面は caulk、上にふた)はもう太陽の影を落としません。`sun`、`q3map_sun`、`q3map_sunExt`、`q3map_skyLight`(空全体に広がる多数の弱い太陽)がすべて有効で、太陽には 2 段階の影(マップ全体、カメラ周辺)があります。"),
            T("**Surface light:** `q3map_surfacelight` per triangle, divided by `q3map_lightSubdivide`, with the exact form factor and the back splash (`q3map_backsplash`); the colour from `q3map_lightRGB` or `q3map_lightImage`.",
              "**Flächenlicht:** `q3map_surfacelight` je Dreieck, unterteilt nach `q3map_lightSubdivide`, mit dem genauen Formfaktor und dem Rückschein (`q3map_backsplash`); die Farbe aus `q3map_lightRGB` oder `q3map_lightImage`.",
              "**面光源:**`q3map_surfacelight` 按三角形计算,按 `q3map_lightSubdivide` 细分,使用精确的形状因子和背光(`q3map_backsplash`);颜色取自 `q3map_lightRGB` 或 `q3map_lightImage`。",
              "**面光源:**`q3map_surfacelight` を三角形ごとに `q3map_lightSubdivide` で分割し、正確な形態係数とバックスプラッシュ(`q3map_backsplash`)で計算します。色は `q3map_lightRGB` か `q3map_lightImage` から取ります。"),
            T("**What casts shadows:** only what casts shadows in q3map2 - world brushes and func_group (other entities only with `_castShadows`), no nodraw surfaces, transparent shaders only with `alphashadow`, patches only with **{{KeLBuild}} > {{KePatchSchatten}}** (also passed to the quick bake). Back faces go into the shadow maps, so outside light no longer leaks inside.",
              "**Was Schatten wirft:** nur, was in q3map2 Schatten wirft - Welt-Brushes und func_group (andere Entities nur mit `_castShadows`), keine nodraw-Flächen, durchsichtige Shader nur mit `alphashadow`, Patches nur mit **{{KeLBuild}} > {{KePatchSchatten}}** (geht auch an das Schnell-Backen). Rückseiten kommen mit in die Schattenkarten, das Licht von draußen leckt nicht mehr nach innen.",
              "**什么会投射阴影:**只有在 q3map2 中投射阴影的东西——世界画刷和 func_group(其他实体只有设置 `_castShadows` 时),nodraw 表面不投射,透明着色器只有带 `alphashadow` 时才投射,面片只有开启 **{{KeLBuild}} > {{KePatchSchatten}}** 时才投射(也会传给快速烘焙)。背面也会写入阴影贴图,室外的光不再漏进室内。",
              "**影を落とすもの:**q3map2 で影を落とすものだけです。ワールドのブラシと func_group(ほかのエンティティは `_castShadows` があるときだけ)、nodraw 面は落とさず、透明なシェーダーは `alphashadow` があるときだけ、パッチは **{{KeLBuild}} > {{KePatchSchatten}}** がオンのときだけ(クイックベイクにも渡されます)。裏面も影のマップに入るので、外の光が中に漏れなくなりました。"),
            T("**Models and patches:** misc_model entities cast shadows like in q3map2 (in the world always, otherwise only with `_castShadows`; `_castShadows 0` switches it off) and their surfaces with `q3map_surfacelight` shine per triangle; patches shine over the same tessellation as in -light. Brush faces shine over the triangles of `-meta`; `q3map_sunExt` with deviance and samples and lights with `_samples` get their jittered copies with q3map2's own random sequence.",
              "**Modelle und Patches:** misc_model werfen Schatten wie in q3map2 (in der Welt immer, sonst nur mit `_castShadows`; `_castShadows 0` schaltet es ab), und ihre Flächen mit `q3map_surfacelight` leuchten je Dreieck; Patches leuchten über dieselbe Unterteilung wie in -light. Brush-Flächen leuchten über die Dreiecke von `-meta`; `q3map_sunExt` mit Abweichung und Proben und Lichter mit `_samples` bekommen ihre verschobenen Kopien mit der Zufallsfolge von q3map2.",
              "**模型和面片:**misc_model 实体像 q3map2 一样投射阴影(在世界中总是投射,否则只在设置 `_castShadows` 时;`_castShadows 0` 可关闭),带 `q3map_surfacelight` 的表面按三角形发光;面片按与 -light 相同的细分发光。画刷面按 `-meta` 的三角形发光;带偏差和采样数的 `q3map_sunExt` 以及带 `_samples` 的灯光,会用 q3map2 自己的随机序列得到抖动副本。",
              "**モデルとパッチ:**misc_model は q3map2 と同じく影を落とします(ワールドでは常に、ほかは `_castShadows` があるときだけ。`_castShadows 0` でオフ)。`q3map_surfacelight` のある面は三角形ごとに光ります。パッチは -light と同じ分割で光ります。ブラシの面は `-meta` の三角形で光り、偏差とサンプル数のある `q3map_sunExt` と `_samples` のあるライトは、q3map2 自身の乱数列でずらしたコピーを持ちます。"),
            T("**Like in the game:** with real-time light (and with the quick bake shown) nodraw surfaces such as caulk disappear, and transparent shaders are drawn with the alphaFunc and blendFunc of their first stage.",
              "**Wie im Spiel:** mit Echtzeit-Licht (und beim gezeigten Schnell-Backen) verschwinden nodraw-Flächen wie caulk, und durchsichtige Shader werden mit alphaFunc und blendFunc ihrer ersten Stufe gezeichnet.",
              "**与游戏一致:**开启实时光照(以及显示快速烘焙结果)时,caulk 等 nodraw 表面会消失,透明着色器按其第一层的 alphaFunc 和 blendFunc 绘制。",
              "**ゲームと同じ:**リアルタイムライト(とクイックベイクの表示)では caulk などの nodraw 面が消え、透明なシェーダーは最初のステージの alphaFunc と blendFunc で描かれます。"),
        ]),
        ("p", T("**Light through patches:** patches cast shadows only with **{{KeLBuild}} > {{KePatchSchatten}}** - as in q3map2, where they cast shadows only with `-patchshadows`. The switch is saved in the map (worldspawn `_bhed_patchshadows`) and applies to the real-time light, the quick bake and every full build: the light stage of any Build menu entry gets `-patchshadows`. Off, the default entries behave as in NetRadiant: only [Final] has `-patchshadows`, the others let the light through patches, and the build window says so. The light grid ignores patches in any case (q3map2), so models under a patch stay lit in the game.",
                "**Licht durch Patches:** Patches werfen nur mit **{{KeLBuild}} > {{KePatchSchatten}}** Schatten - wie in q3map2, wo sie nur mit `-patchshadows` Schatten werfen. Der Schalter wird in der Karte gespeichert (worldspawn `_bhed_patchshadows`) und gilt für Echtzeit-Licht, Schnell-Backen und jeden vollen Bau: der Licht-Schritt jedes Eintrags im Build-Menü bekommt `-patchshadows`. Aus verhalten sich die Vorgabe-Einträge wie in NetRadiant: nur [Final] hat `-patchshadows`, die anderen lassen das Licht durch Patches, und das Bau-Fenster sagt es. Das Lichtgitter übergeht Patches immer (q3map2), Modelle unter einem Patch bleiben im Spiel also hell.",
                "**光线穿过面片:**面片只有开启 **{{KeLBuild}} > {{KePatchSchatten}}** 时才投射阴影——与 q3map2 相同,那里面片只有加 `-patchshadows` 才投射阴影。此开关保存在地图中(worldspawn `_bhed_patchshadows`),适用于实时光照、快速烘焙和每次完整编译:Build 菜单中任何条目的光照步骤都会加上 `-patchshadows`。关闭时,默认条目与 NetRadiant 相同:只有 [Final] 带 `-patchshadows`,其他条目让光线穿过面片,编译窗口会给出提示。光照网格始终忽略面片(q3map2),因此游戏中面片下的模型仍然是亮的。",
                "**パッチを通る光:**パッチは **{{KeLBuild}} > {{KePatchSchatten}}** がオンのときだけ影を落とします。q3map2 でも `-patchshadows` があるときだけ影を落とすのと同じです。このスイッチはマップに保存され (worldspawn `_bhed_patchshadows`)、リアルタイムライト、クイックベイク、すべてのフルビルドに使われます: Build メニューのどの項目でもライトの段階に `-patchshadows` が付きます。オフのとき既定の項目は NetRadiant と同じで、`-patchshadows` があるのは [Final] だけです。ほかの項目では光がパッチを通り抜け、ビルドウィンドウがそれを知らせます。ライトグリッドは常にパッチを無視するため (q3map2)、ゲーム内でパッチの下のモデルは明るいままです。")),
        ("img", "h_patchlicht_aus", T(
            "Built with [Test] Quick and Crude, {{KePatchSchatten}} off: the light goes through the curved patch, as in q3map2.",
            "Gebaut mit [Test] Quick and Crude, {{KePatchSchatten}} aus: das Licht geht durch den gebogenen Patch, wie in q3map2.",
            "用 [Test] Quick and Crude 编译,{{KePatchSchatten}} 关闭:光线穿过弯曲的面片,与 q3map2 相同。",
            "[Test] Quick and Crude でビルド、{{KePatchSchatten}} はオフ: q3map2 と同じく光が曲面パッチを通り抜けます。")),
        ("img", "h_patchlicht_an", T(
            "The same with {{KePatchSchatten}} on: the light stage gets -patchshadows, and the patch casts its shadow.",
            "Dasselbe mit {{KePatchSchatten}} an: der Licht-Schritt bekommt -patchshadows, und der Patch wirft seinen Schatten.",
            "同样的场景,{{KePatchSchatten}} 开启:光照步骤加上 -patchshadows,面片投下阴影。",
            "同じ場面で {{KePatchSchatten}} をオン: ライトの段階に -patchshadows が付き、パッチが影を落とします。")),
        ("img", "h_karte9c_himmel", T(
            "A room whose ceiling is a sky brush (caulk on all other sides, a caulk lid above): the sun of the sky shader shines through the sky into the room, as after the bake.",
            "Ein Raum, dessen Decke ein Himmels-Brush ist (caulk auf allen anderen Seiten, ein caulk-Deckel darüber): die Sonne des Himmels-Shaders scheint durch den Himmel in den Raum, wie nach dem Backen.",
            "天花板为天空画刷的房间(其他各面为 caulk,上方再加 caulk 盖子):天空着色器的太阳穿过天空照进房间,与烘焙后一致。",
            "天井が空のブラシ(ほかの面はすべて caulk、上に caulk のふた)の部屋:空シェーダーの太陽が空を通って部屋に差し込みます。ベイク後と同じです。")),
        ("note", T(
            "Measured against the quick bake at fixed camera points (md_ch_ben, mp/duel1, md_eitd_jedi with sky light, a large tester map): the mean brightness difference fell from 2-95 to 0.2-2 of 255. What remains is mostly the lightmap resolution (one luxel per 16 units) and bounce light, which the preview does not have.",
            "Gemessen gegen das Schnell-Backen an festen Kamerapunkten (md_ch_ben, mp/duel1, md_eitd_jedi mit Himmelslicht, eine große Testerkarte): die mittlere Helligkeitsabweichung sank von 2-95 auf 0,2-2 von 255. Was bleibt, ist vor allem die Auflösung der Lightmaps (ein Luxel je 16 Einheiten) und das Bounce-Licht, das die Vorschau nicht hat.",
            "在固定摄像机位置与快速烘焙比较(md_ch_ben、mp/duel1、带天空光的 md_eitd_jedi、一张大型测试地图):平均亮度偏差从 2-95 降到 0.2-2(满值 255)。剩下的差别主要来自光照贴图的分辨率(每 16 单位一个 luxel)以及预览没有的反弹光。",
            "固定したカメラ位置でクイックベイクと比較しました(md_ch_ben、mp/duel1、空の光のある md_eitd_jedi、大きなテスターのマップ)。明るさの平均の差は 255 のうち 2-95 から 0.2-2 に下がりました。残る差は主にライトマップの解像度(16 ユニットに 1 ルクセル)と、プレビューにないバウンス光です。")),
        ("h2", "gebautwiespiel", T("Built map as in the game", "Gebaute Karte wie im Spiel", "已构建的地图如游戏中显示", "ビルドしたマップをゲームと同じ表示に")),
        ("p", T("**{{KeLBuild}} > {{KeSpielbildMenue}}** (on by default) shows the result of the build as the game shows it: after {{KeBackenMenue}}, or with a template .bsp, the camera draws the built map through the same renderer as the preview in the Mission tab - every shader stage with tcGen, rgbGen, tcMod and blend, the sky box, glow, fog, the game's gamma, and only what the game draws from the camera's place (PVS). The light comes from the lightmaps exactly as in the game (lightmap times 1; the editor used to double it). Selection, handles and lines stay on top as before.",
            '**{{KeLBuild}} > {{KeSpielbildMenue}}** (ab Werk an) zeigt das Ergebnis des Baus so, wie das Spiel es zeigt: nach {{KeBackenMenue}} oder mit einer Vorlage-.bsp zeichnet die Kamera die gebaute Karte durch denselben Zeichner wie die Vorschau im Mission-Tab - alle Stufen der Shader mit tcGen, rgbGen, tcMod und Blend, die Himmelsbox, Glühen, Nebel, das Gamma des Spiels und nur, was das Spiel vom Ort der Kamera aus zeichnet (PVS). Das Licht kommt aus den Lightmaps genau wie im Spiel (Lightmap mal 1; der Editor hatte es bisher verdoppelt). Auswahl, Griffe und Linien bleiben darüber wie bisher.',
            '**{{KeLBuild}} > {{KeSpielbildMenue}}**(默认开启)按游戏中的样子显示构建结果:{{KeBackenMenue}} 之后或使用模板 .bsp 时,摄像机通过与任务选项卡预览相同的渲染器绘制已构建的地图——所有着色器阶段(含 tcGen、rgbGen、tcMod 和混合)、天空盒、辉光、雾、游戏的伽马,并且只绘制游戏从摄像机位置会绘制的部分(PVS)。光照与游戏中完全相同地来自光照贴图(光照贴图乘以 1;编辑器以前会加倍)。选择、手柄和线条仍像以前一样显示在上方。',
            '**{{KeLBuild}} > {{KeSpielbildMenue}}**(初期設定でオン)は、ビルドの結果をゲームと同じように表示します。{{KeBackenMenue}} の後、またはテンプレート .bsp があるとき、カメラはミッションタブのプレビューと同じレンダラーでビルド済みマップを描きます。tcGen、rgbGen、tcMod、ブレンドを含むシェーダーの全ステージ、スカイボックス、グロー、フォグ、ゲームのガンマ、そしてゲームがカメラの位置から描く部分だけ(PVS)です。光はゲームとまったく同じくライトマップから来ます(ライトマップ×1。以前のエディターは 2 倍にしていました)。選択、ハンドル、線はこれまでどおり上に表示されます。')),
        ("note", T('Measured against the game (Movie Duels, the same maps and cameras as for the Mission tab preview): with the original .bsp as template, the floors of twelve test maps match with factor 1.00 to 1.02 (one map 1.10 to 1.20 in one corner). Map models (misc_model_static), the sky portal (misc_skyportal) and weather come through the same code as in the preview (the ship on duel_deathstar matches the game with factor 1.00); curved surfaces (patches) are divided as in the game and drop rows and columns with distance. Doors, platforms and other brush entities stand where the game puts them when the map loads (the closed blast doors on duel_deathstar match with factor 1.00), areas behind closed doors stay dark as in the game, and effects (fx_runner) and surface sprites come through the same code as in the preview. Not part of this picture: figures. The real-time light stays a preview; its tooltip says what it cannot show.',
            'Gemessen gegen das Spiel (Movie Duels, dieselben Karten und Kameras wie für die Vorschau im Mission-Tab): mit der Original-.bsp als Vorlage stimmen die Böden von zwölf Testkarten mit Faktor 1,00 bis 1,02 (eine Karte in einer Ecke 1,10 bis 1,20). Kartenmodelle (misc_model_static), das Himmelsportal (misc_skyportal) und Wetter kommen über denselben Code wie in der Vorschau (das Schiff auf duel_deathstar stimmt mit Faktor 1,00); gekrümmte Flächen (Patches) werden wie im Spiel unterteilt und lassen mit dem Abstand Zeilen und Spalten weg. Türen, Plattformen und andere Brush-Entities stehen, wo das Spiel sie beim Laden der Karte hinstellt (die geschlossenen Schotten auf duel_deathstar stimmen mit Faktor 1,00), Gebiete hinter geschlossenen Türen bleiben dunkel wie im Spiel, und Effekte (fx_runner) und Surface-Sprites kommen über denselben Code wie in der Vorschau. Nicht in diesem Bild: Figuren. Das Echtzeit-Licht bleibt eine Vorschau; sein Tooltip sagt, was es nicht zeigen kann.',
            '与游戏对比测量(Movie Duels,与任务选项卡预览相同的地图和摄像机):以原始 .bsp 作为模板时,十二张测试地图的地面以 1.00 到 1.02 的系数吻合(一张地图的一角为 1.10 到 1.20)。地图模型(misc_model_static)、天空传送门(misc_skyportal)和天气通过与预览相同的代码显示(duel_deathstar 上的飞船以系数 1.00 吻合);曲面(补丁)按游戏的方式细分,并随距离省略行和列。门、平台和其他画刷实体位于游戏在加载地图时放置它们的位置(duel_deathstar 上关闭的防爆门以系数 1.00 吻合),关闭的门后面的区域与游戏中一样保持黑暗,特效(fx_runner)和表面精灵通过与预览相同的代码显示。此画面中没有:角色。实时光照仍是预览;它的提示说明了它无法显示的内容。',
            'ゲームと比較して測定しました(Movie Duels、ミッションタブのプレビューと同じマップとカメラ)。元の .bsp をテンプレートにすると、12 個のテストマップの床は係数 1.00 から 1.02 で一致します(1 つのマップの片隅だけ 1.10 から 1.20)。マップモデル(misc_model_static)、スカイポータル(misc_skyportal)、天候はプレビューと同じコードで描かれます(duel_deathstar の船は係数 1.00 で一致)。曲面(パッチ)はゲームと同じように分割され、距離に応じて行と列を省きます。ドア、プラットフォームなどのブラシエンティティは、マップ読み込み時にゲームが置く位置にあります(duel_deathstar の閉じた隔壁扉は係数 1.00 で一致)。閉じたドアの向こうのエリアはゲームと同じく暗いままで、エフェクト(fx_runner)とサーフェススプライトはプレビューと同じコードで描かれます。この画像に含まれないもの:キャラクター。リアルタイムライトはプレビューのままで、ツールチップに表示できないものが書かれています。')),
        ("h2", "nebel", T("Fog, effects and sky portal", "Nebel, Effekte und Skyportal", "雾、特效和天空传送门", "フォグ、エフェクト、スカイポータル")),
        ("list", [
            T("**Fog:** brushes with a `fogParms` shader and the global fog of worldspawn (`_fog`) are drawn as the game draws them (RB_FogPass with R_FogFactor): colour and distance to opaque from the shader, the fog surface from the visible side of the brush (or `q3map_fogDir`), deeper below it is denser.",
              "**Nebel:** Brushes mit einem `fogParms`-Shader und der globale Nebel von worldspawn (`_fog`) werden gezeichnet wie im Spiel (RB_FogPass mit R_FogFactor): Farbe und Sichtweite aus dem Shader, die Nebeloberfläche aus der sichtbaren Seite des Brushes (oder `q3map_fogDir`), tiefer darunter wird er dichter.",
              "**雾:**带 `fogParms` 着色器的画刷和 worldspawn 的全局雾(`_fog`)按游戏方式绘制(RB_FogPass 与 R_FogFactor):颜色和不透明距离取自着色器,雾面取自画刷的可见面(或 `q3map_fogDir`),越往下越浓。",
              "**フォグ:**`fogParms` シェーダーのブラシと worldspawn のグローバルフォグ(`_fog`)は、ゲームと同じように描かれます(RB_FogPass と R_FogFactor)。色と不透明になる距離はシェーダーから、フォグの面はブラシの見える面(または `q3map_fogDir`)から取り、その下へ深いほど濃くなります。"),
            T("**Effects:** fx_runner entities play their effect with the effect system of the cutscene preview - repeat interval `delay`/`random`, direction to the `target` (or `angle`, otherwise up); START_OFF and ONESHOT wait for a use. **{{KeLBuild}} > {{KeEchtEffekte}}** switches them off. The editor has no .bsp, so particles do not bounce off walls.",
              "**Effekte:** fx_runner spielen ihren Effekt mit dem Effektsystem der Zwischensequenz-Vorschau - Wiederholung nach `delay`/`random`, Richtung zum `target` (oder `angle`, sonst nach oben); START_OFF und ONESHOT warten auf ein use. **{{KeLBuild}} > {{KeEchtEffekte}}** schaltet sie ab. Der Editor hat keine .bsp, Teilchen prallen deshalb nicht an Wänden ab.",
              "**特效:**fx_runner 实体用过场动画预览的特效系统播放特效——按 `delay`/`random` 重复,方向指向 `target`(或 `angle`,否则向上);START_OFF 和 ONESHOT 会等待 use。**{{KeLBuild}} > {{KeEchtEffekte}}** 可关闭。编辑器没有 .bsp,所以粒子不会在墙上反弹。",
              "**エフェクト:**fx_runner はカットシーンプレビューのエフェクトシステムでエフェクトを再生します。`delay`/`random` で繰り返し、向きは `target`(または `angle`、なければ上)。START_OFF と ONESHOT は use を待ちます。**{{KeLBuild}} > {{KeEchtEffekte}}** でオフにできます。エディターには .bsp がないため、パーティクルは壁で跳ね返りません。"),
            T("**Decals:** impact and scorch marks of the effects land on the faces the game would mark: like q3map2, the editor builds the world of the map with its BSP tree - only the visible part of each brush face, the models of `misc_model` baked into the world, and only faces next to an open leaf of the tree (no mark on the floor hidden under a step). On a large map this takes a few seconds like a build; it runs in the background, and until it is done the last world counts. Faces whose shader moves its vertices (`deformVertexes move`) count over the whole box of the movement, as q3map2 sorts them into the tree.",
              "**Decals:** Einschläge und Brandflecken der Effekte landen auf den Flächen, die auch das Spiel markiert: der Editor baut die Welt der Karte wie q3map2 mit ihrem BSP-Baum - von jeder Brushseite nur der sichtbare Teil, die in die Welt gebackenen Modelle der `misc_model` und nur Flächen an einem offenen Blatt des Baums (kein Fleck auf dem Boden, der unter einer Stufe verborgen liegt). Auf einer großen Karte dauert das wie ein Bau ein paar Sekunden; es läuft im Hintergrund, und bis es fertig ist, gilt die letzte Welt. Flächen, deren Shader die Ecken bewegt (`deformVertexes move`), zählen wie in q3map2 über den ganzen Kasten der Bewegung.",
              "**贴花:**特效的弹痕和烧痕落在游戏也会标记的表面上:编辑器像 q3map2 那样用 BSP 树构建地图世界——每个画刷面只取可见部分,包括烘焙进世界的 `misc_model` 模型,并且只取开放叶子旁边的面(台阶下被遮住的地面不会留下痕迹)。在大型地图上这和一次编译一样需要几秒;它在后台运行,完成之前沿用上一次的世界。着色器会移动顶点的面(`deformVertexes move`)像 q3map2 一样按整个移动范围计算。",
              "**デカール:**エフェクトの弾痕や焦げ跡は、ゲームでも跡が付く面に付きます。エディターは q3map2 と同じように BSP ツリーでマップのワールドを作ります。各ブラシ面の見える部分だけ、ワールドに焼き込まれた `misc_model` のモデル、そしてツリーの開いたリーフに接する面だけです(段差の下に隠れた床には跡が付きません)。大きなマップではビルドと同じく数秒かかります。バックグラウンドで計算され、終わるまでは前のワールドが使われます。シェーダーが頂点を動かす面(`deformVertexes move`)は、q3map2 と同じく動く範囲全体で数えられます。"),
            T("With `-meta` (the default of the Build menu) q3map2 puts the faces together anew; the editor does the same and follows the BSP step of the Build menu entry you built last, also `-np`, `-maxarea`, `-patchmeta` and `-nofog`. Like q3map2 it cuts faces at fog brushes, adds the copies of `q3map_cloneShader` and `q3map_backShader` and projects `_decal` entities onto the world. A mark takes only the first 64 faces in its box - the same ones as in the game.",
              "Mit `-meta` (die Vorgabe im Build-Menü) setzt q3map2 die Flächen neu zusammen; der Editor macht es genauso und richtet sich nach dem BSP-Schritt des zuletzt gebauten Eintrags im Build-Menü, auch nach `-np`, `-maxarea`, `-patchmeta` und `-nofog`. Wie q3map2 schneidet er Flächen an Nebel-Brushes, legt die Kopien von `q3map_cloneShader` und `q3map_backShader` an und projiziert `_decal`-Entities auf die Welt. Eine Marke nimmt nur die ersten 64 Flächen in ihrem Kasten - dieselben wie im Spiel.",
              "使用 `-meta`(Build 菜单的默认设置)时,q3map2 会重新组合表面;编辑器也同样处理,并以 Build 菜单中最后构建的条目的 BSP 步骤为准,包括 `-np`、`-maxarea`、`-patchmeta` 和 `-nofog`。编辑器像 q3map2 一样在雾画刷处切分表面,添加 `q3map_cloneShader` 和 `q3map_backShader` 的副本,并把 `_decal` 实体投射到世界上。一个贴花只取其范围内的前 64 个表面——与游戏中相同。",
              "`-meta`(Build メニューの既定)では q3map2 が面を組み直します。エディターも同じように処理し、Build メニューで最後にビルドした項目の BSP ステップ(`-np`、`-maxarea`、`-patchmeta`、`-nofog` も含む)に従います。q3map2 と同じく、フォグブラシで面を切り分け、`q3map_cloneShader` と `q3map_backShader` のコピーを作り、`_decal` エンティティをワールドに投影します。マークはボックス内の最初の 64 面だけを使います。ゲームと同じ面です。"),
            T("**Sky portal:** with a misc_skyportal the sky shows the scene from the portal's place, with the same view and field of view, as in the game (CG_DrawSkyBoxPortal).",
              "**Skyportal:** mit einem misc_skyportal zeigt der Himmel die Szene vom Ort des Portals, mit demselben Blick und Bildwinkel, wie im Spiel (CG_DrawSkyBoxPortal).",
              "**天空传送门:**放置 misc_skyportal 后,天空显示从传送门位置看到的场景,视线和视野与游戏相同(CG_DrawSkyBoxPortal)。",
              "**スカイポータル:**misc_skyportal があると、空にはポータルの位置から同じ向き・同じ視野で見た場面が、ゲームと同じように映ります(CG_DrawSkyBoxPortal)。"),
        ]),
        ("img", "h_karte9d_nebel", T(
            "A fog brush over the floor of a room (fogParms 0.1 0.9 0.1, 256): the floor gets the fog colour of the game.",
            "Ein Nebel-Brush über dem Boden eines Raums (fogParms 0.1 0.9 0.1, 256): der Boden bekommt die Nebelfarbe des Spiels.",
            "房间地面上方的雾画刷(fogParms 0.1 0.9 0.1, 256):地面呈现游戏中的雾色。",
            "部屋の床の上のフォグブラシ(fogParms 0.1 0.9 0.1, 256):床がゲームと同じフォグの色になります。")),
        ("h2", "backen", T("Quick bake", "Schnell-Backen", "快速烘焙", "クイックベイク")),
        ("steps", [
            T("Switch on **{{KeLBuild}} > {{KeBackenMenue}}**. Two seconds after the last change bhed_q3map2 builds the map in a temporary folder - `-meta`, `-vis -fast` (when the map has no leak) and `-light -fast` - at low priority, so the editor stays smooth.",
              "Schalte **{{KeLBuild}} > {{KeBackenMenue}}** ein. Zwei Sekunden nach der letzten Änderung baut bhed_q3map2 die Karte in einem temporären Ordner - `-meta`, `-vis -fast` (wenn die Karte nicht leckt) und `-light -fast` - mit niedriger Priorität, der Editor bleibt flüssig.",
              "打开 **{{KeLBuild}} > {{KeBackenMenue}}**。最后一次修改两秒后,bhed_q3map2 会在临时文件夹中以低优先级编译地图——`-meta`、`-vis -fast`(地图没有泄漏时)和 `-light -fast`——编辑器保持流畅。",
              "**{{KeLBuild}} > {{KeBackenMenue}}** をオンにします。最後の変更から 2 秒後に、bhed_q3map2 が一時フォルダーで低優先度のままマップをビルドします(`-meta`、リークがなければ `-vis -fast`、`-light -fast`)。エディターは滑らかなままです。"),
            T("When it is done, the camera shows the real lightmaps of this build; top right it says **{{KeLichtGebacken}}**. Any change cancels a running build and starts a new one after the pause. Your map and a .bsp next to it are not touched.",
              "Ist er fertig, zeigt die Kamera die echten Lightmaps dieses Baus; oben rechts steht **{{KeLichtGebacken}}**. Jede Änderung bricht einen laufenden Bau ab und startet nach der Pause einen neuen. Deine Karte und eine .bsp daneben bleiben unberührt.",
              "完成后,摄像机显示这次编译的真实光照贴图,右上角显示 **{{KeLichtGebacken}}**。任何修改都会取消正在进行的编译,并在暂停后重新开始。你的地图及其旁边的 .bsp 不会被改动。",
              "完了すると、カメラにこのビルドの本物のライトマップが表示され、右上に **{{KeLichtGebacken}}** と表示されます。変更すると実行中のビルドは中止され、間を置いて新しいビルドが始まります。マップとその隣の .bsp には触れません。"),
        ]),
        ("img", "h_karte7_gebacken", T(
            "The same room after the quick bake: the lightmaps of bhed_q3map2.",
            "Derselbe Raum nach dem Schnell-Backen: die Lightmaps von bhed_q3map2.",
            "快速烘焙后的同一个房间:来自 bhed_q3map2 的光照贴图。",
            "クイックベイク後の同じ部屋:bhed_q3map2 のライトマップ。")),
        ("p", T(
            "The quick bake writes the same lightmaps, light grid and visibility as bhed_q3map2 on the command line with the same switches - the self-test compares the files. It is faster than a full build because of `-fast` and the missing bounce passes; for the release build use the {{KeLBuild}} menu.",
            "Das Schnell-Backen schreibt dieselben Lightmaps, dasselbe Lichtgitter und dieselbe Sichtbarkeit wie bhed_q3map2 auf der Kommandozeile mit denselben Schaltern - der Selbsttest vergleicht die Dateien. Schneller als ein voller Bau ist es durch `-fast` und die fehlenden Bounce-Durchgänge; für die fertige Karte nimm das Menü {{KeLBuild}}.",
            "快速烘焙写出的光照贴图、光照网格和可见性,与在命令行中用相同开关运行 bhed_q3map2 的结果完全相同——自检会比较这些文件。它之所以比完整编译快,是因为使用了 `-fast` 并且没有反弹过程;最终版本请使用 {{KeLBuild}} 菜单。",
            "クイックベイクは、同じスイッチでコマンドラインから bhed_q3map2 を実行した場合と同じライトマップ・ライトグリッド・可視性を書き出します(セルフテストがファイルを比較しています)。フルビルドより速いのは `-fast` を使いバウンスを行わないためです。完成版には {{KeLBuild}} メニューを使ってください。")),
        ("h2", "tempo", T("What got faster", "Was schneller wurde", "哪些变快了", "速くなった点")),
        ("p", T(
            "Every change was measured before and after (median of three runs, low priority, the same maps) and gives the same result: the .bsp of -bsp and -light is byte for byte the one of a 1-thread build of Stage 6, the saved maps are identical.",
            "Jede Änderung wurde vorher und nachher gemessen (Median aus drei Läufen, niedrige Priorität, dieselben Karten) und ergibt dasselbe: die .bsp von -bsp und -light ist Byte für Byte die eines Baus mit einem Thread der Stufe 6, die gespeicherten Karten sind gleich.",
            "每项改动都在前后进行了测量(三次运行的中位数、低优先级、相同的地图),结果完全相同:-bsp 和 -light 生成的 .bsp 与第 6 阶段单线程编译的结果逐字节一致,保存的地图也完全相同。",
            "すべての変更は前後で計測し(3 回の中央値、低優先度、同じマップ)、結果は同じです。-bsp と -light の .bsp はステージ 6 の 1 スレッドビルドとバイト単位で一致し、保存されるマップも同一です。")),
        ("list", [
            T("**-light** uses all cores for the lightmaps (light by light in blocks, the adaptive supersampling predicted and checked exactly) and for the light grid.",
              "**-light** nutzt alle Kerne für die Lightmaps (Licht für Licht in Blöcken, das adaptive Supersampling vorhergesagt und exakt geprüft) und für das Lichtgitter.",
              "**-light** 在光照贴图(按光源分块计算,自适应超采样先预测再精确核对)和光照网格上使用所有核心。",
              "**-light** はライトマップ(ライトごとにブロック単位で計算し、適応スーパーサンプリングは予測してから正確に確認)とライトグリッドに全コアを使います。"),
            T("**-bsp**: the T-junction fix searches in parallel and applies in order; finding equal light grid points uses buckets instead of a search through all points.",
              "**-bsp**: die T-Junction-Behebung sucht parallel und wendet der Reihe nach an; gleiche Punkte im Lichtgitter werden über Eimer gefunden statt durch eine Suche über alle Punkte.",
              "**-bsp**:T 形接缝修复并行搜索、按顺序应用;光照网格中相同的点通过分桶查找,而不是遍历所有点。",
              "**-bsp**:T ジャンクション修正は並列に探索して順に適用します。ライトグリッドの同じ点は全点の探索ではなくバケットで見つけます。"),
            T("**-vis** stays as in NetRadiant-Custom: with several threads it may differ slightly from build to build, with `-threads 1` it is always the same. Two ways to make it the same with several threads were tried and measured; both were slower.",
              "**-vis** bleibt wie in NetRadiant-Custom: mit mehreren Threads kann es von Bau zu Bau etwas abweichen, mit `-threads 1` ist es immer gleich. Zwei Wege, es mit mehreren Threads gleich zu machen, wurden ausprobiert und gemessen; beide waren langsamer.",
              "**-vis** 保持与 NetRadiant-Custom 相同:多线程时每次编译可能略有不同,使用 `-threads 1` 时总是相同。我们尝试并测量了两种让多线程结果一致的方法,两者都更慢。",
              "**-vis** は NetRadiant-Custom と同じです。複数スレッドではビルドごとにわずかに異なることがあり、`-threads 1` では常に同じです。複数スレッドで同じ結果にする方法を 2 つ試して計測しましたが、どちらも遅くなりました。"),
            T("**Editor**: invert selection, the selection test of the views and the NPC files of the optimizer (read in the background).",
              "**Editor**: Auswahl umkehren, die Auswahlabfrage der Ansichten und die NPC-Dateien des Optimierers (im Hintergrund gelesen).",
              "**编辑器**:反选、视图的选择查询,以及优化器的 NPC 文件(在后台读取)。",
              "**エディター**:選択の反転、ビューの選択判定、オプティマイザーの NPC ファイル(バックグラウンドで読み込み)。"),
        ]),
        ("table", _KOPF, tempo7.Q3MAP2),
        ("table", _EDITOR_KOPF, tempo7.EDITOR),
        ("p", T(
            "kejim_post \"Textures loaded\" was so fast before only because Stage 6 loaded Jedi Outcast without its folders in this run (0 shaders, every face with the replacement image - the error fixed in Stage 7); now the 1187 shaders and their images are loaded. Steps that were not changed vary by about 20 % from run to run.",
            "kejim_post „Texturen geladen“ war vorher nur so schnell, weil Stufe 6 Jedi Outcast in diesem Lauf ohne seine Ordner lud (0 Shader, jede Fläche mit dem Ersatzbild - der in Stufe 7 behobene Fehler); jetzt werden die 1187 Shader und ihre Bilder geladen. Nicht geänderte Schritte schwanken von Lauf zu Lauf um etwa 20 %.",
            "kejim_post 的“纹理加载完成”之前之所以这么快,只是因为第 6 阶段在这次运行中加载 Jedi Outcast 时没有其文件夹(0 个着色器,每个面都是替代图像——第 7 阶段修复的错误);现在会加载 1187 个着色器及其图像。未改动的步骤每次运行的波动约为 20%。",
            "kejim_post の「テクスチャの読み込み」が以前とても速かったのは、ステージ 6 がこの実行で Jedi Outcast をフォルダーなしで読み込んでいたため(シェーダー 0 個、すべての面が代替画像。ステージ 7 で修正した不具合)です。現在は 1187 個のシェーダーとその画像を読み込みます。変更していない処理は実行ごとに約 20 % 変動します。")),
        ("note", T(
            "Times on the test laptop (32 threads), median of three runs; \"-\" = not measured. Your computer will give other numbers, the ratio stays similar.",
            "Zeiten auf dem Test-Laptop (32 Threads), Median aus drei Läufen; \"-\" = nicht gemessen. Dein Rechner liefert andere Zahlen, das Verhältnis bleibt ähnlich.",
            "测试笔记本(32 线程)上的时间,三次运行的中位数;\"-\" = 未测量。你的电脑会得到不同的数字,但比例相近。",
            "時間はテスト用ノート PC(32 スレッド)での値で、3 回の中央値です。\"-\" は未計測です。お使いのコンピューターでは数値は異なりますが、比率は同程度です。")),
    ],
}

SEITEN = [KARTE_TEMPO]
