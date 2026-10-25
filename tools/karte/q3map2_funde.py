# -*- coding: utf-8 -*-
"""Stufe 6: bekannte Fehler von q3map2 (NRC, Xonotic-NetRadiant, GtkRadiant)
an bhed_q3map2 nachgestellt - je Fall eine kleine Karte, die den Fehler
ausloest, und die Pruefung, dass er bei uns nicht (mehr) auftritt.

  python tools/karte/q3map2_funde.py <bhed_q3map2.exe> [fall ...]

Faelle:
  nrc270   .bsp -> .map mit -game ja/jk2: Ursprungs-Pinsel bekommen
           system/origin (q3map2 setzt "origin" bei JA/JK2 nur dafuer)
  nrc43    q3map_origin (q3map_<surfaceparm>) wirkt wie surfaceparm origin
  nrc72    mit den Shadern von Jedi Academy ist common/caulk nodraw
  nrc101   -dirty dunkelt Flaechen neben dem Himmel ab (divVerent aa00101d):
           Raum mit Himmel als Decke, -dirtdebug; die Wand oben muss so hell
           sein wie in der Mitte (Gegenprobe: mit fester Decke dunkler)
  nrc102   Kartenname mit Punkt (test.q3.map, foo_0.1.map, GL#154): -bsp,
           -vis, -light mit vollem Pfad (wie der Editor) und ohne Endung;
           .bsp/.prt/.srf heissen wie die Karte, -light findet die .srf
"""
import os
import shutil
import struct
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from q3map2_pfade import kasten, raum  # noqa: E402

JA_PFAD = os.environ.get('BHED_JA_PFAD', r'C:\Program Files (x86)\Steam\steamapps\common\Jedi Academy\GameData original')
NL = '\n'
NUL = b'\x00'


def lauf(exe, args, ordner):
    r = subprocess.run([exe] + args, capture_output=True, timeout=900, cwd=ordner)
    return r.returncode, r.stdout.decode('utf-8', 'replace')


def schreiben(pfad, text):
    with open(pfad, 'w', encoding='utf-8', newline=NL) as f:
        f.write(text)


def lumps(bsp):
    if not os.path.exists(bsp):
        return None
    d = open(bsp, 'rb').read()
    if d[:4] != b'RBSP':
        return None
    out = []
    for i in range(18):
        ofs, laenge = struct.unpack_from('<ii', d, 8 + i * 8)
        out.append(d[ofs:ofs + laenge])
    return out


def mit_func_static(urs_shader):
    w = raum().rstrip()
    w += NL + '{' + NL + '"classname" "func_static"' + NL + kasten(64, 64, 64, 128, 128, 128, 'base/wall') + \
        kasten(88, 88, 88, 104, 104, 104, urs_shader) + '}' + NL
    return w


def nrc270(exe, ordner):
    fehler = 0
    karte = os.path.join(ordner, 'urs.map')
    schreiben(karte, mit_func_static('system/origin'))
    rc, aus = lauf(exe, ['-game', 'ja', '-bsp', '-meta', karte], ordner)
    if rc != 0:
        print('FEHL  nrc270: -bsp: Rueckgabe %d' % rc)
        return 1
    for spiel in ('ja', 'jk2'):   # der Editor ruft -convert mit -game ja/jk2 (gui/app_karte_bsp.cpp)
        ziel = os.path.join(ordner, 'urs_converted.map')
        if os.path.exists(ziel):
            os.remove(ziel)
        rc, aus = lauf(exe, ['-game', spiel, '-convert', '-format', 'map', os.path.join(ordner, 'urs.bsp')], ordner)
        text = open(ziel, encoding='utf-8', errors='replace').read() if os.path.exists(ziel) else ''
        gut = rc == 0 and 'system/origin' in text and 'common/origin' not in text
        print(('ok    ' if gut else 'FEHL  ') + 'nrc270: -game %s -convert: Ursprungs-Pinsel mit system/origin' % spiel)
        fehler += 0 if gut else 1
    return fehler


def nrc43(exe, ordner):
    os.makedirs(os.path.join(ordner, 'base', 'shaders'))   # JA: shaders/*.shader (games.cpp shaderPath)
    schreiben(os.path.join(ordner, 'base', 'shaders', 's6.shader'), 'textures/s6/urs' + NL + '{' + NL + '\tq3map_origin' + NL + '}' + NL)
    karte = os.path.join(ordner, 'u43.map')
    schreiben(karte, mit_func_static('s6/urs'))
    rc, aus = lauf(exe, ['-game', 'ja', '-fs_basepath', ordner, '-bsp', '-meta', karte], ordner)
    l = lumps(os.path.join(ordner, 'u43.bsp'))
    ents = l[0].decode('latin-1') if l else ''
    gut = rc == 0 and '"classname" "func_static"' in ents and '"origin" "96.000000 96.000000 96.000000"' in ents
    print(('ok    ' if gut else 'FEHL  ') + 'nrc43: q3map_origin wirkt wie surfaceparm origin (func_static bekommt "origin" 96 96 96)')
    return 0 if gut else 1


def nrc72(exe, ordner):
    if not os.path.isdir(JA_PFAD):
        print('ok    nrc72: uebersprungen (kein Jedi-Academy-Ordner: %s)' % JA_PFAD)
        return 0
    karte = os.path.join(ordner, 'u72.map')
    schreiben(karte, raum().replace('base/wall', 'common/caulk', 2))
    rc, aus = lauf(exe, ['-game', 'ja', '-fs_basepath', JA_PFAD, '-fs_homepath', ordner, '-bsp', '-meta', karte], ordner)
    l = lumps(os.path.join(ordner, 'u72.bsp'))
    if not l:
        print('FEHL  nrc72: -bsp: Rueckgabe %d' % rc)
        return 1
    shader = [l[1][i:i + 64].split(NUL)[0].decode('latin-1') for i in range(0, len(l[1]), 72)]
    flaechen = [struct.unpack_from('<i', l[13], i)[0] for i in range(0, len(l[13]), 148)]
    caulk = [n for n in flaechen if 0 <= n < len(shader) and shader[n] == 'textures/common/caulk']
    gut = len(flaechen) > 0 and not caulk
    print(('ok    ' if gut else 'FEHL  ') + 'nrc72: common/caulk mit den JA-Shadern nodraw: %d Flaechen, davon caulk %d' % (len(flaechen), len(caulk)))
    return 0 if gut else 1


def wand_baender(l, wand_shader):
    """Mittlere Dirt-Werte (-dirtdebug: Lightmap = Dirt) der Wandflaechen oben (z > 216) und
    in der Mitte (96 < z < 160). Die Groessenfelder der Flaechen bleiben bei RBSP nach -light 0;
    darum ueber die Lightmap-Koordinaten der Ecken: affine Abbildung (u, v) -> z."""
    shader = [l[1][i:i + 64].split(NUL)[0].decode('latin-1') for i in range(0, len(l[1]), 72)]
    lm = l[14]
    oben, mitte = [], []
    for i in range(0, len(l[13]), 148):
        sh, _, typ, erste, anzahl = struct.unpack_from('<5i', l[13], i)
        num = struct.unpack_from('<i', l[13], i + 36)[0]
        if not (0 <= sh < len(shader)) or shader[sh] != wand_shader or num < 0 or anzahl < 3:
            continue
        ecken = [(struct.unpack_from('<3f', l[10], (erste + k) * 80), struct.unpack_from('<2f', l[10], (erste + k) * 80 + 20)) for k in range(anzahl)]
        (p0, t0), (p1, t1), (p2, t2) = ecken[0], ecken[1], ecken[2]
        # z = a*u + b*v + c aus drei Ecken (Cramersche Regel)
        det = (t1[0] - t0[0]) * (t2[1] - t0[1]) - (t2[0] - t0[0]) * (t1[1] - t0[1])
        if abs(det) < 1e-12:
            continue
        ka = ((p1[2] - p0[2]) * (t2[1] - t0[1]) - (p2[2] - p0[2]) * (t1[1] - t0[1])) / det
        kb = ((t1[0] - t0[0]) * (p2[2] - p0[2]) - (t2[0] - t0[0]) * (p1[2] - p0[2])) / det
        us = [t[0] * 128 for _, t in ecken]
        vs = [t[1] * 128 for _, t in ecken]
        for y in range(int(min(vs)) + 1, int(max(vs)) - 1):
            for x in range(int(min(us)) + 1, int(max(us)) - 1):
                u, v = (x + 0.5) / 128.0, (y + 0.5) / 128.0
                z = p0[2] + ka * (u - t0[0]) + kb * (v - t0[1])
                o = num * 128 * 128 * 3 + (y * 128 + x) * 3
                wert = sum(lm[o:o + 3]) / 3.0
                if z > 216:
                    oben.append(wert)
                elif 96 < z < 160:
                    mitte.append(wert)
    m = lambda v: sum(v) / len(v) if v else 0.0
    return m(oben), m(mitte), len(oben), len(mitte)


def nrc101(exe, ordner):
    os.makedirs(os.path.join(ordner, 'base', 'shaders'))
    schreiben(os.path.join(ordner, 'base', 'shaders', 's6.shader'),
              'textures/s6/himmel' + NL + '{' + NL + '\tsurfaceparm sky' + NL + '\tsurfaceparm noimpact' + NL + '\tsurfaceparm nolightmap' + NL + '}' + NL)
    ergebnis = {}
    for name, decke in (('himmel', 's6/himmel'), ('fest', 's6/decke')):
        w = '{' + NL + '"classname" "worldspawn"' + NL + '"_ambient" "120"' + NL
        w += kasten(-256, -256, -16, 256, 256, 0, 's6/boden')
        w += kasten(-256, -256, 256, 256, 256, 272, decke)
        w += kasten(-272, -256, 0, -256, 256, 256, 's6/wand')
        w += kasten(256, -256, 0, 272, 256, 256, 's6/wand')
        w += kasten(-256, -272, 0, 256, -256, 256, 's6/wand')
        w += kasten(-256, 256, 0, 256, 272, 256, 's6/wand')
        w += '}' + NL + '{' + NL + '"classname" "info_player_start"' + NL + '"origin" "0 0 24"' + NL + '}' + NL
        karte = os.path.join(ordner, 'u101%s.map' % name)
        schreiben(karte, w)
        rc1, _ = lauf(exe, ['-game', 'ja', '-fs_basepath', ordner, '-bsp', '-meta', karte], ordner)
        rc2, _ = lauf(exe, ['-game', 'ja', '-fs_basepath', ordner, '-light', '-fast', '-dirty', '-dirtdebug', '-dirtdepth', '64', karte], ordner)
        l = lumps(os.path.join(ordner, 'u101%s.bsp' % name))
        if not l or rc1 or rc2:
            print('FEHL  nrc101: %s: -bsp %d, -light %d' % (name, rc1, rc2))
            return 1
        ergebnis[name] = wand_baender(l, 'textures/s6/wand')
    ho, hm, hn, _ = ergebnis['himmel']
    fo, fm, fn, _ = ergebnis['fest']
    q_himmel = ho / hm if hm else 0.0
    q_fest = fo / fm if fm else 0.0
    gut = hn > 0 and fn > 0 and q_himmel > 0.985 and q_fest < 0.96
    print(('ok    ' if gut else 'FEHL  ') + 'nrc101: Dirt oben/Mitte an der Wand: unter dem Himmel %.3f (soll > 0.985), unter fester Decke %.3f (Gegenprobe, soll < 0.96)' % (q_himmel, q_fest))
    return 0 if gut else 1


def nrc102(exe, ordner):
    fehler = 0
    for name in ('u102.q3', 'foo_0.1'):
        for mit_endung in (True, False):
            unter = os.path.join(ordner, name.replace('.', '_') + ('_m' if mit_endung else '_o'))
            os.makedirs(unter)
            karte = os.path.join(unter, name + '.map')
            schreiben(karte, raum())
            arg = karte if mit_endung else karte[:-4]
            rcs = []
            for st in (['-bsp', '-meta'], ['-vis'], ['-light', '-fast']):
                rc, aus = lauf(exe, ['-game', 'ja', '-fs_basepath', ordner] + st + [arg], unter)
                rcs.append(rc)
            dateien = sorted(os.listdir(unter))
            soll = [name + e for e in ('.bsp', '.map', '.srf')]   # -vis loescht die .prt (ohne -saveprt)
            l = lumps(os.path.join(unter, name + '.bsp'))
            licht = bool(l) and len(l[14]) > 0
            gut = rcs == [0, 0, 0] and all(d in dateien for d in soll) and licht and not any(d.endswith('.bsp') and d != name + '.bsp' for d in dateien)
            fehler += 0 if gut else 1
            print(('ok    ' if gut else 'FEHL  ') + 'nrc102: "%s" %s Endung: Rueckgaben %s, Dateien %s, Lightmaps %s' %
                  (name + '.map', 'mit' if mit_endung else 'ohne', rcs, ' '.join(dateien), 'ja' if licht else 'nein'))
    return fehler


FAELLE = {'nrc270': nrc270, 'nrc43': nrc43, 'nrc72': nrc72, 'nrc101': nrc101, 'nrc102': nrc102}


def main():
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
    exe = os.path.abspath(sys.argv[1])
    faelle = sys.argv[2:] or list(FAELLE)
    fehler = 0
    for f in faelle:
        ordner = tempfile.mkdtemp(prefix='bhed_s6_' + f + '_')
        try:
            fehler += FAELLE[f](exe, ordner)
        finally:
            shutil.rmtree(ordner, ignore_errors=True)
    print('q3map2_funde: %d Fehlschlaege' % fehler)
    return 1 if fehler else 0


if __name__ == '__main__':
    sys.exit(main())
