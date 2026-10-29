# Notizen sprache_es: Grundlage für weitere Sprachen, dann Spanisch

Zweig `sprache_es` ab karte1 = rc606 + H2 (2406e182), Worktree `Y:\sprache_es`,
Bau `build-sprache`. Auftrag: SP\auftrag_sprachen.md (Agent ES).

## Teil 1: Grundlage (eigener Commit, Signal für Agent CS)

Eine Liste für alle Erzeuger: `tools/sprachen.py` (kurz, Code, Name, Wert von
`i18n::Language`, CJK). Die vier ersten Sprachen (en, de, zh, ja) bleiben, wo
sie sind; jede weitere hat eigene Dateien, so dass parallele Arbeit nur
Ergänzungen sind:

| Was | Datei der Sprache |
|---|---|
| Programmtexte | `tools/i18n_sprache_<kurz>.py` (`TEXTE = {Kennung: Text}`) |
| Handbuch | `tools/handbuch/inhalt_<kurz>.py` (`TEXTE = {englischer Text: Text}`, `TASTEN`) |
| Helfer | `src/hilfe_sprache_<kurz>.inc` (Stoppwörter, Endungen, Begriffe, Ziele) |
| Helfer-Fragen | `tests/hilfe_fragen_<kurz>.inc` (eingefroren, ≥ 24, Top-3 ≥ 85 %) |

1. **Sprachen erweiterbar.** `Language` um `Spanish`, `Czech` am Ende (0..3
   bleiben; Einstellungsdatei speichert den Code `es`/`cs`, Changelog die
   Spalte). `LanguageInfo.kurz` + `kurzCode()` ersetzen die drei doppelten
   `switch` (app.cpp, hilfe.cpp, tour.cpp). Sprachwahl im Programm (Ansicht)
   und im Karten-Editor (View, Misc) laufen über `languages()`.
   `fromSystemLocale`: es-* → Spanisch, cs-* → Tschechisch (sk-SK bleibt
   Englisch). Test `sprache` (rot: kompilierte nicht ohne die Werte; grün),
   Selbsttest `pruefung`/Modus 1 (Ansicht > Sprache > Español/Čeština) und
   `karteneditor` Stufe 7 (View > Language > Español/Čeština).
2. **Übersetzungen je Sprache in eigenen Dateien** (Tabelle oben).
   `gen_i18n.py` liest sie; Formatplatzhalter müssen zum Englischen passen
   (sonst Abbruch); unbekannte Kennungen brechen nicht ab (anderer Zweig kann
   umbenennen), `lint_i18n` nennt sie.
3. **Englisch statt leer.** Fehlt ein Text, steht `nullptr` in der Tabelle;
   `trIn()` gibt den englischen. `hatEigenenText()` für Tests. Test
   `sprache`: jede Kennung in es/cs nie leer, ohne eigenen Text genau
   englisch. `lint_i18n.py` listet je weiterer Sprache, was fehlt (Warnung);
   `lint_i18n.py --sprache es` endet mit Fehler, solange etwas fehlt.
   `gen_i18n.py --fehlend` liefert die Liste als JSON.
   Nebenbei: `lint_i18n_erzeugt.py` kopierte nur 2 der Quelldateien in den
   Wegwerfbaum und scheiterte seit Stufe 2 – jetzt alle `i18n_*.py`.
4. **Schrift.** ImGui 1.92 lädt Glyphen nach; Microsoft Sans Serif, Segoe UI,
   Tahoma (+ fett) haben Latin-1 und Latin Extended-A. Geprüft: Selbsttest
   (`schriftSchritte`, Modus 1 und `pruefung`): jede geladene Schrift
   (`IsGlyphInFont`) hat ñ á é í ó ú ü ¿ ¡ und ě š č ř ž ý ů ď ť ň (groß und
   klein) – 4 Schriften, 144 Zeichen, 0 fehlen. Qt-Maße (DirectWrite):
   `qtFehlendeZeichen()`, ctest `karte_qttext` für Segoe UI, Tahoma, Tahoma
   fett (Gegenprobe: Tahoma hat kein 中). Handbuch: UTF-8, Schrift Segoe UI
   (dieselbe Datei).
5. **Changelog.** `gen_changelog.py`: `[es]`/`[cs]` erlaubt, Pflicht nur
   en/de/zh/ja; fehlt eine weitere, `nullptr` → `changelog::text()` gibt
   Englisch. ctest `changelog_sprachen` (neue Fassung mit [es], alte ohne,
   ohne [de] bricht ab), Test `sprache`, Selbsttest `pruefung` (Fenster auf
   Spanisch zeigt alte Fassung englisch).
6. **Handbuch.** 6 Sprachen, Umschalter, `suche.js`/`touren.json` je
   Sprache, `index.html` leitet auch nach es/cs. Weitere Sprachen: Text aus
   `inhalt_<kurz>.py`, sonst Englisch; Bild der Sprache, sonst das englische
   mit Hinweis „Bild noch auf Englisch“. `T` zählt `fehlend_neu` (kein
   Abbruch), `--pflicht=es` macht es zum Fehler. Zwei Stellen setzten Texte
   je Sprache von Hand zusammen (`_OFFEN.get("de")`, `m.get("zh")`) – jetzt
   `verbinde()`/`formatiere()`, die alle Sprachen kennen. Tastennamen je Land
   über `tasten_namen()`. `handbuch_sprachen` prüft zusätzlich, dass jede
   Sprache Seiten, Suchindex, Touren und einen Umschalter zu allen hat.
   Suche faltet Akzente (`animacion` → animación, `cestina` → čeština,
   `uber` → Über) und markiert im Original: ctest `handbuch_suche` im Browser
   ohne Fenster (Edge headless, eigener Profilordner): alter Stand 5 von 7
   daneben, neuer 7 von 7.
7. **Helfer.** Akzente gefaltet (Latin-1/Ext-A auf den Grundbuchstaben;
   ä ö ü ß weiter ae oe ue ss), Stoppwörter und Endungen je Sprache vor den
   allgemeinen, Begriffe je Sprache (gleicher Name ergänzt), Ziele je
   Sprache; `Index::setzeSprache`. hilfetest: Faltung es/cs, spanische
   Stoppwörter und Stämme (animación/animaciones/animar/animado → anim,
   textura(s) → textur wie texture); die vier ersten Sprachen unverändert
   (Hauptliste 99/99, Zweitliste 18/24, H2-Liste 42/48 = 87,5 % wie in H2).

## Offen / Hinweise für CS

- NRC-Menünamen (KeL/KeT aus `karte_menues.py`) bleiben in Spanisch englisch
  (Auftrag: nur übersetzen, wenn NRC sie übersetzt – NRC hat nur Englisch).
