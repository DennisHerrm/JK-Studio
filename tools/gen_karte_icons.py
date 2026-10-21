# -*- coding: utf-8 -*-
"""Erzeugt gui/karte_icons.cpp: die Symbole von NetRadiant-Custom als ein
Bildstreifen (PNG im Programm) fuer den Karten-Editor.

Quelle sind NRCs Bitmaps (setup/data/tools/bitmaps im NRC-Quelltext, GPL v2,
wie das Programm): die SVGs werden mit PyMuPDF auf GROESSE x GROESSE
gerastert, die drei PNGs (notex, shadernotex, nomodel) verkleinert. Das
Ergebnis ist ein Streifen in einer Zeile; der Index steht in kKarteIcons
(Name ohne Endung -> Spalte).

Zwei Streifen wie NRC (set_icon_theme, theme.cpp:55): die dunklen Bitmaps fuer
dunkle Farbgebungen und die hellen fuer helle - NRC erzeugt sie in
Bitmaps_generateLight (libs/gtkutil/image.cpp:42-74), indem es in den SVGs
"#c0c0c0" durch "#808080", "#8E8E8F" durch "#656565" und "#f2d230" durch
"#ccb128" ersetzt.

  python -I tools/gen_karte_icons.py <NRC>/setup/data/tools/bitmaps
"""
import io
import os
import sys

import pymupdf
from PIL import Image

GROESSE = 48
HIER = os.path.dirname(os.path.abspath(__file__))
ZIEL = os.path.join(os.path.dirname(HIER), 'gui', 'karte_icons.cpp')


HELL = (('#c0c0c0', '#808080'), ('#8E8E8F', '#656565'), ('#f2d230', '#ccb128'))


def streifen_bauen(ordner, namen, hell):
    bilder = []
    for n in namen:
        p = os.path.join(ordner, n)
        if n.endswith('.svg'):
            daten = open(p, 'rb').read()
            if hell:
                for alt, neu in HELL:
                    daten = daten.replace(alt.encode(), neu.encode())
            d = pymupdf.open(stream=daten, filetype='svg')
            s = d[0]
            pix = s.get_pixmap(matrix=pymupdf.Matrix(GROESSE / s.rect.width, GROESSE / s.rect.height), alpha=True)
            im = Image.frombytes('RGBA', (pix.width, pix.height), pix.samples)
        else:
            im = Image.open(p).convert('RGBA')
        if im.size != (GROESSE, GROESSE):
            im = im.resize((GROESSE, GROESSE), Image.LANCZOS)
        bilder.append((os.path.splitext(n)[0], im))
    streifen = Image.new('RGBA', (GROESSE * len(bilder), GROESSE), (0, 0, 0, 0))
    for i, (_, im) in enumerate(bilder):
        streifen.paste(im, (i * GROESSE, 0))
    puffer = io.BytesIO()
    streifen.save(puffer, 'PNG', optimize=True)
    return bilder, puffer.getvalue()


def feld(z, name, daten):
    z.append('const unsigned char %s[] = {' % name)
    for i in range(0, len(daten), 24):
        z.append('    ' + ','.join(str(b) for b in daten[i:i + 24]) + ',')
    z.append('};')
    z.append('const std::size_t %sGroesse = %d;' % (name, len(daten)))


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    ordner = sys.argv[1]
    namen = sorted(n for n in os.listdir(ordner) if n.endswith(('.svg', '.png')) and n != 'splash.svg')
    bilder, daten = streifen_bauen(ordner, namen, False)
    _, daten_hell = streifen_bauen(ordner, namen, True)
    z = ['// Erzeugt von tools/gen_karte_icons.py - nicht von Hand aendern.',
         '//',
         '// Die Symbole von NetRadiant-Custom (setup/data/tools/bitmaps, GNU GPL v2),',
         '// gerastert auf %dx%d, als PNG-Streifen in einer Zeile: dunkel und hell.' % (GROESSE, GROESSE),
         '#include "karte_icons.h"',
         '',
         'namespace bhed::gui::karteneditor {',
         '']
    feld(z, 'kKarteIconPng', daten)
    feld(z, 'kKarteIconPngHell', daten_hell)
    z.append('const int kKarteIconGroesse = %d;' % GROESSE)
    z.append('const KarteIcon kKarteIcons[] = {')
    for i, (n, _) in enumerate(bilder):
        z.append('    {"%s", %d},' % (n, i))
    z.append('};')
    z.append('const std::size_t kKarteIconAnzahl = %d;' % len(bilder))
    z.append('')
    z.append('}  // namespace bhed::gui::karteneditor')
    open(ZIEL, 'w', encoding='utf-8', newline='\n').write('\n'.join(z) + '\n')
    print('%d Symbole, PNG %d + %d Byte -> %s' % (len(bilder), len(daten), len(daten_hell), ZIEL))


if __name__ == '__main__':
    main()
