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


FAELLE = {'nrc270': nrc270, 'nrc43': nrc43, 'nrc72': nrc72}


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
