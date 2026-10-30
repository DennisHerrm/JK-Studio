# -*- coding: utf-8 -*-
"""Liste reste2 Punkt 4 (Decals im Karten-Editor): die Testkarte fuer den ctest karte_marken.

Ein dichter Raum (Boden, vier Waende, Decke) mit Spieler-Start, damit q3map2 den Raum
ausserhalb fuellt (FillOutside). Der Boden reicht im Osten 224 Einheiten unter der Wand
hindurch nach draussen; eine Stufe von 16, ein Detail-Pinsel von 8 (Detail macht kein Blatt
deckend), ein misc_model (Kasten 32 x 32 x 32, .ase), das q3map2 in die Welt setzt. Dazu das
Modell und die mit bhed_q3map2 gebaute .bsp: der ctest vergleicht die Marken auf der Welt des
Editors mit denen auf der .bsp.

  python -I gen_markenraum.py <bhed_q3map2.exe> <tests/karte/q3map2>
"""
import os
import shutil
import subprocess
import sys
import tempfile

DETAIL = 134217728   # C_DETAIL in den Inhaltsflags der Seite


def kasten(x0, y0, z0, x1, y1, z1, tex, inhalt=0):
    t = '%s 0 0 0 0.5 0.5 %d 0 0' % (tex, inhalt)
    return ['{',
            '( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s' % (x1, y1, z1, x1, y0, z1, x0, y1, z1, t),
            '( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s' % (x1, y1, z1, x0, y1, z1, x1, y1, z0, t),
            '( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s' % (x1, y1, z1, x1, y1, z0, x1, y0, z1, t),
            '( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s' % (x0, y0, z0, x1, y0, z0, x0, y1, z0, t),
            '( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s' % (x0, y0, z0, x0, y0, z1, x1, y0, z0, t),
            '( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s' % (x0, y0, z0, x0, y1, z0, x0, y0, z1, t),
            '}']


def karte():
    z = ['// entity 0', '{', '"classname" "worldspawn"', '"message" "BehavEd Markenraum"']
    z += kasten(-544, -544, -16, 768, 544, 0, 'test/boden')         # Boden, im Osten bis x 768 (draussen)
    z += kasten(-544, -544, 0, -512, 544, 256, 'test/wand')         # Westwand
    z += kasten(512, -544, 0, 544, 544, 256, 'test/wand')           # Ostwand
    z += kasten(-512, -544, 0, 512, -512, 256, 'test/wand')         # Suedwand
    z += kasten(-512, 512, 0, 512, 544, 256, 'test/wand')           # Nordwand
    z += kasten(-544, -544, 256, 544, 544, 272, 'test/wand')        # Decke
    z += kasten(128, -64, 0, 192, 64, 16, 'test/stufe')             # Stufe 16
    z += kasten(-256, -64, 0, -192, 64, 8, 'test/stufe', DETAIL)    # Detail-Pinsel 8 hoch
    z += ['}']
    z += ['// entity 1', '{', '"classname" "info_player_start"', '"origin" "-320 320 24"', '"angle" "0"', '}']
    z += ['// entity 2', '{', '"classname" "misc_model"', '"origin" "0 256 0"', '"model" "models/test/marken.ase"', '}']
    return '\n'.join(z) + '\n'


def modell():
    """Ein Kasten 32 x 32 x 32 (Boden auf z 0), zwoelf Dreiecke nach aussen."""
    e = [(-16, -16, 0), (16, -16, 0), (16, 16, 0), (-16, 16, 0), (-16, -16, 32), (16, -16, 32), (16, 16, 32), (-16, 16, 32)]
    f = [(0, 2, 1), (0, 3, 2), (4, 5, 6), (4, 6, 7), (0, 1, 5), (0, 5, 4), (1, 2, 6), (1, 6, 5), (2, 3, 7), (2, 7, 6), (3, 0, 4), (3, 4, 7)]
    z = ['*3DSMAX_ASCIIEXPORT\t200', '*MATERIAL_LIST {', '\t*MATERIAL_COUNT 1', '\t*MATERIAL 0 {',
         '\t\t*MATERIAL_NAME "textures/test/modell"', '\t\t*MAP_DIFFUSE {', '\t\t\t*BITMAP "textures/test/modell.tga"', '\t\t}',
         '\t}', '}', '*GEOMOBJECT {', '\t*NODE_NAME "kasten"', '\t*NODE_TM {', '\t\t*NODE_NAME "kasten"',
         '\t\t*TM_ROW0 1.0000\t0.0000\t0.0000', '\t\t*TM_ROW1 0.0000\t1.0000\t0.0000', '\t\t*TM_ROW2 0.0000\t0.0000\t1.0000',
         '\t\t*TM_ROW3 0.0000\t0.0000\t0.0000', '\t}', '\t*MESH {', '\t\t*TIMEVALUE 0',
         '\t\t*MESH_NUMVERTEX %d' % len(e), '\t\t*MESH_NUMFACES %d' % len(f), '\t\t*MESH_VERTEX_LIST {']
    z += ['\t\t\t*MESH_VERTEX %d\t%.4f\t%.4f\t%.4f' % (i, x, y, w) for i, (x, y, w) in enumerate(e)]
    z += ['\t\t}', '\t\t*MESH_FACE_LIST {']
    z += ['\t\t\t*MESH_FACE %d: A: %d B: %d C: %d AB: 1 BC: 1 CA: 1 *MESH_SMOOTHING 1 *MESH_MTLID 0' % (i, a, b, c) for i, (a, b, c) in enumerate(f)]
    z += ['\t\t}', '\t\t*MESH_NUMTVERTEX 4', '\t\t*MESH_TVERTLIST {', '\t\t\t*MESH_TVERT 0\t0.0000\t0.0000\t0.0000',
          '\t\t\t*MESH_TVERT 1\t1.0000\t0.0000\t0.0000', '\t\t\t*MESH_TVERT 2\t1.0000\t1.0000\t0.0000',
          '\t\t\t*MESH_TVERT 3\t0.0000\t1.0000\t0.0000', '\t\t}', '\t\t*MESH_NUMTVFACES %d' % len(f), '\t\t*MESH_TFACELIST {']
    z += ['\t\t\t*MESH_TFACE %d\t0\t%d\t%d' % (i, 1 + i % 2, 2 + i % 2) for i in range(len(f))]
    z += ['\t\t}', '\t}', '\t*MATERIAL_REF 0', '}']
    return '\n'.join(z) + '\n'


def main():
    q3, ziel = sys.argv[1], sys.argv[2]
    open(os.path.join(ziel, 'markenraum.map'), 'w', encoding='ascii', newline='\n').write(karte())
    open(os.path.join(ziel, 'marken.ase'), 'w', encoding='ascii', newline='\n').write(modell())
    with tempfile.TemporaryDirectory() as d:
        os.makedirs(os.path.join(d, 'base', 'models', 'test'))
        shutil.copy(os.path.join(ziel, 'marken.ase'), os.path.join(d, 'base', 'models', 'test', 'marken.ase'))
        shutil.copy(os.path.join(ziel, 'markenraum.map'), os.path.join(d, 'markenraum.map'))
        r = subprocess.run([q3, '-game', 'ja', '-fs_basepath', d, '-fs_game', 'base', '-fs_homepath', '', '-meta',
                            os.path.join(d, 'markenraum.map')], capture_output=True, text=True, errors='replace')
        print(r.stdout[-2500:])
        if r.returncode != 0 or not os.path.exists(os.path.join(d, 'markenraum.bsp')):
            print('FEHLER: bhed_q3map2', r.returncode, r.stderr[-800:])
            return 1
        shutil.copy(os.path.join(d, 'markenraum.bsp'), os.path.join(ziel, 'markenraum.bsp'))
    return 0


if __name__ == '__main__':
    sys.exit(main())
