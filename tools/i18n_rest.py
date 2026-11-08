# -*- coding: utf-8 -*-
"""Oberflaechentexte von REST (08.11.: Fehler der Modell-Bibliothek in der Sprache der Oberflaeche, Code-Pruefung N6)
fuer tools/gen_i18n.py und die weiteren Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("MbFeKeinMd3", "not an .md3 model", "keine .md3", "不是 .md3 模型", ".md3 モデルではありません", "no es un modelo .md3", "není model .md3"),
    ("MbFeMd3PasstNicht", "the surfaces do not match the file", "die Flächen passen nicht zur Datei", "表面与文件不符", "サーフェスがファイルと一致しません",
     "las superficies no coinciden con el archivo", "plochy neodpovídají souboru"),
    ("MbFeMd3HinterFlaechen", "frames or tags behind the surfaces", "Bilder oder Tags hinter den Flächen", "帧或标签位于表面之后", "フレームまたはタグがサーフェスの後ろにあります",
     "fotogramas o etiquetas detrás de las superficies", "snímky nebo tagy za plochami"),
    ("MbFeFlaecheAbgeschnitten", "surface %d is cut off", "Fläche %d abgeschnitten", "表面 %d 被截断", "サーフェス %d が途中で切れています", "la superficie %d está cortada",
     "plocha %d je useknutá"),
    ("MbFeFlaecheUnbrauchbar", "surface %d is unusable", "Fläche %d unbrauchbar", "表面 %d 无法使用", "サーフェス %d は使用できません", "la superficie %d no se puede usar",
     "plocha %d je nepoužitelná"),
    ("MbFeFlaecheOhneNetz", "surface %d has no mesh", "Fläche %d ohne Netz", "表面 %d 没有网格", "サーフェス %d にメッシュがありません", "la superficie %d no tiene malla",
     "plocha %d nemá síť"),
    ("MbFeZuVieleEcken", "surface %s: %d vertices (limit %d)", "Fläche %s: %d Ecken (Grenze %d)", "表面 %s: %d 个顶点(上限 %d)", "サーフェス %s: 頂点 %d (上限 %d)",
     "superficie %s: %d vértices (límite %d)", "plocha %s: %d vrcholů (limit %d)"),
    ("MbFeZuVieleDreiecke", "surface %s: %d triangles (limit %d)", "Fläche %s: %d Dreiecke (Grenze %d)", "表面 %s: %d 个三角形(上限 %d)", "サーフェス %s: 三角形 %d (上限 %d)",
     "superficie %s: %d triángulos (límite %d)", "plocha %s: %d trojúhelníků (limit %d)"),
    ("MbFeKeinGlm", "not a Ghoul2 model (.glm)", "kein Ghoul2-Modell (.glm)", "不是 Ghoul2 模型(.glm)", "Ghoul2 モデル(.glm)ではありません", "no es un modelo Ghoul2 (.glm)",
     "není model Ghoul2 (.glm)"),
    ("MbFeGlaPfadZuLang", "name or path of the .gla longer than 63 characters", "Name oder Pfad der .gla länger als 63 Zeichen", ".gla 的名称或路径超过 63 个字符",
     ".gla の名前またはパスが 63 文字を超えています", "nombre o ruta del .gla de más de 63 caracteres", "název nebo cesta .gla delší než 63 znaků"),
    ("MbFeTeilNameZuLang", "name or shader of \"%s\" longer than 63 characters", "Name oder Shader von \"%s\" länger als 63 Zeichen", "\"%s\" 的名称或着色器超过 63 个字符",
     "\"%s\" の名前またはシェーダーが 63 文字を超えています", "nombre o shader de \"%s\" de más de 63 caracteres", "název nebo shader „%s“ delší než 63 znaků"),
    ("MbFeOhneLods", "surface \"%s\" lacks LOD levels", "Fläche \"%s\" ohne alle LOD-Stufen", "表面 \"%s\" 缺少 LOD 级别", "サーフェス \"%s\" に LOD レベルが足りません",
     "a la superficie \"%s\" le faltan niveles LOD", "plocha „%s“ nemá všechny úrovně LOD"),
    ("MbFeZuVieleKnochen", "surface \"%s\" (LOD %d) uses %d bones, a .glm holds at most 32", "Fläche \"%s\" (LOD %d) hat %d Knochen, eine .glm trägt höchstens 32",
     "表面 \"%s\"(LOD %d)使用 %d 根骨骼,.glm 最多 32 根", "サーフェス \"%s\" (LOD %d) のボーンは %d 本、.glm は最大 32 本です",
     "la superficie \"%s\" (LOD %d) usa %d huesos, un .glm admite como máximo 32", "plocha „%s“ (LOD %d) používá %d kostí, .glm unese nejvýše 32"),
    ("MbFeQuelleOhneBecken", "source without pelvis", "Quelle ohne Becken", "源没有骨盆", "ソースに骨盤がありません", "origen sin pelvis", "zdroj bez pánve"),
    ("MbFeZielOhneBecken", "target without pelvis", "Ziel ohne Becken", "目标没有骨盆", "ターゲットに骨盤がありません", "destino sin pelvis", "cíl bez pánve"),
    ("MbFeBilderAusserhalb", "frames outside the source", "Bilder außerhalb der Quelle", "帧超出源范围", "フレームがソースの範囲外です", "fotogramas fuera del origen",
     "snímky mimo zdroj"),
    ("MbFeNichtsZuBacken", "nothing to bake", "nichts zu backen", "没有可烘焙的内容", "ベイクするものがありません", "nada que hornear", "není co zapéct"),
    ("MbFeFremdesSkelett", "world of another skeleton", "Welt eines anderen Skeletts", "另一骨架的世界矩阵", "別のスケルトンのワールド行列です", "mundo de otro esqueleto",
     "svět jiné kostry"),
    ("MbFeVorratZuGross", "pool larger than a 24-bit index", "Vorrat größer als ein 24-Bit-Verweis", "池超出 24 位索引", "プールが 24 ビットの参照を超えています",
     "reserva mayor que un índice de 24 bits", "zásoba větší než 24bitový odkaz"),
    ("MbFeOhneBeckenBeine", "no pelvis and two legs", "kein Becken und zwei Beine", "没有骨盆和两条腿", "骨盤と 2 本の脚がありません", "sin pelvis y dos piernas",
     "chybí pánev a dvě nohy"),
    ("MbFeKeinPlatzBelegt", "no slot has a source", "kein Platz mit einer Quelle belegt", "没有槽位指定了来源", "ソースが割り当てられたスロットがありません",
     "ninguna ranura tiene un origen", "žádný slot nemá zdroj"),
    ("MbFeAnderesSkelett", "another skeleton than the GLA", "anderes Skelett als die GLA", "骨架与 GLA 不同", "GLA と異なるスケルトンです", "otro esqueleto que el del GLA",
     "jiná kostra než GLA"),
    ("MbFeKnochenAnders", "bone %s is missing in the GLA or has another base pose", "Knochen %s fehlt in der GLA oder hat eine andere Grundstellung",
     "骨骼 %s 在 GLA 中缺失或基础姿势不同", "ボーン %s が GLA にないか、基本姿勢が異なります", "el hueso %s falta en el GLA o tiene otra pose base",
     "kost %s v GLA chybí nebo má jinou základní pózu"),
    ("MbFeKeinSkelett", "no skeleton", "kein Skelett", "没有骨架", "スケルトンがありません", "sin esqueleto", "žádná kostra"),
    ("MbFeGrundanimFehlt", "base animation %s is missing", "Grundanimation %s fehlt", "缺少基础动画 %s", "基本アニメーション %s がありません", "falta la animación base %s",
     "chybí základní animace %s"),
    ("MbFeGrundbildFehlt", "frame of the base animation not in the GLA", "Bild der Grundanimation nicht in der GLA", "基础动画的帧不在 GLA 中", "基本アニメーションのフレームが GLA にありません",
     "fotograma de la animación base fuera del GLA", "snímek základní animace není v GLA"),
    ("MbFeVerweisHinterVorrat", "reference behind the pool", "Verweis hinter dem Vorrat", "引用超出池范围", "参照がプールの外を指しています", "referencia fuera de la reserva",
     "odkaz za zásobou"),
    ("MbFeKeinKinoPlatz", "no free slot BOTH_CIN_n", "kein Platz BOTH_CIN_n frei", "没有空闲的 BOTH_CIN_n 槽位", "空いている BOTH_CIN_n スロットがありません",
     "no hay ranura BOTH_CIN_n libre", "žádný volný slot BOTH_CIN_n"),
    ("MbFePlaetzeFiguren", "slots and figures do not match", "Plätze und Figuren passen nicht", "槽位与角色不匹配", "スロットとフィギュアが一致しません",
     "las ranuras y las figuras no coinciden", "sloty a postavy si neodpovídají"),
    ("MbFeFigurOhneGrund", "figure without base animation", "Figur ohne Grundanimation", "角色没有基础动画", "フィギュアに基本アニメーションがありません",
     "figura sin animación base", "postava bez základní animace"),
    ("MbFeFehlt", "missing: %s", "fehlt: %s", "缺少: %s", "ありません: %s", "falta: %s", "chybí: %s"),
    ("MbFeNichtUnterBecken", "not below the pelvis: %s", "nicht unter dem Becken: %s", "不在骨盆之下: %s", "骨盤の下にありません: %s", "no está bajo la pelvis: %s",
     "není pod pánví: %s"),
    ("MbFeQuellKnochenFehlt", "%s: bone %s is missing", "%s: Knochen %s fehlt", "%s: 缺少骨骼 %s", "%s: ボーン %s がありません", "%s: falta el hueso %s", "%s: chybí kost %s"),
    ("MbFeQuellGrundstellung", "%s: base pose of %s differs", "%s: Grundstellung von %s anders", "%s: %s 的基础姿势不同", "%s: %s の基本姿勢が異なります",
     "%s: la pose base de %s es distinta", "%s: základní póza %s se liší"),
    ("MbFeSequenzNichtInQuelle", "%s: frames %d+%d not in %s (%d frames)", "%s: Frames %d+%d nicht in %s (%d Frames)", "%s: 帧 %d+%d 不在 %s 中(%d 帧)",
     "%s: フレーム %d+%d が %s にありません (%d フレーム)", "%s: fotogramas %d+%d fuera de %s (%d fotogramas)", "%s: snímky %d+%d nejsou v %s (%d snímků)"),
    ("MbFeQuellVerweis", "%s: reference behind the pool", "%s: Verweis hinter dem Vorrat", "%s: 引用超出池范围", "%s: 参照がプールの外を指しています",
     "%s: referencia fuera de la reserva", "%s: odkaz za zásobou"),
    ("MbFeBereichAusserhalb", "range outside the GLA", "Bereich außerhalb der GLA", "范围超出 GLA", "範囲が GLA の外です", "rango fuera del GLA", "rozsah mimo GLA"),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
