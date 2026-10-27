# -*- coding: utf-8 -*-
"""
Entitaetsdefinitionen (.def) fuer den Karten-Editor aus dem OpenJK-Quelltext.

NetRadiant-Custom liest die Entitaetsklassen aus .def-Dateien: hintereinander
gehaengte /*QUAKED ... */-Bloecke, genau so, wie sie im Spielcode stehen
(radiant/eclass_def.cpp, Eclass_ScanFile). Die Gamepacks, die NRC mitbringt,
sind laut NRCs eigenem download-gamepacks.sh "proprietary" (JediAcademyPack)
- deshalb werden die Bloecke hier aus OpenJK (GPL v2) gezogen:

  code/game     -> data/karte/ja/sp_entities.def   (Jedi Academy Einzelspieler)
  codemp/game   -> data/karte/ja/mp_entities.def   (Jedi Academy Mehrspieler)
  codeJK2/game  -> data/karte/jk2/sp_entities.def  (Jedi Outcast Einzelspieler)

Jeder Block wird unveraendert uebernommen (nur CR LF -> LF), in der
Reihenfolge der Dateinamen und innerhalb einer Datei in Textreihenfolge.
Kommt eine Klasse mehrfach vor, gewinnt in NRC die erste
(EClass_InsertSortedList); die spaeteren bleiben trotzdem stehen.

  python -I tools/gen_karten_def.py <OpenJK-Ordner> [<Commit>]
"""
import glob
import os
import sys

HIER = os.path.dirname(os.path.abspath(__file__))
WURZEL = os.path.dirname(HIER)

ZIELE = [
    ("code/game", "data/karte/ja/sp_entities.def", "Jedi Academy, Einzelspieler"),
    ("codemp/game", "data/karte/ja/mp_entities.def", "Jedi Academy, Mehrspieler"),
    ("codeJK2/game", "data/karte/jk2/sp_entities.def", "Jedi Outcast, Einzelspieler"),
]

# Stufe 1b: Bloecke, die Raven fuer Radiant mit "/*QUAK-ED" abgeschaltet hat,
# obwohl der Spielcode die Klasse kennt. In codeJK2 gilt das fuer NPC_spawner:
# SP_NPC_spawner steht in g_spawn.cpp:620 ({"NPC_spawner", SP_NPC_spawner}),
# sein Block in NPC_spawn.cpp:1500 beginnt aber mit "/*QUAK-ED", und der
# einzige echte NPC-Block heisst "NPC_*****" (NPC_spawn.cpp:1644). Ohne den
# Spawner koennte man in JK2-Karten keinen NPC_spawner anlegen - Ravens
# eigene Karten benutzen ihn ueberall. Der Block wird unveraendert
# uebernommen, nur das Schluesselwort heisst wieder QUAKED.
ABGESCHALTET = {
    "codeJK2/game": ("NPC_spawner",),
}


def bloecke(text):
    """Alle /*QUAKED ... */ wie der Zustandsautomat in Eclass_ScanFile."""
    raus = []
    i = 0
    while True:
        a = text.find("/*QUAKED", i)
        if a < 0:
            return raus
        b = text.find("*/", a + 8)
        if b < 0:
            return raus
        raus.append(text[a:b + 2])
        i = b + 2


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    quelle = sys.argv[1]
    commit = sys.argv[2] if len(sys.argv) > 2 else "unbekannt"
    for ordner, ziel, titel in ZIELE:
        dateien = []
        for muster in ("*.c", "*.cpp", "*.h"):
            dateien += glob.glob(os.path.join(quelle, ordner, muster))
        dateien.sort(key=lambda p: os.path.basename(p).lower())
        if not dateien:
            sys.exit("keine Quelldateien in %s" % os.path.join(quelle, ordner))
        teile = []
        anzahl = 0
        for d in dateien:
            text = open(d, "rb").read().decode("latin-1").replace("\r\n", "\n")
            for name in ABGESCHALTET.get(ordner, ()):
                text = text.replace("/*QUAK-ED " + name + " ", "/*QUAKED " + name + " ")
            gefunden = bloecke(text)
            if not gefunden:
                continue
            teile.append("// --- %s/%s ---\n" % (ordner, os.path.basename(d)))
            for b in gefunden:
                teile.append(b + "\n\n")
                anzahl += 1
        kopf = (
            "// %s: Entitaetsklassen fuer den Karten-Editor von JK Studio.\n"
            "// Erzeugt von tools/gen_karten_def.py aus OpenJK %s (Commit %s),\n"
            "// den /*QUAKED*/-Bloecken des Spielcodes - nicht von Hand aendern.\n"
            "// Herkunft: https://github.com/JACoders/OpenJK, GNU GPL v2.\n"
            "//   Copyright (C) 1999 - 2005, Id Software, Inc.\n"
            "//   Copyright (C) 2000 - 2013, Raven Software, Inc.\n"
            "//   Copyright (C) 2001 - 2013, Activision, Inc.\n"
            "//   Copyright (C) 2013 - 2015, OpenJK contributors\n"
            "// %d Bloecke.\n\n" % (titel, ordner, commit, anzahl)
        )
        pfad = os.path.join(WURZEL, ziel)
        os.makedirs(os.path.dirname(pfad), exist_ok=True)
        with open(pfad, "w", encoding="latin-1", newline="\n") as f:
            f.write(kopf + "".join(teile))
        print("%s: %d Bloecke aus %d Dateien" % (ziel, anzahl, len(dateien)))


if __name__ == "__main__":
    main()
