#!/usr/bin/env python3
"""-vis anhalten und fortsetzen: Byte fuer Byte das Ergebnis ohne Pause? (Auftrag vis2 Punkt 3)

bhed_q3map2 -vis -bhedvisstate schreibt die fertigen Fluesse nach <karte>.bhedvisstate (visflow.cpp,
BhedVisState). Eine Datei <karte>.bhedvisstop haelt den VIS an: Rueckgabe 3, die .bsp bleibt
unveraendert, die .prt bleibt liegen. Der naechste Lauf mit derselben .prt und denselben Schaltern
uebernimmt die fertigen Fluesse und rechnet nur den Rest.

Geprueft wird auf der Gitterkarte von q3map2_visgleich.py (groesser, damit ein Thread lange genug
rechnet), je einmal fuer -vis, -vis -nopassage und -vis -passageOnly:
  * Bezug: -vis -threads 1 ohne Pause.
  * Anhalten mitten im Fluss (BHED_VIS_STOPAFTER: nach einer festen Zahl fertiger Fluesse, wie eine
    Stoppdatei in diesem Augenblick - ohne Wettlauf mit der Uhr): Rueckgabe 3, .bsp unveraendert,
    Zwischenstand und .prt da.
  * Fortsetzen mit 8 Threads: "vis resumed", alle Lumps gleich dem Bezug, Zwischenstand geloescht.
Dazu fuer -vis:
  * zweimal anhalten (das zweite Mal mit 8 Threads), dann fortsetzen: gleich dem Bezug.
  * Stoppdatei schon vor dem Start: Rueckgabe 3, kein Fluss gerechnet, Stoppdatei weg, Fortsetzen gleich dem Bezug.
  * beschaedigter Zwischenstand (abgeschnitten): "state damaged", Ergebnis gleich dem Bezug.
  * Zwischenstand anderer Schalter (-nopassage): "other vis switches", Ergebnis gleich dem Bezug.
  * Zwischenstand einer anderen Karte (andere .prt): "other portals", Ergebnis gleich dem Bezug.
  * -vis -saveprt -hint (Schalter vor denen des Zwischenstands): anhalten, fortsetzen, die .prt bleibt.
  * ohne -bhedvisexact (NRCs Regel, nicht deterministisch): Anhalten und Fortsetzen laufen, das
    Ergebnis hat dieselbe Clusterzahl und liegt bei der Zahl sichtbarer Cluster-Paare hoechstens
    2 % neben dem Bezug (gemessen: 0,4 % auf dieser Karte, unter 0,01 % auf grossen Karten).

Aufruf:  q3map2_vispause.py ctest <bhed_q3map2.exe> <arbeitsordner>
Nur die Standardbibliothek.
"""
import os
import shutil
import struct
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import q3map2_visgleich as vg  # noqa: E402

GITTER = (14, 14)       # Raeume: ein Thread rechnet -vis darauf einige Sekunden
GRENZE = 600
NACH = 900              # Anhalten nach so vielen fertigen Fluessen (die Karte hat ueber 3000)


def paare(bsp):
    """(Clusterzahl, sichtbare Cluster-Paare) aus dem Visibility-Lump"""
    d = open(bsp, 'rb').read()
    o, n = struct.unpack_from('<ii', d, 8 + 8 * 16)
    cluster = struct.unpack_from('<i', d, o)[0]
    return cluster, bin(int.from_bytes(d[o + 8:o + n], 'little')).count('1')


class Lauf:
    def __init__(self, exe, arbeit, name):
        self.exe = exe
        self.o = os.path.join(arbeit, name)
        if os.path.exists(self.o):
            shutil.rmtree(self.o)
        os.makedirs(os.path.join(self.o, 'spiel', 'base'))
        for e in ('.bsp', '.prt'):
            shutil.copyfile(os.path.join(arbeit, 'gitter' + e), os.path.join(self.o, 'gitter' + e))
        self.vorher = open(self.datei('.bsp'), 'rb').read()

    def datei(self, endung):
        return os.path.join(self.o, 'gitter' + endung)

    def ausgabe(self):
        with open(os.path.join(self.o, 'ausgabe.txt'), encoding='latin1') as f:
            return f.read()

    def vis(self, schalter, threads, stand=False, exakt=True, anhalten=None):
        """ein -vis-Lauf; anhalten: 'sofort' (Stoppdatei vor dem Start) oder eine Zahl (nach so vielen
        fertigen Fluessen). Rueckgabe des Programms, 'haengt' nach GRENZE Sekunden."""
        args = [self.exe, '-game', 'ja', '-fs_basepath', os.path.join(self.o, 'spiel') + os.sep] + schalter.split()
        if exakt and (threads > 1 or stand):
            args.append('-bhedvisexact')
        if stand:
            args += ['-bhedvisstate', '-bhedvisstatetime', '0.05']
        args += ['-threads', str(threads), 'gitter.bsp']
        if anhalten == 'sofort':
            open(self.datei('.bhedvisstop'), 'w').close()
        env = dict(os.environ)
        env.pop('BHED_VIS_STOPAFTER', None)
        if isinstance(anhalten, int):
            env['BHED_VIS_STOPAFTER'] = str(anhalten)
        with open(os.path.join(self.o, 'ausgabe.txt'), 'wb') as f:
            p = subprocess.Popen(args, cwd=self.o, stdout=f, stderr=subprocess.STDOUT, creationflags=vg.IDLE, env=env)
            try:
                return p.wait(GRENZE)
            except subprocess.TimeoutExpired:
                subprocess.run(['taskkill', '/PID', str(p.pid), '/T', '/F'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                p.wait()
                return 'haengt'


def ctest(exe, arbeit):
    if os.path.exists(arbeit):
        shutil.rmtree(arbeit)
    os.makedirs(os.path.join(arbeit, 'spiel', 'base'))
    with open(os.path.join(arbeit, 'gitter.map'), 'w', encoding='utf-8', newline='\n') as f:
        f.write(vg.karte(*GITTER))
    if vg.q3(exe, arbeit, ['-meta', 'gitter.map']) != 0:
        print('q3map2_vispause: FEHLER - -meta')
        return 1
    fehler = 0

    def pruefe(was, gut, mehr=''):
        nonlocal fehler
        print('q3map2_vispause: %-58s %s%s' % (was, 'ok' if gut else 'FEHLER', (' - ' + mehr) if mehr and not gut else ''))
        fehler += not gut

    def gleich(lauf, bezug):
        lumps = vg.vislump(lauf.datei('.bsp'))[1]
        anders = [i for i in range(len(lumps)) if lumps[i] != bezug[i]]
        return not anders, 'anders in Lump %s' % anders

    def angehalten(lauf, rc, was):
        zeile = [z for z in lauf.ausgabe().splitlines() if 'vis paused' in z]
        pruefe(was + ': Rueckgabe 3, .bsp unveraendert, Stand da', rc == 3 and open(lauf.datei('.bsp'), 'rb').read() == lauf.vorher
               and os.path.exists(lauf.datei('.bhedvisstate')) and os.path.exists(lauf.datei('.prt'))
               and not os.path.exists(lauf.datei('.bhedvisstop')) and bool(zeile), 'Rueckgabe %s' % rc)
        t = zeile[0].split('paused,')[1].split() if zeile else ['-1', 'of', '0']
        return int(t[0]), int(t[2])

    def fortgesetzt(lauf, rc, bezug, was, text='vis resumed'):
        g, mehr = gleich(lauf, bezug) if rc == 0 else (False, 'Rueckgabe %s' % rc)
        pruefe(was + ': "%s", gleich dem Bezug' % text, g and text in lauf.ausgabe() and not os.path.exists(lauf.datei('.bhedvisstate')), mehr)

    bezug = {}
    for schalter in ('-vis', '-vis -nopassage', '-vis -passageOnly'):
        b = Lauf(exe, arbeit, 'bezug')
        t = time.monotonic()
        if b.vis(schalter, 1) != 0:
            print('q3map2_vispause: FEHLER - %s -threads 1' % schalter)
            return 1
        bezug[schalter] = vg.vislump(b.datei('.bsp'))[1]
        print('q3map2_vispause: %-17s -threads 1 ohne Pause %.1f s, %d Cluster, %d sichtbare Paare' % ((schalter, time.monotonic() - t) + paare(b.datei('.bsp'))))
        if schalter == '-vis':
            bezugpaare = paare(b.datei('.bsp'))

        a = Lauf(exe, arbeit, 'pause')
        fertig, alle = angehalten(a, a.vis(schalter, 1, stand=True, anhalten=NACH), '%s anhalten mitten im Fluss' % schalter)
        pruefe('%s angehalten mit fertigen und offenen Fluessen (%d von %d)' % (schalter, fertig, alle), NACH <= fertig < alle)
        fortgesetzt(a, a.vis(schalter, 8, stand=True), bezug[schalter], '%s fortsetzen mit 8 Threads' % schalter)

    schalter = '-vis'
    a = Lauf(exe, arbeit, 'zweimal')
    f1 = angehalten(a, a.vis(schalter, 1, stand=True, anhalten=NACH), 'erstes Anhalten')[0]
    f2 = angehalten(a, a.vis(schalter, 8, stand=True, anhalten=NACH // 2), 'zweites Anhalten (8 Threads)')[0]
    pruefe('zweites Anhalten hat mehr fertige Fluesse (%d, %d)' % (f1, f2), f2 >= f1 + NACH // 2 and f1 >= NACH)
    fortgesetzt(a, a.vis(schalter, 32, stand=True), bezug[schalter], 'nach zwei Pausen fortsetzen')

    a = Lauf(exe, arbeit, 'sofort')
    f0, alle = angehalten(a, a.vis(schalter, 8, stand=True, anhalten='sofort'), 'Stoppdatei vor dem Start')
    pruefe('Stoppdatei vor dem Start: kein Fluss gerechnet (%d von %d fertig)' % (f0, alle), f0 == 0 and alle > 0)
    fortgesetzt(a, a.vis(schalter, 8, stand=True), bezug[schalter], 'danach fortsetzen')

    a = Lauf(exe, arbeit, 'kaputt')
    angehalten(a, a.vis(schalter, 1, stand=True, anhalten=NACH), 'anhalten (fuer beschaedigten Stand)')
    with open(a.datei('.bhedvisstate'), 'r+b') as f:
        f.truncate(os.path.getsize(a.datei('.bhedvisstate')) - 7)
    fortgesetzt(a, a.vis(schalter, 8, stand=True), bezug[schalter], 'beschaedigter Stand', 'vis state not used (state damaged)')

    a = Lauf(exe, arbeit, 'schalter')
    angehalten(a, a.vis('-vis -nopassage', 1, stand=True, anhalten=NACH), 'anhalten mit -nopassage')
    fortgesetzt(a, a.vis(schalter, 8, stand=True), bezug[schalter], 'Stand anderer Schalter', 'vis state not used (other vis switches)')

    a = Lauf(exe, arbeit, 'portale')
    angehalten(a, a.vis(schalter, 1, stand=True, anhalten=NACH), 'anhalten (fuer andere Portale)')
    with open(a.datei('.prt'), 'ab') as f:
        f.write(b'\n')
    with open(a.datei('.prt'), 'rb') as f:
        prt = f.read()
    b = Lauf(exe, arbeit, 'bezug')
    with open(b.datei('.prt'), 'wb') as f:
        f.write(prt)
    if b.vis(schalter, 1) != 0:
        print('q3map2_vispause: FEHLER - -vis mit geaenderter .prt')
        return 1
    fortgesetzt(a, a.vis(schalter, 8, stand=True), vg.vislump(b.datei('.bsp'))[1], 'Stand anderer Portale', 'vis state not used (other portals)')

    # andere Schalter vor denen des Zwischenstands (-bhedvisstatetime nahm einmal das erste uebrige Argument als Zeit)
    schalter2 = '-vis -saveprt -hint'
    b = Lauf(exe, arbeit, 'bezug')
    if b.vis(schalter2, 1) != 0:
        print('q3map2_vispause: FEHLER - %s -threads 1' % schalter2)
        return 1
    a = Lauf(exe, arbeit, 'saveprt')
    angehalten(a, a.vis(schalter2, 1, stand=True, anhalten=NACH), '%s anhalten' % schalter2)
    fortgesetzt(a, a.vis(schalter2, 8, stand=True), vg.vislump(b.datei('.bsp'))[1], '%s fortsetzen' % schalter2)
    pruefe('%s: die .prt bleibt, kein unbekannter Schalter' % schalter2, os.path.exists(a.datei('.prt')) and 'nknown' not in a.ausgabe())

    a = Lauf(exe, arbeit, 'regel')
    fr = angehalten(a, a.vis(schalter, 2, stand=True, exakt=False, anhalten=NACH), 'NRCs Regel anhalten (2 Threads)')[0]
    rc = a.vis(schalter, 8, stand=True, exakt=False)
    p = paare(a.datei('.bsp')) if rc == 0 else (0, 0)
    pruefe('NRCs Regel fortsetzen: Paare %d gegen %d (hoechstens 2 %% daneben)' % (p[1], bezugpaare[1]),
           rc == 0 and fr >= 0 and 'vis resumed' in a.ausgabe() and p[0] == bezugpaare[0] and abs(p[1] - bezugpaare[1]) <= 0.02 * bezugpaare[1]
           and not os.path.exists(a.datei('.bhedvisstate')), 'Rueckgabe %s' % rc)
    exakt = Lauf(exe, arbeit, 'regel2')
    angehalten(exakt, exakt.vis(schalter, 1, stand=True, anhalten=NACH), 'anhalten (deterministisch)')
    rc = exakt.vis(schalter, 8, stand=True, exakt=False)
    pruefe('Stand des deterministischen VIS nicht in NRCs Regel uebernommen', rc == 0 and 'vis state not used (other vis switches)' in exakt.ausgabe(), 'Rueckgabe %s' % rc)

    print('q3map2_vispause: %s' % ('alles gut' if fehler == 0 else '%d FEHLER' % fehler))
    return 1 if fehler else 0


def main(argv):
    if len(argv) == 3 and argv[0] == 'ctest':
        return ctest(os.path.abspath(argv[1]), os.path.abspath(argv[2]))
    print(__doc__)
    return 2


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
