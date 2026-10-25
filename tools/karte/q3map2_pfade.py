# -*- coding: utf-8 -*-
"""Stufe 6 (NRC#130): bhed_q3map2 mit Pfaden ausserhalb von ASCII.

NRC-Meldung #130: "Pfad mit Umlauten oder anderen Nicht-ASCII-Zeichen: Karte
oeffnet nicht"; Garux: "Editor and all the tools are ASCII only". Deutsche
Nutzer heissen aber Joerg (C:\\Users\\Joerg\\...). Dieser Test baut eine kleine,
dichte Karte in einem Ordner mit Umlaut, Leerzeichen, chinesischen und
japanischen Zeichen: -bsp -meta, -vis, -light -fast; danach muessen .bsp, .prt
und .srf neben der Karte liegen und die BSP Licht haben.

  python tools/karte/q3map2_pfade.py <bhed_q3map2.exe>
"""
import os
import shutil
import struct
import subprocess
import sys
import tempfile


def kasten(x0, y0, z0, x1, y1, z1, tex='common/caulk'):
    t = ' %s 0 0 0 0.5 0.5 0 0 0\n' % tex
    p = lambda a, b, c: '( %g %g %g ) ' % (a, b, c)
    return ('{\n' + p(x1, y1, z1) + p(x1, y0, z1) + p(x0, y1, z1) + t + p(x1, y1, z1) + p(x0, y1, z1) + p(x1, y1, z0) + t +
            p(x1, y1, z1) + p(x1, y1, z0) + p(x1, y0, z1) + t + p(x0, y0, z0) + p(x1, y0, z0) + p(x0, y1, z0) + t +
            p(x0, y0, z0) + p(x0, y0, z1) + p(x1, y0, z0) + t + p(x0, y0, z0) + p(x0, y1, z0) + p(x0, y0, z1) + t + '}\n')


def raum():
    w = '{\n"classname" "worldspawn"\n'
    w += kasten(-256, -256, -16, 256, 256, 0, 'base/floor')     # Boden
    w += kasten(-256, -256, 256, 256, 256, 272, 'base/floor')   # Decke
    w += kasten(-272, -256, 0, -256, 256, 256, 'base/wall')
    w += kasten(256, -256, 0, 272, 256, 256, 'base/wall')
    w += kasten(-256, -272, 0, 256, -256, 256, 'base/wall')
    w += kasten(-256, 256, 0, 256, 272, 256, 'base/wall')
    w += kasten(-32, -32, 0, 32, 32, 64, 'base/wall')           # ein Block in der Mitte (zwei Blaetter, Portale)
    w += '}\n'
    w += '{\n"classname" "light"\n"origin" "0 0 192"\n"light" "300"\n}\n'
    w += '{\n"classname" "info_player_start"\n"origin" "-128 0 24"\n}\n'
    return w


def lump_laenge(bsp, nr):
    with open(bsp, 'rb') as f:
        kopf = f.read(8 + 18 * 8)
    if kopf[:4] != b'RBSP':
        return -1
    ofs, laenge = struct.unpack_from('<ii', kopf, 8 + nr * 8)
    return laenge


def main():
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
    exe = os.path.abspath(sys.argv[1])
    basis = tempfile.mkdtemp(prefix='bhed_s6_')
    fehler = 0
    try:
        ordner = os.path.join(basis, 'Jörg Müller', '测试 地图', 'マップ テスト')
        os.makedirs(ordner)
        karte = os.path.join(ordner, 'Räume ü 测试.map')
        with open(karte, 'w', encoding='utf-8', newline='\n') as f:
            f.write(raum())
        schritte = [['-bsp', '-meta'], ['-vis'], ['-light', '-fast']]
        for s in schritte:
            r = subprocess.run([exe, '-game', 'ja'] + s + [karte], capture_output=True, timeout=600)
            ausgabe = r.stdout.decode('utf-8', 'replace')
            if r.returncode != 0:
                fehler += 1
                print('FEHL  %s: Rueckgabe %d' % (' '.join(s), r.returncode))
                print('\n'.join(ausgabe.splitlines()[-12:]))
            else:
                print('ok    %s' % ' '.join(s))
            if s[0] == '-bsp':   # -vis ohne -saveprt loescht die .prt wieder
                da = os.path.exists(karte[:-4] + '.prt')
                print(('ok    ' if da else 'FEHL  ') + 'nach -bsp neben der Karte: ' + os.path.basename(karte[:-4] + '.prt'))
                fehler += 0 if da else 1
        stamm = karte[:-4]
        for endung in ('.bsp', '.srf'):
            da = os.path.exists(stamm + endung)
            print(('ok    ' if da else 'FEHL  ') + 'neben der Karte: ' + os.path.basename(stamm + endung))
            fehler += 0 if da else 1
        if os.path.exists(stamm + '.bsp'):
            licht = lump_laenge(stamm + '.bsp', 14)   # LUMP_LIGHTMAPS
            gut = licht > 0
            print(('ok    ' if gut else 'FEHL  ') + 'BSP mit Lightmaps (%d Bytes)' % licht)
            fehler += 0 if gut else 1
    finally:
        shutil.rmtree(basis, ignore_errors=True)
    print('q3map2_pfade: %d Fehlschlaege' % fehler)
    return 1 if fehler else 0


if __name__ == '__main__':
    sys.exit(main())
