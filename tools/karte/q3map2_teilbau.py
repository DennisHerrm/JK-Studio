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
e) lcb1: -light -bhedlightcache mit -bounce (1, 2, 8, die Final-Eintraege des Karten-Editors,
   -super/-bouncegrid, -external, GPU-Backen und Wechsel CPU -> GPU) auf zweiraum.map: Licht
   verschoben, nichts, nur Nachbearbeitung, Entity, Brush, Shader geaendert - jede .bsp und jede
   externe Lightmap Byte fuer Byte (cmp) gleich dem vollen Bau ohne Cache, Treffer je Durchgang.
   BHED_TEILBAU_NUR=bounce: nur dieser Teil.
f) lcb2: -light -bhedlightcache nach Geometrie-Aenderungen in Raum B (Struktur-Brush verschoben, dazu,
   geloescht, Detail-Brush, Patch, misc_model, Shader-Tausch, Brush-Entity; bounce 0/1/8, CPU/GPU): Raum A
   kommt aus dem Cache, jede Datei Byte fuer Byte gleich dem vollen Bau. BHED_TEILBAU_NUR=geometrie.
g) lcb2: dasselbe auf einer offenen Karte unter dem Himmel (eine Sonne, Himmelslicht mit 65 Sonnen): eine Kiste in einer
   Ecke verschoben (Struktur, Detail, mit Licht dazu) - der groesste Teil des Hofs kommt aus dem Cache; eine hohe Saeule
   dazu bzw. geloescht, deren Schatten weit faellt. BHED_TEILBAU_NUR=offen.
BHED_TEILBAU_NUR=karten: nur a) bis d). ctest startet die vier Teile als q3map2_teilbau, _bounce, _geometrie und _offen.
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


NUR = os.environ.get('BHED_TEILBAU_NUR', '')
for quelle in ([] if NUR in ('bounce', 'geometrie', 'offen') else KARTEN):
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

# e) lcb1: der Licht-Cache mit -bounce
ZWEI = os.path.join(DATEN, 'zweiraum.map')
GPU = os.environ.get('BHED_TEILBAU_GPU', '1') != '0'   # GPU-Backen (ohne GPU rechnet bhed_q3map2 auf dem Prozessor, gleiche Bytes)


def lichtbau(ordner, name, text, schalter, cache):
    k = os.path.join(ordner, name + '.map')
    open(k, 'w', encoding='latin-1', newline='').write(text)
    lauf('-meta', k)
    lauf('-vis', '-threads', '1', k)
    return lauf('-light', *schalter, *(['-bhedlightcache'] if cache else []), k)


def dateien(ordner, name):
    """Die .bsp und jede externe Lightmap (<ordner>/<name>/lm_*.tga) mit Inhalt. Nur die Uhrzeit, die q3map2
    (wie NRCs) beim Schreiben in den Kopf der .bsp setzt ("I LOVE MY Q3MAP2 ... on Tue Nov 10 06:46:14 2026"),
    ist ausgenommen - zwei volle Baeue in verschiedenen Sekunden unterscheiden sich sonst darin."""
    bsp = open(os.path.join(ordner, name + '.bsp'), 'rb').read()
    d = {name + '.bsp': re.sub(rb'(I LOVE MY Q3MAP2 [^\n]*? on )[^\n]*\n', rb'\1(Uhrzeit)\n', bsp, count=1)}
    lm = os.path.join(ordner, name)
    if os.path.isdir(lm):
        for f in sorted(os.listdir(lm)):
            d[name + '/' + f] = open(os.path.join(lm, f), 'rb').read()
    return d


def paesse(ausgabe):
    return [tuple(int(x) for x in m) for m in re.findall(r'light cache pass (\d+) of (\d+): (\d+) of (\d+) lightmaps from the cache', ausgabe)]


def licht_a(text):
    return text.replace('"origin" "-768 0 192"', '"origin" "-720 0 192"', 1)


def start_weg(text):
    return text.replace('"origin" "-768 0 24"', '"origin" "-704 64 24"', 1)


def shader_a(text):   # die erste Wand (Raum A) bekommt innen einen anderen Shader (ihre erste Seite liegt verdeckt unter der Decke)
    i = text.index('test/wand', text.index('test/wand') + 1)
    return text[:i] + 'test/wand2' + text[i + len('test/wand'):]


if NUR in ('', 'bounce') and os.path.exists(ZWEI):
    name = 'zweiraum'
    text = open(ZWEI, encoding='latin-1', newline='').read()
    final_fast = ['-fast', '-patchshadows', '-samples', '2', '-filter', '-bounce', '8']
    extreme = ['-fastbounce', '-patchshadows', '-samples', '4', '-randomsamples', '-bounce', '8']
    gpu = ['-bhedgpubake', '-bhedgpubakecheck', '0']
    nach = ['-gamma', '1.3', '-exposure', '200', '-saturation', '0.7', '-compensate', '1.5']
    # (Name, Schalter des ersten Baus, Schalter der beiden Vergleichsbauten, Aenderung, Erwartung)
    #   Erwartung: 'treffer' = der erste Durchgang und der erste Bounce nehmen Lightmaps aus dem Cache,
    #              'alle' = jeder Durchgang alles aus dem Cache, 'erster' = der erste Durchgang alles,
    #              sonst der Grund, warum der Cache nicht greift
    faelle = [
        ('-bounce 1, Licht verschoben', LICHT + ['-bounce', '1'], None, licht_a, 'treffer'),
        ('-bounce 2, Licht verschoben', LICHT + ['-bounce', '2'], None, licht_a, 'treffer'),
        ('-bounce 8, Licht verschoben', LICHT + ['-bounce', '8'], None, licht_a, 'treffer'),
        ('[Final] Fast, Licht verschoben', final_fast, None, licht_a, 'treffer'),
        ('[Final] Extreme Quality, Licht verschoben', extreme, None, licht_a, 'treffer'),
        ('-super 2 -bouncegrid -bounce 2, Licht verschoben', LICHT + ['-super', '2', '-bouncegrid', '-bounce', '2'], None, licht_a, 'treffer'),
        ('-external -bounce 2, Licht verschoben', LICHT + ['-external', '-bounce', '2'], None, licht_a, 'treffer'),
        ('-bounce 2, nichts geaendert', LICHT + ['-bounce', '2'], None, lambda t: t, 'alle'),
        ('-bounce 2, nur Nachbearbeitung', LICHT + ['-bounce', '2'], LICHT + ['-bounce', '2'] + nach, lambda t: t, 'erster'),
        ('-bounce 2, Entity (info_player_start)', LICHT + ['-bounce', '2'], None, start_weg, 'alle'),
        ('-bounce 2, Brush (Detail in Raum B)', LICHT + ['-bounce', '2'], None, lambda t: zusatz_pinsel(t, True, 960, -192, 40), 'treffer'),
        ('-bounce 2, Shader (Wand in Raum A)', LICHT + ['-bounce', '2'], None, shader_a, 'treffer'),
    ]
    if GPU:
        faelle += [
            ('[Final] Fast mit GPU-Backen, Licht verschoben', final_fast + gpu, None, licht_a, 'treffer'),
            ('[Final] Extreme Quality mit GPU-Backen, Licht verschoben', extreme + gpu, None, licht_a, 'treffer'),
            ('[Final] Fast, Cache vom Prozessor, Teilbau mit GPU-Backen, Licht verschoben', final_fast, final_fast + gpu, licht_a, 'treffer'),
        ]
    erste = {}   # Schalter -> Ordner mit dem ersten Bau (Cache)
    for was, schalter, vergleich, aendern, erwartung in faelle:
        vergleich = vergleich or schalter
        schluessel = ' '.join(schalter)
        if schluessel not in erste:
            o = os.path.join(ARBEIT, 'e%d_erst' % len(erste))
            shutil.rmtree(o, ignore_errors=True)
            os.makedirs(o)
            lichtbau(o, name, text, schalter, True)
            erste[schluessel] = o
        a = os.path.join(ARBEIT, 'e_teil')
        b = os.path.join(ARBEIT, 'e_voll')
        for o in (a, b):
            shutil.rmtree(o, ignore_errors=True)
        shutil.copytree(erste[schluessel], a)
        os.makedirs(b)
        cache = os.path.exists(os.path.join(a, name + '.bhedlight'))
        neu = aendern(text)
        ausgabe = lichtbau(a, name, neu, vergleich, True)
        lichtbau(b, name, neu, vergleich, False)
        da, db = dateien(a, name), dateien(b, name)
        anders = sorted(k for k in set(da) | set(db) if da.get(k) != db.get(k))
        if name + '.bsp' in anders:   # welche Lumps
            la, lb = lumps(os.path.join(a, name + '.bsp')), lumps(os.path.join(b, name + '.bsp'))
            anders.append('Lumps %s' % [i for i in range(18) if la[i] != lb[i]])
        p = paesse(ausgabe)
        text_p = ', '.join('%d/%d' % (x[2], x[3]) for x in p) or '(keine Zeile)'
        if erwartung == 'treffer':
            ok = len(p) >= 2 and p[0][2] > 0 and p[1][2] > 0 and all(x[2] < x[3] for x in p[:1])
        elif erwartung == 'alle':
            ok = len(p) >= 1 and all(x[2] == x[3] for x in p) and 'light cache found' in ausgabe
        elif erwartung == 'erster':
            ok = len(p) >= 1 and p[0][2] == p[0][3]
        else:
            ok = ('light cache not usable: ' in ausgabe and erwartung in ausgabe.split('light cache not usable: ', 1)[1].split('\n', 1)[0]
                  and all(x[2] == 0 for x in p))
        pruefe(cache and ok, 'zweiraum, %s: aus dem Cache je Durchgang %s' % (was, text_p))
        pruefe(not anders and len(da) >= 1, 'zweiraum, %s: %d Dateien gleich dem vollen Bau (andere: %s)' % (was, len(da), anders))
    # lcb2: der Cache eines angehaltenen Licht-Schritts - die Datei endet mitten im letzten Durchgang (der Karten-Editor macht
    # sie aus <karte>.bhedlight.neu): die ganzen Durchgaenge kommen aus dem Cache, der abgeschnittene wird neu gerechnet
    schalter = LICHT + ['-bounce', '2']
    a = os.path.join(ARBEIT, 'e_teil')
    b = os.path.join(ARBEIT, 'e_voll')
    for o in (a, b):
        shutil.rmtree(o, ignore_errors=True)
    shutil.copytree(erste[' '.join(schalter)], a)
    os.makedirs(b)
    cachedatei = os.path.join(a, name + '.bhedlight')
    ganz = os.path.getsize(cachedatei)
    with open(cachedatei, 'r+b') as f:
        f.truncate(ganz - 100)
    ausgabe = lichtbau(a, name, text, schalter, True)
    lichtbau(b, name, text, schalter, False)
    da, db = dateien(a, name), dateien(b, name)
    anders = sorted(k for k in set(da) | set(db) if da.get(k) != db.get(k))
    p = paesse(ausgabe)
    text_p = ', '.join('%d/%d' % (x[2], x[3]) for x in p) or '(keine Zeile)'
    pruefe('2 of 3 passes kept' in ausgabe and len(p) == 3 and p[0][2] == p[0][3] and p[1][2] == p[1][3] and p[2][2] == 0,
           'zweiraum, -bounce 2, Cache mitten im dritten Durchgang abgeschnitten: 2 von 3 Durchgaengen behalten, aus dem Cache je Durchgang %s' % text_p)
    # der neue Cache: Kopf (Kennung 8, Fassung 32, Rechenweg 32 (lcfix), 3 x 16 Bytes), dann je Durchgang Nummer (i32), Laenge (u64) und so viele Bytes
    import struct
    roh = open(cachedatei, 'rb').read()
    pos, ganze = 8 + 32 + 32 + 3 * 16, []
    while pos + 12 <= len(roh):
        nr, laenge = struct.unpack_from('<iQ', roh, pos)
        pos += 12 + laenge
        if pos <= len(roh):
            ganze.append(nr)
    pruefe(not anders and len(da) >= 1 and ganze == [0, 1, 2] and pos == len(roh),
           'zweiraum, -bounce 2, Cache abgeschnitten: %d Dateien gleich dem vollen Bau (andere: %s), der neue Cache hat die Durchgaenge %s ganz' %
           (len(da), anders, ganze))

# f) lcb2: der Licht-Cache nach Geometrie-, Shader- und Brush-Entity-Aenderungen (Raum B), Raum A bleibt
def quader(x0, y0, z0, x1, y1, z1, tex, detail=False):
    flags = ' 134217728 0 0' if detail else ' 0 0 0'
    e = [((x1, y1, z1), (x1, y0, z1), (x0, y1, z1)), ((x1, y1, z1), (x0, y1, z1), (x1, y1, z0)),
         ((x1, y1, z1), (x1, y1, z0), (x1, y0, z1)), ((x0, y0, z0), (x1, y0, z0), (x0, y1, z0)),
         ((x0, y0, z0), (x0, y0, z1), (x1, y0, z0)), ((x0, y0, z0), (x0, y1, z0), (x0, y0, z1))]
    return '{\n' + ''.join('( %g %g %g ) ( %g %g %g ) ( %g %g %g ) %s 0 0 0 0.5 0.5%s\n' % (a + b + c + (tex, flags)) for a, b, c in e) + '}\n'


def in_welt(text, b):   # vor die schliessende Klammer der worldspawn
    tiefe = 0
    for i, ch in enumerate(text):
        if ch == '{':
            tiefe += 1
        elif ch == '}':
            tiefe -= 1
            if tiefe == 0:
                return text[:i] + b + text[i:]
    return text


def patch(x, y, z):   # ein gewoelbter 3x3-Patch, 128 x 128, 32 hoch
    zeilen = []
    for i in range(3):
        zeilen.append('( ' + ' '.join('( %g %g %g %g %g )' % (x + 64 * i, y + 64 * j, z + (32 if i == 1 and j == 1 else 0), i / 2, j / 2)
                                      for j in range(3)) + ' )')
    return '{\npatchDef2\n{\ntest/boden\n( 3 3 0 0 0 )\n(\n' + '\n'.join(zeilen) + '\n)\n}\n}\n'


def modell(x, y, z, winkel=0):
    return '{\n"classname" "misc_model"\n"model" "models/test/marken.ase"\n"origin" "%g %g %g"\n"angle" "%g"\n}\n' % (x, y, z, winkel)


def brushent(x, y):
    return '{\n"classname" "func_static"\n' + quader(x, y, 0, x + 48, y + 48, 96, 'test/wand') + '}\n'


def sonne():
    """Eine Sonne senkrecht von oben als Entity (light mit Ziel und _sun). In den geschlossenen Raeumen beleuchtet sie nichts,
    steht aber in der Lichtliste jeder Lightmap: der Raum ihrer Strahlen ist dann die Lightmap, nach oben gezogen, UND der Raum
    zu den Punktlichtern. lcb2: der zweite ging verloren, sobald eine Sonne in der Liste stand (Verweis in einen Vektor, der
    beim Anhaengen der Sonne umzog) - ein verschobener Brush, der nur den Schatten eines Punktlichts aendert, wurde nicht
    bemerkt, und die Lightmap kam falsch aus dem Cache."""
    return ('{\n"classname" "info_null"\n"targetname" "sonne_ziel"\n"origin" "0 0 0"\n}\n'
            '{\n"classname" "light"\n"origin" "0 0 1024"\n"target" "sonne_ziel"\n"_sun" "1"\n"light" "50"\n}\n')


SAEULE = quader(640, -160, 0, 704, -96, 256, 'test/wand')   # Struktur-Saeule in Raum B (Boden bis Decke)


def shader_b(text):   # die Saeule in Raum B bekommt einen anderen Shader
    return text.replace(SAEULE, SAEULE.replace('test/wand', 'test/wand2'), 1)


if NUR in ('', 'geometrie') and os.path.exists(ZWEI):
    name = 'zweiraum'
    os.makedirs(os.path.join(ARBEIT, 'base', 'models', 'test'), exist_ok=True)
    shutil.copyfile(os.path.join(DATEN, 'marken.ase'), os.path.join(ARBEIT, 'base', 'models', 'test', 'marken.ase'))
    # Grundkarte: zweiraum mit Saeule, Patch, Modell und Brush-Entity in Raum B
    grund = in_welt(open(ZWEI, encoding='latin-1', newline='').read(), SAEULE + patch(800, -224, 0)) + modell(900, 160, 0) + brushent(560, 128) + sonne()
    final_fast = ['-fast', '-patchshadows', '-samples', '2', '-filter', '-bounce', '8']
    gpu = ['-bhedgpubake', '-bhedgpubakecheck', '0']
    aenderungen = [
        ('Struktur-Brush verschoben', lambda t: t.replace(SAEULE, quader(672, -160, 0, 736, -96, 256, 'test/wand'), 1)),
        ('Struktur-Brush dazu', lambda t: in_welt(t, quader(880, 0, 0, 944, 64, 256, 'test/wand'))),
        ('Struktur-Brush geloescht', lambda t: t.replace(SAEULE, '', 1)),
        ('Detail-Brush dazu', lambda t: in_welt(t, quader(880, 0, 0, 912, 32, 32, 'test/boden', True))),
        # lcb2: eine schwebende Platte zwischen dem Licht von Raum B (768 0 192) und der Ostwand - ihr Schatten faellt auf Wand
        # und Decke, sie liegt aber nicht ueber diesen Lightmaps (nur der Raum zum Punktlicht bemerkt sie, s. sonne())
        ('Detail-Platte vor der Wand dazu', lambda t: in_welt(t, quader(880, -32, 128, 888, 32, 224, 'test/boden', True))),
        ('Patch verschoben', lambda t: t.replace(patch(800, -224, 0), patch(816, -224, 0), 1)),
        ('misc_model gedreht', lambda t: t.replace(modell(900, 160, 0), modell(900, 160, 0, 90), 1)),
        ('Shader-Tausch', shader_b),
        ('Brush-Entity verschoben', lambda t: t.replace(brushent(560, 128), brushent(592, 128), 1)),
    ]
    # (Schalter, welche Aenderungen): bounce 0 und 1 alle, [Final] Fast (bounce 8) und GPU eine Auswahl
    reihen = [(LICHT, aenderungen),
              (LICHT + ['-bounce', '1'], aenderungen),
              (final_fast, [aenderungen[0], aenderungen[3], aenderungen[4], aenderungen[7]]),
              (final_fast + gpu if GPU else None, [aenderungen[1], aenderungen[4], aenderungen[5], aenderungen[8]])]
    for r, (schalter, liste) in enumerate(reihen):
        if schalter is None:
            continue
        erst = os.path.join(ARBEIT, 'f%d_erst' % r)
        shutil.rmtree(erst, ignore_errors=True)
        os.makedirs(erst)
        lichtbau(erst, name, grund, schalter, True)
        for was, aendern in liste:
            a = os.path.join(ARBEIT, 'f_teil')
            b = os.path.join(ARBEIT, 'f_voll')
            for o in (a, b):
                shutil.rmtree(o, ignore_errors=True)
            shutil.copytree(erst, a)
            os.makedirs(b)
            cache = os.path.exists(os.path.join(a, name + '.bhedlight'))
            neu = aendern(grund)
            ausgabe = lichtbau(a, name, neu, schalter, True)
            lichtbau(b, name, neu, schalter, False)
            da, db = dateien(a, name), dateien(b, name)
            anders = sorted(k for k in set(da) | set(db) if da.get(k) != db.get(k))
            if name + '.bsp' in anders:
                la, lb = lumps(os.path.join(a, name + '.bsp')), lumps(os.path.join(b, name + '.bsp'))
                anders.append('Lumps %s' % [i for i in range(18) if la[i] != lb[i]])
            p = paesse(ausgabe)
            text_p = ', '.join('%d/%d' % (x[2], x[3]) for x in p) or '(keine Zeile)'
            # Raum A sieht die Aenderung nicht: der erste Durchgang nimmt Lightmaps aus dem Cache, aber nicht alle
            ok = neu != grund and len(p) >= 1 and 0 < p[0][2] < p[0][3]
            pruefe(cache and ok, 'zweiraum %s, %s: aus dem Cache je Durchgang %s' % (' '.join(schalter), was, text_p))
            pruefe(not anders and len(da) >= 1, 'zweiraum %s, %s: %d Dateien gleich dem vollen Bau (andere: %s)' % (' '.join(schalter), was, len(da), anders))

# g) lcb2: der Licht-Cache auf einer offenen Karte unter dem Himmel - eine Sonne und das Himmelslicht (65 Sonnen ueber die Halbkugel)
HIMMEL = ('textures/test/himmel\n{\n\tsurfaceparm sky\n\tsurfaceparm noimpact\n\tsurfaceparm nolightmap\n'
          '\tq3map_sun 1 0.95 0.85 180 60 35\n\tq3map_skylight 60 5\n}\n')


def hof():
    """Ein Hof, 4096 x 4096, Mauern 192 hoch, darueber und oben Himmel; Saeulen und Kisten ueber den Hof verteilt, ein
    Punktlicht. Die Sonne steht 35 Grad hoch (lange Schatten: was sich aendert, wirft seinen Schatten weit weg)."""
    w = quader(-2048, -2048, -16, 2048, 2048, 0, 'test/boden')
    for x0, y0, x1, y1 in ((-2064, -2048, -2048, 2048), (2048, -2048, 2064, 2048), (-2048, -2064, 2048, -2048), (-2048, 2048, 2048, 2064)):
        w += quader(x0, y0, 0, x1, y1, 192, 'test/wand') + quader(x0, y0, 192, x1, y1, 1024, 'test/himmel')
    w += quader(-2048, -2048, 1024, 2048, 2048, 1040, 'test/himmel')
    for x, y, h in ((-1536, -1536, 256), (-1024, 512, 320), (0, 0, 384), (640, -896, 256), (1280, 1280, 320), (-512, 1408, 192)):
        w += quader(x, y, 0, x + 64, y + 64, h, 'test/wand')
    for x, y in ((-1792, 1152), (256, 1024), (896, 320), (-320, -1344)):
        w += quader(x, y, 0, x + 96, y + 96, 96, 'test/boden', True)
    return ('{\n"classname" "worldspawn"\n"message" "BehavEd q3map2 test: courtyard under the sky (lcb2)"\n' + w + '}\n'
            '{\n"classname" "info_player_start"\n"origin" "-1792 -1792 24"\n}\n'
            '{\n"classname" "light"\n"origin" "-1024 1024 128"\n"light" "300"\n}\n')


HOF_KISTE = quader(1536, -1664, 0, 1600, -1600, 128, 'test/wand')          # Struktur-Kiste in der Suedost-Ecke
HOF_DETAIL = quader(1664, 1536, 0, 1728, 1600, 96, 'test/boden', True)     # Detail-Kiste in der Nordost-Ecke
HOF_MITTE = quader(0, 0, 0, 64, 64, 384, 'test/wand')                      # die Saeule in der Mitte

if NUR in ('', 'offen'):
    name = 'hof'
    os.makedirs(os.path.join(ARBEIT, 'base', 'shaders'), exist_ok=True)
    open(os.path.join(ARBEIT, 'base', 'shaders', 'test_himmel.shader'), 'w', encoding='ascii', newline='\n').write(HIMMEL)
    grund = in_welt(hof(), HOF_KISTE + HOF_DETAIL)
    final_fast = ['-fast', '-patchshadows', '-samples', '2', '-filter', '-bounce', '8']
    gpu = ['-bhedgpubake', '-bhedgpubakecheck', '0']
    kiste_weg = quader(1568, -1664, 0, 1632, -1600, 128, 'test/wand')
    # (Name, Aenderung, was der erste Durchgang aus dem Cache nehmen muss: 'meist' = mehr als die Haelfte der Lightmaps,
    #  'einige' = mindestens eine, None = nichts verlangt). Gemessen beim Anlegen, erster Durchgang, von 65 Raw-Lightmaps (in
    #  Klammern mit dem Strahlraum davor, der jede Sonne ueber ihre ganze Strecke gegen jede Ebene pruefte und einer Lightmap
    #  unter mehr als 63 Sonnen keinen Abdruck gab): Licht 47 (37), Detail-Kiste 40 (19), Struktur-Kiste 21 (18), Kiste und
    #  Licht 21 (18), hohe Saeule 8 von 70 (8), Saeule geloescht 0 von 60 (0).
    aenderungen = [
        # nur das Punktlicht verschoben: der Boden und die Mauern weit weg sehen es nicht (sie stehen unter 66 Sonnen)
        ('Licht verschoben', lambda t: t.replace('"origin" "-1024 1024 128"', '"origin" "-960 1024 128"', 1), 'meist'),
        # eine Detail-Kiste in der Ecke um 32 verschoben: der Strahlbaum aendert sich nur in ihrem Blatt
        ('Detail-Kiste in der Ecke verschoben', lambda t: t.replace(HOF_DETAIL, quader(1664, 1568, 0, 1728, 1632, 96, 'test/boden', True), 1), 'meist'),
        # eine Struktur-Kiste verschoben: ihre Seiten sind Ebenen des BSP - jeder Strahl, der eine davon irgendwo kreuzt, wird
        # an anderer Stelle geschnitten, und die Flaechen ihres Blocks werden neu zerlegt (der Boden ist eine andere Lightmap)
        ('Struktur-Kiste in der Ecke verschoben', lambda t: t.replace(HOF_KISTE, kiste_weg, 1), 'einige'),
        # die typische Aenderung eines Mappers: die Kiste verschoben und ein Licht daneben
        ('Kiste verschoben und Licht dazu',
         lambda t: t.replace(HOF_KISTE, kiste_weg, 1) + '{\n"classname" "light"\n"origin" "1472 -1472 96"\n"light" "200"\n}\n', 'einige'),
        # eine hohe Saeule dazu: ihr Schatten (Sonne und Himmelslicht) faellt quer ueber den Hof auf Lightmaps weit weg von ihr
        ('hohe Saeule dazu', lambda t: in_welt(t, quader(-896, -1024, 0, -832, -960, 896, 'test/wand')), 'einige'),
        # eine Saeule weg: die Flaechen werden anders zu Lightmaps zusammengelegt (60 statt 65) - nur die Bytes zaehlen
        ('Saeule in der Mitte geloescht', lambda t: t.replace(HOF_MITTE, '', 1), None),
    ]
    reihen = [(LICHT, aenderungen),
              (LICHT + ['-bounce', '1'], [aenderungen[1], aenderungen[4]]),
              (final_fast, [aenderungen[0], aenderungen[3], aenderungen[5]]),
              (final_fast + gpu if GPU else None, [aenderungen[1], aenderungen[3], aenderungen[4]])]
    for r, (schalter, liste) in enumerate(reihen):
        if schalter is None:
            continue
        erst = os.path.join(ARBEIT, 'g%d_erst' % r)
        shutil.rmtree(erst, ignore_errors=True)
        os.makedirs(erst)
        lichtbau(erst, name, grund, schalter, True)
        for was, aendern, soll in liste:
            a = os.path.join(ARBEIT, 'g_teil')
            b = os.path.join(ARBEIT, 'g_voll')
            for o in (a, b):
                shutil.rmtree(o, ignore_errors=True)
            shutil.copytree(erst, a)
            os.makedirs(b)
            cache = os.path.exists(os.path.join(a, name + '.bhedlight'))
            neu = aendern(grund)
            ausgabe = lichtbau(a, name, neu, schalter, True)
            lichtbau(b, name, neu, schalter, False)
            da, db = dateien(a, name), dateien(b, name)
            anders = sorted(k for k in set(da) | set(db) if da.get(k) != db.get(k))
            if name + '.bsp' in anders:
                la, lb = lumps(os.path.join(a, name + '.bsp')), lumps(os.path.join(b, name + '.bsp'))
                anders.append('Lumps %s' % [i for i in range(18) if la[i] != lb[i]])
            p = paesse(ausgabe)
            text_p = ', '.join('%d/%d' % (x[2], x[3]) for x in p) or '(keine Zeile)'
            # der erste Durchgang nimmt nie alle Lightmaps aus dem Cache und so viele, wie die Aenderung verlangt
            ok = (neu != grund and len(p) >= 1 and p[0][2] < p[0][3] and
                  (soll is None or (p[0][2] > 0 and (soll != 'meist' or 2 * p[0][2] > p[0][3]))))
            pruefe(cache and ok, 'hof %s, %s: aus dem Cache je Durchgang %s' % (' '.join(schalter), was, text_p))
            pruefe(not anders and len(da) >= 1, 'hof %s, %s: %d Dateien gleich dem vollen Bau (andere: %s)' % (' '.join(schalter), was, len(da), anders))

print('q3map2_teilbau: %d Fehlschlaege' % fehler)
sys.exit(1 if fehler else 0)
