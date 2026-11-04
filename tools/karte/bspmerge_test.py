"""rc609 B1: Brush-Entities in fertige Karten einfuegen (Teil-Compile und BSP-Merge), Stufe 1.

  python bspmerge_test.py <bhed_q3map2.exe> <bspmergetest.exe> <tests/karte/q3map2> <arbeitsordner>

Original: raum.map (Welt, func_door, Lichter) plus ein target_print "drucker", gebaut mit -meta,
-vis und -light -fast (Lightmaps und VIS, damit "Byte-gleich" etwas heisst). Teil: worldspawn mit
Platzhalter plus trigger_multiple (target "drucker"), trigger_once (target "fehlt", reicht in die
Wand) und func_usable aus einem soliden nodraw-Brush (steckt im Boden) und einem origin-Brush,
gebaut mit q3map2 -bsp wie der Befehl "In fertige Karte einfuegen".
Gegenproben: sichtbarer func_door im Teil (Fehler "sichtbar", Stufe 3), Teil als IBSP (Fehler "format").
Die Welt des Teils ist nur ein Platzhalter und wird nicht uebernommen.

rc609 B13 Stufe 2: ein playerclip-Brush in der Oeffnung der Mittelwand (func_static mit der Marke _bhed_welt wie
karte::teilKarte) kommt in die Welt; Gegenprobe ein solider nodraw-Brush (Fehler "welt").
Stufe 3: Licht-Teil wie der Karten-Editor ihn baut (Welt von raum.map als Schattenwerfer, ihre Lichter, ein neuer func_door
mit test/tuer), die Befehle aus teilLichtBefehle (Schalter aus _q3map2_cmdline der Karte), uebernommen nur die Tuer.
"""
import os
import shutil
import subprocess
import sys

Q3, PRUEF, DATEN, ARBEIT = sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4]
fehler = 0

SHADER = '''textures/bm/trigger
{
	surfaceparm nodraw
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm trigger
}

textures/bm/nodraw_solid
{
	surfaceparm nodraw
	surfaceparm nomarks
}

textures/bm/clip
{
	surfaceparm nodraw
	surfaceparm nonsolid
	surfaceparm playerclip
	surfaceparm monsterclip
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm nolightmap
}

textures/bm/origin
{
	surfaceparm nodraw
	surfaceparm nonsolid
	surfaceparm origin
	surfaceparm trans
}
'''


def lauf(*args, spiel='ja'):
    r = subprocess.run([Q3, '-game', spiel, '-fs_basepath', ARBEIT, '-fs_homepath', ''] + list(args), capture_output=True, text=True,
                       errors='replace', creationflags=0x08000000 if os.name == 'nt' else 0)
    return r.stdout + r.stderr


def pruefe(ok, was):
    global fehler
    print(('OK    ' if ok else 'FEHL  ') + was)
    if not ok:
        fehler += 1


def quader(x0, y0, z0, x1, y1, z1, shader):
    return ('{\n( %g %g %g ) ( %g %g %g ) ( %g %g %g ) %s 0 0 0 0.5 0.5 0 0 0\n' % (x1, y1, z1, x1, y0, z1, x0, y1, z1, shader) +
            '( %g %g %g ) ( %g %g %g ) ( %g %g %g ) %s 0 0 0 0.5 0.5 0 0 0\n' % (x1, y1, z1, x0, y1, z1, x1, y1, z0, shader) +
            '( %g %g %g ) ( %g %g %g ) ( %g %g %g ) %s 0 0 0 0.5 0.5 0 0 0\n' % (x1, y1, z1, x1, y1, z0, x1, y0, z1, shader) +
            '( %g %g %g ) ( %g %g %g ) ( %g %g %g ) %s 0 0 0 0.5 0.5 0 0 0\n' % (x0, y0, z0, x1, y0, z0, x0, y1, z0, shader) +
            '( %g %g %g ) ( %g %g %g ) ( %g %g %g ) %s 0 0 0 0.5 0.5 0 0 0\n' % (x0, y0, z0, x0, y0, z1, x1, y0, z0, shader) +
            '( %g %g %g ) ( %g %g %g ) ( %g %g %g ) %s 0 0 0 0.5 0.5 0 0 0\n}\n' % (x0, y0, z0, x0, y1, z0, x0, y0, z1, shader))


def schreibe(name, text):
    m = os.path.join(ARBEIT, name + '.map')
    open(m, 'w', encoding='latin-1', newline='').write(text)
    return m


def welt(inhalt=quader(-8, -8, -8, 8, 8, 8, 'system/caulk')):
    """worldspawn mit einem Platzhalter-Brush (ohne Brush ueberspringt q3map2 die Welt; karte::teilKarte nimmt eine Kopie des ersten neuen Brushes)"""
    return '// entity 0\n{\n"classname" "worldspawn"\n"message" "bspmerge Teil"\n' + inhalt + '}\n'


def ent(schluessel, brushes='', marke='_bhed_neu'):
    """eine neue Entity mit der Marke, wie karte::teilKarte sie schreibt (nur markierte kommen ins Entity-Lump)"""
    return '{\n' + ''.join('"%s" "%s"\n' % kv for kv in schluessel + [(marke, '1')]) + brushes + '}\n'


def befunde(orig, teil):
    r = subprocess.run([PRUEF, 'befunde', orig, teil], capture_output=True, text=True, errors='replace')
    return r.stdout


shutil.rmtree(ARBEIT, ignore_errors=True)
os.makedirs(os.path.join(ARBEIT, 'base', 'shaders'))   # games.cpp: Jedi Academy liest shaders/, nicht scripts/
open(os.path.join(ARBEIT, 'base', 'shaders', 'bspmerge.shader'), 'w', newline='\n').write(SHADER)

r = subprocess.run([PRUEF, 'einheit'], capture_output=True, text=True, errors='replace')
print(r.stdout.strip())
pruefe(r.returncode == 0, 'Einheit: Ueberschneidung konvexer Brushes, Grenzen je Spiel')

# Original
raum = open(os.path.join(DATEN, 'raum.map'), encoding='latin-1', newline='').read()
orig_map = schreibe('original', raum + ent([('classname', 'target_print'), ('targetname', 'drucker'), ('message', 'bspmerge'), ('origin', '0 0 64')]))
lauf('-meta', orig_map)
lauf('-vis', orig_map)
lauf('-light', '-fast', orig_map)
orig = os.path.join(ARBEIT, 'original.bsp')
pruefe(os.path.exists(orig), 'Original gebaut (-meta, -vis, -light -fast)')

# Teil wie "In fertige Karte einfuegen": nur die neuen Brush-Entities, q3map2 -bsp
P0 = (-150, -50, 64)       # im trigger_multiple
P1 = (480, -210, 48)       # im trigger_once (reicht durch die Wand bei x 512..528)
URSPRUNG = (0, 132, 24)    # Mitte des origin-Brushes
P2 = (0, 132, 40)          # im func_usable (Weltkoordinaten)
teil_map = schreibe('teil', welt() +
                    ent([('classname', 'trigger_multiple'), ('target', 'drucker'), ('wait', '1')], quader(-200, -100, 0, -100, 0, 128, 'bm/trigger')) +
                    ent([('classname', 'trigger_once'), ('target', 'fehlt')], quader(450, -240, 0, 540, -180, 96, 'bm/trigger')) +
                    ent([('classname', 'func_usable'), ('targetname', 'schalter')],
                        quader(-32, 100, -8, 32, 164, 56, 'bm/nodraw_solid') + quader(-8, 124, 16, 8, 140, 32, 'bm/origin')))
aus = lauf('-bsp', teil_map)
teil = os.path.join(ARBEIT, 'teil.bsp')
pruefe(os.path.exists(teil), 'Teil gebaut (worldspawn mit Platzhalter, Lecks egal)' + ('' if os.path.exists(teil) else ': ' + aus[-400:]))

if os.path.exists(orig) and os.path.exists(teil):
    args = [PRUEF, 'pruefen', orig, teil, os.path.join(ARBEIT, 'gemischt.bsp')] + [str(v) for v in P0 + P1 + P2 + URSPRUNG]
    r = subprocess.run(args, capture_output=True, text=True, errors='replace')
    print(r.stdout.strip())
    pruefe(r.returncode == 0, 'Original + Teil: Welt Byte-gleich, Indizes, Blaetter, CM-Port, Punkt-im-Brush, Meldungen, Grenzen')

    # Gegenproben
    lauf('-bsp', schreibe('sichtbar', welt() + ent([('classname', 'func_door')], quader(-64, -64, 0, 0, 0, 96, 'test/tuer'))))
    z = befunde(orig, os.path.join(ARBEIT, 'sichtbar.bsp'))
    pruefe('befund fehler sichtbar' in z and 'ergebnis nein' in z, 'sichtbarer func_door im Teil: Fehler "sichtbar" (Stufe 3), nichts eingefuegt')
    os.makedirs(os.path.join(ARBEIT, 'q3'), exist_ok=True)
    m = os.path.join(ARBEIT, 'q3', 'ibsp.map')
    open(m, 'w', encoding='latin-1', newline='').write(open(teil_map, encoding='latin-1', newline='').read())
    lauf('-bsp', m, spiel='quake3')
    z = befunde(orig, os.path.join(ARBEIT, 'q3', 'ibsp.bsp'))
    pruefe('befund fehler format Different BSP formats: RBSP 1 and IBSP 46' in z, 'Teil als IBSP 46 (quake3) zu RBSP 1: Fehler "format"')

    # --- Stufe 2: Clip in die Welt ---------------------------------------------------------------
    CLIP = (-4, -64, 0, 4, 64, 128)   # die Oeffnung der Mittelwand (x -8..8, y -64..64, z 0..128)
    lauf('-bsp', schreibe('teilclip', welt() + ent([('classname', 'func_static')], quader(*CLIP, 'bm/clip'), marke='_bhed_welt')))
    clip = os.path.join(ARBEIT, 'teilclip.bsp')
    pruefe(os.path.exists(clip), 'Teil mit Clip gebaut')
    if os.path.exists(clip):
        r = subprocess.run([PRUEF, 'welt', orig, clip, os.path.join(ARBEIT, 'gemischt_clip.bsp')] + [str(v) for v in CLIP], capture_output=True, text=True,
                           errors='replace')
        print(r.stdout.strip())
        pruefe(r.returncode == 0, 'Stufe 2: Clip in der Welt (Blaetter ueber den Baum, CM-Port Punkt und Strahl, Welt sonst gleich, Lumps Byte-gleich)')
    lauf('-bsp', schreibe('teilfest', welt() + ent([('classname', 'func_static')], quader(*CLIP, 'bm/nodraw_solid'), marke='_bhed_welt')))
    z = befunde(orig, os.path.join(ARBEIT, 'teilfest.bsp'))
    pruefe('befund fehler welt' in z and 'ergebnis nein' in z, 'solider nodraw-Brush fuer die Welt: Fehler "welt" (aendert Baum und VIS), nichts eingefuegt')

    # --- Stufe 3: sichtbarer func_door mit Licht --------------------------------------------------
    weltText = raum[:raum.index('// entity 1')]
    lichter = raum[raum.index('// entity 2'):raum.index('// entity 4')]
    tuer = ent([('classname', 'func_door'), ('angle', '90'), ('targetname', 'neutuer')], quader(-190, -220, 0, -174, -156, 128, 'test/tuer'))
    lmap = schreibe('teillicht', weltText + lichter + tuer)
    r = subprocess.run([PRUEF, 'befehle', Q3, ARBEIT, orig, lmap], capture_output=True, text=True, errors='replace')
    befehle = [z[len('befehl '):] for z in r.stdout.splitlines() if z.startswith('befehl ')]
    print('\n'.join(z for z in r.stdout.splitlines() if z.startswith('cmdline')))
    pruefe(len(befehle) == 2 and ' -bsp -meta ' in befehle[0] and ' -light -fast ' in befehle[1],
           'Befehle des Licht-Teils aus _q3map2_cmdline: ' + ' | '.join(b[b.index(' -bsp') if ' -bsp' in b else b.index(' -light'):] for b in befehle))
    for b in befehle:
        subprocess.run(b, capture_output=True, text=True, errors='replace', creationflags=0x08000000 if os.name == 'nt' else 0)
    tl = os.path.join(ARBEIT, 'teillicht.bsp')
    pruefe(os.path.exists(tl), 'Licht-Teil gebaut und beleuchtet')
    if os.path.exists(tl):
        r = subprocess.run([PRUEF, 'licht', orig, tl, os.path.join(ARBEIT, 'gemischt_licht.bsp')], capture_output=True, text=True, errors='replace')
        print(r.stdout.strip())
        pruefe(r.returncode == 0, 'Stufe 3: neues *N mit Flaechen, Indizes umgerechnet, Lightmaps im Bereich, Welt-Lightmaps Byte-gleich, Grenzen')

print('bspmerge: %d Fehlschlaege' % fehler)
sys.exit(1 if fehler else 0)
