# Notizen rc609 P: Spiegel und Portale im Echtzeit-Bild des Karten-Editors

Wunsch des Testers (04.11.): „a shader what is basically a mirror … reflection on the floor … misc_portal_surface … would be cool if that would be in real time render too“.

## Umsetzung
- Gemeinsamer Kern `bhed/portalsicht` (src/portalsicht.cpp): misc_portal_surface/misc_portal_camera wie `SP_misc_portal_surface`, `locateCamera`/`setCamera` (g_misc.cpp) und `CG_Portal` (cg_ents.cpp); die Wahl der Fläche und die zweite Sicht wie `R_SortDrawSurfs`, `R_MirrorViewBySurface`, `SurfIsOffscreen`, `IsMirror`, `R_GetPortalOrientations`, `R_MirrorPoint`/`R_MirrorVector` (tr_main.cpp), die Schnittebene wie `RB_BeginDrawingView` (tr_backend.cpp). Movie Duels 9 hat dieselben Stellen (nur umbenannt).
- `gui/app_portalsicht.cpp`: Flächen mit `sort portal` einer .bsp (R_PlaneForSurface: SF_FACE aus lightmapVecs[2], SF_TRIANGLES aus PlaneFromPoints) mit ihren Blättern; nur Flächen in Blättern im Bild (R_MarkLeaves) und Schirme im Snapshot (SV_AddEntitiesVisibleFromPoint).
- Vorschau (spiegelFuer) und Spielbild des Editors (app_karte_spielbild.cpp, Zielsatz 7) rufen dasselbe; das Spielbild zeichnet die zweite Sicht mit demselben Zeichner wie die Vorschau (gpumap).
- Echtzeit-Licht (app_karte_cam.cpp): Pinselseiten mit Portal-Shader und die Entities des Editors (live), zweite Sicht in Zielbild 202 mit Schnittebene (SV_ClipDistance), umgekehrter Wicklung im Spiegel, Licht und Schatten für diese Sicht; die Portalfläche zeigt sie an derselben Bildstelle, ihre erste Stufe mischt sich darüber (alphaGen portal je Ecke); ohne Stufe (textures/system/mirror1) zeichnet das Spiel nichts darüber.

## Berichtigt in der Vorschau (gegen OpenJK/MD9)
- Schnittebene genau auf der Spiegelebene (vorher eine Einheit davor: unten fehlte am Spiegelbild ein Streifen).
- pvsOrigin der zweiten Sicht ist `e.oldorigin` (Spiegel: der Ort des misc_portal_surface, Portalkamera: die Kamera); PVS und Gebietsmaske von dort, die ferne Ebene vom abgebildeten Auge (vorher galt beim Spiegel das PVS der echten Kamera).
- Schirm: das erste misc_portal_surface in Entity-Reihenfolge in 64 Einheiten (vorher das nächste); nur Schirme im Snapshot.
- Fläche: die erste in der Sortierung, die nicht ausserhalb des Bilds liegt, nicht abgewandt ist (je Dreieck die erste Ecke) und als Portalkamera innerhalb von portalRange (ohne `alphaGen portal` ist portalRange 0, dann zeichnet das Spiel die Portalkamera nie); vorher die nächste mit Kugeltest und ohne portalRange. Nur Flächen in Blättern im Bild.
- Portalkamera: Richtung über DirToByte/ByteToDir (eine der 162 Richtungen), Roll als ganze Grad (`(int)(clientNum/256.0*360)`), kein Schaukeln (im SP-Renderer auskommentiert; vorher `roll + 4 sin(t)` ohne spawnflags 4).

## Abweichungen, die bleiben (benannt)
- Gleicher Rang in der Sortierung: das Spiel nimmt die erste nach qsort der drawSurfs (Reihenfolge des Wegs durch den BSP-Baum); hier die nächste.
- Patches (SF_GRID) mit Portal-Shader: nicht als Portalfläche (die Engine nimmt die Ebene aus drei Ecken der ersten Gitterzeile).
- Wie die Vorschau zeichnet die zweite Sicht keine Halme, keinen Nebel und kein dynamisches Licht; im Spiegel steht der Himmel statt des Himmelsportals.
- G_PickTarget wählt unter mehreren Zielen zufällig, hier das erste; das Weiterschalten mehrerer Kameras (cycleCamera, wait) ist nicht nachgebaut.
- Die Gebietsmaske der Hauptsicht bleibt die des Auges (das Spiel überschreibt sie mit der des letzten sichtbaren Portalschirms).

## Berichtigt im Echtzeit-Licht
- Ein Portal-Shader ohne jede Stufe (textures/system/mirror1) ist kein Portal: FinishShader setzt dann sort SS_FOG (tr_shader.cpp, MD9 gleich). Testkarte und Handbuch nehmen textures/system/mirror_yavin.

## Tests
- ctest: 203 von 203 grün (Vollbau ohne Warnungen). Neu `portalsicht`: DirToByte/ByteToDir, Schirme wie g_misc.cpp/CG_Portal (Roll 10 -> 9 Grad, G_Find, angle/angles, G_SetMovedir), SurfIsOffscreen (Bild, abgewandt, portalRange, 64 Einheiten), Sortierung, portalPlane, pvsOrigin, R_MirrorPoint.
- Selbsttest `editorportal` (neu), rot vor dem Einbau (alte Verdrahtung aus 54ade1d8): 1 OK, 15 FEHLER - Vorschau duel_executor/duel_emperor mit Schnitt d -1 statt 0, Spielbild ohne Sicht, Echtzeit-Licht ohne Spiegel und Portal. Danach wurde der Test geschärft (Gegenprobe duel_calodan statt duel_emperor, deren Portalfläche schlecht im Bild lag; Kamera der Reichweitenprobe hinter die Säule; Live-Prüfung über das ganze Bild; mirror1 -> mirror_yavin).
- Grün (p_gruen5): 15 OK, 0 FEHLER.
  - Vorschau gleich der Engine-Formel: Abbildung und Schnitt Abweichung 0 (duel_executor, duel_wbw, duel_calodan).
  - Editor-Spielbild gegen Vorschau, gleiche Kamera, Größe 1350 x 931 und Shaderzeit: mittlere Abweichung 0,000 im ganzen Bild und im Fenster auf der Portalfläche (Schwelle 1 bzw. 3 von 255 für Kantenglättung und Rundung - nicht gebraucht).
  - Echtzeit-Licht live: misc_portal_surface 100 über den Boden -> im nächsten Bild kein Spiegel (Bildabstand 3,18), zurück -> bitgleich (0,000); misc_portal_camera verschoben -> Bildschirm anders (2,89); 268 vor dem Bildschirm (portalRange 256) keine Sicht, Bildschirm schwarz (0,0 0,0 0,0).
  - Spielbild der schnellgebackenen Testkarte: dieselbe Spiegelsicht wie das Echtzeit-Licht (Abweichung 0,000), Bildschirm zeigt die Portalkamera.
- Spielvergleich Movie Duels (eigener fs_homepath, Fenster außer Sicht, 1920 x 1440, cg_fov 80; Editor mit Vorlage und Kamera nach viewpos):
  - duel_executor (-1159 1532 68, Blick 90): Spiegelboden mit Säule und Decke wie im Spiel; Mittel ganzes Bild Spiel 18,2 18,9 20,3 / Editor 18,0 18,7 20,1, untere Hälfte 9,8 10,2 10,9 / 9,7 10,0 10,7.
  - duel_wbw (3439 1664 68): der Ring stimmt; im Ring zeigt das Spiel Schwarz, der Editor (wie die Vorschau) schwach die Sicht der misc_portal_camera (Mitte Spiel 30,0 29,4 32,2 / Editor 39,7 39,4 41,6). Ursache offen (das Spiel steht dort in der dritten Person, viewpos ist nur auf ganze Einheiten).
  - duel_calodan: die Kamera stand nicht über dem Wasser; kein Vergleich.
- missionfoto md_agditf_jedi (Karte mit misc_portal_surface), alter Stand (Verdrahtung 54ade1d8) gegen neu bei 3, 8 und 15 s: mittlere Abweichung 0,007 bis 0,045 von 255, nur im Spiegelbild des Bodens (Schnittebene genau auf der Ebene) und im Filmstreifen/FPS-Text.
- karteneditor kurz (BHED_KARTE_NUR=fotos): 78 OK, 0 FEHLER.
- Lints: lint_sources, lint_cbuffer, lint_feldinit, lint_gapp, lint_d3dbesitz, lint_merkmalsbits, lint_tabellenid ok; lint_portability ohne neue Funde; lint_headers braucht cl (nicht in dieser Shell).

## Offen
- Tempo: kein Bildverlust ohne sichtbares Portal ist nicht gemessen (parallele Agenten); der Hauptchat misst allein.

## English
- Map editor: mirrors and portal cameras (a face with a `portal` shader and a misc_portal_surface, with or without a misc_portal_camera) now show in the real-time light and in "Built map as in the game", through the same code as the Mission tab preview; moving the misc_portal_surface or the camera or setting the shader changes the view in the next picture.
- Mission tab preview: mirrors and portal cameras follow the game more closely - the cut lies exactly on the mirror plane, the second view uses the visibility of the misc_portal_surface or camera, the camera direction and roll are rounded like the game, a portal camera beyond its `alphaGen portal` range (or without one) shows no view, and a portal shader without any stage is no portal (as in the game).

## Deutsch
- Karten-Editor: Spiegel und Portalkameras (eine Fläche mit `portal`-Shader und ein misc_portal_surface, mit oder ohne misc_portal_camera) erscheinen jetzt im Echtzeit-Licht und in "Gebaute Karte wie im Spiel", über denselben Code wie die Vorschau im Mission-Tab; wer das misc_portal_surface oder die Kamera verschiebt oder den Shader setzt, sieht die neue Sicht im nächsten Bild.
- Vorschau im Mission-Tab: Spiegel und Portalkameras folgen dem Spiel genauer - der Schnitt liegt genau auf der Spiegelebene, die zweite Sicht nimmt die Sichtbarkeit vom misc_portal_surface bzw. der Kamera aus, Richtung und Roll der Kamera werden gerundet wie im Spiel, eine Portalkamera jenseits ihrer Weite von `alphaGen portal` (oder ohne) zeigt keine Sicht, und ein Portal-Shader ganz ohne Stufe ist kein Portal (wie im Spiel).

## 中文
- 地图编辑器:镜子和传送门摄像机(带 `portal` 着色器的面加上 misc_portal_surface,有或没有 misc_portal_camera)现在会在实时光照和"按游戏显示已编译地图"中显示,使用与任务选项卡预览相同的代码;移动 misc_portal_surface 或摄像机、或设置着色器后,下一帧即显示新的视野。
- 任务选项卡预览:镜子和传送门摄像机更贴近游戏——剪切面正好位于镜面上,第二个视野使用 misc_portal_surface 或摄像机处的可见性,摄像机方向和滚转像游戏一样取整,超出 `alphaGen portal` 距离(或没有该设置)的传送门摄像机不显示视野,完全没有阶段的传送门着色器不是传送门(与游戏相同)。

## 日本語
- マップエディター:ミラーとポータルカメラ(`portal` シェーダーの面と misc_portal_surface、misc_portal_camera の有無を問わず)が、リアルタイムライトと「ビルド済みマップをゲームどおりに表示」に表示されるようになりました。ミッションタブのプレビューと同じコードです。misc_portal_surface やカメラを動かしたりシェーダーを設定したりすると、次のフレームで新しい視界になります。
- ミッションタブのプレビュー:ミラーとポータルカメラがよりゲームに近くなりました。切断面はミラー面ちょうどにあり、2 つ目の視界は misc_portal_surface またはカメラの位置からの可視性を使い、カメラの向きとロールはゲームと同じく丸められ、`alphaGen portal` の距離を超えた(または設定のない)ポータルカメラは視界を表示せず、ステージがまったくないポータルシェーダーはポータルになりません(ゲームと同じ)。

## Español
- Editor de mapas: los espejos y las cámaras de portal (una cara con un shader `portal` y un misc_portal_surface, con o sin misc_portal_camera) aparecen ahora en la luz en tiempo real y en "Mapa compilado como en el juego", con el mismo código que la vista previa de la pestaña Misión; al mover el misc_portal_surface o la cámara, o al asignar el shader, la vista cambia en la imagen siguiente.
- Vista previa de la pestaña Misión: los espejos y las cámaras de portal siguen al juego con más precisión: el corte queda justo sobre el plano del espejo, la segunda vista usa la visibilidad desde el misc_portal_surface o la cámara, la dirección y el giro de la cámara se redondean como en el juego, una cámara de portal más allá del alcance de `alphaGen portal` (o sin él) no muestra vista, y un shader de portal sin ninguna etapa no es un portal (como en el juego).

## Čeština
- Editor map: zrcadla a portálové kamery (plocha se shaderem `portal` a misc_portal_surface, s misc_portal_camera i bez ní) se nyní zobrazují ve světle v reálném čase a v „Sestavená mapa jako ve hře“, stejným kódem jako náhled na kartě Mise; posunutí misc_portal_surface nebo kamery či nastavení shaderu změní pohled v příštím snímku.
- Náhled na kartě Mise: zrcadla a portálové kamery přesněji odpovídají hře – řez leží přesně v rovině zrcadla, druhý pohled bere viditelnost z misc_portal_surface nebo kamery, směr a náklon kamery se zaokrouhlují jako ve hře, portálová kamera za dosahem `alphaGen portal` (nebo bez něj) neukazuje pohled a shader portálu bez jediné fáze portálem není (jako ve hře).
