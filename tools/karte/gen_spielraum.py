# -*- coding: utf-8 -*-
"""Stufe 8b Punkt 3 (Im Editor spielen): die Testkarte fuer den ctest karte_spielen.

Ein Raum mit Boden, einer Wand im Osten, einer Stufe von 16 (steigbar, STEPSIZE 18)
und einer von 24 (nicht steigbar), einer Rampe (30 Grad), einem playerclip-Brush,
einem monsterclip-Brush (haelt den Spieler nicht), einem nonsolid-Brush, einem
func_door (Brush-Entity), einem trigger_multiple und einem Patch (Bogen ueber dem
Boden). Dazu die Shaderdatei (surfaceparms) und die mit bhed_q3map2 gebaute .bsp:
der ctest vergleicht die Kollision aus den Brushes mit der aus der .bsp.

  python -I gen_spielraum.py <bhed_q3map2.exe> <tests/karte/q3map2>
"""
import os
import shutil
import subprocess
import sys
import tempfile


def kasten(x0, y0, z0, x1, y1, z1, tex):
    t = '%s 0 0 0 0.5 0.5 0 0 0' % tex
    return ['{',
            '( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s' % (x1, y1, z1, x1, y0, z1, x0, y1, z1, t),
            '( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s' % (x1, y1, z1, x0, y1, z1, x1, y1, z0, t),
            '( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s' % (x1, y1, z1, x1, y1, z0, x1, y0, z1, t),
            '( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s' % (x0, y0, z0, x1, y0, z0, x0, y1, z0, t),
            '( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s' % (x0, y0, z0, x0, y0, z1, x1, y0, z0, t),
            '( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s' % (x0, y0, z0, x0, y1, z0, x0, y0, z1, t),
            '}']


def rampe(x0, x1, y0, y1, hoehe, tex):
    """Ein Keil: unten x0..x1, steigt in +x von 0 auf hoehe."""
    t = '%s 0 0 0 0.5 0.5 0 0 0' % tex
    return ['{',
            '( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s' % (x0, y0, 0, x1, y0, 0, x0, y1, 0, t),             # unten (-z)
            '( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s' % (x0, y0, 0, x0, y1, 0, x1, y0, hoehe, t),         # schraeg
            '( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s' % (x1, y1, 0, x1, y0, 0, x1, y1, hoehe, t),         # +x
            '( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s' % (x0, y0, 0, x0, y0, 10, x1, y0, 0, t),            # -y
            '( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s' % (x0, y1, 0, x1, y1, 0, x0, y1, 10, t),            # +y
            '}']


def patch_bogen(x0, x1, y0, y1, z, tex):
    """patchDef2 3x3: ein Bogen von y0 nach y1, in der Mitte z hoch."""
    zeilen = ['{', 'patchDef2', '{', tex, '( 3 3 0 0 0 )', '(']
    ym = (y0 + y1) // 2
    for x in (x0, (x0 + x1) // 2, x1):
        zeilen.append('( ( %d %d 0 0 0 ) ( %d %d %d 0 0.5 ) ( %d %d 0 0 1 ) )' % (x, y0, x, ym, z, x, y1))
    zeilen += [')', '}', '}']
    return zeilen


def karte():
    z = ['// entity 0', '{', '"classname" "worldspawn"', '"message" "BehavEd Spielraum"']
    z += kasten(-1024, -1024, -16, 1024, 1024, 0, 'test/boden')       # Boden
    z += kasten(512, -1024, 0, 544, 1024, 256, 'test/wand')          # Ostwand (x 512)
    z += kasten(128, -64, 0, 192, 64, 16, 'test/stufe')              # Stufe 16 (x 128..192)
    z += kasten(-192, -64, 0, -128, 64, 24, 'test/stufe')            # Stufe 24 (x -192..-128)
    z += rampe(-128, 0, 256, 384, 74, 'test/stufe')                  # Rampe 30 Grad (y 256..384)
    z += kasten(-64, -448, 0, 64, -384, 128, 'test/clip')            # playerclip (y -448..-384)
    z += kasten(-64, -640, 0, 64, -576, 128, 'test/monsterclip')     # monsterclip (y -640..-576)
    z += kasten(-64, 576, 0, 64, 640, 128, 'test/nonsolid')          # nonsolid (y 576..640)
    z += patch_bogen(-256, -192, -320, -192, 64, 'test/bogen')       # Patch-Bogen
    z += ['}']
    z += ['// entity 1', '{', '"classname" "func_door"', '"angle" "90"']
    z += kasten(256, 576, 0, 320, 640, 128, 'test/tuer')
    z += ['}']
    z += ['// entity 2', '{', '"classname" "trigger_multiple"']
    z += kasten(256, -640, 0, 320, -576, 128, 'test/trigger')
    z += ['}']
    z += ['// entity 3', '{', '"classname" "info_player_start"', '"origin" "-320 320 24"', '"angle" "0"', '}']
    return '\n'.join(z) + '\n'


SHADER = '''textures/test/clip
{
	surfaceparm nodraw
	surfaceparm playerclip
	surfaceparm nonopaque
	surfaceparm trans
}

textures/test/monsterclip
{
	surfaceparm nodraw
	surfaceparm monsterclip
	surfaceparm nonopaque
	surfaceparm trans
}

textures/test/nonsolid
{
	surfaceparm nonsolid
	surfaceparm trans
}

textures/test/trigger
{
	surfaceparm nodraw
	surfaceparm trigger
}
'''


def main():
    q3, ziel = sys.argv[1], sys.argv[2]
    open(os.path.join(ziel, 'spielraum.map'), 'w', encoding='ascii', newline='\n').write(karte())
    open(os.path.join(ziel, 'spielraum.shader'), 'w', encoding='ascii', newline='\n').write(SHADER)
    with tempfile.TemporaryDirectory() as d:
        os.makedirs(os.path.join(d, 'base', 'shaders'))
        shutil.copy(os.path.join(ziel, 'spielraum.shader'), os.path.join(d, 'base', 'shaders', 'spielraum.shader'))
        shutil.copy(os.path.join(ziel, 'spielraum.map'), os.path.join(d, 'spielraum.map'))
        r = subprocess.run([q3, '-game', 'ja', '-fs_basepath', d, '-fs_game', 'base', '-fs_homepath', '', '-meta',
                            os.path.join(d, 'spielraum.map')], capture_output=True, text=True, errors='replace')
        print(r.stdout[-1500:])
        if r.returncode != 0 or not os.path.exists(os.path.join(d, 'spielraum.bsp')):
            print('FEHLER: bhed_q3map2', r.returncode, r.stderr[-800:])
            return 1
        shutil.copy(os.path.join(d, 'spielraum.bsp'), os.path.join(ziel, 'spielraum.bsp'))
    return 0


if __name__ == '__main__':
    sys.exit(main())
