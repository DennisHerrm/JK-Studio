# -*- coding: utf-8 -*-
"""rc608 Teil 13 (Decal-Sonderfaelle): Testkarten fuer den ctest karte_marken.

markensonder.map: ein dichter Raum mit allem, was die Zeichenflaechen der Welt ausser der Meta-Stufe noch aendert:
  - drei Nebel (fogparms): einer in der Welt (schneidet Boden und einen stehenden Patch an seiner Ober- und Unterseite),
    einer in einer func_group an der Wand (seine Rueckseiten, koplanar mit Boden und Wand), einer in einer func_static
    (CreateMapFogs geht alle Entitaeten durch)
  - q3map_cloneShader und q3map_backShader auf Kisten und auf einem Patch
  - ein nodraw-Patch (steht in der .bsp, das Spiel ueberspringt ihn)
  - q3map_terrain (noClip, indexed, 179 Grad) an einer Kiste, die in der Wand steckt
  - ein Dach aus zwei flachen Keilen (-np)
  - T-Kreuzungen an der Wand (-maxarea zieht dann Zufall, MaxAreaIndexes)
  - eine func_group mit _patchMeta 1 (TriangulatePatchSurface fragt nur die Welt)
markensonder_wpm.map: dieselbe Karte, der worldspawn mit _patchMeta 1 und _patchQuality 2.
markendecal.map: _decal-Entitaeten auf Boden, Kiste, Wand, Patch, ohne Ziel und mit origin (-meta, ohne, -maxarea).

Gebaut mit bhed_q3map2 (-game ja) und den Schaltern je .bsp (siehe BAUTEN).

  python -I gen_markensonder.py <bhed_q3map2.exe> <tests/karte/q3map2>
"""
import os
import shutil
import subprocess
import sys
import tempfile

SHADER = """textures/sonder/a
{
	{
		map $whiteimage
	}
}

textures/sonder/nebel
{
	surfaceparm fog
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm nolightmap
	fogparms ( 0.5 0.5 0.5 ) 256
	{
		map $whiteimage
	}
}

textures/sonder/klon
{
	q3map_cloneShader textures/sonder/kopie
	{
		map $whiteimage
	}
}

textures/sonder/kopie
{
	surfaceparm nomarks
	{
		map $whiteimage
	}
}

textures/sonder/rueck
{
	q3map_backShader textures/sonder/hinten
	{
		map $whiteimage
	}
}

textures/sonder/hinten
{
	q3map_lightmapSampleSize 8
	{
		map $whiteimage
	}
}

textures/sonder/nichts
{
	surfaceparm nodraw
	surfaceparm nonsolid
	{
		map $whiteimage
	}
}

textures/sonder/gelaende
{
	q3map_terrain
	{
		map $whiteimage
	}
}

textures/sonder/fleck
{
	polygonOffset
	surfaceparm nonsolid
	{
		map $whiteimage
	}
}

textures/sonder/ohnenebel
{
	q3map_noFog
	{
		map $whiteimage
	}
}
"""

# je Kasten sechs Seiten (wie gen_markenmeta.py)
SEITEN = [
    ((1, 1, 1), (1, 0, 1), (0, 1, 1)),   # oben
    ((1, 1, 1), (0, 1, 1), (1, 1, 0)),   # +y
    ((1, 1, 1), (1, 1, 0), (1, 0, 1)),   # +x
    ((0, 0, 0), (1, 0, 0), (0, 1, 0)),   # unten
    ((0, 0, 0), (0, 0, 1), (1, 0, 0)),   # -y
    ((0, 0, 0), (0, 1, 0), (0, 0, 1)),   # -x
]


def kasten(x0, y0, z0, x1, y1, z1, tex, massstab=0.5):
    t = '{\n'
    for s in SEITEN:
        p = tuple(tuple((x1 if a[0] else x0, y1 if a[1] else y0, z1 if a[2] else z0)) for a in s)
        t += '%s sonder/%s 0 0 0 %g %g 0 0 0\n' % (' '.join('( %g %g %g )' % q for q in p), tex, massstab, massstab)
    return t + '}\n'


def keil(x0, y0, z0, x1, y1, z1, tex, gespiegelt=False):
    """Ein flacher Keil, die schraege Seite faellt nach +x (wie gen_markenmeta.py), gespiegelt nach -x"""
    t = '{\n'
    for p in [
        ((x0, y0, z1), (x0, y1, z1), (x1, y0, z0)),
        ((x0, y1, z0), (x0, y1, z1), (x1, y1, z0)),
        ((x0, y0, z0), (x1, y0, z0), (x0, y1, z0)),
        ((x0, y0, z0), (x0, y0, z1), (x1, y0, z0)),
        ((x0, y0, z0), (x0, y1, z0), (x0, y0, z1)),
    ]:
        if gespiegelt:   # x an der Mitte spiegeln dreht die Ebene um: die Reihenfolge der Punkte umkehren
            p = tuple((x0 + x1 - q[0], q[1], q[2]) for q in reversed(p))
        t += '%s sonder/%s 0 0 0 0.5 0.5 0 0 0\n' % (' '.join('( %g %g %g )' % q for q in p), tex)
    return t + '}\n'


def patch(tex, punkte):
    """punkte: Zeilen von (x, y, z, s, t)"""
    zeilen = ''
    for zeile in punkte:
        zeilen += '( ' + ''.join('( %g %g %g %g %g ) ' % p for p in zeile) + ')\n'
    return '{\npatchDef2\n{\nsonder/%s\n( %d %d 0 0 0 )\n(\n%s)\n}\n}\n' % (tex, len(punkte), len(punkte[0]), zeilen)


def stehender_patch(tex):
    # Zeilen nach x (-100, 0, 100), Spalten nach oben (z 0 bis 128): der Nebel schneidet ihn bei z 72
    return patch(tex, [[(x, -150 + (24 if x == 0 else 0), z, i * 0.5, j * 0.25) for j, z in enumerate((0, 32, 64, 96, 128))]
                       for i, x in enumerate((-100, 0, 100))])


def flacher_patch(tex, x0, y0, z, umgedreht=False):
    """umgedreht: Zeilen und Spalten getauscht - der Patch zeigt in die andere Richtung (fuer Decals: weg vom Ziel)"""
    punkt = lambda i, j: (x0 + 32 * i, y0 + 32 * j, z + (8 if i == 1 and j == 1 else 0), i * 0.5, j * 0.5)
    if umgedreht:
        return patch(tex, [[punkt(i, j) for j in range(3)] for i in range(3)])
    return patch(tex, [[punkt(i, j) for i in range(3)] for j in range(3)])


def karte(welt_patchmeta):
    t = '// entity 0\n{\n"classname" "worldspawn"\n"message" "JK Studio Markensonder"\n'
    if welt_patchmeta:
        t += '"_patchMeta" "1"\n"_patchQuality" "2"\n'
    t += kasten(-512, -512, -16, 512, 512, 0, 'a')
    t += kasten(-512, -512, 256, 512, 512, 272, 'a', 0.25)
    t += kasten(-528, -512, 0, -512, 512, 256, 'a')
    t += kasten(512, -512, 0, 528, 512, 256, 'a')
    t += kasten(-512, -528, 0, 512, -512, 256, 'a')
    t += kasten(-512, 512, 0, 512, 528, 256, 'a')
    # T-Kreuzungen: Kisten an der Nordwand und aufeinander
    t += kasten(-128, 256, 0, 64, 512, 64, 'a')
    t += kasten(-64, 320, 64, 0, 512, 128, 'a')
    t += kasten(64, 256, 0, 128, 512, 32, 'a')
    # Kopien und Rueckseiten
    t += kasten(-400, -400, 0, -300, -300, 32, 'klon')
    t += kasten(300, -450, 0, 400, -350, 32, 'rueck')
    t += kasten(200, 480, 0, 260, 540, 40, 'gelaende')   # steckt in der Nordwand: noClip behaelt die ganze Seite
    # ein Dach aus zwei Keilen (je 20 Grad, First bei x -72): -np 60 glaettet am First und setzt ueber beide Ebenen zusammen
    t += keil(-200, 120, 0, -72, 220, 46, 'a', True)
    t += keil(-72, 120, 0, 56, 220, 46, 'a')
    t += kasten(-250, -100, 0, -200, -50, 24, 'ohnenebel')   # im Nebel, aber q3map_noFog
    # der Nebel der Welt: Boden, eine Kiste und der stehende Patch liegen darin
    t += kasten(-300, -300, -16, 300, 0, 72, 'nebel')
    t += stehender_patch('a')
    t += flacher_patch('nichts', -480, 380, 0)
    t += flacher_patch('rueck', 380, 380, 0)
    t += flacher_patch('klon', 380, -200, 0)
    t += '}\n'
    # ein Nebel in einer func_group an der Ostwand (koplanar mit Boden und Wand), dazu _patchMeta (fragt q3map2 nicht)
    t += '// entity 1\n{\n"classname" "func_group"\n"_patchMeta" "1"\n'
    t += kasten(400, 100, 0, 512, 300, 128, 'nebel')
    t += flacher_patch('a', 420, 120, 0)
    t += '}\n'
    # ein Nebel in einer func_static (eigenes Modell, schneidet trotzdem die Welt)
    t += '// entity 2\n{\n"classname" "func_static"\n'
    t += kasten(-450, 250, -8, -350, 350, 64, 'nebel')
    t += '}\n// entity 3\n{\n"classname" "info_player_start"\n"origin" "0 -400 64"\n"angle" "90"\n}\n'
    return t


def decal(nummer, patchtext, ziel=None, ursprung=None):
    t = '// entity %d\n{\n"classname" "_decal"\n' % nummer
    if ziel is not None:
        t += '"target" "%s"\n' % ziel
    if ursprung is not None:
        t += '"origin" "%s"\n' % ursprung
    return t + patchtext + '}\n'


def decalkarte():
    """_decal-Entitaeten (ProcessDecals, MakeEntityDecals): auf Boden, Kiste, Wand, einen Patch, ohne Ziel, mit origin"""
    t = '// entity 0\n{\n"classname" "worldspawn"\n"message" "JK Studio Markendecal"\n'
    t += kasten(-512, -512, -16, 512, 512, 0, 'a')
    t += kasten(-512, -512, 256, 512, 512, 272, 'a', 0.25)
    t += kasten(-528, -512, 0, -512, 512, 256, 'a')
    t += kasten(512, -512, 0, 528, 512, 256, 'a')
    t += kasten(-512, -528, 0, 512, -512, 256, 'a')
    t += kasten(-512, 512, 0, 512, 528, 256, 'a')
    t += kasten(-80, 0, 0, -40, 40, 24, 'a')
    t += kasten(100, 100, 0, 160, 160, 32, 'klon')   # die Kopie traegt keine sideRefs: kein Decal auf ihr
    t += flacher_patch('a', 200, -300, 0)
    t += '}\n'
    t += decal(1, flacher_patch('fleck', -120, -20, 96, True), 'unten')   # Boden und Kiste
    t += '// entity 2\n{\n"classname" "info_null"\n"targetname" "unten"\n"origin" "-88 12 -64"\n}\n'
    for nummer, umgedreht in ((3, False), (9, True)):   # an die Ostwand, beide Richtungen (eine faellt am backface check)
        wand = [[(400, -40 + 40 * i, 40 + 40 * j, i * 0.5, j * 0.5) for j in range(3)] for i in range(3)]
        if umgedreht:
            wand = [list(z) for z in zip(*wand)]
        t += decal(nummer, patch('fleck', wand), 'wand')
    t += '// entity 4\n{\n"classname" "info_null"\n"targetname" "wand"\n"origin" "640 0 80"\n}\n'
    t += decal(5, flacher_patch('fleck', 200, -300, 80, True), 'unten')    # auf den flachen Patch (schraeg nach unten)
    t += decal(6, flacher_patch('fleck', 90, 90, 64, True), None)          # ohne target: FindTargetEntity("") findet den worldspawn
    t += decal(7, flacher_patch('fleck', -300, 200, 0) + flacher_patch('fleck', -300, 140, 0, True), 'unten', '0 0 120')   # mit origin
    t += '// entity 8\n{\n"classname" "info_player_start"\n"origin" "0 -400 64"\n"angle" "90"\n}\n'
    return t


# Name der .bsp: (Karte, Schalter von -bsp)
BAUTEN = [
    ('markensonder', 'markensonder', ['-meta']),
    ('markensonder_np', 'markensonder', ['-meta', '-np', '60']),
    ('markensonder_max', 'markensonder', ['-meta', '-maxarea']),
    ('markensonder_maxo', 'markensonder', ['-maxarea']),
    ('markensonder_pm', 'markensonder', ['-meta', '-patchmeta']),
    ('markensonder_nofog', 'markensonder', ['-meta', '-nofog']),
    ('markensonder_wpm', 'markensonder_wpm', ['-meta']),
    ('markendecal', 'markendecal', ['-meta']),
    ('markendecal_ohne', 'markendecal', []),
    ('markendecal_max', 'markendecal', ['-meta', '-maxarea']),
]


def bauen(q3, ziel, ausgabe, name, schalter):
    with tempfile.TemporaryDirectory() as d:
        os.makedirs(os.path.join(d, 'base', 'shaders'))
        shutil.copy(os.path.join(ziel, 'markensonder.shader'), os.path.join(d, 'base', 'shaders', 'markensonder.shader'))
        shutil.copy(os.path.join(ziel, name + '.map'), os.path.join(d, name + '.map'))
        r = subprocess.run([q3, '-game', 'ja', '-fs_basepath', d, '-fs_game', 'base', '-fs_homepath', ''] + schalter +
                           [os.path.join(d, name + '.map')], capture_output=True, text=True, errors='replace')
        if r.returncode != 0 or not os.path.exists(os.path.join(d, name + '.bsp')):
            print('FEHLER: bhed_q3map2', ausgabe, r.returncode, r.stdout[-1500:], r.stderr[-800:])
            return False
        shutil.copy(os.path.join(d, name + '.bsp'), os.path.join(ziel, ausgabe + '.bsp'))
        print(ausgabe, 'gebaut', [z.strip() for z in r.stdout.splitlines() if 'fog' in z or 'WARNING' in z or 'meta' in z.lower()][:8])
    return True


def main():
    q3, ziel = sys.argv[1], sys.argv[2]
    open(os.path.join(ziel, 'markensonder.shader'), 'w', encoding='ascii', newline='\n').write(SHADER)
    open(os.path.join(ziel, 'markensonder.map'), 'w', encoding='ascii', newline='\n').write(karte(False))
    open(os.path.join(ziel, 'markensonder_wpm.map'), 'w', encoding='ascii', newline='\n').write(karte(True))
    open(os.path.join(ziel, 'markendecal.map'), 'w', encoding='ascii', newline='\n').write(decalkarte())
    gut = True
    for ausgabe, name, schalter in BAUTEN:
        gut = bauen(q3, ziel, ausgabe, name, schalter) and gut
    return 0 if gut else 1


if __name__ == '__main__':
    sys.exit(main())
