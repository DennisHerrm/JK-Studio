#!/usr/bin/env python3
"""Bauen anhalten und fortsetzen wie im Karten-Editor (Auftrag vis2c): gleich dem Bau ohne Pause?

Der Editor legt beim Anhalten <karte>.bhedpause an (bhed/karte/baupause.h) und setzt auf zwei Wegen fort:
"Karte wie angehalten" (die uebrigen Schritte auf den Kopien, Ergebnisse zurueck) und "aktuelle Karte" (alle
Schritte neu, der VIS-Zwischenstand kommt vorher zurueck neben die .bsp). Dieser Test baut eine Gitterkarte
mit Lichtern (-meta, -vis -bhedvisexact, -light -bounce 4 -bhedlightcache) mit bhed_q3map2 und ruft dazwischen
ueber tests/kartepause.cpp genau die Dateischritte des Editors auf:
  * Bezug: der Bau ohne Pause.
  * VIS mitten im Fluss angehalten (BHED_VIS_STOPAFTER wie eine Stoppdatei in diesem Augenblick):
    - "Karte wie angehalten": alle Lumps gleich dem Bezug, die Karte daneben unberuehrt, Zwischenstand weg;
    - "aktuelle Karte", Karte unveraendert, dazwischen ein -vis -fast (loeschte einen Zwischenstand neben der
      .bsp): "vis resumed", gleich dem Bezug;
    - "aktuelle Karte", Karte geaendert: "vis state not used (other portals)", gleich dem vollen Bau der
      geaenderten Karte.
  * LIGHT beendet, sobald Durchgang 2 von 5 geschrieben gemeldet ist (taskkill, wie "Bauen anhalten" im Licht):
    - "Karte wie angehalten": der Licht-Schritt findet den geretteten Cache ("2 of 5 passes kept"), nimmt in den
      Durchgaengen 1 und 2 jede Lightmap daraus, gleich dem Bezug;
    - abgebrochen statt angehalten (nur Cache retten, .bsp von vor dem Licht zurueck), danach der Licht-Schritt
      noch einmal: ebenso.
  * NRCs Regel (ohne -bhedvisexact): VIS anhalten, "Karte wie angehalten" - gleiche Clusterzahl, sichtbare
    Paare hoechstens 2 % neben dem Bezug (wie q3map2_vispause).

Aufruf:  q3map2_baupause.py ctest <bhed_q3map2.exe> <kartepause.exe> <arbeitsordner> [nur <fall> ...]
Nur die Standardbibliothek.
"""
import os
import shutil
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import q3map2_visgleich as vg  # noqa: E402
import q3map2_vispause as vp  # noqa: E402

GITTER = (7, 7)         # 49 Raeume: der Fluss hat einige hundert Portalseiten, ein Licht-Durchgang dauert Sekunden
NACH = 150              # VIS anhalten nach so vielen fertigen Fluessen
GRENZE = 900
LICHT = '-light -samples 3 -bounce 4 -threads 4 -bhedlightcache'
PAESSE = 5              # der erste Durchgang und 4 Bounces
NACH_PASS = 2           # LIGHT beenden, sobald dieser Durchgang geschrieben ist


def karte(seed=316):
    """die Gitterkarte mit einem Licht in jedem dritten Raum"""
    text = vg.karte(GITTER[0], GITTER[1], seed)
    # grobe Lightmaps: ein Licht-Durchgang dauert so um die Sekunde statt einer halben Minute
    text = text.replace('"classname" "worldspawn"\n', '"classname" "worldspawn"\n"_lightmapscale" "2"\n', 1)
    for i in range(0, GITTER[0], 3):
        for j in range(0, GITTER[1], 3):
            text += '{\n"classname" "light"\n"origin" "%d %d 200"\n"light" "400"\n}\n' % (i * 256 + 40, j * 256 + 40)
    return text


class Bau:
    def __init__(self, exe, helfer, arbeit, name, text):
        self.exe, self.helfer = exe, helfer
        self.o = os.path.join(arbeit, name).replace('\\', '/')
        if os.path.exists(self.o):
            shutil.rmtree(self.o)
        os.makedirs(os.path.join(self.o, 'spiel', 'base'))
        self.stamm = self.o + '/gitter'
        with open(self.stamm + '.map', 'w', encoding='utf-8', newline='\n') as f:
            f.write(text)
        self.text = ''

    def befehle(self, exakt=True):
        """die drei Schritte, wie der Editor sie baut: -bhedvisstate haengt befehlMitVisStand an"""
        b = '"%s" -game ja -fs_basepath "%s/spiel/" ' % (self.exe.replace('\\', '/'), self.o)
        roh = [b + '-meta "%s.map"' % self.stamm,
               b + '-vis -saveprt %s-threads 8 "%s.map"' % ('-bhedvisexact ' if exakt else '', self.stamm),
               b + LICHT + ' "%s.map"' % self.stamm]
        with open(self.o + '/roh.txt', 'w', encoding='utf-8', newline='\n') as f:
            f.write('\n'.join(roh) + '\n')
        fertig = self.hilf('visstand', self.o + '/roh.txt')[1].splitlines()
        with open(self.o + '/befehle.txt', 'w', encoding='utf-8', newline='\n') as f:
            f.write('\n'.join(fertig) + '\n')
        return fertig

    def hilf(self, *args):
        p = subprocess.run([self.helfer] + list(args), stdout=subprocess.PIPE, stderr=subprocess.STDOUT, encoding='utf-8', errors='replace')
        return p.returncode, p.stdout

    def lauf(self, befehl, stopp=None, bis_pass=None, name='ausgabe.txt'):
        """ein Schritt. stopp: VIS nach so vielen Fluessen anhalten; bis_pass: den Prozess beenden, sobald dieser
        Licht-Durchgang geschrieben gemeldet ist. Rueckgabe des Programms ('beendet' nach dem taskkill)."""
        env = dict(os.environ)
        env.pop('BHED_VIS_STOPAFTER', None)
        if stopp:
            env['BHED_VIS_STOPAFTER'] = str(stopp)
        p = subprocess.Popen(befehl, cwd=self.o, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, creationflags=vg.IDLE, env=env)
        marke = ('light cache pass %d of %d:' % (bis_pass, PAESSE)).encode() if bis_pass else None
        text = b''
        beendet = False
        ende = time.monotonic() + GRENZE
        while True:
            z = p.stdout.readline()
            if not z:
                break
            text += z
            if (marke and marke in z and b'vertex lit surfaces' in z) or time.monotonic() > ende:
                subprocess.run(['taskkill', '/PID', str(p.pid), '/T', '/F'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                beendet = True
                break
        p.wait()
        self.text = text.decode('latin1')
        with open(self.o + '/' + name, 'wb') as f:
            f.write(text)
        return 'beendet' if beendet else p.returncode

    def licht(self, stamm, befehl, **mehr):
        """ein Licht-Schritt wie im Editor: die .bsp vorher sichern, nach dem Ende die Sicherung weg"""
        self.hilf('vorlicht', stamm)
        rc = self.lauf(befehl, **mehr)
        if rc == 0:
            self.hilf('vorlichtweg', stamm)
        return rc

    def alle(self, befehle, ab=0, stamm=None):
        for b in befehle[ab:]:
            rc = self.licht(stamm or self.stamm, b) if ' -light ' in b else self.lauf(b)
            if rc != 0:
                return rc
        return 0

    def lumps(self):
        return vg.vislump(self.stamm + '.bsp')[1]


def ctest(exe, helfer, arbeit, nur):
    fehler = 0

    def pruefe(was, gut, mehr=''):
        nonlocal fehler
        print('q3map2_baupause: %-74s %s%s' % (was, 'ok' if gut else 'FEHLER', (' - ' + str(mehr)) if mehr and not gut else ''), flush=True)
        fehler += not gut

    def will(fall):
        return not nur or fall in nur

    def gleich(bau, bezug):
        lumps = bau.lumps() if os.path.exists(bau.stamm + '.bsp') else []
        anders = [i for i in range(len(bezug)) if i >= len(lumps) or lumps[i] != bezug[i]]
        return not anders, 'anders in Lump %s' % anders

    def ohne_stand(bau):
        return not os.path.exists(bau.stamm + '.bhedpause') and not os.path.exists(bau.stamm + '.bhedprelight')

    t0 = time.monotonic()
    b = Bau(exe, helfer, arbeit, 'bezug', karte())
    befehle = b.befehle()
    pruefe('-bhedvisstate am VIS-Schritt, nur dort', ' -bhedvisstate ' in befehle[1] and '-bhedvisstate' not in befehle[0] + befehle[2])
    if b.alle(befehle) != 0:
        print('q3map2_baupause: FEHLER - der Bezug baut nicht\n' + b.text[-600:])
        return 1
    bezug = b.lumps()
    bezugpaare = vp.paare(b.stamm + '.bsp')
    print('q3map2_baupause: Bezug in %.1f s, %d Cluster, %d sichtbare Paare' % ((time.monotonic() - t0,) + bezugpaare), flush=True)

    def vis_anhalten(name, text=None, exakt=True):
        a = Bau(exe, helfer, arbeit, name, text or karte())
        bef = a.befehle(exakt)
        rc0 = a.lauf(bef[0])
        vorher = open(a.stamm + '.bsp', 'rb').read() if rc0 == 0 else b''
        rc = a.lauf(bef[1], stopp=NACH, name='vis.txt')
        rc2, aus = a.hilf('sichern', a.stamm, '1', a.o + '/befehle.txt', a.o + '/vis.txt')
        ps = a.stamm + '.bhedpause/gitter'
        pruefe(name + ': VIS angehalten (Rueckgabe 3), Zwischenstand mit Karte, .bsp, .prt, .srf und VIS-Stand',
               rc0 == 0 and rc == 3 and rc2 == 0 and all(os.path.exists(ps + e) for e in ('.map', '.bsp', '.prt', '.srf', '.bhedvisstate'))
               and open(ps + '.bsp', 'rb').read() == vorher and not os.path.exists(a.stamm + '.bhedvisstate')
               and os.path.exists(a.stamm + '.bhedpause/stand.json') and 'saved state: step 1, vis ' in aus and 'vis -1 of' not in aus,
               '%s %s %s' % (rc, rc2, aus.strip()))
        return a, bef

    def alt_weiter(a):
        """'Karte wie angehalten': die uebrigen Befehle auf den Kopien, Ergebnisse zurueck"""
        rc, aus = a.hilf('befehlealt', a.stamm)
        rest = aus.splitlines()
        ps = a.stamm + '.bhedpause/gitter'
        texte = []
        for bef in rest:
            r = a.licht(ps, bef) if ' -light ' in bef else a.lauf(bef)
            texte.append(a.text)
            if r != 0:
                return r, texte
        return a.hilf('altfertig', a.stamm)[0], texte

    if will('visalt'):
        a, bef = vis_anhalten('visalt')
        karte_vorher = open(a.stamm + '.map', 'rb').read()
        with open(a.stamm + '.map', 'ab') as f:   # der Anwender hat inzwischen weitergearbeitet
            f.write(b'// spaeter geaendert\n')
        rc, texte = alt_weiter(a)
        g, mehr = gleich(a, bezug) if rc == 0 else (False, 'Rueckgabe %s' % rc)
        pruefe('visalt: "Karte wie angehalten" - "vis resumed", alle Lumps gleich dem Bau ohne Pause',
               g and len(texte) == 2 and 'vis resumed' in texte[0], mehr)
        pruefe('visalt: die Karte daneben bleibt die des Anwenders, Zwischenstand weg',
               open(a.stamm + '.map', 'rb').read() == karte_vorher + b'// spaeter geaendert\n' and ohne_stand(a))

    if will('visaktuell'):
        a, bef = vis_anhalten('visaktuell')
        # ein Testbau dazwischen: -vis -fast -bhedvisstate raeumt am Ende den Zwischenstand neben der .bsp weg
        schnell = bef[1].replace(' -vis ', ' -vis -fast ')
        rc = a.lauf(bef[0]) or a.lauf(schnell)
        rc2, aus = a.hilf('aktuell', a.stamm)
        rc3 = a.alle(bef)
        vis_text = open(a.o + '/ausgabe.txt', 'rb').read().decode('latin1')
        g, mehr = gleich(a, bezug) if rc3 == 0 else (False, 'Rueckgabe %s' % rc3)
        pruefe('visaktuell: nach einem -vis -fast dazwischen kommt der VIS-Stand zurueck neben die .bsp', rc == 0 and rc2 == 0 and 'put back' in aus, aus)
        pruefe('visaktuell: "aktuelle Karte" (unveraendert) - gleich dem Bau ohne Pause, Zwischenstand weg', g and ohne_stand(a), mehr)

    if will('visanders'):
        # ein Pfeiler mehr: andere Portale
        neu = karte().replace('}\n{\n"classname" "info_player_start"', vg._quader(1100, 1100, 0, 1132, 1132, 256) + '}\n{\n"classname" "info_player_start"', 1)
        b2 = Bau(exe, helfer, arbeit, 'bezug2', neu)
        if b2.alle(b2.befehle()) != 0:
            print('q3map2_baupause: FEHLER - der Bezug der geaenderten Karte baut nicht')
            return 1
        a, bef = vis_anhalten('visanders')
        with open(a.stamm + '.map', 'w', encoding='utf-8', newline='\n') as f:
            f.write(neu)
        a.hilf('aktuell', a.stamm)
        texte = []
        rc = 0
        for x in bef:
            rc = rc or (a.licht(a.stamm, x) if ' -light ' in x else a.lauf(x))
            texte.append(a.text)
        g, mehr = gleich(a, b2.lumps()) if rc == 0 else (False, 'Rueckgabe %s' % rc)
        pruefe('visanders: "aktuelle Karte" (geaendert) - "other portals", gleich dem vollen Bau der neuen Karte',
               g and 'vis state not used (other portals)' in texte[1] and b2.lumps() != bezug and ohne_stand(a), mehr)

    def licht_beenden(name):
        a = Bau(exe, helfer, arbeit, name, karte())
        bef = a.befehle()
        rc = a.lauf(bef[0]) or a.lauf(bef[1])
        vor = open(a.stamm + '.bsp', 'rb').read() if rc == 0 else b''
        r = a.licht(a.stamm, bef[2], bis_pass=NACH_PASS, name='licht.txt')
        pruefe(name + ': LIGHT beendet, sobald Durchgang %d von %d geschrieben gemeldet ist' % (NACH_PASS, PAESSE),
               rc == 0 and r == 'beendet' and os.path.exists(a.stamm + '.bhedlight.neu') and not os.path.exists(a.stamm + '.bhedlight'), '%s %s' % (rc, r))
        return a, bef, vor

    def cache_genutzt(text):
        """findet der Licht-Schritt den geretteten Cache und nimmt in den fertigen Durchgaengen jede Lightmap daraus?"""
        z = [x for x in text.splitlines() if 'light cache' in x]
        gefunden = any('found (geometry and switches unchanged, %d of %d passes kept)' % (NACH_PASS, PAESSE) in x for x in z)
        voll = 0
        for n in range(1, PAESSE + 1):
            for x in z:
                if 'pass %d of %d:' % (n, PAESSE) in x and 'lightmaps from the cache' in x:
                    w = x.split(':')[2].split()
                    voll += n <= NACH_PASS and w[0] == w[2] and int(w[2]) > 0
                    voll -= n > NACH_PASS and int(w[0]) > 0
        return gefunden and voll == NACH_PASS, '; '.join(z)[:600]

    if will('lichtalt'):
        a, bef, vor = licht_beenden('lichtalt')
        rc, aus = a.hilf('sichern', a.stamm, '2', a.o + '/befehle.txt', a.o + '/licht.txt')
        ps = a.stamm + '.bhedpause/gitter'
        pruefe('lichtalt: Cache gerettet (%d Durchgaenge), im Zwischenstand die .bsp von vor dem Licht' % NACH_PASS,
               rc == 0 and '%d of %d passes written, rescued' % (NACH_PASS, PAESSE) in aus and os.path.exists(a.stamm + '.bhedlight')
               and not os.path.exists(a.stamm + '.bhedlight.neu') and os.path.exists(ps + '.bhedlight')
               and open(ps + '.bsp', 'rb').read() == vor, aus.strip())
        rc, texte = alt_weiter(a)
        g, mehr = gleich(a, bezug) if rc == 0 else (False, 'Rueckgabe %s' % rc)
        ok, zeilen = cache_genutzt(texte[0] if texte else '')
        pruefe('lichtalt: "Karte wie angehalten" - Durchgang 1 und %d ganz aus dem Cache, die uebrigen gerechnet' % NACH_PASS, ok, zeilen)
        pruefe('lichtalt: alle Lumps gleich dem Bau ohne Pause, Zwischenstand weg', g and ohne_stand(a), mehr)

    if will('lichtabbruch'):
        a, bef, vor = licht_beenden('lichtabbruch')
        rc, aus = a.hilf('lichtretten', a.stamm, a.o + '/licht.txt')
        pruefe('lichtabbruch: nach "Bauen abbrechen" bleibt der Cache der fertigen Durchgaenge, die .bsp ist die von vor dem Licht',
               rc == 0 and 'rescued' in aus and 'not rescued' not in aus and open(a.stamm + '.bsp', 'rb').read() == vor and ohne_stand(a), aus.strip())
        rc = a.licht(a.stamm, bef[2])
        g, mehr = gleich(a, bezug) if rc == 0 else (False, 'Rueckgabe %s' % rc)
        ok, zeilen = cache_genutzt(a.text)
        pruefe('lichtabbruch: der naechste Licht-Schritt nimmt Durchgang 1 und %d aus dem Cache' % NACH_PASS, ok, zeilen)
        pruefe('lichtabbruch: alle Lumps gleich dem Bau ohne Abbruch', g, mehr)

    if will('regel'):
        a, bef = vis_anhalten('regel', exakt=False)
        rc, texte = alt_weiter(a)
        p = vp.paare(a.stamm + '.bsp') if rc == 0 else (0, 0)
        pruefe('regel: NRCs Regel, "Karte wie angehalten" - Paare %d gegen %d (hoechstens 2 %% daneben)' % (p[1], bezugpaare[1]),
               rc == 0 and 'vis resumed' in texte[0] and p[0] == bezugpaare[0] and abs(p[1] - bezugpaare[1]) <= 0.02 * bezugpaare[1] and ohne_stand(a),
               'Rueckgabe %s' % rc)

    print('q3map2_baupause: %s (%.0f s)' % ('alles gleich' if fehler == 0 else '%d FEHLER' % fehler, time.monotonic() - t0))
    return 1 if fehler else 0


def main(argv):
    if len(argv) >= 5 and argv[1] == 'ctest':
        nur = argv[6:] if len(argv) > 6 and argv[5] == 'nur' else []
        return ctest(os.path.abspath(argv[2]), os.path.abspath(argv[3]), os.path.abspath(argv[4]), nur)
    print(__doc__)
    return 2


if __name__ == '__main__':
    sys.exit(main(sys.argv))
