#!/usr/bin/env python3
"""Stufe 8c/8g: q3map2 -light -bhedgpubake (die Lichtschleife auf der GPU) gegen die CPU.

Dieselbe Testkarte wie q3map2_gpu.py (Raum, Saeulen, Durchgang, drei Lichter, ein
Patch), dazu ein Spot und Bounce-Runden (Flaechenlichter): gebaut (-bsp -meta,
-vis), dann beleuchtet mit -samples 2 -filter -patchshadows -bounce 2. Jeder GPU-Lauf
mit -bhedgpubakecheck 1: die CPU rechnet jede Lightmap ein zweites Mal und vergleicht
alle Luxel (der Bericht zaehlt sie).

  0. Shader vorab uebersetzt  - (Stufe 8g) die exe nennt weder d3dcompiler_47.dll noch
                                 dxcompiler.dll noch D3DCompile
  1. CPU                       - die Vorlage
  2. GPU                       - Lightmaps auf der GPU gerechnet; mit double: 100 % der
                                 Luxel gleich und alle 18 Lumps Byte-gleich mit der CPU
  3. GPU noch einmal           - dasselbe Ergebnis wie 2 (wiederholtes Backen)
  4. GPU mit Geraeteverlust    - BHED_GPUBAKE_VERLUST=2 taeuscht nach zwei Dispatches
     mitten im Backen          einen Verlust vor: kein Absturz, Meldung "lost", der
                                 Rest auf der CPU, die Lumps gleich der CPU
  5. GPU ohne double           - BHED_GPUBAKE_OHNE_DOUBLE: float-Naeherungen, nur die
                                 Obergrenze (>= 25 % der Luxel gleich)
  6. kleine Bloecke            - BHED_GPUBAKE_BUDGET_MB=1: viele Bloecke, Lightmaps ueber dem
                                 Speicherbudget in Teilen ihrer Lichtliste (jeder Teil addiert auf
                                 den vorigen); Lumps gleich der CPU
  7. -samples 4                - die Unterabtastung mit Rekursion (drei Ebenen): CPU und GPU
                                 Byte-gleich
  7b. nicht endlich            - BHED_GPUBAKE_NAN: ein nicht endlicher Wert in einer Lightmap und
                                 einem Gitterpunkt (Treiberfehler vorgetaeuscht) - beide rechnet
                                 die CPU neu, Lumps gleich
  7c. Adapterwahl              - -bhedgpuadapter 0 und ein unbekannter Name (Meldung, die schnellste)
  8. Himmel mit Sonne          - ein Hof unter einem Himmels-Shader mit q3map_sunExt (eigener
                                 Spielordner im Test): CPU und GPU, mit double Byte-gleich.
                                 Rot vor der Behebung in 8c: der skyIndex eines Shaders
                                 entsteht erst in LightWorld und war fuer die GPU immer -1.

Ohne Direct-3D-11-GPU meldet q3map2 "no usable GPU" und rechnet auf der CPU - die
Lumps muessen dann gleich sein, die GPU-Pruefungen entfallen (ok mit Hinweis).

Aufruf: q3map2_gpubake.py <bhed_q3map2.exe>   (nur die Standardbibliothek)
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
from q3map2_gpu import karte as grundkarte  # noqa: E402

NL = '\n'


def karte():
    t = grundkarte()
    t += ('{' + NL + '"classname" "light"' + NL + '"origin" "-300 -300 200"' + NL + '"light" "500"' + NL +
          '"target" "spotziel"' + NL + '}' + NL)
    t += '{' + NL + '"classname" "info_null"' + NL + '"targetname" "spotziel"' + NL + '"origin" "-300 -300 0"' + NL + '}' + NL
    return t


HIMMEL_SHADER = ('textures/test/himmel' + NL + '{' + NL + '\tqer_editorimage textures/test/himmel' + NL +
                 '\tsurfaceparm sky' + NL + '\tsurfaceparm noimpact' + NL + '\tsurfaceparm nolightmap' + NL +
                 '\tq3map_sunExt 1 0.9 0.8 350 45 60 2 8' + NL + '\tq3map_skylight 60 3' + NL + '}' + NL)


def hofkarte():
    w = '{' + NL + '"classname" "worldspawn"' + NL
    w += kasten(-512, -512, -16, 512, 512, 0, 'base/floor')
    w += kasten(-528, -512, 0, -512, 512, 320, 'base/wall')
    w += kasten(512, -512, 0, 528, 512, 320, 'base/wall')
    w += kasten(-512, -528, 0, 512, -512, 320, 'base/wall')
    w += kasten(-512, 512, 0, 512, 528, 320, 'base/wall')
    w += kasten(-512, -512, 320, 512, 512, 336, 'test/himmel')
    for x, y in ((-300, -300), (200, -100), (0, 250)):
        w += kasten(x, y, 0, x + 64, y + 64, 200, 'base/wall')
    w += kasten(-200, 0, 120, 100, 80, 136, 'base/wall')
    w += '}' + NL
    w += '{' + NL + '"classname" "info_player_start"' + NL + '"origin" "-400 0 24"' + NL + '}' + NL
    return w


def lumps(bsp):
    d = open(bsp, 'rb').read()
    return [d[o:o + n] for o, n in (struct.unpack_from('<ii', d, 8 + 8 * i) for i in range(18))]


def lauf(exe, args, ordner, env=None):
    e = dict(os.environ)
    e.update(env or {})
    r = subprocess.run([exe] + args, capture_output=True, timeout=1800, cwd=ordner, env=e)
    return r.returncode, r.stdout.decode('utf-8', 'replace')


def bericht(aus):
    """Die zwei Zeilen des GPU-Backens: Kopf (Lightmaps GPU/CPU) und Abweichungsbericht."""
    k = re.search(r'BehavEd GPU bake: .*?, (\d+) rounds, (\d+) lightmaps on the GPU \(\d+ without light, (\d+) in parts\), (\d+) on the CPU \((\d+) alpha shadow/unsafe, '
                  r'(\d+) not finite, (\d+) too big\)', aus)
    m = re.search(r'BehavEd GPU bake report: (\d+) luxels in (\d+) lightmaps checked on the CPU, ([0-9.]+) % identical; (\d+) lightmaps with a '
                  r'difference \((\d+) in styles\), mean \|diff\| ([0-9.]+), max \|diff\| ([0-9.]+); light grid: (\d+) points on the GPU, (\d+) on the CPU', aus)
    if not k or not m:
        return None
    return {'runden': int(k.group(1)), 'gpu': int(k.group(2)), 'teile': int(k.group(3)), 'cpu': int(k.group(4)), 'alpha': int(k.group(5)),
            'nichtendlich': int(k.group(6)), 'zugross': int(k.group(7)), 'luxel': int(m.group(1)), 'lms': int(m.group(2)), 'gleich': float(m.group(3)), 'anders': int(m.group(4)),
            'stile': int(m.group(5)), 'mittel': float(m.group(6)), 'max': float(m.group(7)), 'gitter': int(m.group(8)), 'gittercpu': int(m.group(9))}


def main():
    if len(sys.argv) != 2 or not os.path.isfile(sys.argv[1]):
        print('Aufruf: q3map2_gpubake.py <bhed_q3map2.exe>')
        return 2
    exe = os.path.abspath(sys.argv[1])
    fehler = 0

    def melde(gut, text):
        nonlocal fehler
        print(('ok    ' if gut else 'FEHL  ') + text)
        fehler += 0 if gut else 1

    # 0. die Shader sind im Bau vorab uebersetzt - die exe laedt keinen Shader-Uebersetzer
    #    (weder d3dcompiler_47.dll noch dxcompiler.dll) und ruft D3DCompile nicht
    roh = open(exe, 'rb').read().lower()
    melde(b'd3dcompiler' not in roh and b'dxcompiler' not in roh and b'd3dcompile\0' not in roh,
          'Shader vorab uebersetzt: kein d3dcompiler_47.dll/dxcompiler.dll/D3DCompile in der exe')

    with tempfile.TemporaryDirectory(prefix='bhed_gpubake_') as t:
        vis = os.path.join(t, 'vis')
        os.makedirs(vis)
        m = os.path.join(vis, 'bake.map')
        with open(m, 'w', encoding='utf-8', newline=NL) as f:
            f.write(karte())
        for args in (['-bsp', '-meta'], ['-vis']):
            rc, aus = lauf(exe, ['-game', 'ja'] + args + [m], vis)
            if rc != 0:
                print('FEHL  %s: Rueckgabe %d' % (args[0], rc))
                return 1
        licht = ['-game', 'ja', '-light', '-fast', '-patchshadows', '-samples', '2', '-filter', '-bounce', '2']
        licht4 = ['-game', 'ja', '-light', '-fast', '-patchshadows', '-samples', '4', '-filter', '-bounce', '1']
        gpu = ['-bhedgpubake', '-bhedgpubakecheck', '1']
        ergebnis = {}
        for art, schalter, env in (('cpu', licht, None), ('gpu', licht + gpu, None), ('gpu2', licht + gpu, None),
                                   ('verlust', licht + gpu, {'BHED_GPUBAKE_VERLUST': '2'}),
                                   ('float', licht + gpu, {'BHED_GPUBAKE_OHNE_DOUBLE': '1'}),
                                   ('klein', licht + gpu, {'BHED_GPUBAKE_BUDGET_MB': '1'}),
                                   ('cpu4', licht4, None), ('gpu4', licht4 + gpu, None),
                                   ('nan', licht + gpu, {'BHED_GPUBAKE_NAN': '1'}),
                                   ('adapter0', licht + gpu + ['-bhedgpuadapter', '0'], None),
                                   ('adapterx', licht + gpu + ['-bhedgpuadapter', 'keine-solche-gpu'], None)):
            d = os.path.join(t, art)
            os.makedirs(d)
            for e in ('.map', '.bsp', '.srf'):
                shutil.copy(os.path.join(vis, 'bake' + e), os.path.join(d, 'bake' + e))
            rc, aus = lauf(exe, schalter + [os.path.join(d, 'bake.map')], d, env)
            if rc != 0:
                print('FEHL  -light %s: Rueckgabe %d' % (art, rc))
                return 1
            ergebnis[art] = (lumps(os.path.join(d, 'bake.bsp')), aus)
        cpu = ergebnis['cpu'][0]
        aus = ergebnis['gpu'][1]
        if 'BehavEd GPU bake' not in aus:
            melde(False, '-bhedgpubake: keine Meldung von q3map2 - der Schalter fehlt in dieser bhed_q3map2.exe')
            print('q3map2_gpubake: %d Fehler' % fehler)
            return 1
        if 'no usable GPU' in aus or 'BehavEd GPU bake report' not in aus:
            gleich = ergebnis['gpu'][0] == cpu
            melde(gleich, '-bhedgpubake ohne Direct-3D-11-GPU: die CPU hat gerechnet, Lumps gleich (%s)' %
                  next((x for x in aus.splitlines() if 'BehavEd GPU bake' in x), 'ohne Meldung'))
            print('q3map2_gpubake: %d Fehler' % fehler)
            return 1 if fehler else 0
        doppelt = ', double,' in aus
        name = re.search(r'BehavEd GPU bake: (.*?) \(feature level', aus)
        b = bericht(aus)
        melde(b is not None and b['gpu'] > 0 and b['lms'] >= b['gpu'] and b['luxel'] > 0 and (b['gleich'] == 100.0 or not doppelt),
              '-bhedgpubake auf %s (%s): %s Lightmaps auf der GPU, %s auf der CPU, %s Luxel in %s Lightmaps nachgerechnet, %.4f %% gleich, max %s' %
              (name.group(1) if name else '?', 'double' if doppelt else 'float', b and b['gpu'], b and b['cpu'], b and b['luxel'], b and b['lms'],
               b['gleich'] if b else 0, b and b['max']))
        melde(b is not None and b['gitter'] > 0 and b['gittercpu'] == 0,
              'Lichtgitter auf der GPU: %s Punkte, %s auf der CPU' % (b and b['gitter'], b and b['gittercpu']))
        g = ergebnis['gpu'][0]
        if doppelt:
            anders = [i for i in range(18) if g[i] != cpu[i]]
            melde(not anders and len(cpu[14]) > 0, 'GPU mit double: alle 18 Lumps Byte-gleich mit der CPU%s' % ('' if not anders else ' - anders: %s' % anders))
        melde(ergebnis['gpu2'][0] == g, 'zweites GPU-Backen: dasselbe Ergebnis wie das erste (alle Lumps)')
        av = ergebnis['verlust'][1]
        melde('graphics device was lost' in av and ergebnis['verlust'][0][14] == cpu[14] and ergebnis['verlust'][0][15] == cpu[15],
              'Geraeteverlust mitten im Backen (vorgetaeuscht nach 2 Dispatches): Meldung, kein Absturz, Lightmaps und Gitter gleich der CPU')
        bf = bericht(ergebnis['float'][1])
        melde(bf is not None and bf["gleich"] >= 25.0 and 'float only' in ergebnis['float'][1],
              'GPU ohne double (float-Naeherungen): %.4f %% der Luxel gleich (Obergrenze >= 25 %%), %s Lightmaps auf der GPU' %
              (bf['gleich'] if bf else 0, bf and bf['gpu']))
        bk = bericht(ergebnis['klein'][1])
        bl = re.search(r'BehavEd GPU bake: .*?(\d+) blocks', ergebnis['klein'][1])
        melde(bk is not None and bk['teile'] > 0 and bk['gpu'] > 0 and bl is not None and int(bl.group(1)) > 2 and
              (not doppelt or ergebnis['klein'][0] == cpu),
              'kleine Bloecke (1 MB): %s Bloecke, %s Lightmaps ueber dem Budget in Teilen der Lichtliste, %s zu gross (CPU), %s auf der GPU, Lumps gleich der CPU' %
              (bl.group(1) if bl else '?', bk and bk['teile'], bk and bk['zugross'], bk and bk['gpu']))
        bn = bericht(ergebnis['nan'][1])
        melde(bn is not None and bn['nichtendlich'] >= 1 and bn['gittercpu'] >= 1 and ergebnis['nan'][0][14] == cpu[14] and ergebnis['nan'][0][15] == cpu[15],
              'nicht endliche Werte (vorgetaeuscht: eine Lightmap, ein Gitterpunkt): %s Lightmaps und %s Gitterpunkte neu auf der CPU, Lightmaps und Gitter gleich der CPU' %
              (bn and bn['nichtendlich'], bn and bn['gittercpu']))
        a0 = ergebnis['adapter0'][1]
        ax = ergebnis['adapterx'][1]
        melde(re.search(r'BehavEd GPU bake: adapter 0: .* - chosen', a0) is not None and 'no adapter "keine-solche-gpu" - the fastest one' in ax and
              re.search(r'BehavEd GPU bake: adapter \d+: .* - chosen', ax) is not None and ergebnis['adapterx'][0][14] == cpu[14],
              'Adapterwahl: -bhedgpuadapter 0 nimmt Adapter 0, ein unbekannter Name nimmt die schnellste GPU (mit Meldung), Lightmaps gleich der CPU')
        b4 = bericht(ergebnis['gpu4'][1])
        if doppelt:
            anders = [i for i in range(18) if ergebnis['gpu4'][0][i] != ergebnis['cpu4'][0][i]]
            melde(not anders and b4 is not None and b4['gleich'] == 100.0 and b4['gpu'] > 0,
                  '-samples 4 (Unterabtastung in drei Ebenen): %s Lightmaps auf der GPU, %.4f %% gleich, alle 18 Lumps Byte-gleich%s' %
                  (b4 and b4['gpu'], b4['gleich'] if b4 else 0, '' if not anders else ' - anders: %s' % anders))
        # 8. Himmel mit Sonne (eigener Spielordner mit dem Himmels-Shader)
        spiel = os.path.join(t, 'spiel')
        os.makedirs(os.path.join(spiel, 'base', 'shaders'))
        with open(os.path.join(spiel, 'base', 'shaders', 'test.shader'), 'w', encoding='ascii', newline=NL) as f:
            f.write(HIMMEL_SHADER)
        hv = os.path.join(t, 'hof')
        os.makedirs(hv)
        hm = os.path.join(hv, 'hof.map')
        with open(hm, 'w', encoding='utf-8', newline=NL) as f:
            f.write(hofkarte())
        grund = ['-game', 'ja', '-fs_basepath', spiel]
        for args in (['-bsp', '-meta'], ['-vis']):
            rc, _ = lauf(exe, grund + args + [hm], hv)
            if rc != 0:
                print('FEHL  Hof %s: Rueckgabe %d' % (args[0], rc))
                return 1
        hof = {}
        for art, extra in (('hcpu', []), ('hgpu', gpu)):
            d = os.path.join(t, art)
            os.makedirs(d)
            for e in ('.map', '.bsp', '.srf'):
                shutil.copy(os.path.join(hv, 'hof' + e), os.path.join(d, 'hof' + e))
            rc, aus = lauf(exe, grund + ['-light', '-fast', '-samples', '2', '-filter', '-bounce', '1', '-threads', '4'] + extra + [os.path.join(d, 'hof.map')], d)
            if rc != 0:
                print('FEHL  Hof -light %s: Rueckgabe %d' % (art, rc))
                return 1
            hof[art] = (lumps(os.path.join(d, 'hof.bsp')), aus)
        hs = re.search(r'(\d+) sun/sky lights', hof['hcpu'][1])
        bh = bericht(hof['hgpu'][1])
        melde(hs is not None and int(hs.group(1)) > 0 and bh is not None and bh['gpu'] > 0 and (bh['gleich'] == 100.0 or not doppelt),
              'Hof unter Himmel: %s Sonnen-/Himmelslichter, %s Lightmaps auf der GPU, %.4f %% der Luxel gleich' %
              (hs.group(1) if hs else '?', bh and bh['gpu'], bh['gleich'] if bh else 0))
        if doppelt:
            anders = [i for i in range(18) if hof['hgpu'][0][i] != hof['hcpu'][0][i]]
            melde(not anders, 'Hof unter Himmel mit double: alle 18 Lumps Byte-gleich mit der CPU%s' % ('' if not anders else ' - anders: %s' % anders))
    print('q3map2_gpubake: %d Fehler' % fehler)
    return 1 if fehler else 0


if __name__ == '__main__':
    sys.exit(main())
