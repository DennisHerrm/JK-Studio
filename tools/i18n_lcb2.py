# -*- coding: utf-8 -*-
"""Oberflaechentexte von LCB2 (10.11.: Licht-Cache auch nach Geometrie-Aenderungen, rc612).

Das Fenster Baufortschritt sagt im Licht-Schritt, ob der Licht-Cache die Lightmaps nach einer Geometrie-,
Shader- oder Brush-Entity-Aenderung ueber ihren Inhalt findet, und nennt geaenderte worldspawn-Schluessel als
eigenen Grund, warum er nicht greift (gui/app_karte_bau.cpp, bauLichtCacheText).
Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("KeLcGefundenInhalt", "Light cache found: geometry changed, lightmaps are matched by their content",
     "Licht-Cache gefunden: Geometrie geändert, Lightmaps werden über ihren Inhalt zugeordnet",
     "找到光照缓存:几何体已更改,光照贴图按内容匹配",
     "ライトキャッシュあり: ジオメトリが変わったため、ライトマップを内容で照合します",
     "Caché de luz encontrada: la geometría cambió, los lightmaps se asignan por su contenido",
     "Mezipaměť světla nalezena: geometrie se změnila, lightmapy se přiřazují podle obsahu"),
    ("KeLcGrundWelt", "worldspawn keys changed", "worldspawn-Schlüssel haben sich geändert",
     "worldspawn 键已更改", "worldspawn のキーが変わりました",
     "cambiaron las claves de worldspawn", "změnily se klíče worldspawn"),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
