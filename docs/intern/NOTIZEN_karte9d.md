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
