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
LICHT = os.environ.get('BHED_TEILBAU_LICHT', '-fast -threads 1').split()   # Schalter fuer -light
import shutil
import subprocess
import sys

Q3, DATEN = sys.argv[1], sys.argv[2]
ARBEIT = sys.argv[3] if len(sys.argv) > 3 else os.path.join(os.environ.get('TEMP', '.'), 'bhed_q3map2_teilbau')
KARTEN = sys.argv[4:] or [os.path.join(DATEN, 'raum.map'), os.path.join(DATEN, 'raum_einfach.map')]
fehler = 0


def lauf(*args):
    spiel = os.environ.get('BHED_TEILBAU_SPIEL', ARBEIT)   # Spielordner (Shader, Bilder), sonst keiner
    r = subprocess.run([Q3, '-game', 'ja', '-fs_basepath', spiel, '-fs_homepath', ''] + list(args), capture_output=True, text=True,
                       errors='replace', creationflags=0x40 if os.name == 'nt' else 0)
    return r.stdout + r.stderr


def bauen(karte, wieder):
    t = ''
    t += lauf('-meta', karte)
    t += lauf('-vis', '-threads', '1', *(['-bhedvisreuse'] if wieder else []), karte)
    t += lauf('-light', *LICHT, karte)
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

    # b) und d) -light -bhedlightcache: Lichter geaendert bzw. nur die Nachbearbeitung
    licht = re.search(r'"classname" "light"[^}]*?"origin" "([-\d.]+) ([-\d.]+) ([-\d.]+)"', text)
    if licht:
        lx, ly, lz = (float(v) for v in licht.groups())
        alt = licht.group(0)
        verschoben = alt.replace('"origin" "%s %s %s"' % licht.groups(), '"origin" "%g %g %g"' % (lx + 48, ly, lz), 1)
        mitLicht = text.replace(alt, verschoben, 1)
    else:   # ohne Licht: eines am Startpunkt dazu
        mitLicht = text + '{\n"classname" "light"\n"origin" "%g %g %g"\n"light" "300"\n}\n' % (sx, sy, sz + 64)
    nachbearbeitet = text.replace('"classname" "worldspawn"', '"classname" "worldspawn"\n"_minlight" "20"', 1)
    nachSchalter = ['-gamma', '1.3', '-exposure', '200', '-saturation', '0.7', '-compensate', '1.5']
    # (was, Karte, zusaetzliche -light-Schalter in allen drei Laeufen, Nachbearbeitung nur im zweiten)
    lichtFaelle = [
        ('Licht verschoben bzw. neu', mitLicht, [], []),
        ('nur Nachbearbeitung (-gamma 1.3 -exposure 200 -saturation 0.7 -compensate 1.5, _minlight 20)', nachbearbeitet, [], nachSchalter),
        # -super: StoreSurfaceLightmaps liest die Cluster, die der Filterdurchgang setzt (Fehler bis zum Cache-Format 2)
        ('Licht verschoben bzw. neu, -super 2', mitLicht, ['-super', '2'], []),
        ('nur Nachbearbeitung, -super 2 (_minlight 20)', nachbearbeitet, ['-super', '2'], nachSchalter),
    ]
    for was, neu, extra, nach in lichtFaelle:
        a = os.path.join(ARBEIT, name + '_la')
        b = os.path.join(ARBEIT, name + '_lb')
        for o in (a, b):
            shutil.rmtree(o, ignore_errors=True)
            os.makedirs(o)
        ka = os.path.join(a, name + '.map')
        kb = os.path.join(b, name + '.map')
        open(ka, 'w', encoding='latin-1', newline='').write(text)
        lauf('-meta', ka)
        lauf('-vis', '-threads', '1', ka)
        lauf('-light', *LICHT, *extra, '-bhedlightcache', ka)
        cache = os.path.exists(os.path.join(a, name + '.bhedlight'))
        open(ka, 'w', encoding='latin-1', newline='').write(neu)
        lauf('-meta', ka)
        lauf('-vis', '-threads', '1', ka)
        ausgabe = lauf('-light', *LICHT, *extra, '-bhedlightcache', *nach, ka)
        open(kb, 'w', encoding='latin-1', newline='').write(neu)
        lauf('-meta', kb)
        lauf('-vis', '-threads', '1', kb)
        lauf('-light', *LICHT, *extra, *nach, kb)
        la, lb = lumps(os.path.join(a, name + '.bsp')), lumps(os.path.join(b, name + '.bsp'))
        anders = [i for i in range(18) if la[i] != lb[i]]
        m = re.search(r'light cache: (\d+) of (\d+) lightmaps and (\d+) of (\d+) vertex lit surfaces taken from the cache, grid ([a-z ]+);', ausgabe)
        zeile = m.group(0) if m else '(keine Zeile)'
        aus = int(m.group(1)) if m else -1
        gitter = m.group(5) if m else ''
        if nach or 'minlight' in was:
            ok = m is not None and aus == int(m.group(2)) and gitter == 'from the cache'
        else:
            ok = m is not None and aus < int(m.group(2))
        pruefe(cache and ok, '%s, %s: %s' % (name, was, zeile))
        pruefe(not anders, '%s, %s: .bsp gleich dem vollen -light (andere Lumps: %s)' % (name, was, anders))
    # d) nur -light noch einmal auf der schon beleuchteten .bsp (Single: light stage) mit neuer Nachbearbeitung
    a = os.path.join(ARBEIT, name + '_lc')
    b = os.path.join(ARBEIT, name + '_ld')
    for o in (a, b):
        shutil.rmtree(o, ignore_errors=True)
        os.makedirs(o)
    ka = os.path.join(a, name + '.map')
    kb = os.path.join(b, name + '.map')
    nach = ['-gamma', '0.8', '-saturation', '1.4']
    for k in (ka, kb):
        open(k, 'w', encoding='latin-1', newline='').write(text)
        lauf('-meta', k)
        lauf('-vis', '-threads', '1', k)
    # dieselbe Folge ohne Cache: erst -light, dann -light mit den neuen Werten (auch der worldspawn-Befehlszeile wegen)
    lauf('-light', *LICHT, '-bhedlightcache', ka)
    ausgabe = lauf('-light', *LICHT, '-bhedlightcache', *nach, ka)
    lauf('-light', *LICHT, kb)
    lauf('-light', *LICHT, *nach, kb)
    la, lb = lumps(os.path.join(a, name + '.bsp')), lumps(os.path.join(b, name + '.bsp'))
    anders = [i for i in range(18) if la[i] != lb[i]]
    m = re.search(r'light cache: (\d+) of (\d+) lightmaps and (\d+) of (\d+) vertex lit surfaces taken from the cache, grid ([a-z ]+);', ausgabe)
    pruefe(m is not None and m.group(1) == m.group(2) and m.group(5) == 'from the cache',
           '%s, nur -light noch einmal (-gamma 0.8 -saturation 1.4): %s' % (name, m.group(0) if m else '(keine Zeile)'))
    pruefe(not anders, '%s, nur -light noch einmal: .bsp gleich zweimal vollem -light (andere Lumps: %s)' % (name, anders))
print('q3map2_teilbau: %d Fehlschlaege' % fehler)
sys.exit(1 if fehler else 0)
