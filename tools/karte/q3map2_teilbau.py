# -*- coding: utf-8 -*-
"""Stufe 8a Punkt 5: Teil-Kompilierungen gegen die volle Kompilierung, Byte fuer Byte.

  python q3map2_teilbau.py <bhed_q3map2.exe> <tests/karte/q3map2> [<arbeitsordner>] [karte.map ...]

a) VIS wiederverwenden (-bhedvisreuse): je Karte
   1. voller Bau (-meta, -vis -threads 1 -bhedvisreuse, -light -fast) - legt den Cache an
   2. Aenderung an Detail bzw. Entity -> Bau mit -bhedvisreuse im selben Ordner: die
      Sichtbarkeit muss wiederverwendet werden ("visibility reused") und die .bsp gleich
      der eines vollen Baus der geaenderten Karte in einem frischen Ordner sein
   3. Aenderung an der Struktur -> Bau mit -bhedvisreuse: neu gerechnet ("computed"),
      ebenfalls gleich dem vollen Bau
Die Karten aus tests/karte/q3map2 brauchen kein Spiel (-game quake3 nicht, sondern ja
ohne Spielordner: die Shader fehlen, die Geometrie ist trotzdem dieselbe).
"""
import os
import re
import shutil
import subprocess
import sys

Q3, DATEN = sys.argv[1], sys.argv[2]
ARBEIT = sys.argv[3] if len(sys.argv) > 3 else os.path.join(os.environ.get('TEMP', '.'), 'bhed_q3map2_teilbau')
KARTEN = sys.argv[4:] or [os.path.join(DATEN, 'raum.map'), os.path.join(DATEN, 'raum_einfach.map')]
fehler = 0


def lauf(*args):
    r = subprocess.run([Q3, '-game', 'ja', '-fs_basepath', ARBEIT, '-fs_homepath', ''] + list(args), capture_output=True, text=True,
                       errors='replace', creationflags=0x40 if os.name == 'nt' else 0)
    return r.stdout + r.stderr


def bauen(karte, wieder):
    t = ''
    t += lauf('-meta', karte)
    t += lauf('-vis', '-threads', '1', *(['-bhedvisreuse'] if wieder else []), karte)
    t += lauf('-light', '-fast', '-threads', '1', karte)
    return t


def lumps(p):
    import struct
    d = open(p, 'rb').read()
    return [d[o:o + l] for o, l in (struct.unpack_from('<ii', d, 8 + 8 * i) for i in range(18))]


def pruefe(ok, was):
    global fehler
    print(('OK    ' if ok else 'FEHL  ') + was)
    if not ok:
        fehler += 1


def zusatz_pinsel(text, detail, x, y, z):
    """Ein 16er-Wuerfel bei (x, y, z) in die worldspawn (Quake-Format, wie die Testkarten)."""
    flags = ' 134217728 0 0' if detail else ''
    s = 8
    ebenen = [((x + s, y + s, z + s), (x + s, y - s, z + s), (x - s, y + s, z + s)),
              ((x + s, y + s, z + s), (x - s, y + s, z + s), (x + s, y + s, z - s)),
              ((x + s, y + s, z + s), (x + s, y + s, z - s), (x + s, y - s, z + s)),
              ((x - s, y - s, z - s), (x + s, y - s, z - s), (x - s, y + s, z - s)),
              ((x - s, y - s, z - s), (x - s, y - s, z + s), (x + s, y - s, z - s)),
              ((x - s, y - s, z - s), (x - s, y + s, z - s), (x - s, y - s, z + s))]
    b = '{\n' + ''.join('( %g %g %g ) ( %g %g %g ) ( %g %g %g ) test/boden 0 0 0 0.5 0.5%s\n' % (a + c + d + (flags,)) for a, c, d in ebenen) + '}\n'
    # vor die schliessende Klammer der worldspawn (die erste Entitaet)
    tiefe = 0
    for i, ch in enumerate(text):
        if ch == '{':
            tiefe += 1
        elif ch == '}':
            tiefe -= 1
            if tiefe == 0:
                return text[:i] + b + text[i:]
    return text


def start(text):
    m = re.search(r'"classname" "info_player_start"[^}]*?"origin" "([-\d.]+) ([-\d.]+) ([-\d.]+)"', text) or \
        re.search(r'"origin" "([-\d.]+) ([-\d.]+) ([-\d.]+)"[^}]*?"classname" "info_player_start"', text)
    return tuple(float(v) for v in m.groups()) if m else (0.0, 0.0, 64.0)


for quelle in KARTEN:
    name = os.path.splitext(os.path.basename(quelle))[0]
    text = open(quelle, encoding='latin-1', newline='').read()
    sx, sy, sz = start(text)
    faelle = [
        ('Entity (worldspawn-Schluessel)', text.replace('"classname" "worldspawn"', '"classname" "worldspawn"\n"_bhed_teilbau" "1"', 1), True),
        ('Detail-Pinsel', zusatz_pinsel(text, True, sx + 40, sy, sz + 40), True),
        # Struktur: neu gerechnet, wenn sich die Portale aendern (ein Pinsel ganz im Festen aendert sie nicht)
        ('Struktur-Pinsel', zusatz_pinsel(text, False, sx + 40, sy, sz + 40), None),
    ]
    for was, neu, wieder in faelle:
        a = os.path.join(ARBEIT, name + '_a')
        b = os.path.join(ARBEIT, name + '_b')
        for o in (a, b):
            shutil.rmtree(o, ignore_errors=True)
            os.makedirs(o)
        ka = os.path.join(a, name + '.map')
        kb = os.path.join(b, name + '.map')
        open(ka, 'w', encoding='latin-1', newline='').write(text)
        bauen(ka, True)
        ersterCache = os.path.exists(os.path.join(a, name + '.bhedvis'))
        open(ka, 'w', encoding='latin-1', newline='').write(neu)
        ausgabe = bauen(ka, True)
        open(kb, 'w', encoding='latin-1', newline='').write(neu)
        bauen(kb, False)
        la, lb = lumps(os.path.join(a, name + '.bsp')), lumps(os.path.join(b, name + '.bsp'))
        anders = [i for i in range(18) if la[i] != lb[i]]
        wiederverwendet = 'visibility reused' in ausgabe
        pruefe(ersterCache and (wieder is None or wiederverwendet == wieder),
               '%s, %s: Sichtbarkeit %s' % (name, was, 'wiederverwendet' if wiederverwendet else 'neu gerechnet'))
        pruefe(not anders, '%s, %s: .bsp gleich dem vollen Bau (andere Lumps: %s)' % (name, was, anders))
print('q3map2_teilbau: %d Fehlschlaege' % fehler)
sys.exit(1 if fehler else 0)
