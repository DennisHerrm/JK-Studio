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
    # --- CSG Tool (csg.cpp:1470-1588) ------------------------------------------------
    ("KeCsgTitle", "CSG Tool", "CSG-Werkzeug", "CSG 工具", "CSG ツール"),
    ("KeCsgGrid", "Grid->", "Raster->", "网格->", "グリッド->"),
    ("KeCsgThickness", "Thickness", "Dicke", "厚度", "厚さ"),
    ("KeCsgMinusFaces", "-faces", "-Flächen", "-面", "-面"),
    ("KeCsgPlusFaces", "+faces", "+Flächen", "+面", "+面"),
    ("KeCsgMinusProj", "-proj", "-Proj", "-投影", "-投影"),
    ("KeCsgMinusCam", "-cam", "-Kam", "-相机", "-カメラ"),
    ("KeCsgTipMinusFaces", "Exclude selected faces", "Gewählte Flächen auslassen", "排除选中的面", "選択した面を除外"),
    ("KeCsgTipPlusFaces", "Only process selected faces", "Nur gewählte Flächen bearbeiten", "只处理选中的面", "選択した面だけを処理"),
    ("KeCsgTipProj", "Exclude faces, most orthogonal to active projection", "Flächen auslassen, die am meisten quer zur aktiven Projektion stehen",
     "排除与当前投影最正交的面", "現在の投影に最も直交する面を除外"),
    ("KeCsgTipCam", "Exclude faces, most orthogonal to camera view", "Flächen auslassen, die am meisten quer zur Kamerablickrichtung stehen",
     "排除与相机视线最正交的面", "カメラの視線に最も直交する面を除外"),
    ("KeCsgTipCaulk", "Caulk some faces", "Einige Flächen mit Caulk", "部分面使用 Caulk", "一部の面を Caulk にする"),
    ("KeCsgTipRemoveInner", "Remove inner brush", "Inneren Pinsel entfernen", "移除内部笔刷", "内側のブラシを削除"),
    ("KeCsgTipShrink", "Shrink brush", "Pinsel schrumpfen", "收缩笔刷", "ブラシを縮小"),
    ("KeCsgTipExpand", "Expand brush", "Pinsel erweitern", "扩展笔刷", "ブラシを拡大"),
    ("KeCsgTipDiag", "Hollow::diagonal joints", "Aushöhlen::Gehrungen", "挖空::斜接", "くり抜き::斜め継ぎ"),
    ("KeCsgTipWrap", "Hollow::wrap", "Aushöhlen::Umhüllen", "挖空::包裹", "くり抜き::ラップ"),
    ("KeCsgTipExtrude", "Hollow::extrude faces", "Aushöhlen::Flächen extrudieren", "挖空::挤出面", "くり抜き::面を押し出し"),
    ("KeCsgTipPull", "Hollow::pull faces", "Aushöhlen::Flächen ziehen", "挖空::拉出面", "くり抜き::面を引き出し"),
]
