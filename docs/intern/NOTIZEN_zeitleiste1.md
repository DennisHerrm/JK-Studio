# Notizen zeitleiste1: neue Zeitleiste im Mission-Tab

Zweig `zeitleiste1` ab zeitleiste0 (23ffa48b), Worktree `Y:\mission`, Bau
`Y:/mission/build-mission`, Texte `tools/i18n_zeitleiste1.py` (noch nicht
angelegt). Auftrag: `<SP>\auftrag_zeitleiste1.md` + `<SP>\auftrag_gemeinsam.md`,
Vorlage `<SP>\zeitleiste_entwurf\entwurf_freigegeben.html`.

## Stand bei der Pause (29.10., 16:56, shank: "PAUSE um 17:00")

Fertig: nur gelesen, noch keine Codeaenderung, kein Bau, kein Test.

- Auftrag, gemeinsame Regeln, Entwurf (ganzer Quelltext, 796 Zeilen) und
  `NOTIZEN_zeitleiste0.md` gelesen.
- `include/bhed/timeline.h`, `src/timeline.cpp` gelesen (buildTimeline:
  TimelineEvent/Track, Kind Camera/Sound/Wait/Set/Move/Other/Use/Saber;
  Dauer nur bei wait/waitsignal/camera/move/rotate).

Was der Entwurf festlegt (Merkliste fuer den Nachbau in ImGui):

- Farben: Abspielkopf #f2a93b, Fahrt #5b9cf0, Schwenk #e3a24b, Zoom #9fb4d9,
  Blende #c9ccd3, Sprache #6fcf8a, Musik #b892f2, Laufen #e9c15a,
  Animation #4ec3c0, Warten schraffiert #3b4252/#333948, Hinweis #f2665c.
- Zeilen: Kamerabilder (50 px, alle 2 s), Uebersicht: "Kamera – Ort"
  (Block je MOVE bis zum naechsten, Name der Markierung bzw. "(x, y, z)",
  36 px), "Kamera – Drehung" (PAN, mind. 350 ms breit, "(Schnitt)" bei
  Dauer 0); Detail: Kamerafahrt/Schwenk/Zoom/Blende mit Unterbahnen (26 px
  je Bahn, Warnchip "springt zurueck"), Kurven Sichtfeld (FOV) und Blende
  (34 px, linear); Sprache (48 px, Sprecher fett, Untertitel klein,
  Wellenform unten), Musik (30 px), Skriptablauf (30 px, waits schraffiert
  "wartet auf ‚x‘" / "wartet n s", Marken set/use/sonst, Kartenwechsel rot
  "⇥"), Darsteller-Kopf (24 px), je Darsteller 40 px (oben Animationen ohne
  STAND/IDLE/BOTH_SITn, unten "laeuft 1, 2 …", gestrichelt "steht still"
  bei Luecken > 300 ms, unsichtbar schraffiert), aufgeklappt Animation +
  Befehle (je 30 px).
- Bedienung: Start mit 30-s-Ausschnitt, Alles, +/- (Faktor 1,6),
  Strg+Mausrad um den Zeiger (1,25/0,8), Navigationsleiste 16 px nur wenn
  hineingezoomt, Filter Nur aktive/Probleme, Suche, Tab Uebersicht/Detail,
  Leertaste, Pfeile (1 s, Umschalt 5 s), Pos1, Strg+Z.
- Ziehen: Raster 50 ms; Kamera verschieben + rechte Kante (8 px) Dauer;
  Saetze verschieben; waits nur Kante (alles danach rueckt nach); Hinweis
  waehrend des Ziehens mit der Skriptaenderung; geaenderte Bloecke
  gestrichelt umrandet.
- "Was passiert hier?": Texte je Art (shot/dialog/wait/ev/lauf/anim/film/
  musik) siehe Entwurf Zeilen 293-348.

## Korrektur von shank (29.10., gilt nach der Pause)

Der obere Teil des Entwurfs (Kamerabild, Draufsicht, "Was passiert hier?")
war nur zum Verstehen da und wird NICHT als eigene neue Fenster gebaut.
Uebernommen wird die Zeitleiste selbst.

- Punkt 5: nur der Filmstreifen in der Zeitleiste, kein zusaetzliches
  Vorschaubild; die Vorschau ist die vorhandene 3D-Ansicht des Mission-Tabs,
  die dem Abspielkopf folgt.
- Punkt 9 (Draufsicht) entfaellt als eigenes Panel; Wege und Halte zeigt die
  vorhandene 3D-Ansicht mit ihren Bahnen und Griffen.
- Punkt 6: die Erklaerung beim Klick kommt in die vorhandene rechte
  Seitenleiste des Mission-Tabs (wo heute Script Flow/Eigenschaften stehen),
  mit "Skriptzeile oeffnen" - kein neues Fenster.

## Arbeitsstand (laufend)

- Punkt 1: `include/bhed/spuren.h`, `src/spuren.cpp`, Kern-Test
  `tests/spurentest.cpp` (ctest `spuren`, eigene Mini-Szene) - gruen.
  Zeitleiste und Feldansicht "Spuren" zeichnen nur noch daraus
  (`spurenBauen` liest das Modell, `kategorieVon` ist weg).
- GUI: `gui/app_zeitleiste.cpp` (Zeilen, Bloecke, Marken, Navigationsleiste,
  Ziehen, Erklaerung), Transportleiste in `drawTimeline` (Uebersicht/Detail,
  Nur aktive, Probleme, Suche, -/Alles/+), Erklaerung unten in der
  Script-Flow-Spalte (`drawFlowArea`). Texte `tools/i18n_zeitleiste1.py`.
- Selbsttest `zeitleiste1` (`gui/selbsttest_zeitleiste1.inc`, mit
  `BHED_ZL1_NUR=mini` nur die Mini-Szene); alte Tests (zeitleiste,
  werkzeuge, handbuch, vorschau_b) auf die neuen Zeilen umgestellt.
- Handbuch: Seite `zeitleiste` neu (inhalt_szene.py), Texte auf anderen
  Seiten nachgezogen; Bilder ueber `BHED_HB_NUR=zeitleiste1`.

## Weitermachen

1. Bestand lesen: `drawTimeline` (`gui/app_view3d_bedienung.inc` ab ~858),
   `spurenBauen`/`drawSpurenAnsicht` (`gui/app_ansichten.cpp` 1070-1446),
   `prepareScene` (`gui/app_view3d.cpp` ~5735), `include/bhed/ablauf.h`,
   `camtrack.h`, `scene.h`, Export `gui/selbsttest_zeitleiste0.inc`.
2. Punkt 1 des Auftrags: `include/bhed/spuren.h` + `src/spuren.cpp` (ohne
   ImGui), Kern-Test `tests/spurentest.cpp` mit Mini-Szene, Leiste und
   Spurenansicht darauf umstellen, Gegenprobe mit dem Export zeitleiste0.
3. Danach Punkte 2-12 der Reihe nach (Auftrag).

Befehle:

- Bauen: `cmake --build Y:/mission/build-mission --config Release --parallel 4 --target behaved`
- Selbsttest: `sh /y/mission_lauf.sh <modus> <name> [sekunden] [VAR=wert ...]`
  (Laufordner `/y/mission_laeufe/<name>`, Fotos dort unter `fotos`)
- Export-Gegenprobe: `sh /y/mission_lauf.sh zeitleiste0 <name> 1500 BHED_ZL0_AUS=<Ordner>`
- Fehlt Y:: `cmd //c "subst Y: <SP>\wp"`

Keine eigenen Prozesse laufen; Worktree ausser dieser Datei unveraendert.
