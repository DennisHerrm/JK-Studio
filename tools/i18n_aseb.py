# -*- coding: utf-8 -*-
"""Oberflaechentexte von ASEB (.ase, .md3 und weitere Modellformate zu Brushes, Rechtsklick "Modell in Brushes umwandeln")
fuer tools/gen_i18n.py und die weiteren Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("KeMbMenue", "Convert model to brushes...", "Modell in Brushes umwandeln...", "将模型转换为画刷...", "モデルをブラシに変換...",
     "Convertir modelo en brushes...", "Převést model na brushe..."),
    ("KeMbTip",
     "Turns the selected misc_model into brushes in the same place (origin, angles, modelscale, also mirrored), with the textures "
     "q3map2 would bake for it (_remap and skin included) - in the window Geometry to brushes, one undo step. Not in NetRadiant-Custom.",
     "Macht aus dem gewählten misc_model Brushes an derselben Stelle (Ursprung, Winkel, modelscale, auch gespiegelt) mit den Texturen, "
     "die q3map2 ihm einbacken würde (mit _remap und Skin) - im Fenster Geometrie zu Brushes, ein Undo-Schritt. Nicht in NetRadiant-Custom.",
     "把选中的 misc_model 在同一位置(原点、角度、modelscale,也包括镜像)转换为画刷,并使用 q3map2 为它烘焙的纹理(包括 _remap 和皮肤)"
     "——在“几何体转画刷”窗口中进行,一步撤销。NetRadiant-Custom 中没有。",
     "選択した misc_model を同じ位置(原点、角度、modelscale、ミラーも)のブラシにし、q3map2 が焼き込むテクスチャ(_remap とスキンを含む)を"
     "付けます。ウィンドウ「ジオメトリをブラシに」で行い、元に戻すは 1 回です。NetRadiant-Custom にはありません。",
     "Convierte el misc_model seleccionado en brushes en el mismo sitio (origen, ángulos, modelscale, también reflejado), con las texturas "
     "que q3map2 le integraría (con _remap y skin), en la ventana Geometría a brushes, con un solo paso de deshacer. No está en NetRadiant-Custom.",
     "Převede vybraný misc_model na brushe na stejném místě (počátek, úhly, modelscale, i zrcadlený) s texturami, které by mu q3map2 "
     "zapekl (včetně _remap a skinu) - v okně Geometrie na brushe, jeden krok zpět. Není v NetRadiant-Custom."),
    ("KeMbKeins", "Select a misc_model with a model first.", "Zuerst ein misc_model mit Modell wählen.", "请先选择一个带模型的 misc_model。",
     "先にモデル付きの misc_model を選択してください。", "Primero selecciona un misc_model con modelo.", "Nejprve vyberte misc_model s modelem."),
    ("KeGiGlm",
     "%s: Ghoul2 models (.glm) are animated characters, not map geometry - q3map2 cannot bake them, so they cannot become brushes.",
     "%s: Ghoul2-Modelle (.glm) sind animierte Figuren, keine Kartengeometrie - q3map2 kann sie nicht einbacken, darum werden sie keine Brushes.",
     "%s:Ghoul2 模型(.glm)是动画角色,不是地图几何体——q3map2 无法烘焙它们,所以不能变成画刷。",
     "%s: Ghoul2 モデル(.glm)はアニメーションするキャラクターで、マップのジオメトリではありません。q3map2 が焼き込めないため、ブラシにはできません。",
     "%s: los modelos Ghoul2 (.glm) son personajes animados, no geometría de mapa; q3map2 no puede integrarlos, así que no pueden convertirse en brushes.",
     "%s: modely Ghoul2 (.glm) jsou animované postavy, ne geometrie mapy - q3map2 je neumí zapéct, proto z nich nemohou být brushe."),
    ("KeGiModellNamen", "Shaders as q3map2 bakes this model", "Shader so, wie q3map2 dieses Modell einbackt", "着色器与 q3map2 烘焙此模型时相同",
     "シェーダーは q3map2 がこのモデルを焼き込むときと同じ", "Shaders como q3map2 integra este modelo", "Shadery jako při zapečení modelu v q3map2"),
    ("KeGiModellNamenTip",
     "The brushes get the shader names q3map2 gives this model as misc_model: the material name if it starts with textures/ or models/, "
     "otherwise the diffuse bitmap (*BITMAP in an .ase) - without extension, an absolute path from textures/ or models/ on, a name "
     "without folder in the folder of the model; an .md3 keeps the shader of each surface. The surfaces of one closed body stay one part.",
     "Die Brushes bekommen die Shadernamen, die q3map2 diesem Modell als misc_model gibt: den Materialnamen, wenn er mit textures/ oder "
     "models/ beginnt, sonst das Diffuse-Bild (*BITMAP in einer .ase) - ohne Endung, ein absoluter Pfad ab textures/ oder models/, ein "
     "Name ohne Ordner im Ordner des Modells; eine .md3 behält den Shader jeder Fläche. Die Flächen eines geschlossenen Körpers bleiben ein Teil.",
     "画刷获得 q3map2 把此模型作为 misc_model 时给出的着色器名:材质名以 textures/ 或 models/ 开头时用材质名,否则用漫反射贴图(.ase 中的 "
     "*BITMAP)——去掉扩展名,绝对路径从 textures/ 或 models/ 开始截取,没有文件夹的名称放在模型所在的文件夹;.md3 保留每个面的着色器。"
     "同一封闭物体的各个面仍是一个部件。",
     "ブラシには、q3map2 がこのモデルを misc_model として扱うときのシェーダー名が付きます: マテリアル名が textures/ か models/ で始まれば"
     "その名前、そうでなければディフューズ画像(.ase の *BITMAP)。拡張子は外し、絶対パスは textures/ か models/ から、フォルダーのない名前は"
     "モデルのフォルダーに置きます。.md3 は各サーフェスのシェーダーのままです。閉じた 1 つの物体の面は 1 つの部品のままです。",
     "Los brushes reciben los nombres de shader que q3map2 da a este modelo como misc_model: el nombre del material si empieza por "
     "textures/ o models/, si no la imagen difusa (*BITMAP en un .ase), sin extensión; una ruta absoluta desde textures/ o models/, un "
     "nombre sin carpeta en la carpeta del modelo; un .md3 conserva el shader de cada superficie. Las superficies de un cuerpo cerrado "
     "siguen siendo una sola pieza.",
     "Brushe dostanou názvy shaderů, které q3map2 dá tomuto modelu jako misc_model: název materiálu, pokud začíná textures/ nebo "
     "models/, jinak difuzní obrázek (*BITMAP v .ase) - bez přípony, absolutní cesta od textures/ nebo models/, název bez složky ve "
     "složce modelu; .md3 ponechá shader každé plochy. Plochy jednoho uzavřeného tělesa zůstanou jednou částí."),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
