# Notizen CW-IMPORT: Clone-Wars-Kette Teil 2 (14./15.11.2026, `Y:\p_ap9`, Zweig `p_cwimport`, mit CW-PRUEF auf karte1 0f7e74478 rebased)

Auftrag: `JKStudio_Uebergabe\plaene\clonewars_kette.md`, Teil CW-IMPORT, dazu die Vorgabe des Hauptchats vom 14.11. (shank: "das ist
wichtig das es zwar drin ist aber nicht standard"). Build `Y:/p_ap9/build` (`--parallel 2 -- /nodeReuse:false`, Leerlauf;
`-DBEHAVED_MD_QUELLEN=Y:/werkzeuge/sp_654alt/md9`), Laeufe `cw_*` unter `Y:\p_ap9\build\laeufe`, Skripte und Protokolle
`Y:\werkzeuge\cwimport\`. Quellen: OpenJK `Y:\werkzeuge\sp_af9c\m3\ojq\src`, Movie Duels Update 9 `Y:\werkzeuge\sp_654alt\md9`,
q3map2 im Repo, Shader von Jedi Academy aus `GameData\base\assets1.pk3` (nur gelesen), Source SDK 2013 (GitHub, gelesen).

## Pruefprofil "Clone-Wars-Kette" (ab Werk aus)

- `PruefOptionen::clonewars` (optimierer.h, Makro `BHED_KARTE_CLONEWARS_SCHALTER`): `pruefeKarte` ruft die 16 Pruefungen von CW-PRUEF nur
  damit; ohne entsteht nur die Liste der Shader und Bilder fuer den Textbericht (`clonewarsInventar`, keine Befunde).
- Einstellung `OptimizerCloneWars` (karte=-Leser), Seite Optimizer: Klappliste "Pruefprofil" (Standard / Clone-Wars-Kette, die Masse von
  CW-PRUEF nur beim Profil), Build-Menue "Clone-Wars-Kette (erweiterte Pruefungen)" (prueft einen offenen Optimierer neu). Knopf JSON und
  JSON im Rechtsklick nur mit dem Profil. "Modell + Clip" legt seine Clip-Brushes nur mit dem Profil in die Ebene Collision.
- **Beleg gegen karte1** (Werkzeug `tests/kartecwvergleich.cpp`, dieselbe Datei gegen den Kern von karte1 `Y:/karte/build-test` und von
  CW-PRUEF `Y:/p_gd/build` uebersetzt, `Y:\werkzeuge\cwimport\basis`, `basis_cw`, Vergleich `vergleich.py`): auf t1_sour, duel1 und kor1
  (die .map aus Open .bsp von CW-PRUEF, Original-.bsp) sind die Befunde mit Profil aus **gleich** denen von karte1 348f0e345 (9/6/7 Befunde
  samt Objekten), mit Profil an **gleich** denen von CW-PRUEF (16/11/14), die geschriebene .map bytegleich. Einziger Unterschied in den
  Grenzen (keine Befunde): SHADER_MAX_VERTEXES fuer Movie Duels 3999 statt 999 - das ist die Korrektur von CW-PRUEF nach der Quelle von
  Update 9 (qfiles.h:33), keine Clone-Wars-Regel; bewusst nicht hinter das Profil gelegt (Frage 1 unten).
- Oberflaeche: Selbsttest cwpruef (Profil Standard: Fehlerraum ohne CW-Befund, kein Knopf JSON; Klick Build > Clone-Wars-Kette: neu
  geprueft, JSON da) 26 OK; Originalkarten mit Profil an (Lauf cw_orig_an) - Text- und JSON-Berichte gleich cw_orig4 von CW-PRUEF (bis auf
  Pfade und Millisekunden). Rot belegt: ctest karte_cwvergleich auf dem Kern von CW-PRUEF (1 CW-Befund bei Schalter aus).

## Punkt 1: Testfigur (ab Werk aus)

- Masse aus `karte::spielfigur` (CW-PRUEF, Quellen: OpenJK code/game/g_client.cpp:55-58 bzw. MD9 :94-96 x/y +-16, codemp g_client.c:31
  +-15; bg_public.h:51-53 bzw. MD9 :76-78 z -24 ... 40, geduckt 16; STEPSIZE bg_public.h:71/MD9 :93), Tuer/Gang aus den Einstellungen des
  Optimierers (110 x 64, 128 x 96). Zeichnung `karte::testfigurZeichnung` (src/karte/cwimport.cpp): Kasten, andere Hoehe, STEPSIZE,
  Tuer- und Gangrahmen (quer umschaltbar), Ursprungskreuz; Beschriftung in 2D.
- Spielermodell: `models/players/kyle/model.glm` (Einstellung BhedTestFigureModelPath), `karte::glmLesenInPose` - GLA-Bild aus der
  animation.cfg neben der GLA: stehend BOTH_STAND1 (PM_Footsteps ohne Bewegung, bg_pmove.cpp:8336), geduckt BOTH_CROUCH1 (:8180, MD9
  :14393) im gehaltenen letzten Bild (loopFrames -1 -> BONE_ANIM_OVERRIDE_FREEZE, bg_panimate.cpp:4723); gezeichnet am Ursprung wie
  CG_Player (cg_players.cpp:7178-7184). `glmStandbild` hat dafuer einen Bild-Parameter (Vorgabe 0 wie bisher).
- Oberflaeche: View > Testfigur (Zeigen F7, Geduckt, Marken, Spielermodell, Rahmen drehen, Vor die Kamera), Knopf am Ende der
  Werkzeugleiste mit Rechtsklickmenue und Tooltip (F7), Shift+F7 und "Testfigur hierher" im Rechtsklickmenue der Ansichten. 2D ueber
  kartegpu bzw. ImGui, Kamera ueber kartegpu (Modell schattiert, Linien sichtbar voll, verdeckt gestrichelt).
- Tests: ctest karte_cwimport (Masse je Regelwerk, Marken, quer, Tuermass aus Einstellungen, Pose); Selbsttest cwimport P1 auf Direct3D 12
  und 11 gruen (cw_p1_d12 22/0, cw_p1_d11 21/0 - der Unterschied ist die Abgeberpruefung von D3D12), Fotos 2538 x 1472 angesehen
  (stehend und geduckt, Modell im Kasten). Rot: fuer Punkt 1 gab es keinen eigenen roten Lauf (der Test brauchte die Funktionen zum
  Uebersetzen); ehrlich nachgetragen: die Punkte 2-6 hatten einen roten Lauf (cw_rot 27/20).

## Punkt 2: Import mit Massstab und Achsen

- `NetzOptionen` um Achsen, Vorzeichen und `umdrehen`; `karte::achsVorlage`: Vorgabe (wie bisher, ab Werk), Battlefield 3 / Y oben
  (x, -z, y), Battlefront 2 / Z oben (wie sie sind), Source/GMod (Z oben, 64 / 72: Source SDK 2013 src/game/shared/gamerules.cpp:47-61
  VEC_HULL_MAX (16, 16, 72), Jedi Academy 64), Eigene (Achse und Vorzeichen je Q3-Achse). Gespiegelte Achsen drehen den Umlauf wie eine
  gespiegelte Lage; "Umlauf/Normalen umdrehen" (Empire at War) dreht zusaetzlich (auch fuer Modelle wie q3map2 und Ziehen/Ablegen).
- Dialog: Vorlage, eigene Achsen, Umlauf, Groesse in Einheiten und Testfiguren, "Hoehe = Testfigur", Vorderansicht des Modells neben der
  Testfigur (Kasten, STEPSIZE, geduckt). Gemerkt: BhedImportPreset, BhedImportAxes, BhedImportFlip. Vorgabe liest unveraendert.
- Selbsttest P2: Vorgabe 10 x 30 x 20 wie bisher, BF2 10 x 20 x 30, Umlauf (erstes Dreieck andersherum), Source x 64/72, BF3, Hoehe 64,
  Vorschau gezeichnet, Einstellungen gemerkt.

## Punkte 3 bis 6: Werkzeuge (src/karte/cwwerkzeuge.cpp, Modify > Clone-Wars-Werkzeuge, oben im Rechtsklickmenue der Ansichten)

- **Raum skalieren** (Dialog: Faktor, Name, um die Mitte der Auswahl unten): Pinsel/Patches skaliert, Punkt-Entities wandern, Modelle
  behalten die Groesse; info_notnull mit cw_scale/cw_room; Kommentar `// cw_scale 1.2 cw_room "..." pivot x y z (JK Studio: room scale,
  data only)` nach `// entity N` (nur bei cw_scale, liest sich wieder gleich); "Skalieren und als Teil speichern" ruft File > Save
  selected. Eine Undo-Stufe.
- **Treppe zu Clip-Rampe**: Tritte = begehbare Flaechen (Normale z >= 0,7), Richtung vom untersten zum obersten Tritt, Rampe durch die
  Vorderkanten (so flach, dass keine darueber liegt), Fuss auf der Unterkante, oben bis zum Ende des obersten Tritts; textures/system/clip
  (JKA: playerclip + monsterclip). Stufen: eine verdeckte Seite (caulk, sonst die untere) wird textures/system/nodraw (JKA nodraw +
  nonsolid; SetBrushContents map.cpp:455-503: eine nicht feste Seite macht den Pinsel durchlaessig), misc_model ohne Clip-Spawnflags
  (eClipFlags model.cpp:357-371), reine Kollisionspinsel der Auswahl entfernt. Warnung bei steiler als MIN_WALK_NORMAL.
  **Beleg an q3map2** (Selbsttest, Pmove navSpielerPmove gegen leseKollision der gebauten .bsp, forwardmove 127, 8 ms): vorher groesster
  Hoehensprung je Bild 16,0 (Stufen), nachher 1,13 (glatt), oben beide 88,125. **Rot-Gruen mit CW-PRUEF:** ctest - nicht feste sichtbare
  Stufen melden Sicht gegen Kollision (rot), nach dem Werkzeug 0; feste Stufen danach 0; Selbsttest - der Optimierer (Profil an) meldet an
  der Treppe weder laufohnekollision noch kollisionsabstand.
- **Glatte Clip-Wand**: Ebene (Richtung aus den Seiten zur Kamera, Flaeche gewichtet; vorn am weitesten vorstehenden Punkt, mindestens 8
  dick) oder konvexe Huelle (QuickHull wie NRC).
- **Ebene Collision**: `kollisionsEbeneHolen` legt sie beim ersten Werkzeug an (violett, die aktuelle Ebene bleibt), "Ebene Collision
  zeigen" (Modify-Menue) blendet sie ueber LayersSetVisible aus/ein. Ohne Werkzeug keine Ebene (ctest).
- Gefunden und behoben (ctest rot): die Werkzeuge lasen Wicklungen, bevor der Pinsel sie gebaut hatte (bRep) - leere Auswahl; die Wand
  nahm die Hauptachse der Punkte und kippte bei Vorspruengen um 0,7 Grad - jetzt die Seiten zur Kamera.

## stufehoch an Rampen (Entscheidung Hauptchat 14.11., Code von CW-PRUEF)

- Die Pruefung stufehoch meldete die steigende Seitenkante einer Rampe (seitlich liegt der Boden an einer Stelle 18 bis 31,6 tiefer),
  auch an der Clip-Rampe des Werkzeugs. Die Oberkante einer Stufe liegt waagerecht: Kanten mit mehr als 1 Einheit Hoehenunterschied
  zaehlen nicht mehr (optimierer_cw.cpp stufen()). Rot: ctest karte_cwimport "stufehoch: die Seitenkante der Clip-Rampe ist keine Stufe"
  (step 19.2 at 4.8 -64 19.2), gruen danach; karte_cwpruef 69/0 unveraendert. Auf den Originalkarten mit Profil an: kor1 stufehoch 56 -> 27
  (Saeulenfuesse und Felsen mit steigenden Kanten), t1_sour und duel1 gleich.

## Gegenprobe (Vorgaben unveraendert, gegen karte1 0f7e74478)

- `Y:\werkzeuge\cwimport\gegenprobe.sh` -> `gegenprobe.out`: der Kern dieses Zweigs schreibt t1_sour, duel1, kor1 (Open .bsp) und die
  Testkarten vergleich_bp, vergleich_q3, oa_fan, markenmeta_bp nach dem Lesen **bytegleich** wie der Kern von karte1 0f7e74478
  (`basis/`, Bibliotheken aus Y:/karte/build-test vom 15.11. 10:00); bhed_q3map2 dieses Zweigs und das von karte1 bauen cw_treppe und
  duel1 (-bsp -meta, Movie Duels) zu **derselben .bsp** - einziger Unterschied ist die Uhrzeit im Kopf ("I LOVE MY Q3MAP2 ... on <Datum>",
  `zeitlos_gleich.py`). q3map2 ist in beiden Zweigen unveraendert (git diff 348f0e345 -- third_party/q3map2 leer).
- Optimierer: Profil aus = karte1 0f7e74478 (Befunde und Objekte gleich, nur die Grenze SHADER_MAX_VERTEXES fuer Movie Duels 3999,
  Entscheidung 1); Profil an = CW-PRUEF bis auf stufehoch kor1 (oben).

## Tests

- Bau nach dem Rebase auf karte1 0f7e74478 (alle Ziele): 0 Fehler, 0 Warnungen (bau7); danach inkrementell 0 Warnungen.
- ctest: karte_cwimport 35/0, karte_cwpruef 69/0, karte_cwvergleich, karte_optimierer, karte_grenzen, karte_geoimport,
  karte_md3import, karte_befehle, einstellungen_alle, handbuch_suche gruen; handbuch_sprachen und hilfe_orte waren rot (Bilder fehlten
  noch bzw. src/hilfe_orte.inc veraltet - auch der Teil des Model-Tabs aus TEST1411, mit tools/gen_hilfe_orte.py neu erzeugt),
  `gen_handbuch.py --pruefen`: handbuch_sprachen ok.
- Selbsttest-Gruppen karteneditor stufe3, stufe4, stufe5, stufe9, modobj, aseb, kreste, rest8, cwpruef, cwimport (Import und
  Optimierer, darin cwimport Punkte 1-6 und cwpruef): Direct3D 11 (Lauf cw_gr3_d11) 741 OK, Direct3D 12 (Lauf cw_gr4_d12) 742 OK. Rot bleiben je 5,
  Fehler, die der Bau von karte1 0f7e74478 genauso hat (Lauf cw_s9_karte1 mit Y:/karte/build-test): stufe9 "Im Spiel testen" (2),
  "Modell ziehen ... .pk3", "Pack map as .pk3 ... Import-Ordner" und NPC-Wege - kein Code dieses Zweigs beteiligt, an den Hauptchat
  gemeldet. Die Port-Pruefungen von stufe3 schlagen nur fehl, wenn zwei Laeufe gleichzeitig bauen (allein gruen).
- Gefunden: der Gruppenlauf zeigte im langen Lauf nur die Kamera (Anordnung einer frueheren Gruppe) - der Test der Testfigur wartete auf
  eine 2D-Ansicht; jetzt zaehlt 2D nur, wenn eine 2D-Ansicht gezeichnet wird (xyAufrufe). Die Pruefungen der Werkzeugleiste (befehle,
  stufe6) kennen nur NRC-Befehle: eigene Befehle mit der Vorsilbe "Bhed" (Knopf der Testfigur) sind dort ausgenommen und werden in der
  Gruppe cwimport per Klick und Rechtsklick geprueft. Gruppen rahmen, tasten, befehle, stufe2, stufe6: Direct3D 11 (cw_ui_d11)
  2106 OK, 2 FEHLER (genau diese beiden: Checkliste und "Knopf BhedTestFigure: keine Erwartung"), nach der Ausnahme befehle und
  stufe6 unter Direct3D 12 (cw_ui2_d12) 1667 OK, 0 FEHLER. Ein Eintrag in die Erwartungstabelle von stufe6 geht nicht ("die Tabelle
  nur echte Befehle", cw_ui_d12) - deshalb die Ausnahme in der Schleife.
- Handbuchbilder h_cwimport_figur, _import, _rampe in 6 Sprachen (2538 x 1472, angesehen: de, en, ja; ohne Tooltip, Ansichten auf der
  Treppe); karte5 in 6 Sprachen (Profil an).
- Lints (vor dem Rebase): i18n, i18n_erzeugt, menue, settings, headers, sources, feldinit, cjkumbruch, doppelanzeige, abstand, layout,
  gapp, imgui_context, tabellenid ok; portability 97, pruefcodes 11 (beide wie vorher).

## Fragen

1. SHADER_MAX_VERTEXES fuer Movie Duels (3999 statt 999) aendert die Grenzen-Tabelle auch ohne Profil (Korrektur von CW-PRUEF nach Update
   9, kein Befund betroffen) - so lassen?
2. stufehoch (CW-PRUEF) meldet die Seitenkante einer Clip-Rampe (seitlicher Abfall zwischen 18 und 31,6 an der Schraege) - Vorschlag:
   Stufen nur an Kanten quer zur Steigung zaehlen (Code von CW-PRUEF, nicht geaendert).
3. Commits: Pruefprofil und Punkt 1 je ein Commit; Punkte 2-6 teilen Dialog, Menue, Texte und Selbsttest und stehen in einem Commit
   (entschieden: ein Commit, die Nachricht nennt jeden Punkt).

Entschieden (Hauptchat 14.11.): 1. die Grenze bleibt (Korrektur nach MD9, im Changelog als Fehlerbehebung), 2. stufehoch behoben (oben),
3. ein Commit fuer 2-6. Offen: keine.

## Changelog

### English

- Map editor: the extensions for the requirements of a Clone Wars mod are in, but off by default - with the defaults the optimizer,
  the import and the saved map behave exactly as before. The check profile "Clone Wars chain" (Build menu, Preferences > Optimizer) turns
  on the extended checks and the JSON report. A test figure in original size (View > Test figure, F7, toolbar): the box the engine moves
  the player with, STEPSIZE mark, door and corridor sizes, and the game's player model standing or crouched. The geometry import gets
  templates for Battlefield 3 (Y up), Battlefront 2 (Z up), Source/GMod (scale 64/72) and custom axes, "Flip winding/normals", the size
  in test figures and a preview next to the figure. New tools under Modify > Clone Wars tools: scale a room (factor as cw_scale on an
  info_notnull and as a comment in the .map), stairs to clip ramp (the player walks up smoothly in the game), smooth clip wall, and the
  layer Collision for all their clip brushes.
- Map editor, optimizer: the limit SHADER_MAX_VERTEXES for Movie Duels follows its Update 9 source (4000 vertices per surface instead of 1000); the step check no longer reports the rising side of a ramp.

### Deutsch

- Karten-Editor: die Erweiterungen fuer die Anforderungen eines Clone-Wars-Mods sind drin, aber ab Werk aus - mit den Vorgaben verhalten
  sich Optimierer, Import und gespeicherte Karte genau wie bisher. Das Pruefprofil "Clone-Wars-Kette" (Build-Menue, Einstellungen >
  Optimizer) schaltet die erweiterten Pruefungen und den JSON-Bericht ein. Eine Testfigur in Originalgroesse (View > Testfigur, F7,
  Werkzeugleiste): der Kasten, mit dem die Engine die Spielfigur bewegt, die Marke STEPSIZE, Tuer- und Gangmasse und das Spielermodell des
  Spiels stehend oder geduckt. Der Geometrie-Import bekommt Vorlagen fuer Battlefield 3 (Y oben), Battlefront 2 (Z oben), Source/GMod
  (Massstab 64/72) und eigene Achsen, "Umlauf/Normalen umdrehen", die Groesse in Testfiguren und eine Vorschau neben der Figur. Neue
  Werkzeuge unter Modify > Clone-Wars-Werkzeuge: Raum skalieren (Faktor als cw_scale an einem info_notnull und als Kommentar in der .map),
  Treppe zu Clip-Rampe (die Spielfigur laeuft im Spiel glatt hinauf), glatte Clip-Wand und die Ebene Collision fuer alle ihre Clip-Pinsel.
- Karten-Editor, Optimierer: die Grenze SHADER_MAX_VERTEXES fuer Movie Duels folgt der Quelle von Update 9 (4000 Ecken je Flaeche statt 1000); die Stufenpruefung meldet die steigende Seite einer Rampe nicht mehr.

### 中文

- 地图编辑器:针对一个 Clone Wars 模组需求的扩展已加入,但默认关闭——使用默认设置时,优化器、导入和保存的地图与以前完全相同。检查配置
  “Clone Wars 流程”(Build 菜单、首选项 > Optimizer)会打开扩展检查和 JSON 报告。原始尺寸的测试人物(视图 > 测试人物、F7、工具栏):引擎移动玩家
  所用的盒子、STEPSIZE 标记、门和走廊尺寸,以及站立或蹲下的游戏玩家模型。几何体导入新增 Battlefield 3(Y 向上)、Battlefront 2(Z 向上)、
  Source/GMod(比例 64/72)和自定义坐标轴模板、“翻转环绕方向/法线”、以测试人物计的尺寸以及人物旁的预览。Modify > Clone Wars 工具 下的新工具:
  缩放房间(系数作为 info_notnull 上的 cw_scale 并作为 .map 中的注释)、楼梯转为 clip 坡道(游戏中玩家平滑地走上去)、平滑 clip 墙,以及存放所有
  这些 clip 画刷的图层 Collision。
- 地图编辑器,优化器:Movie Duels 的 SHADER_MAX_VERTEXES 限制改为依据其 Update 9 源码(每个表面 4000 个顶点,而不是 1000);台阶检查不再把坡道上升的侧边报告为台阶。

### 日本語

- マップエディター: Clone Wars MOD の要件に向けた拡張が入りましたが、既定ではオフです。既定のままなら最適化ツール、インポート、保存される
  マップはこれまでとまったく同じです。チェックプロファイル「Clone Wars チェーン」(Build メニュー、設定 > Optimizer)で拡張チェックと JSON
  レポートが有効になります。実物大のテストフィギュア(表示 > テストフィギュア、F7、ツールバー): エンジンがプレイヤーを動かす箱、STEPSIZE の印、
  ドアと廊下の寸法、立ちまたはしゃがみのゲームのプレイヤーモデル。ジオメトリのインポートに Battlefield 3(Y が上)、Battlefront 2(Z が上)、
  Source/GMod(倍率 64/72)、カスタム軸のテンプレート、「巻き方向/法線を反転」、テストフィギュア何体分かのサイズ、フィギュアの横のプレビューが
  付きました。Modify > Clone Wars ツール の新しいツール: 部屋の拡大縮小(係数は info_notnull の cw_scale と .map のコメント)、階段をクリップ
  スロープに(ゲーム内でプレイヤーが滑らかに上ります)、滑らかなクリップ壁、それらのクリップブラシをまとめるレイヤー Collision。
- マップエディター、最適化ツール: Movie Duels の SHADER_MAX_VERTEXES の上限は Update 9 のソースに従います(面あたり 1000 ではなく 4000 頂点)。段差チェックはスロープの上り側の辺を報告しなくなりました。

### Español

- Editor de mapas: las ampliaciones para los requisitos de un mod de Clone Wars están incluidas, pero desactivadas por defecto: con los
  valores predeterminados, el optimizador, la importación y el mapa guardado se comportan exactamente como antes. El perfil de
  comprobación "Cadena Clone Wars" (menú Build, Preferencias > Optimizer) activa las comprobaciones ampliadas y el informe JSON. Una
  figura de prueba a tamaño original (Ver > Figura de prueba, F7, barra de herramientas): la caja con la que el motor mueve al jugador, la
  marca STEPSIZE, las medidas de puertas y pasillos y el modelo del jugador del juego de pie o agachado. La importación de geometría
  recibe plantillas para Battlefield 3 (Y arriba), Battlefront 2 (Z arriba), Source/GMod (escala 64/72) y ejes propios, "Invertir orden /
  normales", el tamaño en figuras de prueba y una vista previa junto a la figura. Nuevas herramientas en Modify > Herramientas Clone Wars:
  escalar una sala (factor como cw_scale en un info_notnull y como comentario en el .map), escalera a rampa clip (en el juego el jugador
  sube sin saltos), pared clip lisa y la capa Collision para todos sus brushes clip.
- Editor de mapas, optimizador: el límite SHADER_MAX_VERTEXES de Movie Duels sigue su código de Update 9 (4000 vértices por superficie en lugar de 1000); la comprobación de escalones ya no informa del lado ascendente de una rampa.

### Čeština

- Editor map: rozšíření pro požadavky módu Clone Wars jsou zabudována, ale ve výchozím stavu vypnutá - s výchozím nastavením se
  optimalizátor, import i uložená mapa chovají přesně jako dosud. Profil kontrol "Řetězec Clone Wars" (nabídka Build, Předvolby >
  Optimizer) zapne rozšířené kontroly a zprávu JSON. Testovací postava ve skutečné velikosti (Zobrazit > Testovací postava, F7, panel
  nástrojů): kvádr, kterým engine pohybuje hráčem, značka STEPSIZE, rozměry dveří a chodeb a model hráče ze hry ve stoje nebo přikrčený.
  Import geometrie dostal šablony pro Battlefield 3 (Y nahoru), Battlefront 2 (Z nahoru), Source/GMod (měřítko 64/72) a vlastní osy,
  "Obrátit pořadí/normály", velikost v testovacích postavách a náhled vedle postavy. Nové nástroje v Modify > Nástroje Clone Wars: změna
  měřítka místnosti (faktor jako cw_scale na info_notnull a jako komentář v .map), schody na clip rampu (hráč ve hře vyjde plynule),
  hladká clip stěna a vrstva Collision pro všechny jejich clip brushe.
- Editor map, optimalizátor: limit SHADER_MAX_VERTEXES pro Movie Duels se řídí zdrojem Update 9 (4000 vrcholů na plochu místo 1000); kontrola schodů už nehlásí stoupající stranu rampy.
