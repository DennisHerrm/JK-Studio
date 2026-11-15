# Notizen CW-MODELL: Clone-Wars-Kette Teil 3 - Modelle (15.11.2026, `Y:\p_ap9`, Zweig `p_cwmodell` von karte1 13ea0a9bd)

Auftrag: `JKStudio_Uebergabe\auftraege\auftrag_cwmodell.md`, Liste des Bruders Abschnitte 2 (Detailstufen MD3), 7 (Ghoul2), 8 (Leistung).
Build `Y:/p_ap9/build` (`--parallel 2 -- /nodeReuse:false`, Leerlauf), Laeufe `cw_*` unter `Y:\p_ap9\build\laeufe`, Skripte und Protokolle
`Y:\werkzeuge\cwmodell\`. Quellen (nur gelesen): OpenJK `Y:\werkzeuge\sp_af9c\m3\ojq\src` (der im Auftrag genannte Ordner
`sp_af9c\openjk` ist leer), Movie Duels Update 9 `Y:\werkzeuge\sp_654alt\md9`, Spieldateien `GameData\base\assets0-3.pk3` und
`GameData Movie Duels\MD\*.pk3`.

## Vorgabe "drin, aber nicht Standard" (shank 14.11.)

- Die Werkzeuge (Detailstufen erzeugen, Bolts, eigenes Skelett, Sequenz anlegen) sind immer sichtbar, wirken aber nur auf Aufruf; jeder
  Aufruf ist ein Rueckgaengig-Schritt im Dokument.
- Nur mit dem Pruefprofil "Clone-Wars-Kette" (`OptimizerCloneWars`, ab Werk aus): der Richtwert der niedrigsten Stufe (Figur 500,
  Ausruestung 300 Ecken), die Warnungen danach in der Stufenanzeige, die Bolt-Pruefung ohne Markierung "Als Fahrzeug pruefen" und die
  Optimierer-Pruefungen `modelldetail` / `modellstufen`.
- **Beleg Bytegleichheit** (Werkzeug `tests/modell_cwvergleich.cpp`, nur Funktionen von 13ea0a9bd): gegen den Kern von 13ea0a9bd gebaut
  (`Y:\werkzeuge\cwmodell\basis\modellcwvergleich_13ea0a9bd.exe`) und gegen den dieses Zweigs - 1858 Dateien (JKA assets0-3 und Movie
  Duels MD_Assets*, MD_Models_*, MD_Weapons: 623 .glm, 999 .md3, 117 .gla, 119 animation.cfg, dazu data/fixtures) schreiben Speichern,
  Laden (Dokument je Stufe) und animation.cfg **dieselben Bytes** (`basis/vorher.txt` = `nachher_*.txt`). ctest `modell_cwvergleich`
  vergleicht die eingebauten Proben (.md3 mit 2 Bildern und Tag, .gla, zwei animation.cfg) und data/fixtures/model.glm mit dem Soll
  `tests/modell/cwvergleich_soll.txt` (erzeugt mit dem 13ea0a9bd-Werkzeug).

## Punkt 1: Detailstufen erzeugen

### Ausgangslage (geprueft)
- Kein Vereinfacher im Code (git grep quadric/decimat/simplify/Vereinfach). MD3 wird geschrieben: `md3Schreiben` (bhed/modell/md3datei.h)
  auf der gelesenen Datei - Bilder, Tags, Kopf wie gelesen, Ecken je Bild ueber `Ecke::dateiIndex`. `name_1.md3` / `name_2.md3` liest
  heute nur die 3D-Vorschau (REST3d, gui/app_view3d_spiel.inc, md3LodWieImSpiel); der Model-Tab kannte keine Stufen einer .md3.
- RE_RegisterModel (OpenJK code/rd-vanilla/tr_model.cpp:630-700): .md3 mit `iLODStart = MD3_MAX_LODS-1`, laedt name_2, name_1, name;
  fehlt eine hoehere Stufe nach einer geladenen: ERR_DROP "has LOD %d but is missing LOD %d" (:650-653). R_ComputeLOD (tr_mesh.cpp:189-232):
  Radius aus den Grenzen von md3[0] (Stufe 0), `flod = 1 - projectedRadius * r_lodscale`, mal numLods. R_AddMD3Surfaces (tr_mesh.cpp:
  320-380) prueft das Bild gegen md3[0]->numFrames und zeichnet die Flaechen von md3[lod] mit demselben Bild - jede Stufe braucht alle
  Bilder. Ghoul2: G2_ComputeLOD (tr_ghoul2.cpp:834-905), Schatten der grossen Flaeche aus der letzten Stufe (:2307,
  SHADER_MAX_VERTEXES / 2), R_LoadMDXM (:3803-3809) weist Flaechen ueber SHADER_MAX_VERTEXES / _INDEXES ab.

### Vorgaben der Anteile (gemessen, `lod_verhaeltnis.py`, `md3_lod.py`)
| Art | Modelle | LOD 2 / LOD 1 | LOD 3 / LOD 1 | letzte Stufe (Median) |
|---|---|---|---|---|
| Figuren auf _humanoid (.glm) | 51 | 0,672 (0,518 ... 0,870) | 0,408 (0,318 ... 0,596) | 772 Ecken (max 4772) |
| davon mit 4 Stufen | 28 | 0,744 | 0,429 (LOD 4: 0,283) | - |
| andere Figuren (.glm) | 14 | 0,643 | 0,425 | 628 |
| Waffen (.glm) | 9 | 0,560 (0,425 ... 0,755) | 0,308 (0,194 ... 0,755) | 168 (max 404) |
| .md3 mit name_1 (meist Ansichtswaffen) | 14 / 7 | 0,749 | 0,491 | - |
Vorgaben: Figur 0,65 / 0,40 (vier Stufen 0,74 / 0,43 / 0,28), Waffe/Ausruestung 0,55 / 0,30, Requisit 0,75 / 0,50. Die Art kommt aus
dem Pfad (models/players/ Figur, models/weapons*/ und models/items/ Ausruestung) und dem Skelett (.glm an *default: Ausruestung).

### Verfahren und Wahl (Entscheidung)
- **meshoptimizer v1.3** (zeux/meshoptimizer, MIT, Copyright (c) 2016-2026 Arseny Kapoulkine; `third_party/meshoptimizer`: simplifier.cpp,
  allocator.cpp, meshoptimizer.h, LICENSE.md, unveraendert, Release 2026-09-25) - `meshopt_simplifyWithAttributes`: Kantenkollaps nach
  Quadriken (Garland/Heckbert 1997) mit Attributen (Hoppe 1999) auf **vorhandene Ecken**. Lizenz in THIRD-PARTY.md und im Release-Zip
  (tools/release_paket.py `meshoptimizer-MIT.txt`).
- Warum diese Wahl (nach Ergebnis, nicht nach Aufwand): (1) jede Ecke einer Stufe ist eine Ecke von LOD 1 - eine .md3 behaelt alle Bilder
  ohne Mitteln (jedes Bild nimmt den Ort seiner Ecke; R_AddMD3Surfaces braucht sie), eine .glm behaelt die Gewichte der Datei Bit fuer Bit
  (hoechstens 4, Summe 1 - ein Kollaps mit neuer Lage muesste Gewichte mischen und auf 4 kuerzen, dann verformt sich die Figur anders);
  (2) Naehte (gleiche Orte, andere UV/Normale) behandelt der Vereinfacher als Naht und laesst sie geschlossen; (3) gleiche Bytes bei gleicher
  Eingabe (keine Zufaelle, keine Faeden). Blenders Decimate (Collapse) setzt neue Orte und mischt die Ecken-Daten - fuer Ghoul2 und
  mehrbildige .md3 nicht verlustfrei.
- **Guete bei gleichem Bild** (ctest modell_detailstufen, model.glm _humanoid mit Ravens 4 Stufen, gleiche Eckenzahl wie die Datei):
  Abstand der Ecken von LOD 1 zur Flaeche der Stufe (Einheiten) - LOD 2 mit 1281 Ecken: Raven groesster 2,324 / mittel 0,112, erzeugt
  **0,493 / 0,041**; LOD 3 mit 716 Ecken: Raven 2,229 / 0,223, erzeugt **1,627 / 0,238** (groesster deutlich kleiner, mittlerer gleich auf).
  Nebenbei gemessen: Ravens grobe Stufen liegen nur zu 41,7 % auf Orten von LOD 1 (`lod_teilmenge.py`) - von Hand bzw. mit neuer Lage
  gebaut.
- **Gewichte der Attribute** 0,1 (Normale, UV nach der UV-Dichte, Knochengewichte je Flaeche in 27 Spalten): an model.glm verglichen -
  alle 1: LOD 2 0,842 / 0,062, LOD 3 2,576 / 0,321; 0,25: 0,493 / 0,049 bzw. 2,016 / 0,279; 0,1: 0,493 / 0,041 bzw. 1,627 / 0,238; 0:
  0,397 / 0,035 bzw. 1,810 / 0,211. 0,1 haelt Normalen, UV und Gewichte noch im Fehler und ist an beiden Stufen naeher an der Form als
  die Datei. Unter "Erweitert" einstellbar. Geprueft und verworfen: Orte unter 0,01 Einheiten als einen Ort zaehlen (74 Randorte der
  Datei liegen so) - die letzte Stufe wurde schlechter (mittel 0,344 statt 0,321 bei Gewicht 1), deshalb ohne.
- **Gemeinsam statt je Flaeche**: alle vereinfachten Flaechen einer Stufe in einem Puffer - an der Grenze zweier Flaechen liegen Ecken am
  selben Ort, fuer meshoptimizer die Seiten einer Naht: eine Kante kollabiert in beiden Flaechen, der Rand bleibt geschlossen (ctest:
  jede Randkante zwischen Flaechen gibt es in derselben Stufe an der Nachbarflaeche). Offene Raender bleiben (meshopt_SimplifyLockBorder),
  Orte, die ein Tag oder eine _off/_cap_-Flaeche mitbenutzt, sind gesperrt (meshopt_SimplifyVertex_Lock). Je Flaeche zuerst gemessen:
  von 1504 Orten der 17 vereinfachten Flaechen sind 515 Randorte, davon 182 nur an anderen vereinfachten Flaechen (`raender.py`); je
  Flaeche erreichte LOD 3 nur 887 statt 758 Ecken (9 Ziele verfehlt), gemeinsam genau 758.
- Ziel in Ecken: meshoptimizer zielt auf Dreiecke; gesucht wird das groesste Dreiecksziel, dessen Ergebnis hoechstens so viele Ecken hat
  (binaere Suche). Danach je Flaeche: ueber SHADER_MAX_VERTEXES - 1 bzw. (letzte Stufe einer .glm) SHADER_MAX_VERTEXES / 2 - 1 wird die
  Flaeche fuer sich weiter vereinfacht, ihr Rand bleibt.
- Unberuehrt: Tags (Flagge oder "*"), "_off" am Ende, "_cap_" im Namen - in jeder Stufe gleich Stufe 0.

### Oberflaeche
- Bereich Modellieren, Reiter Aendern (wie 3ds Max ProOptimizer/MultiRes) bzw. Modifier (wie Blender Decimate), nach den JKA-Grenzen
  und ab Werk zugeklappt (Kopfzeile "3 Stufen"; zuerst stand sie unter dem Stapel und schob Extrudieren/Fase aus dem Bild - AP6): Vorlage, Stufen 2/3/4 (.md3 hoechstens 3), Anteil je Stufe mit Ziel in Ecken, Erweitert (Gewichte), Erzeugen (Hauptknopf),
  Entfernen, Tabelle vorher -> nachher (Klick zeigt die Stufe), groesste Abweichung. Eintrag in der Modifier-Liste / "Modifier hinzufuegen"
  (oeffnet die Gruppe - ein Eintrag fuer das ganze Modell, kein Eintrag im Stapel einer Flaeche), Menue des Stapels (auch Rechtsklick der
  Ansicht), Aktionssuche, Tooltips.
- Speichern einer .md3 mit Stufen: name.md3 wie bisher, dazu name_1.md3 / name_2.md3 (md3StufeSchreiben: Kopf, Bilder, Tags der Datei, die
  Flaechen der Stufe). Liegt daneben noch eine aeltere name_2.md3 und es gibt nur zwei Stufen, nennt die Meldung sie.

## Punkt 4: Anzeige je Stufe
- "Detailstufe und Skin" (Ansehen/Pruefen): Tabelle Stufe / Ecken / Dreiecke / Teilflaechen (gezaehlt, was das Spiel ohne Haut zeichnet:
  ohne Tags und ab Werk abgeschaltete Flaechen), Tooltip mit groesster Teilflaeche und Befunden: Ecken ueber SHADER_MAX_VERTEXES (rot),
  Schattengrenze der letzten Stufe einer .glm, MD3_MAX_VERTS 4096, MD3_MAX_SURFACES 64 (qfiles.h:101-105); bei einer .md3 auch die Dateien
  name_1/_2 daneben (von der Platte bzw. wie RE_RegisterModel aus den Spielordnern). Ohne Stufen der Knopf "Detailstufen erzeugen ...".
- Clone-Wars-Kette: Richtwert-Befund und "nur eine Stufe" unter der Tabelle; Optimierer (Profil an): `modelldetail` (Hinweis: Laufzeit-.md3,
  mehr Ecken als SHADER_MAX_VERTEXES des Regelwerks, keine _1.md3) und `modellstufen` (Fehler: _2.md3 ohne _1.md3, ERR_DROP) - mit
  Beschreibung, Detailtext KeOptD55/56, JSON wie die anderen.
- "Gleiche Modelle zusammenfassen / Atlas" (Hilfreich, nicht umgesetzt): es braeuchte (1) eine Liste gleicher Modelle je Karte (gleicher
  Pfad und Skin - der Optimierer kennt die Modelle schon) und den Hinweis "dieselbe Datei mehrfach einsetzen"; (2) fuer den Atlas je Modell
  die Shader mit einem einzigen Bild ohne Stufen-Effekte, xatlas (third_party/xatlas, schon im Repo) zum Packen der UV-Inseln aller
  Flaechen in einen Atlas, das Zusammensetzen der Bilder (Zweierpotenz, hoechstens 2048), neue UV je Ecke und ein neuer Shader - sowie die
  Pruefung, dass Flaechen mit Glanz/Blend/Animation nicht zusammengelegt werden. Faellt nicht von selbst ab (Bilder neu backen).

## Punkt 3: Bolts fuer Sitze und Muendungen
- Lage aus dem Dreieck wie G2_ProcessSurfaceBolt2 (tr_ghoul2.cpp:2080 ff., in bhed/glm.h boltMatrix): Seite 0 die laengste, Seite 2 die
  kuerzeste, Ecke 2 der Ursprung; boltDreieck baut Ecke 0 = Ursprung + a X, Ecke 1 = Ursprung + 2a Y, Ecke 2 = Ursprung - die gelesene
  Lage ist die gesetzte (ctest).
- Namen, die das Spiel sucht (Vorlagen mit Quelle):
| Name | Wofuer | OpenJK | Movie Duels Update 9 |
|---|---|---|---|
| *driver | Fahrersitz (CLASS_VEHICLE) | code/game/g_client.cpp:1210, g_vehicles.cpp:298, bg_vehicleLoad.cpp:1697; codemp/game/g_vehicles.c:135 | code/game/g_client.cpp:1728 |
| *droidunit | Droide (R2/R5) | code/game/g_client.cpp:1213; codemp/game/g_client.c:1857 | code/game/g_client.cpp:1731 |
| *exhaust1..n | Auspuff (exhaustFX, turboFX) | code/game/g_client.cpp:1217-1222, n = MAX_VEHICLE_EXHAUSTS 4 (g_vehicles.h:102) | :1736-1740, n = 12 (g_vehicles.h:104) |
| *muzzle1..n, sonst *flash1..n | Muendungen der Fahrzeugwaffen | code/game/g_client.cpp:1224-1234, n = MAX_VEHICLE_MUZZLES 10 (g_vehicles.h:99); codemp/game/g_client.c:1869 | :1743-1752, n = 12 (g_vehicles.h:103) |
| *trail1..4 | Spuren (trailFX, nur MP) | codemp/cgame/cg_players.c:8229-8232 | codemp/cgame/cg_players.c:17269-17273 |
| turretNGunnerViewTag | Blick des Schuetzen (freier Name aus der .veh) | codemp/game/g_client.c:1883, cg_players.c:7171 | code/game/g_client.cpp:1755-1762 (auch SP) |
| *seat, *flash01, *flash02 | misc_emplaced_gun | code/game/g_emplaced.cpp:837-839 | :860-862 |
| *cannonflash | Kanone (E-Web) | code/game/g_emplaced.cpp:472 | :499 |
| *flash | Muendung eines Waffenmodells | code/game/NPC_spawn.cpp:668-684, cgame/cg_players.cpp:1072 | NPC_spawn.cpp:1314-1334, cg_players.cpp:1277 |
  Dazu die Tags, die der Spielcode an Figuren sucht (gesuchteTags, g2.h). Sitze weiterer Mitfahrer: das Spiel sucht keinen eigenen Bolt
  (MP-Passagiere ohne Tag) - fuer Schuetzen der freie Name aus turretNGunnerViewTag.
- Model-Tab, Bereich Modellieren: Gruppe "Bolts (Point-Helfer)" (Reiter Erstellen) bzw. "Bolts (Empties an Knochen)" (Reiter Objekt):
  Vorlage (Gruppen Fahrzeug, Geschuetz, Waffe, Figur; belegte Namen gesperrt; Quelle im Tooltip), Name, Knochen, Ort, Drehung (Euler XYZ wie
  3ds Max), Anlegen / An den Knochen / Uebernehmen / Entfernen, Liste der Bolts. Schalter "Als Fahrzeug pruefen": Hinweis ohne *driver bzw.
  ohne jede Muendung; mit dem Clone-Wars-Profil immer. Nur .glm (eine .md3 traegt Tags, kein Bolt-Dreieck).

## Punkt 2: Eigenes Skelett und animation.cfg fuer Fahrzeuge
### Was fehlte (festgehalten vor der Arbeit)
| Schritt | vorher | jetzt |
|---|---|---|
| Knochen anlegen (AT-RT: Rumpf, Beine, Sitz; AT-TE: sechs Beine; Droide) | fehlte ganz - nur GLAs aus Dateien oder Importe als Quelle fuers Retargeting | Neues Skelett, Knochen anlegen/verschieben/umhaengen/entfernen (bhed/modell/skelett.h), im Dokument mit Rueckgaengig |
| GLA speichern | ja (schreibGla der geladenen GLA) | dasselbe mit dem eigenen Skelett (Kopf models/players/<n>/<n>) |
| GLM daran binden | nur "Andere GLA" (Knochen muessen passen) | Neues Skelett bindet alle Flaechen an model_root; "Auswahl an Knochen" starr (Gewicht 1, alle Stufen); feinere Gewichte im Bereich Weights |
| Animationen keyen | Control Rig/Dopesheet auf vorhandenen Bildern | "Sequenz anlegen" haengt Bilder (Grundstellung) und die Zeile der animation.cfg an, danach wie jede Sequenz stellen und keyen |
| animation.cfg | Editor/Ausgabe vorhanden, Namenspruefung nie aufgerufen (cfgPruefen ohne `bekannt` - unbekannte Namen still verworfen) | Pruefung mit der animTable des Zielspiels (bhed/modell/animnamen.h) |
- Wie das Spiel liest: G_ParseAnimationFile (OpenJK code/game/NPC_stats.cpp:879, unbekannter Name :927-940 uebergangen, Warnung nur ohne
  FINAL_BUILD; MD9 :1096), BG_ParseAnimationFile (codemp/game/bg_panimate.c:2302/2429; MD9 :6008). Fahrzeuge laden ihre animation.cfg wie
  jedes NPC (G_ParseAnimFileSet, Skelettname aus der GLA). Die animTable steht in cgame/animtable.h (ENUM2STRING): JKA SP 1544, MP 1543,
  JK2 1210, Movie Duels SP = MP 3213 Namen - erzeugt nach src/modell/animnamen.inc (tools/modell/gen_animnamen.py).
- Menschliche Figuren auf _humanoid.gla: unveraendert - Knochen gibt es nur am eigenen Skelett (Gruppe sperrt sonst), die Bytegleichheit
  oben deckt _humanoid-Modelle und die GLA ab.

- Ort: Bereich Animation, wie 3ds Max im Reiter Erstellen (Systems > Bones), wie Blender im Reiter Knochen im Bearbeitungsmodus (Setup-
  Modus des Motion-Felds, wie eine Armature im Edit Mode); ab Werk zugeklappt, solange das Modell kein eigenes Skelett hat. Bolts: Bereich
  Modellieren, Reiter Erstellen bzw. Objekt, ab Werk zugeklappt. Beide auch ueber Menue des Stapels, Rechtsklick der Ansicht und Suche.

## Entscheidungen (so getroffen, shank: "mach die fragen so wie du sie denkst das sie richtig waeren")
1. Vereinfacher meshoptimizer statt eigener Umsetzung oder Blender-Code (Begruendung oben: Ecken bleiben, Naehte, gleiche Bytes, Guete).
2. Detailstufen als Werkzeug fuer das ganze Modell, nicht als Eintrag im Stapel einer Flaeche (eine .glm hat fuer alle Flaechen dieselbe
   Zahl Stufen; Ravens Werkzeug machte es genauso). Der Stapel rechnet danach auf jeder Stufe (Glaetten auf LOD 2 ...).
3. Clone-Wars-Richtwert: 500 Ecken je Figur, 300 fuer Waffen/Ausruestung in der letzten Stufe; Requisiten ohne Richtwert. Begruendung:
   150-250 Figuren - bei 500 sind 250 ferne Figuren 125 000 Ecken (heute JKA-Median der letzten Stufe 772); das Gewehr des Bruders hatte
   1000, JKA-Waffen 168 (max 404) - 300 ist erreichbar und viermal billiger als heute bei ihm. Raender und Naehte koennen mehr halten
   (model.glm: 536 statt 500) - die Tabelle warnt dann.
4. Die Namenspruefung der animation.cfg (unbekannte Namen) gilt ohne Profil: das Spiel verwirft die Zeile tatsaechlich, die Warnung
   aendert keine Datei (wie CW-IMPORT Entscheidung 1).
5. "Als Fahrzeug pruefen" ist ein Schalter je Modell (nicht in der Datei); ohne ihn und ohne Profil keine Bolt-Hinweise.
6. Eigenes Skelett im Dokument (Inhalt::skelett): Rueckgaengig nimmt Knochen, Bindung und Sequenzen zusammen zurueck; ein Skelett aus einer
   Datei laesst sich nicht aendern.
7. Optimierer-Pruefungen fuer Laufzeit-.md3 nur mit dem Profil (sie aendern sonst den Bericht gegenueber karte1).

## Rot -> Gruen

| Punkt | rot | gruen |
|---|---|---|
| animation.cfg: unbekannte Namen im Werkzeug (die GUI rief cfgPruefen ohne `bekannt` auf - Befund nie gezeigt) | Selbsttest cwmodell Lauf cw_rot: "Befund ATRT_HOP nein, 1 Befunde" (25 OK, 3 FEHLER) | nach dem Fix (modellCfgBefunde und Frage beim Backen mit animNamenSpiel) cw_gruen: "Befund ATRT_HOP ja, 2 Befunde"; endgueltig cw_g4 28 OK / 0 |
| Skelett: "Auswahl an Knochen" direkt nach "Knochen anlegen" (die Anzeige nimmt das Skelett erst im naechsten Bild) | cw_gruen 27 OK / 1 FEHLER ("Flaeche ... starr an barrel") | Knochenzahl aus dem Dokument: cw_gruen2 28 / 0 |
| Detailstufen je Flaeche (Raender fest): LOD 3 von model.glm nur 887 statt 758 Ecken, 9 Ziele verfehlt | ctest modell_detailstufen (Ausgabe "verfehlt 9") | gemeinsam vereinfacht: 758, verfehlt 0, Randkanten zwischen Flaechen geschlossen (ctest) |
| neue Gruppen verschoben die Werkzeuge des Bereichs Modellieren (AP6: Extrudieren/Fase nicht ganz sichtbar) | Selbsttest ap6 cw_ap6 80 OK / 30 FEHLER, cw_ap6b 83 / 21 (Basis 13ea0a9bd: cw_ap6_basis 95 / 0) | Gruppen ab Werk zugeklappt, Detailstufen nach den JKA-Grenzen, Bolts nur im Reiter Erstellen/Objekt: cw_ap6c 95 / 0 |
| Bilder: Bolt-Liste schob Hinweis und Schalter aus dem Bild, "-0.00", Tipp brach ".gla" am Punkt, Kopf der Knochenfelder zeigte Eltern = sich selbst | Fotos cw_hb_en (angesehen) | Liste hoechstens 8 Zeilen, Hinweis davor, gerundete Anzeige, GLA/GLM im Tipp, nach "Bone erstellen" die Felder fuer den naechsten Knochen der Kette; Fotos neu (angesehen: es lod_max, de bolts, ja skelett) |
| Neue Funktionen (Detailstufen, Bolts, Skelett, Namen, Optimierer) | die Tests liessen sich auf 13ea0a9bd nicht uebersetzen (Funktionen fehlten) - kein eigener roter Lauf | ctest modell_detailstufen, modell_cwskelett, karte_cwpruef (neu 5 Faelle), karte_optimierer (56 Vorlagen) gruen |

## Tests

- Bau alle Ziele (bau15) 0 Fehler, 0 Warnungen; danach behaved inkrementell 0 Warnungen (bau16-18).
- ctest (37, -R modell_|karte_cwpruef|karte_cwimport|karte_cwvergleich|karte_optimierer|karte_grenzen|hilfe_orte|handbuch|einstellungen_alle|
  karte_md3import|glm|gla) 100 % gruen; dazu modell_cwvergleich (Soll aus 13ea0a9bd), modell_detailstufen, modell_cwskelett.
- Bytegleich: `vergleich_lauf.sh` mit dem Endstand (nachher_ende.txt) = basis/vorher.txt (13ea0a9bd), 1858 Dateien.
- Selbsttest cwmodell (BHED_MODELL_NUR=cwmodell): Direct3D 12 28 OK / 0 (cw_g4), Direct3D 11 27 / 0 (cw_g3_11; ein Pruefpunkt
  weniger im Rahmen unter D3D11). Darin: Speichern ohne Erzeugen = Datei des Spiels (kyle 330284 Bytes); Erzeugen kyle 5260/3419/2104
  Ecken, Abweichung 0,85; Richtwert mit Profil 2104 -> 1247 (Raender/Naehte), Befund ja; ohne Profil kein Richtwert; bacta_tank zeigt
  bacta_tank_1.md3 (289 Ecken); ship_part03 erzeugt und gespeichert: _1.md3 481, _2.md3 462 Ecken, alle Bilder; Bolts *driver/*muzzle1
  gespeichert und gelesen, Rueckgaengig; Skelett an blaster_w: 3 Knochen, 15 Bilder, GLA/GLM/animation.cfg geschrieben und gelesen,
  Rueckgaengig bis zur GLA der Datei.
- Gruppen, die die Aenderungen beruehren (gegen Basis 13ea0a9bd mit Y:\karte\build-test): ap6 95/0 (Basis 95/0), ap13s 123/0 (mit
  BHED_FOTOS; ohne fehlen 4 Datei-Pruefungen nur wegen des Ordners), ap10 72/0, ap5 121/0, mb2 39/0, test1411 N 12/0.
- Lints: i18n, i18n_erzeugt, menue, settings, headers, sources, feldinit, cjkumbruch, doppelanzeige, abstand, layout, gapp,
  imgui_context, tabellenid, endtable, hoehe ok; portability 97 und pruefcodes 11 wie auf 13ea0a9bd.
- Handbuch: Seite modell-clonewars, Bilder h_cwmodell_lod_max, _lod_blender, _stufen, _bolts, _skelett in 6 Sprachen (2516 x 1416,
  Lauf BHED_MODELL_NUR=cwmodellhb; je Bild die Pruefung "kein Pfad des Laufs" gruen; angesehen: en alle fuenf, de bolts, es lod_max,
  ja skelett); gen_handbuch --pruefen ok, src/hilfe_orte.inc neu (470 Orte).
- Kein Spielstart noetig: dass das Spiel name_1.md3 laedt, steht in RE_RegisterModel (oben) und zeigt die 3D-Vorschau (REST3d) schon;
  die geschriebenen Dateien liest readMd3 mit allen Bildern.

## Fund (fuer den Hauptchat)

- Movie Duels Update 9: die eigene `models/players/_humanoid/animation.cfg` (MD_Assets2.pk3, 2635 Zeilen) nennt 7 Namen, die die
  animTable von Update 9 nicht kennt (BOTH_INV_USE, BOTH_PARRY_WALK_DUAL, BOTH_PARRY_WALK_STAFF, BOTH_RUN3_MP, BOTH_SPRINT_MP,
  BOTH_SPRINT_SABER, BOTH_SPRINT_SABER_MP) - G_ParseAnimationFile uebergeht diese Zeilen. Mit Zielspiel Movie Duels zeigt die Gruppe
  animation.cfg jetzt diese 7 Warnungen. Die JKA-Datei hat keine.

## Nachtrag nach der Pruefung durch den Hauptchat (15.11. spaet)

### zh-Bilder waren die deutschen
- Ursache: `Y:\werkzeuge\cwmodell\hb_modell.sh` setzte fuer den Bildlauf im Modus modellblock `SPRACHE=zh` (lauf.py schreibt das als
  `language=` in die Einstellungen). Das Programm kennt nur den Code `zh-Hans` (findLanguage, src/i18n.cpp, Tabelle der Sprachen) und nahm
  die Sprache des Systems (Deutsch). BHED_SPRACHE wertet nur der Modus handbuch aus, nicht modellblock. Die anderen Bildlaeufe dieses
  Pakets (ap6, ap13s, ap10, ap5, mb2, test1411, cwmodell) schrieben keine Handbuchbilder (kein bilder.py) - nur hb_modell.sh; betroffen
  waren genau die fuenf zh/h_cwmodell_*.
- Rot: neue Pruefung in `gen_handbuch.py --pruefen` (ctest handbuch_sprachen) - ein Bild, das in zwei Sprachen Byte fuer Byte gleich ist,
  ist ein FEHLER, ausser Datei und Sprachgruppe stehen in `tools/handbuch/bilder_gleich_erlaubt.txt`. Mit den alten zh-Bildern:
  "Bilder in mehreren Sprachen gleich (5) ... h_cwmodell_bolts.webp (de=zh) ..." -> handbuch_sprachen FEHLER (ctest rot).
- Dazu im Bildlauf selbst: "die Oberflaeche spricht die Sprache des Laufs (BHED_SPRACHE)" (selbsttest_cwmodell.inc, zh: "Sprache des
  Laufs zh-Hans, Oberflaeche zh").
- Gruen: hb_modell.sh setzt `SPRACHE=zh-Hans`; alle sechs Sprachen neu aufgenommen (dabei das Bild wie Blender ans Ende des Felds
  gerollt - die Gruppe stand nach den JKA-Grenzen nur mit Kopfzeile im Bild); handbuch_sprachen ok. Angesehen: alle fuenf zh-Bilder.
- **Fund fuer den Hauptchat:** die Liste der erlaubten Gleichheiten ist der Stand von 13ea0a9bd (39 Zeilen, ohne Pruefung, ob jedes Bild
  wirklich ohne Text ist). Verdaechtig, weil sie Oberflaeche zeigen duerften: h_tour_spielordner, h_tour_update, h_weg_bereich,
  h_weg_pfad, h_karte_portale (en=zh), h_weg_abdeckung, h_weg_route, k_kampf_an, k_kampf_ende, f_gruppe, g_gang_baum (de=zh),
  h_karte8b_terrain, h_zl_untertitel, k_folgen_blick, t1_ergebnis, t3_5 (cs=en). Werden sie neu aufgenommen, faellt ihre Zeile weg.

### .md3 speichern und name_1/_2 daneben (Rueckfrage)
| Behauptung | vorher (Lauf cw_md3rot) | nachher (cw_md3gruen) |
|---|---|---|
| ohne erzeugte Stufen schreibt Speichern keine name_1.md3 / name_2.md3 | gruen ("_1 nicht da, _2 nicht da, Stufen 1") | gruen |
| vorhandene name_1/_2 (Ravens bacta_tank_1.md3 als name_1, eine fremde name_2) bleiben ohne erzeugte Stufen Byte fuer Byte | gruen | gruen |
| Stufen erzeugt und name_1/_2 liegen schon da: vor dem Ueberschreiben fragt das Programm (im Programm, kein Windows-Dialog) | **rot**: "Frage 0, Zeilen 0, Dateien GEAENDERT" - sie wurden still ueberschrieben | "Frage 3, Zeilen 2, Dateien unberuehrt"; nach "Trotzdem" stehen die erzeugten Stufen darin (alle Bilder) |
- Umsetzung: modellSpeichern (gui/app_modelltab_mb3.cpp) sammelt mit modellLodMd3Vorhanden die Dateien, die Speichern ueberschreiben
  wuerde (nur mit erzeugten Stufen); liegt eine da, die Frage im Programm (frageArt 3, Text "Speichern schreibt die erzeugten
  Detailstufen neben die .md3 und ueberschreibt diese Dateien:", je Datei eine Zeile); zusammen mit Grenz-Befunden eine Frage. Geloescht
  wird nie etwas. Selbsttest cwmodell 32 OK / 0, mb2 (Frage bei Grenzen) 39 / 0.

## Changelog

### English

- Model tab: Generate detail levels, like ProOptimizer in 3ds Max and Decimate in Blender (area Modeling, under the modifier stack).
  Templates measured on the game files (figures 0.65 / 0.40, weapons and equipment 0.55 / 0.30, props 0.75 / 0.50), 2 to 4 levels, one
  undo step; a .glm carries the levels in the file, saving a .md3 writes name_1.md3 and name_2.md3 next to it with all frames and tags.
  Every vertex of a coarser level is a vertex of LOD 1 (weights and UVs exactly as in the file), borders between surfaces stay closed,
  tags, _off and _cap_ surfaces stay as they are, every surface stays below the limits of the target game (the lowest level of a .glm
  below the shadow limit). The simplifier is meshoptimizer (MIT). Without Generate nothing changes and saving writes the same bytes.
- Model tab: vertices, triangles and surfaces per level in "Level of detail and skin" with the limits of the target game, for a .md3
  also the files name_1.md3 / name_2.md3 next to it. Before saving overwrites name_1/name_2 files that are already there, the program asks.
- Model tab: bolts for seats and muzzles - add, name, link to a bone, move, rotate; the names the game looks for (*driver, *droidunit,
  *muzzle1.., *exhaust1.., *seat, *flash ...) as templates with their source. "Check as vehicle" warns without a driver seat or muzzle.
- Model tab: an own skeleton for vehicles, walkers and droids - new skeleton, add/move/re-parent/remove bones, bind selected surfaces
  rigidly, add sequences for keying, save .gla, .glm and animation.cfg; all as undo steps. Skeletons from files (_humanoid) stay untouched.
- animation.cfg: names the target game does not know (its animation table; OpenJK 1544, Movie Duels 3213) are reported - the game skips
  such lines without a word.
- Clone Wars chain (check profile, off by default): guideline for the lowest level (500 vertices per figure, 300 for weapons and
  equipment) with warnings; bolt check always on; the optimizer reports run-time .md3 models without detail levels and a _2.md3 without
  _1.md3 (ERR_DROP).

### Deutsch

- Model-Tab: Detailstufen erzeugen, wie ProOptimizer in 3ds Max und Decimate in Blender (Bereich Modellieren, unter dem Modifier-Stapel).
  Vorlagen nach den Spieldateien (Figuren 0,65 / 0,40, Waffen und Ausrüstung 0,55 / 0,30, Requisiten 0,75 / 0,50), 2 bis 4 Stufen, ein
  Rückgängig-Schritt; eine .glm trägt die Stufen in der Datei, Speichern einer .md3 schreibt name_1.md3 und name_2.md3 daneben mit allen
  Bildern und Tags. Jede Ecke einer gröberen Stufe ist eine Ecke von LOD 1 (Gewichte und UV genau wie in der Datei), Ränder zwischen
  Flächen bleiben geschlossen, Tags, _off- und _cap_-Flächen bleiben, wie sie sind, jede Fläche bleibt unter den Grenzen des Zielspiels
  (die niedrigste Stufe einer .glm unter der Schattengrenze). Der Vereinfacher ist meshoptimizer (MIT). Ohne Erzeugen ändert sich nichts,
  und Speichern schreibt dieselben Bytes.
- Model-Tab: Ecken, Dreiecke und Teilflächen je Stufe in "Detailstufe und Skin" mit den Grenzen des Zielspiels, bei einer .md3 auch die
  Dateien name_1.md3 / name_2.md3 daneben. Bevor Speichern vorhandene name_1/name_2 überschreibt, fragt das Programm.
- Model-Tab: Bolts für Sitze und Mündungen - anlegen, benennen, an einen Knochen hängen, verschieben, drehen; die Namen, die das Spiel
  sucht (*driver, *droidunit, *muzzle1.., *exhaust1.., *seat, *flash ...), als Vorlagen mit Quelle. "Als Fahrzeug prüfen" warnt ohne
  Fahrersitz oder Mündung.
- Model-Tab: ein eigenes Skelett für Fahrzeuge, Läufer und Droiden - neues Skelett, Knochen anlegen/verschieben/umhängen/entfernen,
  gewählte Flächen starr binden, Sequenzen zum Keyen anlegen, .gla, .glm und animation.cfg speichern; alles als Rückgängig-Schritte.
  Skelette aus Dateien (_humanoid) bleiben unberührt.
- animation.cfg: Namen, die das Zielspiel nicht kennt (seine Animationstabelle; OpenJK 1544, Movie Duels 3213), werden gemeldet - das
  Spiel übergeht solche Zeilen wortlos.
- Clone-Wars-Kette (Prüfprofil, ab Werk aus): Richtwert für die niedrigste Stufe (500 Ecken je Figur, 300 für Waffen und Ausrüstung)
  mit Warnungen; die Bolt-Prüfung immer an; der Optimierer meldet Laufzeit-.md3 ohne Detailstufen und eine _2.md3 ohne _1.md3 (ERR_DROP).

### 中文

- Model 选项卡: 生成细节级别, 如同 3ds Max 的 ProOptimizer 和 Blender 的 Decimate(建模区域, 修改器堆栈下方)。模板按游戏文件测得(人物 0.65 / 0.40,
  武器和装备 0.55 / 0.30, 道具 0.75 / 0.50), 2 到 4 级, 一步撤销; .glm 把各级别存在文件中, 保存 .md3 时在旁边写出带全部帧和标签的 name_1.md3 和
  name_2.md3。较粗级别的每个顶点都是 LOD 1 的顶点(权重和 UV 与文件完全相同), 表面之间的边界保持闭合, 标签、_off 和 _cap_ 表面保持原样, 每个表面都低于
  目标游戏的限制(.glm 的最低级别低于阴影限制)。简化器为 meshoptimizer(MIT)。不点“生成”就什么都不变, 保存写出相同的字节。
- Model 选项卡: “细节级别和皮肤”中显示每级的顶点、三角形和表面数及目标游戏的限制, .md3 还包括旁边的 name_1.md3 / name_2.md3。保存覆盖已存在的 name_1/name_2 之前, 程序会先询问。
- Model 选项卡: 座位和枪口的挂点 - 添加、命名、链接到骨骼、移动、旋转; 游戏查找的名称(*driver、*droidunit、*muzzle1..、*exhaust1..、*seat、
  *flash ...)作为带出处的模板。“作为载具检查”在缺少驾驶员座位或枪口时警告。
- Model 选项卡: 载具、步行机和机器人的自定义骨架 - 新建骨架, 添加/移动/改变父级/移除骨骼, 刚性绑定所选表面, 添加用于打关键帧的序列, 保存 .gla、
  .glm 和 animation.cfg; 全部为可撤销步骤。来自文件的骨架(_humanoid)保持不变。
- animation.cfg: 目标游戏不认识的名称(其动画表; OpenJK 1544 个, Movie Duels 3213 个)会被报告 - 游戏会默默跳过这些行。
- Clone Wars 流程(检查配置, 默认关闭): 最低级别的参考值(每个人物 500 个顶点, 武器和装备 300 个)及警告; 挂点检查始终开启; 优化器报告没有细节级别的
  运行时 .md3 以及有 _2.md3 却没有 _1.md3 的情况(ERR_DROP)。

### 日本語

- Model タブ: 詳細レベルの生成。3ds Max の ProOptimizer、Blender の Decimate と同じです(モデリング領域、モディファイアスタックの下)。ゲーム
  ファイルで測ったテンプレート(フィギュア 0.65 / 0.40、武器と装備 0.55 / 0.30、小道具 0.75 / 0.50)、2-4 レベル、元に戻すは 1 回。.glm は
  レベルをファイル内に持ち、.md3 を保存すると全フレームとタグ付きの name_1.md3 と name_2.md3 を横に書きます。粗いレベルの頂点はすべて LOD 1 の
  頂点(ウェイトと UV はファイルのまま)、面の境界は閉じたまま、タグ・_off・_cap_ の面はそのまま、各面はターゲットゲームの上限未満(.glm の最低
  レベルは影の上限未満)。簡略化は meshoptimizer(MIT)。生成しなければ何も変わらず、保存は同じバイトを書きます。
- Model タブ: 「詳細レベルとスキン」にレベルごとの頂点・三角形・サーフェス数とターゲットゲームの上限、.md3 では横の name_1.md3 / name_2.md3 も。既にある name_1/name_2 を保存で上書きする前に、プログラムが確認します。
- Model タブ: 座席と銃口のボルト - 追加、名前付け、ボーンへのリンク、移動、回転。ゲームが探す名前(*driver、*droidunit、*muzzle1..、*exhaust1..、
  *seat、*flash ...)を出典付きのテンプレートとして。「乗り物としてチェック」は運転席や銃口がないと警告します。
- Model タブ: 乗り物・歩行機・ドロイド用の独自スケルトン - 新しいスケルトン、ボーンの追加/移動/親の変更/削除、選んだ面のリジッドな結び付け、
  キー用のシーケンス追加、.gla・.glm・animation.cfg の保存。すべて元に戻せます。ファイルのスケルトン(_humanoid)は変わりません。
- animation.cfg: ターゲットゲームが知らない名前(アニメーション表; OpenJK 1544、Movie Duels 3213)を報告します - ゲームはそのような行を黙って
  飛ばします。
- Clone Wars チェーン(チェックプロファイル、既定オフ): 最低レベルの目安(フィギュア 500 頂点、武器と装備 300)と警告。ボルトのチェックは常にオン。
  最適化ツールは詳細レベルのない実行時 .md3 と、_1.md3 のない _2.md3(ERR_DROP)を報告します。

### Español

- Pestaña Model: generar niveles de detalle, como ProOptimizer en 3ds Max y Decimate en Blender (área Modelado, bajo la pila de
  modificadores). Plantillas medidas en los archivos del juego (figuras 0,65 / 0,40, armas y equipo 0,55 / 0,30, accesorios 0,75 / 0,50),
  de 2 a 4 niveles, un paso de deshacer; un .glm lleva los niveles en el archivo, guardar un .md3 escribe name_1.md3 y name_2.md3 al lado
  con todos los fotogramas y etiquetas. Cada vértice de un nivel más basto es un vértice de LOD 1 (pesos y UV exactamente como en el
  archivo), los bordes entre superficies quedan cerrados, las etiquetas y las superficies _off y _cap_ quedan como están, cada superficie
  queda por debajo de los límites del juego (el nivel más bajo de un .glm por debajo del límite de sombra). El simplificador es
  meshoptimizer (MIT). Sin Generar nada cambia y guardar escribe los mismos bytes.
- Pestaña Model: vértices, triángulos y superficies por nivel en "Nivel de detalle y skin" con los límites del juego; en un .md3 también
  los archivos name_1.md3 / name_2.md3 de al lado. Antes de que guardar sobrescriba name_1/name_2 ya existentes, el programa pregunta.
- Pestaña Model: bolts para asientos y bocas de fuego: añadir, nombrar, enlazar a un hueso, mover, girar; los nombres que busca el juego
  (*driver, *droidunit, *muzzle1.., *exhaust1.., *seat, *flash ...) como plantillas con su fuente. "Comprobar como vehículo" avisa sin
  asiento de conductor o boca de fuego.
- Pestaña Model: un esqueleto propio para vehículos, caminantes y droides: esqueleto nuevo, añadir/mover/cambiar de padre/quitar huesos,
  unir superficies elegidas de forma rígida, añadir secuencias para poner claves, guardar .gla, .glm y animation.cfg; todo como pasos de
  deshacer. Los esqueletos de archivos (_humanoid) no se tocan.
- animation.cfg: se informan los nombres que el juego no conoce (su tabla de animaciones; OpenJK 1544, Movie Duels 3213); el juego salta
  esas líneas sin aviso.
- Cadena Clone Wars (perfil de comprobación, desactivado por defecto): valor guía para el nivel más bajo (500 vértices por figura, 300 para
  armas y equipo) con avisos; la comprobación de bolts siempre activa; el optimizador informa de .md3 de ejecución sin niveles de detalle y
  de un _2.md3 sin _1.md3 (ERR_DROP).

### Čeština

- Karta Model: vytvoření úrovní detailu jako ProOptimizer v 3ds Max a Decimate v Blenderu (oblast Modelování, pod zásobníkem modifikátorů).
  Šablony změřené na souborech hry (postavy 0,65 / 0,40, zbraně a výstroj 0,55 / 0,30, rekvizity 0,75 / 0,50), 2 až 4 úrovně, jeden krok
  zpět; .glm nese úrovně v souboru, uložení .md3 zapíše vedle name_1.md3 a name_2.md3 se všemi snímky a tagy. Každý vrchol hrubší úrovně
  je vrcholem LOD 1 (váhy a UV přesně jako v souboru), hranice mezi plochami zůstanou uzavřené, tagy a plochy _off a _cap_ zůstanou, jak
  jsou, každá plocha zůstane pod limity cílové hry (nejnižší úroveň .glm pod limitem stínu). Zjednodušovač je meshoptimizer (MIT). Bez
  Vytvořit se nic nezmění a uložení zapíše tytéž bajty.
- Karta Model: vrcholy, trojúhelníky a plochy po úrovních v "Úroveň detailu a skin" s limity cílové hry, u .md3 i soubory name_1.md3 /
  name_2.md3 vedle. Než uložení přepíše už existující name_1/name_2, program se zeptá.
- Karta Model: bolty pro sedadla a ústí - přidat, pojmenovat, napojit na kost, posunout, otočit; jména, která hra hledá (*driver,
  *droidunit, *muzzle1.., *exhaust1.., *seat, *flash ...), jako šablony se zdrojem. "Kontrolovat jako vozidlo" varuje bez sedadla řidiče
  nebo ústí.
- Karta Model: vlastní kostra pro vozidla, chodce a droidy - nová kostra, přidat/posunout/přepojit/odebrat kosti, pevně navázat zvolené
  plochy, přidat sekvence ke klíčování, uložit .gla, .glm a animation.cfg; vše jako kroky zpět. Kostry ze souborů (_humanoid) zůstanou
  nedotčené.
- animation.cfg: jména, která cílová hra nezná (její tabulka animací; OpenJK 1544, Movie Duels 3213), se hlásí - hra takové řádky beze
  slova přeskočí.
- Řetězec Clone Wars (profil kontrol, ve výchozím stavu vypnuto): vodítko pro nejnižší úroveň (500 vrcholů na postavu, 300 pro zbraně a
  výstroj) s varováními; kontrola boltů vždy zapnutá; optimalizátor hlásí běhové .md3 bez úrovní detailu a _2.md3 bez _1.md3 (ERR_DROP).
