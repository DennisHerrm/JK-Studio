# -*- coding: utf-8 -*-
"""Oberflaechentexte von rc609 B13 (In fertige Karte einfuegen, Stufe 2 und 3: Clips in die Welt, sichtbare Mover mit
Lightmaps) fuer tools/gen_i18n.py und die weiteren Sprachen.

Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("KeBmEinfuegenLicht",
     "Visible brush entities: compiling the part again with the world as shadow caster and the lights of the map (q3map2 -bsp, -light) ...",
     "Sichtbare Brush-Entities: der Teil wird noch einmal mit der Welt als Schattenwerfer und den Lichtern der Karte gebaut (q3map2 -bsp, -light) ...",
     "可见画刷实体:以世界作为投影体并使用地图的灯光再次编译该部分(q3map2 -bsp、-light)……",
     "見えるブラシエンティティ: ワールドを影の投影元に、マップのライトで部分をもう一度ビルドします (q3map2 -bsp、-light) ...",
     "Entidades de brush visibles: la parte se compila otra vez con el mundo como proyector de sombras y las luces del mapa (q3map2 -bsp, -light) ...",
     "Viditelné brush entity: část se sestaví znovu se světem jako vrhačem stínů a se světly mapy (q3map2 -bsp, -light) ..."),
    ("KeBmEinfuegenClips", "Inserted %d {clip brush|clip brushes} into %d {2:leaf|leaves} of the world of %s.", "%d {Clip-Brush|Clip-Brushes} in %d {2:Blatt|Blätter} der Welt von %s eingefügt.",
     "已将 %d 个裁剪画刷插入 %d 个世界叶节点(%s)。", "%d 個のクリップブラシをワールドの %d 個のリーフに挿入しました (%s)。",
     "{Se insertó|Se insertaron} %d {brush clip|brushes clip} en %d {2:hoja|hojas} del mundo de %s.", "{Vložen|Vloženy|Vloženo} %d {clip brush|clip brushe|clip brushů} do %d {2:listu|listů|listů} světa mapy %s."),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
