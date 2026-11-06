"""Alle Seiten des Handbuchs in Lesereihenfolge.

Jede Seite: id (Dateiname), gruppe, titel, kurz (Einleitung), optional
titelbild + meta (fuer die Karten der Tutorials), bloecke.
Bausteine siehe gen_handbuch.py (bloecke_html).
"""
from inhalt_basis import T
import inhalt_start
import inhalt_hilfe1  # Stufe H1: der Helfer und die Touren
import inhalt_hilfe2  # Stufe H2: neue Tutorials mit Tour
import inhalt_tutgeo  # rc610 TUT-GEO: Tutorial "Ein Modell zu Brushes machen"
import inhalt_modobj  # rc610 MODOBJ: Modell-Browser mit .obj, .fbx usw., Setzen wandelt in .md3
import inhalt_r2a  # rc610 R2a: Gras im Echtzeit-Licht, Spiegel mit allen Stufen, Nachfragen
import inhalt_funktionen
import inhalt_skript
import inhalt_karte
import inhalt_karteneditor
import inhalt_karteneditor2
import inhalt_kartebauen
import inhalt_karte4
import inhalt_bspmerge  # rc609 B1: Brush-Entities in fertige Karten einfuegen
import inhalt_karte5
import inhalt_karte6
import inhalt_karte7
import inhalt_karte8
import inhalt_karte8b
import inhalt_karte8c
import inhalt_karte8g
import inhalt_karte9
import inhalt_menue
import inhalt_menue3
import inhalt_menue5
import inhalt_menue5b
import inhalt_dock  # dock1: Fenster frei anordnen, mehrere Bildschirme
import inhalt_menue6
import inhalt_menue7
import inhalt_mission
import inhalt_tutorials
import inhalt_kurz
import inhalt_kurz2
import inhalt_kurz3
import inhalt_referenz
import inhalt_befehle
import inhalt_knoepfe
import inhalt_befehlseiten

GRUPPEN = [
    ("start", T("Start here", "Einstieg", "入门", "はじめに")),
    ("skript", T("Scripts", "Skripte", "脚本", "スクリプト")),
    ("karte", T("Map and scene", "Karte und Szene", "地图与场景", "マップとシーン")),
    ("mission", T("Missions", "Missionen", "任务", "ミッション")),
    ("tut", T("Tutorials", "Tutorials", "教程", "チュートリアル")),
] + inhalt_kurz.GRUPPEN + [
    ("ref", T("Reference", "Nachschlagen", "参考", "リファレンス")),
    ("befehle", T("Commands", "Befehle", "命令", "コマンド")),
]

inhalt_tutgeo.einhaengen(inhalt_karte9.KARTE_WERKZEUGE9, inhalt_tutorials.UEBERSICHT)
inhalt_modobj.einhaengen(inhalt_karteneditor2.BAUEN)
inhalt_r2a.einhaengen(inhalt_karte7.KARTE_TEMPO, inhalt_karteneditor.KARTENEDITOR, inhalt_bspmerge.BSPMERGE)

SEITEN = (inhalt_start.SEITEN[:1] + inhalt_hilfe1.SEITEN + inhalt_funktionen.SEITEN + inhalt_start.SEITEN[1:] + [inhalt_skript.FENSTER] + inhalt_dock.SEITEN + inhalt_skript.SEITEN + inhalt_karte.SEITEN + inhalt_karteneditor.SEITEN + inhalt_karteneditor2.SEITEN + inhalt_kartebauen.SEITEN + inhalt_karte4.SEITEN + inhalt_bspmerge.SEITEN + inhalt_karte5.SEITEN + inhalt_karte6.SEITEN + inhalt_karte7.SEITEN + inhalt_karte8.SEITEN + inhalt_karte8b.SEITEN + inhalt_karte8g.SEITEN + inhalt_karte9.SEITEN + inhalt_menue.SEITEN + inhalt_menue3.SEITEN + inhalt_menue5.SEITEN + inhalt_menue5b.SEITEN + inhalt_menue6.SEITEN + inhalt_menue7.SEITEN +
          inhalt_mission.SEITEN + inhalt_tutorials.SEITEN + inhalt_karte8c.SEITEN + inhalt_hilfe2.SEITEN + [inhalt_tutgeo.seite()] + inhalt_kurz.SEITEN + inhalt_kurz2.SEITEN + inhalt_kurz3.SEITEN + inhalt_referenz.SEITEN + inhalt_knoepfe.SEITEN + inhalt_befehle.SEITEN + inhalt_befehlseiten.SEITEN)

import inhalt_schritte
inhalt_schritte.anwenden(SEITEN)

_ids = [s["id"] for s in SEITEN]
assert len(_ids) == len(set(_ids)), "doppelte Seiten-id"
_gruppen = {g for g, _ in GRUPPEN}
for _s in SEITEN:
    assert _s["gruppe"] in _gruppen, _s["id"]
