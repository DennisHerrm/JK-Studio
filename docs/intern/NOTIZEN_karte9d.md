# Stufe 9d: Karten-Editor – Echtzeit-Ansicht komplett, Shader live, Patch-Brücke

Arbeitskopie `Y:\licht`, Zweig `karte9d`, ab `1274434d` (karte1 = rc606). Auftrag: sieben Punkte
nach Vorrang; Vorwissen `NOTIZEN_karte9c_licht.md` (Offene Punkte) und `NOTIZEN_karte9.md`
(Integration rc606, Undo-Fund). Regeln wie bisher: Fehler erst nachstellen (rot), dann beheben
(grün); „wie im Spiel“ heißt q3map2/OpenJK 1:1; eigene Funktionen mit „(nicht in
NetRadiant-Custom)“. Neue Texte in `tools/i18n_karte9d.py`.

## Commits

| Punkt | Commit | Inhalt |
|---|---|---|
| 1 | b6cf2099 | Behebungen und Importe warten auf die misc_model-Rückgewinnung (Hinweis, Selbsttest optsicher) |
| 2–5 | cca4fca1 | Licht wie q3map2 (Modelle, Patches, -meta, Proben), Nebel, Effekte, Skyportal, Shader live, Patch-Brücke, Selbsttest stufe9d, Unit-Tests |
| 3 | 128f46e6 | Unit-Tests Nebel gegen die OpenJK-Formel, Effektzeit in der Bildzeit-Zeile |
| Fund | f5bcb52c | Rückgewinnung: Netze, die die .bsp nicht hat, bekommen `_remap` auf nodraw (der graue Keil) |
| Fund | b2b8eab3 | Modell-Import: Shader-Skript unter `shaders/` statt `scripts/` |
| 6 | 085e00fb | Effekte: cullrange wie PlayEffect, Läufer außerhalb des Bilds übersprungen |
| 7 | 1f14bff9 | Menügrößen Build und Curve, Handbuch (4 Sprachen) und Bilder |
| 7 | (dieser) | Notizen |

## Punkt 1: Optimierer während der misc_model-Rückgewinnung

- `bspRueckLaeuft()` (Stand 1/2/3 der Rückgewinnung nach File > Open .bsp) und `bspRueckSperre()`:
  läuft sie, unterbleiben **Beheben/Anwenden**, **Fix all safe**, **File > Import** und
  **Geometrie als Brushes importieren** mit dem Hinweis `KeRkWarten` („Modelle werden noch
  zurückgewonnen … Behebungen und Importe gehen, sobald das fertig ist“) in der Konsole und oben in
  den Einzelheiten des Optimierers; dessen Knöpfe sind gesperrt. Grund (Fund rc606): eine
  Geometrieänderung in dieser Zeit ließ die Rückgewinnung verfallen oder sie nach einem Undo als
  neuen Schritt kommen, der den Redo-Stapel leerte. Einzelne Handgriffe (ein Pinsel verschieben)
  bleiben frei – der Stufe-8a-Test „Ergebnis verworfen“ gilt weiter.
- Test `optsicher` auf md_rev_TarisUpper2 (.bsp wie File > Open .bsp) mit `BHED_OPTSICHER_ALT=1`:
  zuerst der Ablauf aus 9c (prüfen, „Fix all safe“ und Undo, während die Rückgewinnung läuft).
  - **Rot** (Bau mit abgeschalteter Sperre): behoben 2, Karte anders, kein Hinweis, Redo nach dem
    Undo 1 → nach der Rückgewinnung **0** (der Verlust aus rc606), 14 OK / 1 FEHLER.
  - **Grün**: Rückgewinnung lief, behoben 0, Karte gleich, Hinweis da, danach 212 misc_model, nichts
    verworfen; der übrige Lauf wie rc606 (Rest-Dreiecke 48814 ×4, Modelle 212 ×4, 0 Stichproben
    anders) – **15 OK, 0 FEHLER**.
- Handbuchsatz in `inhalt_karte5.py` (Optimierer), 4 Sprachen.

## Punkt 2: Echtzeit-Licht, Rest wie q3map2

- **misc_model werfen Schatten** wie in -light (die Modelle stehen nach -bsp als Dreiecke in der
  .bsp): `echtModellFlaechen` setzt sie wie AddTriangleModels/ModelGetTransform (ohne target in der
  Welt, mit target in der Pinsel-Entität dieses Namens, ohne passendes Ziel gar nicht; Shader nach
  Skin und `_remap`), castShadows wie ParseEntityCompileParams (Welt 1, sonst 0, dann
  `_castShadows`/`_cs` des Modells bzw. der Eltern; in einer Pinsel-Entität nur, wenn diese selbst
  wirft – PopulateTraceNodes). In der Schattengeometrie mit denselben Regeln wie Pinsel (kein nodraw,
  durchscheinend nur mit alphashadow/lightfilter, Himmel als Himmel).
- **Flächenlicht von misc_model und Patches**: RadLightForTriangles über die Modell-Dreiecke (Normale
  je Ecke, RadClipWindingEpsilon mischt sie mit, die Lichtnormale ist ihr Mittel); RadLightForPatch
  über das Netz, das -light sieht: DrawSurfaceForMesh (MakeMeshNormals vor/nach PutMeshOnCurve),
  ExpandLongestCurve, IterationsForCurve(…, patchSubdivisions 8), SubdivideMesh2, PutMeshOnCurve,
  RemoveLinearMeshColumnsRows, MeshQuadIterator (ebene Vierecke ein Licht, sonst zwei Dreiecke);
  entartete Patches fallen weg wie in ParsePatch. autosprite-Flächen mit Licht werden ein Punktlicht
  (CreateSurfaceLights).
- **Pinselflächen in den Dreiecken von -meta**: StripFaceSurface (kleinste Ecke, beidseitig
  abwechselnd, TriEval mit triangle_min_angle_squared_sin/triangle_area2x, sonst FanFaceSurface um den
  Mittelpunkt) statt der Fächer.
- **`_samples`/`_deviance` und `q3map_sunExt` mit Abweichung und Proben**: die Jitter-Kopien
  (CreateEntityLights) und die Sonnen (CreateSunLight, Ablehnung außerhalb der Abweichung) mit dem
  Zufall von q3map2 (`srand( 0 )`, MSVC-rand: LCG, 38/7719/21238 …) in derselben Reihenfolge; vorher
  fehlten die Kopien (ein Licht mit photons/numSamples war zu dunkel). Entity-Sonnen (`_sun`) mit
  numSamples Sonnen wie CreateSunLight.
  - Befund beim Messen: die Proben einer Sonne bekamen je eine Himmelskarte über die ganze Karte
    (1024² auf ~30000 Einheiten) – Licht leckte nach innen (Taris P0 Mittel 1,1 → 19,3). Jetzt
    teilen alle Proben einer Sonne die Kaskaden der Hauptsonne (Schatten aus deren Richtung, Licht
    aus der eigenen) – P0 wieder 1,13.
- Unit-Tests `karte_echtlicht` (neu): Zufall, Proben/Jitter, Patch-Netz (eben 2×2, gewölbt
  unterteilt, entartet verworfen), Patch-Flächenlicht (Photonen = value·Fläche·areaScale, Seite),
  StripFaceSurface (Viereck, Fünfeck mit Ecke auf der Kante, Fächer), misc_model (castShadows,
  target ohne Ziel, Flächenlicht je Dreieck), sunExt-Parsing, Nebel.
- Selbsttest stufe9d/2: Modell des Spiels (das dickste der ersten 48 unter models/map_objects)
  zwischen Punktlicht und Boden; der Messpunkt liegt nachweislich im Schatten (die Strecken Licht →
  Boden treffen ein Modelldreieck, Möller–Trumbore auf den Dreiecken von echtModellFlaechen):
  Boden 45,0 ohne → 37,4 mit Modell, 220 Modelldreiecke in der Schattengeometrie.

### Messung (wie 9c: Echtzeit gegen Schnell-Backen, Blöcke 16 × 16, Mittel / Max von 255)

`BHED_KARTE_NUR=licht9c`, `SP/s9d/messen.sh`. Vorher = Bau b6cf2099 (rc606 + Punkt 1,
`SP/s9d/vorher_bin`), nachher = 085e00fb.

| Karte | Punkt | vorher Mittel / Max | nachher Mittel / Max |
|---|---|---|---|
| md_ch_ben | P0 / P1 / P2 | 0,86 / 30,3 · 0,32 / 7,0 · 0,29 / 6,4 | gleich |
| mp/duel1 | P0 / P1 / P2 | 0,71 / 22,5 · 0,43 / 31,3 · 0,72 / 15,7 | gleich |
| md_eitd_jedi | P0 | 0,17 / 28,8 | gleich |
| Taris converted (ohne Modelle) | P0 | 1,13 / 148,6 | 1,13 / 148,6 |
| | P1 | 0,00 / 0,00 | 0,00 / 0,00 |
| | P2 | 4,88 / 182,0 | 4,86 / 182,1 |
| Taris mit 212 zurückgewonnenen misc_model (`taris_mm2`, gleiche Karte beide Läufe) | P0 | 1,10 / 148,6 | 1,10 / 148,6 |
| | P1 | 0,36 / 104,9 | 0,49 / 105,4 |
| | P2 | 8,30 / 184,7 | 8,34 / 184,7 |

- Die drei Prüfkarten ohne Modelle/Patch-Flächenlicht ändern sich nicht (dieselben Lichterzahlen 192 /
  608 / 102). Auf Taris kommen 3400 Patch-Flächenlichter dazu (7540 → 10939); das Bild an den drei
  Punkten bleibt gleich. Mit Modellen (taris_mm2, 212 misc_model, 32607 Lichter) ebenso: die
  Modellschatten ändern die drei Punkte nicht messbar.
- **Ziel „Max deutlich kleiner als 188“ nicht erreicht.** Der Rest an P2 ist ein heller Streifen
  (Lichtleiste) auf dem Steg: in der Vorschau fast weiß, gebacken grau – nicht geklärt. Die
  Grenzwerte im Test (Mittel ≤ 15, Max ≤ 200) bleiben.
- Zwischenstand mit eigener Karte je Sonnenprobe: Taris P0 Mittel 19,3 (behoben, siehe oben).

## Punkt 3: Nebel, Effekte, Skyportal

- **Nebel** (`echtNebelSammeln`, `echtNebelDichte`, GPU `nebelFuer`/`nebelDichte` in `kShader4`):
  q3map2 CreateMapFogs/EmitFogs – jeder Pinsel mit `fogParms`-Shader ist ein Nebel; sichtbare Seite
  aus `q3map_fogDir` (Normale am meisten entgegen), sonst die erste sichtbare achsparallele Seite von
  +Z rückwärts, dann die übrigen (visibleHull: ein Teil der Seite liegt außerhalb deckender Pinsel –
  geprüft an Mitte und Ecken, je 1 Einheit davor); worldspawn `_fog`/`fog` = globaler Nebel.
  OpenJK R_LoadFogs (Farbe, tcScale = 1/(max(1,Tiefe)·8), Fläche = −Normale/Abstand der Seite;
  fehlender Shader: rot/250), RB_CalcFogTexCoords und R_FogFactor (fogTable = Wurzel), gemischt wie
  RB_FogPass (SRC_ALPHA/ONE_MINUS_SRC_ALPHA) auf allem Deckenden und Alpha-getesteten; ein Shader mit
  fogParms ohne Stufe zeichnet nur den Nebel (FP_LE). Zugehörigkeit je Bildpunkt: erster Nebel,
  dessen Pinsel die Stelle enthält (q3map2 schneidet die Flächen an den Pinselseiten), sonst der
  globale.
  - Abweichung: s und t werden je Bildpunkt gerechnet statt je Ecke interpoliert – gleich für s und
    für t mit dem Auge im Nebel; mit dem Auge außerhalb ist t/(t−eye_t) nicht linear (eine Ecke im
    Spiel liegt nach dem Schnitt am Pinselrand). Wie die Kartenvorschau: r_drawfog 1 (RB_FogPass
    auch für den globalen Nebel), tr.identityLight 1.
- **Effekte** (`gui/app_karte_echtfx.inc`): fx_runner nach g_fx.cpp wie loadMapEffects der Vorschau
  (fxFile → effects/…efx, Richtung target/angle/oben, delay/random, erster Durchgang nach 600 ms,
  START_OFF/ONESHOT warten auf use), gezeichnet mit `buildEffectMesh`/`effectFollowUpsAt` (efxed,
  gegen FxScheduler/FxPrimitives); Effekt-Shader wie RE_RegisterShader (mit Skript die erste Stufe,
  ohne Skript der implizite 2D-Shader). **Build > Echtzeit-Effekte (fx_runner)** (`BhedRealtimeEffects`).
  Grenze: keine .bsp im Editor – usePhysics prallt nicht ab.
- **Skyportal**: CG_DrawSkyBoxPortal – erst die Szene vom misc_skyportal aus (gleicher Blick und
  Bildwinkel, mit Himmelsbox, Licht und Schatten für diesen Blick vorbereitet) in Zielbild 201,
  dann zeigt jede Himmelsfläche (C_SKY) der Hauptszene dieses Bild an derselben Bildstelle.
  Nur mit Echtzeit-Licht.
- Tests (Selbsttest `BHED_KARTE_NUR=stufe9d`, Fotos `karteneditor_s9d_*`): siehe Punkt 7.

## Punkt 4: Shader live bearbeiten

- Texturbrowser, Rechtsklick: **Shader live bearbeiten… (nicht in NetRadiant-Custom)**; im Fenster
  „Shader der Auswahl“ (der aktuelle Shader). Die ganze .shader-Datei an der Zeile des Shaders,
  Syntaxfarben (Shadernamen, Schlüsselwörter, Zahlen, Pfade, Klammern, Kommentare) als farbiger Text
  über dem durchsichtigen Eingabefeld, links Zeilennummern und der Rand des Skript-Editors
  (`randZeichnenSkript`: geändert / gespeichert / zurück).
- **Speichern nie in einen Spielordner**: liegt die Datei im Spiel (Archiv oder Spielordner), geht sie
  unter demselben Pfad in den Import-Ordner (steht im Suchpfad vor dem Spiel und ersetzt die ganze
  Datei); im Import-Ordner oder einem Zusatz-Suchordner des Anwenders bleibt sie dort. Danach
  „Flush & Reload Shaders“ (neue Datei im Import-Ordner: Suchpfad neu lesen); das Echtzeit-Licht
  leert dabei seine Shader-Zwischenspeicher (Skripte, Lichtfarben, Himmelsproben, Schatten- und
  Nebelgeometrie). Ein Wächter (0,5 s) sieht `shaders/` und `scripts/` des Import-Ordners und die
  offene Datei: jede Änderung (auch aus einem anderen Editor) lädt neu.
- Fund nebenbei: JA liest Shader aus `shaders/` (shaderpath); der MD3-Import der Stufe 9 legt seine
  `.shader` unter `scripts/` ab – dort findet sie weder der Editor noch das Spiel (an den Hauptstrang).

## Punkt 5: Patch-Brücke

- **Curve > Patch-Brücke zwischen Kanten (nicht in NetRadiant-Custom)** – NRC und bobToolz haben
  nur „Merge Patches“ für Patches mit gemeinsamer Kante. Zwei gewählte Patches; je eine Randkante
  (erste/letzte Zeile bzw. Spalte der Kontrollpunkte): mit gewählten Kontrollpunkten die Kante, die
  sie enthält, sonst das nächste Paar gleich langer Kanten (auch umgekehrt laufend). Neuer Patch
  n × 3: erste und letzte Zeile **genau** diese Kontrollpunkte, dazwischen die Mitte; Vorderseite wie
  der erste Patch, dessen Shader, NaturalTexture; ein Undo-Schritt „patchBridge“.

## Punkt 6: Bildzeit

Kamera dreht 240 Bilder, Echtzeit-Licht an (`BHED_LICHT_BILDZEIT`):

| Karte | 9c | 9d erste Fassung | 9d (085e00fb) |
|---|---|---|---|
| Taris converted (10892 Lichter, 512 je Bild) | 15,4 ms (p99 21) | 13,6 ms (p99 20,9), Lichtliste 0,9 ms | – |
| duel_mos_espa (172 Lichter, 168 fx_runner) | 5,3–9,0 ms | **27,9 ms** (p99 50,0) | **10,1 ms** (p99 17,2), Effekte 3,5 ms |

- Die erste 9d-Fassung zahlte die 168 Effekte jedes Bild voll (Effekte 11,8 ms je Bild). Jetzt
  cullrange wie PlayEffect (FxScheduler.cpp:1174: ein Primitiv entsteht nicht, wenn der Effekt weiter
  als cullrange vom Auge startet) und Läufer außerhalb des Sichtkegels werden nicht gerechnet, wenn
  eine sichere Hülle ihres Effekts (Ursprung, Geschwindigkeit, Beschleunigung und Schwerkraft über die
  längste Lebensdauer, Größe) ganz neben dem Kegel liegt; Effekte mit Folgeeffekten, Emittern oder
  Blitzen immer. Das Bild bleibt gleich (Selbsttest stufe9d 23 OK).
- Gemessen, während die anderen beiden Agenten bauten (bis zu 11 MSBuild/cl): die Werte sind „unter
  Last“; die Lichtliste schwankte dabei zwischen 0,08 und 8,7 ms. Eine Messung auf ruhiger Maschine
  steht aus.

## Fund: der graue Keil auf md_rev_TarisUpper2 (Foto shank)

Gemeldet: im Testfenster mit `SP/s9d/taris_mm/md_rev_TarisUpper2_mm.map` (Messpunkt P2, -498 7628
1943, Blick 240) ein riesiger, flach schattierter grauer Keil aus großen Dreiecken quer über der
Szene. Untersucht:

- **Nicht der Optimierer**: taris_mm ist der Kartentext *vor* „Fix all safe“ (optsicher merkt ihn nach
  der Rückgewinnung); optsicher zeigt 0 Stichproben anders nach dem Fix.
- **Nicht 9d-Licht/Nebel/Portal/Brücke**: dasselbe Bild mit dem Binary rc606 + Punkt 1
  (`SP/s9d/vorher_bin`, Lauf mmvorher, `SP/s9d/bilder_mm_vorher/…_p2_echt.bmp`).
- **Zwei Teile:**
  1. **Fehler der Rückgewinnung (Stufe 8a), behoben (f5bcb52c):** `control_station.md3` mit
     `modelscale_vec 26,4 8,75 32,8` und `_remap panel4 → amace_taris/light3` (vier Stück bei
     y 5144, z 411 und weitere) – in der .bsp steht davon nur die Lichtleiste (1 von 5 Netzen, Rest
     0,0000). Die vier anderen Netze galten als nodraw („Shader nicht in der .bsp“), kamen aber
     sichtbar zurück: gestreckte dunkle Kästen mitten im Bild. Jetzt bekommt jedes nicht gefundene
     Netz einen `_remap` auf `textures/system/nodraw` (q3map2 setzt keine nodraw-Flächen, surface.cpp).
     **Rot/grün** mit `kartemodellrueck <original.bsp> <dump>` auf der .bsp und dem Modell-Dump des Laufs
     (`SP/s9d/keil/rot.txt`, `gruen.txt`): vorher 30 Treffer mit fehlenden Netzen ohne nodraw, nachher
     26 davon versteckt; 4 bleiben (grass_ot02, mp_scepter: das fehlende Netz teilt seinen Shader mit dem
     gefundenen – ein `_remap` gilt je Shader). Im Bild (`SP/s9d/keil_vorher_nachher.png`) sind die
     Kästen weg; optsicher mit der neuen Rückgewinnung 15 OK / 0 FEHLER.
  2. **Der große Keil selbst gehört zur Karte:** an der Stelle (-200 7250 2100) stehen in der Original-.bsp
     Dreiecksflächen der Welt (Modell 0, Typ triangle soup, Shader amace_taris/base3, base4,
     buildingroof4 – aus den BSP-Lumps gelesen). Die Rückgewinnung erkennt sie keinem Modell zu und
     macht daraus dünne Detail-Pinsel in der Ebene „Unerkannte Flächen“ (Option, vorgegeben an). Im Spiel
     stehen dieselben Dreiecke; „flach schattiert“ wirkt er, weil ein Pinsel keine glatten Eckennormalen
     hat (das Spiel beleuchtet die Dreiecke mit den Normalen des Modells) – eine Grenze der Stufe 8a,
     offen.

## Fund: Modell-Import legte Shader unter scripts/ ab

- Der MD3-Import der Stufe 9 (Modell ziehen, Geometrie als Modell + Clip) schrieb
  `scripts/<name>.shader`. JA, JK2 und MD lesen `shaders/` (tr_shader.cpp ScanAndLoadShaderFiles(
  "shaders"), shaderpath in ja.game/jk2.game) – weder der Editor noch das Spiel fanden den Shader.
- Jetzt `shaders/<name>.shader` (`kSpielShaderOrdner`). Frühere Importe bleiben, wo sie sind: das
  Inventar liest `<Import>/scripts/*.shader` mit (nur aus dem Import-Ordner, nur ohne gleichnamige
  Datei unter shaders/), die Konsole sagt es (`KeImpSkripte`, 4 Sprachen), und „Im Spiel testen“ und
  „Karte als .pk3 packen“ legen sie im Archiv unter `shaders/` ab (`importArchivName`; liegt daneben
  schon `shaders/x.shader`, gilt die).
- **Rot**: ctest kartemd3import mit der Erwartung shaders/ und dem Archivnamen – 2 Fehler; **grün**
  danach. Selbsttest stufe9 (Modell ziehen) prüft jetzt zusätzlich, dass der Editor den Shader nach dem
  Neuladen findet, und das .pk3 `shaders/geo_wuerfel.shader`.

## Punkt 7: Handbuch, Tests

- **Selbsttest `BHED_KARTE_NUR=stufe9d`** (auch im vollen Lauf karteneditor, nach 9c), 23 OK / 0 FEHLER:
  - 9d/2 Modellschatten (siehe Punkt 2).
  - 9d/3 Nebel: Testshader `textures/bhed9d/nebel` (fogParms 0.1 0.9 0.1, 256) im Import-Ordner
    (`shaders/bhed9d_test.shader`), Steinraum mit Nebelpinsel über dem Boden, Blick von oben: Boden
    ohne 42,9 38,5 33,5 → mit 30,6 173,6 27,8, erwartet nach der Formel 30,59 173,56 27,84 (Dichte
    0,70711, Abweichung 0,03); globaler Nebel (worldspawn `_fog`): Abweichung 0,5; Foto
    `karteneditor_s9d_nebel`.
  - 9d/4 Shader live: Editor öffnen, fogParms grün → rot, Speichern → Ziel im Import-Ordner, Boden
    30,6 173,6 → 174,8 29,3 (ohne Neustart); die Datei von außen auf blau: der Wächter lädt neu (Boden
    blau); ein Spiel-Shader (00mj_larshomestead/bswitch) hätte als Ziel den Import-Ordner; Foto
    `karteneditor_s9d_shaderlive`.
  - 9d/3 Effekte: fx_runner mit `env/fire` vor der Kamera: 1 Läufer, Ecken > 0, Bild anders; Build-Menü
    aus → keine Ecken; Foto `karteneditor_s9d_effekt`.
  - 9d/3 Skyportal: Himmelsraum, misc_skyportal in einem beleuchteten Steinraum weit weg: der Himmel
    zeigt den Portalraum (0 0 0 → 79,6 68,8 56,2; ohne Bilder der Himmelsbox ist der Himmel im Test
    sonst schwarz); Foto `karteneditor_s9d_skyportal`.
  - 9d/5 Patch-Brücke: zwei 3×3-Patches mit 128 Lücke → neuer Patch, Zeilen genau die Kanten x 128 und
    x 256, Undo-Schritte +1, Undo/Redo; ein Patch allein: abgelehnt, nichts geändert.
- **optsicher** (Punkt 1 und die neue Rückgewinnung): 15 OK / 0 FEHLER.
- **ctest**: karte_echtlicht (neu: Zufall, Proben, Patches, Streifen, Modelle, Nebel), karte_md3import
  (shaders/, Archivname), karte_modellrueck, karte_optimierer, handbuch_sprachen, hilfe_orte.
- **Handbuch** (4 Sprachen, `BHED_HB_NUR=karte9d` und `karte7`): `inhalt_karte7.py` (Modelle und
  Patches, neuer Abschnitt „Nebel, Effekte und Skyportal“ mit Bild `h_karte9d_nebel`),
  `inhalt_karteneditor.py` (Shader live, Bild `h_karte9d_shader`), `inhalt_karteneditor2.py`
  (Patch-Brücke), `inhalt_karte5.py` (Optimierer wartet), `inhalt_karte4.py` (nodraw-`_remap` der
  Rückgewinnung); Build-Menü-Bild neu (Echtzeit-Effekte). Menügrößen: Build + 24, Curve 390 × 453.

## Schlussrunde

Auf dem Stand 1f14bff9, alles in einem Lauf (`SP/s9d/ende_s9d.sh`, Idle-Priorität, Werte unter
Last – die anderen Agenten bauten und testeten gleichzeitig):

| Schritt | Ergebnis |
|---|---|
| Bau Release | ok, 0 Warnungen (/W4 /WX) |
| ctest | 158 / 158 (314 s) |
| Selbsttest karteneditor (voll, mit stufe9d) | 3205 OK / 0 FEHLER (1395 s), Spitze 1123 MB; Stufe 7 große Karte 18,21 ms (p99 27,74) unter Last |
| Selbsttest Modus 1 | 2644 OK / 1 FEHLER im Gesamtlauf: „Copy schreibt //(BHVD) + Zeile in die Windows-Zwischenablage“ – ein anderer Agent testete gleichzeitig, die Zwischenablage ist geteilt; allein wiederholt: 2645 OK / 0 FEHLER (Lauf e9d1b) |
| Selbsttest optsicher | 15 OK / 0 FEHLER |
| Selbsttest stufe9d | 23 OK / 0 FEHLER |
| Kartenprüfung (278 Karten, gegen rc606) | exit 0 (820 s), alle Summen gleich: geprüft 278, abgestürzt 0, nicht geladen 0, D3D 0, nicht gezeichnete Entities 1, NPC ohne Modell 2, fehlende .efx 27 |
| Neubau (`--clean-first`) | 0 Fehler, 0 Warnungen (811 s) |
| Lints | lint_sources, lint_fassung, lint_cbuffer, lint_headers, lint_feldinit, lint_gapp, lint_imgui_context ok; lint_i18n 704 (1274434d: 705), lint_portability 191 (192), lint_settings 1, lint_fremdprogramm 1, lint_d3dbesitz 11 (gpumap_win32.cpp, unberührt) wie vorher; lint_i18n_erzeugt schlug schon auf 1274434d fehl (i18n_karte2 fehlt in der Prüfkopie) |

## Was gegen das Vorbild geprüft wurde

Geprüft gegen das Vorbild:
- Punkt 1: der Ablauf aus 9c auf md_rev_TarisUpper2 (optsicher, rot → grün), Undo/Redo und Kartentext.
- Nebel: Bildpunkte gegen die portierte OpenJK-Formel (RB_CalcFogTexCoords + R_FogFactor, Mischung
  wie RB_FogPass) – Abweichung 0,03 (Nebelpinsel) bzw. 0,5 (globaler Nebel) von 255; Unit-Test mit
  einem von Hand gerechneten Wert (0,70711), sichtbare Seite nach EmitFogs, q3map_fogDir, „bad shader“.
- Licht: Echtzeit gegen das Schnell-Backen (bhed_q3map2 -light -fast) an festen Punkten auf vier
  Karten und auf Taris mit 212 Modellen, vorher/nachher.
- Zufall von q3map2: MSVC rand() nach srand( 0 ) (38, 7719, 21238) und die Jitter-Kopie daraus.
- Patch-Netz, Flächenlicht, StripFaceSurface: Unit-Tests mit Sollwerten aus den q3map2-Formeln.
- Rückgewinnung: kartemodellrueck auf der Original-.bsp (Netze, `_remap`); der Keil an den BSP-Lumps.
- Modell-Import: der Ordner gegen ScanAndLoadShaderFiles( "shaders" ); cullrange gegen
  FxScheduler.cpp:1174.

Nicht gegen das Vorbild geprüft:
- Effekte: der Test prüft, dass ein fx_runner spielt (Läufer, Ecken, Bild anders) und abschaltbar ist –
  das Effektsystem selbst ist in der Zwischensequenz-Vorschau gegen FxScheduler/FxPrimitives geprüft,
  ein Bildvergleich mit dem Spiel fehlt; ohne .bsp kein Abprallen.
- Skyportal: geprüft, dass der Himmel den Portalraum zeigt (anderes, helles Bild); kein Pixelvergleich
  mit dem Spiel.
- Modellschatten: der Messpunkt liegt geometrisch im Schatten und wird dunkler; die Helligkeit dort ist
  nicht gegen q3map2 gemessen, und auf Taris ändern die Modellschatten die drei Messpunkte nicht messbar.
- Shader live und Patch-Brücke: eigene Funktionen ohne Vorbild – geprüft auf ihr Ergebnis (Datei im
  Import-Ordner, Bild ändert sich ohne Neustart; Kontrollpunkte genau, ein Rückgängig-Schritt).
- Nebel je Bildpunkt statt je Ecke; Bildzeit nur unter Last.

## Offene Punkte

- Taris: Max an P2 bleibt 182–185 (heller Streifen auf dem Steg), Ziel „deutlich kleiner“ nicht erreicht.
- Schattengruppen (`_castShadows` > 1 mit `_receiveShadows` ≠ 1) und `_receiveShadows 0` – die Vorschau
  hat eine Tiefe je Schattenkarte.
- Rückgewinnung: 4 Treffer, deren fehlendes Netz den Shader mit dem gefundenen teilt; die „Unerkannten
  Flächen“ wirken flach schattiert (Pinsel ohne Eckennormalen).
- Effekte: Folgeeffekte ohne cullrange; kein Abprallen (keine .bsp); START_OFF/ONESHOT laufen nicht
  (es gibt kein use im Editor).
- Nebel und Skyportal nur mit Echtzeit-Licht, nicht beim gezeigten Schnell-Backen.
- Bildzeit auf ruhiger Maschine messen.
- Shader-Editor: kein Suchen/Ersetzen im Fenster (NRCs Shader-Fenster hat es).
- Die beiden Pk3-Packer in app_karte_bau.cpp (Im Spiel testen, Karte als .pk3) tragen den Import-Ordner
  doppelt ein (vorhanden seit Stufe 9) – zusammenlegen.

## Changelog

## English

- Map editor, optimizer: while **File > Open .bsp** still recovers the misc_model entities, fixes (Apply, Fix all safe) and imports wait with a hint - before, a change in that time could drop the recovery or bring it back after an Undo as a new step that cleared Redo.
- Real-time light like q3map2, the rest:
  - misc_model entities cast shadows (in the world always, otherwise with `_castShadows`) and their surface light shines; patches shine over the same tessellation as -light.
  - Brush faces shine over the triangles of `-meta`; `q3map_sunExt` with deviance and samples and lights with `_samples` get their jittered copies (q3map2's own random sequence).
- Real-time view like the game: fog brushes (`fogParms`) and the global fog (`_fog`) as the game draws them, fx_runner effects (**Build > Real-time effects**), and the sky of a map with misc_skyportal shows the portal view.
- **Edit shader live...** in the texture browser menu: the shader file in an editor with syntax colours; saving never writes into the game folder (it goes to the import folder), and shaders, texture browser, camera and real-time light reload at once - also when a .shader file in the import folder changes outside.
- **Curve > Bridge patch edges**: a new patch joins one edge each of two patches, with exactly their control points, in one undo step.
- Fixed: a recovered misc_model could show meshes the .bsp did not have (a huge grey wedge on md_rev_TarisUpper2) - such meshes now get a `_remap` to nodraw.
- Fixed: dropped models (.obj/.fbx/.glb) wrote their shader script to `scripts/`, which Jedi Academy, Jedi Outcast and Movie Duels do not read - now `shaders/`; older imports are still read and packed under `shaders/`.

## Deutsch

- Karten-Editor, Optimierer: solange **File > Open .bsp** noch die misc_model zurückgewinnt, warten Behebungen (Anwenden, Fix all safe) und Importe mit einem Hinweis - vorher konnte eine Änderung in dieser Zeit die Rückgewinnung verfallen lassen oder sie nach einem Rückgängig als neuen Schritt bringen, der das Wiederholen leerte.
- Echtzeit-Licht wie q3map2, der Rest:
  - misc_model werfen Schatten (in der Welt immer, sonst mit `_castShadows`), und ihr Flächenlicht leuchtet; Patches leuchten über dieselbe Unterteilung wie -light.
  - Brush-Flächen leuchten über die Dreiecke von `-meta`; `q3map_sunExt` mit Abweichung und Proben und Lichter mit `_samples` bekommen ihre verschobenen Kopien (mit der Zufallsfolge von q3map2).
- Echtzeit-Ansicht wie im Spiel: Nebel-Brushes (`fogParms`) und der globale Nebel (`_fog`) so, wie das Spiel sie zeichnet, fx_runner-Effekte (**Build > Echtzeit-Effekte**), und der Himmel einer Karte mit misc_skyportal zeigt den Portalblick.
- **Shader live bearbeiten...** im Menü des Texturbrowsers: die Shader-Datei in einem Editor mit Syntaxfarben; Speichern schreibt nie in den Spielordner (sondern in den Import-Ordner), und Shader, Texturbrowser, Kamera und Echtzeit-Licht laden sofort neu - auch wenn sich eine .shader-Datei im Import-Ordner von außen ändert.
- **Curve > Patch-Brücke zwischen Kanten**: ein neuer Patch verbindet je eine Kante zweier Patches, genau mit ihren Kontrollpunkten, in einem Rückgängig-Schritt.
- Behoben: ein zurückgewonnenes misc_model konnte Netze zeigen, die die .bsp nicht hatte (ein riesiger grauer Keil auf md_rev_TarisUpper2) - solche Netze bekommen jetzt einen `_remap` auf nodraw.
- Behoben: gezogene Modelle (.obj/.fbx/.glb) legten ihr Shader-Skript unter `scripts/` ab, das Jedi Academy, Jedi Outcast und Movie Duels nicht lesen - jetzt `shaders/`; ältere Importe werden weiter gelesen und unter `shaders/` gepackt.

## 中文

- 地图编辑器,优化器:当 **File > Open .bsp** 仍在恢复 misc_model 时,修复(应用、Fix all safe)和导入会带提示地等待——此前在这段时间内的修改可能使恢复结果作废,或在撤销后作为新的一步出现并清空重做。
- 与 q3map2 一致的实时光照(其余部分):
  - misc_model 实体投射阴影(在世界中总是投射,否则需 `_castShadows`),其面光源也会发光;面片按与 -light 相同的细分发光。
  - 画刷面按 `-meta` 的三角形发光;带偏差和采样数的 `q3map_sunExt` 以及带 `_samples` 的灯光会得到抖动副本(使用 q3map2 自己的随机序列)。
- 与游戏一致的实时视图:雾画刷(`fogParms`)和全局雾(`_fog`)按游戏方式绘制,fx_runner 特效(**Build > 实时特效**),带 misc_skyportal 的地图天空显示传送门视野。
- 纹理浏览器菜单中的**实时编辑着色器...**:在带语法着色的编辑器中打开着色器文件;保存从不写入游戏文件夹(而是写入导入文件夹),着色器、纹理浏览器、摄像机和实时光照会立即重新加载——导入文件夹中的 .shader 文件在外部被修改时也一样。
- **Curve > 在面片边之间搭桥**:用一个新面片连接两个面片的各一条边,控制点完全相同,一步撤销。
- 修复:恢复出的 misc_model 可能显示 .bsp 中没有的网格(md_rev_TarisUpper2 上的巨大灰色楔形)——这类网格现在会得到指向 nodraw 的 `_remap`。
- 修复:拖入的模型(.obj/.fbx/.glb)把着色器脚本写到 `scripts/`,而 Jedi Academy、Jedi Outcast 和 Movie Duels 不读取该文件夹——现在写到 `shaders/`;旧的导入仍会被读取,并以 `shaders/` 打包。

## 日本語

- マップエディター、最適化:**File > Open .bsp** がまだ misc_model を復元している間は、修正(適用、Fix all safe)とインポートはヒントを出して待機します。以前はその間の変更で復元結果が失われたり、元に戻した後に新しいステップとして追加されてやり直しが消えたりすることがありました。
- q3map2 と同じリアルタイムライト(残りの部分):
  - misc_model が影を落とし(ワールドでは常に、ほかは `_castShadows` があるとき)、その面光源も光ります。パッチは -light と同じ分割で光ります。
  - ブラシの面は `-meta` の三角形で光り、偏差とサンプル数のある `q3map_sunExt` と `_samples` のあるライトはずらしたコピーを持ちます(q3map2 自身の乱数列)。
- ゲームと同じリアルタイムビュー:フォグブラシ(`fogParms`)とグローバルフォグ(`_fog`)をゲームと同じように描画し、fx_runner のエフェクト(**Build > リアルタイムエフェクト**)、misc_skyportal のあるマップの空にはポータルからの眺めが映ります。
- テクスチャブラウザーのメニューの**シェーダーをライブ編集...**:シェーダーファイルを構文の色分け付きエディターで開きます。保存はゲームフォルダーには書き込まず(インポートフォルダーへ)、シェーダー・テクスチャブラウザー・カメラ・リアルタイムライトがすぐに再読み込みされます。インポートフォルダーの .shader ファイルが外部で変更されたときも同様です。
- **Curve > パッチの辺をブリッジ**:2 つのパッチの辺を 1 本ずつ、制御点そのままの新しいパッチでつなぎます。元に戻すは 1 回です。
- 修正:復元した misc_model が .bsp にないメッシュを表示することがありました(md_rev_TarisUpper2 の巨大な灰色のくさび)。そのようなメッシュには nodraw への `_remap` が付きます。
- 修正:ドラッグしたモデル(.obj/.fbx/.glb)がシェーダースクリプトを Jedi Academy・Jedi Outcast・Movie Duels が読まない `scripts/` に書いていました。今は `shaders/` です。古いインポートは引き続き読み込まれ、`shaders/` として詰められます。
