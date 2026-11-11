#!/usr/bin/env python3
"""Fast doppelte Portal-Ecken: verdeckt das VIS etwas, das man sieht? (Auftrag vis4)

q3map2 - seit id's q3map, NetRadiant-Custom 20260114 genauso - macht im VIS drei Fehler, sobald Ecken eines
Portals fast doppelt in der Portaldatei stehen (wenige Tausendstel auseinander, wie sie der BSP-Schritt an
schraegen Flaechen schreibt):

1. Trennebene mit der Quelle auf beiden Seiten (visflow.cpp, AddSeperators / ClipToSeperators). Die Seite der
   Quelle nimmt der Code vom ERSTEN ihrer Punkte, der weiter als ON_EPSILON von der Ebene liegt. Teilen sich zwei
   Portale eine Kante und steht deren Ecke in beiden mit anderen Ziffern, liegen die drei Punkte der Ebene fast auf
   einer Geraden; das Kreuzprodukt in einfacher Genauigkeit verfehlt dann die eigenen Punkte um mehr als ON_EPSILON,
   eine fast doppelte Ecke der Quelle liegt "vorn" und gilt als Seite der ganzen Quelle. Die Ebene wird genommen,
   obwohl Quelle und Durchgang auf derselben Seite liegen, und schneidet alles dahinter weg.
2. Portal-Ebene aus den ersten drei Punkten (vis.cpp, PlaneFromWinding). Sind zwei davon fast derselbe Punkt, dreht
   sich die Ebene um die Gerade durch die anderen beiden - BasePortalVis und die Fluesse rechnen mit ihr.
3. Liegen die ersten drei Punkte genau auf einer Geraden (die doppelte Ecke an einer Kante laengs einer Achse), gibt
   PlaneFromPoints auf und hinterlaesst die Ebene 0 0 0 0: nichts liegt vor oder hinter ihr, das Portal sieht nichts
   und wird von nichts gesehen.

Die Testkarte ist die kleinste, die alle drei zeigt: drei Ketten aus je vier Blaettern L0 -a- L1 -b- L2 -c- L3 (eigene
Masse; nur die Portaldatei, die .bsp dazu ist ein kleines Gitter - das VIS liest aus ihr nichts als die Entities).
In den Ketten T und E teilen a und b eine Kante (L1 ist ein Keil), deren Ecken in b um Tausendstel anders stehen als
in a, und b hat eine Ecke doppelt: in T am Anfang und am Ende der Wicklung von b (Fehler 1), in E unter den ersten
drei Punkten (Fehler 2). Kette N ist ein gerader Gang aus drei gleichen Quadraten, b hat eine Ecke doppelt an einer
Kante laengs der z-Achse (Fehler 3). Durch a, b und c gehen Geraden von L0 nach L3 (hier ohne das Programm
nachgerechnet), also muss L0 L3 sehen.

Geprueft wird:

1. Gegenprobe ohne das Programm: es gibt Geraden von a durch das Innere von b nach c, in beiden Ketten.
2. Mit NRCs Ebenen (-bhedvisnrcplanes) ist der Visibility-Lump der von NRCs q3map2.exe (NRC unten: -vis,
   -vis -nopassage, -vis -passageOnly geben dort dasselbe, auch mit 8 Threads) - und darin sieht L0 der Kette T
   L3 nicht, L0 der Ketten E und N weder L2 noch L3. -bhedvischeck meldet das: Portale auf Sichtlinien, die in einen
   nicht sichtbaren Cluster fuehren, und Ebenen, die die eigene Wicklung verfehlen oder keine sind.
3. Ohne Schalter (die Vorgabe): in jeder Kette sieht jedes Blatt jedes, mit -vis, -vis -nopassage und
   -vis -passageOnly, mit 1 und 8 Threads, mit -bhedvisfixed und -bhedvisexact, und mit -bhedvisstrictplanes (eine
   Trennebene muss dann auch die drei Punkte halten, aus denen sie gemacht ist). -bhedvischeck findet Sichtlinien
   und kein Portal darauf, das nicht sichtbar ist. Das Programm nennt, was es berichtigt hat.
4. Ein Zwischenstand (-bhedvisstate) und ein gemerktes VIS (-bhedvisreuse) mit NRCs Ebenen werden ohne den Schalter
   nicht uebernommen.

Dass die Vorgabe dort, wo NRC richtig rechnet, dieselben Bytes gibt wie NRCs q3map2.exe, prueft q3map2_visgleich
(Gitterkarte, ohne den Schalter).

Aufruf:  q3map2_visecken.py ctest <bhed_q3map2.exe> <arbeitsordner>
Nur die Standardbibliothek.
"""
import os
import re
import shutil
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import q3map2_visgleich as vg  # noqa: E402
import q3map2_visfest as vf  # noqa: E402

# drei Ketten: Blaetter 0-3 (T: Trennebene), 4-7 (E: Portal-Ebene verdreht), 8-11 (N: keine Portal-Ebene); je Kette c, b, a
PORTALE = '''PRT1
12
9
0
6 2 3 0 (960 384 -200 ) (960 384 -128 ) (960 1408 -128 ) (960 1408 -512 ) (960 944 -512 ) (960 640 -480 )
4 2 1 0 (1279.998047 639.993286 -512.003296 ) (1496.250122 383.991455 -262.509033 ) (1024 384 -208 ) (1280 639.993286 -512.003296 )
3 1 0 0 (1280 384 -512 ) (1280 640 -512 ) (1496.25 384 -262.5 )
6 6 7 0 (960 384 -200 ) (960 384 -128 ) (960 1408 -128 ) (960 1408 -512 ) (960 944 -512 ) (960 640 -480 )
4 6 5 0 (1279.996704 639.993164 -511.994629 ) (1279.994751 639.994629 -511.994507 ) (1496.241089 384.002319 -262.496338 ) (1024 384 -208 )
3 5 4 0 (1280 384 -512 ) (1280 640 -512 ) (1496.25 384 -262.5 )
4 10 11 0 (256 0 0 ) (256 128 0 ) (256 128 128 ) (256 0 128 )
5 10 9 0 (128 0 0 ) (128 0 0.001953 ) (128 0 128 ) (128 128 128 ) (128 128 0 )
4 9 8 0 (0 0 0 ) (0 0 128 ) (0 128 128 ) (0 128 0 )
'''
# sha256 (16 Zeichen) des Visibility-Lumps von NRCs q3map2.exe (NetRadiant-Custom 20260114) auf dieser Portaldatei:
# -vis, -vis -nopassage, -vis -passageOnly mit -threads 1 und -vis mit -threads 8 geben denselben
NRC = '14fd21d4cbfd904a'
# was jedes Blatt dort sieht (Bit k = Blatt k): L0 der Kette T ohne L3; L0 der Kette E ohne L2 und L3, L2 und L3 ohne L0;
# in der Kette N (ein gerader Gang) L0 ohne L2 und L3, L1 ohne L3, L2 ohne L0, L3 ohne L0 und L1
NRC_SICHT = [0x007, 0x00F, 0x00F, 0x00F, 0x030, 0x0F0, 0x0E0, 0x0E0, 0x300, 0x700, 0xE00, 0xC00]
ALLES = [0x00F] * 4 + [0x0F0] * 4 + [0xF00] * 4
ABSTAND = 1.0       # so weit bleiben die Geraden der Gegenprobe von den Kanten von b (wie c_clear in bhed_vischeck.cpp)
LINIEN = 1000       # so viele Sichtlinien muss -bhedvischeck auf der Karte mindestens finden (gemessen: 2078)


def wicklungen():
    aus = []
    for z in PORTALE.splitlines()[4:]:
        t = z.replace('(', ' ').replace(')', ' ').split()
        n = int(t[0])
        r = [float(x) for x in t[-3 * n:]]
        aus.append([tuple(r[3 * i:3 * i + 3]) for i in range(n)])
    return aus


def _sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def _dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def _kreuz(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def _punkte(w, n=6):
    """Punkte im Inneren der Wicklung: im Faecher von Punkt 0 aus"""
    aus = []
    for i in range(1, len(w) - 1):
        for u in range(1, n):
            for v in range(1, n - u):
                aus.append(tuple(w[0][k] + (w[i][k] - w[0][k]) * u / n + (w[i + 1][k] - w[0][k]) * v / n for k in range(3)))
    return aus


def geraden(a, b, c):
    """Zahl der Geraden von Punkten in a zu Punkten in c, die b im Inneren treffen (ABSTAND von jeder Kante,
    Kanten kuerzer als ON_EPSILON zaehlen nicht: das ist die doppelte Ecke)"""
    normal = (0.0, 0.0, 0.0)
    for i in range(1, len(b) - 1):
        k = _kreuz(_sub(b[i], b[0]), _sub(b[i + 1], b[0]))
        normal = (normal[0] + k[0], normal[1] + k[1], normal[2] + k[2])
    laenge = _dot(normal, normal) ** 0.5
    normal = tuple(x / laenge for x in normal)
    dist = sum(_dot(normal, p) for p in b) / len(b)
    zahl = 0
    for p in _punkte(a):
        for q in _punkte(c):
            d = _sub(q, p)
            steigung = _dot(normal, d)
            if abs(steigung) < 1e-9:
                continue
            t = (dist - _dot(normal, p)) / steigung
            if not 0 < t < 1:
                continue
            x = tuple(p[k] + d[k] * t for k in range(3))
            seite = 0
            for i in range(len(b)):
                kante = _sub(b[(i + 1) % len(b)], b[i])
                nach_innen = _kreuz(normal, kante)
                l = _dot(nach_innen, nach_innen) ** 0.5
                if l < 0.1:
                    continue
                e = _dot(nach_innen, _sub(x, b[i])) / l
                if e >= ABSTAND and seite >= 0:
                    seite = 1
                elif e <= -ABSTAND and seite <= 0:
                    seite = -1
                else:
                    seite = None
                    break
            zahl += seite is not None and seite != 0
    return zahl


def sicht(bsp):
    """was jedes der 12 Blaetter sieht (Bit k = Blatt k)"""
    cluster, alle = vf.pvs(bsp)
    if cluster != 12:
        return None
    return [alle >> (64 * i) & 0xFFF for i in range(12)]    # leafbytes ist 8: ein Wort je Blatt


def ctest(exe, arbeit):
    if os.path.exists(arbeit):
        shutil.rmtree(arbeit)
    os.makedirs(os.path.join(arbeit, 'spiel', 'base'))
    fehler = 0

    def pruefe(was, gut, mehr=''):
        nonlocal fehler
        print('q3map2_visecken: %-86s %s%s' % (was, 'ok' if gut else 'FEHLER', (' - ' + mehr) if mehr and not gut else ''))
        fehler += not gut

    # 1. Gegenprobe ohne das Programm
    w = wicklungen()
    for name, (c, b, a) in (('T', w[0:3]), ('E', w[3:6]), ('N', w[6:9])):
        n = geraden(a, b, c)
        pruefe('Kette %s: %d Geraden von a durch das Innere von b nach c (ohne das Programm gerechnet)' % (name, n), n > 0)

    # die .bsp: ein kleines Gitter; die Portaldatei daneben wird durch die Testkarte ersetzt
    if not vf.karte(arbeit, exe, 'ecken', (2, 2)):
        print('q3map2_visecken: FEHLER - -meta')
        return 1
    with open(os.path.join(arbeit, 'ecken.prt'), 'w', encoding='latin1', newline='\n') as f:
        f.write(PORTALE)

    def lauf(name, schalter, threads, **mehr):
        a = vf.Lauf(exe, arbeit, name, 'ecken')
        rc = a.vis(schalter, threads, fest=False, **mehr)
        return a, rc

    def zeile(a, muster):
        m = re.search(muster, a.ausgabe())
        return int(m.group(1)) if m else 0

    # 2. NRCs Ebenen: NRCs Lump, mit den Luecken; die Pruefung meldet sie
    for schalter in ('-vis', '-vis -nopassage', '-vis -passageOnly'):
        a, rc = lauf('nrc', schalter + ' -bhedvisnrcplanes', 1)
        vis = a.lumps()[0] if rc == 0 else '-'
        pruefe('%s -bhedvisnrcplanes: Visibility %s wie NRCs q3map2.exe' % (schalter, vis), vis == NRC, 'NRC %s, Rueckgabe %s' % (NRC, rc))
        if rc == 0:
            s = sicht(a.datei('.bsp'))
            pruefe('%s -bhedvisnrcplanes: L0 der Kette T sieht L3 nicht, L0 der Ketten E und N weder L2 noch L3' % schalter, s == NRC_SICHT, 'Sicht %s' % s)
    a, rc = lauf('nrc', '-vis -bhedvisnrcplanes -bhedvischeck', 1)
    paare, linien, _ = a.pruefung()
    falsch = zeile(a, r'vis check: (\d+) portal planes of the flows miss their own winding by more than 0\.1 or are none')
    pruefe('die Pruefung schlaegt an: %d Portale auf Sichtlinien nicht sichtbar, %d davon in einen nicht sichtbaren Cluster'
           % (linien[2], linien[3]), rc == 0 and linien[2] > 0 and linien[3] > 0, 'Rueckgabe %s' % rc)
    pruefe('die Pruefung zaehlt %d Portal-Ebenen, die ihre Wicklung verfehlen oder keine sind (b der Ketten E und N, beide Seiten)' % falsch, falsch == 4)

    # 3. die Vorgabe: jedes Blatt einer Kette sieht jedes
    for schalter, threads, pruefen in (('-vis', 1, True), ('-vis', 8, False), ('-vis -nopassage', 1, True), ('-vis -nopassage', 8, False),
                                       ('-vis -passageOnly', 1, True), ('-vis -passageOnly', 8, False),
                                       ('-vis -bhedvisfixed', 8, False), ('-vis -bhedvisexact', 8, False),
                                       ('-vis -bhedvisstrictplanes', 1, True)):
        a, rc = lauf('neu', schalter + (' -bhedvischeck' if pruefen else ''), threads)
        s = sicht(a.datei('.bsp')) if rc == 0 else None
        pruefe('%s -threads %d: in allen drei Ketten sieht jedes Blatt jedes' % (schalter, threads), s == ALLES, 'Sicht %s, Rueckgabe %s' % (s, rc))
        if pruefen and rc == 0:
            paare, linien, _ = a.pruefung()
            pruefe('%s: %d Sichtlinien, %d Portale darauf nicht sichtbar, %d ausserhalb portalflood' % (schalter, linien[0], linien[2], paare[1]),
                   linien[0] >= LINIEN and linien[2] == 0 and paare[1] == 0)
            ebenen = zeile(a, r'BehavEd: (\d+) portal planes from all points of the winding')
            passagen = zeile(a, r'BehavEd: (\d+) separating planes left out in the passages')
            fluesse = zeile(a, r'BehavEd: (\d+) separating planes left out in the flows')
            pruefe('%s: %d Portal-Ebenen aus allen Punkten, %d Trennebenen in den Passagen und %d in den Fluessen ausgelassen'
                   % (schalter, ebenen, passagen, fluesse), ebenen == 2 and (fluesse > 0 if 'nopassage' in schalter else passagen > 0))

    # 4. Zwischenstand und gemerktes VIS mit NRCs Ebenen gelten nicht fuer die Vorgabe
    a, rc = lauf('stand', '-vis -bhedvisnrcplanes', 1, stand=True, anhalten=4)
    pruefe('mit NRCs Ebenen angehalten (Rueckgabe 3, Stand da)', rc == 3 and os.path.exists(a.datei('.bhedvisstate')), 'Rueckgabe %s' % rc)
    rc = a.vis('-vis', 1, fest=False, stand=True)
    s = sicht(a.datei('.bsp')) if rc == 0 else None
    pruefe('ohne den Schalter fortgesetzt: Stand nicht uebernommen, jedes Blatt sieht jedes',
           s == ALLES and 'vis state not used (other vis switches)' in a.ausgabe(), 'Sicht %s, Rueckgabe %s' % (s, rc))
    a, rc = lauf('merken', '-vis -saveprt -bhedvisreuse -bhedvisnrcplanes', 1)
    pruefe('-bhedvisreuse mit NRCs Ebenen: VIS gemerkt', rc == 0 and os.path.exists(a.datei('.bhedvis')), 'Rueckgabe %s' % rc)
    rc = a.vis('-vis -saveprt -bhedvisreuse', 1, fest=False)
    s = sicht(a.datei('.bsp')) if rc == 0 else None
    pruefe('-bhedvisreuse ohne den Schalter: neu gerechnet, jedes Blatt sieht jedes',
           s == ALLES and 'visibility computed (no matching' in a.ausgabe(), 'Sicht %s, Rueckgabe %s' % (s, rc))
    rc = a.vis('-vis -saveprt -bhedvisreuse', 1, fest=False)
    s = sicht(a.datei('.bsp')) if rc == 0 else None
    pruefe('-bhedvisreuse noch einmal: uebernommen, jedes Blatt sieht jedes', s == ALLES and 'visibility reused from' in a.ausgabe(),
           'Sicht %s, Rueckgabe %s' % (s, rc))

    print('q3map2_visecken: %s' % ('alles gut' if fehler == 0 else '%d FEHLER' % fehler))
    return 1 if fehler else 0


def main(argv):
    if len(argv) == 3 and argv[0] == 'ctest':
        return ctest(os.path.abspath(argv[1]), os.path.abspath(argv[2]))
    print(__doc__)
    return 2


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
