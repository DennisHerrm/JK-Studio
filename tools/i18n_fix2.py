"""Oberflaechentexte von FIX2 (08.11.: Funde der Code-Pruefung seit rc610 ausserhalb von Painting/CJK).

P5/P14: Meldungen und Hinweise des Animations-Imports (bhed/modell/animimport.h) als Art (bhed/modell/fehler.h) - vorher
deutsche Saetze der Bibliothek in jeder Sprache. P10: Herkunft und Lizenz der aus Blender portierten Teile im Fenster Ueber.
Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    # P10: Blender-Teile (Chamfer mit Segmenten, Gewichtsfarben, Oktaeder-Knochen, Abfall des Gewichtspinsels)
    ("AboutBlenderLizenz",
     "Chamfer with segments, weight colours, octahedral bones and the weight brush falloff: ported from Blender (GPL-2.0-or-later, Copyright Blender Authors), "
     "used under GPL v2 like JK Studio - see THIRD-PARTY.md.",
     "Chamfer mit Segmenten, Gewichtsfarben, Oktaeder-Knochen und der Abfall des Gewichtspinsels: portiert aus Blender (GPL-2.0-or-later, Copyright Blender "
     "Authors), unter der GPL v2 wie JK Studio - siehe THIRD-PARTY.md.",
     "带分段的倒角、权重颜色、八面体骨骼和权重笔刷衰减: 移植自 Blender (GPL-2.0-or-later, Copyright Blender Authors), 与 JK Studio 一样按 GPL v2 使用 - 见 THIRD-PARTY.md。",
     "セグメント付きの面取り、ウェイトの色、八面体ボーン、ウェイトブラシの減衰: Blender から移植 (GPL-2.0-or-later、Copyright Blender Authors)、JK Studio と同じく "
     "GPL v2 で使用 - THIRD-PARTY.md を参照。",
     "Chaflán con segmentos, colores de peso, huesos octaédricos y la caída del pincel de pesos: portados de Blender (GPL-2.0-or-later, Copyright Blender "
     "Authors), usados bajo la GPL v2 como JK Studio; véase THIRD-PARTY.md.",
     "Zkosení se segmenty, barvy vah, osmistěnné kosti a útlum štětce vah: převzato z Blenderu (GPL-2.0-or-later, Copyright Blender Authors), použito pod GPL v2 "
     "jako JK Studio - viz THIRD-PARTY.md."),
    # P14: Fehler des Imports
    ("MbFeKeineAnimation", "no animation in the file", "keine Animation in der Datei", "文件中没有动画", "ファイルにアニメーションがありません",
     "no hay animación en el archivo", "v souboru není žádná animace"),
    ("MbFeAchsenUnbestimmt", "the axes of the file are left-handed or undetermined (up %s, front %s)",
     "die Achsen der Datei sind linkshändig oder unbestimmt (oben %s, vorn %s)", "文件的坐标轴为左手系或无法确定 (上 %s, 前 %s)",
     "ファイルの軸が左手系か不明です (上 %s、前 %s)", "los ejes del archivo son zurdos o indeterminados (arriba %s, delante %s)",
     "osy souboru jsou levotočivé nebo neurčité (nahoru %s, dopředu %s)"),
    # P5: Bildzahl aus dem letzten Schluessel begrenzt
    ("MbFeAnimZuLang", "animation too long: %.0f frames would need %.0f MB (%.0f MB free) - is a key time in milliseconds instead of seconds?",
     "Animation zu lang: %.0f Bilder bräuchten %.0f MB (%.0f MB frei) - steht eine Schlüsselzeit in Millisekunden statt Sekunden?",
     "动画过长: %.0f 帧需要 %.0f MB (可用 %.0f MB) - 是否有关键帧时间以毫秒而非秒表示?",
     "アニメーションが長すぎます: %.0f フレームに %.0f MB 必要です (空き %.0f MB) - キーの時間が秒ではなくミリ秒になっていませんか?",
     "animación demasiado larga: %.0f fotogramas necesitarían %.0f MB (%.0f MB libres); ¿hay un tiempo de clave en milisegundos en lugar de segundos?",
     "animace je příliš dlouhá: %.0f snímků by potřebovalo %.0f MB (volných %.0f MB) - není čas klíče v milisekundách místo v sekundách?"),
    # P14: Hinweise des Imports
    ("MbHwBvhOhneSeiten", "BVH: no sides in the bone names, left = +X assumed", "BVH: keine Seiten in den Knochennamen, links = +X angenommen",
     "BVH: 骨骼名称中没有左右, 假定左 = +X", "BVH: ボーン名に左右がありません。左 = +X とみなします",
     "BVH: los nombres de los huesos no indican lado, se supone izquierda = +X", "BVH: názvy kostí neuvádějí stranu, předpokládá se vlevo = +X"),
    ("MbHwBvhBoden", "BVH: base pose set onto the floor of the frames (%.1f cm)", "BVH: Grundstellung auf den Boden der Bilder gesetzt (%.1f cm)",
     "BVH: 基础姿势已放到各帧的地面上 (%.1f cm)", "BVH: 基本ポーズをフレームの床に合わせました (%.1f cm)",
     "BVH: postura base colocada sobre el suelo de los fotogramas (%.1f cm)", "BVH: základní póza posazena na podlahu snímků (%.1f cm)"),
    ("MbHwUngleicheSkalierung", "non-uniform scaling removed (only rotation and position taken)",
     "ungleichmäßige Skalierung entfernt (nur Drehung und Ort übernommen)", "已移除非均匀缩放 (只采用旋转和位置)",
     "不均一なスケールを除去しました (回転と位置のみ使用)", "escala no uniforme eliminada (solo se toman rotación y posición)",
     "nerovnoměrné měřítko odstraněno (převzaty jen rotace a poloha)"),
    ("AiVornAusSkelett", "%s (from the skeleton)", "%s (aus dem Skelett)", "%s (来自骨架)", "%s (スケルトンから)", "%s (del esqueleto)", "%s (z kostry)"),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
