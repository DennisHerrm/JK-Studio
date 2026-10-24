# -*- coding: utf-8 -*-
"""Oberflaechentexte der Stufe 2 des Karten-Editors (Bauwerkzeuge) fuer
tools/gen_i18n.py.

Wie in i18n_karte.py ist die englische Spalte bei Menues, Knoepfen, Dialogen
und Einstellungsseiten WORTGLEICH NetRadiant-Custom 20260114 (mit
"Begriffe wie NetRadiant" zeigt der Editor genau diese Spalte).
"""

# (Kennung, English, Deutsch, 中文, 日本語)
KARTE2 = [
    # --- Einstellungsseite Clipper (clippertool.cpp:163-180) -------------------
    ("KePrClipCaulk", "Caulk Clipper Cuts", "Schnittflächen mit Caulk", "裁剪切面使用 Caulk", "クリッパーの切断面を Caulk にする"),
    ("KePrClipResetFlip", "Reset Flipped State", "Umdrehen zurücksetzen", "重置翻转状态", "反転状態をリセット"),
    ("KePrClipResetPoints", "Reset Points on Split", "Punkte nach dem Teilen zurücksetzen", "分割后重置点", "分割後に点をリセット"),
    ("KePrClip2Points", "2 Points in 2D Views", "2 Punkte in 2D-Ansichten", "2D 视图中使用 2 个点", "2D ビューでは 2 点"),
    ("KePrClipDouble", "On DoubleClick do: ", "Bei Doppelklick: ", "双击时执行: ", "ダブルクリックで: "),
    ("KePrClipClip", "Clip    ", "Abschneiden    ", "裁剪    ", "クリップ    "),
    ("KePrClipSplit", "Split", "Teilen", "分割", "分割"),
]
