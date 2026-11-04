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
     "All menus the game reads (its .pk3 files and mod folders). Ctrl+S saves a loose menu of a mod or base folder back to its place and keeps the old one as .bak; a menu inside a .pk3 asks once where your copy goes.",
     "Alle Menüs, die das Spiel liest (seine .pk3-Dateien und Mod-Ordner). Strg+S speichert ein loses Menü eines Mod- oder base-Ordners an seinen Ort zurück und behält das alte als .bak; ein Menü in einer .pk3 fragt einmal, wohin deine Kopie kommt.",
     "游戏读取的所有菜单(其 .pk3 文件和模组文件夹)。Ctrl+S 会把模组或 base 文件夹中的散装菜单保存回原处,并把旧文件保留为 .bak;.pk3 中的菜单会询问一次副本保存到哪里。",
     "ゲームが読み込むすべてのメニュー (.pk3 ファイルと MOD フォルダー)。Ctrl+S で MOD フォルダーや base フォルダーの単体メニューは元の場所に保存され、古いファイルは .bak として残ります。.pk3 の中のメニューは、コピーの保存先を一度だけ尋ねます。",
     "Todos los menús que lee el juego (sus .pk3 y carpetas de mod). Ctrl+S guarda un menú suelto de una carpeta de mod o base en su sitio y conserva el anterior como .bak; un menú dentro de un .pk3 pregunta una vez dónde poner tu copia.",
     "Všechna menu, která hra čte (její soubory .pk3 a složky modů). Ctrl+S uloží volné menu ze složky modu nebo base zpět na jeho místo a staré ponechá jako .bak; u menu uvnitř .pk3 se jednou zeptá, kam dát vaši kopii."),
    ("MeStartDateiTip",
     "A .menu or menus .txt on your disk. Save writes back to the same file; in a mod or base folder the old one is kept as .bak. The game's program folder (its .exe files) stays locked.",
     "Ein .menu oder eine Menü-.txt auf der Platte. Speichern schreibt in dieselbe Datei zurück; in einem Mod- oder base-Ordner bleibt die alte als .bak. Der Programmordner des Spiels (mit seinen .exe) bleibt gesperrt.",
     "磁盘上的 .menu 或菜单 .txt。保存会写回同一个文件;在模组或 base 文件夹中,旧文件保留为 .bak。游戏的程序文件夹(含 .exe)保持锁定。",
     "ディスク上の .menu またはメニューの .txt。保存すると同じファイルに書き戻します。MOD フォルダーや base フォルダーでは古いファイルを .bak として残します。ゲームのプログラムフォルダー (.exe のある場所) には書き込みません。",
     "Un .menu o un .txt de menús del disco. Guardar escribe en el mismo archivo; en una carpeta de mod o base conserva el anterior como .bak. La carpeta de programa del juego (sus .exe) sigue bloqueada.",
     "Soubor .menu nebo .txt s menu na disku. Uložení zapíše zpět do stejného souboru; ve složce modu nebo base ponechá starý jako .bak. Programová složka hry (její .exe) zůstává zamčená."),
]


def zeilen_vier():
    """(Kennung, English, Deutsch, 中文, 日本語) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
