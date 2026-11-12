# -*- coding: utf-8 -*-
"""Oberflaechentexte von LCFIX (12.11., Entscheidungen B4 und B5 von shank, plaene/entscheidungen_1211.md).

Das Fenster Baufortschritt nennt den Grund, warum der Licht-Cache bzw. der VIS-Zwischenstand nicht greift
(gui/app_karte_bau.cpp, bauLichtCacheText / bauVisStandText):
  KeLcGrundWeg      der Licht-Cache stammt von einem anderen Rechenweg (Grafikkarte ohne double gegen Prozessor)
  KeBpGrundFassung  der VIS-Zwischenstand stammt von einer anderen Programmversion (die Vorlage KeBpVisNicht sagt "wird von
                    vorn gerechnet")
Je Zeile: Kennung, English, Deutsch, 中文, 日本語, Español, Čeština.
"""

ZEILEN = [
    ("KeLcGrundWeg", "the cache was computed another way (graphics card without double precision, or the processor)",
     "der Cache wurde auf einem anderen Rechenweg erstellt (Grafikkarte ohne double oder Prozessor)",
     "缓存是用另一种计算方式生成的(不支持 double 的显卡,或处理器)",
     "キャッシュは別の計算方法で作られています(double 非対応のグラフィックカード、またはプロセッサー)",
     "la caché se calculó de otra manera (tarjeta gráfica sin doble precisión o el procesador)",
     "mezipaměť byla spočítána jiným způsobem (grafická karta bez double nebo procesor)"),
    ("KeBpGrundFassung", "it comes from another program version",
     "er stammt aus einer anderen Programmversion",
     "它来自另一个程序版本",
     "別のプログラムバージョンのものです",
     "procede de otra versión del programa",
     "pochází z jiné verze programu"),
]


def zeilen_vier():
    """(Kennung, en, de, zh, ja) fuer tools/gen_i18n.py"""
    return [z[:5] for z in ZEILEN]


def texte(sprache):
    """Kennung -> Text fuer tools/i18n_sprache_es.py und _cs.py"""
    nr = {"es": 5, "cs": 6}[sprache]
    return {z[0]: z[nr] for z in ZEILEN}
