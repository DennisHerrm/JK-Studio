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
  gt113    -patchmeta behaelt Patches mit surfaceparm playerclip (Kollision)
  gl143    -meta -patchmeta: die Patches werden Dreiecke, die Ausgabe ist nicht leer
  gl108    q3map_lightmapBrightness 0.25 macht die Lightmap einer Flaeche dunkler
  gl128    q3map_lightSubdivide teilt ein Flaechenlicht, der Backsplash bleibt
  relicht  (vis2) ein zweites -light auf einer schon beleuchteten quake3-.bsp mit Lichtstilen
           stuerzte ab (auch NRCs q3map2.exe): der Stil-Trick schreibt externe Lightmaps und
           stellt den Flaechen Flare-Flaechen fuer "nomipmaps" voran - die Nummern passten nicht
           mehr zur .srf. Das zweite -light muss dasselbe ergeben wie das erste auf der Ausgabe
           von -vis (alle 17 Lumps, Shader-Datei und externe Lightmaps gleich), mit und ohne
           -randomsamples (dann Bounce-Runden)
  kemd1    (rc609 KEMD) misc_model mit spawnflags 4 (forceMeta) und einer ebenen Flaeche, -bsp ohne -meta:
           die Flaeche bleibt ESurfaceType::ForcedMeta mit Lightmap-Achse ( 0 0 0 ), -light stuerzte in
           MapSingleLuxel ab (lm->vecs nullptr; NRCs q3map2.exe ebenso). Rueckgabe 0, Lightmaps fuer die
           Welt, die Modellflaeche per Ecke beleuchtet (LIGHTMAP_BY_VERTEX)
"""
import os
import re
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


PATCH = ('{' + NL + 'patchDef2' + NL + '{' + NL + '%s' + NL + '( 3 3 0 0 0 )' + NL + '(' + NL +
         '( ( -96 -96 0 0 0 ) ( -96 0 64 0 0.5 ) ( -96 96 0 0 1 ) )' + NL +
         '( ( 0 -96 64 0.5 0 ) ( 0 0 128 0.5 0.5 ) ( 0 96 64 0.5 1 ) )' + NL +
         '( ( 96 -96 0 1 0 ) ( 96 0 64 1 0.5 ) ( 96 96 0 1 1 ) )' + NL + ')' + NL + '}' + NL + '}' + NL)


def raum_mit(extra_welt='', licht=True, boden='base/floor'):
    w = '{' + NL + '"classname" "worldspawn"' + NL
    w += kasten(-256, -256, -16, 256, 256, 0, boden) + kasten(-256, -256, 256, 256, 256, 272, 'base/floor')
    w += kasten(-272, -256, 0, -256, 256, 256, 'base/wall') + kasten(256, -256, 0, 272, 256, 256, 'base/wall')
    w += kasten(-256, -272, 0, 256, -256, 256, 'base/wall') + kasten(-256, 256, 0, 256, 272, 256, 'base/wall')
    w += extra_welt + '}' + NL
    if licht:
        w += '{' + NL + '"classname" "light"' + NL + '"origin" "0 0 192"' + NL + '"light" "300"' + NL + '}' + NL
    w += '{' + NL + '"classname" "info_player_start"' + NL + '"origin" "-128 0 24"' + NL + '}' + NL
    return w


def shader_datei(ordner, text):
    os.makedirs(os.path.join(ordner, 'base', 'shaders'), exist_ok=True)
    schreiben(os.path.join(ordner, 'base', 'shaders', 's6.shader'), text)


def flaechen_je_shader(l):
    shader = [l[1][i:i + 64].split(NUL)[0].decode('latin-1') for i in range(0, len(l[1]), 72)]
    out = []
    for i in range(0, len(l[13]), 148):
        sh, _, typ = struct.unpack_from('<3i', l[13], i)
        out.append((shader[sh] if 0 <= sh < len(shader) else '?', typ))
    return out


def gt113(exe, ordner):
    shader_datei(ordner, 'textures/s6/clip' + NL + '{' + NL + '\tsurfaceparm nodraw' + NL + '\tsurfaceparm nolightmap' + NL + '\tsurfaceparm nonsolid' + NL +
                 '\tsurfaceparm trans' + NL + '\tsurfaceparm nomarks' + NL + '\tsurfaceparm playerclip' + NL + '}' + NL)
    karte = os.path.join(ordner, 'g113.map')
    schreiben(karte, raum_mit(PATCH % 's6/clip'))
    rc, _ = lauf(exe, ['-game', 'ja', '-fs_basepath', ordner, '-bsp', '-meta', '-patchmeta', karte], ordner)
    l = lumps(os.path.join(ordner, 'g113.bsp'))
    f = flaechen_je_shader(l) if l else []
    patches = [x for x in f if x == ('textures/s6/clip', 2)]
    gut = rc == 0 and len(patches) == 1
    print(('ok    ' if gut else 'FEHL  ') + 'gt113: -patchmeta mit playerclip-Patch: %d Kollisions-Patch(es) in der BSP (soll 1)' % len(patches))
    return 0 if gut else 1


def gl143(exe, ordner):
    karte = os.path.join(ordner, 'g143.map')
    schreiben(karte, raum_mit(PATCH % 'base/patch'))
    rc, _ = lauf(exe, ['-game', 'ja', '-fs_basepath', ordner, '-bsp', '-meta', '-patchmeta', '-subdivisions', '8', karte], ordner)
    l = lumps(os.path.join(ordner, 'g143.bsp'))
    f = flaechen_je_shader(l) if l else []
    dreiecke = [x for x in f if x[0] == 'textures/base/patch' and x[1] != 2]   # Meta-Flaechen (planar oder Dreiecks-Suppe)
    kollision = [x for x in f if x == ('textures/base/patch', 2)]
    gut = rc == 0 and len(dreiecke) >= 1 and len(kollision) == 1 and len(l[11]) > 0
    print(('ok    ' if gut else 'FEHL  ') + 'gl143: -meta -patchmeta: %d Dreiecks-Flaeche(n) und %d Kollisions-Patch aus dem Patch, %d Indizes' %
          (len(dreiecke), len(kollision), len(l[11]) // 4 if l else 0))
    return 0 if gut else 1


def mittel_je_shader(l, name):
    """Mittlerer Lightmap-Wert der Flaechen eines Shaders (ueber die Lightmap-ST der Ecken)."""
    shader = [l[1][i:i + 64].split(NUL)[0].decode('latin-1') for i in range(0, len(l[1]), 72)]
    werte = []
    for i in range(0, len(l[13]), 148):
        sh, _, typ, erste, anzahl = struct.unpack_from('<5i', l[13], i)
        num = struct.unpack_from('<i', l[13], i + 36)[0]
        if not (0 <= sh < len(shader)) or shader[sh] != name or num < 0:
            continue
        uv = [struct.unpack_from('<2f', l[10], (erste + k) * 80 + 20) for k in range(anzahl)]
        us = [t[0] * 128 for t in uv]
        vs = [t[1] * 128 for t in uv]
        for y in range(int(min(vs)) + 1, int(max(vs)) - 1):
            for x in range(int(min(us)) + 1, int(max(us)) - 1):
                o = num * 128 * 128 * 3 + (y * 128 + x) * 3
                werte.append(sum(l[14][o:o + 3]) / 3.0)
    return sum(werte) / len(werte) if werte else 0.0


def gl108(exe, ordner):
    shader_datei(ordner, 'textures/s6/dunkel' + NL + '{' + NL + '\tq3map_lightmapBrightness 0.25' + NL + '}' + NL)
    werte = {}
    for name, boden in (('normal', 's6/hell'), ('dunkel', 's6/dunkel')):
        karte = os.path.join(ordner, 'g108%s.map' % name)
        schreiben(karte, raum_mit(boden=boden))
        rc1, _ = lauf(exe, ['-game', 'ja', '-fs_basepath', ordner, '-bsp', '-meta', karte], ordner)
        rc2, _ = lauf(exe, ['-game', 'ja', '-fs_basepath', ordner, '-light', '-fast', karte], ordner)
        l = lumps(os.path.join(ordner, 'g108%s.bsp' % name))
        werte[name] = mittel_je_shader(l, 'textures/' + boden) if l and not rc1 and not rc2 else 0.0
    q = werte['dunkel'] / werte['normal'] if werte['normal'] else 0.0
    gut = werte['normal'] > 5 and 0.15 < q < 0.4
    print(('ok    ' if gut else 'FEHL  ') + 'gl108: q3map_lightmapBrightness 0.25: Boden %.1f gegen %.1f, Verhaeltnis %.2f (soll ~0.25)' % (werte['dunkel'], werte['normal'], q))
    return 0 if gut else 1


def gl128(exe, ordner):
    zahlen = {}
    for name, unterteilung in (('ohne', ''), ('mit', '\tq3map_lightSubdivide 32' + NL)):
        shader_datei(ordner, 'textures/s6/lampe' + NL + '{' + NL + '\tq3map_surfacelight 2000' + NL + unterteilung + '}' + NL)
        karte = os.path.join(ordner, 'g128%s.map' % name)
        schreiben(karte, raum_mit(kasten(-64, -64, 240, 64, 64, 256, 's6/lampe'), licht=False))
        rc1, _ = lauf(exe, ['-game', 'ja', '-fs_basepath', ordner, '-bsp', '-meta', karte], ordner)
        rc2, aus = lauf(exe, ['-game', 'ja', '-fs_basepath', ordner, '-light', '-fast', '-v', karte], ordner)
        m = [z for z in aus.split('\n') if 'diffuse (area) lights' in z]
        zahlen[name] = int(m[-1].split()[0]) if m and not rc1 and not rc2 else -1
    # q3map_backsplash (Vorgabe 0.05) legt zu jedem Flaechenlicht ein zweites an: die Zahl ist gerade
    gut = zahlen['ohne'] > 0 and zahlen['mit'] > zahlen['ohne'] and zahlen['mit'] % 2 == 0 and zahlen['ohne'] % 2 == 0
    print(('ok    ' if gut else 'FEHL  ') + 'gl128: Flaechenlichter ohne q3map_lightSubdivide %d, mit 32 %d (mehr, mit Backsplash je Licht)' % (zahlen['ohne'], zahlen['mit']))
    return 0 if gut else 1


def ibsp_lumps(bsp):
    if not os.path.exists(bsp):
        return None
    d = open(bsp, 'rb').read()
    if d[:4] != b'IBSP':
        return None
    return [d[o:o + n] for o, n in (struct.unpack_from('<ii', d, 8 + i * 8) for i in range(17))]


def relicht(exe, ordner):
    # raum.map mit zwei Lichtern mit Stil (wie ctest q3map2_determinismus mit raum_stil.map, game quake3)
    text = open(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'tests', 'karte', 'q3map2', 'raum.map'), encoding='utf-8').read()
    text = text.replace('"light" "400"' + NL, '"light" "400"' + NL + '"style" "1"' + NL, 1).replace('"light" "300"' + NL, '"light" "300"' + NL + '"style" "5"' + NL, 1)
    fehler = 0
    for art, licht in (('fast', ['-light', '-fast', '-patchshadows', '-samples', '2', '-filter', '-bounce', '2']),
                       ('zufall', ['-light', '-fastbounce', '-patchshadows', '-samples', '4', '-randomsamples', '-bounce', '2'])):
        erg = {}
        for name, stufen in (('neu', [['-meta'], ['-vis'], licht]), ('zweimal', [['-meta'], ['-vis'], licht, licht])):
            unter = os.path.join(ordner, art + '_' + name)
            os.makedirs(os.path.join(unter, 'maps'))
            schreiben(os.path.join(unter, 'maps', 'r.map'), text)
            rcs = []
            for st in stufen:
                rc, _ = lauf(exe, ['-game', 'quake3', '-fs_basepath', ordner, '-threads', '4'] + st + ['maps/r.map'], unter)
                rcs.append(rc & 0xffffffff)
                if rc != 0:
                    break
            extern = sorted(os.listdir(os.path.join(unter, 'maps', 'r'))) if os.path.isdir(os.path.join(unter, 'maps', 'r')) else []
            dateien = {e: open(os.path.join(unter, 'maps', 'r', e), 'rb').read() for e in extern}
            sh = os.path.join(unter, 'scripts', 'q3map2_r.shader')
            erg[name] = (rcs, ibsp_lumps(os.path.join(unter, 'maps', 'r.bsp')), dateien, open(sh, 'rb').read() if os.path.exists(sh) else None)
        rn, ln, dn, sn = erg['neu']
        rz, lz, dz, sz = erg['zweimal']
        # die Entities ohne _q3map2_cmdline (der Verlauf der Schritte)
        for l in (ln, lz):
            if l:
                l[0] = re.sub(rb'"_q3map2_cmdline" "[^"]*"', b'', l[0])
        anders = [i for i in range(17) if ln and lz and ln[i] != lz[i]]
        gut = all(r == 0 for r in rn + rz) and len(rz) == 4 and ln is not None and lz is not None and not anders and dn == dz and len(dn) > 0 and sn == sz and sn
        fehler += 0 if gut else 1
        print(('ok    ' if gut else 'FEHL  ') + 'relicht (%s): zweites -light auf der beleuchteten quake3-.bsp mit Stilen - Rueckgaben %s / %s, '
              'Lumps anders %s, externe Lightmaps %d %s, Shader-Datei %s' %
              (art, rn, ['0x%x' % r if r > 255 else r for r in rz], anders, len(dn), 'gleich' if dn == dz else 'anders',
               'gleich' if sn == sz and sn else 'anders/fehlt'))
    return fehler


def md3_viereck(pfad, shader):
    """Ein MD3 mit einem Frame und einer Flaeche: waagrechtes Viereck in z = 8 (Normale lat 0 lng 0 = genau +z,
    damit ClassifySurfaces es eben findet)."""
    ecken = [(-16, -16, 8), (16, -16, 8), (16, 16, 8), (-16, 16, 8)]
    dreiecke = [(0, 1, 2), (0, 2, 3)]
    st = [(0, 1), (1, 1), (1, 0), (0, 0)]
    normale = 0
    flaeche_kopf = 4 + 64 + 4 * 10
    o_dreiecke = flaeche_kopf
    o_shader = o_dreiecke + 12 * len(dreiecke)
    o_st = o_shader + 68
    o_ecken = o_st + 8 * len(ecken)
    o_ende = o_ecken + 8 * len(ecken)
    f = b'IDP3' + b'kiste'.ljust(64, NUL) + struct.pack('<10i', 0, 1, 1, len(ecken), len(dreiecke), o_dreiecke, o_shader, o_st, o_ecken, o_ende)
    f += b''.join(struct.pack('<3i', *d) for d in dreiecke)
    f += shader.encode('latin-1').ljust(64, NUL) + struct.pack('<i', 0)
    f += b''.join(struct.pack('<2f', *s) for s in st)
    f += b''.join(struct.pack('<4h', x * 64, y * 64, z * 64, normale) for x, y, z in ecken)
    kopf = 4 + 4 + 64 + 4 * 9
    o_frames = kopf
    o_flaechen = o_frames + 56
    rahmen = struct.pack('<10f', -16, -16, 8, 16, 16, 8, 0, 0, 0, 24) + b'frame'.ljust(16, NUL)
    daten = b'IDP3' + struct.pack('<i', 15) + b'kiste'.ljust(64, NUL) + struct.pack('<9i', 0, 1, 0, 1, 0, o_frames, o_frames, o_flaechen, o_flaechen + len(f))
    os.makedirs(os.path.dirname(pfad), exist_ok=True)
    with open(pfad, 'wb') as d:
        d.write(daten + rahmen + f)


def kemd1(exe, ordner):
    md3_viereck(os.path.join(ordner, 'base', 'models', 's6', 'kiste.md3'), 'models/s6/kiste')
    karte = os.path.join(ordner, 'kemd1.map')
    schreiben(karte, raum_mit() + '{' + NL + '"classname" "misc_model"' + NL + '"model" "models/s6/kiste.md3"' + NL +
              '"origin" "64 0 0"' + NL + '"spawnflags" "4"' + NL + '}' + NL)
    rc1, _ = lauf(exe, ['-game', 'ja', '-fs_basepath', ordner, '-bsp', karte], ordner)
    rc2, aus = lauf(exe, ['-game', 'ja', '-fs_basepath', ordner, '-light', '-threads', '1', karte], ordner)
    l = lumps(os.path.join(ordner, 'kemd1.bsp'))
    modell = []
    welt = 0
    if l and not rc1 and not rc2:
        shader = [l[1][i:i + 64].split(NUL)[0].decode('latin-1') for i in range(0, len(l[1]), 72)]
        for i in range(0, len(l[13]), 148):
            sh, _, typ = struct.unpack_from('<3i', l[13], i)
            lm0 = struct.unpack_from('<i', l[13], i + 36)[0]   # lightmapNum[0] (nach shader, fog, typ, 4 Zahlen, styles)
            if shader[sh] == 'models/s6/kiste':
                modell.append(lm0)
            elif lm0 >= 0:
                welt += 1
    gut = rc1 == 0 and rc2 == 0 and 'Chose a 0 valued axis' in aus and modell == [-3] and welt > 0 and len(l[14]) > 0
    print(('ok    ' if gut else 'FEHL  ') + 'kemd1: forceMeta-misc_model ohne -meta: -bsp %s, -light %s, Modellflaeche lightmapNum %s (soll [-3]), '
          'Weltflaechen mit Lightmap %d, Lightmap-Lump %d Bytes' % (rc1, '0x%x' % (rc2 & 0xffffffff), modell, welt, len(l[14]) if l else -1))
    return 0 if gut else 1


FAELLE = {'nrc270': nrc270, 'nrc43': nrc43, 'nrc72': nrc72, 'nrc101': nrc101, 'nrc102': nrc102, 'gt113': gt113, 'gl143': gl143, 'gl108': gl108, 'gl128': gl128,
          'relicht': relicht, 'kemd1': kemd1}


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
