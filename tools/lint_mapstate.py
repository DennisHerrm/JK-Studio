#!/usr/bin/env python3
"""Prueft: was zur KARTE gehoert, wird beim WECHSEL auch weggeraeumt.

Anlass rc216. clearMap() raeumte die aufgestellten Modelle, aber
loadMapFromBytes() - der Weg beim WECHSEL - tat es nicht. Also blieb das
Schiff der vorigen Karte stehen, und zwar bis zum Neustart.

Beide Wege muessen dieselben Zustaende zuruecksetzen. Ausgenommen sind reine
DATEICACHES: die haengen am Dateinamen, nicht an der Karte, und sie
wegzuwerfen waere nur Verschwendung.

rc609 F9: die Funktionen stehen laengst nicht mehr alle in gui/app.cpp
(loadMapFromBytes in app_ablage.inc), und der Wechsel raeumt ueber
Hilfsfunktionen (kartenSzeneVergessen, karteGeometrieLesen, ...). Gelesen
werden daher alle Quellen in gui/, und die Rumpfe der aufgerufenen
gui-Funktionen zaehlen mit (bis Tiefe 4). Als zurueckgesetzt gilt
`g_app->x.clear()`, jede Zuweisung `g_app->x = ...` und `++g_app->x`.

--selbsttest: nimmt aus dem Wechsel-Rumpf absichtlich Zustaende heraus und
erwartet, dass der Pruefer sie meldet (sonst prueft er nichts).
"""
import pathlib
import re
import sys

root = pathlib.Path(__file__).resolve().parent.parent

# Dateicaches: absichtlich NICHT beim Wechsel geleert.
erlaubt = {'md3Files', 'md3Meshes', 'settings.mapPath'}

# Was auf NUMMERN zeigt, muss beim Wechsel zurueckgesetzt werden (rc217:
# brushMeshes liegt nach Untermodellnummer ab, pickedEntity ist ein Index).
nach_nummer = ['brushMeshes', 'pickedEntity']

TIEFE = 4


def quellen_lesen() -> dict:
    texte = {}
    for muster in ('*.cpp', '*.inc'):
        for p in sorted((root / 'gui').glob(muster)):
            t = p.read_text(encoding='utf-8', errors='replace')
            texte[p.name] = re.sub(r'//[^\n]*', '', t)
    return texte


KOPF = re.compile(r'^[A-Za-z_][\w:<>,*& ]*?[\s*&](\w+)\(', re.M)


def definitionen(texte: dict) -> dict:
    """Name -> Liste der Rumpfe freier Funktionen (Kopf in Spalte 0)."""
    d: dict = {}
    for t in texte.values():
        for m in KOPF.finditer(t):
            rest = t[m.end():]
            k = rest.find('{')
            s = rest.find(';')
            if k < 0 or (0 <= s < k):
                continue
            anfang = m.end() + k
            ende = t.find('\n}\n', anfang)
            if ende < 0:
                continue
            d.setdefault(m.group(1), []).append(t[anfang:ende])
    return d


RESET = re.compile(r'(\+\+\s*)?g_app->(\w+(?:\.\w+)*?)\s*(\.clear\(\)|=(?!=))?')
AUFRUF = re.compile(r'\b(\w+)\s*\(')


def zurueckgesetzt(name: str, defs: dict, ersatz: dict) -> set:
    gesehen = set()
    ergebnis = set()

    def lauf(n: str, tiefe: int) -> None:
        if n in gesehen or n not in defs or tiefe > TIEFE:
            return
        gesehen.add(n)
        for rumpf in ersatz.get(n, defs[n]):
            for m in RESET.finditer(rumpf):
                if m.group(1) or m.group(3):
                    ergebnis.add(m.group(2))
            for a in AUFRUF.finditer(rumpf):
                lauf(a.group(1), tiefe + 1)

    lauf(name, 0)
    return ergebnis


def pruefen(defs: dict, ersatz: dict, ausgabe: bool) -> int:
    if 'clearMap' not in defs or 'loadMapFromBytes' not in defs:
        print('lint_mapstate: clearMap() oder loadMapFromBytes() nicht gefunden')
        return 1
    beim_leeren = zurueckgesetzt('clearMap', defs, ersatz)
    beim_wechsel = zurueckgesetzt('loadMapFromBytes', defs, ersatz)
    fehler = 0
    offen = [n for n in nach_nummer if n not in beim_wechsel]
    if offen:
        fehler += len(offen)
        if ausgabe:
            print('Zeigt auf NUMMERN der Karte, wird beim Wechsel aber nicht zurueckgesetzt:')
            for o in offen:
                print(f'   g_app->{o}')
            print('   (Nummern werden je Karte neu vergeben - siehe rc217)')
    fehlt = sorted(beim_leeren - beim_wechsel - erlaubt - set(offen))
    if fehlt:
        fehler += len(fehlt)
        if ausgabe:
            print('Beim KARTENWECHSEL nicht weggeraeumt, obwohl clearMap() es tut:')
            for f in fehlt:
                print(f'   g_app->{f}')
            print('   (so blieb in rc215 das Schiff der vorigen Karte stehen)')
    return fehler


def selbsttest(defs: dict) -> int:
    # Ein Zustand, den nur der Wechsel-Rumpf selbst zuruecksetzt, und eine
    # Nummer: beide einzeln herausnehmen, beide muessen gemeldet werden.
    rumpf = defs['loadMapFromBytes'][0]
    proben = ['g_app->moverKlaenge.clear();', 'g_app->pickedEntity = -1;']
    for probe in proben:
        if probe not in rumpf:
            print(f'lint_mapstate --selbsttest: Probe fehlt im Rumpf: {probe}')
            return 1
        ersatz = {'loadMapFromBytes': [rumpf.replace(probe, '')]}
        if pruefen(defs, ersatz, False) == 0:
            print(f'lint_mapstate --selbsttest: vergessenes "{probe}" NICHT erkannt')
            return 1
    print(f'lint_mapstate --selbsttest: {len(proben)} absichtlich vergessene Zustaende erkannt')
    return 0


def main() -> int:
    defs = definitionen(quellen_lesen())
    if 'loadMapFromBytes' not in defs:
        print('lint_mapstate: loadMapFromBytes() nicht gefunden')
        return 1
    if '--selbsttest' in sys.argv[1:]:
        return selbsttest(defs)
    n = pruefen(defs, {}, True)
    if n:
        print(f'Kartenzustandspruefung: {n} Beanstandung(en)')
        return 1
    print('Kartenzustandspruefung: ok')
    return 0


if __name__ == '__main__':
    sys.exit(main())
