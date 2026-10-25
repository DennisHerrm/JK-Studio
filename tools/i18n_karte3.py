# -*- coding: utf-8 -*-
"""Oberflaechentexte des Karten-Editors, Stufe 3 (Karten bauen), fuer tools/gen_i18n.py.

Englische Spalte bei NRC-Beschriftungen wortgleich NRC (ohne '&'); bei den
eigenen Zusaetzen (Programm-Eintraege in Misc und Help) die Begriffe des
Programms.
"""

# (Kennung, English, Deutsch, 中文, 日本語)
KARTE3 = [
    # --- Punkt 0b: eine Leiste, die umschaltet (Programm-Eintraege) ---------
    ("KeLLanguage", "Language", "Sprache", "语言", "言語"),
    ("KeLColorScheme", "Color Scheme", "Farbschema", "配色方案", "配色"),
    ("KeLScaling", "Scaling", "Skalierung", "缩放", "拡大縮小"),
    ("KeLManual", "Manual", "Handbuch", "手册", "マニュアル"),
    ("KeLAboutBehavEd", "About BehavEd", "Über BehavEd", "关于 BehavEd", "BehavEd について"),
    # --- Punkt 3: Dialog "Build Menu" (build.cpp BuildMenuDialog_construct) ---
    ("KeBmTitel", "Build Menu", "Build-Menü", "编译菜单", "ビルドメニュー"),
    ("KeBmGruppe", "Build menu", "Build-Menü", "编译菜单", "ビルドメニュー"),
    ("KeBmBefehle", "Commandline", "Befehlszeile", "命令行", "コマンドライン"),
    ("KeBmVariablen", "Build Variables", "Build-Variablen", "编译变量", "ビルド変数"),
    ("KeBmKonstanten", "Constants:", "Konstanten:", "常量：", "定数:"),
    ("KeBmWerkzeuge", "Editables:", "Bearbeitbar:", "可编辑：", "編集可能:"),
    ("KeBmResetTipp", "Reset to editor start state", "Auf den Stand beim Start des Editors zurücksetzen",
     "重置为编辑器启动时的状态", "エディター起動時の状態に戻す"),
    ("KeBmKopiert", "Copied to clipboard.", "In die Zwischenablage kopiert.", "已复制到剪贴板。", "クリップボードにコピーしました。"),
    # --- Punkt 4: Bauen aus dem Editor (eigene Zusaetze; Konsole und Statuszeile) ---
    ("KeBmAbbrechen", "Cancel build", "Bauen abbrechen", "取消编译", "ビルドを中止"),
    ("KeBmStatus", "Building %s: step %d of %d, %s", "Baue %s: Schritt %d von %d, %s", "正在编译 %s：第 %d 步，共 %d 步，%s",
     "ビルド中 %s: ステップ %d / %d、%s"),
    ("KeBmFertig", "=== build finished: %s, %d step(s), %s s ===", "=== Bauen fertig: %s, %d Schritt(e), %s s ===",
     "=== 编译完成：%s，%d 步，%s 秒 ===", "=== ビルド完了: %s、%d ステップ、%s 秒 ==="),
    ("KeBmGestoppt", "=== build stopped: %s, step %d of %d (%s), %s s ===", "=== Bauen angehalten: %s, Schritt %d von %d (%s), %s s ===",
     "=== 编译已停止：%s，第 %d 步，共 %d 步（%s），%s 秒 ===", "=== ビルド停止: %s、ステップ %d / %d (%s)、%s 秒 ==="),
    ("KeBmAbgebrochen", "=== build cancelled: %s, step %d of %d, %s s ===", "=== Bauen abgebrochen: %s, Schritt %d von %d, %s s ===",
     "=== 编译已取消：%s，第 %d 步，共 %d 步，%s 秒 ===", "=== ビルド中止: %s、ステップ %d / %d、%s 秒 ==="),
    ("KeBmLeckHinweis",
     "Leak: the map is not sealed. The leak line is shown in the views; step along it with Ctrl+Shift+K / Ctrl+Shift+L "
     "(Next / Previous leak spot), View > Pointfile switches it off.",
     "Leck: die Karte ist nicht dicht. Die Leck-Linie steht in den Ansichten; mit Strg+Umschalt+K / Strg+Umschalt+L "
     "(Next / Previous leak spot) an ihr entlang, View > Pointfile schaltet sie aus.",
     "泄漏：地图未封闭。泄漏线显示在视图中；用 Ctrl+Shift+K / Ctrl+Shift+L（下一个/上一个泄漏点）沿线查看，View > Pointfile 可关闭。",
     "リーク: マップが閉じていません。リーク線がビューに表示されます。Ctrl+Shift+K / Ctrl+Shift+L (次/前のリーク地点) で辿り、"
     "View > Pointfile で消せます。"),
    ("KeBmArbeitsordner", "The map lies in the game folder; it is built in the work folder %s (never in the game folder).",
     "Die Karte liegt im Spielordner; gebaut wird im Arbeitsordner %s (nie im Spielordner).",
     "地图位于游戏文件夹中；将在工作文件夹 %s 中编译（绝不写入游戏文件夹）。",
     "マップはゲームフォルダーにあります。作業フォルダー %s でビルドします (ゲームフォルダーには書き込みません)。"),
    ("KeBmZielImSpiel", "The output folder %s lies in the game folder; building in the work folder %s instead.",
     "Der Ziel-Ordner %s liegt im Spielordner; gebaut wird stattdessen im Arbeitsordner %s.",
     "输出文件夹 %s 位于游戏文件夹中；改为在工作文件夹 %s 中编译。",
     "出力フォルダー %s はゲームフォルダー内にあります。代わりに作業フォルダー %s でビルドします。"),
    ("KePrBuildZiel", "Output Folder", "Ziel-Ordner", "输出文件夹", "出力フォルダー"),
    ("KePrBuildZielHinweis", "Empty: next to the .map. Never inside the game folder - a map there is built in the work folder.",
     "Leer: neben der .map. Nie im Spielordner - liegt die Karte dort, wird im Arbeitsordner gebaut.",
     "留空：与 .map 同目录。绝不写入游戏文件夹——地图在游戏文件夹中时在工作文件夹中编译。",
     "空欄: .map と同じ場所。ゲームフォルダーには書き込みません — マップがそこにあれば作業フォルダーでビルドします。"),
    ("KeDbgTitel", "Q3Map debug window", "Q3Map-Debugfenster", "Q3Map 调试窗口", "Q3Map デバッグウィンドウ"),
]
