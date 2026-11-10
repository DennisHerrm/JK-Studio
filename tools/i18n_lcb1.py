# -*- coding: utf-8 -*-
"""Oberflaechentexte von LCB1 (10.11.: Licht-Cache auch mit -bounce, rc612 Punkt 1).

Das Fenster Baufortschritt sagt im Licht-Schritt, was der Licht-Cache tut: wie viele Lightmaps der laufende
Durchgang uebernimmt, oder warum er nicht greift (gui/app_karte_bau.cpp, bauFortschrittZeichnen).
Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("KeLcUebernommen", "Light cache: %d of %d lightmaps reused (pass %d of %d)",
     "Licht-Cache: %d von %d Lightmaps übernommen (Durchgang %d von %d)",
     "光照缓存:已重用 %d / %d 个光照贴图(第 %d / %d 遍)",
     "ライトキャッシュ: %d / %d 個のライトマップを再利用(パス %d / %d)",
     "Caché de luz: %d de %d lightmaps reutilizados (pasada %d de %d)",
     "Mezipaměť světla: znovu použito %d z %d lightmap (průchod %d z %d)"),
    ("KeLcGefunden", "Light cache found: each pass reuses the lightmaps whose input is unchanged",
     "Licht-Cache gefunden: jeder Durchgang übernimmt die Lightmaps mit unveränderten Eingaben",
     "找到光照缓存:每一遍都会重用输入未改变的光照贴图",
     "ライトキャッシュあり: 各パスで入力が変わっていないライトマップを再利用します",
     "Caché de luz encontrada: cada pasada reutiliza los lightmaps cuyas entradas no han cambiado",
     "Mezipaměť světla nalezena: každý průchod znovu použije lightmapy s nezměněnými vstupy"),
    ("KeLcNicht", "Light cache not used: %s - every lightmap is computed.",
     "Licht-Cache greift nicht: %s - jede Lightmap wird neu gerechnet.",
     "未使用光照缓存:%s——所有光照贴图都将重新计算。",
     "ライトキャッシュは使われません: %s - すべてのライトマップを計算し直します。",
     "La caché de luz no se usa: %s; se calculan todos los lightmaps.",
     "Mezipaměť světla se nepoužije: %s – všechny lightmapy se spočítají znovu."),
    ("KeLcGrundKein", "no cache from an earlier build yet", "noch kein Cache aus einem früheren Bau", "还没有之前构建的缓存",
     "以前のビルドのキャッシュがまだありません", "aún no hay caché de una compilación anterior", "zatím není mezipaměť z dřívějšího sestavení"),
    ("KeLcGrundFormat", "the cache is from an older version", "der Cache stammt aus einer älteren Version", "缓存来自旧版本",
     "キャッシュが古いバージョンのものです", "la caché es de una versión anterior", "mezipaměť pochází ze starší verze"),
    ("KeLcGrundDefekt", "the cache file is damaged", "die Cache-Datei ist beschädigt", "缓存文件已损坏", "キャッシュファイルが壊れています",
     "el archivo de caché está dañado", "soubor mezipaměti je poškozený"),
    ("KeLcGrundSchalter", "the light switches changed", "die Licht-Schalter haben sich geändert", "光照开关已更改", "ライトのスイッチが変わりました",
     "los parámetros de luz cambiaron", "přepínače světla se změnily"),
    ("KeLcGrundGeometrie", "geometry, shaders or brush entities changed", "Geometrie, Shader oder Brush-Entities haben sich geändert",
     "几何体、着色器或画刷实体已更改", "ジオメトリ、シェーダー、ブラシエンティティが変わりました",
     "cambiaron la geometría, los shaders o las entidades de brush", "změnila se geometrie, shadery nebo brush entity"),
    ("KeLcAusSchalter", "Light cache off: every lightmap is computed (Build > Partial compile: reuse light)",
     "Licht-Cache aus: jede Lightmap wird gerechnet (Kompilieren > Teil-Kompilierung: Licht wiederverwenden)",
     "光照缓存已关闭:所有光照贴图都会计算(编译 > 部分编译:重用光照)",
     "ライトキャッシュはオフ: すべてのライトマップを計算します(ビルド > 部分コンパイル:ライトを再利用)",
     "Caché de luz desactivada: se calculan todos los lightmaps (Build > Compilación parcial: reutilizar la luz)",
     "Mezipaměť světla je vypnutá: počítají se všechny lightmapy (Build > Dílčí kompilace: znovu použít světlo)"),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
