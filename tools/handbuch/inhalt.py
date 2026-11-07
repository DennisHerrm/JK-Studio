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
import inhalt_aseb  # ASEB: .ase, .md3 usw. zu Brushes, Rechtsklick "Modell in Brushes umwandeln"
import inhalt_r2a  # rc610 R2a: Gras im Echtzeit-Licht, Spiegel mit allen Stufen, Nachfragen
import inhalt_r3  # rc610 R3: weitere Stufen und Glow im Echtzeit-Licht, gespiegelte Modelle
import inhalt_mb1a1  # Modell-Block 1a-1: Model-Tab, Ansehen/Pruefen wie ModView, Bedienung wie 3ds Max
import inhalt_mb1a2  # Modell-Block 1a-2: Weights, Bedienung wie 3ds Max oder Blender, Ins Skript
import inhalt_mb2  # Modell-Block Stufe 2: Speichern, GLA, Karten-GLA
import inhalt_mb3  # Modell-Block Stufe 3: Control Rig, Posen, Ghosts, Pfade
import inhalt_mb3b  # Modell-Block Stufe 3b: Dopesheet, Kurveneditor, Animationswerkzeuge
import inhalt_mb4  # Modell-Block Stufe 4: IK/FK, Fussrolle, Pfade, Raumwechsel
import inhalt_mb7  # Modell-Block Stufe 7: Figuren im Mission-Tab animieren
import inhalt_ph  # Modell-Block Stufe 6: Auto-Physik
import inhalt_rt  # Modell-Block Stufe 5: Retargeting
import inhalt_uv1b  # Modell-Block 1b: UV erstellen
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
inhalt_aseb.einhaengen(inhalt_karte9.KARTE_WERKZEUGE9)
inhalt_modobj.einhaengen(inhalt_karteneditor2.BAUEN)
inhalt_r2a.einhaengen(inhalt_karte7.KARTE_TEMPO, inhalt_karteneditor.KARTENEDITOR, inhalt_bspmerge.BSPMERGE)
inhalt_r3.einhaengen(inhalt_karte7.KARTE_TEMPO)

SEITEN = (inhalt_start.SEITEN[:1] + inhalt_hilfe1.SEITEN + inhalt_funktionen.SEITEN + inhalt_start.SEITEN[1:] + [inhalt_skript.FENSTER] + inhalt_dock.SEITEN + inhalt_skript.SEITEN + inhalt_karte.SEITEN + inhalt_karteneditor.SEITEN + inhalt_karteneditor2.SEITEN + inhalt_kartebauen.SEITEN + inhalt_karte4.SEITEN + inhalt_bspmerge.SEITEN + inhalt_karte5.SEITEN + inhalt_karte6.SEITEN + inhalt_karte7.SEITEN + inhalt_karte8.SEITEN + inhalt_karte8b.SEITEN + inhalt_karte8g.SEITEN + inhalt_karte9.SEITEN + inhalt_menue.SEITEN + inhalt_menue3.SEITEN + inhalt_menue5.SEITEN + inhalt_menue5b.SEITEN + inhalt_menue6.SEITEN + inhalt_menue7.SEITEN +
          inhalt_mission.SEITEN + [inhalt_mb1a1.seite()] + inhalt_mb1a2.seiten() + [inhalt_mb2.seite()] + [inhalt_mb3.seite()] + [inhalt_mb3b.seite()] + [inhalt_mb4.seite()] + [inhalt_ph.seite()] + [inhalt_rt.seite()] + [inhalt_uv1b.seite()] + [inhalt_mb7.seite()] + inhalt_tutorials.SEITEN + inhalt_karte8c.SEITEN + inhalt_hilfe2.SEITEN + [inhalt_tutgeo.seite()] + inhalt_kurz.SEITEN + inhalt_kurz2.SEITEN + inhalt_kurz3.SEITEN + inhalt_referenz.SEITEN + inhalt_knoepfe.SEITEN + inhalt_befehle.SEITEN + inhalt_befehlseiten.SEITEN)

import inhalt_schritte
inhalt_schritte.anwenden(SEITEN)

_ids = [s["id"] for s in SEITEN]
assert len(_ids) == len(set(_ids)), "doppelte Seiten-id"
_gruppen = {g for g, _ in GRUPPEN}
for _s in SEITEN:
    assert _s["gruppe"] in _gruppen, _s["id"]
