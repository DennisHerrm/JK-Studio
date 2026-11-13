#!/usr/bin/env python3
"""Eine volle Runde der Selbsttests mit EINEM Befehl (TESTTEMPO, rc612; docs/intern/tests_tempo.md).

    python -I tools/testrunde.py                      volle Runde (englisch, ohne Doppel), 6 Laeufe gleichzeitig
    python -I tools/testrunde.py --mit-sprachen       dazu die Wiederholungen in es/cs/ja (meldet die Community
                                                      einen Fehler in einer Sprache)
    python -I tools/testrunde.py --liste              nur zeigen, was liefe (und was als Doppel wegbleibt)
    python -I tools/testrunde.py mb_mb5 kuerzel       nur diese Laeufe
    python -I tools/testrunde.py --betroffen karte1   nur, was der Unterschied <basis>..HEAD beruehrt
                                                      (tools/test_betroffen.py)
    python -I tools/testrunde.py --pruefe-doppel      belegt aus den Protokollen einer Runde MIT Doppeln
                                                      (--mit-doppelten), dass jedes Doppel enthalten ist
    python -I tools/testrunde.py --namen vorher.txt   Pruefzeilen aller Laeufe als Liste (Vergleich vorher/nachher)
    python -I tools/testrunde.py --vergleich a.txt b.txt
    python -I tools/testrunde.py --pruefe-ports       zwei Runden nebeneinander bekommen nie denselben Portbereich der
                                                      Bau-Ueberwachung (ctest testrunde_ports)

Jeder Lauf bekommt einen eigenen Ordner (Kopie von jkstudio.exe, bhed_q3map2.exe und data, eigene
Einstellungen), laeuft mit normaler Prioritaet (--leerlauf: wie frueher), wird ueber 8 GB oder nach seiner Zeitgrenze beendet und
gilt als HAENGT, wenn er 3 Minuten lang weder ins Protokoll schreibt noch rechnet (dann wartet er auf
etwas, das nicht kommt). Am Ende steht EINE Zeile je Lauf in <laeufe>/<praefix>summe.txt, die
FEHLER-Zeilen darunter.

Laeufe mit `pausen="bild"` in der Tabelle laufen mit BHED_TEST_PAUSEN=bild (Pausen nach Bildern statt
Wanduhr, nachgewiesen gleiche Pruefzeilen). Wird ein solcher Lauf rot, wiederholt ihn die Runde sofort
nach der Wanduhr; es zaehlt die Wiederholung, und die Zeile nennt, was nur nach Bildern rot war.

Lange Laeufe fahren in Teilen nebeneinander, zusammen mit denselben Pruefzeilen wie der eine Lauf:
der volle karteneditor als ke_* (BHED_KARTE_GRUPPEN, jeder Teil mit eigenem Portbereich der
Bau-Ueberwachung, BHED_BAU_PORT - systemweit reserviert, mehrere Runden nebeneinander stoeren sich nicht),
kartenpruefung als kartenpruefung_1v4 ... (BHED_TEST_ANTEIL=i/N).
Ein Teil beginnt mit frischem Programm: was eine Gruppe der naechsten hinterlaesst, wirkt ueber die
Teilgrenzen nicht mehr - --ungeteilt faehrt stattdessen die ganzen Laeufe (sie bleiben auch beim Namen
erreichbar); --pruefe-doppel belegt die Gleichheit der Pruefzeilen.

Die Orte der Testdaten dieses Rechners stehen unter ORTE und lassen sich per Umgebungsvariable
aendern (TR_SP, TR_S6, TR_TESTDATEN, TR_GROSS, TR_STEAM, TR_BELEGE, TR_CFG). Fehlt eine Datei, die ein
Lauf braucht, startet er nicht und zaehlt als ROT ("Testdaten fehlen") - er liefe sonst mit weniger
Pruefungen gruen durch.
"""

from __future__ import annotations

import argparse
import ctypes
import ctypes.wintypes
import glob
import os
import random
import re
import shutil
import socket
import subprocess
import sys
import time

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# --- ORTE -------------------------------------------------------------------------------------------------
SP = os.environ.get("TR_SP", r"Y:\werkzeuge\sp_af9c")              # Einstellungen, dekompilierte Karten
S6 = os.environ.get("TR_S6", r"Y:\werkzeuge\sp_654neu")            # Daten des Modell-Blocks
# Feste Orte ausserhalb der Laufordner (bis 11.11. lag TESTDATEN unter Y:\karte\laeufe und ging beim Aufraeumen der
# Laufordner verloren; Herkunft und Pruefsummen: Y:\werkzeuge\testdaten_HERKUNFT.md).
#   TESTDATEN  zz_JediTempleGarden_MD.pk3 (BHED_WE_PK3: wegerzeugung, we_taspir1, we_t1d, deckung, kampfpunkte, wegmalen -
#              der Ordner wird dort Spielordner, deshalb liegt sonst nichts darin) und bm_duel1.map (kemd)
#   GROSS_ORT  q3\duel_mos_espa.map und bsp\duel_mos_espa.bsp: die grosse Karte des karteneditor (Gruppen stufe7 und gross),
#              mit q3map2 -convert aus Movie Duels dekompiliert (bytegleiche Kopie aus sp_af9c\karte_dekomp\md)
TESTDATEN = os.environ.get("TR_TESTDATEN", r"Y:\werkzeuge\testdaten")
GROSS_ORT = os.environ.get("TR_GROSS", r"Y:\werkzeuge\testdaten_gross")
STEAM = os.environ.get("TR_STEAM", r"C:\Program Files (x86)\Steam\steamapps\common")
BELEGE = os.environ.get("TR_BELEGE", r"C:\Users\shank\Downloads\behaved-1_0_0-rc567\JKStudio_Uebergabe\plaene\modellblock_stufe0\belege")
CFG = os.environ.get("TR_CFG", os.path.join(SP, "behaved.cfg.sicher"))
MD =os.path.join(STEAM, "Jedi Academy", "GameData Movie Duels")
# JKA-Original: seit 11.11. wieder "GameData" (vorher "GameData original") - der Ordner, den es gibt
JKA = next((p for p in (os.path.join(STEAM, "Jedi Academy", "GameData", "base"), os.path.join(STEAM, "Jedi Academy", "GameData original", "base"))
            if os.path.isfile(os.path.join(p, "assets0.pk3"))), os.path.join(STEAM, "Jedi Academy", "GameData", "base"))

# Fuer alle Laeufe gesetzt: wo die JKA-Daten dieses Rechners liegen (TR_STEAM). Die Vorgaben der Selbsttests nennen seit
# GDPFAD denselben Ordner "GameData"; fehlt er, melden die JKA-Gruppen "uebersprungen (Daten fehlen)" - die Zeile des Laufs
# nennt die Zahl
UMGEBUNG = {
    "BHED_JKA_BASIS": JKA, "BHED_MB_JKA": JKA, "BHED_KEMD_JA": JKA.replace("\\", "/"), "BHED_JA_PFAD": os.path.dirname(JKA),
    "BHED_WE_PK3": os.path.join(TESTDATEN, "zz_JediTempleGarden_MD.pk3"),
}

# --- Die Laeufe ---------------------------------------------------------------------------------------------
# name, modus, sek (Zeitgrenze), var (BHED_*=wert; <LAUF> = Laufordner, <FOTOS> = sein Ordner fotos, <EIGEN> = ein Name,
# den nur dieser Lauf dieser Runde traegt), dazu
#   sprache   language= der Einstellungen (Vorgabe en)
#   cfg       andere Einstellungsdatei
#   sprachlauf True: wiederholt einen englischen Lauf in einer anderen Sprache - laeuft seit dem 11.11. nur mit
#             --mit-sprachen (Entscheidung shank: "eigentlich nicht wichtig ist erstmal englisch. wenn da was waere.
#             Muss man mir bescheid sagen von der community."). Hier wird weniger geprueft als bis rc611.
#             Laeufe, die eine andere Sprache BRAUCHEN (mb_ptfix2 mit ja, cjkweit), sind keine Sprachlaeufe.
#   behalten  Einstellungen des genannten Laufs uebernehmen (zweiter Start); laeuft nach ihm
#   vorlage   "<ordner>|<name>": Ordner in den Laufordner kopieren
#   sperre    Laeufe mit derselben Sperre nie gleichzeitig (seit KLEIN traegt sie kein Lauf mehr: jeder Lauf bekommt einen
#             eigenen, systemweit reservierten Bereich der Bau-Ueberwachung, BHED_BAU_PORT - auch gegen eine zweite Runde nebenan)
#   braucht   Dateien, ohne die der Lauf nicht startet (ROT "Testdaten fehlen")
#   befehl    statt jkstudio.exe (nur die Probe der Haengt-Erkennung)
#   doppel    (lauf, grund): jede Pruefzeile dieses Laufs steht auch in <lauf> - bleibt in der vollen Runde weg
#   allein    grund: laeuft nicht im Pool (eigene Vorbereitung, Messung, Bildlauf) - nur wenn genannt
#   bereich   Stichwort fuer tools/test_betroffen.py
#   pausen    "bild": BHED_TEST_PAUSEN=bild ist fuer diesen Lauf nachgewiesen (gleiche Pruefzeilen mit gleichem Stand
#             neben der Wanduhr, dazu ein Rot-Stand: docs/intern/NOTIZEN_testtempo_1111.md).
LAEUFE: list[dict] = []


def lauf(name: str, modus: str, sek: int = 2400, var: tuple | list = (), **mehr) -> None:
    LAEUFE.append(dict(name=name, modus=modus, sek=sek, var=list(var), **mehr))


GROSS_DATEIEN = [os.path.join(GROSS_ORT, "q3", "duel_mos_espa.map"), os.path.join(GROSS_ORT, "bsp", "duel_mos_espa.bsp")]
GROSS = ["BHED_FOTOS=<FOTOS>", "BHED_KARTE_GROSS=" + GROSS_DATEIEN[0], "BHED_KARTE_GROSS_BSP=" + GROSS_DATEIEN[1]]
MISSION = ["BHED_MISSION_ARCHIV=" + os.path.join(MD, "MD", "MD_Missions_Ep3.pk3"), "BHED_MISSION_NAME=md_am_sith"]
MB = ["BHED_RT_GLA=" + S6 + r"\rt\gla", "BHED_MB_BELEGE=" + BELEGE, "BHED_MB7_BSP=" + S6 + r"\mb7\karte\mb7raum.bsp"]

# Die langen zuerst (der Pool nimmt in dieser Reihenfolge)
# Der volle karteneditor in Teilen nebeneinander: jede Gruppe des vollen Laufs (gui/selbsttest_handbuch.inc, gruppe("...")) steht
# in genau einem Teil; zusammen dieselben Pruefzeilen (docs/intern/NOTIZEN_testtempo_1111.md). Was zusammenbleibt: die Gruppen
# der Stufe 1 (bauen aufeinander auf), stufe8c + stufe8g + stufe9 (die gebaute Karte). Jeder Teil baut mit eigenem Portbereich -
# seit KLEIN (rc612) auch ke_bau: Stufe 3 prueft die Adresse der Instanz (s3Adresse) und NRCs Vorgabe 39000 an der Rechnung
# selbst (ke::bauMonitorPortAus), so stoeren sich zwei Runden nebeneinander nicht mehr ("Port nicht belegt").
KE_TEILE = {   # die langen zuerst (Zeiten mit Pausen nach Bildern am 11.11.: 540, 337, 300, 256, 217, 140 s; der ganze Lauf 1797 s)
    "ke_stufe8": "stufe8",
    "ke_befehle": "befehle,stufe6",
    "ke_licht": "stufe7,stufe8b,gross",
    "ke_bau": "stufe3,stufe4,stufe5",
    "ke_karte": "stufe8c,stufe8g,stufe9,stufe9c,stufe9d,licht1,modobj,aseb,kreste,r2a,r3,rd,rest8,reste1,fotos",
    "ke_grund": "rahmen,tasten,numpad,mass,xy,kamera,auswahl,textur,entitaeten,konsole,stufe2",
}
for name_, gruppen_ in KE_TEILE.items():
    lauf(name_, "karteneditor", 5400, GROSS + ["BHED_KARTE_GRUPPEN=" + gruppen_], bereich="karteneditor", teil_von="karteneditor", pausen="bild",
         **({"braucht": GROSS_DATEIEN} if {"stufe7", "gross"} & set(gruppen_.split(",")) else {}))
lauf("karteneditor", "karteneditor", 9000, GROSS, bereich="karteneditor", braucht=GROSS_DATEIEN, pausen="bild",
     doppel=("+".join(KE_TEILE), "laeuft in Teilen nebeneinander (BHED_KARTE_GRUPPEN)"))
lauf("karteneditor_es", "karteneditor", 9000, GROSS, sprache="es", bereich="karteneditor", sprachlauf=True, braucht=GROSS_DATEIEN)
lauf("karteneditor_cs", "karteneditor", 9000, GROSS, sprache="cs", bereich="karteneditor", sprachlauf=True, braucht=GROSS_DATEIEN)
lauf("menueeditor", "menueeditor", 5400, bereich="menue", pausen="bild")
lauf("menueeditor_ja", "menueeditor", 5400, cfg=SP + r"\rc608\nur_ja.cfg", bereich="menue", sprachlauf=True)
lauf("menueeditor_es", "menueeditor", 5400, sprache="es", bereich="menue", sprachlauf=True)
lauf("menueeditor_cs", "menueeditor", 5400, sprache="cs", bereich="menue", sprachlauf=True)
# mission1 bleibt EIN Lauf: in Teilen (BHED_M1_TEIL) lief "jka zwischen gizmo" anders als im ganzen Lauf ("Gizmo: Kamera ... nach
# Stop wie vorher" rot, im ganzen Lauf gruen) - die Abschnitte haengen an ihrer Reihenfolge (NOTIZEN_testtempo_1111.md, R3)
lauf("mission1", "mission1", 3600, ["BHED_FOTOS=<FOTOS>"], bereich="vorschau")
lauf("kartenpruefung", "kartenpruefung", 3600, bereich="vorschau")
lauf("allekarten", "allekarten", 3600, bereich="vorschau")
# Modus 1 laeuft seit Runde 2 mit Pausen nach Bildern: die Stelle am Tutorial-Fenster wartet jetzt auf den Zustand (sie hing nicht
# an der Pausenart), nachgewiesen gruen und gleich in beiden Arten und rot mit denselben Zeilen (NOTIZEN_testtempo_1111.md)
lauf("1", "1", 3600, bereich="programm", pausen="bild")
lauf("1_es", "1", 3600, sprache="es", bereich="programm", sprachlauf=True)
lauf("1_cs", "1", 3600, sprache="cs", bereich="programm", sprachlauf=True)
# "mp" und "sp" standen bis rc611 in der Runde, sind aber keine Modi: jeder unbekannte Wert von BHED_EDITORTEST
# faellt auf den Hauptlauf (selbsttest.cpp, "Jeder andere Wert: alles") - zweimal Modus 1, 2629 von 2629 Pruefzeilen gleich
lauf("mp", "mp", 3600, doppel=("1", "kein eigener Modus: faellt auf den Hauptlauf"), bereich="programm")
lauf("sp", "sp", 3600, doppel=("1", "kein eigener Modus: faellt auf den Hauptlauf"), bereich="programm")
lauf("we_taspir1", "wegerzeugung", 3600, ["BHED_WE_KARTE=taspir1", "BHED_WE_ENT=<LAUF>/we.ent"], bereich="vorschau")
lauf("we_jtg", "wegerzeugung", 3600, ["BHED_WE_KARTE=jeditemple_garden", "BHED_WE_PK3=<LAUF>/jtg/zz_JediTempleGarden_MD.pk3", "BHED_WE_ENT=<LAUF>/we.ent"],
     vorlage=S6 + r"\r2b\jtg|jtg", bereich="vorschau")
lauf("we_t1d", "wegerzeugung", 3600, ["BHED_WE_KARTE=t1_danger", "BHED_WE_ENT=<LAUF>/we.ent"], bereich="vorschau")
for m in ("hilfe1", "hilfe2", "tutgeo", "ruhen"):
    lauf(m, "karteneditor", 3600, ["BHED_KARTE_NUR=" + m, "BHED_FOTOS=<FOTOS>"], bereich="karteneditor")
for m in ("licht1", "modobj", "r2a"):
    lauf(m, "karteneditor", 3600, ["BHED_KARTE_NUR=" + m, "BHED_FOTOS=<FOTOS>"], bereich="karteneditor",
         doppel=("karteneditor", "Teil des vollen Laufs (BHED_KARTE_NUR waehlt ihn nur aus)"))
for sp in ("es", "cs"):
    for m in ("hilfe1", "hilfe2", "tutgeo"):
        lauf(m + "_" + sp, "karteneditor", 3600, ["BHED_KARTE_NUR=" + m], sprache=sp, bereich="karteneditor", sprachlauf=True)
    lauf("kuerzel_" + sp, "kuerzel", 2400, sprache=sp, bereich="programm", sprachlauf=True,
         doppel=("1_" + sp, "Schrittlisten stehen unveraendert im Hauptlauf"))
    lauf("zeitleiste1_" + sp, "zeitleiste1", 2400, sprache=sp, bereich="vorschau", sprachlauf=True)
for sp in ("ja", "zh-Hans"):
    lauf("cjkweit_" + sp[:2], "karteneditor", 1500, ["BHED_KARTE_NUR=cjkweit"], sprache=sp, bereich="karteneditor")
lauf("optsicher", "karteneditor", 3600, ["BHED_KARTE_NUR=optsicher", "BHED_OPTSICHER_KARTEN=" + SP + r"\s9c\taris\md_rev_TarisUpper2.bsp"], bereich="karteneditor")
lauf("kemd", "kemd", 1500, ["BHED_KEMD_NUR=shader", "BHED_KEMD_KARTE=" + os.path.join(TESTDATEN, "bm_duel1.map")], bereich="karteneditor")
lauf("wegerzeugung", "wegerzeugung", 1500, ["BHED_WE_SOLID=-320 1920 328"], bereich="vorschau")
lauf("editorportal", "editorportal", bereich="karteneditor")
lauf("editorportal_tester", "editorportal", 1500, ["BHED_P_NUR=tester", "BHED_FOTOS=<FOTOS>"], bereich="karteneditor",
     doppel=("editorportal", "Teil des vollen Laufs (BHED_P_NUR waehlt ihn nur aus)"))
for m in ("ablauf", "umriss", "missionfoto", "letzte"):
    lauf(m, m, 1500, MISSION, bereich="vorschau")
lauf("zeitleiste0", "zeitleiste0", 1500, ["BHED_ZL0_AUS=<LAUF>/zl0aus"], bereich="vorschau")

# Modell-Block: je Gruppe ein Lauf (BHED_MODELL_NUR), die Rundlaeufe je Spiel
for g in ("funktionen tasten mb2 blender haut mb3 rig mb5 mb6 uv uvmd3 md mdkarte mb7 ph rt rtjka pr pt ptfix ptfix3 ptstufen ptsprung ai "
          "pfix pfix2 pfixkarte").split():
    lauf("mb_" + g, "modellblock", 2400, MB + ["BHED_MODELL_NUR=" + g, "BHED_POSEN=<FOTOS>", "BHED_FOTOS=<FOTOS>"], bereich="modell",
         **({"pausen": "bild"} if g in ("funktionen", "tasten", "mb2", "mb5") else {}))
lauf("mb_ohnenur", "modellblock", 2400, MB, bereich="modell", doppel=("mb_funktionen+mb_tasten", "ohne BHED_MODELL_NUR laufen genau funktionen und tasten"))
lauf("mb_ptfix2", "modellblock", 2400, MB + ["BHED_MODELL_NUR=ptfix2", "BHED_FOTOS=<FOTOS>"], sprache="ja", bereich="modell")
lauf("mb_mb7spiel", "modellblock", 2400, MB + ["BHED_MODELL_NUR=mb7spiel", "BHED_FIGUR_BAU=<LAUF>/bau"], bereich="modell")
lauf("mb_mb7textur", "modellblock", 1200, MB + ["BHED_MODELL_NUR=mb7textur", "BHED_MB7_LEER=" + S6 + r"\mb7\raum_licht.bsp"], bereich="modell")
for s in ("md", "jka", "jk2"):
    for g, k in (("uvrundlauf", "uvrl"), ("rundlauf", "rl"), ("rigrundlauf", "rrl"), ("kurvenrundlauf", "kr"), ("ikrundlauf", "irl"), ("bild", "bild"),
                 ("bild2", "bild2")):
        lauf("mb_%s_%s" % (k, s), "modellblock", 3000, MB + ["BHED_MODELL_NUR=" + g, "BHED_MB_SPIEL=" + s], bereich="modell")

# Teilmodi, deren Schrittlisten der groessere Modus unveraendert enthaelt (selbsttest.cpp, Abschnitt "a.teil == 0"):
# zum gezielten Wiederholen gedacht, in der vollen Runde doppelt
for m in ("kuerzel", "ziehen", "fenster", "zeitleiste", "ton", "szenen", "geteilt", "lesezeichen"):
    lauf(m, m, bereich="programm", doppel=("1", "Schrittlisten stehen unveraendert im Hauptlauf"))
lauf("kartenfunde", "kartenfunde", bereich="vorschau", doppel=("gegenstaende", "derselbe Modus unter zweitem Namen (nurGegenstaende)"))
for m in ("fix604b", "geschuetzanim", "zielstrahl", "effektgrund", "figurdeform"):
    lauf(m, m, bereich="vorschau", doppel=("gegenstaende", "fix604bSchritte laufen im Modus gegenstaende mit"))
lauf("figurstufen", "figurstufen", bereich="vorschau", doppel=("gegenstaende", "figurStufenSchritte laufen im Modus gegenstaende mit"))
for m in ("kamera", "gluehen", "npcklasse"):
    lauf(m, m, bereich="vorschau", doppel=("karte", "Schrittliste laeuft im Modus karte mit"))

BEREICH = {
    "programm": "ablage original eigenstaendig einzel icons pruefung reiter vergleich f78 fundef spielordnerfehlt",
    "vorschau": """figurleistung figurlicht gegenstaende geschuetze gras holo jk2 karte lava mission
        missionsliste modellimfesten laserfalle ort pruefung3 rest605 roff spulen teil10 teil10b teil11 teil13 teil13b teil13c teil13d teil6 teil9
        tuerlauf wegpunkte zeitleiste1 deckung kampfpunkte modellsetzen wegmalen wegpunkte2 enttext dateiort""",
    "karteneditor": "bspmerge kartemission lumpedit patchlicht texturliste",
    "modell": "modell modellleistung",
    "dock": "dock1 dock2mass dock4 dock5 tabfenster",
    "update": "updateleiste",
    "handbuch": "handbuch",
}
# einzel (ein zweiter Start reicht an den laufenden weiter) prueft nur mit BHED_EINZEL=<Name>: ohne den Namen nimmt sich der
# Selbsttest von der Einzelinstanz aus, der zweite Start laeuft dann als eigenes Programm weiter und alle sechs Pruefungen sind
# rot (so in der Runde rc611 und am 11.11.). Der Name ist je Lauf eigen, damit zwei Runden nebeneinander sich nicht treffen.
EIGENE_VAR = {"einzel": ["BHED_EINZEL=<EIGEN>"]}
# Runde 2: nach den Messfenstern in karte ("Ruhe: ...") und der Bedingung am Tutorial-Fenster (reiter) nachgewiesen
NACH_BILDERN = {"karte", "reiter"}
for bereich_, namen_ in BEREICH.items():
    for m in namen_.split():
        lauf(m, m, var=EIGENE_VAR.get(m, ()), bereich=bereich_, **({"pausen": "bild"} if m in NACH_BILDERN else {}))
lauf("menueloeschen", "menueloeschen", bereich="menue", pausen="bild")
lauf("missionsbau", "missionsbau", bereich="vorschau", pausen="bild")
lauf("dock1neustart", "dock1neustart", 1500, behalten="dock1", bereich="dock")
lauf("tabfensterneustart", "tabfensterneustart", 1500, behalten="tabfenster", bereich="dock")
lauf("updateleiste2", "updateleiste2", 600, behalten="updateleiste", bereich="update")

# Zusammengefuehrter Stand rc612: die Gruppen und Modi der Zweige, die neben TESTTEMPO entstanden (dessen Tabelle kennt den
# Stand a00f6cf90). Kopfzeile/ViewCube (kopfrot, kopf - mit den zwei Zeilen von kopfab am Ende), Boden-Raster, Navigation,
# Bearbeiten in der Pose, Szenen-Explorer, Auto-Skin, Zeitleiste; der Modus uiform (Formular-Baustein, Mindestgroessen). Die
# Gruppe numpad des Karten-Editors und die Malgruppe ptsprung stehen schon oben (voller karteneditor-Lauf, mb_ptsprung). Nicht
# im Pool: Bildlaeufe (rasterfotos, ap1foto, ap2foto, ap12foto, ap3foto, mbefoto) und die Messung ptlang. ap3: Rollout-Baustein und
# rechtes Feld des Model-Tabs (AP3). ap6: Bereich Modellieren nach dem Entwurf (AP6; Bildlaeufe ap6foto, ap6hb nicht im Pool).
# ap8: der Bereich Weights nach dem Entwurf (AP8; Bildlauf ap8foto nicht im Pool).
for g in "kopfrot kopf raster nav mbv ap2 ap12 ap1 ap3 ap6 ap8".split():
    lauf("mb_" + g, "modellblock", 2400, MB + ["BHED_MODELL_NUR=" + g, "BHED_POSEN=<FOTOS>", "BHED_FOTOS=<FOTOS>"], bereich="modell")
# AP10: das rechte Feld im Bereich Animation nach dem Entwurf (nicht im Pool: der Bildlauf ap10foto)
lauf("mb_ap10", "modellblock", 2400, MB + ["BHED_MODELL_NUR=ap10", "BHED_POSEN=<FOTOS>", "BHED_FOTOS=<FOTOS>"], bereich="modell")
# ENTSCHEID2: Control Rig wie CAT (FK -> IK beim Erstellen, IK/FK ab dem Bild), acht Preset-Kategorien, Generator-Gaenge in der Liste
lauf("mb_ent2", "modellblock", 1800, MB + ["BHED_MODELL_NUR=ent2", "BHED_FOTOS=<FOTOS>"], bereich="modell")
# AP4: der Model-Tab im Bedienmodus wie Blender - Werkzeugleiste, Outliner oben rechts, Seitenleiste (nicht im Pool: der Bildlauf ap4foto)
lauf("mb_ap4", "modellblock", 2400, MB + ["BHED_MODELL_NUR=ap4", "BHED_POSEN=<FOTOS>", "BHED_FOTOS=<FOTOS>"], bereich="modell")
# AP5: der Bereich Ansehen/Pruefen vollstaendig nach dem Entwurf (nicht im Pool: der Bildlauf ap5foto)
lauf("mb_ap5", "modellblock", 2400, MB + ["BHED_MODELL_NUR=ap5", "BHED_POSEN=<FOTOS>", "BHED_FOTOS=<FOTOS>"], bereich="modell")
# AP13S: der Modifier-Stapel (wie 3ds Max und wie Blender), Vorschau = angewendet, Speichern mit leerem Stapel (nicht im Pool: die
# Bildlaeufe ap13sfoto, ap13shb)
lauf("mb_ap13s", "modellblock", 2400, MB + ["BHED_MODELL_NUR=ap13s", "BHED_POSEN=<FOTOS>", "BHED_FOTOS=<FOTOS>"], bereich="modell")
# AP14: Zusammenfuehrung des Model-Tabs bei Mindestgroesse - wertTabelle ohne Bruch im Wort, Zeitleiste, Suche ohne gleiche Zeilen,
# Wuensche an den Rollout-Baustein (nicht im Pool: der Bildlauf ap14foto)
lauf("mb_ap14", "modellblock", 1800, MB + ["BHED_MODELL_NUR=ap14", "BHED_FOTOS=<FOTOS>"], bereich="modell")
lauf("uiform", "uiform", 3600, bereich="programm")
# AP9: das rechte Feld des Bereichs Painting nach dem Entwurf (Gruppen Ziel, Werkzeuge, Pinsel, Ebenen, Fuer das Spiel; Tasten [ ]).
# Nicht im Pool: der Bildlauf ap9foto.
lauf("mb_ap9", "modellblock", 2400, MB + ["BHED_MODELL_NUR=ap9", "BHED_POSEN=<FOTOS>", "BHED_FOTOS=<FOTOS>"], bereich="modell")
# AP7: das rechte Feld des Bereichs UV nach dem Entwurf (Auswahl, Schritte 1-3, Pruefen; nicht im Pool: der Bildlauf ap7foto)
lauf("mb_ap7", "modellblock", 2400, MB + ["BHED_MODELL_NUR=ap7", "BHED_POSEN=<FOTOS>", "BHED_FOTOS=<FOTOS>"], bereich="modell")
# AP13K: Knochen-Darstellung (R14), Durchsicht, Knochen im Vordergrund, Rig-Stile (nicht im Pool: der Bildlauf ap13kfoto)
lauf("mb_ap13k", "modellblock", 1800, MB + ["BHED_MODELL_NUR=ap13k", "BHED_FOTOS=<FOTOS>"], bereich="modell")
# KLEIN: die Art je Werkzeug (Extrudieren/Einfuegen), Eingefrorenes im Treffertest der Unterobjekte, der Kamera-Knopf des Gizmos, die
# Meldungen des Rigs in der Zaehlung der Zeitleiste, das About-Fenster bei Schrift 24; "Als Home setzen" bleibt - der zweite Start
# (kleinneustart) prueft es mit den Einstellungen des ersten
lauf("mb_klein", "modellblock", 2400, MB + ["BHED_MODELL_NUR=klein", "BHED_POSEN=<FOTOS>", "BHED_FOTOS=<FOTOS>"], bereich="modell")
lauf("mb_kleinneustart", "modellblock", 1500, MB + ["BHED_MODELL_NUR=kleinneustart", "BHED_FOTOS=<FOTOS>"], behalten="mb_klein", bereich="modell")
# AP11: mehrere Modelle in Reitern ueber der Ansicht, je Modell bleibt alles, Schliessen/Beenden fragen je Modell, Absturzrichtungen
# (schnell wechseln, Fenster offen, Strich offen), Kopieren/Einfuegen von Flaechen zwischen Modellen, In neuem Tab oeffnen (nicht im
# Pool: die Bildlaeufe ap11foto, ap11hb)
lauf("mb_ap11", "modellblock", 2400, MB + ["BHED_MODELL_NUR=ap11", "BHED_FOTOS=<FOTOS>"], bereich="modell")
# KLEIN2: Esc schliesst jedes Rechtsklick-Menue des Model-Tabs (Explorer, Zeitleiste, Dopesheet, Befehlsfeld, Werkzeugleiste, Ansicht,
# Menueleiste) und tut sonst nichts; Untermenues, die unten nicht passen, gehen nach oben auf und bleiben im Fenster
lauf("mb_klein2", "modellblock", 1500, MB + ["BHED_MODELL_NUR=klein2"], bereich="modell")

# Nicht im Pool: eigene Vorbereitung, Messungen (allein und mit BHED_D3D_DEBUG=0), Bildlaeufe fuer das Handbuch
lauf("update", "update", 1500, allein="braucht den lokalen Update-Server (tools/karte/updater_uebergang.py, ctest updater_uebergang)", bereich="update")
lauf("umzug", "umzug", 600, allein="braucht einen vorbereiteten %APPDATA%-Ersatz (zwei Starts)", bereich="update")
for m in ("leistung", "tabfensterleistung"):
    lauf(m, m, 3600, ["BHED_D3D_DEBUG=0"], allein="Bildzeit-Messung: allein im Messfenster", bereich="vorschau")
for sp in ("en", "de", "zh-Hans", "ja", "es", "cs"):
    lauf("bm_" + sp[:2], "menueeditor", 3600, ["BHED_MENUE_BILDER=1", "BHED_FOTOS=<FOTOS>"], sprache=sp, bereich="menue",
         allein="Handbuchbilder des Menue-Editors (2560x1500); die Pruefzeilen stehen alle im Lauf menueeditor")
# Probe der Haengt-Erkennung: ein Befehl, der nichts schreibt und nicht rechnet, mit einem Kind. Erwartet (mit --haengt 20):
# "HAENGT" nach 20 s, beide Prozesse beendet.
lauf("probe_haengt", "-", 600, allein="Probe der Haengt-Erkennung, nur beim Namen (mit --haengt 20)",
     befehl=[sys.executable, "-I", "-c", "import subprocess, sys, time; "
             "subprocess.Popen([sys.executable, '-I', '-c', 'import time; time.sleep(3600)  # tr_probe_kind']); time.sleep(3600)  # tr_probe_eltern"])


def geteilt(name: str, n: int) -> None:
    """Den Lauf in n Anteilen nebeneinander (<name>_1vN ...): jeder nimmt mit BHED_TEST_ANTEIL=i/n den i-ten von n
    zusammenhaengenden Abschnitten der Liste (Karten, Modelle), zusammen dieselben Eintraege. Der ungeteilte bleibt beim Namen."""
    a = next(x for x in LAEUFE if x["name"] == name)
    teile = [dict(a, name="%s_%dv%d" % (name, i, n), var=a["var"] + ["BHED_TEST_ANTEIL=%d/%d" % (i, n)], teil_von=name) for i in range(1, n + 1)]
    ort = LAEUFE.index(a)
    LAEUFE[ort:ort] = teile
    a["doppel"] = ("+".join(b["name"] for b in teile), "laeuft in %d Anteilen nebeneinander (BHED_TEST_ANTEIL)" % n)


geteilt("kartenpruefung", 4)
# Die Rundlaeufe des Modell-Blocks (bild, bild2, kurvenrundlauf, ikrundlauf, rigrundlauf) kennen BHED_TEST_ANTEIL ebenso, laufen
# hier aber ungeteilt: der Nachweis (ganz gegen Anteile) steht nur fuer kartenpruefung (docs/intern/NOTIZEN_testtempo_1111.md).


# --- Ein Lauf -----------------------------------------------------------------------------------------------
class _Zaehler(ctypes.Structure):
    _fields_ = [("cb", ctypes.wintypes.DWORD), ("PageFaultCount", ctypes.wintypes.DWORD), ("PeakWorkingSetSize", ctypes.c_size_t),
                ("WorkingSetSize", ctypes.c_size_t), ("QuotaPeakPagedPoolUsage", ctypes.c_size_t), ("QuotaPagedPoolUsage", ctypes.c_size_t),
                ("QuotaPeakNonPagedPoolUsage", ctypes.c_size_t), ("QuotaNonPagedPoolUsage", ctypes.c_size_t), ("PagefileUsage", ctypes.c_size_t),
                ("PeakPagefileUsage", ctypes.c_size_t)]


def _speicher_und_cpu(p: subprocess.Popen) -> tuple[int, float]:
    """Arbeitsspeicher (Bytes) und verbrauchte Rechenzeit (s) des Prozesses; (0, -1) wenn nicht lesbar."""
    h = ctypes.wintypes.HANDLE(int(p._handle))
    z = _Zaehler()
    z.cb = ctypes.sizeof(z)
    speicher = z.WorkingSetSize if ctypes.windll.psapi.GetProcessMemoryInfo(h, ctypes.byref(z), z.cb) else 0
    t = [ctypes.wintypes.FILETIME() for _ in range(4)]
    if not ctypes.windll.kernel32.GetProcessTimes(h, *[ctypes.byref(x) for x in t]):
        return speicher, -1.0
    cpu = sum(((x.dwHighDateTime << 32) | x.dwLowDateTime) for x in t[2:]) / 1e7
    return speicher, cpu


class _Eintrag(ctypes.Structure):   # PROCESSENTRY32W
    _fields_ = [("dwSize", ctypes.wintypes.DWORD), ("cntUsage", ctypes.wintypes.DWORD), ("th32ProcessID", ctypes.wintypes.DWORD),
                ("th32DefaultHeapID", ctypes.c_size_t), ("th32ModuleID", ctypes.wintypes.DWORD), ("cntThreads", ctypes.wintypes.DWORD),
                ("th32ParentProcessID", ctypes.wintypes.DWORD), ("pcPriClassBase", ctypes.c_long), ("dwFlags", ctypes.wintypes.DWORD),
                ("szExeFile", ctypes.c_wchar * 260)]


def _beginn(pid: int) -> int:
    """Wann der Prozess entstand (FILETIME als Zahl); 0 wenn nicht lesbar."""
    k = ctypes.windll.kernel32
    k.OpenProcess.restype = ctypes.wintypes.HANDLE
    h = k.OpenProcess(0x1000, False, pid)   # PROCESS_QUERY_LIMITED_INFORMATION
    if not h:
        return 0
    t = [ctypes.wintypes.FILETIME() for _ in range(4)]
    ok = k.GetProcessTimes(ctypes.wintypes.HANDLE(h), *[ctypes.byref(x) for x in t])
    k.CloseHandle(ctypes.wintypes.HANDLE(h))
    return ((t[0].dwHighDateTime << 32) | t[0].dwLowDateTime) if ok else 0


def _nachkommen(pid: int) -> list[int]:
    """Die Kinder und Kindeskinder von pid, die nach ihrem Elternprozess entstanden sind.

    Windows vergibt PIDs neu: ein fremder Prozess kann als "Kind" einer PID gefuehrt sein, die vor unserem Prozess ein
    anderer trug (taskkill /T folgt dem blind, Vorfall 11.11.). Ein echtes Kind ist juenger als sein Elternprozess.
    """
    k = ctypes.windll.kernel32
    k.CreateToolhelp32Snapshot.restype = ctypes.wintypes.HANDLE
    snap = k.CreateToolhelp32Snapshot(0x2, 0)   # TH32CS_SNAPPROCESS
    kinder: dict[int, list[int]] = {}
    e = _Eintrag()
    e.dwSize = ctypes.sizeof(e)
    weiter = k.Process32FirstW(ctypes.wintypes.HANDLE(snap), ctypes.byref(e))
    while weiter:
        kinder.setdefault(e.th32ParentProcessID, []).append(e.th32ProcessID)
        weiter = k.Process32NextW(ctypes.wintypes.HANDLE(snap), ctypes.byref(e))
    k.CloseHandle(ctypes.wintypes.HANDLE(snap))
    aus: list[int] = []
    offen = [pid]
    while offen:
        eltern = offen.pop()
        t0 = _beginn(eltern)
        for kind in kinder.get(eltern, []):
            t1 = _beginn(kind)
            if kind != pid and kind not in aus and t0 and t1 >= t0:
                aus.append(kind)
                offen.append(kind)
    return aus


def _beenden(p: subprocess.Popen) -> None:
    """Den eigenen Prozess und seine echten Kinder beenden - jede PID einzeln, nie nach Namen, nie ein ganzer Baum blind."""
    k = ctypes.windll.kernel32
    k.OpenProcess.restype = ctypes.wintypes.HANDLE
    kinder = _nachkommen(p.pid)   # vor dem Beenden gelesen: danach haengen sie an keinem Elternprozess mehr
    p.kill()
    for pid in kinder:
        h = k.OpenProcess(0x0001, False, pid)   # PROCESS_TERMINATE
        if h:
            k.TerminateProcess(ctypes.wintypes.HANDLE(h), 1)
            k.CloseHandle(ctypes.wintypes.HANDLE(h))
    try:
        p.wait(timeout=60)
    except subprocess.TimeoutExpired:
        pass


_PORTBEREICHE: dict[int, int] = {}   # erster Port -> Handle der Reservierung (von den laufenden Laeufen dieser Runde belegt)


def _reservieren(basis: int) -> int:
    """Den Bereich ab `basis` systemweit fuer diesen Prozess reservieren: ein benannter Mutex, den es nur einmal gibt. Das
    Handle (0: ein anderer Prozess hat den Bereich). Windows gibt es von selbst frei, wenn die Runde endet oder abstuerzt."""
    k = ctypes.WinDLL("kernel32", use_last_error=True)
    k.CreateMutexW.restype = ctypes.wintypes.HANDLE
    k.CreateMutexW.argtypes = [ctypes.c_void_p, ctypes.wintypes.BOOL, ctypes.wintypes.LPCWSTR]
    k.CloseHandle.argtypes = [ctypes.wintypes.HANDLE]
    ctypes.set_last_error(0)
    h = k.CreateMutexW(None, False, "Local\\bhed_testrunde_bauport_%d" % basis)
    if not h:
        return 0
    if ctypes.get_last_error() == 183:   # ERROR_ALREADY_EXISTS: eine andere Runde haelt ihn
        k.CloseHandle(h)
        return 0
    return h


def _freigeben(basis: int) -> None:
    h = _PORTBEREICHE.pop(basis, 0)
    if h:
        k = ctypes.WinDLL("kernel32", use_last_error=True)
        k.CloseHandle.argtypes = [ctypes.wintypes.HANDLE]
        k.CloseHandle(h)


def _portbereich() -> int:
    """Erster Port eines freien Bereichs von 64 Ports fuer BHED_BAU_PORT (40000 + 64 k, unter den dynamischen Ports ab 49152).

    Alle Runden nehmen dasselbe Raster - zwei Bereiche ueberlappen nie. Frei heisst: in dieser Runde nicht vergeben, von keiner
    anderen Runde reserviert (_reservieren - bis rc612 konnten zwei Runden nebeneinander denselben Bereich wuerfeln, solange
    keine gerade baute, und stoerten sich dann: "Port nicht belegt"), und auf dem ersten, zweiten und letzten Port lauscht gerade
    niemand (ein Programm ausserhalb der Runden). 0, wenn keiner frei ist. Freigabe mit _freigeben am Ende des Laufs.
    """
    for _ in range(600):
        basis = 40000 + 64 * random.randrange(140)
        if basis in _PORTBEREICHE:
            continue
        handle = _reservieren(basis)
        if not handle:
            continue
        frei = True
        for port in (basis, basis + 1, basis + 63):
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            try:
                s.bind(("127.0.0.1", port))
            except OSError:
                frei = False
            finally:
                s.close()
            if not frei:
                break
        _PORTBEREICHE[basis] = handle
        if frei:
            return basis
        _freigeben(basis)
    return 0


def pruefzeilen(log: str) -> list[tuple[str, str]]:
    """(OK|FEHLER, Name) je Pruefzeile des Detailprotokolls."""
    aus = []
    if os.path.isfile(log):
        with open(log, encoding="utf-8", errors="replace") as f:
            for z in f:
                if z.startswith("Knopftest OK: "):
                    aus.append(("OK", z[14:].strip()))
                elif z.startswith("Knopftest FEHLER: "):
                    aus.append(("FEHLER", z[18:].strip()))
    return aus


class Lauf:
    def __init__(self, a: dict, o: argparse.Namespace):
        self.a, self.o = a, o
        self.name = a["name"]
        self.ordner = os.path.join(o.laeufe, o.praefix + self.name)
        self.p: subprocess.Popen | None = None
        self.ergebnis = ""
        self.beginn = 0.0
        self.wanduhr = False      # Pausen nach der Wanduhr erzwingen (Wiederholung eines roten Laufs)
        self.vorlauf = ""         # was der Lauf mit Pausen nach Bildern ergab, bevor er wiederholt wurde
        self.ports = 0            # erster Port des eigenen Bereichs der Bau-Ueberwachung (0: keiner frei, dann NRCs 39000-39063)
        self.fehlt: list[str] = []   # Testdaten, ohne die der Lauf nicht startet

    def starten(self) -> None:
        a, o, d = self.a, self.o, self.ordner
        shutil.rmtree(d, ignore_errors=True)
        os.makedirs(os.path.join(d, "fotos"))
        self.fehlt = [f for f in a.get("braucht", []) if not os.path.exists(f)]
        self.beginn = time.time()
        if self.fehlt:
            return
        if a.get("befehl"):   # Probe der Haengt-Erkennung: irgendein Befehl statt jkstudio.exe
            self.fortschritt = (self.beginn, -1, -1.0)
            self.spitze = 0
            self.nach_bildern = False
            self.p = subprocess.Popen(a["befehl"], cwd=d, stdin=subprocess.DEVNULL, creationflags=0x08000000)   # CREATE_NO_WINDOW
            return
        if a.get("behalten"):
            shutil.copytree(os.path.join(o.laeufe, o.praefix + a["behalten"], "einst"), os.path.join(d, "einst"))
            for f in glob.glob(os.path.join(o.laeufe, o.praefix + a["behalten"], "*_erwartet.txt")):
                shutil.copy(f, d)
        else:
            os.makedirs(os.path.join(d, "einst"))
        if a.get("vorlage"):
            v, n = a["vorlage"].split("|", 1)
            shutil.copytree(v, os.path.join(d, n))
        shutil.copy(os.path.join(o.exe, "jkstudio.exe"), d)
        if os.path.exists(os.path.join(o.exe, "bhed_q3map2.exe")):
            shutil.copy(os.path.join(o.exe, "bhed_q3map2.exe"), d)
        shutil.copytree(os.path.join(o.exe, "data"), os.path.join(d, "data"))
        cfg = os.path.join(d, "einst", "behaved.cfg")
        if not a.get("behalten"):
            shutil.copy(a.get("cfg", CFG), cfg)
        if a.get("sprache"):
            with open(cfg, encoding="utf-8", errors="replace") as f:
                zeilen = f.read().splitlines()
            zeilen = [("language=" + a["sprache"]) if z.startswith("language=") else z for z in zeilen]
            with open(cfg, "w", encoding="utf-8", newline="\n") as f:
                f.write("\n".join(zeilen) + "\n")
        env = dict(os.environ)
        env.update(UMGEBUNG)
        env.update({"BHED_EINSTELLUNGEN": os.path.join(d, "einst"), "BHED_EDITORTEST": a["modus"],
                    "BHED_Q3MAP2": os.path.join(d, "bhed_q3map2.exe"), "BHED_HANDBUCH": os.path.join(REPO, "handbuch")})
        self.nach_bildern = bool(a.get("pausen")) and not o.pausen_wanduhr and not self.wanduhr
        if self.nach_bildern:
            env["BHED_TEST_PAUSEN"] = a["pausen"]
        if not self.ports:
            self.ports = _portbereich()
        if self.ports:
            env["BHED_BAU_PORT"] = str(self.ports)
        else:
            print("%s: kein freier Portbereich - der Lauf baut mit 39000-39063 und kann einen anderen stoeren" % self.name, flush=True)
        for v in a["var"] + o.var:
            k, w = v.split("=", 1)
            env[k] = (w.replace("<FOTOS>", os.path.join(d, "fotos")).replace("<LAUF>", d)
                      .replace("<EIGEN>", "%s%s_%d" % (re.sub(r"\W", "", o.praefix), self.name, os.getpid())))
        self.beginn = time.time()
        self.fortschritt = (self.beginn, -1, -1.0)   # wann zuletzt Protokoll oder Rechenzeit weiter war
        self.spitze = 0
        self.p = subprocess.Popen([os.path.join(d, "jkstudio.exe")], cwd=d, env=env, stdin=subprocess.DEVNULL,
                                  creationflags=0x40 if o.leerlauf else 0x20)   # IDLE_ bzw. NORMAL_PRIORITY_CLASS

    def fertig(self) -> bool:
        """True, sobald der Lauf zu Ende ist (oder beendet wurde); setzt self.ergebnis."""
        if self.fehlt:
            self.gruen, self.dauer, self.fehler = False, 0.0, []
            self.ergebnis = "%s: ROT, nicht gestartet - Testdaten fehlen: %s" % (self.name, ", ".join(self.fehlt))
            return True
        p = self.p
        rc: object = p.poll()
        if rc is None:
            speicher, cpu = _speicher_und_cpu(p)
            self.spitze = max(self.spitze, speicher)
            log = os.path.join(self.ordner, "jkstudio-detail.log")
            groesse = os.path.getsize(log) if os.path.exists(log) else 0
            jetzt = time.time()
            if groesse != self.fortschritt[1] or cpu - self.fortschritt[2] > 1.0 or cpu < 0.0:
                self.fortschritt = (jetzt, groesse, cpu)
            if self.spitze > 8 * 1024 ** 3:
                rc = "8 GB"
            elif jetzt - self.beginn > self.a["sek"]:
                rc = "Zeitgrenze %d s" % self.a["sek"]
            elif jetzt - self.fortschritt[0] > self.o.haengt:
                rc = "HAENGT (%d s ohne Protokoll und ohne Rechenzeit)" % self.o.haengt
            else:
                return False
            _beenden(p)
        dauer = time.time() - self.beginn
        log = os.path.join(self.ordner, "jkstudio-detail.log")
        zeilen = []
        if os.path.exists(log):
            with open(log, encoding="utf-8", errors="replace") as f:
                zeilen = f.read().splitlines()
        wichtig = [z for z in zeilen if "Selbsttest" in z or "Knopftest" in z or "usicherung" in z]
        with open(os.path.join(self.o.laeufe, "letzter_%s%s.txt" % (self.o.praefix, self.name)), "w", encoding="utf-8", newline="\n") as f:
            f.write("\n".join(wichtig) + "\n")
        schluss = [z for z in wichtig if z.startswith("Knopftest fertig")]
        zeiten = [z for z in wichtig if z.startswith("Knopftest Zeiten: gesamt")]
        pause = re.search(r"Pause (\d+)x ([0-9.]+) s", zeiten[-1]) if zeiten else None
        self.gruen = rc == 0 and bool(schluss) and schluss[-1].endswith(" 0 FEHLER")
        # Pruefungen, deren Spieldaten fehlen, sind weder OK noch FEHLER (meldeUebersprungen): gruen, aber nicht geprueft
        ausgelassen = sum(1 for z in wichtig if z.startswith("Knopftest uebersprungen (Daten fehlen)"))
        self.ergebnis = "%s: %s, exit %s, %d MB, %s%s (%.0f s%s)" % (
            self.name, "gruen" if self.gruen else "ROT", rc, self.spitze // 1024 ** 2, schluss[-1] if schluss else "kein 'Knopftest fertig'",
            ", %d uebersprungen (Daten fehlen)" % ausgelassen if ausgelassen else "", dauer,
            ", davon Pausen %s s" % pause.group(2) if pause else "")
        self.fehler = [z for z in wichtig if "FEHLER" in z and not z.startswith("Knopftest fertig")][:20]
        self.dauer = dauer + getattr(self, "dauer_vorlauf", 0.0)
        if not self.gruen and self.nach_bildern:
            # Rot mit Pausen nach Bildern: derselbe Lauf noch einmal nach der Wanduhr (dem Massstab). Es zaehlt die
            # Wiederholung; was nur mit Pausen nach Bildern rot war, bleibt in der Zeile stehen (ein Wackler der neuen
            # Pausen - die Stelle braucht eine Bedingung statt der Pause - oder ein Zeitrennen im Programm).
            self.vorlauf = "; mit Pausen nach Bildern ROT (%s): %s" % (
                schluss[-1][18:] if schluss else rc, " | ".join(z[18:120] for z in self.fehler[:4]))
            self.dauer_vorlauf = dauer
            if os.path.exists(log):   # das Protokoll des roten Laufs aufheben
                shutil.copy(log, os.path.join(self.o.laeufe, "%s%s_nach_bildern-detail.log" % (self.o.praefix, self.name)))
            self.wanduhr = True
            self.starten()
            return False
        self.ergebnis += self.vorlauf
        _freigeben(self.ports)
        return True


def gesperrt(o: argparse.Namespace) -> bool:
    """Messsperre anderer (Datei juenger als 10 Minuten): solange keinen neuen Lauf starten."""
    return bool(o.sperrdatei) and os.path.exists(o.sperrdatei) and time.time() - os.path.getmtime(o.sperrdatei) < 600


def auswahl(o: argparse.Namespace) -> tuple[list[dict], list[tuple[dict, str]]]:
    """(was laeuft, [(was wegbleibt, warum)])."""
    alle = {a["name"]: a for a in LAEUFE}
    unbekannt = [n for n in o.namen_wahl if n not in alle]
    if unbekannt:
        sys.exit("unbekannte Laeufe: " + ", ".join(unbekannt) + " (--liste nennt alle)")
    wahl = set(o.namen_wahl)
    if o.betroffen:
        sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
        import test_betroffen
        bereiche, gruende = test_betroffen.bereiche(o.betroffen, REPO)
        for g in gruende:
            print(g)
        if bereiche is not None:
            wahl |= {a["name"] for a in LAEUFE if a.get("bereich") in bereiche and not a.get("allein")}
            if not wahl:
                return [], []
    laeuft, weg = [], []
    for a in LAEUFE:
        genannt = a["name"] in o.namen_wahl
        if wahl and a["name"] not in wahl:
            continue
        geteilt_ = a.get("doppel") and any(b.get("teil_von") == a["name"] for b in LAEUFE)   # der ganze Lauf zu Teilen
        if o.ungeteilt and a.get("teil_von") and not genannt:
            weg.append((a, "Teil von %s (--ungeteilt: der ganze Lauf faehrt)" % a["teil_von"]))
        elif o.ungeteilt and geteilt_:
            laeuft.append(a)
        elif a.get("allein") and not genannt:
            weg.append((a, "allein: " + a["allein"]))
        elif a.get("doppel") and not genannt and not o.mit_doppelten:
            weg.append((a, "doppelt, enthalten in %s: %s" % a["doppel"]))
        elif a.get("sprachlauf") and not genannt and not o.mit_sprachen:
            weg.append((a, "Sprachlauf (Entscheidung shank 11.11.: volle Runde nur englisch; --mit-sprachen)"))
        else:
            laeuft.append(a)
    return laeuft, weg


def runde(o: argparse.Namespace) -> int:
    auftraege, weg = auswahl(o)
    if o.liste:
        for a in auftraege:
            print("%-22s %-14s %-12s %5d s  %s%s" % (a["name"], a["modus"], a.get("bereich", ""), a["sek"], a.get("sprache", ""),
                                                   " " + " ".join(v for v in a["var"] if not v.startswith(("BHED_RT_GLA", "BHED_MB_BELEGE", "BHED_MB7_BSP")))))
        for a, warum in weg:
            print("  bleibt weg: %-20s %s" % (a["name"], warum))
        print("%d Laeufe, %d bleiben weg" % (len(auftraege), len(weg)))
        return 0
    if not os.path.isfile(os.path.join(o.exe, "jkstudio.exe")):
        sys.exit("keine jkstudio.exe in " + o.exe + " (--exe)")
    os.makedirs(o.laeufe, exist_ok=True)
    summe = os.path.join(o.laeufe, o.praefix + "summe.txt")
    open(summe, "w", encoding="utf-8").close()
    offen = [Lauf(a, o) for a in auftraege]
    genannt = {x.name for x in offen}
    laufend: list[Lauf] = []
    fertig: dict[str, Lauf] = {}
    t0 = time.time()
    while offen or laufend:
        for x in list(laufend):
            if x.fertig():
                laufend.remove(x)
                fertig[x.name] = x
                with open(summe, "a", encoding="utf-8", newline="\n") as f:
                    f.write(x.ergebnis + "\n" + "".join("    " + z[:240] + "\n" for z in x.fehler))
                print(x.ergebnis, flush=True)
                for z in x.fehler[:5]:
                    print("    " + z[:200], flush=True)
        while len(laufend) < o.pool and offen and not gesperrt(o):
            belegt = {y.a.get("sperre") for y in laufend} - {None}
            frei = [x for x in offen if x.a.get("sperre") not in belegt and (x.a.get("behalten") not in genannt or x.a.get("behalten") in fertig)]
            if not frei:
                break
            x = frei[0]
            offen.remove(x)
            x.starten()
            laufend.append(x)
        time.sleep(2)
    rot = [x for x in fertig.values() if not x.gruen]
    schluss = "RUNDE: %d Laeufe, %d rot%s, %.0f s Wanduhr, %.0f s Summe der Laeufe" % (
        len(fertig), len(rot), " (" + ", ".join(x.name for x in rot) + ")" if rot else "", time.time() - t0, sum(x.dauer for x in fertig.values()))
    with open(summe, "a", encoding="utf-8", newline="\n") as f:
        f.write(schluss + "\n")
    print(schluss)
    berichte_zusammen(o, [x for x in fertig.values() if x.a.get("teil_von") == "kartenpruefung"])
    if o.namen:
        namen_schreiben(o, o.namen)
    return 1 if rot else 0


def berichte_zusammen(o: argparse.Namespace, teile: list[Lauf]) -> None:
    """Die Berichte der Anteile von kartenpruefung als EIN Bericht <laeufe>/<praefix>kartenpruefung.jsonl, nach der Nummer
    der Karte sortiert - wie ihn der ungeteilte Lauf schreibt (Auswertung: tools/kartenpruefung.py --nur-auswerten)."""
    if not teile:
        return
    zeilen: list[str] = []
    for x in teile:
        bericht = os.path.join(x.ordner, "kartenpruefung.jsonl")
        if os.path.isfile(bericht):
            with open(bericht, encoding="utf-8", errors="replace") as f:
                zeilen += [z for z in f.read().splitlines() if z.strip()]

    def nummer(z: str) -> int:
        m = re.search(r'"nr":(\d+)', z)
        return int(m.group(1)) if m else -1
    ziel = os.path.join(o.laeufe, o.praefix + "kartenpruefung.jsonl")
    with open(ziel, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(sorted(zeilen, key=nummer)) + "\n")
    print("kartenpruefung: %d Karten aus %d Anteilen nach %s" % (len(zeilen), len(teile), ziel))


# --- Nachweise ----------------------------------------------------------------------------------------------
def ohne_ziffern(s: str) -> str:
    return re.sub(r"\d+([.,]\d+)?", "#", s)


def namen_schreiben(o: argparse.Namespace, ziel: str) -> None:
    """Alle Pruefzeilen der Runde <praefix>*: 'lauf<TAB>OK|FEHLER<TAB>name', sortiert.

    Die Teile eines Laufs (ke_*, <name>_1vN) stehen unter dem Namen des ganzen Laufs - so vergleicht --vergleich eine Runde
    in Teilen unmittelbar mit einer Runde, die den ganzen Lauf fuhr."""
    ganz = {a["name"]: a.get("teil_von", a["name"]) for a in LAEUFE}
    zeilen = []
    for d in sorted(glob.glob(os.path.join(o.laeufe, o.praefix + "*"))):
        if os.path.isdir(d):
            n = os.path.basename(d)[len(o.praefix):]
            n = ganz.get(n, n)
            zeilen += ["%s\t%s\t%s" % (n, s, t) for s, t in pruefzeilen(os.path.join(d, "jkstudio-detail.log"))]
    with open(ziel, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(sorted(zeilen)) + "\n")
    print("%d Pruefzeilen nach %s" % (len(zeilen), ziel))


def vergleich(a: str, b: str, ziffern: bool) -> int:
    """Gleiche Menge von Pruefzeilen (Lauf, Name)? Der Stand OK/FEHLER wird getrennt genannt."""
    def lesen(p):
        m = {}
        with open(p, encoding="utf-8", errors="replace") as f:
            for z in f:
                t = z.rstrip("\n").split("\t", 2)
                if len(t) == 3:
                    k = (t[0], ohne_ziffern(t[2]) if ziffern else t[2])
                    if m.get(k) != "FEHLER":   # steht eine Zeile mehrfach da (Teile eines Laufs), zaehlt jedes FEHLER
                        m[k] = t[1]
        return m
    x, y = lesen(a), lesen(b)
    nur_a, nur_b = sorted(set(x) - set(y)), sorted(set(y) - set(x))
    anders = sorted(k for k in set(x) & set(y) if x[k] != y[k])
    for k in nur_a:
        print("nur in %s: %s: %s" % (a, k[0], k[1][:200]))
    for k in nur_b:
        print("nur in %s: %s: %s" % (b, k[0], k[1][:200]))
    for k in anders:
        print("%s -> %s: %s: %s" % (x[k], y[k], k[0], k[1][:200]))
    print("VERGLEICH: %d / %d Pruefzeilen, nur links %d, nur rechts %d, anderer Stand %d" % (len(x), len(y), len(nur_a), len(nur_b), len(anders)))
    return 1 if nur_a or nur_b or anders else 0


def pruefe_doppel(o: argparse.Namespace) -> int:
    """Jede Pruefzeile eines Doppels steht auch im genannten Lauf - belegt aus den Protokollen einer Runde."""
    def namen(lauf_: str) -> set[str] | None:
        s: set[str] = set()
        for teil in lauf_.split("+"):
            log = os.path.join(o.laeufe, o.praefix + teil, "jkstudio-detail.log")
            if not os.path.isfile(log):
                return None
            s |= {t for _, t in pruefzeilen(log)}
        return s
    rot = 0
    for a in LAEUFE:
        if not a.get("doppel"):
            continue
        eigen, gross = namen(a["name"]), namen(a["doppel"][0])
        if eigen is None or gross is None:
            print("%-20s nicht belegbar: Protokoll von %s oder %s fehlt" % (a["name"], a["name"], a["doppel"][0]))
            continue
        fehlt = sorted(eigen - gross)
        if fehlt and o.ziffern:
            ohne = {ohne_ziffern(t) for t in gross}
            fehlt = [t for t in fehlt if ohne_ziffern(t) not in ohne]
        rot += 1 if fehlt else 0
        print("%-20s %s: %d von %d Pruefzeilen in %s" % (a["name"], "FEHLT" if fehlt else "enthalten", len(eigen) - len(fehlt), len(eigen), a["doppel"][0]))
        for t in fehlt[:3]:
            print("      nur im Doppel: " + t[:200])
    return 1 if rot else 0


def ports_kind(saat: int) -> int:
    """Eine zweite Runde fuer --pruefe-ports: holt mit dem Zufall `saat` einen Bereich, nennt ihn und haelt ihn, bis die
    Elternrunde die Leitung schliesst."""
    random.seed(saat)
    print(_portbereich(), flush=True)
    sys.stdin.read()
    return 0


def pruefe_ports() -> int:
    """Zwei Runden, die zufaellig dasselbe wuerfeln (gleicher Zufall), holen nacheinander einen Portbereich, waehrend noch
    niemand lauscht (wie zwischen zwei Bauten). Gleicher Bereich = ROT: die Laeufe stoeren sich, sobald beide bauen (bis rc612
    war das so)."""
    rot = 0
    for saat in (1, 7, 12345):
        kind = subprocess.Popen([sys.executable, "-I", os.path.abspath(__file__), "--ports-kind", str(saat)], stdin=subprocess.PIPE,
                                stdout=subprocess.PIPE, text=True)
        andere = int((kind.stdout.readline() if kind.stdout else "").strip() or 0)
        random.seed(saat)
        eigene = _portbereich()
        gleich = andere == eigene or not andere or not eigene
        rot += 1 if gleich else 0
        print("Zufall %5d: die andere Runde haelt %d-%d, diese bekommt %d-%d -> %s" % (
            saat, andere, andere + 63, eigene, eigene + 63, "GLEICH (ROT)" if gleich else "verschieden"))
        _freigeben(eigene)
        if kind.stdin:
            kind.stdin.close()
        kind.wait(timeout=30)
    print("Portbereiche: %s" % ("ROT - zwei Runden koennen denselben Bereich bekommen" if rot else "gruen - jede Runde bekommt ihren eigenen"))
    return 1 if rot else 0


def main() -> int:
    for strom in (sys.stdout, sys.stderr):   # umgeleitet schreibt Windows sonst cp1252 - eine Pruefzeile mit "Č" brach --vergleich ab
        strom.reconfigure(encoding="utf-8", errors="replace")
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("namen_wahl", nargs="*", metavar="lauf", help="nur diese Laeufe (auch Doppel und 'allein')")
    ap.add_argument("--exe", default=os.path.join(REPO, "build", "bin", "Release"), help="Ordner mit jkstudio.exe, bhed_q3map2.exe und data")
    ap.add_argument("--laeufe", help="Ordner der Laufordner (Vorgabe: <exe>/../../laeufe)")
    ap.add_argument("--praefix", default="tr_")
    ap.add_argument("--pool", type=int, default=6, help="Laeufe gleichzeitig")
    ap.add_argument("--liste", action="store_true")
    ap.add_argument("--mit-doppelten", action="store_true", help="auch die Laeufe, die ein anderer ganz enthaelt")
    ap.add_argument("--mit-sprachen", action="store_true", help="auch die Wiederholungen der englischen Laeufe in es/cs/ja")
    ap.add_argument("--ungeteilt", action="store_true",
                    help="karteneditor und kartenpruefung als EIN Lauf statt in Teilen (jede Gruppe erbt dann den Zustand ihrer Vorgaengerin)")
    ap.add_argument("--betroffen", metavar="BASIS", help="nur die Bereiche, die git diff BASIS..HEAD beruehrt")
    ap.add_argument("--var", action="append", default=[], metavar="BHED_X=wert", help="zusaetzlich fuer jeden Lauf")
    ap.add_argument("--pausen-wanduhr", action="store_true", help="BHED_TEST_PAUSEN nirgends setzen (Vergleichslauf)")
    ap.add_argument("--leerlauf", action="store_true",
                    help="Leerlauf- statt normaler Prioritaet (neben Bauten 6-10x langsamer, siehe docs/intern/tests_tempo.md)")
    ap.add_argument("--haengt", type=int, default=180, help="Sekunden ohne Protokoll und Rechenzeit, bis ein Lauf als haengend beendet wird")
    ap.add_argument("--sperrdatei", default=os.environ.get("TR_SPERRDATEI", r"Y:\werkzeuge\MESSUNG_LAEUFT.txt"),
                    help="solange diese Datei juenger als 10 min ist, startet kein neuer Lauf (Messfenster)")
    ap.add_argument("--namen", metavar="DATEI", help="Pruefzeilen der Runde <praefix>* in diese Datei")
    ap.add_argument("--nur-namen", action="store_true", help="nichts starten, nur --namen aus vorhandenen Laufordnern")
    ap.add_argument("--vergleich", nargs=2, metavar=("A", "B"), help="zwei --namen-Dateien vergleichen")
    ap.add_argument("--ziffern", action="store_true", help="beim Vergleich Zahlen in den Namen uebergehen (Messwerte)")
    ap.add_argument("--pruefe-doppel", action="store_true")
    ap.add_argument("--pruefe-ports", action="store_true", help="zwei Runden nebeneinander bekommen nie denselben Portbereich")
    ap.add_argument("--ports-kind", type=int, metavar="SAAT", help=argparse.SUPPRESS)
    o = ap.parse_args()
    if o.ports_kind is not None:
        return ports_kind(o.ports_kind)
    if o.pruefe_ports:
        return pruefe_ports()
    if o.vergleich:
        return vergleich(o.vergleich[0], o.vergleich[1], o.ziffern)
    o.exe = os.path.abspath(o.exe)
    o.laeufe = os.path.abspath(o.laeufe or os.path.join(o.exe, "..", "..", "laeufe"))
    if o.pruefe_doppel:
        return pruefe_doppel(o)
    if o.nur_namen:
        namen_schreiben(o, o.namen or sys.exit("--nur-namen braucht --namen DATEI"))
        return 0
    return runde(o)


if __name__ == "__main__":
    sys.exit(main())
