# Notizen p_jka: Spielvergleiche Original-JKA und Reste aus 1d (07.11.2026, paralleles Fenster)

Zweig `jka1` (Worktree `Y:\jka`, Basis e6daa5ca8), Build `Y:/jka/build-jka`, Laeufe `jk_*` unter `Y:\jka\laeufe`,
Spielskripte im Scratchpad `pj/` (spiel.py, vergleich.py, vergleich_uv.py).

## Quellen

- Autodesk-Hilfe 3ds Max 2024 "Keyboard Shortcuts" (help.autodesk.com, GUID-A73E1B09-7BFE-4A22-8153-1D3D2237B8E9), Tabellen
  Edit Poly / Editable Poly: Extrude Mode Shift+E, Bevel Mode Ctrl+Shift+B, Chamfer Mode Ctrl+Shift+C, Collapse Ctrl+Alt+C,
  Connect Shift+Ctrl+E, Cut Alt+C, Make Planar Alt+M (nur Edit Poly), Quickslice Shift+Ctrl+Q, Repeat Last ";", Target Weld
  Mode Shift+Ctrl+W; Main UI / Editable Polygon Object: Extrude Face (Poly) Alt+Shift+E, Select Sub-object Loop Alt+L, Ring
  Alt+R, Sub-Object Level 1-5.
- Autodesk-Hilfe "Editable Poly (Vertex)" / "(Edge)": Remove "The keyboard shortcut is Backspace", Clean Remove mit Strg.
- Autodesk-Hilfe "Additional Panels" (Make Planar): "Forces all selected sub-objects to be coplanar. The plane's normal is the
  average surface normal of the selection."
- `plaene/modellblock_stufe0/belege/max_hotkeys_de.txt` ist die Autodesk-Karte der Hauptoberflaeche (Q W E R, 1-6 Ebenen,
  Strg+A ...); Editable-Poly-Werkzeuge stehen dort nicht.
- Movie Duels Update 9 `code/cgame/cg_view.cpp` (CG_DrawActiveFrame: WP_SABER/WP_MELEE erzwingen die dritte Person, solange
  cg_fpls 0), OpenJK `code/cgame/cg_view.cpp` 2090-2095 ebenso; `code/game/g_cmds.cpp` Cmd_SetViewpos_f (origin[2] -= 25),
  `bg_pmove.cpp` viewheight = standheight + STANDARD_VIEWHEIGHT_OFFSET; `cg_consolecmds.cpp` CG_Viewpos_f.

## 1. Gebaut

- Max-Tasten des Modellierens nach der Autodesk-Tabelle 2024 geprueft: Shift+E Extrude, Ctrl+Shift+W Target Weld,
  Ctrl+Shift+E Connect, Alt+C Cut, Ctrl+Shift+C Chamfer, Backspace / Ctrl+Backspace Remove stimmen (Shift+E steht in den
  Tabellen Edit Poly/Editable Poly; Alt+Shift+E ist dieselbe Aktion in der Main UI). Neu belegt, weil Autodesk sie belegt:
  Bevel Ctrl+Shift+B, Loop Alt+L, Ring Alt+R (im Bereich Modellieren vor ModView "Texturen neu laden" Alt+R, wie in Max
  kontextabhaengig), Make Planar Alt+M, Collapse Ctrl+Alt+C (Sofortbefehl, neu). Quickslice (Shift+Ctrl+Q, zwei Klicks in
  der Ansicht) bleibt ohne Taste: das interaktive Werkzeug gibt es nicht; Slice hat in Max keine Vorgabetaste.
- Jede Aktion ohne Taste nennt im Tooltip den Weg "Netz bearbeiten > Gruppe > Name" (Text `MdWegHint`, tools/i18n_pjka.py).
- Make Planar (Max Edit Geometry, Alt+M; `ebenMachen` in src/modell/wof/mesh_werkzeuge2.cpp): Normale = Mittel der
  Einheitsnormalen (Newell) der gewaehlten Vielecke bzw. der Vielecke an den Punkten, Ebene durch die Mitte der Punkte,
  jeder Punkt senkrecht darauf, UV bleibt; Knopf im Rollout Geometrie bearbeiten, Menue, Rechtsklick, Suche, Tooltip.
- Collapse (Max Edit Geometry, Ctrl+Alt+C): jede zusammenhaengende Gruppe der Auswahl ein Punkt in ihrer Mitte (mergen Mitte,
  WL PlanCollapse) als Sofortbefehl.
- Slice (Max Edit Geometry "Slice Plane"; `schneiden` in mesh_werkzeuge.cpp): Kaestchen mit Achse und Lage (Vorgabe: Mitte
  des Kastens der Auswahl bzw. des Modells), an der Polygon-/Element-Ebene nur die gewaehlten Vielecke, sonst alle; jede
  kreuzende Kante bekommt einen Punkt genau auf der Ebene (auch im Nachbarvieleck, keine T-Stelle, UV gemischt), jedes
  Vieleck mit zwei Ecken auf der Ebene wird geteilt; die neuen Kanten sind danach gewaehlt.
- Selbsttest `mdspiel` (gui/selbsttest_md.inc): statt Bau -meta jetzt Schnell-Backen (-meta, -vis -fast, -light -fast) mit
  und ohne Requisit; je ein Foto der Kamera im Spielbild ("Built map as in the game") 1920 x 1440 (`messBreite`), ohne
  Auswahl; beide .bsp in den Laufordner (`s3bau_mit`, `s3bau_ohne`); das Protokoll nennt den Augpunkt.
- Spielskript `pj/spiel.py` (Scratchpad): Movie Duels oder Original-JKA, Karte als `pj_raum`, `PJ_ART=kamera` setzt eine
  ICARUS-Skriptkamera (camera ENABLE/MOVE/PAN/ZOOM 80, mit `bhed compile` uebersetzt, `runscript pj_kamera`) genau auf den
  Augpunkt; sonst Spielerfigur per setviewpos (dritte Person fuer die Figur). `viewpos` schreibt den Augpunkt ins Protokoll.

## 2. Rot -> gruen

- ctest `modell_mesh` Make Planar: rot mit einer Attrappe (Ablehnung immer) "Abstand nachher 0,348 / 0,287" 2 FEHL ->
  gruen 0,000000 / 0,000000, Mitte bleibt 1,1, UV gleich, zwei Punkte abgelehnt.
- ctest `modell_mesh` Slice: rot mit Attrappe (sofort zurueck) 2 FEHL ("12/20/10", "10/15/7") -> gruen siehe 4; Collapse
  nutzt mergen (schon gruen), Pruefung "Deckel -> Spitze 5/8/5".
- Erste Person in Movie Duels (1d offen): `md_probe1` mit setviewpos und `give all` + `weapon 2` (wie 1d) -> viewpos
  (-288 -143 57) statt (-208 -144 40): die Kamera steht cg_thirdPersonRange 80 hinter und 16 ueber dem Spieler, im Bild
  der Schwertgriff. Ursache: cg_view.cpp (MD 9 und OpenJK) rendert mit WP_SABER/WP_MELEE immer die dritte Person (MD nur
  mit cg_fpls 1 nicht), `weapon 2` wechselte nicht. Gruen mit der Skriptkamera: viewpos (-208 -144 40) : 0 in MD und JKA.
- Vorschau-Foto `jk_mds1` mit Auswahlfarbe, Griff und Arbeitsbereich ueber dem Requisit -> Auswahl aufgehoben (`jk_mds2`).
- Max-Tasten: kleine Tabellenaenderung ohne eigenen Rotlauf (Testregel), gruen in `jk_md1` 76/0 (neu 11 Pruefzeilen
  "Max-Taste wie Autodesk", Weg im Tooltip, Strg+Umschalt+B oeffnet Bevel, Esc bricht ab).

## 3. Im Spiel

Fenster 1920 x 1440, stumm, ausserhalb des Sichtbaren, kein Fokus, Foto per PrintWindow (md/foto7.py), Paarbilder
ungeskaliert untereinander (Spiel oben, Vorschau unten) im Scratchpad `pj/`, angesehen.

### 1d Requisit (Selbsttest `jk_mds2`, Augpunkt -208 -144 40, Blick +X, Sichtfeld 80)

Silhouette je Seite = Unterschied Bild mit/ohne Requisit (Kanal > 24); `pj/vergleich.py`.

| Spiel | Versatz Schwerpunkt | Deckung | Kasten Spiel / Vorschau | mittl. Abw. ganz / Requisit |
|---|---|---|---|---|
| Movie Duels (`md_mit1`/`md_ohne1`) | -0,0 / -0,8 px | 0,991 | 731-1188 x 613-1292 / 731-1188 x 613-1291 | 1,56 / 0,34 |
| Original-JKA (`jka_mit1`/`jka_ohne1`) | -0,2 / -0,0 px | 0,982 | 731-1188 x 614-1291 / gleich | 1,59 / 0,79 |

- Paarbilder `pj/v_md_paar.png`, `pj/v_jka_paar.png` (1920 x 2880), Masken `pj/v_*_masken.png`.
- Das Requisit ist im Spiel und in der Vorschau fast schwarz: m0raum hat kein Licht ("ambient" 8), die Lightmap des
  eingebackenen misc_model ist dunkel; das Spielbild der Vorschau zeigt dasselbe.
- Original-JKA ist etwas heller als Movie Duels und die Vorschau (Mittel ganzes Bild 77,5 / 76,7 / 77,3 gegen 75,5; im
  Requisit 3,9 gegen 3,2). Nicht weiter verfolgt (Messgrund: Unterschied der Spiele, nicht des Requisits).
- Die .bsp der Spiellaeufe ist die aus `jk_mds1` (gleiche Lage und Kamera; die .bsp von `jk_mds2` unterscheidet sich ab
  Byte 204 - Zeitstempel/Pfade im Bau, nicht geprueft).

### 1b UV im Original-JKA (Selbsttest `jk_uvs1`, BHED_MB_SPIEL=jka)

Der Sturmtruppler des Original-JKA mit "Requisit" neu abgewickelt, 4 Texturen umgebacken (Mitten 2205, mittlere Abweichung
1,47), als Mod im eigenen fs_basepath. Karte `pj/uvkarte/uvraum.bsp` = m0raum mit "ambient" 60 und einem light 500 (sonst
ist die Figur fast schwarz), Spieler als stormtrooper, dritte Person von vorn, r_lodbias -5 (LOD 0), cg_fov 60.

- neu (neue UV + umgebackene Texturen) gegen alt: mittlere Abweichung 0,22 im ganzen Bild, 1,04 im Kasten der Figur
  (506-1424 x 815-1440) - im Bild nicht zu unterscheiden (`pj/v_uv_alt_neu.png`).
- nurglm (nur die neue .glm, alte Texturen) gegen alt: 2,15 / 10,35 - dasselbe Flickenmuster wie die Vorschau "nachher"
  im Model-Tab (`pj/v_uv_nurglm_vorschau_nachher.png`; die Vorschau zeigt LOD 1, das Spiel LOD 0): das Original liest die
  neue UV so, wie JK Studio sie zeigt.

### Spielordner

- Jeder Original-JKA-Start: steam.exe lief, ActiveUser != 0 (geprueft im Skript), kein anderes jasp.exe. Die 10
  Original-Dateien (exe/dll, base/*.pk3) aus "GameData original" nach GameData kopiert, alles schon Vorhandene in GameData
  (base/saves von shank) vorher bytegleich gesichert und verglichen, danach entfernt: "GameData nachher gleich: True |
  GameData original gleich: True" bei allen 6 Laeufen (jka_mit1 zweimal, jka_ohne1, uv_alt2, uv_neu2, uv_nurglm2).
  Configs liegen im eigenen fs_basepath, nicht im Spielordner. Kein Steam-Dialog.
- Movie Duels: "Spielordner MD gleich: True" bei allen 3 Laeufen.

## 4. Schluss

- Bau: frischer Bau `build-jka` (alle Ziele) 0 Warnungen; Folgebauten nach Make Planar, Slice/Collapse 0 / 0.
- ctest 231/231 (`pj/ctest1.txt` und nach Slice/Collapse noch einmal 231/231; auch `q3map2_pr316`), `modell_mesh` mit Make
  Planar (3), Slice (3), Collapse (1).
- Selbsttests: `jk_md2` (md) 79/0 (Slice 16/34/20 und Rueckgaengig bytegleich, 12 Max-Tasten), `jk_mds2` (mdspiel) 5/0,
  `jk_uvs1` (uvspiel jka) 1/0, `jk_tasten1` 43/0, `jk_funk2` (funktionen) 60/0, Handbuchbilder `jk_hb2_*` 6 Sprachen je 2/0
  (h_md_modell zeigt die neuen Knoepfe, cs angesehen: umbricht sauber). Modus 1 `jk_modus2` 2635/0; der erste Lauf
  `jk_modus1` (vor Slice/Collapse) hatte 2634/1 "Ton: ... der Klang kommt, sobald sie gezeichnet wird" (Ton-Pruefung, nicht
  von hier beruehrt, unter Volllast von 7 Agenten) - im Wiederholungslauf gruen.
- Lints wie in 1d: portability 99, pruefcodes 11, efxflags-Meldung, fremdprogramm (kartenpruefung.py), sonst ok.
- Bildzeit: keine Messung im parallelen Fenster (Regel); die Aenderungen betreffen Tasten, Tooltips und Werkzeuge ohne
  Zeichenpfad. Messung offen, nach dem Fenster allein.

## 5. Abweichungen

- Ring Alt+R gilt nur im Bereich Modellieren (dort vor ModView "Texturen neu laden"), wie Max die Tabelle Editable Poly
  vor der Main UI nimmt.
- Make Planar: Max erlaubt zusaetzlich X/Y/Z-Knoepfe (Ebene senkrecht zur Achse) - nicht nachgebaut.
- Slice: Ebene nur quer zu X/Y/Z (Max: frei drehbarer Slice-Plane-Gizmo); ein Vieleck mit mehr als zwei Ecken auf der Ebene
  (nicht konvex) bleibt ungeteilt (bekommt nur die Punkte).
- Spielvergleich 1d mit der Skriptkamera statt der Spielerfigur (erste Person mit Waffe ginge nur ohne Schwert; `weapon 2`
  wechselte in Movie Duels nicht).
- 1b im Original-JKA auf einer eigenen hellen Testkarte (die Kartenvorlage hat kein Licht), nicht auf duel_deathstar (gibt
  es im Original nicht).

## 5b. Runde 2 (07.11. abends) und Nachtest ZUS (08.11.)

- Hinge (Max Edit Poly "Hinge From Edge"): die gewaehlten Vielecke schwenken um eine ihrer Randkanten, mit Waenden dazwischen;
  QuickSlice (Max Shift+Ctrl+Q): zwei Klicks in einer Ansicht ziehen eine Linie, geschnitten wird an der Ebene durch diese Linie
  quer zur Blickrichtung (dieselbe Rechnung wie Slice); Slice an einer beliebigen Ebene. ctest `modell_mesh` gruen.
- Nachtest ZUS: Bau 0 Warnungen; ctest 231/231 (`zs_ctest_jka`); Selbsttest funktionen 60/0 (`zs_jk_funk`); md zuerst 83/2
  (`zs_jk_md`): der QuickSlice-Schritt suchte eine sichtbare Ansicht von vorn, der Test lief mit einer Ansicht - Testfehler,
  der Test schaltet jetzt vier Ansichten an (wie Max) und danach zurueck; Modus 1 `zs_jk_m1` 2634/1 (Ton, siehe Gesamtnotiz).

## 6. Offen

- 1d-Luecken: Chamfer mit Segmenten/Profil.
- Original-JKA etwas heller als Movie Duels/Vorschau (77,5 gegen 75,5 im Mittel des ganzen Bildes) - Ursache nicht gesucht.
- Max-Tasten ohne eigenen Rotlauf (kleine Tabellenaenderung).

## Changelog

### English
- Model tab, 3ds Max mode: the modelling keys follow Autodesk's current table - new Ctrl+Shift+B Bevel, Alt+L Loop, Alt+R Ring (in the Modeling area) Alt+M Make Planar and Ctrl+Alt+C Collapse; every tool without a key names its way through the menu in the tooltip.
- New Make Planar (Edit Geometry, Alt+M): moves the selection onto one plane through its centre with the average normal; UVs stay.
- New Slice (cut along a plane across X, Y or Z, selected polygons or the whole model) and Collapse (each connected group becomes one vertex).
- New Hinge (swing selected polygons about one of their border edges, walls in between) and QuickSlice (Shift+Ctrl+Q: two clicks in a view cut the model along that line), as in 3ds Max.

### Deutsch
- Model-Tab, 3ds-Max-Modus: die Tasten des Modellierens folgen der aktuellen Autodesk-Tabelle - neu Strg+Umschalt+B Abschrägen, Alt+L Schleife, Alt+R Ring (im Bereich Modellieren) Alt+M Eben machen und Strg+Alt+C Kollabieren; jedes Werkzeug ohne Taste nennt im Tooltip den Weg über das Menü.
- Neu „Eben machen“ (Geometrie bearbeiten, Alt+M): schiebt die Auswahl in eine Ebene durch ihre Mitte mit der gemittelten Normale; die UV bleibt.
- Neu „Schneiden (Ebene)“ (Schnitt quer zu X, Y oder Z, gewählte Polygone oder das ganze Modell) und „Kollabieren“ (jede zusammenhängende Gruppe wird ein Punkt).
- Neu Scharnier (Hinge: gewählte Polygone um eine ihrer Randkanten schwenken, mit Wänden dazwischen) und Schnellschnitt (Umschalt+Strg+Q: zwei Klicks in einer Ansicht schneiden das Modell entlang dieser Linie), wie in 3ds Max.

### 中文
- 模型选项卡, 3ds Max 模式: 建模按键遵循 Autodesk 当前的快捷键表 - 新增 Ctrl+Shift+B 倒角、Alt+L 循环、Alt+R 环形 (在建模区域) 、Alt+M 平面化和 Ctrl+Alt+C 塌陷; 没有按键的工具在提示中写出菜单路径。
- 新增“平面化” (编辑几何体, Alt+M): 把选择移到经过其中心、使用平均法线的同一平面; UV 保持不变。
- 新增“切片” (沿垂直于 X、Y 或 Z 的平面切开选中多边形或整个模型) 和“塌陷” (每个相连的组变为一个顶点)。
- 新增铰链 (Hinge: 让选中的多边形绕其一条边界边旋转, 中间生成侧壁) 和快速切片 (Shift+Ctrl+Q: 在视图中点击两次, 沿该线切开模型), 与 3ds Max 相同。

### 日本語
- モデルタブ、3ds Max 風: モデリングのキーが Autodesk の現行の表に従います - 新たに Ctrl+Shift+B ベベル、Alt+L ループ、Alt+R リング (モデリング領域) 、Alt+M 平面化、Ctrl+Alt+C 集約。キーのないツールはツールチップにメニューでの場所を表示します。
- 新しい「平面化」(ジオメトリ編集、Alt+M): 選択を、その中心を通り平均法線を持つ一つの平面に移します。UV はそのままです。
- 新しい「スライス」(X・Y・Z に垂直な平面で選択ポリゴンまたはモデル全体を切る) と「集約」(つながった各グループを一つの頂点に)。
- 新しいヒンジ (選択したポリゴンを境界エッジの一つを軸に回し、間に壁を作る) とクイックスライス (Shift+Ctrl+Q: ビューで 2 回クリックしてその線に沿ってモデルを切る) を 3ds Max と同じように追加。

### Español
- Pestaña Modelo, modo 3ds Max: las teclas de modelado siguen la tabla actual de Autodesk - nuevas Ctrl+Mayús+B Bisel, Alt+L Bucle, Alt+R Anillo (en el área Modelado) , Alt+M Hacer plano y Ctrl+Alt+C Colapsar; cada herramienta sin tecla indica en la ayuda su camino por el menú.
- Nuevo «Hacer plano» (Editar geometría, Alt+M): lleva la selección a un plano por su centro con la normal media; las UV se mantienen.
- Nuevos «Cortar (plano)» (corte por un plano perpendicular a X, Y o Z, polígonos elegidos o todo el modelo) y «Colapsar» (cada grupo conectado se convierte en un vértice).
- Nuevos Bisagra (Hinge: gira los polígonos seleccionados alrededor de una de sus aristas de borde, con paredes entre medias) y Corte rápido (Mayús+Ctrl+Q: dos clics en una vista cortan el modelo por esa línea), como en 3ds Max.

### Čeština
- Karta Model, režim 3ds Max: klávesy modelování odpovídají aktuální tabulce Autodesk - nově Ctrl+Shift+B zkosení, Alt+L smyčka, Alt+R prstenec (v oblasti Modelování) , Alt+M zarovnání do roviny a Ctrl+Alt+C sloučení do bodu; každý nástroj bez klávesy uvádí v nápovědě cestu v nabídce.
- Nové „Zarovnat do roviny“ (Upravit geometrii, Alt+M): přesune výběr do jedné roviny procházející jeho středem s průměrnou normálou; UV zůstanou.
- Nové „Řez rovinou“ (řez rovinou kolmou k X, Y nebo Z, vybrané polygony nebo celý model) a „Sloučit do bodu“ (každá souvislá skupina se stane jedním vrcholem).
- Nově Pant (Hinge: otočí vybrané polygony kolem jedné z jejich okrajových hran, se stěnami mezi nimi) a Rychlý řez (Shift+Ctrl+Q: dvě kliknutí v pohledu rozříznou model podél této čáry), jako v 3ds Max.
