# -*- coding: utf-8 -*-
"""Erzeugt src/menue/cvarvorgaben_jk2.inc: die Cvars von Jedi Outcast, wie das Spiel sie im Hauptmenue hat.

Fuer jk2mp gibt es keine Quellen; die Werte stammen darum aus dem laufenden Spiel (`cvarlist` im Protokoll
qconsole.log, gestartet ohne die Einstellungen des Anwenders), ohne die Werte, die der Testlauf selbst setzt (Fenster,
Ton); fuer die gilt weiter die Tabelle der Engine. Die Oberflaeche fragt auch nach Cvars, die in keinem Menue stehen (color1).
Aufruf: python tools/gen_menue_cvarvorgaben_jk2.py <log jk2mp> <log jk2sp>"""
import os
import re
import sys

VOM_LAUF = {"in_mouse", "in_joystick", "r_fullscreen", "r_mode", "r_customwidth", "r_customheight", "s_initsound", "s_volume",
            "s_musicvolume", "logfile", "com_maxfps", "sp_language", "fs_basepath", "fs_cdpath", "fs_homepath"}
ZEILE = re.compile(r'^[ SUVRICLA]{8}(\S+) "(.*)"\s*$')


def tabelle(name, log):
    werte = {}
    for z in open(log, encoding="latin-1"):
        m = ZEILE.match(z.rstrip("\r\n"))
        if m and m.group(1).lower() not in VOM_LAUF:
            werte[m.group(1)] = m.group(2)
    # Beim Oeffnen kopiert das Spiel name nach ui_Name (uiScript update "ui_GetName")
    for z in open(log, encoding="latin-1"):
        m = ZEILE.match(z.rstrip("\r\n"))
        if m and m.group(1) == "name":
            werte["ui_Name"] = m.group(2)
    zeilen = ["const CvarVorgabe %s[] = {" % name]
    for k in sorted(werte, key=str.lower):
        zeilen.append('    {"%s", "%s"},' % (k, werte[k].replace("\\", "\\\\").replace('"', '\\"')))
    zeilen.append("};")
    return "\n".join(zeilen) + "\n"


def main():
    log_mp, log_sp = sys.argv[1:3]
    ziel = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "src", "menue", "cvarvorgaben_jk2.inc")
    with open(ziel, "w", encoding="utf-8", newline="\n") as f:
        f.write("// Erzeugt von tools/gen_menue_cvarvorgaben_jk2.py - nicht von Hand aendern.\n")
        f.write(tabelle("kVorgabenJk2Mp", log_mp))
        f.write(tabelle("kVorgabenJk2Sp", log_sp))
    print(ziel)


if __name__ == "__main__":
    main()
