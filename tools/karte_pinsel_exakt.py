# -*- coding: utf-8 -*-
"""
Exakte Gegenrechnung zu ctest karte_massenlast (Bruchzahlen statt float).

  kartemassenlast <wurzel> "*" > liste.txt
  python -I tools/karte_pinsel_exakt.py <wurzel> liste.txt

kartemassenlast schreibt mit "*" je ungueltigem Pinsel und je 500. gueltigem
Pinsel eine Zeile "LISTE <spiel/format/karte.map> <entitaet> <primitiv>
<gueltig> <beitragende Flaechen>". Dieses Skript liest die Ebenenpunkte
derselben Pinsel aus den Quake3-Karten (<wurzel>/<spiel>/q3; q3map2 schreibt
sie mit drei Nachkommastellen, also exakt als Bruch darstellbar) und rechnet
den Schnitt der Halbraeume exakt: Ecken aus allen Ebenentripeln, Rang der
Ecken (leer / flach / Volumen), Beschraenktheit ueber den Rezessionskegel,
beitragende Flaechen = Ebenen mit drei nicht kollinearen Ecken.

Gueltige Pinsel muessen exakt beschraenkt sein, Volumen haben und gleich viele
beitragende Flaechen. Fuer ungueltige Pinsel wird der Grund gesucht:
  leer / flach / offen        so von q3map2 geschrieben (3 Nachkommastellen,
                              fast kollineare Punkte drehen eine Ebene um)
  plane3_opposing             zwei Ebenen mit Normale bis 0.0001 gegenueber
                              und Abstand bis 0.02 (libs/math/plane.h:115-125):
                              windingForClipPlane schneidet nicht (Nadel)
  plane3_inside               Normalen bis 0.001 gleich (brush.h:1445-1454):
                              die weitere Ebene faellt weg, der Rest ist flach
                              oder reicht ueber die Weltgrenze
  Weltgrenze                  Ecken jenseits von 65536 (m_maxWorldCoord):
                              die Wicklung stoesst an den Rand, isBounded
  Euler-Meldung               NRC zeigt den Pinsel, meldet nur
                              "Final B-Rep: inconsistent vertex count"
Uebrig bleibende Pinsel stehen unter "unerklaert".
"""
import os
import re
import sys
from fractions import Fraction
from itertools import combinations

ZAHL = r"\s*(\S+)\s+(\S+)\s+(\S+)\s*"
EBENE = re.compile(r"\(" + ZAHL + r"\)\s*\(" + ZAHL + r"\)\s*\(" + ZAHL + r"\)")


def karte_lesen(pfad):
    """Entitaeten -> Primitive (Liste der Ebenenpunkte, None fuer Patches)."""
    ents = []
    tiefe = 0
    cur = prim = None
    patch = False
    for zeile in open(pfad, encoding="latin-1"):
        t = zeile.strip()
        if not t or t.startswith("//"):
            continue
        if t == "{":
            tiefe += 1
            if tiefe == 1:
                cur = []
                ents.append(cur)
            elif tiefe == 2:
                prim = []
                patch = False
            continue
        if t == "}":
            if tiefe == 2:
                cur.append(None if patch else prim)
            tiefe -= 1
            continue
        if tiefe == 2:
            if t.startswith("patchDef"):
                patch = True
            elif t.startswith("(") and not patch:
                prim.append([Fraction(x) for x in EBENE.match(t).groups()])
    return ents


def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def kreuz(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def punkt(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def ebenen_von(punkte):
    """q3map2 PlaneFromPoints: Normale (p0-p1)x(p2-p1) zeigt nach aussen, innen n.x <= d."""
    pl = []
    for e in punkte:
        p0, p1, p2 = tuple(e[0:3]), tuple(e[3:6]), tuple(e[6:9])
        n = kreuz(sub(p0, p1), sub(p2, p1))
        if n != (0, 0, 0):
            pl.append((n, punkt(n, p0)))
    return pl


def rang(pts):
    if not pts:
        return -1
    vs = [v for v in (sub(p, pts[0]) for p in pts[1:]) if v != (0, 0, 0)]
    if not vs:
        return 0
    for b in vs[1:]:
        c = kreuz(vs[0], b)
        if c != (0, 0, 0):
            return 3 if any(punkt(c, d) != 0 for d in vs) else 2
    return 1


def ecken(pl):
    e = set()
    for a, b, c in combinations(range(len(pl)), 3):
        (n1, d1), (n2, d2), (n3, d3) = pl[a], pl[b], pl[c]
        det = punkt(n1, kreuz(n2, n3))
        if det == 0:
            continue
        k23, k31, k12 = kreuz(n2, n3), kreuz(n3, n1), kreuz(n1, n2)
        x = tuple((d1 * k23[i] + d2 * k31[i] + d3 * k12[i]) / det for i in range(3))
        if all(punkt(n, x) <= d for n, d in pl):
            e.add(x)
    return list(e)


def offen(pl):
    """Rezessionskegel {v: n.v <= 0} nichttrivial? Kandidaten +-(ni x nj)."""
    ns = [n for n, _ in pl]
    kand = []
    for i in range(len(ns)):
        for j in range(i + 1, len(ns)):
            c = kreuz(ns[i], ns[j])
            if c != (0, 0, 0):
                kand += [c, (-c[0], -c[1], -c[2])]
    if not kand:
        return True
    return any(all(punkt(n, v) <= 0 for n in ns) for v in kand)


def kategorie(pl):
    e = ecken(pl)
    r = rang(e)
    if r < 0:
        return "leer", e
    if r < 3:
        return "flach", e
    if offen(pl):
        return "offen", e
    return "Volumen", e


def beitragend(pl, e):
    return sum(1 for n, d in pl if rang([x for x in e if punkt(n, x) == d]) == 2)


def normiert(pl):
    out = []
    for n, d in pl:
        l = float(punkt(n, n)) ** 0.5
        out.append(((float(n[0]) / l, float(n[1]) / l, float(n[2]) / l), float(d) / l))
    return out


def grund(pl, beitr):
    k, e = kategorie(pl)
    if k != "Volumen":
        return "so geschrieben: " + k
    fl = normiert(pl)
    paare = [(i, j) for i in range(len(fl)) for j in range(len(fl)) if i != j]
    if any(all(abs(fl[i][0][c] + fl[j][0][c]) < 0.0001 for c in range(3)) and abs(fl[i][1] + fl[j][1]) < 0.02
           for i, j in paare):
        return "NRC plane3_opposing (Nadel)"
    weg = {i for i, j in paare
           if all(abs(fl[i][0][c] - fl[j][0][c]) <= 0.001 for c in range(3))
           and (fl[i][1] > fl[j][1] or (fl[i][1] == fl[j][1] and i > j))}
    if weg:
        rest = [pl[i] for i in range(len(pl)) if i not in weg]
        k2, e2 = kategorie(rest)
        if k2 != "Volumen":
            return "NRC plane3_inside (Doppelebene) -> " + k2
        if max(abs(float(c)) for x in e2 for c in x) > 65536:
            return "NRC plane3_inside (Doppelebene) -> ueber die Weltgrenze"
    if max(abs(float(c)) for x in e for c in x) > 65536:
        return "ueber die Weltgrenze"
    if beitr > 0:
        return "nur Euler-Meldung (NRC zeigt ihn)"
    return "unerklaert"


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 2
    wurzel, listendatei = sys.argv[1], sys.argv[2]
    karte_name, karte = None, None
    gueltig_ok = gueltig_falsch = 0
    gruende = {}
    for zeile in open(listendatei, encoding="utf-8"):
        if not zeile.startswith("LISTE "):
            continue
        _, name, ei, pi, gueltig, beitr = zeile.split()
        if "/q3/" not in name:
            continue
        if name != karte_name:
            karte_name, karte = name, karte_lesen(os.path.join(wurzel, name))
        pl = ebenen_von(karte[int(ei)][int(pi)])
        if gueltig == "1":
            k, e = kategorie(pl)
            if k == "Volumen" and beitragend(pl, e) == int(beitr):
                gueltig_ok += 1
            else:
                gueltig_falsch += 1
                print("gueltig, aber exakt", k, name, ei, pi)
        else:
            g = grund(pl, int(beitr))
            gruende[g] = gruende.get(g, 0) + 1
            if g == "unerklaert":
                print("unerklaert:", name, ei, pi)
    print("gueltige Stichprobe: %d exakt bestaetigt, %d abweichend" % (gueltig_ok, gueltig_falsch))
    for g in sorted(gruende):
        print("ungueltig: %-60s %d" % (g, gruende[g]))
    return 1 if gueltig_falsch or gruende.get("unerklaert") else 0


if __name__ == "__main__":
    sys.exit(main())
