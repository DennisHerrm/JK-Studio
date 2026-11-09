# -*- coding: utf-8 -*-
"""Oberflaechentexte von REST8 (09.11.: doppelte Shadernamen - der Bau nimmt den Eintrag von bhed_q3map2, das Spiel einen anderen).

Der Optimierer des Karten-Editors meldet Shader der Karte, die q3map2 und das Spiel verschieden aufloesen (Pruefung "shadervorrang").
Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("KeOptPrShaderVorrang", "Shaders the build and the game resolve differently", "Shader, die Bau und Spiel verschieden auflösen",
     "构建与游戏解析不同的着色器", "ビルドとゲームで解決が異なるシェーダー", "Shaders que la compilación y el juego resuelven distinto",
     "Shadery, které sestavení a hra řeší různě"),
    ("KeOptPrShaderVorrangE",
     "These shaders are defined in more than one shader file (or twice in one), and bhed_q3map2 takes another definition than the game: "
     "q3map2 reads loose files before every .pk3 and the .pk3 of base before those of the mod, and the first definition wins; the game "
     "joins the files in reverse search order, and the first entry of that text wins. What the build reads from the shader (surfaceparm, "
     "surface light, lightmap size) comes from the build's definition, what the game draws from the game's. The real-time light shows the "
     "build's definition, Built map as in the game the game's. Fix: give your shader a name of its own, or remove the extra definition.",
     "Diese Shader stehen in mehr als einer Shader-Datei (oder zweimal in einer), und bhed_q3map2 nimmt eine andere Definition als das "
     "Spiel: q3map2 liest lose Dateien vor jeder .pk3 und die .pk3 von base vor denen des Mods, und die erste Definition gilt; das Spiel "
     "hängt die Dateien in umgekehrter Suchfolge aneinander, und der erste Eintrag dieses Textes gilt. Was der Bau aus dem Shader liest "
     "(surfaceparm, Flächenlicht, Lightmap-Größe), kommt aus der Definition des Baus, was das Spiel zeichnet, aus der des Spiels. Das "
     "Echtzeit-Licht zeigt die Definition des Baus, Gebaute Karte wie im Spiel die des Spiels. Abhilfe: dem eigenen Shader einen eigenen "
     "Namen geben oder die zusätzliche Definition entfernen.",
     "这些着色器在多个着色器文件中定义(或在一个文件中定义了两次),而 bhed_q3map2 采用的定义与游戏不同:q3map2 先读取松散文件,再读取任何 "
     ".pk3,并且先读 base 的 .pk3 再读模组的,第一个定义生效;游戏按相反的搜索顺序拼接这些文件,该文本中的第一个条目生效。构建从着色器读取的内容"
     "(surfaceparm、表面光、光照贴图大小)来自构建所用的定义,游戏绘制的内容来自游戏所用的定义。实时光照显示构建的定义,“按游戏显示已构建的地图”"
     "显示游戏的定义。解决方法:给自己的着色器起一个独有的名称,或删除多余的定义。",
     "これらのシェーダーは複数のシェーダーファイルで(または 1 つのファイルで 2 回)定義されており、bhed_q3map2 はゲームとは別の定義を使います: "
     "q3map2 はどの .pk3 よりも先にばらのファイルを読み、MOD の .pk3 より先に base の .pk3 を読み、最初の定義が有効になります。ゲームはファイルを"
     "検索順の逆につなげ、そのテキストの最初のエントリが有効になります。ビルドがシェーダーから読むもの(surfaceparm、面光源、ライトマップサイズ)は"
     "ビルドの定義から、ゲームが描くものはゲームの定義から来ます。リアルタイムライトはビルドの定義を、「ビルドしたマップをゲームと同じ表示に」は"
     "ゲームの定義を表示します。対処: 自分のシェーダーに固有の名前を付けるか、余分な定義を削除してください。",
     "Estos shaders están definidos en más de un archivo de shaders (o dos veces en uno), y bhed_q3map2 toma otra definición que el juego: "
     "q3map2 lee los archivos sueltos antes que cualquier .pk3 y los .pk3 de base antes que los del mod, y vale la primera definición; el "
     "juego une los archivos en el orden de búsqueda inverso, y vale la primera entrada de ese texto. Lo que la compilación lee del shader "
     "(surfaceparm, luz de superficie, tamaño del lightmap) viene de la definición de la compilación; lo que dibuja el juego, de la del "
     "juego. La luz en tiempo real muestra la definición de la compilación; Mapa compilado como en el juego, la del juego. Solución: da a "
     "tu shader un nombre propio o elimina la definición sobrante.",
     "Tyto shadery jsou definované ve více souborech shaderů (nebo dvakrát v jednom) a bhed_q3map2 bere jinou definici než hra: q3map2 čte "
     "volné soubory před každým .pk3 a .pk3 z base před těmi z modu a platí první definice; hra spojí soubory v obráceném pořadí hledání "
     "a platí první záznam tohoto textu. Co sestavení čte ze shaderu (surfaceparm, plošné světlo, velikost lightmapy), pochází z definice "
     "sestavení, co hra kreslí, z definice hry. Světlo v reálném čase ukazuje definici sestavení, Sestavená mapa jako ve hře definici hry. "
     "Náprava: dejte svému shaderu vlastní název, nebo odstraňte nadbytečnou definici."),
    ("KeOptD38",
     "%s {shader is|shaders are} defined more than once, and the build takes another definition than the game (%s)",
     "%s {Shader steht|Shader stehen} mehrfach, und der Bau nimmt eine andere Definition als das Spiel (%s)",
     "%s 个着色器有多个定义,构建采用的定义与游戏不同 (%s)",
     "複数回定義され、ビルドがゲームと別の定義を使うシェーダー %s 個 (%s)",
     "%s {shader está definido|shaders están definidos} más de una vez, y la compilación toma otra definición que el juego (%s)",
     "%s {shader je definován|shadery jsou definovány|shaderů je definováno} vícekrát a sestavení bere jinou definici než hra (%s)"),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
