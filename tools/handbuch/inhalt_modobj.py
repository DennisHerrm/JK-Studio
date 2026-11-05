"""rc610 MODOBJ: der Modell-Browser zeigt .obj, .fbx usw. wie NetRadiant-Custom, Setzen wandelt in .md3 um.

Meldung (05.11.): "NetRadiant can show the obj models here but JKS cant." - shank: "kannst du das so adden fuer obj
fbx usw." und "convertiert er die obj und fbx nicht direkt in JKA useable format?". Ersetzt den Absatz unter
"Modelle" der Seite karteneditor_bauen (inhalt_karteneditor2.py) und fuegt die Umwandlung und einen Verweis auf das
Tutorial k-geometrie an.

Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_tutgeo.py: die ersten vier gehen in
T(...), Spanisch und Tschechisch nach inhalt_es.py / inhalt_cs.py (ES, CS).
"""

X = {
    "absatz": (
        "Entities like `misc_model` show their model in both views, with skin, `modelscale` and angles. The folders of the {{KeLModelBrowser}} "
        "(<kbd>/</kbd>) list every model type NetRadiant-Custom lists: `.md3`, `.ase` and everything its model loader reads (`.obj`, `.fbx`, "
        "`.glb`/`.gltf`, `.3ds`, `.dae`, `.md2`, `.mdl`, `.md5mesh`, `.blend`, `.x`, `.ply`, `.stl` and more), plus `.glm` - also in capital "
        "letters and from `.pk3` files. A double click on a folder loads its models; a click on a model gives it to the selected entities, a "
        "double click inserts a new `misc_model` in front of the camera. **{{KeLRefreshModels}}** reloads the models after they changed.",
        "Entities wie `misc_model` zeigen ihr Modell in beiden Ansichten, mit Skin, `modelscale` und Winkeln. Die Ordner des {{KeLModelBrowser}} "
        "(<kbd>/</kbd>) listen jede Modellart, die NetRadiant-Custom listet: `.md3`, `.ase` und alles, was sein Modell-Lader liest (`.obj`, "
        "`.fbx`, `.glb`/`.gltf`, `.3ds`, `.dae`, `.md2`, `.mdl`, `.md5mesh`, `.blend`, `.x`, `.ply`, `.stl` und mehr), dazu `.glm` - auch groß "
        "geschrieben und aus `.pk3`-Dateien. Ein Doppelklick auf einen Ordner lädt dessen Modelle; ein Klick auf ein Modell gibt es den gewählten "
        "Entities, ein Doppelklick setzt ein neues `misc_model` vor die Kamera. **{{KeLRefreshModels}}** lädt die Modelle neu, wenn sie sich "
        "geändert haben.",
        "`misc_model` 等实体会在两个视图中显示其模型,包括皮肤、`modelscale` 和角度。{{KeLModelBrowser}}(<kbd>/</kbd>)的文件夹会列出 "
        "NetRadiant-Custom 列出的所有模型类型:`.md3`、`.ase` 以及其模型加载器能读取的一切(`.obj`、`.fbx`、`.glb`/`.gltf`、`.3ds`、`.dae`、"
        "`.md2`、`.mdl`、`.md5mesh`、`.blend`、`.x`、`.ply`、`.stl` 等),另外还有 `.glm`——大写扩展名和 `.pk3` 文件中的模型也会列出。双击文件夹会"
        "加载其中的模型;单击模型会把它赋给选中的实体,双击则在摄像机前插入一个新的 `misc_model`。模型修改后,**{{KeLRefreshModels}}** 重新加载它们。",
        "`misc_model` などのエンティティは、両方のビューでモデルをスキン、`modelscale`、角度付きで表示します。{{KeLModelBrowser}}(<kbd>/</kbd>)の"
        "フォルダーには、NetRadiant-Custom が一覧にするすべてのモデル形式が並びます: `.md3`、`.ase`、そのモデルローダーが読めるもの全部(`.obj`、"
        "`.fbx`、`.glb`/`.gltf`、`.3ds`、`.dae`、`.md2`、`.mdl`、`.md5mesh`、`.blend`、`.x`、`.ply`、`.stl` など)、さらに `.glm`。大文字の拡張子や "
        "`.pk3` ファイル内のものも出ます。フォルダーをダブルクリックするとそのモデルを読み込みます。モデルをクリックすると選択中のエンティティに割り当て、"
        "ダブルクリックするとカメラの前に新しい `misc_model` を置きます。モデルを変更したら **{{KeLRefreshModels}}** で読み直します。",
        "Las entidades como `misc_model` muestran su modelo en ambas vistas, con skin, `modelscale` y ángulos. Las carpetas del {{KeLModelBrowser}} "
        "(<kbd>/</kbd>) muestran todos los tipos de modelo que muestra NetRadiant-Custom: `.md3`, `.ase` y todo lo que lee su cargador de modelos "
        "(`.obj`, `.fbx`, `.glb`/`.gltf`, `.3ds`, `.dae`, `.md2`, `.mdl`, `.md5mesh`, `.blend`, `.x`, `.ply`, `.stl` y más), además de `.glm`; "
        "también en mayúsculas y desde archivos `.pk3`. Un doble clic en una carpeta carga sus modelos; un clic en un modelo se lo da a las "
        "entidades seleccionadas, un doble clic inserta un `misc_model` nuevo delante de la cámara. **{{KeLRefreshModels}}** vuelve a cargar "
        "los modelos cuando han cambiado.",
        "Entity jako `misc_model` ukazují svůj model v obou pohledech, se skinem, `modelscale` a úhly. Složky {{KeLModelBrowser}} (<kbd>/</kbd>) "
        "uvádějí každý typ modelu, který uvádí NetRadiant-Custom: `.md3`, `.ase` a vše, co čte jeho zavaděč modelů (`.obj`, `.fbx`, "
        "`.glb`/`.gltf`, `.3ds`, `.dae`, `.md2`, `.mdl`, `.md5mesh`, `.blend`, `.x`, `.ply`, `.stl` a další), k tomu `.glm` - i velkými "
        "písmeny a ze souborů `.pk3`. Dvojklik na složku načte její modely; klik na model ho dá vybraným entitám, dvojklik vloží nový "
        "`misc_model` před kameru. **{{KeLRefreshModels}}** modely znovu načte, když se změnily."),
    "md3": (
        "The game itself only loads `.md3` and `.glm` models (`misc_model_static` only `.md3`). With **MD3** switched on (top left in the "
        "{{KeLModelBrowser}}, on by default), placing a model of another format converts it: the editor writes an `.md3` with the same triangles, "
        "its textures and a shader into the import folder (`models/import/<name>/`), and the entity gets that path (large models also a "
        "`modelscale`). **Pack map as .pk3** and **Test in game** take these files along into the map's `.pk3`. So the model also works in "
        "`misc_model_static` and `misc_model_breakable`, which the game loads at run time. With **MD3** off, `misc_model` keeps the original path "
        "like in NetRadiant-Custom - q3map2 bakes `.obj` and the other formats into the map; entities the game loads at run time get the `.md3` "
        "anyway. The map check of the Optimizer reports entities the game loads at run time with a model it cannot load.",
        "Das Spiel selbst lädt nur `.md3`- und `.glm`-Modelle (`misc_model_static` nur `.md3`). Mit eingeschaltetem **MD3** (oben links im "
        "{{KeLModelBrowser}}, Vorgabe an) wandelt das Setzen ein Modell eines anderen Formats um: der Editor schreibt eine `.md3` mit denselben "
        "Dreiecken, ihre Texturen und einen Shader in den Import-Ordner (`models/import/<name>/`), und die Entity bekommt diesen Pfad (große "
        "Modelle auch ein `modelscale`). **Pack map as .pk3** und **Im Spiel testen** nehmen diese Dateien in die `.pk3` der Karte mit. So geht "
        "das Modell auch in `misc_model_static` und `misc_model_breakable`, die das Spiel zur Laufzeit lädt. Mit ausgeschaltetem **MD3** behält "
        "`misc_model` den Original-Pfad wie in NetRadiant-Custom - q3map2 backt `.obj` und die anderen Formate in die Karte ein; Entities, die das "
        "Spiel zur Laufzeit lädt, bekommen trotzdem die `.md3`. Die Kartenprüfung des Optimierers meldet Entities, die das Spiel zur Laufzeit "
        "lädt, mit einem Modell, das es nicht laden kann.",
        "游戏本身只加载 `.md3` 和 `.glm` 模型(`misc_model_static` 只加载 `.md3`)。打开 **MD3**({{KeLModelBrowser}} 左上角,默认打开)时,"
        "放置其他格式的模型会先转换:编辑器把三角形相同的 `.md3`、它的纹理和一个着色器写入导入文件夹(`models/import/<名称>/`),实体得到这个路径"
        "(大模型还会得到 `modelscale`)。**Pack map as .pk3** 和 **在游戏中测试** 会把这些文件一起放进地图的 `.pk3`。这样模型也能用于游戏在运行时"
        "加载的 `misc_model_static` 和 `misc_model_breakable`。关闭 **MD3** 时,`misc_model` 与 NetRadiant-Custom 一样保留原始路径——q3map2 会把 "
        "`.obj` 和其他格式烘焙进地图;游戏在运行时加载的实体仍然得到 `.md3`。优化器的地图检查会报告游戏在运行时加载、但模型格式无法加载的实体。",
        "ゲーム自体が読めるのは `.md3` と `.glm` のモデルだけです(`misc_model_static` は `.md3` のみ)。**MD3** をオンにすると({{KeLModelBrowser}} の"
        "左上、既定でオン)、ほかの形式のモデルは配置時に変換されます。エディターは同じ三角形の `.md3`、そのテクスチャ、シェーダーをインポートフォルダー"
        "(`models/import/<名前>/`)に書き込み、エンティティにはそのパスが入ります(大きなモデルには `modelscale` も)。**Pack map as .pk3** と "
        "**ゲームでテスト** はこれらのファイルをマップの `.pk3` に一緒に入れます。これで、ゲームが実行時に読み込む `misc_model_static` や "
        "`misc_model_breakable` でもモデルが使えます。**MD3** をオフにすると、`misc_model` は NetRadiant-Custom と同じく元のパスのままです。q3map2 が "
        "`.obj` などをマップに焼き込みます。ゲームが実行時に読み込むエンティティには、それでも `.md3` が入ります。オプティマイザーのマップ検査は、"
        "ゲームが実行時に読み込むのに読めないモデルを持つエンティティを報告します。",
        "El juego solo carga modelos `.md3` y `.glm` (`misc_model_static` solo `.md3`). Con **MD3** activado (arriba a la izquierda en el "
        "{{KeLModelBrowser}}, activado por defecto), colocar un modelo de otro formato lo convierte: el editor escribe un `.md3` con los mismos "
        "triángulos, sus texturas y un shader en la carpeta de importación (`models/import/<nombre>/`), y la entidad recibe esa ruta (los modelos "
        "grandes también un `modelscale`). **Pack map as .pk3** y **Probar en el juego** llevan estos archivos en el `.pk3` del mapa. Así el modelo "
        "funciona también en `misc_model_static` y `misc_model_breakable`, que el juego carga en tiempo de ejecución. Con **MD3** desactivado, "
        "`misc_model` conserva la ruta original como en NetRadiant-Custom: q3map2 integra `.obj` y los demás formatos en el mapa; las entidades que "
        "el juego carga en tiempo de ejecución reciben igualmente el `.md3`. La comprobación del mapa del optimizador avisa de las entidades que el "
        "juego carga en tiempo de ejecución con un modelo que no puede cargar.",
        "Hra sama načítá jen modely `.md3` a `.glm` (`misc_model_static` jen `.md3`). Se zapnutým **MD3** (vlevo nahoře v {{KeLModelBrowser}}, "
        "ve výchozím stavu zapnuto) se model jiného formátu při umístění převede: editor zapíše do složky importu (`models/import/<název>/`) "
        "`.md3` se stejnými trojúhelníky, jeho textury a shader a entita dostane tuto cestu (velké modely i `modelscale`). **Pack map as .pk3** "
        "a **Testovat ve hře** vezmou tyto soubory do `.pk3` mapy. Model tak funguje i v `misc_model_static` a `misc_model_breakable`, které hra "
        "načítá za běhu. S vypnutým **MD3** si `misc_model` ponechá původní cestu jako v NetRadiant-Custom - q3map2 zapeče `.obj` a ostatní "
        "formáty do mapy; entity, které hra načítá za běhu, dostanou `.md3` i tak. Kontrola mapy v optimalizátoru hlásí entity, které hra "
        "načítá za běhu, s modelem, který neumí načíst."),
    "bild": (
        "The {{KeLModelBrowser}} with `.obj`, `.fbx`, `.glb`, `.ase` and `.md3` models of one folder; top left the switch **MD3**.",
        "Der {{KeLModelBrowser}} mit `.obj`-, `.fbx`-, `.glb`-, `.ase`- und `.md3`-Modellen eines Ordners; oben links der Schalter **MD3**.",
        "{{KeLModelBrowser}} 显示某个文件夹中的 `.obj`、`.fbx`、`.glb`、`.ase` 和 `.md3` 模型;左上角是开关 **MD3**。",
        "あるフォルダーの `.obj`・`.fbx`・`.glb`・`.ase`・`.md3` モデルを表示した {{KeLModelBrowser}}。左上にスイッチ **MD3**。",
        "El {{KeLModelBrowser}} con modelos `.obj`, `.fbx`, `.glb`, `.ase` y `.md3` de una carpeta; arriba a la izquierda el interruptor **MD3**.",
        "{{KeLModelBrowser}} s modely `.obj`, `.fbx`, `.glb`, `.ase` a `.md3` jedné složky; vlevo nahoře přepínač **MD3**."),
    "verweis": (
        "To turn a model into brushes instead: [[k-geometrie]].", "Ein Modell stattdessen zu Brushes machen: [[k-geometrie]].",
        "如果想把模型变成画刷:[[k-geometrie]]。", "モデルをブラシにする場合: [[k-geometrie]]。",
        "Para convertir un modelo en brushes: [[k-geometrie]].", "Chcete-li z modelu místo toho udělat brushe: [[k-geometrie]]."),
}

ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}


def _t(schluessel):
    from inhalt_basis import T
    t = X[schluessel]
    return T(t[0], t[1], t[2], t[3])


def einhaengen(bauen):
    """Der Absatz unter "Modelle" der Seite karteneditor_bauen: neu, dazu die Umwandlung und der Verweis auf k-geometrie."""
    b = bauen["bloecke"]
    i = next(n for n, x in enumerate(b) if x[0] == "h2" and x[1] == "modelle")
    i = next(n for n in range(i + 1, len(b)) if b[n][0] == "p")
    b[i] = ("p", _t("absatz"))
    b.insert(i + 1, ("img", "h_modobj_browser", _t("bild")))
    b.insert(i + 2, ("p", _t("md3")))
    b.insert(i + 3, ("note", _t("verweis")))
