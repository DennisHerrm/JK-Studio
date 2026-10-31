# -*- coding: utf-8 -*-
"""Abschluss rc607 (Decals, -meta): die Testkarten fuer die Meta-Stufe im ctest karte_marken.

Baut mit -meta (der Vorgabe von NetRadiant-Custom):
  markenreihe_meta.bsp   aus markenreihe.map (100 Deckflaechen in einem Kasten)
  markenmeta.map/.bsp    ein Raum mit allem, was die Meta-Stufe unterscheidet: mehrere Shader und
                         Texturmassstaebe, schraege Seiten und T-Kreuzungen, func_group mit eigener
                         Abtastung und Farbe, ein Patch, Shader mit q3map_nonplanar, tessSize,
                         q3map_notjunc, q3map_lightmapSampleSize, q3map_lightmapAxis, q3map_shadeAngle,
                         q3map_tcGen, q3map_invert, q3map_textureSize, q3map_globaltexture,
                         surfaceparm pointlight und autosprite
  markenmeta_bp.map/.bsp dieselbe Geometrie im Format brushDef (Brush Primitives)

  python -I gen_markenmeta.py <bhed_q3map2.exe> <tests/karte/q3map2>
"""
import os
import shutil
import subprocess
import sys
import tempfile

SHADER = """textures/meta/a
{
	{
		map $whiteimage
	}
}

textures/meta/b
{
	q3map_lightmapSampleSize 8
	{
		map $whiteimage
	}
}

textures/meta/punkt
{
	surfaceparm pointlight
	{
		map $whiteimage
	}
}

textures/meta/krumm
{
	q3map_nonplanar
	q3map_shadeAngle 60
	{
		map $whiteimage
	}
}

textures/meta/teil
{
	tessSize 64
	{
		map $whiteimage
	}
}

textures/meta/ohnet
{
	q3map_notjunc
	q3map_lightmapAxis z
	{
		map $whiteimage
	}
}

textures/meta/tc
{
	q3map_tcGen ivector ( 256 0 0 ) ( 0 128 0 )
	q3map_textureSize 256 128
	{
		map $whiteimage
	}
}

textures/meta/innen
{
	q3map_invert
	q3map_globaltexture
	{
		map $whiteimage
	}
}

textures/meta/sprite
{
	deformVertexes autosprite
	{
		map $whiteimage
	}
}
"""

# je Punkt der Kasten als sechs Seiten, wie in gen_markenreihe.py
SEITEN = [
    ((1, 1, 1), (1, 0, 1), (0, 1, 1)),   # oben
    ((1, 1, 1), (0, 1, 1), (1, 1, 0)),   # +y
    ((1, 1, 1), (1, 1, 0), (1, 0, 1)),   # +x
    ((0, 0, 0), (1, 0, 0), (0, 1, 0)),   # unten
    ((0, 0, 0), (0, 0, 1), (1, 0, 0)),   # -y
    ((0, 0, 0), (0, 1, 0), (0, 0, 1)),   # -x
]


def seite(p, tex, bp, massstab=0.5, drehung=0, schub=(0, 0)):
    punkte = ' '.join('( %g %g %g )' % q for q in p)
    if bp:
        return '%s ( ( %g 0 %g ) ( 0 %g %g ) ) meta/%s 0 0 0\n' % (punkte, 1 / (massstab * 64), schub[0] / 64, 1 / (massstab * 64), schub[1] / 64, tex)
    return '%s meta/%s %g %g %g %g %g 0 0 0\n' % (punkte, tex, schub[0], schub[1], drehung, massstab, massstab)


def kasten(x0, y0, z0, x1, y1, z1, tex, bp, massstab=0.5, drehung=0, schub=(0, 0)):
    t = '{\n' + ('brushDef\n{\n' if bp else '')
    for s in SEITEN:
        p = tuple(tuple((x1 if a[0] else x0, y1 if a[1] else y0, z1 if a[2] else z0)) for a in s)
        t += seite(p, tex, bp, massstab, drehung, schub)
    return t + ('}\n' if bp else '') + '}\n'


def keil(x0, y0, z0, x1, y1, z1, tex, bp):
    """Ein Keil: die schraege Seite faellt nach +x (Kanten, die keine Achse entlang laufen)."""
    t = '{\n' + ('brushDef\n{\n' if bp else '')
    for p in [
        ((x0, y0, z1), (x0, y1, z1), (x1, y0, z0)),   # schraeg oben
        ((x0, y1, z0), (x0, y1, z1), (x1, y1, z0)),   # +y (Normale nach aussen wie in SEITEN)
        ((x0, y0, z0), (x1, y0, z0), (x0, y1, z0)),   # unten
        ((x0, y0, z0), (x0, y0, z1), (x1, y0, z0)),   # -y
        ((x0, y0, z0), (x0, y1, z0), (x0, y0, z1)),   # -x
    ]:
        t += seite(p, tex, bp)
    return t + ('}\n' if bp else '') + '}\n'


def patch(bp):
    # ein flacher Bogen ueber dem Boden; die Raender liegen gerade (AddPatchEdges)
    zeilen = ''
    for j, y in enumerate((-64, 0, 64)):
        zeilen += '( '
        for i, x in enumerate((-200, -136, -72)):
            z = 48 if i == 1 and j == 1 else 16
            zeilen += '( %d %d %d %g %g ) ' % (x, y, z, i * 0.5, j * 0.5)
        zeilen += ')\n'
    return '{\npatchDef2\n{\nmeta/a\n( 3 3 0 0 0 )\n(\n' + zeilen + ')\n}\n}\n'


def karte(bp):
    t = '// entity 0\n{\n"classname" "worldspawn"\n"message" "JK Studio Markenmeta"\n'
    # der dichte Raum: 1024 x 1024 x 256
    t += kasten(-512, -512, -16, 512, 512, 0, 'a', bp)
    t += kasten(-512, -512, 256, 512, 512, 272, 'a', bp, 0.25)
    t += kasten(-528, -512, 0, -512, 512, 256, 'b', bp)
    t += kasten(512, -512, 0, 528, 512, 256, 'b', bp, 0.5, 90)
    t += kasten(-512, -528, 0, 512, -512, 256, 'punkt', bp)
    t += kasten(-512, 512, 0, 512, 528, 256, 'ohnet', bp)
    # T-Kreuzungen: drei Kisten stehen auf derselben Wand und aufeinander
    t += kasten(-128, 256, 0, 64, 512, 64, 'a', bp, 0.5, 0, (12, 7))
    t += kasten(-64, 320, 64, 0, 512, 128, 'a', bp)
    t += kasten(64, 256, 0, 128, 512, 32, 'b', bp)
    # schraeg, mit Treffern auf derselben Textur
    t += keil(200, -128, 0, 328, 64, 74, 'a', bp)
    t += keil(200, 64, 0, 328, 128, 74, 'krumm', bp)
    t += keil(340, -128, 0, 468, 64, 74, 'krumm', bp)
    # unterteilt, Koordinatenmassstab, umgedreht, Sprite
    t += kasten(-400, -400, 0, -240, -240, 8, 'teil', bp)
    t += kasten(-200, -400, 0, -40, -240, 8, 'tc', bp)
    t += kasten(0, -400, 0, 160, -240, 8, 'innen', bp)
    t += kasten(200, -400, 0, 216, -384, 16, 'sprite', bp)
    t += patch(bp)
    t += '}\n'
    # eine func_group mit eigener Abtastung und Farbe: nicht mit der Welt zusammengesetzt
    t += '// entity 1\n{\n"classname" "func_group"\n"_lightmapsamplesize" "4"\n'
    t += kasten(-400, 100, 0, -240, 260, 16, 'a', bp)
    t += '}\n// entity 2\n{\n"classname" "func_group"\n"_color" "1 0.5 0.25"\n"_cs" "0"\n'
    t += kasten(-200, 100, 0, -160, 260, 16, 'a', bp)
    t += '}\n// entity 3\n{\n"classname" "info_player_start"\n"origin" "0 0 64"\n"angle" "0"\n}\n'
    return t


def bauen(q3, ziel, name, shader, ausgabe=None):
    with tempfile.TemporaryDirectory() as d:
        os.makedirs(os.path.join(d, 'base', 'shaders'))
        shutil.copy(os.path.join(ziel, shader + '.shader'), os.path.join(d, 'base', 'shaders', shader + '.shader'))
        shutil.copy(os.path.join(ziel, name + '.map'), os.path.join(d, name + '.map'))
        r = subprocess.run([q3, '-game', 'ja', '-fs_basepath', d, '-fs_game', 'base', '-fs_homepath', '', '-meta',
                            os.path.join(d, name + '.map')], capture_output=True, text=True, errors='replace')
        if r.returncode != 0 or not os.path.exists(os.path.join(d, name + '.bsp')):
            print('FEHLER: bhed_q3map2', name, r.returncode, r.stdout[-1500:], r.stderr[-800:])
            return False
        shutil.copy(os.path.join(d, name + '.bsp'), os.path.join(ziel, (ausgabe or name) + '.bsp'))
        print(name, 'gebaut', [z for z in r.stdout.splitlines() if 'surfaces merged' in z or 'WARNING' in z][:6])
    return True


def main():
    q3, ziel = sys.argv[1], sys.argv[2]
    open(os.path.join(ziel, 'markenmeta.shader'), 'w', encoding='ascii', newline='\n').write(SHADER)
    open(os.path.join(ziel, 'markenmeta.map'), 'w', encoding='ascii', newline='\n').write(karte(False))
    open(os.path.join(ziel, 'markenmeta_bp.map'), 'w', encoding='ascii', newline='\n').write(karte(True))
    gut = bauen(q3, ziel, 'markenreihe', 'markenreihe', 'markenreihe_meta')
    gut = bauen(q3, ziel, 'markenmeta', 'markenmeta') and gut
    gut = bauen(q3, ziel, 'markenmeta_bp', 'markenmeta') and gut
    return 0 if gut else 1


if __name__ == '__main__':
    sys.exit(main())
