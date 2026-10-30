#!/usr/bin/env python3
"""
Sucht Oberflaechentexte, die fest im Quelltext stehen statt durch tr() zu
gehen. Ohne den Pruefer bleiben beim Uebersetzen einzelne Knoepfe englisch,
und das faellt erst auf, wenn jemand die Sprache umstellt.

Erlaubt sind:
  * Kennungen fuer ImGui, die mit ## anfangen oder gar nicht angezeigt werden
  * Formatzeichenketten, die nur Platzhalter und Satzzeichen enthalten
  * Produktnamen aus der Ausnahmeliste
"""
import re, sys, pathlib

# Funktionen, deren erstes Zeichenkettenargument der Nutzer SIEHT
SHOWN = ['Text', 'TextUnformatted', 'TextDisabled', 'TextColored', 'Button',
         'SmallButton', 'MenuItem', 'BeginMenu', 'Selectable', 'Checkbox',
         'RadioButton', 'BeginCombo', 'BeginPopupModal', 'BeginTabItem',
         'CollapsingHeader', 'TreeNode', 'SliderFloat', 'InputText',
         'SetTooltip', 'LabelText', 'BulletText']

AUSNAHMEN = {'BehavEd', 'ICARUS', 'OpenJK', 'ImGui', 'Dear ImGui', 'JKA', 'JK2',
             # uebersetzung1: Spielnamen, wie das Spiel sie schreibt
             'Jedi Academy', 'Jedi Outcast', 'Movie Duels',
             # Schalter der Suche wie in jedem Code-Editor (Regex, Gross/klein,
             # ganzes Wort) und das Schliessen-Kreuz - Zeichen, keine Woerter
             '.*', 'Aa', 'ab', 'x',
             # M5: Pfeile hoch/runter ("^", "v") im Layout-Panel des Menue-Editors
             'v',
             # Schluesselwoerter der .menu-Dateien (Eigenschaften im Menue-Editor),
             # Spieltermini wie die ICARUS-Befehle - bleiben in jeder Sprache
             'type', 'style', 'textalign', 'visible'}

CALL = re.compile(r'ImGui::(' + '|'.join(SHOWN) + r')\s*\(\s*("(?:[^"\\]|\\.)*")')

def harmlos(lit):
    # Was hinter ## steht, zeigt ImGui nicht (Kennung des Elements).
    s = lit[1:-1].split('##', 1)[0]
    if s == '':
        return True
    # uebersetzung1: ohne Platzhalter und Escapes (\n) kein Buchstabe mehr -
    # "%s %s:%d  %s = %lld%s", "= %s", "%s\n%s" sind reine Formate.
    if not re.search(r'[A-Za-z]', re.sub(r'\\.|%[-+#0]*[0-9]*(?:\.[0-9]+)?(?:hh|h|ll|l|z|j|t|L)?[a-zA-Z%]', '', s)):
        return True
    # nur Platzhalter, Satzzeichen, Leerzeichen
    # --- Was als Platzhalter zaehlt ------------------------------------
    #
    # Die Regel lautete `%[a-zA-Z]` - ein Prozentzeichen DIREKT vor einem
    # Buchstaben. Damit galt "%d" als Platzhalter, "%-8s" oder "%6.2f" aber
    # nicht: eine Diagnosezeile mit Spaltenbreiten wurde beanstandet,
    # dieselbe Zeile ohne Breiten nicht. Das ist kein Unterschied, den
    # jemand gemeint hat.
    #
    # Der Ausdruck kennt jetzt Kennzeichen, Breite und Genauigkeit. Das
    # Leerzeichen als Kennzeichen (% d) ist BEWUSST nicht dabei - sonst
    # ginge "100% sicher" als Formatzeichenkette durch, und das ist Fliesstext.
    #
    # Nachgemessen ueber alle Zeichenketten in gui/: genau zwei aendern ihr
    # Urteil, beide neue Diagnosezeilen. Keine wird strenger.
    if (re.fullmatch(r'[\s%.\-+#0-9a-zA-Z()\[\]:,]*', s) and
            re.search(r'%[-+#0]*[0-9]*(?:\.[0-9]+)?[a-zA-Z]', s)):
        return True
    if s in AUSNAHMEN:
        return True
    # reine Trennzeichen
    if re.fullmatch(r'[\s\-=_.|:]*', s):
        return True
    return False

root = pathlib.Path(__file__).resolve().parent.parent
bad = 0
# uebersetzung1: auch die Teile, die eine .cpp einbindet (gui/*.inc: Anordnung,
# Ablage, Selbsttests ...) - vorher sah der Pruefer dort weder feste Texte noch
# Benutzungen und meldete fast 700 Texte als "nirgends benutzt".
_GUI = sorted((root / 'gui').glob('*.cpp')) + sorted((root / 'gui').glob('*.h')) + sorted((root / 'gui').glob('*.inc'))
for f in _GUI:
    for n, line in enumerate(f.read_text(encoding='utf-8').split('\n'), 1):
        for m in CALL.finditer(line):
            if not harmlos(m.group(2)):
                print(f"{f.relative_to(root)}:{n}: fester Text in ImGui::{m.group(1)}(): {m.group(2)}")
                bad += 1
# --- Uebersetzte Texte, die nirgends benutzt werden ------------------------
#
# Anlass: beim Herausziehen der Bedienleiste aus der Modellspalte ging die
# HAUTAUSWAHL verloren. Der Uebersetzer meldete nichts - der Code lief ja
# weiter, nur zeigte er das Feld nicht mehr. Aufgefallen ist es allein
# daran, dass ModelSkin, ModelSkinHint und ModelSurfaces ploetzlich
# nirgends mehr vorkamen.
#
# Ein ungenutzter Text ist also nicht bloss Ballast, sondern oft die Spur
# einer verschwundenen Bedienung.
# Die Namen kommen aus include/bhed/i18n.h, NICHT aus gen_i18n.py.
#
# Der Erzeuger ist nicht mehr die einzige Quelle: von Hand ergaenzte Texte
# landen direkt in i18n.h und i18n.cpp. Genau die sah dieser Pruefer bisher
# nicht - und damit auch nicht, wenn sie spaeter ungenutzt liegenblieben.
# Aufgefallen bei TlHeight/TlHeightHint, die mit dem Hoehenregler
# verschwanden und trotzdem stehenblieben.
#
# Die Aufzaehlung IST massgeblich: was dort steht, wird uebersetzt.
# NUR die Aufzaehlung Str - i18n.h enthaelt auch Language, und deren Werte
# (English, German, ...) sind Sprachen, keine Texte. Der erste Anlauf hat
# sie mitgezaehlt und vier falsche Beanstandungen erzeugt.
_hdr = (root / 'include' / 'bhed' / 'i18n.h').read_text(encoding='utf-8')
_von = _hdr.find('enum class Str {')
_bis = _hdr.find('};', _von)
_names = re.findall(r'^\s{4}(\w+),\s*$', _hdr[_von:_bis], re.M)
_code = ''
# Nicht src/i18n.cpp: die erzeugte Tabelle der Zaehltexte nennt jede Kennung.
_QUELLEN = [p for p in _GUI + sorted((root / 'src').rglob('*.cpp')) + sorted((root / 'src').rglob('*.inc'))
            if p.name != 'i18n.cpp']
for _f in _QUELLEN:
    _code += _f.read_text(encoding='utf-8')
# uebersetzung2: der Helfer (src/hilfe.cpp kettenText/kennungLabel) holt die Namen der
# Ketten und Hinweise aus src/hilfe_orte.inc zur Laufzeit ueber strVonName - aus
# "ViewMap>TlKamera>CamPilotKey" wird "Mission view > Camera > Set key". Ohne diese
# Namen galt TlKamera als unbenutzt; entfernt, hiesse der Pfad "Mission view >  > Set key".
_orte = set()
for _m in re.finditer(r'^\{"[^"]*", "[^"]*", "([^"]*)", "([^"]*)"',
                      (root / 'src' / 'hilfe_orte.inc').read_text(encoding='utf-8'), re.M):
    _orte.update(_m.group(1).split('>'))
    _orte.add(_m.group(2))
_unused = [n for n in _names if f'Str::{n}' not in _code and n not in _orte]
if _unused:
    print(f"{len(_unused)} uebersetzte Texte werden nirgends benutzt:")
    for _u in _unused:
        print(f"   {_u}")
    print("   (haeufig die Spur einer Bedienung, die verlorengegangen ist)")
    bad += len(_unused)

# --- Weitere Sprachen (tools/sprachen.py): was noch englisch ist -------------
#
# Fehlt einer weiteren Sprache (Spanisch, Tschechisch) ein Text in
# tools/i18n_sprache_<kurz>.py, zeigt das Programm den englischen - nie einen
# leeren. Das ist erlaubt: neue Texte anderer Zweige kommen in vier Sprachen.
# Darum hier nur eine WARNUNG (zaehlt nicht als Beanstandung). Mit
# `--sprache <kurz>` prueft der Lauf nur diese Sprache und endet mit Fehler,
# wenn ihr ein Text fehlt (Pflicht am Ende ihrer Stufe).
import json  # noqa: E402
import subprocess  # noqa: E402
_lauf = subprocess.run([sys.executable, str(root / 'tools' / 'gen_i18n.py'), '--fehlend'],
                       cwd=root, capture_output=True, text=True, encoding='utf-8')
if _lauf.returncode != 0:
    print("gen_i18n.py --fehlend scheiterte:", _lauf.stderr[:400])
    sys.exit(1)
_stand = json.loads(_lauf.stdout.strip().splitlines()[-1])

# --- Zaehltexte nur ueber trN (uebersetzung1) --------------------------------
#
# Ein Text mit Formen je Zahl ("%d {file|files}") zeigt mit tr() immer die
# Form "other" - "1 files", cs "2 souborů". Beanstandet wird jede Stelle, an
# der die naechste oeffnende Uebersetzungsfunktion vor Str::<Zaehltext> tr(,
# trK( oder trIn( ist.
_zaehl = set(_stand.get('zaehl', []))
_OEFFNER = re.compile(r'\b(trN|trKN|trNIn|tr|trK|trIn)\(')
for _f in _QUELLEN:
    for _n, _zeile in enumerate(_f.read_text(encoding='utf-8').split('\n'), 1):
        for _m in re.finditer(r'Str::(\w+)', _zeile):
            if _m.group(1) not in _zaehl:
                continue
            _vor = list(_OEFFNER.finditer(_zeile, 0, _m.start()))
            if _vor and _vor[-1].group(1) in ('tr', 'trK', 'trIn'):
                print(f"{_f.relative_to(root)}:{_n}: Zaehltext {_m.group(1)} ueber {_vor[-1].group(1)}() statt trN/trKN")
                bad += 1

_nur = sys.argv[sys.argv.index('--sprache') + 1] if '--sprache' in sys.argv else None
for _kurz, _fehlt in _stand['fehlend'].items():
    if _nur is not None and _kurz != _nur:
        continue
    _fremd = _stand['fremd'][_kurz]
    print(f"Sprache {_kurz}: {len(_fehlt)} Text(e) noch englisch"
          + (f", {len(_fremd)} Kennung(en) in i18n_sprache_{_kurz}.py gibt es nicht" if _fremd else ""))
    for _k in _fehlt[:40]:
        print(f"   fehlt ({_kurz}): {_k}")
    if len(_fehlt) > 40:
        print(f"   ... und {len(_fehlt) - 40} weitere")
    for _k in _fremd:
        print(f"   gibt es nicht ({_kurz}): {_k}")
if _nur is not None:
    _f = _stand['fehlend'].get(_nur)
    if _f is None:
        print(f"unbekannte weitere Sprache: {_nur}")
        sys.exit(1)
    print(f"i18n-Pruefung {_nur}:", "ok (0 fehlend)" if not _f and not _stand['fremd'][_nur] else "unvollstaendig")
    sys.exit(1 if _f or _stand['fremd'][_nur] else 0)

print("i18n-Pruefung:", "ok" if bad == 0 else f"{bad} Beanstandung(en)")
sys.exit(1 if bad else 0)
