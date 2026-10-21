# -*- coding: utf-8 -*-
"""
Testkarten fuer den Abgleich mit dem echten q3map2 (ctest karte_vergleich).

Schreibt tests/karte/vergleich_q3.map (Quake3-Texdef) und
tests/karte/vergleich_bp.map (Brush Primitives) im NRC-Format: ein
geschlossener Raum aus sechs Pinseln (Texturen dort werden nicht verglichen),
darin frei schwebend
  - ein Wuerfel, jede Flaeche mit anderer Verschiebung/Skalierung/Drehung,
    auch negativer Skalierung, auf vier Texturen unterschiedlicher Groesse,
  - ein Keil mit zwei schraegen Ebenen (eine davon mit Normale ~ (2, 1.5, 2)),
  - ein gebogener 3x3-Patch,
  - info_player_start, damit q3map2 den Raum als dicht erkennt.

Die BSPs dazu erzeugt q3map2 von NetRadiant-Custom 20260114 ohne -meta:

  q3map2.exe -game ja -fs_basepath "<JA GameData original>/" vergleich_q3.map
  q3map2.exe -game ja -fs_basepath "<JA GameData original>/" vergleich_bp.map

(Ausgabe nie in den Spielordner; die .bsp liegen als vergleich_q3.bsp /
vergleich_bp.bsp neben den Karten in tests/karte.)

  python -I tools/gen_karte_vergleich.py
"""
import os

HIER = os.path.dirname(os.path.abspath(__file__))
ZIEL = os.path.join(os.path.dirname(HIER), "tests", "karte")


def zahl(x):
    s = ("%.10f" % x).rstrip("0").rstrip(".")
    return "0" if s in ("-0", "") else s


def quader(mins, maxs):
    """Ebenenpunkte wie Brush_ConstructCuboid (brushmanip.cpp:37-74)."""
    box = [(0, 1), (2, 0), (1, 2)]
    ebenen = []
    for i in range(3):
        p1 = list(maxs)
        p2 = list(maxs)
        p2[box[i][0]] = mins[box[i][0]]
        p1[box[i][1]] = mins[box[i][1]]
        ebenen.append((tuple(maxs), tuple(p1), tuple(p2)))
    for i in range(3):
        p1 = list(mins)
        p2 = list(mins)
        p1[box[i][0]] = maxs[box[i][0]]
        p2[box[i][1]] = maxs[box[i][1]]
        ebenen.append((tuple(mins), tuple(p1), tuple(p2)))
    return ebenen   # oben, +y? ... Reihenfolge: +z, +x, +y, -z, -x, -y nach box


def punkte(e):
    return " ".join("( %s %s %s )" % tuple(zahl(c) for c in p) for p in e)


def flaeche_q3(e, tex, td):
    return "%s %s %s 0 0 0\n" % (punkte(e), tex, " ".join(zahl(x) for x in td))


def flaeche_bp(e, tex, m):
    return "%s ( ( %s %s %s ) ( %s %s %s ) ) %s 0 0 0\n" % ((punkte(e),) + tuple(zahl(x) for x in m) + (tex,))


RAUM = "imperial/basic"
RAUM_TEILE = [
    ((-528, -528, -16), (528, 528, 0)),
    ((-528, -528, 512), (528, 528, 528)),
    ((-528, -528, 0), (-512, 528, 512)),
    ((512, -528, 0), (528, 528, 512)),
    ((-512, -528, 0), (512, -512, 512)),
    ((-512, 512, 0), (512, 528, 512)),
]

# Wuerfel: Texturen und Quake3-Texdef (shift s, shift t, rotate, scale s, scale t) je Flaeche
WUERFEL = ((-300, -50, 100), (-200, 50, 200))
WUERFEL_Q3 = [
    ("imperial/caution", (16, 8, 0, 0.5, 0.25)),
    ("imperial/edge", (13.5, -7, 30, 0.37, 0.61)),
    ("imperial/door_1new_onoff", (100, 200, 90, 2, 2)),
    ("imperial/caution", (0, 0, 45, 1, 1)),
    ("imperial/edge", (0, 0, 0, -0.5, 0.5)),
    ("imperial/grid", (-33, 17, 180, 0.125, 0.125)),
]
WUERFEL_BP = [
    ("imperial/caution", (0.00390625, 0.001, 0.25, -0.002, 0.0078125, 0.5)),
    ("imperial/edge", (0.01, -0.003, 0.125, 0.004, 0.009, -0.75)),
    ("imperial/door_1new_onoff", (-0.0025, 0, 0.3, 0, 0.0025, 0.1)),
    ("imperial/caution", (0.0078125, 0, 0, 0, 0.0078125, 0)),
    ("imperial/edge", (0.002, 0.002, 0.6, -0.002, 0.002, 0.4)),
    ("imperial/grid", (0.03125, 0.0001, -0.5, 0.0002, -0.03125, 0.25)),
]

# Keil: Kasten 0..128 x -64..64 x 100..228, zwei schraege Ebenen
KEIL = ((0, -64, 100), (128, 64, 228))
KEIL_SCHRAEG = [
    ((128, 64, 180), (128, 0, 228), (80, 64, 228)),    # Normale ~ (0.62, 0.47, 0.62)
    ((0, -64, 160), (0, 64, 160), (96, -64, 228)),     # Neigung in x (Normale ~ (-0.58, 0, 0.82))
]
KEIL_Q3 = [("imperial/grid", (5, 3, 15, 0.3, 0.7)), ("imperial/caution", (-12, 40, 200, 0.75, -0.5))]
KEIL_BP = [("imperial/grid", (0.0042, -0.0013, 0.2, 0.0011, 0.0055, 0.7)),
           ("imperial/caution", (0.012, 0, -0.33, 0.003, -0.02, 0.05))]

PATCH = (
    "{\npatchDef2\n{\nimperial/floor\n( 3 3 0 0 0 )\n(\n"
    "( ( 200 -100 120 0 0 ) ( 200 -36 200 0 0.5 ) ( 200 28 120 0 1 ) )\n"
    "( ( 264 -100 120 0.5 0 ) ( 264 -36 200 0.5 0.5 ) ( 264 28 120 0.5 1 ) )\n"
    "( ( 328 -100 160 1 0 ) ( 328 -36 240 1 0.5 ) ( 328 28 160 1 1 ) )\n"
    ")\n}\n}\n"
)


def karte(bp):
    teile = []
    nr = 0

    def pinsel(flaechen):
        nonlocal nr
        s = "// brush %d\n{\n" % nr
        if bp:
            s += "brushDef\n{\n" + "".join(flaechen) + "}\n"
        else:
            s += "".join(flaechen)
        s += "}\n"
        nr += 1
        return s

    for mins, maxs in RAUM_TEILE:
        if bp:
            fl = [flaeche_bp(e, RAUM, (0.0019531, 0, 0, 0, 0.0019531, 0)) for e in quader(mins, maxs)]
        else:
            fl = [flaeche_q3(e, RAUM, (0, 0, 0, 0.5, 0.5)) for e in quader(mins, maxs)]
        teile.append(pinsel(fl))
    ebenen = quader(*WUERFEL)
    if bp:
        teile.append(pinsel([flaeche_bp(e, t, m) for e, (t, m) in zip(ebenen, WUERFEL_BP)]))
    else:
        teile.append(pinsel([flaeche_q3(e, t, td) for e, (t, td) in zip(ebenen, WUERFEL_Q3)]))
    ebenen = quader(*KEIL)
    if bp:
        fl = [flaeche_bp(e, "imperial/edge", (0.004, 0, 0, 0, 0.004, 0)) for e in ebenen]
        fl += [flaeche_bp(e, t, m) for e, (t, m) in zip(KEIL_SCHRAEG, KEIL_BP)]
    else:
        fl = [flaeche_q3(e, "imperial/edge", (0, 0, 0, 0.5, 0.5)) for e in ebenen]
        fl += [flaeche_q3(e, t, td) for e, (t, td) in zip(KEIL_SCHRAEG, KEIL_Q3)]
    teile.append(pinsel(fl))
    teile.append("// brush %d\n" % nr + PATCH)
    text = ("\n//@$& layerdef \"0\" -1 0 0 0\n// entity 0\n{\n\"classname\" \"worldspawn\"\n" + "".join(teile) + "}\n"
            "// entity 1\n{\n\"classname\" \"info_player_start\"\n\"origin\" \"-400 -400 24\"\n}\n")
    return text


def main():
    os.makedirs(ZIEL, exist_ok=True)
    for name, bp in (("vergleich_q3.map", False), ("vergleich_bp.map", True)):
        with open(os.path.join(ZIEL, name), "w", encoding="ascii", newline="\n") as f:
            f.write(karte(bp))
        print(os.path.join(ZIEL, name))


if __name__ == "__main__":
    main()
