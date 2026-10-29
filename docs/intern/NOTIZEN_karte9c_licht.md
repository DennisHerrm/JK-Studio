# Stufe 9c: Tester-Feedback – Werkzeugleisten und Echtzeit-Licht „wie im Spiel“

Arbeitskopie `Y:\licht`, Zweig `licht9c`, ab `83ca36cd` (karte1). Dateiname: karte1 brachte mit
6fe2c104 eine eigene `NOTIZEN_karte9c.md` (Stufe 9 Teil 3, Asset-Browser) – beim Merge bleibt die
von karte1 unter ihrem Namen, diese hier heißt `NOTIZEN_karte9c_licht.md`. Wunsch eines erfahrenen
JKA-Mappers, shank: „also dass das Lighting richtig geht“. Erledigt: **1** (Leisten), **2**
(Echtzeit-Licht wie im Spiel, mit Messung vorher/nachher), **3** (Handbuch, i18n), **4**
(Schlussrunde), **5** (diese Notizen).

## Commits

| Punkt | Commit | Inhalt |
|---|---|---|
| 2 | 86f46b31 | Messgerät: Echtzeit-Bild gegen gebackenes Bild (BHED_KARTE_NUR=licht9c) |
| 2 | 64423317 | Licht wie q3map2: Himmel beendet den Sonnenstrahl, Schattenregeln, Flächenlicht, 512 Lichter, Kacheln |
| 1 | 615fc753 | Leisten verschiebbar wie NRCs QToolBars, Patch-Schatten (Build-Menü) |
| 2 | ff1da765 | Tempo auf großen Karten (Fernfeld, Würfelseiten, Hülle per Halbierung) |
| 1/2 | 9d745829 | Selbsttests Leisten (auch über echten Neustart) und Himmel-Sonne |
| 3 | 4aa77847 | Handbuch (4 Sprachen) |
| 2 | 08449b12 | Himmelslicht-Farben aus den Himmelsbildern (SkyProbes), Lichtfarbe fehlender Bilder, Grenzwerte |
| 2 | 2fdf0961 | eine Schattenprobe für schwache Lichter; Prüfung Durchsichtig (Spiel-Shader) |
| – | 43eaafd2 | Merge karte1 (6731aa8d Optimierer „Fix all safe“) |
| – | 819975bb | Schlussrunde: Sollmaß Menü Build mit „Patch shadows“ (264 × 748) |
| – | 2e87ee57 | Merge karte1 (18995492 Selbsttest optsicher) |
| 3 | 15f744fd | Handbuchbilder (4 Sprachen): Himmel-Sonne, angedockte Leisten; Stufe-7-Bilder neu |
| – | 9e58d0d7 | Notizen (erste Fassung) |
| 2 | 589dfd9b | Durchsichtige Shader mit dem Bild ihrer ersten Stufe (Alpha wie im Spiel) |
| – | db3b6fc9 | Merge karte1 (6fe2c104 Asset-Browser, Texturen überblenden); danach laut Hauptstrang kein weiterer Merge |
| – | (dieser) | Notizen |

## Punkt 1: Werkzeugleisten

- **Versteckte Knöpfe**: wie NRC (gtkmisc.cpp:147 `g_toolbarHiddenButtons`) sind OpenMap,
  SaveMap, Undo, Redo, SelectTouching ab Werk versteckt; das Kontextmenü der Leiste blendet sie
  ein (gab es schon, Vorgabe bleibt). Auffindbar: der Griff jeder Leiste hat einen Tooltip
  („Rechtsklick … ab Werk versteckt, wie in NetRadiant-Custom“, 4 Sprachen), dazu ein Absatz
  im Handbuch (Karten-Editor > Menüs, Fenster und Anordnungen).
- **Verschiebbar wie QToolBars**: der Griff (gepunkteter Streifen) zieht Haupt-, Filter- und
  Plugin-Leiste an einen anderen Rand; beim Ziehen zeigt ein Rahmen den Zielrand (der nächste
  Rand des Bereichs zwischen Menü- und Statuszeile), die Reihenfolge folgt der Mauslage entlang
  des Rands (QMainWindowLayout hover/drop). Gespeichert als `BhedToolbarDock`
  („haupt=oben:0;filter=rechts:0;plugin=rechts:1“). `karte::leistenAndocken`
  (src/karte/teiler.cpp): oben/unten über die ganze Breite, links/rechts dazwischen, je Rand
  eine Zeile mit fitLayout; die Mitte bekommt die Teiler wie bisher.
- Tests: ctest karte_teiler (Vorgabe, links/unten, zwei Leisten an einem Rand, Text hin und
  zurück); Selbsttest stufe9c (Griffe ziehen, Einstellungen neu lesen, zurück); echter Neustart
  `BHED_KARTE_NUR=leisten1/leisten2` mit einem Einstellungsordner (`SP/s9c/leisten_neustart.sh`):
  Lauf 1 Hauptleiste links, Filterleiste unten → Lauf 2 nach dem Neustart gleich
  („haupt=links:0;filter=unten:0;plugin=rechts:1“). Der eine FEHLER in Lauf 2 („New map Brush
  Type: Brush Primitives“) kommt von Lauf 1 (die Selbsttest-Einrichtung stellt Axial ein und
  speichert) – wie beim Split-Neustart der Stufe 8b, nicht von den Leisten.
- Nicht: schwebende (abgerissene) Leisten, mehrere Zeilen je Rand.

## Punkt 2: Echtzeit-Licht wie im Spiel

### Was q3map2 tut und die Vorschau jetzt auch

- **Sonne** (light_trace.cpp TraceLine mit testAll, 1702-1755): der Strahl zur Sonne läuft bis
  zum ersten festen Blatt; hell nur, wenn er davor eine Himmelsfläche traf (C_SKY) und kein
  deckendes Dreieck. Nachgebildet mit zwei Karten je Sonne: **S** = größte Tiefe der
  Himmelsflächen, die von der Sonne wegzeigen (die der Stelle nächste), **O** = kleinste Tiefe
  der Verdecker unter S (deckende Dreiecke, Volumen strukturell deckender Pinsel, alphashadow;
  die übrigen Seiten der Himmelspinsel nicht). Hell, wenn S da ist, t ≥ S und t ≤ O. Damit wirft
  der Himmels-Brush (caulk, Deckel darüber) **keinen Schatten mehr auf seine Sonne** (gemeldeter
  Fehler; vorher galt nur ein Shader mit Skybox-Bild als Himmel, caulk und Himmel ohne farbox
  verdeckten).
- **Was blockiert** (PopulateWithBSPModel, SetupTraceNodes_r): castShadows (worldspawn und
  func_group 1, andere Entities `_castShadows`/`_cs`, sonst 0), kein C_NODRAW, durchscheinend
  nur mit alphashadow/lightfilter (Alpha-Test mit dem Bild), Patches nur mit -patchshadows
  (neu: **Build > Patch shadows**, geht auch an das Schnell-Backen), strukturell deckende Pinsel
  als Volumen für Punkt-/Spot-/Flächenlicht (TraceLine ohne testAll hält an festen Blättern).
- **Shader-Licht**: `sun` (SOF2-Schreibweise, in JA-Himmeln üblich – fehlte!), jede
  `q3map_sun`/`q3map_sunExt`, `q3map_skyLight` (CreateSkyLights mit Horizont und **SkyProbes**:
  Farbe je Richtung aus den sechs skyParms-Bildern, auf das Größte normiert), `q3map_surfacelight`
  je Dreieck unterteilt nach `q3map_lightSubdivide` (RadSubdivideDiffuseLight,
  RadClipWindingEpsilon) mit **PointToPolygonFormFactor** (wie -fast ohne -faster; vorher die
  -faster-Näherung) und Hülle wie SetupEnvelopes, Rückschein als Flächenlicht
  (`q3map_backsplash`, ab Werk 5 % / 23), Farbe aus `q3map_lightRGB` bzw. `q3map_lightImage`
  bzw. dem Shaderbild (fehlt es: weiß wie q3map2s `*default`). `_ambient`/`_minlight` wie bisher.
- **Lichter in der Ferne**: jedes Licht, dessen Hülle das Bild trifft, Rang nach dem Beitrag am
  Auge; bis 512 je Bild in einem Puffer, je Kachel (32 Bildpunkte) eine Bitmaske. Die ersten 128
  mit Schattenwürfel (32 × 512², 96 × 256²), wer auf seinen Würfel wartet, fehlt ein Bild lang
  (statt durch Wände zu leuchten), die übrigen ohne Schatten.
- **Schattenqualität**: Würfel 5 Proben (PCF) mit Versatz entlang der Normale um gut einen Texel,
  Sonne 3 × 3 Proben in zwei Kaskaden (ganze Karte 2048², um die Kamera 3072 Einheiten 2048²,
  auf Texel gerundet), Himmelssonnen je eine 1024²-Karte (höchstens 16, mehr: gleichmäßig
  ausgewählt, Energie erhalten); Rückseiten werden mitgezeichnet.
- **Wie im Spiel gezeichnet**: mit Echtzeit-Licht und gebacken fehlen nodraw-Flächen (caulk,
  clip …), alphaFunc und blendFunc der ersten Stufe gelten (statt qer_trans/qer_alphafunc), mit
  dem Bild dieser Stufe (map/clampMap/animMap, mit Alpha – qer_editorimage ist oft ein .jpg ohne
  Alpha); gebacken wird der Himmel mitgezeichnet. Selbsttest: 00mj_larshomestead/lh_net_metal
  (alphaFunc) als Gitter im Himmelsraum, Foto `karteneditor_s9c_gitter` – durch die Maschen sieht
  man die Wand dahinter.
- Vorschau bleibt ohne Bounce (beschriftet wie bisher).

### Messung (Echtzeit-Bild gegen Schnell-Backen, gezeichnet wie im Spiel)

`BHED_KARTE_NUR=licht9c`, `SP/s9c/messen.sh`: je Karte die ersten Startpunkte (sonst unter den
ersten Lichtern), Kamera 40 über dem Punkt, 12° nach unten; Abweichung der Helligkeit
(0,299 R + 0,587 G + 0,114 B, 0..255) über Blöcke von 16 × 16 Bildpunkten: Mittel und größte.
Dekompilierte Karten haben keine Lichter (q3map2 entfernt sie) – sie werden aus der .bsp
geschätzt (lichtschaetzung). Testerkarte: Kopie von md_rev_TarisUpper2.bsp, mit bhed_q3map2
-convert dekompiliert (`SP/s9c/taris`). Vorher = Bau 86f46b31 (nur Messgerät), nachher = 08449b12 mit der Durchsichtig-Prüfung (Lauf nachher2).

| Karte | Punkt (Ort / Blick) | vorher Mittel | vorher Max | nachher Mittel | nachher Max |
|---|---|---|---|---|---|
| md_ch_ben | P0 (354 25 32 / 180) | 33,24 | 138,25 | **0,83** | **30,25** |
| | P1 (3532 403 604 / 120) | 52,01 | 174,85 | **0,44** | **7,20** |
| | P2 (588 6163 476 / 240) | 48,26 | 162,11 | **0,50** | **6,60** |
| mp/duel1 | P0 (768 40 64 / 90) | 1,16 | 27,63 | **0,72** | **17,61** |
| | P1 (832 816 192 / 270) | 2,96 | 115,32 | **0,42** | **27,43** |
| | P2 (472 128 96 / 0) | 2,43 | 119,24 | **0,72** | **16,51** |
| md_eitd_jedi (q3map_skyLight + Sonne) | P0 (8448 3885 -274 / 315) | 95,42 | 255,00 | **0,17** | **32,41** |
| Testerkarte md_rev_TarisUpper2 | P0 (-7248 -6416 72 / 0) | 1,91 | 151,96 | **1,14** | **148,11** |
| | P1 (-5646 7188 1278 / 120) | 34,20 | 182,58 | **0,00** | **0,00** |
| | P2 (-498 7628 1943 / 240) | 67,82 | 198,58 | **4,86** | **188,31** |

Je Karte (Mittel über die Punkte / größte): md_ch_ben 44,50 / 174,85 → **0,59 / 30,25**;
mp/duel1 2,18 / 119,24 → **0,62 / 27,43**; md_eitd_jedi 95,42 / 255 → **0,17 / 32,41**;
Taris 34,64 / 198,58 → **2,00 / 188,31**. Lichter (alle / vorher): md_ch_ben 192 / 91,
mp/duel1 608 / 160, md_eitd_jedi 102 / 1 (die Himmelslichter fehlten), Taris 7540 / 1090
(Flächenlicht jetzt je Dreieck und Stück). Zwischenstand 64423317 (ohne SkyProbes): md_ch_ben
11,00 / 53,79 – die Himmelslichter (`q3map_skyLight 50 6`) waren mit si.color statt der
Skybox-Farben ~20 % zu hell. Bilder: `SP/s9c/bilder_vorher`, `SP/s9c/bilder_nachher2`
(je Punkt `_echt.bmp` / `_gebacken.bmp`), Protokolle `SP/s9c/messung_*.txt`.
Testräume (Selbsttest stufe9c): Punktlicht 0,12 / 0,80; Himmel mit Sonne und Flächenlicht
1,65 / 6,28 → 0,40 / 8,45 (fehlendes Shaderbild: weiß wie q3map2s `*default`, nicht die
notex-Farbe des Editors).

Grenzwert im Selbsttest: Karten aus `BHED_LICHT_KARTEN` je Punkt Mittel ≤ 15, Max ≤ 200;
Testräume im Selbsttest stufe9c immer: Himmelsraum (Sonne + Flächenlicht durch den
Himmels-Brush) Mittel ≤ 4 / Max ≤ 16 (ist 0,40 / 8,45), Punktlicht-Raum ≤ 1 / 4 (ist 0,14 / 0,80),
Boden unter dem Himmel ±20 % (ist 103 gegen 101). Rot vorher: md_eitd_jedi Boden schwarz
(Sonne verdeckt, Block-Mittel 95,4).

Der Rest ist vor allem Auflösung (ein Luxel je 16 Einheiten, die Vorschau rechnet je
Bildpunkt – Spitzen unter Lichtern) und Ränder; Taris P1 zeigt nur Flächen ohne Lightmap.

### Bildzeit (Kamera dreht 240 Bilder, Echtzeit-Licht an)

| Karte | vorher 86f46b31 | 64423317 | ff1da765 | 2fdf0961 |
|---|---|---|---|---|
| Taris (7493 Lichter, 512 je Bild; vorher 1043 / 64) | 11,37 (p99 16,66) | 46,89 (62,51) | **15,51 (20,91)**, 15,43 (20,97) | 23,97 (33,48) unter Last |
| duel_mos_espa (172 / 166; vorher 43 / 43) | 10,22 (16,77) | 5,45 (16,74) | 9,02 (12,37) | 5,31 (13,70) |

Mittel in ms (p99), Bildabstand der ganzen Anwendung (alle Ansichten). Mit dem Würfel-Budget
2 statt 8 und 1 statt 4 Mio. Dreiecken: 15,71 ms – nicht das Nadelöhr; mit 256 statt 512
Lichtern: 8,84 ms (p99 16,80) – die Schleife je Bildpunkt kostet. Ziel < 16 ms Mittel auf der
RTX 4070: auf ruhiger Maschine erreicht (15,4–15,5 ms), unter der Last der anderen Agenten
derselbe Stand 23,6–24,0 ms.

Hinweis: die beiden anderen Agenten bauten und testeten gleichzeitig auf derselben Maschine
(RTX 4070) – Laufzeiten schwanken; duel_mos_espa lag zwischen 5,5 und 9,0 ms.

## Punkt 3: Handbuch und i18n

- Texte: `tools/i18n_karte9c.py` (Griff-Tooltip, Patch shadows), `gen_i18n.py`; Handbuch
  `inhalt_karteneditor.py` (Leisten: versteckte Knöpfe, Griff, Bild `h_karte9c_leisten`),
  `inhalt_karte7.py` (Lichtzahl, neuer Abschnitt „Echtzeit-Licht wie im Spiel“ mit Bild
  `h_karte9c_himmel` und der Messung). ctest handbuch_sprachen grün.
- Bilder: `BHED_EDITORTEST=handbuch BHED_HB_NUR=karte9c` (neu) und `karte7` (Build-Menü mit dem
  neuen Eintrag, das neue Licht) je Sprache (`SP/s9c/hb_fotos.sh`), `tools/handbuch/bilder.py`.

## Schlussrunde

Skript `SP/s9c/ende_s9c.sh` (nach `SP/s9b/ende_s9b.sh`, Pfade `Y:/licht/build-licht`), Summe
`SP/s9c/ende_summe.txt`. Stand: nach dem ersten Merge von karte1 (43eaafd2).

| Schritt | Ergebnis |
|---|---|
| Bau | ok |
| ctest gesamt | **148/148** (276 s) |
| karteneditor voll (mit großer Karte) | 3176 OK, 14 FEHLER (979 s, Spitze 1083 MB) → 1 davon von 9c: Menü Build 241 × 724 statt 264 × 748 (neuer Eintrag „Patch shadows“) – Sollwert angepasst (819975bb), `BHED_KARTE_NUR=fotos` danach **78 OK, 0 FEHLER**; die übrigen **13** (Optimierer „verdeckt“, „Jede Prüfung schlägt an“, Beheben/Anwenden/Rückgängig) kommen aus karte1 6731aa8d: die Testkarte nutzt die unbekannten Shader `test/a`/`test/b`, die neue Regel „nur Pinsel mit bekannten, ganz deckenden Shadern“ unterdrückt dort „verdeckt“ – nicht von 9c, an den Karten-Agenten |
| Stufe 7 im Lauf: große Karte mit Echtzeit-Licht | 172 Lichter, 164 je Bild, Mittel 6,40 ms, p99 12,19 ms |
| Stufe 9c im Lauf | Leisten, Himmel (echt 103 / gebacken 101), Testräume 0,40 / 8,18 und 0,14 / 0,81, Durchsichtig – alle OK |
| Modus 1 | **2645 OK, 0 FEHLER** (439 s) |
| übrige Modi | nicht nötig – außerhalb des Karten-Editors nur neue i18n-Texte |
| Kartenprüfung (278 MD-Karten, gegen 9b) | alle Summen gleich (0 Abstürze, 0 nicht geladen, 0 D3D-Meldungen, 1 → 1 nicht gezeichnete Entity) (549 s) |
| sauberer Neubau | **0 Fehler, 0 Warnungen** (522 s) |
| Leisten über echten Neustart | Lauf 1 13 OK / 0, Lauf 2 OK (1 FEHLER von Lauf 1: Brush-Typ, s. Punkt 1) |
| nach 589dfd9b (Stufenbild) und dem zweiten Merge: Bau, ctest | ok, **148/148** (245 s) |
| karteneditor voll (zweiter Lauf) | **3177 OK, 13 FEHLER** (980 s, Spitze 1049 MB) – nur noch die 13 aus karte1 6731aa8d (Optimierer, s. o.); Stufe 9c im Lauf 23 OK |
| nach dem dritten Merge (db3b6fc9): Bau, ctest | ok (ein erster Bau scheiterte am gleichzeitigen Kopieren von `data/` – mit `-m:1` wiederholt: 0 Fehler, 0 Warnungen), **149/149** (222 s) |
| karteneditor voll (dritter Lauf) | **3185 OK, 14 FEHLER** (1048 s, Spitze 1046 MB) – die 13 aus karte1 6731aa8d und neu „Menü Modify > Repeat 225 × 154“ (hier 234): die Liste „Transforms“ im Repeat-Menü wächst mit den neuen Stufe-9-Schritten von karte1 6fe2c104, die vor den Fotos laufen – nicht von 9c |

## Merge karte1

Zweimal `git merge karte1`:
- **43eaafd2** (karte1 6731aa8d, Optimierer „Fix all safe“): Konflikte in
  `include/bhed/karte/optimierer.h`, `src/karte/optimierer_pruefen.cpp` (beide lesen die erste
  Stufe – zusammengeführt: `durchsichtig` von karte1 und alphaFunc/blendFunc von 9c in einem
  Durchgang, GL_ONE GL_ZERO deckend), `gui/selbsttest_handbuch.inc` (beide Deklarationen und
  Includes), `handbuch/*/suche.js` (neu erzeugt), i18n neu erzeugt. Danach ctest karte_echtlicht,
  karte_optimierer, karte_teiler, handbuch… grün, dann die Schlussrunde.
- **2e87ee57** (karte1 18995492, Selbsttest optsicher): ohne Konflikt, nur
  `gui/selbsttest_optsicher.inc`; Bau 0 Warnungen.
- **db3b6fc9** (karte1 6fe2c104, Asset-Browser und Texturen überblenden): Konflikte nur
  in `handbuch/*/suche.js` (neu erzeugt) und im Namen dieser Datei (s. oben); i18n neu erzeugt.

## Vorfall git stash

Beim Bauen der Vorher-Fassung habe ich `git stash push`/`pop` benutzt. Die Stash-Liste ist für
alle Worktrees gemeinsam: `pop` holte den gerade angelegten Stash des Agenten in `Y:\karte`
(„WIP on karte1: 83ca36cd …“, 23:13) statt meines. Sofort zurückgelegt
(`git stash store` mit demselben Commit 2b1ae2c0 und derselben Nachricht, wieder stash@{0}),
seine Dateien in meinem Baum verworfen, meinen Stash per Hash angewendet und gelöscht. Seine
Liste war rund 5 Minuten ohne den Eintrag. Künftig kein `git stash` in geteilten Worktrees.

## Offene Punkte

- misc_models werfen in der Vorschau keine Schatten und strahlen kein Flächenlicht ab
  (q3map2: Dreiecke mit `_castShadows`); Patches strahlen kein Flächenlicht ab (RadLightForPatch).
- Flächenlicht: Fächer statt der Dreiecke von -meta (StripFaceSurface); die Fläche des
  Editor-Pinsels statt der durch CSG beschnittenen .bsp-Fläche.
- Sonne mit Abweichung/Proben (`q3map_sunExt`): eine Sonne, weiche Kanten nur über PCF.
- S/O-Karte: liegt in derselben Sonnenspalte unter der Stelle noch ein Himmel, der von der Sonne
  wegzeigt, gilt die Stelle als dunkel (selten).
- Lichtstile, lightJunior, PVS-Beschnitt der Lichter (q3map2) – die Vorschau zeigt alle Lichter
  und verlässt sich auf die Schatten.
- alphashadow und alphaFunc nutzen das Editorbild; blendFunc-Stufen ohne Licht (wie die meisten
  im Spiel); gebacken nur der Alpha-Test.
- Taris: Bildzeit knapp unter 16 ms; die Lichtliste kostet auf dem Prozessor gut 2 ms bei 7493
  Lichtern.

## Was danach

- Nebel, Effekte und Skyportal in der Echtzeit-Ansicht.
- Shader live bearbeiten und neu laden.
- Patch-Brücke zwischen zwei Patch-Kanten.

## Changelog

## English

- Map editor, toolbars: drag the handle of the main, filter or plugin toolbar to dock it at another edge (top, left, right, bottom), like NetRadiant-Custom's toolbars; edge and order are kept after a restart. The handle's tooltip explains that Open, Save, Undo, Redo and Select Touching are hidden by default (as in NetRadiant-Custom) and shown via the toolbar's context menu.
- Real-time light like in the game (q3map2 -light -fast):
  - The sky brush no longer shadows its own sun: the ray to the sun ends at the sky, as in q3map2.
  - `sun`, every `q3map_sun`/`q3map_sunExt` and `q3map_skyLight` with the colours of the sky box; surface light per triangle with `q3map_lightSubdivide`, the exact form factor and the back splash.
  - Only what casts shadows in q3map2 blocks light (world and func_group, no nodraw, transparent only with alphashadow, patches with the new **Build > Patch shadows**).
  - Up to 512 lights per frame, also far away; 128 with soft shadows, the sun in two cascades; outside light no longer leaks inside.
  - nodraw surfaces such as caulk disappear and transparent shaders use their alphaFunc/blendFunc, like in the game.
  - Measured against the quick bake on four maps: mean difference 0.2-2 instead of 2-95 (of 255).

## Deutsch

- Karten-Editor, Werkzeugleisten: der Griff legt die Haupt-, Filter- oder Plugin-Leiste an einen anderen Rand (oben, links, rechts, unten), wie in NetRadiant-Custom; Rand und Reihenfolge bleiben nach einem Neustart. Der Tooltip des Griffs sagt, dass Öffnen, Speichern, Rückgängig, Wiederholen und Select Touching ab Werk versteckt sind (wie in NetRadiant-Custom) und über das Kontextmenü der Leiste erscheinen.
- Echtzeit-Licht wie im Spiel (q3map2 -light -fast):
  - Der Himmels-Brush wirft keinen Schatten mehr auf seine eigene Sonne: der Strahl zur Sonne endet am Himmel wie in q3map2.
  - `sun`, jede `q3map_sun`/`q3map_sunExt` und `q3map_skyLight` mit den Farben der Skybox; Flächenlicht je Dreieck mit `q3map_lightSubdivide`, genauem Formfaktor und Rückschein.
  - Licht hält nur auf, was in q3map2 Schatten wirft (Welt und func_group, kein nodraw, durchsichtig nur mit alphashadow, Patches mit dem neuen **Build > Patch shadows**).
  - Bis zu 512 Lichter je Bild, auch in der Ferne; 128 mit weichen Schatten, die Sonne in zwei Stufen; Licht von draußen leckt nicht mehr nach innen.
  - nodraw-Flächen wie caulk verschwinden, durchsichtige Shader nutzen alphaFunc/blendFunc - wie im Spiel.
  - Gegen das Schnell-Backen auf vier Karten gemessen: mittlere Abweichung 0,2-2 statt 2-95 (von 255).

## 中文

- 地图编辑器,工具栏:拖动主工具栏、过滤工具栏或插件工具栏的把手,可将其停靠到另一边(上、左、右、下),与 NetRadiant-Custom 相同;停靠位置和顺序在重启后保持不变。把手的提示说明:打开、保存、撤销、重做和 Select Touching 默认隐藏(与 NetRadiant-Custom 相同),可通过工具栏的右键菜单显示。
- 与游戏一致的实时光照(q3map2 -light -fast):
  - 天空画刷不再遮挡自己的太阳:与 q3map2 一样,射向太阳的光线在天空处结束。
  - 支持 `sun`、所有 `q3map_sun`/`q3map_sunExt` 以及带天空盒颜色的 `q3map_skyLight`;面光源按三角形计算,支持 `q3map_lightSubdivide`、精确形状因子和背光。
  - 只有在 q3map2 中投射阴影的东西才会挡光(世界和 func_group,nodraw 不挡,透明的只有带 alphashadow 时,面片需开启新的 **Build > Patch shadows**)。
  - 每帧最多 512 个光源,包括远处的;其中 128 个带柔和阴影,太阳有两级阴影;室外的光不再漏进室内。
  - caulk 等 nodraw 表面会消失,透明着色器使用其 alphaFunc/blendFunc——与游戏一致。
  - 在四张地图上与快速烘焙比较:平均偏差为 0.2-2,之前为 2-95(满值 255)。

## 日本語

- マップエディター、ツールバー:メイン・フィルター・プラグインのツールバーのハンドルをドラッグすると、NetRadiant-Custom と同じように別の辺(上・左・右・下)にドッキングできます。辺と順序は再起動後も保たれます。ハンドルのツールチップには、開く・保存・元に戻す・やり直し・Select Touching が(NetRadiant-Custom と同じく)初期状態で非表示であり、ツールバーの右クリックメニューで表示できることが書かれています。
- ゲームと同じリアルタイムライト(q3map2 -light -fast):
  - 空のブラシが自分の太陽の影を落とさなくなりました。q3map2 と同じく、太陽への光線は空で終わります。
  - `sun`、すべての `q3map_sun`/`q3map_sunExt`、スカイボックスの色を使う `q3map_skyLight`。面光源は三角形ごとに `q3map_lightSubdivide`、正確な形態係数、バックスプラッシュで計算します。
  - 光を遮るのは q3map2 で影を落とすものだけです(ワールドと func_group、nodraw は遮らず、透明なものは alphashadow があるときだけ、パッチは新しい **Build > Patch shadows** がオンのとき)。
  - 1 フレームに最大 512 個のライト(遠くのものも)。そのうち 128 個は柔らかい影、太陽は 2 段階の影。外の光が中に漏れなくなりました。
  - caulk などの nodraw 面は消え、透明なシェーダーは alphaFunc/blendFunc を使います。ゲームと同じです。
  - 4 つのマップでクイックベイクと比較:平均の差は 255 のうち 0.2-2(以前は 2-95)。
