# Notizen REST5: Chamfer-Restabweichungen, reste1-Beenden, Modus-1-Wackler, duel_exegol (09.11.2026, `Y:\karte`, Zweig `karte1`, Basis 50ca5d8c0)

Build `Y:/karte/build-test` (`--parallel 8 -- /nodeReuse:false`), Laeufe `r5_*` unter `Y:\karte\laeufe`, Skripte und Protokolle
`Y:\werkzeuge\r5\`. Quelle Chamfer: Blender 5.0 `source/blender/bmesh/tools/bmesh_bevel.cc` (Kopie
`Y:/werkzeuge/mr/blender_src/bmesh_bevel_50.cc`), GPL-2.0-or-later; Blender nicht gestartet.

## Punkt 1: Chamfer wie Blender (`src/modell/wof/mesh_werkzeuge.cpp`, Blender-Teile)

Rot/Gruen: ctest `modell_mesh`, neuer Block `chamferRest5Test` (vier Pruefungen). Rot mit dem alten Quelltext
(`Y:\werkzeuge\r5\rot_modell_mesh.txt`) 4 FEHL, Gruen 0 Fehler (`gruen_modell_mesh.txt`).

- Genau eine ungewaehlte Kante zwischen zwei gefasten (build_boundary :3219 / :3230, offset_on_edge_between :1788): die Mitte
  der zwei Treffpunkte auf ihr statt der WL-Regel (naehere Versetzung). Test: 2x2-Kasten, Nachbar -x nach (-0,5 / 0,25)
  geschoben, gefast +x und die Kante zu ihm: Punkte auf +y/-y bei 0,10590 (= 0,1 * (1 + Wurzel 5 / 2) / 2), vorher 0,1.
- bevel_merge_uvs mit den Eimern von update_uv_vert_map (:676) und determine_uv_vert_connectivity (:764): die alten Ecken am
  Punkt kommen in BM_LOOPS_OF_VERT-Reihenfolge in den ersten Eimer mit einer Ecke innerhalb STD_UV_CONNECT_LIMIT. Eine neue
  Ecke kommt in den Eimer der ersten Ecke mit derselben Vertreter-Flaeche oder mit verbundener alter Ecke - beide Bedingungen
  haengen nur an der Vertreter-Flaeche und sind zusammen eine Aequivalenz, die Eimer sind also genau die Klassen "Eimer der
  alten Ecke der Vertreter-Flaeche" (unabhaengig von der Reihenfolge). Vorher paarweise Vereinigung (transitiv, 1~3, 3~2).
  Test: Wuerfelecke, alte UV am Punkt u, u+0,00016, u+0,00008 -> Eimer {1,3}, {2}; am neuen Punkt zwischen 2. und 3. Flaeche
  bleiben beide UV getrennt (vorher gemittelt, 2,9134 0,2717 in beiden).
- Faecher (bevel_build_trifan :5989, BM_face_split -> bmesh_kernel_split_face_make_edge): je Schnitt zwei neue Ecken, die die
  Daten ihrer Vorlage kopieren und in keinem Eimer liegen (BM_face_split registriert keine UVFace) - sie behalten die UV vor
  dem Mitteln. Mit L0..L(n-1) ab boundstart und l_fan = L(n-1): Dreieck aus L(m-1), L(m) hat L(m-1) aus dem Vieleck, L(m) als
  Kopie (ausser m = n-2), die Spitze nur bei m = 1 aus dem Vieleck. Die Faecher-Ecken mitteln nicht mehr selbst mit (vorher
  zaehlten sie als eigene Glieder und verschoben den Mittelwert). Test: 2x2-Kasten, obere Randkante, 2 Segmente: der
  Profilpunkt hat in den zwei Dreiecken verschiedene UV, genau eine davon wie die Streifen (vorher beide gleich).
- Randpunkte ab Blenders boundstart (vm->boundstart :492, efirst = next_bev :596 nach bevel_vert_construct /
  fast_bevel_edge_order :6337 und dem Umdrehen :6561): das Blender-Netz ist dieses mit umgedrehter Umlaufrichtung (passt zu
  bev_ccw_test), die Kanten in der Reihenfolge von n.kanten (BM_mesh_bm_from_me, bmesh_disk_edge_append). efirst ist die erste
  gewaehlte Kante, ausser ihr Nachbar gegen den Uhrzeiger kommt in BM_EDGES_OF_VERT vor dem anderen und der andere ist gewaehlt.
  info.rand und das Eckvieleck (bevel_build_poly :5911) beginnen dort; damit stimmen face_arr[0] (angehaengte Vertreter-Flaeche
  ohne Naht) und find_face_internal_boundverts (die ersten drei inneren Randpunkte). Test: 2x2-Kasten, drei von vier Kanten der
  Mitte, alle vier Lagen: erste Ecke des Dreiecks = Randpunkt rechts von efirst (vorher 3/4, "frei 2: 28 statt 27").

## Punkt 2: reste1 "Programm laeuft nach No weiter"

- Altbestand: schon im Lauf `zs_s_ke_reste1` (exe vom 08.11. 00:36, vor der Basis 8a8dd88e9 vom 08.11. 20:13) dasselbe.
- Ursache im Test: der letzte Schritt "Filter zurueck, Echtzeit-Licht aus, neue Karte" rief ke::neueKarte() mit der durch den
  fx_runner geaenderten Karte - neueKarte fragt dann "New Map" (mitRueckfrage) und laesst die Frage offen. Beim Beenden wartet
  karteneditor::beendenFrage auf diese Frage, antworteNein kennt nur die Fragen des Programms (askSaveOpen, frageOffen) - nach
  60 Bildern meldet der Test "laeuft nach No weiter". Das Programm verhaelt sich richtig (eine offene Kartenfrage blockiert das
  Beenden, sie bleibt sichtbar).
- Behoben im Test: vor neueKarte alsGespeichertMarkieren (wie selbsttest_editorportal.inc). Dazu nennt die Fehlermeldung jetzt
  eine offene Frage des Map Editors mit Titel.
- Rot `r5_reste1`: "Beenden: Programm laeuft nach "No" weiter (offene Frage im Map Editor: "New Map")"; Gruen `r5_reste1g`: 13 OK, 0 FEHLER, das Programm endet nach "No".

## Punkt 3: Modus 1 "Reste1/5: Map-Tab ohne Karte ruht"

- Der eine Fehlschlag (`r2_s_modus1`, REST2-Schlussrunde unter Volllast): 253 Bilder in 3,49 s, mapDirty 0, Karte 0; in allen
  anderen 40 Protokollen 13 Bilder in 3,4 s. Nicht nachgestellt: 10 Laeufe `reiter` parallel (`r5_reiter1..10`) und 8 mit
  BHED_TEST_BREMSE 20/50/100/200 (`r5_rb*`) - Reste1/5 immer 11-13 Bilder (die Bremse von 50+ laesst andere, zeitabhaengige
  Tour-Pruefungen fallen; das ist die kuenstliche Bremse, nicht Reste1/5).
- Ursache nicht belegt. Das Programm zaehlt jetzt, warum die Hauptschleife nicht ruht (App::ruheSperre: Fensternachricht,
  Selbsttest, Abspielen, Karte neu, Tongeraet, Model-Tab, Ton, Tour, Kamera der Karte, q3map2, Textfeld, Maustaste;
  darfRuhen und main_win32), und Reste1/5 schreibt die Gruende ins Protokoll.
- Robuster ohne Aufweichen: statt fester 2 s Pause wartet der Test, bis das Programm eine volle Sekunde lang durch nichts vom
  Ruhen abgehalten wird (hoechstens 30 s; danach wird trotzdem gemessen), misst dann 2 s mit derselben Grenze (8 Bilder/s).
  Weckten nur Fensternachrichten (von aussen) und kein Grund des Programms, wird die Messung bis zu dreimal wiederholt; ein
  Grund aus darfRuhen schlaegt sofort fehl.
- Gruen unter Last: 6 Laeufe `reiter` parallel mit BHED_TEST_BREMSE 15-90 (`r5_rg1..6`) neben zwei Vorschau-Laeufen: Reste1/5
  immer gruen, 7-8 Bilder in 2,1-2,2 s, "nicht geruht wegen: -". (rg3-rg6 fallen bei Bremse 45+ in den zeitabhaengigen
  Tour-Klicks, wie schon ohne die Aenderung in `r5_rb*` - Folge der kuenstlichen Bremse.) Der volle Modus 1 der Schlussrunde
  laeuft parallel unter Last mit.

## Punkt 4: duel_exegol (REST4 Punkt 7)

- Die Gegenstaende bestimmt mit Strahlen und Rasterbild aus der .bsp (`Y:\werkzeuge\r5\strahl.py`, `raster.py`):
  - die rot-blaue Figur oben ist die eingebackene Statue `models/map_objects/rift/statue` (Flaeche 2, misc_model als
    Dreiecksflaeche, 4375-4542 Einheiten entfernt, x 3115-3860, bis z 1703) - kein Spieler/NPC (info_player_start bei
    0 -2304 24 liegt ausserhalb des Bilds), kein Mover, kein Modell-Entity (vor der Kamera nur waypoint/point_combat, Relais,
    Lichter, fx_runner);
  - der dunkle schraege Gegenstand sind nahe Felsen `textures/exegol_sh/rocky_3` (Flaechen 5968-5979, 370-500 Einheiten, Cluster
    der Kamera 167). Sie stehen auch im Spielbild - dort sind sie so schwarz wie die Waende dahinter (Differenzbild
    `exB_diff.png`: an ihnen gleich); in der Vorschau fallen sie auf, weil die Statue und die Waende dahinter heller sind.
- Gegenprobe mit freiem Blick auf die Statue (Kamera 1500 -5000 600, Nick -5, Gier 18; `r5stA` Spiel, `r5_stA` Vorschau,
  `stA_paar.png`): das Spiel zeichnet die Statue als schwarzen Schattenriss (sichtbar vor der hellen Saeule, `stADE_statue.png`),
  die Vorschau rot-blau beleuchtet.
- Gepruefter und verworfener Ansatz: ParseTriSurf nimmt fuer Dreiecksflaechen immer lightmapsVertex und die Stile der Ecken
  (ShaderForShaderNum, Movie Duels code/rd-vanilla/tr_bsp.cpp:532/:324; ComputeFinalVertexColor tr_surface.cpp:1063), q3map2
  trug fuer die Statue Lightmap 0 ein. Die Vorschau beim Laden darauf umgestellt (lightmap -3, Eckenstile): das Bild der Statue
  blieb gleich (Bereich 830-1060 x 330-840: vorher 0,76/3,50/7,53, nachher 0,76/3,50/7,58; Spiel 0,00/0,01/0,09) - die
  Vorschau nimmt ihr Licht also nicht ueber die Lightmap-Nummer. Aenderung zurueckgenommen (keine Wirkung, kein Beleg).
- Die Eckenfarben der Statue: Stil 0 im Mittel 8/15/24 (118 von 448 Ecken 0/0/0), die schaltbaren Stile 32/33 hell
  (115/208/255; Lichter flickernow0/2, START_OFF, im Vorschau-Protokoll "Lichtstil 32 0.00, 33 0.00").
- Nebenbefund Spiel: die Helligkeit des Spielbilds haengt am Laufstand - mit einer zusaetzlich gesetzten Render-Variablen
  (r_showcluster 1 oder r_novis 1, `r5stD`/`r5stE`) ist das ganze Bild 2-4x heller (Mittel 0,02/0,74/1,99 statt 0,02/0,18/0,93),
  ueber 8 s gleichbleibend (`r5stC`, 18 Bilder). Die Statue ist in beiden Faellen schwarz. Nicht weiter verfolgt.

## Schluss (`Y:\werkzeuge\r5\schluss.sh`, Summe `schluss_summe.txt`)

- Sauberer Neubau (`--clean-first`) 0 Warnungen (997 s); ctest 244/244 (-j 12 ohne q3map2_pr316), q3map2_pr316 allein 1/1.
- Parallel: modellblock funktionen 61/0, md 86/0, karteneditor (alle Gruppen) 3391/0, Modus 1 2635/0 (Reste1/5: 8 Bilder in
  2,04 s, "nicht geruht wegen: -").
- Lints wie die Basis (portability 99, pruefcodes 11, efxflags-/fremdprogramm-Meldung bekannt, link/tidy nur unter Linux, sonst
  ok); Handbuch `--pruefen` ok. Kein Handbuchtext geaendert (die Bedienung ist gleich).

## Offen

- Punkt 4 (duel_exegol): Gegenstaende bestimmt (Statue als eingebackenes misc_model, nahe Felsen), die Ursache fuer die helle
  Statue in der Vorschau nicht gefunden. Naechster Schritt: in der Vorschau die Farbe der Statue verfolgen (welcher Zeichenweg
  zeichnet Flaeche 2: Stufen von models/map_objects/rift/statue mit rgbGen vertex, welche Eckenfarbe kommt im Zeichner an - per
  Protokollzeile fuer Flaeche 2 in buildMesh/gpumap), dann gegen ComputeFinalVertexColor angleichen; danach das Flaechenlicht
  (Spiel 0,5/2,2 gegen Vorschau 0,9/3,5 ohne Nebel) erneut vergleichen. Dazu der Nebenbefund: das Spielbild wird mit einer
  zusaetzlich gesetzten Render-Variablen 2-4x heller - vor dem Flaechenvergleich klaeren, welcher Spielstand gilt.
- Punkt 3: Ursache des einmaligen Fehlschlags nicht nachgestellt; die Zaehler nennen sie beim naechsten Mal.

## Changelog

## English

- Model tab, Chamfer with segments: a single unselected edge between two chamfered edges gets its point halfway between both meeting points, UVs are merged in Blender's buckets, the fan corners keep the UVs Blender gives them, and corner polygons start where Blender's do.

## Deutsch

- Model-Tab, Chamfer mit Segmenten: Eine einzelne nicht gewählte Kante zwischen zwei gefasten Kanten bekommt ihren Punkt in der Mitte der beiden Treffpunkte, UVs werden in Blenders Eimern gemittelt, die Fächerecken behalten die UVs, die Blender ihnen gibt, und Eckvielecke beginnen wie bei Blender.

## 中文

- 模型标签页，带分段的倒角：两条倒角边之间的单条未选中边，其点位于两个交点的中间；UV 按 Blender 的分组合并；扇形角点保留 Blender 赋予的 UV；角多边形的起点与 Blender 相同。

## 日本語

- モデルタブ、セグメント付きチャンファー：面取りした 2 本のエッジの間にある未選択のエッジが 1 本だけの場合、その点は 2 つの交点の中間に置かれ、UV は Blender のバケットでまとめられ、扇形の角は Blender が与える UV を保ち、角のポリゴンは Blender と同じ位置から始まります。

## Español

- Pestaña Model, Chamfer con segmentos: una sola arista no seleccionada entre dos aristas biseladas recibe su punto a mitad de camino entre ambos puntos de encuentro, las UV se unen en los grupos de Blender, las esquinas del abanico conservan las UV que Blender les da y los polígonos de esquina empiezan donde los de Blender.

## Čeština

- Karta Model, Chamfer se segmenty: jediná nevybraná hrana mezi dvěma zkosenými hranami dostane svůj bod uprostřed mezi oběma průsečíky, UV se slučují v Blenderových skupinách, rohy vějíře si ponechají UV, které jim dá Blender, a rohové mnohoúhelníky začínají tam, kde v Blenderu.

