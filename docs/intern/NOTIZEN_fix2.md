# Notizen FIX2 (08.11.2026): Funde der Code-Pruefung seit rc610 ausserhalb von Painting/CJK

Auftrag `JKStudio_Uebergabe/auftraege/auftrag_fix2_0811.md`, Funde aus `plaene/codepruefung_seit_rc610.md` (PRUEF3).
Worktree `Y:\fix2`, Zweig `fix2` (Basis fe1f374c3), Build `Y:/fix2/build-fix2`, Laeufe `Y:/fix2/laeufe` (Vorsatz `f2_`),
Skripte `Y:\werkzeuge\f2\`.

## P5: Animationsimport, Bildzahl begrenzt

- Vorher: `n = floor(sekunden * fps) + 1` als `int` ohne Grenze; ein Schluessel bei 2e7 s (ms statt s) legte je Bild einen
  Vektor aller Knochen an (bad_alloc, Programmende), eine Zeit ueber int gab unter MSVC n = 1 und "gelesen".
- Jetzt: erst alle Animationen ausmessen (in double), dann ablehnen (`FehlerArt::AnimZuLang`: Bilder, MB noetig, MB frei),
  wenn alle Bilder nicht in `int` passen (`GlaAnimation::numFrames`, `AnimEntry::firstFrame`) oder die Bilder
  (je Bild und Knochen Welt `Starr` + lokale Stellung `BoneMatrix`, dazu je Bild ein Vektor) mehr brauchen als frei ist
  (`GlobalMemoryStatusEx` ullAvailPhys). Grund der Grenze: der Speicher, den der Import selbst braucht, und der Typ der
  Bildzahl - keine feste Zahl. `bad_alloc` beim Abtasten gibt dieselbe Meldung.
- Rot `f2_p5_rot_ctest` (ctest modell_animimport unter 3-GB-Jobgrenze, `Y:\werkzeuge\f2\jobgrenze.py`): Absturz
  0xC0000409 bei 3000 MB. Gruen `f2_p5_gruen_ctest`: 0 Fehler (2 s gelesen, 61 Bilder; 2e7 s und 1e9 s abgelehnt).
- Bytegleich: presetbau-Ausgabe der vier Testdateien und einer BVH mit 10000 Bildern vorher = nachher (`f2_p5_bytegleich.txt`).

## P14: Meldungen des Animationsimports in 6 Sprachen

- Fehler und Hinweise der Bibliothek als Art (`bhed/modell/fehler.h`: KeineAnimation, KeinSkelett, AchsenUnbestimmt,
  AnimZuLang, BvhOhneSeiten, BvhBoden, UngleicheSkalierung; `AnimImport::fehlerArt`, `hinweise`, `vornAusSkelett`),
  Text in der Sprache der Oberflaeche ueber `modellFehlerText`; assimp-Meldungen bleiben, wie assimp sie schreibt.
- Rot `f2_ai_rot` (Selbsttest modellblock/ai, en): 2 FEHLER ("(aus dem Skelett)", "keine Animation in der Datei").
  Gruen `f2_ai_gruen` 31/0 (en), `f2_ai_gruen_es` 31/0 (es: "del esqueleto", "no hay animación en el archivo").

## P17: presetbau --von/--bis

- Bereich ausserhalb der Animation gibt "FEHLER --von/--bis ..." statt Lesen ausserhalb von `roh`. ctests
  `presetbau_bereich`, `presetbau_bereich2` (testgang.bvh, 31 Bilder): rot 0/2 ("nichts zu schreiben" bzw. Schleife aus
  Bildern ausserhalb), gruen 2/2.

## P9: Animationsimport-Tempo

- `schluesselVor` lief je Bild, Knoten und Art vom ersten Schluessel; jetzt laeuft der Index je Spur mit (nur bei
  aufsteigenden Zeiten, sonst wie bisher von vorn - dasselbe i), `Decompose` der Ruhe einmal je Knoten.
- Bytegleich: FNV der Bilder und Grundstellungen der vier Testdateien gleich (`f2_p9_ctest.txt` gegen
  `f2_p5_gruen_ctest.txt`), presetbau-Ausgabe der langen BVH (10000 Bilder, 53 Gelenke, `Y:\werkzeuge\f2\langbvh.py`)
  sha256 2bffb0a20d62def7 vorher und nachher.
- Zeit (presetbau --einmal, `Y:\werkzeuge\f2\p9zeit.py`): vorher 2,72/2,68 s (ohne fremde Builds, 29 % Grundlast),
  im Wechsel gemessen 2,89 s vorher gegen 0,64-0,74 s nachher, waehrend andere Agenten bauten. **Messung ohne fremde
  Last offen.**

## P11: Karten-Editor Echtzeit-Licht, spielDarstellung zwischengespeichert

- Je Shader einmal gerechnet; neu nach Flush & Reload Shaders (`Editor::neuGeladen`), anderem Spielkontext (weak_ptr)
  und dem Leeren der Shader-Skripte in `weltAktualisieren` - dieselben Ereignisse wie `skript()` und `echtShaderInfo`.
  Aufrufer nehmen eine Referenz.
- Bildgleich: alle 32 Fotos von karteneditor r3 (8), rd (9) und editorportal (15, mit duel_executor, duel_wbw,
  duel_calodan) bytegleich vorher (`f2_*_vorher`, Basis-exe) und nachher (`f2_*_nachher`), `f2_p11_bildvergleich.txt`.
- **Bildzeit offen**: andere Agenten bauten und spielten Movie Duels; Messweg: `leistung` mit `BHED_LEISTUNG_NUR=e`,
  `BHED_LEISTUNG_MAP=Y:\werkzeuge\sp_af9c\s9c\taris\md_rev_TarisUpper2_converted.map`, `BHED_D3D_DEBUG=0`, Basis-exe
  `Y:\werkzeuge\f2\basis_bin` gegen den neuen Bau, allein.

## P10: Lizenzkopf der Blender-Teile

- `src/modell/wof/mesh_werkzeuge.cpp`: SPDX "BSD-3-Clause AND GPL-2.0-or-later", die Blender-Teile (segmentieren mit
  FaseEnde/FaseInfo, die Segment-Daten in chamferKern: selcount, tri_corner_test, pipe_test) zwischen Marken; als Ganzes
  GPL v2 wie JK Studio. THIRD-PARTY.md: Blender-Zeile nennt fase_blender.h und die markierten Teile; neue Zeile fuer die
  bisher nicht genannten Blender-Teile (weight_to_rgb, bone_octahedral_verts, paint_vertex_weight_utils, Pinsel-Abfall:
  gewichte.*, app_modelltab_ansicht.cpp). Fenster Ueber: Zeile `AboutBlenderLizenz` (6 Sprachen). Handbuch
  Modellieren > Herkunft und Lizenz (6 Sprachen).
- lint_sources prueft: jede Datei, die Blenders Quellen nennt (bmesh_*.cc), traegt GPL-2.0-or-later im Kopf und steht in
  der Blender-Zeile. Rot `f2_lint_p10_rot.txt` 3 Beanstandungen, gruen `f2_lint_p10_gruen.txt` ok.

## Wetter JK2: zwei "rain init" mit verschiedenem count

- OpenJK spielt JK2 mit rd-vanilla (`code/rd-vanilla/tr_WorldEffects.cpp`, kein JK2_MODE-Zweig): `R_WorldEffectCommand`
  liest nur das erste Wort, jedes "rain ..." legt eine neue Wolke an (bis MAX_PARTICLE_CLOUDS 5, :1891-1911). Zwei
  fx_rain mit verschiedenem count sind zwei Configstrings (`G_FindConfigstringIndex`, codeJK2/game/g_fx.cpp:305-321),
  also zwei Wolken; gleicher count einer. `gui/app_wetter.cpp` macht genau das - keine Aenderung. Nicht geprueft: der
  Renderer des Original-jk2sp.exe (Quelle nicht auf der Platte).

## P16: tcMod turb und rotate wie rd-vanilla

- Quelle (OpenJK `code/rd-vanilla`, MD9 gleich): `ComputeTexCoords` (tr_shade.cpp:1725) wendet jede tcMod der Reihe nach
  an; `RB_CalcTurbulentTexCoords` (tr_shade_calc.cpp:936) addiert `sinTable[(int)(((x + z) / 1024 + now) * 1024) & MASK]
  * amplitude`, `RB_CalcRotateTexCoords` (:1009) liest sin/cos ebenso aus der Tafel; `tr.sinTable[i] = sin(DEG2RAD(i * 360
  / 1023))` (tr_init.cpp:1727). Hier stand turb nach der ganzen Matrix (wie rend2) mit `sin(frac(...) * 2 pi)`, rotate mit
  genauem cos/sin.
- Jetzt: `gpu::texModAbbildung` gibt zusaetzlich den linearen Teil der Schritte hinter turb (turbMat); `gpu::turbWerte`
  rechnet 1024 now und seinen Rest modulo 1024 in double und die Matrix mal Amplitude. Neuer float4 am Ende der
  Stapelkonstanten (`gTexTurbMat`, Lage 44, Block 48 float), der Figurenstufe (`gStufeTurbMat`, kStufeFloats 56) und des
  Echtzeit-Lichts (`tcTurb`). Die vier Shaderstellen (Vorschau Karte und Figurenstufe, Echtzeit-Licht kShader3/4) nehmen
  den Index wie `(int)` (zur Null hin) `& 1023` aus der Tafel; rotate auf der CPU aus der Tafel. Mehrere turb in einer
  Stufe: der letzte zaehlte (wie vor FIX2) - in FIX3 behoben (Abschnitt unten).
- Rot `f2_p16_rot.txt` (ctest gpuconst, Shader-Rechnung gegen rd-vanilla an drei Ecken und drei Zeiten): turb + scale
  0,57, rotate 0,0026, scroll + turb 0,0017, turb + rotate + scale 1,18; gruen `f2_p16_gruen.txt`: alle 0,000000.
- Fotos (Bau mit P16, `f2_r3_p16`, `f2_rd_p16`, `f2_ep_p16`): alle 3D-Bilder bytegleich mit P11-nachher (die Testkarten
  zeigen kein turb und kein rotate); nur `karteneditor_start.bmp` weicht ab (Mauskoordinaten x -224/-240 in der
  Statusleiste - die Maus steht anderswo, kein Bildfehler).
- **Offen**: ein Spielfoto mit einem turb-Shader (z. B. Wasser mit turb + scale) gegen die Vorschau; ohne Spielstart
  hier nur gegen den rd-vanilla-Code geprueft (ctest).

## Schluss

- Sauberer Neubau (`--clean-first`, `f2_neubau.log`): 0 Warnungen.
- ctest 237/237 (`f2_s_ctest.txt`).
- Selbsttests auf dem Neubau: modellblock ai 31/0, pr 15/0, gluehen 9/0, figurlicht 8/0, Modus 1 2635/0. Mit dem Bau von
  P16 (Code gleich bis auf Kommentare): karteneditor r3 22/0, rd 20/0, editorportal 19/0.
- Lints: sources, cbuffer, i18n, i18n_erzeugt, settings, endtable, d3dbesitz, fassung, feldinit, gapp, tabellenid,
  merkmalsbits, abstand, bildvergleich, breitenabzug, doppelanzeige, headers, hoehe, imgui_alpha, imgui_context, layout,
  mapstate, msvclink, resize, rueckkopplung ok; portability 99, pruefcodes 11, fremdprogramm 1 (kartenpruefung.py),
  efxflags "keine Flagtabelle" wie vorher, keine Meldung zu geaenderten Dateien; link/tidy nur unter Linux.
- Handbuch `--pruefen`: handbuch_sprachen ok.

## Abweichungen

- P5: die Grenze ist der freie Arbeitsspeicher (und int fuer die Bildzahl), keine feste Bildzahl: OpenJK begrenzt eine
  Sequenz auf 65535 Bilder (`animation_t` unsigned short, code/game/bg_public.h:461), Movie Duels nicht (int, MD9
  bg_public.h:722) - eine feste Zahl haette lange Aufnahmen fuer Movie Duels abgelehnt.
- P10: die Blender-Teile bleiben in mesh_werkzeuge.cpp (markiert, Kopf "BSD-3-Clause AND GPL-2.0-or-later") statt nach
  fase_blender.cpp verschoben - sie brauchen viele Hilfen der Datei (PolyNetz, Vielecke, Plan).

## Offen

- Bildzeit P11 (Karten-Editor Echtzeit-Licht) und Zeit P9 ohne fremde Last (andere Agenten bauten/spielten).
- P16 Spielfoto mit turb-Shader (FIX3 Punkt 2).

## FIX3 Punkt 1: mehrere tcMod turb in einer Stufe

Auftrag `auftrag_fix3_0811.md`, Laufnamen `f3_`, Skripte `Y:\werkzeuge\f2\f3\`.

- Quelle: rd-vanilla `ComputeTexCoords` (tr_shade.cpp:1725) ruft fuer JEDE tcMod turb `RB_CalcTurbulentTexCoords` an ihrer
  Stelle der Kette (bis `TR_MAX_TEXMODS` 4, tr_local.h:279; MD9 gleich). Bisher galt nur der letzte turb einer Stufe.
- Jetzt: `gpu::texModAbbildung` liefert eine `TurbKette` (je turb Amplitude, Phase und der lineare Teil der Schritte hinter
  ihm), `gpu::turbFelder` die Werte fuer die Shader. Die Shader addieren je turb `turbVersatz` (eine HLSL-Funktion, von
  Karte, Figurenstufe und Echtzeit-Licht gemeinsam genutzt, `gpu::turbHlsl`).
  - Stapelblock: zweiter bis vierter turb in `gTurbMat2..4` (48, 52, 56), `gTurbQ23` (60) und `gTurbQ4` (13, in der Luecke
    hinter gAlphaSchwelle) - der Block bleibt 64 float, ein Ring-Eintrag (256 Bytes), kein zusaetzlicher Speicher je Stapel.
    Lagen aus dem Shader gelesen: "gTurbQ4@13 gTurbMat2@48 gTurbMat3@52 gTurbMat4@56 gTurbQ23@60 - passt".
  - Figurenstufe: `gStufeTurbMat2..4`, `gStufeTurbQ23`, `gStufeTurbQ4` (kStufeFloats 56 -> 76), im Ring zwei Eintraege je Stufe
    (`kStufeRingEintraege`).
  - Echtzeit-Licht: `tcTurb2..4`, `tcTurbQ23`, `tcTurbQ4` in B3 (kShader3/kShader4).
- Rot `f3_p1/f3_p1_rot.txt` (ctest gpuconst, Shader gegen rd-vanilla an drei Ecken und drei Zeiten): "turb, scale 2 3, turb"
  0,854, "turb, turb, rotate 30" 0,400, "turb, rotate 30, turb, scale 2 3" 1,200, "turb, turb, turb, turb" 0,423 (vier FEHL).
  Gruen `f3_p1/f3_p1_gruen.txt`: alle acht Faelle 0,000000; die Probe liest jetzt den gepackten Block an den Lagen der Shader.
  gpushader (Text: vier `+= turbVersatz(` in Karte und Figur), hlsl, figurstufen, gpudraw gruen.
- Selbsttests auf dem Bau: karteneditor r3 22/0 (`f3_p1_r3`), rd 20/0 (`f3_p1_rd`), editorportal 19/0 (`f3_p1_ep`),
  figurlicht 8/0. Fotos gegen FIX2-P16: r3 7, rd 8, editorportal 15 bytegleich; nur `karteneditor_start.bmp` weicht wie in FIX2
  ab (Mauskoordinaten in der Statusleiste). Die Testkarten dort zeigen kein turb - geprueft gegen die Spielformel per ctest,
  im Spiel per Punkt 2.

## Changelog

## English

- Animation import: a file whose last key lies far away (a time in milliseconds instead of seconds) gives the message "animation too long" instead of ending the program.
- Animation import: errors and hints (no animation, axes, BVH sides and floor, non-uniform scale, front from the skeleton) appear in the language of the interface.
- Animation import: long takes are read much faster (a 10000-frame BVH in a quarter of the time), with the same result.
- Map editor: the real-time light computes the game look of each shader only once instead of every frame - same picture.
- Preview and map editor: "tcMod turb" acts at its place in the chain (a following "tcMod scale" scales the wave like in the game) and, like "tcMod rotate", reads the game's sine table.
- Preview and map editor: several "tcMod turb" in one shader stage each act at their own place, as in the game (before, only the last one counted).
- About window, guide and THIRD-PARTY.md name the parts ported from Blender (chamfer with segments, weight colours, octahedral bones, weight brush falloff) and their licence GPL-2.0-or-later.

## Deutsch

- Animations-Import: eine Datei, deren letzter Schlüssel weit weg liegt (Zeit in Millisekunden statt Sekunden), gibt die Meldung „Animation zu lang“, statt das Programm zu beenden.
- Animations-Import: Fehler und Hinweise (keine Animation, Achsen, BVH-Seiten und -Boden, ungleichmäßige Skalierung, vorn aus dem Skelett) erscheinen in der Sprache der Oberfläche.
- Animations-Import: lange Aufnahmen werden viel schneller gelesen (eine BVH mit 10000 Bildern in einem Viertel der Zeit), mit gleichem Ergebnis.
- Karten-Editor: das Echtzeit-Licht rechnet das Spielbild jedes Shaders nur einmal statt in jedem Bild - gleiches Bild.
- Vorschau und Karten-Editor: „tcMod turb“ wirkt an seiner Stelle der Kette (ein folgendes „tcMod scale“ skaliert die Welle wie im Spiel) und liest wie „tcMod rotate“ die Sinustafel des Spiels.
- Vorschau und Karten-Editor: mehrere „tcMod turb“ in einer Shaderstufe wirken jeder an seiner Stelle wie im Spiel (bisher zählte nur der letzte).
- Fenster Über, Handbuch und THIRD-PARTY.md nennen die aus Blender portierten Teile (Chamfer mit Segmenten, Gewichtsfarben, Oktaeder-Knochen, Abfall des Gewichtspinsels) und ihre Lizenz GPL-2.0-or-later.

## 中文

- 动画导入: 最后一个关键帧位置很远的文件 (时间以毫秒而非秒表示) 会显示"动画过长"的提示, 而不再导致程序退出。
- 动画导入: 错误和提示 (没有动画、坐标轴、BVH 左右与地面、非均匀缩放、由骨架确定前方) 以界面语言显示。
- 动画导入: 长动作捕捉读取快得多 (10000 帧的 BVH 只需四分之一的时间), 结果相同。
- 地图编辑器: 实时光照对每个着色器的游戏外观只计算一次, 而不是每帧计算 - 画面相同。
- 预览和地图编辑器: "tcMod turb" 在链中自己的位置起作用 (随后的 "tcMod scale" 像游戏中一样缩放波动), 并像 "tcMod rotate" 一样读取游戏的正弦表。
- 预览和地图编辑器: 同一着色器阶段中的多个 "tcMod turb" 像游戏中一样各自在自己的位置起作用 (以前只有最后一个有效)。
- 关于窗口、手册和 THIRD-PARTY.md 列出从 Blender 移植的部分 (带分段的倒角、权重颜色、八面体骨骼、权重笔刷衰减) 及其许可 GPL-2.0-or-later。

## 日本語

- アニメーションのインポート: 最後のキーが遠く離れたファイル (時間が秒ではなくミリ秒) では、プログラムが終了せず「アニメーションが長すぎます」と表示されます。
- アニメーションのインポート: エラーとヒント (アニメーションなし、軸、BVH の左右と床、不均一なスケール、スケルトンから決めた前方) がインターフェースの言語で表示されます。
- アニメーションのインポート: 長いテイクの読み込みが大幅に速くなりました (10000 フレームの BVH が 4 分の 1 の時間)。結果は同じです。
- マップエディター: リアルタイムライトは各シェーダーのゲームでの見た目を毎フレームではなく一度だけ計算します - 画像は同じです。
- プレビューとマップエディター: 「tcMod turb」がチェーンの自分の位置で働き (後に続く「tcMod scale」がゲームと同じく波を拡大縮小)、「tcMod rotate」と同じくゲームのサインテーブルを使います。
- プレビューとマップエディター: 1 つのシェーダーステージにある複数の「tcMod turb」が、ゲームと同じくそれぞれ自分の位置で働きます (以前は最後の 1 つだけが有効でした)。
- バージョン情報、ガイド、THIRD-PARTY.md に Blender から移植した部分 (セグメント付きの面取り、ウェイトの色、八面体ボーン、ウェイトブラシの減衰) とそのライセンス GPL-2.0-or-later を記載しました。

## Español

- Importación de animaciones: un archivo cuya última clave está muy lejos (un tiempo en milisegundos en lugar de segundos) muestra el aviso "animación demasiado larga" en lugar de cerrar el programa.
- Importación de animaciones: los errores y avisos (sin animación, ejes, lados y suelo de BVH, escala no uniforme, delante según el esqueleto) aparecen en el idioma de la interfaz.
- Importación de animaciones: las tomas largas se leen mucho más rápido (un BVH de 10000 fotogramas en una cuarta parte del tiempo), con el mismo resultado.
- Editor de mapas: la luz en tiempo real calcula el aspecto de juego de cada shader una sola vez en lugar de en cada fotograma; la imagen es la misma.
- Vista previa y editor de mapas: "tcMod turb" actúa en su lugar de la cadena (un "tcMod scale" posterior escala la onda como en el juego) y, como "tcMod rotate", lee la tabla de senos del juego.
- Vista previa y editor de mapas: varios "tcMod turb" en una misma etapa de shader actúan cada uno en su lugar, como en el juego (antes solo contaba el último).
- La ventana Acerca de, la guía y THIRD-PARTY.md nombran las partes portadas de Blender (chaflán con segmentos, colores de peso, huesos octaédricos, caída del pincel de pesos) y su licencia GPL-2.0-or-later.

## Čeština

- Import animací: soubor, jehož poslední klíč leží daleko (čas v milisekundách místo v sekundách), zobrazí hlášení „animace je příliš dlouhá“ místo ukončení programu.
- Import animací: chyby a upozornění (žádná animace, osy, strany a podlaha BVH, nerovnoměrné měřítko, předek z kostry) se zobrazují v jazyce rozhraní.
- Import animací: dlouhé záznamy se načítají mnohem rychleji (BVH s 10000 snímky za čtvrtinu času), se stejným výsledkem.
- Editor map: světlo v reálném čase počítá herní vzhled každého shaderu jen jednou místo v každém snímku - obraz je stejný.
- Náhled a editor map: „tcMod turb“ působí na svém místě v řetězci (následující „tcMod scale“ škáluje vlnu jako ve hře) a stejně jako „tcMod rotate“ čte sinusovou tabulku hry.
- Náhled a editor map: více „tcMod turb“ v jedné fázi shaderu působí každý na svém místě jako ve hře (dříve platil jen poslední).
- Okno O programu, příručka a THIRD-PARTY.md uvádějí části převzaté z Blenderu (zkosení se segmenty, barvy vah, osmistěnné kosti, útlum štětce vah) a jejich licenci GPL-2.0-or-later.
