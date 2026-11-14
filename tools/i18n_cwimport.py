# -*- coding: utf-8 -*-
"""Oberflaechentexte von CW-IMPORT (Clone-Wars-Kette Teil 2, plaene/clonewars_kette.md): das Pruefprofil "Clone-Wars-Kette" (ab Werk
aus), die Testfigur, Massstab und Achsen beim Import, der Massstab je Raum, Treppe zu Clip-Rampe, glatte Clip-Wand und die Ebene
Collision.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    # --- Pruefprofil (shank 14.11.: "das ist wichtig das es zwar drin ist aber nicht standard") ---
    ("KeCwProfil", "Check profile", "Prüfprofil", "检查配置", "チェックプロファイル", "Perfil de comprobación", "Profil kontrol"),
    ("KeCwProfilStandard", "Standard", "Standard", "标准", "標準", "Estándar", "Standardní"),
    ("KeCwProfilClonewars", "Clone Wars chain", "Clone-Wars-Kette", "Clone Wars 流程", "Clone Wars チェーン", "Cadena Clone Wars",
     "Řetězec Clone Wars"),
    ("KeCwMenue", "Clone Wars chain (extended checks)", "Clone-Wars-Kette (erweiterte Prüfungen)", "Clone Wars 流程(扩展检查)",
     "Clone Wars チェーン(拡張チェック)", "Cadena Clone Wars (comprobaciones ampliadas)", "Řetězec Clone Wars (rozšířené kontroly)"),
    ("KeCwProfilTip",
     "Off by default. On: the optimizer also runs the 16 checks from the requirements of a Clone Wars mod (visible floor against collision, "
     "nearly coincident faces, steps, doors and corridors, textures, models, props), offers the report as JSON with coordinates, and "
     "\"Model + clip\" puts its clip brushes into the layer Collision. Off: everything behaves as without these additions.",
     "Ab Werk aus. An: der Optimierer führt zusätzlich die 16 Prüfungen aus den Anforderungen eines Clone-Wars-Mods aus (sichtbarer Boden "
     "gegen Kollision, fast deckungsgleiche Flächen, Stufen, Türen und Gänge, Texturen, Modelle, Requisiten), bietet den Bericht als JSON "
     "mit Koordinaten an, und \"Modell + Clip\" legt seine Clip-Pinsel in die Ebene Collision. Aus: alles verhält sich wie ohne diese "
     "Ergänzungen.",
     "默认关闭。开启时:优化器还会运行来自一个 Clone Wars 模组需求的 16 项检查(可见地面与碰撞、几乎重合的面、台阶、门和走廊、纹理、模型、道具),"
     "提供带坐标的 JSON 报告,并且“模型 + clip”会把它的 clip 笔刷放入图层 Collision。关闭时:一切都与没有这些补充时相同。",
     "既定ではオフ。オン: 最適化ツールは Clone Wars MOD の要件から来た 16 のチェック(見える床とコリジョン、ほぼ重なった面、段差、ドアと廊下、"
     "テクスチャ、モデル、小道具)も実行し、座標付きの JSON レポートを書き出せ、「モデル + クリップ」はクリップブラシをレイヤー Collision に"
     "入れます。オフ: すべてこれらの追加がないときと同じに動きます。",
     "Desactivado por defecto. Activado: el optimizador ejecuta además las 16 comprobaciones de los requisitos de un mod de Clone Wars "
     "(suelo visible frente a colisión, caras casi coincidentes, escalones, puertas y pasillos, texturas, modelos, accesorios), ofrece el "
     "informe como JSON con coordenadas, y \"Modelo + clip\" pone sus brushes clip en la capa Collision. Desactivado: todo se comporta "
     "como sin estas ampliaciones.",
     "Ve výchozím stavu vypnuto. Zapnuto: optimalizátor navíc provede 16 kontrol z požadavků módu Clone Wars (viditelná podlaha proti "
     "kolizi, téměř shodné plochy, schody, dveře a chodby, textury, modely, rekvizity), nabídne zprávu jako JSON se souřadnicemi a "
     "\"Model + clip\" vloží své clip brushe do vrstvy Collision. Vypnuto: vše se chová jako bez těchto doplňků."),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
