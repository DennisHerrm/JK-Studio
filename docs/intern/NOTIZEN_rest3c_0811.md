# Notizen REST3c: Rest von REST3b (09.11.2026, `Y:\karte`, Zweig `karte1`, Basis 827e62e66)

Build `Y:/karte/build-test` (`--parallel 8 -- /nodeReuse:false`), Laeufe `r3c_*` unter `Y:\karte\laeufe`, Skripte und Spielbilder
`Y:\werkzeuge\r3c\`. Spielbilder ueber `screenshot_tga` (`Y:\werkzeuge\r3c\spiel.py`, Vorlage r3b, dazu `R3C_REIHE=n` fuer eine
Bildreihe je 0,5 s), Movie Duels per Popen, Spielordner jedes Mal unveraendert. Fuer Vergleiche ohne die zufaelligen Blitze von
duel_exegol eine eigene Kopie der .bsp im eigenen fs_homepath mit umbenanntem func_timer (`exegol_ohneblitz.bsp`, gleiche Laenge
des Entity-Lumps).

## Punkt 1: Nebel duel_exegol (de70d0089)

Drei Ursachen, je im Spiel bzw. Programm belegt:

- **Weitere `map $lightmap`-Stufen fehlten.** Die Waende und Boeden von duel_exegol (rocky_*, wall_*, floor_*, sith_wall, rift,
  cuts) haben nach der Grundstufe weitere Lightmap-Stufen: `blendFunc GL_SRC_ALPHA GL_ONE` mit `alphaGen const 0.125` und zwei
  `GL_DST_COLOR GL_ONE` mit `tcMod transform 1 0 0 1 0 0.333/0.666` (die Lightmap-Seite 4096 x 4096 hat drei Baender, das zweite
  und dritte sind die Blitzlichter), alle mit `glow`. Die Vorschau liess jede `$lightmap`-Stufe ausser der ersten weg. Jetzt ist
  jede weitere eine Zusatzstufe mit der Lightmap der Flaeche als Bild (ParseStage: TCGEN_LIGHTMAP als Vorgabe, Lightmap geklemmt
  und ohne Mipmaps, ohne Lightmap tr.whiteImage), dazu wird `tcGen lightmap` gelesen. Im Spiel belegt: mit `r_DynamicGlow 0`
  Mittel G 8,3 / B 12,9, mit Gluehen 16,8 / 27,3 - das Gluehen dieser Stufen ist der helle Schleier.
- **RB_FogPass im Gluehdurchgang.** RB_StageIteratorGeneric ruft den Nebel auch, wenn RB_DrawSurfs die Flaechen mit glow-Stufe
  fuer den Gluehpuffer zeichnet (tr_shade.cpp:2198-2203 MD9, :2150-2153 OpenJK) - die Nebelfarbe geht verwischt ins Gluehen.
  zeichneNebel kann jetzt in den vollen Gluehpuffer zeichnen, nur fuer Stapel mit glow-Stufe (Welt, Brush-Entities, Modelle).
- **Achsen eines skalierten autosprite-Movers.** zeichneKarte merkt ihre Werte als Bilddaten; der Mover-Weg fuer autosprite
  uebergab die Sicht im Raum des Modells (mit dem Massstab der Emitter-Achsen), und zeichneNebel las danach die Blickrichtung
  daraus. Sobald die Nebelmodelle schrumpften (size end 0, nonlinear 65..85 % von 16 s), verschwand der globale Nebel - das
  "nach Skriptende noch dunkler" (es lag nicht am Skript: mit 60-s-Skript dasselbe ab 14 s).

Rot/Gruen:

- rest605 moversprite: neue Probe "Nebel rechnet mit der Blickrichtung der Kamera": rot Abweichung 0,936 (Nebelrichtung
  0,049 0 -0,004 statt 0,985 0 -0,174), gruen 0. Neue Probe "weitere $lightmap-Stufen und Nebel im Gluehen": 74 Lightmap-Stufen
  mit glow, 25 Nebelaufrufe im Gluehen (vorher gab es weder das eine noch das andere).
- Spielbild (Kamera -600 -5952 208, Nick 10; Spiel Mittel G 15,4..18,9 / B 25,5..29,8 je nach der 4-s-Welle der Nebelmodelle,
  ohne Blitz): Vorschau vorher 4,7 / 8,5 (2..12 s), 0,6 / 2,1 (20 s); jetzt 14,0 / 25,3 bei 12, 16 und 20 s. Himmel links Spiel
  26,9 / 41,2 - Vorschau 26,7 / 42,2; Boden 6,2 / 14,3 - 6,2 / 16,1. Mittlere Abweichung je Bildpunkt 3,0 von 255.
- Gegenprobe Uniform-Massstab (Testkarte p27f, Chrom): unveraendert 3,94 / 2,82.

Offen (genau): Ohne Gluehen ist das Spiel im Mittel noch um G 3,1 / B 2,8 heller (Wand rechts Spiel 8,0 / 11,9, Vorschau
3,7 / 8,6) - im Spielbild ohne Gluehen sind dort weiche Schwaden sichtbar. Belegt ist, dass sie weder von den Nebel-Runnern
(fogsprites*.efx aus der .bsp genommen: Schwaden bleiben) noch vom Wetter (fx_wind entfernt: bleiben) noch von func_timer
kommen; die Nebelmodelle selbst sind in Spiel und Vorschau fast unsichtbar (Bild fogsprite.tga RGB im Mittel 7/11/14, Alpha 30).
Die Quelle der Schwaden ist nicht gefunden.

## Punkt 2: Detailstufen fuer Gegenstaende und Geschuetze (736fded89)

- Ghoul2-Gegenstaende und Geschuetze laden alle Stufen (R_LoadMDXM) und waehlen je Bild wie G2_ComputeLOD mit s.radius und
  s.modelScale ihrer Klasse: Gegenstaende 20 (FinishSpawningItem), Laserfalle 60, Wachgeschuetz 30, E-Web 80, Geschuetzstellung 110
  (Jedi Outcast keiner), Ionenkanone 320 mit modelScale 2, misc_turret turbo 256 mit modelScale 2 (sonst keiner; Jedi Outcast 80),
  misc_turretG2 turbo 128 x customscale; ohne Zeile in der Spawnfunktion 0 (G2_ComputeLOD: Stufe 0). Quellen in den Kommentaren.
- Beleg: duel_cloudy, Kamera 1800 2450 450: Gegenstand-Stufen 0:169 1:1 2:61 (vorher immer 0). Kein Spielvergleich (die Formel
  lodWieImSpiel ist seit MERGE2/REST3b gegen Movie Duels gemessen).
- Offen: Fahrzeuge als eigene Entity (Radius aus der .veh-Huelle), md3-Anbauten nach R_ComputeLOD, der Spieler im AT-ST nach dem
  Einsteigen (G_DriveATST s.radius 320).

## Punkt 3: Chrom (346bc90af)

- Glanz und Umgebungs-Chrom einer skalierten Entity nehmen die Normale des Modells wie tess.normal (G2_SkinSurface: Knochen mal
  Normale, der Massstab steckt nur in den Achsen): diag(1/s) R^T der gezeichneten Normalen statt R^T der normierten - genau auch
  bei ungleichem Massstab je Achse. Gegenprobe gleicher Massstab (p27f): Chrom 3,94, Figur 2,82 wie vorher.
- Offen: kein Spielvergleich mit ungleichem Massstab je Achse (kein solches Modell gebaut); das Glanzband am Unterarm (7/255) -
  Ursache weiter nicht gefunden.

## Punkt 4: Handbuch (11dfde493)

- Seite "Die Karte", 6 Sprachen: autosprite auf gebogenen Patches wie im Spiel; autosprite auf einem Patch bricht das Spiel ab
  ("Autosprite shader ... had odd vertex count", tr_shade_calc.cpp:364), die Kartenpruefung meldet es. `gen_handbuch.py --pruefen`
  ok (es/cs 0 englisch).

## Schlussrunde

- Sauberer Neubau (--clean-first) 0 Warnungen, 0 Fehler. Parallel (`Y:/werkzeuge/r3c/schluss.sh`, Summe `schluss_summe.txt`):
  ctest 245/245, Modus 1 2635/0, mission1 98/0, figurlicht 8/0, karteneditor rd 38/0, r3 22/0, editorportal 19/0, laserfalle 9/0,
  modellblock md 86/0, funktionen 61/0, rest605 moversprite 4/0, gluehen 9/0, teil10 3/0, teil13b 5/0. Lints wie die Basis
  (portability 99, pruefcodes 11, fremdprogramm 1, link/tidy nur unter Linux; d3dbesitz nach `// geliehen` an g_nebelZiel ok,
  merkmalsbits ok mit 28 Bits). Handbuch `--pruefen` ok. Keine Bildzeit-Messung (Auftrag).

## Changelog

## English

- Preview: shader stages that use the lightmap again after the first one (`map $lightmap` with `tcGen lightmap`, `tcMod`, blend and glow) are drawn like the game; `tcGen lightmap` is read.
- Preview: the fog is also drawn into the glow of surfaces with a glow stage, like the game - the hazy glow of duel_exegol.
- Preview: the map fog no longer fades when effect models that turn towards the camera shrink.
- Preview: weapons and items on the ground and turrets use the level of detail the game picks for their class.
- Preview: shine and chrome on scaled figures are exact also for a different scale per axis.

## Deutsch

- Vorschau: Shaderstufen, die nach der ersten noch einmal die Lightmap nehmen (`map $lightmap` mit `tcGen lightmap`, `tcMod`, Mischart und Glühen), werden wie im Spiel gezeichnet; `tcGen lightmap` wird gelesen.
- Vorschau: Der Nebel geht wie im Spiel auch in das Glühen von Flächen mit Glühstufe - der dunstige Schein von duel_exegol.
- Vorschau: Der Nebel der Karte verblasst nicht mehr, wenn Effektmodelle, die sich zur Kamera drehen, kleiner werden.
- Vorschau: Waffen und Gegenstände am Boden und Geschütze nehmen die Detailstufe, die das Spiel für ihre Klasse wählt.
- Vorschau: Glanz und Chrom an skalierten Figuren stimmen auch bei unterschiedlichem Maßstab je Achse.

## 中文

- 预览:在第一个阶段之后再次使用光照贴图的着色器阶段(带 `tcGen lightmap`、`tcMod`、混合方式和辉光的 `map $lightmap`)会像游戏一样绘制;会读取 `tcGen lightmap`。
- 预览:雾也会像游戏一样进入带辉光阶段的表面的辉光——duel_exegol 的朦胧光晕。
- 预览:朝向镜头的特效模型变小时,地图的雾不再消退。
- 预览:地上的武器和物品以及炮塔使用游戏为其类别选择的细节级别。
- 预览:缩放角色上的光泽和镀铬在各轴缩放不同时也准确。

## 日本語

- プレビュー: 最初のステージの後にもう一度ライトマップを使うシェーダーステージ (`tcGen lightmap`、`tcMod`、ブレンド、グロー付きの `map $lightmap`) をゲームと同じく描画します。`tcGen lightmap` を読み込みます。
- プレビュー: 霧はゲームと同じく、グローステージを持つ面のグローにも入ります。duel_exegol のかすんだ光です。
- プレビュー: カメラの方を向くエフェクトモデルが小さくなっても、マップの霧が薄れなくなりました。
- プレビュー: 地面の武器やアイテム、タレットは、ゲームがそのクラスに選ぶ詳細レベルを使います。
- プレビュー: 拡大縮小したキャラクターの光沢とクロームは、軸ごとに倍率が違っても正確です。

## Español

- Vista previa: las etapas de shader que vuelven a usar el lightmap después de la primera (`map $lightmap` con `tcGen lightmap`, `tcMod`, modo de mezcla y brillo) se dibujan como en el juego; se lee `tcGen lightmap`.
- Vista previa: la niebla también entra en el brillo de las superficies con etapa de brillo, como en el juego: el resplandor brumoso de duel_exegol.
- Vista previa: la niebla del mapa ya no se desvanece cuando se encogen los modelos de efecto que giran hacia la cámara.
- Vista previa: las armas y objetos en el suelo y las torretas usan el nivel de detalle que el juego elige para su clase.
- Vista previa: el brillo y el cromado de figuras escaladas son exactos también con una escala distinta por eje.

## Čeština

- Náhled: fáze shaderu, které po první znovu používají lightmapu (`map $lightmap` s `tcGen lightmap`, `tcMod`, režimem míchání a září), se kreslí jako ve hře; `tcGen lightmap` se načítá.
- Náhled: mlha jde jako ve hře i do záře ploch s fází záře - zamlžený svit v duel_exegol.
- Náhled: mlha mapy už nebledne, když se zmenšují efektové modely otočené ke kameře.
- Náhled: zbraně a předměty na zemi a věže používají úroveň detailů, kterou hra volí pro jejich třídu.
- Náhled: lesk a chrom na zvětšených postavách jsou přesné i při různém měřítku v jednotlivých osách.
