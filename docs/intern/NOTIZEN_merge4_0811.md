# Notizen MERGE4: fix7 und mr4 nach karte1, MR9-Reste (08.11.2026, `Y:\karte`, Zweig `karte1`, Basis 7918c7d28)

Build `Y:/karte/build-test` (`--parallel 8 -- /nodeReuse:false`), Laeufe `m4_*` unter `Y:\karte\laeufe`, Skripte und Protokolle
`Y:\werkzeuge\mg4\` (Patch-Skripte unter `p\`, an karte1 angepasst aus `Y:\werkzeuge\m4\m9\`).

## Teil 1: Merges

- fix7 (625b15e4e) -> 76c256244: ein Konflikt in `tests/defgpu.cpp` (jeStufe): DEF1s `verformung(f.info.deform2, zeit)` mit
  fix7s `kStufeFloats`/`kStufeDefAt` statt der Zahlen 72/32. Lagenpruefung: ctest `figurglanz` (cbLagenPruefen gegen
  kStufenLagen per D3DReflect) gruen, d. h. "passt". Bau 0 Warnungen, ctest 243/243.
- mr4 (1c7226e4b, MR7-MR9) -> 248a2e770: Konflikte nur in den erzeugten `handbuch/*/karte.html` und `suche.js`; mit
  `gen_handbuch.py` neu erzeugt, `gen_i18n.py` und `gen_hilfe_orte.py` ergeben keine Aenderung. Bau 0 Warnungen, ctest 243/243.
- texCache und JK2-Endungsfolge (FIX6 P54) nach dem Regelwerkwechsel: die Vorschau ist richtig - ein Wechsel von/zu Jedi
  Outcast laeuft immer ueber `rescanGamePaths` (leert texCache, figurStufenBilder; der Bildvorrat lebt nur waehrend eines
  Ladens), `setzeVorhanden` des Registers (MR9 P2) fragt `textureCandidates` je Aufruf. Kein Rot moeglich.
  Gefunden statt dessen: das Spielbild des Echtzeit-Lichts (Bildlager, app_karte_cam.cpp) suchte die Datei wie der Editor
  (QERApp_LoadImage: Name ohne Endung, jpg, png, tga) und blieb nach dem Wechsel des Regelwerks bei den alten Dateien -> Teil 2.5.
- Voller Gruenlauf (`gruen.sh`, Summe `gruen_summe.txt`): sauberer Neubau 0 Warnungen (809 s), ctest 243/243, figurlicht 8/0,
  mission1 98/0, modellblock funktionen 61/0, md 86/0, haut 1/0, pt 41/0, ptfix3 12/0, karteneditor rd 33/0, r3 22/0,
  editorportal 19/0, geschuetze (BHED_GV_DXT=1, t1_sour) 16/0, laserfalle 9/0, menueeditor en 2787/0, cjkweit ja 19/0,
  Modus 1 2635/0; Lints wie die Basis bis auf lint_portability 100 (MR9: `src/bildregister.cpp` std::to_string ohne
  `<string>`, behoben -> 99); Handbuch `--pruefen` ok.

## Teil 2: MR9-Reste (Rot `m4_rot_*`, Gruen `m4_gr_*`, Selbsttests karteneditor rd und geschuetze t1_sour OpenJK DXT)

1. P57 (Spielbild im Echtzeit-Licht immer in der Fassung des Spiels): Bildlager-Auftrag mit zwei Merkmalen - `spiel` (Pyramide
   wie Upload32, ohne Gamma und Basisstufe des Editors) und `dxt` (nur JKA/JK2). Rot: "Stufen Echtzeit 5, Spiel 5, gleich 0"
   (Movie Duels, Gamma 2 im Editor); Gruen: "gleich 5". `echtBildTextur` holt jetzt ebenfalls ueber `spielLaden`.
   P55: das Bild wandert in die Stufen (std::move statt Kopie), DXT im Bildlager ohne die gemeinsame Mannschaft
   (`dxt5Rundlauf(..., mitMannschaft false)`, gleiche Bytes) - die DXT-Stufenvergleiche in rd bleiben bytegleich (36/0).
   P56: `texturBilderAnhalten()` vor `destroyApp` in wWinMain (anhalten oeffentlich). Kein Rot-Test (Beenden waehrend des
   Ladens ist nicht steuerbar nachzustellen).
2. MR9 Punkt 4a (Klammern des Ersten): eigener CLAMP-Abtaster `abtasterKlemm`, `Zustand3::klammern` aus SpielLaden (BildWahl
   des Registers) an Welt-, Bewegt-, Gluehstufen, Effekten und Gras des Echtzeit-Lichts. Rot "clampMap zuerst: 0 0", Gruen
   "clampMap zuerst: 1 1, map zuerst: 0 0".
3. P59 (Figuren): dieselbe Datei zweimal im Hautshader - die erste Entscheidung gilt (figurStufenListe je mappingName). Rot
   "Stufen 2, davon ohne TC 1", Gruen "ohne TC 0". Dazu ctest bilder (Register) und geschuetze (Karte) - beide waren durch MR8
   schon richtig (gruen ohne Aenderung).
4. Punkt 4b (Echtzeit-Licht ohne gebautes Spielbild: Registrierreihenfolge wie die Vorschau) - nicht umgesetzt, siehe Offen.
5. Endungsfolge des Spielbilds: das Bildlager sucht in der Fassung des Spiels die Dateien aus `textureCandidates` (im Hauptfaden
   bestimmt) - OpenJK/Movie Duels die genannte Endung zuerst, dann jpg, png, tga; Jedi Outcast 1.04 nur jpg, tga (FIX6 P54).
   Die Stufenbilder behalten dafuer ihre Endung (`s.bild`, animMap), der Editor schneidet sie in `bildLaden` selbst ab. Ein
   Wechsel von/zu Jedi Outcast leert das Bildlager. Rot (alt): "OpenJK m4end.tga 51/119/255 (Ersatzbild), m4png.png
   51/119/255"; Gruen "m4end.tga 0/0/255, m4png.png 0/255/0; Jedi Outcast m4png.png 51/119/255 (Ersatzbild)". Gegenprobe nur
   des Leerens (Bedingung `m_jk2` voruebergehend entfernt, Lauf `m4_rot2_rd`): "Jedi Outcast m4png.png 0/255/0" FEHLER - das
   Leeren beim Wechsel ist noetig.
- Gruen gesamt: rd 36/0, geschuetze 18/0, ctest 243/243 (inkrementeller Bau 0 Warnungen).

## Teil 3: Chamfer-Feinheiten - nicht bearbeitet (Kontext), siehe Offen

## Offen

- Punkt 4b: ohne gebautes Spielbild entscheidet im Echtzeit-Licht weiter der zuerst gezeichnete Shader. Genau ginge es nur mit
  der Flaechenfolge der .bsp (q3map2 ProcessWorldModel: Patches, dann Pinselseiten, Modelle, Decals; mit -meta umsortiert) -
  ohne Bau gibt es sie nicht. Machbar und genau: wenn die Karte aus einer .bsp geoeffnet wurde, deren Register (vorWelt +
  weltShaderReihenfolge) nehmen.
- Teil 3 Chamfer (NOTIZEN_ptfix3_0811.md Punkt 3): Naehte bei ungeraden Segmenten, UV an Ecken/Faechern/Streifen, ein Ringpunkt
  zwischen zwei gefasten Kanten.
- P55 Rest: shaderZiehen holt weiter zuerst den Editor-Eintrag (zweites Lesen/Entpacken je Bild).
- Das Spielbild eines Shaders ohne Stufenbild (`holen(st.shader)`) laedt weiter `texturName` des Editors (qer_editorimage),
  nicht das Bild der ersten Stufe.
- Keine Bildzeit gemessen (Schlussrunde).

## Changelog

## English

- Map editor, real-time light: the game picture uses the game's version of every image under Movie Duels too (no editor gamma or texture quality), keeps clampMap of the shader that loaded the image first, and finds image files like the game (the named extension first; Jedi Outcast only jpg and tga) - also after switching the rules.
- Mission preview: the same image twice in a character shader is compressed like the first use, as in the game.
- Closing the program while the map editor still loads images no longer risks a crash at exit.

## Deutsch

- Karten-Editor, Echtzeit-Licht: Das Spielbild nimmt auch unter Movie Duels die Fassung des Spiels (ohne Gamma und Texturqualität des Editors), übernimmt clampMap des Shaders, der das Bild zuerst lud, und sucht Bilddateien wie das Spiel (die genannte Endung zuerst; Jedi Outcast nur jpg und tga) - auch nach einem Wechsel des Regelwerks.
- Mission-Vorschau: Dasselbe Bild zweimal in einem Figurenshader wird wie im Spiel so komprimiert wie beim ersten Mal.
- Das Programm zu schließen, während der Karten-Editor noch Bilder lädt, riskiert keinen Absturz beim Beenden mehr.

## 中文

- 地图编辑器，实时光照：游戏画面在 Movie Duels 下也使用游戏版本的图像（不带编辑器的伽马和纹理质量），沿用最先加载该图像的着色器的 clampMap，并像游戏一样查找图像文件（先找指定的扩展名；Jedi Outcast 只找 jpg 和 tga）——切换规则后同样如此。
- 任务预览：角色着色器中两次使用同一图像时，会像游戏中一样按第一次使用的方式压缩。
- 在地图编辑器仍在加载图像时关闭程序，不再有退出时崩溃的风险。

## 日本語

- マップエディター、リアルタイムライト：ゲーム画像は Movie Duels でもゲーム版の画像を使い（エディターのガンマとテクスチャ品質なし）、最初に画像を読み込んだシェーダーの clampMap を引き継ぎ、ゲームと同じ順で画像ファイルを探します（指定の拡張子が先、Jedi Outcast は jpg と tga のみ）。ルールを切り替えた後も同様です。
- ミッションプレビュー：キャラクターシェーダーで同じ画像を2回使うと、ゲームと同じく最初の使用と同じ圧縮になります。
- マップエディターが画像を読み込んでいる最中にプログラムを閉じても、終了時にクラッシュする恐れがなくなりました。

## Español

- Editor de mapas, luz en tiempo real: la imagen del juego usa también en Movie Duels la versión del juego de cada imagen (sin la gamma ni la calidad de textura del editor), conserva el clampMap del shader que cargó la imagen primero y busca los archivos de imagen como el juego (primero la extensión indicada; Jedi Outcast solo jpg y tga), también tras cambiar las reglas.
- Vista previa de misión: la misma imagen dos veces en un shader de personaje se comprime como en su primer uso, igual que en el juego.
- Cerrar el programa mientras el editor de mapas aún carga imágenes ya no arriesga un cierre inesperado al salir.

## Čeština

- Editor map, světlo v reálném čase: herní obraz používá i v Movie Duels herní verzi každého obrázku (bez gamy a kvality textur editoru), přebírá clampMap shaderu, který obrázek načetl jako první, a hledá soubory obrázků jako hra (nejdřív uvedenou příponu; Jedi Outcast jen jpg a tga) – i po přepnutí pravidel.
- Náhled mise: stejný obrázek dvakrát ve shaderu postavy se komprimuje jako při prvním použití, stejně jako ve hře.
- Zavření programu, zatímco editor map ještě načítá obrázky, už nehrozí pádem při ukončení.
