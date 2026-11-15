# -*- coding: utf-8 -*-
"""CW-MODELL: die Namen, die das Spiel in einer animation.cfg kennt (animTable, cgame/animtable.h), je Zielspiel nach
src/modell/animnamen.inc.

G_ParseAnimationFile (OpenJK code/game/NPC_stats.cpp:927-940, Movie Duels Update 9 :1096) und BG_ParseAnimationFile (codemp/game/
bg_panimate.c:2429, MD9 :6008) suchen jeden Namen mit GetIDForString(animTable, ...) - ein unbekannter Name wird uebergangen (nur in
Entwicklerfassungen mit Warnung). Die Tabelle steht in cgame/animtable.h: ENUM2STRING(NAME) in der Reihenfolge von anims.h.

Aufruf (aus dem Repo): python -I tools/modell/gen_animnamen.py <openjk-src> <movieduels-update9>
  z. B. python -I tools/modell/gen_animnamen.py Y:/werkzeuge/sp_af9c/m3/ojq/src Y:/werkzeuge/sp_654alt/md9
"""
import io
import os
import re
import sys

oj, md = sys.argv[1], sys.argv[2]
QUELLEN = [
    ("JkaSp", oj, "code/cgame/animtable.h"),
    ("JkaMp", oj, "codemp/cgame/animtable.h"),
    ("Jk2Sp", oj, "codeJK2/cgame/animtable.h"),
    ("MdSp", md, "code/cgame/animtable.h"),
    ("MdMp", md, "codemp/cgame/animtable.h"),
]
listen = {}
for kenn, wurzel, rel in QUELLEN:
    text = io.open(os.path.join(wurzel, rel), encoding="latin-1").read()
    # Kommentare weg (// bis Zeilenende, /* */)
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    text = re.sub(r"//[^\n]*", "", text)
    namen = re.findall(r"ENUM2STRING\s*\(\s*([A-Za-z0-9_]+)\s*\)", text)
    listen[kenn] = (namen, rel)

aus = ["// animnamen.inc - ERZEUGT von tools/modell/gen_animnamen.py, nicht von Hand aendern (CW-MODELL, 15.11.2026)",
       "// Die Namen der animTable je Zielspiel (cgame/animtable.h, ENUM2STRING in der Reihenfolge von anims.h):"]
for kenn, (namen, rel) in listen.items():
    quelle = "OpenJK" if kenn in ("JkaSp", "JkaMp", "Jk2Sp") else "Movie Duels Update 9"
    aus.append("//   %-6s %4d Namen  %s %s" % (kenn, len(namen), quelle, rel))
gleich = listen["MdSp"][0] == listen["MdMp"][0]
aus.append("// MdMp %s MdSp." % ("ist gleich" if gleich else "unterscheidet sich von"))
aus.append("")
for kenn, (namen, rel) in listen.items():
    if kenn == "MdMp" and gleich:
        continue
    aus.append("const char* const kAnim%s[] = {" % kenn)
    zeile = "   "
    for n in namen:
        teil = ' "%s",' % n
        if len(zeile) + len(teil) > 130:
            aus.append(zeile)
            zeile = "   "
        zeile += teil
    if zeile.strip():
        aus.append(zeile)
    aus.append("};")
assert gleich, "Movie Duels MP unterscheidet sich von SP: src/modell/animnamen.cpp braucht dann eine eigene Liste"
aus.append("")
io.open("src/modell/animnamen.inc", "w", encoding="utf-8", newline="\n").write("\n".join(aus))
print({k: len(v[0]) for k, v in listen.items()}, "MD SP == MP:", gleich)
