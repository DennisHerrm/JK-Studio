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
| 7 | eea07670 | Notizen |
| VIS | 0344b276 | VIS-Fund md_rev_TarisUpper2: Speicher wie CreatePassages, Hinweis vor vollem VIS, Deko-Prüfung nur nicht abdichtende Pinsel |

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

## Nachauftrag: VIS auf md_rev_TarisUpper2 („[Final] Extreme Quality“, 12:23 h bei 45 %)

Frage (Kumpel von shank): die per File > Open .bsp zurückgewonnene Karte mit „[Final] Extreme Quality“ –
VIS seit gestern, PassagePortalFlow 45 %, laut Fenster noch mindestens 15 h. Normal oder ein Fehler bei uns?

**Antwort: normal, kein Fehler der Rückgewinnung und kein langsameres VIS.** Die Original-.bsp hatte nie
ein volles VIS: ihr worldspawn trägt `"_q3map2_cmdline" "-bsp -meta; -vis -fast; -light -fast -patchshadows
-samples 2 -filter -bounce 2"` (NRC q3map2 2.5.17n-git-68ecbed). „[Final] Extreme Quality“ rechnet das volle
VIS (`-vis`) – auf einer sehr offenen Karte mit 38042 Portalen; dafür braucht es etwa 11 GB Arbeitsspeicher.

### 1. Struktur: Original gegen zurückgewonnen

`SP/vis/bspstruktur.py` (Lumps der .bsp, Cluster aus dem Visibility-Lump), `SP/vis/mapzaehlen.py` (.map).
Zurückgewonnen = `taris_mm` (Open .bsp mit misc_model-Rückgewinnung, vor „Fix all safe“), gebaut mit
bhed_q3map2 `-meta`.

| | Original .bsp | zurückgewonnen, `-meta` |
|---|---|---|
| Cluster | 9535 (Visibility-Lump) | 9598 (.prt), +0,7 % |
| Leafs / Knoten | 18458 / 18379 | 18562 / 18483 |
| strukturelle Weltpinsel | 6787 (nur in festen Blättern) | 6718 (ohne detail-Flag; q3map2: „6718 structural brushes“) |
| Weltpinsel gesamt | 17682 | 97710 (92717 detail: Unerkannte Flächen, misc_model-Ersatz) |
| Portale | stehen nicht in der .bsp | 38042 (76084 Portalseiten) |
| sichtbare Cluster je Cluster | 6414,5 (67,3 %) | mit `-vis -fast`: 6518 (67,9 %) |

- Die Struktur ist dieselbe: die Rückgewinnung (Stufe 8a) und die Dekompilierung (convert_map.cpp mit der
  Stufe-7-Regel für structural) lassen die strukturellen Pinsel strukturell und machen alles andere detail;
  die vielen zusätzlichen Pinsel sind detail und erzeugen keine Portale.
- Per Cluster verglichen (`SP/vis/visvergleich.py`, 4200 über gleiche Blatt-Grenzen zugeordnete Cluster,
  Stichprobe 400): Original 2758,9, unser `-vis -fast` 2778,4 sichtbare Cluster, 98 % gemeinsam – die
  Sichtbarkeit des Originals ist die eines `-vis -fast`.
- Kein Fehler, also kein Rot/Grün an der Rückgewinnung.

### 2. Tempo: bhed_q3map2 gegen NRCs q3map2.exe

Gleiche .prt, `-threads 32`, Idle, nacheinander (`SP/vis/q3.py`, `q3lauf.ps1`; die anderen Agenten bauten
gleichzeitig – Werte unter Last).

| Karte | Portale | bhed Wand / CPU | NRC Wand / CPU |
|---|---|---|---|
| ctf3 | 2755 | 1 s / 20 s | 2 s / 24 s |
| mb2_manaan | 5051 | 17 s / 390 s | 22 s / 496 s |
| t3_byss | 7284 | 36 s / 545 s | 44 s / 700 s |
| duel_mos_espa | 6756 | 202 s / 2350 s | 206 s / 2950 s |
| md_rev_TarisUpper2 (bis 8 GB) | 38042 | BasePortalVis 13 s, CreatePassages bis „6.“ in 30 s | 13 s, „6.“ in 32 s |

- **Unser VIS ist nicht langsamer**: rund 20 % weniger CPU-Zeit (MSVC-Bau gegen NRCs MinGW-Bau), die Wandzeit
  gleich oder besser. Auf Taris endet jeder Lauf mit dem 8-GB-Wächter in CreatePassages bei 65 % – beide
  gleich schnell bis dahin; PassagePortalFlow auf Taris lässt sich hier nicht messen.
- Threads: im Mittel 15–23 der 32 Kerne belegt (die anderen Agenten); am Ende jeder großen Karte läuft der
  „Schwanz“: der Pacifier steht auf 9 (alle Portale verteilt), die teuersten letzten Portale rechnen auf
  wenigen Threads weiter (duel_mos_espa: Pacifier fertig nach 115 s, danach 90 s mit etwa 2 Kernen).
- Mit 32 Threads ist die Sichtbarkeit von Lauf zu Lauf und zwischen beiden Programmen leicht verschieden
  (bekannt seit Stufe 6: fertige Portale kürzen spätere ab, je nach Thread-Zeitpunkt); mit `-threads 1`
  sind beide Byte für Byte gleich – nachgeprüft auf ctf3: 145528 Bytes Sichtbarkeit gleich, bhed 16 s, NRC 22 s
  (NRC druckt dabei 10 MB Passagen-Speicher, bhed richtig 21 MB, zusammen 32 MB).

### 3. Speicher: NRC druckt einen falschen Bedarf

NRCs `PassageMemory` (visflow.cpp) zählt nur `numportals` der `numportals * 2` Portalseiten und summiert in
einem `int`: für Taris „12 average number of passages per leaf, 268 MB required passage memory“. Gemessen:
der Prozess war bei 65 % von CreatePassages über 8 GB. Richtig (aus der .prt): 995496 Passagen à
(16 + 9512) Bytes = 9045 MB, dazu portalfront/-flood/-vis je Portalseite 2071 MB – **11116 MB**.
bhed_q3map2 druckt jetzt „13 … / 9045 MB required passage memory“ und „BehavEd: vis memory 11116 MB“
(NRC-Fehler nicht übernommen). Hat der PC des Kumpels weniger freien Speicher, lagert Windows während
PassagePortalFlow aus – das macht das VIS zusätzlich extrem langsam.

### 4. Dauer: aus den Portalen nicht berechenbar

Die Messreihe (`SP/vis/kal.py`, `kal2.py`, 15 Karten aus dem Spiel, die größte taspir1 mit 12342 Portalen in
26 s) zeigt: die Dauer hängt an der Offenheit, nicht an der Portalzahl. duel_mos_espa (6756 Portale) braucht
2350 CPU-s, taspir1 (12342) 433 CPU-s, kejim_post (47152, Stufe 7) etwa 10 min, Taris (38042) Tage. Auch die
Momente der might-see-Zahlen nach BasePortalVis (Σm², Σm³) liegen um den Faktor 3–5 daneben (manaan Σm³ 1,1e14
→ 402 s, t3_byss 8,4e13 → 517 s, mos_espa 1,9e14 → 2165 s; Taris 5,1e17). Deshalb keine Zahl, sondern:
- **Hinweis vor dem vollen VIS** (eigene Funktion): vor jedem `-vis` ohne `-fast` liest der Editor die .prt
  des BSP-Schritts (`visBedarfAusPrt`, wie LoadPortals/PassageMemory) und schreibt „Volles VIS: N Portale,
  C Cluster – braucht etwa X GB Arbeitsspeicher (dieser PC: Y GB)“. Ab der Portal-Schwelle des Optimierers
  (8000) oder ab 75 % des eingebauten Speichers eine Warnung, auch im Fortschrittsfenster: Stunden, auf
  offenen Karten Tage, der Fortschritt zählt Portale und die teuren kommen zuletzt; bei einer Karte aus
  Open .bsp mit `-vis -fast` in `_q3map2_cmdline` der Satz dazu; Auswege `-vis -fast`, detail,
  „Teil-Kompilierung: VIS wiederverwenden“.
- Der lineare „noch mindestens“-Wert im Fenster ist für PassagePortalFlow nur eine Untergrenze (SortPortals:
  die Portale kommen nach steigender might-see).

### 5. Optimierer: „Deko als Struktur“ nur noch Pinsel, die nicht abdichten

Die Prüfung `deko` (klein ≤ 64, dünn ≤ 8 bei ≤ 256, verwinkelt) schlug bisher jeden solchen Pinsel vor – auch
einen Stopfen in der Außenwand. Jetzt mit der .bsp des letzten Baus (ohne Leck): Punkte 2 Einheiten vor jeder
Seite (Mitte und Ecken ein Viertel zur Mitte), die in keinem anderen festen Strukturpinsel liegen; liegt einer
in einem Blatt ohne Cluster (FillOutside), dichtet der Pinsel ab und wird nicht vorgeschlagen.
- **Rot/grün**: neue Testkarte `tests/karte/q3map2/raum_deko.map` mit `raum_deko.nrc.bsp` (NRCs q3map2
  `-meta`): Stopfen 16 × 32 × 32 in einem Loch der Ostwand, Wandstück darunter, Kiste im Raum. Als detail leckt
  der Stopfen (NRC: „leaked“). Mit abgeschalteter Abdicht-Prüfung: `karteoptimierer` 1 FEHL („3 small …“);
  mit: 266 Prüfungen, 0 FEHL – nur die Kiste, „2 more seal the map“; mit Leck (.lin) wie ohne .bsp.
- Auf Taris (`karteoptimierer` mit taris.map und taris.bsp, 231 ms): **3730 Vorschläge, 1995 dichten ab**.
  Nachgebaut (`SP/vis/deko_anwenden.py`, dieselben Regeln, 3709 Pinsel als detail) und gebaut: **kein Leck**;
  strukturelle Pinsel 6718 → 3009, Cluster 9598 → 7548, Portale 38042 → 33800, VIS-Speicher 11,1 → 9,7 GB;
  die Karte wird aber noch offener (`-vis -fast`: 79,8 % statt 67,9 % sichtbar) – das volle VIS bleibt lang.

### 6. VIS wiederverwenden (Stufe 8a) auf Taris

`SP/vis/wieder.py`: `-meta`, `-vis -fast -bhedvisreuse` (rechnet, 19 s); einen Detail-Pinsel um 8 verschoben,
`-meta`, `-vis …`: „visibility reused“ (1 s; der Cache vergleicht die ganze .prt Byte für Byte); einen
Strukturpinsel verschoben: „visibility computed“. Die Option ist vorgegeben aus – der Hinweis nennt sie.

### 7. Tests

- ctest `karte_bau`: `visBedarfAusPrt` (von Hand: 3 Cluster, 4 Portale → 22 Passagen, 720 Bytes; raum.nrc.prt:
  8 Cluster, 10 Portale, 52 Passagen; kaputte .prt), `bauVisVoll`, `visFastInKommandozeile`; für Taris gibt
  dieselbe Rechnung 995496 Passagen und 11116 MB – genau die Zahl, die bhed_q3map2 druckt.
- ctest `karte_optimierer`: raum_deko (siehe 5), Vorlagen 35.
- Selbsttest `stufe9d`, Teil VIS: Steinraum mit Zwischenwand und Tür, Portal-Schwelle 1, `_q3map2_cmdline` mit
  `-vis -fast`: nach dem Bau stehen in der Konsole vor dem VIS-Befehl die Zeile mit den Zahlen aus der .prt,
  die Warnung, der Satz zum Original und die Auswege; mit `-vis -fast` nichts davon. Ergebnis: stufe9d 25 OK / 0 FEHLER.
- Selbsttest `stufe5` (Optimierer mit `deko` auf den Testkarten) 67 OK / 0 FEHLER; ctest karte_bau,
  karte_optimierer, karte_optsicher, q3map2*, handbuch, i18n, hilfe: alle grün; Bau 0 Warnungen; lint_i18n +1
  („KeOptD35 nirgends benutzt“ - wie KeOptD01..34, die über strVonName gesucht werden).

### 8. Empfehlung für den Kumpel

1. Das laufende VIS abbrechen. Es rechnet, was das Original nie hatte; der Rest dauert sehr wahrscheinlich
   Tage (die teuren Portale kommen zuletzt), und ohne 12 GB freien Speicher lagert Windows aus.
2. Für dieselbe Sichtbarkeit wie das Original: `-vis -fast` – im Build-Menü „[Final] Extreme Quality“
   kopieren (Build > Customize…) und `-vis` durch `-vis -fast` ersetzen. Das Licht von „Extreme Quality“
   (`-samples 4 -randomsamples -bounce 8`, ohne `-fast`) braucht auf dieser Karte ebenfalls lange.
3. Vorher **Build > Teil-Kompilierung: VIS wiederverwenden** einschalten: spätere Bauten mit unveränderten
   Portalen (Licht, Entities, Detail-Pinsel) überspringen das VIS.
4. Wer trotzdem das volle VIS will: nur auf einem Rechner mit 16 GB und mehr, nach dem Optimierer-Vorschlag
   „Deko als Struktur“ (3730 Pinsel, bleibt dicht), und mit Tagen rechnen.

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
- VIS-Fund: Struktur und Sichtbarkeit gegen die Original-.bsp (Cluster, Leafs, Knoten, je Cluster die
  sichtbaren Cluster); VIS-Tempo gegen NRCs q3map2.exe auf vier Karten; der Speicherbedarf gegen den Prozess
  (über 8 GB bei 65 % CreatePassages) und gegen die Formel; die Deko-Prüfung gegen NRCs Leck-Meldung.

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
- VIS-Dauer: keine Zahl (die Messreihe zeigt, dass sie aus Portalen und might-see nicht folgt); das volle
  VIS auf Taris konnte hier wegen des 8-GB-Wächters nicht laufen.

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
- VIS: eine Restzeit für PassagePortalFlow, die die Kosten der späten Portale kennt (der lineare Wert ist nur
  eine Untergrenze); „VIS wiederverwenden“ vorgegeben einschalten?

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
- Map editor, build: before a full VIS (`-vis` without `-fast`) the console names portals, clusters and the memory it needs (from the portal file); from the optimizer's portal threshold or when memory gets short a warning follows, also in the progress window, with the ways out (`-vis -fast` for tests, detail, Partial compile: reuse VIS) - and, for a map from Open .bsp, whether the original was built with `-vis -fast`.
- bhed_q3map2 `-vis` prints the real passage memory (NetRadiant-Custom's q3map2 counted half the portals in an int: 268 MB instead of 9 GB on md_rev_TarisUpper2) and the total.
- Optimizer, Decoration as structural: after a build without a leak only brushes with no face at the void of that .bsp are suggested - a small plug in an outer wall is no longer proposed as detail.

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
- Karten-Editor, Bauen: vor einem vollen VIS (`-vis` ohne `-fast`) nennt die Konsole Portale, Cluster und den Speicherbedarf (aus der Portaldatei); ab der Portal-Schwelle des Optimierers oder wenn der Speicher knapp wird, folgt eine Warnung, auch im Fortschrittsfenster, mit den Auswegen (`-vis -fast` für Tests, detail, Teil-Kompilierung: VIS wiederverwenden) - und bei einer Karte aus Open .bsp, ob das Original mit `-vis -fast` gebaut wurde.
- bhed_q3map2 `-vis` druckt den echten Passagen-Speicher (der q3map2 von NetRadiant-Custom zählte die Hälfte der Portale in einem int: 268 MB statt 9 GB auf md_rev_TarisUpper2) und die Summe.
- Optimierer, Deko als Struktur: nach einem Bau ohne Leck werden nur Pinsel ohne Seite am Raum außerhalb dieser .bsp vorgeschlagen - ein kleiner Stopfen in einer Außenwand gilt nicht mehr als detail-Kandidat.

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
- 地图编辑器,编译:在完整 VIS(不带 `-fast` 的 `-vis`)之前,控制台会给出门户数、簇数和所需内存(来自门户文件);达到优化器的门户阈值或内存不足时,会出现警告(进度窗口中也会显示),并给出解决办法(测试用 `-vis -fast`、detail、部分编译:重用 VIS)——对于通过 Open .bsp 打开的地图,还会说明原始地图是否用 `-vis -fast` 编译。
- bhed_q3map2 `-vis` 会打印真实的 passage 内存(NetRadiant-Custom 的 q3map2 只统计一半门户并用 int 累加:在 md_rev_TarisUpper2 上显示 268 MB 而不是 9 GB)以及总量。
- 优化器,装饰为结构:在一次无泄漏的编译之后,只建议在该 .bsp 中没有面朝向地图外部的笔刷——外墙上的小塞子不再被建议设为 detail。

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
- マップエディター、ビルド:完全な VIS(`-fast` のない `-vis`)の前に、コンソールにポータル数、クラスター数、必要なメモリが表示されます(ポータルファイルから)。最適化のポータルしきい値に達するかメモリが足りなくなると、進行状況ウィンドウにも警告と対策(テストには `-vis -fast`、detail、部分コンパイル:VIS を再利用)が出ます。Open .bsp で開いたマップでは、元のマップが `-vis -fast` でビルドされたかどうかも表示されます。
- bhed_q3map2 の `-vis` が実際の passage メモリと合計を表示します(NetRadiant-Custom の q3map2 はポータルの半分だけを int で数えていました:md_rev_TarisUpper2 で 9 GB ではなく 268 MB)。
- 最適化、構造の装飾:リークのないビルドの後は、その .bsp でマップの外側に面していないブラシだけを提案します。外壁の小さな栓はもう detail の候補になりません。

## Integration 2

Zweig `karte1` (Worktree `Y:\karte`), Ausgang 2406e182 (rc606 + H2). Skripte: Schlussrunde `SP/int2/ende.sh`
(nach `SP/rc606/ende.sh`), Wiederholungen allein und Handbuchbilder `SP/int2/nach.sh`, Selbsttests über
`SP/int2/idle.sh` (wie `SP/s8d/idle_jk.sh`: ein Prozess zur Zeit, Idle, 8-GB-Wächter; dazu `bhed_q3map2.exe`
im Laufordner, `BHED_Q3MAP2` und `BHED_HANDBUCH`). Summen `SP/int2/ende_summe.txt`, `SP/int2/nach_summe.txt`.

| Commit | Inhalt |
|---|---|
| 14d9bf50 | Merge `karte9d` (744c1e7a) – Konflikte nur in den erzeugten `handbuch/*/suche.js` (gen_handbuch.py); `i18n.h`/`i18n.cpp` mit gen_i18n.py nachgeprüft (gleich), `src/hilfe_orte.inc` aktuell. Bau 0 Warnungen, ctest 158/158 |
| bd536102 | Merge `mission1` (59acb90c) – `tools/gen_i18n.py` (beide Seiten hängen ein Textmodul an: KARTE9D und MISSION1 behalten); `i18n.*` und `suche.js` neu erzeugt. Bau 0 Warnungen, ctest 158/158 |
| 39a2c83e | Merge `menue2` (21942f86) – Konflikte unten; `i18n.*`, `src/hilfe_orte.inc` und das Handbuch (153 Seiten, 8 Dateien in Konflikt) neu erzeugt. Bau 0 Warnungen, ctest 164/164 |
| (dieser) | Handbuchbilder `h_tour_menue` (4 Sprachen) mit der M2-Leiste; Kopf von `selbsttest_mission1.inc`: die Gizmo-Prüfungen brauchen `BHED_FOTOS`; Notizen |

### Konflikte menue2

- **Leiste des Menü-Editors** (`gui/app_menue.cpp`, 5 Stellen): H2 hatte an die alten Textknöpfe Hinweise
  gehängt (`Me…Hint`: leicht verständlich, zugleich Suchwörter des Helfers, `src/hilfe.cpp` nimmt den
  Hinweis in den Index), M2 hat die Leiste durch Symbolknöpfe (`iconKnopf`) mit eigenen Hinweisen
  (`MeTip…`) ersetzt. Behalten: die M2-Leiste; die zehn Knöpfe, zu denen H2 einen Hinweis geschrieben hat
  (aus dem Spiel, Datei öffnen, Speichern, Export, Bearbeiten, Bedienen, alle Items, Raster, am Raster,
  an Kanten), zeigen den H2-Text; die zehn gleichbedeutenden `MeTip…` aus `tools/i18n_menue2.py`
  entfernt (sonst unbenutzt). Die neuen Knöpfe aus M2 behalten ihre `MeTip…`.
- **`Stand()` / `gui/menueeditor.h`**: die Felder beider Seiten (H2 `exporte`; M2 Bildvergleich,
  Diagnose, Paket, Klänge …).
- **`zuEinemSchritt` (H2) und `exportiere()` (M2)**: H2s Funktion (KI-Aufträge als ein
  Rückgängig-Schritt) bleibt vor M2s gemeinsamer Exportfunktion; H2s Exportzähler steht darin und zählt
  so beide Exportarten. Die Tour „Ein Menü ändern“ (Schritt 5, Knopf Export) wartet auf diesen Zähler;
  der Knopf öffnet seit M2 ein Menü mit „Export .pk3“ und „nur Änderungen“ – beides zählt.
- **`tools/gen_hilfe_orte.py`** (kein Konflikt, aber nötig): der Ortskatalog las im Menü-Editor nur
  `ImGui::Button/RadioButton/Checkbox` – nach M2 fehlten ihm die Leistenknöpfe (die Tour und der Helfer
  zeigen auf `knopf:MeAusSpiel` usw.). Jetzt liest er auch `iconKnopf(tr(Str::Me…), UiIcon::…,
  tr(Str::Hinweis))` (Hinweis aus dem Aufruf wie `leisten()`), die Leiste vor gleichnamigen Knöpfen in
  Fenstern (Prüfen, Schrift bauen); Knöpfe, die erst mit offenem Menü da sind: dazu Video, Schrift,
  Texte, Prüfen, im Spiel testen. `src/hilfe_orte.inc`: 179 Orte, 17 Leistenknöpfe des Menü-Editors mit
  Hinweis (hilfetest grün, Selbsttest hilfe2 mit der Tour tut-menue grün).
- `tools/gen_i18n.py`: MENUE2 nach KARTE9D und MISSION1.
- Handbuch: das Bild `h_tour_menue` zeigte die alte Leiste (Optionsknöpfe, Haken) → neu aus dem Selbsttest
  `BHED_EDITORTEST=handbuch BHED_HB_NUR=hilfe2` in vier Sprachen (`tools/handbuch/bilder.py`);
  `h_tour_spielordner` und `h_tour_update` kamen bitgleich heraus.

### Schlussrunde (Endstand 39a2c83e)

| Schritt | Ergebnis |
|---|---|
| sauberer Neubau (`--clean-first`, /W4 /WX) | **0 Fehler, 0 Warnungen** (938 s) |
| ctest gesamt | **164/164** (265 s) |
| lint_sources, lint_fassung | ok |
| karteneditor voll (mit großer Karte, Fotos) | 3205 OK, 3 FEHLER (1806 s, Spitze 1213 MB) – alle unter Last, allein grün (unten) |
| menueeditor | **46 OK, 0 FEHLER** |
| mission1 | 90 OK, 8 FEHLER – die vier Gizmo-Prüfungen je Durchgang lesen die Fotos, `BHED_FOTOS` fehlte im Lauf (alle Werte −1); mit `BHED_FOTOS` **98 OK, 0 FEHLER** |
| karteneditor + `BHED_KARTE_NUR=hilfe2` | **44 OK, 0 FEHLER** |
| karteneditor + `BHED_KARTE_NUR=hilfe1` | **59 OK, 0 FEHLER** |
| karteneditor + `BHED_KARTE_NUR=stufe9d` | **25 OK, 0 FEHLER** |
| optsicher (md_rev_TarisUpper2, ohne `BHED_OPTSICHER_ALT`) | **14 OK, 0 FEHLER** |
| Modus 1 | **2645 OK, 0 FEHLER** (648 s; die Zwischenablage-Schritte gingen trotz paralleler Läufe) |
| kuerzel | **199 OK, 0 FEHLER** |
| übrige 22 Modi | 0 FEHLER bis auf spulen; dieselben Zahlen wie rc606 (karte 227, kartenfunde 53, fix604b 32, laserfalle 8, geschuetzanim 3, zielstrahl 2, effektgrund 1, figurdeform 1, jk2 15, tuerlauf 13, kamera 95, missionsbau 95, szenen 26, ton 34, npcklasse 12, gluehen 8, gras 13, figurstufen 8, missionsliste 11, rest605 6, modellimfesten 1); spulen 12 OK, 1 FEHLER unter Last, allein grün (unten) |
| Kartenprüfung (278 MD-Karten, gegen kp606) | alle 17 Summen gleich (0 Abstürze, 0 nicht geladen, 0 GPU-/D3D-/Protokollfehler, 1 → 1 nicht gezeichnete Entity, 2 → 2 NPC ohne Modell, 27/29/139/5 fehlende efx/Klänge/Texturen/Modelle) |

### Wiederholungen allein (dieselbe exe, je ein Prozess)

Während der Schlussrunde bauten und testeten andere Agenten (MSBuild, ein VIS-Lauf, fremde jkstudio);
die drei Prüfungen zählen Bilder bzw. messen Bildabstände.

| Prüfung | im Gesamtlauf | allein |
|---|---|---|
| karteneditor Stufe 3: „Bauen fertig“ (−light ohne .bsp, 1200 Bilder) und die Folgeprüfung „ERROR rot, exit 1“ | ERROR kam, der Prozess war nach 1200 Bildern (7,2 s) noch nicht zu Ende | `BHED_KARTE_NUR=stufe3` **222 OK, 0 FEHLER**, fertig nach 350 Bildern (rc606: 362) |
| karteneditor Stufe 7: große Karte p99 < 50 ms | Mittel 27,58 ms, p99 66,37 ms | `BHED_KARTE_NUR=stufe7` **76 OK, 0 FEHLER**, Mittel 11,51 ms, p99 20,64 ms (9d: 10,1 / 17,2) |
| spulen: end_jedi zurück so flüssig wie vor | Einzelbilder 1,3–3,1 s, alle gemessenen Teile zusammen < 1 ms (Ansicht 0,5 ms) | **13 OK, 0 FEHLER**, zurück Mittel 4,7 ms, 95 % 7,4 ms |

Nach den Bildern und dem Kommentar: Bau 0 Warnungen, ctest 164/164.

### Gegen das Vorbild geprüft / nicht geprüft

Die Integration bringt kein neues Verhalten; gegen Spiel und Werkzeug prüfen die Selbsttests der drei
Zweige (siehe deren Notizen: q3map2/NRC bei 9d, OpenJK/ICARUS bei mission1, Engine-Menüs bei M2), alle
hier auf dem Endstand grün. Die Kartenprüfung ist gegen kp606 gleich. Nicht geprüft: Tempo auf ruhiger
Maschine (alle Zeiten unter Last bzw. allein, während andere Agenten liefen).

### Offen

- Die Tour „Ein Menü ändern“ sagt in Schritt 5 „{{MeExportPk3}} schreibt das geänderte Menü in eine .pk3“;
  seit M2 öffnet der Knopf eine Auswahl (alles / nur Änderungen). Der Satz stimmt weiter, nennt die
  Auswahl aber nicht – bei Gelegenheit in `inhalt_hilfe2.py`/`inhalt_touren.py` ergänzen (4 Sprachen).
- Die drei lastempfindlichen Prüfungen (oben) sind unverändert; mit vielen parallelen Bauten können sie
  im Gesamtlauf rot werden.

## Integration 3

Zweig `karte1` (Worktree `Y:\karte`), Ausgang dc61305b (rc606 + H2 + 9d + mission1 + menue2). Skripte (nach
int2): Selbsttests über `SP/int3/idle.sh` (Idle, 8-GB-Wächter, `bhed_q3map2.exe` im Laufordner, `BHED_Q3MAP2`,
`BHED_HANDBUCH`; dazu `SPRACHE=es|cs` setzt `language=` in den Einstellungen des Laufs), Schlussrunde
`SP/int3/ende.sh` (Modus 1 als `1 1es 1cs`, mission1 mit `BHED_FOTOS`), Wiederholungen allein `SP/int3/nach.sh`,
es/cs-Bilder `SP/int3/bilder.sh`. Summen `SP/int3/ende_summe.txt`, `nach_summe.txt`, `bilder_summe.txt`.
Pause 29.10. 17:00–17:27 (Rechner neu gestartet) nach dem ersten Merge.

| Commit | Inhalt |
|---|---|
| b2be8691 | Merge `sprache_es` (b9101a67). Bau 0 Warnungen, ctest 166/166 |
| 62cec6fe | Merge `sprache_cs` (12a2716f). Bau 0 Warnungen, ctest 166/166 |
| 1a564aa4 | Handbuch: der Helfer versteht auch Tschechisch; drei tschechische Texte auf den Sechs-Sprachen-Wortlaut |
| acd5c576 | Selbsttest Karten-Editor-Fotos: wieder in der Sprache des Laufs messen |
| 3a907791 | es/cs-Handbuchbilder neu, deren englisches Bild sich seit 2406e182 geändert hat |
| f6bf047c | Tschechisch: MeshTex „Počet dlaždic“ lief ins Zahlenfeld → „Dlaždice“; Bild `h_karte4_meshtex` (cs) neu |
| (dieser) | Notizen |

### Konflikte

- **sprache_es**: `gui/selbsttest_menue.inc` (M2-Vergleichslauf `BHED_MENUE_VERGLEICH` und der ES-Kommentar
  „Kurzname aus tools/sprachen.py“ beide), `tools/handbuch/gen_handbuch.py` (die KARTE9D-Texte und die Schleife
  über die weiteren Sprachen beide; `tools/gen_i18n.py` mischte sich von selbst: KARTE9D, MISSION1, MENUE2 vor dem
  Abschnitt „Weitere Sprachen“). Erzeugt: `i18n.h/.cpp` (gen_i18n.py), das Handbuch mit `suche.js`
  (gen_handbuch.py; 4 Dateien in Konflikt, dazu `es|cs/menue_editor_m2.html` neu). `src/hilfe_orte.inc` blieb
  gleich (179 Orte).
- **sprache_cs**: `gui/selbsttest.cpp` (AskUnprotect) und `gui/selbsttest_vorschau_a.inc` (FindSimilarAny) –
  ES und CS hatten dieselben zwei Englisch-Annahmen von Modus 1 behoben; eine Fassung behalten (ES: fängt auch
  ein fehlendes `%s` ab). `gui/selbsttest_karte3bis5.inc` (3 Stellen) – die ES-Schleifen über `i18n::languages()`
  enthalten die CS-Ergänzung (Tschechisch in den Listen der Breitenprüfungen). `gui/selbsttest_hilfe1.inc` –
  Spanisch und Tschechisch beide. `src/i18n.cpp` und 153 tschechische Handbuchseiten neu erzeugt.
- **Zwei Lösungen für dieselbe Sache** (kein Textkonflikt, acd5c576): ES ließ die Karten-Editor-Fotos auf
  Englisch laufen, weil der Helfer-Eintrag Help 233 statt 247 breit machte; CS behob die Ursache (der Eintrag folgt
  „Begriffe wie NetRadiant“, `trK` in `gui/app_karte_menue.cpp`). Beide zusammen hätten einen Rückfall des
  CS-Fixes in jeder anderen Sprache verdeckt – die Umschaltung ist raus. `BHED_KARTE_NUR=fotos`: Spanisch
  **78 OK / 0**, Tschechisch **78 OK / 0** (Help 247 × 202 wie NRC).
- **Handbuchtexte** (1a564aa4): ES änderte „Four languages“ → „Six languages“ und die Sprachlisten bei „Sprache“
  und beim Helfer; CS hatte die alten englischen Schlüssel übersetzt (schon mit sechs Sprachen) – nach dem Merge
  fielen drei tschechische Texte auf Englisch zurück (48 statt 45). Schlüssel nachgezogen; der Satz
  „… Chinese, Japanese and Spanish all work“ nennt jetzt auch Tschechisch (CS hat Helfer-Wörter und Frageliste),
  in allen sechs Sprachen.

### Fund: Tschechisch „Počet dlaždic“ im MeshTex-Dialog

ES hatte die Platzprüfung der Plugin-Dialoge verschärft (kein Text darf in ein fremdes Feld rechts von ihm
ragen – auf Spanisch lief „N.º de mosaicos“ ins Zahlenfeld); CS kannte diese Prüfung nicht. Zusammen rot:
karteneditor voll und gezielt `BHED_KARTE_NUR=stufe4 BHED_STUFE4_NUR=texte` **89 OK, 1 FEHLER** („cs MeshTex
Set S/T Scale: 22 Texte, 0 zu breit, 2 in fremden Feldern – mtskach: ragt in mtskacheln“; das alte cs-Bild
`h_karte4_meshtex` zeigt „Počet dlaždi|1.000“). `KeMtTiles` cs → „Dlaždice“ (wie de „Kacheln“, es „Mosaicos“):
derselbe Lauf **90 OK, 0 FEHLER**; Bild aus `BHED_HB_NUR=karte4 BHED_SPRACHE=cs` neu.

### Was auf Spanisch/Tschechisch noch englisch ist

Die seit 2406e182 dazugekommenen Texte gibt es nur in vier Sprachen; sie fallen auf Englisch zurück, nie leer
(ctest `sprache`: jede Kennung in es/cs nicht leer, ohne eigenen Text genau englisch).

| | es | cs |
|---|---|---|
| Programmtexte (`lint_i18n.py --sprache es/cs`) | **121** | **121** |
| davon KARTE9D / MISSION1 / MENUE2 | 19 / 1 / 101 | 19 / 1 / 101 |
| Handbuchtexte (`gen_handbuch.py --pflicht=es/cs`) | **45** | **45** |
| Handbuchseiten mit englischen Texten | **9** | **9** |
| Handbuchbilder englisch (mit Hinweis) | **9** | **9** |

- Seiten (Texte): `menue_editor_m2` (28), `karte_tempo` (6), `figuren`, `karte_optimierer`, `karteneditor`,
  `menue_editor` (je 2), `karte_oeffnen`, `karte_teilbau`, `karteneditor_bauen` (je 1). Unter den 45 sind je
  Sprache 6 Texte, deren Übersetzung schon da ist, aber zum alten englischen Wortlaut gehört (9d/mission1/menue2
  haben ihn geändert; `gen_handbuch.py`: „12 Einträge … nicht mehr gebraucht“).
- Bilder: `h_karte9d_nebel`, `h_karte9d_shader`, `menue_detail`, `menue_diagnose`, `menue_modell`,
  `menue_reiter_hierarchie`, `menue_reiter_text`, `menue_texte_deutsch`, `menue_video`.
- Neu aufgenommen (3a907791), weil sie in es/cs noch das alte Programm zeigten: `h_karte7_*` (6),
  `h_tour_menue`, `menue_bedienen`, `menue_original`, `menue_palette`, `menue_suchen` (das alte es-Bild
  `h_tour_menue` zeigte zudem „No se encontraron los tutoriales“). Sie zeigen das Programm, wie es jetzt auf
  Spanisch/Tschechisch aussieht – die neuen M2-/9d-Beschriftungen darin englisch. Nach der Übersetzung neu
  aufnehmen: Sätze `karte7`, `karte9d`, `hilfe2` (`BHED_EDITORTEST=handbuch BHED_HB_NUR=…`) und
  `BHED_EDITORTEST=menueeditor`, je mit `BHED_SPRACHE=es|cs` und `BHED_FOTOS`, dann `tools/handbuch/bilder.py`.
- Maßtabellen (Schritt 4): die Menügrößen der Karten-Editor-Fotos (`karteneditorFotoSchritte`, 38 Menüs) und die
  Breitenprüfungen änderten sich durch das Zusammenkommen nicht (Volllauf und `fotos` in es/cs grün).

### Schlussrunde (Endstand vor dem Fix; der Fix danach gezielt, unten)

| Schritt | Ergebnis |
|---|---|
| sauberer Neubau (`--clean-first`, /W4 /WX) | **0 Fehler, 0 Warnungen** (1166 s) |
| ctest gesamt | **166/166** (450 s) |
| lint_sources, lint_fassung, lint_i18n_erzeugt | ok |
| karteneditor voll (Englisch, große Karte, Fotos) | 3278 OK, 7 FEHLER (1256 s, Spitze 1132 MB): 1 × cs-MeshTex (Fund oben), 1 × große Karte p99, 5 × Stufe 8/2 „Editor-Daten“ – behoben bzw. allein grün (unten) |
| menueeditor | **46 OK, 0 FEHLER** (dazu in es und cs je 46/0 beim Bildlauf) |
| mission1 (`BHED_FOTOS`) | **98 OK, 0 FEHLER** |
| karteneditor `BHED_KARTE_NUR=hilfe1` / `hilfe2` / `stufe9d` | **59/0**, **44/0**, **25/0** |
| optsicher (md_rev_TarisUpper2) | **14 OK, 0 FEHLER** |
| Modus 1 Englisch | 2647 OK, 1 FEHLER (Zwischenablage) – allein **2648/0** |
| Modus 1 Spanisch | **2648 OK, 0 FEHLER** |
| Modus 1 Tschechisch | 2643 OK, 7 FEHLER (Tutorial k-werte-bearbeiten, unten) – allein **2648/0** |
| kuerzel | **199 OK, 0 FEHLER** |
| übrige 22 Modi | 0 FEHLER bis auf spulen, dieselben Zahlen wie Integration 2 (karte 227, kartenfunde 53, fix604b 32, laserfalle 8, geschuetzanim 3, zielstrahl 2, effektgrund 1, figurdeform 1, jk2 15, tuerlauf 13, kamera 95, missionsbau 95, szenen 26, ton 34, npcklasse 12, gluehen 8, gras 13, figurstufen 8, missionsliste 11, rest605 6, modellimfesten 1); spulen 11 OK, 2 FEHLER – allein **13/0** |
| Kartenprüfung (278 MD-Karten, gegen kp606) | alle 17 Summen gleich (0 Abstürze, 0 nicht geladen, 0 GPU-/D3D-/Protokollfehler, 1 → 1 nicht gezeichnete Entity, 2 → 2 NPC ohne Modell, 27/29/139/5 fehlende efx/Klänge/Texturen/Modelle) |

### Wiederholungen allein (dieselbe exe, je ein Prozess)

Während der Schlussrunde liefen die Agenten in `Y:\vis`, `Y:\menue` und `Y:\mission` (Bauten, eigene jkstudio).

| Prüfung | im Gesamtlauf | allein |
|---|---|---|
| Stufe 7: große Karte p99 < 50 ms | Mittel 25,56 ms, p99 82,03 ms | `stufe7` **78 OK, 0 FEHLER**, Mittel 14,13 ms, p99 26,07 ms |
| Stufe 8/2: Editor-Daten (−keeplights, `.bhed`, Byte-Vergleich, Öffnen mit/ohne Begleitdatei; 5 Prüfungen) | „Failed to get a listening socket on port 39000“ – der Port der Bauüberwachung war belegt (fremder jkstudio-Bau), der Bau brach in Schritt 1 ab | `stufe8`: die 5 Prüfungen **OK** (zweimal); 3 andere FEHLER, s. Offen |
| Modus 1: Copy → Zwischenablage | FEHLER | **2648 OK, 0 FEHLER** |
| Modus 1 Tschechisch: Tutorial k-werte-bearbeiten mit offenem Editor | ein Klick „Další“ (Schritt 4 → 5) kam nicht an, die 6 Folgeprüfungen rot | **2648 OK, 0 FEHLER** |
| spulen: intro2_sith, end_jedi zurück so flüssig wie vor | FEHLER | **13 OK, 0 FEHLER** (intro2_sith zurück Mittel 18,3 ms, 95 % 31,5 ms) |

Nach dem Fix (nur `tools/i18n_sprache_cs.py` → `src/i18n.cpp`): Bau 0 Warnungen, `stufe4`/`texte` **90/0**,
ctest **166/166 (405 s)**.

### Gegen das Vorbild geprüft / nicht geprüft

Die Integration bringt außer dem tschechischen Kurztext kein neues Verhalten. Gegen NRC geprüft: die 38
Menügrößen der Karten-Editor-Fotos auf Englisch (Volllauf), Spanisch und Tschechisch (`fotos`), die Platzprüfung
der nachgebauten NRC-Dialoge in allen sechs Sprachen (Stufe 4: `texte` 90/0, `reste` im Volllauf); Spiel/OpenJK
über die Selbsttests der Zweige (alle grün) und die Kartenprüfung (gleich kp606). Nicht geprüft: Tempo auf
ruhiger Maschine; die Selbsttests außer Modus 1, menueeditor und `fotos` nur auf Englisch; die neuen englischen
Texte in es/cs (Übersetzung folgt).

### Offen

- Übersetzung der 121 Programmtexte und 45 Handbuchtexte je Sprache (`tools/i18n_sprache_es|cs.py`,
  `tools/handbuch/inhalt_es|cs.py`), danach die Bildsätze oben neu (es/cs).
- `BHED_KARTE_NUR=stufe8` allein: „Notify when a build is done“ nicht ganz sichtbar (Eintrag 329..858, Menü
  330..856), der Klick kommt nicht an (unter der Maus nichts) → „Build > Melden: aus“ rot. Im Volllauf grün;
  mit der exe von Integration 2 allein ebenso rot (34 OK / 6) – nicht aus dieser Integration, nicht behoben.
- Parallele jkstudio-Bauten teilen den Port 39000 der Bauüberwachung (NRC `watchbsp`); ein zweiter Bau zur selben
  Zeit bricht ab. Für den Benutzer selten (zwei Programme bauen gleichzeitig), für parallele Testläufe ein Grund
  für rote Bauprüfungen.
- Das Tutorial-Klicken in Modus 1 (`k-werte-bearbeiten`, Pause 0,25 s je „Weiter“) ist unter Last nicht sicher –
  bisher nicht als lastempfindlich bekannt.
- Aus den Sprach-Notizen unverändert: NRC-Menünamen bleiben englisch; die Auflösungsliste des Menü-Editors
  beginnt fest mit „Window“; Pluralformen im Tschechischen; `lint_i18n.py` ohne `--sprache` meldet die bekannten
  „nirgends benutzt“ (liest `gui/*.inc` nicht).

## Integration 4

Zweig `karte1` (Worktree `Y:\karte`), Ausgang 1247acc6 (Integration 3 + modell1). Gemergt werden nur die im Auftrag
genannten Stände (spätere Commits der Zweige kommen in Integration 5). Skripte (nach int3): Selbsttests über
`SP/int4/idle.sh`, Schlussrunde `SP/int4/ende.sh`, Konflikthelfer `SP/int4/loese.py` (Generator-Listen: beide
Seiten behalten). Bau je Merge inkrementell in `Y:/karte/build-test` (`--parallel 4`), ctest unter Idle; fünf andere
Agenten bauten und testeten gleichzeitig (alle Zeiten unter Last).

| Commit | Inhalt |
|---|---|
| 0adbdd00 | Merge `uebersetzung1` (3cb87d4c) – keine Konflikte; erzeugte Dateien mit den Generatoren nachgeprüft (gleich). Bau 0 Warnungen (574 s), ctest 166/166 (513 s) |
| 5c1cde30 | Merge `pruefung1` (32dfcdd8) – keine Konflikte. Bau 0 Warnungen (504 s), ctest 168/168 (450 s) |
| d216318c | Merge `vis1` (214c4ab2) – Konflikte nur in den Generator-Listen (`tools/gen_i18n.py`, `tools/handbuch/gen_handbuch.py`: VIS1 nach UEBERSETZUNG1, vor den weiteren Sprachen) und den erzeugten Dateien (neu erzeugt). Bau 0 Warnungen (1339 s unter Last) |
| 5c074713 | Merge `zeitleiste1` (3be1a9c9) – `CMakeLists.txt` (Tests von pruefung1 und der Zeitleiste beide), `gui/selbsttest.cpp` (modell1 und zeitleiste0/1 beide), Generator-Listen, erzeugte Dateien |
| 669d0f72 | Merge `menue5` (3f83163c, enthält menue3/M4) – Konflikte und zwei Funde unten |
| ec9b2602 | Merge `dock1` (865cebde) – `gui/selbsttest.cpp` (modell1, zeitleiste0/1 und dock1 nebeneinander; zeitleiste1 behält seine Endschritte), `tools/handbuch/inhalt.py` (Dock-Seite und die Menü-Editor-Seiten), Generator-Listen, erzeugte Dateien; `gui/app_menue.cpp` ohne Konflikt (dock1: Panels als eigene Fenster; M3–M5b-Leiste) |
| 3ee05b49 | i18n nach den Merges: es/cs-Texte der alten Zeitleiste entfernt, der Pfeil „v“ des Menü-Editors ist ein Zeichen (Fund unten) |
| (dieser) | Notizen |

**Abweichung vom Ablauf (Zeitgrenze 08:40):** Bau und ctest je Merge dauerten unter der Last der fünf anderen Agenten
bis zu 22 min + 9 min (vis1: Bau 1339 s, ctest blieb bei `updater_uebergang` > 5 min stehen). Darum: Merges 1–2 je
Bau + ctest grün; Merge 3 gebaut (0 Warnungen), ctest nach 80/169 grünen Tests abgebrochen (eigene PIDs per
`taskkill /T`, Restordner `bhed_uebergang_*` entfernt); die Merges 4–6 während des ctest von Merge 3 in einem
Wegwerf-Klon (`git clone --shared` im SP, danach gelöscht) gelöst und committet, in `Y:\karte` vorgespult und
**zusammen** gebaut und getestet (unten). Die Konfliktlösungen sind dieselben wie bei Einzelmerges; nur der
Zwischenstand nach Merge 4 und 5 ist nicht für sich gebaut.

### Konflikte menue5 (669d0f72)

- **`gui/app_menue.cpp`, Leiste** (6 Stellen): M4 ersetzt `iconKnopf` durch `leistenKnopf` (Symbol oder Symbol +
  Text, Einstellung „Leisten mit Beschriftung“), M5b setzt den Projektknopf davor und lässt „Speichern“ im Projekt das
  Projekt speichern. menue5 zweigt vor Integration 2 ab und nutzt deshalb die zehn `MeTip…`, die Integration 2
  zugunsten der H2-Hinweise (`Me…Hint`, zugleich Suchwörter des Helfers) entfernt hat. Behalten: die M4/M5b-Leiste
  mit den H2-Hinweisen dieser zehn Knöpfe (Speichern: `MeSpeichernHint`, im Projekt `MePrjSpeichernTip`).
- **Leerer Editor**: M5b `zeichneProjektLeer()` (Text, Neues Projekt / Projekt öffnen) und darunter der H2-Knopf
  „Zeig mir wie“ (Tour `tut-menue`).
- **`src/menue/vorschau.cpp`**: zeitleiste1 hat RE_Font_DrawString nach `schriftZeichnen()` verlegt (auch für die
  Untertitel der Vorschau), M3 hat in der alten Inline-Fassung die asiatischen Zweige ergänzt (fScaleAsian,
  fAsianYAdjust, AnyLanguage_ReadCharFromString, asiatische Glyphen). Jetzt trägt die gemeinsame Funktion den
  M3-Code (Parameter `Kodierung`, `asienGlyphen`, Standard Latin1 wie `schriftBreite`); dabei auch der M3-Fix,
  dass ein `^` ohne Ziffer gezeichnet wird (tr_font.cpp: „purposely falls thrugh“) – gilt damit auch für Untertitel.
- `gui/menueeditor.h` (H2 `zuEinemSchritt` und die M5b-Projektfunktionen beide), `src/tour.cpp`,
  `tests/tourtest.cpp`, `gui/tour_messwerte.cpp` (H2 hatte `modus:4` = Menü-Editor schon, die H2-Fassung ist die
  größere), `tools/handbuch/inhalt.py` (hilfe2- und menue3/5/5b-Seiten), `tools/gen_i18n.py` (MENUE3, MENUE5).
- `MeAufloesungFenster` stand in `i18n_uebersetzung1.py` und `i18n_menue3.py` (gleicher Text; gen_i18n:
  „doppelte Kennung“) – die Zeile in `i18n_uebersetzung1.py` entfernt.

### Funde aus dem Zusammenspiel (nachgestellt → behoben → grün)

1. **Zwei Touren `tut-menue`.** H2 hat „Ein Menü ändern“ (aus dem Spiel öffnen, verschieben, bedienen,
   exportieren; Selbsttest hilfe2), M5b „Dein erstes eigenes Menü“ (Projekt, Knopf, Layout, Ereignis, Speichern;
   Selbsttest menueeditor `m5b`) – beide unter derselben Kennung, die M5b-Schritte hätten die H2-Schritte
   überschrieben. Rot: `gen_handbuch.py` bricht ab („doppelte Seiten-id“). Die M5b-Tour heißt jetzt
   **`tut-menue-eigen`** (Seite `inhalt_menue5b.py`, Schritte `inhalt_touren.py`, Tourliste, Selbsttest
   `selbsttest_menue.inc`, Kommentare in `tour_messwerte.cpp`, Helferbereich „menue“ in `src/hilfe.cpp`,
   Tutorial-Übersicht: beide Karten unter „Menü-Editor“). Grün: Handbuch 158 Seiten × 6 Sprachen, beide Seiten mit
   ihren Bildern (`h_tour_menue`, `h_tut_menue_1…`), dieselben 18 Tourschritte ohne Bild wie vorher.
2. **Ortskatalog ohne Leiste des Menü-Editors.** `tools/gen_hilfe_orte.py` las seit Integration 2
   `iconKnopf/iconTextKnopf`; mit M4 heißen die Leistenknöpfe `leistenKnopf` → 167 statt ≥ 179 Orte, 16 Knöpfe
   fehlten (darunter `knopf:MeAusSpiel`, das Ziel von Schritt 2 der Tour `tut-menue`, und `MeSpeichern`, Schritt 6
   von `tut-menue-eigen`). Jetzt liest er auch `leistenKnopf` und bedingte Hinweise (`tr(a ? Str::X : Str::Y)`, X
   gilt): **183 Orte**, nach dock1 191.
3. **lint_i18n** (liest seit uebersetzung1 auch `gui/*.inc`): 25 es/cs-Übersetzungen zu Kennungen der alten
   Zeitleiste, die zeitleiste1 entfernt hat (`TlBewegung` … `TracksMoving`) → entfernt; der Knopf „v“ (nach unten,
   neben „^“) im Layout-Panel von M5 galt als fester Text → in die Zeichen-Ausnahmen wie „x“. Rot 6 → 5
   Beanstandungen; übrig (offen, unten): 5 übersetzte Texte, die niemand mehr benutzt.

Bekannt und **hier nicht behoben** (Auftrag): mit pruefung1 sind die Echtzeit-Decals im Karteneditor unsichtbar
(`app_karte_echtfx.inc` reicht keine BSP herein, „ohne Flächen keine Marke“); das behebt der Zweig `reste1`.

### Was auf Spanisch/Tschechisch noch englisch ist (für Übersetzung Runde 2)

uebersetzung1 hatte es/cs auf 0 gebracht; die Texte der danach gemergten Zweige fallen auf Englisch zurück, nie leer.

| | es | cs |
|---|---|---|
| Programmtexte (`gen_i18n.py --fehlend`) | **354** | **354** |
| davon zeitleiste1 / menue5 / menue3 / dock1 / vis1 | 154 / 111 / 63 / 24 / 2 | 154 / 111 / 63 / 24 / 2 |
| Handbuchtexte (`gen_handbuch.py --pflicht=es/cs`) | **149** | **149** |
| Handbuchbilder englisch | **25** | **25** |

`gen_handbuch.py`: „66 Einträge in uebersetzung/ werden nicht mehr gebraucht“ – vor allem es/cs-Übersetzungen zu
Handbuchtexten, deren englischen Wortlaut zeitleiste1 (Zeitleiste neu) und vis1 geändert haben.

### Prüfungen (Endstand 3ee05b49, unter Last)

| Schritt | Ergebnis |
|---|---|
| Bau nach Merge 1 / 2 / 3 (inkrementell, /W4 /WX) | **0 Warnungen** (574 / 504 / 1339 s) |
| ctest nach Merge 1 / 2 | **166/166** (513 s) / **168/168** (450 s) |
| ctest nach Merge 3 | 80/169 grün, dann abgebrochen (Zeit, s. oben) |
| Bau nach Merge 4–6 zusammen (inkrementell, fast alles neu übersetzt) | **0 Fehler, 0 Warnungen** (890 s) |
| ctest gesamt auf dem Endstand | **177/177** (796 s) |
| lint_sources, lint_fassung, lint_i18n_erzeugt | ok |
| lint_i18n | 5 Beanstandungen (unbenutzte Texte, offen); vorher 6 |
| Handbuch-Bau (`gen_handbuch.py`) | 159 Seiten × 6 Sprachen, ohne Änderung gegenüber dem Commit |
| Selbsttest menueeditor `BHED_MENUE_NUR=m5b` (Tour `tut-menue-eigen` mit echten Aktionen) | **24 OK, 0 FEHLER** (23 s; Tour läuft mit 6 Schritten unter der neuen Kennung) |
| Selbsttest karteneditor `BHED_KARTE_NUR=hilfe2` (u. a. Tour `tut-menue` auf der M4-Leiste) | **44 OK, 0 FEHLER** (76 s, wie Integration 2/3) |
| Selbsttests dock1 / menueeditor (voll) | **86 OK, 0 FEHLER** (29 s) / **105 OK, 0 FEHLER** (64 s) |
| karteneditor kurz + Fotos (`BHED_KARTE_NUR=fotos`, `BHED_FOTOS`) / `stufe9d` / `hilfe1` | **78/0** (44 s) / **28/0** / **59/0** |
| zeitleiste / modellleistung / kamera / missionsbau | **47/0** / **7/0** / **95/0** / **95/0** |
| zeitleiste1 | 73 OK, 4 FEHLER – genau der Stand des Zweigs (NOTIZEN_zeitleiste1 „zl2k3: 73 OK, 4 FEHLER“: nur der JK2-Teil im vollen Lauf, kejim_post nach „New mission“ nicht in der Liste; allein grün) – nicht aus dem Zusammenspiel |
| mission1 (`BHED_FOTOS`) | nur angelaufen: 47 OK, 0 FEHLER bis zur Zeitgrenze des Laufs (240 s, wegen 08:40) – nicht fertig |

Nicht mehr geschafft (Zeitgrenze): sauberer Neubau (`--clean-first`), Selbsttest Modus 1, mission1 zu Ende,
alle Modi, Kartenprüfung. Das Skript dafür liegt bereit: `SP/int4/ende.sh`
(`SCHRITTE="neubau ctest modi"`, Modi wie im Auftrag).

### Gegen das Vorbild geprüft / nicht geprüft

Die Integration bringt außer den Funden kein neues Verhalten. `schriftZeichnen` folgt jetzt für Menüs und
Untertitel derselben Portierung von RE_Font_DrawString (M3, gegen tr_font.cpp; „^“ ohne Ziffer wird gezeichnet) –
im Programm nur über ctest geprüft, nicht im Spiel. Nicht geprüft: Modus 1 und die übrigen Modi auf dem Endstand, sauberer Neubau,
Kartenprüfung, Tempo (alles unter Last).

### Offen / nächster Schritt

- **Nächster Schritt:** Rest der Schlussrunde auf diesem Stand: sauberer Neubau und Modus 1
  (`SCHRITTE="neubau" sh SP/int4/ende.sh`, dann `SCHRITTE=modi MODI="1 mission1" WEITER=1 sh SP/int4/ende.sh`), danach wenn Zeit
  alle Modi + Kartenprüfung gegen kp606 (wie `SP/int3/ende.sh`, Schritt `kp`).
- lint_i18n: 5 übersetzte Texte, die niemand mehr benutzt – `TlKamera`, `TlKatBlick`, `TlKatAblauf` (zeitleiste1
  hat die Bedienung entfernt), `MeKlaenge`, `MeTipKlaenge` (menue5 hat den Knopf „Klänge“ nicht mehr). Entfernen oder
  wieder anschließen ist Sache der Zweige (Löschen ändert `i18n.h` → Neubau).
- es/cs: 354 Programmtexte, 149 Handbuchtexte, 25 Bilder (Tabelle oben) – Übersetzung Runde 2.
- Echtzeit-Decals im Karteneditor (pruefung1) – behebt `reste1`.
- Aus Integration 3 unverändert: Port 39000 der Bauüberwachung bei parallelen Bauten; `stufe8` allein
  („Notify when a build is done“); Tutorial-Klicken in Modus 1 unter Last.

## Integration 5

Zweig `karte1` (Worktree `Y:\karte`), Ausgang b6309500 (Integration 4). Gemergt werden nur die im Auftrag
genannten Stände (spätere Commits von M7, dock4, Zeitleiste 4, pruefung3, uebersetzung2 kommen in Integration 6).
Skripte: `SP/int5/` – `bauct.sh` (Bau und/oder ctest unter Idle, Summe `summe.txt`), `selbst.sh` (Selbsttests
nacheinander über `idle.sh`, das jetzt mit `CFG=` einen anderen Einstellungsstand nimmt), `ende.sh` (Neubau),
`loese.py` (Generator-Listen: beide Seiten). Die Merges 3–5 wurden in einem Wegwerf-Klon (`git clone --shared` im
SP, danach gelöscht) gelöst, während in `Y:\karte` der vorige Merge baute und testete; in `Y:\karte` dann je einzeln
vorgespult, **gebaut und mit ctest geprüft** (kein Sammelschritt wie in Integration 4). Alle Zeiten unter Last
(andere Agenten bauten und testeten gleichzeitig).

| Commit | Inhalt |
|---|---|
| 086fb3a1 | Merge `reste1` (524a08b1) – keine Konflikte. Bau 0 Warnungen (622 s), ctest **178/178** (415 s) |
| 6edf475c | Merge `vis1` (6236b69f, vis3) – keine Konflikte. Bau 0 Warnungen (415 s), ctest **178/178** (277 s) |
| da67549b | Merge `zeitleiste1` (160467ee, Zeitleiste 3) – Konflikte nur in `handbuch/*/suche.js` (neu erzeugt; die es/cs-Seite `zeitleiste` folgt dem neuen englischen Text). Bau 0 Warnungen (502 s), ctest **178/178** (298 s) |
| 454387b0 | Merge `menue6` (72e43757, M6) – Konflikte unten. Bau 0 Warnungen (682 s), ctest **183/183** (362 s) |
| 41e04b7d | Merge `dock1` (56f58c66, dock3) – Konflikte nur in `suche.js` (neu erzeugt). Bau 0 Warnungen (525 s), ctest **183/183** (281 s) |
| d7dd6b28 | Punkt 1: überholte Texte entfernt, Helferpfad der Kameraleiste, Handbuchsatz zum Ton-Schalter |
| 3cae6104 | Punkt 2: Untertitel von Jedi Outcast zeichnen ein `^` ohne Ziffer nicht (JK2_MODE) |
| daf88e15, 0a544a42 | Fund 3: Selbsttest zeitleiste1 – Aufnahmeprüfungen unabhängig von der Bildzeit |
| (dieser) | Notizen |

### Konflikte menue6 (454387b0)

- **`gui/app_menue.cpp`, Leiste** (2 Stellen): menue6 zweigt wie menue5 vor Integration 2 ab und nutzt noch
  `MeTipExport`/`MeTipAnKanten`, die Integration 2 zugunsten der H2-Hinweise entfernt hat. Behalten: die H2-Hinweise
  (`MeExportPk3Hint`, `MeAnKantenHint`), dazu die M6-Knöpfe „Menüfluss“ (vor dem Export), Tab-Reihenfolge und
  Referenzbild (nach „An Kanten“) sowie `zeichneFlussFenster`/`zeichneSchliessenFrage`. Vor dem Commit geprüft: jede
  `Str::`-Kennung in `app_menue.cpp` und `app_menue_m6.inc` steht in den i18n-Listen.
- `tools/gen_i18n.py` (DOCK1 und MENUE6 vor „Weitere Sprachen“), `tools/handbuch/inhalt.py` (beide Importe; die
  Seitenliste von karte1 mit `inhalt_dock` und `inhalt_hilfe2`, `inhalt_menue6` nach `inhalt_menue5b` – die Liste von
  menue6 kannte hilfe2 noch nicht).
- Erzeugte Dateien (i18n.h/.cpp, 620 Handbuchseiten de/en/ja/zh – jede Seite trägt die Navigation –, `suche.js`)
  mit den Generatoren: 3213 Texte, Ortskatalog **195 Orte** (vorher 191, die M6-Knöpfe), Handbuch 160 Seiten × 6 Sprachen.

### Punkt 1: unbenutzte Texte (lint_i18n rot 5 → grün)

| Text | Herkunft | Befund | Gemacht |
|---|---|---|---|
| `TlKatBlick`, `TlKatAblauf` | Kategorien „Blick“/„Ablauf“ der alten Zeitleiste | zeitleiste1 baut die Zeilen anders (`ZlDarsteller`, `ZlWartet` …) und hat laut NOTIZEN_zeitleiste1 „TlKat*“ entfernt – diese zwei blieben stehen | überholt: entfernt (gen_i18n.py, es/cs) |
| `TlKamera` | Zeile „Kamera“ der alten Zeitleiste | blieb laut zeitleiste1 für den Ortskatalog: Pfad der Kameraleiste im Helfer `ViewMap>TlKamera>…` (über `strVonName`, darum sah lint_i18n keine Benutzung). Die neue Zeitleiste heißt die Zeile **„Kamera – Ort“** (`ZlOrt`) – genau das Tourziel `zeitleiste.kamera` der Vorbedingung | Pfad jetzt Map view > Camera – position > … (gen_hilfe_orte.py, 9 Orte); `TlKamera` entfernt |
| `MeKlaenge`, `MeTipKlaenge` | M2-Knopf „Klänge“ | M4 (992622f5) ersetzte ihn bewusst durch den Ton-Schalter der Leiste (`MeTonAn`/`MeTonAus`, Einstellung `menueTon`, hält laufende Klänge an) – keine Bedienung verloren | überholt: entfernt (i18n_menue2.py, es/cs). Die Handbuchseite `menue_editor_m2` nannte noch `{{MeKlaenge}}` – jetzt der Ton-Schalter (4 Sprachen; die es/cs-Übersetzung mitgezogen, damit sie nicht englisch zurückfällt) |

Rot: lint_i18n „5 übersetzte Texte werden nirgends benutzt“; grün: „i18n-Prüfung: ok“. Danach Handbuch 160 × 6, es/cs
weiter 172 Texte englisch (unverändert), Selbsttests hilfe1/hilfe2 grün (unten).

### Punkt 2: `^` ohne Ziffer in Untertiteln (rot → grün)

Quellen: `Y:\quellen` enthält weder tr_font.cpp noch cg_text.cpp; gelesen aus `SP\m3\ojq\src` (OpenJK:
`code/rd-common/tr_font.cpp`, `code/cgame/cg_text.cpp`, `codeJK2/cgame/cg_text.cpp`) und `SP\mdgit\code` (Movie Duels:
`rd-common/tr_font.cpp`, `cgame/cg_text.cpp`).

- **CG_DrawCaptionText** (JKA, JK2, MD gleich): `cgi_R_Font_DrawString(x, y, cg.captionText[i], textcolor_caption,
  cgs.media.qhFontMedium, -1, fFontScale)` mit dem unveränderten Text; CG_CaptionText bricht nur die Zeilen
  (Breite über RE_Font_StrLenPixels) und nimmt keine `^` heraus.
- **RE_Font_DrawString** ohne JK2_MODE (OpenJK SP und Movie Duels gleich): `case '^'` mit Ziffer setzt die Farbe,
  sonst „purposely falls thrugh“ – das `^` wird gezeichnet. **Unter JK2_MODE** (Jedi Outcast) endet `case '^'` immer
  mit `break`: ein `^` ohne Ziffer wird nicht gezeichnet, das nächste Zeichen normal. Die Breiten (StrLenPixels:
  JKA/MD überspringen `^0`–`^9`, JK2 zählt alles) hatte `untertitel::breite` schon richtig.
- **Befund:** Jedi Academy und Movie Duels richtig; **Jedi Outcast falsch** – seit Integration 4 zeichnete die
  gemeinsame Funktion das `^` auch dort. Behoben: `schriftZeichnen(…, jk2Modus)`, `untertitel::zeichnen(…, Spiel)`
  reicht das Spiel durch, die Zeitleiste zeichnet ihre Untertitel darüber. ctest `untertitel` mit „a^^1b“:
  **rot** (JK2: 3 Bilder statt 2), **grün** (JKA/MD: `^` gezeichnet, JK2 nicht; `b` in allen drei rot).
- Nicht Teil dieses Punkts: `mbRoundCalcs` ist für normale Schriften aus (nur Ersatzschriften), `RE_Font_GetVariant`
  betrifft die `_sharp`-Varianten.
- **Nebenbefund (nicht behoben, Sache des Menü-Zweigs):** JK2-Menüs im Menü-Editor zeichnen ein `^` ohne Ziffer
  ebenfalls (die `Umgebung` der Vorschau kennt das Spiel nicht; `z.spiel == Spiel::JK2` wäre vorhanden), und
  `schriftBreite` überspringt `^0`–`^9` auch für JK2, wo StrLenPixels sie mitzählt (Item_SetTextExtents, Ausrichtung).

### Fund 3 aus den Läufen: Selbsttest zeitleiste1 hing an der Bildzeit (rot → grün)

- Rot: `i5zeitleiste1` und Wiederholung `i5zl1rot` je **79 OK, 1 FEHLER** – „Aufnahme mini: das Modell steht am
  gesteuerten Ort“: das Modell war 80 Einheiten vom Ort der Szene, die Prüfung wollte mehr als 100 (Zeitleiste 3 maß
  119 … 391). Das Programm war richtig (das Modell stand am gesteuerten Ort); wie weit die Figur in 45 + 35 + 35
  gehaltenen Bildern kommt, hängt von der Bildzeit ab.
- Zwischenlauf `i5zl1gr` (Vergleich mit dem gesteuerten Ort, Toleranz 1): 78 OK, 2 FEHLER – academy1 2,46 Einheiten
  daneben (die Figur gleitet nach dem Loslassen aus, gezeichnet wird ein Bild später) und md_am_sith „die Vorschau
  läuft den Weg wie das Spiel“: guard1 lief mit 45 Proben 492 Einheiten weit, hinter das Wegenetz („finds no path“).
  Die Proben aus Zeitleiste 3 kommen je Bild, sobald die Figur 4 Einheiten weiter ist – also auch bildzeitabhängig.
- Behoben (nur der Test): W wird losgelassen, sobald der gesteuerte Ort 330 Einheiten vom Start ist (statt nach
  45 Proben); die Modellprüfung vergleicht das gezeichnete Modell mit dem gesteuerten Ort (≤ 1 Einheit, gesteuert
  > 32 Einheiten vom Ort der Szene – der Fehler vor Zeitleiste 3 bleibt erkannt) und wartet dafür bis zu 60 Bilder.
  Grün: `i5zl1gr2` **80 OK, 0 FEHLER** (academy1 0,79 nach 15 Bildern, md_am_sith 375 Einheiten, mini 0).

Sonst zeigten die Selbsttests keine Fehler aus dem Zusammenspiel.

### Prüfungen (unter Last)

| Schritt | Ergebnis |
|---|---|
| Bau je Merge (inkrementell, /W4 /WX) | **0 Warnungen** (Tabelle oben) |
| ctest je Merge | 178/178, 178/178, 178/178, 183/183, 183/183 |
| Bau + ctest nach Punkt 1 und 2 | 0 Warnungen (612 s), **183/183** (334 s) |
| Sauberer Neubau (`--clean-first`, alle Ziele) auf 0a544a42 | **0 Fehler, 0 Warnungen** (1360 s) |
| ctest gesamt danach | **183/183** (440 s) |
| lint_sources, lint_fassung (rc606), lint_i18n_erzeugt, lint_i18n, lint_gapp, lint_imgui_context, lint_layout, lint_tabellenid, lint_hoehe, lint_feldinit, lint_abstand, lint_doppelanzeige | ok |
| übrige Lints | dieselben Befunde wie auf b6309500 (d3dbesitz 20, efxflags, endtable 2, fremdprogramm, mapstate, pruefcodes 11, settings 1); lint_portability 233 → 236: drei neue „std::size_t ohne `<cstddef>`“ in den M6-Tests `menuefluss`, `menuefokus`, `menuelesbar` (wie 113 gleichartige Befunde vorher; Sache von menue6) |
| Handbuch-Bau (`gen_handbuch.py`), gen_i18n, gen_hilfe_orte | 160 Seiten × 6 Sprachen, 3208 Texte, 195 Orte – ohne Änderung gegenüber dem Commit |
| Selbsttest reste1 (`BHED_KARTE_NUR=reste1`) | **13 OK, 0 FEHLER** |
| menueeditor, Movie-Duels-Daten / JKA-Daten („GameData original“, nur gelesen) | **121 OK, 0 FEHLER** / **121 OK, 0 FEHLER**; GameData original danach unverändert (keine Datei neuer als der Start) |
| zeitleiste1 voll | 79/1 → **80 OK, 0 FEHLER** (Fund 3) |
| zeitleiste | **47 OK, 0 FEHLER** |
| dock1 ohne / mit `BHED_DOCKSPACE=1` | **86 OK, 0 FEHLER** / **92 OK, 0 FEHLER** |
| dock2mass | **29 OK, 0 FEHLER** |
| karteneditor kurz + Fotos (`BHED_KARTE_NUR=fotos`, `BHED_FOTOS`) | **78 OK, 0 FEHLER** |
| modellleistung | **7 OK, 0 FEHLER** |
| hilfe1 / hilfe2 (karteneditor) | **59 OK, 0 FEHLER** / **44 OK, 0 FEHLER** |
| mission1 voll (`BHED_FOTOS`) | **98 OK, 0 FEHLER** (636 s) |
| Modus 1 (en, `BHED_HANDBUCH`) | erster Lauf 2648 OK, 2 FEHLER (478 s) – beide in der Windows-Zwischenablage („Copy schreibt //(BHVD) …“, „… legt das ganze Skript in die Zwischenablage“); gleichzeitig liefen vier Selbsttests anderer Agenten (missionsbau, Menü-Editor, Handbuch), die dieselbe Zwischenablage des Systems benutzen. Wiederholung: **2650 OK, 0 FEHLER** (500 s), 0 Zusicherungen |

Die Selbsttests liefen auf dem Stand nach Punkt 2 (3cae6104), zeitleiste1 grün auf daf88e15, Modus 1 auf dem
sauberen Neubau.

### Was auf Spanisch/Tschechisch noch englisch ist (für Übersetzung Runde 2)

| | es | cs |
|---|---|---|
| Programmtexte (`gen_i18n.py`) | **428** (Integration 4: 354; dazu vor allem M6) | **428** |
| Handbuchtexte | **172** | **172** |
| Handbuchbilder englisch | **32** | **32** |

### Gegen das Vorbild geprüft / nicht geprüft

- Punkt 2 gegen die Quellen (OpenJK SP mit und ohne JK2_MODE, Movie Duels): CG_DrawCaptionText, CG_CaptionText,
  RE_Font_DrawString, RE_Font_StrLenPixels; im Programm über ctest geprüft, nicht im laufenden Spiel (keine Spielstarts).
- Punkt 1 gegen die Bedienung: die Kategorien und die Kamerazeile gibt es in der neuen Zeitleiste nicht mehr, der
  Ton-Schalter ersetzt „Klänge“ (M4-Commit, Handbuch M4); Ortskatalog und Touren über hilfe1/hilfe2.
- Nicht geprüft (Auftrag): alle Modi, Kartenprüfung (Schlussrunde nach Integration 6); Tempo (alles unter Last).

### Offen / nächster Schritt

- **Nächster Schritt:** Integration 6 (M7, dock4, Zeitleiste 4, pruefung3, uebersetzung2), danach die Schlussrunde mit
  allen Modi und der Kartenprüfung.
- Für den Menü-Zweig: JK2-Menüs – `^` ohne Ziffer und die Breite mit Farbcodes wie JK2_MODE (Punkt 2, Nebenbefund);
  `#include <cstddef>` in den drei M6-Tests (lint_portability).
- Für den Zeitleisten-Zweig: Fund 3 ändert `gui/selbsttest_zeitleiste1.inc` (`halteTaste` mit `genug` statt
  `probenBis`, Modellprüfung mit Warten) – bei Zeitleiste 4 mitnehmen.
- es/cs: 428 Programmtexte, 172 Handbuchtexte, 32 Bilder (Tabelle oben) – Übersetzung Runde 2.
- Aus Integration 4 unverändert offen: Bilder der Seite zeitleiste in de/zh-Hans/ja (NOTIZEN_zeitleiste1), Stufe 8
  „Notify when a build is done“ allein und g_figurBilder (NOTIZEN_reste1).

### Changelog

#### English

- Jedi Outcast subtitles in the timeline preview no longer draw a ^ that is not followed by a digit, like the game.
- Helper: the way to the camera toolbar now names the timeline row "Camera – position".

#### Deutsch

- Untertitel von Jedi Outcast in der Vorschau der Zeitleiste zeichnen ein ^ ohne folgende Ziffer nicht mehr – wie das Spiel.
- Helfer: der Weg zur Kameraleiste nennt jetzt die Zeile „Kamera – Ort“ der Zeitleiste.

#### 中文

- 时间轴预览中的 Jedi Outcast 字幕不再绘制后面没有数字的 ^,与游戏一致。
- 帮助:通往摄像机工具栏的路径现在使用时间轴行名“摄像机 – 位置”。

#### 日本語

- タイムラインのプレビューで、Jedi Outcast の字幕は数字が続かない ^ を描画しなくなりました(ゲームと同じ)。
- ヘルプ: カメラツールバーへの道順に、タイムラインの行名「カメラ – 位置」を表示します。

## Integration 6

Zweig `karte1` (Worktree `Y:\karte`), Ausgang f5d5b210 (Integration 5). Gemergt: die im Auftrag genannten Spitzen
von reste2, uebersetzung2, zeitleiste1 (Zeitleiste 4), menue6 (M7), dock1 (dock4) und – Zusatz von shank während
der Arbeit – `vis1` bis 69d4ce07 (vis4). Pruefung 4a/4b und die späteren Stände kommen in Integration 7.
Skripte: `SP/int6/` – `bauct.sh` (Bau und/oder ctest unter Idle, Summe `summe.txt`), `selbst.sh` (Selbsttests
nacheinander über `idle.sh`), `ende.sh` (Neubau), `gen.sh` (alle Generatoren), `klick.sh` (Handbuchbild
`c_tl_klick` neu, unten), `loese.py`, `loese_m7a.py`/`loese_m7b.py`/`fix_scene.py` (Konfliktlösungen). Die Merges
2–6 wurden in einem Wegwerf-Klon (`git clone --shared` im SP, danach gelöscht) gelöst, während in `Y:\karte` der
vorige Merge baute; in `Y:\karte` dann je einzeln vorgespult, **gebaut und mit ctest geprüft**. Alle Zeiten unter
Last (andere Agenten bauten und testeten gleichzeitig).

| Commit | Inhalt |
|---|---|
| eceefdb3 | Merge `reste2` (59a2dac1) – keine Konflikte; erzeugte Dateien mit den Generatoren nachgeprüft (gleich). Bau 0 Warnungen (791 s), ctest **183/183** (426 s) |
| 32c6398b | Merge `uebersetzung2` (31be737d) – `tools/gen_i18n.py` (RESTE2 und UEBERSETZUNG2 beide), erzeugte Dateien neu. Bau 0 Warnungen (535 s), ctest **183/183** (505 s) |
| 834cb33b | Merge `zeitleiste1` (0fc0081f) – Konflikte unten. Bau 0 Warnungen (442 s), ctest **184/184** (292 s) |
| 0a9c3e47 | Merge `menue6` (634aa8eb, M7) – Konflikte unten. Bau 0 Warnungen (419 s), ctest 187/188: `untertitel` rot (der bekannte Farbcode-Konflikt, Fund 1) |
| 1468ef6a | Merge `dock1` (dd3eab81, dock4) – Konflikte unten. Bau 0 Warnungen (369 s), ctest 187/188 (`untertitel`, wie vorher) |
| 450866cc | Fund 1: Untertitel von Jedi Outcast wie die Spielfotos, `untertitel::breite` über `schriftBreite(…, Jk2Schrift::Sp)` |
| 30be8cb8 | `MeKompOriginal` als Zähltext mit `trN` (Auftrag) |
| 8047b15e | Fund 2: lint_i18n – die Cvar-Namen der M7-Beispielwerte |
| | Bau + ctest nach 450866cc–8047b15e: 0 Warnungen (102 s), **188/188** (274 s) |
| c809c2f1 | Merge `vis1` (69d4ce07, vis4) – Konflikte nur in den erzeugten Dateien (i18n.h/.cpp, `suche.js`), neu erzeugt. Bau 0 Warnungen (710 s), ctest **188/188** (282 s, darin `q3map2_gpubake` 11,8 s und `karte_gpuwahl` grün) |
| 0ce0164c | Fund 3: Handbuchbild `c_tl_klick` (4 Sprachen) mit dem gemergten Programm neu |
| (dieser) | Notizen |

### Konflikte zeitleiste1 (834cb33b)

- **`ActorState::beineTimerBisMs` zweimal** (`include/bhed/scene.h`, `src/scene.cpp`): pruefung2 (karte1, Todesanimation:
  G_CheckSpecialDeathAnim fragt den Timer der Beine ab) und Zeitleiste 4 (Aufnahme: solange ein Timer läuft, setzt
  PM_Footsteps die Beine nicht neu) haben dasselbe Feld mit derselben Bedeutung (< 0 kein Timer, 1e300 hält immer)
  je eigens angelegt – der Kopf hätte es nach dem Merge zweimal gehabt. Jetzt ein Feld mit beiden Kommentaren,
  gesetzt wie in Zeitleiste 4 (`beineFest ? beineBis : -1.0`: ohne bekannte Animationslänge kein Timer – dann löst
  ein Weg die Animation ab), dazu `torsoTimerBisMs` aus Zeitleiste 4. Der Unterschied betrifft nur Figuren ohne
  Skelett (keine Länge); ctest `pruefung2` und die Zeitleistentests grün, Selbsttests zeitleiste1/mission1 grün.
- `tools/handbuch/LIESMICH.md`: die Sprachen es/cs (uebersetzung2) und die Übungsmission auf taspir1 (Zeitleiste 4) beide.
- `handbuch/*/suche.js` neu erzeugt. Die Cherry-Picks von daf88e15/0a544a42 (8385d9c7, 1e2eda71) liefen ohne Konflikt ein.

### Konflikte menue6 (0a9c3e47)

- **`src/menue/vorschau.cpp`, `include/bhed/menue/vorschau.h`** (Farbcodes von Jedi Outcast): die M7-Fassung nach den
  Spielfotos (jk2sp.exe und jk2mp.exe: jedes Zeichen nach `^` ist eine Farbe, ColorIndex(c) = (c − '0') & 7) gilt –
  die Spielfotos sind maßgebend; karte1 3cae6104 hatte OpenJK JK2_MODE gelesen (nur das `^` fällt weg). Der
  Untertitel-Test folgt in 450866cc (Fund 1).
- `gui/app_menue.cpp`, `gui/menueeditor.h`: die Figurenzeiten von reste2 (`modellMs`, `modellZeichnungen`) und die
  M7-Zähler (`modellStufen`, `beispielPrims`, `platzhalterPrims`, `ladebalken`) beide.
- `gui/karte_bau_win32.cpp`: `arbeitsspeicherMB()` (reste2) und `losgeloestStarten(…, pid)` (M7) beide.
- `tools/handbuch/inhalt.py`: die Seitenliste von karte1 (mit `inhalt_dock`, `inhalt_hilfe2`), `inhalt_menue7` nach `inhalt_menue6`.
- Erzeugte Dateien (i18n.h/.cpp, 624 Handbuchseiten, `suche.js`) mit den Generatoren: 3256 Texte, 195 Orte, 161 Seiten × 6 Sprachen.

### Konflikte dock1 (1468ef6a)

- **`handbuch/bilder/{en,de,zh,ja}/c_tl_klick.webp`**: Zeitleiste 4 zeigt die neue Zeitleiste (ohne „Window“ in der
  Menüleiste), dock4 die Menüleiste mit „Window“ über der alten Zeitleiste (dock1 kannte Zeitleiste 3/4 nicht). Im
  Merge das Bild von Zeitleiste 4 behalten, danach mit dem gemergten Programm neu aufgenommen (Fund 3).
- `handbuch/bilder/de/h_karte8_menue.webp`: karte1 behalten – das Build-Menü mit „Dasselbe VIS in jedem Bau“ (vis1);
  dock4 hatte es ohne vis1 neu aufgenommen, und „Window“ liegt außerhalb des Ausschnitts.
- Erzeugte Seiten und `suche.js` neu.

### Funde aus dem Zusammenspiel (nachgestellt → behoben → grün)

1. **Untertitel von Jedi Outcast** (der bekannte Konflikt). Quellen (OpenJK unter `SP/m3/ojq/src`): CG_DrawCaptionText
   (`codeJK2/cgame/cg_text.cpp:452`) → `cgi_R_Font_DrawString` (`cg_syscalls.cpp:276`) → `CG_R_FONTDRAWSTRING` →
   `re.Font_DrawString` (`code/client/cl_cgame.cpp:979`), die Oberfläche über `uii.R_Font_DrawString = re.Font_DrawString`
   (`cl_ui.cpp:247`) – Untertitel und Menüs gehen durch dieselbe Funktion des Renderers, in jk2sp.exe also so, wie M7
   die Menüs fotografiert hat. Rot: ctest `untertitel` nach Merge 4 und 5 („JK2: ^ ohne Ziffer nicht gezeichnet, ^1
   färbt rot“ – mit M7 zeichnet JK2 „a^^1b“ als a, 1, b in Magenta). Test angeglichen an die Spielfotos: „A^Bx^Cy^0z“ –
   JK2 zeichnet A, x grün, y gelb, z schwarz; JKA/MD zeichnen das `^` ohne Ziffer, `^0` schwarz; die Glyphen tragen im
   Test ihr Zeichen (s), so prüft er, *welche* Zeichen gezeichnet sind. Breite: JKA 80, JK2 100 (jk2sp.exe zählt jedes
   Zeichen). `untertitel::breite` nutzt jetzt `schriftBreite(…, Jk2Schrift::Sp)` statt einer eigenen Schleife – die
   hatte dabei das Ersatzmaß von GetLetterHorizAdvance nicht (ein Zeichen ohne Vorschub zählt wie '.', auch unter
   JK2_MODE in RE_Font_StrLenPixels). Rot mit dem neuen Test und dem alten `breite`: „Breite: Zeichen ohne Vorschub
   zählt wie '.' (JKA und JK2)“ (1 Fehler, die Zeichenprüfung schon grün durch M7); grün mit 450866cc: „alles gut“.
2. **lint_i18n** nach Merge 4: 2 Beanstandungen – `ImGui::Checkbox("g_jediVmerc")`, `("g_forceBasedTeams")` in
   `gui/app_menue_m7.inc` (Beispielwerte der MP-Ownerdraws). M7 hatte noch einen lint_i18n, der `gui/*.inc` nicht
   las. Es sind Cvar-Namen wie in der Konsole des Spiels – in die Ausnahmen wie die .menu-Schlüsselwörter. Grün: „i18n-Prüfung: ok“.
3. **Handbuchbild `c_tl_klick`** (Merge-Konflikt dock1/Zeitleiste 4): beide Absichten – neue Zeitleiste und Menüleiste
   mit „Window“ – zeigt nur ein Bild des gemergten Programms. Von den 22 Bildern, die Zeitleiste 4 neu aufgenommen hat,
   reicht nur dieses in die Menüleiste (Ausschnitt `0 0 …`, die anderen beginnen bei y ≥ 90; Regel wie
   `SP/d4_hb_auswahl.py`). Bildlauf `BHED_EDITORTEST=handbuch` bis zu diesem Foto (`BHED_HB_BIS=c_tl_klick`, Mission und
   Archiv auf X: wie `/y/zl4_hbvoll.sh`), en / de / zh-Hans / ja je **4 OK, 0 FEHLER** (209–212 s), mit
   `tools/handbuch/bilder.py` umgewandelt. Durchgesehen: neue Zeitleiste (Übersicht/Detail, „walks 1“ mit Pfeil) und das
   Fenster-Menü in der Menüleiste (en „Window“, de „Fenster“, ja „ウィンドウ“); die übrigen Handbuchseiten ändern sich nicht.

Nebenbefund ohne Änderung: der Selbsttest stufe8g, den vis4 mit 3 FEHLERN meldete („Pack map as .pk3“ 1–2 Pixel außerhalb
des Fensters), ist auf dem gemergten Stand grün (**43 OK, 0 FEHLER**, der Klick auf „Pack map as .pk3##baupk3“ und die
Prüfung der .pk3 gehen durch) – reste2 bricht hohe Menüs wie NRC in Spalten um (b49c49e2, 0a7587a1).

**Auftrag `MeKompOriginal`** (30be8cb8): M7 hatte den Text ohne Mehrzahl formuliert („copies: %d“, trN gab es im Zweig
noch nicht); jetzt „(%d {copy|copies})“ / „(%d {Kopie|Kopien})“ über `trN`. ctest `sprache` (jeder Zähltext jeder
Sprache für 0/1/2/3/5/21) grün, lint_i18n ok.

### Prüfungen (unter Last)

| Schritt | Ergebnis |
|---|---|
| Bau je Merge (inkrementell, /W4 /WX) | **0 Warnungen** (Tabelle oben) |
| ctest je Merge | 183/183, 183/183, 184/184, 187/188 (`untertitel`, Fund 1), 187/188, nach den Funden 188/188, nach vis1 188/188 |
| Sauberer Neubau (`--clean-first`, alle Ziele, 179 Projekte) auf c809c2f1 | **0 Fehler, 0 Warnungen** (689 s) |
| ctest gesamt danach | **188/188** (283 s) |
| lint_sources, lint_fassung (rc606), lint_i18n (0 Beanstandungen), lint_i18n_erzeugt, lint_gapp, lint_imgui_context, lint_layout, lint_tabellenid, lint_hoehe, lint_feldinit, lint_abstand, lint_doppelanzeige, lint_headers, lint_msvclink, lint_merkmalsbits | ok |
| übrige Lints | wie Integration 5 (efxflags, endtable 2, fremdprogramm, mapstate, pruefcodes 11, settings 1); lint_portability 236 → **232** (M7 hat `<cstddef>` nachgezogen); **lint_d3dbesitz 20 → 21**: „Tabelle g_figurBilder wird in shutdown() nicht geleert“ – kommt aus reste2 (auf 59a2dac1 allein ebenso), Falschmeldung: `shutdown()` ruft `figurBilderAllesLoesen()`, das freigibt und leert; der Prüfer liest nur den Rumpf von `shutdown()` |
| lint_i18n_erzeugt (Hinweis aus vis4: lief dort nicht durch) | hier ok – im Wegwerf-Klon und in `Y:\karte` („i18n-Erzeugungsprüfung: ok“) |
| Handbuch-Bau (`gen_handbuch.py`), gen_i18n, gen_hilfe_orte | 161 Seiten × 6 Sprachen, 3259 Texte, 195 Orte – ohne Änderung gegenüber dem Commit |
| menueeditor, Movie-Duels-Daten / JKA-Daten („GameData original“, nur gelesen) | **134 OK, 0 FEHLER** / **134 OK, 0 FEHLER**; GameData original danach unverändert (keine Datei neuer als der Start) |
| zeitleiste1 / zeitleiste | **87 OK, 0 FEHLER** / **47 OK, 0 FEHLER** |
| dock1 / dock4 / dock2mass (DockSpace ab Werk an) | **97 / 106 / 29 OK**, je 0 FEHLER (dock4 ohne `BHED_DOCK4_EXTERN`: die Ablage aus einem zweiten Prozess – im Zweig 107 – läuft nicht mit) |
| karteneditor kurz + Fotos (`BHED_KARTE_NUR=fotos`, `BHED_FOTOS`) | **78 OK, 0 FEHLER** |
| karteneditor `stufe8` / `stufe8g` / `stufe9d` / `hilfe2` | **40 / 43 / 28 / 44 OK**, je 0 FEHLER |
| modellleistung | **10 OK, 0 FEHLER** |
| mission1 (`BHED_FOTOS`) | **98 OK, 0 FEHLER** (440 s) |
| Modus 1 (en, DockSpace an – „DockSpace von Bereich 0/1/2 aufgebaut“, `BHED_HANDBUCH`) | **2650 OK, 0 FEHLER** (441 s), 0 Zusicherungen |

Die Selbsttests und Modus 1 liefen auf dem sauberen Neubau von c809c2f1 (0ce0164c ändert nur Bilder) (Einstellungen `behaved.cfg.sicher`, nur eigener Ordner).

### Was auf Spanisch/Tschechisch noch englisch ist (für Übersetzung Runde 3)

| | es | cs |
|---|---|---|
| Programmtexte (`gen_i18n.py --fehlend`) | **49** | **49** |
| davon menue6 (M7) / reste2 / vis1 (vis4) | 33 / 13 / 3 | 33 / 13 / 3 |
| Handbuchtexte (`gen_handbuch.py --pflicht=es/cs`) | **44** | **44** |
| Handbuchbilder englisch | **5** (`h_dock_ecke`; M7: `menue_datapad`, `menue_gizmo`, `menue_komponente`, `menue_ladebild`) | **5** |

Handbuchtexte: vor allem die von dock4 neu geschriebene Seite „Fenster und Bildschirme anordnen“, dazu M7 (Seite
`menue_editor_m7`), reste2 (hohe Menüs in Spalten, Decals), Zeitleiste 4 (Aufnahme) und vis4. `gen_handbuch.py`:
„30 Einträge in uebersetzung/ werden nicht mehr gebraucht“ – es/cs-Übersetzungen zu Sätzen, deren englischen
Wortlaut dock4 und Zeitleiste 4 geändert haben. Nicht gezählt (der Generator sieht es nicht): es/cs-Bilder mit
Menüleiste ohne „Window“ (dock4: 13) und es/cs-Bilder, die noch die alte Zeitleiste zeigen (Zeitleiste 3/4 hat nur
en/de/zh/ja neu aufgenommen).

### Gegen das Vorbild geprüft / nicht geprüft

- Fund 1 gegen die Quellen (Weg CG_DrawCaptionText → re.Font_DrawString in JK2, wie oben; RE_Font_StrLenPixels unter
  JK2_MODE mit GetLetterHorizAdvance) und gegen die Spielfotos von M7 (jk2sp.exe/jk2mp.exe, `SP\m7\caret`) – im
  Programm über ctest, kein eigener Spielstart (Untertitel von JK2 nicht im Spiel fotografiert).
- `beineTimerBisMs`: beide Bedeutungen gegen ihre Herkunft (pruefung2: G_CheckSpecialDeathAnim; Zeitleiste 4:
  PM_SetAnimFinal/bodyPlay) – über ctest `pruefung2` und die Selbsttests zeitleiste1/mission1.
- vis4: bitgleich laut Zweig; hier `q3map2_gpubake` und `karte_gpuwahl` (ctest), stufe8g, stufe9d grün; kein eigener
  Bytevergleich großer Karten.
- Nicht geprüft (Auftrag): alle Modi, Kartenprüfung (Schlussrunde), Tempo (alles unter Last).

### Offen / nächster Schritt

- **Nächster Schritt:** Integration 7 (Pruefung 4a/4b, vis4-Rest), danach die Schlussrunde mit allen Modi und der Kartenprüfung.
- lint_d3dbesitz: Falschmeldung `g_figurBilder` (reste2) – der Prüfer sollte die Hilfsfunktionen aus `shutdown()`
  mitlesen oder die Freigabe in `shutdown()` stehen.
- stufe8g: auf diesem Stand grün; der Abschluss-Agent sollte es im vis-Zweig nach dem nächsten Merge von karte1 noch
  einmal sehen (dort 3 FEHLER ohne reste2).
- es/cs: 49 Programmtexte, 44 Handbuchtexte, 5 Bilder (Tabelle oben), dazu die es/cs-Bilder ohne „Window“ bzw. mit der alten Zeitleiste – Übersetzung Runde 3.
- Unverändert aus Integration 5: Bilder der Seite zeitleiste in de/zh/ja (soweit nicht von Zeitleiste 4 erneuert),
  18 Tutorial-Schritte ohne Bild. Der überlange MD-Schlüssel im Einstellungs-Assistenten (uebersetzung2) hat M7 mit der
  Tabelle gelöst (57ab5264).

### Changelog

#### English

- Jedi Outcast subtitles in the timeline preview treat every character after ^ as a colour code, as the game does (checked with photos of jk2sp.exe).
- Menu editor: the original of a component names its copies with the right plural.

#### Deutsch

- Untertitel von Jedi Outcast in der Vorschau der Zeitleiste nehmen jedes Zeichen nach ^ als Farbcode – wie das Spiel (mit Fotos aus jk2sp.exe geprüft).
- Menü-Editor: das Original einer Komponente nennt seine Kopien in der richtigen Mehrzahl.

#### 中文

- 时间轴预览中的 Jedi Outcast 字幕把 ^ 后面的每个字符都当作颜色代码,与游戏一致(用 jk2sp.exe 的截图核对)。
- 菜单编辑器:组件原件以正确的单复数显示其副本数量。

#### 日本語

- タイムラインのプレビューで、Jedi Outcast の字幕は ^ の後の文字をすべて色コードとして扱います(ゲームと同じ。jk2sp.exe の画面で確認)。
- メニューエディター: コンポーネントの原本が、コピーの数を正しい単複形で表示します。
