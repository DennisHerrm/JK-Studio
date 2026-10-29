# -*- coding: utf-8 -*-
"""Erzeugt src/menue/cvarvorgaben.inc: die Start-Cvars der SP-Oberflaeche wie die Engine sie
anlegt - UI_Init (code/ui/ui_atoms.cpp: ui.Cvar_Create) und die cvarTable von
code/ui/ui_main.cpp, je fuer OpenJK (Jedi Academy) und Movie Duels; fuer den Mehrspieler
(M5) CL_Init (codemp/client/cl_main.cpp) und codemp/ui/ui_xcvar.h aus OpenJK (Movie Duels hat
keine Mehrspieler-Quellen veroeffentlicht - MD-MP nimmt dieselben Vorgaben).
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

def lies_mp(wurzel):
    # M5: Mehrspieler - die Cvars, die der Server (sv_hostname, sv_maxclients, fraglimit ...) und der
    # Client anlegen (codemp/client/cl_main.cpp CL_Init: Cvar_Get
    # "model" DEFAULT_MODEL"/default", char_color_* ...) und die UI-Tabelle codemp/ui/ui_xcvar.h
    # (XCVAR_DEF); der Client legt sie zuerst an, Cvar_Register aendert vorhandene Werte nicht.
    bg = open(os.path.join(wurzel, "codemp", "game", "bg_public.h"), encoding="latin-1").read()
    makros = dict(re.findall(r'#define\s+(DEFAULT_MODEL\w*)\s+"([^"]*)"', bg))
    werte = {}
    # Com_Init: SV_Init (sv_init.cpp, darin SV_BotInitCvars aus sv_bot.cpp) vor CL_Init
    for datei in (("server", "sv_init.cpp"), ("server", "sv_bot.cpp"), ("client", "cl_main.cpp")):
        t = open(os.path.join(wurzel, "codemp", *datei), encoding="latin-1").read()
        for name, makro, wert in re.findall(r'Cvar_Get\s*\(\s*"(\w+)"\s*,\s*(\w*)\s*"([^"]*)"', t):
            werte.setdefault(name.lower(), makros.get(makro, "") + wert)
    t = open(os.path.join(wurzel, "codemp", "ui", "ui_xcvar.h"), encoding="latin-1").read()
    for name, wert in re.findall(r'XCVAR_DEF\(\s*(\w+)\s*,\s*"([^"]*)"', t):
        werte.setdefault(name.lower(), wert)
    return werte
oj, md, mp = lies(sys.argv[1]), lies(sys.argv[2]), lies_mp(sys.argv[1])
zeilen = ["// cvarvorgaben.inc - erzeugt von tools/gen_menue_cvarvorgaben.py, nicht von Hand aendern.",
          "// Portiert aus OpenJK code/ui/ui_atoms.cpp (UI_Init) und code/ui/ui_main.cpp (cvarTable)",
          "// sowie den gleichen Dateien von Movie Duels (GPL v2, siehe LICENSE).",
          "struct CvarVorgabe {", "    const char* name;", "    const char* wert;", "};"]
for titel, w in (("kVorgabenJa", oj), ("kVorgabenMd", md), ("kVorgabenMp", mp)):
    zeilen.append("const CvarVorgabe %s[] = {" % titel)
    for k in sorted(w):
        zeilen.append('    {"%s", "%s"},' % (k, w[k].replace("\\", "\\\\")))
    zeilen.append("};")
open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src", "menue", "cvarvorgaben.inc"), "w", encoding="utf-8", newline="\n").write("\n".join(zeilen) + "\n")
print("cvarvorgaben:", len(oj), "JA,", len(md), "MD,", len(mp), "MP")
