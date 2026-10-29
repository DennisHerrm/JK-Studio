#!/usr/bin/env python3
"""Ist -vis mit mehreren Threads Byte fuer Byte das Ergebnis von -threads 1? (Auftrag vis1)

bhed_q3map2 rechnet den Portalfluss (PassagePortalFlow, PortalFlow, PassageFlow) mit beliebig
vielen Threads so, wie ihn ein Thread rechnet (visflow.cpp, BhedVector): jeder Fluss liest
portalvis genau der Portale vor ihm in der sortierten Reihenfolge, wartet auf noch laufende und
nimmt portalflood fuer alle danach. Mit -threads 1 ist das NRCs Rechnung - die Erwartung unten ist
der Visibility-Lump von NRCs q3map2.exe (NetRadiant-Custom 20260114) mit -threads 1.

Die Testkarte erzeugt dieses Skript (immer dieselbe, ohne Zufall der Python-Bibliothek): ein Gitter
von Raeumen mit Tueren, Fenstern und Pfeilern - genug Portale, damit viele Fluesse gleichzeitig
laufen und aufeinander warten muessen, und klein genug fuer einen schnellen Test.

Aufruf
------
  q3map2_visgleich.py ctest <bhed_q3map2.exe> <arbeitsordner>
      -meta, dann -vis mit -threads 1 und mehrfach mit 8, 32 und 100 Threads, dazu -vis -nopassage und
      -vis -passageOnly mit 1 und 8 Threads: alle Lumps gleich dem 1-Thread-Lauf, der
      Visibility-Lump gleich NRCs (ERWARTUNG).
  q3map2_visgleich.py karte <ziel.map>
      schreibt nur die Testkarte (fuer den Vergleich mit NRCs q3map2.exe).

Nur die Standardbibliothek.
"""
import hashlib
import os
import shutil
import struct
import subprocess
import sys

IDLE = 0x00000040 if os.name == 'nt' else 0

# sha256 (16 Zeichen) des Visibility-Lumps von NRCs q3map2.exe -threads 1 auf der Testkarte
ERWARTUNG = {
    '-vis': 'a1a1f1d839d4599f',
    '-vis -nopassage': '388b016abb0a4fdb',
    '-vis -passageOnly': 'f8f2997bcdc35b36',
}


class _Zufall:
    """feste Folge (LCG wie ANSI C rand), damit die Karte auf jedem Rechner dieselbe ist"""

    def __init__(self, s):
        self.s = s

    def __call__(self, n):
        self.s = (self.s * 1103515245 + 12345) & 0x7fffffff
        return (self.s >> 8) % n


def _quader(x0, y0, z0, x1, y1, z1, tex='test/wand'):
    f = ' %s 0 0 0 0.5 0.5 0 0 0\n' % tex
    e = [((x1, y1, z1), (x1, y0, z1), (x0, y1, z1)), ((x1, y1, z1), (x0, y1, z1), (x1, y1, z0)),
         ((x1, y1, z1), (x1, y1, z0), (x1, y0, z1)), ((x0, y0, z0), (x1, y0, z0), (x0, y1, z0)),
         ((x0, y0, z0), (x0, y0, z1), (x1, y0, z0)), ((x0, y0, z0), (x0, y1, z0), (x0, y0, z1))]
    return '{\n' + ''.join(' '.join('( %d %d %d )' % p for p in d) + f for d in e) + '}\n'


def karte(nx=9, ny=9, seed=316):
    """Gitter aus nx * ny Raeumen (256 Einheiten, Waende 16), je Wand eine Tuer oder ein Fenster
    an fester Zufallsstelle, einzelne Pfeiler und Podeste; aussen dicht."""
    z = _Zufall(seed)
    r, t, h = 256, 16, 256
    w, d = nx * r, ny * r
    b = [_quader(-t, -t, -t, w + t, d + t, 0, 'test/boden'), _quader(-t, -t, h, w + t, d + t, h + t, 'test/decke'),
         _quader(-t, -t, 0, 0, d + t, h), _quader(w, -t, 0, w + t, d + t, h),
         _quader(0, -t, 0, w, 0, h), _quader(0, d, 0, w, d + t, h)]

    def wand(a0, a1, fest, quer):
        # Wandstueck von a0 bis a1 mit einer Oeffnung; quer: Wand parallel zu y (sonst zu x)
        o = a0 + 32 + 16 * z((a1 - a0 - 112) // 16)
        breite = 48 + 16 * z(3)
        art = z(3)    # 0 Tuer, 1 Fenster, 2 breites Tor
        unten, oben = (0, 128) if art == 0 else (96, 176) if art == 1 else (0, 200)
        stuecke = [(a0, o, 0, h), (o + breite, a1, 0, h), (o, o + breite, oben, h)]
        if unten > 0:
            stuecke.append((o, o + breite, 0, unten))
        for s0, s1, z0, z1 in stuecke:
            if s1 > s0 and z1 > z0:
                b.append(_quader(fest, s0, z0, fest + t, s1, z1) if quer else _quader(s0, fest, z0, s1, fest + t, z1))

    for i in range(1, nx):
        for j in range(ny):
            wand(j * r, (j + 1) * r, i * r - t // 2, True)
    for j in range(1, ny):
        for i in range(nx):
            wand(i * r, (i + 1) * r, j * r - t // 2, False)
    for i in range(nx):
        for j in range(ny):
            x, y = i * r + 64 + 16 * z(6), j * r + 64 + 16 * z(6)
            if z(3) == 0:
                b.append(_quader(x, y, 0, x + 32, y + 32, h))          # Pfeiler bis zur Decke
            elif z(3) == 0:
                b.append(_quader(x, y, 0, x + 64, y + 48, 48))         # Podest
    text = '// BehavEd vis1: Testkarte von tools/karte/q3map2_visgleich.py\n{\n"classname" "worldspawn"\n'
    text += ''.join('// brush %d\n' % k + q for k, q in enumerate(b))
    text += '}\n{\n"classname" "info_player_start"\n"origin" "%d %d 64"\n}\n' % (r // 2, r // 2)
    return text


def vislump(bsp):
    d = open(bsp, 'rb').read()
    lumps = []
    for i in range(18 if d[:4] == b'RBSP' else 17):
        o, n = struct.unpack_from('<ii', d, 8 + 8 * i)
        lumps.append(d[o:o + n])
    return hashlib.sha256(lumps[16]).hexdigest()[:16], [hashlib.sha256(x).hexdigest()[:16] for x in lumps]


def q3(exe, ordner, args):
    with open(os.path.join(ordner, 'ausgabe.txt'), 'ab') as f:
        p = subprocess.run([exe, '-game', 'ja', '-fs_basepath', os.path.join(ordner, 'spiel') + os.sep] + args,
                           cwd=ordner, stdout=f, stderr=subprocess.STDOUT, creationflags=IDLE)
    return p.returncode


def ctest(exe, arbeit):
    if os.path.exists(arbeit):
        shutil.rmtree(arbeit)
    os.makedirs(os.path.join(arbeit, 'spiel', 'base'))
    with open(os.path.join(arbeit, 'gitter.map'), 'w', encoding='utf-8', newline='\n') as f:
        f.write(karte())
    if q3(exe, arbeit, ['-meta', 'gitter.map']) != 0:
        print('q3map2_visgleich: FEHLER - -meta')
        return 1
    with open(os.path.join(arbeit, 'gitter.prt'), encoding='latin1') as f:
        f.readline()
        cluster, portale = int(f.readline()), int(f.readline())
    print('q3map2_visgleich: Testkarte %d Cluster, %d Portale' % (cluster, portale))
    fehler = 0
    for schalter in ERWARTUNG:
        # 100: mehr als die 64 Plaetze von NRCs Thread-Tafel (threads.cpp, dort ein Ueberlauf)
        threads = [1, 8, 8, 32, 100] if schalter == '-vis' else [1, 8]
        eins = None
        for k, n in enumerate(threads):
            o = os.path.join(arbeit, 'v%d' % k)
            os.makedirs(o, exist_ok=True)
            for e in ('.bsp', '.prt'):
                shutil.copyfile(os.path.join(arbeit, 'gitter' + e), os.path.join(o, 'gitter' + e))
            os.makedirs(os.path.join(o, 'spiel', 'base'), exist_ok=True)
            if q3(exe, o, schalter.split() + (['-bhedvisexact'] if n > 1 else []) + ['-threads', str(n), 'gitter.bsp']) != 0:
                print('q3map2_visgleich: FEHLER - %s -threads %d' % (schalter, n))
                return 1
            vis, lumps = vislump(os.path.join(o, 'gitter.bsp'))
            if eins is None:
                eins = (vis, lumps)
                soll = ERWARTUNG[schalter]
                gut = soll is None or vis == soll
                print('q3map2_visgleich: %-17s -threads 1: Visibility %s, NRC %s%s' % (schalter, vis, soll or '-', '' if gut else '  FEHLER'))
                fehler += not gut
            else:
                # -threads nimmt main.cpp vorher weg: auch _q3map2_cmdline im Entity-Lump ist gleich
                anders = [i for i in range(len(lumps)) if lumps[i] != eins[1][i]]
                print('q3map2_visgleich: %-17s -threads %-2d: %s' % (schalter, n, 'gleich' if not anders else 'ANDERS in Lump %s' % anders))
                fehler += bool(anders)
            shutil.rmtree(o)
    print('q3map2_visgleich: %s' % ('alles gleich' if fehler == 0 else '%d FEHLER' % fehler))
    return 1 if fehler else 0


def main(argv):
    if len(argv) == 3 and argv[0] == 'ctest':
        return ctest(os.path.abspath(argv[1]), os.path.abspath(argv[2]))
    if len(argv) == 2 and argv[0] == 'karte':
        with open(argv[1], 'w', encoding='utf-8', newline='\n') as f:
            f.write(karte())
        return 0
    print(__doc__)
    return 2


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
