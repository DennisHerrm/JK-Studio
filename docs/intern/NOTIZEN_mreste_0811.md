# Notizen MRESTE (08.11.2026): Reste des Modell-Blocks vor rc611

Ein Agent (Auftrag `auftraege/auftrag_mreste_0811.md`), Laeufe `mr_*`, Skripte und Protokolle unter `Y:\werkzeuge\mr\`.
Basis 174d7e5b3 (karte1). kFassung bleibt rc610, nichts gepusht. Neue Texte in `tools/i18n_mreste.py`, Selbsttest `gui/selbsttest_mr.inc`.

## Punkt 1: Blender-Modus in allen Stufen

Abgleich der Aktionstabellen je Stufe (Skript `Y:\werkzeuge\mr\tastenvergleich.py`): Kurven, Rig, Painting hatten fuer jede
Max-Taste eine Blender-Taste; Modellieren hatte 12, UV 14, Figur-Animation 1 Aktion nur mit Max-Taste; Weights hatte im
Blender-Modus keine einzige Taste von Blenders Weight Paint. Quelle der Blender-Tasten: Blender 5.0
`scripts/presets/keyconfig/keymap_data/blender_default.py` (nur gelesen, Blender nicht gestartet).

- Modellieren (km_edit_mesh): Alt+Klick Kantenschleife (mesh.loop_select), Strg+Alt+Klick Kantenring (mesh.edgering_select),
  auf Vielecken Alt+Klick die Flaechenschleife, Strg+L Verbundene waehlen (mesh.select_linked, neue Aktion auch in Menue,
  Rechtsklick, Suche), J Connect (mesh.vert_connect_path), Strg+X Remove (mesh.dissolve_mode). Ohne Blender-Taste bleiben
  (Blender hat dort keine): Rand, Element, Make Planar, Collapse (M > Collapse), QuickSlice (Bisect), Target Weld, Clean Remove,
  Bevel der Vielecke - Menue, Rechtsklick, Suche und Tooltip mit Weg.
- UV (km_uv_editor): Strg+E Naht markieren (uv.mark_seam), P Pin (uv.pin), Umschalt+W Menue Ausrichten mit Begradigen und
  Ausrichten (IMAGE_MT_uvs_align). Packen, Entspannen, Dichte haben in Blender 5.0 nur in der Legacy-Belegung Tasten.
- Weights (km_weight_paint): Strg+Strich zieht ab (INVERT), Umschalt+Strich glaettet (SMOOTH), Umschalt+X Gewicht aufnehmen
  (paint.weight_sample; auch "Gewicht aufnehmen" in Suche/Rechtsklick, dann ein Klick in die Ansicht), Strg+X Gewicht auf die
  Auswahl (paint.weight_set), F / Umschalt+F / Strg+F Radius / Staerke / Gewicht ziehen (wm.radial_control: Klick uebernimmt,
  Esc/Rechtsklick bricht ab). Das Ziehen folgt der Maus in der Ansicht (der bestaetigende Klick gilt sonst nicht den Tasten).
- Zaehlung ab 1 wie die Zeitleiste: Flugphase und Warnung der Auto-Physik (Model-Tab und Mission), Spanne von "Reduzieren".
- Figur-Animation Q (Max: Auswahl-Werkzeug) bleibt ohne Blender-Taste: im Blender-Modus gibt es kein Gizmo, ein Klick waehlt immer.
- Rot `mr_bl_rot` 47 OK / 31 FEHLER (Testcode mit altem Verhalten gebaut), gruen `mr_bl_g3` 67 / 0.
  Dazwischen zwei eigene Fehler gefunden und behoben: das Ziehen F endete nicht beim Klick (der Klick setzt den Fokus erst im
  Bild), Gewicht aufnehmen traf ueber das Dreieck nicht - jetzt die Ecken wie ein Tupfer.

## Punkt 3: Chamfer mit Segmenten

- `chamferKanten(n, kanten, segmente, profil)`: jeder Streifen wird in `segmente` Vierecke geteilt, die Bogenpunkte liegen auf
  Blenders Profil (Superellipse, r = -2 ln 2 / ln profil; 0,5 Kreis, 0,25 gerade), linear im Betrag wie alle Bewegungen des Plans;
  Endflaechen, Flicken und Nachbarstreifen bekommen dieselben Bogenpunkte. Kaestchen: Segmente und Max "Tension" / Blender "Shape".
- ctest `modell_mesh`: rot (Segmentieren abgeschaltet) 4 FEHL, gruen alle ok - eine Kante 3 Segmente 14/21/9 mit 4 Punkten auf dem
  Kreis Radius = Betrag, Profil 0,25 Bogenpunkt auf der Geraden, vier Deckelkanten 12/20/10 bzw. 16/28/14, alle zwoelf Kanten
  48/84/38. Selbsttest md 86/0 (neu: Chamfer 3 Segmente ueber das Kaestchen 14/21/9, Rueckgaengig bytegleich).
- Abweichung (Teil 1, in Punkt 3b behoben): Ecken mit drei und mehr gewaehlten Kanten schlossen ein Vieleck, Profile != 0,5
  standen bei gleichen Parameterwinkeln. Chamfer an Punkten hat keine Segmente (wie Max).

## Punkt 8: teil6 3 FEHLER

- Ursache (nachgestellt `mr_teil6_rot` 13/3, Protokoll `mr_teil6_d2`): die Zielfigur steht am Startpunkt der ersten laufenden
  Figur der Grundmission md_am_sith; seit dem Wechsel der Movie-Duels-Daten ist das ani1, und ihr NPC_spawner steht genau dort.
  Das Wachgeschuetz nimmt den naechsten Feind und traf ani1 (Schmerzen bei ani1), der Knall der Kiste ebenso. Kein Fehler der
  Vorschau, ein Fehler des Tests.
- Behoben: der Test entfernt NPC_* der Grundkarte naeher als 600 Einheiten am Zielort (ani1, R2-D2) und schreibt Ziel und
  Schmerzen ins Protokoll. Gruen `mr_teil6_g3` 16 / 0.

## Punkt 4: model2 und misc_model_ghoul als .glm (Teil 2)

Quellen (OpenJK, MD9 gleich): InitMover macht ein model2 mit ".glm" (strstr, gross/klein genau) zum Ghoul2-Modell
(`g_mover.cpp:1069-1080`, codeJK2 `:1061-1073`, s.radius 120) - nur fuer func_door/plat/button/train/static/rotating/bobbing/
pendulum/wall/usable; InitBBrush (func_breakable) setzt nur s.modelindex2, CG_Mover reicht dann ein leeres ghoul2 weiter,
R_AddGhoulSurfaces zeichnet nichts. SP_misc_model_ghoul (`g_misc_model.cpp:180-216`, s.radius 50, modelscale nur ohne
modelscale_vec, Ursprung + mins[2] * (1 - Groesse)). Gezeichnet ohne Haut (Shader der Datei, `tr_ghoul2.cpp:2260-2295`), Bild 0
der GLA (`:1922-1924`), Wurzel rootMatrix (`:92-99`), RB_SurfaceGhoul (Normale nur mit dem ersten Knochen), gestellt ueber
ent.axis (CG_Mover: lerpAngles + modelAngles; CG_General: AnglesToAxis + ScaleModelAxis), LOD G2_ComputeLOD mit s.radius.
"*default" als GLA: die eingebaute FakeGLAFile (`tr_model.cpp:112-131, 155-163`).

- Neu `modell::glmStandbild` (Ghoul2 ohne Animation als starres .md3 mit einem Bild, Stufe je LOD) und `modell::standardGla`.
  Vorschau und Spielbild des Map-Tabs (kartenModelleAufstellen, `kartenModellStufe` je Bild und Sicht) und die Editor-Kamera
  (`karte::glmLesenImSpiel`, Stufe 0) zeichnen es; `modelAngles` am model2 jetzt auch in Vorschau und Spielbild (fehlte fuer
  .md3; JK2 kennt den Schluessel nicht).
- Dabei gefunden (Spielfoto): die Editor-Kamera legte das model2 im Spielbild ein zweites Mal im Editor-Licht ueber das Bild
  (A dreimal so hell wie im Spiel: Vorschau 46,3 gegen Spiel 15,0, misc_model_ghoul 14,7 gegen 14,5). Das model2 gehoert jetzt
  zur Geometrie wie die Pinsel seiner Entity (gewaehlt zusaetzlich im Rest, wie die Pinsel).
- Karten der Spiele (Y:/werkzeuge/mr/glm_karten.py): .glm als model2/misc_model_ghoul nur in JK2 (artus_mine 6 ore_cart als
  func_train, yavin_swamp glider, kejim_base comlink/crystal als misc_model_ghoul); in JKA und Movie Duels keine.
- ctest `modell_dokument` + 8 Pruefungen (Kette, Gewichte, Normale, Wurzel, OFF/Tag, Knochenzahl, ohne GLA, FakeGLAFile),
  235/235.
- Selbsttest `karteneditor` mit `BHED_KARTE_NUR=rdglm` (turret_canon.glm: A func_static model2, B misc_model_ghoul,
  C func_breakable model2): rot `mr2_glm_rot4` 12/2 (Editor A 0,000, Spielbild A 0,000 - nicht gezeichnet), Zwischenstand
  `mr2_glm_g2` (Spielbild A doppelt, 46,3 gegen 14,7), gruen `mr2_glm_g4` 15/0 (Editor A 0,502, Spielbild A 0,334 / B 0,292,
  Helligkeit 18,6 / 17,8, C 0), Schlussstand `mr2_glm_g5` 15/0; `rd` (model2 .md3) `mr2_rd` 20/0.
- Spiel gegen Vorschau: dieselbe Karte mit bhed_q3map2 gebaut, Movie Duels per Skriptkamera am Augpunkt der Editor-Kamera
  (`Y:/werkzeuge/mr/spiel_glm.py`, Spielordner unveraendert): Lage und Groesse gleich (A x 465-633 / 464-634, B 1372-1540 in
  beiden, Hoehe 86 / 88 Bildpunkte), Haltung (Bild 0, Wurzel) gleich, C im Spiel ebenfalls unsichtbar, Helligkeit der Modelle
  Spiel 15,0 / 14,5, Vorschau 15,2 / 14,7 (vor der Behebung des Doppelzeichnens A 46,3). Paarbild
  `Y:/werkzeuge/mr/p4_paar_spiel_vorschau.png` (oben Spiel 1920x1440, unten Spielbild 1920x1080; `_hell4` vierfach aufgehellt).
- Bildzeit allein (`leistung`, BHED_D3D_DEBUG=0, CPU-Mittel ms, vorher = exe von Teil 1 / nachher `mr2_leistung_neu2`): Mission
  duel1 ruhend 0,35 / 0,42, md_toaj ruhend 0,50 / 0,63, Szene spielt 3,71 / 3,57, duel_mustafar 1,08 / 1,10, Map Editor
  Vorlage 2,25 / 2,27, md_am_sith 1,61 / 1,62; der unberuehrte Menue-Editor schwankt ebenso (0,42 / 0,49), also Rauschen. Die
  LOD-Wahl gibt eine Zahl zurueck und baut nur fuer Stufe > 0 einen Schluessel (keine Kopie je Modell und Bild).
- Nicht eigens fotografiert: die Mission-Vorschau (gleicher Aufstell-Code kartenModelleAufstellen/kartenModellStufe wie das
  Spielbild, eigene Zeichenschleife); misc_model/misc_model_static liegen im Spielbild weiter zusaetzlich im Editor-Licht
  darueber, wenn der Filter "Models" nicht aus ist (Altbestand, nicht Teil des Punkts).
- Erst gelernt: ein "origin"-Schluessel ohne Ursprungs-Brush verschiebt die Brushes beim Bau nicht - das Spiel stellt das
  Brush-Modell dann ausserhalb der Karte auf und zeichnet weder Brush noch model2 (der Test nimmt einen Ursprungs-Brush).

## Punkt 3b: Chamfer wie Blender (Teil 3)

Quelle: Blender 5.0 `source/blender/bmesh/tools/bmesh_bevel.cc` (Tag v5.0.0, GPL-2.0-or-later, nur gelesen; Blender nicht
gestartet), Kopie `Y:/werkzeuge/mr/blender_src/bmesh_bevel_50.cc`. Portiert in die neue Datei
`src/modell/wof/fase_blender.cpp` (GPL-Kopf, THIRD-PARTY.md); die Netz-Seite bleibt in `mesh_werkzeuge.cpp`.

- Profil je Streifenende wie `set_profile_params` (:1901): die Mitte ist der naechste Punkt der Kante zur Geraden Start-Ende
  (project_to_edge), bei kollinearem Start-Mitte-Ende v bzw. der Treffpunkt der versetzten Nachbarlinien (selcount >= 3);
  vorher lag die Mitte immer auf v - an einer Wuerfelecke lag der Bogen damit schraeg (Rotlauf: 3 von 9 Punkten auf der
  Kugel). Profil `superR` wie :7890/:7928 (>= 0,95 Rechteck), gleiche Sehnen `find_even_superellipse_chords` (:7403,
  :7307 mit find_superellipse_chord_endpoint), seg2 = Zweierpotenz >= seg (mindestens 4) fuer das Gitter.
- Zwei gefaste Kanten ohne Ecke (selcount 2, zwei Ringpunkte, z. B. die Deckelkanten): beide Profile auf die Ebene durch
  v und die Ringpunkte (move_weld_profile_planes :2067), die Punkte gemittelt (build_vmesh :6150).
- Ecke mit Ring >= 3 und selcount >= 2: Gitter wie `bevel_build_rings` (:5517): pipe_test -> pipe_adj_vmesh (:4829,
  snap_to_pipe_profile), tri_corner_test == 1 -> tri_corner_adj_vmesh (Wuerfelecke make_cube_corner_adj_vmesh :4550 mit
  kubischer Teilung, interp_vmesh, snap_to_superellipsoid, make_unit_cube_map), sonst adj_vmesh (:4714, Startmitte mit
  find_profile_fullness, cubic_subdiv :4296 mit sabin_gamma). Vierecke (i,j,k)..(i,j+1,k) je Randpunkt, bei ungerader
  Segmentzahl ein n-Eck in der Mitte; die Randpunkte der Ecke kommen aus dem Gitter (wie Blender: build_rings kopiert
  j = 0 aus vm1). tri_corner_test braucht die Kantenwinkel (BM_edge_calc_face_angle_signed_ex, hohl = die andere
  Flaeche vor der Ebene der ersten). Reihenfolge der Randpunkte gegen den Uhrzeiger wie Blender (unsere Vielecke laufen im
  Uhrzeiger: umgekehrt). Alles im Raum "Betrag 1"; jede Formel ist homogen um v, der Plan bewegt linear.
- ctest `modell_mesh`: rot `Y:/werkzeuge/mr/p3b/ct_rot.txt` (Testcode gegen den alten Stand gebaut) 7 FEHL, gruen
  `ct_g1.txt` 0 FEHL. Zahlen aus Blenders Regeln: alle zwoelf Kanten ns 2 56/108/54 (je Ecke n (ns/2)^2 = 3 Vierecke,
  3 + 3 (ns - 1) + 1 = 7 Punkte), ns 3 96/192/98 (6 Vierecke + Dreieck, 12 Punkte), ns 4 152/300/150 (12 Vierecke, 19 Punkte),
  bei ns 3 und 4 alle Punkte einer Ecke auf der Kugel r = Betrag um v - Betrag (1,1,1); Profil 0,7 drei gleiche Sehnen auf
  u^r + w^r = 1 (vorher 0,1407 / 0,0647 / 0,1407); Kreuzung im 2x2-Kasten (pipe) 46/84/40 eben; Spitze einer hohen
  dreiseitigen Pyramide (adj_vmesh, n = 3) ns 2 16/27/13 mit Mitte auf der Achse, ns 3 24/42/20. Selbsttest md `mr3_md` 86/0.
- Abweichungen: square_out_adj_vmesh (Profil >= 0,95 an Ecken mit selcount >= 3 und gerader Segmentzahl, keine
  Wuerfelecke) nicht portiert - dort adj_vmesh mit Rechteckprofil; Profil unter 0,01 zaehlt als 0,01 (Blender: bei 0
  build_square_in_vmesh); die Ecken-Daten (UV, Normale) der Gitterpunkte nach Abstand zu den Ringpunkten gemischt
  (Blender: interpoliert in einer Vertreter-Flaeche); Blenders Startpunkt des Rings (boundstart) ist nicht bekannt - bei
  Segmentzahlen ohne Zweierpotenz kann interp_vmesh dadurch winzig anders runden. Enden einer einzelnen gefasten Kante an
  Punkten mit vier und mehr Kanten (Blender M_POLY/M_TRI_FAN) bleiben ein Vieleck mit den Bogenpunkten.

## Altbestand aus Punkt 4 (Teil 3)

- misc_model/misc_model_static im Spielbild ("Built map as in the game") lagen mit Filter "Models" an ein zweites Mal im
  Editor-Licht ueber der gebauten Karte. Jetzt gehoert das Modell von misc_model (von q3map2 eingebacken) und misc_model_*
  (vom Spiel aufgestellt, dieselbe Regel wie kartenModelleAufstellen) zur Geometrie der Kamera wie das model2 und die Pinsel;
  der Rest (Spielbild) zeichnet es nur gewaehlt in der Auswahlfarbe (`kameraszene.cpp`, auch das gewaehlte model2 nicht mehr
  im Editor-Licht). Selbsttest `karteneditor` BHED_KARTE_NUR=rdglm um D (misc_model_static) und E (misc_model,
  crate_open.md3) erweitert, Filter Models an und erst nach dem neuen Schnell-Backen gemessen: rot `mr3_glm_rot2` 17/1
  (D 42,6 gegen 26,8 ohne Filter), gruen `mr3_glm_g4` 19/0 (alle Helligkeiten gleich). Ein erster Rotlauf `mr3_glm_rot` mass
  ohne das Abwarten des Backens (Editor-Geometrie statt Spielbild) und zaehlt nicht.
- Movie Duels "model2scale" (G_SpawnModel2Scale, MD9 `code/game/g_mover.cpp:1083-1097`, aus InitMover ohne .glm und
  InitBBrush; CG_Mover skaliert die Achsen, `cg_ents.cpp:1719-1727`): Prozent, nur > 0, hoechstens 1023; nur im
  Movie-Duels-Regelwerk (OpenJK-SP und JK2 kennen den Schluessel nicht). `karte::model2Groesse`, benutzt von
  kartenModelleAufstellen (Vorschau und Spielbild) und der Editor-Kamera. Selbsttest neu BHED_KARTE_NUR=rdm2 (zweiter Wuerfel
  mit model2scale 50): rot `mr3_m2_rot` 13/1 (Rot 1,000), gruen `mr3_m2_g1`/`mr3_m2_g2` 14/0 (Editor und Spielbild 0,465;
  Erwartung hoechstens 0,58 aus Perspektive und innerem Rechteck).
- Mission-Vorschau mit .glm-model2: dieselbe gebaute Karte (Schnell-Backen) in der Mission-Vorschau, Kamera wie die
  Editor-Kamera; Pruefung A und B als Ghoul2 aufgestellt (rdglm). Foto `Y:/werkzeuge/mr/fotos_glm/rd_glm_mission.png`
  (2355 x 1498, Lauf `mr3_glm_foto2` mit BHED_GLM_FENSTER=3840x2160, angesehen): A und B gleich gross in derselben Haltung,
  C fehlt wie im Spiel, D und E da. Die LOD-Wahl je Sicht (Spiegel/Portal) laeuft ueber kartenModellStufe mit der Kamera
  der jeweiligen Sicht; ein Spiegel ist in dieser Karte nicht fotografiert.
- Offen: misc_model_ghoul zeichnet die Editor-Kamera im Editor-Licht nicht (B 0,000 im Editor, Altbestand aus Punkt 4).

## Painting: Funde der Pruefung seit rc610 (Teil 3)

Quelle `JKStudio_Uebergabe/plaene/codepruefung_seit_rc610.md` (PRUEF3). Neue Selbsttest-Gruppe `modellblock` BHED_MODELL_NUR=ptfix,
ctest `modell_malen` erweitert (`ebenenGrenzeTest`).

- P1 (H): die Leinwaende gehoeren zum Modell (Schluessel nur der Pfad), nicht zu Detailstufe oder Haut; bei einem Wechsel der
  Stufe holen sie nur ihre Dreiecke neu (`dreieckeAus`, wie bei neuem Netz). `modellUngespeichert()` zaehlt bemalte, nicht
  gebaute Bilder mit (`modellMalenUngespeichert`: `l.stand != l.gespeichert`) - Beenden und anderes Modell fragen dann wie beim
  Dokument. Rot `mr3_ptfix_rot2` (alle vier Pruefungen FEHLER), gruen `mr3_s_ptfix` 5/0: "ungespeichert nein" nach 2629 bemalten Texeln, nach LOD 2 "0 Texel bemalt".
- P4 (M): jeder Schritt traegt eine fortlaufende Nummer ueber alle Leinwaende (`UndoSchritt::nummer`); Strg+Z nimmt den
  juengsten aller Bilder, Strg+Y den zuletzt zurueckgenommenen; im Bereich Painting nie mehr auf das Modelldokument durch.
  Rot: Helm 0, Rumpf 2629 Texel nach sechsmal Strg+Z.
- P3 (M): der Deckkraft-Regler merkt den alten Wert vor `SliderFloat` (ImGui setzt ihn schon im Klickbild). Rot: nach Klick
  auf die Reglermitte und Strg+Z 0,5 statt 1,0.
- P2 (M): Ebenen-Schritte ohne neue Bildpunkte (Sichtbar, Mischart, Deckkraft, Name) merken nur die Eigenschaften; jeder
  Schritt (auch Neu/Loeschen/Umbacken) wird auf `grenzeBytes` gekuerzt (`schrittAblegen`). ctest `modell_malen` rot (3 Fehler): 50 x Sichtbar 106 MB,
  10 x Neue Ebene 151 MB bei 4 MB Grenze; gruen in der Grenze, Rueckgaengig stellt Sichtbarkeit her, Bildpunkte gleich.
- P6 (M, Tempo): der Vergleich "bemalt" mit dem Original nur bei geaendertem `l.stand` (je Leinwand gemerkt) - gleiche Anzeige.
- P15 (N, Tempo): Mitte und Umkreis je Dreieck einmal in `dreieckeSetzen` (`Leinwand::kugeln`), dieselbe Rechnung wie vorher
  je Tupfer - Treffer bytegleich (pt gruen).
- P12 (N): Kopf von `fuellen` berichtigt (fuellt im ganzen Bild, auch ueber die UV-Inseln hinaus; die Maske schuetzt).
- Nicht bearbeitet (Kontext): P7, P8, P13, P19.

## Schluss fuer die Punkte 1, 3, 8 (Laeufe `mr_s_*`)

- Sauberer Neubau `build-test` (--clean-first) 0 Warnungen, 12 min 24 s; ctest 235/235.
- Modell-Block: blender 67/0, md 86/0, uv 50/0 (die alte Erwartung "Naht markieren ohne Blender-Taste" auf Strg+E nachgezogen),
  mb2 37/0, mb5 159/0, mb7 24/0, ph 29/0, rig 26/0, rt 19/0, tasten 43/0, funktionen 60/0, pfix 17/0; modell 14/0, kuerzel 202/0;
  teil6 16/0. Lints wie die Basis (portability 99, pruefcodes 11, fremdprogramm 1 bekannt), i18n ok (der verwaiste Text
  MbGwTastenBlender entfernt); Handbuch `--pruefen --pflicht=es --pflicht=cs` ok.
- Nicht gelaufen (gehoeren in die Schlussrunde des ganzen Auftrags): Modus 1, karteneditor voll, mission1, Handbuchbilder.

| modellleistung (allein, BHED_D3D_DEBUG=0, Bild-CPU Mittel / p99 ms) | vorher (exe des Rotlaufs) | nachher |
|---|---|---|
| ohne Figur ruhig | 0.13 / 0.32 | 0.11 / 0.27 |
| stormtrooper Animation | 0.42 / 0.82 | 0.40 / 0.82 |
| stormtrooper Knochen und Animation | 0.48 / 0.91 | 0.43 / 0.73 |
| jedi_hf Animation | 1.00 / 2.31 | 0.96 / 1.30 |

## Schluss Teil 3 (Laeufe `mr3_s_*`, Summe `Y:/werkzeuge/mr/schluss3_summe.txt`)

- Bau 0 Warnungen (inkrementell nach den Kopfdateien, kein --clean-first in Teil 3); ctest 235/235; parallel: Modus 1 2635/0,
  modellblock ptfix 5/0, pt 41/0, md 86/0; karteneditor rdglm 19/0, rdm2 14/0, rd 21/0, r3 22/0 (r3 misst das Spielbild mit den
  Modellen des Editors darueber - nach der Behebung weiter gruen). Lints wie die Basis (portability 99, pruefcodes 11,
  fremdprogramm 1 bekannt), i18n ok.
- Bildzeit nicht gemessen: waehrend der Schlussrunde liefen fremde Baue (cl.exe/cmake anderer Agenten) - offen, allein
  nachholen (`leistung` fuer Editor-Kamera und Vorschau, `ptzeit` fuer P6/P15).
- Offen aus Punkt 2 (Painting): Paarbild Spiel gegen Vorschau mit gleicher Haltung und Kamera (ICARUS-Skriptkamera in Movie
  Duels) nicht gemacht; "alle Shader-Stufen in 3D" nicht eigens geprueft (REST Punkt 1: die Stufen mit dem bemalten Bild
  bekommen ihre Kopie, pt "PT Grafikkarte" prueft Flaeche und zwei Shader-Stufen). Funde P7, P8, P13, P19 offen.

## Offen (nicht bearbeitet, Kontext)

- Siehe Teil 3 (Abschnitte unten bzw. oben je Punkt).

## Changelog

## English

- Model tab, Blender mode: modelling takes Blender's keys Alt+click (edge loop), Ctrl+Alt+click (edge ring), Ctrl+L (select linked), J (connect) and Ctrl+X (dissolve).
- Model tab, Blender mode: the UV editor takes Ctrl+E (mark seam), P (pin) and Shift+W (align menu).
- Model tab, Blender mode: weight painting takes Blender's keys - Ctrl and Shift strokes subtract and smooth, Shift+X samples the weight, Ctrl+X sets it on the selection, F, Shift+F and Ctrl+F drag radius, strength and weight.
- Model tab: Chamfer has Segments and Tension (Blender: Shape) for rounded edges.
- Map preview and map editor: Ghoul2 models (.glm) as "model2" of a brush entity and as misc_model_ghoul are drawn like in the game (first frame of their animation, level of detail by distance); "modelAngles" now turns a model2 in the preview too, and the game view no longer draws a model2 a second time in editor light.
- Model tab: Chamfer with segments fills corners where three or more chamfered edges meet with a grid of quads like Blender's bevel, places the round edge in the plane across the edge and spaces other shapes in equal steps.
- Map editor: in "Built map as in the game" misc_model and misc_model_static are no longer drawn a second time in editor light; Movie Duels maps scale a model2 by "model2scale" in the preview, the game view and the editor camera.
- Painting: painted images are no longer lost when the level of detail or the skin changes, and closing or loading another model asks about unbuilt paintings; Ctrl+Z/Ctrl+Y step through the strokes of all images and never undo a model edit in the Painting area; undoing an opacity click restores the old value; layer steps stay within the undo memory limit.

## Deutsch

- Model-Tab, Blender-Modus: Modellieren nimmt Blenders Tasten Alt+Klick (Kantenschleife), Strg+Alt+Klick (Kantenring), Strg+L (Verbundene wählen), J (Verbinden) und Strg+X (Auflösen).
- Model-Tab, Blender-Modus: der UV-Editor nimmt Strg+E (Naht markieren), P (Pin) und Umschalt+W (Menü Ausrichten).
- Model-Tab, Blender-Modus: Weights nimmt Blenders Tasten - Striche mit Strg und Umschalt ziehen ab und glätten, Umschalt+X nimmt das Gewicht auf, Strg+X setzt es auf die Auswahl, F, Umschalt+F und Strg+F ziehen Radius, Stärke und Gewicht.
- Model-Tab: Chamfer hat Segmente und Spannung (Blender: Form) für runde Kanten.
- Kartenvorschau und Karten-Editor: Ghoul2-Modelle (.glm) als „model2“ einer Brush-Entity und als misc_model_ghoul werden wie im Spiel gezeichnet (erstes Bild ihrer Animation, Detailstufe nach Abstand); „modelAngles“ dreht ein model2 jetzt auch in der Vorschau, und das Spielbild zeichnet ein model2 nicht mehr ein zweites Mal im Editor-Licht.
- Model-Tab: Chamfer mit Segmenten füllt Ecken, an denen drei oder mehr gefaste Kanten zusammenkommen, mit einem Gitter aus Vierecken wie Blenders Bevel, legt die Rundung in die Ebene quer zur Kante und verteilt andere Formen in gleichen Schritten.
- Karten-Editor: in „Gebaute Karte wie im Spiel“ werden misc_model und misc_model_static nicht mehr ein zweites Mal im Editor-Licht gezeichnet; Movie-Duels-Karten skalieren ein model2 mit „model2scale“ in der Vorschau, im Spielbild und in der Editor-Kamera.
- Painting: bemalte Bilder gehen beim Wechsel der Detailstufe oder der Haut nicht mehr verloren, Schließen und ein anderes Modell fragen nach nicht gebauten Bildern; Strg+Z/Strg+Y gehen durch die Striche aller Bilder und nehmen im Bereich Painting nie eine Modelländerung zurück; Rückgängig nach einem Klick auf die Deckkraft stellt den alten Wert her; Ebenen-Schritte bleiben in der Speichergrenze des Rückgängig.

## 中文

- 模型选项卡, Blender 模式: 建模接受 Blender 的按键 Alt+单击 (边循环)、Ctrl+Alt+单击 (边环)、Ctrl+L (选择相连)、J (连接) 和 Ctrl+X (融并)。
- 模型选项卡, Blender 模式: UV 编辑器接受 Ctrl+E (标记接缝)、P (固定) 和 Shift+W (对齐菜单)。
- 模型选项卡, Blender 模式: 权重绘制接受 Blender 的按键——按住 Ctrl 和 Shift 绘制为减去和平滑, Shift+X 采样权重, Ctrl+X 设置到所选, F、Shift+F 和 Ctrl+F 拖动半径、强度和权重。
- 模型选项卡: 倒角有分段和张力 (Blender: 形状), 可做圆角边。
- 地图预览和地图编辑器: 作为画刷实体 "model2" 和 misc_model_ghoul 的 Ghoul2 模型 (.glm) 按游戏方式绘制 (动画第一帧, 按距离选择细节级别); "modelAngles" 现在在预览中也会旋转 model2, 游戏画面不再以编辑器光照再绘制一次 model2。
- 模型选项卡: 带分段的倒角在三条或更多倒角边相交的角落像 Blender 的倒角一样用四边形网格填充, 圆弧位于垂直于边的平面内, 其他形状等距分布。
- 地图编辑器: 在"按游戏方式显示构建的地图"中 misc_model 和 misc_model_static 不再以编辑器光照再绘制一次; Movie Duels 地图在预览、游戏画面和编辑器相机中按 "model2scale" 缩放 model2。
- 绘制: 切换细节级别或皮肤时不再丢失已绘制的图像, 关闭或加载其他模型时会询问未构建的绘制; Ctrl+Z/Ctrl+Y 按顺序撤销/重做所有图像的笔画, 在绘制区域中不会撤销模型修改; 单击不透明度后撤销会恢复旧值; 图层步骤保持在撤销内存上限内。

## 日本語

- モデルタブ、Blender 風: モデリングで Blender のキー Alt+クリック (辺ループ)、Ctrl+Alt+クリック (辺リング)、Ctrl+L (リンク選択)、J (接続)、Ctrl+X (溶解) が使えます。
- モデルタブ、Blender 風: UV エディターで Ctrl+E (シーム)、P (ピン)、Shift+W (整列メニュー) が使えます。
- モデルタブ、Blender 風: ウェイトペイントで Blender のキーが使えます - Ctrl と Shift のストロークで減算とスムーズ、Shift+X でウェイト取得、Ctrl+X で選択に設定、F・Shift+F・Ctrl+F で半径・強さ・ウェイトをドラッグ。
- モデルタブ: 面取りにセグメントとテンション (Blender: シェイプ) が付き、丸い辺を作れます。
- マッププレビューとマップエディター: ブラシエンティティの「model2」と misc_model_ghoul の Ghoul2 モデル (.glm) をゲームと同じく描画します (アニメーションの最初のフレーム、距離による詳細度)。「modelAngles」はプレビューでも model2 を回転させ、ゲーム画面で model2 がエディター照明で二重に描かれなくなりました。
- モデルタブ: セグメント付きの面取りは、面取りした辺が 3 本以上集まる角を Blender のベベルと同じく四角形の格子で埋め、丸みを辺に垂直な面に置き、他の形は等間隔に並べます。
- マップエディター: 「ゲーム同様のビルド済みマップ」で misc_model と misc_model_static がエディター照明で二重に描かれなくなりました。Movie Duels のマップはプレビュー、ゲーム画面、エディターカメラで model2 を「model2scale」で拡大縮小します。
- ペイント: 詳細度やスキンを切り替えても描いた画像が失われなくなり、閉じるときや別のモデルを読み込むときに未ビルドの画像について確認します。Ctrl+Z/Ctrl+Y はすべての画像のストロークを順にたどり、ペイント領域でモデルの変更を取り消すことはありません。不透明度をクリックした後の元に戻すで以前の値に戻ります。レイヤー操作は元に戻すメモリの上限内に収まります。

## Español

- Pestaña Model, modo Blender: el modelado acepta las teclas de Blender Alt+clic (bucle de aristas), Ctrl+Alt+clic (anillo de aristas), Ctrl+L (seleccionar vinculado), J (unir) y Ctrl+X (disolver).
- Pestaña Model, modo Blender: el editor UV acepta Ctrl+E (marcar costura), P (fijar) y Mayús+W (menú Alinear).
- Pestaña Model, modo Blender: los pesos aceptan las teclas de Blender: trazos con Ctrl y Mayús restan y suavizan, Mayús+X muestrea el peso, Ctrl+X lo aplica a la selección, F, Mayús+F y Ctrl+F arrastran radio, intensidad y peso.
- Pestaña Model: Chamfer tiene Segmentos y Tensión (Blender: Forma) para aristas redondeadas.
- Vista previa del mapa y editor de mapas: los modelos Ghoul2 (.glm) como "model2" de una entidad de brush y como misc_model_ghoul se dibujan como en el juego (primer fotograma de su animación, nivel de detalle según la distancia); "modelAngles" ya gira un model2 también en la vista previa, y la vista de juego ya no dibuja un model2 por segunda vez con la luz del editor.
- Pestaña Model: Chamfer con segmentos rellena las esquinas donde se juntan tres o más aristas biseladas con una rejilla de cuadriláteros como el bisel de Blender, coloca el redondeo en el plano transversal a la arista y reparte las demás formas a pasos iguales.
- Editor de mapas: en "Mapa compilado como en el juego" misc_model y misc_model_static ya no se dibujan por segunda vez con la luz del editor; los mapas de Movie Duels escalan un model2 con "model2scale" en la vista previa, la vista de juego y la cámara del editor.
- Painting: las imágenes pintadas ya no se pierden al cambiar el nivel de detalle o la piel, y cerrar o cargar otro modelo pregunta por las pinturas sin construir; Ctrl+Z/Ctrl+Y recorren los trazos de todas las imágenes y en el área Painting nunca deshacen un cambio del modelo; deshacer tras un clic en la opacidad restaura el valor anterior; los pasos de capas respetan el límite de memoria del deshacer.

## Čeština

- Karta Model, režim Blender: modelování přijímá klávesy Blenderu Alt+klepnutí (smyčka hran), Ctrl+Alt+klepnutí (prstenec hran), Ctrl+L (vybrat propojené), J (spojit) a Ctrl+X (rozpustit).
- Karta Model, režim Blender: editor UV přijímá Ctrl+E (označit šev), P (připíchnout) a Shift+W (nabídka Zarovnat).
- Karta Model, režim Blender: váhy přijímají klávesy Blenderu - tahy s Ctrl a Shift odečítají a vyhlazují, Shift+X odebere váhu, Ctrl+X ji nastaví na výběr, F, Shift+F a Ctrl+F táhnou poloměr, sílu a váhu.
- Karta Model: Chamfer má segmenty a napětí (Blender: tvar) pro zaoblené hrany.
- Náhled mapy a editor map: modely Ghoul2 (.glm) jako „model2“ entity ze štětců a jako misc_model_ghoul se kreslí jako ve hře (první snímek animace, úroveň detailu podle vzdálenosti); „modelAngles“ nyní otáčí model2 i v náhledu a herní pohled už nekreslí model2 podruhé ve světle editoru.
- Karta Model: Chamfer se segmenty vyplní rohy, kde se setkají tři a více zkosených hran, mřížkou čtyřúhelníků jako zkosení v Blenderu, klade zaoblení do roviny napříč hranou a ostatní tvary rozloží ve stejných krocích.
- Editor map: v „Sestavená mapa jako ve hře“ se misc_model a misc_model_static už nekreslí podruhé ve světle editoru; mapy Movie Duels zvětšují model2 podle „model2scale“ v náhledu, herním pohledu i kameře editoru.
- Painting: pomalované obrázky se už neztratí při změně úrovně detailu nebo skinu, zavření a načtení jiného modelu se ptá na nesestavené malby; Ctrl+Z/Ctrl+Y procházejí tahy všech obrázků a v oblasti Painting nikdy nevrátí změnu modelu; zpět po klepnutí na krytí obnoví starou hodnotu; kroky vrstev zůstávají v paměťovém limitu historie.
