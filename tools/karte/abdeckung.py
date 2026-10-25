# -*- coding: utf-8 -*-
"""Abdeckung der NRC-Checkliste durch Wirkungspruefungen (Stufe 6).

Auftrag shank (25.10.2026): "dass er bitte schaut das alle buttons gehen".
Jeder Eintrag aus tools/karte/nrc_inventar.md (Menuepunkte, Werkzeugknoepfe,
registrierte Befehle, Dialoge, Einstellungsseiten und -felder, Plugin-
Eintraege, Mausbelegung) und jeder eigene Eintrag ausserhalb von NRC braucht
mindestens eine Pruefung im Selbsttest (Modus karteneditor), die seine
WIRKUNG prueft: Zustand vorher/nachher (Karte, Auswahl, Ansicht, Datei,
Einstellung). "Kein Absturz" allein zaehlt nicht.

Grundlage ist das Protokoll eines Selbsttest-Laufs: die Zeilen
"Knopftest OK: <Pruefung>" und "Knopftest FEHLER: <Pruefung>" (letzter_<lauf>.txt,
gekuerzt als tools/karte/abdeckung_lauf.txt im Repo). Je Eintrag nennt eine
Regel die Pruefungen (regulaere Ausdruecke ueber den Pruefnamen); der Eintrag
ist abgedeckt, wenn jede Anforderung mindestens eine OK-Pruefung hat und
keine passende Pruefung FEHLER meldete.

Die Pruefnamen der Stufe 6 (gui/selbsttest_karte6.inc) sind fest:
  "Wirkung <Befehl>: ..."        jeder Command/Toggle ueber ausfuehren()
  "Rueckgaengig <Befehl>: ..."   Undo/Redo byte-gleich (nur bei Kartenaenderung)
  "Robust <Befehl>: ..."         leere Auswahl, leere Karte, gemischte Auswahl
  "Taste <Text> = <Befehl>: ..." das Standardkuerzel ueber echte Tastenereignisse
  "Ereignis <Befehl>: ..."       KeyEvent (Taste halten)
  "Menue <Weg> = <Befehl>: ..."  der Menuepunkt ueber echte Mausklicks
  "Knopf <Befehl>: ..."          der Werkzeugknopf ueber echte Mausklicks
"""
import os
import re

# --- Begruendete Ausnahmen (Schluessel -> Grund) -----------------------------------
# Ein Eintrag hier braucht keine eigene Wirkungspruefung; der Grund sagt, wo
# die Wirkung stattdessen geprueft ist.
AUSNAHMEN = {
    'Dialog|Install Map Developer\'s Files':
        'ja.game/jk2.game haben keinen install-Ordner: DoInstallDevFilesDlg zeigt bei JA/JK2 nichts (Stand [~] seit Stufe 2)',
}

# --- Ruf-Menuepunkte ohne Befehlsnamen (MenueArt::Ruf in src/karte/menues.inc) ---
RUFE = {
    'Set 2D &Background image...': 'WXY_SetBackgroundImage',
    'Bug report': 'OpenBugReportURL',
    '&About': 'DoAbout',
}

# --- Ausdruecklich zugeordnete Pruefungen ------------------------------------------
# Schluessel -> Liste von Anforderungen; jede Anforderung ist ein regulaerer
# Ausdruck (re.match am Anfang des Pruefnamens) - oder eine Liste davon, von
# denen einer reichen muss.
EXPLIZIT = {}


def esc(t):
    return re.escape(t)


def schluessel(abschnitt, text):
    """(Schluessel, Anforderungen) eines Eintrags der Checkliste."""
    text = text.split(' -- ')[0]
    if abschnitt == 'Menues':
        m = re.match(r'(ITEM|CHECK) (.*?)(?: -> `([^`]*)`)?  \((\S+)\)$', text)
        if not m:
            return None, None
        label, cmd, quelle = m.group(2), m.group(3), m.group(4)
        if cmd:
            return 'Menue|' + cmd + '|' + quelle, [r'Menue .* = ' + esc(cmd) + r': ']
        if label in RUFE:
            return 'Menue|' + label, [r'Menue .* = ' + esc(RUFE[label]) + r': ']
        mru = re.match(r'&(\d)$', label)
        if mru:
            return 'Menue|MRU ' + mru.group(1), [r'Menue .* = MRU' + mru.group(1) + r': ']
        return 'Menue|' + label + '|' + quelle, None
    if abschnitt == 'Werkzeugleisten':
        m = re.search(r'-> `([^`]*)`', text)
        if not m:
            return None, None
        return 'Knopf|' + m.group(1), [r'Knopf ' + esc(m.group(1)) + r': ']
    if abschnitt == 'Registrierte Befehle':
        m = re.match(r'`([^`]+)` \| (\w+) \| (.*?) \|', text)
        if not m:
            return None, None
        name, art, taste = m.group(1), m.group(2), m.group(3)
        k = 'Befehl|' + name + '|' + art
        if art in ('Commands', 'Toggles'):
            return k, [r'Wirkung ' + esc(name) + r': ']
        if art == 'Shortcuts':
            return k, [r'Taste .* = ' + esc(name) + r': ']
        if art == 'KeyEvents':
            return k, [r'Ereignis ' + esc(name) + r': ']
        return k, None
    if abschnitt == 'Dialoge und Fenster':
        m = re.match(r'"(.*)"  \(', text)
        return ('Dialog|' + m.group(1), None) if m else (None, None)
    if abschnitt == 'Einstellungsseiten':
        m = re.match(r'(.*?): "(.*)"  \(', text)
        return ('Seite|' + m.group(2), None) if m else (None, None)
    if abschnitt == 'Einstellungsfelder':
        m = re.match(r'(\w+) (.*)  \((\S+)\)$', text)
        return ('Feld|' + m.group(3), None) if m else (None, None)
    if abschnitt == 'Plugins':
        m = re.match(r'(.*?): (.*) -> `([^`]*)`', text)
        if not m:
            return None, None
        cmd = m.group(3)
        return 'Plugin|' + cmd, [r'Wirkung ' + esc(cmd) + r': ', r'Menue .* = ' + esc(cmd) + r': ']
    if abschnitt == 'Mausbelegung':
        teile = [t.strip() for t in text.split(' | ')]
        if len(teile) < 6:
            return None, None
        test = teile[5][len('Test: '):] if teile[5].startswith('Test: ') else teile[5]
        namen = [t.strip() for t in test.split(';') if t.strip()]
        return 'Maus|' + teile[0] + '|' + teile[1], [esc(n) for n in namen]
    return None, None


# --- Eigene Eintraege (nicht in NRC) ----------------------------------------------
EIGENE = [
    # (Schluessel, Beschreibung, Anforderungen)
]


def protokoll_lesen(pfade):
    ok, fehler = [], []
    for p in pfade:
        for z in open(p, encoding='utf-8', errors='replace'):
            z = z.rstrip('\r\n')
            m = re.match(r'(?:.*?)Knopftest OK: (.*)$', z)
            if m:
                ok.append(m.group(1))
                continue
            m = re.match(r'(?:.*?)Knopftest FEHLER: (.*)$', z)
            if m:
                fehler.append(m.group(1))
    return ok, fehler


def bewerten(anforderungen, ok, fehler):
    """(Stand, Pruefungen): Stand 'OK', 'FEHLER' oder 'FEHLT'."""
    treffer = []
    stand = 'OK'
    for a in anforderungen:
        rx = [re.compile(x) for x in (a if isinstance(a, list) else [a])]
        gut = [n for n in ok if any(r.match(n) for r in rx)]
        schlecht = [n for n in fehler if any(r.match(n) for r in rx)]
        if schlecht:
            stand = 'FEHLER'
            treffer += ['FEHLER: ' + schlecht[0]]
        elif not gut:
            if stand == 'OK':
                stand = 'FEHLT'
            treffer += ['fehlt: ' + (a if isinstance(a, str) else ' | '.join(a))]
        else:
            treffer += [gut[0]] + (['(+%d)' % (len(gut) - 1)] if len(gut) > 1 else [])
    return stand, treffer


def abdeckung(eintraege, ok, fehler):
    """Je Eintrag (abschnitt, text) -> (abschnitt, schluessel, stand, pruefungen, grund)."""
    erg = []
    for abschnitt, text in eintraege:
        k, anf = schluessel(abschnitt, text)
        if k is None:
            erg.append((abschnitt, text, 'FEHLT', ['Eintrag nicht lesbar'], ''))
            continue
        if k in AUSNAHMEN:
            erg.append((abschnitt, k, 'AUSNAHME', [], AUSNAHMEN[k]))
            continue
        anf = EXPLIZIT.get(k, anf)
        if not anf:
            erg.append((abschnitt, k, 'FEHLT', ['keine Regel'], ''))
            continue
        stand, treffer = bewerten(anf, ok, fehler)
        erg.append((abschnitt, k, stand, treffer, ''))
    for k, beschreibung, anf in EIGENE:
        stand, treffer = bewerten(anf, ok, fehler)
        erg.append(('Eigene Eintraege', k, stand, treffer, beschreibung))
    return erg


def tabelle_schreiben(erg, pfad, laeufe):
    zeilen = ['# Abdeckung: jeder Eintrag mit Wirkungspruefung', '',
              'Erzeugt von `tools/karte/nrc_inventar.py abdeckung` aus `tools/karte/nrc_inventar.md` und dem Protokoll '
              + ', '.join('`%s`' % os.path.basename(x) for x in laeufe) + '. Nicht von Hand aendern.', '',
              '| Art | Eintraege | mit Wirkungspruefung | Ausnahme | fehlt | FEHLER |', '|---|---|---|---|---|---|']
    arten = []
    for e in erg:
        if e[0] not in arten:
            arten.append(e[0])
    for a in arten:
        z = [e for e in erg if e[0] == a]
        zeilen.append('| %s | %d | %d | %d | %d | %d |' % (a, len(z), sum(1 for e in z if e[2] == 'OK'), sum(1 for e in z if e[2] == 'AUSNAHME'),
                                                         sum(1 for e in z if e[2] == 'FEHLT'), sum(1 for e in z if e[2] == 'FEHLER')))
    zeilen.append('| **Summe** | %d | %d | %d | %d | %d |' % (len(erg), sum(1 for e in erg if e[2] == 'OK'), sum(1 for e in erg if e[2] == 'AUSNAHME'),
                                                            sum(1 for e in erg if e[2] == 'FEHLT'), sum(1 for e in erg if e[2] == 'FEHLER')))
    zeilen += ['', '## Begruendete Ausnahmen', '']
    for e in erg:
        if e[2] == 'AUSNAHME':
            zeilen.append('- `%s`: %s' % (e[1], e[4]))
    for a in arten:
        zeilen += ['', '## ' + a, '', '| Eintrag | Stand | Pruefung (Wirkung) |', '|---|---|---|']
        for e in erg:
            if e[0] != a:
                continue
            p = '; '.join(e[3]).replace('|', '\\|')
            if e[2] == 'AUSNAHME':
                p = e[4]
            zeilen.append('| `%s` | %s | %s |' % (e[1].replace('|', ' \\| '), e[2], p))
    open(pfad, 'w', encoding='utf-8', newline='\n').write('\n'.join(zeilen) + '\n')
