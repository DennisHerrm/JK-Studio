# -*- coding: utf-8 -*-
"""lcfix: der Abdruck des Licht-Caches (-light -bhedlightcache) kennt jede Eingabe, die das Licht aendert.

  python q3map2_lichtabdruck.py <bhed_q3map2.exe> <arbeitsordner>

Je Fall: erster Bau mit Cache, EINE Eingabe aendern, Teilbau mit Cache gegen den vollen Bau der geaenderten Karte ohne
Cache - die .bsp (bis auf die Uhrzeit im Kopf) und jede externe Lightmap Byte fuer Byte. Dazu je Fall die Gegenprobe, dass
die Aenderung ueberhaupt wirkt (der volle Bau unterscheidet sich vom ersten Bau in den genannten Lumps: 10 = Vertices mit
dem Vertexlicht, 15 = Lichtgitter) - sonst bewiese der Vergleich nichts.

Die Karte: ein geschlossener Raum mit einem Punktlicht, davor ein Schattenwerfer, dahinter eine Platte als eingebackenes
misc_model (ohne Lightmap, also vertexbeleuchtet) und die Punkte des Lichtgitters.

Faelle (BHED_LICHTABDRUCK_NUR=<name>[,<name>...] waehlt aus):
  Fund P4-1 der Codepruefung (Vertexlicht und Lichtgitter kamen aus dem Cache, obwohl sich ihr Schattenwerfer aenderte):
    alpha         der Alphakanal der Textur eines alphashadow-Shaders (gleiche Bildgroesse)
    modell        eine Punkt-Entity mit "model" und "_cs" verschoben
    modelldatei   die Modelldatei einer solchen Entity geaendert
    subdivisions  -subdivisions 32 gegen 8 mit -patchshadows (der Schalter steht vor -light, nicht in dessen Schaltern)
    q3mapshader   ein Shader "name:q3map" ohne Stufen (q3map2 merkt sich dessen Skripttext nicht): q3map_shadeAngle
  Gegenproben, dass eine Eingabe im Abdruck steckt:
    shaderskript  q3map_surfacelight eines Shaders im Skript geaendert
    lichtbild     die Farbe der Textur eines Flaechenlichts
    sonne         q3map_sun des Himmels-Shaders
    faeden        nichts geaendert, nur -threads 1 gegen 8: alles kommt aus dem Cache, die Bytes sind gleich
  Fund P4-2 (der Hash verlor bei Laenge 7 mod 8 die unteren drei Bits des letzten Bytes):
    floodlight    worldspawn "_floodlight" "255 99 99 256 1" gegen "... 7" (15 Zeichen)
  Fund P4-3 (die Fassung des Compilers stand in keinem Schluessel):
    fassung       derselbe Bau mit anderer Fassung (BHED_Q3_FASSUNG): der Cache wird mit Meldung verworfen
    altformat     ein Cache mit der Kennung des alten Formats: mit Meldung verworfen, kein Absturz
Nur die Standardbibliothek.
"""
import os
import re
import shutil
import struct
import subprocess
import sys

Q3 = sys.argv[1]
ARBEIT = sys.argv[2] if len(sys.argv) > 2 else os.path.join(os.environ.get('TEMP', '.'), 'bhed_q3map2_lichtabdruck')
NUR = [n for n in os.environ.get('BHED_LICHTABDRUCK_NUR', '').split(',') if n]
LICHT = ['-fast', '-threads', '1']
NAME = 'abdruck'
fehler = 0


def pruefe(ok, was):
    global fehler
    print(('OK    ' if ok else 'FEHL  ') + was)
    sys.stdout.flush()
    if not ok:
        fehler += 1


def lauf(spiel, *args, env=None):
    e = dict(os.environ)
    e.pop('BHED_Q3_FASSUNG', None)
    e.update(env or {})
    r = subprocess.run([Q3, '-game', 'ja', '-fs_basepath', spiel, '-fs_homepath', ''] + list(args), capture_output=True, text=True,
                       errors='replace', creationflags=0x40 if os.name == 'nt' else 0, env=e)
    return r.stdout + r.stderr + '\n(Rueckgabe %d)\n' % r.returncode


def quader(x0, y0, z0, x1, y1, z1, tex, detail=False):
    flags = ' 134217728 0 0' if detail else ' 0 0 0'
    e = [((x1, y1, z1), (x1, y0, z1), (x0, y1, z1)), ((x1, y1, z1), (x0, y1, z1), (x1, y1, z0)),
         ((x1, y1, z1), (x1, y1, z0), (x1, y0, z1)), ((x0, y0, z0), (x1, y0, z0), (x0, y1, z0)),
         ((x0, y0, z0), (x0, y0, z1), (x1, y0, z0)), ((x0, y0, z0), (x0, y1, z0), (x0, y0, z1))]
    return '{\n' + ''.join('( %g %g %g ) ( %g %g %g ) ( %g %g %g ) %s 0 0 0 0.5 0.5%s\n' % (a + b + c + (tex, flags)) for a, b, c in e) + '}\n'


def ase(name, shader, ecken, dreiecke):
    """Ein ASE-Modell (wie tests/karte/q3map2/marken.ase): ecken = [(x, y, z)], dreiecke = [(a, b, c, glaettungsgruppe)]."""
    t = ['*3DSMAX_ASCIIEXPORT\t200', '*MATERIAL_LIST {', '\t*MATERIAL_COUNT 1', '\t*MATERIAL 0 {', '\t\t*MATERIAL_NAME "%s"' % shader,
         '\t\t*MAP_DIFFUSE {', '\t\t\t*BITMAP "%s.tga"' % shader, '\t\t}', '\t}', '}', '*GEOMOBJECT {', '\t*NODE_NAME "%s"' % name,
         '\t*NODE_TM {', '\t\t*NODE_NAME "%s"' % name, '\t\t*TM_ROW0 1.0000\t0.0000\t0.0000', '\t\t*TM_ROW1 0.0000\t1.0000\t0.0000',
         '\t\t*TM_ROW2 0.0000\t0.0000\t1.0000', '\t\t*TM_ROW3 0.0000\t0.0000\t0.0000', '\t}', '\t*MESH {', '\t\t*TIMEVALUE 0',
         '\t\t*MESH_NUMVERTEX %d' % len(ecken), '\t\t*MESH_NUMFACES %d' % len(dreiecke), '\t\t*MESH_VERTEX_LIST {']
    t += ['\t\t\t*MESH_VERTEX %d\t%.4f\t%.4f\t%.4f' % ((i,) + tuple(e)) for i, e in enumerate(ecken)]
    t += ['\t\t}', '\t\t*MESH_FACE_LIST {']
    t += ['\t\t\t*MESH_FACE %d: A: %d B: %d C: %d AB: 1 BC: 1 CA: 1 *MESH_SMOOTHING %d *MESH_MTLID 0' % (i, a, b, c, g)
          for i, (a, b, c, g) in enumerate(dreiecke)]
    t += ['\t\t}', '\t}', '\t*MATERIAL_REF 0', '}', '']
    return '\n'.join(t)


def ase_platte(n=8, x0=0.0, x1=224.0, y0=-112.0, y1=112.0, z=lambda x: 0.0, gruppe=lambda i: 1, getrennt=False):
    """Eine Platte aus n x n Feldern, Normale nach oben; z(x) hebt sie an, getrennt = jede Spalte mit eigenen Ecken."""
    ecken, dreiecke = [], []
    if not getrennt:
        for i in range(n + 1):
            for j in range(n + 1):
                x = x0 + (x1 - x0) * i / n
                ecken.append((x, y0 + (y1 - y0) * j / n, z(x)))
        for i in range(n):
            for j in range(n):
                a, b, c, d = i * (n + 1) + j, (i + 1) * (n + 1) + j, (i + 1) * (n + 1) + j + 1, i * (n + 1) + j + 1
                dreiecke += [(a, b, c, gruppe(i)), (a, c, d, gruppe(i))]
    else:
        for i in range(n):
            for j in range(n):
                k = len(ecken)
                for di, dj in ((0, 0), (1, 0), (1, 1), (0, 1)):
                    x = x0 + (x1 - x0) * (i + di) / n
                    ecken.append((x, y0 + (y1 - y0) * (j + dj) / n, z(x)))
                dreiecke += [(k, k + 1, k + 2, gruppe(i)), (k, k + 2, k + 3, gruppe(i))]
    return ase('platte', 'textures/test/modell', ecken, dreiecke)


def ase_kasten(x0, y0, z0, x1, y1, z1):
    e = [(x0, y0, z0), (x1, y0, z0), (x1, y1, z0), (x0, y1, z0), (x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1)]
    d = [(0, 2, 1), (0, 3, 2), (4, 5, 6), (4, 6, 7), (0, 1, 5), (0, 5, 4), (1, 2, 6), (1, 6, 5), (2, 3, 7), (2, 7, 6), (3, 0, 4), (3, 4, 7)]
    return ase('kasten', 'textures/test/modell', e, [t + (1,) for t in d])


def tga(pfad, farbe, breite=16, hoehe=16):
    """32 Bit, unkomprimiert, einfarbig: farbe = (r, g, b, a)."""
    r, g, b, a = farbe
    kopf = struct.pack('<BBBHHBHHHHBB', 0, 0, 2, 0, 0, 0, 0, 0, breite, hoehe, 32, 8)
    os.makedirs(os.path.dirname(pfad), exist_ok=True)
    with open(pfad, 'wb') as f:
        f.write(kopf + bytes((b, g, r, a)) * (breite * hoehe))


def schreibe(pfad, text):
    os.makedirs(os.path.dirname(pfad), exist_ok=True)
    with open(pfad, 'w', encoding='latin-1', newline='\n') as f:
        f.write(text)


LICHT_ORT = (-192, 0, 250)
WERFER = (-64, -128, 0, -48, 128, 240)     # zwischen Licht und Platte: die Platte und Gitterpunkte dahinter liegen im Schatten
PLATTE = '{\n"classname" "misc_model"\n"model" "models/test/platte.ase"\n"origin" "0 0 2"\n}\n'


def raum(welt='', ents='', schluessel='', decke='test/wand', licht=True):
    """Innen 512 x 512 x 384. Das Punktlicht steht links oben, die Platte liegt rechts am Boden."""
    w = quader(-272, -272, -16, 272, 272, 0, 'test/boden')
    w += quader(-272, -272, 384, 272, 272, 400, decke)
    w += quader(-272, -272, 0, -256, 272, 384, 'test/wand') + quader(256, -272, 0, 272, 272, 384, 'test/wand')
    w += quader(-256, -272, 0, 256, -256, 384, 'test/wand') + quader(-256, 256, 0, 256, 272, 384, 'test/wand')
    return ('{\n"classname" "worldspawn"\n' + schluessel + w + welt + '}\n'
            '{\n"classname" "info_player_start"\n"origin" "-192 -192 24"\n}\n' +
            ('{\n"classname" "light"\n"origin" "%d %d %d"\n"light" "300"\n}\n' % LICHT_ORT if licht else '') + PLATTE + ents)


def grunddaten(spiel):
    schreibe(os.path.join(spiel, 'base', 'models', 'test', 'platte.ase'), ase_platte())


def bauen(ordner, spiel, text, schalter, cache, env=None, vorlauf=()):
    k = os.path.join(ordner, NAME + '.map')
    schreibe(k, text)
    t = lauf(spiel, '-meta', k)
    t += lauf(spiel, '-vis', '-threads', '1', k)
    return t + lauf(spiel, *vorlauf, '-light', *schalter, *(['-bhedlightcache'] if cache else []), k, env=env)


def lumps(p):
    d = open(p, 'rb').read()
    return [d[o:o + n] for o, n in (struct.unpack_from('<ii', d, 8 + 8 * i) for i in range(18))]


def dateien(ordner):
    bsp = open(os.path.join(ordner, NAME + '.bsp'), 'rb').read()
    d = {NAME + '.bsp': re.sub(rb'(I LOVE MY Q3MAP2 [^\n]*? on )[^\n]*\n', rb'\1(Uhrzeit)\n', bsp, count=1)}
    lm = os.path.join(ordner, NAME)
    if os.path.isdir(lm):
        for f in sorted(os.listdir(lm)):
            d[NAME + '/' + f] = open(os.path.join(lm, f), 'rb').read()
    return d


def cachezeilen(ausgabe):
    z = [x.split('BehavEd: ', 1)[1] for x in ausgabe.splitlines() if 'BehavEd: light cache' in x]
    kopf = next((x for x in z if x.startswith('light cache found') or x.startswith('light cache not usable')), '(keine Zeile)')
    summe = next((x for x in z if x.startswith('light cache: ')), '(keine Summe)')
    return kopf, summe


def fall(kurz, was, alt, neu, wirksam=(10,), s_alt=None, s_neu=None, davor=None, danach=None, env_neu=None, vor_alt=(), vor_neu=(),
         cache_aendern=None, erwartet=None):
    """alt/neu: Kartentext; davor/danach(spiel): Spieldaten vor dem ersten Bau bzw. vor Teilbau und vollem Bau;
    wirksam: Lumps, in denen sich der volle Bau der geaenderten Karte vom ersten Bau unterscheiden muss;
    erwartet(ausgabe des Teilbaus) -> (ok, text): eine zusaetzliche Pruefung der Meldungen."""
    if NUR and kurz not in NUR:
        return
    s_alt = s_alt or LICHT
    s_neu = s_neu or s_alt
    spiel = os.path.join(ARBEIT, kurz + '_spiel')
    erst, teil, voll = (os.path.join(ARBEIT, kurz + e) for e in ('_erst', '_teil', '_voll'))
    for o in (spiel, erst, teil, voll):
        shutil.rmtree(o, ignore_errors=True)
    os.makedirs(os.path.join(spiel, 'base'))
    os.makedirs(erst)
    os.makedirs(voll)
    grunddaten(spiel)
    if davor:
        davor(spiel)
    a0 = bauen(erst, spiel, alt, s_alt, True, vorlauf=vor_alt)
    cache = os.path.exists(os.path.join(erst, NAME + '.bhedlight'))
    shutil.copytree(erst, teil)
    if cache_aendern:
        cache_aendern(os.path.join(teil, NAME + '.bhedlight'))
    if danach:
        danach(spiel)
    a1 = bauen(teil, spiel, neu, s_neu, True, env=env_neu, vorlauf=vor_neu)
    a2 = bauen(voll, spiel, neu, s_neu, False, env=env_neu, vorlauf=vor_neu)
    for nr, a in enumerate((a0, a1, a2)):
        with open(os.path.join(ARBEIT, kurz + '_ausgabe%d.txt' % nr), 'w', encoding='utf-8', errors='replace') as f:
            f.write(a)
    ok_lauf = all(os.path.exists(os.path.join(o, NAME + '.bsp')) for o in (erst, teil, voll)) and a1.count('(Rueckgabe 0)') == 3
    if not ok_lauf:
        pruefe(False, '%s (%s): ein Lauf ist gescheitert (Ausgaben in %s)' % (kurz, was, ARBEIT))
        return
    l0, l2 = lumps(os.path.join(erst, NAME + '.bsp')), lumps(os.path.join(voll, NAME + '.bsp'))
    wirkt = [i for i in range(18) if l0[i] != l2[i] and i != 0]
    dt, dv = dateien(teil), dateien(voll)
    anders = sorted(k for k in set(dt) | set(dv) if dt.get(k) != dv.get(k))
    if NAME + '.bsp' in anders:
        lt = lumps(os.path.join(teil, NAME + '.bsp'))
        anders.append('Lumps %s' % [i for i in range(18) if lt[i] != l2[i]])
    kopf, summe = cachezeilen(a1)
    print('      %s: %s' % (kurz, kopf))
    print('      %s: %s' % (kurz, summe))
    pruefe(cache and all(i in wirkt for i in wirksam),
           '%s (%s): die Aenderung wirkt - der volle Bau weicht vom ersten ab in den Lumps %s (verlangt %s)' % (kurz, was, wirkt, list(wirksam)))
    pruefe(not anders, '%s (%s): Teilbau mit Cache gleich dem vollen Bau, %d Dateien (andere: %s)' % (kurz, was, len(dv), anders))
    if erwartet:
        ok, text = erwartet(a1)
        pruefe(ok, '%s (%s): %s' % (kurz, was, text))


def aus_cache(ausgabe):
    m = re.search(r'light cache: (\d+) of (\d+) lightmaps and (\d+) of (\d+) vertex lit surfaces taken from the cache, grid ([a-z ]+);', ausgabe)
    return (int(m.group(1)), int(m.group(2)), int(m.group(3)), int(m.group(4)), m.group(5)) if m else None


# --- Fund P4-1 -------------------------------------------------------------------------------------------------------
GITTER_SHADER = ('textures/test/gitter\n{\n\tsurfaceparm alphashadow\n\tsurfaceparm trans\n\tcull none\n\t{\n\t\tmap textures/test/gitter.tga\n'
                 '\t\talphaFunc GE128\n\t}\n}\n')


def alpha_davor(spiel):
    schreibe(os.path.join(spiel, 'base', 'shaders', 'test_gitter.shader'), GITTER_SHADER)
    tga(os.path.join(spiel, 'base', 'textures', 'test', 'gitter.tga'), (200, 200, 200, 255))


karte = raum(welt=quader(*WERFER, 'test/gitter', True))
fall('alpha', 'Alphakanal der alphashadow-Textur 255 -> 0', karte, karte, wirksam=(10, 15), davor=alpha_davor,
     danach=lambda spiel: tga(os.path.join(spiel, 'base', 'textures', 'test', 'gitter.tga'), (200, 200, 200, 0)))


def werfer_ent(x, y):
    return '{\n"classname" "misc_model_static"\n"model" "models/test/kasten.ase"\n"origin" "%d %d 0"\n"_cs" "1"\n}\n' % (x, y)


def kasten_davor(spiel):
    schreibe(os.path.join(spiel, 'base', 'models', 'test', 'kasten.ase'), ase_kasten(-8, -128, 0, 8, 128, 240))


fall('modell', 'Punkt-Entity mit model und _cs verschoben', raum(ents=werfer_ent(-56, 0)), raum(ents=werfer_ent(-240, 200)), davor=kasten_davor)
fall('modelldatei', 'Modelldatei der Punkt-Entity mit _cs geaendert (240 hoch -> 16 hoch)', raum(ents=werfer_ent(-56, 0)), raum(ents=werfer_ent(-56, 0)),
     davor=kasten_davor,
     danach=lambda spiel: schreibe(os.path.join(spiel, 'base', 'models', 'test', 'kasten.ase'), ase_kasten(-8, -128, 0, 8, 128, 16)))

# ein ebener Patch als Wand zwischen Licht und Platte, dessen Oberkante ein Bogen ist: ohne Unterteilung (-subdivisions 32)
# ist die Kante ein Dach ueber drei Punkte, mit Unterteilung (8) der Bogen - der Streifen dazwischen wirft nur dann Schatten.
# Die Oberkante ist die erste Zeile des Patches: q3map2 misst die laengste Kurve eines 3x3-Patches nur an seiner ersten
# Zeile und Spalte (ParsePatch), und nur aus ihr folgt die Zahl der Unterteilungen.
BOGEN = ('{\npatchDef2\n{\ntest/boden\n( 3 3 0 0 0 )\n(\n' +
         '\n'.join('( ' + ' '.join('( -56 %d %g %g %g )' % (y, z * h, (y + 96) / 192, z)
                                   for z in (1, 0.5, 0)) + ' )' for y, h in ((-96, 40), (0, 280), (96, 40))) + '\n)\n}\n}\n')
karte = raum(welt=BOGEN)
fall('subdivisions', '-subdivisions 32 -> 8 mit -patchshadows', karte, karte, s_alt=LICHT + ['-patchshadows'],
     vor_alt=('-subdivisions', '32'), vor_neu=('-subdivisions', '8'))

# ein Dach aus zwei ebenen Patches, die sich am First treffen; ihr Shader ist ein "name:q3map" ohne Stufen (q3map2 merkt
# sich von dem keinen Skripttext) mit "surfaceparm pointlight" (vertexbeleuchtet). Mit -shade glaettet SmoothNormals die
# Normalen am First nur, wenn der Winkel zwischen den Seiten (rund 84 Grad) unter q3map_shadeAngle liegt. Modelle taugen
# dafuer nicht: SmoothNormals uebergeht Dreieckssuppen.
def dachpatch(xa, za, xb, zb):
    zeilen = ['( ' + ' '.join('( %g %g %g %g %g )' % (xa + (xb - xa) * t, y, za + (zb - za) * t, t, (y + 112) / 224.0)
                              for t in (0, 0.5, 1)) + ' )' for y in (-112, 0, 112)]
    return '{\npatchDef2\n{\ntest/dach\n( 3 3 0 0 0 )\n(\n' + '\n'.join(zeilen) + '\n)\n}\n}\n'


def dach_shader(winkel):
    return 'textures/test/dach:q3map\n{\n\tsurfaceparm pointlight\n\tcull none\n\tq3map_shadeAngle %d\n}\n' % winkel


karte = raum(welt=dachpatch(0, 2, 112, 102) + dachpatch(112, 102, 224, 2))
fall('q3mapshader', 'Shader "name:q3map" ohne Stufen: q3map_shadeAngle 179 -> 20 mit -shade', karte, karte, s_alt=LICHT + ['-shade'],
     davor=lambda spiel: schreibe(os.path.join(spiel, 'base', 'shaders', 'test_dach.shader'), dach_shader(179)),
     danach=lambda spiel: schreibe(os.path.join(spiel, 'base', 'shaders', 'test_dach.shader'), dach_shader(20)))

# --- Gegenproben ------------------------------------------------------------------------------------------------------
LAMPE = quader(-16, -16, 352, 16, 16, 384, 'test/lampe', True)


def lampe_shader(wert):
    return 'textures/test/lampe\n{\n\tq3map_surfacelight %d\n\t{\n\t\tmap textures/test/lampe.tga\n\t}\n}\n' % wert


def lampe_davor(spiel):
    schreibe(os.path.join(spiel, 'base', 'shaders', 'test_lampe.shader'), lampe_shader(4000))
    tga(os.path.join(spiel, 'base', 'textures', 'test', 'lampe.tga'), (255, 60, 40, 255))


karte = raum(welt=LAMPE, licht=False)
fall('shaderskript', 'q3map_surfacelight 4000 -> 12000 im Shader-Skript', karte, karte, wirksam=(10, 15), davor=lampe_davor,
     danach=lambda spiel: schreibe(os.path.join(spiel, 'base', 'shaders', 'test_lampe.shader'), lampe_shader(12000)))
fall('lichtbild', 'Farbe der Textur eines Flaechenlichts rot -> blau', karte, karte, wirksam=(10, 15), davor=lampe_davor,
     danach=lambda spiel: tga(os.path.join(spiel, 'base', 'textures', 'test', 'lampe.tga'), (40, 60, 255, 255)))


def himmel(sonne):
    return 'textures/test/himmel\n{\n\tsurfaceparm sky\n\tsurfaceparm noimpact\n\tsurfaceparm nolightmap\n\tq3map_sun %s\n}\n' % sonne


karte = raum(decke='test/himmel', licht=False)
fall('sonne', 'q3map_sun des Himmels-Shaders (Richtung und Hoehe)', karte, karte, wirksam=(10, 15),
     davor=lambda spiel: schreibe(os.path.join(spiel, 'base', 'shaders', 'test_himmel.shader'), himmel('1 0.95 0.85 180 60 70')),
     danach=lambda spiel: schreibe(os.path.join(spiel, 'base', 'shaders', 'test_himmel.shader'), himmel('1 0.95 0.85 180 200 50')))


def alles_aus_cache(ausgabe):
    z = aus_cache(ausgabe)
    return (z is not None and z[0] == z[1] and z[2] == z[3] and z[3] > 0 and z[4] == 'from the cache',
            'alles kommt aus dem Cache (%s)' % (z,))


karte = raum(welt=quader(*WERFER, 'test/wand', True))
fall('faeden', 'nichts geaendert, -threads 1 -> 8', karte, karte, wirksam=(), s_alt=['-fast', '-threads', '1'], s_neu=['-fast', '-threads', '8'],
     erwartet=alles_aus_cache)


# --- Fund P4-2 -------------------------------------------------------------------------------------------------------
def flut(wert):
    return raum(schluessel='"_floodlight" "%s"\n' % wert)


fall('floodlight', 'worldspawn _floodlight "255 99 99 256 1" -> "255 99 99 256 7" (15 Zeichen, nur die letzte Ziffer)',
     flut('255 99 99 256 1'), flut('255 99 99 256 7'), wirksam=(10, 15))


# --- Fund P4-3 -------------------------------------------------------------------------------------------------------
def verworfen(ausgabe):
    kopf, _ = cachezeilen(ausgabe)
    z = aus_cache(ausgabe)
    return (kopf.startswith('light cache not usable: older cache format') and z is not None and z[0] == 0 and z[2] == 0 and z[4] == 'traced',
            'der Cache wird mit Meldung verworfen und nichts uebernommen ("%s", %s)' % (kopf, z))


def altes_format(pfad):
    with open(pfad, 'r+b') as f:
        f.write(b'BHLICHT5')


karte = raum(welt=quader(*WERFER, 'test/wand', True))
fall('fassung', 'derselbe Bau mit einer anderen Fassung des Compilers', karte, karte, wirksam=(), env_neu={'BHED_Q3_FASSUNG': 'lcfix-andere-fassung'},
     erwartet=verworfen)
fall('altformat', 'Cache mit der Kennung BHLICHT5 (rc611)', karte, karte, wirksam=(), cache_aendern=altes_format, erwartet=verworfen)

print('q3map2_lichtabdruck: %d Fehlschlaege' % fehler)
sys.exit(1 if fehler else 0)
