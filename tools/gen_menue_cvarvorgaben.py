# -*- coding: utf-8 -*-
"""Erzeugt src/menue/cvarvorgaben.inc: die Start-Cvars der SP-Oberflaeche wie die Engine sie
anlegt - UI_Init (code/ui/ui_atoms.cpp: ui.Cvar_Create) und die cvarTable von
code/ui/ui_main.cpp, je fuer OpenJK (Jedi Academy) und Movie Duels.
Aufruf: python tools/gen_menue_cvarvorgaben.py <OpenJK-Quellen> <MovieDuels-Quellen>"""
import os, re, sys

def lies(wurzel):
    werte = {}
    t = open(os.path.join(wurzel, "code", "ui", "ui_main.cpp"), encoding="latin-1").read()
    for name, wert in re.findall(r'\{\s*&\w+\s*,\s*"(\w+)"\s*,\s*"([^"]*)"', t):
        werte.setdefault(name.lower(), wert)
    t = open(os.path.join(wurzel, "code", "ui", "ui_atoms.cpp"), encoding="latin-1").read()
    for name, wert in re.findall(r'ui\.Cvar_Create\s*\(\s*"(\w+)"\s*,\s*"([^"]*)"', t):
        werte[name.lower()] = wert   # UI_Init nach der Tabelle: gewinnt
    return werte

oj, md = lies(sys.argv[1]), lies(sys.argv[2])
zeilen = ["// cvarvorgaben.inc - erzeugt von tools/gen_menue_cvarvorgaben.py, nicht von Hand aendern.",
          "// Portiert aus OpenJK code/ui/ui_atoms.cpp (UI_Init) und code/ui/ui_main.cpp (cvarTable)",
          "// sowie den gleichen Dateien von Movie Duels (GPL v2, siehe LICENSE).",
          "struct CvarVorgabe {", "    const char* name;", "    const char* wert;", "};"]
for titel, w in (("kVorgabenJa", oj), ("kVorgabenMd", md)):
    zeilen.append("const CvarVorgabe %s[] = {" % titel)
    for k in sorted(w):
        zeilen.append('    {"%s", "%s"},' % (k, w[k].replace("\\", "\\\\")))
    zeilen.append("};")
open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src", "menue", "cvarvorgaben.inc"), "w", encoding="utf-8", newline="\n").write("\n".join(zeilen) + "\n")
print("cvarvorgaben:", len(oj), "JA,", len(md), "MD")
