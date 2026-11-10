# Notizen MBKOPF: Kopfzeile der Ansicht bedienbar, Anzeige-Optionen dort, ViewCube/Gizmo, Figur von vorne (10.11.2026, `Y:\p_mbkopf`, Zweig `p_mbkopf`, Basis a00f6cf90)

Auftrag `auftraege/auftrag_mbkopf_1011.md` (Teile A, B, C mit Punkt 4a, 4a2, 4b). Drei Agenten nacheinander: der erste (Opus)
schrieb den Code ohne Bau, der zweite (Fable) hat ihn gebaut, geprüft, berichtigt und getestet, der dritte (Opus) hat die
Schlussrunde gemacht, dabei zwei Fehler gefunden und behoben, die Handbuchbilder aufgenommen und die Commits zusammengefasst.

## Aufbau

- `gui/app_modelltab_kopf.cpp`: Katalog der 19 Anzeige-Schalter (Ort in 3ds Max / Blender, Taste, Tooltip, `geht()` mit
  Grund), die Menüs, die Felder der Kopfzeile (`InvisibleButton`: Hervorhebung, Tooltip, Klick links und rechts), das
  Rechtsklick-Menü (`modellAnzeigeKontext`), die Suche (`modellKopfAktionenSuche`).
- `gui/wof/app_modelltab_viewcube.cpp` (BSD-3): der ViewCube ist eine Portierung von WhiteoutDex
  `src/native_sources/WhiteoutDexRenderer/max_viewcube.h` (Stand 09.11., shanks Maßstab: „genau so“; es ersetzt dort
  WhiteoutFlakes `imgui_viewcube.h`): 26 Ziele (3 × 3 Teile je Fläche), Kompassring, Home-Flug, Pfeile, Rollpfeile,
  Rechtsklick-Menü, Durchsichtigkeit; das Navigations-Gizmo wie Blender ist eigene Zeichnung.
- `gui/app_modelltab_ansicht.cpp`: `AnsichtArt::Orthographisch`, Schattierung je Ansicht (`ModellAnsicht::sch`), weicher
  Wechsel der Blickrichtung (`modellBlickSetzen`, 200 ms wie Blender `smooth_viewtx`, Glättung 3s²−2s³ wie
  `view3d_smoothview_apply`), `modellStartansicht`, Hinweis und Statistik in der Überlagerung (auch im Foto).
  `umformen()` und `gizmoZiehen` sind nicht angefasst (MBVERS).

## Entscheidungen

- **Je Ansicht** (wie 3ds Max: Shading-Menü, G und 7 wirken auf die aktive Ansicht; Blender: Viewport Shading je
  3D-Ansicht): Drahtgitter, Kanten über Flächen, zweiseitig, weiße Texturen, Raster, Statistik.
- **Für alle Ansichten** bleiben Knochen, Flossen, Gewichte, Normalen, Tags, Ursprung, Hüllkasten, Boden,
  Auswahlklammer, Riss-Finder, Nummern, Höhenlineal, Hintergrund, Silhouette: Eigenschaften der Figur bzw. des Programms
  (3ds Max: Objekt-Anzeigeeigenschaften gelten in allen Ansichten). Das Sichtfeld gehört der einen Perspektivkamera
  (`modelCam`), die sich alle perspektivischen Ansichten teilen.
- **Schalter, die eine gewählte Fläche brauchen** (Ecken-/Dreiecksnummern, Riss-Finder): ohne Auswahl steht gelb in der
  Ansicht (und im Foto), was fehlt. Die Auswahlklammer zeigt ohne Auswahl nichts - sie umschließt die Auswahl (wie 3ds Max).
- **Knochenflossen** sind ohne Knochen und im Blender-Modus ausgegraut, der Tooltip nennt den Grund (Blender zeigt
  Oktaeder ohne Flossen) - statt still wirkungslos.
- **ViewCube wie 3ds Max / wie `max_viewcube.h`**: er dreht die Ansicht, die Projektion bleibt. In der Perspektive schaut
  man danach perspektivisch von der Fläche, Kante oder Ecke; in einer Orthoansicht gibt eine Fläche ihre Achsenansicht,
  Kante und Ecke die freie Ansicht „Orthographisch“ (U). Der erste Entwurf schaltete jede Fläche auf die Orthoansicht -
  berichtigt. Der Klick wirkt auf das Ziel, das beim Drücken unter der Maus lag (wie die Vorlage); Dauer 0,28 s
  (Vorlage), Home 0,5 s mit Abstand (geometrisch) und Drehpunkt (geradlinig), am Ende genau die Home-Ansicht.
- **Abweichungen von der Vorlage, jeweils begründet**: (1) je Ansicht ein Würfel (dort einer), Maße mit der
  Schriftgröße (dort feste Bildpunkte); (2) genau senkrecht bei Oben/Unten (dort 1,55 rad = 88,8°, weil ihre Kamera
  nicht senkrecht schauen kann); (3) die Unterseite dreht wie die vorhandene Ansicht [Unten] (Vorne am unteren Bildrand),
  damit Perspektive und Orthoansicht gleich stehen; (4) der Ring folgt der Maus auch von unten gesehen; (5) Tooltips an
  jedem Teil (dort nur Home); (6) „Aktuelle Ansicht als Home setzen“ gilt bis zur nächsten Figur - in der Vorlage ein
  statischer Zustand bis zum Programmende, also auch dort nicht über einen Neustart; (7) Home ist eine Perspektive: aus
  einer Orthoansicht wechselt Home zuerst zur Perspektive.
- **Kompass**: Norden liegt wie in 3ds Max und der Vorlage hinter der Fläche Vorne (Welt +Y), S steht unter Vorne: wer
  der Figur ins Gesicht schaut, schaut nach Norden. Belegt im Selbsttest: Klick auf S dreht auf das Gesicht (Skelett).
  „Winkel für Norden“ dreht im Uhrzeigersinn von oben (180 legt N an die Vorderseite der Figur). Im Blender-Modus
  kein Kompass (Blender hat keinen).
- **Gizmo wie Blender**: Klick auf eine Kugel gibt die Achsenansicht (orthographisch, Auto Perspective), erneuter Klick die
  Gegenseite, Ziehen kreist und macht aus einer Achsenansicht die Perspektive. Der Kamera-Knopf fehlt: der Model-Tab hat
  keine Szenenkamera.
- **Vorne** ist die Seite −Y: eine Figur schaut in ihrer .glm nach −Y (die Engine dreht sie beim Zeichnen). Der Test
  liest das nicht von einer Achse ab, sondern misst am Skelett: die Augenknochen `leye`/`reye` liegen vor dem `cranium`.
  Links ist −X (wie „Left“ in 3ds Max: man sieht die rechte Körperseite, die Figur schaut im Bild nach rechts).
- **Zoom Grenzen (Z) behält die Richtung** (wie 3ds Max), **Startansicht/Home und „Ansicht zurücksetzen“** drehen nach vorne
  und passen auf die Figur in der Pose ein. Blender „Alles zeigen“ (Pos1) behält die Richtung wie `view3d.view_all`.
- 3ds Max kennt kein „Ansicht drehen in 15-Grad-Schritten“ (nur Alt+Mitteltaste und den ViewCube) - nicht gebaut.

## Abweichung vom Entwurf 04.11. (für MBENTWURF)

Bewusst, „shank 10.11.“: die 18 Anzeige-Schalter stehen nicht mehr im rechten Feld (Gruppe „Anzeige“), sondern in den
Menüs der Kopfzeile jeder Ansicht ([+], [Schattierung]; Blender: Overlays, Viewport Shading). Das rechte Feld zeigt
einmal den Hinweis auf den neuen Ort. Die Skin-Liste ist nicht angefasst.

## Ergebnisse (rot → grün)

Rot: der alte Stand a00f6cf90 mit nur dem Test `selbsttest_mbkopf_rot.inc` gebaut (exe `build/rot_exe`), Lauf
`BHED_MODELL_NUR=kopfrot`: **4 OK, 17 FEHLER**. Grün: derselbe Test auf dem neuen Stand **22 OK, 0 FEHLER**
(eine Prüfzeile kam nach dem Rot-Lauf dazu: „Esc schließt das Menü“, siehe unten).

| Fehler von shank | alt (rot) | neu (grün) |
|---|---|---|
| „von der Seite“: Gesicht zur Kamera (Kosinus Schädel → Augen gegen die Blickrichtung), Sturmtruppler und Kyle | −0,000 (von der Seite) | 1,000; linkes Auge 33 Punkte rechts vom rechten |
| „rechts aus der Mitte“: Rahmen um alle Ecken der Figur (Anteile der Ansicht) | x 0,439..0,529, y 0,457..1,143 (unten abgeschnitten) | x 0,224..0,776, y 0,074..0,943 |
| dasselbe in den vier Ansichten (Alt+W), zurück, nach „Ansicht zurücksetzen“ | wie oben | wie oben |
| Kopfzeile: Klick auf [+], [Perspektive] | kein Menü | Menü offen |
| 4a: Schalter ohne Wirkung (Ruhe und Pose, nichts gewählt) | Riss-Finder, Ecken-Nummern, Dreiecks-Nummern (je 0 Bildpunkte) | keiner (gelber Hinweis in der Ansicht) |
| 4a mit gewähltem torso | keiner | keiner |
| ViewCube: Klick auf die Kante Vorne/Rechts | Blick bleibt −1 0 0 | Perspektive von vorne rechts |
| Perspektive ↔ Orthographisch mit Blick auf das Gesicht | Ansicht Hinten, Gesicht −1,000 (Rücken) | Ansicht Vorne, Gesicht 1,000 |

- Der Rechtsklick auf [Standard] öffnete bis rc611 das Rechtsklick-Menü der Ansicht; die Prüfzeile „sein eigenes Menü“
  kam erst nach dem Rot-Lauf dazu (im Rot-Lauf stand dort „öffnet ein Menü“ und war deshalb grün). Nicht noch einmal
  gegen den alten Stand gelaufen.
- **Vom Rot-Test gefunden**: ein per Rechtsklick geöffnetes Menü der Kopfzeile blieb nach Esc offen und verschluckte
  den nächsten Klick (der Klick auf den ViewCube drehte nicht: „offene Popups 1“). Die Menüs schließen jetzt
  ausdrücklich auf Esc (wie die Suche); danach „offene Popups 0“ und der Klick dreht.
- **Vom Rot-Test gefunden**: `modellPerspektiveUmschalten` (Ziffernblock 5, Knopf des Gizmos) hatte seit REST M9 Vorne
  und Hinten vertauscht; mit der neuen Startansicht zeigte das Umschalten den Rücken. Jetzt aus `ansichtAchsen`.
- **Beim Prüfen gefunden**: erneutes Laden derselben Figur blieb im ersten Entwurf von der Seite (die Ladewege setzten
  Gier 180, nur ein neuer Pfad passte ein). Beide Ladewege rufen jetzt `modellAnsichtEinpassen()`.

Neue Teile nach `max_viewcube.h` (Kompass, Pfeile, Rollpfeile, Home-Flug, Home setzen, Rechtsklick-Menü,
Durchsichtigkeit): rot auf dem ersten Würfel mit Platzhaltern der Prüf-Schnittstelle - `kopf` **91 OK, 35 FEHLER**
(alle 35 in diesen Teilen); grün auf der Portierung - `kopf` **111 OK, 0 FEHLER**. Gemessen u. a.: Deckkraft 0,55
ohne und 1,0 mit Maus; Klick auf N/O/S/W gibt Gier 270/180/90/0 bei unveränderter Neigung 75; Ziehen am Ring ändert nur
die Gier; Norden 90° legt N nach +X; Home ist kurz nach dem Klick unterwegs und endet auf 0,001 genau auf der
Startansicht bzw. dem gesetzten Home; Rollpfeil rechts −90°, zweimal links +90°, Klick auf die Fläche wieder 0;
Klick auf S dreht auf das Gesicht (Skelett); in der Ansicht Oben ist der Ring ein Kreis mit N oben.

`kopf` prüft außerdem: Tooltip an jedem Feld, Menüs per Klick links und rechts, Drahtgitter nur in einer der vier
Ansichten (Bild der anderen unverändert), F3 gilt weiter, rechtes Feld ohne Gruppe „Anzeige“ mit einmaligem Hinweis,
Rechtsklick-Menü, Suche, 4a über den Katalog (19 Schalter × Max/Blender × Ruhe/Pose × ohne/mit Auswahl, dazu jeder
Bereich), Statistik unten links und Hinweis oben (kein Bildpunkt anderswo), Nummern an der posierten Ecke, die Ansichten
Vorne und Links gegen das Skelett, ViewCube in Perspektive und Orthoansicht, Blender-Kopfzeile, Gizmo (Kugeln,
Gegenseite, Ziehen, Zoom, Verschieben, Persp/Ortho), Ziffernblock 4/Umschalt+4/9/Strg+1, Umschalt+Z, Strg+`.

Bestehende Tests, die von der alten Startansicht abhingen: `funktionen` (der Rahmen über die Ansicht begann auf dem
jetzt klickbaren Feld [+]; „Einpassen“ erwartete die Kamera bei +X; der Zug am Y-Griff dreht dafür auf die Seite),
`mb2` und `blender` (Mal- und Klickstellen von der Seite ausgemessen: `mb1::ladenSeite` stellt dort die Startansicht
bis rc611 her). `umformen()` und `gizmoZiehen` sind unverändert.

**Schlussrunde (dritter Agent): zwei Fehler, die erst dort auffielen**

- `blender` blieb nach dem Commit mit `ladenSeite` rot (**64 OK, 3 FEHLER**; Basis a00f6cf90 **67/0**, gleich gebaut und
  gleichzeitig gelaufen): nicht die Weights (die waren behoben), sondern Alt+Klick / Strg+Alt+Klick / Alt+Klick auf Vielecke
  im neuen Requisit. Gemessen: Klickpunkt y −1060 (über der Ansicht), halbe Höhe der Ansicht Vorne 6,0, Abstand 8,0. Ursache
  im Programm: die neue Startansicht passte auch ein **leeres Dokument** (neues Requisit) ein - auf den Mindestabstand 8 vor
  dem leeren Ursprung; der erste Kasten (64 Einheiten) ragte weit über den Rand. Bis rc611 blieb die Kamera bei einem neuen
  Requisit stehen. Jetzt: ein Dokument ohne gezeichnete Fläche und ohne Ausdehnung behält die Kamera (wie bis rc611),
  `drawModellArbeitsflaeche`. Grün: **67/0**, `md` (neues Requisit, Kasten, Modellieren) **86/0**.
- Modus 1 hatte **2634 OK, 1 FEHLER** „Tour-Prüfung: während About offen ist, dunkelt die Tour nichts ab“; die Gruppe
  `reiter` allein auf diesem Zweig **90/1**, auf der Basis **91/0**. Ursache: das About-Fenster ist durch die Lizenzzeile des
  ViewCube (BSD-3 verlangt sie) eine Zeile höher und überdeckt jetzt den Mittelpunkt des Tour-Ziels im Hauptfenster dahinter;
  `tour.cpp` entschied „Ziel liegt im Modal“ allein nach der Lage. Jetzt entscheidet das Fenster des Ziels (das Modal oder
  ein darüber geöffnetes Popup, z. B. eine Klappliste im Modal); nur ein Ziel ohne Element zählt nach der Lage. Gemessen:
  About 124,290-2414,1182, Mitte des Ziels 2302,1047 (überdeckt); mit der Behebung `reiter` **91/0**, Modus 1 **2635/0**.
- Beim Ansehen der Handbuchbilder gefunden (klein, sichtbar, ohne eigenen Rotlauf): (1) die Menüs der Kopfzeile öffneten an
  der Maus und lagen über ihrem eigenen Feld - jetzt klappen sie unter dem Feld auf wie die Viewport Label Menus von
  3ds Max bzw. die Kopfzeile von Blender, Felder am rechten Rand (Overlays, Viewport Shading) richten das Menü an ihrer
  rechten Kante aus; Prüfzeile `kopf` „das Menü … klappt unter seinem Feld auf“ ([+] 554,176 bei Kopfzeile unten 176;
  Overlays rechts bündig bei 1971). (2) Das About war 2290 Punkte breit: `BulletText` bricht nicht um, die Lizenzzeilen
  (UV, Modellieren, ViewCube, Blender, Animationen) liefen in einer Zeile - jetzt Punkt plus umbrochener Text
  (`cjkTextWrapped`), About 949 Punkte breit. (3) Der einmalige Hinweis „Anzeige-Optionen jetzt oben“ stand in den
  Handbuchbildern - der Handbuch-Lauf setzt ihn als gesehen.
- `mb5` im Handbuch-Lauf auf Japanisch und Spanisch rot (158/1 „Pfad ziehen“): von vorne läuft der Gang auf die Kamera zu,
  die Pfadpunkte bei +8 und +9 liegen fast aufeinander, in den schmaleren Ansichten griff der Zug den Punkt bei +9
  (Statuszeile Frame 30302 statt 30301). Der Pfad-Test schaut jetzt von der Seite wie bis rc611 (`mb1::seitenAnsicht`).
- Im Handbuchbild h_mb1_vier (alt wie neu) standen nach Alt+W die Orthoansichten Vorne und Links unten abgeschnitten: sie
  wurden beim Laden eingepasst, als die Pose noch nicht gerechnet war. Neue Prüfzeilen in `kopfrot` (Oben, Vorne, Links):
  rot **23/2** (Vorne/Links Rahmen y 0,432..1,345), jetzt passen die Orthoansichten nach der Pose mit der Startansicht ein:
  grün **25/0** (y 0,045..0,955). Danach liefen alle bedienenden modellblock-Gruppen noch einmal grün (Tabelle).

## Schlussrunde (10.11. nachmittags, dritter Agent)

Bau von 6ad0ff795 0 Fehler, 0 Warnungen; **sauberer Neubau** aller Ziele (clean + ALL_BUILD, 591 s mit /MP6) 0 Fehler,
0 Warnungen; die Folgebauten nach den Behebungen ebenso. Selbsttests parallel (6 + 5 gleichzeitig, ctest -j 10).

| Lauf | Ergebnis |
|---|---|
| ctest | zuerst 244 von 246 (`handbuch_sprachen`: die drei neuen Bilder h_mbk_* fehlten noch), am Ende **245 von 246**; rot nur `q3map2_teilbau`, 2 Fälle „zweiraum -bounce 2, Brush (Detail in Raum B)“ und „Shader (Wand in Raum A)“ - bekannt auf a00f6cf90 (lcb2-Zwischenstand, ein anderer Zweig stellt sie um), dieser Zweig ändert an q3map2 und dem Test nichts |
| modellblock (md) | kopfrot 25/0, kopf 113/0, funktionen 61/0, tasten 43/0, mb2 37/0, blender 67/0, haut 1/0, bild 1/0 (552 Modelle, 549 gleich, 3 wie im Spiel anders), bild2 2/0, mb3 21/0, rundlauf 2/0, rig 26/0, rigrundlauf 4/0, mb5 159/0, kurvenrundlauf 2/0, mb6 37/0, ikrundlauf 2/0, mb7 24/0, mb7textur 1/0, ph 29/0, rt 19/0, rtjka 1/0, pr 15/0, ai 31/0, uv 50/0, uvmd3 3/0, uvrundlauf 1/0, md 86/0, mdkarte 7/0, pt 41/0, ptfix 5/0, ptfix2 5/0, ptfix3 12/0, ptstufen 4/0, pfix 17/0, pfix2 5/0, pfixkarte 5/0 |
| modellblock JKA / JK2 | rundlauf, uvrundlauf, rigrundlauf, kurvenrundlauf, ikrundlauf, bild (JKA 77 Modelle), bild2: alle 0 FEHLER |
| menueeditor | 2787/0 |
| Modus 1 | 2634/1 (Tour/About, siehe oben) → 2635/0 |
| reiter, Karten-Editor hilfe1 / hilfe2 / tutgeo / stufe8c (alles, was die Tour prüft) | 91/0, 59/0, 55/0, 23/0, 35/0; hilfe2 meldet nach der Zusammenfassung beim Beenden „Programm läuft nach No weiter (Exit Radiant)“ - auf a00f6cf90 gleich |
| Lints | zeilengleich mit a00f6cf90 bis auf Zählerstände (msvclink 111 statt 110 Dateien, settings 97/85 statt 92/80 Schlüssel/Felder); efxflags, fremdprogramm, portability, pruefcodes schon auf der Basis mit gleicher Ausgabe beanstandet |

Aufruf-Fehler der ersten Welle, keine Programmfehler: `mb7textur` braucht `BHED_MB7_LEER` (ohne 0/3), `ptfix2` läuft mit
`SPRACHE=ja` (ohne 4/1, steht im Test). `Y:\werkzeuge\mbk\schluss2.sh` ruft beide richtig auf.

## Gegen das Vorbild geprüft / nicht geprüft

- Geprüft gegen den Quelltext der Vorlage `max_viewcube.h` (Zeile für Zeile portiert: Maße, Zonen, Reihenfolge der
  Treffer, Animation, Home, Pfeile, Rollpfeile, Kompass, Menü, Deckkraft); gegen Blender-Quellwerte (200 ms,
  Glättung, 15 Grad). 3ds Max, Blender und WhiteoutDex wurden nicht gestartet (verboten): das Verhalten von Max
  (Projektion bleibt beim ViewCube, Menüpunkte) stammt aus der Vorlage und der Max-Hilfe, nicht aus einem Vergleichslauf.
- Nicht geprüft: Bildzeit vor/nach (keine Messung; ViewCube und Kopfzeile sind wenige Zeichenaufrufe je Ansicht).

## Offen

- **X-Ray** (Blender Alt+Z, 3ds Max Alt+X „See-Through“): nicht gebaut. Geprüft, ob es mit einer kleinen, klar begrenzten
  Änderung in `gui/gpumap_win32.cpp` geht: nein. Der Ghost-Weg zeichnet die Figur einfarbig ohne Schattierung (ein
  Bildpunkt je Ghost über den Stencil) - Blender zeigt im Solid-Modus die schattierte Figur halb durchsichtig (X-Ray Alpha,
  Vorgabe 0,5), 3ds Max See-Through ebenso; das wäre eine Näherung. Echtes X-Ray braucht zusätzlich einen Mischzustand mit
  Faktor im Figurendurchgang (`zeichneFigurIn`), die Auswahl durch die Figur hindurch (`dreieckUnter`, Rahmen - Blender wählt
  mit X-Ray auch Verdecktes), einen Schalter je Ansicht (`ModellSchattierung`), Menüeintrag und Taste, Texte in sechs Sprachen
  und das Handbuch; dazu baut der D3D12-Zweig `zeichneModellAnsicht` auf eine Stapelliste um. Offen bis nach dem D3D12-Umbau.
- Kamera-Knopf des Blender-Gizmos (keine Szenenkamera im Model-Tab).
- „Aktuelle Ansicht als Home setzen“ gilt bis zur nächsten Figur, nicht über einen Neustart (wie die Vorlage).
- Skin-Liste: nicht angefasst (MBENTWURF).
- Handbuchbilder: alle Bilder mit einer Ansicht des Model-Tabs sind in sechs Sprachen neu aufgenommen (35 je Sprache:
  mbkopf 3 neu, mb1a1, mb1a2, mb2, mb3, mb5, mb6, ph ohne h_ph_figur, rt, ai, uv, md ohne h_md_karte, pt, dazu h_ueber mit
  der Lizenzzeile), ohne `BHED_HB_FENSTER`, 2538 breit (h_ueber als Ausschnitt in voller Auflösung). Nicht neu: die Gruppe
  mb7, h_ph_figur (Mission-Tab) und h_md_karte (Karten-Editor) - sie zeigen keine Ansicht des Model-Tabs.
- Beobachtet, nicht geändert: im Gizmo (Blender) liegt die halb durchsichtige Gegenkugel −Y vorne über der beschrifteten
  Kugel Y, die durchscheint; in h_mbk_cube ist N des Kompasses vom Würfel verdeckt (die Bildunterschrift nennt den Ring mit
  N, O, S, W).

## Changelog

## English

- Model tab: the header of a view ([+] [Perspective] [Standard]) can now be operated. Every field lights up under the mouse, has a tooltip and opens its menu with a left or right click, like the viewport label menus of 3ds Max; in Blender mode the header is laid out like Blender's (View on the left, Overlays and Viewport Shading on the right). Each of the four views has its own header.
- Model tab: the display options moved from the right panel into these menus - shading (wireframe, edged faces, two-sided, white textures, vertex weights, silhouette), [+] (grid, floor, origin, bounding box, selection bracket, statistics, height ruler, plain background, field of view) and "Bones and checks" (bones, fins, normals, tags, crack finder, vertex and triangle numbers). Shading, grid and statistics now belong to one view, so one of the four views can show the wireframe alone. The keys stay, the search (X) finds every switch and its tooltip says where it is.
- Model tab: vertex numbers, triangle numbers and the crack finder now say in the view that a surface has to be selected, instead of showing nothing; bone fins are greyed out with the reason when they cannot show.
- Model tab: new ViewCube like the one of 3ds Max (3ds Max mode) at the top right of the view - click one of its 26 faces, edges and corners to look from there, drag it to orbit; a compass ring below it (drag to turn about the vertical axis, click N, E, S or W), arrows to the neighbouring faces and roll arrows when you look straight at a face, a Home button that flies the camera back, a right-click menu (Home, Set current view as Home, Orthographic); half transparent until the mouse is over it. In Blender mode the navigation gizmo takes its place (click an axis, drag to orbit, zoom, move, perspective/orthographic). The view turns smoothly. New free orthographic view (U).
- Model tab: a figure is shown from the front and centred after loading, with Home and with "Reset view" (before: from the side). Switching between perspective and orthographic while looking at the face no longer shows the back. In the four views the top, front and left views show the whole figure (before, front and left were cut off at the bottom).
- Model tab, Blender mode: Numpad 9 (opposite side), Shift+Numpad 4/6 (roll), Shift+Z (wireframe), Ctrl+` (gizmo).
- About window: the licence notes wrap, so the window is no longer wider than a 1920 screen.

## Deutsch

- Model-Tab: Die Kopfzeile einer Ansicht ([+] [Perspektive] [Standard]) lässt sich jetzt bedienen. Jedes Feld leuchtet unter der Maus auf, hat einen Tooltip und öffnet mit Klick links oder rechts sein Menü, wie die Viewport Label Menus von 3ds Max; im Blender-Modus ist die Kopfzeile wie in Blender aufgebaut (links View, rechts Overlays und Viewport Shading). Jede der vier Ansichten hat ihre eigene Kopfzeile.
- Model-Tab: Die Anzeige-Optionen sind aus dem rechten Feld in diese Menüs gewandert - Schattierung (Drahtgitter, Kanten über Flächen, zweiseitig, weiße Texturen, Gewichte, Silhouette), [+] (Raster, Boden, Ursprung, Hüllkasten, Auswahlklammer, Statistik, Höhenlineal, einfarbiger Hintergrund, Blickwinkel) und „Knochen und Prüfen“ (Knochen, Flossen, Normalen, Tags, Riss-Finder, Ecken- und Dreiecksnummern). Schattierung, Raster und Statistik gehören jetzt der einen Ansicht: Eine der vier Ansichten kann allein das Drahtgitter zeigen. Die Tasten bleiben, die Suche (X) findet jeden Schalter, und sein Tooltip nennt den Ort.
- Model-Tab: Ecken-Nummern, Dreiecksnummern und der Riss-Finder sagen jetzt in der Ansicht, dass eine Fläche gewählt sein muss, statt nichts zu zeigen; Knochenflossen sind mit Begründung ausgegraut, wenn sie sich nicht zeigen können.
- Model-Tab: Neu ist der ViewCube wie in 3ds Max (Modus wie 3ds Max) oben rechts in der Ansicht - eine seiner 26 Flächen, Kanten und Ecken anklicken, um von dort zu schauen, am Würfel ziehen zum Kreisen; darunter ein Kompassring (ziehen dreht um die Hochachse, N, O, S oder W anklicken), Pfeile zu den Nachbarflächen und Rollpfeile beim Blick gerade auf eine Fläche, ein Home-Knopf, der die Kamera zurückfliegen lässt, ein Rechtsklick-Menü (Home, Aktuelle Ansicht als Home setzen, Orthographisch); halb durchsichtig, bis die Maus darüber ist. Im Blender-Modus steht dort das Navigations-Gizmo (Achse anklicken, ziehen zum Kreisen, Zoom, Verschieben, Perspektive/Orthographisch). Die Ansicht dreht weich. Neu ist die freie orthographische Ansicht (U).
- Model-Tab: Eine Figur steht nach dem Laden, mit Home und mit „Ansicht zurücksetzen“ von vorne und mittig im Bild (vorher von der Seite). Der Wechsel zwischen Perspektive und Orthographisch mit Blick auf das Gesicht zeigt nicht mehr den Rücken. In den vier Ansichten zeigen Oben, Vorne und Links die ganze Figur (vorher waren Vorne und Links unten abgeschnitten).
- Model-Tab, Blender-Modus: Ziffernblock 9 (Gegenseite), Umschalt+Ziffernblock 4/6 (Rollen), Umschalt+Z (Drahtgitter), Strg+` (Gizmo).
- Über-Fenster: Die Lizenzhinweise brechen um; das Fenster ist nicht mehr breiter als ein 1920er Bildschirm.

## 中文

- 模型选项卡: 视图标题栏 ([+] [透视] [标准]) 现在可以操作。每个字段在鼠标悬停时高亮, 带有提示, 左键或右键单击即可打开菜单, 如同 3ds Max 的视口标签菜单; 在 Blender 模式下, 标题栏的布局与 Blender 相同 (左侧为视图, 右侧为叠加层和视图着色)。四个视图各有自己的标题栏。
- 模型选项卡: 显示选项已从右侧面板移到这些菜单中——着色 (线框、边面、双面、白色纹理、顶点权重、剪影)、[+] (网格、地面、原点、包围盒、选择框、统计、高度标尺、纯色背景、视野) 以及“骨骼与检查” (骨骼、鳍、法线、标记、裂缝查找、顶点和三角形编号)。着色、网格和统计现在属于单个视图, 因此四个视图中的一个可以单独显示线框。按键保持不变, 搜索 (X) 能找到每个开关, 其提示会说明它的位置。
- 模型选项卡: 顶点编号、三角形编号和裂缝查找现在会在视图中提示需要先选中一个面, 而不是什么都不显示; 骨骼鳍无法显示时会变灰并说明原因。
- 模型选项卡: 视图右上角新增与 3ds Max 相同的 ViewCube (3ds Max 模式)——单击 26 个面、边、角之一即可从该处观察, 拖动立方体可环绕; 下方有指南针环 (拖动绕垂直轴旋转, 单击北、东、南、西), 正对某个面时有通向相邻面的箭头和滚动箭头, Home 按钮让相机飞回, 右键菜单 (Home、将当前视图设为 Home、正交); 鼠标不在其上时为半透明。Blender 模式下此处为导航 Gizmo (单击轴、拖动环绕、缩放、平移、透视/正交)。视图平滑旋转。新增自由正交视图 (U)。
- 模型选项卡: 加载后以及使用 Home 和“重置视图”时, 角色正面居中显示 (以前是侧面)。正对面部时在透视与正交之间切换不再显示背面。四视图中的顶视图、前视图和左视图显示完整的角色 (以前前视图和左视图底部被截断)。
- 模型选项卡, Blender 模式: 小键盘 9 (对面)、Shift+小键盘 4/6 (滚动)、Shift+Z (线框)、Ctrl+` (Gizmo)。
- 关于窗口: 许可说明会自动换行, 窗口不再比 1920 宽的屏幕更宽。

## 日本語

- モデルタブ: ビューのヘッダー ([+] [パース] [標準]) を操作できるようになりました。各フィールドはマウスを重ねると明るくなり、ツールチップを表示し、左クリックか右クリックでメニューを開きます (3ds Max のビューポートラベルメニューと同じ)。Blender モードではヘッダーが Blender と同じ配置になります (左にビュー、右にオーバーレイとビューポートシェーディング)。4 つのビューそれぞれにヘッダーがあります。
- モデルタブ: 表示オプションを右パネルからこれらのメニューに移しました - シェーディング (ワイヤーフレーム、エッジ面、両面、白いテクスチャ、頂点ウェイト、シルエット)、[+] (グリッド、床、原点、バウンディングボックス、選択ブラケット、統計、高さ定規、単色の背景、視野角)、「ボーンとチェック」 (ボーン、フィン、法線、タグ、クラック検出、頂点番号と三角形番号)。シェーディング、グリッド、統計はビューごとになり、4 つのビューのうち 1 つだけをワイヤーフレームにできます。キーはそのままで、検索 (X) は各スイッチを見つけ、ツールチップに場所を表示します。
- モデルタブ: 頂点番号、三角形番号、クラック検出は、何も表示しないのではなく、サーフェスの選択が必要なことをビューに表示します。ボーンのフィンは表示できないとき、理由付きでグレー表示になります。
- モデルタブ: ビュー右上に 3ds Max と同じ ViewCube (3ds Max モード) を追加しました - 26 個の面、辺、角のどれかをクリックするとその方向から見て、立方体をドラッグで周回します。下にコンパスリング (ドラッグで垂直軸まわりに回転、北・東・南・西をクリック)、面を正面から見ているときは隣の面への矢印とロール矢印、カメラが戻っていく Home ボタン、右クリックメニュー (Home、現在のビューをホームに設定、平行投影) があり、マウスを重ねるまで半透明です。Blender モードではナビゲーションギズモになります (軸をクリック、ドラッグで周回、ズーム、移動、透視/平行投影)。ビューはなめらかに回転します。自由な平行投影ビュー (U) も追加しました。
- モデルタブ: 読み込み後、Home、「ビューをリセット」で、フィギュアを正面から中央に表示します (以前は横から)。顔を見ている状態でパースと平行投影を切り替えても背中が表示されなくなりました。4 分割ビューの上、前、左のビューはフィギュア全体を表示します (以前は前と左の下が切れていました)。
- モデルタブ、Blender モード: テンキー 9 (反対側)、Shift+テンキー 4/6 (ロール)、Shift+Z (ワイヤーフレーム)、Ctrl+` (ギズモ)。
- バージョン情報ウィンドウ: ライセンス表記が折り返されるようになり、ウィンドウが幅 1920 の画面より広くならなくなりました。

## Español

- Pestaña Modelo: la cabecera de una vista ([+] [Perspectiva] [Estándar]) ya se puede usar. Cada campo se ilumina bajo el ratón, tiene información emergente y abre su menú con clic izquierdo o derecho, como los menús de etiqueta de los visores de 3ds Max; en modo Blender la cabecera se organiza como en Blender (Vista a la izquierda, Superposiciones y Sombreado del visor a la derecha). Cada una de las cuatro vistas tiene su propia cabecera.
- Pestaña Modelo: las opciones de visualización pasaron del panel derecho a estos menús: sombreado (alambre, caras con aristas, doble cara, texturas blancas, pesos de vértices, silueta), [+] (rejilla, suelo, origen, caja envolvente, corchetes de selección, estadísticas, regla de altura, fondo de un color, campo de visión) y «Huesos y comprobaciones» (huesos, aletas, normales, tags, buscar grietas, números de vértices y triángulos). El sombreado, la rejilla y las estadísticas pertenecen ahora a una sola vista: una de las cuatro puede mostrar sola el alambre. Las teclas siguen igual, la búsqueda (X) encuentra cada interruptor y su información emergente dice dónde está.
- Pestaña Modelo: los números de vértices, los números de triángulos y el buscador de grietas indican ahora en la vista que hay que seleccionar una superficie, en lugar de no mostrar nada; las aletas de los huesos aparecen atenuadas con el motivo cuando no pueden mostrarse.
- Pestaña Modelo: nuevo ViewCube como el de 3ds Max (modo 3ds Max) arriba a la derecha de la vista: haz clic en una de sus 26 caras, aristas y esquinas para mirar desde ahí y arrástralo para orbitar; debajo, un anillo de brújula (arrastrar gira sobre el eje vertical, clic en N, E, S u O), flechas a las caras vecinas y flechas de giro al mirar de frente a una cara, un botón Inicio que hace volar la cámara de vuelta y un menú contextual (Inicio, Establecer la vista actual como Inicio, Ortográfica); semitransparente hasta que el ratón está encima. En modo Blender ocupa su lugar el gizmo de navegación (clic en un eje, arrastrar para orbitar, zoom, mover, perspectiva/ortográfica). La vista gira suavemente. Nueva vista ortográfica libre (U).
- Pestaña Modelo: una figura se muestra de frente y centrada tras cargarla, con Inicio y con «Restablecer vista» (antes: de lado). Cambiar entre perspectiva y ortográfica mirando la cara ya no muestra la espalda. En las cuatro vistas, las vistas superior, frontal e izquierda muestran la figura entera (antes la frontal y la izquierda se cortaban por abajo).
- Pestaña Modelo, modo Blender: numérico 9 (lado opuesto), Mayús+numérico 4/6 (girar), Mayús+Z (alambre), Ctrl+` (gizmo).
- Ventana Acerca de: los avisos de licencia se ajustan a varias líneas; la ventana ya no es más ancha que una pantalla de 1920.

## Čeština

- Karta Model: záhlaví pohledu ([+] [Perspektiva] [Standard]) lze nyní ovládat. Každé pole se pod myší rozsvítí, má popisek a levým nebo pravým tlačítkem otevře svou nabídku, jako nabídky popisků pohledů v 3ds Max; v režimu Blender je záhlaví uspořádáno jako v Blenderu (vlevo Pohled, vpravo Překryvy a Stínování pohledu). Každý ze čtyř pohledů má vlastní záhlaví.
- Karta Model: možnosti zobrazení se přesunuly z pravého panelu do těchto nabídek - stínování (drátový model, plochy s hranami, oboustranné, bílé textury, váhy vrcholů, silueta), [+] (mřížka, podlaha, počátek, obalový kvádr, závorky výběru, statistika, výškové pravítko, jednobarevné pozadí, zorné pole) a „Kosti a kontroly“ (kosti, ploutve, normály, tagy, hledání prasklin, čísla vrcholů a trojúhelníků). Stínování, mřížka a statistika nyní patří jednomu pohledu, takže jeden ze čtyř pohledů může sám ukazovat drátový model. Klávesy zůstávají, hledání (X) najde každý přepínač a jeho popisek uvádí, kde je.
- Karta Model: čísla vrcholů, čísla trojúhelníků a hledání prasklin nyní v pohledu oznámí, že je třeba vybrat povrch, místo aby neukázaly nic; ploutve kostí jsou zašedlé s uvedením důvodu, když se nemohou zobrazit.
- Karta Model: nově ViewCube jako v 3ds Max (režim 3ds Max) vpravo nahoře v pohledu - klepnutím na jednu z jeho 26 stěn, hran a rohů se díváte odtud, tažením krychle obíháte; pod ní kompasový kruh (tažení otáčí kolem svislé osy, klepnutí na S, V, J nebo Z), šipky k sousedním stěnám a šipky natočení při pohledu přímo na stěnu, tlačítko Domů, které kameru nechá přeletět zpět, a nabídka pravého tlačítka (Domů, Nastavit aktuální pohled jako Domů, Ortografický); poloprůhledná, dokud nad ní není myš. V režimu Blender je na jejím místě navigační gizmo (klepnutí na osu, tažení pro obíhání, zoom, posun, perspektiva/ortografický). Pohled se otáčí plynule. Nový volný ortografický pohled (U).
- Karta Model: postava je po načtení, po Domů a po „Obnovit pohled“ zobrazena zepředu a uprostřed (dříve z boku). Přepnutí mezi perspektivou a ortografickým pohledem při pohledu na obličej už neukáže záda. Ve čtyřech pohledech ukazují pohledy shora, zepředu a zleva celou postavu (dříve byly pohledy zepředu a zleva dole oříznuté).
- Karta Model, režim Blender: numerická 9 (opačná strana), Shift+numerická 4/6 (natočení), Shift+Z (drátový model), Ctrl+` (gizmo).
- Okno O aplikaci: licenční poznámky se zalamují; okno už není širší než obrazovka 1920.
