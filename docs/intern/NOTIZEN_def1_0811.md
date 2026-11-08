# Notizen DEF1: deformVertexes-Reste, Wolken, ERR_DROP-Pruefung (08.11.2026, `Y:\karte`, Zweig `karte1`, Basis 40539269d)

Build `Y:/karte/build-test` (`--parallel 8 -- /nodeReuse:false`), Laeufe `d1_*` unter `Y:\karte\laeufe`, Skripte und Protokolle
`Y:\werkzeuge\d1\`. Engine-Abgleich: OpenJK `Y:\werkzeuge\sp_654alt\ojk` (code/rd-vanilla/tr_shade_calc.cpp, tr_shader.cpp,
tr_surface.cpp), Movie Duels `Y:\werkzeuge\sp_654alt\md9` (gleich).

## Punkt 1: P44 Rest - jeder deformVertexes-Eintrag an seiner Stelle (e3a1a48f1)

- Shaderleser: `DeformZusatz::eintraege[3]` (shader.deforms[], hoechstens MAX_SHADER_DEFORMS), jeder Eintrag mit eigenen Werten; die
  Einzelfelder (deformWave, bulge, move, normal) fassen je Art den letzten wirksamen zusammen (Merkmale der Shader-Varianten).
- `gpu::verformung`: drei allgemeine Plaetze (je float4), d[12..14] die Spreads, d[15] die Folge zu je 4 Bit (kDefCode...: wave fest,
  wave mit Tafel 2..6, normal 7, bulge 8, move 9, autosprite 10, autosprite2 11). Die Konstantenlage bleibt (gDeform,
  gDeformSpread/Art/Normal.xy, gBulge, gDeformMove) - gpuconst, gpushader, hlsl unveraendert gruen.
- Karten-VS: `verformeEcke(.., von, bis)` vor und nach dem autosprite-Schritt; nach autosprite Normale -gVorn und st 0..1 wie
  RB_AddQuadStampExt (tr_surface.cpp:78-126). Das Plaettchen rundet jetzt wie RB_AddQuadStampExt (left/up = Achse * Radius, dann
  origin +- left +- up), die Mitte beim Hochladen wie AutospriteDeform (`0.25f * (x0 + x1 + x2 + x3)`, :409), autosprite2 precise.
- `gpu::deformLage`: was die Vorschau nicht an seiner Stelle zeigen kann, heisst Fehlt (wave/bulge vor autosprite - Mitte und
  Radius kommen im Spiel aus den einzeln verformten Ecken, der Vertex-Shader sieht nur die hochgeladene Mitte; wave, bulge, normal an
  autosprite2 - die Normale der Ecke weicht beim Hochladen der langen Achse; ein zweites autosprite). normal vor autosprite wirkt
  nicht (die Normalen ersetzt das Plaettchen), move davor verschiebt die Mitte (gleich bis auf die Rundung der Mitte).
- In allen Shadern der Spieldaten (base assets0-3, MD alle pk3, `Y:\werkzeuge\d1\scan2.py`) steht autosprite nie zusammen mit einer
  anderen Verformung und keine Art zweimal; elf Shader haben normal + wave (beide Reihenfolgen) - die waren schon richtig.
- Rot (`Y:\werkzeuge\d1\rot_p44.txt`, Stand 40539269d mit alten Headern und alter bhed_core.lib): zwei wave -> 1 Eintrag, zwei move
  -> 1 Eintrag, autosprite + wave -> wave ohne Stelle (vor dem Drehen auf der Mitte mit der Richtung der Ecke als Normale).
- Gruen (`Y:\werkzeuge\d1\p44_defgpu.txt`, ctest defgpu, WARP und Intel UHD Graphics, je 8192 Ecken): zwei wave, bulge/wave/bulge,
  normal/normal/wave, zwei move bitgleich (Karte, Figur, verformeEcke); autosprite, autosprite+wave+bulge, autosprite+normal+wave
  bitgleich zu AutospriteDeform + RB_DeformTessGeometry; move+autosprite bis 16 ulp der groessten Koordinate; autosprite2 + move:
  der Unterschied mit/ohne move ist genau der Versatz des Spiels. gpuconst (Plaetze, Codes, deformLage), gpushader, hlsl gruen.
- Erster Gruenlauf zeigte zwei Testfehler und einen echten: autosprite2 + move auf der Intel UHD 328 von 8192 Ecken anders -
  der autosprite2-Schritt war nicht precise, die Varianten rechneten ihn verschieden; behoben (precise).

## Punkt 2: P47 - Folge-Ende und normal-Zweig nur mit kDeformNormal (444cdc90f)

- `if (art == 0u) break;` (dicht gepackt), R_NoiseGet4f/Q_rsqrt und der normal-Zweig nur unter `VERFORM_NORMAL` (Kartenvarianten
  mit kDeformNormal; Figur und Probe immer).
- Gleiche Bits: ctest defgpu und hlsl gruen.
- Uebersetzungszeit (`Y:\werkzeuge\d1\uezeit.cpp`, vs_5_0, Fahnen 0, min aus 5, vorher/nachher abwechselnd zweimal,
  `uezeit_p47.txt`): Kartenvariante ohne normal 0,94-1,8 s -> 67-180 ms (26044 -> 21608 Byte); mit normal und Figur im Rauschen
  des Rechners gleich (0,9-1,4 s bzw. 1,5-2,8 s). Bildzeit nicht gemessen (Auftrag).

## Punkt 3: P46 - Wolkenstufe als ps_5_0 (8a5da193f)

- Rot (`Y:\werkzeuge\d1\p46_rot.txt`, ctest turbgpu mit neuem Pixel-Shader-Fall, der Kette wie in der Wolkenstufe, ps_4_0 wie bisher):
  WARP 0 abweichend, Intel UHD Graphics scroll 0, rotate+scale 6487 von 32768, scale/scroll/rotate/transform 12867 von 32768.
- Behebung: `gpu::wolkenPsZiel` - ab Merkmalsstufe 11_0 ps_5_0 (wie kartenVsZiel), holeShader nimmt es fuer kWolke.
- Gruen (`Y:\werkzeuge\d1\p46_gruen.txt`): ps_5_0 auf WARP und Intel UHD 0 von 32768 in allen drei Ketten; hlslprobe uebersetzt die
  Wolkenstufe mit wolkenPsZiel (110 Shader, 0 Fehlschlaege).

## Punkt 4: P45 Rest - der Optimierer meldet Shader mit ERR_DROP (a2ecc73c4)

- Neue Pruefung `spielabbruch` (Fehler) im Optimierer des Karten-Editors, Titel/Erklaerung/Angabe in 6 Sprachen (tools/i18n_def1.py,
  KeOptPrSpielAbbruch, KeOptPrSpielAbbruchE, KeOptD37), im Handbuch in der Pruefungstabelle (inhalt_karte5.py).
- Erkennung in `skriptBlock` (Werte nur auf der Zeile der Anweisung wie COM_ParseExt(text, qfalse); `ShaderDatei::zeilen`):
  - deformVertexes (ParseDeform tr_shader.cpp:1814-1943): ohne Art zaehlt die Zeile nicht, ab der vierten verworfen, ohne div bzw.
    dritte Zahl DEFORM_NONE; wave: frequency 0 -> EvalWaveForm (noise/random gehen, fehlende Form nicht), sonst TableForFunc;
    move: immer TableForFunc (tr_shade_calc.cpp:32-52, :129-170, :263-280).
  - rgbGen/alphaGen wave ohne Form (RB_CalcWaveColor/Alpha -> EvalWaveForm; die Form bleibt ueber ein zweites "wave" ohne Form
    stehen, eine andere bekannte Art ersetzt die Welle, eine unbekannte nicht), tcMod stretch ohne Form, fuenf tcMod in einer Stufe
    (ParseTexMod :504, beim Laden).
- Unbekannte Wellennamen sind sin (NameToGenFunc) und brechen nicht ab.
- Rot (`Y:\werkzeuge\d1\rot2_ctest.txt`, ctest karte_optimierer): 9 Abbruch-Faelle nicht gemeldet, Befund fehlt (der Befund
  hing dabei noch an shaderBekanntGeprueft - in pruefOpt() aus, die Pruefung braucht nur o.shader; behoben).
- Gruen (`Y:\werkzeuge\d1\gruen45_46.txt`): karte_optimierer bestanden (18 Faelle, Befund, Vorlage KeOptD37).

## Punkt 5: Chamfer-Feinheiten - nicht bearbeitet

- Grund: Umfang (drei Teile aus bmesh_bevel.cc: Naehte bei ungeraden Segmenten mit choose_rep_face/UV-Inseln, UV ueber
  Vertreter-Flaechen fuer Ecken/Faecher/Streifen, ein Ringpunkt zwischen zwei gefasten Kanten mit offset_meet) gegen die Schlussrunde;
  Stand der Lektuere in NOTIZEN_ptfix3_0811.md "Punkt 3".

## Schlussrunde (`Y:\werkzeuge\d1\schluss.sh`, parallel, Summe `schluss_summe.txt`)

- Sauberer Neubau (`--clean-first`) 0 Warnungen, 0 Fehler (`neubau.txt`).
- ctest 243/243, Modus 1 2635/0, mission1 98/0, figurlicht 8/0, editorportal 19/0, karteneditor rd 21/0, r3 22/0, modellblock md
  86/0, funktionen 61/0.
- Lints wie die Basis: portability 99 (erst 100 - `<cstdint>` in gpuconst.h fehlte, nachgezogen), pruefcodes 11,
  efxflags-/fremdprogramm-Meldung bekannt, cjkumbruch 0, link/tidy nur unter Linux; Handbuch `--pruefen` ok (6 Sprachen).
- Handbuch neu erzeugt (gen_i18n, gen_handbuch, gen_hilfe_orte): neue Zeile in der Pruefungstabelle des Optimierers.
- Kein Spielfoto: Punkt 1 aendert in den Shadern der Spieldaten nur die Normale von autosprite-Flaechen (jetzt -vorn wie im Spiel,
  sichtbar nur mit tcGen environment oder Modell-Licht); keine Bildzeit-Messung (Auftrag).

## Offen

- P44: wave/bulge vor autosprite, wave/bulge/normal an autosprite2 und ein zweites autosprite zeigt die Vorschau nicht (in keinem
  Shader der Spieldaten); dafuer braeuchte der Vertex-Shader alle vier Ecken des Vierecks (eigener Puffer je Netz).
- P44: an einem Mover mit autosprite nimmt die Vorschau -gVorn im Raum des Modells als Normale des Plaettchens, das Spiel die
  Weltachse (backEnd.viewParms.ori.axis[0]) - wirkt nur auf Eintraege hinter dem autosprite und tcGen environment.
- P44: autosprite2 selbst rechnet weiter nicht Bit fuer Bit wie Autosprite2Deform (normierte Achse beim Hochladen, Nebenachse
  q * (l / |q|) statt VectorNormalize).
- P45: die Pruefung kennt die Shader, die der Optimierer aus dem Texturinventar liest (Shaderdateien des Spiels und der Karte);
  nicht gegen ein echtes Spiel mit ERR_DROP geprueft (keine Spielstarts im Auftrag).
- Punkt 5 (Chamfer) ganz.

## Changelog

## English

- Shaders: several deformVertexes lines of the same kind all act, each with its own values, and autosprite acts at its place in the order like in the game (what follows it moves the camera-facing quad).
- Map editor, optimizer: new check for shaders that stop the game with an error (ERR_DROP) - a wave without a valid wave function or more than four tcMod in one stage.
- Sky: the clouds compute their moving texture exactly like the game on every graphics card (also on Intel).
- Faster loading: map shaders with deformVertexes but without "normal" compile about ten times faster.

## Deutsch

- Shader: Mehrere deformVertexes-Zeilen derselben Art wirken alle, jede mit ihren eigenen Werten, und autosprite wirkt wie im Spiel an seiner Stelle der Reihenfolge (was danach kommt, bewegt das zur Kamera gedrehte Viereck).
- Karten-Editor, Optimierer: neue Prüfung für Shader, an denen das Spiel mit einem Fehler (ERR_DROP) abbricht - eine Welle ohne gültige Wellenform oder mehr als vier tcMod in einer Stufe.
- Himmel: Die Wolken rechnen ihre bewegte Textur auf jeder Grafikkarte genau wie das Spiel (auch auf Intel).
- Schnelleres Laden: Kartenshader mit deformVertexes, aber ohne „normal“ übersetzen etwa zehnmal schneller.

## 中文

- 着色器: 同一类型的多行 deformVertexes 全部生效, 每行使用自己的数值; autosprite 与游戏一样在其顺序位置生效 (其后的变形作用于朝向摄像机的四边形)。
- 地图编辑器, 优化器: 新增检查, 找出会使游戏以错误 (ERR_DROP) 中止的着色器 - 没有有效波形函数的波, 或一个阶段中超过四个 tcMod。
- 天空: 云的移动纹理在每块显卡上都与游戏完全一致地计算 (包括 Intel)。
- 加载更快: 带 deformVertexes 但没有 "normal" 的地图着色器编译速度约快十倍。

## 日本語

- シェーダー: 同じ種類の deformVertexes を複数行書くと、それぞれ自分の値ですべて効くようになり、autosprite はゲームと同じく順番どおりの位置で効きます (その後の変形はカメラを向いた四角形に効きます)。
- マップエディター、オプティマイザー: ゲームをエラー (ERR_DROP) で停止させるシェーダーの新しいチェック - 有効な波形のない波、または 1 つのステージに 5 つ以上の tcMod。
- 空: 雲の動くテクスチャは、どのグラフィックカードでもゲームとまったく同じに計算されます (Intel でも)。
- 読み込みの高速化: deformVertexes を使い "normal" を使わないマップのシェーダーは約 10 倍速くコンパイルされます。

## Español

- Shaders: varias líneas deformVertexes del mismo tipo actúan todas, cada una con sus propios valores, y autosprite actúa en su lugar del orden como en el juego (lo que viene después mueve el cuadrilátero orientado a la cámara).
- Editor de mapas, optimizador: nueva comprobación de shaders que detienen el juego con un error (ERR_DROP): una onda sin función de onda válida o más de cuatro tcMod en una etapa.
- Cielo: las nubes calculan su textura en movimiento exactamente como el juego en cualquier tarjeta gráfica (también en Intel).
- Carga más rápida: los shaders de mapa con deformVertexes pero sin "normal" se compilan unas diez veces más rápido.

## Čeština

- Shadery: více řádků deformVertexes stejného druhu nyní působí všechny, každý se svými hodnotami, a autosprite působí jako ve hře na svém místě v pořadí (co následuje, pohybuje čtyřúhelníkem natočeným ke kameře).
- Editor map, optimalizátor: nová kontrola shaderů, na kterých se hra zastaví s chybou (ERR_DROP) - vlna bez platné vlnové funkce nebo více než čtyři tcMod v jedné fázi.
- Obloha: mraky počítají svou pohyblivou texturu na každé grafické kartě přesně jako hra (i na Intelu).
- Rychlejší načítání: shadery map s deformVertexes, ale bez „normal“ se překládají asi desetkrát rychleji.
