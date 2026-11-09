# Notizen REST3b: Rest von REST3 (09.11.2026, `Y:\karte`, Zweig `karte1`, Basis d79c99cca)

Build `Y:/karte/build-test` (`--parallel 8 -- /nodeReuse:false`), Laeufe `r3b_*` unter `Y:\karte\laeufe`, Skripte, Testkarte und
Spielbilder `Y:\werkzeuge\r3b\`. Spielbilder neu ueber `screenshot_tga` (R_ScreenShot liest den Bildpuffer des Spiels,
`Y:\werkzeuge\r3b\spiel.py`, Movie Duels per Popen nach den Regeln, Spielordner jedes Mal unveraendert): die bisherigen
Fensterfotos (PrintWindow eines DPI-virtualisierten Fensters) waren senkrecht umgerechnet - waagrechte Kanten ueber zwei Zeilen
verschmiert, um bis 0,35 Bildpunkte verschoben.

## Punkt 1: Chrom-Rest p27f - Ursache: die Ruhelage der Figur (de7bb5a8c)

- Gegen das Spielbild per screenshot_tga: die Welt stimmt ohne Versatz (Kiste 0,19), die Figur lag in der Vorschau 0,6 Bildpunkte
  zu tief = 0,126 Einheiten in 240 Abstand. Jede Spur endet SURFACE_CLIP_EPSILON (0,125, cm_local.h:152) vor der Flaeche
  (CM_TraceThroughBrush, enterFrac = (d1 - SURFACE_CLIP_EPSILON) / (d1 - d2)); eine stehende Figur hat ihren Ursprung 24,125 ueber
  dem Boden. schwerkraft.cpp stellte sie auf 24 (navlauf rechnete schon 24,125). Jetzt kRuhe = 24,125 fuer alle Hoehen der Rechnung.
- Rot/Gruen: ctest pruefung4b mit den Spielwerten absolut (Laeufer z 24,000 -> 24,125, Gleiter 6,159/6,284 ... -> gleich);
  p27f Chrom-Bildpunkte 9,67 -> 3,94 von 255, ganze Figur 7,94 -> 2,82, Rumpf 7,04 -> 1,98.
- Nebenbei: die Vergleichskamera der Selbsttests (geschuetze, modellsetzen, teil6, teil9, himmelbox) hatte fovY 64,3727 statt
  CG_CalcFov 64,36644 (0,09 Bildpunkte am Rand).
- Rest: ein Glanzband am Unterarm (7/255 auf 90 x 80 Bildpunkten) - Ursache nicht gefunden (Gewichte wie G2_GetVertBoneWeight,
  Umgebung im Raum der Entity, Q_rsqrt wie das Spiel).
- Ungleicher Massstab je Achse: unveraendert genaehert (R^T n).

## Punkt 2: Detailstufen fuer Anbauten (nach dem letzten Schlussbau, siehe unten)

- Waffen, Griffe, Kinomodelle stehen in der Ghoul2-Liste der Figur; R_AddGhoulSurfaces waehlt je Modell die Stufe mit
  G2_ComputeLOD aus der refEntity der Figur und den numLods des Modells (tr_ghoul2.cpp:2668-2674 OpenJK). hiltFor laedt jetzt
  alle Stufen (289 von 389 Waffen-/Griffmodellen haben 2-3), anbauNetzImBild waehlt wie figurNetzImBild (gemeinsame figurLodLage).
- misc_atst_drivable: s.radius 320 (code/game/g_misc.cpp:3084 OpenJK, codeJK2/game/g_misc.cpp:2737; der Spieler im AT-ST
  G_DriveATST codeJK2/game/g_client.cpp:1466).
- Nicht gemacht: Fahrzeuge als eigene Entity (Radius aus der .veh-Huelle), Geschuetze/Gegenstaende (s.radius je Klasse: misc_turret
  256/80, Panel 30, emplaced 80/110, Gegenstaende 20 ...), md3-Anbauten (R_ComputeLOD). Bolzen (Tags) rechnet das Spiel immer an
  Stufe 0 (G2_GetBoltMatrixLow, G2_FindSurface(..., 0)) - so auch die Vorschau.

## Punkt 3: Gluehen mit mehreren Instanzen (b11b1cf76)

- Instanzen eines Modells teilen den Puffer; zeichneGluehen nahm die CPU-Ecken der zuletzt gezeichneten. Jetzt Sicht und Ecken je
  Instanz und Bild (CpuInstanz), vor dem Gluehaufruf zurueckgeschrieben.
- Rot/Gruen (rest605 moversprite, duel_exegol, 377 Nebelmodelle mit glow + autosprite): eigene 4 / fremde 373 -> 377 / 0.

## Punkt 4: autosprite auf Patches (b1dee67e0)

- RB_DeformTessGeometry gilt fuer das Gitter, das RB_SurfaceGrid in der Detailstufe des Bildes in tess legt: gitterTess +
  verformeTess je Bild (Ecken und Indizes des Patches auf dem CPU-Weg, nicht mehr in patchLodSchreiben).
- autosprite auf einem Patch: AutospriteDeform bricht ab (ERR_DROP), ausser bei 2 x 2 Ecken - die Vorschau zeichnet ihn dann
  unverformt, die Kartenpruefung meldet "spielabbruch" (Text in 6 Sprachen ergaenzt).
- Rot/Gruen: Testkarte r3bp (Patch mit autosprite2) gegen das Spielbild: Patchbereich 16,2 -> 0,07 von 255, ueber 40 Abweichung
  64784 -> 0 Bildpunkte. ctest defgpu: tess wie RB_SurfaceGrid und Verformung bitgleich (vier Faelle, dazu der Abbruchfall);
  karteoptimierer: spielabbruch fuer autosprite auf einem Patch (rot 0 -> 1).
- Grenze: ein unvollstaendiges letztes Viereck liest Autosprite2Deform im Spiel aus dem Rest von tess (nicht nachbildbar); sehr
  grosse Gitter teilt das Spiel auf mehrere tess (SHADER_MAX_VERTEXES) - nicht nachgebildet.

## Punkt 5: Kartenflaechen nur aus $whiteimage (bc14ba59b)

- Rot: Testkarte r3bp, Kasten mit `map $whiteimage` - im Spiel weiss (255), in der Vorschau fehlte er (0). Gruen: 255, ganzes Bild
  0,026 von 255.

## Punkt 6: Nebel duel_exegol - im Spielbild belegt, Ursache offen

- Spielbild (screenshot_tga, Kamera -600 -5952 208, Nick 10): Mittel G 18,0 / B 28,8; Vorschau (2..12 s) G 4,7 / B 8,5 - die
  Nebelvierecke (maps/fogsprite_*.md3, textures/exegol_sh/fog_gfx_*: autosprite, rgbGen lightingDiffuse, alphaGen wave, glow)
  sind in der Vorschau kaum sichtbar, im Spiel grosse helle Schwaden. Nach dem Ende des Skripts (12 s) wird die Vorschau noch
  dunkler (20 s: 0,6 / 2,1). Ursache nicht gefunden (Kandidaten: Groesse "size { end 0 parm 65 85 nonlinear }", Licht der
  Emittermodelle mit den skalierten Achsen, Lebensdauer nach dem Skriptende). Bilder: `Y:\werkzeuge\r3b\exegol_vier.png`,
  `exegol_paar4.png` (4-fach aufgehellt).

## Schlussrunde

(siehe Bericht)

## Changelog

## English

- Preview: figures stand 0.125 units above the floor like in the game (every trace stops that far before a surface) - most of the remaining difference on shiny characters.
- Preview: surfaces that turn towards the camera are also deformed on curved patches like the game; the map check reports `deformVertexes autosprite` on a patch, which stops the game.
- Preview: the glow of many copies of the same model (e.g. the fog of duel_exegol) uses each copy's own turned surfaces.
- Preview: map faces whose shader only uses `$whiteimage` are drawn white like the game instead of missing.
- Preview: weapons and saber hilts in a hand use the level of detail the game picks; a drivable AT-ST uses the radius 320 of the game.

## Deutsch

- Vorschau: Figuren stehen wie im Spiel 0,125 Einheiten über dem Boden (jede Spur endet so weit vor einer Fläche) - der größte Teil des Rests an glänzenden Figuren.
- Vorschau: Flächen, die sich zur Kamera drehen, werden auch auf gebogenen Patches wie im Spiel verformt; die Kartenprüfung meldet `deformVertexes autosprite` auf einem Patch, das bricht das Spiel ab.
- Vorschau: Das Glühen vieler Kopien desselben Modells (etwa der Nebel von duel_exegol) nimmt die gedrehten Flächen jeder Kopie.
- Vorschau: Kartenflächen, deren Shader nur `$whiteimage` nutzt, sind wie im Spiel weiß statt unsichtbar.
- Vorschau: Waffen und Schwertgriffe in der Hand nehmen die Detailstufe, die das Spiel wählt; ein AT-ST zum Einsteigen den Radius 320 des Spiels.

## 中文

- 预览:角色像游戏中一样站在地面上方 0.125 个单位(每次追踪都在表面前这么远停下)——这是闪亮角色剩余差异的大部分。
- 预览:朝向镜头的表面在弯曲的面片上也像游戏一样变形;地图检查会报告面片上的 `deformVertexes autosprite`,它会使游戏中止。
- 预览:同一模型多个副本的发光(例如 duel_exegol 的雾)使用每个副本自己转向的表面。
- 预览:着色器只使用 `$whiteimage` 的地图面像游戏一样画成白色,而不是缺失。
- 预览:手中的武器和光剑柄使用游戏选择的细节级别;可驾驶的 AT-ST 使用游戏的半径 320。

## 日本語

- プレビュー: キャラクターはゲームと同じく床から 0.125 ユニット上に立ちます (すべてのトレースは面の手前でその距離だけ止まります)。光沢のあるキャラクターに残っていた差のほとんどです。
- プレビュー: カメラの方を向く面は、曲がったパッチ上でもゲームと同じく変形します。マップチェックは、ゲームを停止させるパッチ上の `deformVertexes autosprite` を報告します。
- プレビュー: 同じモデルの多くのコピー (duel_exegol の霧など) のグローは、各コピー自身の向きを変えた面を使います。
- プレビュー: シェーダーが `$whiteimage` だけを使うマップの面は、消えるのではなくゲームと同じく白く描かれます。
- プレビュー: 手に持った武器とセーバーの柄はゲームが選ぶ詳細レベルを使います。乗り込める AT-ST はゲームの半径 320 を使います。

## Español

- Vista previa: las figuras están 0,125 unidades sobre el suelo como en el juego (cada trazado se detiene a esa distancia de una superficie): la mayor parte de la diferencia restante en figuras brillantes.
- Vista previa: las superficies que giran hacia la cámara también se deforman en parches curvos como en el juego; la comprobación del mapa avisa de `deformVertexes autosprite` en un parche, que detiene el juego.
- Vista previa: el brillo de muchas copias del mismo modelo (por ejemplo, la niebla de duel_exegol) usa las superficies giradas de cada copia.
- Vista previa: las caras del mapa cuyo shader solo usa `$whiteimage` se dibujan blancas como en el juego en lugar de faltar.
- Vista previa: las armas y empuñaduras de sable en la mano usan el nivel de detalle que elige el juego; un AT-ST conducible usa el radio 320 del juego.

## Čeština

- Náhled: postavy stojí jako ve hře 0,125 jednotky nad zemí (každé trasování se zastaví tak daleko před plochou) - většina zbývajícího rozdílu u lesklých postav.
- Náhled: plochy, které se otáčejí ke kameře, se i na zakřivených patchích deformují jako ve hře; kontrola mapy hlásí `deformVertexes autosprite` na patchi, které hru zastaví.
- Náhled: záře mnoha kopií stejného modelu (třeba mlha v duel_exegol) používá otočené plochy každé kopie.
- Náhled: plochy mapy, jejichž shader používá jen `$whiteimage`, jsou jako ve hře bílé místo chybějících.
- Náhled: zbraně a jílce mečů v ruce používají úroveň detailů, kterou volí hra; AT-ST k nastoupení používá poloměr 320 ze hry.
