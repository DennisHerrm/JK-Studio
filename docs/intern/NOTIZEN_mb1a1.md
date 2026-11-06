# Notizen Modell-Block 1a-1: Model-Tab von Grund auf fuer JKA - Dokument, Gestell, Ansehen/Pruefen (ModView), Modus wie 3ds Max

Auftrag `auftrag_mb1a1.md`. Direkt in `Y:\karte` auf `karte1` ab 4c3c86e7 (rc610), Build `build-test`, Laeufe `laeufe/mb1_*` (rot `mb1_rot` mit `laeufe/mb1_exe_rot`, gruen `mb1_g*`, `mb1_t*`, Bildvergleich `mb1_b*`, Handbuch `mb1_hb*`, Schluss `mb1e_*`), Laufskript `lauf.py` wie rc610-Endrunde. kFassung bleibt rc610. Kein Spiel gestartet, 3ds Max und Blender nicht gestartet.

## Plan (vor dem Bau)

- Datenstruktur `bhed::modell` (`include/bhed/modell/`, `src/modell/`): `Ecke` (wie GlmVertex), `Halbkante` (Ecke, Punkt, naechste, gegen), `Netz` (Ecken, Punkte = Ecken gleichen Orts, Halbkanten, 3 je Dreieck), `Teil` (GLM-Flaeche mit Eltern/Kindern, Flaggen, Shader, ein Netz je LOD; Tag = ISBOLT), `Inhalt`, `Dokument` (Undo/Redo als Schnappschuesse, Tiefe 32). Laden ueber `readGlm(bytes, out, fehler, stufe)` je LOD und `readMd3`; `zuGlm` gibt je Stufe genau das GlmModel von readGlm.
- `g2.h`: Ghoul2-Instanz wie im Spiel (G2_SetSurfaceOnOff, G2_SetRootSurface, G2API_SetSurfaceOnOffFromSkin, RenderSurfaces, G2_ComputeLOD/ProjectRadius, Grenzen und Pruefungen je Zielspiel).
- Oberflaeche `gui/app_modelltab.cpp` (Dokument, Anzeige, Bereiche, Explorer, Befehlsfeld, Tasten) und `gui/app_modelltab_ansicht.cpp` (Ansichten, Pose, Ueberlagerung, Maus, Gizmo, Foto), Zustand `gui/modelltab.h` (`App::mtab`).
- Tests: ctest `modell_dokument`, Selbsttest `BHED_EDITORTEST=modellblock` (`BHED_MODELL_NUR=bild|funktionen|tasten`), Handbuch `BHED_HB_NUR=mb1a1`.

## Umsetzung

- **Modelldokument**: alle LOD-Stufen (readGlm hat dafuer den Parameter `stufe`; Stufe n folgt ueber mdxmLOD_t::ofsEnd). Halbkanten mit Gegenkante ueber verschweisste Punkte (UV-Naehte verbinden), entartete Dreiecke ohne Nachbarn, nicht mannigfaltige Kanten gezaehlt; `randEcken` fuer den Rissfinder. Undo ueber `aendern`, Ziehen des Gizmos ueber `vorschau` (ein Schritt je Zug). `ausMd3` (starr, jede Flaeche gezeichnet), `ausGlmModell` als Rueckfall ohne Bytes.
- **Ghoul2 wie im Spiel** (OpenJK `code/rd-vanilla`, MD9 gleich): an/aus/aus ohne Kinder (nur OFF und NODESCENDANTS von aussen, Eintrag nur bei Aenderung), Wurzel, Haut-`*off` (leert die Ueberschreibungen), gezeichnet = RenderSurfaces ab der Wurzel (offFlags == 0, NODESCENDANTS beendet). Die Leiste "Kappen" ist G2_SetSurfaceOnOff(..., 0) fuer jede Flaeche mit OFF in der Datei. Gezeichnet wird ueber den Ghoul2-Weg des Mission-Tabs (`zeichneFigurIn`) mit dem Dokument als Quelle.
- **LOD automatisch**: G2_ComputeLOD mit ProjectRadius. SP-Figuren: refEntity per memset (cg_players.cpp:6882), Radius 0 -> immer LOD 1; MP: CG_RadiusForCent 64, r_lodscale 5 (codemp), SP 10.
- **Grenzen je Zielspiel** (JKA SP/MP, JK2 SP, MD SP/MP): SHADER_MAX_VERTEXES 1000 (MD9 4000, "raised from 1000"), RB_CheckOverflow bricht bei `>=` ab (Ecken) und bekommt numTriangles als Indexzahl (6000 bzw. 24000), 32 Knochenverweise (5 Bit), 4 Gewichte, MAX_G2_MODELS 512 (JKA/JK2 SP) bzw. 1024, Abschnitte der animation.cfg mit firstFrame + numFrames <= Bilder der GLA (G2_bones.cpp:838-839; dahinter T-Pose in OpenJK-Fassungen, im Original Standpose laut Stufe 0), Flaechen ohne Bild.
- **Gestell nach Entwurf**: Bereichsreiter (Ansehen/Pruefen Start; Modellieren 1d, UV 1b, Weights 1a-2, Painting 1c, Animation 2-7 mit Hinweis), Tastenzeile, links Szenen-Explorer (Modell -> Flaechen nach Hierarchie/Ebenen, Tags mit Anbauten, Knochen mit Markierung "vom Spielcode gesucht", Suche) und darunter die Animationsliste (Suche, Gruppen, Kopieren, Ins Skript; neu: Rechtsklick Primaer/Sekundaer), Mitte eine oder vier Ansichten (Oben, Vorne, Links, Perspektive; Alt+W), rechts das Befehlsfeld (Bedienmodus, Rollouts Modell, Detailstufe und Skin, Anzeige, Anbauten, Pruefen fuers Spiel), unten Transport mit Zaehlung ab 0, Interpolation und der Zeile Primaer/Sekundaer.
- **Ansehen/Pruefen wie ModView**: Rechtsklick-Menue (An, Aus, Aus ohne Kinder, Als Wurzel, Ursprungszustand, _off alle an/aus, alle an), Anbauen an Tag (boltMatrix = G2_ProcessSurfaceBolt2) oder Knochen (Weltmatrix = Endmatrix * BasePose, G2_GetBoltMatrixLow), ersetzen/hinzufuegen, loesen; Sekundaer ab lower_lumbar (anderer Start per Rechtsklick auf einen Knochen) im eigenen Takt; Interpolation an/aus (tr_ghoul2.cpp:1531); Anzeige: Drahtgitter, Kanten, Knochen als Bone-Kasten (Taper 90 %, Flossen), Gewichte (Zahl je Ecke bzw. Gewicht des gewaehlten Knochens), Normalen, Tags, Ursprung, Huelle, Boden (Hoehe, Vorgabe -24 wie DT85), Raster, weisse Texturen, Risse (offene Kanten der gewaehlten Flaechen), zweiseitig, Statistik, Auswahlklammer, FOV 10/80/90; GLA-Override aus den Archiven; Texturen neu laden (Alt+R); Picmip; Foto der aktiven Ansicht >= 1920 als BMP.
- **Bedienung wie 3ds Max** (Vorgabe, Einstellung `modelControls`, Blender-Belegung kommt in 1a-2; Bone-Flossen `modelBoneFins`): Mitte schieben, Alt+Mitte kreisen, Rad und Strg+Alt+Mitte zoomen, links waehlen (Strg dazu, Alt weg), Rahmen; Q/W/E/R mit Gizmo auf den gewaehlten Flaechen (alle LOD-Stufen, Strg+Z), F5-F8, +/-, P/T/F/L/B, Z (Zoom Grenzen der Auswahl, sonst aller gezeichneten in der Pose, Seitenverhaeltnis beachtet), F3, F4, G, J, 7, Strg+A/D/I, `, . / Pos1 Ende`, H, X (alle Aktionen suchen, Enter/Esc), Strg+Z/Y, Alt+R. Die Tasten gelten nach einem Klick in Ansicht, Explorer, Befehlsfeld oder Leiste (sonst die von BehavEd, z. B. Alt+X Beenden).
- Mehrere Modellziele in `gpumap_win32.cpp` (`modellZielWaehlen`, 6), Drahtgitter/beidseitig ueber einen Rasterzusatz nur fuer das Modellfenster, Zusatzfiguren (Anbauten, Boden), `leseModellZiel`; `gpu::baueOrtho`.
- Vorgabe `splitModel` 0.72 statt 0.55 und der Explorer 15 statt 18 Zeichen breit (Platz fuer Ansicht und Befehlsfeld; gespeicherte Teiler bleiben).

## Rot und gruen

- Einpassen (Werkzeugleiste "Ansicht zuruecksetzen", seit rc591): die Kamera stand IN der Mitte der Figur (Orbit::turn nahm den Drehpunkt vor der schon in die Mitte gesetzten Kamera). Rot `mb1_rot`: "Kamera -0.05 2.41 34.98, Abstand zur Mitte 0.000002, Kreis 73.24". Behoben in `modellAnsichtEinpassen` (Kamera um den Abstand vor die Mitte). Gruen `mb1_g2`: Abstand 73.24 = Kreis.
- Haut und Flaechennamen mit "_off": R_LoadMDXM streicht "_off" auch am Flaechennamen (tr_ghoul2.cpp:3722-3735; unter JK2_MODE nur fuer JKA-Modelle), RenderSurfaces vergleicht die Haut mit diesem Namen. Gefunden im Bildvergleich Movie Duels (Droideka, Flaeche `force_shield_off` mit Flagge 0, Hautzeile `force_shield_off,...shield.jpg`): applySkin traf die Flaeche nicht. Rot ctest `modell` (glmtest, neue Probe) FEHL, gruen ok.
- Drahtgitter wirkte nicht auf Flaechen mit Shaderskript (der Rasterzustand war je Stufe gemerkt): `mb1_g1` 0 Punkte anders, `mb1_g2` 52298.
- Im Andockraum (Einstellungen der Testlaeufe) steht frameH auf -FLT_MIN ("bis an den Rand"): der Explorer bekam 0.55 * -FLT_MIN = -1 (ganze Hoehe), die Animationsliste lag unsichtbar darunter. Rot: alter Selbsttest `modell` mit dem neuen Programm 8/14 (`mb1e_modell`, mit rc610 `e10_exe4` 13/0), gruen `mb1e_modell3` 13/0 (Hoehe aus dem freien Platz).
- Kennungskonflikt "10" (LOD-Knopf und Blickwinkel im selben Fenster, gemeldet vom Selbsttest): PushID je LOD-Knopf.
- Pruefungen des Selbsttests, die beim ersten Lauf an der Testumgebung scheiterten (Kyle in MD hat nur eine LOD-Stufe -> Sturmtruppler; Zeilen im Explorer mit SpanAvailWidth breiter als die Spalte; Knoepfe des Befehlsfelds ausserhalb des Sichtbaren; X-Achse zeigt in der Ansicht von vorn zur Kamera -> Y-Griff; zweiseitig mit weissen Texturen, weil die Shader des Sturmtrupplers teils cull none sind), angepasst.

## Bildvergleich (Weg bis rc610 gegen Dokument, bitgleich)

Je Modell 400 x 560, Bild 0 der GLA, eingepasste Kamera, Haut wie geladen; Toleranz 0 (derselbe Zeichner, dieselben Daten - jede Abweichung ist entweder eine Regel des Spiels oder ein Fehler). Abweichungen nur zulaessig, wenn RenderSurfaces eine Flaeche anders zeichnet als der alte Weg (Flagge statt Name, Wurzel 0) - sie werden mit Flaechen genannt.

- JKA (GameData original): 77 Modelle, 77 gleich.
- JK2 (Jedi Outcast): 41 Modelle, 41 gleich.
- Movie Duels: 552 Modelle, 551 gleich, 1 anders wie im Spiel: Droideka, `force_shield_off` (Flagge 0, im Spiel "force_shield") zeichnet RenderSurfaces, der Weg bis rc610 blendete es am Namen aus (80024 Punkte). Laufzeit 346 s; der Test leert den Bilder-Zwischenspeicher je Modell (ohne: 8 GB nach rund 350 Modellen, `mb1_bmd`).
- Halbkanten: 117453 Netze (alle Stufen aller Modelle der drei Spiele) ohne verletzte Invariante; nicht mannigfaltige Kanten gezaehlt (MD 21360, JKA 1687, JK2 542 - Doppelflaechen und Kappen in den Originaldaten).

## Schluss

- Sauberer Neubau (`--clean-first`, alle Ziele): 0 Warnungen, 0 Fehler. ctest -j 16: 221 von 221 (neu `modell_dokument`; `modell` mit der Probe zu "_off", `gpucam` mit der Ortho-Probe).
- Selbsttests auf dem Endstand (`mb1e_exe4`, parallel ausser der Bildzeitmessung): modellblock funktionen 60/0, tasten 43/0, bild md/jka/jk2 je 1/0 (Zahlen oben), alter Model-Tab `modell` 13/0, `modellleistung` (allein) 10/0 (Sturmtruppler ruhig Bild-CPU 0.45 ms, Animation 0.55 ms), `mission1` 98/0, Modus 1 2651/0. Handbuchbilder 6 Sprachen (`BHED_HB_NUR=mb1a1`, 2560 x 1440 @1.25, 2538 breit, angesehen).
- Lints: sources, fassung, i18n, i18n_erzeugt, headers, feldinit, gapp, settings, d3dbesitz, mapstate, imgui_context, rueckkopplung, cbuffer, tabellenid, endtable, layout ok; portability 99 (wie vorher).
- Gegen das Vorbild geprueft: ModView-Quelle (Funktionsliste, Rissfinder, Vorgaben), OpenJK/MD9 (RenderSurfaces, G2_SetSurfaceOnOff, G2_SetRootSurface, G2API_SetSurfaceOnOffFromSkin, R_LoadMDXM "_off", G2_ComputeLOD, ProjectRadius, RB_CheckOverflow, MAX_G2_MODELS, r_lodscale, CG_RadiusForCent), max_hotkeys_de.txt (Pruefzeile je Taste). Nicht geprueft: das Bild im Spiel (kein Spielstart), 3ds Max/Blender selbst.

## Offen

- Mission-Tab und Karten-Figuren nutzen weiter den alten Sichtbarkeitsweg (isVisible: "_off" am Namen blendet aus, auch ohne Flagge). RenderSurfaces zeichnet solche Flaechen (Droideka `force_shield_off` in Movie Duels, Flagge 0). Ob der Spielcode sie beim Spawnen abschaltet (MD9 `g_client.cpp` nennt `force_shield`), ist fuer den Mission-Tab zu pruefen - hier nicht geaendert.
- Nicht im Spiel verglichen (kein Spielstart): LOD-Wahl, Anbauten und Sekundaeranimation sind aus den Quellen (OpenJK, MD9) portiert und gegen die eigenen Formeln/Matrizen geprueft; die Bildgleichheit belegt nur den Rundlauf gegen den bisherigen Zeichenweg.
- ModView-Funktionen nicht uebernommen: Hintergrundfarbe, Alpha/Texturfilter-Schalter, Dreieck-/Ecken-Nummern, Hoehenlineal, nicht schattenwerfende Flaechen, .ideal/.mvs-Dateien, Stapelumwandlung. Die Ueberlagerungen (Knochen, Tags, Normalen ...) sind nicht im Foto (nur das gezeichnete Modell).
- Gizmo im Bereich Ansehen/Pruefen wirkt auf die Bindepose der gewaehlten Flaechen (Speichern als .glm kommt mit dem GLM-Schreiber in Stufe 2). Durchsicht (Max Alt+X) nicht belegt - Alt+X bleibt Beenden.
- Die Namen langer Flaechen sind in der schmalen Explorer-Spalte nur mit waagerechtem Rollen ganz lesbar.

## Vorschlag fuer 1a-2

- Weights-Bereich: Gewichte live in der Pose (GPU-Skinning vorhanden, Gewichte im Dokument je Ecke 4 - fuer mehr als 4 waehrend des Malens eine eigene Liste mit "Limit Total" beim Uebernehmen), JKA-Pruefung immer an (die Funktionen aus `g2.h` gibt es schon).
- Blender-Modus: eine zweite Tastentabelle neben `maxTasten()` (G/R/S modal mit Achse und Zahl, Tab, 1/2/3, Leertaste, Zaehlung ab 1 mit GLA-Frame im Tooltip), Oktaeder-Knochen, eine grosse Ansicht mit Werkzeugleiste links.
- Mission-Tab auf dieselben Ghoul2-Regeln umstellen (siehe Offen).

## Changelog

### English
- Model tab, built anew for Jedi Academy: areas View/Check, Modeling, UV, Weights, Painting and Animation (the later areas come in the next stages), a scene explorer with surfaces, tags and bones, one or four views (top, front, left, perspective; Alt+W), a command panel with rollouts and the timeline counted from 0. The model is loaded into its own document with every level of detail; an unchanged model draws exactly as before.
- View/Check like ModView, drawn the way the game draws it: surfaces on, off, off without descendants or as root (right-click), skins with "*off", LOD 1-10 or automatic like the game, attach sabers and other models to tags or bones, primary and secondary animation (upper body from lower_lumbar), interpolation, wireframe, edged faces, bones as bone objects, vertex weights, normals, tags, origin, bounding box, floor, grid, white textures, crack finder, two-sided, field of view, GLA override, reload textures and a photo at least 1920 pixels wide.
- Check for the game: vertices and triangles per surface (Jedi Academy and Jedi Outcast stop at 1000 vertices, Movie Duels at 4000), 32 bones per surface, 4 weights per vertex, Ghoul2 instances, animation.cfg lines behind the end of the GLA (T-pose) and surfaces without texture - for JKA, JK2 and Movie Duels, single and multiplayer.
- Controls like 3ds Max: middle mouse pans, Alt+middle orbits, wheel zooms, click and frame select, Q/W/E/R with a gizmo (with undo), F3/F4, F5-F8, P/T/F/L/B, Z, G, J, 7, Ctrl+A/D/I, / , . Home End, H and X to search all actions. The Blender key map follows.
- Model tab: "Reset view" puts the camera in front of the model again (it stood inside it); skin lines for surfaces ending in "_off" apply like in the game.

### Deutsch
- Model-Tab, neu für Jedi Academy gebaut: Bereiche Ansehen/Prüfen, Modellieren, UV, Weights, Painting und Animation (die späteren Bereiche kommen in den nächsten Stufen), ein Szenen-Explorer mit Flächen, Tags und Knochen, eine oder vier Ansichten (oben, vorne, links, Perspektive; Alt+W), ein Befehlsfeld mit Rollouts und die Zeitleiste ab 0 gezählt. Das Modell kommt mit allen Detailstufen in ein eigenes Dokument; ein unverändertes Modell sieht genau aus wie vorher.
- Ansehen/Prüfen wie ModView, gezeichnet wie im Spiel: Flächen an, aus, aus ohne Kinder oder als Wurzel (Rechtsklick), Skins mit "*off", LOD 1-10 oder automatisch wie im Spiel, Säbel und andere Modelle an Tags oder Knochen anbauen, Primär- und Sekundäranimation (Oberkörper ab lower_lumbar), Interpolation, Drahtgitter, Flächen mit Kanten, Knochen als Bone-Objekte, Gewichte an den Ecken, Normalen, Tags, Ursprung, Hüllkasten, Boden, Raster, weiße Texturen, Risse finden, zweiseitig, Blickwinkel, GLA-Override, Texturen neu laden und ein Foto mindestens 1920 Punkte breit.
- Prüfen fürs Spiel: Ecken und Dreiecke je Fläche (Jedi Academy und Jedi Outcast brechen bei 1000 Ecken ab, Movie Duels bei 4000), 32 Knochen je Fläche, 4 Gewichte je Ecke, Ghoul2-Instanzen, Zeilen der animation.cfg hinter dem Ende der GLA (T-Pose) und Flächen ohne Textur - für JKA, JK2 und Movie Duels, Einzel- und Mehrspieler.
- Bedienung wie 3ds Max: mittlere Maus schiebt, Alt+Mitte kreist, Rad zoomt, Klick und Rahmen wählen, Q/W/E/R mit Gizmo (mit Rückgängig), F3/F4, F5-F8, P/T/F/L/B, Z, G, J, 7, Strg+A/D/I, / , . Pos1 Ende, H und X zum Suchen aller Aktionen. Die Blender-Belegung folgt.
- Model-Tab: "Ansicht zurücksetzen" stellt die Kamera wieder vor das Modell (sie stand in ihm); Skin-Zeilen für Flächen mit "_off" am Ende wirken wie im Spiel.

### 中文
- 为 Jedi Academy 重新构建的模型选项卡:查看/检查、建模、UV、权重、绘制和动画区域(后面的区域将在后续阶段提供),带面、标记和骨骼的场景资源管理器,一个或四个视图(顶、前、左、透视;Alt+W),带卷展栏的命令面板,以及从 0 开始计数的时间轴。模型连同所有细节级别载入自己的文档;未修改的模型绘制效果与以前完全相同。
- 像 ModView 一样查看/检查,并按游戏的方式绘制:面开启、关闭、不含子级关闭或设为根(右键),带 "*off" 的皮肤,LOD 1-10 或与游戏相同的自动级别,将光剑和其他模型附加到标记或骨骼,主要和次要动画(从 lower_lumbar 起的上半身),插值、线框、带边面、骨骼对象、顶点权重、法线、标记、原点、包围盒、地面、网格、白色纹理、裂缝查找、双面、视野、GLA 覆盖、重新加载纹理,以及至少 1920 像素宽的照片。
- 针对游戏检查:每个面的顶点和三角形(Jedi Academy 和 Jedi Outcast 在 1000 个顶点时中止,Movie Duels 为 4000)、每个面 32 根骨骼、每个顶点 4 个权重、Ghoul2 实例、超出 GLA 末尾的 animation.cfg 行(T 姿势)以及没有纹理的面——适用于 JKA、JK2 和 Movie Duels 的单人和多人模式。
- 像 3ds Max 一样操作:鼠标中键平移,Alt+中键环绕,滚轮缩放,单击和框选,Q/W/E/R 与 Gizmo(可撤销),F3/F4、F5-F8、P/T/F/L/B、Z、G、J、7、Ctrl+A/D/I、/ , . Home End,H 和 X 搜索所有操作。Blender 键位随后提供。
- 模型选项卡:"重置视图" 重新把摄像机放在模型前面(以前在模型内部);以 "_off" 结尾的面的皮肤行与游戏中相同地生效。

### 日本語
- Jedi Academy 向けに作り直したモデルタブ: 表示/チェック、モデリング、UV、ウェイト、ペイント、アニメーションの各領域 (後の領域は次のステージで追加)、サーフェス・タグ・ボーンを表示するシーンエクスプローラー、1 つまたは 4 つのビュー (上、前、左、パース。Alt+W)、ロールアウト付きのコマンドパネル、0 から数えるタイムライン。モデルはすべての詳細レベルと一緒に専用のドキュメントに読み込まれ、変更していないモデルは以前とまったく同じに描画されます。
- ModView のような表示/チェックを、ゲームと同じ描画で: サーフェスのオン・オフ・子孫も含めてオフ・ルート設定 (右クリック)、"*off" 付きスキン、LOD 1-10 またはゲームと同じ自動選択、セイバーなどのモデルをタグやボーンに取り付け、プライマリとセカンダリのアニメーション (lower_lumbar から上の上半身)、補間、ワイヤーフレーム、エッジ面、ボーンオブジェクト、頂点ウェイト、法線、タグ、原点、バウンディングボックス、床、グリッド、白いテクスチャ、クラック検出、両面、視野角、GLA オーバーライド、テクスチャの再読み込み、幅 1920 ピクセル以上の写真。
- ゲーム向けチェック: サーフェスごとの頂点と三角形 (Jedi Academy と Jedi Outcast は頂点 1000、Movie Duels は 4000 で停止)、サーフェスごとにボーン 32、頂点ごとにウェイト 4、Ghoul2 インスタンス、GLA の末尾を超える animation.cfg の行 (T ポーズ)、テクスチャのないサーフェス。JKA、JK2、Movie Duels のシングルとマルチプレイ向け。
- 3ds Max と同じ操作: 中ボタンでパン、Alt+中ボタンで周回、ホイールでズーム、クリックと枠で選択、Q/W/E/R とギズモ (元に戻せる)、F3/F4、F5-F8、P/T/F/L/B、Z、G、J、7、Ctrl+A/D/I、/ , . Home End、H、X ですべての操作を検索。Blender のキー割り当ては後で追加されます。
- モデルタブ: "ビューをリセット" でカメラが再びモデルの前に来ます (以前はモデルの中にありました)。"_off" で終わるサーフェスのスキン行がゲームと同じように効きます。

### Español
- Pestaña Model, construida de nuevo para Jedi Academy: áreas Ver/Comprobar, Modelado, UV, Pesos, Pintura y Animación (las áreas posteriores llegan en las próximas etapas), un explorador de escena con superficies, tags y huesos, una o cuatro vistas (superior, frontal, izquierda, perspectiva; Alt+W), un panel de comandos con persianas y la línea de tiempo contada desde 0. El modelo se carga en su propio documento con todos los niveles de detalle; un modelo sin cambios se dibuja exactamente igual que antes.
- Ver/Comprobar como ModView, dibujado como lo dibuja el juego: superficies activadas, desactivadas, desactivadas sin descendientes o como raíz (clic derecho), skins con "*off", LOD 1-10 o automático como en el juego, acoplar sables y otros modelos a tags o huesos, animación primaria y secundaria (torso desde lower_lumbar), interpolación, alambre, caras con aristas, huesos como objetos hueso, pesos de vértices, normales, tags, origen, caja envolvente, suelo, rejilla, texturas blancas, buscar grietas, doble cara, campo de visión, GLA override, recargar texturas y una foto de al menos 1920 píxeles de ancho.
- Comprobar para el juego: vértices y triángulos por superficie (Jedi Academy y Jedi Outcast se detienen a los 1000 vértices, Movie Duels a los 4000), 32 huesos por superficie, 4 pesos por vértice, instancias Ghoul2, líneas del animation.cfg más allá del final del GLA (pose T) y superficies sin textura, para JKA, JK2 y Movie Duels, un jugador y multijugador.
- Controles como 3ds Max: el botón central desplaza, Alt+central orbita, la rueda hace zoom, clic y marco seleccionan, Q/W/E/R con gizmo (con deshacer), F3/F4, F5-F8, P/T/F/L/B, Z, G, J, 7, Ctrl+A/D/I, / , . Inicio Fin, H y X para buscar todas las acciones. El mapa de teclas de Blender llegará después.
- Pestaña Model: "Restablecer vista" vuelve a poner la cámara delante del modelo (estaba dentro de él); las líneas de skin de superficies que terminan en "_off" funcionan como en el juego.

### Čeština
- Karta Model, nově postavená pro Jedi Academy: oblasti Prohlížet/Kontrola, Modelování, UV, Váhy, Malování a Animace (pozdější oblasti přijdou v dalších fázích), průzkumník scény s povrchy, tagy a kostmi, jeden nebo čtyři pohledy (shora, zepředu, zleva, perspektiva; Alt+W), panel příkazů s roletami a časová osa počítaná od 0. Model se načte do vlastního dokumentu se všemi úrovněmi detailu; nezměněný model se vykreslí přesně jako dřív.
- Prohlížet/Kontrola jako ModView, vykresleno jako ve hře: povrchy zapnout, vypnout, vypnout bez potomků nebo jako kořen (pravé tlačítko), skiny s "*off", LOD 1-10 nebo automaticky jako ve hře, připojení mečů a jiných modelů k tagům nebo kostem, primární a sekundární animace (horní část těla od lower_lumbar), interpolace, drátový model, plochy s hranami, kosti jako objekty kostí, váhy vrcholů, normály, tagy, počátek, obalový kvádr, podlaha, mřížka, bílé textury, hledání trhlin, oboustranné, zorné pole, přepsání GLA, znovu načtení textur a fotka široká alespoň 1920 bodů.
- Kontrola pro hru: vrcholy a trojúhelníky na povrch (Jedi Academy a Jedi Outcast skončí při 1000 vrcholech, Movie Duels při 4000), 32 kostí na povrch, 4 váhy na vrchol, instance Ghoul2, řádky animation.cfg za koncem GLA (pozice T) a povrchy bez textury - pro JKA, JK2 a Movie Duels, jeden i více hráčů.
- Ovládání jako 3ds Max: prostřední tlačítko posouvá, Alt+prostřední krouží, kolečko přibližuje, klik a rámeček vybírají, Q/W/E/R s gizmem (s vrácením), F3/F4, F5-F8, P/T/F/L/B, Z, G, J, 7, Ctrl+A/D/I, / , . Home End, H a X pro hledání všech akcí. Klávesy Blenderu následují.
- Karta Model: "Obnovit pohled" opět postaví kameru před model (stála v něm); řádky skinu pro povrchy končící na "_off" fungují jako ve hře.
