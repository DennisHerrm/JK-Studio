# Notizen KLEIN (12.11.): Kleinkram und rote Tests - Welle 12.11. Abschnitt 6, rc612

Zweig `p_klein` (Worktree `Y:\p_ap4`), rebased auf karte1 61ec20b03 (mit FIX612C, FIX612B, FIX612, DXR, AP14). Werkzeuge, Läufe und Belege: `Y:\werkzeuge\klein`
(`laeufe\letzter_<lauf>.txt`, `<runde>_.out`). Begonnen von Fable 5.2, nach dem Limit fortgesetzt von Opus 5.5.
Entscheidungen von shank (plaene/entscheidungen_1211.md): A11 Kamera-Knopf bleibt blass mit Hinweis, A12 „Home zurücksetzen“ in den
Rechtsklick des ViewCube.

## Je Punkt: Ursache, Behebung, Rot → Grün

| P | Ursache | Behebung | Rot (alter Stand) | Grün |
|---|---|---|---|---|
| 1 | Test: die Wurzelzeile der Entity List trägt den ganzen Kartenpfad; in einem langen Laufordner (ab ~43 Zeichen) ist jede Zeile breiter als die Liste (sie rollt waagrecht wie ein QTreeView), `klick()` meldete „nicht ganz sichtbar“ | Klick auf den sichtbaren Teil der Zeile, mit Prüfung (volle Höhe, links nicht abgeschnitten, rechts nur bei Rollen) | ke_grund im langen Ordner: 750/1 „Klick ##el… nicht ganz sichtbar (929..1696, sichtbar 928..1610)“ | ke_grund im langen Ordner 750/0; ke_grund (g1, g2) ohne Fehler an der Stelle |
| 2 | Test: zwei Runden nebeneinander würfelten denselben Portbereich der Bau-Überwachung (geprüft wurde nur, ob gerade jemand lauscht), ke_bau baute fest auf NRCs 39000 | `tools/testrunde.py`: jeder Bereich systemweit reserviert (benannter Mutex `Local\bhed_testrunde_bauport_<basis>`, frei beim Ende der Runde); ke_bau bekommt einen eigenen Bereich, Sperre „ports“ entfällt; NRCs Vorgabe 39000 prüft Stufe 3 an `ke::bauMonitorPortAus()`; ctest `testrunde_ports` | `p2_bereich.py` auf testrunde.py von karte1: 3 von 3 Zufallswerten GLEICH; zwei ke_bau nebeneinander: „Port 39000 nicht belegt: bind/listen 10048“ (p2rota/p2rotb) | `--pruefe-ports` 3/3 verschieden; ctest `testrunde_ports` grün; ke_bau mit eigenem Bereich 622/0 (g2, neben ke_grund im selben Pool) |
| 3 | Test: W13 (Max-Teil) spielt 0,7 s Wanduhr ab und hält an - das Endbild hängt an der Last (grün bis Bild 30308, rot ab 30309); der Blender-Teil lief in dieser Pose, nach rhumerus 0,3 rückte die Ecke, `vordereEcke()` fand eine andere, der Tupfer malte die andere | W12 malt am Ort der Ecke `p->a` (aus `modellEckenBild`), W13 stellt das Bild vor dem Abspielen wieder her | `BHED_TEST_BREMSE=80` und `=200`: 104/1, W13 30293 → 30311 | mb_ap8 105/0 ohne Bremse, mit `BHED_TEST_BREMSE=80` 105/0 und `=200` 105/0 (W13 endete bei 30313 bzw. 30322 - jenseits der alten Grenze 30309) |
| 4 | Programm: alle Werkzeuge teilten `ModellMesh::art` (nach Extrudieren „Je Vieleck“ (2) zeigte Einfügen eine leere Auswahl); der Treffertest der Unterobjekte (Ecke, Kante, Dreieck, Polygon, Rahmen) nahm eingefrorene Flächen | Art je Werkzeug (`modellMeshArtMerken`, gemerkt beim Übernehmen); `dreieckUnterWaehlbar` - Eingefrorenes nimmt nicht teil, liegt dort nur Eingefrorenes, nennt die Statuszeile die Fläche | mb_klein (Bau von 64a0b747a): 4a 3 FEHLER, 4b 7 FEHLER; der Klick im Bereich Modellieren an seiner eigenen Stelle mit dem alten Treffertest (Rot-Probe p4brot): wählt die eingefrorene Fläche head | mb_klein 30/0 (g4) |
| 5 | Programm: das gesetzte Home galt nur bis zur nächsten Figur | je Figur in `Settings::modellHome` (Schlüssel `modelHome`, höchstens 64, zuletzt gesetzte zuerst); Tooltip; „Aktuelle Ansicht als Home setzen“ auch in der Suche (wie Max); **„Home zurücksetzen“** im Rechtsklick des ViewCube und in der Suche (A12), blass ohne gesetztes Home | mb_klein 1 FEHLER (dieselbe Figur wieder geladen), mb_kleinneustart 2 FEHLER | mb_klein 30/0, mb_kleinneustart 9/0 (mit Home zurücksetzen, blass ohne Home) (g4); mb_kopf 117/0 |
| 6 | a Programm: der Kamera-Knopf fehlte (Blender: Zoom, Move, Camera, Persp/Ortho). b/c (Gegenkugel, N des Kompasses) hat MBRASTER behoben | vierter Knopf an Blenders Stelle, blass; Tooltip und Statuszeile „keine Szenenkamera“, die Ansicht bleibt (A11) | mb_klein 3 FEHLER (vier Knöpfe, Tooltip, Klick) | mb_klein 30/0; b/c: mb_kopf 117/0 (MBKOPF A/B) |
| 7 | fehlte: die zweite Taste eines Befehls (rc612 NUMPAD: Ziffernblock) ließ sich nur über Yes, Clear und Reset ändern | Knopf **Edit 2nd** unter Reset All (NRCs vier Knöpfe bleiben an ihrem gemessenen Platz) und Rechtsklick-Menü je Zeile (Edit, Edit 2nd, Clear, Reset); Rückfrage „Key already used“ wie bei der ersten Taste; die eigene erste Taste als zweite ändert nichts; `Belegung::setzenZweit`, gespeichert als `<Befehl>#Num` | ke_grund (Bau ohne Umsetzung): 752/11, genau die Zeilen „Numpad 2. Taste“ | ke_grund 760/0 (g2, 10 Zeilen „Numpad 2. Taste“ OK); ctest karte_befehle grün (setzenZweit) |
| 8 | Programm: Compile! heißt 1,5 s „Done“ bzw. 2 s „Failed“ (nur `###compile`), der Tourschritt `knopf:Compile` fand ihn dann nicht | feste Kennung `###compile` als Ersatz in `tour.cpp` (`festeKennung`) | reiter: „k-uebersetzen Schritt 2 … keine Zeile verdeckt“ FEHLER | reiter 94/0 |
| 9 | Generator kannte die zwei von Hand eingetragenen Vorlagen nicht | `rig_vorlagen_gen.py` erzeugt Unreal Mannequin und 100STYLE, `--pruefen`, ctest `rig_vorlagen_aktuell` | `p9_rot.py`: der alte Generator löschte 94 Zeilen | `--pruefen` „40 Vorlagen … ist aktuell“, Schreiblauf bytegleich |
| 10 | Programm: bei Schrift 24 war About 1060 Punkte hoch, Close/Changelog/Guide lagen unter dem Rand eines 1920x1080-Fensters | Liste in eigenem Feld: so hoch wie ihr Inhalt, solange das Fenster passt, sonst rollt sie; Fenster im Programmfenster gehalten | mb_klein 1 FEHLER | mb_klein 30/0 |
| 11 | Programm: Meldungen von Rig und IK nannten Frames der GLA („IK ab Frame 30300“), die Zeitleiste zählt ab 0 (Max) bzw. 1 (Blender) | `modellZeitBildZahl()` für 13 Meldungsstellen (Rig, IK, Kurven, Zeitleiste) | mb_klein 4 FEHLER | mb_klein 30/0 |
| 12 | Test: der zweite Rechtsklick zählt nur innerhalb 250 ms Programmzeit als Doppelklick; unter Last lagen die zwei Klicks weiter auseinander | TESTTEMPO-Patch eingebaut: die zwei Rechtsklicks in Simulationszeit | `BHED_TEST_BREMSE=400`: „Doppel-M2 in der Kamera“ + 3 Folgezeilen | dieselbe Bremse: Doppel-M2 grün |

## Offen

- Unter `BHED_TEST_BREMSE=400` bleibt in ke_grund „Zweimal: der zweite um max( 8, Raster ) weiter“ rot - vorher wie nachher, ohne Bremse
  grün. Die zweite info_notnull liegt bei 144 statt 136 (der Erzeugungszähler eins zu weit; vermutlich wirkt ein Strg+Klick unter der
  Bremse zweimal). Nicht Teil des Doppel-M2-Patches, nicht weiter verfolgt.
- „Edit 2nd“ steht nicht in „Alle Aktionen suchen“ (ein Knopf in einem Dialog; der Dialog selbst ist über Edit > Shortcuts erreichbar).

## Selbsttest und Läufe

Alle Läufe mit `tools/testrunde.py --leerlauf`, höchstens zwei jkstudio gleichzeitig (Lastregel 12.11.). Belege: `Y:\werkzeuge\klein\laeufe\letzter_<runde>_<lauf>.txt`.

| Bau | Stand | Läufe |
|---|---|---|
| bau3 | 64a0b747a (Tests, altes Programm) | r1 (mb_klein 11/19, mb_kleinneustart 3/2), r1t (reiter rot), p12rot, p3b80/p3b200 (rot) |
| bau5 | 0a4259959 (Basis 73c18113f, ohne Punkt 7) | g1: ke_grund 752/11 (nur Punkt 7 rot), reiter 94/0, mb_ap8 105/0, mb_kopf 117/0 |
| bau6 | rebased auf 499527165, mit Punkt 7 | g2: ke_grund 760/0, ke_bau 622/0, mb_kopf 117/0, mb_ap13k 66/0; ctest karte_befehle, rig_vorlagen_aktuell, testrunde_ports grün |
| bau7-9 | rebased auf 133c47078 (FIX612C) | g3: mb_kleinneustart 9/0, mb_kopf 117/0; p4brot (Rot-Probe 4b Modellieren); g4: mb_klein 30/0, mb_kleinneustart 9/0; p3g80/p3g200: mb_ap8 105/0 |
| bau11 | rebased auf fe0cbece7 | g5: mb_klein 30/0, mb_kleinneustart 9/0, mb_kopf 117/0, ke_grund 760/0, ke_bau 622/0; ctest karte_befehle, rig_vorlagen_aktuell, testrunde_ports, handbuch* grün |
| bau12 | rebased auf 1465cb6a3 | g6: mb_klein 30/0, mb_kleinneustart 9/0, mb_kopf 117/0, ke_grund 760/0, ke_bau 622/0; ctest karte_befehle, rig_vorlagen_aktuell, testrunde_ports, handbuch* grün |
| bau13 | rebased auf 61ec20b03 (Abgabestand) | g7: mb_klein 30/0, mb_kleinneustart 9/0, mb_kopf 117/0, mb_ap8 105/0, ke_grund 760/0, ke_bau 622/0; ctest karte_befehle, rig_vorlagen_aktuell, testrunde_ports, handbuch* grün |

Alle Bauten ohne Warnungen (/W4 /WX). Nicht gelaufen: der volle karteneditor (die Teile ke_grund und ke_bau decken die
geänderten Gruppen numpad, entitaeten, stufe3 ab), reiter nach dem Rebase (tour.cpp unverändert, grün auf bau5), Sprachläufe.

## Fotos (angesehen)

- `numpad_zweite_taste_menue` (ke_grund g2): Rechtsklick-Menü der Zeile mit Edit, Edit 2nd, Clear, Reset; Knopf Edit 2nd unter Reset All, passt in 80 Punkte.
- `klein_gizmo_kamera` (mb_klein): vierter Knopf blass, Tooltip „View Camera / The Model tab has no scene camera.“
- `klein_about_schrift24` (1920x1080-Fenster, Schrift 24; Bild 1898 breit = Innenmaß dieses Fensters, Absicht des Tests): About passt, die Liste rollt, Close/Changelog/Guide ganz zu sehen.
- `klein_home_zuruecksetzen` (mb_kleinneustart): Rechtsklick des ViewCube mit Home, Set current view as Home, Reset Home, Orthographic.

## Handbuch

- `inhalt_mbkopf.py`: Absatz ViewCube - ein gesetztes Home bleibt bei der Figur, **Home zurücksetzen**; Absatz Navigations-Gizmo - der
  Kamera-Knopf (blass). Bild `h_mbk_blender` in sechs Sprachen neu.
- `inhalt_klein.py` (neu, eingehängt in `inhalt_karteneditor.py` nach dem Absatz Ziffernblock): Absatz „Zweite Taste“. Bild
  `h_karteneditor_tasten` in sechs Sprachen neu (mit Edit 2nd).
- Texte der Oberfläche: `tools/i18n_klein.py` (MkHomeSetzenHint, MkHomeZurueck, MkHomeZurueckHint, KeEditZweit, KeEditZweitHint).

## Schlussrunde des Zweigs

- Bau ohne Warnungen (inkrementell, `--parallel 2`, Leerlauf).
- ctest: `karte_befehle`, `rig_vorlagen_aktuell`, `testrunde_ports`, `handbuch*` grün.
- Selbsttest: mb_klein 30/0, mb_kleinneustart 9/0, ke_grund 760/0, ke_bau 622/0, mb_kopf 117/0, mb_ap13k 66/0, mb_ap8 105/0 (ohne und mit
  Bremse 80/200), reiter 94/0, Handbuch-Läufe mbkopf und karteneditor je Sprache (12) grün.
- Lints: i18n, i18n_erzeugt, settings, gapp, imgui_alpha, imgui_context, feldinit, headers, tabellenid, sources, layout, endtable,
  doppelanzeige, hoehe, resize ok; portability 101 und pruefcodes 11 wie karte1.
- Nicht gelaufen (bewusst): volle Runde, Sprachläufe, karteneditor ganz (Hauptchat nach dem Zusammenführen).

## Geänderte Dateien (für die Nachbarn)

gui/app.cpp, gui/app_karte_bau.cpp, gui/app_karte_dialoge.cpp, gui/karteneditor.h, gui/app_modelltab_ansicht.cpp, gui/app_modelltab_blender.cpp,
gui/app_modelltab_ik.cpp, gui/app_modelltab_kopf.cpp, gui/app_modelltab_kurven.cpp, gui/app_modelltab_rig.cpp, gui/app_modelltab_zeit.cpp,
gui/modelltab.h (Block am Ende, zwei Kommentare), gui/wof/app_modelltab_mesh.cpp, gui/wof/app_modelltab_viewcube.cpp, gui/tour.cpp/.h,
gui/selbsttest.cpp (je eine Zeile), gui/selbsttest_{klein (neu),ap8,karte1bis2,karte3bis5,mbkopf,numpad,programm}.inc, include/bhed/settings.h,
src/settings.cpp, include/bhed/karte/befehle.h, src/karte/befehle.cpp, tests/kartebefehle.cpp, tools/i18n_klein.py (neu), tools/gen_i18n.py,
tools/i18n_sprache_es/cs.py, tools/handbuch/{gen_handbuch,inhalt_klein (neu),inhalt_mbkopf,inhalt_karteneditor,inhalt_es,inhalt_cs}.py,
tools/testrunde.py, tools/test_betroffen.py, tools/modell/rig_vorlagen_gen.py, CMakeLists.txt (Block am Ende).

## Changelog

## English

- Model tab: Set current view as Home stays with the figure, also after a restart; Reset Home in the right-click menu of the ViewCube and in Search all actions brings back the start view.
- Model tab: each modelling tool remembers its own type - Inset no longer opens with the type last used for Extrude.
- Model tab: frozen surfaces can no longer be picked through vertices, edges, triangles or polygons, neither by click nor by rectangle; the click goes through to what lies behind.
- Model tab: the navigation gizmo like Blender has the camera button; it is pale because the Model tab has no scene camera.
- Model tab: messages of the control rig and IK count frames like the timeline (from 0 like 3ds Max, from 1 like Blender).
- Map editor: the second key of a command (by default the numpad key of the grid commands) can be set on its own - Edit 2nd in Edit > Shortcuts and in the right-click menu of a row.
- The tutorial "Compile and fix errors" keeps the Compile button highlighted while it shows Done or Failed.
- The About window fits the program window at large font sizes; the list scrolls and the buttons stay visible.

## Deutsch

- Model-Tab: „Aktuelle Ansicht als Home setzen“ bleibt bei der Figur, auch über den Neustart; „Home zurücksetzen“ im Rechtsklick des ViewCube und in „Alle Aktionen suchen“ holt die Startansicht zurück.
- Model-Tab: jedes Werkzeug des Modellierens merkt sich seine eigene Art - Einfügen öffnet nicht mehr mit der zuletzt beim Extrudieren gewählten.
- Model-Tab: eingefrorene Flächen lassen sich auch über Ecken, Kanten, Dreiecke und Polygone nicht mehr wählen, weder per Klick noch per Rahmen; der Klick geht zu dem, was dahinter liegt.
- Model-Tab: das Navigations-Gizmo wie Blender hat den Kamera-Knopf; er ist blass, weil der Model-Tab keine Szenenkamera hat.
- Model-Tab: Meldungen von Control Rig und IK zählen Bilder wie die Zeitleiste (wie 3ds Max ab 0, wie Blender ab 1).
- Karten-Editor: die zweite Taste eines Befehls (ab Werk die Ziffernblock-Taste der Raster-Befehle) lässt sich einzeln belegen - „2. Taste“ in Edit > Shortcuts und im Rechtsklick einer Zeile.
- Das Tutorial „Compile and fix errors“ hebt den Knopf Compile weiter hervor, während er „Done“ oder „Failed“ zeigt.
- Das Fenster About passt bei großer Schrift ins Programmfenster; die Liste rollt, die Knöpfe bleiben sichtbar.

## 中文

- 模型选项卡: “将当前视图设为 Home”会随角色保留, 重新启动后仍然有效; ViewCube 右键菜单和“搜索所有操作”中的“重置 Home”可恢复起始视图。
- 模型选项卡: 每个建模工具都会记住自己的类型——“插入”不再使用上次“挤出”所选的类型打开。
- 模型选项卡: 冻结的面不能再通过顶点、边、三角形或多边形选中, 无论单击还是框选; 单击会作用于其后方的内容。
- 模型选项卡: 像 Blender 时的导航小部件增加了摄像机按钮; 由于模型选项卡没有场景摄像机, 它呈灰色。
- 模型选项卡: 控制绑定和 IK 的消息按时间轴计数帧 (像 3ds Max 时从 0 开始, 像 Blender 时从 1 开始)。
- 地图编辑器: 可以单独设置命令的第二个按键 (默认是网格命令的小键盘按键)——位于 Edit > Shortcuts 中的“第二键”和某一行的右键菜单。
- 教程“Compile and fix errors”在 Compile 按钮显示 Done 或 Failed 时仍会高亮该按钮。
- 字体较大时“关于”窗口也能完整显示在程序窗口内; 列表可滚动, 按钮保持可见。

## 日本語

- モデルタブ: 「現在のビューをホームに設定」はフィギュアごとに保持され、再起動後も残ります。ViewCube の右クリックメニューと「すべてのアクションを検索」の「ホームをリセット」で開始ビューに戻ります。
- モデルタブ: モデリングの各ツールがそれぞれのタイプを記憶します。インセットが押し出しで最後に選んだタイプで開くことはなくなりました。
- モデルタブ: フリーズしたサーフェスは頂点、辺、三角形、ポリゴンからも、クリックでも矩形でも選択できなくなりました。クリックは背後にあるものに届きます。
- モデルタブ: Blender 風のナビゲーションギズモにカメラボタンが加わりました。モデルタブにはシーンカメラがないため淡色です。
- モデルタブ: コントロールリグと IK のメッセージはタイムラインと同じ数え方でフレームを示します (3ds Max 風は 0 から、Blender 風は 1 から)。
- マップエディター: コマンドの 2 つ目のキー (既定ではグリッドコマンドのテンキー) を個別に割り当てられます。Edit > Shortcuts の「2 つ目」と行の右クリックメニューから。
- チュートリアル「Compile and fix errors」は、Compile ボタンが Done や Failed を表示している間もボタンを強調表示します。
- 文字サイズが大きくても About ウィンドウがプログラムのウィンドウに収まります。リストはスクロールし、ボタンは見えたままです。

## Español

- Pestaña Model: «Establecer la vista actual como Inicio» se conserva con la figura, también tras reiniciar; «Restablecer Inicio» en el menú contextual del ViewCube y en Buscar todas las acciones recupera la vista inicial.
- Pestaña Model: cada herramienta de modelado recuerda su propio tipo; Insertar ya no se abre con el último tipo elegido para Extruir.
- Pestaña Model: las superficies congeladas ya no se pueden seleccionar mediante vértices, aristas, triángulos o polígonos, ni con clic ni con rectángulo; el clic llega a lo que hay detrás.
- Pestaña Model: el gizmo de navegación como Blender tiene el botón de cámara; aparece atenuado porque la pestaña Model no tiene cámara de escena.
- Pestaña Model: los mensajes del control rig y de IK cuentan los fotogramas como la línea de tiempo (desde 0 como 3ds Max, desde 1 como Blender).
- Editor de mapas: la segunda tecla de una orden (de fábrica, la del teclado numérico en las órdenes de rejilla) se puede asignar por separado: «2.ª tecla» en Edit > Shortcuts y en el menú contextual de una fila.
- El tutorial «Compile and fix errors» mantiene resaltado el botón Compile mientras muestra Done o Failed.
- La ventana Acerca de cabe en la ventana del programa con fuentes grandes; la lista se desplaza y los botones siguen visibles.

## Čeština

- Karta Model: „Nastavit aktuální pohled jako Domů“ zůstává u postavy, i po restartu; „Obnovit Domů“ v místní nabídce ViewCube a ve Vyhledat všechny akce vrátí výchozí pohled.
- Karta Model: každý modelovací nástroj si pamatuje svůj vlastní typ - Vsadit se už neotevírá s typem naposledy zvoleným pro Vysunout.
- Karta Model: zmrazené plochy už nelze vybrat přes vrcholy, hrany, trojúhelníky ani polygony, ani kliknutím, ani obdélníkem; kliknutí dopadne na to, co leží za nimi.
- Karta Model: navigační gizmo jako Blender má tlačítko kamery; je bledé, protože karta Model nemá kameru scény.
- Karta Model: zprávy control rigu a IK počítají snímky jako časová osa (jako 3ds Max od 0, jako Blender od 1).
- Editor map: druhou klávesu příkazu (z výroby klávesu numerického bloku u příkazů mřížky) lze nastavit samostatně - „2. klávesa“ v Edit > Shortcuts a v místní nabídce řádku.
- Tutoriál „Compile and fix errors“ ponechá tlačítko Compile zvýrazněné, i když ukazuje Done nebo Failed.
- Okno O aplikaci se při velkém písmu vejde do okna programu; seznam se posouvá a tlačítka zůstávají vidět.
