# Notizen MERGE1: fix2 und mr4 nach karte1, Chamfer-Rest (08.11.2026, `Y:\karte`, Zweig `karte1`, Basis 521fd9ca0)

Build `Y:/karte/build-test`, Laeufe `mg_*` unter `Y:\karte\laeufe`, Skripte und Protokolle `Y:\werkzeuge\mg\`.

## Teil 1: Zusammenfuehren

- `git merge d04f18e5d` (fix2): Konflikte nur in erzeugten Dateien (`include/bhed/i18n.h`, `src/i18n.cpp`, `handbuch/*/suche.js`);
  mit den Generatoren neu erzeugt (gen_i18n 5514 Texte x 6 Sprachen, gen_handbuch, gen_hilfe_orte 209 Orte, `--pruefen` ok).
  `fehler.h` (angehaengte Arten von FIX2), `mesh_werkzeuge.cpp` (Lizenzkopf, Blender-Marken), `gen_i18n.py` (i18n_mreste und
  i18n_fix2 eingebunden), CMakeLists ohne Konflikt. karte1 hat `src/modell/wof/` seit fe1f374c3 nicht geaendert, die Marken
  von FIX2 umfassen also alle Blender-Teile. Bau 0 Warnungen, ctest 237/237. Merge-Commit a71baeb07.
- `git merge c5d16c76b` (mr4): ohne Konflikt (auch `app_modelltab_malen.cpp`/`_retarget.cpp`: die Painting-Aenderungen von
  karte1 seit fe1f374c3 enthalten keine umbrechenden Texte). Erzeugte Dateien unveraendert neu erzeugt. `lint_cjkumbruch` 0 Stellen
  (auch die neuen Painting-Texte). Bau 0 Warnungen, ctest 240/240. Merge-Commit 0d7bc2ad2.
- Lints nach beiden Merges wie die Basis (portability 99, pruefcodes 11, efxflags-/fremdprogramm-Meldung bekannt), dazu
  `lint_cjkumbruch` 0; Handbuch `--pruefen --pflicht=es --pflicht=cs` ok.

### Gruen-Runde nach beiden Merges (Skript `Y:\werkzeuge\mg\gruen.sh`, Summe `gruen_summe.txt`)

- Sauberer Neubau (`--clean-first`, `--parallel 8`) 0 Warnungen, 18 min 08 s; ctest 240/240.
- modellblock pt 41/0, ptfix 5/0, ptfix2 (ja) 5/0, ptstufen 4/0, ai 31/0, pr 15/0, md 86/0, blender 67/0, funktionen 60/0.
- karteneditor rd 21/0, rdglm 20/0, rdm2 14/0, r3 22/0, cjkweit (ja) 17/0; editorportal 19/0; mission1 98/0; Modus 1 2635/0.

## Teil 2: Chamfer-Rest (Blender 5.0 bmesh_bevel.cc, `Y:\werkzeuge\mr\blender_src\bmesh_bevel_50.cc`)

- **square_out_adj_vmesh** (:5183) in `fase_blender.cpp` (`quadratAussen`): Profil >= 0,95 (PRO_SQUARE_R), selcount >= 3, gerade
  Segmentzahl - vor pipe und tri_corner wie bevel_build_rings (:5528). Je Randpunkt braucht es efirst/elast (die gefasten Kanten
  der Seiten i - 1 und i) und e1->fnext->no (`fase::RandKanten`, von der Netzseite aus den alten Vielecken der Ringpunkte).
  Ohne Gehrungen (miter sharp) sind is_patch_start/is_arc_start nie gesetzt; ungerade Segmentzahlen ruft Blender hier nicht auf.
- **Ende einer einzelnen gefasten Kante** (build_boundary_terminal_edge :2923): bei vier Kanten am Punkt drei Randpunkte ->
  Faecher vom Randpunkt der gegenueberliegenden Kante (M_TRI_FAN, bevel_build_trifan :5971), ab fuenf Kanten ein Vieleck (M_POLY,
  wie bisher), bei drei Kanten zwei Randpunkte ohne Flicken (M_NONE, wie bisher). Profil unter 0,25: die Punkte auf den nicht
  anliegenden Kanten ruecken um Betrag * Wurzel 2 (:3010). Ab drei Kanten liegt die Ebene des Profils durch den alten Punkt
  (move_profile_plane :2027, `fase::endEbene`).
- **UV der Gitterpunkte** wie bevel_build_rings (:5629) mit bev_create_ngon (:907): jede Ecke eines Gitter-Vierecks in der
  Vertreter-Flaeche ihres Randpunkts (boundvert_rep_face :843: ebev->fprev), bei gerader Segmentzahl die Ecken (i, j, ns/2) und
  (i, ns/2, k) vorher auf die Kante gezogen (se[1]/se[2]/se[3]), mit Mittelwert-Koordinaten (interp_weights_poly_v2,
  BM_loop_interp_from_face) aus den alten Ecken; danach bevel_merge_uvs (gleiche Vertreter-Flaeche oder gleiche UV am alten Punkt
  -> gemittelt). Die UV wandert linear mit dem Betrag (exakt, solange die UV einer alten Flaeche affin ist - Dreiecke, Kasten).
  Gewichte ebenso gemischt (vier groesste, Summe 1). Normalen: Blender rechnet sie aus der Form; hier ebenso (normalenNeu nach dem
  Werkzeug), die gemischte Normale ist nur der Anfangswert.

| Pruefung (ctest `modell_mesh`) | rot (`ct_rot_final.txt`, Testcode gegen den alten Stand) | gruen (`ct_g2.txt`) |
|---|---|---|
| Pyramidenspitze, 2/4 Segmente, Profil 1: Mitte = alte Spitze | FEHL (hoechster Punkt z 2,87033) | ok |
| eine Kante an Punkten mit vier Kanten, 2 Segmente, Profil 0,5 / 0,2: 32/60/30, 4 Dreiecke, gegenueber 0,1 / 0,1 Wurzel 2 | FEHL (0 Dreiecke; bei 0,2 gegenueber bei 0,1) | ok |
| Pyramidenkante, 3 Segmente: Bogenpunkte an der Spitze in der Ebene durch Spitze und Randpunkte | FEHL (Abstand 0,029114) | ok |
| Kasten alle Kanten, 2 Segmente: UV des Gitter-Vierecks der Deckflaeche an den gezogenen Orten | FEHL (UV 0,5639 / 0,6667 / 0,8723 / 1,0 bei v = 0) | ok |

- Kasten alle Kanten mit Profil 1 (2 und 4 Segmente, alle Punkte der Ecke auf der Wuerfelflaeche im Raster 0,1, Mitte = alter
  Punkt): schon vorher gruen (die Wuerfelecke mit r = 1e4 ergab dasselbe auf 1e-4); bleibt als Nachweis im Test.
- Geaenderte Erwartung: Kreuzung im 2x2-Kasten 46/84/40 -> 46/88/44 (die vier aeusseren Kantenenden haben vier Kanten und sind
  jetzt Faecher wie in Blender).
- ctest modell_mesh 96 ok / 0, Selbsttest md `mg_c_md` 86/0. Handbuch (Model-Tab, Modellieren) in 6 Sprachen um einen Satz
  ergaenzt, THIRD-PARTY und Lizenzkopf von `mesh_werkzeuge.cpp` nennen die neuen Blender-Teile (math_geom.cc, bmesh_interp.cc).

### Abweichungen / offen

- Ungerade Segmentzahl: Naehte (is_seam, UV nicht durchgehend ueber die Kante) waehlen in Blender fuer die Mittelspalte und die
  Mitte eine Flaeche nach choose_rep_face (UV-Insel, Auswahl, Material, Mittelpunkt z/x/y) und ziehen Ecken auf Nahtkanten
  (snap_edges_for_vmesh_vert); hier gilt die Regel ohne Naht (Mittelspalte halb in f, halb in f2, Mitte je Randpunkt in f).
- bevel_merge_uvs mittelt hier nur unter den Gitter-Vierecken, nicht mit den Streifen und Flicken; die Ecken der Flicken
  (M_POLY/M_TRI_FAN) und Streifen behalten die bisherigen Ecken-Daten (Blender: auch dort Vertreter-Flaechen).
- Die Bedingung "Ecke im Gitter" bleibt topologisch wie bisher (ein Ringpunkt je ungewaehlter Kante; Blender legt zwischen zwei
  gefasten Kanten mit mehreren ungewaehlten Kanten nur einen Randpunkt).

## Changelog

## English

- Model tab: Chamfer with profile 1 keeps corners where three or more chamfered edges meet square, like Blender's bevel.
- Model tab: the end of a single chamfered edge at a vertex with four edges becomes a fan of triangles like in Blender, and its curve lies in the plane through the old vertex.
- Model tab: the corner grids of Chamfer take their texture coordinates from the neighbouring faces like Blender, instead of a blur of the corner values.

## Deutsch

- Model-Tab: Chamfer mit Profil 1 lässt Ecken, an denen drei oder mehr gefaste Kanten zusammenkommen, eckig wie Blenders Bevel.
- Model-Tab: Das Ende einer einzelnen gefasten Kante an einem Punkt mit vier Kanten wird wie in Blender ein Fächer aus Dreiecken, seine Rundung liegt in der Ebene durch den alten Punkt.
- Model-Tab: Die Eckgitter von Chamfer nehmen ihre Texturkoordinaten wie Blender aus den Nachbarflächen statt aus einem Gemisch der Eckwerte.

## 中文

- 模型标签页:轮廓为 1 的倒角让三条或更多倒角边相交的角保持方角,与 Blender 的倒角相同。
- 模型标签页:单条倒角边在有四条边的顶点处的末端像 Blender 一样变成三角形扇,其弧线位于经过原顶点的平面上。
- 模型标签页:倒角的角落网格像 Blender 一样从相邻面获取纹理坐标,不再混合角点的值。

## 日本語

- モデルタブ: プロファイル 1 の面取りでは、面取りした辺が 3 本以上集まる角が Blender のベベルと同じく四角いままになります。
- モデルタブ: 4 本の辺を持つ頂点で終わる 1 本だけの面取り辺の端は Blender と同じく三角形のファンになり、その曲線は元の頂点を通る平面上にあります。
- モデルタブ: 面取りの角の格子は角の値を混ぜるのではなく、Blender と同じく隣の面からテクスチャ座標を取ります。

## Español

- Pestaña Model: Chamfer con perfil 1 deja en ángulo recto las esquinas donde se juntan tres o más aristas biseladas, como el bisel de Blender.
- Pestaña Model: el extremo de una sola arista biselada en un vértice con cuatro aristas se convierte en un abanico de triángulos como en Blender, y su curva queda en el plano que pasa por el vértice antiguo.
- Pestaña Model: las rejillas de esquina de Chamfer toman sus coordenadas de textura de las caras vecinas como Blender, en lugar de una mezcla de los valores de las esquinas.

## Čeština

- Karta Model: Chamfer s profilem 1 ponechá rohy, kde se setkají tři a více zkosených hran, hranaté jako zkosení v Blenderu.
- Karta Model: konec jediné zkosené hrany ve vrcholu se čtyřmi hranami se jako v Blenderu stane vějířem trojúhelníků a jeho oblouk leží v rovině procházející původním vrcholem.
- Karta Model: mřížky rohů u Chamfer berou texturové souřadnice ze sousedních ploch jako Blender, ne ze směsi hodnot rohů.
