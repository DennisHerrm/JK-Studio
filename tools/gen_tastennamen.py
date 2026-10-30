# -*- coding: utf-8 -*-
"""Menue-Editor M7: die Tastennamen der Engine (OpenJK code/client/cl_keys.cpp keynames[MAX_KEYS],
codemp gleich) als C++-Tabelle fuer src/menue/tasten.inc: je Tastennummer upper, lower und Name.

    python tools/gen_tastennamen.py <OpenJK-Quellen> <Movie-Duels-code> [ausgabe]

Dazu die Befehlslisten der Tastenbelegung (g_bindCommands) aus code/ui/ui_shared.cpp (mit und ohne
JK2_MODE), codemp/ui/ui_shared.c und Movie Duels code/ui/ui_shared.cpp.

keynum aus keycodes.h (nicht immer der Index: SHIFT_ENTER ist A_ENTER).
"""
import re
import sys


def wert(t):
    r"""Zahl, Zeichen ('a', '\'') oder breites Zeichen (L'\u00A2'), auch mit (word) davor"""
    t = re.sub(r"^\(word\)\s*", "", t.strip())
    if t.startswith("L'"):
        t = t[1:]
    if t.startswith("'"):
        c = t[1:-1]
        if c.startswith("\\u"):
            return int(c[2:], 16)
        return ord(c[1]) if c.startswith("\\") else ord(c)
    return int(t, 0)


def felder(zeile):
    """Die Felder einer Initialisierung { a, b, "n", K, f } mit Kommas in Zeichen-Literalen"""
    out, buf, i, in_char, in_str = [], "", 0, False, False
    while i < len(zeile):
        c = zeile[i]
        if in_char:
            buf += c
            if c == "\\":
                buf += zeile[i + 1]
                i += 1
            elif c == "'":
                in_char = False
        elif in_str:
            buf += c
            if c == '"':
                in_str = False
        elif c == "'":
            in_char = True
            buf += c
        elif c == '"':
            in_str = True
            buf += c
        elif c == ",":
            out.append(buf.strip())
            buf = ""
        else:
            buf += c
        i += 1
    if buf.strip():
        out.append(buf.strip())
    return out


def bindbefehle(text, jk2):
    """g_bindCommands[] mit #ifdef/#ifndef JK2_MODE ... #else ... #endif"""
    t = text[text.index("g_bindCommands[]"):]
    t = t[t.index("{") + 1:t.index("};")]
    out, stapel = [], []
    for zeile in t.splitlines():
        z = zeile.strip()
        if z.startswith("#ifdef JK2_MODE"):
            stapel.append(jk2)
        elif z.startswith("#ifndef JK2_MODE"):
            stapel.append(not jk2)
        elif z.startswith("#else"):
            stapel[-1] = not stapel[-1]
        elif z.startswith("#endif"):
            stapel.pop()
        elif all(stapel):
            out += re.findall(r'"([^"]*)"', re.sub(r"//.*", "", z))
    return out


def main():
    oj, md = sys.argv[1], sys.argv[2]
    quelle, kopf = oj + "/code/client/cl_keys.cpp", oj + "/code/client/keycodes.h"
    ziel = sys.argv[3] if len(sys.argv) > 3 else "src/menue/tasten.inc"
    enum = open(kopf, encoding="latin-1").read()
    e = enum[enum.index("typedef enum"):]
    e = re.sub(r"//[^\n]*", "", e[:e.index("}")])
    nummern, n = {}, 0
    for teil in e[e.index("{") + 1:].split(","):
        teil = teil.strip()
        if not teil:
            continue
        if "=" in teil:
            name, v = [x.strip() for x in teil.split("=")]
            n = wert(v)
        else:
            name = teil
        nummern[name] = n
        n += 1
    text = open(quelle, encoding="latin-1").read()
    block = text[text.index("keyname_t keynames[MAX_KEYS]"):]
    block = block[block.index("{") + 1:block.index("\n};")]
    zeilen = []
    for zeile in block.splitlines():
        zeile = zeile.strip()
        if not zeile.startswith("{"):
            continue
        f = felder(zeile[1:zeile.rindex("}")])
        upper, lower, name, keynum = wert(f[0]), wert(f[1]), f[2], f[3]
        if keynum not in nummern:
            raise SystemExit("unbekannte Taste %s" % keynum)
        zeilen.append((upper, lower, None if name == "NULL" else name.strip('"'), nummern[keynum]))
    z = ["// tasten.inc - ERZEUGT von tools/gen_tastennamen.py aus OpenJK code/client/cl_keys.cpp (keynames).",
         "// Nicht von Hand aendern.  OpenJK: GNU General Public License version 2 (LICENSE).",
         "// upper, lower, Name (nullptr = keiner), keynum; Index = Tastennummer",
         "inline constexpr TastenName kTastenNamen[] = {"]
    for upper, lower, name, nr in zeilen:
        z.append("    {0x%x, 0x%x, %s, 0x%x}," % (upper, lower, '"%s"' % name if name else "nullptr", nr))
    z.append("};")
    for name, datei, jk2 in (("kBindSp", oj + "/code/ui/ui_shared.cpp", False), ("kBindJk2", oj + "/code/ui/ui_shared.cpp", True),
                             ("kBindMp", oj + "/codemp/ui/ui_shared.c", False), ("kBindMd", md + "/ui/ui_shared.cpp", False)):
        befehle = bindbefehle(open(datei, encoding="latin-1").read(), jk2)
        z.append("inline constexpr const char* %s[] = {%s};" % (name, ", ".join('"%s"' % b for b in befehle)))
    open(ziel, "w", encoding="utf-8", newline="\n").write("\n".join(z) + "\n")
    print("%d Tasten -> %s" % (len(zeilen), ziel))


if __name__ == "__main__":
    main()
