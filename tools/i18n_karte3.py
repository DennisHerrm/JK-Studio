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
]
