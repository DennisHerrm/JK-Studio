"""ASEB: .ase, .md3 und die weiteren Modellformate werden Brushes; Rechtsklick auf ein misc_model "Modell in Brushes umwandeln".

Anlass (Tester GP, Discord): "This update now have .ase to brush or not?" - shank (06.11.): "kannst du das nach stufe 3
einfuegen bitte das ase auch geht". Ergaenzt den Abschnitt "Geometrie zu Brushes" der Seite karte_werkzeuge9 um die Formate
und den Rechtsklick (Bilder h_aseb_fenster und h_aseb_menue aus dem Selbsttest, BHED_HB_NUR=aseb); den Tipp zu Spielmodellen im Tutorial k-geometrie
aendert inhalt_tutgeo.py.

Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_modobj.py: die ersten vier gehen in
T(...), Spanisch und Tschechisch nach inhalt_es.py / inhalt_cs.py (ES, CS).
"""

X = {
    "formate": (
        "**.ase, .md3 and every other model format q3map2 reads** (.3ds, .dae, .lwo, .ms3d, .x, .ply, .stl ...) work in the same window. "
        "They are read exactly as q3map2 reads a misc_model, and the brushes get the shader names q3map2 would bake: the material name if it "
        "starts with `textures/` or `models/`, otherwise the diffuse bitmap (`*BITMAP` of an .ase from 3ds Max) without extension - an "
        "absolute path from `textures/` or `models/` on, a name without folder in the folder of the model; an .md3 keeps the shader of each "
        "surface. There is no shader prefix and no Y up (these formats already have Z up). The surfaces of one closed body - for example a "
        "box with a Multi/Sub-Object material - stay one part, so it becomes solid brushes. Ghoul2 models (.glm) are animated characters, "
        "not map geometry: the console says so and the window stays closed.",
        "**.ase, .md3 und jedes weitere Modellformat, das q3map2 liest** (.3ds, .dae, .lwo, .ms3d, .x, .ply, .stl ...), gehen im selben "
        "Fenster. Sie werden genau so gelesen, wie q3map2 ein misc_model liest, und die Brushes bekommen die Shadernamen, die q3map2 "
        "einbacken würde: den Materialnamen, wenn er mit `textures/` oder `models/` beginnt, sonst das Diffuse-Bild (`*BITMAP` einer .ase "
        "aus 3ds Max) ohne Endung - ein absoluter Pfad ab `textures/` oder `models/`, ein Name ohne Ordner im Ordner des Modells; eine .md3 "
        "behält den Shader jeder Fläche. Es gibt keinen Shader-Vorsatz und kein Y oben (diese Formate haben schon Z oben). Die Flächen eines "
        "geschlossenen Körpers - etwa ein Kasten mit einem Multi/Sub-Object-Material - bleiben ein Teil und werden so feste Brushes. "
        "Ghoul2-Modelle (.glm) sind animierte Figuren, keine Kartengeometrie: Die Konsole sagt das, und das Fenster bleibt zu.",
        "**.ase、.md3 以及 q3map2 能读取的其他所有模型格式**(.3ds、.dae、.lwo、.ms3d、.x、.ply、.stl ……)都可在同一窗口中使用。它们的读取方式"
        "与 q3map2 读取 misc_model 完全相同,画刷获得 q3map2 会烘焙的着色器名:材质名以 `textures/` 或 `models/` 开头时用材质名,否则用漫反射贴图"
        "(3ds Max 导出的 .ase 中的 `*BITMAP`),去掉扩展名——绝对路径从 `textures/` 或 `models/` 开始截取,没有文件夹的名称放在模型所在的文件夹;"
        ".md3 保留每个面的着色器。这里没有着色器前缀,也没有 Y 轴向上(这些格式本来就是 Z 轴向上)。同一封闭物体的各个面——例如带 "
        "Multi/Sub-Object 材质的箱子——仍是一个部件,因此会变成实心画刷。Ghoul2 模型(.glm)是动画角色,不是地图几何体:控制台会说明这一点,窗口不会打开。",
        "**.ase、.md3、そして q3map2 が読めるほかのすべてのモデル形式**(.3ds、.dae、.lwo、.ms3d、.x、.ply、.stl など)も同じウィンドウで使えます。"
        "q3map2 が misc_model を読むのとまったく同じように読み込み、ブラシには q3map2 が焼き込むシェーダー名が付きます: マテリアル名が `textures/` "
        "か `models/` で始まればその名前、そうでなければディフューズ画像(3ds Max の .ase の `*BITMAP`)を拡張子なしで使います。絶対パスは "
        "`textures/` か `models/` から、フォルダーのない名前はモデルのフォルダーに置きます。.md3 は各サーフェスのシェーダーのままです。シェーダーの"
        "接頭辞と Y 上はありません(これらの形式は最初から Z が上です)。閉じた 1 つの物体の面(たとえば Multi/Sub-Object マテリアルの箱)は 1 つの"
        "部品のままなので、固体のブラシになります。Ghoul2 モデル(.glm)はアニメーションするキャラクターで、マップのジオメトリではありません。"
        "コンソールにそう表示され、ウィンドウは開きません。",
        "**.ase, .md3 y cualquier otro formato de modelo que lea q3map2** (.3ds, .dae, .lwo, .ms3d, .x, .ply, .stl ...) funcionan en la "
        "misma ventana. Se leen exactamente como q3map2 lee un misc_model, y los brushes reciben los nombres de shader que q3map2 integraría: "
        "el nombre del material si empieza por `textures/` o `models/`, si no la imagen difusa (`*BITMAP` de un .ase de 3ds Max) sin "
        "extensión; una ruta absoluta desde `textures/` o `models/`, un nombre sin carpeta en la carpeta del modelo; un .md3 conserva el "
        "shader de cada superficie. No hay prefijo de shader ni Y arriba (estos formatos ya tienen Z arriba). Las superficies de un cuerpo "
        "cerrado, por ejemplo una caja con un material Multi/Sub-Object, siguen siendo una sola parte y se convierten en brushes sólidos. "
        "Los modelos Ghoul2 (.glm) son personajes animados, no geometría de mapa: la consola lo dice y la ventana no se abre.",
        "**.ase, .md3 a každý další formát modelu, který q3map2 čte** (.3ds, .dae, .lwo, .ms3d, .x, .ply, .stl ...), fungují ve stejném "
        "okně. Čtou se přesně tak, jak q3map2 čte misc_model, a brushe dostanou názvy shaderů, které by q3map2 zapekl: název materiálu, "
        "pokud začíná `textures/` nebo `models/`, jinak difuzní obrázek (`*BITMAP` v .ase z 3ds Max) bez přípony - absolutní cesta od "
        "`textures/` nebo `models/`, název bez složky ve složce modelu; .md3 ponechá shader každé plochy. Není tu předpona shaderu ani Y "
        "nahoru (tyto formáty už mají Z nahoru). Plochy jednoho uzavřeného tělesa - třeba bedny s materiálem Multi/Sub-Object - zůstanou "
        "jednou částí, a proto se z nich stanou pevné brushe. Modely Ghoul2 (.glm) jsou animované postavy, ne geometrie mapy: konzole to "
        "řekne a okno se neotevře."),
    "bild_fenster": (
        "An .ase as from 3ds Max: a chest with a Multi/Sub-Object material and a handle become two closed parts with the shaders q3map2 bakes.",
        "Eine .ase wie aus 3ds Max: Eine Truhe mit Multi/Sub-Object-Material und ein Griff werden zwei geschlossene Teile mit den Shadern, die q3map2 einbackt.",
        "一个像从 3ds Max 导出的 .ase:带 Multi/Sub-Object 材质的箱子和把手成为两个封闭部件,使用 q3map2 烘焙的着色器。",
        "3ds Max から書き出したような .ase: Multi/Sub-Object マテリアルの箱と取っ手が、q3map2 が焼き込むシェーダーを持つ 2 つの閉じた部品になります。",
        "Un .ase como el de 3ds Max: un cofre con material Multi/Sub-Object y un asa se convierten en dos partes cerradas con los shaders que integra q3map2.",
        "Soubor .ase jako z 3ds Max: truhla s materiálem Multi/Sub-Object a držadlo se stanou dvěma uzavřenými částmi se shadery, které zapéká q3map2."),
    "rechtsklick": (
        "**Convert a placed model:** select a `misc_model` (also one from a .pk3), right-click into a view and choose **{{KeMbMenue}}** at "
        "the top of the menu - or **{{KeLFile}} > {{KeMbMenue}}**. The same window opens with the bodies of the model in the place of the "
        "entity: origin, angles and `modelscale`/`modelscale_vec` (mirrored models with the right winding), `_remap` and skin like q3map2. "
        "**{{KeGiEinfuegen}}** replaces the misc_model with the brushes in one undo step. Not in NetRadiant-Custom.",
        "**Ein gesetztes Modell umwandeln:** Wähle ein `misc_model` (auch eines aus einer .pk3), klick mit rechts in eine Ansicht und wähle "
        "oben im Menü **{{KeMbMenue}}** - oder **{{KeLFile}} > {{KeMbMenue}}**. Dasselbe Fenster öffnet sich mit den Körpern des Modells an "
        "der Stelle der Entity: Ursprung, Winkel und `modelscale`/`modelscale_vec` (gespiegelte Modelle mit richtigem Umlauf), `_remap` und "
        "Skin wie bei q3map2. **{{KeGiEinfuegen}}** ersetzt das misc_model durch die Brushes in einem Undo-Schritt. Nicht in NetRadiant-Custom.",
        "**转换已放置的模型:** 选中一个 `misc_model`(也可以来自 .pk3),在视图中单击右键,选择菜单顶部的 **{{KeMbMenue}}**——或者 "
        "**{{KeLFile}} > {{KeMbMenue}}**。同一窗口会打开,模型的各个物体位于实体的位置:原点、角度和 `modelscale`/`modelscale_vec`(镜像的模型"
        "使用正确的绕序),`_remap` 和皮肤与 q3map2 相同。**{{KeGiEinfuegen}}** 以一步撤销用画刷替换 misc_model。NetRadiant-Custom 中没有。",
        "**配置済みのモデルを変換する:** `misc_model`(.pk3 内のものも可)を選択し、ビューで右クリックしてメニューの一番上の **{{KeMbMenue}}** を"
        "選びます。**{{KeLFile}} > {{KeMbMenue}}** でもかまいません。同じウィンドウが開き、モデルの物体がエンティティの位置に入ります: 原点、角度、"
        "`modelscale`/`modelscale_vec`(ミラーしたモデルは正しい向きの面で)、`_remap` とスキンは q3map2 と同じです。**{{KeGiEinfuegen}}** で "
        "misc_model をブラシに置き換えます(元に戻すは 1 回)。NetRadiant-Custom にはありません。",
        "**Convertir un modelo colocado:** selecciona un `misc_model` (también uno de un .pk3), haz clic derecho en una vista y elige "
        "**{{KeMbMenue}}** arriba del menú, o **{{KeLFile}} > {{KeMbMenue}}**. Se abre la misma ventana con los cuerpos del modelo en el "
        "lugar de la entidad: origen, ángulos y `modelscale`/`modelscale_vec` (los modelos reflejados con el orden de vértices correcto), "
        "`_remap` y skin como en q3map2. **{{KeGiEinfuegen}}** sustituye el misc_model por los brushes en un solo paso de deshacer. No está "
        "en NetRadiant-Custom.",
        "**Převést umístěný model:** vyberte `misc_model` (i takový z .pk3), klikněte pravým tlačítkem do pohledu a nahoře v nabídce zvolte "
        "**{{KeMbMenue}}** - nebo **{{KeLFile}} > {{KeMbMenue}}**. Otevře se stejné okno s tělesy modelu na místě entity: počátek, úhly a "
        "`modelscale`/`modelscale_vec` (zrcadlené modely se správným pořadím vrcholů), `_remap` a skin jako v q3map2. **{{KeGiEinfuegen}}** "
        "nahradí misc_model brushi v jednom kroku zpět. Není v NetRadiant-Custom."),
    "bild_menue": (
        "With a misc_model selected: File > Convert model to brushes... - the same entry is at the top of the right-click menu of the views.",
        "Mit gewähltem misc_model: File > Modell in Brushes umwandeln... - derselbe Eintrag steht oben im Rechtsklick-Menü der Ansichten.",
        "选中 misc_model 后:File > 将模型转换为画刷...——视图右键菜单的顶部也有同一项。",
        "misc_model を選択した状態で File > モデルをブラシに変換... - ビューの右クリックメニューの一番上にも同じ項目があります。",
        "Con un misc_model seleccionado: File > Convertir modelo en brushes...; la misma entrada está arriba del menú contextual de las vistas.",
        "S vybraným misc_model: File > Převést model na brushe... - stejná položka je nahoře v nabídce pravého tlačítka pohledů."),
}

ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}


def _t(schluessel):
    from inhalt_basis import T
    t = X[schluessel]
    return T(t[0], t[1], t[2], t[3])


def einhaengen(werkzeuge9):
    """Abschnitt Geometrie zu Brushes der Seite karte_werkzeuge9: nach dem ersten Absatz die Formate und der Rechtsklick."""
    b = werkzeuge9["bloecke"]
    i = next(n for n, x in enumerate(b) if x[0] == "h2" and x[1] == "geometrie")
    i = next(n for n in range(i + 1, len(b)) if b[n][0] == "p")
    b.insert(i + 1, ("p", _t("formate")))
    b.insert(i + 2, ("img", "h_aseb_fenster", _t("bild_fenster")))
    b.insert(i + 3, ("p", _t("rechtsklick")))
    b.insert(i + 4, ("img", "h_aseb_menue", _t("bild_menue")))
