# -*- coding: utf-8 -*-
"""Oberflaechentexte von rc609 ME2 (Menue-Editor: ein loses Menue im Mod-Ordner an seinen Ort speichern) fuer
tools/gen_i18n.py und die weiteren Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("MeOrtGespeichert", "Saved %s in its place; the previous version is next to it as .bak.",
     "%s an seinem Ort gespeichert; der vorige Stand liegt daneben als .bak.",
     "已在原位置保存 %s;之前的版本以 .bak 留在旁边。",
     "%s を元の場所に保存しました。以前の版は隣に .bak として残っています。",
     "%s guardado en su sitio; la versión anterior queda al lado como .bak.",
     "%s uloženo na své místo; předchozí verze zůstala vedle jako .bak."),
    ("MeOrtVerdeckt",
     "Note: the game reads this menu from %s first (a folder's .pk3 files before its loose files, later folders before earlier ones) - your change only shows in the game when that version is gone.",
     "Hinweis: Das Spiel liest dieses Menü zuerst aus %s (die .pk3 eines Ordners vor seinen losen Dateien, spätere Ordner vor früheren) - deine Änderung zeigt sich im Spiel erst, wenn diese Fassung weg ist.",
     "注意:游戏会先从 %s 读取此菜单(同一文件夹中 .pk3 先于散装文件,后面的文件夹先于前面的)——只有那个版本不存在时,你的修改才会在游戏中生效。",
     "注意: ゲームはこのメニューを先に %s から読み込みます (フォルダー内では .pk3 が単体ファイルより先、後のフォルダーが前のフォルダーより先)。そちらがなくなるまで、変更はゲームに反映されません。",
     "Nota: el juego lee primero este menú de %s (los .pk3 de una carpeta antes que sus archivos sueltos, las carpetas posteriores antes que las anteriores); tu cambio solo se verá en el juego cuando esa versión no esté.",
     "Pozor: hra čte toto menu nejdřív z %s (soubory .pk3 složky před jejími volnými soubory, pozdější složky před dřívějšími) - vaše změna se ve hře projeví, až tato verze zmizí."),
    ("MeInPk3Kopie",
     "%s is inside %s: the editor does not replace the game's .pk3 files (a loose file next to it would not count in the game), so this saves a copy.",
     "%s liegt in %s: Der Editor ersetzt keine .pk3 des Spiels (eine lose Datei daneben gälte im Spiel nicht), darum wird eine Kopie gespeichert.",
     "%s 位于 %s 中:编辑器不会替换游戏的 .pk3 文件(放在旁边的散装文件在游戏中不生效),因此保存的是副本。",
     "%s は %s の中にあります。エディターはゲームの .pk3 を置き換えません (隣に置いた単体ファイルはゲームでは使われません)。そのためコピーとして保存します。",
     "%s está dentro de %s: el editor no sustituye los .pk3 del juego (un archivo suelto al lado no contaría en el juego), así que se guarda una copia.",
     "%s je uvnitř %s: editor nenahrazuje soubory .pk3 hry (volný soubor vedle by ve hře neplatil), proto se uloží kopie."),
]


def zeilen_vier():
    """(Kennung, English, Deutsch, 中文, 日本語) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
