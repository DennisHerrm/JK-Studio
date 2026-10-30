# NOTIZEN reste2 - Fehlerliste Karte/Modell vor rc607

Zweig `reste2` (Worktree `Y:\hilfe`, ab karte1 0a544a42 mit reste1 und modell1), Build
`Y:/hilfe/build-hilfe`. Laeufe in `Y:\rs2\<name>` (ausserhalb des Worktrees, Idle-Prioritaet,
Skript `SP\rs2_lauf.sh`).

## Stand

- Fertig (je rot/gruen, committet): 1 Build-Menue hoeher als der Bildschirm, 2 g_figurBilder, 3 Absturz an der
  Leck-Linie, 4 Decals wie im Spiel, 5 Testdaten ausserhalb des Worktrees, 6 GPU-Tabelle in der Sprache des
  Programms, 7 Foto modell_knochen und Menue-Figuren in modellleistung.
- 8 Schlussrunde: siehe unten.

## 1. Stufe 8 allein: "Notify when a build is done"

Nachgestellt: der Befund aus i3ns8/i3ns8alt (29.10.) - "nicht ganz sichtbar (329..858, sichtbar
330..856)" und "Klick kam nicht an (aktiv: -, unter der Maus: -, Fenster -)" - kam in diesem Stand
nicht von selbst (s8rot: 37 OK / 0). Ein Messschritt (nicht eingecheckt) zeigte die Lage: das
Build-Menue hat 34 Eintraege und ist damit fast so hoch wie der Bildschirm (2560 x 1600, Arbeitsflaeche
1528 hoch).
- Stil des Programms (Vorgabe): ImGui begrenzt das Menue auf den Bildschirm und rollt es. Mit 20
  Eintraegen mehr (s8diagp20) genau der alte Befund: der Eintrag liegt unter dem sichtbaren Teil (die
  Meldung nannte nur die waagerechte Lage - es war die senkrechte), die Maus darunter ausserhalb jedes
  Fensters. Ein Mensch dreht das Mausrad - der Test tat es nicht. Ursache im Test.
- Stil wie NRC: das Menue war 1688 hoch (Fenster 4..1692) und lief unten ueber den Bildschirm hinaus;
  "Notify" (1644..1692) und "Im Spiel testen" lagen ausserhalb - fuer einen Menschen unerreichbar (der
  Selbsttest kam nur hin, weil er die Maus auch ausserhalb des Bildschirms setzen kann). Ursache im
  Programm. NRC (Qt 5.15, Fusion) hat keinen Scroller (SH_Menu_Scrollable 0) und bricht ein zu hohes
  Menue in Spalten um (QMenuPrivate::updateActionRects, qmenu.cpp:478-489: passt eine Zeile nicht mehr
  unter column_max_y, beginnt rechts daneben eine neue Spalte).

Behoben:
- Programm (gui/app_karte_qtmenue.cpp): die Menues im Stil von NRC brechen wie QMenu in Spalten um
  (zeileAnfangen/zeileBeenden je Eintrag, Untermenue und Trenner; Breite = Spalten mal
  max_column_width, Hoehe = tiefste Zeile wie sizeHint). Abweichung: column_max_y ist die Hoehe, in der
  ImGui das Menue zeigen kann (GetPopupAllowedExtentRect, ohne Taskleiste) statt des ganzen
  Bildschirms (Windows: UseFullScreenForPopupMenu) - sonst laege die letzte Zeile einer vollen Spalte
  unter der Taskleiste. Untermenues in einer linken Spalte gehen rechts neben dem ganzen Menue auf (ImGui
  meidet das ganze Elternfenster), Qt oeffnet sie neben der Spalte.
- Selbsttest (gui/selbsttest_karte8.inc): so viele Fuelleintraege, wie Zeilen des Programms auf den
  Bildschirm passen (hier 51) - das Menue ist damit auf jedem Bildschirm zu hoch. Stil des Programms:
  Melden liegt unter dem sichtbaren Teil, das Menue rollt; `radBisSichtbar` dreht das Mausrad ueber dem
  Menue, bis der Eintrag zu sehen ist, dann der Klick. Stil wie NRC: mindestens zwei Spalten, jeder
  Eintrag auf dem Bildschirm, der Klick schaltet Melden wieder an.
- rot (s8rot2, Test neu, Programm alt): Stil des Programms gruen (Mausrad), Stil wie NRC "114
  Eintraege, 1 Spalten, 85 ausserhalb von 4..1524 (Fenster 4..5532)".
- gruen (s8gruen2): 21 OK / 0, "85 Eintraege, 3 Spalten, 0 ausserhalb von 4..1524 (Fenster 16..1524)".
- Menues mit einer Spalte bleiben wie bisher (ganze Fensterbreite, WorkRect unveraendert) - die Spalten
  greifen erst, wenn das Menue im letzten Bild mehr als eine hatte oder gerade umbricht.

## 2. g_figurBilder: Bilder alter Haeute

Nachgestellt (Selbsttest modellleistung, `BHED_MODELL1_NUR=haut`): Sturmtruppler mit model_default
gemerkt, dann dreimal model_officer und zurueck. rot (hautrot2): Figurenbilder 6 -> 9, Inhalte 6 -> 7 - die
Eintraege der alten Bilder (Schluessel: Adresse des TextureSet::Tex) blieben stehen, die nach Inhalt
geteilten Grafikbilder fuer immer. Ein erster Versuch mit bloßem Hin und Her blieb gruen (hautrot: 7 -> 7),
weil die Speicherverwaltung dieselben Adressen wieder vergab - die Inhalte der anderen Haut blieben aber.

Behoben: jedes TextureSet::Tex traegt eine `TexWache` (include/bhed/mapview.h), die meldet, wenn das Bild
verschwindet oder neuen Inhalt bekommt (Zerstoeren, Zuweisen; `texWegMelden`). gpumap (gui/gpumap_win32.cpp)
schluesselt die Figurenbilder nach der Adresse der Wache, loest bei der Meldung den Eintrag samt Grafikbild
und den geteilten Inhalt mit seiner letzten Adresse (je Adresse ihr Inhalt, je Inhalt die Zahl der
Adressen); nur der Faden der Grafik raeumt, beim Herunterfahren hoert niemand mehr zu. Das behebt nebenbei
den alten Fall "neues Bild an der Adresse eines alten mit denselben Bildpunkt-Adressen".
- gruen (hautgruen): 6 -> 6, 6 -> 6; die beiden Fotos gleich, und beide Byte fuer Byte gleich denen des alten
  Programms (hautrot2) - das Bild aendert sich nicht.

## 3. Absturz "2D-Ansicht XY auf die Leck-Linie"

Nachgestellt (l3rot: Lauf ohne bhed_q3map2.exe, `BHED_KARTE_NUR=stufe3 BHED_STUFE3_NUR=leck`): "ABSTURZ code
0xC0000005 ... lesend an Adresse 0x4" im Schritt "2D-Ansicht XY auf die Leck-Linie". Der Bau scheitert ohne
q3map2 sofort, die Leck-Linie bleibt leer, und der Schritt rechnet `p.front() + p.back()` auf der leeren
Liste; die Pruefung danach griff auf p[0]/p[1], die "Previous leak spot"-Pruefung auf punktdatei[0]. Das
Programm selbst kommt mit einer leeren Linie zurecht (punktdateiSichtbar nur mit Punkten, Next/Previous
leak spot und File > Pointfile pruefen die Laenge) - ein Nutzer erreicht den Zustand ohne Absturz. Behoben
im Test (gui/selbsttest_karte3bis5.inc): ohne Linie meldet der Schritt den Fehler und laesst die Linie aus,
die Pruefungen fragen die Laenge ab.
- gruen: l3gruen (ohne q3map2) laeuft durch, kein Absturz (25 OK / 56 FEHLER - alles Folgen des fehlenden
  Baus); l3mit (mit q3map2): 55 OK / 0 FEHLER.

## 4. Decals im Karteneditor wie im Spiel

Befund: die Annahme der Liste (der Editor nehme die ganzen Seiten, q3map2 nur den sichtbaren Teil) traf
den Kern nicht. Der Boden unter der Stufe 16 liegt auch in der .bsp in der Huelle der Bodenseite
(visibleHull ist die KONVEXE Huelle der sichtbaren Bruchstuecke - um die Stufe herum sichtbar, also
die ganze Flaeche). Dass er im Spiel keine Marke bekommt, liegt an R_BoxSurfaces_r: es sammelt die Flaechen
aus den Blaettern des BSP, die der Kasten der Marke beruehrt, und ein deckendes Blatt (das Innere der
Stufe) verweist auf keine Flaeche. Liegt der Kasten ganz ueber der Stufe, findet das Spiel den Boden nicht;
der Editor fand mit seinem eigenen Kastenbaum (Median der Flaechen) jede Flaeche, deren Kasten den der Marke
schnitt. Ein erster Schritt mit der Huelle allein blieb darum rot (Stufe 16: Editor 3 Bruchstuecke 1152,
.bsp 2 / 576).

Behoben: `src/karte/sichtbareseiten.inc` (eingebunden in spielkollision.cpp) portiert ProcessWorldModel aus
bhed_q3map2 (NRC 20260114 mit den Aenderungen aus PR #316) Zeile fuer Zeile mit denselben Zahlentypen und
Epsilons: Ebenentabelle (FindFloatPlane mit Hash, SnapPlaneImproved), ParseRawBrush (die Ebenenpunkte, wie
der Editor sie schreibt: ganzzahlig oder "%10.10lf"), RemoveDuplicateBrushPlanes, SetBrushContents,
CreateBrushWindings (HIGH_PRECISION, OFFSET_WINDING_CREATION), AddBrushBevels (der vorhandene Port, jetzt
mit der Ebenentabelle), FinishBrush/MergeOrigin/AdjustBrushesForOrigin/MoveBrushesToWorld, FaceBSP
(SelectSplitPlaneNum mit Blockgroesse, BuildFaceTree_r mit coplanarBrushes), MakeTreePortals,
FilterStructuralBrushesIntoTree (SplitBrush), FloodEntities, FillOutside, CullSides, ClipSidesIntoTree
(visibleHull), der zweite Baum aus den sichtbaren Seiten, FilterDrawsurfsIntoTree (FilterFaceIntoTree mit
WindingOnNodePlane, FilterPatchIntoTree, FilterTrianglesIntoTree, AddReferenceToLeaf) - und daraus die
Welt fuer R_MarkFragments mit genau diesem Baum und den Verweisen seiner Blaetter. misc_model ohne target
kommen ueber echtModellFlaechen hinein (SF_TRIANGLES). Neu im Shaderleser: deformVertexes autosprite,
polygonOffset, q3map_noClip, q3map_indexed, q3map_terrain (CullSides und ClipSidesIntoTree fragen sie).
- Tempo: SelectSplitPlaneNum zaehlt je Ebene einmal und prueft achsparallele Ebenen an den Grenzen der
  Wicklung (der Abstand ist dort genau Koordinate minus dist) - dasselbe Ergebnis. Taris (13684 Pinsel):
  82 s -> 21 s (unter Last); bhed_q3map2 braucht fuer den ganzen BSP-Schritt 43 s. Darum rechnet der Editor
  im Hintergrund (`markenEingabe` schreibt im Hauptfaden ab, `markenWeltBauen` rechnet, abbrechbar),
  sobald Decals gebraucht werden und die Geometrie eine Viertelsekunde ruht; bis dahin gilt die letzte Welt.
- ctest karte_marken (gegen bhed_q3map2): spielraum jetzt ohne Ausnahme (Stufe 16: 2 / 576 wie die .bsp);
  neu markenraum (tools/karte/gen_markenraum.py, dicht, mit Spieler-Start): Boden an der Ostwand endet an
  der Wand (1728 statt 2304 - der Teil unter der Wand und draussen fehlt in der .bsp), Stufe 16, der Boden
  unter einem Detail-Pinsel bekommt ein Stueck (Detail macht kein Blatt deckend), misc_model oben und mit
  Boden daneben, Decke; je Ebene dieselbe Flaeche der ebenen Flaechen wie in der .bsp (14 Ebenen). 29/29.
- Selbsttest reste1 (Decal im Editor) zusaetzlich: die Welt hat Pinsel und Flaechen (Bau im Hintergrund).
- Nicht nachgebaut (benannt): die Verdickung um den Kasten von "deformVertexes move" beim Einsortieren
  (FilterWindingIntoTree_r, shaderInfo minmax), meta/T-Verbindungen/Unterteilung (aendern die Zerlegung der
  Flaechen, nicht ihre Flaeche), die Reihenfolge der Flaechen in den Blaettern (zaehlt nur, wenn mehr als 64
  Flaechen im Kasten liegen).

## 5. Stufe 3/4 ".bsp wie NRCs" ausserhalb des Worktrees

Nachgestellt (s3rot, Laufordner `Y:\rs2\s3rot`, `BHED_STUFE3_NUR=bau`): 113 OK / 1 FEHLER - "tests/karte/q3map2
nicht gefunden (BHED_Q3MAP2_DATEN)". `arbeitskopie()` sucht die Testdaten nur in den Ordnern ueber dem
Laufordner. Behoben: CMake gibt selbsttest.cpp den Quellordner mit (`BHED_QUELLORDNER`, wie die ctests ihre
Daten ueber CMake bekommen); liegt der Laufordner ausserhalb, sucht `arbeitskopie()` dort.
- gruen (s3gruen, derselbe Laufordner ausserhalb): 114 OK / 0 FEHLER ("Lumps 1= 2= 3= 6= 8= leafs= models=
  brushsides=").

## 6. Statistikfenster der Karte: GPU-Tabelle deutsch

Die Tabelle stand mit festen deutschen Texten im Code (gui/app_view3d.cpp drawKartenStatistik, dazu
gpu::zustandsLage). Jetzt `gpuTabellenZeilen()` mit Texten aus tools/i18n_reste2.py (en, de, zh, ja; Zaehltexte
fuer die Aufrufe; die Zustaende ueber gpu::zustandsArt); es/cs zeigen bis zur Uebersetzungsrunde englisch.
Der Bericht der Grafik im Detailprotokoll bleibt deutsch (intern). Selbsttest Modus karte: die Tabelle in
en/zh/ja ohne deutsches Wort, in de mit.
- rot (gpurot: dieselbe Pruefung mit den alten Texten in gpuTabellenZeilen): in allen vier Sprachen "GPU: 0
  Aufrufe, 0 uebersprungen | Zustaende: Tiefe und Keulen | Karte 0 Aufrufe ...".

## 7. Modell-Tab: Foto modell_knochen, Menue-Figuren in modellleistung

Nachgestellt: zwei Laeufe des Modus modell (m7a, m7b) - modell_knochen.bmp verschieden (6807 Bildpunkte im
Bereich des Modells, 689..1090 x 728..1318), das Foto modell_npc davor gleich. Ursache: Zeit - der Drehteller
dreht je Bild um DeltaTime mal 30 Grad (modellDrehteller), wo er nach 0,6 s und den Klicks stehen blieb, hing
am Tempo des Laufs; das Foto folgte gleich danach. Behoben im Test: vor dem Foto der eingepasste Blick
(modellAnsichtEinpassen). gruen: zwei Laeufe (m7c, m7d) Byte fuer Byte gleich, 13 OK / 0.

Menue-Editor-Figuren in modellleistung: der Menue-Editor zaehlt je gezeichneter Figur die Prozessorzeit bis zu
ihrem Bild (Laden, Knochen, zeichneMenueFigur) und die Zahl der Figuren (menueeditor::Stand modellMs,
modellZeichnungen). modellleistung oeffnet ein Menue mit 1 und mit 4 Figuren (stormtrooper, jedi_hf, jedi_hm,
clonetrooper_p2, ITEM_TYPE_MODEL), prueft, dass sie gezeichnet werden, macht ein Foto und misst 120 Bilder:
Bild-CPU, Abstand, Figuren-CPU je Bild, Figuren je Bild, Aufrufe je Bild. Danach zurueck in den Model-Tab
(ein erster Lauf ohne das liess die Ruhe-Pruefungen im Menue-Editor laufen: 11 Bilder in 2,8 s, rot).
- m7voll2 (ohne D3D-Debugschicht, unter Last): 10 OK / 0. 1 Figur: Bild-CPU 0,26 ms (p99 0,53), Figuren-CPU
  0,043 ms je Bild, 72 Aufrufe; 4 Figuren: Bild-CPU 0,28 ms (p99 0,48), Figuren-CPU 0,094 ms je Bild, 163
  Aufrufe. In allen Messbloecken des Model-Tabs 0 Bilder hochgeladen (der Hautwechsel aus Punkt 2 laedt
  nichts nach).

## English
- Map editor, menus like NetRadiant-Custom: a menu taller than the screen (the Build menu with many entries) now wraps into columns like in NetRadiant-Custom - before, its last entries ran past the bottom of the screen and could not be reached.
- Map editor, real-time effects: decals (impact and scorch marks) land where the game puts them. The editor builds the world of the map like q3map2 with its BSP tree: only the visible part of each brush face, the misc_model baked into the world carry marks, and no mark lands on floor hidden under a step. On large maps this takes a few seconds and runs in the background.
- Model tab and figures in the preview: switching skins no longer keeps the images of the old skin in graphics memory.
- Map statistics window: the GPU table is shown in the program language (it was always German).

## Deutsch
- Karten-Editor, Menüs wie NetRadiant-Custom: ein Menü, das höher ist als der Bildschirm (das Build-Menü mit vielen Einträgen), bricht jetzt wie in NetRadiant-Custom in Spalten um - vorher liefen seine letzten Einträge unten über den Bildschirm hinaus und waren nicht zu erreichen.
- Karten-Editor, Echtzeit-Effekte: Decals (Einschläge, Brandflecken) landen dort, wo das Spiel sie hinsetzt. Der Editor baut die Welt der Karte wie q3map2 mit ihrem BSP-Baum: von jeder Brushseite nur der sichtbare Teil, die in die Welt gebackenen misc_model tragen Marken, und auf Boden, der unter einer Stufe verborgen liegt, landet keine. Auf großen Karten dauert das ein paar Sekunden und läuft im Hintergrund.
- Model-Tab und Figuren in der Vorschau: ein Hautwechsel lässt die Bilder der alten Haut nicht mehr im Grafikspeicher.
- Statistikfenster der Karte: die GPU-Tabelle erscheint in der Sprache des Programms (sie war immer deutsch).

## 中文
- 地图编辑器,NetRadiant-Custom 风格的菜单:比屏幕还高的菜单(条目很多的 Build 菜单)现在会像 NetRadiant-Custom 一样分成多列——以前它最后的条目会超出屏幕底部,无法点到。
- 地图编辑器,实时特效:贴花(弹痕、烧痕)会落在游戏中会出现的位置。编辑器像 q3map2 一样用 BSP 树构建地图世界:每个画刷面只取可见部分,烘焙进世界的 misc_model 也会留下痕迹,而台阶下被遮住的地面不会留下痕迹。在大型地图上这需要几秒,并在后台进行。
- 模型标签页和预览中的角色:切换皮肤后,旧皮肤的图像不再留在显存中。
- 地图统计窗口:GPU 表格以程序语言显示(以前始终是德语)。

## 日本語
- マップエディター、NetRadiant-Custom 風のメニュー:画面より高いメニュー(項目の多い Build メニュー)が、NetRadiant-Custom と同じように複数の列に折り返されるようになりました。以前は最後の項目が画面の下にはみ出し、選べませんでした。
- マップエディター、リアルタイムエフェクト:デカール(着弾痕、焦げ跡)がゲームと同じ場所に付くようになりました。エディターは q3map2 と同じように BSP ツリーでマップのワールドを作ります。各ブラシ面の見える部分だけを使い、ワールドに焼き込まれた misc_model にも跡が付き、段差の下に隠れた床には跡が付きません。大きなマップでは数秒かかり、バックグラウンドで計算されます。
- モデルタブとプレビューのキャラクター:スキンを切り替えても、古いスキンの画像がグラフィックメモリに残らなくなりました。
- マップの統計ウィンドウ:GPU の表がプログラムの言語で表示されるようになりました(以前は常にドイツ語でした)。
