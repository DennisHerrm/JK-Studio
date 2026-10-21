# -*- coding: utf-8 -*-
"""Erzeugt gui/karte_icons.cpp: die Symbole von NetRadiant-Custom als ein
Bildstreifen (PNG im Programm) fuer den Karten-Editor.

Quelle sind NRCs Bitmaps (setup/data/tools/bitmaps im NRC-Quelltext, GPL v2,
wie das Programm): die SVGs werden mit PyMuPDF auf GROESSE x GROESSE
gerastert, die drei PNGs (notex, shadernotex, nomodel) verkleinert. Das
Ergebnis ist ein Streifen in einer Zeile; der Index steht in kKarteIcons
(Name ohne Endung -> Spalte).

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


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    ordner = sys.argv[1]
    namen = sorted(n for n in os.listdir(ordner) if n.endswith(('.svg', '.png')) and n != 'splash.svg')
    bilder = []
    for n in namen:
        p = os.path.join(ordner, n)
        if n.endswith('.svg'):
            d = pymupdf.open(p)
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
    daten = puffer.getvalue()
    z = ['// Erzeugt von tools/gen_karte_icons.py - nicht von Hand aendern.',
         '//',
         '// Die Symbole von NetRadiant-Custom (setup/data/tools/bitmaps, GNU GPL v2),',
         '// gerastert auf %dx%d, als ein PNG-Streifen in einer Zeile.' % (GROESSE, GROESSE),
         '#include "karte_icons.h"',
         '',
         'namespace bhed::gui::karteneditor {',
         '',
         'const unsigned char kKarteIconPng[] = {']
    for i in range(0, len(daten), 24):
        z.append('    ' + ','.join(str(b) for b in daten[i:i + 24]) + ',')
    z.append('};')
    z.append('const std::size_t kKarteIconPngGroesse = %d;' % len(daten))
    z.append('const int kKarteIconGroesse = %d;' % GROESSE)
    z.append('const KarteIcon kKarteIcons[] = {')
    for i, (n, _) in enumerate(bilder):
        z.append('    {"%s", %d},' % (n, i))
    z.append('};')
    z.append('const std::size_t kKarteIconAnzahl = %d;' % len(bilder))
    z.append('')
    z.append('}  // namespace bhed::gui::karteneditor')
    open(ZIEL, 'w', encoding='utf-8', newline='\n').write('\n'.join(z) + '\n')
    print('%d Symbole, PNG %d Byte -> %s' % (len(bilder), len(daten), ZIEL))


if __name__ == '__main__':
    main()
