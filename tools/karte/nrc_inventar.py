# -*- coding: utf-8 -*-
"""Checkliste aller Menues, Werkzeugknoepfe, Befehle, Dialoge, Einstellungs-
seiten und Plugins von NetRadiant-Custom (NRC) - und ihr Stand im Karten-Editor.

Gewuenscht (shank, 21.10.2026): "jeden Button und seine Funktion nachbauen".
Die Liste entsteht aus dem NRC-Quelltext (radiant/*.cpp, plugins/*/*.cpp);
der Stand jedes Eintrags steht in der Markdown-Datei und bleibt beim
Neuerzeugen erhalten (Schluessel: Abschnitt, Art, Beschriftung, Befehl, Datei).

Stand je Eintrag (Markdown-Kaestchen):
  [ ]  noch nicht nachgebaut
  [~]  sichtbar, aber ausgegraut (Funktion kommt spaeter; die Notiz hinter
       " -- " nennt die Stufe)
  [x]  nachgebaut und getestet

Aufruf:
  python tools/karte/nrc_inventar.py erzeugen <nrc-quelltext> tools/karte/nrc_inventar.md
  python tools/karte/nrc_inventar.py pruefen tools/karte/nrc_inventar.md
  python tools/karte/nrc_inventar.py befehle tools/karte/nrc_inventar.md   (Befehlsnamen, je Zeile einer)
  python tools/karte/nrc_inventar.py setzen tools/karte/nrc_inventar.md <stand> <abschnitt> <muster> [notiz]
      setzt den Stand ([x], [~] oder [ ]; Kurzform x, ~, -) aller Eintraege des
      Abschnitts ("Menues", "Werkzeugleisten", "Registrierte Befehle", ...;
      "*" = alle), deren Text auf den regulaeren Ausdruck <muster> passt.

Gegenueber dem ersten Stand (Stufe 1a, Scratchpad) zaehlen auskommentierte
Zeilen ("//"), Kommentarbloecke und "#if 0"-Bloecke nicht mehr mit (dort
standen z. B. RedisperseRows mit Ctrl+E, MakeOverlayPatch mit Y und
PatchDenseCylinder - alle in NRC abgeschaltet), und Menuepunkte mit eigenem
Rueckruf statt Befehlsnamen ("Set 2D Background image...", "Bug report",
"About") sind dabei.
"""
import glob
import os
import re
import sys

KAESTCHEN = re.compile(r'^(\s*)- \[([ x~])\] (.*)$')


def ohne_kommentare(t):
    """Kommentare und #if-0-Bloecke durch Leerzeichen ersetzen (Zeilen bleiben)."""
    out = []
    i, n = 0, len(t)
    zustand = None  # None, '"', "'", '//', '/*'
    while i < n:
        c = t[i]
        if zustand is None:
            if t.startswith('//', i):
                zustand = '//'
                out.append('  ')
                i += 2
                continue
            if t.startswith('/*', i):
                zustand = '/*'
                out.append('  ')
                i += 2
                continue
            if c in '"\'':
                zustand = c
            out.append(c)
            i += 1
        elif zustand in '"\'':
            out.append(c)
            if c == '\\' and i + 1 < n:
                out.append(t[i + 1])
                i += 2
                continue
            if c == zustand or c == '\n':
                zustand = None
            i += 1
        elif zustand == '//':
            if c == '\n':
                zustand = None
                out.append(c)
            else:
                out.append(' ')
            i += 1
        else:  # '/*'
            if t.startswith('*/', i):
                zustand = None
                out.append('  ')
                i += 2
                continue
            out.append('\n' if c == '\n' else ' ')
            i += 1
    s = ''.join(out)
    # #if 0 ... #endif (verschachtelte #if mitgezaehlt) ausblenden
    zeilen = s.split('\n')
    tiefe = 0
    for k, z in enumerate(zeilen):
        st = z.strip()
        if tiefe == 0:
            if re.match(r'#\s*if\s+0\b', st):
                tiefe = 1
                zeilen[k] = ' ' * len(z)
        else:
            if re.match(r'#\s*if', st):
                tiefe += 1
            elif re.match(r'#\s*endif', st):
                tiefe -= 1
            elif tiefe == 1 and re.match(r'#\s*else', st):
                tiefe = 0
            zeilen[k] = ' ' * len(z)
    return '\n'.join(zeilen)


TOK = re.compile(r'''
 (?P<fn>^(?:void|QMenu\s*\*|static\s+void)\s+(?P<fname>\w+)\s*\([^;{]*\)\s*\{) |
 (?P<auf>\{) | (?P<zu>\}) |
 (?P<sub>(?P<subvar>\w+)\s*=\s*(?P<par>\w+)\s*->\s*addMenu\(\s*"(?P<sublabel>[^"]*)"\s*\)) |
 (?P<top>(?P<topvar>\w+)\s*=\s*menubar->addMenu\(\s*"(?P<toplabel>[^"]*)"\s*\)) |
 (?P<item>create_(?P<chk>check_)?menu_item_with_mnemonic\(\s*(?P<ivar>\w+)\s*,\s*"(?P<ilabel>[^"]*)"\s*,\s*(?:"(?P<icmd>[^"]*)"|(?P<icb>make\w*Call\w*|\w+Caller)))|
 (?P<sep>(?P<svar>\w+)\s*->\s*addSeparator\(\s*\)) |
 (?P<tb>toolbar_append_(?P<tbk>button|toggle_button)\(\s*\w+\s*,\s*"(?P<tbtip>[^"]*)"\s*,\s*"(?P<tbicon>[^"]*)"\s*,\s*"(?P<tbcmd>[^"]*)") |
 (?P<reg>Global(?P<regk>Commands|Toggles|KeyEvents|Shortcuts)_insert\(\s*"(?P<regname>[^"]+)"(?P<regrest>[^;]*);) |
 (?P<filt>add_filter_command\(\s*\w+\s*,\s*"(?P<filtname>[^"]+)"(?P<filtrest>[^;]*);) |
 (?P<titel>setWindowTitle\(\s*"(?P<titeltext>[^"]+)"\s*\)) |
 (?P<seite>createPage\(\s*"(?P<seitebaum>[^"]+)"\s*,\s*"(?P<seitetitel>[^"]+)"\s*\)) |
 (?P<feld>page\.append(?P<feldart>CheckBox|Combo|Spinner|Entry|Slider|Radio|RadioIcons|PathEntry)\((?P<feldrest>[^;]*);)
''', re.X | re.M | re.S)


def kopf_vor(t, pos):
    # Text der Anweisung vor pos (seit dem letzten ; { oder }).
    vor = t[:pos]
    k = max(vor.rfind(';'), vor.rfind('{'), vor.rfind('}'))
    return re.sub(r'\s+', ' ', vor[k + 1:]).strip()


def bedingung_gilt(kopf):
    # Gilt eine if-Bedingung fuer Jedi Academy / Jedi Outcast (.game:
    # q3map2_type ja/jk2, gametype "ja"/"jk2", keine WADs)? None: haengt am
    # Layout oder am Laufzeitzustand - beide Zweige bleiben, mit Vermerk.
    if 'doom3' in kopf:
        return '!=' in kopf
    if 'game_has_killConnect' in kopf:
        return False
    if 'TextureBrowser::wads' in kopf:
        return '!TextureBrowser::wads' in kopf
    return None


def gilt(stapel):
    # stapel: Liste von (art, kopf); art 'if', 'else' (kopf = das if davor) oder ''.
    for art, kopf in stapel:
        b = bedingung_gilt(kopf) if art in ('if', 'else') else None
        if art == 'if' and b is False:
            return False
        if art == 'else' and b is True:
            return False
    return True


def vermerk(stapel):
    teile = []
    for a, k in stapel:
        if a in ('if', 'else') and bedingung_gilt(k) is None and re.search(r'[Ss]tyle|builtInGroupDialog', k):
            teile.append(k if a == 'if' else 'nicht (' + k + ')')
    return (' [wenn: ' + ' und '.join(teile) + ']') if teile else ''


# Plugins aus contrib/ (nur die fuer JA/JK2 im NRC-Paket mitgelieferten).
# Quelle: contrib/bobtoolz/bobToolz-GTK.cpp:52, contrib/prtview/prtview.cpp:164,
# contrib/brushexport, contrib/meshtex, contrib/shaderplug, contrib/sunplug.
PLUGINS = [
    ('bobToolz', ['About...', 'Stair Builder...', 'Aperture Door...', 'Door Builder...', 'Find Duplicates',
                  'Intersect...', 'Make Chain...', 'Path Plotter...', 'Reset Textures...', 'PitOMatic',
                  'Vis Viewer', 'Brush Cleanup', 'Polygon Builder', 'Caulk Selection', 'Explode Entity',
                  'Tree Planter', 'Drop Entity', 'Plot Splines', 'Merge Patches', 'Split patch',
                  'Split patch columns', 'Split patch rows', 'Flip terrain'], 'contrib/bobtoolz/bobToolz-GTK.cpp:52'),
    ('Portal Viewer', ['About Portal Viewer', 'Configure Portal Viewer', 'Toggle portals (3D)',
                       'Toggle portals (2D)', 'Unload .prt file', 'Load .prt file'], 'contrib/prtview/prtview.cpp:164'),
    ('brushexport', ['Export selected as Wavefront Object', 'About'], 'contrib/brushexport/plugin.cpp'),
    ('meshtex', ['Set S/T Scale...', 'S/T Align', 'General Function...', 'About'], 'contrib/meshtex/PluginUI.cpp'),
    ('shaderplug', ['Create tag file', 'About'], 'contrib/shaderplug/shaderplug.cpp'),
    ('sunplug', ['About...'], 'contrib/sunplug/sunplug.cpp'),
]


def sammeln(root):
    files = sorted(glob.glob(os.path.join(root, 'radiant', '*.cpp')) +
                   glob.glob(os.path.join(root, 'plugins', '*', '*.cpp')))
    menus, toolbars, cmds, dialoge, seiten, felder = [], [], [], [], [], []
    for f in files:
        roh = open(f, encoding='utf-8', errors='replace').read()
        t = ohne_kommentare(roh)
        rel = os.path.relpath(f, root).replace(os.sep, '/')
        fname = None
        depth = {}
        stapel = []          # (art, kopf) je offener Klammer
        letztes_if = {}      # Tiefe -> Kopf des zuletzt geschlossenen if
        for m in TOK.finditer(t):
            ln = t.count('\n', 0, m.start()) + 1
            if m.group('fn'):
                fname = m.group('fname')
                depth = {}
                stapel = [('', fname)]
                letztes_if = {}
                continue
            if m.group('auf'):
                kopf = kopf_vor(t, m.start())
                if re.match(r'if\b', kopf):
                    stapel.append(('if', kopf))
                elif re.match(r'else\s+if\b', kopf):
                    # else if: das vorige if gilt nicht, dieses schon - als
                    # ein Eintrag mit beiden Koepfen (der Vermerk nennt beide).
                    vorher = letztes_if.get(len(stapel), '')
                    stapel.append(('if', 'nicht (' + vorher + ') und ' + kopf[5:].strip()))
                elif re.match(r'else\b', kopf):
                    stapel.append(('else', letztes_if.get(len(stapel), '')))
                else:
                    stapel.append(('', kopf))
                continue
            if m.group('zu'):
                if stapel:
                    art, kopf = stapel.pop()
                    if art == 'if':
                        letztes_if[len(stapel)] = kopf
                continue
            # einzeilige Bedingung ohne Klammern (entity.cpp:735)
            einzeln = kopf_vor(t, m.start())
            eff = list(stapel)
            if re.match(r'if\b', einzeln):
                eff.append(('if', einzeln))
            if not gilt(eff):
                continue
            vm = vermerk(eff)
            if m.group('top'):
                depth[m.group('topvar')] = 0
                menus.append((rel, ln, fname, 0, 'MENU', m.group('toplabel'), ''))
            elif m.group('sub'):
                d = depth.get(m.group('par'), 0) + 1
                depth[m.group('subvar')] = d
                menus.append((rel, ln, fname, d, 'SUB', m.group('sublabel'), ''))
            elif m.group('item'):
                d = depth.get(m.group('ivar'), 0) + 1
                label = (m.group('ilabel') or '(dynamisch)') + vm
                cmd = m.group('icmd') or ''
                menus.append((rel, ln, fname, d, 'CHECK' if m.group('chk') else 'ITEM', label, cmd))
            elif m.group('sep'):
                if m.group('svar') in depth or m.group('svar') in ('menu', 'submenu'):
                    menus.append((rel, ln, fname, depth.get(m.group('svar'), 0) + 1, 'SEP', '---', ''))
            elif m.group('tb'):
                toolbars.append((rel, ln, fname, m.group('tbk'), m.group('tbtip') + vm, m.group('tbicon'),
                                 m.group('tbcmd')))
            elif m.group('reg') or m.group('filt'):
                rest = m.group('regrest') if m.group('reg') else m.group('filtrest')
                key = re.search(r'QKeySequence\(\s*"([^"]*)"\s*\)', rest)
                key2 = re.search(r'QKeySequence\(\s*([+]?Qt::[^)]*)\)', rest)
                taste = key.group(1) if key else (re.sub(r'\s+', ' ', key2.group(1)).strip() if key2 else '')
                if m.group('reg'):
                    cmds.append((rel, ln, m.group('regk'), m.group('regname'), taste))
                else:
                    cmds.append((rel, ln, 'Toggles', m.group('filtname'), taste))
            elif m.group('titel'):
                dialoge.append((rel, ln, m.group('titeltext')))
            elif m.group('seite'):
                seiten.append((rel, ln, m.group('seitebaum'), m.group('seitetitel')))
            elif m.group('feld'):
                rest = re.sub(r'\s+', ' ', m.group('feldrest')).strip()
                texte = re.findall(r'"([^"]*)"', rest)
                beschr = ' / '.join(x for x in texte[:2] if x) or rest[:60]
                felder.append((rel, ln, m.group('feldart'), beschr))
    return menus, toolbars, cmds, dialoge, seiten, felder


def schluessel_aus_zeile(abschnitt, text):
    """Schluessel fuer den Stand: Text ohne Zeilennummer und ohne Notiz."""
    text = text.split(' -- ')[0]
    text = re.sub(r':\d+\)', ')', text)
    return abschnitt + '|' + text.strip()


def stand_lesen(md):
    stand = {}
    if not os.path.exists(md):
        return stand
    abschnitt = ''
    for z in open(md, encoding='utf-8').read().split('\n'):
        if z.startswith('## '):
            abschnitt = z[3:].strip()
        m = KAESTCHEN.match(z)
        if m:
            text = m.group(3)
            notiz = text.split(' -- ', 1)[1] if ' -- ' in text else ''
            stand[schluessel_aus_zeile(abschnitt, text)] = (m.group(2), notiz)
    return stand


def erzeugen(root, md):
    alt = stand_lesen(md)
    menus, toolbars, cmds, dialoge, seiten, felder = sammeln(root)
    zeilen = []
    abschnitt = ['']

    def eintrag(ind, text):
        k = schluessel_aus_zeile(abschnitt[0], text)
        st, notiz = alt.get(k, (' ', ''))
        zeilen.append(f'{ind}- [{st}] {text}' + (f' -- {notiz}' if notiz else ''))

    def kopf(titel):
        abschnitt[0] = titel
        zeilen.append(f'\n## {titel}\n')

    n_items = sum(1 for x in menus if x[4] in ('ITEM', 'CHECK'))
    n_plug = sum(len(p[1]) for p in PLUGINS)
    zeilen.append('# NetRadiant-Custom: Checkliste fuer den Karten-Editor\n')
    zeilen.append('Erzeugt von `tools/karte/nrc_inventar.py` aus dem NRC-Quelltext (Stand master 2026); '
                  'der Stand je Eintrag wird beim Neuerzeugen uebernommen.\n')
    zeilen.append('Stand: `[x]` nachgebaut und getestet, `[~]` sichtbar und ausgegraut (Notiz: Stufe), '
                  '`[ ]` offen.\n')
    zeilen.append(f'Menuepunkte: {n_items}, Werkzeugknoepfe: {len(toolbars)}, registrierte Befehle: {len(cmds)}, '
                  f'Dialoge: {len(dialoge)}, Einstellungsseiten: {len(seiten)}, Einstellungsfelder: {len(felder)}, '
                  f'Plugin-Eintraege: {n_plug}\n')
    kopf('Menues')
    last = None
    for rel, ln, fn, d, k, label, cmd in menus:
        if fn != last:
            zeilen.append(f'\n### {fn} ({rel})\n')
            last = fn
        ind = '  ' * d
        if k == 'SEP':
            zeilen.append(f'{ind}* ---')
        elif k in ('MENU', 'SUB'):
            zeilen.append(f'{ind}* {k} {label}  ({rel}:{ln})')
        else:
            eintrag(ind, f'{k} {label}' + (f' -> `{cmd}`' if cmd else '') + f'  ({rel}:{ln})')
    kopf('Werkzeugleisten')
    last = None
    for rel, ln, fn, k, tip, icon, cmd in toolbars:
        if fn != last:
            zeilen.append(f'\n### {fn} ({rel})\n')
            last = fn
        tip1 = tip.replace('\n', ' / ').replace('\\n', ' / ').replace('\\t', '')
        eintrag('', f'{k} {tip1} | {icon} -> `{cmd}`  ({rel}:{ln})')
    kopf('Registrierte Befehle')
    zeilen.append('Name | Art | Standardtaste | Quelle\n')
    for rel, ln, k, name, key in sorted(cmds, key=lambda x: (x[3].lower(), x[2])):
        eintrag('', f'`{name}` | {k} | {key or "-"} | ({rel}:{ln})')
    kopf('Dialoge und Fenster')
    for rel, ln, titel in dialoge:
        eintrag('', f'"{titel}"  ({rel}:{ln})')
    kopf('Einstellungsseiten')
    for rel, ln, baum, titel in seiten:
        eintrag('', f'{baum}: "{titel}"  ({rel}:{ln})')
    kopf('Einstellungsfelder')
    for rel, ln, art, beschr in felder:
        eintrag('', f'{art} {beschr}  ({rel}:{ln})')
    kopf('Plugins')
    for name, punkte, quelle in PLUGINS:
        zeilen.append(f'\n### {name} ({quelle})\n')
        for p in punkte:
            eintrag('', f'{name}: {p}  ({quelle})')
    open(md, 'w', encoding='utf-8', newline='\n').write('\n'.join(zeilen) + '\n')
    print(f'{n_items} Menuepunkte, {len(toolbars)} Knoepfe, {len(cmds)} Befehle, {len(dialoge)} Dialoge, '
          f'{len(seiten)} Seiten, {len(felder)} Felder, {n_plug} Plugin-Eintraege')


def lesen(md):
    """(abschnitt, stand, text, notiz) je Eintrag; Fehler bei Zeilen ohne Kaestchen."""
    eintraege, fehler = [], []
    abschnitt = ''
    for nr, z in enumerate(open(md, encoding='utf-8').read().split('\n'), 1):
        if z.startswith('## '):
            abschnitt = z[3:].strip()
            continue
        if re.match(r'^\s*- ', z):
            m = KAESTCHEN.match(z)
            if not m:
                fehler.append(f'Zeile {nr}: Eintrag ohne Stand-Kaestchen: {z.strip()[:80]}')
                continue
            text = m.group(3)
            notiz = text.split(' -- ', 1)[1] if ' -- ' in text else ''
            if m.group(2) == '~' and not notiz:
                fehler.append(f'Zeile {nr}: ausgegraut ohne Notiz zur Stufe: {text[:80]}')
            eintraege.append((abschnitt, m.group(2), text, notiz))
    return eintraege, fehler


def pruefen(md):
    eintraege, fehler = lesen(md)
    if not eintraege:
        fehler.append('keine Eintraege gefunden')
    kopf = open(md, encoding='utf-8').read()
    m = re.search(r'Menuepunkte: (\d+), Werkzeugknoepfe: (\d+), registrierte Befehle: (\d+)', kopf)
    if not m:
        fehler.append('Kopfzeile mit den Zahlen fehlt')
    else:
        soll = {'Menues': int(m.group(1)), 'Werkzeugleisten': int(m.group(2)), 'Registrierte Befehle': int(m.group(3))}
        for ab, n in soll.items():
            ist = sum(1 for e in eintraege if e[0] == ab)
            if ist != n:
                fehler.append(f'{ab}: Kopf sagt {n}, gefunden {ist}')
    je = {}
    for ab, st, _, _ in eintraege:
        je.setdefault(ab, {' ': 0, '~': 0, 'x': 0})[st] += 1
    for ab, z in je.items():
        print(f'{ab}: {z["x"]} fertig, {z["~"]} ausgegraut, {z[" "]} offen')
    g = {k: sum(z[k] for z in je.values()) for k in ' ~x'}
    print(f'Gesamt: {g["x"]} fertig, {g["~"]} ausgegraut, {g[" "]} offen von {len(eintraege)}')
    for f in fehler:
        print('FEHLER: ' + f)
    return 1 if fehler else 0


def befehle(md):
    eintraege, _ = lesen(md)
    namen = []
    for ab, _, text, _ in eintraege:
        if ab == 'Registrierte Befehle':
            m = re.match(r'`([^`]+)`', text)
            if m and m.group(1) not in namen:
                namen.append(m.group(1))
    print('\n'.join(namen))
    return 0


def setzen(md, stand, abschnitt, muster, notiz):
    stand = {'x': 'x', '~': '~', '-': ' ', ' ': ' '}[stand]
    rx = re.compile(muster)
    zeilen = open(md, encoding='utf-8').read().split('\n')
    ab = ''
    n = 0
    for i, z in enumerate(zeilen):
        if z.startswith('## '):
            ab = z[3:].strip()
            continue
        m = KAESTCHEN.match(z)
        if not m or (abschnitt != '*' and ab != abschnitt):
            continue
        text = m.group(3).split(' -- ')[0]
        if not rx.search(text):
            continue
        neu = f'{m.group(1)}- [{stand}] {text}'
        if notiz:
            neu += f' -- {notiz}'
        zeilen[i] = neu
        n += 1
    open(md, 'w', encoding='utf-8', newline='\n').write('\n'.join(zeilen))
    print(f'{n} Eintraege gesetzt')
    return 0 if n else 1


if __name__ == '__main__':
    if len(sys.argv) == 4 and sys.argv[1] == 'erzeugen':
        erzeugen(sys.argv[2], sys.argv[3])
    elif len(sys.argv) == 3 and sys.argv[1] == 'pruefen':
        sys.exit(pruefen(sys.argv[2]))
    elif len(sys.argv) == 3 and sys.argv[1] == 'befehle':
        sys.exit(befehle(sys.argv[2]))
    elif len(sys.argv) in (6, 7) and sys.argv[1] == 'setzen':
        sys.exit(setzen(sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5], sys.argv[6] if len(sys.argv) == 7 else ''))
    else:
        print(__doc__)
        sys.exit(2)
