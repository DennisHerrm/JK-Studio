# Notizen REST2: Chamfer-Feinheiten und letzte Reste vor rc611 (08.11.2026, `Y:\karte`, Zweig `karte1`, Basis 8a8dd88e9)

Build `Y:/karte/build-test` (`--parallel 8 -- /nodeReuse:false`), Laeufe `r2_*` unter `Y:\karte\laeufe`, Skripte und Protokolle
`Y:\werkzeuge\r2\`. Quelle Chamfer: Blender 5.0 `source/blender/bmesh/tools/bmesh_bevel.cc` (Kopie
`Y:/werkzeuge/mr/blender_src/bmesh_bevel_50.cc`, Zeilen unten), GPL-2.0-or-later; Blender nicht gestartet.

## Punkt 1: Chamfer-Feinheiten (`src/modell/wof/mesh_werkzeuge.cpp`, Blender-Teile)

Rot/Gruen: ctest `modell_mesh` (neuer Block "Chamfer-Feinheiten", `chamferFeinTest`), Rot `Y:\werkzeuge\r2\rot_modell_mesh.txt`
7 FEHL, Gruen 0 Fehler. Jede Flaeche des Testkastens bekommt eine eigene affine UV (Versatz 10 je Flaeche): jede Kante ist eine
Naht, jede Flaeche eine eigene UV-Insel.

- (c) Ein Randpunkt zwischen zwei gefasten Kanten (build_boundary :3153): zwei und mehr ungewaehlte Kanten dazwischen teilen
  EINEN Randpunkt. Lage nach Blender: nur Kanten in der Ebene (eh_on_plane :2845) -> offset_meet (:1521) wie ohne Kanten
  dazwischen mit f = e1->fnext; eine Kante ausserhalb der Ebene und good_offset_on_edge_between (:1772) -> offset_on_edge_between
  (:1788, Mitte der zwei Treffpunkte, loop_slide ist die Vorgabe des Operators); sonst offset_meet mit edges_between (windschief:
  Mitte, dann auf das erste passende Vieleck fallen lassen, point_between_edges :1356). Bei genau einer ungewaehlten Kante bleibt
  die WL-Regel (Punkt auf der Kante, 1 / sin des naeheren Winkels) - Blender naehme dort die Mitte beider Treffpunkte.
  Rot "ist 32/59/29, erwartet 31/57/28" (1 Segment) und "38/71/35 statt 34/64/32" (2 Segmente, vorher ein Gitter statt weld);
  Gruen beides, Punkt bei (-0,1 / -0,1).
- (a) Naehte bei ungerader Segmentzahl (bevel_build_rings :5517-5735): e->is_seam = UV nicht durchgehend (contig_ldata_across_edge
  :992 mit CustomData_data_equals -> layerEqual_propfloat2, Abstand im Quadrat < 0,00001, blenkernel customdata.cc), bv->any_seam,
  UV-Inseln des ganzen Netzes (math_layer_info_init :1081, oberste/unterste Insel auf 0/1 getauscht), choose_rep_face (:1211;
  Materialnummer = Teil), frep_beats_next, die Mittelspalte an einer Naht (oder ohne gefaste Kante) ganz in fc, die Ecken nach
  snap_edges_for_vmesh_vert (:5453) / snap_edge_for_center_vmesh_vert (:5408) gezogen, die Mitte in center_frep
  (frep_for_center_poly :5034 mit is_bad_uv_poly :5013). Rot "Gitter 7, aus einer Flaeche 3, aus der Deckflaeche 1", Gruen 7/7, 4.
  Blender zieht eine Ecke der Mittelspalte auch auf eine Kante, die nicht zur Vertreter-Flaeche gehoert (Kommentar in
  snap_edge_for_center_vmesh_vert: "probably wrong") - uebernommen, der Test laesst Fusspunkte auf jeder alten Kante zu.
- (b) UV der Streifen (bevel_build_edge_polygons :7077: gerade Segmentzahl links f1 / rechts f2, an der Mittellinie gezogen;
  ungerade mit Naht der Mittelstreifen in choose_rep_face samt Nachbarstreifen; ohne Naht halb f1, halb f2), der Ecken ohne Gitter
  (bevel_build_poly :5892: mit Naht alles in frep_for_center_poly, aussen auf die naehere Kante gezogen, Profilpunkte ab seg/2;
  sonst je Randpunkt boundvert_rep_face :843 nach build_boundary / build_boundary_terminal_edge :2923) und der Faecher (die Ecken
  des Vielecks, BM_face_split). Danach bevel_merge_uvs (:808) ueber alle neuen Ecken und die neu gebauten alten Vielecke
  (register_uv_face :645: angehaengt facerep bzw. face_arr[0]; verbunden bei gleicher angehaengter Flaeche oder UV am alten Punkt
  innerhalb STD_UV_CONNECT_LIMIT 0,0001). Gilt jetzt auch fuer 1 Segment. Rot: Streifen 1/2 Segmente, 8 Eckdreiecke, 12 Streifen;
  Gruen alle.
- Plan-Modell: jeder neue Punkt wandert linear mit dem Betrag. Entscheidungen am Punkt (innen, naehere Kante, Flaeche fuer
  is_bad_uv_poly) und die Richtung der UV fallen bei einem kleinen Betrag (ein Tausendstel der kuerzesten Kante am Punkt, die
  Flaeche auf den Betrag 1 hochgerechnet); beim Betrag 1 laege ein Punkt jenseits der Nachbarecken. Die UV-Richtung in double
  (uvInFlaeche). Bei nicht affiner UV einer alten Flaeche bleibt die UV linear im Betrag (wie zuvor im Gitter).
- Abweichung: update_uv_vert_map haengt eine Ecke an den ersten passenden Eimer, hier Vereinigung (transitiv) wie bisher.
- Selbsttest modellblock md 86/0 (`r2_md1`).

## Punkt 2: Spielbild ohne Stufenbild

Das Spielbild eines Shaders, dessen erste Stufe weder alphaFunc noch blendFunc hat, lud `texturName` (qer_editorimage). Das Spiel
bindet bundle[0] der Bildstufe (RB_IterateStagesGeneric, rd-vanilla tr_shade.cpp:2001; ParseShader ueberliest qer_*,
tr_shader.cpp:2220). Jetzt `SpielDarstellung::grundBild` = das Bild der Bildstufe der Grundstufe (basisStufen: ohne Lightmap die
erste, sonst die des Paars), mit Endung (Suche wie R_LoadImage). Selbsttest karteneditor rd "REST2 P2": Rot `r2_rot_rd`
"r2licht 255/211/41, r2erste 255/211/41 (gelb = qer_editorimage)", Gruen rot bzw. gruen wie die Vorschau sie hochlaedt (DXT5).

## Punkt 3: P55 Rest

`shaderZiehen` holt im Spielbild nur noch die Fassung des Spiels; die des Editors nur fuer den Himmel (Farbe, Wuerfel) und als
Ersatz, wenn das Spiel die Datei nicht findet (gleiches Bild wie vorher). Messung im Bildlager nach dem Echtzeit-Licht der
Stufen-Karte (rd, "RD P55 Bildlager", fertige Eintraege, Bildpunkte aller Stufen RGBA): vorher Editor 8 Eintraege (46080 Byte),
Spiel 9 (31428 Byte); nachher Editor 10 (78848 Byte), Spiel 6 (38240 Byte). Der Vergleich ist nicht sauber: die Karte hat seit 4b
zwei Wuerfel mehr (m9klemme, m8normal, 64x64), und die Fassung des Editors fordern auch die anderen Ansichten an (Kamera vor dem
Einschalten des Echtzeit-Lichts, Texturbrowser) - die 2 Eintraege mehr sind diese Wuerfel. Das Echtzeit-Licht selbst holt keine
Fassung des Editors mehr; die Spielfassung teilt jetzt ein Bild je Name ueber die Shader (9 -> 6, vorher je Shader ein Eintrag
"Shader + Spiel"). Keine Bildzeit gemessen (laut Auftrag). Bild unveraendert: rd 38/0 (Stufen Echtzeit gegen Spielbild, DXT-Stufen
bytegleich).

## Punkt 4: 4b

Ohne Spielbild nimmt eine aus einer .bsp geoeffnete Karte die Register-Reihenfolge dieser .bsp (`bspBildRegisterBauen`: vorWelt
+ weltShaderReihenfolge aus `<stamm>_original.bsp`). Eine .map hat keine Folge (q3map2 ordnet beim Bauen um) - dort entscheidet
weiter, wer zuerst zeichnet. Dazu: ein Spielbild zaehlt nur mit "Built map as in the game" (backen); vorher galt das Register
eines frueheren Spielbilds weiter, auch einer anderen Karte. `echtRegisterNeu` baut das Register neu wie nach dem Laden (mit .bsp
deren Folge, sonst leer). Selbsttest rd "REST2 4b" (die gebaute .bsp geoeffnet, der in der .bsp spaetere Shader zuerst gefragt).
Rot nicht eigens gelaufen: vorher fing echtRegisterNeu leer an, dann entscheidet der zuerst gefragte (belegt in MR9 P4 "map
zuerst: 0 0") - also das Gegenteil der Erwartung.

## Handbuch

Modellieren (inhalt_md1d.py, 6 Sprachen): Gitter, Streifen und Eckvielecke nehmen ihre UV aus den ersetzten Flaechen, die mittlere
Reihe folgt bei ungerader Segmentzahl den Naehten, nicht gewaehlte Kanten zwischen zwei gefasten teilen einen Eckpunkt. Kein neues
Bild (die Bedienung ist gleich). `gen_handbuch.py --pruefen --pflicht=es --pflicht=cs` ok, ctest tour/handbuch 5/5.

## Schluss (`Y:\werkzeuge\r2\schluss.sh`, Summe `schluss_summe.txt`)

- Sauberer Neubau 0 Warnungen (843 s), ctest 243/243, modellblock funktionen 61/0, md 86/0, karteneditor rd 38/0, r3 22/0,
  geschuetze (BHED_GV_DXT=1, t1_sour) 18/0, figurlicht 8/0, mission1 98/0; Modus 1 2634/1 unter Volllast (Reste1/5 "Map-Tab ohne
  Karte ruht": 253 Bilder in 3,5 s), einmal wiederholt `r2_modus1b` 2635/0 (13 Bilder in 3,3 s) - einmaliger Ausreisser.
- Lints wie die Basis (portability 99, pruefcodes 11, efxflags-/fremdprogramm-Meldung bekannt), Handbuch ok.

## Offen

- Chamfer: bei genau einer ungewaehlten Kante zwischen zwei gefasten bleibt die WL-Lage (1 / sin des naeheren Winkels) statt
  Blenders Mitte beider Treffpunkte (offset_on_edge_between); update_uv_vert_map als Vereinigung statt erster Eimer; die Faecher
  uebernehmen die UV des Vielecks auch an den Ecken der neuen Kante (Blender: neue Loops ohne Eimer); die Reihenfolge der
  Randpunkte beginnt nicht genau bei Blenders boundstart (angehaengte Flaeche des Eckvielecks ohne Naht).
- 4b: Rot nicht eigens gelaufen (abgeleitet, siehe oben). Ein Spielbild zaehlt jetzt nur mit "Built map as in the game".
- `BHED_KARTE_NUR=reste1` (nicht im Auftrag, `r2_reste1`) endet mit "Beenden: Programm laeuft nach No weiter" nach 13/0 - nicht
  gegen die Basis geprueft.
- Keine Bildzeit gemessen (Auftrag).

## Changelog

## English

- Model tab, Chamfer with segments: like Blender's Bevel, all unselected edges between two chamfered edges share one corner point, odd segment counts respect UV seams, and the strips and corners take their UVs from the faces they replace.
- Map editor, real-time light: a shader whose first stage neither blends nor alpha-tests shows the image of its stage like the game, not the editor image; a map opened from a .bsp uses that .bsp's order to decide which shader loads a shared image first.

## Deutsch

- Model-Tab, Chamfer mit Segmenten: Wie Blenders Bevel teilen alle nicht gewählten Kanten zwischen zwei gefasten Kanten einen Eckpunkt, ungerade Segmentzahlen achten auf UV-Nähte, und Streifen und Ecken nehmen ihre UV aus den Flächen, die sie ersetzen.
- Karten-Editor, Echtzeit-Licht: Ein Shader, dessen erste Stufe weder mischt noch ausschneidet, zeigt wie im Spiel das Bild seiner Stufe statt des Editorbilds; eine aus einer .bsp geöffnete Karte entscheidet nach der Reihenfolge dieser .bsp, welcher Shader ein gemeinsames Bild zuerst lädt.

## 中文

- 模型标签页，带分段的倒角：与 Blender 的 Bevel 一样，两条倒角边之间所有未选中的边共用一个角点，奇数分段会考虑 UV 接缝，条带和角落从它们所替换的面获取 UV。
- 地图编辑器，实时光照：第一阶段既不混合也不做 alpha 测试的着色器会像游戏中一样显示其阶段的图像，而不是编辑器图像；从 .bsp 打开的地图按该 .bsp 的顺序决定哪个着色器先加载共享图像。

## 日本語

- モデルタブ、セグメント付きチャンファー：Blender の Bevel と同じく、面取りした 2 本のエッジの間にある未選択のエッジはすべて 1 つの角の点を共有し、奇数のセグメント数では UV シームを考慮し、帯と角は置き換える面から UV を受け取ります。
- マップエディター、リアルタイムライト：最初のステージがブレンドもアルファテストもしないシェーダーは、エディター画像ではなくゲームと同じくステージの画像を表示します。.bsp から開いたマップは、その .bsp の順序で共有画像を最初に読み込むシェーダーを決めます。

## Español

- Pestaña Model, Chamfer con segmentos: como el Bevel de Blender, todas las aristas no seleccionadas entre dos aristas biseladas comparten un punto de esquina, los números impares de segmentos respetan las costuras UV, y las tiras y esquinas toman sus UV de las caras que sustituyen.
- Editor de mapas, luz en tiempo real: un shader cuya primera etapa no mezcla ni hace prueba alfa muestra la imagen de su etapa como el juego, no la imagen del editor; un mapa abierto desde un .bsp usa el orden de ese .bsp para decidir qué shader carga primero una imagen compartida.

## Čeština

- Karta Model, Chamfer se segmenty: stejně jako Bevel v Blenderu sdílejí všechny nevybrané hrany mezi dvěma zkosenými hranami jeden rohový bod, lichý počet segmentů respektuje UV švy a pruhy i rohy berou UV z ploch, které nahrazují.
- Editor map, světlo v reálném čase: shader, jehož první fáze nemíchá ani netestuje alfu, ukazuje jako hra obrázek své fáze, ne obrázek editoru; mapa otevřená z .bsp rozhoduje podle pořadí v tomto .bsp, který shader načte sdílený obrázek jako první.
