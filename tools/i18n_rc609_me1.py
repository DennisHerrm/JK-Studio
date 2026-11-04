# -*- coding: utf-8 -*-
"""Oberflaechentexte von rc609 ME1 (Menue-Editor: Menue oeffnen auf dem Startbildschirm) fuer tools/gen_i18n.py und die weiteren Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("MeStartMenueOeffnen", "Open menu", "Menü öffnen", "打开菜单", "メニューを開く", "Abrir menú", "Otevřít menu"),
    ("MeStartMenueOeffnenSub",
     "Edit an existing .menu from the game, a mod folder or your disk and save it - no project needed.",
     "Ein vorhandenes .menu aus dem Spiel, einem Mod-Ordner oder von der Platte bearbeiten und speichern - ohne Projekt.",
     "编辑游戏、模组文件夹或磁盘中现有的 .menu 并保存——无需项目。",
     "ゲーム、MOD フォルダー、ディスクにある既存の .menu を編集して保存します。プロジェクトは不要です。",
     "Edita un .menu existente del juego, de una carpeta de mod o del disco y guárdalo, sin proyecto.",
     "Upravte existující .menu ze hry, ze složky modu nebo z disku a uložte ho - bez projektu."),
    ("MeStartAusSpielTip",
     "All menus the game reads (its .pk3 files and mod folders). Saving asks once where to put your copy - the game folders stay unchanged.",
     "Alle Menüs, die das Spiel liest (seine .pk3-Dateien und Mod-Ordner). Beim Speichern fragt das Programm einmal, wohin deine Kopie kommt - die Spielordner bleiben unverändert.",
     "游戏读取的所有菜单(其 .pk3 文件和模组文件夹)。保存时会询问一次副本的位置——游戏文件夹保持不变。",
     "ゲームが読み込むすべてのメニュー (.pk3 ファイルと MOD フォルダー)。保存時にコピーの保存先を一度だけ尋ねます。ゲームフォルダーは変更しません。",
     "Todos los menús que lee el juego (sus .pk3 y carpetas de mod). Al guardar pregunta una vez dónde poner tu copia; las carpetas del juego no cambian.",
     "Všechna menu, která hra čte (její soubory .pk3 a složky modů). Při uložení se jednou zeptá, kam dát vaši kopii - složky hry zůstanou beze změny."),
    ("MeStartDateiTip",
     "A .menu or menus .txt on your disk. Save writes back to the same file (outside the game folders).",
     "Ein .menu oder eine Menü-.txt auf der Platte. Speichern schreibt in dieselbe Datei zurück (außerhalb der Spielordner).",
     "磁盘上的 .menu 或菜单 .txt。保存会写回同一个文件(游戏文件夹以外)。",
     "ディスク上の .menu またはメニューの .txt。保存すると同じファイルに書き戻します (ゲームフォルダー以外)。",
     "Un .menu o un .txt de menús del disco. Guardar escribe en el mismo archivo (fuera de las carpetas del juego).",
     "Soubor .menu nebo .txt s menu na disku. Uložení zapíše zpět do stejného souboru (mimo složky hry)."),
]


def zeilen_vier():
    """(Kennung, English, Deutsch, 中文, 日本語) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
