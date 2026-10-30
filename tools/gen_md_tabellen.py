# -*- coding: utf-8 -*-
"""Menue-Editor M7: die festen Tabellen der Movie-Duels-Oberflaeche (code/ui/ui_main.cpp: charMD,
eraNames, duelMaps, singleSaberHilts, staffSaberHilts) als C++-Tabellen fuer src/menue/md_tabellen.inc.

    python tools/gen_md_tabellen.py <Movie Duels code/ui/ui_main.cpp> [ausgabe]

Movie Duels ist GPL v2 (OpenJK-Abkoemmling); die Tabellen sind Daten des Spiels, die seine Feeder
(FEEDER_MD_MODELS/VARIANTS, FEEDER_DUEL_MAPS, FEEDER_*_SABER_HILTS) und UI_DUELMAPS_SELECTION zeigen.
"""
import re
import sys


def tokens(s):
    """C-Literale einer Initialisierung: Zeichenketten, Bezeichner/Zahlen, { } ,"""
    i = 0
    out = []
    while i < len(s):
        c = s[i]
        if s.startswith("//", i):
            i = s.find("\n", i)
            if i < 0:
                break
            continue
        if s.startswith("/*", i):
            i = s.find("*/", i) + 2
            continue
        if c.isspace():
            i += 1
            continue
        if c == '"':
            j = i + 1
            buf = ""
            while s[j] != '"':
                if s[j] == "\\":
                    buf += s[j:j + 2]
                    j += 2
                    continue
                buf += s[j]
                j += 1
            out.append(("s", buf))
            i = j + 1
            continue
        if c in "{},":
            out.append((c, c))
            i += 1
            continue
        j = i
        while j < len(s) and not s[j].isspace() and s[j] not in "{},":
            j += 1
        out.append(("w", s[i:j]))
        i = j
    return out


def tabelle(text, name):
    """Die Zeilen von 'name[] = { { ... }, ... };' als Listen von (art, wert)"""
    m = re.search(r"\b" + name + r"\s*\[\s*\w*\s*\]\s*(?:=\s*)?\{", text)
    if not m:
        raise SystemExit("Tabelle nicht gefunden: " + name)
    i = m.end()
    tiefe = 1
    j = i
    while tiefe:
        if text.startswith("//", j):
            j = text.find("\n", j)
            continue
        if text[j] == '"':
            j = text.find('"', j + 1) + 1
            continue
        if text[j] == "{":
            tiefe += 1
        elif text[j] == "}":
            tiefe -= 1
        j += 1
    t = tokens(text[i:j - 1])
    zeilen, zeile, in_zeile = [], [], False
    for art, wert in t:
        if art == "{":
            in_zeile, zeile = True, []
        elif art == "}":
            zeilen.append(zeile)
            in_zeile = False
        elif in_zeile and art != ",":
            zeile.append((art, wert))
    return zeilen, t


def cstr(x):
    return '"' + x + '"'


def main():
    quelle = sys.argv[1]
    ziel = sys.argv[2] if len(sys.argv) > 2 else "src/menue/md_tabellen.inc"
    text = open(quelle, encoding="latin-1").read()
    enum = re.search(r"typedef enum\s*\{([^}]*)\}\s*eraMD_t", text).group(1)
    epochen = [e.strip() for e in re.sub(r"//[^\n]*", "", enum).split(",") if e.strip()]   # mit TOTAL_ERAS: Abschlusszeile von charMD
    figuren, _ = tabelle(text, "charMD")
    _, namen = tabelle(text, "eraNames")
    duell, _ = tabelle(text, "duelMaps")
    einhand, _ = tabelle(text, "singleSaberHilts")
    stab, _ = tabelle(text, "staffSaberHilts")
    z = ["// md_tabellen.inc - ERZEUGT von tools/gen_md_tabellen.py aus Movie Duels code/ui/ui_main.cpp",
         "// (charMD, eraNames, duelMaps, singleSaberHilts, staffSaberHilts). Nicht von Hand aendern.",
         "//",
         "//   Movie Duels / OpenJK: GNU General Public License version 2 (LICENSE).",
         ""]
    z.append("inline constexpr const char* kMdEpochen[] = {" + ", ".join(cstr(w) for a, w in namen if a == "s") + "};")
    z.append("// name, icon, npc, model, skin, era, desc, isSubDiv")
    z.append("inline constexpr MdFigur kMdFiguren[] = {")
    for f in figuren:
        s = [w for a, w in f]
        era = epochen.index(s[12])
        sub = s[20] in ("qtrue", "true", "1")
        z.append("    {%s, %s, %s, %s, %s, %d, %s, %s}," % (cstr(s[0]), cstr(s[2]), cstr(s[3]), cstr(s[4]), cstr(s[5]), era, cstr(s[19]),
                                                         "true" if sub else "false"))
    z.append("};")
    z.append("// duelMapName, duelMapDesc, duelMapBSP, duelMapImage")
    z.append("inline constexpr MdDuellKarte kMdDuellKarten[] = {")
    for d in duell:
        s = [w for a, w in d]
        z.append("    {%s}," % ", ".join(cstr(x) for x in s[:4]))
    z.append("};")
    for name, liste in (("kMdEinhandGriffe", einhand), ("kMdStabGriffe", stab)):
        z.append("inline constexpr MdGriff %s[] = {" % name)
        for g in liste:
            s = [w for a, w in g]
            z.append("    {%s, %s}," % (cstr(s[0]), cstr(s[1])))
        z.append("};")
    open(ziel, "w", encoding="utf-8", newline="\n").write("\n".join(z) + "\n")
    print("%d Figuren, %d Epochen + Abschluss, %d Duellkarten, %d/%d Griffe -> %s" % (len(figuren), len(epochen), len(duell), len(einhand), len(stab), ziel))


if __name__ == "__main__":
    main()
