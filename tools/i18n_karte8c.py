# -*- coding: utf-8 -*-
"""Oberflaechentexte des Karten-Editors, Stufe 8c, fuer tools/gen_i18n.py.

Punkt 1: Build > "Pack map as .pk3" fuer das Tutorial "Erste eigene Karte" -
eigene Funktion (nicht in NetRadiant-Custom). Das Programm schreibt nie in
einen Spielordner: das Archiv liegt neben der gebauten .bsp, und die liegt nach
bauOrdnerVorschau nie im Spielordner. Ins Spiel kopiert es der Anwender selbst.
"""

# (Kennung, English, Deutsch, 中文, 日本語)
KARTE8C = [
    ("KePkMenue", "Pack map as .pk3", "Karte als .pk3 packen", "将地图打包为 .pk3", "マップを .pk3 にまとめる"),
    ("KePkMenueTip",
     "Writes <map>.pk3 next to the compiled .bsp, with maps/<map>.bsp (and <map>.bhed, if there is one). Copy it yourself into GameData/base or the folder of your mod - the program never writes into game folders (not in NetRadiant-Custom)",
     "Schreibt <Karte>.pk3 neben die gebaute .bsp, mit maps/<Karte>.bsp (und <Karte>.bhed, falls vorhanden). Kopiere sie selbst nach GameData/base oder in den Ordner deiner Mod - das Programm schreibt nie in Spielordner (nicht in NetRadiant-Custom)",
     "在已编译的 .bsp 旁写入 <地图>.pk3,其中包含 maps/<地图>.bsp(如有 <地图>.bhed 也一并放入)。请自行把它复制到 GameData/base 或你的模组文件夹——程序从不写入游戏文件夹(NetRadiant-Custom 中没有)",
     "コンパイル済みの .bsp の隣に <マップ>.pk3 を書き出します。中身は maps/<マップ>.bsp(<マップ>.bhed があればそれも)です。GameData/base かお使いの MOD のフォルダーへはご自分でコピーしてください。プログラムがゲームのフォルダーに書き込むことはありません(NetRadiant-Custom にはありません)"),
    ("KePkFertig",
     "Wrote %s (%s). Copy it into GameData/base or your mod folder, start the game and type devmap %s in the console.",
     "%s geschrieben (%s). Kopiere sie nach GameData/base oder in deinen Mod-Ordner, starte das Spiel und gib in der Konsole devmap %s ein.",
     "已写入 %s(%s)。请把它复制到 GameData/base 或你的模组文件夹,启动游戏并在控制台输入 devmap %s。",
     "%s を書き出しました(%s)。GameData/base か MOD のフォルダーにコピーし、ゲームを起動してコンソールで devmap %s と入力してください。"),
    ("KePkKeineBsp",
     "No compiled map: %s is missing - build the map first (Build menu).",
     "Keine gebaute Karte: %s fehlt - baue die Karte zuerst (Menü Build).",
     "没有已编译的地图:缺少 %s——请先编译地图(Build 菜单)。",
     "コンパイル済みのマップがありません:%s がありません。先にマップをビルドしてください(Build メニュー)。"),
    ("KePkSpielordner",
     "%s lies inside a game folder - the program never writes there.",
     "%s liegt in einem Spielordner - dort schreibt das Programm nie.",
     "%s 位于游戏文件夹中——程序从不在那里写入。",
     "%s はゲームのフォルダーの中にあります。プログラムはそこに書き込みません。"),
    ("KePkFehler", "Could not write %s: %s", "%s konnte nicht geschrieben werden: %s", "无法写入 %s:%s", "%s を書き込めませんでした:%s"),
]
