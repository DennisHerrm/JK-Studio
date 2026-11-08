# Notizen REST3: Grafik-Reste (08./09.11.2026, `Y:\karte`, Zweig `karte1`, Basis d7efd8cf9)

Build `Y:/karte/build-test` (`--parallel 8 -- /nodeReuse:false`), Laeufe `r3_*` unter `Y:\karte\laeufe`, Skripte und Protokolle
`Y:\werkzeuge\r3\`. Engine-Abgleich: OpenJK `Y:\werkzeuge\sp_654alt\ojk` (code/rd-vanilla/tr_shade_calc.cpp, tr_surface.cpp,
shared/qcommon/q_math.c, q_shared.h), Movie Duels `Y:\werkzeuge\sp_654alt\md9` (AutospriteDeform/Autosprite2Deform/
GlobalVectorToLocal gleich bis auf die Schreibweise).

## Punkte 1-3: autosprite/autosprite2 auf der CPU wie RB_DeformTessGeometry (0d352bd5f)

- `gpu::verformeTess` (gpudraw.h/.cpp): Shader mit autosprite oder autosprite2 (`gpu::spriteAufCpu`) rechnet der Prozessor ganz -
  die Eintraege der Reihe nach wie RB_DeformTessGeometry (tr_shade_calc.cpp:549): RB_CalcDeformVertexes/Normals/BulgeVertexes/
  MoveVertexes (:129-280), AutospriteDeform (:377) mit RB_AddQuadStamp/Ext (tr_surface.cpp:77-154: Normale -axis[0] der WELT, st und
  Lightmap-st 0..1, Farbe der ersten Ecke, Wicklung 0,1,3 / 3,1,2, im Spiegel left = -left), Autosprite2Deform (:444: Startwert
  WORLD_SIZE 131072 statt 999999, lange Achse unnormiert, VectorNormalize q_math.c:1158, l = 0.5 * sqrt in double, VectorMA).
- zeichneKarte (gpumap_win32.cpp): die Abschnitte der CPU-Stapel (je Flaeche Ecken min..max wie tess, Patches und ERR_DROP-Faelle
  bleiben unverformt) stehen HINTER den Ecken des Netzes im Vertexpuffer, ihre Indizes zeigen dorthin (np.cpuIdx). Je Sicht (Zeit,
  Achsen, Spiegel - cpuStand) neu gerechnet und mit UpdateSubresource nachgeschrieben (Puffer DEFAULT statt IMMUTABLE nur mit
  CPU-Ecken); Effekt-/Grasnetz wie bisher je Bild. Am Mover die Sicht im Raum des Modells (zeichneMover, GlobalVectorToLocal),
  vornWelt aus g_moverLod. pipelineFor setzt fuer solche Shader nur kAutosprite/kAutosprite2 (Licht- und Nebeldurchgang lassen sie
  aus wie bisher), keine Verformungsmerkmale; der Vertex-Shader reicht die Ecke durch (die autosprite-Bloecke und spriteStelle sind
  weg, baueAutosprites/baueAutosprites2 entfernt, DeformLage::Fehlt entfaellt).
- Rot (`Y:\werkzeuge\r3\rot_p123.txt`, alter Vertex-Shader + alte bhed_core.lib gegen RB_DeformTessGeometry, je WARP / Intel UHD):
  wave/bulge vor autosprite 8192/8192, autosprite+autosprite 8192, autosprite2+wave 8184, autosprite2 allein 1895/1908,
  autosprite2 lange Kanten 7676/7684, Mover autosprite+wave 8192, Spiegel autosprite 8192 Ecken anders.
- Gruen (ctest defgpu, `Y:\werkzeuge\r3\defgpu_gruen.txt`): 15 Faelle (darunter alle roten, dazu normal/move vor/nach, Mover
  autosprite2, gemischte Indexfolgen wie kor1 3,0,2 / 2,0,1) - Ort, Normale, st, Lightmap-st, Farbe und Indizes bitgleich zur
  abgeschriebenen Engine (tests/spieldeform.h, unabhaengig von verformeTess); der Vertex-Shader der Karte reicht die Ecken auf WARP
  und Intel UHD bitgleich durch. gpudraw/gpuconst/gpushader auf die CPU umgestellt.
- Im Programm (rest605 moversprite, duel_exegol, 377 Nebelemitter): 5,4 Mio. Ecken von der CPU verformt, die Sicht im Raum des
  Modells exakt gleich GlobalVectorToLocal der Kameraachsen, mit Weltachsen exakt die Weltachsen (Probe `gpu::spriteSichtProbe`).
- Nebenbefund: der Test "Bildpunkte mit/ohne Umrechnung > 1000" war schon auf d7efd8cf9 rot (0 Bildpunkte). Bisect ueber
  Laufordner-exes: rel610 1146, MERGE3 1091, DEF1 0. Ursache DEF1 `nrm = -gVorn` im autosprite-Block: vorher rechnete rgbGen
  lightingDiffuse mit der Hochlade-Normalen (Richtung, Radius ~50) und machte den Nebel hell; jetzt mit -vorn wie das Spiel -
  der Nebel ist dort dunkel und die Emitter kaum gedreht. Die Pruefung prueft jetzt die Sicht selbst. Ein Spielfoto des Nebels von
  duel_exegol (Helligkeit gegen Movie Duels) steht aus.

## Punkt 6: Leuchthuelle liegender Schwerter (469430223)

- Ursache: parseShaderScript legte Shader ohne Bilddatei nicht ab; gfx/effects/solidWhite_cull (shaders/gfx2.shader in
  assets1.pk3: nur `map $whiteimage`, GL_ONE GL_ONE, rgbGen vertex) fehlte in der Tabelle (9514 Eintraege), texturenMitShader gab
  nichts, die Huelle (CG_Item, cg_ents.cpp:1075-1100 OpenJK) wurde nie gezeichnet. Jetzt bleibt ein Shader mit einer $whiteimage-Stufe
  (ParseStage bindet tr.whiteImage, tr_shader.cpp:1236-1255); dabei kommen aus gfx2.shader auch clear und solidWhite dazu.
- Rot: laserfalle "Gegenstandachsen" mit neuer Bedingung huelle == 2: an der ersten Falle 1 Aufruf, Protokoll "Shader nicht in der
  Tabelle (9514 Eintraege)" (r3_laser_rot). Gruen: 2 Aufrufe, laserfalle 9/0 (r3_laser_gruen); ctest figurstufen mit dem Shader
  wortgleich aus gfx2.shader.
- Folge fuer Karten: eine Flaeche mit einem Shader nur aus $whiteimage ist kein tr.defaultShader-Kasten mehr (der Kommentar in
  app_ablage.inc wollte das schon so); im Spiel ist sie weiss.

## Punkt 7: NVIDIA/AMD aus dem Code belegt (451baa1d6)

- ctest `precisebeleg`: uebersetzt die Shader mit Anspruch so, wie das Programm sie baut, und prueft D3DDisassemble mit einer
  ablaufbewussten Rueckwaertsscheibe (Verzweigungen getrennt, Schleifen bis zum Fixpunkt, Bedingungen zaehlen mit): jede beitragende
  Anweisung [precise] fuer die gebrauchten Anteile, keine mad, dp2/dp3/dp4, div, rcp, rsq, sqrt, exp, log, sincos. Regeln: D3D11.3
  Functional Spec 3.1.3.1 (add/sub/mul 0,5 ULP), 3.1.3.2 (verschmolzene Anweisungen nur "nicht ungenauer", rcp/sqrt 1 ULP, Denormale
  an Ein- und Ausgang auf 0), HLSL `precise` (Variable Syntax: getrennte Rechnungen bleiben getrennt, Reihenfolge bleibt).
- Grenzen des Anspruchs: Kamera gViewProj (cb0[0..3]), Stellung der Figur gFigurWelt (cb2[384..387]) und ihre Haut (dynamisch
  indizierte Knochen); Wolken nur die tcMod-Kette (die Kuppelkoordinate rechnet der echte Shader mit div/sqrt, im Spiel auf der CPU
  an einem Gitter - nicht beansprucht); Figur-uv laeuft auch durch tcGen environment/Glanz (dp3/sqrt, nicht beansprucht).
- Rot (erste Fassung, `Y:\werkzeuge\r3\pb_rot.txt`): Karte deformVertexes `dp4 r3.x, cb1[6], icb[r0.y]` (d3[k] mit laufendem k als dp4
  gegen eine Einheitszeile), tcGen vector `dp3` (dot()). Behoben: Auswahl des Spreads per ?:, DotProduct ausgeschrieben x*v0+y*v1+z*v2
  precise. Gruen: 11 Kartenvarianten, Figur 242 Anweisungen, Wolken-Kette ps_5_0 206, vs3-Kette (vs_5_0 O3) 208, je 0 Verstoesse;
  die Gegenprobe (a*b+c ohne precise) wird gemeldet. defgpu, turbgpu, gpuconst, hlsl, figurglanz danach gruen.
- Handbuch (Seite "Die Karte", 6 Sprachen, tools/handbuch/inhalt_rest3.py): autosprite auf dem Prozessor; bitgleich auf jeder
  Grafikkarte - gemessen auf WARP und Intel UHD, fuer NVIDIA/AMD aus dem Code (nicht gemessen), unter 2^-126 auf der Grafikkarte 0,
  unter Feature-Level 11_0 (vs_4_0) nicht zugesichert.

## Schlussrunde (`Y:\werkzeuge\r3\schluss.sh`, parallel nach dem sauberen Neubau, Summe `schluss_summe.txt`)

- Sauberer Neubau (`--clean-first`) 0 Warnungen, 0 Fehler (`neubau.txt`).
- ctest 245/245; Modus 1 2635/0, mission1 98/0, figurlicht 8/0, karteneditor rd 38/0, r3 22/0, editorportal 19/0, laserfalle 9/0,
  modellblock md 86/0, funktionen 61/0; rest605 moversprite 1/0.
- Lints: merkmalsbits meldete kAutosprite2 als ungenutzt -> mit kAutosprite zusammengelegt (das Merkmal heisst jetzt "Ecken fertig von
  der CPU", fuer autosprite und autosprite2; danach ok); portability nach zwei fehlenden Includes in precisebeleg.cpp 99 wie die
  Basis; pruefcodes 11, efxflags-/fremdprogramm-Meldung bekannt, cjkumbruch 0, link/tidy nur unter Linux; Handbuch `--pruefen` ok.
  Nach dem Zusammenlegen: Bau 0 Warnungen, ctest 245/245 (die Selbsttests liefen vorher; der Vertex-Shader beider Merkmale war schon
  der ohne Merkmal).
- Keine Bildzeit-Messung (Auftrag), kein Spielstart.

## Nicht bearbeitet (Kontext)

- Punkt 4: Chrom-Rest p27f (9,5/255 an Kanten/Glanzspitzen, Spielfoto `Y:\werkzeuge\mg2\p27e\`) und die Normale bei ungleichem
  Massstab je Achse exakt wie das Spiel.
- Punkt 5: Detailstufen fuer Anbauten, Fahrzeuge, Geschuetze, Tags, Waffen in der Hand wie G2_ComputeLOD; JK2-Spieler im AT-ST
  (Radius 320).
- Spielfoto duel_exegol-Nebel (siehe Nebenbefund Punkte 1-3).

## Changelog

## English

- Preview: surfaces that turn towards the camera (`deformVertexes autosprite` and `autosprite2`) are computed on the processor exactly like the game, together with every other deformVertexes line before and after them - also on brush entities, models and in mirrors.
- Preview: the glow shell around a lightsaber lying on the floor is drawn (shaders made only of `$whiteimage` were missing).
- Graphics: the bit-identical parts of the picture are proven from the shader code for every graphics card (new check); the manual says honestly what is measured and what follows from the code.

## Deutsch

- Vorschau: Flächen, die sich zur Kamera drehen (`deformVertexes autosprite` und `autosprite2`), rechnet die Vorschau wie das Spiel auf dem Prozessor, zusammen mit allen anderen deformVertexes-Zeilen davor und danach - auch an Brush-Entities, Modellen und im Spiegel.
- Vorschau: Die Leuchthülle um ein am Boden liegendes Lichtschwert wird gezeichnet (Shader nur aus `$whiteimage` fehlten).
- Grafik: Die bitgleichen Teile des Bildes sind für jede Grafikkarte aus dem Shader-Code belegt (neue Prüfung); das Handbuch sagt ehrlich, was gemessen ist und was aus dem Code folgt.

## 中文

- 预览:朝向镜头的表面(`deformVertexes autosprite` 和 `autosprite2`)像游戏一样在处理器上计算,连同其前后所有其他 deformVertexes 行——在笔刷实体、模型上以及镜子中也是如此。
- 预览:会绘制落在地上的光剑周围的发光外壳(只由 `$whiteimage` 组成的着色器之前缺失)。
- 图形:画面中逐位一致的部分已由着色器代码为每块显卡证明(新检查);手册如实说明哪些经过测量、哪些由代码得出。

## 日本語

- プレビュー: カメラの方を向く面 (`deformVertexes autosprite` と `autosprite2`) は、その前後の他の deformVertexes 行もすべて含めて、ゲームと同じくプロセッサーで計算されます。ブラシエンティティ、モデル、鏡の中でも同じです。
- プレビュー: 床に落ちているライトセーバーの周りの光る殻が描かれます (`$whiteimage` だけのシェーダーが欠けていました)。
- グラフィック: 画面のビット単位で同じ部分は、すべてのグラフィックカードについてシェーダーのコードから証明されます (新しいチェック)。マニュアルは、何が測定済みで何がコードから導かれるかを正直に記しています。

## Español

- Vista previa: las superficies que giran hacia la cámara (`deformVertexes autosprite` y `autosprite2`) se calculan en el procesador exactamente como en el juego, junto con todas las demás líneas deformVertexes antes y después de ellas, también en entidades de brush, modelos y espejos.
- Vista previa: se dibuja la envoltura brillante alrededor de un sable de luz tirado en el suelo (faltaban los shaders hechos solo de `$whiteimage`).
- Gráficos: las partes de la imagen idénticas bit a bit están demostradas desde el código del shader para cualquier tarjeta gráfica (nueva comprobación); el manual dice con honestidad qué está medido y qué se deduce del código.

## Čeština

- Náhled: plochy, které se otáčejí ke kameře (`deformVertexes autosprite` a `autosprite2`), počítá náhled na procesoru přesně jako hra, spolu se všemi ostatními řádky deformVertexes před nimi i za nimi - také na brush entitách, modelech a v zrcadle.
- Náhled: kreslí se zářící obal kolem světelného meče ležícího na zemi (chyběly shadery jen z `$whiteimage`).
- Grafika: bitově shodné části obrazu jsou pro každou grafickou kartu doloženy z kódu shaderu (nová kontrola); příručka poctivě říká, co je změřeno a co plyne z kódu.
