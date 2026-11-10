# Notizen MBRASTER: Boden-Raster im Model-Tab wie 3ds Max bzw. Blender (10.11.2026, `Y:\p_mbkopf`, Zweig `p_mbkopf`, auf MBKOPF 82cc9b1c8)

Auftrag `auftraege/auftrag_mbraster_1011.md`. shank 10.11.: „ich habe gesehen es gibt im model Tab keinen boden wie in 3ds max
oder blender. Kann man den einfuegen einen grid“. Dazu drei Bildfehler aus der MBKOPF-Abnahme: (a) Gegenkugel -Y des
Blender-Gizmos über der Kugel Y, (b) der ViewCube verdeckt das N des Kompasses, (c) Raster mit ViewCube/Gizmo, Auswahlklammer und
Boden zusammen ansehen. Ein Agent (Opus).

Quellen (online gelesen, nicht gestartet): Autodesk-Hilfe „Home Grid“ (Grid and Snap Settings, 3ds Max 2017:
https://help.autodesk.com/cloudhelp/2017/ENU/3DSMax/files/GUID-55F70CB0-0590-4F1E-9879-EF070AF11CE6.htm), „Viewport grid and
display improvements“ (3ds Max 2027: https://help.autodesk.com/cloudhelp/2027/ENU/3DSMax-What-s-New/files/GUID-5ADB2162-8023-4DE4-9827-74A37D24F933.htm),
die Farbthemen `ame-dark.clrx` / `ame-light.clrx` der installierten 3ds Max 2027 (nur gelesen); Blender 5.0 (installiert ist 5.0):
`source/blender/draw/engines/overlay/shaders/overlay_grid_frag.glsl`, `overlay_grid_vert.glsl`, `overlay_grid.hh`,
`overlay_instance.cc`, `overlay_shader_shared.hh`, `editors/space_view3d/view3d_draw.cc` (view3d_grid_steps_ex),
`editors/space_view3d/view3d_gizmo_navigate_type.cc` (gizmo_axis_draw), `release/datafiles/userdef/userdef_default_theme.c`
(https://github.com/blender/blender, Zweig blender-v5.0-release; Abschriften unter `Y:\werkzeuge\mbr\quellen\`).

## Aufbau

- `gui/gpumap.h` `ModellRaster`, `ModellAnsichtArt::raster`; `gui/gpumap_win32.cpp` ein eigener Durchgang in
  `zeichneModellAnsicht` (Shader, Konstantenpuffer b8, Misch-, Tiefen- und Rasterzustand, `gibRasterFrei`): ein Viereck aus 8 x 8
  Teilen ohne Eckpuffer (SV_VertexID). Feste Orthoansicht („hinten“, Blender GRID_BACK): nach Hintergrund und grauem Boden, vor
  Ghosts und Figur, ohne Tiefentest - die Figur liegt immer davor. Perspektive und freie Orthoansicht: nach der Figur mit
  Tiefentest LESS_EQUAL, ohne Tiefe zu schreiben, Tiefenverschiebung -16 / 2^24 und -1 je Neigung eines Bildpunkts - auf dem
  grauen Boden (dieselbe Ebene) gewinnt das Raster ohne Flimmern, die Figur verdeckt es (bei 100 Einheiten Abstand 0,01 Einheiten).
- `src/gpushader.cpp` `modellRasterVertexShaderHlsl` / `modellRasterPixelShaderHlsl`: die Linien wie Blender
  overlay_grid_frag.glsl (Dreieckswelle durch die Änderung je Bildpunkt geteilt - gleich dünn auch schräg -, LINE_STEP mit
  DISC_RADIUS, Achsen mit +0,1 Bildpunkt). Blender: Stufe aus grid_res (4 Bildpunkte), drei Stufen A/B/C überblendet, Ausblenden
  nach Blickwinkel ((1 - |V·n|)²)² und ab der halben Sichtweite; 3ds Max: feste Neben- und Hauptlinie, in der Perspektive bei der
  Ausdehnung abgeschnitten.
- `gui/app_modelltab_ansicht.cpp` `rasterFuer`: Ebene, Lage, Stufen, Farben je Ansicht; der graue Boden bleibt `bodenModell`
  (das alte Raster aus 17 x 17 Vierecken ist weg); `modellHintergrundFarbe`, `modellBodenAufTiefstenPunkt`,
  `modellGizmoKugelnZeichnen` (Blender-Teil des Gizmos, GPL wie die übrigen Blender-Teile dieser Datei).
- `gui/app_modelltab_kopf.cpp`: Katalog mit Modus (3ds Max / Blender) und `wirkt` je Ansicht; [+] > „Rastereinstellungen“ (Max);
  Overlays > „Hilfslinien“ (Blender: Raster, Boden, Achsen X Y Z in einer Zeile, Maßstab, Unterteilungen, Bodenhöhe); Suche und
  Rechtsklick über den Katalog. Der graue Boden heißt jetzt „Grauer Boden (ModView)“ (Blenders Floor heißt „Boden“).
- Einstellungen `modelGridSpacing` (8), `modelGridMajor` (8), `modelGridExtent` (7); in den Einstellungen Gruppe „Model-Tab:
  Raster“. Je Ansicht (`ModellSchattierung`, wie bisher nicht gespeichert): `raster` (ab Werk an), `rasterBoden` (an),
  `achsen` (X, Y an, Z aus).

## Entscheidungen

- **Ab Werk an** in allen Ansichten (3ds Max: Home Grid sichtbar; Blender: Floor, Grid, Axes X und Y an, Z aus).
- **3ds Max**: G schaltet das Home Grid samt Weltachsen der aktiven Ansicht. Perspektive: auf dem Boden, fest um den Ursprung,
  7 Linien weit („Perspective View Grid Extent“ 7, „Inhibit Perspective View Grid Resize“ an). Achsenansichten: in der Bildebene,
  bildfüllend; der Abstand wächst mit dem Zoom um den Faktor der Hauptlinien („the grid expands indefinitely to keep major
  divisions“), feiner als der Rasterabstand wird es nie („Inhibit Grid Subdivision Below Grid Spacing“ an). Gewechselt wird,
  wenn die Nebenlinien näher als 4 Bildpunkte stünden - Max nennt keine Schwelle, 4 Bildpunkte sind Blenders „the grid begins to
  appear when it comprises 4 pixels“. Freie Orthoansicht (U): die Bodenebene bildfüllend.
- **Farben 3ds Max** aus den Themen von 3ds Max 2027: GridIntensity #696969 (dunkel) bzw. #9e9e9e (hell, Grund ab Helligkeit
  0,5), „Grid“ #000000 für die Weltachsen. Die Hauptlinie ist dunkler als die Nebenlinie („heavier or major lines“); Max nennt
  keinen Wert: 3/4 der Helligkeit, damit sie auf dem hellsten Grau des Verlaufs (56) sichtbar bleibt (79).
- **Blender**: Portierung von overlay_grid (5.0): Stufen = Maßstab · Unterteilungen^i, in den Achsenansichten 3 Stufen feiner
  („Allow 3 more subdivisions“); Viereck um die Kamera so groß wie die Sichtweite (8192 wie baueSicht; Blender clip_end), Ausblenden
  ab 4096 (clip_end / 2); Farben aus dem Thema (grid #545454 +10 mit Alpha 0x80, grid_emphasis +20 bzw. -10 nach dem Hintergrund,
  Achsen 0,85 zu #ff3352 / #8bdc00 / #2890ff gemischt, -20). Z-Achse in Perspektive/freier Ansicht auf der der Kamera zugewandten
  Ebene XZ bzw. YZ (sync_view).
- **Bodenhöhe**: das Raster liegt in Bodenhöhe `an.bodenZ` (ab Werk -24 wie ModView und DEFAULT_MINS_2), nicht auf Z = 0 wie in
  Max und Blender: eine JKA-Figur hat ihren Ursprung in der Hüfte. In den Seitenansichten liegt die waagrechte Achse deshalb auf
  Bodenhöhe. Tooltips erklären das; „Tiefster Punkt“ legt den Boden unter die Figur in der Pose (auf 1/8 abgerundet; Requisiten
  mit Ursprung am Boden: 0).
- **Vorgaben**: Rasterabstand 8 wie das Raster des Karten-Editors (NetRadiant „Grid 8“), Hauptlinie alle 8 Linien = 64 Einheiten =
  Höhe eines Spielers (DEFAULT_MINS_2 -24 bis DEFAULT_MAXS_2 +40). Blender Maßstab/Unterteilungen = dieselben zwei Werte.
- **Gizmo (a)**: wie Blender gizmo_axis_draw - von hinten nach vorn, die vordere Gegenkugel deckend (Alpha min(Tiefe + 1, 1); bis
  hier immer 110/255), Farben zur Hintergrundfarbe ausgeblendet ((Tiefe + 1) · 0,25 + 0,5), Größe 0,92 bis 1,08, Beschriftung
  schwarz/unter der Maus weiß, Achsfarben des Blender-Themas. Kugelradius bleibt 0,22 (Blender 0,20) - Trefferflächen wie MBKOPF.
- **Kompass (b)**: die Buchstaben liegen nach dem Würfel obenauf, mit dunklem Rand; auch der Buchstabe hinter dem Würfel ist zu
  lesen und zu treffen. Abweichung von max_viewcube.h (die Vorlage zeichnet ihn davor - von vorne verschwindet er dort ebenso).

## Abweichungen von den Vorbildern

- Bodenhöhe statt Z = 0 (oben). Linien kantengeglättet (Max zeichnet harte Linien) - eigene Grafik.
- 3ds Max 2027 färbt die Weltachsen („Viewport axis are now colored“, Farben für positive/negative Achse einstellbar); die
  Standardwerte nennt die Hilfe nicht, die Themen der 2027 haben weiter „Grid“ #000000 - hier schwarz wie 3ds Max bis 2026.
- Blender prüft die Tiefe weich im Shader (linearstep gegen die Szenentiefe); hier die Grafikkarte mit Tiefenverschiebung - die
  Figur verdeckt hart. In den festen Orthoansichten liegt das Raster über dem grauen Boden (Blender: nur wo nichts ist) - der
  graue Boden zählt als Grund.
- Blenders Z-Achse in einem Durchgang (Blender teilt sie bei Z = 0 für die Reihenfolge der Durchsichtigkeit).
- Die Schalter „Inhibit …“ von Max gibt es nicht (immer an wie ab Werk in Max).

## Ergebnisse (rot → grün)

- Selbsttest `modellblock`, Gruppe `raster` (neu, `gui/selbsttest_mbraster.inc`): rot auf dem alten Stand mit dem neuen Test
  **6 OK / 20 FEHLER** (ab Werk aus, keine Achsen, Ortho nicht bildfüllend, kein Zoomwechsel, keine Unterteilungen, kein
  Ausblenden, helles Thema, grauer Boden mit Raster), grün **40 OK / 0 FEHLER** (der Test wuchs danach um Zusatzprüfungen: Figur
  verdeckt das Raster, Schalter je Ansicht, beide Themen, freie Orthoansicht).
- Gruppe `kopfab` (Bildfehler a und b): rot **0 / 2** (Fotos angesehen: Gegenkugel -Y halb durchsichtig über Y, das N hinter dem
  Würfel), grün **2 / 0**; `kopf` mit A/B am Ende **115 / 0**. Der erste kopf-Rotlauf war ungültig (Tab nicht offen bzw. N und S
  auf einer Stelle), gültig ist der kopfab-Lauf.
- Bildfehler c (Raster mit ViewCube, Gizmo, Auswahlklammer und grauem Boden): in den Fotos angesehen, Handbuchbild `h_mbr_vier`
  zeigt Raster, Klammer um den gewählten Rumpf und ViewCube zusammen.
- Schlussrunde: Bau 0 Fehler / 0 Warnungen (/W4 /WX, Teilbauten `bau_f` bis `bau_h`); ctest **245 von 246** - rot nur
  `q3map2_teilbau` (2 Fälle „zweiraum -bounce 2“), bekannt auf dem lcb2-Zwischenstand a00f6cf90 unter diesem Zweig, MBRASTER
  ändert an q3map2 nichts; `handbuch_sprachen` grün nach den neuen Bildern. `modellblock` alle Gruppen 0 FEHLER (u. a. raster
  40, kopf 115, kopfrot 25, funktionen 61, tasten 43, blender 67, mb5 159, md 86, pt 41, uv 50; die Rund- und Bildläufe auch
  für JKA und JK2, ptfix2 mit SPRACHE=ja 5/0, mb7textur mit BHED_MB7_LEER 1/0); Modus 1 **2635 / 0**; menueeditor **2787 / 0**;
  Handbuch 14 Gruppen x 6 Sprachen (84 Läufe) 0 FEHLER; Lints wie rc611 (portability 99, pruefcodes 11, fremdprogramm 1,
  efxflags-Hinweis; alle übrigen ok).
- Handbuchbilder: alle 37 Model-Tab-Bilder je Sprache neu (sie zeigen jetzt das Raster), 2538 x 1472, dazu `h_mbr_max`,
  `h_mbr_vier`, `h_mbr_blender`. Angesehen; zwei Bildschritte berichtigt: in `h_mbr_max` deckte der Tooltip von
  „Rastereinstellungen“ die Zeilen des Untermenüs zu (Maus jetzt auf dem freien Rand des Untermenüs), in `h_mbr_vier` passte
  das Einpassen nur den gewählten Rumpf ein, sodass die Orthoansichten kein Raster zeigten (jetzt `modellAllesEinpassenIn`).
  `tools/handbuch/gen_handbuch.py` kennt jetzt die Texte aus `tools/i18n_mbraster.py` (sonst brach es mit „unbekannter
  Oberflaechentext {{MrRasterEinst}}“ ab).

## Tests, deren Erwartung „Raster ab Werk an“ geändert hat

- `funktionen` (`gui/selbsttest_mb1a1.inc`, Anzeige-Schalter): das Grundbild schaltet das Raster jetzt ausdrücklich aus, dann G
  an/aus wie vorher - die Prüfung vergleicht Bilder mit und ohne Raster und brauchte einen Ausgangszustand ohne.
- `tasten` (`gui/selbsttest_mb1a1.inc`): das erste G schaltet aus, das zweite wieder an (vorher umgekehrt); rot 41/2 auf der
  alten Erwartung, grün 43/0.
- `kopf` (`gui/selbsttest_mbkopf.inc`): „[+] > Raster“ prüft das Umschalten statt „ist danach an“; 4a überspringt Schalter des
  anderen Modus und Schalter ohne Wirkung in dieser Ansicht (`modellKopfSchalterHier`: Blenders Grid wirkt nur in den
  Achsenansichten, Floor nur in den freien).
- Sonst keiner: die Bildvergleiche `bild` (552 Modelle), `bild2` und die Rundläufe zeichnen ohne `ModellAnsichtArt::raster`
  und blieben unverändert grün.

## Bildzeit

Richtwert, allein in der Messsperre (18:45, beim Anlegen 0 fremde Bau-/Testprozesse), ohne D3D-Debugschicht, Modus
`modellleistung` mit `BHED_MODELL1_NUR=raster` (Sturmtruppler läuft, jedes Bild neu gezeichnet, 120 Bilder je Block, zwei Runden
abwechselnd; Skript `Y:\werkzeuge\mbr\messung.sh`, Ergebnis `belege\messung1.out`):

| Block | Bild-CPU Mittel / p99 (ms) | Grafikkarte Median (ms) | Zeichenaufrufe |
|---|---|---|---|
| Raster aus | 0,43 / 0,72 · 0,43 / 0,84 | 0,073 · 0,052 | 70 |
| wie 3ds Max | 0,43 / 1,11 · 0,43 / 1,14 | 0,078 · 0,054 | 71 |
| wie Blender | 0,45 / 0,87 · 0,47 / 1,17 | 0,114 · 0,112 | 71 |

Ein Zeichenaufruf mehr; die CPU-Mittel bleiben gleich (Max) bzw. +0,02 bis 0,04 ms (Blender), die Grafikkarte +0,005 ms (Max)
bzw. rund +0,05 ms (Blender: das Viereck reicht bis zur Sichtweite, drei Stufen je Bildpunkt). Bildabstand im Mittel 4,2 bis
4,4 ms in allen Blöcken. Kein Ruhefenster-Wert; keine Entscheidung hängt daran.

## Gegen das Vorbild geprüft / nicht geprüft

- Geprüft gegen die Quellen: Blender-Shader und -Thema Zeile für Zeile (Stufen, LINE_STEP, Ausblenden, Farben), Gizmo
  gizmo_axis_draw; 3ds-Max-Verhalten aus der Hilfe (Home Grid, Extent 7, Inhibit-Schalter) und Farben aus den Themen von 2027.
- Nicht geprüft: ein Vergleichsbild aus 3ds Max oder Blender (Programme nicht gestartet); die Schwelle des Stufenwechsels in Max.

## Offen

- 3ds Max 2027 färbt die Weltachsen; die Standardfarben nennt die Hilfe nicht - hier schwarz wie bis 3ds Max 2026. Wer 3ds Max
  2027 ansehen darf, kann die Farben nachtragen.
- Die Schwelle des Max-Stufenwechsels (4 Bildpunkte) ist aus Blender entlehnt; Max nennt keine.
- Kein Vergleichsbild aus 3ds Max oder Blender (Programme werden nicht gestartet).
- `q3map2_teilbau` (2 Fälle) gehört zum lcb2-Zwischenstand unter diesem Zweig, nicht zu MBRASTER.

## Changelog

## English

- Model tab: every view now shows a floor grid from the start. In 3ds Max mode it is the home grid - thin lines, darker major lines, black world axes, on the floor in the perspective view (7 lines like Max), filling the orthographic views and growing with the zoom. In Blender mode it is Blender's floor - it fades out towards the horizon, X axis red, Y axis green (Z blue when switched on), subdivisions that change with the zoom in the side views. The figure hides the grid; lines stay one pixel thin even at a glancing angle.
- Model tab: grid switches per view - G or [+] > Grid (3ds Max), Overlays > Guides with Grid, Floor, Axes X Y Z, Scale and Subdivisions (Blender). [+] > Grid settings and the settings (Model tab: grid) set the spacing (8 JKA units like the map editor's grid), the major lines (every 8th line = 64 units, the height of a player) and the size in the perspective view. The grid lies at the floor height (-24, the feet of a JKA figure; 3ds Max and Blender use Z = 0); "Lowest point" puts it under the figure. The grey ModView floor is now called "Grey floor (ModView)" and can be shown together with the grid.
- Model tab: the navigation gizmo (Blender mode) draws its axis balls like Blender - the front ball covers the one behind, the opposite balls are opaque in front and fade towards the back; the compass letters below the ViewCube (3ds Max mode) stay readable when the cube is in front of them.

## Deutsch

- Model-Tab: jede Ansicht zeigt ab Werk ein Boden-Raster. Im Modus wie 3ds Max ist es das Home Grid - feine Linien, dunklere Hauptlinien, schwarze Weltachsen, in der Perspektive auf dem Boden (7 Linien wie in Max), in den Orthoansichten bildfüllend und mit dem Zoom wachsend. Im Modus wie Blender ist es Blenders Boden - er blendet zum Horizont aus, X-Achse rot, Y-Achse grün (Z blau, wenn eingeschaltet), in den Seitenansichten Unterteilungen, die mit dem Zoom wechseln. Die Figur verdeckt das Raster; die Linien bleiben auch schräg gesehen einen Bildpunkt dünn.
- Model-Tab: Schalter je Ansicht - G bzw. [+] > Raster (3ds Max), Overlays > Hilfslinien mit Raster, Boden, Achsen X Y Z, Maßstab und Unterteilungen (Blender). [+] > Rastereinstellungen und die Einstellungen (Model-Tab: Raster) stellen den Abstand (8 JKA-Einheiten wie das Raster des Karten-Editors), die Hauptlinien (jede 8. Linie = 64 Einheiten, die Höhe eines Spielers) und die Größe in der Perspektive ein. Das Raster liegt in Bodenhöhe (-24, die Füße einer JKA-Figur; 3ds Max und Blender nehmen Z = 0); „Tiefster Punkt“ legt es unter die Figur. Der graue ModView-Boden heißt jetzt „Grauer Boden (ModView)“ und lässt sich zusammen mit dem Raster zeigen.
- Model-Tab: das Navigations-Gizmo (Modus wie Blender) zeichnet seine Achsenkugeln wie Blender - die vordere Kugel deckt die hintere, die Gegenkugeln sind vorne deckend und blenden nach hinten aus; die Kompassbuchstaben unter dem ViewCube (Modus wie 3ds Max) bleiben lesbar, wenn der Würfel davor steht.

## 中文

- 模型选项卡: 每个视图默认显示地面栅格。3ds Max 模式下为主栅格——细线、较深的主线、黑色世界坐标轴, 透视图中位于地面 (与 Max 相同为 7 条线), 正交视图中填满视图并随缩放变化。Blender 模式下为 Blender 的地面——向地平线淡出, X 轴红色, Y 轴绿色 (开启后 Z 为蓝色), 侧视图中细分随缩放变化。角色会遮挡栅格; 即使倾斜观察, 线条也保持一个像素宽。
- 模型选项卡: 每个视图的开关——G 或 [+] > 栅格 (3ds Max), 叠加层 > 参考线, 含栅格、地面、坐标轴 X Y Z、缩放和细分 (Blender)。[+] > 栅格设置和设置 (模型选项卡: 栅格) 可设置间距 (8 个 JKA 单位, 与地图编辑器的栅格相同)、主线 (每 8 条一条 = 64 单位, 玩家的高度) 和透视图中的大小。栅格位于地面高度 (-24, JKA 角色的脚; 3ds Max 和 Blender 使用 Z = 0); “最低点”将其放到角色之下。灰色的 ModView 地面现在称为“灰色地面 (ModView)”, 可与栅格同时显示。
- 模型选项卡: 导航 Gizmo (Blender 模式) 像 Blender 一样绘制坐标轴球——前面的球遮住后面的球, 对面的球在前方不透明、向后淡出; ViewCube 下方的指南针字母 (3ds Max 模式) 在立方体位于其前方时仍可阅读。

## 日本語

- モデルタブ: どのビューにも最初から床グリッドが表示されます。3ds Max モードではホームグリッド - 細い線、濃いメジャーライン、黒いワールド軸。パースでは床の上 (Max と同じく 7 本)、平行投影ビューではビュー全体を埋め、ズームで変わります。Blender モードでは Blender の床 - 地平線に向かってフェードアウトし、X 軸は赤、Y 軸は緑 (オンにすると Z は青)、側面ビューではズームで細分化が変わります。フィギュアはグリッドを隠し、斜めから見ても線は 1 ピクセルの細さのままです。
- モデルタブ: ビューごとのスイッチ - G または [+] > グリッド (3ds Max)、オーバーレイ > ガイド (グリッド、床、軸 X Y Z、スケール、細分化) (Blender)。[+] > グリッド設定と設定 (モデルタブ: グリッド) で間隔 (8 JKA 単位、マップエディタのグリッドと同じ)、メジャーライン (8 本ごと = 64 単位、プレイヤーの高さ)、パースでの大きさを設定します。グリッドは床の高さ (-24、JKA フィギュアの足。3ds Max と Blender は Z = 0) にあり、「最低点」でフィギュアの下に置けます。灰色の ModView の床は「灰色の床 (ModView)」という名前になり、グリッドと一緒に表示できます。
- モデルタブ: ナビゲーションギズモ (Blender モード) は軸の球を Blender と同じように描きます - 手前の球が奥の球を隠し、反対側の球は手前で不透明、奥に向かってフェードします。ViewCube の下のコンパスの文字 (3ds Max モード) は、キューブが手前にあっても読めます。

## Español

- Pestaña Model: cada vista muestra de serie una rejilla del suelo. En modo 3ds Max es la rejilla de inicio: líneas finas, líneas principales más oscuras, ejes del mundo en negro; en perspectiva sobre el suelo (7 líneas como en Max), en las vistas ortográficas llena la vista y crece con el zoom. En modo Blender es el suelo de Blender: se desvanece hacia el horizonte, eje X rojo, eje Y verde (Z azul si se activa), en las vistas laterales subdivisiones que cambian con el zoom. La figura tapa la rejilla; las líneas siguen teniendo un píxel de grosor aunque se miren de lado.
- Pestaña Model: interruptores por vista - G o [+] > Rejilla (3ds Max), Superposiciones > Guías con Rejilla, Suelo, Ejes X Y Z, Escala y Subdivisiones (Blender). [+] > Ajustes de la rejilla y los ajustes (Pestaña Model: rejilla) fijan la separación (8 unidades de JKA, como la rejilla del editor de mapas), las líneas principales (cada 8 líneas = 64 unidades, la altura de un jugador) y el tamaño en perspectiva. La rejilla está a la altura del suelo (-24, los pies de una figura de JKA; 3ds Max y Blender usan Z = 0); «Punto más bajo» la pone bajo la figura. El suelo gris de ModView se llama ahora «Suelo gris (ModView)» y puede mostrarse junto con la rejilla.
- Pestaña Model: el gizmo de navegación (modo Blender) dibuja sus esferas de ejes como Blender: la esfera de delante tapa la de detrás, las opuestas son opacas delante y se desvanecen hacia atrás; las letras de la brújula bajo el ViewCube (modo 3ds Max) siguen siendo legibles cuando el cubo está delante.

## Čeština

- Karta Model: každý pohled od začátku ukazuje podlahovou mřížku. V režimu 3ds Max je to domovská mřížka - tenké čáry, tmavší hlavní čáry, černé osy světa; v perspektivě na podlaze (7 čar jako v Max), v ortografických pohledech vyplní pohled a roste s přiblížením. V režimu Blender je to podlaha Blenderu - k horizontu slábne, osa X červená, osa Y zelená (Z modrá, je-li zapnutá), v bočních pohledech dělení, které se mění s přiblížením. Postava mřížku zakrývá; čáry zůstávají jeden pixel tenké i při šikmém pohledu.
- Karta Model: přepínače pro každý pohled - G nebo [+] > Mřížka (3ds Max), Překryvy > Vodítka s mřížkou, podlahou, osami X Y Z, měřítkem a dělením (Blender). [+] > Nastavení mřížky a nastavení (Karta Model: mřížka) určují rozteč (8 jednotek JKA jako mřížka editoru map), hlavní čáry (každá 8. čára = 64 jednotek, výška hráče) a velikost v perspektivě. Mřížka leží ve výšce podlahy (-24, chodidla postavy JKA; 3ds Max a Blender používají Z = 0); „Nejnižší bod“ ji položí pod postavu. Šedá podlaha z ModView se nyní jmenuje „Šedá podlaha (ModView)“ a lze ji zobrazit spolu s mřížkou.
- Karta Model: navigační gizmo (režim Blender) kreslí koule os jako Blender - přední koule zakrývá zadní, protilehlé koule jsou vpředu neprůhledné a dozadu slábnou; písmena kompasu pod ViewCube (režim 3ds Max) zůstávají čitelná, i když je krychle před nimi.
