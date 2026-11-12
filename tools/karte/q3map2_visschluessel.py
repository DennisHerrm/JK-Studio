#!/usr/bin/env python3
"""lcfix: die Schluessel von VIS-Wiederverwendung (-bhedvisreuse, <karte>.bhedvis) und VIS-Zwischenstand
(-bhedvisstate, <karte>.bhedvisstate) kennen die Regel des VIS und die Fassung von bhed_q3map2.

Fund P4-4 der Codepruefung seit rc611: der Schluessel von -bhedvisreuse enthielt -bhedvisexact und -bhedvisfixed
nicht - ein VIS nach NRCs Regel (mehrere Threads, je Lauf andere Bits) wurde von einem Lauf mit -bhedvisexact oder
-bhedvisfixed uebernommen, die .bsp trug dann nicht das zugesagte Ergebnis.
Fund P4-3: die Fassung des Compilers stand in keinem der beiden Schluessel.

Auf der Gitterkarte von q3map2_visgleich.py (14 x 14 Raeume, wie q3map2_vispause.py):
  * Bezug: -vis -saveprt -bhedvisexact -threads 8 (= -threads 1) und -vis -saveprt -bhedvisfixed -threads 8.
  * NRCs Regel mit -bhedvisreuse (8 Threads), so oft, bis ihr VIS-Lump vom Bezug abweicht; danach auf derselben
    Karte -bhedvisreuse -bhedvisexact bzw. -bhedvisreuse -bhedvisfixed: "visibility computed", VIS gleich dem Bezug.
  * -bhedvisexact nach -bhedvisexact und -threads 1 (ohne Schalter, dasselbe Ergebnis) nach -bhedvisexact:
    "visibility reused", gleich dem Bezug (die Wiederverwendung geht weiter).
  * NRCs Regel nach -bhedvisexact: "visibility computed" (eine andere Regel rechnet einmal neu).
  * -bhedvisreuse einer anderen Fassung (BHED_Q3_FASSUNG): "visibility computed", gleich dem Bezug.
  * Zwischenstand einer anderen Fassung: "vis state not used (state of another version)", gleich dem Bezug.

Aufruf:  q3map2_visschluessel.py ctest <bhed_q3map2.exe> <arbeitsordner>
Nur die Standardbibliothek.
"""
import os
import shutil
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import q3map2_visgleich as vg  # noqa: E402

GITTER = (14, 14)
GRENZE = 600
NACH = 900              # Zwischenstand nach so vielen fertigen Fluessen (die Karte hat ueber 3000)
VERSUCHE = 8            # Laeufe nach NRCs Regel, bis einer vom Bezug abweicht


class Lauf:
    def __init__(self, exe, arbeit, name):
        self.exe = exe
        self.o = os.path.join(arbeit, name)
        if os.path.exists(self.o):
            shutil.rmtree(self.o)
        os.makedirs(os.path.join(self.o, 'spiel', 'base'))
        for e in ('.bsp', '.prt'):
            shutil.copyfile(os.path.join(arbeit, 'gitter' + e), os.path.join(self.o, 'gitter' + e))
        self.text = ''

    def datei(self, endung):
        return os.path.join(self.o, 'gitter' + endung)

    def vis(self, schalter, fassung=None, anhalten=None):
        args = [self.exe, '-game', 'ja', '-fs_basepath', os.path.join(self.o, 'spiel') + os.sep, '-vis', '-saveprt'] + schalter.split() + ['gitter.bsp']
        env = dict(os.environ)
        for k in ('BHED_Q3_FASSUNG', 'BHED_VIS_STOPAFTER', 'BHED_VIS_FIXED_CHAINS'):
            env.pop(k, None)
        if fassung:
            env['BHED_Q3_FASSUNG'] = fassung
        if anhalten:
            env['BHED_VIS_STOPAFTER'] = str(anhalten)
        with open(os.path.join(self.o, 'ausgabe.txt'), 'wb') as f:
            p = subprocess.Popen(args, cwd=self.o, stdout=f, stderr=subprocess.STDOUT, creationflags=vg.IDLE, env=env)
            try:
                rc = p.wait(GRENZE)
            except subprocess.TimeoutExpired:
                subprocess.run(['taskkill', '/PID', str(p.pid), '/T', '/F'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                p.wait()
                rc = 'haengt'
        with open(os.path.join(self.o, 'ausgabe.txt'), encoding='latin1') as f:
            self.text = f.read()
        return rc

    def lump(self):
        return vg.vislump(self.datei('.bsp'))[0]


def ctest(exe, arbeit):
    if os.path.exists(arbeit):
        shutil.rmtree(arbeit)
    os.makedirs(os.path.join(arbeit, 'spiel', 'base'))
    with open(os.path.join(arbeit, 'gitter.map'), 'w', encoding='utf-8', newline='\n') as f:
        f.write(vg.karte(*GITTER))
    if vg.q3(exe, arbeit, ['-meta', 'gitter.map']) != 0:
        print('q3map2_visschluessel: FEHLER - -meta')
        return 1
    fehler = 0

    def pruefe(was, gut, mehr=''):
        nonlocal fehler
        print('q3map2_visschluessel: %-86s %s%s' % (was, 'ok' if gut else 'FEHLER', (' - ' + mehr) if mehr and not gut else ''))
        sys.stdout.flush()
        fehler += not gut

    def meldung(lauf):
        z = [x for x in lauf.text.splitlines() if 'visibility reused' in x or 'visibility computed' in x or 'vis state not used' in x
             or 'vis resumed' in x]
        return z[0].split('BehavEd: ', 1)[-1] if z else '(keine Zeile)'

    def erwartet(lauf, rc, was, text, bezug):
        m = meldung(lauf)
        gut = rc == 0 and text in lauf.text and (bezug is None or lauf.lump() == bezug)
        pruefe(was, gut, 'Rueckgabe %s, "%s", VIS %s' % (rc, m, 'gleich dem Bezug' if rc == 0 and bezug and lauf.lump() == bezug else 'anders'))

    # Bezug je Regel
    b = Lauf(exe, arbeit, 'bezug_exakt')
    if b.vis('-bhedvisexact -threads 8') != 0:
        print('q3map2_visschluessel: FEHLER - Bezug -bhedvisexact')
        return 1
    exakt = b.lump()
    b = Lauf(exe, arbeit, 'bezug_fest')
    if b.vis('-bhedvisfixed -threads 8') != 0:
        print('q3map2_visschluessel: FEHLER - Bezug -bhedvisfixed')
        return 1
    fest = b.lump()

    # P4-4: VIS nach NRCs Regel, dann dieselbe Karte mit -bhedvisexact bzw. -bhedvisfixed
    for regel, schalter, bezug in (('exakt', '-bhedvisexact', exakt), ('fest', '-bhedvisfixed', fest)):
        a = Lauf(exe, arbeit, 'nrc_' + regel)
        nrc = None
        for versuch in range(1, VERSUCHE + 1):
            if os.path.exists(a.datei('.bhedvis')):
                os.remove(a.datei('.bhedvis'))
            if a.vis('-bhedvisreuse -threads 8') != 0:
                break
            nrc = a.lump()
            if nrc != bezug:
                break
        pruefe('NRCs Regel (8 Threads) weicht vom Bezug %s ab (Versuch %d)' % (schalter, versuch), nrc is not None and nrc != bezug)
        rc = a.vis('-bhedvisreuse %s -threads 8' % schalter)
        erwartet(a, rc, '%s nach NRCs Regel: "visibility computed", gleich dem Bezug' % schalter, 'visibility computed', bezug)

    # die Wiederverwendung geht weiter, wo die Regel dieselbe ist
    a = Lauf(exe, arbeit, 'gleich')
    rc = a.vis('-bhedvisreuse -bhedvisexact -threads 8')
    erwartet(a, rc, '-bhedvisexact zum ersten Mal: "visibility computed"', 'visibility computed', exakt)
    rc = a.vis('-bhedvisreuse -bhedvisexact -threads 8')
    erwartet(a, rc, '-bhedvisexact nach -bhedvisexact: "visibility reused"', 'visibility reused', exakt)
    rc = a.vis('-bhedvisreuse -threads 1')
    erwartet(a, rc, '-threads 1 (dasselbe Ergebnis) nach -bhedvisexact: "visibility reused"', 'visibility reused', exakt)
    rc = a.vis('-bhedvisreuse -threads 8')
    erwartet(a, rc, 'NRCs Regel (8 Threads) nach -bhedvisexact: "visibility computed"', 'visibility computed', None)
    rc = a.vis('-bhedvisreuse -bhedvisfixed -threads 8')
    erwartet(a, rc, '-bhedvisfixed nach NRCs Regel: "visibility computed"', 'visibility computed', fest)
    rc = a.vis('-bhedvisreuse -bhedvisfixed -threads 2')
    erwartet(a, rc, '-bhedvisfixed (2 Threads) nach -bhedvisfixed (8 Threads): "visibility reused"', 'visibility reused', fest)

    # P4-3: eine andere Fassung von bhed_q3map2
    a = Lauf(exe, arbeit, 'fassung')
    a.vis('-bhedvisreuse -bhedvisexact -threads 8')
    rc = a.vis('-bhedvisreuse -bhedvisexact -threads 8', fassung='lcfix-andere-fassung')
    erwartet(a, rc, '-bhedvisreuse einer anderen Fassung: "visibility computed", gleich dem Bezug', 'visibility computed', exakt)

    a = Lauf(exe, arbeit, 'stand')
    rc = a.vis('-bhedvisstate -bhedvisstatetime 0.05 -bhedvisexact -threads 1', anhalten=NACH)
    pruefe('Zwischenstand angelegt (Rueckgabe 3)', rc == 3 and os.path.exists(a.datei('.bhedvisstate')), 'Rueckgabe %s' % rc)
    rc = a.vis('-bhedvisstate -bhedvisstatetime 0.05 -bhedvisexact -threads 8', fassung='lcfix-andere-fassung')
    erwartet(a, rc, 'Zwischenstand einer anderen Fassung: "state of another version", gleich dem Bezug',
             'vis state not used (state of another version)', exakt)

    print('q3map2_visschluessel: %s' % ('alles gut' if fehler == 0 else '%d FEHLER' % fehler))
    return 1 if fehler else 0


def main(argv):
    if len(argv) == 3 and argv[0] == 'ctest':
        return ctest(os.path.abspath(argv[1]), os.path.abspath(argv[2]))
    print(__doc__)
    return 2


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
