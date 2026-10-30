# -*- coding: utf-8 -*-
"""Abschluss rc607 (Decals im Karten-Editor): die zweite Testkarte fuer den ctest karte_marken.

Ein dichter Raum mit Spieler-Start (FillOutside). Im Westen liegt ein Boden mit
"deformVertexes move" (test/bewegt: move 4 4 32 sin 0 1 0 1 - mins 0 0 0, maxs 4 4 32) unter
einer Stufe von 16: q3map2 verdickt die Wicklung einer solchen Flaeche beim Einsortieren in den
Baum um diesen Kasten (FilterWindingIntoTree_r), der Boden unter der Stufe steht damit auch in
den offenen Blaettern darueber. Im Osten 10 x 10 Saeulen (8 x 8, 32 hoch, Abstand 16): eine grosse
Marke von oben beruehrt 100 Deckflaechen, R_BoxSurfaces_r nimmt nur die ersten 64 - in der
Reihenfolge der Blaetter und ihrer Flaechen (der Boden liegt tiefer als der Kasten der Marke, die
Seiten zeigen quer zur Richtung und zaehlen nicht). Gebaut wie markenraum, aber ohne -meta (meta setzt
die Flaechen neu zusammen; das baut der Editor nicht nach).

  python -I gen_markenreihe.py <bhed_q3map2.exe> <tests/karte/q3map2>
"""
import os
import shutil
import subprocess
import sys
import tempfile

SHADER = """textures/test/bewegt
{
	deformVertexes move 4 4 32 sin 0 1 0 1
	{
		map $whiteimage
	}
}

textures/test/flach
{
	deformVertexes move 0 0 32 sin 0 1 0 1
	{
		map $whiteimage
	}
}
"""


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


def karte():
    z = ['// entity 0', '{', '"classname" "worldspawn"', '"message" "JK Studio Markenreihe"']
    z += kasten(-544, -288, -16, 0, 288, 0, 'test/bewegt')         # Boden West: deformVertexes move
    z += kasten(0, -288, -16, 544, 288, 0, 'test/boden')           # Boden Ost
    z += kasten(-544, -288, 0, -512, 288, 256, 'test/wand')        # Westwand
    z += kasten(512, -288, 0, 544, 288, 256, 'test/wand')          # Ostwand
    z += kasten(-512, -288, 0, 512, -256, 256, 'test/wand')        # Suedwand
    z += kasten(-512, 256, 0, 512, 288, 256, 'test/wand')          # Nordwand
    z += kasten(-544, -288, 256, 544, 288, 272, 'test/wand')       # Decke
    z += kasten(-192, -64, 0, -128, 64, 16, 'test/stufe')          # Stufe 16 auf dem bewegten Boden
    for i in range(10):                                            # 10 x 10 Saeulen im Osten
        for j in range(10):
            x = 96 + 16 * i
            y = -80 + 16 * j
            z += kasten(x, y, 0, x + 8, y + 8, 32, 'test/klotz')
    z += ['}']
    z += ['// entity 1', '{', '"classname" "info_player_start"', '"origin" "-320 160 24"', '"angle" "0"', '}']
    return '\n'.join(z) + '\n'


def main():
    q3, ziel = sys.argv[1], sys.argv[2]
    open(os.path.join(ziel, 'markenreihe.map'), 'w', encoding='ascii', newline='\n').write(karte())
    open(os.path.join(ziel, 'markenreihe.shader'), 'w', encoding='ascii', newline='\n').write(SHADER)
    with tempfile.TemporaryDirectory() as d:
        os.makedirs(os.path.join(d, 'base', 'shaders'))
        shutil.copy(os.path.join(ziel, 'markenreihe.shader'), os.path.join(d, 'base', 'shaders', 'markenreihe.shader'))
        shutil.copy(os.path.join(ziel, 'markenreihe.map'), os.path.join(d, 'markenreihe.map'))
        r = subprocess.run([q3, '-game', 'ja', '-fs_basepath', d, '-fs_game', 'base', '-fs_homepath', '',
                            os.path.join(d, 'markenreihe.map')], capture_output=True, text=True, errors='replace')
        print(r.stdout[-2500:])
        if r.returncode != 0 or not os.path.exists(os.path.join(d, 'markenreihe.bsp')):
            print('FEHLER: bhed_q3map2', r.returncode, r.stderr[-800:])
            return 1
        shutil.copy(os.path.join(d, 'markenreihe.bsp'), os.path.join(ziel, 'markenreihe.bsp'))
    return 0


if __name__ == '__main__':
    sys.exit(main())
