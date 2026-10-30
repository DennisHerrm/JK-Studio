# NOTIZEN reste2 - Fehlerliste Karte/Modell vor rc607

Zweig `reste2` (Worktree `Y:\hilfe`, ab karte1 0a544a42 mit reste1 und modell1), Build
`Y:/hilfe/build-hilfe`. Laeufe in `Y:\rs2\<name>` (ausserhalb des Worktrees, Idle-Prioritaet,
Skript `SP\rs2_lauf.sh`).

## Stand

- Fertig: 1 (Stufe 8 "Notify when a build is done"), 3 (Absturz an der Leck-Linie), 4 (Decals wie im
  Spiel).
- In Arbeit: 2, 5-8.

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

## English

## Deutsch

## 中文

## 日本語
