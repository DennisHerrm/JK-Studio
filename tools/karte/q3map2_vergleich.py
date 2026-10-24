#!/usr/bin/env python3
"""bhed_q3map2.exe gegen NRCs q3map2.exe - feste Erwartungen (Stufe 3 Punkt 2).

Was der Test zeigt
------------------
bhed_q3map2.exe ist q3map2 aus NetRadiant-Custom 20260114, mit MSVC statt
MinGW/GCC gebaut (third_party/q3map2/README.md, src/q3map2/CMakeLists.txt).
Dieser Test baut die Karten in tests/karte/q3map2 mit den Schaltern des
Build-Menues ("[Final] Fast" des JA-Spielpakets) und vergleicht jede Stufe mit
dem, was NRCs q3map2.exe unter denselben Bedingungen schreibt. Die Erwartungen
in erwartung.json stammen aus NRCs q3map2.exe (Aufruf "erwartung", siehe unten).

Warum nicht einfach "alle Bytes gleich"
---------------------------------------
NRCs q3map2.exe schreibt selbst nicht zweimal dieselbe .bsp, sobald eine Karte
mehr als einen Shader hat (NOTIZEN_karte3.md, Punkt 2):
  - surface_meta.cpp:144 sortiert Meta-Dreiecke zuerst nach dem ZEIGER auf
    shaderInfo_t; die Shader liegen in einem std::multiset, die Adressen kommen
    vom Heap. Reihenfolge und Zusammenfassung der Flaechen (Lumps surfaces,
    drawverts, drawindexes, leafsurfaces, brushsides.surfaceNum) sind je Lauf
    anders.
  - die Lightmap-Koordinaten der Ecken sind nach -bsp nicht belegt.
  - lightmaps_ydnar.cpp:837 dasselbe beim Packen der Lightmaps; die
    Reihenfolge entscheidet auch, welche "identischen" Lightmaps gemittelt
    werden.
  - -vis und -light mit mehreren Threads haengen von deren Reihenfolge ab.
Darum: -threads 1, und je Stufe Merkmale - jeder Lump roh und dazu Merkmale,
die von der Reihenfolge nicht abhaengen (die Dreiecke je Shader mit allen
Eckdaten als Menge, die Lumps ohne ihre Verweise auf Flaechen, die Ecken ohne
Lightmap-Koordinaten ...). Erwartet wird jedes Merkmal, das bei NRC in allen
Laeufen gleich war UND bei uns in allen Laeufen gleich war; die anderen stehen
mit Grund unter "schwankt".

-vis bekommt NRCs .bsp/.prt/.srf nach -bsp (<karte>.nrc.bsp, .nrc.prt,
.nrc.srf) als Eingabe, -light unsere .bsp nach -vis (die mit NRCs Byte fuer
Byte gleich sein muss) und NRCs .srf - so vergleicht jede Stufe dieselbe
Eingabe.

Aufrufe
-------
  q3map2_vergleich.py pruefen <bhed_q3map2.exe> <tests/karte/q3map2>
      ctest q3map2_vergleich. Ohne Spielordner: -fs_basepath ist ein leerer
      Ordner (die Shader sind dann alle "implizit", wie bei einer Karte mit
      fehlenden Texturen) - so laeuft der Test auf jedem Rechner gleich.
  q3map2_vergleich.py erwartung <NRC-q3map2.exe> <bhed_q3map2.exe> <tests/karte/q3map2> [--laeufe N]
      schreibt erwartung.json und <karte>.nrc.bsp/.prt/.srf neu (NRC je
      Stufe N-mal, unsere N-mal). Bricht ab, wenn ein Merkmal bei beiden
      stabil ist und trotzdem abweicht.

Nur die Standardbibliothek.
"""
import collections
import hashlib
import json
import os
import shutil
import struct
import subprocess
import sys
import tempfile

SURF = 148
VERT = 80
NAMEN = ['entities', 'shaders', 'planes', 'nodes', 'leafs', 'leafsurfaces', 'leafbrushes', 'models', 'brushes', 'brushsides',
         'drawverts', 'drawindexes', 'fogs', 'surfaces', 'lightmaps', 'lightgrid', 'visibility', 'lightarray']

# Schalter wie data/karte/ja/default_build_menu.xml "[Final] Fast", dazu -threads 1
SPIEL = ['-game', 'ja']
STUFEN = {
    'bsp': ['-meta', '-threads', '1'],
    'vis': ['-vis', '-threads', '1'],
    'light': ['-light', '-fast', '-patchshadows', '-samples', '2', '-filter', '-bounce', '8', '-threads', '1'],
}
# Karten und Stufen; die Leck-Karte endet nach -bsp (kein .prt, aber .lin)
KARTEN = {'raum': ('bsp', 'vis', 'light'), 'raum_leck': ('bsp',), 'raum_einfach': ('bsp', 'vis', 'light')}
IDLE = 0x00000040 if os.name == 'nt' else 0

# Pflicht sind nur Merkmale, die von Heap-Adressen und Speicherresten nicht
# abhaengen koennen (siehe oben). Die anderen werden verglichen und gemeldet,
# lassen den Test aber nicht scheitern: dass sie hier bei 6 Laeufen stabil
# waren, heisst nicht, dass ein anderer Rechner dieselben Adressen vergibt.
PFLICHT = {
    'bsp': {'lump:entities', 'lump:shaders', 'lump:planes', 'lump:nodes', 'lump:leafbrushes', 'lump:brushes', 'lump:fogs',
            'lump:lightmaps', 'lump:lightgrid', 'lump:visibility', 'lump:lightarray', 'ohne_flaechen:leafs',
            'ohne_flaechen:models', 'ohne_flaechen:brushsides', 'dreiecke', 'anzahl:dreiecke', 'datei:.prt', 'datei:.lin'},
    # gleiche Eingabe, ein Thread: -vis schreibt nur visibility, alles andere geht durch
    'vis': {'lump:' + n for n in NAMEN} | {'datei:.prt'},
    # gleiche Eingabe, ein Thread; Lightmap-Nummern, -Positionen und -Inhalte
    # haengen von der Packreihenfolge ab (CompareSurfaceInfo, MergeBSPLuxels)
    'light': {'lump:' + n for n in NAMEN if n not in ('drawverts', 'surfaces', 'lightmaps')}
             | {'ohne_lightmap:drawverts', 'ohne_lightmap:surfaces', 'dreiecke', 'anzahl:dreiecke'},
}


def pflicht_fuer(karte, stufe, namen):
    """raum_einfach hat nur EINEN Shader: kein Zeigervergleich entscheidet etwas, die
    Reihenfolge haengt nur noch vom Verfahren ab (std::sort wie libstdc++, Nr. 7).
    Dort ist jeder Lump Pflicht - ausser drawverts nach -bsp (unbelegte
    Lightmap-Koordinaten der Ecken, Speicherreste)."""
    if karte == 'raum_einfach':
        return {n for n in namen if not (stufe == 'bsp' and n == 'lump:drawverts')}
    return PFLICHT[stufe] & set(namen)


def h(b):
    return hashlib.sha256(b).hexdigest()[:20]


def lumps(pfad):
    d = open(pfad, 'rb').read()
    if d[:4] != b'RBSP':
        raise ValueError('%s: keine RBSP-Datei' % pfad)
    k = []
    for i in range(18):
        o, l = struct.unpack_from('<ii', d, 8 + 8 * i)
        k.append(d[o:o + l])
    return k


def ohne(daten, groesse, von, bis):
    """Felder [von, bis) jedes Elements auf 0 setzen."""
    b = bytearray(daten)
    for i in range(len(b) // groesse):
        b[i * groesse + von:i * groesse + bis] = bytes(bis - von)
    return bytes(b)


def ecke(v, i):
    e = v[i * VERT:(i + 1) * VERT]
    return e[:20] + e[52:]   # ohne Lightmap-Koordinaten (Bytes 20..52)


def dreiecke(k):
    """Dreiecke je (Shader, Fog, Typ) mit allen Eckdaten ausser den Lightmap-Koordinaten, als Menge."""
    s, v, ix = k[13], k[10], k[11]
    z = collections.Counter()
    for i in range(len(s) // SURF):
        sh, fog, typ, fv, nv, fi, ni = struct.unpack_from('<7i', s, i * SURF)
        if typ == 2 or ni == 0:   # Patch: ganze Flaeche (Kontrollpunkte)
            rec = bytearray(s[i * SURF:(i + 1) * SURF])
            rec[12:16] = bytes(4)
            rec[20:24] = bytes(4)
            rec[36:84] = bytes(48)
            z[repr((sh, fog, typ, bytes(rec).hex(), [ecke(v, fv + j).hex() for j in range(nv)]))] += 1
            continue
        idx = struct.unpack_from('<%di' % ni, ix, fi * 4)
        for t in range(0, ni - 2, 3):
            z[repr((sh, fog, typ, sorted(ecke(v, fv + idx[t + j]).hex() for j in range(3))))] += 1
    return h(repr(sorted(z.items())).encode()), sum(z.values())


def merkmale(pfad):
    k = lumps(pfad)
    m = {'lump:' + n: h(k[i]) for i, n in enumerate(NAMEN)}
    m['ohne_flaechen:leafs'] = h(ohne(k[4], 48, 32, 40))
    m['ohne_flaechen:models'] = h(ohne(k[7], 40, 24, 32))
    m['ohne_flaechen:brushsides'] = h(ohne(k[9], 12, 8, 12))
    m['dreiecke'], anzahl = dreiecke(k)
    m['anzahl:dreiecke'] = str(anzahl)
    m['ohne_lightmap:drawverts'] = h(ohne(k[10], VERT, 20, 52))
    m['ohne_lightmap:surfaces'] = h(ohne(k[13], SURF, 36, 84))
    for ext in ('.prt', '.lin', '.srf'):
        p = pfad[:-4] + ext
        if os.path.exists(p):
            m['datei:' + ext] = h(open(p, 'rb').read())
    return m


def laufen(exe, ordner, karte, stufe, ausgabe):
    basis = os.path.join(ordner, 'spiel')
    os.makedirs(os.path.join(basis, 'base'), exist_ok=True)
    args = [exe, '-v'] + SPIEL + ['-fs_basepath', basis + os.sep] + STUFEN[stufe] + ['maps/%s.map' % karte]
    with open(ausgabe, 'ab') as f:
        p = subprocess.run(args, cwd=ordner, stdout=f, stderr=subprocess.STDOUT, creationflags=IDLE)
    return p.returncode


def ordner_vorbereiten(ordner, daten, karte, stufe, eingabe):
    if os.path.exists(ordner):
        shutil.rmtree(ordner)
    os.makedirs(os.path.join(ordner, 'maps'))
    shutil.copyfile(os.path.join(daten, karte + '.map'), os.path.join(ordner, 'maps', karte + '.map'))
    for ext, quelle in (eingabe or {}).items():
        shutil.copyfile(quelle, os.path.join(ordner, 'maps', karte + ext))


def eingabe_fuer(daten, karte, stufe, vorher):
    if stufe == 'vis':
        return {'.bsp': os.path.join(daten, karte + '.nrc.bsp'), '.prt': os.path.join(daten, karte + '.nrc.prt'),
                '.srf': os.path.join(daten, karte + '.nrc.srf')}
    if stufe == 'light':
        return {'.bsp': vorher, '.srf': os.path.join(daten, karte + '.nrc.srf')}
    return None


def pruefen(exe, daten):
    erwartung = json.load(open(os.path.join(daten, 'erwartung.json'), encoding='utf-8'))
    fehler = 0
    with tempfile.TemporaryDirectory(prefix='bhed_q3vgl_') as tmp:
        for karte, stufen in KARTEN.items():
            vorher = None
            for stufe in stufen:
                o = os.path.join(tmp, karte, stufe)
                ordner_vorbereiten(o, daten, karte, stufe, eingabe_fuer(daten, karte, stufe, vorher))
                log = os.path.join(o, 'ausgabe.txt')
                code = laufen(exe, o, karte, stufe, log)
                bsp = os.path.join(o, 'maps', karte + '.bsp')
                if code != 0 or not os.path.exists(bsp):
                    print('FEHL  %s %s: Rueckgabe %d, .bsp %s' % (karte, stufe, code, 'da' if os.path.exists(bsp) else 'fehlt'))
                    print(open(log, encoding='latin-1').read()[-2000:])
                    fehler += 1
                    break
                ist = merkmale(bsp)
                soll = erwartung['karten'][karte][stufe]['erwartet']
                pflicht = sorted(pflicht_fuer(karte, stufe, soll))
                fehlt = sorted(PFLICHT[stufe] - set(soll) - {'datei:.prt', 'datei:.lin'})
                for n in fehlt:   # eine Pflicht ohne Erwartung waere ein stiller Durchlauf
                    print('FEHL  %s %s: keine Erwartung fuer %s (erwartung.json neu schreiben)' % (karte, stufe, n))
                anders = [n for n in pflicht if ist.get(n) != soll[n]]
                for n in anders:
                    print('FEHL  %s %s: %s ist %s, NRC %s' % (karte, stufe, n, ist.get(n), soll[n]))
                hinweis = [n for n in soll if n not in pflicht and ist.get(n) != soll[n]]
                fehler += len(anders) + len(fehlt)
                print('%s %-5s Pflicht %d/%d wie NRC, dazu Byte-gleich %d/%d%s' % (
                    karte, stufe, len(pflicht) - len(anders), len(pflicht), len(soll) - len(pflicht) - len(hinweis),
                    len(soll) - len(pflicht), (' (anders: %s)' % ', '.join(hinweis)) if hinweis else ''))
                vorher = bsp
    print('q3map2_vergleich: %d Abweichung(en)' % fehler)
    return 0 if fehler == 0 else 1


def erwartung_schreiben(nrc, uns, daten, laeufe):
    ergebnis = {'werkzeug': 'NetRadiant-Custom 20260114 q3map2.exe (2.5.17n-git-68ecbed)',
                'schalter': {s: SPIEL + a for s, a in STUFEN.items()}, 'laeufe': laeufe, 'karten': {}}
    tmp = tempfile.mkdtemp(prefix='bhed_q3erw_')
    try:
        for karte, stufen in KARTEN.items():
            ergebnis['karten'][karte] = {}
            vorher_uns = None
            for stufe in stufen:
                if stufe == 'vis':
                    # NRCs -bsp-Ergebnis wird Eingabe von -vis (und Testdatei)
                    o = os.path.join(tmp, karte, 'nrc_bsp')
                    ordner_vorbereiten(o, daten, karte, 'bsp', None)
                    laufen(nrc, o, karte, 'bsp', os.path.join(o, 'ausgabe.txt'))
                    shutil.copyfile(os.path.join(o, 'maps', karte + '.bsp'), os.path.join(daten, karte + '.nrc.bsp'))
                    for ext in ('.prt', '.srf'):
                        shutil.copyfile(os.path.join(o, 'maps', karte + ext), os.path.join(daten, karte + '.nrc' + ext))
                werte = {'nrc': [], 'uns': []}
                for wer, exe in (('nrc', nrc), ('uns', uns)):
                    for n in range(laeufe):
                        o = os.path.join(tmp, karte, stufe, '%s%d' % (wer, n))
                        eingabe = eingabe_fuer(daten, karte, stufe, vorher_uns)
                        ordner_vorbereiten(o, daten, karte, stufe, eingabe)
                        code = laufen(exe, o, karte, stufe, os.path.join(o, 'ausgabe.txt'))
                        if code != 0:
                            raise SystemExit('%s %s %s: Rueckgabe %d' % (wer, karte, stufe, code))
                        werte[wer].append(merkmale(os.path.join(o, 'maps', karte + '.bsp')))
                # Eingabe fuer -light: unser -vis-Ergebnis (gleich NRCs, siehe Pruefung)
                vorher_uns = os.path.join(tmp, karte, stufe, 'uns0', 'maps', karte + '.bsp')
                erwartet, schwankt, abweichung = {}, {}, {}
                for name in sorted(werte['nrc'][0]):
                    n_stabil = len({w.get(name) for w in werte['nrc']}) == 1
                    u_stabil = len({w.get(name) for w in werte['uns']}) == 1
                    if n_stabil and u_stabil:
                        if werte['nrc'][0][name] == werte['uns'][0].get(name):
                            erwartet[name] = werte['nrc'][0][name]
                        else:
                            abweichung[name] = (werte['nrc'][0][name], werte['uns'][0].get(name))
                    else:
                        schwankt[name] = 'NRC %s, uns %s' % ('stabil' if n_stabil else 'schwankt', 'stabil' if u_stabil else 'schwankt')
                # Bei beiden stabil, aber verschieden: bei einem Pflicht-Merkmal ein
                # Fehler; sonst die Zeiger-Reihenfolge (jedes Programm hat sein eigenes
                # Heap-Muster) - festgehalten, aber nicht erwartet.
                pflicht = pflicht_fuer(karte, stufe, werte['nrc'][0])
                fehler = {n: x for n, x in abweichung.items() if n in pflicht}
                for n in abweichung:
                    if n not in fehler:
                        schwankt[n] = 'NRC und uns je stabil, aber verschieden (Reihenfolge der Shader-Zeiger)'
                ergebnis['karten'][karte][stufe] = {'erwartet': erwartet, 'schwankt': schwankt}
                print('%s %-5s erwartet %d (Pflicht %d), nicht erwartet %d (%s), Pflicht abweichend %d %s' % (
                    karte, stufe, len(erwartet), len(pflicht & set(erwartet)), len(schwankt), ', '.join(sorted(schwankt)),
                    len(fehler), fehler or ''))
                if fehler:
                    raise SystemExit('Pflicht-Merkmale weichen ab - keine Erwartung geschrieben')
        with open(os.path.join(daten, 'erwartung.json'), 'w', encoding='utf-8', newline='\n') as f:
            json.dump(ergebnis, f, indent=1, sort_keys=True)
            f.write('\n')
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    return 0


def main(argv):
    if len(argv) >= 3 and argv[0] == 'pruefen':
        return pruefen(argv[1], argv[2])
    if len(argv) >= 4 and argv[0] == 'erwartung':
        laeufe = int(argv[argv.index('--laeufe') + 1]) if '--laeufe' in argv else 6
        return erwartung_schreiben(argv[1], argv[2], argv[3], laeufe)
    print(__doc__)
    return 2


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
