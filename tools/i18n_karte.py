# -*- coding: utf-8 -*-
"""Oberflaechentexte des Karten-Editors (Stufe 1b) fuer tools/gen_i18n.py.

Eigene Datei, weil es Hunderte Beschriftungen aus NetRadiant-Custom (NRC)
sind. Die englische Spalte ist bei Menues, Knoepfen und Dialogen WORTGLEICH
NRCs Beschriftung (ohne das Mnemonik-'&'): mit "Begriffe wie NetRadiant"
zeigt der Editor genau diese Spalte, damit die JKHub-Tutorials 1:1 passen.

Kennungen: Ke... allgemein, KeMenu... Menuetitel, KeCmd<NRC-Befehl>
Beschriftung eines Befehls (Menue), KeTip<NRC-Befehl> Tooltip eines
Werkzeugknopfs (NRCs Beschreibung).
"""

# (Kennung, English, Deutsch, 中文, 日本語)
KARTE = [
    # --- Arbeitsbereich ---------------------------------------------------
    ("ViewMapEditor", "Map Editor", "Karten-Editor", "地图编辑器", "マップエディター"),
    ("ModeMapEditorHint",
     "Build and edit maps for Jedi Academy and Jedi Outcast - like NetRadiant-Custom, with the same menus and shortcuts.",
     "Karten für Jedi Academy und Jedi Outcast bauen und bearbeiten - wie in NetRadiant-Custom, mit denselben Menüs und Tastenkürzeln.",
     "为 Jedi Academy 和 Jedi Outcast 制作和编辑地图 - 与 NetRadiant-Custom 相同的菜单和快捷键。",
     "Jedi Academy と Jedi Outcast のマップを作成・編集します - NetRadiant-Custom と同じメニューとショートカットです。"),
    ("KeConfirmModified",
     "The current map has changed since it was last saved.\nDo you want to save the current map before continuing?",
     "Die aktuelle Karte wurde seit dem letzten Speichern geändert.\nMöchtest du sie speichern, bevor es weitergeht?",
     "当前地图自上次保存后已被修改。\n要在继续之前保存当前地图吗?",
     "現在のマップは前回の保存後に変更されています。\n続行する前に保存しますか?"),
    ("KeSave", "Save", "Speichern", "保存", "保存"),
    ("KeDiscard", "Discard", "Verwerfen", "放弃", "破棄"),
    ("KeCancel", "Cancel", "Abbrechen", "取消", "キャンセル"),
    ("KeGameLoading", "Loading game data for %s ...", "Lade Spieldaten für %s ...", "正在加载 %s 的游戏数据 ...",
     "%s のゲームデータを読み込み中 ..."),
    ("KeGameLoaded", "%s: %d shaders, %d entity classes (%.1f s)", "%s: %d Shader, %d Entity-Klassen (%.1f s)",
     "%s: %d 个着色器, %d 个实体类 (%.1f 秒)", "%s: シェーダー %d 個、エンティティクラス %d 個 (%.1f 秒)"),
    ("KeGameNoFolders",
     "No game folder set up for %s - textures cannot be shown. Add the game folders in the program settings.",
     "Kein Spielordner für %s eingerichtet - Texturen können nicht angezeigt werden. Spielordner in den Programmeinstellungen hinzufügen.",
     "未为 %s 设置游戏文件夹 - 无法显示纹理。请在程序设置中添加游戏文件夹。",
     "%s のゲームフォルダーが設定されていません - テクスチャを表示できません。プログラムの設定でゲームフォルダーを追加してください。"),
    ("KeMapLoadFailed", "Could not load %s", "%s konnte nicht geladen werden", "无法加载 %s", "%s を読み込めませんでした"),
    ("KeMapSaveFailed", "Could not save %s: %s", "%s konnte nicht gespeichert werden: %s", "无法保存 %s: %s",
     "%s を保存できませんでした: %s"),
    ("KeNrcTerms", "Terms like NetRadiant (English)", "Begriffe wie NetRadiant (Englisch)", "使用 NetRadiant 术语(英文)",
     "NetRadiant の用語(英語)"),
    ("KeNrcTermsHint",
     "Shows the exact English labels of NetRadiant-Custom everywhere in the map editor, so JKHub tutorials match word for word. Tooltips stay in the program language.",
     "Zeigt im ganzen Karten-Editor die englischen Beschriftungen von NetRadiant-Custom, damit die JKHub-Tutorials Wort für Wort passen. Tooltips bleiben in der Programmsprache.",
     "在整个地图编辑器中显示 NetRadiant-Custom 的英文原文标签,使 JKHub 教程逐字对应。工具提示仍使用程序语言。",
     "マップエディター全体で NetRadiant-Custom の英語表記をそのまま表示し、JKHub のチュートリアルと一字一句一致させます。ツールチップはプログラムの言語のままです。"),

    # --- File-Menue (create_file_menu, mainframe.cpp:866-890) ------------------
    ("KeMenuFile", "File", "Datei", "文件", "ファイル"),
    ("KeCmdNewMap", "New Map", "Neue Karte", "新建地图", "新規マップ"),
    ("KeCmdOpenMap", "Open...", "Öffnen...", "打开...", "開く..."),
    ("KeCmdSaveMap", "Save", "Speichern", "保存", "保存"),
    ("KeCmdSaveMapAs", "Save as...", "Speichern unter...", "另存为...", "名前を付けて保存..."),
    ("KeRecentFiles", "Recent Files", "Zuletzt geöffnet", "最近的文件", "最近使ったファイル"),
]
