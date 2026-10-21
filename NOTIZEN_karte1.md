# Karten-Editor, Stufe 1a: der Kern ohne Oberflaeche

Stand: Zweig `main` in Y:\karte, Commits `17e790c` bis zum Notiz-Commit (Liste
unten). Portiert 1:1 aus NetRadiant-Custom (NRC, Stand master 2026, Quelltext
in `SP\nrc_src\netradiant-custom`), Vergleichs-Compiler q3map2 aus NRC
20260114 (`SP\nrc_dl\nrc\q3map2.exe`). Alle Quellangaben in den Kommentaren
haben die Form `Funktion (datei:zeile)` bzw. `(:zeile)`, wenn die Datei im
Dateikopf genannt ist.

`SP` = `C:\Users\shank\AppData\Local\Temp\claude\c--Users-shank-Downloads-behaved-1-0-0-rc567-behaved\af9c6614-e078-4efa-a574-1fa901786464\scratchpad`

## Commits

| Commit | Punkt | Inhalt |
|---|---|---|
| `17e790c` | 1 | GPL v2: LICENSE, README-Abschnitt, Herkunftskoepfe, ctest `karte_lizenz` |
| `7377efd` | 2a | Mathe, Wicklungen, Pinsel (buildWindings/buildBRep), Undo, ctest `karte_geometrie` |
| `cf6f49e` | 2b | Texturprojektion AP / BP / Valve 220, ctest `karte_texdef` |
| `085af2b` | 2c | Patches patchDef2/3 mit NRC-Tesselierung, ctest `karte_patch` |
| `447c328` | 2d | Entitaeten, Kartenbaum, Ebenen, Auswahl, Undo, ctest `karte_entitaet` |
| `f345299` | 2e | .map lesen/schreiben bytegleich wie NRC, ctest `karte_rundlauf` |
| `beeba5a` | 2f | Entitaetsklassen .def/.ent, Defs aus OpenJK, ctest `karte_klassen` |
| `bfd2121` | 2g | Spiel-Dateisystem und Textur-Inventar, ctest `karte_texturen` |
| `79ec056` | 3a | Geometrie und st gegen das echte q3map2, ctest `karte_vergleich` |
| (dieser) | 3c, 4 | Massenlast ueber alle dekompilierten Original-BSPs, ctest `karte_massenlast`, `tools/karte_dekomp.py`, `tools/karte_pinsel_exakt.py`; diese Notizen |

## Was portiert ist (NRC-Datei:Zeile -> unsere Datei)

Modul `bhed::karte`, Quellen in `src/karte/`, Kopfdateien in
`include/bhed/karte/`, alles reine Logik ohne Oberflaeche, gebaut in
`bhed_core`.

| NRC | unsere Datei |
|---|---|
| libs/math/vector.h:35-575, plane.h, line.h, matrix.h, aabb.h, pi.h, quaternion.h:82-176, libs/generic/vector.h | `include/bhed/karte/mathe.h` (gleiche float/double-Mischung: linker Operand bestimmt den Typ, `vector3_dot` in double, Matrix4 float spaltenweise) |
| radiant/winding.h:207-217, winding.cpp:126-382 (Winding_createInfinite #else-Zweig, Winding_Clip, ClassifyPlane, FindAdjacent, Opposite, Centroid) | `wicklung.h`, `src/karte/wicklung.cpp` |
| radiant/brush.h:595-2523 (FacePlane, FaceShader, FaceTexdef, Face, Brush: addPlane/addFace/erase/removeEmptyFaces, plane_unique:2331, windingForClipPlane:2125, removeDegenerateEdges:2343, removeDegenerateFaces:2371, removeDuplicateEdges:2410, verifyConnectivityGraph:2436, buildWindings:2477, contributes:2233), radiant/brush.cpp:98-332 (buildBRep), radiant/brushmanip.cpp:37-74 (Brush_ConstructCuboid), libs/render.h:152-270 (UniqueVertexBuffer) | `pinsel.h`, `src/karte/pinsel.cpp` |
| radiant/brush_primit.h:29-187 (brushprimit_texdef_t, TextureProjection, ComputeAxisBase), radiant/brush_primit.cpp:43-2174 (Texdef_toTransform/fromTransform, Texdef_EmitTextureCoordinates:274, Texdef_Assign/Shift/Scale/Rotate, BPTexdef_*:1043-1136, TexMatToFakeTexCoords:629, FakeTexCoordsToTexMat:678, Texdef_FitTexture:1204, Texdef_transformLocked, Texdef_ProjectTexture, Texdef_Convert, Texdef_from_ST, BP_from_ST) | `texdef.h`, `src/karte/texdef.cpp` |
| radiant/undo.cpp:160-339 (UndoStack, RadiantUndoSystem start/finish/undo/redo/clear), include/iundo.h, libs/undolib.h | `rueckgaengig.h`, `src/karte/rueckgaengig.cpp` |
| radiant/patch.h:114-961 (BezierCurve, Kontrollpunkte, SetShader:825, transform:650, snapto:695, importState:937, Tokens:1010-1261), radiant/patch.cpp:63-2867 (BezierCurveTree, setDims:156, isValid:196, InvertMatrix:292, TransposeMatrix:300, TesselateSubMatrixFixed:1815, TesselateSubMatrix:1900, BuildTesselationCurves:2244, BuildVertexArray:2578), libs/render.h:1170-1240 | `patch.h`, `src/karte/patch.cpp` |
| libs/entitylib.h:618-628 (EntityKeyValues::setKeyValue), plugins/entity/entity.cpp:88-110 (node_for_eclass), libs/traverselib.h (TraversableNodeSet mit Undo), libs/layers.h, radiant/map.cpp:492-554 (Map_FindWorldspawn, createWorldspawn), radiant/selection.cpp / include/iselection.h (Auswahl Primitive/Entitaeten/Flaechen) | `karte.h`, `src/karte/karte.cpp` |
| libs/script/scripttokeniser.h, scripttokenwriter.h, libs/stream/textstream.h:254-282 (Decimal), libs/stringio.h, plugins/mapq3/parse.cpp:37-241 (LayersParser, Entity_parseTokens, Map_Read), plugins/mapq3/write.cpp:42-50 (Layer_Write, Map_Write), plugins/mapq3/plugin.cpp:279-325 (Formaterkennung), radiant/brushtokens.h:507-653, radiant/patch.h:1010-1261, radiant/referencecache.cpp:122-140 (MapResource_save) | `kartenformat.h`, `src/karte/kartenformat.cpp`, `src/karte/tokenizer.h` |
| libs/eclasslib.h:126-320, radiant/eclass.cpp:70-230 (Liste, Eclass_ForName, EntityClassQuake3_Construct), radiant/eclass_def.cpp:265-382 (Eclass_ScanFile, QUAKED, Eclass_InitFromText), radiant/eclass_xml.cpp:520-532 (.ent), libs/stringio.h:203-218 | `eklasse.h`, `src/karte/eklasse.cpp`, `src/karte/eklasse_lesen.cpp` |
| plugins/vfspk3/vfs.cpp:150-286 (InitDirectory, GetListInternal), radiant/qe3.cpp (QE_InitVFS), plugins/shaders/shaders.cpp:1157-1687 (parseQuake3, ParseShaderFile, Shaders_Load), libs/shaderlib.h:46-55, include/ishaders.h:28-41, radiant/texwindow.cpp:589-1650 (texture_name_ignore, ShowDirectory, TextureGroups, notex 64x64), radiant/image.cpp:36-60 | `texturen.h`, `src/karte/texturen.cpp` |
| OpenJK code/game, codemp/game, codeJK2/game (QUAKED-Bloecke, GPL v2) | `tools/gen_karten_def.py` -> `data/karte/ja/sp_entities.def` (242 Klassen), `data/karte/ja/mp_entities.def` (228), `data/karte/jk2/sp_entities.def` (195) |

Schnittstellen fuer Stufe 1b:
- Jede Aenderung an Entitaet, Pinsel, Flaeche, Patch und Entitaetenliste
  sichert sich selbst im Undo (`Rueckgaengigbar::sichern`, einmal je
  Befehl wie `UndoStack::save`), sobald das Objekt in der Karte angemeldet
  ist. Befehle klammert `Befehl b(karte.undo, "Name")` (RAII, wie
  `UndoableCommand`).
- Auswahl (`Karte::auswahl`) auf Entitaeten-, Primitiv- und Flaechenebene,
  in Auswahlreihenfolge, mit Modi Primitive/Entitaeten/Flaechen/Kanten/Ecken
  (`SelectionSystem::EMode`/`EComponentMode`).
- Texturoperationen als Kernoperationen an `Flaeche` (verschieben, skalieren,
  drehen, einpassen, projizieren, umrechnen, Texture Lock beim
  Transformieren), Patch-Matrix-Operationen an `Patch`, Transformationen an
  `Pinsel`/`Patch`.
- Pinsel-Kanten, -Ecken, Kanten-Flaechen-Zuordnung und Grenzen aus
  `buildBRep` fuer die Komponenten-Auswahl.

## Tests und Rot/Gruen-Nachweis

Je Punkt ein gezielter Test, erst rot gesehen (fehlender Code oder
absichtliche Mutation), dann gruen; danach `ctest` ueber alles. Der einzige
rote ctest ausserhalb des Moduls ist `tour` (fehlende erzeugte
`handbuch/*/suche.js` im Arbeitsbaum Y:\karte, unabhaengig von dieser Stufe).

| Punkt | ctest | rot | gruen |
|---|---|---|---|
| 1 | `karte_lizenz` | vor LICENSE/README: 11 Fehlschlaege (LICENSE fehlt, README ohne Lizenzabschnitt, ...) | 0; prueft auch den Herkunftskopf jeder Datei in src/karte und include/bhed/karte |
| 2a | `karte_geometrie` | Mutation `plane_unique` (`!selfIsLater`): "doppelte Ebene: Flaeche 0 bleibt, Flaeche 6 nicht" | 0 |
| 2b | `karte_texdef` | Mutation `Texdef_toTransform` (Vorzeichen shift t): 6 Fehlschlaege (AP-Projektion, Umrechnungen, Texture Lock) | 0 |
| 2c | `karte_patch` | Mutation `BezierCurve_IsCurved` (Winkel ohne /pi): 7 Fehlschlaege (Viertelkreis 17 statt 9 Spalten, ...) | 0 |
| 2d | `karte_entitaet` | Mutation `node_for_eclass` ohne classname: 4 Fehlschlaege | 0 |
| 2e | `karte_rundlauf` | Mutation Trenner des SimpleTokenWriter: 6 Fehlschlaege (alle bytegleichen Rundlaeufe) | 0 |
| 2f | `karte_klassen` | Mutation Flag-Slot `x` -> `y` in Eclass_InitFromText: "func_door: Spawnflags nach Bit" | 0 |
| 2g | `karte_texturen` | Mutation Paketreihenfolge (aufsteigend statt absteigend): 2 Fehlschlaege | 0 |
| 3a | `karte_vergleich` | Mutation `ComputeAxisBase` ohne `negieren(texS)`: BP-st weicht bis 3.12 ab | 0 |
| 3c | `karte_massenlast` | Mutation Decimal `%10.2lf` statt `%10.10lf`: hoth2/hoth3/siege_hoth je Format rot (Strukturen und zweites Schreiben) | 0 ueber alle 672 Karten |

### Rundlauf (Punkt 3, bytegleich)

- `tests/karte/oa_fan.map` (OpenArena, GPL v2, von NRC geschrieben, 44
  Patches) und `tests/karte/nrc_tiny_structural_brush.map` (NRC-Regressions-
  test, CR LF, Brush Primitives): lesen -> schreiben bytegleich.
- 52 von NRC geschriebene Karten aus oeffentlichen Repos (`SP\dl_nrc_maps`,
  nur lokal): 50 bytegleich. Die zwei anderen sind erklaert:
  `Q3Rally ... q3r_ctf4_01.map` enthaelt `//@$& layer -1011176789` ohne
  passende `layerdef`; NRC selbst meldet "primitive layer index out of
  bounds" und legt den Pinsel in Ebene 0 (parse.cpp), schreibt die Zeile also
  ebenfalls nicht mehr. `quakeshack ... hw_lights.map` ist eine Quake-1-Karte
  (Valve 220 ohne Flag-Spalten); im Quake3-Format schreibt NRC wie wir die
  Flags `0 0 0` dazu.
- Ravens JA-Beispielkarten von icculus (`SP\dl_ja_examples`, 4 SP + 4 MP,
  Quake3-Texdef ohne NRC-Kopf): alle 8 ohne Fehler gelesen, im NRC-Format
  geschrieben (Ebene "0", `// entity N`/`// brush N`, Decimal-Zahlen), erneut
  gelesen und geschrieben bytegleich (stabil).
- Aufruf mit den lokalen Ordnern:
  `karterundlauf tests/karte <NRC-Karten> <fremde Karten>` bzw.
  `-DBEHAVED_KARTEN_NRC=... -DBEHAVED_KARTEN_FREMD=...`.

### Geometrie und st gegen q3map2 (Punkt 3)

`tools/gen_karte_vergleich.py` erzeugt `tests/karte/vergleich_q3.map` und
`vergleich_bp.map` (Raum, Wuerfel mit anderer Verschiebung/Skalierung/Drehung
je Flaeche inkl. negativer Skalierung, Keil mit zwei schraegen Ebenen,
gebogener Patch). q3map2 `-game ja` ohne `-meta` hat daraus die BSPs daneben
gebaut. Ergebnis je Karte (14 Flaechen, 1 Patch):

- xyz: roh hoechstens 1/24 entfernt, nach q3map2s Einrasten auf 1/8
  (`g_brushSnap`, surface.cpp:695-697) exakt gleich.
- st am eingerasteten Punkt mit dem Flaechen-Bias aus
  `CalcSurfaceTextureBias`: Quake3-Texdef hoechstens 2.38e-7, Brush
  Primitives hoechstens 4.77e-7.
- Patch-Kontrollpunkte exakt gleich.
- Toleranz 1e-5, hergeleitet aus q3map2 (tests/kartevergleich.cpp): q3map2
  rechnet st am eingerasteten Punkt in float (texMat bzw. vecs, x, y float)
  und zieht je Flaeche den Bias ab (CalcSurfaceTextureBias,
  surface.cpp:228-245); drei float-Rundungen bei |st| < 32 (~5.7e-6) plus die
  float-Rundung von x, y (2^-14 * |Matrix| <= 2.4e-6) ergeben < 1e-5. Die
  xyz-Schranke roh ist 1/16 plus 2^-14 (float bei |x| <= 528).

### Massenlast (Punkt 3)

Daten (nur unter `SP\karte_dekomp`, nicht im Repo): `tools/karte_dekomp.py`
holt alle `maps/*.bsp` aus den .pk3 und ruft q3map2 (NRC 20260114) mit
`-game ja|jk2 -fs_basepath <GameData>/ [-fs_game MD] -convert -format map_bp`
bzw. `-format map` auf.

- JA: 61 BSPs = 57 Karten + die 4 aelteren Fassungen von mp/ctf1, ctf2, ctf4,
  ctf5 aus assets0.pk3 (von assets3.pk3 verdeckt, trotzdem geprueft);
  JK2: 45; Movie Duels: 230 (aus `MD/*.pk3`). Zusammen 336 BSPs, 672 Karten,
  q3map2 ohne einen Fehler; Dauer 70 s (JA), 42 s (JK2), 544 s (MD).

Ergebnis `kartemassenlast SP\karte_dekomp` (je Format bp und q3 gleich):

| Spiel | Karten | Entitaeten | Pinsel | Flaechen | Patches | ohne gueltige Wicklung | Zeit bp / q3 |
|---|---|---|---|---|---|---|---|
| JA | 61 | 38024 | 303824 | 1809739 | 10620 | 3 | 24.4 s / 22.0 s |
| JK2 | 45 | 50432 | 148660 | 891135 | 10735 | 0 | 11.3 s / 10.6 s |
| MD | 230 | 60480 | 830227 | 4748428 | 163069 | 1478 | 77.5 s / 72.7 s |

- Alle 672 Karten lesen ohne eine einzige Meldung (ausser der
  Formaterkennung), schreiben -> lesen ergibt dieselben Strukturen
  (Entitaeten mit Schluesseln in Reihenfolge, Pinsel mit allen beitragenden
  Flaechen samt Ebenenpunkten, Shader, Texdef/BP und Flags, Patches mit allen
  Kontrollpunkten). Ein Patch in `duel_jedhacity` hat von q3map2 `nan` als st;
  das wird wie bei NRC als `nan` gelesen und geschrieben.
- Zweites Schreiben bytegleich in 618 Karten. In 54 Karten (je Format 2 JA,
  25 MD) gibt es Pinsel ohne beitragende Flaeche: NRC schreibt fuer sie nur
  `// brush N`, beim naechsten Lesen fallen sie weg und die Nummern ruecken
  nach; das dritte Schreiben ist dann gleich dem zweiten.
- Laufzeit fuer alles (lesen, buildBRep aller Pinsel, schreiben, erneut lesen,
  vergleichen, bei leeren Pinseln ein drittes Mal): 218.5 s, hoechster
  Arbeitsspeicher 654 MB (Release, eine Karte nach der anderen).
- **Pinsel ohne gueltige Wicklung**: 1481 je Format von 1282711 (JA 3, JK2 0,
  MD 1478; ungueltig = entartet, weniger als 4 beitragende Flaechen oder
  Euler-Fehler). `tools/karte_pinsel_exakt.py` rechnet sie mit Bruchzahlen
  nach (die Ebenenpunkte stehen mit drei Nachkommastellen in der Karte, also
  exakt): alle 1481 sind erklaert, keiner bleibt uebrig.
  - 1405 leer, 4 flach, 10 offen: so von q3map2 geschrieben. Ursache ist die
    Ausgabe mit drei Nachkommastellen: fast kollineare Ebenenpunkte (z. B.
    `( 4175 4610 69 ) ( 4216.941 4710.859 69 ) ( 4217 4711 69 )`) drehen die
    Ebene um, aus der Unterseite wird eine zweite Oberseite.
  - 31 "Nadeln": zwei Flaechen mit fast gegenueberliegender Normale (bis
    0.0001) und Abstand unter 0.02 gelten in NRC als `plane3_opposing`
    (plane.h:123), `windingForClipPlane` schneidet nicht mit ihnen, die
    Wicklung reicht bis an den Rand -> `isBounded` falsch.
  - 30 mit zwei Ebenen, deren Normalen bis 0.001 gleich sind
    (`plane3_inside`, brush.h:1445): die weitere faellt weg, der Rest ist
    flach (19) oder reicht ueber die Weltgrenze 65536 (11).
  - 1 Pinsel (mb2_manaan) mit einer Kante von 0.0003: Wicklungen da, nur
    "Final B-Rep: inconsistent vertex count" wie in NRC (brush.cpp:258); NRC
    zeigt ihn.
  - Gegenprobe: eine Stichprobe von 2562 gueltigen Pinseln (jeder 500.) ist
    exakt beschraenkt, hat Volumen und dieselbe Zahl beitragender Flaechen.
  Weil unser Code NRC 1:1 folgt, verhaelt sich NRC bei diesen Pinseln gleich
  (die Pfade wurden mit einer voruebergehenden Ablaufverfolgung in
  `buildBRep` bestaetigt und danach wieder entfernt).
- Aufruf: `kartemassenlast <wurzel>` bzw. `-DBEHAVED_KARTE_DEKOMP=<wurzel>`;
  `kartemassenlast <wurzel> <Namensteil>` zeigt Einzelheiten zu einer Karte,
  `kartemassenlast <wurzel> "*" > liste.txt` liefert die Liste fuer
  `python -I tools/karte_pinsel_exakt.py <wurzel> liste.txt`.

## Entscheidungen

- **Lizenz.** Das ganze Projekt steht jetzt unter GPL v2 (LICENSE = NRC
  `GPL`, 18011 Bytes). Ausgenommen sind die Raven/Activision-Daten unter
  `data/`. Jede portierte Datei traegt die Originalzeilen
  "Copyright (C) 1999-2006 Id Software, Inc. and contributors" bzw. die
  OpenJK-Zeilen.
- **Gamepack.** NRCs JediAcademyPack steht in NRCs eigenem
  `download-gamepacks.sh` als "proprietary". Deshalb erzeugt
  `tools/gen_karten_def.py` die .def aus den QUAKED-Bloecken von OpenJK (GPL
  v2). Abweichungen zu NRCs Paket: JA SP fehlt nur die Compiler-Entitaet
  `_decal`; JK2 SP hat statt `NPC_spawner` den OpenJK-Block `NPC_*****`
  (OpenJK hat fuer JK2 keinen eigenen `NPC_spawner`-Block); NRCs JA-MP-.ent
  hat 24 Mod-Klassen mehr (`_skybox`, `fx_weather`, `target_screenshake`, ...)
  und bei den gemeinsamen 224 Klassen 25 mit anderen Spawnflags und 1 mit
  anderem Punkt/Gruppe-Typ; wir haben 4 Klassen, die NRC
  nicht hat (`fx_wind`, `item_*****`, `misc_model_breakable`,
  `npc_human_merc`). Movie Duels = JA-SP-Klassen + 12 Klassen aus
  `include/bhed/entdefs.h`.
- **Zahlenformat.** Wie NRC unter Windows: `Decimal` = `%10.10lf` in einem
  22-Byte-Puffer (also hoechstens 21 Zeichen), Nullen am Ende weg, `-0`
  bleibt; Dateien im msvcrt-Textmodus (Schreiben LF -> CR LF, Lesen
  CR LF -> LF, Strg+Z beendet die Datei).
- **patchDef3** ist wie in NRC nur in den Doom3-Formaten erlaubt; im
  Quake3-Format meldet der Leser einen Fehler wie NRC. Die Doom3-Token-
  Funktionen sind trotzdem da.
- **Shaderliste.** NRC kopiert eine Standard-`shaderlist.txt` in den
  Spielordner, wenn keine da ist (`shaderlist_findOrInstall`). Wir schreiben
  nie in Spielordner; die Standardliste kann als Parameter uebergeben werden.
  Fehlt ein Bild, ist die Textur wie bei NRC 64x64 (notex).
- **Texdef-Format** wird jeder Funktion mitgegeben statt in NRCs globaler
  `g_bp_globals` gehalten; Wirkung gleich, kein versteckter Zustand.
- **Standard-Texturskalierung**: Grundwert wie NRC 0.5
  (`g_texdef_default_scale`); die JA/JK2-`.game` setzen `default_scale` 0.25.
  Stufe 1b muss beim Laden des Spiels `setzeStandardTexturSkalierung(0.25f)`
  aufrufen (NRC liest den Wert aus der .game-Datei).
- **Undo einer Flaeche** schreibt die gesicherte Projektion direkt zurueck.
  NRC setzt im `importState` (brush.h:1042-1052) erst den Shader (mit
  BP-Umrechnung) und dann die Projektion per `Texdef_Assign`; der Endstand
  ist derselbe.
- **acos** in `TexMatToFakeTexCoords` ausdruecklich in double (NRC ruft `acos`
  ohne `std::` mit einem float, das ist die double-Fassung aus der C-Bibliothek).
- **Namen** deutsch, NRC-Namen stehen im Kommentar daneben.
- **Testdaten.** Im Repo nur kleine GPL-Karten und die zwei q3map2-
  Vergleichs-BSPs; NRC-Karten aus dem Netz, icculus-Beispiele und die
  dekompilierten Original-BSPs bleiben unter SP. Die Tests mit diesen Ordnern
  laufen nur, wenn die CMake-Variablen gesetzt sind.

## Offene Punkte

### Fuer Stufe 1b (Oberflaeche)

- Layout wie NRC, 2D/3D-Ansichten, Navigation, Auswahl mit der Maus (die
  Auswahl-Datenstruktur ist da, das Treffen per Strahl/Rechteck noch nicht:
  NRC `SelectionTest`/`Selector` aus radiant/selection.cpp).
- Pinsel aufziehen (`Brush_ConstructCuboid` ist da) und verschieben
  (`Pinsel::transformieren` mit Texture Lock ist da).
- Texturbrowser: `Texturinventar::verzeichnisse()`/`verzeichnis()` liefern
  Gruppen und Inhalte wie NRC, `groesse()` die Bildgroesse; Bilder laden
  ueber `image.h`.
- Entity-Inspector: Klassen mit Kommentar, Flag-Namen und Attributen aus
  `EntitaetsKlassen`; Schluessel setzen ueber `Entitaet::setzen` (mit Undo).
- Hotkeys, Undo/Redo-Menue (`Rueckgaengig::undoName/redoName`).
- `Karte::kontext.texturen` auf das Inventar des gewaehlten Spiels setzen,
  dann stimmen st und BP-Umrechnung mit den echten Texturgroessen.
- Die .def-Dateien unter `data/karte` muessen ins Release (Paket-Skript) bzw.
  eingebettet werden.

### NRC vollstaendig: Checkliste und fehlende Kern-Bausteine

shank will jede NRC-Funktion nachgebaut haben: alle Menuepunkte,
Werkzeugknoepfe, Dialoge, Einstellungsseiten und die mitgelieferten Plugins.
Die Checkliste aus dem NRC-Quelltext liegt in
`SP\nrc_inventar.md` (237 Menuepunkte, 67 Werkzeugknoepfe, 302 registrierte
Befehle, 24 Dialoge, 18 Einstellungsseiten, Plugins bobToolz, prtview,
brushexport, meshtex, shaderplug, sunplug). Sie sollte vor Stufe 1b ins Repo
kopiert und dort abgehakt werden.

Der Kern ist so gebaut, dass diese Funktionen als Kernoperationen mit Undo
darauf sitzen (nicht in der Oberflaeche): jede Operation ist eine Folge von
Aenderungen an angemeldeten Objekten innerhalb eines `Befehl`, die
Bausteine Ebene/Wicklung/Pinsel-Kopie/Flaeche hinzufuegen/entfernen,
Texturprojektion und Patch-Kontrollpunkte sind vorhanden. Noch zu portieren
(je mit NRC-Quelle):

- **CSG** (radiant/csg.cpp): Subtract (`Brush_subtract`:545, `CSG_Subtract`:710),
  Merge (`Brush_merge`:840, `CSG_Merge`:937), Wrap Merge (`CSG_WrapMerge`:1115,
  konvexe Huelle aus libs/quickhull), Intersect (:1216), Make Room (:1401),
  Hollow mit seinen Modi (`CSG_Hollow`:1438), CSG Tool.
- **Clipper** (radiant/clippertool.cpp, selection_mtor_clip.cpp,
  `Scene_BrushSplitByPlane` csg.cpp:795): Clip, Split, Flip; Ebene aus
  2/3 Klickpunkten.
- **Manipulatoren als Kernoperationen** (radiant/selection_mtor_*.cpp,
  selection_mtable_*.h): Verschieben, Drehen, Skalieren, Scheren, Ziehen
  (Drag), Extrudieren, UV; Komponenten ziehen (brush.h
  `FaceInstance`/`BrushInstance`: Vertex-/Edge-/Face-Drag,
  `Brush::vertexModeTransform`, Huellenbau im Vertex-Modus) inkl. Texture
  Vertex Lock (`texdefAusSt` ist da).
- **Pinsel-Grundformen** (radiant/brushmanip.cpp): Prism, Cone, Sphere, Rock,
  Icosahedron; Snap To Grid fuer Pinsel (`Brush::snapto`, Patch `einrasten`
  ist da); AutoCaulk.
- **Patch-Werkzeuge** (radiant/patch.cpp, patchmanip.cpp): Prefabs (Bevel,
  Endcap, Zylinder normal/dicht/sehr dicht/eckig, Kegel, Kugel, "exakt"),
  Simple Patch Mesh, Cap Selection (`CapTexture`, Kappen), Zeilen/Spalten
  einfuegen/loeschen (`InsertRemove`), Redisperse, Smooth, Textur Reset /
  Naturalize / Invert X/Y, Overlay, Deform, Thicken; Patch-st-Operationen
  (`SetTextureRepeat`, `TranslateTexture`, `ScaleTexture`, `RotateTexture`,
  `FitTexture`, `ProjectTexture`) analog zu `Flaeche`.
- **Auswahl-Abfragen** (radiant/select.cpp): Invert, Inside, Touching, All Of
  Type, Textured, Expand To Primitives/Entities/Layers, Connected Entities,
  Find Entity/Brush (Nummern wie `// entity N`/`// brush N`), Region.
- **Bearbeiten**: Copy/Paste ueber das .map-Format der Auswahl
  (`Map_ExportSelected`/`Map_ImportSelected`), Duplicate und "make unique"
  (targetname-Umbenennung), Delete, Parent, Move Primitives to Entity,
  Klassenwechsel (`Scene_EntitySetClassname`, Punkt <-> Gruppe), Connect /
  KillConnect (target/targetname), Farbe setzen/normalisieren.
- **Transformationen von Entitaeten** (plugins/entity: origin, angle/angles,
  modelscale, `rotation`), Nudge/Rotate/Flip/Arbitrary/Repeat Transforms
  ueber die Auswahl.
- **Filter** (radiant/filters.cpp: Regeln nach Klassenname, Shader-Flags
  `QER_*`, Inhalts-Flags) und Ebenen-Operationen (anlegen, umbenennen,
  verschieben, verstecken; Ebene am Primitiv ist schon da, aber noch nicht
  im Undo-Zustand).
- **Datei**: Karte importieren/zusammenfuehren, Auswahl speichern, Pointfile
  (`.lin`), Portaldatei (`.prt`, prtview), Build-Menue (q3map2-Aufrufe aus
  der .game/.xlink-Konfiguration), Map Info.
- **Modelle** (misc_model, md3/glm/ase, Skins) fuer Anzeige und Auswahl-
  Grenzen.
- **Plugins**: bobToolz (Stair/Door Builder, Find Duplicates, Intersect,
  Merge/Split Patches, Polygon Builder, Explode/Drop Entity, Path Plotter,
  Vis Viewer, Tree Planter, ...), prtview, brushexport (OBJ aus Wicklungen und
  st, beides da), meshtex (Patch-st), shaderplug (Tag-Datei aus dem
  Shader-Inventar), sunplug (`_sun` aus Licht).
- **Einstellungen**: Pinselformat, Texture Lock, Standardskalierung,
  Patch-Unterteilung, Undo-Stufen sind schon Parameter im Kern; die
  uebrigen Seiten der Checkliste brauchen eigene Werte.

### Sonstiges

- `NPC_spawner` fuer JK2 nachtragen: OpenJK hat in codeJK2/game dafuer nur
  den Block `NPC_*****`; aus dem Code von `SP_NPC_spawner` ableiten (NRCs
  Paket nur, wenn seine Lizenz es erlaubt).
- Meldungen des Kerns (`LeseErgebnis::meldungen` im Wortlaut von NRC,
  `Pinsel::eulerFehler` = "Final B-Rep: inconsistent vertex count") gehoeren
  in Stufe 1b in eine Konsole wie bei NRC.
- Grosse Karten: die groesste MD-Karte (`duel_mos_espa`, 31671 Pinsel, 6820
  Patches; `duel_jedi_battleground` hat 44531 Pinsel) braucht im ganzen
  Testdurchlauf (lesen, buildBRep, schreiben, zweimal erneut lesen) 3.9 s;
  Laden im Hintergrund und die Darstellung muss Stufe 1b selbst messen.
- Compiler-Entitaeten `_decal` (JA) und `_skybox` (q3map2) in die .def
  aufnehmen, wenn der Build-Teil kommt.
- Movie-Duels-Inventar laedt in etwa 3.6 s (617 Shaderdateien, 9722
  Shader); fuer die Oberflaeche im Hintergrund laden.

## Changelog

## English

### Map editor (groundwork)
- The project is now licensed under the GNU GPL v2 (game data from Raven/Activision under `data/` excepted); the code ported from NetRadiant-Custom, GtkRadiant, q3map2 and OpenJK is listed in the README.
- New map editor core without a user interface yet: brushes, texture projection (Quake 3, brush primitives, Valve 220), patches, entities, layers, selection and undo work like NetRadiant-Custom, and .map files are read and written byte for byte like NetRadiant-Custom.
- Entity definitions for Jedi Academy (single and multiplayer), Jedi Outcast and Movie Duels, generated from OpenJK, and a texture and shader inventory for all three games.

## Deutsch

### Karten-Editor (Grundlage)
- Das Projekt steht jetzt unter der GNU GPL v2 (ausgenommen die Spieldaten von Raven/Activision unter `data/`); der aus NetRadiant-Custom, GtkRadiant, q3map2 und OpenJK übernommene Code steht in der README.
- Neuer Kern für den Karten-Editor, noch ohne Oberfläche: Brushes, Texturprojektion (Quake 3, Brush Primitives, Valve 220), Patches, Entities, Ebenen, Auswahl und Rückgängig arbeiten wie in NetRadiant-Custom, und .map-Dateien werden Byte für Byte wie von NetRadiant-Custom gelesen und geschrieben.
- Entity-Definitionen für Jedi Academy (Einzel- und Mehrspieler), Jedi Outcast und Movie Duels, erzeugt aus OpenJK, und ein Textur- und Shader-Verzeichnis für alle drei Spiele.

## 中文

### 地图编辑器(基础)
- 本项目现在采用 GNU GPL v2 许可(`data/` 下 Raven/Activision 的游戏数据除外);从 NetRadiant-Custom、GtkRadiant、q3map2 和 OpenJK 移植的代码列在 README 中。
- 新的地图编辑器核心,暂无界面:画刷、纹理投影(Quake 3、Brush Primitives、Valve 220)、曲面片、实体、图层、选择和撤销的行为与 NetRadiant-Custom 相同,.map 文件的读写与 NetRadiant-Custom 逐字节一致。
- 由 OpenJK 生成的 Jedi Academy(单人和多人)、Jedi Outcast 和 Movie Duels 的实体定义,以及三款游戏的纹理和着色器目录。

## 日本語

### マップエディター(基盤)
- プロジェクトは GNU GPL v2 でライセンスされるようになりました(`data/` 内の Raven/Activision のゲームデータを除く)。NetRadiant-Custom、GtkRadiant、q3map2、OpenJK から移植したコードは README に記載しています。
- 新しいマップエディターのコア(UI はまだありません):ブラシ、テクスチャ投影(Quake 3、Brush Primitives、Valve 220)、パッチ、エンティティ、レイヤー、選択、元に戻すが NetRadiant-Custom と同じように動作し、.map ファイルは NetRadiant-Custom とバイト単位で同じに読み書きされます。
- OpenJK から生成した Jedi Academy(シングル/マルチプレイヤー)、Jedi Outcast、Movie Duels のエンティティ定義と、3 つのゲームすべてのテクスチャ・シェーダー一覧。
