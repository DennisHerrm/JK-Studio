# -*- coding: utf-8 -*-
"""Oberflaechentexte von rc610 MODOBJ (Modell-Browser mit .obj, .fbx usw., Setzen wandelt in .md3) fuer tools/gen_i18n.py
und die weiteren Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("KeModBroMd3", "MD3", "MD3", "MD3", "MD3", "MD3", "MD3"),
    ("KeModBroMd3Hint",
     "Convert to MD3 when placing (double-click inserts a misc_model, a click gives the selected entities the model): models the game "
     "cannot load (.obj, .fbx, .glb, .ase ...) become an .md3 with their textures and a shader in the import folder, and the map takes "
     "them along into its .pk3 - so they also work in misc_model_static and misc_model_breakable, which the game loads at run time. "
     "Off: the original path for misc_model, like NetRadiant-Custom (q3map2 bakes it in); entities the game loads at run time still get the .md3.",
     "Beim Setzen in MD3 umwandeln (Doppelklick setzt ein misc_model, ein Klick gibt den gewählten Entities das Modell): Modelle, die "
     "das Spiel nicht laden kann (.obj, .fbx, .glb, .ase ...), werden eine .md3 mit ihren Texturen und einem Shader im Import-Ordner, und "
     "die Karte nimmt sie in ihre .pk3 mit - so gehen sie auch in misc_model_static und misc_model_breakable, die das Spiel zur Laufzeit "
     "lädt. Aus: der Original-Pfad für misc_model wie in NetRadiant-Custom (q3map2 backt es ein); Entities, die das Spiel zur Laufzeit "
     "lädt, bekommen weiter die .md3.",
     "放置时转换为 MD3(双击插入 misc_model,单击把模型赋给所选实体):游戏无法加载的模型(.obj、.fbx、.glb、.ase ……)会在导入文件夹中"
     "变成带纹理和着色器的 .md3,地图会把它们一起打包进 .pk3——这样它们也能用于游戏在运行时加载的 misc_model_static 和 "
     "misc_model_breakable。关闭:misc_model 使用原始路径,与 NetRadiant-Custom 相同(由 q3map2 烘焙);游戏在运行时加载的实体仍然得到 .md3。",
     "配置時に MD3 へ変換(ダブルクリックで misc_model を挿入、クリックで選択中のエンティティにモデルを設定): ゲームが読めないモデル"
     "(.obj、.fbx、.glb、.ase など)はインポートフォルダーでテクスチャとシェーダー付きの .md3 になり、マップはそれを .pk3 に一緒に入れます。"
     "これでゲームが実行時に読み込む misc_model_static や misc_model_breakable でも使えます。オフ: misc_model には NetRadiant-Custom と"
     "同じく元のパス(q3map2 が焼き込みます)。ゲームが実行時に読み込むエンティティには引き続き .md3 を使います。",
     "Convertir a MD3 al colocar (doble clic inserta un misc_model, un clic da el modelo a las entidades seleccionadas): los modelos que "
     "el juego no puede cargar (.obj, .fbx, .glb, .ase ...) se convierten en un .md3 con sus texturas y un shader en la carpeta de "
     "importación, y el mapa los lleva en su .pk3; así funcionan también en misc_model_static y misc_model_breakable, que el juego carga "
     "en tiempo de ejecución. Desactivado: la ruta original para misc_model, como en NetRadiant-Custom (q3map2 lo integra); las "
     "entidades que el juego carga en tiempo de ejecución siguen recibiendo el .md3.",
     "Při umístění převést na MD3 (dvojklik vloží misc_model, klik dá model vybraným entitám): modely, které hra neumí načíst (.obj, "
     ".fbx, .glb, .ase ...), se ve složce importu stanou souborem .md3 s texturami a shaderem a mapa je vezme do svého .pk3 - takže "
     "fungují i v misc_model_static a misc_model_breakable, které hra načítá za běhu. Vypnuto: původní cesta pro misc_model jako v "
     "NetRadiant-Custom (q3map2 ho zapeče); entity, které hra načítá za běhu, dostanou dál .md3."),
    ("KeMoUmgewandelt",
     "Model %s converted to %s (%d {surface|surfaces}, %d {2:triangle|triangles}; MD3, shader and textures in the import folder, the map takes them along into its .pk3).",
     "Modell %s in %s umgewandelt (%d {Fläche|Flächen}, %d {2:Dreieck|Dreiecke}; MD3, Shader und Texturen im Import-Ordner, die Karte nimmt sie in ihre .pk3 mit).",
     "模型 %s 已转换为 %s(%d 个面,%d 个三角形;MD3、着色器和纹理位于导入文件夹,地图会把它们一起打包进 .pk3)。",
     "モデル %s を %s に変換しました(サーフェス %d 個、三角形 %d 個。MD3・シェーダー・テクスチャはインポートフォルダーにあり、マップが .pk3 に一緒に入れます)。",
     "Modelo %s convertido en %s (%d {superficie|superficies}, %d {2:triángulo|triángulos}; MD3, shader y texturas en la carpeta de importación, el mapa los lleva en su .pk3).",
     "Model %s převeden na %s (%d {povrch|povrchy|povrchů}, %d {2:trojúhelník|trojúhelníky|trojúhelníků}; MD3, shader a textury ve složce importu, mapa je vezme do svého .pk3)."),
    ("KeOptPrModellFormat", "Model format the game cannot load", "Modellformat, das das Spiel nicht laden kann",
     "游戏无法加载的模型格式", "ゲームが読めないモデル形式", "Formato de modelo que el juego no puede cargar", "Formát modelu, který hra neumí načíst"),
    ("KeOptPrModellFormatE",
     "These entities are loaded by the game at run time, and the game only reads .md3 and .glm models (misc_model_static only .md3: with "
     "anything else the map does not load, \"is not an md3\"). q3map2 bakes other formats only into misc_model. Fix: give the entity the "
     "model again from the model browser with MD3 switched on - it becomes an .md3 in the import folder that the map takes along.",
     "Diese Entities lädt das Spiel zur Laufzeit, und es liest nur .md3- und .glm-Modelle (misc_model_static nur .md3: mit allem anderen "
     "lädt die Karte nicht, „is not an md3“). Andere Formate backt q3map2 nur in misc_model ein. Abhilfe: der Entity das Modell noch "
     "einmal aus dem Modell-Browser geben, mit eingeschaltetem MD3 - es wird eine .md3 im Import-Ordner, die die Karte mitnimmt.",
     "这些实体由游戏在运行时加载,而游戏只读取 .md3 和 .glm 模型(misc_model_static 只读 .md3:用其他格式时地图无法加载,“is not an md3”)。"
     "其他格式只有 misc_model 会被 q3map2 烘焙。解决:在打开 MD3 的情况下,从模型浏览器重新给该实体赋予模型——它会变成导入文件夹中的 .md3,"
     "地图会一起带走。",
     "これらのエンティティはゲームが実行時に読み込みますが、ゲームが読めるのは .md3 と .glm のモデルだけです(misc_model_static は .md3 のみ: "
     "それ以外ではマップが読み込めません、\"is not an md3\")。ほかの形式を q3map2 が焼き込むのは misc_model だけです。対処: MD3 をオンにして"
     "モデルブラウザーからもう一度エンティティにモデルを設定します。インポートフォルダーの .md3 になり、マップが一緒に持っていきます。",
     "El juego carga estas entidades en tiempo de ejecución y solo lee modelos .md3 y .glm (misc_model_static solo .md3: con cualquier "
     "otro el mapa no carga, \"is not an md3\"). q3map2 solo integra otros formatos en misc_model. Solución: dar de nuevo el modelo a la "
     "entidad desde el navegador de modelos con MD3 activado; se convierte en un .md3 en la carpeta de importación que el mapa lleva consigo.",
     "Tyto entity hra načítá za běhu a čte jen modely .md3 a .glm (misc_model_static jen .md3: s čímkoli jiným se mapa nenačte, „is not "
     "an md3“). Jiné formáty zapeče q3map2 jen do misc_model. Náprava: dát entitě model znovu z prohlížeče modelů se zapnutým MD3 - stane "
     "se z něj .md3 ve složce importu, kterou mapa vezme s sebou."),
    ("KeOptD36",
     "%s {entity|entities} the game loads at run time with a model format it cannot load (misc_model_static: only .md3, otherwise .md3 or .glm)",
     "%s {Entity|Entities}, die das Spiel zur Laufzeit lädt, mit einem Modellformat, das es nicht laden kann (misc_model_static: nur .md3, sonst .md3 oder .glm)",
     "%s 个由游戏在运行时加载的实体使用了游戏无法加载的模型格式(misc_model_static:只能 .md3,其他:.md3 或 .glm)",
     "ゲームが実行時に読み込むエンティティ %s 個が、ゲームの読めないモデル形式を使っています(misc_model_static: .md3 のみ、ほかは .md3 か .glm)",
     "%s {entidad|entidades} que el juego carga en tiempo de ejecución con un formato de modelo que no puede cargar (misc_model_static: solo .md3, si no .md3 o .glm)",
     "%s {entita|entity|entit}, které hra načítá za běhu, s formátem modelu, který neumí načíst (misc_model_static: jen .md3, jinak .md3 nebo .glm)"),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
