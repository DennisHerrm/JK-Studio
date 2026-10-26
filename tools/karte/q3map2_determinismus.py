#!/usr/bin/env python3
"""Ist bhed_q3map2 deterministisch? (Stufe 6 Punkt 3, GL#173)

Je Stufe (-bsp, -vis, -light) dieselbe Eingabe mehrmals bauen, mit mehreren
Threads und einmal mit -threads 1, und die Lumps der .bsp (dazu .prt/.srf)
vergleichen. Jede Stufe bekommt die Ausgabe des ersten Laufs der Stufe davor,
damit eine Abweichung genau einer Stufe zugeordnet wird.

Aufruf
------
  q3map2_determinismus.py <bhed_q3map2.exe> <karte.map> [--basepath <GameData>]
      [--game ja] [--laeufe 3] [--threads N] [--stufen bsp,vis,light] [--arbeit <ordner>]
      [--json <datei>]

  q3map2_determinismus.py ctest <bhed_q3map2.exe> <tests/karte/q3map2>
      ctest q3map2_determinismus: raum.map (-game ja) und eine Fassung mit
      Lichtstilen ("style" 1 und 5) mit -game ja und -game quake3 (dort legt
      q3map2 die Stil-Shader an, GL#173), je 3 Laeufe + 1 Thread.

-vis laeuft dabei immer mit -threads 1: mit mehreren Threads haengt NRCs Ergebnis von der
Reihenfolge der Threads ab (ein Portal schneidet mit portalvis der Portale, die gerade fertig
sind - auch mitten im Lauf). Jede Regel, die das unabhaengig von der Thread-Zahl macht, war auf
kejim_post mehr als doppelt so langsam wie NRC (NOTIZEN_karte6.md, offen fuer Stufe 7);
-bsp und -light sind mit beliebig vielen Threads deterministisch.

Ohne --basepath ist -fs_basepath ein leerer Ordner (alle Shader implizit).
Rueckgabe 0: alle Stufen in allen Laeufen gleich und gleich dem 1-Thread-Lauf.

Nur die Standardbibliothek.
"""
import argparse
import hashlib
import json
import os
import shutil
import struct
import subprocess
import sys
import tempfile

NAMEN = ['entities', 'shaders', 'planes', 'nodes', 'leafs', 'leafsurfaces', 'leafbrushes', 'models', 'brushes', 'brushsides',
         'drawverts', 'drawindexes', 'fogs', 'surfaces', 'lightmaps', 'lightgrid', 'visibility', 'lightarray']
# data/karte/ja/default_build_menu.xml "[Final] Fast"
STUFEN = {
    'bsp': ['-meta'],
    'vis': ['-vis'],
    'light': ['-light', '-fast', '-patchshadows', '-samples', '2', '-filter', '-bounce', '8'],
}
IDLE = 0x00000040 if os.name == 'nt' else 0


def h(b):
    return hashlib.sha256(b).hexdigest()[:16]


def merkmale(bsp):
    d = open(bsp, 'rb').read()
    m = {}
    if d[:4] in (b'RBSP', b'IBSP'):
        # IBSP: 17 Lumps (ohne lightarray); dazwischen steht NRCs Zeitstempel
        # "I LOVE MY Q3MAP2 ... on <Datum>" - gewollt, darum nur die Lumps
        for i, n in enumerate(NAMEN if d[:4] == b'RBSP' else NAMEN[:17]):
            o, l = struct.unpack_from('<ii', d, 8 + 8 * i)
            m[n] = h(d[o:o + l])
    else:
        m['datei'] = h(d)
    for ext in ('.prt', '.srf'):
        p = bsp[:-4] + ext
        if os.path.exists(p):
            m[ext] = h(open(p, 'rb').read())
    return m


PARALLEL = []   # Stufe 7: je Lichtlauf mit mehreren Threads die Zahl der parallel gerechneten Lightmaps


def lauf(exe, ordner, karte, stufe, game, basis, threads, eingabe, quelle):
    if os.path.exists(ordner):
        shutil.rmtree(ordner)
    os.makedirs(os.path.join(ordner, 'maps'))
    shutil.copyfile(quelle, os.path.join(ordner, 'maps', karte + '.map'))
    for ext, q in (eingabe or {}).items():
        shutil.copyfile(q, os.path.join(ordner, 'maps', karte + ext))
    args = [exe, '-game', game, '-fs_basepath', basis.rstrip('/\\') + os.sep] + STUFEN[stufe]
    if threads:
        args += ['-threads', str(threads)]
    args.append('maps/%s.map' % karte)
    # Stufe 7: BHED_Q3_ZEITEN laesst q3map2 Abschnittszeiten und die Zahl der parallel
    # gerechneten grossen Lightmaps melden ("BHED-Parallel: n") - die Ausgabe-BSP bleibt gleich.
    env = dict(os.environ, BHED_Q3_ZEITEN='1')
    with open(os.path.join(ordner, 'ausgabe.txt'), 'wb') as f:
        p = subprocess.run(args, cwd=ordner, stdout=f, stderr=subprocess.STDOUT, creationflags=IDLE, env=env)
    if stufe == 'light' and threads != 1:
        with open(os.path.join(ordner, 'ausgabe.txt'), 'rb') as f:
            for z in f.read().decode('utf-8', 'replace').splitlines():
                if z.startswith('BHED-Parallel: '):
                    PARALLEL.append(int(z.split()[1]))
    bsp = os.path.join(ordner, 'maps', karte + '.bsp')
    if p.returncode != 0 or not os.path.exists(bsp):
        raise RuntimeError('%s %s: Rueckgabe %d (%s)' % (karte, stufe, p.returncode, os.path.join(ordner, 'ausgabe.txt')))
    return bsp, merkmale(bsp)


def ctest(exe, daten):
    arbeit = tempfile.mkdtemp(prefix='q3det_')
    try:
        stil = os.path.join(arbeit, 'raum_stil.map')
        text = open(os.path.join(daten, 'raum.map'), encoding='utf-8').read()
        text = text.replace('"light" "400"\n', '"light" "400"\n"style" "1"\n', 1).replace('"light" "300"\n', '"light" "300"\n"style" "5"\n', 1)
        assert text.count('"style"') == 2, 'raum.map: Lichter nicht gefunden'
        with open(stil, 'w', encoding='utf-8', newline='\n') as f:
            f.write(text)
        rc = 0
        for karte, game in ((os.path.join(daten, 'raum.map'), 'ja'), (stil, 'ja'), (stil, 'quake3')):
            rc |= main([exe, karte, '--game', game, '--laeufe', '3', '--arbeit', os.path.join(arbeit, game + '_' + os.path.basename(karte)[:-4])])
        print('q3map2_determinismus: %s' % ('alle Stufen deterministisch, gleich -threads 1' if rc == 0 else 'NICHT deterministisch'))
        # Stufe 7: die grossen Lightmaps rechnen mit mehreren Threads zeilenweise parallel
        # (BhedIlluminateRawLightmaps) - der Vergleich oben deckt den Weg nur ab, wenn er lief.
        if (os.cpu_count() or 1) >= 4:
            n = max(PARALLEL, default=0)
            print('q3map2_determinismus: zeilenweise parallel gerechnete Lightmaps je Lauf bis %d' % n)
            if n == 0:
                print('q3map2_determinismus: FEHLER - der parallele Weg fuer grosse Lightmaps lief nicht')
                rc = 1
        return rc
    finally:
        shutil.rmtree(arbeit, ignore_errors=True)


def main(argv):
    if argv and argv[0] == 'ctest':
        return ctest(os.path.abspath(argv[1]), os.path.abspath(argv[2]))
    ap = argparse.ArgumentParser()
    ap.add_argument('exe')
    ap.add_argument('karte')
    ap.add_argument('--basepath')
    ap.add_argument('--game', default='ja')
    ap.add_argument('--laeufe', type=int, default=3)
    ap.add_argument('--threads', type=int, default=0, help='0: q3map2 waehlt (alle Kerne)')
    ap.add_argument('--stufen', default='bsp,vis,light')
    ap.add_argument('--arbeit')
    ap.add_argument('--json')
    a = ap.parse_args(argv)
    a.exe = os.path.abspath(a.exe)
    a.karte = os.path.abspath(a.karte)
    karte = os.path.splitext(os.path.basename(a.karte))[0]
    arbeit = a.arbeit or tempfile.mkdtemp(prefix='q3det_')
    basis = a.basepath or os.path.join(arbeit, 'spiel')
    if not a.basepath:
        os.makedirs(os.path.join(basis, 'base'), exist_ok=True)
    ergebnis = {}
    eingabe = None
    gut = True
    for stufe in a.stufen.split(','):
        laeufe = []
        erste = None
        for r in range(a.laeufe):
            threads = 1 if stufe == 'vis' else a.threads   # siehe oben
            bsp, m = lauf(a.exe, os.path.join(arbeit, '%s_%s_%d' % (karte, stufe, r)), karte, stufe, a.game, basis, threads, eingabe, a.karte)
            laeufe.append(m)
            if erste is None:
                erste = bsp
        _, eins = lauf(a.exe, os.path.join(arbeit, '%s_%s_t1' % (karte, stufe)), karte, stufe, a.game, basis, 1, eingabe, a.karte)
        schwankt = sorted({k for m in laeufe[1:] for k in m if m[k] != laeufe[0].get(k)})
        anders1 = sorted(k for k in eins if eins[k] != laeufe[0].get(k))
        ergebnis[stufe] = {'schwankt': schwankt, 'anders_als_1_thread': anders1}
        print('%s %-5s  Laeufe gleich: %-4s  schwankt: %s | gegen -threads 1: %s' %
              (karte, stufe, 'ja' if not schwankt else 'nein', ', '.join(schwankt) or '-', ', '.join(anders1) or 'gleich'))
        sys.stdout.flush()
        gut = gut and not schwankt and not anders1
        ordner0 = os.path.dirname(erste)
        eingabe = {ext: os.path.join(ordner0, karte + ext) for ext in ('.bsp', '.prt', '.srf') if os.path.exists(os.path.join(ordner0, karte + ext))}
    if a.json:
        with open(a.json, 'w', encoding='utf-8', newline='\n') as f:
            json.dump(ergebnis, f, indent=1, sort_keys=True)
    if not a.arbeit:
        shutil.rmtree(arbeit, ignore_errors=True)
    return 0 if gut else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
