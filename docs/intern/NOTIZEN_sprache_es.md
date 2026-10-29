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

Commit 1369f4a8, Signal `SP\sprache_basis_fertig.txt`.

## Teil 2: Spanisch

- **Programmtexte** (`tools/i18n_sprache_es.py`): alle 2365 Texte mit Wörtern
  (2695 insgesamt; 13 ohne Wörter gelten wie Englisch; 317 NRC-Menü- und
  Leistennamen bleiben englisch, s. u.). `lint_i18n.py --sprache es`: 0 fehlend.
  Placeholder-Prüfung im Erzeuger (gleiche `%`-Folge wie Englisch).
- **Handbuch** (`tools/handbuch/inhalt_es.py`): 3292 Texte + 15 eigene
  Beschriftungen, `gen_handbuch.py --pflicht=es`: 0 Texte, 0 Bilder englisch.
  Bilder: alle 575 wie Deutsch, aus dem Selbsttest auf Spanisch
  (`BHED_EDITORTEST=handbuch`, `BHED_SPRACHE=es`, 14 Bildsätze, dazu
  `BHED_EDITORTEST=menueeditor` für die Menü-Editor-Bilder); jeder Lauf
  0 FEHLER. Die Bilder wurden auf abgeschnittene Texte durchgesehen (s. Platz).
  Tastennamen wie auf spanischen Tastaturen (Mayús, Supr, Intro, Inicio, Fin,
  RePág/AvPág im Text). Drei Seiten nannten „vier Sprachen“ – jetzt sechs, in
  allen vier alten Sprachen nachgezogen (inhalt_funktionen, inhalt_schritte2,
  inhalt_hilfe1 und uebersetzung/*.json).
- **Helfer**: eingefrorene Liste `tests/hilfe_fragen_es.inc` (28 Fragen,
  Commit 32a2dc12 vor den Wortlisten): Grundlinie 10/28 (35,7 %), nach
  Übersetzung von Handbuch/Touren 17/28, mit den spanischen Begriffen und
  Zielen 27/28 = 96,4 % (Schranke 85 %). Daneben: „buscar actualisaciones“
  (Tippfehler → Tour k-update statt Menüeintrag). Die Ziele sind mit Kenntnis
  der Liste geschrieben; darum zusätzlich 8 unabhängige Fragen danach (nicht
  angepasst): 7/8 in den Top 3 (daneben: „reproducir música en la escena“).
- **Platz**: die Breitenprüfungen der Map-Editor-Dialoge (Einstellungsseiten,
  Plugin-Dialoge, Stufe 4/6) laufen jetzt über alle Sprachen statt 4.
  Einstellungen (`BHED_STUFE4_NUR=reste`, alter Stand): 24 FEHLER nur in es –
  Baum „Inspector de superf…“ (jede Seite) und drei zu lange Haken (Brush,
  Inspector, Navegador de texturas); gekürzt (Baum „Inspector sup.“,
  „Nav. de texturas“, „Nav. de modelos“; die Seitentitel bleiben lang) →
  156/1 (Brush: die Beschriftungsspalte „Escala de textura predeterminada“
  schob den Haken hinaus) → „Escala de textura“, „Brush en mapas nuevos“ →
  157 OK, 0 FEHLER.
  Plugin-Dialoge: „N.º de mosaicos“ (MeshTex Set S/T Scale) lief ins
  Zahlenfeld, die Prüfung sah es nicht (Platz = eigener Hinweis des
  Radioknopfs) – `pluginTexteGanz` prüft jetzt auch, dass kein Text in ein
  fremdes Feld rechts von ihm ragt: rot 89/1 (es), nach „Mosaicos“ grün 90/0.
  Aus den Handbuchbildern: Zeitleiste „Movimiento de cámara“/„Giro de cámara“,
  Kamerafeld „Todos los trayectos“/„Trayecto activo“, Entity-Liste
  „Destinos de caminar (navgoal)“ (Zähler verdrängt), Kombi „Automático
  (detectado: …)“ abgeschnitten – gekürzt auf Movimiento/Giro, Todos/Activo,
  „Destinos (navgoal)“, „Automático: %s“.

### Glossar (Auszug; Spanisch aus Spanien mit „tú“ wie das Spiel)

Begriffe aus den spanischen Texten von Jedi Academy (assets0.pk3,
strings/spanish), sonst verbreitete Übersetzung:

| Englisch | Spanisch | Quelle |
|---|---|---|
| Force / Force power | Fuerza / poder de la Fuerza | JKA |
| lightsaber / saber / hilt | sable láser / sable / empuñadura | JKA |
| Force jump, push, pull | salto de Fuerza, empujón, tirón | JKA |
| game (session), save, load | partida, Guardar, Cargar | JKA |
| map, level, mission | mapa, nivel, misión | JKA |
| multiplayer, single player | multijugador, un jugador | JKA |
| Jedi Knight, Jedi Master | Caballero jedi, Maestro jedi | JKA |
| mouse, key | ratón, tecla | JKA |
| cutscene | cinemática | verbreitet |
| shot, camera move | plano, movimiento de cámara | Film |
| timeline, track, key(frame) | línea de tiempo, pista, clave | verbreitet |
| playhead | cabezal | verbreitet |
| preview | vista previa | Windows |
| settings / preferences | ajustes / preferencias | Windows |
| File, Edit, View, Help | Archivo, Edición, Ver, Ayuda | Windows |
| undo / redo | deshacer / rehacer | Windows |
| helper (F1) | asistente | – |
| guide | guía | – |
| tab | pestaña | Windows |
| walk target, waypoint | destino, waypoint | Mapper |
| grid, layer, entity | rejilla, capa, entidad | Mapper |
| brush, patch, shader, caulk, leak | brush, patch, shader, caulk, fuga (leak) | Mapper (bleiben) |
| bake, quick bake | hornear, horneado rápido | 3D-Gemeinde |
| script runner | ejecutor de scripts | – |
| compile | compilar | – |

**Nicht übersetzt** (Auftrag „wo Spiel/Mapper nicht übersetzen“): ICARUS-
Befehle und -Werte (`affect`, `SET_NAVGOAL`, `FLUSH`), Entity-Klassen und
-Schlüssel (`info_player_start`, `targetname`, `spawnflags`), Shader-
Schlüssel, Cvars, q3map2-Schalter, Dateinamen und Pfade, Produktnamen; die
Menü- und Werkzeugleistennamen von NetRadiant-Custom (KeL/KeT aus
`karte_menues.py`, 317 Texte) und die Einträge des Plugins-Menüs (bobToolz
„Explode Entity“ …), weil NRC sie nicht übersetzt – Dialoge, Tooltips und
Hinweise des Karten-Editors sind übersetzt; „Begriffe wie NetRadiant“ zeigt
weiter alles NRC-Englisch. Raven-Beschreibungen der `SET_`-Felder und der
Kamerabefehle im Handbuch sind übersetzt (Deutsch übersetzte sie ebenfalls).

## Schlussrunde

- Sauberer Neubau `build-sprache` (`--clean-first`, nach dem letzten
  Quelltext): 0 Warnungen; ctest 160/160; hilfetest es 27/28, Haupt 99/99,
  Zweit 18/24, H2 42/48.
- Selbsttests auf Spanisch (Einstellungen `language=es`): `pruefung` 24 OK,
  0 FEHLER; Karten-Editor `BHED_KARTE_NUR=fotos` 78 OK, 0 FEHLER; Modus 1
  2648 OK, 0 FEHLER (mit `BHED_HANDBUCH`, sonst findet der Lauf ausserhalb
  des Baus keine touren.json; zwei frühere Läufe hatten wechselnde
  Zwischenablage-Fehler, während andere Läufe auf dem Rechner liefen).
  Drei Prüfungen gingen still von Englisch aus und waren auf Spanisch rot:
  Modus 1 suchte „write-protected“ in der Frage und klickte „Every“ (Find
  similar; Folgefehler: View-Menü blieb zu) – jetzt aus `tr(AskUnprotect)`
  bzw. `tr(FindSimilarAny)`; die Karten-Editor-Fotos verglichen Help mit dem
  NRC-Foto, die Breite setzt aber der eigene Eintrag „Helper: what do you
  want to do?“ (es 233 statt 247) – die Fotos laufen jetzt auf Englisch
  (NRC-Fotos sind englisch), danach wieder die Sprache des Laufs.
- Karten-Editor Stufe 4 (alle 6 Sprachen): `reste` 157 OK / 0, `texte`
  90 OK / 0.
- `lint_sources` ok, `lint_fassung` ok (rc606), `lint_i18n --sprache es` ok
  (0 fehlend), `lint_i18n_erzeugt` ok, `gen_handbuch --pruefen --pflicht=es`
  ok (handbuch_sprachen ok).

## Offen / Hinweise für CS

- NRC-Menünamen (KeL/KeT aus `karte_menues.py`) bleiben in Spanisch englisch
  (Auftrag: nur übersetzen, wenn NRC sie übersetzt – NRC hat nur Englisch).
- Neue Texte anderer Zweige (karte9d, menue2, mission1) kommen beim Integrieren
  in vier Sprachen; sie zeigen auf Spanisch englisch, bis
  `tools/i18n_sprache_es.py` bzw. `tools/handbuch/inhalt_es.py` ergänzt sind
  (`lint_i18n.py --sprache es`, `gen_handbuch.py --pflicht=es` nennen sie).
- Fest im Code, darum in jeder Sprache englisch: die Auflösungsliste des
  Menü-Editors beginnt mit „Window“ (`gui/app_menue.cpp`, Zweig menue) – nicht
  angefasst, um menue2 nicht zu stören.
- `lint_i18n.py` ohne `--sprache` meldet schon auf der Basis 2406e182 706
  Beanstandungen (696 „nirgends benutzt“, weil es `gui/*.inc` nicht liest,
  z. B. `Str::MenuView` in `app_anordnung.inc`; 10 feste Texte im
  Menü-Editor/Karten-Editor) – unverändert, nicht aus diesem Zweig.

## Changelog

## English
- New languages: **Spanish** (complete: program, map editor, menu editor, helper, tutorials and the guide) and the groundwork for **Czech**. JK Studio starts in the language of Windows; change it under View > Language (also in the map editor). A text a language does not have yet appears in English, never empty.
- The guide has six languages with a language switch; its search ignores accents ("animacion" finds "animación").
- The helper (F1) understands Spanish questions and ignores accents.

## Deutsch
- Neue Sprachen: **Spanisch** (vollständig: Programm, Karten-Editor, Menü-Editor, Helfer, Tutorials und Handbuch) und die Grundlage für **Tschechisch**. JK Studio startet in der Sprache von Windows; umstellen unter Ansicht > Sprache (auch im Karten-Editor). Einen Text, den eine Sprache noch nicht hat, zeigt es englisch – nie leer.
- Das Handbuch hat sechs Sprachen mit Sprachumschalter; seine Suche übergeht Akzente („animacion“ findet „animación“).
- Der Helfer (F1) versteht spanische Fragen und übergeht Akzente.

## 中文
- 新增语言:**西班牙语**(完整:程序、地图编辑器、菜单编辑器、助手、教程和手册)以及**捷克语**的基础。JK Studio 以 Windows 的语言启动;可在“视图 > 语言”中更改(地图编辑器中也可以)。某种语言尚未翻译的文字以英文显示,绝不留空。
- 手册支持六种语言并带语言切换;搜索忽略重音符号(“animacion”可找到“animación”)。
- 助手(F1)能理解西班牙语问题并忽略重音符号。

## 日本語
- 新しい言語:**スペイン語**(プログラム、マップエディター、メニューエディター、ヘルパー、チュートリアル、ガイドのすべて)と**チェコ語**の基盤。JK Studio は Windows の言語で起動し、「表示 > 言語」で変更できます(マップエディターでも可能)。まだ翻訳されていない文字は英語で表示され、空欄にはなりません。
- ガイドは 6 言語に対応し、言語切り替えがあります。検索はアクセント記号を無視します(「animacion」で「animación」が見つかります)。
- ヘルパー(F1)はスペイン語の質問を理解し、アクセント記号を無視します。

## Español
- Idiomas nuevos: **español** (completo: programa, editor de mapas, editor de menús, asistente, tutoriales y guía) y la base para el **checo**. JK Studio empieza en el idioma de Windows; cámbialo en Ver > Idioma (también en el editor de mapas). Un texto que un idioma aún no tiene aparece en inglés, nunca vacío.
- La guía tiene seis idiomas con selector de idioma; su búsqueda ignora los acentos ("animacion" encuentra "animación").
- El asistente (F1) entiende preguntas en español e ignora los acentos.
