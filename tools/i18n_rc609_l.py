# -*- coding: utf-8 -*-
"""Oberflaechentexte von rc609 L (Licht geht durch Patches: -patchshadows als Einstellung der Karte) fuer tools/gen_i18n.py und die weiteren Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("KePatchSchattenKarteTip",
     "Saved in this map (worldspawn _bhed_patchshadows) and also used by every full build: the light stage gets -patchshadows. "
     "Off, light goes through patches as in q3map2 - of the default build entries only [Final] has -patchshadows. "
     "The light grid ignores patches in any case (q3map2), so models under a patch stay lit.",
     "Wird in dieser Karte gespeichert (worldspawn _bhed_patchshadows) und gilt auch für jeden vollen Bau: der Licht-Schritt bekommt -patchshadows. "
     "Aus geht das Licht wie in q3map2 durch Patches - von den Vorgabe-Einträgen hat nur [Final] -patchshadows. "
     "Das Lichtgitter übergeht Patches immer (q3map2), Modelle unter einem Patch bleiben also hell.",
     "保存在此地图中(worldspawn _bhed_patchshadows),每次完整编译也会使用:光照步骤会加上 -patchshadows。"
     "关闭时,光线会像 q3map2 一样穿过面片——默认编译项中只有 [Final] 带 -patchshadows。"
     "光照网格始终忽略面片(q3map2),因此面片下的模型仍然是亮的。",
     "このマップに保存され (worldspawn _bhed_patchshadows)、すべてのフルビルドでも使われます: ライトの段階に -patchshadows が付きます。"
     "オフのときは q3map2 と同じく光がパッチを通り抜けます - 既定のビルド項目で -patchshadows があるのは [Final] だけです。"
     "ライトグリッドは常にパッチを無視するため (q3map2)、パッチの下のモデルは明るいままです。",
     "Se guarda en este mapa (worldspawn _bhed_patchshadows) y también lo usa cada compilación completa: el paso de luz recibe -patchshadows. "
     "Desactivado, la luz atraviesa los patches como en q3map2: de las entradas predeterminadas solo [Final] tiene -patchshadows. "
     "La rejilla de luz ignora siempre los patches (q3map2), así que los modelos bajo un patch siguen iluminados.",
     "Ukládá se v této mapě (worldspawn _bhed_patchshadows) a platí i pro každé úplné sestavení: krok světla dostane -patchshadows. "
     "Vypnuto, světlo prochází patchi jako v q3map2 - z výchozích položek sestavení má -patchshadows jen [Final]. "
     "Mřížka světla patche vždy přeskakuje (q3map2), modely pod patchem tedy zůstanou světlé."),
    ("KePatchSchattenHinweis",
     "Light goes through patches: this map has %d patches, but the light stage runs without -patchshadows (as in q3map2). "
     "Turn on Build > Patch shadows (saved in the map) or use a build entry with -patchshadows.",
     "Licht geht durch Patches: Die Karte hat %d Patches, aber der Licht-Schritt läuft ohne -patchshadows (wie in q3map2). "
     "Build > Patch-Schatten einschalten (wird in der Karte gespeichert) oder einen Bau-Eintrag mit -patchshadows nehmen.",
     "光线会穿过面片:此地图有 %d 个面片,但光照步骤没有 -patchshadows(与 q3map2 相同)。"
     "请打开 Build > 面片阴影(保存在地图中),或使用带 -patchshadows 的编译项。",
     "光がパッチを通り抜けます: このマップには %d 個のパッチがありますが、ライトの段階は -patchshadows なしで実行されます (q3map2 と同じ)。"
     "Build > パッチの影 をオンにする (マップに保存されます) か、-patchshadows 付きのビルド項目を使ってください。",
     "La luz atraviesa los patches: este mapa tiene %d patches, pero el paso de luz se ejecuta sin -patchshadows (como en q3map2). "
     "Activa Build > Sombras de patches (se guarda en el mapa) o usa una entrada de compilación con -patchshadows.",
     "Světlo prochází patchi: mapa má %d patchů, ale krok světla běží bez -patchshadows (jako v q3map2). "
     "Zapněte Build > Stíny patchů (uloží se v mapě) nebo použijte položku sestavení s -patchshadows."),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
