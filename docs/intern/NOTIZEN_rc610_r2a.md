# Notizen rc610 / R2a: Karten-Editor-Reste vor rc610 (Gras im Echtzeit-Licht, Spiegel-Shader, Absturz, Nachfragen)

Auftrag `auftrag_rc610_r2a.md`. Direkt in `Y:\karte` auf `karte1` ab 92bf3298 (MODOBJ), Build `build-test`, Läufe `laeufe/r2a_*` (rot mit `laeufe/r2a_exe_rot` = 92bf3298 plus Zähler und neue Selbsttests; grün `r2a_g1` bis `r2a_g3`; Bildzeiten `r2a_zvor_*` / `r2a_znach_*`; Schluss `r2ae_*`), Laufskript `lauf.py` wie MODOBJ (dazu `VORLAGE=<ordner>|<name>` für einen vorbereiteten Ordner im Laufordner). kFassung bleibt rc609. Kein Spiel gestartet.

## 1. Gras (surfaceSprites) im Echtzeit-Licht

- Vorbild: `tr_surfacesprites.cpp` (OpenJK rd-vanilla, MD dieselbe Datei) über `bhed/grasflaechen.h` wie das Spielbild (rc608 Teil 13c): dieselben Funktionen (`gras::kandidaten`, `zeichneKandidaten`, `effektDreieck`, `unterteilePatch`, `gitterLod`). Neu `gui/app_karte_echtgras.inc`.
- Flächen: Pinselseiten (Dreiecke wie -meta: `echtFlaechenStreifen` = StripFaceSurface), Patches (Kontrollpunkte mit den Normalen von q3map2 über `echtPatchSteuernetz`, R_SubdividePatchToGrid, je Bild RB_SurfaceGrid-LOD; lodOrigin/lodRadius aus den Grenzen der Kontrollpunkte wie lightmapVecs[0..1]), misc_model (AddTriangleModels über `echtModellFlaechen`). Das Spiel zeichnet Halme auf jeder Fläche, die tess füllt (RB_StageIteratorGeneric -> RB_DrawSurfaceSprites), auch auf Modellen; misc_model_static (zur Laufzeit geladen) zeigt die Echtzeit-Ansicht nicht mit Halmen (offen).
- Stufen: Stufe 1 .. n mit surfaceSprites (tr_shade.cpp:2149 MD), alle Stufen aus dem Skript der Spieldaten des Editors (`echtShaderInfo`: parseShaderScript wie die Vorschau, je Datei einmal, neu nach Flush & Reload Shaders). Fehlt das Bild der Stufe, zeichnet die Echtzeit-Ansicht keine Halme (im Spiel wird der ganze Shader zum Ersatz-Shader, ParseStage).
- Leistung: die festen Halme je Fläche einmal (Schlüssel: Shader und Ecken; `echtLicht.grasNeu` zählt Neubauten), Kacheln zu 256 Halmen mit Kasten samt Überstand gegen Reichweite (ssFademax) und Sichtkegel, das Netz eines Bilds bleibt auf der Grafikkarte, solange Kamera, Karte und (nur bei ssWindIdle/Effekt-Halmen) die Zeit gleich sind; je Stufe ein Zeichenaufruf. Die Kamera zeichnet bei Halmen, die an der Zeit hängen, jedes Bild (R1-Ruhe sonst unverändert).
- Licht: im Spiel die BLAUE Eckenfarbe der Fläche am Fuß (:649), grau. Die Echtzeit-Ansicht hat keine Eckenfarben; die Halme bekommen ihr Licht der Fläche an der Stelle des Halms mit der Normale der Fläche, davon den blauen Anteil (`Zustand3::grauLicht`, ps4 `lm = lm.bbb`), Mindestlicht wie die Fläche (C_VERTEXLIT -> minimumEcken). Begründung: das Echtzeit-Licht ersetzt Lightmap und Eckenlicht von q3map2 je Bildpunkt; der Halm nimmt im Spiel das Licht des Bodens unter sich.
- Wind: Windzonen wirken nur mit Teilchenwolken (R_IsRaining), deren Innen/Außen die .bsp braucht (`wetterFuerGrasIn`) - in der Echtzeit-Ansicht windstill; ssWindIdle (ssWind setzt es mit, tr_shader.cpp) schwingt nach der Uhr des Spielbilds (`spielbildZeit`), dann zeichnet die Kamera jedes Bild neu.
- Testshader: `textures/yavin/grasspatchy_sprite` hat in den Spieldaten kein Bild (gfx/sprites/ss_grass_grasspatchy2 fehlt in assets*.pk3) - das Spielbild zeichnet dort keine Halme. Genommen `textures/yavin/ground_grasssprite` (yavin.shader: vertical 32 24 42 500, ssFademax 1500, ssFadescale 1, ssVariance 1 2, ssWind 0.5).

## 2. Spiegel-Shader des Testers

- Vorbild: RB_StageIteratorGeneric zeichnet eine Portalfläche mit allen Stufen über der Sicht, die R_MirrorViewBySurface vorher ins Bild gelegt hat. Die Echtzeit-Ansicht zeichnete von jedem Shader nur die erste Stufe (Tooltip) - bei Portalflächen reicht das nicht: beim Shader des Testers ist die zweite Stufe der Boden über dem Spiegel. Jetzt `SpielDarstellung::weitere` (alle Stufen außer surfaceSprites, `spielStufe`: blendFunc, alphaFunc, rgbGen/alphaGen const bzw. identity, depthWrite, alphaGen portal) und in `shaderZiehen` jede auf der Tiefe der ersten (GL_LEQUAL), ohne Licht wie die erste gemischte Stufe. Tooltip neu (`KeLichtGrenzenTip2`): Spiegel und Portale mit allen Stufen, Gras als Halme.
- Zweiter Spiegel: R_GetPortalOrientations nimmt den ersten Portal-Eintrag (misc_portal_surface) höchstens 64 Einheiten von der Ebene - für zwei Spiegel auf derselben Höhe also denselben (pvsOrigin); gewählt wird die Fläche (portalSichtWaehlen). „only one mirror view at a time“ (R_SortDrawSurfs) war schon so.
- Das Spielbild (gpumap, unverändert) zeichnet beide Stufen schon wie die Vorschau. Für den Test liegen Shader und Bilder als .pk3 im Laufordner: Extra Resource Path für den Editor, Spielordner der Sitzung für das Spielbild (`setzeSitzungsSpielordner`).

## 3. Kurven-Werkzeug in neuer Karte

- `kurveEinfuegen` nahm `k.welt()` ohne Prüfung -> Absturz ohne worldspawn. Jetzt `weltHolenOderAnlegen` (Map_FindOrInsertWorldspawn, NRC map.cpp:539-554).
- Alle anderen `welt()`-Stellen in `src/karte` und `gui` durchgesehen: prüfen auf nullptr oder vergleichen nur Zeiger (auswahlbefehle, bspmerge, csg, echtlicht, entitaetbefehle, region, ebenen, sichtbareseiten, app_karte_bau/echtlicht/xy, ki, tour_messwerte), oder liegen hinter einer Prüfung (SunPlug: der Dialog öffnet nur mit worldspawn); `app_karte_bsp.cpp` gibt `welt().get()` nur als Ausschluss in eine Liste. Keine weitere Stelle ohne Prüfung gefunden, daher keine weiteren Rotläufe.

## 4. Umriss unter gleich hoher Fläche

- Gemessen statt geraten: im Umrisstest die Maske gegen die Bildpunkte, die das Objekt im fertigen Bild ändert (Bild mit gegen Bild ohne das Objekt, Testhaken `testOhneBrush`). md_am_sith plat1 (func_static *109): der Schild `textures/plasma_mustafar/blueshield` (gemischt, ohne depthWrite) liegt auf Lavahöhe; die Lava (deformVertexes wave) deckt ihn im Bild, die Maske nahm ihn trotzdem (21,6 % der Maske ohne Wirkung).
- NRC: `$CAM_HIGHLIGHT` (renderstate.cpp:1820) prüft nur GL_LEQUAL gegen die Tiefe - gleich hohe verdeckte Flächen würden dort ebenso leuchten; nicht übernommen.
- Behebung: Stencil-Besitz im Hauptbild - wer Tiefe schreibt, schreibt den Stencil 0, die Aufrufe der gewählten Mover 1 (auch gemischte, sobald sie den Tiefentest bestehen); die Maske der Mover prüft Stencil == 1. Gemischte Flächen anderer (Glas) lassen den Wert stehen. Figuren wie bisher gegen die Tiefe.

## 5. Nachfragen

- Kreuz/Alt+F4 (WM_CLOSE -> `confirmQuit`) fragte nicht nach der Karte (nur File > Exit über `rueckfrageDann`). Jetzt `karteneditor::beendenFrage` in `confirmQuit` (nach den Entities der Mission): bringt den Map Editor nach vorn (`tabfenster::zeigen(3)`, auch im eigenen Fenster) und fragt „Exit Radiant“ wie File > Exit (Karte, dann Lumps); File > Exit merkt sich die Antwort (`beendenBeantwortet`), damit nicht zweimal gefragt wird. Das eigene Fenster des Map Editors zu schließen bringt den Tab zurück, ohne Frage, die Karte bleibt (geprüft).
- Teil-Compile: vor dem Bau fragte schon `lumpRueckfrage`; Änderungen im Fenster „Shader und Flächen“ während des Baus ersetzte `bspEinfuegenAbschluss` still. Jetzt dort die Frage (Art 2, `KeLumpsTeilBau`): Behalten = erst speichern, dann auf diesem Stand einfügen; Ersetzen = verwerfen; Abbrechen = nichts einfügen (`bspEinfuegenFertig` „abgebrochen“).

## 6. B3-3 Bilder unnötig neu geladen

- OpenJK tr_model.cpp:212-230 (RE_RegisterModel_Actual): ein schon geladenes Modell registriert seine Shader neu (R_FindShader -> R_FindImageFile setzt iLastLevelUsedOn), seine Bilder bleiben bei RE_RegisterImages_LevelLoadEnd. Hier merkt sich jedes Modell (Figur, Griff, Gegenstand; `ActorAssets::bilder`, gesammelt mit `BildSammler` beim Laden) seine Schlüssel in texCache und meldet sie bei jedem Treffer im Zwischenspeicher als benutzt (`modellBilderGenutzt`).

## 7. Kleinreste aus MODOBJ

- Asset-Browser: `assetsSammeln` nimmt `modellBrowserEndungen()` in einem Gang durch die Archive wie ModelBrowser_constructTree (Endung ohne Groß/klein, jeder Pfad einmal); `kAssetSchritte` 7 -> 4.
- Import-Paket: zwei verschiedene Bilder gleichen Namens aus verschiedenen Ordnern bekommen `<name>` und `<name>_2` (Shader und Bilddatei), dasselbe Bild (gleiche Bytes) bleibt ein Shader (`md3Paket`).

## Rot und grün

Rot: `laeufe/r2a_exe_rot` (92bf3298 plus Zähler, Testhaken und Selbsttests). Für die Nachfrage beim Schließen eine eigene rote exe (`r2a_exe_rot_schliessen`: Endstand ohne den Aufruf von `beendenFrage` in `confirmQuit`), weil der erste Rotlauf die Karte nicht als geändert markiert hatte; für den Spiegel lag der Shader im ersten Rotlauf unter `scripts/` (JKA liest `shaders/`) - Rotlauf `r2a_spiegel_rot2` mit dem vorbereiteten Ordner (`VORLAGE`).

| Punkt | Prüfung | rot | grün |
|---|---|---|---|
| 1 Gras | `karteneditor` r2a, Testraum mit textures/yavin/ground_grasssprite (4 Bodenpinsel, Patch) | `r2a_rot`: 0 Halme | `r2ae2_r2a` 21/0: 951 Halme auf 5 Flächen (1 Patch), 25 Flächen gebaut; Kamera bewegt, Halme aus/an: 0 neu; Textur eines Pinsels auf Stein: im nächsten Bild 951 -> 360, 0 neu; Rückgängig: 951, genau 6 neu (die Seiten des Pinsels) |
| 1 Gras | dieselbe Kamera, Zeit 0, gegen „Gebaute Karte wie im Spiel“ | - | Halme 951 / 951 (1,000); bedeckte Bildpunkte 116303 / 147800 (0,787), an denselben Stellen 0,644. Toleranz: Halmzahl 10 % (q3map2 kann Seiten an Ebenen des Baums zerschneiden), Bildpunkte 25 % und Hälfte gemeinsam - gezählt wird eine Änderung > 24, das hängt am Licht (Echtzeit-Licht je Bildpunkt gegen die Eckenfarben von q3map2 -light -fast) |
| 2 Spiegel | `editorportal` (BHED_P_NUR=tester): Shader des Testers, Anteil Grün (zweite Stufe) über dem Spiegel | `r2a_spiegel_rot2`: Echtzeit grün 0,000 (nur die erste Stufe) | `r2ae2_tester` 4/0: Echtzeit 0,546, Spielbild 0,546; zweiter Spiegel allein: Fläche 6 statt 0 (benutzt); beide im Bild: genau eine Sicht. Spielbild nicht rot geprüft (gpumap unverändert, der Rotlauf hatte die .pk3 nicht als Spielordner) |
| 3 Kurve | ctest `karte_kurven`, Brücke in eine Karte ohne worldspawn | SEGFAULT | OK (worldspawn als erste Entität, 2 Patches) |
| 4 Umriss | `umriss`: Maske gegen die Bildpunkte, die plat1 ändert | `r2a_umriss_rot`: 21743 von 100458 Maskenpunkten (21,6 %) ohne Wirkung | `r2ae_umriss` 49/0: 87 von 64483 (0,1 %); Bild angesehen |
| 5a Schließen | `karteneditor` r2a: Karte geändert, `confirmQuit` | `r2a_schliessen_rot`: confirmQuit 1, keine Frage (auch mit eigenem Fenster) | Frage im Map Editor (Tab 3), Abbrechen = kein Beenden; eigenes Fenster: Frage dort; eigenes Fenster schließen: Karte bleibt |
| 5b Teil-Compile | `bspmerge`: Shader im Fenster tauschen, während gebaut wird | `r2a_bm_rot`: keine Frage, die Datei hat den alten Shader (bswitch statt coals) | `r2ae_bspmerge` 25/0: Frage (Art 2), Behalten: coals und der vierte Trigger in der Datei |
| 6 B3-3 | `umriss`: neues Level mit denselben Figuren, zweites Modell mit derselben Haut | `r2a_zvor_umriss` (allein): 18 Lesezugriffe, texturesFor 94,61 ms | `r2a_znach_umriss` (allein): 0, 5,05 ms; Bildpunkte gleich |
| 7a Asset-Liste | ctest `karte_assets` (.fbx, .OBJ, .mtl daneben) | 1 Fehler (nur .md3) | OK (3 Modelle, nicht die .mtl) |
| 7b Shadernamen | ctest `karte_md3import` (a/holz.jpg, b/holz.jpg, a/holz.jpg) | 1 Fehler (ein Shader) | OK: holz, holz_2; dasselbe Bild = derselbe Shader |

Unterwegs gefunden (eigene Testfehler, behoben): `textures/yavin/grasspatchy_sprite` hat kein Bild in den Spieldaten (dann zeichnet weder Spielbild noch Echtzeit-Licht Halme - im Spiel wäre der Shader der Ersatz-Shader); die Halme schwingen nach der Zeit (ssWind setzt ssWindIdle mit), darum nimmt die Echtzeit-Ansicht jetzt die Uhr des Spielbilds (`spielbildZeit`) - mit fester Zeit gleiche Bilder in jedem Lauf.

## Bildzeit und Ladezeit (allein)

Bildzeit: 240 Bilder, Kamera dreht sich, Mittel / p99 der Bildabstände (die Ansicht läuft mit der Bildwiederholung, ~4,2 ms); dazu die Prozessorzeit der Halme je Bild und die Eckdaten auf der Grafikkarte. Große Grasfläche: 4096 x 4096 aus 256 Pinseln, 16 Lichter.

| Messung | vorher (rot) | nachher |
|---|---|---|
| Gras, Ecke, ohne Gras | 4,25 / 8,62 ms | 4,17 / 7,50 ms |
| Gras, Ecke, mit Gras | 4,37 / 10,44 ms (keine Halme) | 4,22 / 6,30 ms, 342 Halme, 0,36 ms Prozessor, 73872 Bytes |
| Gras, Mitte, ohne Gras | - | 4,20 / 6,87 ms |
| Gras, Mitte, mit Gras | - | 4,20 / 7,10 ms, 1505 Halme, 0,71 ms Prozessor, 325080 Bytes |
| Spiegel mit / ohne (Echtzeit) | 4,27 / 8,62 und 4,34 / 8,56 ms (der zweite misc_portal_surface blieb in 64 Einheiten: beide mit Sicht) | mit 4,90 / 42,82 ms (eine Spitze), ohne 4,32 / 8,58 ms |
| B3-3 texturesFor nach Levelwechsel | 94,61 ms, 18 Lesezugriffe | 5,05 ms, 0 |

## Schluss

- Sauberer Neubau (`--clean-first`) 0 Warnungen, 0 Fehler (`r2a_bau_ende`); danach nur kleine Änderungen (Uhr der Halme, Testcode, Kommentare), inkrementell gebaut, 0 Warnungen (`/WX`).
- ctest -j 16: 220 von 220 (`r2a_ctest_ende2`; vor den Handbuchbildern fehlte `handbuch_sprachen` die zwei neuen Bilder).
- Selbsttests parallel (`r2ae_*`, `r2ae2_*`): `karteneditor` fotos 78/0, stufe9 84/0, modobj 25/0, tutgeo 23/0, licht1 16/0, ruhen 32/0, r2a 21/0; `editorportal` 19/0 (mit dem Spiegel des Testers); `teil13c` 6/0; `lumpedit` 18/0; `bspmerge` 25/0; `umriss` 49/0; Modus 1 2651/0.
- Handbuch: Seite „Karten-Editor: Tempo und Echtzeit-Licht“ (karte_tempo) mit Absätzen zu Spiegeln mit allen Stufen und Gras, Bilder `h_r2a_spiegel` und `h_r2a_gras` (6 Sprachen, BHED_HB_FENSTER=3840x2160@1.5, 1989 bis 2055 breit, angesehen), Notizen auf `karteneditor` (Schließen) und `karte_bspmerge` (Lumps während des Teil-Compiles); `gen_handbuch.py`: es/cs 0 Texte, 0 Bilder englisch. Texte `tools/i18n_rc610_r2a.py`, `tools/handbuch/inhalt_r2a.py`.
- Lints: sources, fassung, i18n, i18n_erzeugt, headers, feldinit, gapp, settings, d3dbesitz, mapstate, imgui_context, rueckkopplung, cbuffer, tabellenid ok; portability 99 (wie MODOBJ).
- Gegen das Vorbild geprüft: Halme gegen das Spielbild (dieselbe Rechnung wie die Vorschau, dort gegen das Spiel gemessen), die Regeln aus der OpenJK-Quelle (tr_surfacesprites.cpp, tr_shade.cpp, R_GetPortalOrientations, R_SortDrawSurfs, RE_RegisterModel_Actual, ParseStage), NRC (Map_FindOrInsertWorldspawn, ModelBrowser_constructTree, $CAM_HIGHLIGHT). Nicht im Spiel geprüft (kein Spielstart): der Shader des Testers im Spiel.

## Offen

- misc_model_static (zur Laufzeit geladene .md3) tragen in der Echtzeit-Ansicht keine Halme (das Spiel zeichnet sie auch auf Modellen); misc_model (eingebacken) ja.
- Die Halme stehen dort, wo q3map2 Pinselseiten an Ebenen des BSP-Baums zerschneidet, im Spiel an anderen Stellen (Dreiecke der Bruchstücke); die Echtzeit-Ansicht nimmt die ganze Seite wie -meta ohne Baum.
- Wind der Wetterzonen (fx_wind) in der Echtzeit-Ansicht: braucht die gebaute Karte, dort windstill.
- Das Spielbild baut seine Halme weiter je Bild (rc608); die Netz-Wiederverwendung der Echtzeit-Ansicht ist dort nicht übernommen (mit ssWindIdle hängen die Halme ohnehin an der Zeit).
- Bildzeit: die Ansicht läuft mit der Bildwiederholung (~4,2 ms), der Unterschied mit/ohne Gras liegt darunter; gemessen ist die Prozessorzeit der Halme (0,36 / 0,71 ms).
- Tipp: das Handbuchbild des Spiegels zeigt einen dunklen Raum im Spiegel (Steinwände); ein hellerer Gegenstand im Spiegel wäre anschaulicher.

## Changelog

### English
- Map editor: grass, bushes and vines (shader stages with surfaceSprites, e.g. textures/yavin/ground_grasssprite) now grow in the real-time light too - on brush faces, patches and misc_model, with the same density, size, fade and spread as "Built map as in the game", lit by the real-time light of the face below them. Each face keeps its sprites until it changes; one draw per stage, only what is in range and in view.
- Map editor: mirrors and portals in the real-time light show all stages of their shader above the reflection, like the game (for example a floor texture with alpha over a tinted mirror).
- Map editor: the curve tool no longer crashes in a new map without a worldspawn.
- Map editor: closing the program with the window cross or Alt+F4 asks about an unsaved map like File > Exit, also when the map editor is in its own window; changes in "Shaders and surfaces" made while a part compiles are no longer replaced without asking (keep / replace).
- Map editor: the asset browser lists the same model types as the model browser (.obj, .fbx, .glb and the rest); two different textures with the same name from different folders get their own shader when a model is converted to .md3.
- Mission preview: the selection outline only surrounds what of the selected object is really visible (a platform's shield under the lava no longer enlarges it); figures that stay from one level to the next keep their textures in memory, another model with the same skin no longer reads them again.

### Deutsch
- Karten-Editor: Gras, Büsche und Ranken (Shaderstufen mit surfaceSprites, z. B. textures/yavin/ground_grasssprite) wachsen jetzt auch im Echtzeit-Licht - auf Brushseiten, Patches und misc_model, mit derselben Dichte, Größe, Ausblendung und Verteilung wie „Gebaute Karte wie im Spiel“, beleuchtet vom Echtzeit-Licht der Fläche darunter. Jede Fläche behält ihre Halme, bis sie sich ändert; ein Zeichenaufruf je Stufe, nur was in Reichweite und im Bild ist.
- Karten-Editor: Spiegel und Portale zeigen im Echtzeit-Licht alle Stufen ihres Shaders über dem Spiegelbild, wie das Spiel (zum Beispiel eine Bodentextur mit Alpha über einem getönten Spiegel).
- Karten-Editor: Das Kurven-Werkzeug stürzt in einer neuen Karte ohne worldspawn nicht mehr ab.
- Karten-Editor: Das Programm über das Kreuz oder Alt+F4 zu schließen fragt wie File > Exit nach einer ungespeicherten Karte, auch wenn der Karten-Editor in einem eigenen Fenster steht; Änderungen in „Shader und Flächen“ während eines Teil-Compiles werden nicht mehr ohne Frage ersetzt (behalten / ersetzen).
- Karten-Editor: Der Asset-Browser listet dieselben Modellarten wie der Modell-Browser (.obj, .fbx, .glb und die übrigen); zwei verschiedene Texturen gleichen Namens aus verschiedenen Ordnern bekommen beim Umwandeln in .md3 je einen eigenen Shader.
- Missionsvorschau: Der Umriss der Auswahl umrandet nur, was vom gewählten Objekt wirklich zu sehen ist (der Schild einer Plattform unter der Lava vergrößert ihn nicht mehr); Figuren, die von einem Level ins nächste bleiben, behalten ihre Bilder im Speicher, ein weiteres Modell mit derselben Haut liest sie nicht noch einmal.

### 中文
- 地图编辑器:草、灌木和藤蔓(带 surfaceSprites 的着色器阶段,例如 textures/yavin/ground_grasssprite)现在也会在实时光照中生长——在画刷面、补丁和 misc_model 上,密度、大小、淡出和分布与“已构建的地图如游戏中显示”相同,并由其下方表面的实时光照照亮。每个面保留其精灵直到它改变;每个阶段一次绘制调用,只处理范围内和画面内的部分。
- 地图编辑器:镜子和传送门在实时光照中显示其着色器在倒影之上的所有阶段,与游戏相同(例如带 alpha 的地面纹理覆盖在着色的镜子上)。
- 地图编辑器:在没有 worldspawn 的新地图中,曲线工具不再崩溃。
- 地图编辑器:通过窗口关闭按钮或 Alt+F4 关闭程序时,会像 File > Exit 一样询问未保存的地图,即使地图编辑器在单独的窗口中;部分编译期间在“着色器和表面”中所做的更改不再在未询问的情况下被替换(保留/替换)。
- 地图编辑器:资源浏览器列出与模型浏览器相同的模型类型(.obj、.fbx、.glb 等);将模型转换为 .md3 时,来自不同文件夹的两张同名不同纹理各自获得自己的着色器。
- 任务预览:选择轮廓只围绕所选对象真正可见的部分(平台在熔岩下的护盾不再使其变大);从一个关卡保留到下一个关卡的角色会把纹理保留在内存中,使用相同皮肤的另一个模型不再重新读取它们。

### 日本語
- マップエディター: 草、茂み、つる(surfaceSprites を持つシェーダーステージ。例: textures/yavin/ground_grasssprite)がリアルタイムライトでも生えるようになりました。ブラシ面、パッチ、misc_model 上に、「ビルドしたマップをゲームと同じ表示」と同じ密度、大きさ、フェード、配置で、下の面のリアルタイムライトに照らされて表示されます。各面は変更されるまでスプライトを保持し、ステージごとに 1 回の描画で、範囲内かつ画面内のものだけを処理します。
- マップエディター: リアルタイムライトのミラーとポータルは、ゲームと同じく映り込みの上にシェーダーの全ステージを表示します(例: 色付きミラーの上のアルファ付き床テクスチャ)。
- マップエディター: worldspawn のない新しいマップでカーブツールがクラッシュしなくなりました。
- マップエディター: ウィンドウの × や Alt+F4 でプログラムを閉じると、File > Exit と同じく未保存のマップについて確認します(マップエディターが別ウィンドウでも)。パーツのコンパイル中に「シェーダーとサーフェス」で行った変更が確認なしに置き換えられることはなくなりました(残す / 置き換える)。
- マップエディター: アセットブラウザーはモデルブラウザーと同じモデル形式(.obj、.fbx、.glb など)を一覧にします。別フォルダーにある同名の異なるテクスチャは、モデルを .md3 に変換するときにそれぞれ独自のシェーダーを得ます。
- ミッションプレビュー: 選択のアウトラインは選択したオブジェクトの実際に見える部分だけを囲みます(溶岩の下のプラットフォームのシールドで大きくならなくなりました)。レベルをまたいで残るキャラクターはテクスチャをメモリに保持し、同じスキンの別モデルが再読み込みしなくなりました。

### Español
- Editor de mapas: la hierba, los arbustos y las enredaderas (etapas de shader con surfaceSprites, p. ej. textures/yavin/ground_grasssprite) crecen ahora también con la luz en tiempo real, en caras de brush, parches y misc_model, con la misma densidad, tamaño, desvanecimiento y reparto que «Mapa compilado como en el juego», iluminados por la luz en tiempo real de la cara de debajo. Cada cara conserva sus sprites hasta que cambia; una llamada de dibujo por etapa, solo lo que está al alcance y en la imagen.
- Editor de mapas: los espejos y portales muestran con la luz en tiempo real todas las etapas de su shader sobre el reflejo, como el juego (por ejemplo una textura de suelo con alfa sobre un espejo teñido).
- Editor de mapas: la herramienta de curvas ya no se cierra con error en un mapa nuevo sin worldspawn.
- Editor de mapas: cerrar el programa con la cruz de la ventana o Alt+F4 pregunta por un mapa sin guardar como File > Exit, también si el editor de mapas está en su propia ventana; los cambios en «Shaders y superficies» hechos mientras se compila una parte ya no se sustituyen sin preguntar (conservar / sustituir).
- Editor de mapas: el navegador de recursos muestra los mismos tipos de modelo que el navegador de modelos (.obj, .fbx, .glb y los demás); dos texturas distintas con el mismo nombre de carpetas diferentes reciben cada una su propio shader al convertir un modelo a .md3.
- Vista previa de misión: el contorno de la selección rodea solo lo que se ve realmente del objeto seleccionado (el escudo de una plataforma bajo la lava ya no lo agranda); las figuras que siguen de un nivel al siguiente conservan sus imágenes en memoria y otro modelo con la misma skin ya no las vuelve a leer.

### Čeština
- Editor map: tráva, keře a popínavé rostliny (fáze shaderu se surfaceSprites, např. textures/yavin/ground_grasssprite) nyní rostou i ve světle v reálném čase - na stěnách brushů, patchích a misc_model, se stejnou hustotou, velikostí, mizením a rozložením jako „Sestavená mapa jako ve hře“, osvětlené světlem v reálném čase plochy pod nimi. Každá plocha si drží své sprity, dokud se nezmění; jedno volání kreslení na fázi, jen to, co je v dosahu a v obraze.
- Editor map: zrcadla a portály ve světle v reálném čase ukazují všechny fáze svého shaderu nad odrazem, jako hra (například texturu podlahy s alfou nad tónovaným zrcadlem).
- Editor map: nástroj křivek už v nové mapě bez worldspawn nespadne.
- Editor map: zavření programu křížkem okna nebo Alt+F4 se zeptá na neuloženou mapu jako File > Exit, i když je editor map ve vlastním okně; změny v „Shadery a plochy“ provedené během sestavování části už se bez dotazu nenahradí (ponechat / nahradit).
- Editor map: prohlížeč assetů uvádí stejné typy modelů jako prohlížeč modelů (.obj, .fbx, .glb a ostatní); dvě různé textury stejného jména z různých složek dostanou při převodu modelu na .md3 každá vlastní shader.
- Náhled mise: obrys výběru obepíná jen to, co je z vybraného objektu opravdu vidět (štít plošiny pod lávou ho už nezvětšuje); postavy, které zůstávají z jednoho levelu do dalšího, si drží své obrázky v paměti a další model se stejným skinem je už znovu nečte.
