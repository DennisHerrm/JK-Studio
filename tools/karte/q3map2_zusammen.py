# -*- coding: utf-8 -*-
"""Stufe 8a Punkt 5c: "Zur fertigen Karte hinzufuegen" - -mergebsp -world -fixnames.

  python q3map2_zusammen.py <bhed_q3map2.exe> <kartezusammen.exe> <tests/karte/q3map2> [<arbeitsordner>] [<original.bsp> <x y z>]

1. raum_licht.bsp + eine Saeule: zusammengefuehrt, die .bsp laedt im CM-Port, die
   Saeule stoppt den Spieler, keine Fehler.
2. Hauptkarte mit einem Inside-, Teil mit einem Outside-Wetterbrush: Fehler "wetter".
3. misc_weather_zone in der Hauptkarte, Inside-Brush des Teils ausserhalb: Warnung "zone".
4. gleicher targetname: Hinweis "namen".
Mit <original.bsp> und einem freien Punkt: eine Saeule dort in eine echte Karte.
"""
import os
import shutil
import subprocess
import sys

Q3, PRUEF, DATEN = sys.argv[1], sys.argv[2], sys.argv[3]
ARBEIT = sys.argv[4] if len(sys.argv) > 4 else os.path.join(os.environ.get('TEMP', '.'), 'bhed_q3map2_zusammen')
fehler = 0


def lauf(*args, spiel=None):
    r = subprocess.run([Q3, '-game', 'ja', '-fs_basepath', spiel or ARBEIT, '-fs_homepath', ''] + list(args), capture_output=True, text=True,
                       errors='replace', creationflags=0x40 if os.name == 'nt' else 0)
    return r.stdout + r.stderr


def pruefen(haupt, teil, merged=None):
    r = subprocess.run([PRUEF, haupt, teil] + ([merged] if merged else []), capture_output=True, text=True, errors='replace')
    return r.stdout


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


def bauen(ordner, name, text, licht=True, spiel=None):
    os.makedirs(ordner, exist_ok=True)
    m = os.path.join(ordner, name + '.map')
    open(m, 'w', encoding='latin-1', newline='').write(text)
    lauf('-meta', m, spiel=spiel)
    if licht:
        lauf('-light', '-fast', m, spiel=spiel)
    return os.path.join(ordner, name + '.bsp')


def welt(inhalt, punkte=''):
    return '// entity 0\n{\n"classname" "worldspawn"\n' + inhalt + '}\n' + punkte


shutil.rmtree(ARBEIT, ignore_errors=True)
os.makedirs(ARBEIT)
raum = open(os.path.join(DATEN, 'raum.map'), encoding='latin-1', newline='').read()

# 1. Saeule in raum_licht.bsp
haupt = os.path.join(ARBEIT, 'f1', 'haupt.bsp')
os.makedirs(os.path.dirname(haupt))
shutil.copy(os.path.join(DATEN, 'raum_licht.bsp'), haupt)
teil = bauen(os.path.join(ARBEIT, 'f1'), 'teil', welt(quader(0, -160, 0, 64, -96, 128, 'test/wand'), '{\n"classname" "light"\n"origin" "32 -40 96"\n"light" "200"\n}\n'))
aus = lauf('-mergebsp', '-world', '-fixnames', haupt, teil)
merged = os.path.join(ARBEIT, 'f1', 'haupt_merged.bsp')
z = pruefen(haupt, teil, merged)
print(z.strip())
pruefe(os.path.exists(merged) and 'befund fehler' not in z, '1. raum_licht + Saeule: zusammengefuehrt, keine Fehler')
k = [l for l in z.splitlines() if l.startswith('kollision ')]
w = k[0].split() if k else []
pruefe(len(w) == 5 and w[1] == '1' and int(w[2]) >= 1 and w[3] == w[2], '1. die .bsp laedt im CM-Port, die Saeule stoppt den Spieler (%s)' % (k[0] if k else '-'))

# 2. Inside in der Hauptkarte, Outside im Teil
mitInside = raum.replace('"classname" "worldspawn"', '"classname" "worldspawn"\n' + quader(-480, -240, 0, -400, -160, 64, 'system/inside'), 1)
haupt2 = bauen(os.path.join(ARBEIT, 'f2'), 'haupt', mitInside, licht=False)
teil2 = bauen(os.path.join(ARBEIT, 'f2'), 'teil', welt(quader(0, -160, 0, 64, -96, 128, 'test/wand') + quader(100, -160, 0, 164, -96, 64, 'system/outside')), licht=False)
z = pruefen(haupt2, teil2)
pruefe('befund fehler wetter' in z, '2. Inside (Hauptkarte) und Outside (Teil) gemischt: Fehler "wetter"')
teil2b = bauen(os.path.join(ARBEIT, 'f2'), 'teilb', welt(quader(0, -160, 0, 64, -96, 128, 'test/wand') + quader(100, -160, 0, 164, -96, 64, 'system/inside')), licht=False)
z = pruefen(haupt2, teil2b)
pruefe('befund fehler wetter' not in z, '2. Inside und Inside: kein Fehler')

# 3. Zone in der Hauptkarte, Inside-Brush des Teils ausserhalb
zone = '{\n"classname" "misc_weather_zone"\n' + quader(-512, -256, 0, 0, 256, 256, 'system/weatherzone') + '}\n'
haupt3 = bauen(os.path.join(ARBEIT, 'f3'), 'haupt', raum + zone, licht=False)
teil3 = bauen(os.path.join(ARBEIT, 'f3'), 'teil', welt(quader(0, -160, 0, 64, -96, 128, 'test/wand') + quader(200, -160, 0, 264, -96, 64, 'system/inside')), licht=False)
z = pruefen(haupt3, teil3)
pruefe('befund warnung zone' in z, '3. misc_weather_zone, Inside-Brush ausserhalb: Warnung "zone"')
teil3b = bauen(os.path.join(ARBEIT, 'f3'), 'teilb', welt(quader(0, -160, 0, 64, -96, 128, 'test/wand') + quader(-300, -160, 0, -236, -96, 64, 'system/inside')), licht=False)
z = pruefen(haupt3, teil3b)
pruefe('befund warnung zone' not in z, '3. Inside-Brush innerhalb der Zone: keine Warnung')

# 4. gleiche Namen
haupt4 = bauen(os.path.join(ARBEIT, 'f4'), 'haupt', raum + '{\n"classname" "info_null"\n"targetname" "tor"\n"origin" "0 0 64"\n}\n', licht=False)
teil4 = bauen(os.path.join(ARBEIT, 'f4'), 'teil', welt(quader(0, -160, 0, 64, -96, 128, 'test/wand'), '{\n"classname" "info_null"\n"targetname" "tor"\n"origin" "32 -40 64"\n}\n'), licht=False)
z = pruefen(haupt4, teil4)
pruefe('befund hinweis namen 1' in z, '4. targetname "tor" in beiden: Hinweis "namen"')

# 5. echte Karte (optional)
if len(sys.argv) > 7:
    orig, x, y, zz = sys.argv[5], float(sys.argv[6]), float(sys.argv[7]), float(sys.argv[8])
    spiel = os.environ.get('BHED_ZUSAMMEN_SPIEL')
    o = os.path.join(ARBEIT, 'f5')
    os.makedirs(o)
    haupt5 = os.path.join(o, 'haupt.bsp')
    shutil.copy(orig, haupt5)
    teil5 = bauen(o, 'teil', welt(quader(x - 32, y - 32, zz, x + 32, y + 32, zz + 96, 'textures/imperial/basic')), spiel=spiel)
    lauf('-mergebsp', '-world', '-fixnames', haupt5, teil5, spiel=spiel)
    z = pruefen(haupt5, teil5, os.path.join(o, 'haupt_merged.bsp'))
    print(z.strip())
    k = [l for l in z.splitlines() if l.startswith('kollision ')]
    w = k[0].split() if k else []
    pruefe(len(w) == 5 and w[1] == '1' and int(w[2]) >= 1 and w[3] == w[2], '5. %s + Saeule bei %g %g %g: laedt, stoppt den Spieler (%s)' % (
        os.path.basename(orig), x, y, zz, k[0] if k else '-'))
print('q3map2_zusammen: %d Fehlschlaege' % fehler)
sys.exit(1 if fehler else 0)
