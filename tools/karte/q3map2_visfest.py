#!/usr/bin/env python3
"""Die feste VIS-Regel -bhedvisfixed: reproduzierbar, gueltig, anhaltbar? (Auftrag vis3)

Mit mehreren Threads nimmt NRCs q3map2 fuer ein Portal, das ein Fluss passiert, portalvis, wenn dessen
Fluss gerade fertig ist, sonst portalflood - jeder Lauf faellt etwas anders aus. Mit -bhedvisfixed legt
eine feste Maschine fest, was als fertig gilt (visflow.cpp, g_bhedFixed): eine feste Zahl von Ketten,
jede mit ihren Fluessen, die im selben Takt rechnen; ein Fluss liest portalvis genau der Fluesse, die
nach der Uhr dieser Maschine bis zu seiner Sicht geendet haben. Das Ergebnis haengt weder vom Lauf noch
von der Thread-Zahl ab.

Geprueft wird auf der Gitterkarte von q3map2_visgleich.py:

1. Reproduzierbar (Gitter 9 x 9): -vis -bhedvisfixed mit 1, 2, 8, 8, 32 und 100 Threads, dazu
   -vis -nopassage und -vis -passageOnly mit 1 und 8 Threads - alle Lumps gleich dem Lauf mit einem Thread.
2. Die Regel selbst: mit einer einzigen Kette (BHED_VIS_FIXED_CHAINS=1) gibt es nur die Reihenfolge eines
   Threads - der Visibility-Lump ist der von NRCs q3map2.exe -threads 1 (ERWARTUNG in q3map2_visgleich.py).
3. Gueltig (Gitter 14 x 14, -bhedvischeck, bhed_vischeck.h): nichts ausserhalb von portalflood; jede Sichtlinie
   durch das Innere der Portale trifft nur sichtbare Portale; gegen den Lauf mit einem Thread (NRCs Regel)
   werden die Paare gezaehlt, die nur einer von beiden hat, und fuer jedes Paar, das nur der andere hat,
   Sichtlinien zwischen genau diesen zwei Portalen gesucht - keine darf es geben. Dieselbe Pruefung besteht
   auch der Lauf mit einem Thread (Gegenprobe). Dass die Pruefung anschlaegt, zeigt BHED_VIS_CHECK_HIDE: sie
   sieht dann einen Teil der sichtbaren Paare als nicht sichtbar an und muss Sichtlinien dazu melden.
   Auf der fertigen .bsp: die Cluster-Sichtbarkeit ist eine Teilmenge der von -vis -fast (portalflood).
4. Nicht lockerer: die Zahl sichtbarer Cluster-Paare liegt hoechstens DICHTE neben dem Lauf mit einem Thread.
5. Anhalten und Fortsetzen (-bhedvisstate, BHED_VIS_STOPAFTER wie in q3map2_vispause.py): mitten im Fluss
   angehalten und mit anderer Thread-Zahl fortgesetzt, auch zweimal, auch -nopassage und -passageOnly -
   alle Lumps gleich dem Lauf ohne Pause. Ein Zwischenstand von NRCs Regel wird nicht uebernommen.

Aufruf:  q3map2_visfest.py ctest <bhed_q3map2.exe> <arbeitsordner>
Nur die Standardbibliothek.
"""
import os
import re
import shutil
import struct
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import q3map2_visgleich as vg  # noqa: E402

GITTER = (14, 14)       # Raeume fuer Gueltigkeit und Pause: ein Thread rechnet -vis darauf einige Sekunden
GRENZE = 600
NACH = 900              # Anhalten nach so vielen fertigen Fluessen (die Karte hat ueber 3000)
LINIEN = 20000          # so viele Sichtlinien muss die Pruefung auf dem grossen Gitter mindestens finden
DICHTE = 0.02           # Anteil sichtbarer Cluster-Paare, um den die feste Regel neben einem Thread liegen darf


def pvs(bsp):
    """(Clusterzahl, Sichtbarkeit aller Cluster als eine Zahl)"""
    d = open(bsp, 'rb').read()
    o, n = struct.unpack_from('<ii', d, 8 + 8 * 16)
    return struct.unpack_from('<i', d, o)[0], int.from_bytes(d[o + 8:o + n], 'little')


def bits(x):
    return bin(x).count('1')


class Lauf:
    def __init__(self, exe, arbeit, name, karte):
        self.exe = exe
        self.o = os.path.join(arbeit, name)
        if os.path.exists(self.o):
            shutil.rmtree(self.o)
        os.makedirs(os.path.join(self.o, 'spiel', 'base'))
        for e in ('.bsp', '.prt'):
            shutil.copyfile(os.path.join(arbeit, karte + e), os.path.join(self.o, 'k' + e))
        self.vorher = open(self.datei('.bsp'), 'rb').read()

    def datei(self, endung):
        return os.path.join(self.o, 'k' + endung)

    def ausgabe(self):
        with open(os.path.join(self.o, 'ausgabe.txt'), encoding='latin1') as f:
            return f.read()

    def lumps(self):
        return vg.vislump(self.datei('.bsp'))

    def vis(self, schalter, threads, fest=True, stand=False, anhalten=None, umgebung=None):
        """ein -vis-Lauf; anhalten: nach so vielen fertigen Fluessen. Rueckgabe des Programms, 'haengt' nach GRENZE s."""
        args = [self.exe, '-game', 'ja', '-fs_basepath', os.path.join(self.o, 'spiel') + os.sep] + schalter.split()
        if fest:
            args.append('-bhedvisfixed')
        if stand:
            args += ['-bhedvisstate', '-bhedvisstatetime', '0.05']
        args += ['-threads', str(threads), 'k.bsp']
        env = dict(os.environ)
        for name in ('BHED_VIS_STOPAFTER', 'BHED_VIS_FIXED_CHAINS', 'BHED_VIS_CHECK_REF', 'BHED_VIS_CHECK_DUMP', 'BHED_VIS_CHECK_HIDE',
                     'BHED_VIS_CHECK_LINES'):
            env.pop(name, None)
        if anhalten is not None:
            env['BHED_VIS_STOPAFTER'] = str(anhalten)
        env.update(umgebung or {})
        with open(os.path.join(self.o, 'ausgabe.txt'), 'wb') as f:
            p = subprocess.Popen(args, cwd=self.o, stdout=f, stderr=subprocess.STDOUT, creationflags=vg.IDLE, env=env)
            try:
                return p.wait(GRENZE)
            except subprocess.TimeoutExpired:
                p.kill()    # nur dieser eine Prozess: q3map2 startet keine weiteren
                p.wait()
                return 'haengt'

    def pruefung(self):
        """die Zahlen von -bhedvischeck: (Paare, ausserhalb portalflood), (Sichtlinien, Portale darauf, nicht sichtbar, davon
        in einen nicht sichtbaren Cluster), (Paare nur hier, nur im Bezug, Sichtlinie gefunden, auf Sichtlinien im Bezug
        sichtbar und hier nicht) - fehlt eine Zeile, steht -1 an den Stellen, die 0 sein muessen"""
        paare, linien, bezug = [0, -1], [0, 0, -1, -1], [0, 0, -1, -1]
        for z in self.ausgabe().splitlines():
            if z.startswith('BehavEd: vis check: ') and 'visible portal pairs' in z:
                paare = [int(x) for x in re.findall(r'\d+', z)]
            elif z.startswith('BehavEd: vis check: ') and 'sight lines' in z:
                linien = [int(x) for x in re.findall(r'\d+', z)]
            elif z.startswith('BehavEd: vis check against '):
                bezug = [int(x) for x in re.findall(r'\d+', z.rsplit(': ', 1)[-1])]
        return paare, linien, bezug


def karte(arbeit, exe, name, gitter):
    with open(os.path.join(arbeit, name + '.map'), 'w', encoding='utf-8', newline='\n') as f:
        f.write(vg.karte(*gitter))
    return vg.q3(exe, arbeit, ['-meta', name + '.map']) == 0


def ctest(exe, arbeit):
    if os.path.exists(arbeit):
        shutil.rmtree(arbeit)
    os.makedirs(os.path.join(arbeit, 'spiel', 'base'))
    if not karte(arbeit, exe, 'klein', (9, 9)) or not karte(arbeit, exe, 'gross', GITTER):
        print('q3map2_visfest: FEHLER - -meta')
        return 1
    fehler = 0

    def pruefe(was, gut, mehr=''):
        nonlocal fehler
        print('q3map2_visfest: %-72s %s%s' % (was, 'ok' if gut else 'FEHLER', (' - ' + mehr) if mehr and not gut else ''))
        fehler += not gut

    # 1. und 2.: reproduzierbar, und mit einer Kette NRCs -threads 1
    for schalter in vg.ERWARTUNG:
        eins = None
        for k, n in enumerate([1, 2, 8, 8, 32, 100] if schalter == '-vis' else [1, 8]):
            a = Lauf(exe, arbeit, 'r%d' % k, 'klein')
            rc = a.vis(schalter, n)
            lumps = a.lumps()[1] if rc == 0 else None
            if eins is None:
                eins = lumps
                pruefe('%s -bhedvisfixed -threads 1 laeuft mit der festen Regel' % schalter, lumps is not None and 'fixed vis rule' in a.ausgabe(), 'Rueckgabe %s' % rc)
            else:
                anders = 'Rueckgabe %s' % rc if lumps is None or eins is None else 'anders in Lump %s' % [i for i in range(len(lumps)) if lumps[i] != eins[i]]
                pruefe('%s -bhedvisfixed -threads %d: gleich dem Lauf mit einem Thread' % (schalter, n), lumps is not None and lumps == eins, anders)
        a = Lauf(exe, arbeit, 'kette', 'klein')
        rc = a.vis(schalter, 8, umgebung={'BHED_VIS_FIXED_CHAINS': '1'})
        vis = a.lumps()[0] if rc == 0 else '-'
        pruefe('%s feste Regel mit einer Kette: Visibility %s wie NRC -threads 1' % (schalter, vis),
               vis == vg.ERWARTUNG[schalter] and 'fixed vis rule' in a.ausgabe(), 'NRC %s' % vg.ERWARTUNG[schalter])

    # 3. gueltig
    eins = Lauf(exe, arbeit, 'eins', 'gross')
    bild = os.path.join(arbeit, 'eins.portalvis')
    rc = eins.vis('-vis -bhedvischeck', 1, fest=False, umgebung={'BHED_VIS_CHECK_DUMP': bild})
    paare, linien, _ = eins.pruefung()
    pruefe('Gegenprobe, ein Thread: %d Sichtlinien, %d Portale darauf nicht sichtbar, %d ausserhalb portalflood' % (linien[0], linien[2], paare[1]),
           rc == 0 and linien[0] >= LINIEN and linien[2] == 0 and paare[1] == 0 and os.path.exists(bild), 'Rueckgabe %s' % rc)
    fest = Lauf(exe, arbeit, 'fest', 'gross')
    rc = fest.vis('-vis -bhedvischeck', 8, umgebung={'BHED_VIS_CHECK_REF': bild})
    paare, linien, bezug = fest.pruefung()
    pruefe('feste Regel: %d sichtbare Portalpaare, %d ausserhalb portalflood' % (paare[0], paare[1]), rc == 0 and paare[0] > 0 and paare[1] == 0, 'Rueckgabe %s' % rc)
    pruefe('feste Regel: %d Sichtlinien, %d Portale darauf nicht sichtbar' % (linien[0], linien[2]), rc == 0 and linien[0] >= LINIEN and linien[2] == 0)
    pruefe('feste Regel gegen einen Thread: %d Paare nur hier, %d nur dort, Sichtlinie fuer %d davon' % tuple(bezug[:3]),
           rc == 0 and bezug[2] == 0 and bezug[3] == 0)
    blind = Lauf(exe, arbeit, 'blind', 'gross')
    rc = blind.vis('-vis -bhedvischeck', 8, umgebung={'BHED_VIS_CHECK_HIDE': '50'})
    linien = blind.pruefung()[1]
    pruefe('die Pruefung schlaegt an: jedes 50. Paar verdeckt, %d Portale auf Sichtlinien nicht sichtbar' % linien[2], rc == 0 and linien[2] > 0)
    schnell = Lauf(exe, arbeit, 'schnell', 'gross')
    rc = schnell.vis('-vis -fast', 1, fest=False)
    if rc == 0 and fest.lumps() is not None:
        (c0, flut), (c1, f), (c2, e) = pvs(schnell.datei('.bsp')), pvs(fest.datei('.bsp')), pvs(eins.datei('.bsp'))
        pruefe('Cluster: feste Regel %d Paare, alle in -vis -fast (%d Paare)' % (bits(f), bits(flut)), c0 == c1 and f & ~flut == 0 and bits(f) > 0)
        # 4. nicht lockerer
        pruefe('Cluster: feste Regel %d Paare, ein Thread %d (hoechstens %.0f %% daneben)' % (bits(f), bits(e), 100 * DICHTE),
               c1 == c2 and abs(bits(f) - bits(e)) <= DICHTE * bits(e))
    else:
        pruefe('-vis -fast', False, 'Rueckgabe %s' % rc)

    # 5. anhalten und fortsetzen
    def angehalten(lauf, rc, was):
        zeile = [z for z in lauf.ausgabe().splitlines() if 'vis paused' in z]
        pruefe(was + ': Rueckgabe 3, .bsp unveraendert, Stand da', rc == 3 and open(lauf.datei('.bsp'), 'rb').read() == lauf.vorher
               and os.path.exists(lauf.datei('.bhedvisstate')) and os.path.exists(lauf.datei('.prt'))
               and not os.path.exists(lauf.datei('.bhedvisstop')) and bool(zeile), 'Rueckgabe %s' % rc)
        t = zeile[0].split('paused,')[1].split() if zeile else ['-1', 'of', '0']
        return int(t[0]), int(t[2])

    def fortgesetzt(lauf, rc, bezug, was, text='vis resumed'):
        lumps = lauf.lumps()[1] if rc == 0 else None
        mehr = 'Rueckgabe %s' % rc if lumps is None else 'anders in Lump %s' % [i for i in range(len(lumps)) if lumps[i] != bezug[i]]
        pruefe(was + ': "%s", gleich dem Lauf ohne Pause' % text,
               lumps == bezug and text in lauf.ausgabe() and not os.path.exists(lauf.datei('.bhedvisstate')), mehr)

    bezug = {}
    for schalter in ('-vis', '-vis -nopassage', '-vis -passageOnly'):
        b = Lauf(exe, arbeit, 'bezug', 'gross')
        if b.vis(schalter, 8) != 0:
            print('q3map2_visfest: FEHLER - %s -bhedvisfixed' % schalter)
            return 1
        bezug[schalter] = b.lumps()[1]
        a = Lauf(exe, arbeit, 'pause', 'gross')
        fertig, alle = angehalten(a, a.vis(schalter, 1, stand=True, anhalten=NACH), '%s anhalten mitten im Fluss (1 Thread)' % schalter)
        pruefe('%s angehalten mit fertigen und offenen Fluessen (%d von %d)' % (schalter, fertig, alle), 0 < fertig < alle)
        fortgesetzt(a, a.vis(schalter, 8, stand=True), bezug[schalter], '%s fortsetzen mit 8 Threads' % schalter)
    schalter = '-vis'
    pruefe('die feste Regel mit 8 Threads ist die mit einem Thread aus Punkt 3', bezug[schalter] == fest.lumps()[1])
    a = Lauf(exe, arbeit, 'zweimal', 'gross')
    f1 = angehalten(a, a.vis(schalter, 8, stand=True, anhalten=NACH), 'erstes Anhalten (8 Threads)')[0]
    f2 = angehalten(a, a.vis(schalter, 3, stand=True, anhalten=NACH // 2), 'zweites Anhalten (3 Threads)')[0]
    pruefe('zweites Anhalten hat mehr fertige Fluesse (%d, %d)' % (f1, f2), f2 > f1 > 0)
    fortgesetzt(a, a.vis(schalter, 32, stand=True), bezug[schalter], 'nach zwei Pausen fortsetzen (32 Threads)')
    a = Lauf(exe, arbeit, 'kaputt', 'gross')
    angehalten(a, a.vis(schalter, 8, stand=True, anhalten=NACH), 'anhalten (fuer beschaedigten Stand)')
    with open(a.datei('.bhedvisstate'), 'r+b') as f:
        f.truncate(os.path.getsize(a.datei('.bhedvisstate')) - 7)
    fortgesetzt(a, a.vis(schalter, 8, stand=True), bezug[schalter], 'beschaedigter Stand', 'vis state not used (state damaged)')
    a = Lauf(exe, arbeit, 'regel', 'gross')
    angehalten(a, a.vis(schalter, 2, fest=False, stand=True, anhalten=NACH), 'NRCs Regel anhalten')
    fortgesetzt(a, a.vis(schalter, 8, stand=True), bezug[schalter], 'Stand von NRCs Regel nicht in die feste uebernommen', 'vis state not used (other vis switches)')

    print('q3map2_visfest: %s' % ('alles gut' if fehler == 0 else '%d FEHLER' % fehler))
    return 1 if fehler else 0


def main(argv):
    if len(argv) == 3 and argv[0] == 'ctest':
        return ctest(os.path.abspath(argv[1]), os.path.abspath(argv[2]))
    print(__doc__)
    return 2


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
