# Notizen mbvers: Verschieben, Drehen, Skalieren und Modellieren in einer Pose

Stand 10.11.2026, Worktree `Y:\p_mbvers`, Zweig `p_mbvers`, Basis a00f6cf90 (karte1). Zwei Agenten nacheinander (Modellwechsel),
Laufnamen `mbv_*`, Bau `Y:/p_mbvers/build`, Laufskript `Y:\werkzeuge\mbv\lauf.py`. kFassung bleibt.
Auftrag: `auftraege/auftrag_mbvers_1011.md`. shank 10.11.: "ein bug wenn ich was bewege, dann geht ein mesh nach links obwohl
ich alles nach rechts bewege" (kyle in BOTH_A2_TR_BL, der Kopf wanderte vom Koerper weg).

## Ursache

Der Model-Tab zeigt das Netz in der Pose (lineares Skinning wie das Spiel, p = Summe w_i M_i v), das Dokument fuehrt die Ecken
in der Bindepose. `umformen()` und `fuerAuswahlDrehen()` (`gui/app_modelltab_ansicht.cpp`) nahmen die Mitte aus den gezeigten
Orten (`haut()`), rechneten Weg, Drehung und Faktoren aber direkt auf den Ecken der Bindepose. Jede Flaeche haengt an anderen
Knochen, also wanderte jede in eine andere Richtung, und gedreht und skaliert wurde um einen Punkt, der in der Bindepose
woanders liegt. Die Modellier-Werkzeuge (`gui/wof/app_modelltab_mesh.cpp`) arbeiteten ganz in der Bindepose, auch QuickSlice,
dessen Ebene aus der gezeigten Ansicht kommt.

**"Keine Animation" ist nicht die Bindepose.** Gemessen (`Y:\werkzeuge\mbv\gla_bild0.py`, Skin-Matrizen von Bild 0 der
`_humanoid.gla`; Drehwinkel aus schiefem Teil und Spur, nicht aus acos der Spur - das macht aus der float-Rundung 0,02 bis 0,05
Grad): kein Knochen steht in der Grundstellung.

| `_humanoid.gla` | model_root | Rumpf und Kopf | Beine | Arme |
|---|---|---|---|---|
| Movie Duels (MD_Assets2.pk3, 60861 Bilder) | 24,00 tiefer | 27,09 tiefer, Drehung 0 | 3,32 bis 3,36 Grad | 48,31 Grad, Weg bis 42,60 |
| Original (assets1.pk3, 21376 Bilder) | 24,00 tiefer | 27,09 tiefer, 0,03 bis 0,07 Grad | 3,31 bis 3,38 Grad | 48,14 bis 48,31 Grad |

Der Fehler galt also auch ohne gewaehlte Animation: der Kopf drehte sich um einen Punkt 27 unter ihm, ein Arm wanderte 48
Grad neben dem Zug (Rot-Lauf unten: 11,57 und 8,93 Einheiten daneben).

## Loesung

- `bhed/modell/posiert.h` (neu): je Ecke die gemischte Matrix A = Summe w_i M_i in double, ihre Umkehrung (Adjunkte, wie
  Blender `invert_m3_m3`), die Umkehrung des staerksten Knochens als Ausweg, die Matrix des ersten Knochens fuer die Normale
  (G2_SkinSurface dreht die Normale nur mit ihm) und deren Umkehrung.
- Gizmo und G/R/S: `umformenPosiert` rechnet je Ecke gezeigt = A v (double), formt dort um (um die gezeigte Mitte der Auswahl
  in der Pose dieses Bilds) und legt v' = A^-1 q ab. Die Normale: n' = N^-1 R N n. Eine Ecke, die sich im Gezeigten nicht
  bewegt, bleibt bitgleich stehen. pfix M6 (Normalen folgen verzerrten Dreiecken) rechnet im Gezeigten, in double: starr
  Bewegtes bleibt starr, die Normalen der Datei bleiben.
- Modellier-Werkzeuge: das Arbeitsnetz geht in die Pose (`wof::inPose`), das Werkzeug arbeitet dort, `wof::ausPose` legt
  unveraenderte Punkte bitgleich zurueck und rechnet neue und bewegte ueber ihre Mischung zurueck; neu gerechnete Normalen
  dreht der erste Knochen zurueck. Vorgaben (Slice-Ebene, Betrag, Weld-Schwelle) gelten im Gezeigten. Symmetrize bleibt in
  der Bindepose (die Symmetrieebene des Modells gibt es nur dort).
- **Bindepose ohne Schwelle:** `modell::bindepose()` - die Pose ist die Bindepose, wenn jede Weltmatrix Wert fuer Wert der
  gleicht, die dieselbe Kette mit der Einheit als lokaler Matrix liefert. Nur dann (und ohne Skelett) ist das Gezeigte die
  Bindepose, und es bleibt beim bisherigen Rechenweg (derselbe Code, bitgleich). Die Skin-Matrizen selbst taugen dafuer nicht:
  Welt * BasePoseInv ist auch in der Grundstellung nur gerundet die Einheit.
- **Ein Modell ohne Skelett bekommt keine Pose** (`posieren()`): das Skelett der Figur davor bleibt im Programm geladen
  (`g_app->anim`) und verschob bisher ein danach geoeffnetes .md3 oder neues Requisit in der Ansicht um den Weg von
  model_root (seine Ecken haengen der Form halber an Knochen 0); der Drehpunkt lag daneben. Mit dem Bearbeiten im Gezeigten
  haetten die Werkzeuge die Koordinaten der Datei um diesen Weg versetzt (aufgefallen an MR "Alt+Klick (Kantenschleife)":
  der Kasten lag in der Datei woanders).
- **Nicht umkehrbar ohne geratene Schwelle:** Determinante 0, oder der zurueckgerechnete Ort waere keine Koordinate des Spiels
  (nicht endlich oder ausserhalb von MAX_WORLD_COORD = 64 * 1024, q_shared.h:802). Dann folgt die Ecke ihrem staerksten Knochen
  (ohne gueltigen Knochen bleibt sie), und die Meldung nennt die Zahl. Keine NaN.

## Vorbild

- **Blender** (Quelle blender/blender, main): `transform_convert_mesh.cc`, `transform_convert_mesh_crazyspace_transdata_set`
  - je Ecke `mul_m3_m3m3(mat, mtx, defmat)` und `invert_m3_m3(imat, mat)`; die Matrizen liefert `crazyspace.cc`,
  `BKE_crazyspace_get_first_deform_matrices_editbmesh`, fuer Modifier vom Typ OnlyDeform mit `deform_matrices_EM` (Armature).
  Was man sieht, bewegt sich so, wie man zieht - so rechnet JK Studio jetzt.
- **3ds Max**: nur die Hilfe gelesen (Edit Poly Modifier: "Show End Result is on by default, and you can still see the
  results of any modifiers above Edit Poly on the stack"; Show Cage zeigt das Endergebnis weiss und die urspruengliche
  Auswahl gelb). Dass eine Umformung unter Skin im unverformten Raum gilt, folgt aus dem Stapel, steht dort aber nicht
  woertlich; am Programm nicht geprueft (3ds Max wurde nicht gestartet).
- **Schalter "in Ruhepose bearbeiten"** (Blender: Rest Position): sinnvoll, weil Bild 0 der `_humanoid.gla` nicht die
  Bindepose ist und man sie sonst nie sieht. Nicht gebaut (siehe Offen).

## Tests

- ctest `modell_posiert` (neu, `tests/modell_posiert.cpp`): Bindepose erkannt (auch zwischen zwei Bildern gemischt), ein Knochen
  um 1/64 verschoben: nicht; Rundlauf gezeigt -> Bindepose -> gezeigt 1,9e-6 (erlaubt 16 float-Schritte = 6,1e-5); Determinante
  0 und fast 0 (Ort ausserhalb der Welt): staerkster Knochen, keine NaN; Normale ueber den ersten Knochen; Arbeitsnetz mit
  Extrude in der Pose, unveraenderte Punkte bitgleich. 21 Pruefungen, 0 Fehler.
- Selbsttest `BHED_EDITORTEST=modellblock`, `BHED_MODELL_NUR=mbv` (`gui/selbsttest_mbv.inc`; Teile `mbvumform`, `mbvmesh`):
  kyle ohne Animation (Bild 0) und in BOTH_A2_TR_BL Bild 274 (groesste Drehung 113 Grad); Figur, Kopf, Kopf+Koerper, rechter
  Arm, jede dritte Ecke des Kopfs; je Ansicht jeder greifbare Achsgriff, frei, Ebene XY, Drehen, Skalieren; G/R/S mit Zahl;
  laufende Animation; Rueckgaengig und Wiederholen bitgleich; die .glm geschrieben und gelesen wie das Dokument (330284 Bytes,
  5567 Ecken); exakte Bindepose; Extrude, Slice, QuickSlice; ein Requisit nach der Figur.

| Lauf | Stand | Ergebnis |
|---|---|---|
| `mbv_rot1` | ae2cd3916, ohne Fix | **112 OK, 102 FEHLER**: 100 von 102 Umform-Faellen rot, 0,33 bis 21,75 Einheiten daneben (9,8 Mio float-Schritte, Normalen 0,65, Bild 20,1 px); gruen nur der Kopf ohne Animation beim Verschieben (er ist in Bild 0 nur verschoben). Ohne Animation: Kopf R Y 25 11,57 daneben, S 1,25 6,85; rechter Arm G X 12 8,93, R 9,54, S 5,90. Extrude 4,67 daneben, Slice-Vorgabe 34,59 neben der gezeigten Mitte und 0 Punkte auf der Ebene, QuickSlice 0 Punkte |
| `mbv_g1` | mit Fix (fix1 + fix2) | 208 OK, 6 FEHLER - alle sechs am Test: X-Griff zeigte in der Perspektive aus dem Bild (die Mitte griff), 4 Bilder Halten reichten in der Perspektive nicht, Laengenpruefung der Normalen falsch. Orte 6,0 Schritte, Extrude 0,000003, Slice 65 Punkte auf der Ebene, QuickSlice 678 |
| `mbv_g3` | dazu das Requisit nach der Figur | 214 OK, **1 FEHLER** (rot gewollt): der Kasten lag in der Datei bei z 24 bis 88, gezeigt 24 neben seinen Koordinaten, 53 Knochen der Figur davor |
| `mbv_g4` | dazu: Modell ohne Skelett bekommt keine Pose (`posieren()`) | 214 OK, 1 FEHLER: das Requisit wird jetzt an seinen Koordinaten gezeigt (0,000000 daneben, keine Pose), der Kasten lag aber noch bei z 24 bis 88 - im Bild, in dem das Requisit die Figur abloest, standen in `g_knochen` noch ihre Knochen. Behoben in `bearbeitungsPose()` (fragt das Dokument) |
| `mbv_s_mbv` | dazu `bearbeitungsPose()` fragt das Dokument (sauberer Neubau, Schlussrunde) | **215 OK, 0 FEHLER**: der Kasten liegt in der Datei bei z 0 bis 64 um die Mitte, gezeigt genau dort (0,000000 daneben), keine Pose zum Bearbeiten, 53 Knochen der Figur weiter geladen |

Gemessen (Laeufe g1 bis g4): Orte hoechstens 7,8 float-Schritte der groessten Koordinate (erlaubt 40, hergeleitet in
`gui/selbsttest_mb1a2.inc`), Normalen 1,2e-7 (erlaubt 8 Schritte = 4,8e-7), Bild rechtwinklig 0,0006 px, Perspektive 0,0022
px - hoechstens 28 % der Schranke des Falls (18 float-Schritte des Schirmorts, in der Perspektive dazu der Mausweg mal 24
Schritte durch den kleinsten Massstab). Der Vorgaenger hatte 0,02 Einheiten, 1 px und 3 px angesetzt und "gemessen"
dazugeschrieben; gemessen war nichts.

In der Perspektive folgt die Mitte dem Mausweg nicht 1:1: der Griff rechnet Bildpunkte mit dem Massstab an der Mitte in
Einheiten um. Steht die Maus still, gilt am Achsgriff genau Weg auf dem Schirm = Mausanteil * w(Griffspitze) / w(alte
Mitte) - der Test prueft diese Formel (0,0022 px) statt einer weiten Toleranz. Das ist das Verhalten des Griffs, nicht der
Fehler dieses Auftrags, und unveraendert.

## Angepasste vorhandene Tests

- `gui/selbsttest_mb1a2.inc`, "MB2 Blender G X 10 / S 2 / R Z 90" (Gruppe `blender`): der Test formt den Rumpf in BOTH_WALK1
  Bild 3 um und pruefte die Koordinaten der Bindepose (genau 10 in X der Datei, Abstaende der Datei mal 2) - er schrieb den
  Fehler fest. Mit dem Fix rot (Lauf `mbv_bl1`, 3 Zeilen), jetzt im Gezeigten geprueft: jede Ecke gegen Weg, Faktor und
  Drehung um die gezeigte Mitte, in float-Schritten; Abbrechen und Rueckgaengig weiter bitgleich an der Datei.
- `gui/selbsttest_mb1a1.inc` "MB1 Gizmo" (Rumpf des Sturmtrupplers entlang Y, ohne Animation) blieb unveraendert gruen: mit
  der `_humanoid.gla` von Movie Duels ist der Rumpf in Bild 0 nur verschoben. Mit der des Originalspiels (Rumpf 0,05 Grad
  gedreht) aendern sich X und Z der Datei um 5e-4 des Wegs - dort wuerde seine Pruefung "X und Z gleich" (exakt) nicht halten.
- MR "Alt+Klick (Kantenschleife)" (Gruppe `blender`) war mit fix1 + fix2 rot ("12, soll 3"): der Kasten des Requisits lag wegen
  des Skeletts der Figur davor woanders. Behoben in `posieren()`, der Test ist unveraendert.
- Sonst keine Erwartung eines vorhandenen Tests geaendert (Diff gegen a00f6cf90: nur `selbsttest_mb1a2.inc`).

## Schlussrunde (10.11. abends, Stand nach WIP 7)

- Sauberer Neubau `Y:/p_mbvers/build` (--clean-first, --parallel 8): 0 Warnungen, 0 Fehler (`Y:\werkzeuge\mbv\bau_sauber2.log`).
- ctest: 246 von 247 gruen, rot nur `q3map2_teilbau` aus der Basis (siehe Offen); neu `modell_posiert` gruen.
- Selbsttest modellblock (Laeufe `mbv_s_*`, parallel, je eigener Laufordner): mbv 215/0, funktionen 61/0, tasten 43/0, mb2 37/0,
  blender 67/0, haut 1/0, mb3 21/0, rundlauf 2/0, rig 26/0, mb5 159/0, mb6 37/0, mb7 24/0, mb7textur 1/0, ph 29/0, rt 19/0,
  pr 15/0, uv 50/0, uvmd3 3/0, md 86/0, mdkarte 7/0, pt 41/0, ptfix 5/0, ptfix2 (ja) 5/0, ptfix3 12/0, ptstufen 4/0, pfix 17/0,
  pfix2 5/0, pfixkarte 5/0; modell 14/0. mb5, ph, rt und mb7textur liefen zuerst ohne BHED_MB_BELEGE/BHED_RT_GLA/BHED_MB7_*,
  ptfix2 zuerst auf Englisch (Pruefung braucht eine andere Sprache) - mit den Variablen wie in frueheren Schlussrunden gruen.
- Modus 1 (`mbv_s_m1`): 2635 OK, 0 FEHLER.
- Lints: alle 32 Ausgaben bytegleich mit der Basis a00f6cf90 (portability 99, pruefcodes 11, efxflags und fremdprogramm wie
  dort; alle anderen ok).
- Handbuch nicht geaendert: kein neuer Schalter, keine neue Bedienung; das Handbuch beschreibt das Umformen in einer Pose nicht
  gesondert (gezogen wird wie bisher, nur das Ergebnis stimmt jetzt).
- Bildzeit nicht gemessen: das Zeichnen rechnet wie vorher; neu je Pose ist nur `modell::bindepose()` (ein Vergleich der
  Weltmatrizen je Knochen), die Rueckrechnung je Ecke laeuft nur waehrend des Umformens und der Werkzeuge.

## Offen

- ctest `q3map2_teilbau` rot (2 Faelle: Lichtcache nach Aenderung von Detail-Brush in Raum B und Shader in Raum A, zweimal
  gleich): kommt aus der Basis a00f6cf90 (WIP lcb2 1); dieser Zweig aendert nichts an q3map2. Auf karte1 folgt danach 688daa357
  (lcb2 1: Lichtcache nach Geometrie-, Shader- und Brush-Entity-Aenderungen).
- Schalter "Bindepose zeigen / in Ruhepose bearbeiten" (Blender: Rest Position): nicht gebaut. Sinnvoll - die Bindepose ist
  sonst nie zu sehen. Er braucht Menue, Aktionssuche, Rechtsklick, Tooltip, Selbsttest und das Handbuch in 6 Sprachen.
- 3ds Max nicht am Programm verglichen (nur Hilfe).
- Stark gestauchte, aber umkehrbare Mischungen (zwei Knochen ueber 170 Grad gegeneinander) werden exakt umgekehrt wie in
  Blender: die Ecke wandert in der Bindepose um ein Vielfaches des gezogenen Wegs. Eine Warnung dafuer gibt es nicht (sie
  braeuchte eine Schwelle ohne Quelle).
- Ecken eines Punkts mit verschiedenen Gewichten: das Arbeitsnetz der Werkzeuge nimmt die Gewichte der ersten Ecke.
- Die Werkzeuge rechnen in der Pose des Bilds, in dem ihr Kaestchen geplant wurde, auch wenn die Animation weiterlaeuft.
- Texture Painting mit Symmetrie und Weights spiegeln rechnen weiter in der Bindepose (dort liegt die Symmetrieebene).
- Mit der `_humanoid.gla` des Originalspiels nicht gelaufen (Selbsttests laufen mit Movie Duels).

## Changelog

## English

- Model tab: moving, rotating and scaling in a pose (gizmo and G/R/S) now moves what you see the way you drag it - every vertex follows the dragged path, all surfaces together, and rotating and scaling use the centre you see. Before, the path was applied to the bind pose, so each surface went its own way (the head left the body). This also holds with no animation selected: frame 0 of the skeleton is a pose too (on the humanoid the arms hang 48 degrees lower than in the bind pose).
- Model tab: when rotating in a pose the normals turn with the surface you see; moving or scaling keeps the normals of the file.
- Model tab, modelling: the tools (Extrude, Bevel, Weld, Slice, QuickSlice, Create primitive and the others) work in the pose you see - directions, planes, distances and thresholds are the ones on screen. Only Symmetrize still mirrors at the symmetry plane of the bind pose.
- Model tab: vertices whose bone blend cannot be inverted in the current pose follow their strongest bone, and a message says how many.
- Model tab: models without a skeleton and figures shown exactly in the bind pose are edited as before, with identical results.
- Model tab: an .md3 or a new prop opened after a figure is shown at its own coordinates again; before, the skeleton of the figure still loaded moved it by the offset of model_root in the view, and a primitive created there landed 24 units higher in the file.

## Deutsch

- Model-Tab: Verschieben, Drehen und Skalieren in einer Pose (Gizmo und G/R/S) bewegt jetzt, was man sieht, so, wie man zieht - jede Ecke folgt dem gezogenen Weg, alle Flächen gemeinsam, und gedreht und skaliert wird um die Mitte, die man sieht. Vorher galt der Weg in der Bindepose, und jede Fläche wanderte in ihre eigene Richtung (der Kopf verließ den Körper). Das gilt auch ohne gewählte Animation: Bild 0 des Skeletts ist ebenfalls eine Pose (beim Humanoiden hängen die Arme 48 Grad tiefer als in der Bindepose).
- Model-Tab: Beim Drehen in einer Pose drehen die Normalen mit der gezeigten Fläche; Verschieben und Skalieren lassen die Normalen der Datei.
- Model-Tab, Modellieren: Die Werkzeuge (Extrudieren, Abschrägen, Verschweißen, Schneiden (Ebene), Schnellschnitt, Grundkörper erstellen und die übrigen) arbeiten in der gezeigten Pose - Richtungen, Ebenen, Abstände und Schwellen sind die auf dem Bildschirm. Nur Symmetrisieren spiegelt weiter an der Symmetrieebene der Bindepose.
- Model-Tab: Ecken, deren Knochenmischung sich in der Pose nicht umkehren lässt, folgen ihrem stärksten Knochen, und eine Meldung nennt ihre Zahl.
- Model-Tab: Modelle ohne Skelett und Figuren, die exakt in der Bindepose stehen, werden wie bisher bearbeitet, mit denselben Ergebnissen.
- Model-Tab: Ein .md3 oder ein neues Requisit, das nach einer Figur geöffnet wird, steht wieder an seinen eigenen Koordinaten; vorher verschob das noch geladene Skelett der Figur es in der Ansicht um den Weg von model_root, und ein dort erstellter Grundkörper lag in der Datei 24 Einheiten höher.

## 中文

- 模型标签页: 在姿势中移动、旋转和缩放 (操纵器和 G/R/S) 现在按拖动的方式移动所见内容 - 每个顶点都沿拖动的路径移动, 所有表面一起移动, 旋转和缩放以所见的中心为准。此前路径作用于绑定姿势, 每个表面各走各的方向 (头部离开了身体)。未选择动画时同样如此: 骨架的第 0 帧也是一个姿势 (人形角色的手臂比绑定姿势低 48 度)。
- 模型标签页: 在姿势中旋转时, 法线随所见表面一起旋转; 移动和缩放保留文件中的法线。
- 模型标签页, 建模: 各工具 (挤出、倒角挤出、焊接、切片、快速切片、创建基本体等) 都在所见姿势中工作 - 方向、平面、距离和阈值均为屏幕上所见。只有对称仍以绑定姿势的对称面镜像。
- 模型标签页: 骨骼混合在当前姿势下不可逆的顶点跟随其最强的骨骼, 并有消息说明数量。
- 模型标签页: 没有骨架的模型以及恰好处于绑定姿势的角色照旧编辑, 结果完全相同。
- 模型标签页: 在角色之后打开的 .md3 或新道具重新显示在其自身坐标处; 此前仍加载的角色骨架会在视图中按 model_root 的偏移移动它, 在那里创建的基本体在文件中高出 24 个单位。

## 日本語

- モデルタブ: ポーズ中の移動・回転・拡大縮小 (ギズモと G/R/S) は、見えているものをドラッグしたとおりに動かすようになりました。すべての頂点がドラッグした経路に従い、すべてのサーフェスが一緒に動き、回転と拡大縮小は見えている中心を基準にします。以前は経路がバインドポーズに適用され、サーフェスごとに別の方向へ動いていました (頭が体から離れました)。アニメーションを選択していない場合も同様です: スケルトンのフレーム 0 もポーズです (ヒューマノイドでは腕がバインドポーズより 48 度下がっています)。
- モデルタブ: ポーズ中に回転すると、法線は見えているサーフェスと一緒に回転します。移動と拡大縮小ではファイルの法線を保ちます。
- モデルタブ、モデリング: 各ツール (押し出し、ベベル、溶接、スライス、クイックスライス、プリミティブ作成など) は、見えているポーズで動作します。方向、平面、距離、しきい値は画面上のものです。対称化だけは引き続きバインドポーズの対称面でミラーします。
- モデルタブ: 現在のポーズでボーンの混合を逆算できない頂点は最も強いボーンに従い、その数がメッセージで表示されます。
- モデルタブ: スケルトンのないモデルと、正確にバインドポーズで表示されているフィギュアは、これまでどおり同じ結果で編集されます。
- モデルタブ: フィギュアの後に開いた .md3 や新しいプロップは、再び自身の座標に表示されます。以前は読み込まれたままのフィギュアのスケルトンがビュー内で model_root の移動量だけずらし、そこで作成したプリミティブはファイル内で 24 単位高い位置になっていました。

## Español

- Pestaña Model: mover, girar y escalar en una pose (gizmo y G/R/S) mueve ahora lo que se ve tal como se arrastra: cada vértice sigue el recorrido arrastrado, todas las superficies juntas, y el giro y la escala usan el centro que se ve. Antes el recorrido se aplicaba a la pose de enlace y cada superficie iba en su propia dirección (la cabeza se separaba del cuerpo). También vale sin animación seleccionada: el fotograma 0 del esqueleto también es una pose (en el humanoide los brazos cuelgan 48 grados más abajo que en la pose de enlace).
- Pestaña Model: al girar en una pose las normales giran con la superficie que se ve; mover y escalar conservan las normales del archivo.
- Pestaña Model, modelado: las herramientas (Extruir, Biselar, Soldar, Cortar (plano), Corte rápido, Crear primitiva y las demás) trabajan en la pose que se ve: direcciones, planos, distancias y umbrales son los de la pantalla. Solo Simetrizar sigue reflejando en el plano de simetría de la pose de enlace.
- Pestaña Model: los vértices cuya mezcla de huesos no se puede invertir en la pose actual siguen a su hueso más fuerte, y un mensaje indica cuántos son.
- Pestaña Model: los modelos sin esqueleto y las figuras mostradas exactamente en la pose de enlace se editan como antes, con resultados idénticos.
- Pestaña Model: un .md3 o un accesorio nuevo abierto después de una figura vuelve a mostrarse en sus propias coordenadas; antes, el esqueleto de la figura aún cargado lo desplazaba en la vista el recorrido de model_root, y una primitiva creada allí quedaba 24 unidades más alta en el archivo.

## Čeština

- Karta Model: posun, otáčení a změna měřítka v póze (gizmo a G/R/S) teď pohybuje tím, co je vidět, tak, jak se táhne - každý vrchol sleduje taženou dráhu, všechny povrchy společně, a otáčí se a mění měřítko kolem středu, který je vidět. Dříve se dráha použila na vazební pózu a každý povrch šel svým směrem (hlava opustila tělo). Platí to i bez vybrané animace: snímek 0 kostry je také póza (u humanoida visí paže o 48 stupňů níž než ve vazební póze).
- Karta Model: při otáčení v póze se normály otáčejí s povrchem, který je vidět; posun a změna měřítka ponechávají normály ze souboru.
- Karta Model, modelování: nástroje (Vysunout, Zkosit, Svařit, Řez rovinou, Rychlý řez, Vytvořit těleso a ostatní) pracují v póze, která je vidět - směry, roviny, vzdálenosti a prahy jsou ty na obrazovce. Jen Symetrizovat dál zrcadlí podle roviny souměrnosti vazební pózy.
- Karta Model: vrcholy, jejichž směs kostí nelze v aktuální póze obrátit, se řídí svou nejsilnější kostí a zpráva uvádí jejich počet.
- Karta Model: modely bez kostry a postavy zobrazené přesně ve vazební póze se upravují jako dosud, se stejnými výsledky.
- Karta Model: .md3 nebo nová rekvizita otevřená po postavě se znovu zobrazuje na svých vlastních souřadnicích; dříve ji stále načtená kostra postavy v pohledu posunula o posun model_root a těleso tam vytvořené leželo v souboru o 24 jednotek výš.
