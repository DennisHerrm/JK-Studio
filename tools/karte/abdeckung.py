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


def _dialog(titel, *knoepfe):
    return [r'Dialog ' + re.escape(titel) + ' > ' + re.escape(k) + ': ' for k in knoepfe]


EXPLIZIT.update({
    'Dialog|Build Menu': _dialog('Build Menu', 'OK', 'Cancel', 'Reset'),
    'Dialog|Mapped Commands': _dialog('Mapped Commands', 'Edit', 'Clear', 'Reset', 'Reset All', 'Reset All > Yes'),
    'Dialog|CSG Tool': _dialog('CSG Tool', 'Shrink', 'Expand', 'Diagonal', 'Wrap', 'Extrude', 'Pull', '-faces', '+faces', '-proj', '-cam',
                               'Caulk', 'Remove inner', 'Offset up', 'Offset down'),
    'Dialog|Entity List': _dialog('Entity List', 'Auto focus') + [re.escape('Umschalt+L: Entity List offen')],
    'Dialog|Q3Map debug window': [re.escape('Q3Map debug window offen: "Entity leaked"'),
                                  re.escape('Q3Map debug window: die Zeile waehlt die Entitaet')],
    'Dialog|Find / Replace Texture(s)': _dialog('Find / Replace Texture(s)', 'Apply', 'Close', 'Within selected brushes only'),
    'Dialog|Entities': [re.escape('Licht gewaehlt: Klassenliste auf light'), re.escape('Value 500 + Enter: light = 500'),
                        re.escape('Delete Key: light weg'), re.escape('Clear All: nur classname und origin bleiben'),
                        re.escape('Spawnflag linear: spawnflags 1')],
    'Dialog|Arbitrary sides': _dialog('Arbitrary sides', 'OK', 'Cancel', 'Truncate'),
    'Dialog|About NetRadiant': _dialog('About NetRadiant', 'OK', 'Changelog', 'About Qt'),
    'Dialog|Set Light intensity': _dialog('Set Light intensity', 'OK', 'Cancel', "Don't show"),
    'Dialog|Engine Path Configuration': _dialog('Engine Path Configuration', 'OK') + [re.escape('OK: Engine Path engine12 (setEnginePath)')],
    'Dialog|Camera': [r'Wirkung ToggleCamera: ', r'Menue .* = ToggleCamera: '],
    'Dialog|Map Info': _dialog('Map Info', 'Spalte Anzahl') + [re.escape('Esc: Map Info zu')],
    'Dialog|Find Brush': _dialog('Find Brush', 'Find', 'Close'),
    'Dialog|Patch density': _dialog('Patch density', 'OK', 'Cancel'),
    'Dialog|Patch deform': _dialog('Patch deform', 'OK', 'Cancel'),
    'Dialog|Cap': _dialog('Cap', 'OK', 'Cancel'),
    'Dialog|Patch thicken': _dialog('Patch thicken', 'OK', 'Cancel', 'Side walls'),
    'Dialog|Global Preferences': _dialog('Global Preferences', 'OK'),
    'Dialog|NetRadiant Preferences': _dialog('NetRadiant Preferences', 'Clean', 'Clean > Yes') + [r'Seite .*: OK uebernimmt, Cancel verwirft'],
    'Dialog|Arbitrary rotation': _dialog('Arbitrary rotation', 'OK', 'Apply', 'Cancel'),
    'Dialog|Arbitrary scale': _dialog('Arbitrary scale', 'OK', 'Apply', 'Cancel'),
    'Dialog|Surface Inspector': _dialog('Surface Inspector', 'Reset', 'Fit', 'Width', 'Height', 'Axial', 'Ortho', 'Cam', 'Natural', 'Match Grid'),
    # Seiten ohne eigenen Feldertest (Spiel/Pfade laden das Spiel neu): die Pruefungen der Stufe 2
    'Seite|Path Settings': [re.escape('Seite Paths: Engine Path = der Ordner ueber dem ersten Spielordner'),
                            re.escape('OK: Engine Path engine12 (setEnginePath)')],
    'Feld|radiant/preferences.cpp:259': [re.escape('OK: Frage "Restart is required" mit "Selected Game"'),
                                         re.escape('Yes (Radiant_Restart): Jedi Outcast mit data/karte/jk2')],
    'Feld|radiant/gtkdlgs.cpp:229': [re.escape('Seite Game: Select mod (basegame)'), re.escape('Custom JA modification: fs_game leer und frei')],
    'Feld|radiant/gtkdlgs.cpp:241': [re.escape('OK: fs_game testmod11, mp, Extra Resource Path - das Spiel neu geladen, gespeichert')],
    'Feld|radiant/gtkdlgs.cpp:260': [re.escape('Mapping mode mp: sp_entities.def uebergangen')],
    'Feld|radiant/mainframe.cpp:398': [re.escape('OK: Engine Path engine12 (setEnginePath)')],
    'Feld|radiant/mainframe.cpp:407': [re.escape('Extra Resource Path: maps/zusatz11.txt im Spiel')],
})

# File > Exit beendet das Programm: geprueft ist der Weg davor (Stufe 4, ConfirmModified)
AUSNAHMEN.update({
    'Befehl|Exit|Commands': 'beendet das Programm - Stufe 4 prueft ConfirmModified (Save/Discard/Cancel) und das Beenden: siehe Pruefungen "Exit"',
    'Menue|Exit|radiant/mainframe.cpp:889': 'wie der Befehl Exit (Stufe 4 prueft den Menuepunkt mit der Rueckfrage)',
    'Feld|radiant/textures.cpp:818': 'Hardware Texture Compression: D3D11 meldet weder OpenGL- noch S3TC-Kompression, die Liste hat wie NRC ohne '
                                     'diese Erweiterungen nur "None" - es gibt nichts zu waehlen',
})


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
        if label == '(dynamisch)':   # Modify > Repeat: die vier Eintraege der wiederholbaren Transformationen
            return 'Menue|' + label + '|' + quelle, [r'Menue .* = Transform' + t + ': ' for t in ('Translate', 'Rotate', 'Scale')]
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
            anf = [r'Wirkung ' + esc(name) + r': ', r'Robust ' + esc(name) + r': ']
            if taste != '-':   # mit Standardtaste: auch ueber die Taste
                anf.append(r'Taste .* = ' + esc(name) + r': ')
            return k, anf
        if art == 'Shortcuts':
            if taste == '-':   # ohne Standardtaste: belegt im Test (Teil "ereignisse", F9)
                return k, [r'Ereignis ' + esc(name) + r': ']
            return k, [r'Taste .* = ' + esc(name) + r': ']
        if art == 'KeyEvents':
            return k, [r'Ereignis ' + esc(name) + r': ']
        return k, None
    if abschnitt == 'Dialoge und Fenster':
        m = re.match(r'"(.*)"  \(', text)
        return ('Dialog|' + m.group(1), None) if m else (None, None)
    if abschnitt == 'Einstellungsseiten':
        m = re.match(r'(.*?): "(.*)"  \(', text)
        return ('Seite|' + m.group(2), [r'Seite ' + esc(m.group(2)) + r': OK uebernimmt, Cancel verwirft']) if m else (None, None)
    if abschnitt == 'Einstellungsfelder':
        m = re.match(r'(\w+) (.*)  \((\S+)\)$', text)
        return ('Feld|' + m.group(3), [r'Feld ' + esc(m.group(3)) + r': ']) if m else (None, None)
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
        namen = [t.strip() for t in test.split('; ') if t.strip()]
        return 'Maus|' + teile[0] + '|' + teile[1], [esc(n) for n in namen]
    return None, None


# --- Eigene Eintraege (nicht in NRC) ----------------------------------------------
EIGENE = [
    # (Schluessel, Beschreibung, Anforderungen)
    ('Eigen|Build > Optimizer...', 'Optimierer (Stufe 5): Fenster, Pruefung, Beheben',
     [re.escape('Fenster "Optimizer" offen, Pruefung im Hintergrund fertig')]),
    ('Eigen|Seite Optimizer', 'Einstellungsseite des Optimierers', [r'.*Optimizer.*']),
    ('Eigen|File > Open .bsp...', 'fertige Karte oeffnen (Stufe 4)', [re.escape('Dialog "Open compiled map" offen'),
                                                                     re.escape('mp/duel1 umgewandelt und geladen')]),
    ('Eigen|Build > Update entities only', 'Originallicht behalten (Stufe 4)',
     [re.escape('duel1.bsp: jeder Lump ausser den Entities Byte-gleich')]),
    ('Eigen|Build > Cancel build', 'Bauen abbrechen (Stufe 3)', [re.escape('Nach dem Abbrechen baut der naechste Eintrag wieder')]),
    ('Eigen|Karten-Editor-Modus', 'der Reiter Karten-Editor (leftMode 3)', [re.escape('Kuerzel gelten nur im Karten-Editor')]),
    ('Eigen|Begriffe wie NetRadiant', 'Umschalter der Begriffe (Seite Interface)', [re.escape('Begriffe wie NetRadiant: Menue "File", "Edit" trotz Deutsch')]),
    ('Eigen|Help > About BehavEd', 'Hilfe des Programms', [re.escape('Help > About BehavEd: der About-Dialog des Programms')]),
    # Stufe 8a (gui/selbsttest_karte8.inc)
    ('Eigen|Build progress', 'Baufortschritt mit Restzeit (Stufe 8a/1)',
     [re.escape('erster Bau: Fenster "Build progress" offen'), re.escape('zweiter Bau: Restzeit aus dem letzten Bau dieser Karte'),
      re.escape('Abbrechen im Fortschrittsfenster')]),
    ('Eigen|Build > Notify when a build is done', 'Taskleiste blinkt (Stufe 8a/1)', [re.escape('Build > Melden: aus, als BhedBuildNotify 0 gespeichert')]),
    ('Eigen|Build > Keep editor data', 'Editor-Daten behalten (Stufe 8a/2)',
     [re.escape('Build > Editor-Daten behalten: an, als BhedKeepEditorData 1 gespeichert'), re.escape('Byte-Vergleich: Lumps 1..17'),
      re.escape('Open .bsp mit Begleitdatei: genaue Quelle')]),
    ('Eigen|Open .bsp > Recover misc_model', 'misc_model aus der Geometrie (Stufe 8a/3)', [re.escape('misc_model zurueck: pillar.md3')]),
    ('Eigen|Open .bsp > Estimate lights', 'Lichter schaetzen, experimentell (Stufe 8a/4)', [re.escape('geschaetzte Lichter: Ebene')]),
    ('Eigen|Build > Partial compile: reuse VIS / reuse light', 'Teil-Kompilierungen (Stufe 8a/5a, 5b, 5d)',
     [re.escape('Build > Teil-Kompilierung: VIS und Licht wiederverwenden an, gespeichert'), re.escape('nach einer Entity-Aenderung: Sichtbarkeit wiederverwendet'),
      re.escape('nur die Nachbearbeitung geaendert')]),
    ('Eigen|Build > Add to compiled map...', 'Zur fertigen Karte hinzufuegen (Stufe 8a/5c)', [re.escape('zusammengefuehrt: <karte>_merged.bsp')]),
    # Stufe 8b (gui/selbsttest_karte8b.inc). Punkt 1: die Knoepfe und Felder des Terrain Generators (NRC master)
    ('Eigen|Terrain Generator > About', 'Kasten About (Stufe 8b/1)', [re.escape('Terrain Generator > About: ')]),
    ('Eigen|Terrain Generator > Target', 'Use Selection / Manual Size (Stufe 8b/1)', [re.escape('Terrain Generator > Target: ')]),
    ('Eigen|Terrain Generator > Base Shape', 'Form, Beschriftung der Formhoehe (Stufe 8b/1)', [re.escape('Terrain Generator > Base Shape: ')]),
    ('Eigen|Terrain Generator > Irregular Grid', 'Vertex-Jitter (Stufe 8b/1)', [re.escape('Terrain Generator > Irregular Grid: ')]),
    ('Eigen|Terrain Generator > Generate', 'Erzeugen, Undo byte-gleich, Tunnel (Stufe 8b/1)',
     [re.escape('Terrain Generator > Generate: '), re.escape('Terrain Generator > Generate, Rueckgaengig: '),
      re.escape('Terrain Generator > Generate (Tunnel): ')]),
    ('Eigen|Terrain Generator > Seed', 'fester Startwert, nicht in NRC (Stufe 8b/1)', [re.escape('Terrain Generator > Seed: ')]),
    ('Eigen|Terrain Generator > Texture', 'Feld folgt dem Texturbrowser (Stufe 8b/1)', [re.escape('Terrain Generator > Texture: ')]),
    ('Eigen|Terrain Generator > Pick', 'Texturseite zeigen (Stufe 8b/1)', [re.escape('Terrain Generator > Pick: ')]),
    ('Eigen|Terrain Generator > Reset to Default', 'Vorgaben (Stufe 8b/1)', [re.escape('Terrain Generator > Reset to Default: ')]),
    ('Eigen|Terrain Generator > Close', 'Werte bleiben (Stufe 8b/1)', [re.escape('Terrain Generator > Close: ')]),
    # Punkt 2: Split-Fenster frei belegen (der echte Neustart: BHED_KARTE_NUR=split1/2/3)
    ('Eigen|Preferences > Layout > Free split windows', 'Schalter, Platzmenue, Strg+Tab, Teiler, Neustart (Stufe 8b/2)',
     [re.escape('Preferences > Layout > Free split windows: '), re.escape('Split > Platzmenue > Camera'), re.escape('Split > Platzmenue > XY'),
      re.escape('Split > Strg+Tab mit Schalter: '), re.escape('Split > Teiler ziehen: '), re.escape('Split > nach dem simulierten Neustart: '),
      re.escape('Regular > 2D-Ansichtstyp nach dem simulierten Neustart: '), re.escape('Free split windows aus: ')]),
    # Punkt 3: Im Editor spielen
    ('Eigen|View > Play in editor (F6)', 'Start, Wand, Treppe, playerclip, Sprung, Noclip, Esc (Stufe 8b/3)',
     [re.escape('View > Play in editor (F6): '), re.escape('Spielen > Wand: '), re.escape('Spielen > Treppe: '),
      re.escape('Spielen > Stufe 24 '), re.escape('Spielen > playerclip '), re.escape('Spielen > Sprung: '),
      re.escape('Spielen > N (echte Taste): '), re.escape('Spielen > Esc (echte Taste): '), re.escape('View > Play in editor (Menue) mit Spawn: ')]),
    ('Eigen|View > Play: start at info_player_start', 'Start am Spawn (Stufe 8b/3)', [re.escape('View > Play: start at info_player_start: ')]),
    # Punkt 4: Logik-Ansicht
    ('Eigen|View > Logic view...', 'Graph, Klick waehlt/zentriert, Doppelklick oeffnet das Skript (Stufe 8b/4)',
     [re.escape('View > Logic view...: '), re.escape('Logic view > Klick: '), re.escape('Logic view > Doppelklick auf ein fehlendes Skript: '),
      re.escape('Logic view > Doppelklick auf ein Skript des Spiels: '), re.escape('Logic view (Testerkarte')]),
    # Punkt 5: Ansichtsmodi der Kamera
    ('Eigen|View > Camera view modes > Collision', 'Kollision farbig (Stufe 8b/5)', [re.escape('View > Camera view modes > Collision: ')]),
    ('Eigen|View > Camera view modes > Lightmap density', 'Luxel wie q3map2 (Stufe 8b/5)', [re.escape('Camera view modes > Lightmap density: ')]),
    ('Eigen|View > Camera view modes > Visibility (PVS)', 'PVS aus der .bsp (Stufe 8b/5)', [re.escape('Camera view modes > Visibility (PVS): ')]),
    ('Eigen|View > Camera view modes > Triangle load', 'Waermekarte aus der .bsp (Stufe 8b/5)', [re.escape('Camera view modes > Triangle load: ')]),
    # Punkt 6: Spiegel-Modus
    ('Eigen|View > Mirror mode > Mirror plane X', 'an, Linie, Ziehen live gespiegelt, Undo/Redo byte-gleich, Entity (Stufe 8b/6)',
     [re.escape('View > Mirror mode > Mirror plane X: '), re.escape('Ziehen: schon waehrend des Ziehens (Befehl offen) ist das Gegenstueck gespiegelt'),
      re.escape('Mirror mode: Strg+Z stellt beide Seiten byte-gleich her'), re.escape('Mirror mode: Strg+Y stellt den Stand nach dem Ziehen byte-gleich her'),
      re.escape('Mirror mode: Punktentitaet gezogen')]),
    ('Eigen|View > Mirror mode > Mirror plane Y', 'Achse Y (Stufe 8b/6)', [re.escape('View > Mirror mode > Mirror plane Y: ')]),
    ('Eigen|View > Mirror mode > Mirror plane Z', 'Achse Z (Stufe 8b/6)', [re.escape('View > Mirror mode > Mirror plane Z: ')]),
    ('Eigen|View > Mirror mode > Plane through selection center (on grid)', 'Ebene an der Auswahl (Stufe 8b/6)',
     [re.escape('View > Mirror mode > Plane through selection center (on grid): ')]),
    ('Eigen|View > Mirror mode > Plane through origin', 'Ebene 0 (Stufe 8b/6)', [re.escape('View > Mirror mode > Plane through origin: ')]),
    ('Eigen|View > Mirror mode > Off', 'aus (Stufe 8b/6)', [re.escape('View > Mirror mode > Off: ')]),
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
