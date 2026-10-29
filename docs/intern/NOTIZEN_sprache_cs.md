# Notizen sprache_cs: Tschechisch

Zweig `sprache_cs` ab karte1 = rc606 + H2 (2406e182), Worktree `Y:\sprache_cs`,
Bau `build-sprache`. Auftrag: SP\auftrag_sprachen.md (Agent CS). Grundlage aus
`sprache_es` (1369f4a8) gemergt, danach nichts mehr von dort geholt.

## Ablauf

1. Bis zum Signal: Glossar und alle Übersetzungen aus den Quelltexten
   vorbereitet (Programm: Tabelle von `gen_i18n.py`; Handbuch: jeder Text,
   den `gen_handbuch.py` erzeugt), außerhalb des Worktrees, ohne die
   Grundlage anzufassen. Jede Zeile maschinell gegen das Englische geprüft:
   gleiche Formatplatzhalter (Reihenfolge), `{{Kennung}}`, `[[Seite]]`,
   `` `Code` ``, `<kbd>`, Zeilenumbrüche, `**`-Paare, führende/folgende
   Leerzeichen. Bewusste Abweichungen (ein übersetzter Oberflächentext in
   `` `...` ``, „90 %“ ohne Formatbedeutung) einzeln freigegeben.
2. Die eingefrorene Frageliste vor dem Merge committet (246de054), beim
   Merge nur in den Aufbau der Grundlage gebracht.
3. Merge der Grundlage, dann die Dateien der Sprache erzeugt.

## Ergebnis

| Was | Datei | Umfang |
|---|---|---|
| Programmtexte | `tools/i18n_sprache_cs.py` | 2365 eigene Texte + 317 NRC-Texte englisch (unten); 13 ohne Wörter gelten ohnehin |
| Handbuch | `tools/handbuch/inhalt_cs.py` | 3304 Texte (alle), Tastennamen |
| Helfer | `src/hilfe_sprache_cs.inc` | 110 Stoppwörter, 44 Endungen, 62 Begriffe, 50 Ziele |
| Helfer-Fragen | `tests/hilfe_fragen_cs.inc` | 26 Fragen, 18 Ziele |
| Handbuchbilder | `handbuch/bilder/cs` | 575 (jedes Bild mit Oberflächentext) |

`lint_i18n.py --sprache cs`: 0 fehlend. `gen_handbuch.py --pruefen --pflicht=cs`:
0 Texte und 0 Bilder englisch.

## Helfer: Messung der eingefrorenen Liste (Top 3)

| Stand | cs-Liste |
|---|---|
| Grundlage ohne tschechische Begriffe (nur übersetztes Handbuch/Touren) | 22/26 = 84,6 % (rot) |
| mit `hilfe_sprache_cs.inc` | 24/26 = 92,3 % |
| + Ziel „Edit > Preferences“ des Karten-Editors (Beschriftung NRC-Englisch) | **25/26 = 96,2 %** |

Die übrigen Listen unverändert (Hauptliste 99/99, Zweitliste 18/24, H2-Liste
42/48). Ehrlich: die verbleibende „Fehlfrage“ („vyzkoušet menu jako ve hře“,
Modus 4) liefert `tour:tut-menue@3` – genau den Schritt „wie im Spiel
ausprobieren“; in der Liste steht `@4`, weil ich beim Schreiben die Schritte
ab 1 statt ab 0 gezählt habe. Die Liste bleibt eingefroren, gezählt wird
streng. Abgestimmt wurde nach der ersten Messung nur das eine Ziel.

## Platz (Texte länger als Englisch)

- Selbsttest Karten-Editor Stufe 4 prüft jetzt auch Tschechisch: Seiten der
  Einstellungen rot (24 von 24: Baumeintrag „Automatické ukládání“ zu breit;
  danach Seite Brush: zwei lange Kästchen) → „Autoukládání“, kürzere Texte →
  0 von 24 rot. Plugin-Dialoge auf Tschechisch: alle Texte passen.
- Fotos durchgesehen: Zeitleiste „Otočení kamery“, die Knöpfe „Všechny
  dráhy / Aktivní dráha“, die Auswahl „Automaticky (rozpoznáno: …)“ und die
  Beschriftung der Kameravorschau waren abgeschnitten → „Švenk kamery“,
  „Všechny/Aktivní“, „Automaticky: %s“, „Klik: pohled kamerou“; neu
  aufgenommen, ganz.
- Modus 1 auf Tschechisch: 9 Fehler, 7 davon Prüfungen mit englischem Text
  („write-protected“, Klick „Every“, dahinter das Menü Ansicht) – jetzt über
  den Programmtext; danach 2646 OK, 2 Fehler (Zwischenablage, wechselnd von
  Lauf zu Lauf, geteilt mit den Testläufen der anderen Agenten).
- Karten-Editor Fotos (`BHED_KARTE_NUR=fotos`) auf Tschechisch: 77/1 – das
  Help-Menü 227 statt 247 breit, weil der Eintrag des Helfers als einziger
  „Begriffe wie NetRadiant“ nicht folgte → `trK` wie Manual und About:
  78 OK, 0 Fehler.

## Testläufe

- Laufaufbau: `bhed_q3map2.exe` neben `jkstudio.exe` und `BHED_Q3MAP2` (der
  Selbsttest sucht nur `build-test`/`build`). Ohne die exe hing der Bildlauf
  karte8 am Dialog „Build monitoring – Failed to execute …“ (vom Hauptstrang
  gemeldet, Lauf beendet, neu gestartet; auch die mp/duel1-Schritte von
  Stufe 4 scheiterten daran).
- Die Meldung „Build monitoring / Failed to execute the following command“
  bleibt englisch: sie ist NRCs `qt_MessageBox` 1:1 (watchbsp.cpp), steht
  gleichlautend in der Konsole zwischen q3map2-Ausgaben und ist in allen
  Sprachen englisch (de/zh/ja ebenso) – wie die NRC-Menüs.
- Stufe 4 (`BHED_KARTE_NUR=stufe4`) mit q3map2: Englisch 315 OK, 0 Fehler;
  Tschechisch 309 OK, 6 Fehler – genau die sechs Größenvergleiche der
  Plugin-Dialoge mit NRCs englischen Fotos (Pixelmaße), die auf Tschechisch
  breiter sind; kein Abschneiden (die Platzprüfung je Sprache ist grün).
  Versucht, sie mit „Begriffe wie NetRadiant“ sprachunabhängig zu machen: die
  Klicks der Liste werden schon beim Aufbau beschriftet, der Lauf fand
  „Zapnout“ nicht mehr – zurückgenommen; sie gelten wie bisher im
  englischen Lauf.

## Nicht übersetzt (und warum)

- **Menüs und Werkzeugleiste von NetRadiant-Custom** (KeL/KeT aus
  `tools/karte_menues.py`, 317 Texte): NRC gibt es nur auf Englisch; Auftrag
  „NetRadiant-Menünamen nur, wenn NRC sie übersetzt“ – wie Spanisch. Im
  Handbuch erscheinen sie über `{{Kennung}}` ebenso englisch; feste Nennungen
  („File > Open .bsp“, „Build > Customize“, „Flip“, „Room“) sind englisch
  geschrieben. Die Dialoge (Einstellungen, Plugin-Dialoge, Oberflächenprüfer)
  und die eigenen Menüpunkte von JK Studio sind übersetzt.
- **Plugin-Menüeinträge** (bobToolz, MeshTex …) und die Einträge der
  Build-Menü-Vorlage (`[Final] Fast` …) bleiben wie in NRC.
- **Spieltermini**: Entity-Klassen, Shader-Schlüssel, ICARUS-Befehle und
  -Werte, Cvars, Dateinamen, Programm-/Spiel-/Mod-Namen, q3map2-Schalter und
  -Ausgaben in Anführungszeichen („MAP LEAKED“, „Token too large“).
- Ravens Hilfetexte der Befehle (`Q3_Interface.h`/`behaved.bhc`) sind im
  Handbuch übersetzt (wie de/zh/ja), im Programm zeigt der Ereignis-Editor
  sie wie in allen Sprachen im Original.

## Glossar (Auszug)

Regeln: Vykání (Sie-Form) mit Imperativ Plural in Anleitungen („Klikněte“,
„Vyberte“); Knöpfe und Menübefehle im Infinitiv wie Windows/Office auf
Tschechisch („Uložit“, „Otevřít“, „Zpět“, „Znovu“). Anführungszeichen gerade
"…", Gedankenstrich „ - “ wie im Englischen (keine zusätzlichen Zeichen).
Star-Wars-Begriffe wie in den tschechischen Ausgaben (Síla, světelný meč,
padawan, temná strana); Fachbegriffe der Mapper-Szene bleiben (brush, patch,
caulk, shader, lightmapa).

| en | cs | en | cs |
|---|---|---|---|
| Events | Události | Script Flow | Průběh skriptu |
| command / block / macro | příkaz / blok / makro | event editor | editor událostí |
| Compile | Kompilovat | Preferences / Settings | Předvolby / Nastavení |
| Helper (Assistent) | Pomocník | tour / tutorial | prohlídka / tutoriál |
| guide | příručka | self-test | autotest |
| map editor / menu editor | editor map / editor menu | mission / scene | mise / scéna |
| cutscene | animační sekvence | timeline / track | časová osa / stopa |
| playhead | ukazatel přehrávání | key (camera key) | klíč (klíč kamery) |
| shot | záběr | figure / character | postava |
| walk target / look target | cíl chůze / cíl pohledu | waypoint | navigační bod |
| entity | entita | brush | brush (mn. brushe) |
| texture / shader | textura / shader | vertex / edge / face | vrchol / hrana / stěna |
| grid / snap | mřížka / přichytit | 2D view / camera | 2D pohled / kamera |
| clipper | ořezávač | light / bake | světlo / zapéct |
| real-time light | světlo v reálném čase | quick bake | rychlé zapečení |
| leak | únik | build (map) | sestavení / sestavit |
| layer / filter / region | vrstva / filtr / oblast | game folder | složka hry |
| lightsaber / the Force | světelný meč / Síla | force jump | skok Silou |
| spawn / spawner | spawn / spawner | undo history | historie kroků zpět |
| camera pan (Zeitleiste) | švenk kamery | autosave | autoukládání |

## Offen

- Pluralformen: Zähler stehen als „%d souborů“ (Genitiv Plural, für 0 und
  ab 5 richtig) oder „Uloženo: %d“; für 2–4 wäre „soubory“ richtig – das
  Programm kennt keine Pluralregeln je Sprache.
- Ravens Hilfetexte im Ereignis-Editor bleiben englisch (wie in allen
  Sprachen).

## English
- Czech (Čeština) as the sixth language: every program text, the whole guide with its pictures, the interactive tutorials and the helper (F1) in Czech. The NetRadiant-Custom menus and toolbar stay English, as in NetRadiant-Custom itself.

## Deutsch
- Tschechisch (Čeština) als sechste Sprache: alle Programmtexte, das ganze Handbuch mit seinen Bildern, die interaktiven Tutorials und der Helfer (F1) auf Tschechisch. Die Menüs und die Werkzeugleiste von NetRadiant-Custom bleiben englisch, wie in NetRadiant-Custom selbst.

## 中文
- 新增第六种语言捷克语(Čeština):所有程序文本、完整的手册及其图片、交互式教程和助手(F1)均提供捷克语版本。NetRadiant-Custom 的菜单和工具栏保持英文,与 NetRadiant-Custom 本身一致。

## 日本語
- 6 番目の言語としてチェコ語(Čeština)を追加:すべてのプログラムテキスト、画像付きのガイド全体、インタラクティブチュートリアル、ヘルパー(F1)がチェコ語になりました。NetRadiant-Custom のメニューとツールバーは NetRadiant-Custom 本体と同じく英語のままです。

## Čeština
- Čeština jako šestý jazyk: všechny texty programu, celá příručka s obrázky, interaktivní tutoriály a pomocník (F1) v češtině. Nabídky a panel nástrojů NetRadiant-Custom zůstávají anglicky, stejně jako v samotném NetRadiant-Custom.
