# -*- coding: utf-8 -*-
"""
Erzeugt include/bhed/changelog.h und src/changelog.cpp aus tools/changelog.txt.

Wie gen_i18n.py: Nicht-ASCII als ausdrueckliche UTF-8-Bytes, damit MSVC ohne
/utf-8 die chinesischen und japanischen Texte nicht in die ANSI-Codepage
umwandelt. Bricht ab, wenn einer Fassung eine der vier ersten Sprachen fehlt -
ein leerer Eintrag im Fenster faellt sonst erst beim Anwender auf. Die
weiteren Sprachen (tools/sprachen.py: es, cs) darf eine Fassung haben; fehlt
eine, steht nullptr in der Tabelle und das Fenster zeigt den englischen Text
(changelog::text) - so bleiben die alten Fassungen gueltig.
"""
import os
import re
import shutil
import sys

HIER = os.path.dirname(os.path.abspath(__file__))
WURZEL = os.path.dirname(HIER)
sys.path.insert(0, HIER)
import sprachen  # noqa: E402

SPRACHEN = list(sprachen.KURZ)              # Reihenfolge wie i18n::Language
PFLICHT = SPRACHEN[:sprachen.ALT]           # en, de, zh, ja: jede Fassung


def esc(s):
    out = []
    prev_hex = False
    for b in s.encode('utf-8'):
        if b == 0x5C:
            out.append('\\\\'); prev_hex = False
        elif b == 0x22:
            out.append('\\"'); prev_hex = False
        elif b == 0x0A:
            out.append('\\n'); prev_hex = False
        elif b < 0x80:
            c = chr(b)
            # Ein Hexzeichen direkt nach \xNN gehoerte sonst zur Zahl.
            if prev_hex and c in '0123456789abcdefABCDEF':
                out.append('""')
            out.append(c); prev_hex = False
        else:
            out.append('\\x%02x' % b); prev_hex = True
    return ''.join(out)


def lies(pfad=os.path.join(HIER, 'changelog.txt')):
    eintraege = []
    akt = None
    sprache = None
    for nr, zeile in enumerate(open(pfad, encoding='utf-8'), 1):
        z = zeile.rstrip('\n').rstrip('\r')
        if z.startswith('#') and not z.startswith('##'):
            continue
        m = re.match(r'^## (\S+) (\d{4}-\d{2}-\d{2})\s*$', z)
        if m:
            akt = {'fassung': m.group(1), 'datum': m.group(2), 'text': {}}
            eintraege.append(akt)
            sprache = None
            continue
        m = re.match(r'^\[(' + '|'.join(SPRACHEN) + r')\]\s*$', z)
        if m:
            if akt is None:
                sys.exit('Zeile %d: Sprache ohne Fassung' % nr)
            sprache = m.group(1)
            akt['text'][sprache] = []
            continue
        if not z.strip():
            continue
        if akt is None or sprache is None:
            sys.exit('Zeile %d: Text ausserhalb einer Fassung/Sprache: %s' % (nr, z))
        if not z.startswith('- '):
            sys.exit('Zeile %d: Punkte beginnen mit "- ": %s' % (nr, z))
        akt['text'][sprache].append(z[2:].strip())
    for e in eintraege:
        for s in PFLICHT:
            if not e['text'].get(s):
                sys.exit('Fassung %s: Sprache %s fehlt' % (e['fassung'], s))
    return eintraege


def main():
    eintraege = lies()
    h = '''// Erzeugt von tools/gen_changelog.py aus tools/changelog.txt - nicht von Hand aendern.
#pragma once

#include <cstddef>

namespace bhed::changelog {

struct Eintrag {
    const char* fassung;   // "rc583"
    const char* datum;     // "2026-10-11"
    // Je Sprache (Reihenfolge wie i18n::Language) die Punkte, durch '\\n'
    // getrennt, UTF-8; nullptr = diese Fassung hat die Sprache nicht.
    const char* text[%d];
};

// Neueste Fassung zuerst.
[[nodiscard]] const Eintrag* eintraege();
[[nodiscard]] std::size_t anzahl();
// Die Punkte einer Fassung in einer Sprache (Index wie i18n::Language) -
// englisch, wenn die Fassung sie nicht hat.
[[nodiscard]] const char* text(const Eintrag& e, int sprache);

}  // namespace bhed::changelog
''' % len(SPRACHEN)
    c = ['// Erzeugt von tools/gen_changelog.py aus tools/changelog.txt - nicht von Hand aendern.',
         '#include "bhed/changelog.h"', '', '#include <cstddef>', '', 'namespace bhed::changelog {', '', 'namespace {',
         'const Eintrag kEintraege[] = {']
    for e in eintraege:
        # je Zeile ein eigenes Literal: MSVC bricht ab 16380 Zeichen je Literal ab (C2026)
        teile = [' '.join('"%s"' % esc(z + ('\n' if i < len(e['text'][s]) - 1 else ''))
                          for i, z in enumerate(e['text'][s])) if s in e['text'] else 'nullptr'
                 for s in SPRACHEN]
        c.append('    {"%s", "%s", {%s}},' % (e['fassung'], e['datum'], ',\n      '.join(teile)))
    c += ['};', '}  // namespace', '',
          'const Eintrag* eintraege() { return kEintraege; }',
          'std::size_t anzahl() { return sizeof(kEintraege) / sizeof(kEintraege[0]); }', '',
          'const char* text(const Eintrag& e, int sprache) {',
          '    constexpr int kSprachen = static_cast<int>(sizeof(e.text) / sizeof(e.text[0]));',
          '    return sprache >= 0 && sprache < kSprachen && e.text[sprache] != nullptr ? e.text[sprache] : e.text[0];',
          '}', '',
          '}  // namespace bhed::changelog', '']
    open(os.path.join(WURZEL, 'include', 'bhed', 'changelog.h'), 'w', encoding='utf-8', newline='\n').write(h)
    open(os.path.join(WURZEL, 'src', 'changelog.cpp'), 'w', encoding='utf-8', newline='\n').write('\n'.join(c))
    print('Changelog erzeugt: %d Fassungen x %d Sprachen' % (len(eintraege), len(SPRACHEN)))


def selbsttest():
    """ctest changelog_sprachen: eine neue Fassung mit [es], eine alte ohne -
    beide gelesen, die alte ohne Abbruch; fehlt eine der vier ersten
    Sprachen, bricht es ab."""
    import tempfile
    ordner = tempfile.mkdtemp(prefix='bhed_changelog_')
    pfad = os.path.join(ordner, 'changelog.txt')
    try:
        with open(pfad, 'w', encoding='utf-8', newline='\n') as f:
            f.write('## rc999 2026-12-01\n[en]\n- New\n[de]\n- Neu\n[zh]\n- \u65b0\n[ja]\n- \u65b0\n[es]\n- Nuevo\n'
                    '## rc998 2026-11-01\n[en]\n- Old\n[de]\n- Alt\n[zh]\n- \u65e7\n[ja]\n- \u65e7\n')
        e = lies(pfad)
        ok = (len(e) == 2 and e[0]['text'].get('es') == ['Nuevo'] and 'cs' not in e[0]['text'] and
              'es' not in e[1]['text'] and e[1]['text']['en'] == ['Old'])
        print('  %s  neue Fassung mit [es], alte ohne: gelesen' % ('ok  ' if ok else 'FEHL'))
        with open(pfad, 'w', encoding='utf-8', newline='\n') as f:
            f.write('## rc999 2026-12-01\n[en]\n- New\n[zh]\n- x\n[ja]\n- x\n[es]\n- Nuevo\n')
        try:
            lies(pfad)
            abbruch = False
        except SystemExit:
            abbruch = True
        print('  %s  ohne [de] bricht ab' % ('ok  ' if abbruch else 'FEHL'))
        return 0 if ok and abbruch else 1
    finally:
        shutil.rmtree(ordner, ignore_errors=True)


if __name__ == '__main__':
    if '--selbsttest' in sys.argv:
        sys.exit(selbsttest())
    main()
