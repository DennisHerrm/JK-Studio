"""Testkarten fuer den ctest q3map2_vergleich (Stufe 3 Punkt 2): zwei Raeume mit
Durchgang, Licht, Detail-Saeule, Tuer (Brush-Modell) und Patch. raum.map ist
dicht, raum_leck.map hat ein Loch in der Ostwand (Punktdatei, Punkt 5),
raum_einfach.map ist raum.map mit nur einem Shader.
Aufruf: python gen_q3map2_testkarten.py tests/karte/q3map2
"""
import os
import sys


def kasten(x0, y0, z0, x1, y1, z1, tex, inhalt=0):
    t = '%s 0 0 0 0.5 0.5 %d 0 0' % (tex, inhalt)
    z = []
    z.append('( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s' % (x1, y1, z1, x1, y0, z1, x0, y1, z1, t))
    z.append('( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s' % (x1, y1, z1, x0, y1, z1, x1, y1, z0, t))
    z.append('( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s' % (x1, y1, z1, x1, y1, z0, x1, y0, z1, t))
    z.append('( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s' % (x0, y0, z0, x1, y0, z0, x0, y1, z0, t))
    z.append('( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s' % (x0, y0, z0, x0, y0, z1, x1, y0, z0, t))
    z.append('( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s' % (x0, y0, z0, x0, y1, z0, x0, y0, z1, t))
    return z


def karte(leck, ein_shader=False):
    zeilen = ['// entity 0', '{', '"classname" "worldspawn"', '"message" "BehavEd q3map2 test"', '"_color" "1 0.95 0.9"',
              '"ambient" "8"']
    pinsel = []
    pinsel.append(kasten(-528, -272, -16, 528, 272, 0, 'test/boden'))         # Boden
    pinsel.append(kasten(-528, -272, 256, 528, 272, 272, 'test/decke'))       # Decke
    pinsel.append(kasten(-528, 256, 0, 528, 272, 256, 'test/wand'))           # Nordwand
    pinsel.append(kasten(-528, -272, 0, 528, -256, 256, 'test/wand'))         # Suedwand
    pinsel.append(kasten(-528, -256, 0, -512, 256, 256, 'test/wand'))         # Westwand
    if leck:
        # Ostwand mit Loch (y -32..32, z 64..128)
        pinsel.append(kasten(512, -256, 0, 528, -32, 256, 'test/wand'))
        pinsel.append(kasten(512, 32, 0, 528, 256, 256, 'test/wand'))
        pinsel.append(kasten(512, -32, 0, 528, 32, 64, 'test/wand'))
        pinsel.append(kasten(512, -32, 128, 528, 32, 256, 'test/wand'))
    else:
        pinsel.append(kasten(512, -256, 0, 528, 256, 256, 'test/wand'))       # Ostwand
    # Mittelwand mit Durchgang (y -64..64, z 0..128)
    pinsel.append(kasten(-8, -256, 0, 8, -64, 256, 'test/mitte'))
    pinsel.append(kasten(-8, 64, 0, 8, 256, 256, 'test/mitte'))
    pinsel.append(kasten(-8, -64, 128, 8, 64, 256, 'test/mitte'))
    # Saeule als Detail in Raum A (CONTENTS_DETAIL = 0x8000000)
    pinsel.append(kasten(-288, -32, 0, -224, 32, 256, 'test/saeule', 134217728))
    for i, p in enumerate(pinsel):
        zeilen.append('// brush %d' % i)
        zeilen.append('{')
        zeilen.extend(p)
        zeilen.append('}')
    # Patch: Bogen ueber dem Boden von Raum B
    zeilen += ['// brush %d' % len(pinsel), '{', 'patchDef2', '{', 'test/patch', '( 3 3 0 0 0 )', '(',
               '( ( 128 -192 0 0 0 ) ( 128 -128 96 0 0.5 ) ( 128 -64 0 0 1 ) )',
               '( ( 256 -192 0 0.5 0 ) ( 256 -128 96 0.5 0.5 ) ( 256 -64 0 0.5 1 ) )',
               '( ( 384 -192 0 1 0 ) ( 384 -128 96 1 0.5 ) ( 384 -64 0 1 1 ) )',
               ')', '}', '}', '}']
    zeilen += ['// entity 1', '{', '"classname" "info_player_start"', '"origin" "-384 128 24"', '"angle" "0"', '}']
    zeilen += ['// entity 2', '{', '"classname" "light"', '"origin" "-256 -160 192"', '"light" "400"', '}']
    zeilen += ['// entity 3', '{', '"classname" "light"', '"origin" "256 160 192"', '"light" "300"',
               '"_color" "0.6 0.8 1"', '}']
    zeilen += ['// entity 4', '{', '"classname" "func_door"', '"angle" "-1"', '"targetname" "tuer"', '// brush 0', '{']
    zeilen += kasten(384, 128, 0, 448, 192, 96, 'test/tuer')
    zeilen += ['}', '}']
    text = '\n'.join(zeilen) + '\n'
    if ein_shader:
        # alle Flaechen mit demselben Shader: kein Vergleich von shaderInfo_t-Zeigern
        # entscheidet mehr etwas (q3map2_vergleich.py, PFLICHT_KARTE)
        for alt in ('test/boden', 'test/decke', 'test/mitte', 'test/saeule', 'test/patch', 'test/tuer'):
            text = text.replace(alt, 'test/wand')
    return text


ziel = sys.argv[1]
os.makedirs(ziel, exist_ok=True)
for name, leck, ein in (('raum.map', False, False), ('raum_leck.map', True, False), ('raum_einfach.map', False, True)):
    with open(os.path.join(ziel, name), 'w', encoding='ascii', newline='\n') as f:
        f.write(karte(leck, ein))
print('ok')
