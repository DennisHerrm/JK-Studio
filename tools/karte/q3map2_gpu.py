#!/usr/bin/env python3
"""Stufe 8b Punkt 9: q3map2 -light -bhedgpu (Schattentests auf der GPU) gibt
dieselben Lumps wie -light ohne GPU.

Eine Testkarte (Raum mit Saeulen, ein Durchgang, drei Lichter, ein Patch)
wird gebaut (-bsp -meta, -vis), dann zweimal beleuchtet - mit -samples 2,
-filter und -patchshadows wie das Build-Menue, -threads 1 (so gehen alle
Lightmaps den Weg ueber die GPU) - und alle 18 Lumps werden
Byte fuer Byte verglichen (der Kopf traegt nur den Zeitstempel). Mit GPU
muss die Ausgabe "BehavEd GPU shadow tests" und Strahlen ohne
Eingabe-Abweichung melden; ohne Direct-3D-11-GPU (Rechner ohne Grafik)
rechnet die CPU, die Lumps muessen trotzdem gleich sein.

Aufruf: q3map2_gpu.py <bhed_q3map2.exe>   (nur die Standardbibliothek)
"""
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from q3map2_pfade import kasten  # noqa: E402

NL = '\n'


def karte():
    w = '{' + NL + '"classname" "worldspawn"' + NL
    w += kasten(-512, -512, -16, 512, 512, 0, 'base/floor')
    w += kasten(-512, -512, 256, 512, 512, 272, 'base/floor')
    w += kasten(-528, -512, 0, -512, 512, 256, 'base/wall')
    w += kasten(512, -512, 0, 528, 512, 256, 'base/wall')
    w += kasten(-512, -528, 0, 512, -512, 256, 'base/wall')
    w += kasten(-512, 512, 0, 512, 528, 256, 'base/wall')
    w += kasten(-16, -512, 0, 16, -64, 256, 'base/wall')     # Trennwand mit Durchgang
    w += kasten(-16, 64, 0, 16, 512, 256, 'base/wall')
    for x, y in ((-300, -300), (-300, 200), (250, -250), (300, 300), (120, 40)):
        w += kasten(x, y, 0, x + 40, y + 40, 160, 'base/wall')   # Saeulen (Schatten)
    w += kasten(-200, -40, 96, -120, 40, 112, 'base/wall')      # eine Platte in der Luft
    w += ('{' + NL + 'patchDef2' + NL + '{' + NL + 'base/floor' + NL + '( 3 3 0 0 0 )' + NL + '(' + NL +
          '( ( 200 -100 0 0 0 ) ( 200 -100 64 0 0.5 ) ( 200 -100 128 0 1 ) )' + NL +
          '( ( 232 -68 0 0.5 0 ) ( 264 -36 64 0.5 0.5 ) ( 232 -68 128 0.5 1 ) )' + NL +
          '( ( 264 -100 0 1 0 ) ( 264 -100 64 1 0.5 ) ( 264 -100 128 1 1 ) )' + NL + ')' + NL + '}' + NL + '}' + NL)
    w += '}' + NL
    for o, l in (('-256 0 200', 400), ('256 128 120', 300), ('300 -300 64', 200)):
        w += '{' + NL + '"classname" "light"' + NL + '"origin" "%s"' % o + NL + '"light" "%d"' % l + NL + '}' + NL
    w += '{' + NL + '"classname" "info_player_start"' + NL + '"origin" "-400 0 24"' + NL + '}' + NL
    return w


def lumps(bsp):
    d = open(bsp, 'rb').read()
    return [d[o:o + n] for o, n in (struct.unpack_from('<ii', d, 8 + 8 * i) for i in range(18))]


def lauf(exe, args, ordner):
    r = subprocess.run([exe] + args, capture_output=True, timeout=1800, cwd=ordner)
    return r.returncode, r.stdout.decode('utf-8', 'replace')


def main():
    if len(sys.argv) != 2 or not os.path.isfile(sys.argv[1]):
        print('Aufruf: q3map2_gpu.py <bhed_q3map2.exe>')
        return 2
    exe = os.path.abspath(sys.argv[1])
    fehler = 0
    with tempfile.TemporaryDirectory(prefix='bhed_gpu_') as t:
        vis = os.path.join(t, 'vis')
        os.makedirs(vis)
        m = os.path.join(vis, 'gpu.map')
        with open(m, 'w', encoding='utf-8', newline=NL) as f:
            f.write(karte())
        for args in (['-bsp', '-meta'], ['-vis']):
            rc, aus = lauf(exe, ['-game', 'ja'] + args + [m], vis)
            if rc != 0:
                print('FEHL  %s: Rueckgabe %d' % (args[0], rc))
                return 1
        ergebnis = {}
        for art in ('cpu', 'gpu'):
            d = os.path.join(t, art)
            os.makedirs(d)
            for e in ('.map', '.bsp', '.srf'):
                shutil.copy(os.path.join(vis, 'gpu' + e), os.path.join(d, 'gpu' + e))
            extra = ['-bhedgpu'] if art == 'gpu' else []
            rc, aus = lauf(exe, ['-game', 'ja', '-light', '-fast', '-patchshadows', '-samples', '2', '-filter', '-threads', '1'] + extra + [os.path.join(d, 'gpu.map')], d)
            if rc != 0:
                print('FEHL  -light %s: Rueckgabe %d' % (art, rc))
                return 1
            ergebnis[art] = (lumps(os.path.join(d, 'gpu.bsp')), aus)
        a, b = ergebnis['cpu'][0], ergebnis['gpu'][0]
        anders = [i for i in range(18) if a[i] != b[i]]
        gut = not anders and len(a[14]) > 0
        print(('ok    ' if gut else 'FEHL  ') + '-light mit/ohne -bhedgpu: alle 18 Lumps Byte-gleich (Lightmaps %d Byte, Gitter %d Byte)%s' %
              (len(a[14]), len(a[15]), '' if gut else ', anders: %s' % anders))
        fehler += 0 if gut else 1
        aus = ergebnis['gpu'][1]
        if 'BehavEd GPU shadow tests:' in aus:
            z = re.search(r'BehavEd GPU: (\d+) rays .*?, (\d+) input mismatches', aus)
            strahlen = int(z.group(1)) if z else 0
            abw = int(z.group(2)) if z else -1
            gut = strahlen > 0 and abw == 0
            name = re.search(r'BehavEd GPU shadow tests: (.*?) \(', aus)
            print(('ok    ' if gut else 'FEHL  ') + '-bhedgpu auf %s: %d Strahlen auf der GPU, %d Eingabe-Abweichungen' %
                  (name.group(1) if name else '?', strahlen, abw))
            fehler += 0 if gut else 1
        else:
            print('ok    -bhedgpu: keine Direct-3D-11-GPU - die CPU hat gerechnet (%s)' %
                  next((x for x in aus.splitlines() if 'BehavEd GPU' in x), 'ohne Meldung'))
    print('q3map2_gpu: %d Fehler' % fehler)
    return 1 if fehler else 0


if __name__ == '__main__':
    sys.exit(main())
