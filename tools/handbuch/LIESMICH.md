# Handbuch pflegen

Das Handbuch (Ordner `handbuch/`, wird neben `behaved.exe` ausgeliefert und über
**Help → Guide and tutorials** bzw. den Knopf **Guide** im About-Fenster
geöffnet) entsteht vollständig aus diesem Ordner. Nichts unter `handbuch/` von
Hand ändern – es wird überschrieben.

**Regel:** Jede Änderung an einer Funktion zieht die betroffene Seite mit –
Text in allen vier Sprachen und, wenn sich das Bild ändert, die Bilder.

## Aufbau

| Datei | Inhalt |
|---|---|
| `inhalt_basis.py` | `T(en, de, zh, ja)` – jeder Text in allen vier Sprachen (fehlt einer, bricht der Bau ab); feste Beschriftungen des Handbuchs |
| `uebersetzung/<sprache>/*.json` | Übersetzungen der Texte, die aus Daten oder Schleifen entstehen (Befehlsseiten, Knopftabellen, Tutorial-Schritte, die Beschreibungen aus `Q3_Interface.h`/`behaved.bhc` …): `{englischer Text: Übersetzung}`. `T("…")` ohne Übersetzung im Aufruf nimmt sie von dort; `Tf(vorlage, werte…)` übersetzt eine Vorlage mit `%s`/`%d`. Texte ohne Wörter (nur `Code`, `{{Kennung}}`, `[[Seite]]`, Zahlen) gelten in jeder Sprache gleich |
| `inhalt.py` | Gruppen (Seitenleiste) und Reihenfolge der Seiten |
| `inhalt_start.py`, `inhalt_skript.py`, `inhalt_karte.py`, `inhalt_szene.py`, `inhalt_mission.py`, `inhalt_tutorials.py`, `inhalt_referenz.py` | die Seiten |
| `gen_handbuch.py` | baut `handbuch/<sprache>/*.html`, den Suchindex `suche.js` je Sprache, kopiert `assets/` |
| `bilder.py` | Fotos des Selbsttests → `handbuch/bilder/<sprache>/*.webp` (Ausschnitt, Größe, Bildfolgen als bewegtes WebP) |
| `assets/` | Stil, Suche/Hell-Dunkel/Bildvergrößerung (`handbuch.js`), Logo |

### Bausteine einer Seite

`("p", T)`, `("h2", "anker", T)` (erscheint in der Seitenleiste und als
Suchtreffer), `("h3", T)`, `("list", [T…])`, `("steps", [T | (T, bild, T)…])`,
`("img", name, T[, "breit"])`, `("imgs", [(name, T)…])`, `("anim", name, T)`,
`("note", T)`, `("warn", T)`, `("keys", [(taste, T)…])`,
`("table", [kopf…], [[zelle…]…])`, `("cards", [seiten-id…])`.

Im Text: `**fett**`, `` `code` ``, `[[seite]]`, `[[seite#anker]]`,
`[[seite#anker|Text]]`, `<kbd>Strg</kbd>` und **`{{Kennung}}`** – eine
Beschriftung des Programms aus `tools/gen_i18n.py`. Sie erscheint in jeder
Sprache genau so wie im Programm; benennt man einen Knopf um, zieht das
Handbuch beim nächsten Bau mit. Eine unbekannte Kennung bricht den Bau ab.

## Bauen

```
python tools/handbuch/gen_handbuch.py            # -> handbuch/
python tools/handbuch/gen_handbuch.py <ordner>   # -> <ordner>/handbuch/
```

Am Ende nennt der Bau fehlende Bilder. Fehlt einer Seite eine Sprache – ein Text
ohne Übersetzung (weder im Aufruf noch in `uebersetzung/`), ein Bild, oder ein
Bild mit Oberfläche nur auf Englisch –, nennt er sie und endet mit Fehler.
`python tools/handbuch/gen_handbuch.py --pruefen` baut nur zur Probe (ctest
`handbuch_sprachen`). Ändert man einen englischen Text, der in `uebersetzung/`
steht, den Eintrag dort mitziehen (der Bau meldet nicht mehr gebrauchte Einträge).

## Bilder neu aufnehmen

Der Selbsttest `BHED_EDITORTEST=handbuch` (gui/selbsttest.cpp,
`handbuchSchritte`) lädt `md_am_sith` aus `MD_Missions_Ep3.pk3`, stellt jede
Szene ein und fotografiert **nur das eigene Fenster**; neben jedes Foto
schreibt er den Ausschnitt (`<name>.rect`). Umgebung:

- `BHED_FOTOS` – Zielordner, `BHED_SPRACHE` – en / de / zh-Hans / ja (Kennungen wie in der Einstellungsdatei)
- `BHED_MISSION_ARCHIV`, `BHED_MISSION_NAME`
- `BHED_MISSIONEN`, `BHED_HB_ZIEL` – die Übungsmission und ihr Archiv. Beim
  Aufnehmen auf ein Laufwerk aus `subst` legen (z. B. `X:\missionen`), damit
  kein Benutzerpfad im Bild steht; danach `subst X: /d`.

Die Bilder des Karten-Editors (`h_karteneditor*`, `h_kartebauen*`) nimmt
`BHED_HB_NUR=karteneditor` auf, nur die der Seite „Karte bauen“
`BHED_HB_NUR=kartebauen` (baut die Testkarten mit `bhed_q3map2.exe`, das der
Lauf wie der Selbsttest der Stufe 3 aus der Arbeitskopie holt), die der
Seiten „Fertige Karten öffnen und ändern“ und „Plugins“ (`h_karte4_*`)
`BHED_HB_NUR=karte4` (öffnet mp/duel1 aus den .pk3 des Spiels), die der
Seite „Optimierer und Grenzen“ (`h_karte5_*`) `BHED_HB_NUR=karte5` (ebenfalls
mp/duel1), die der Seite „Bau-Fortschritt und Teil-Kompilierungen“ und der
Stufe-8a-Abschnitte von „Fertige Karten öffnen und ändern“ (`h_karte8_*`)
`BHED_HB_NUR=karte8` (baut die Testkarten mit `bhed_q3map2.exe`). Die Bilder der Seite „Terrain, Spielen, Logik, Spiegel und mehr“ (`h_karte8b_*`) nimmt
`BHED_HB_NUR=karte8b` auf (eine eigene Testkarte mit Hof, Säule, Auslöser, Tür und NPC_spawner). Die Tabellen der Grenzen auf dieser Seite kommen aus
`grenzen.json`, die `kartegrenzen --json tools/handbuch/grenzen.json` aus
der Tabelle im Code schreibt (der ctest `karte_grenzen` vergleicht). Chinesisch:
`BHED_SPRACHE=zh-Hans`, aber `bilder.py … zh`.

Je Sprache ein Lauf, dann

```
python tools/handbuch/bilder.py <fotoordner> <sprache>
```

Ein neues Bild: im Selbsttest einen Schritt `hb::fotoTeil("name", ausschnitt)`
an passender Stelle, im Inhalt `("img", "name", T(...))`. Bildfolgen heißen
`a_<name>_00 … _NN` und werden zu einem bewegten Bild `a_<name>.webp`.

## Prüfen

- Suche: im Browser `…/en/index.html`, `/` drücken, einen Begriff tippen –
  der Treffer springt an den Abschnitt und markiert den Begriff.
- Alle vier Sprachen ansehen (Sprachwahl oben rechts).
- Kein Pfad mit Benutzernamen in den Bildern.
