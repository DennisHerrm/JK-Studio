# Notizen FIX7: Merge fix6 auf karte1 nach MERGE3, PRUEF8-Funde P60, P61, P62 (08.11.2026, `Y:\fix2`, Zweig `fix7`, Basis 40539269d)

Build `Y:/fix2/build-fix2` (`--parallel 8 -- /nodeReuse:false`), Laeufe `f7_*` unter `Y:\fix2\laeufe`, Skripte und Protokolle
`Y:\werkzeuge\f2\f7\` (Rotproben `rot\`). Engine-Quellen: Movie Duels `Y:\werkzeuge\sp_654alt\md9`, OpenJK
`Y:\werkzeuge\sp_af9c\m3\ojq\src`.

## Merge fix6 (dd610be38)

- Einziger Textkonflikt `src/gpushader.cpp`, aufgeloest als `sE = entGegeben ? gEntSkala.xyz : achsLaenge;` (Kommentar aus fix6
  mit gEntSkala). Sauberer Neubau danach 0 Warnungen. Der alte figurglanz (Lage 11/74/75, 76 float) lief nach dem Merge gruen -
  auch mit absichtlich `achsLaenge.yxz` (Rotprobe r1: OK 0 Fehler) = still, wie PRUEF8 P60 beschreibt.

## P60: Lagen von JeStufe (b3) an einer Stelle, Lagenpruefung

- `bhed/gpushader.h`: kStufeArtAt ... kStufeEntSkalaAt, kStufeFloats (72) und die Tafel `kStufenLagen`; gpumap_win32.cpp
  (stufenWerte, texModWerte, deformWerte, entityWerte, Auge/Achsen - jetzt eine Hilfe `raumWerte` fuer beide Wege), defgpu und
  figurglanz nutzen sie.
- `bhed/cblagen_win32.h` `cbLagenPruefen`: Versaetze und Groesse eines Konstantenpuffers per D3DReflect gegen eine Tafel.
  pruefeStapelLagen nutzt es jetzt (gleiches Verhalten), neu `pruefeStufenLagen` beim ersten Figurenshader (Fehler im Protokoll).
- figurglanz: Lagenpruefung, Profil vs_5_0 wie kartenVsZiel, Lagen aus gpushader.h.
- Rot: r2 (neuer Test, Konflikt falsch aufgeloest `.yxz`) P50 0 von 5 Ecken FEHL; r3 (ein float4 vor gEntSkala eingeschoben)
  "Groesse 76(soll 72) ... gEntSkala@72(soll 68)" FEHL. Gruen: alle 5 Zeilen ok.

## P61: Ersatzweg mit der Ghoul2-Wurzel in welt

- Befund genauer als PRUEF8: Alle Ersatzweg-Aufrufer mit Stufen (Gegenstaende, Geschuetze, Abwuerfe) geben welt mit der
  Ghoul2-Wurzel (g2WurzelWelt seit rc608 Teil 4: Spalte 0 = axis[1], Spalte 1 = -axis[0]). Im Spiel liegt die Wurzel in den Knochen
  (rootMatrix = identityMatrix "0 -1 0 / 1 0 0 / 0 0 1", tr_ghoul2.cpp:102-108, G2_TransformGhoulBones :1811 MD9), tess.xyz und
  tess.normal also im Raum von ent->e.axis. Die Vorschau nahm die normierten Spalten als Achsen: tcGen environment um die Wurzel
  gedreht und der Teiler |Spalte 0| = |axis[1]| (bei gleichem Massstab je Achse nur die Drehung sichtbar).
- Behebung: `gpu::entityAusWelt(welt, mitWurzel, ...)` (gpuskin) und `FigurLicht::weltMitWurzel`; zeichneFigurIn gibt dann Achsen,
  Massstab und Ursprung als Angabe (gEntSkala) mit. Gesetzt bei Gegenstaenden/Geschuetzen und Abwuerfen.
- Dazu: die Huelle des Schwertleuchtens (CG_Item, derselbe refEntity) wurde mit gg.welt OHNE Wurzel gezeichnet, also um 90 Grad
  gegen den Griff verdreht; jetzt mit dessen welt.
- Probe: figurGlanz-Fall mit Wurzel (Massstab 2/1/1) gegen die Achsen der Entity mit entSkala - Rot r4 (Ersatzweg wie vorher)
  0 von 5 Ecken FEHL, gruen 5 von 5 bitgleich. Selbsttest laserfalle "Gegenstandachsen" (FigurAchsenEintrag mit welt und Massstab,
  die erste Falle zum Test mit Schwertleuchten, t3_byss, 13 Fallen): rot Abweichung Achsen 1,00000, ohne Massstab 13 -> gruen
  0,00000 / 0, welt 0. Die Huelle wurde im Selbsttest nicht gezeichnet (1 Aufruf an der ersten Falle; texturenMitShader fand
  gfx/effects/solidWhite_cull offenbar nicht in der Shadertabelle) - der Huellen-Teil ist daher nur am Code begruendet, nicht rot/gruen
  belegt; die Pruefung zaehlt ihn nur.

## P62: fehlende Haut -> ohne Haut wie im Spiel

- G_SetG2PlayerModel (g_client.cpp:2287-2370 MD9, :1730-1789 OpenJK, codeJK2 :788-802): RE_RegisterSkin 0 -> G2API_SetSkin(..., 0)
  setzt keine Haut und keine Flaechen (G2_API.cpp:907-929), die Figur zeichnet die Shader der .glm. Kein Rueckfall auf
  model_default.skin/model.skin. Ein customSkin mit '|' ist auch ohne fuehrenden Strich dreiteilig (strchr, ".../|<customSkin>").
- `figurHautAnwenden` danach, das Vorwaermen nimmt dieselbe Funktion (vorher eigene Kopie der alten Folge).
- Selbsttest `modellblock` mit `BHED_MODELL_NUR=haut`: rot (alte Folge) stormtrooper model_p62_gibtesnicht Haut 1 / 39 Flaechen
  mit Textur, jedi_hm mit fehlendem Teil Haut 1 / 32, ohne fuehrenden Strich nicht gleich der dreiteiligen Haut -> gruen 0 / 0
  (gleich .glm), 0 / 0, gleich; default 39 und dreiteilig 28 Flaechen mit Textur unveraendert.

## Schluss

- Abgebrochen auf Wunsch des Hauptchats (ein Agent): kein grosser Schlusslauf. Bau 0 Warnungen; ctest figurglanz, hlsl, defgpu,
  gpushader, gpuskin, gpuconst, modell, pruefung 8/8; Selbsttests laserfalle 9/0, modellblock haut 1/0. Laufzeit-Lagenpruefung im
  Protokoll "GPU-Stufenpuffer ... - passt".
- Falle beim Bauen: MSBuild baute nach Aenderungen nur in .inc-Dateien selbsttest.cpp nicht neu, und mit shutil.copy2
  zurueckgespielte Quellen (alte Zeitmarke) wurden nicht neu uebersetzt - vor Rot/Gruen-Laeufen die .cpp anfassen.
- Nicht gelaufen: ctest gesamt, figurlicht, mission1, modellblock funktionen, Modus 1, Lints, sauberer Neubau nach den Punkten.

## Offen

- Huelle des Schwertleuchtens im Programm belegen (Karte mit weapon_saber oder Shadertabelle pruefen).
- Volle Schlussrunde (siehe oben).

## Changelog

## English

- Preview: chrome and shine of Ghoul2 items, turrets and dropped weapons are computed in the entity's own axes like the game (they were turned by the Ghoul2 root), and the glow shell of a dropped lightsaber lies exactly on the hilt.
- Mission preview: a character whose skin file is missing or empty is drawn with the shaders of its model like the game, instead of the default skin; three-part skins in .npc files without a leading bar are recognised.

## Deutsch

- Vorschau: Chrom und Glanz von Ghoul2-Gegenständen, Geschützen und fallengelassenen Waffen werden wie im Spiel in den Achsen ihrer Entity gerechnet (sie waren um die Ghoul2-Wurzel gedreht), und die Leuchthülle eines liegenden Lichtschwerts liegt genau auf dem Griff.
- Mission-Vorschau: Eine Figur, deren Hautdatei fehlt oder leer ist, wird wie im Spiel mit den Shadern ihres Modells gezeichnet statt mit der Standardhaut; dreiteilige Häute in .npc-Dateien ohne führenden Strich werden erkannt.

## 中文

- 预览: Ghoul2 物品、炮塔和掉落武器的镀铬与高光与游戏一样按其实体自身的轴计算 (之前被 Ghoul2 根旋转), 掉落光剑的发光外壳正好覆盖在剑柄上。
- 任务预览: 皮肤文件缺失或为空的角色与游戏一样使用其模型的着色器绘制, 而不是默认皮肤; .npc 文件中没有前导竖线的三段式皮肤也能识别。

## 日本語

- プレビュー: Ghoul2 アイテム、タレット、落とした武器のクロームと光沢をゲームと同じくエンティティ自身の軸で計算します (以前は Ghoul2 ルートの分だけ回転していました)。落ちているライトセーバーの発光シェルは柄にぴったり重なります。
- ミッションプレビュー: スキンファイルがない、または空のキャラクターは、デフォルトスキンではなくゲームと同じくモデルのシェーダーで描画します。.npc ファイルで先頭の縦線がない 3 分割スキンも認識します。

## Español

- Vista previa: el cromo y el brillo de los objetos Ghoul2, torretas y armas caídas se calculan en los ejes de su propia entidad como en el juego (estaban girados por la raíz Ghoul2), y la envoltura luminosa de un sable de luz en el suelo queda justo sobre la empuñadura.
- Vista previa de misión: un personaje cuyo archivo de piel falta o está vacío se dibuja con los shaders de su modelo como en el juego, en lugar de la piel por defecto; se reconocen las pieles de tres partes en archivos .npc sin barra inicial.

## Čeština

- Náhled: chrom a lesk předmětů Ghoul2, věží a upuštěných zbraní se počítají jako ve hře v osách jejich entity (byly otočené o kořen Ghoul2) a svítící obal ležícího světelného meče leží přesně na rukojeti.
- Náhled mise: postava, jejíž soubor kůže chybí nebo je prázdný, se kreslí jako ve hře shadery svého modelu místo výchozí kůže; třídílné kůže v souborech .npc bez úvodní svislé čáry se rozpoznají.
