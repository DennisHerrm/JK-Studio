# Notizen MBNAV: Navigation in den Ansichten des Model-Tabs 1:1 wie 3ds Max bzw. Blender (11.11.2026, `Y:\p_mbnav`, Zweig `p_mbnav`, Basis 76808a596)

Auftrag `auftraege/auftrag_mbnav_1011.md` (shank 10.11.: „ist das controlling wie in 3ds max in 3ds max modus? mit Kamera ziehen usw.? Das alles
gleich ist? auch den viewcube bedienen?“). Abgleich je Bedienung: `docs/intern/navigation_abgleich.md`.

## Aufbau

- `gui/app_modelltab_nav.cpp` (neu): die ganze Navigation. Eine Ansicht ist dort eine `Sicht` (Art, Winkel, Punkt in der Bildmitte, halbe Bildhöhe in
  dessen Tiefe) - in diesen Größen rechnen Perspektive und Orthoansichten gleich: `schieben` (1:1), `zoomen` (um einen Punkt), `drehen` (um eine
  Weltachse durch einen Punkt). Darauf: die Maus je Bedienmodus (`modellNavMaus`), die Befehle der Tasten und Menüs, die Navigationsmodi von 3ds Max,
  die weichen Wechsel (Blender Smooth View), der Verlauf der Ansichtsänderungen, das Menü der Ansichten (V) und das Kuchenmenü View (`` ` ``).
- `gui/app_modelltab_ansicht.cpp`: der alte Mausblock (in beiden Modi die Max-Belegung) ist durch `modellNavMaus` ersetzt; läuft ein Navigationsmodus,
  gehören ihm linke und rechte Taste (die Ansicht wählt, malt und öffnet dann nichts). Ziffernblock 5, 4/6/8/2 und das Rollen gehen an die neuen
  Befehle. Die Vierer-Ansicht zeichnet im Blender-Modus in Blenders Anordnung. Die Ansicht Unten hat im Blender-Modus Blenders Achsen.
- `gui/app_modelltab.cpp`: die Tastenlisten (3ds Max 18 neue Einträge, Blender 12 neue, 9 geändert); `gui/app_modelltab_kopf.cpp`: Menü „Navigation“
  unter [Ansicht] bzw. View; `gui/wof/app_modelltab_viewcube.cpp`: Ziehen am Würfel/Gizmo und die Knöpfe des Gizmos über dieselben Rechnungen.
- Fünf Einstellungen (gespeichert): `modelOrbit` (0-3, Vorgabe 2), `modelZoomMouseOrtho`, `modelZoomMousePersp` (an), `modelBlenderOrbitSelection`,
  `modelBlenderZoomToMouse` (aus).

## Entscheidungen

- **Maßstab Blender = der Quelltext von Blender 5.0** (Programm nicht gestartet): Turntable-Drehung 0,4 Grad je Bildpunkt (`view_rotate_sensitivity_
  turntable`, geteilt durch den Maßstab der Oberfläche), um die Welt-Hochachse und die Achse aus `viewrotate_apply` (waagerecht quer zum Blick, zur
  Ansichts-X-Achse überblendet), ohne Kappen am Pol, mit `reverse` bei Kopfstand; Auto Perspective nur aus Achsenansichten (`ED_view3d_persp_ensure`);
  Zoom je Raste 1,2 (`view_zoom_apply_step`); Dolly-Formel des Ziehens (`viewzoom_scale_value`); 15 Grad (`pad_rot_angle`); 32 / 25 Bildpunkte
  (`viewpan_invoke_impl`); Einpassen mit `VIEW3D_MARGIN` 1,4; 200 ms weich (`smooth_viewtx`) mit 3s² − 2s³; ein neuer Befehl beendet den laufenden
  Wechsel sofort (`ED_view3d_smooth_view_force_finish`); Einrasten 15 Grad um 26 Richtungen (`viewrotate_apply_snap`).
- **Maßstab 3ds Max = Autodesk-Hilfe und Hotkey-Tabelle 2026** (Programm nicht gestartet). Wo die Hilfe keine Zahl nennt, bleibt der Wert des
  Model-Tabs und der Test prüft nur Richtung und Drehpunkt: Drehung je Bildpunkt (0,4 Grad wie Blender), Zoom je Raste (0,9 Perspektive / 0,85
  Orthoansicht wie bis rc611), Zoom beim Ziehen (0,9 hoch 0,05 je Bildpunkt bzw. 1,01 je Bildpunkt wie bis rc611), Größe des Orbit-Kreises (0,38 der
  kleineren Seite: innen Platz zum Kreisen, außen zum Rollen), Stehenbleiben vor dem Pol (89 / 90 Grad wie bis rc611).
- **Die Szene folgt der Maus** (Trackball, wie die Hilfe den Orbit von 3ds Max beschreibt und wie Blender rechnet): nach rechts ziehen dreht die
  Kamera nach links, nach unten ziehen hebt sie. Bis rc611 war die senkrechte Richtung gespiegelt (Maus, ViewCube, Gizmo).
- **Schieben 1:1**: der Punkt in der Tiefe des Drehpunkts folgt der Maus genau (Blender `viewmove_apply`; in den Orthoansichten war es schon so).
  Bis rc611 schob die Perspektive um Abstand × 0,0015 je Bildpunkt (131 statt 120 Bildpunkte).
- **Orbit-Drehpunkt wie das Flyout von 3ds Max**, Vorgabe Orbit SubObject: mit Auswahl um ihre Mitte, ohne Auswahl um die Ansichtsmitte (Hilfe: „If no
  objects are selected, the function reverts to the standard Orbit function“). Welche Art 3ds Max ab Werk zeigt, ließ sich ohne Start nicht belegen -
  Frage an shank. Orbit Selected und Orbit SubObject sind hier dasselbe (eine Figur): die Mitte der Auswahl der gerade gewählten Auswahlart.
- **Zoom um den Mauspunkt** in 3ds Max ab Werk an (Customize > Mouse: beide „Default=on“), in Blender aus (`USER_ZOOM_TO_MOUSEPOS` nicht gesetzt).
- **Ansichtsänderungen rückgängig** (Umschalt+Z / Umschalt+Y): ein Wächter vergleicht je Bild und Ansicht - beginnt eine Änderung, kommt der Stand
  davor auf den Stapel; ein Zug, ein weicher Wechsel oder Rasten in aufeinanderfolgenden Bildern sind ein Schritt. So zählen auch ViewCube, Gizmo
  und Menüs, ohne dass jede Stelle sich meldet. 64 Schritte je Ansicht (3ds Max nennt keine Zahl); ein neues Modell leert den Verlauf.
- **Navigationsmodi** (Alt+Z, Strg+P, Strg+R, Strg+W): `ModellTab::navModus`; der Name des Modus steht unten in der Ansicht, Rechtsklick oder Esc
  beendet (der Rechtsklick öffnet dann kein Menü). Blender kennt nur den Bereich (Umschalt+B) - ein Rechteck, dann zu Ende.
- **Blender-Ziffernblock**: Orbit Left = die Kamera geht nach links (Gier 90 → 75), Roll Left = die Szene dreht im Bild gegen den Uhrzeigersinn
  (Roll +15), Ziffernblock 5 behält die Richtung. Bis rc611 waren 4/6 und das Rollen gespiegelt und 5 sprang auf die nächste Achsenansicht; der Test
  `kopf` erwartete diese Werte - berichtigt (drei Prüfzeilen) und mit Pausen über die 200 ms hinaus. Der Test `blender` wartet 0,4 statt 0,15 s.
- **Vierer-Ansicht**: 3ds Max Oben / Vorne / Links / Perspektive, Blender Top / Perspektive / Front / Right (Strg+Alt+Q setzt die drei Orthoansichten wie
  `screen.region_quadview`). Die Ansicht behält ihre Nummer, nur der Platz im Bild wechselt.
- **Kollision, nicht still umbelegt**: 3ds Max legt Zoom Extents All auf Strg+Umschalt+Z; im Model-Tab ist das Wiederholen (`modellDateiTasten`, in
  beiden Modi). Der Befehl steht unter [Ansicht] > Navigation und in der Suche; die Taste bleibt Wiederholen - Frage an shank. Die übrigen neuen
  Tasten sind frei: gleiche Tasten der Bereiche (I Inset / Schlüssel, `[` `]` Eltern/Kinder, Strg+R Loop Cut, Umschalt+C Naht) gehören dem jeweils
  anderen Bedienmodus (Liste des Tests `nav` im Protokoll: „Kollision ...“ nennt nur schon vorhandene Paare aus Bereich und allgemeiner Liste).

## Nicht nachgebaut (mit Grund)

- 3ds Max: SteeringWheels (Umschalt+W) und der Durchgangsmodus (Pfeil hoch; W A S D ...), Kameraansicht (C: keine Szenenkamera), schnelleres Schieben
  mit Strg (Faktor nicht dokumentiert), Field-of-View als Ziehen (es gibt den Regler unter [+]), Disable Viewport / Expert Mode / Redraw / Virtual
  Viewport (im Model-Tab ohne Sinn), der Strg-Modus des Orbit (Hilfe: Welt-Z und Welt-X).
- Blender: Gehen/Fliegen (Umschalt+`` ` ``), Trackball-Orbit (Vorgabe ist Turntable), view3d.dolly (Umschalt+Strg+Mitteltaste), Kamera (Ziffernblock 0,
  Knopf des Gizmos, View Camera im Kuchenmenü ausgegraut), Local View, „alle Bereiche“ (Strg+Pos1, Strg+Ziffernblock ,), NDOF/Trackpad.

## Ergebnisse (rot → grün)

Rot: der alte Stand 76808a596 mit nur dem Test (`build/rot_exe`), `BHED_MODELL_NUR=nav`: **37 OK, 85 FEHLER** (`Y:\werkzeuge\mbn\belege\rot_nav*.{out,log,txt}`).
Grün: derselbe Test auf dem neuen Stand, um die Prüfungen der neuen Teile erweitert (Griff und Rollen des Orbit-Modus, Orbit Point of Interest, Zoom um die Mitte, Einrasten mit Alt, Umschalt+B, die beiden Blender-Schalter, Rechtsklick-Menü, Suche, Meldung bei leerem Verlauf): **132 OK, 0 FEHLER** (Lauf `mbn_e_nav`).

| Bedienung | alt (rot, gemessen) | neu (grün) |
|---|---|---|
| Max: Mitteltaste 120, 60 Bildpunkte in der Perspektive | der Punkt am Drehpunkt wandert 131,26, 65,63 | 120,00, 60,00 |
| Max: Alt+Mitteltaste 50 Bildpunkte nach unten | Nick −20 (Kamera sinkt) | Nick +20 (Kamera steigt) |
| Max: Alt+Mitteltaste in der Ansicht Vorne | nichts | Orthographisch, Nick +12, Gier −20 |
| Max: Umschalt+Alt+Mitteltaste 60, 20 | Nick −8, Gier −24 | nur Gier −24 |
| Max: Rad neben der Mitte (150, 100) | der Punkt unter der Maus wandert auf 166,67, 111,11 | bleibt bei 150, 100 |
| Max: Z in vier Ansichten | nur die aktive eingepasst (101 / 105 / 105 statt 24 / 35 / 35) | alle vier |
| Max: Alt+Mitteltaste mit gewählter Hand | die Hand wandert im Bild um 83 Bildpunkte | bleibt (unter 2) |
| Max: `[` `]`, Strg+Alt+Z, 2x, Alt+Z, Strg+W, Strg+P, Strg+R, I, Umschalt+Z/Y, V, Alt+Pos1, Alt+Strg+H/V | ohne Wirkung | wie 3ds Max |
| ViewCube: am Würfel 50, 30 ziehen (Ansicht Vorne) | Nick −12 | Nick +12 |
| Blender: Mitteltaste 50 Bildpunkte | schiebt (Gier bleibt 90) | Gier 70 |
| Blender: Strg+Mitteltaste 100 hoch | schiebt (Faktor 1,0) | Faktor 0,637 (Dolly-Formel) |
| Blender: Rad | 0,9 / 0,85 | 0,8333 |
| Blender: Ziffernblock 4 | Gier 105 | Gier 75 |
| Blender: Umschalt+Ziffernblock 4 | Roll −15 | Roll +15 |
| Blender: Ziffernblock 5 aus schräger Perspektive | Achsenansicht | Orthographisch, gleiche Richtung |
| Blender: Pos1 | halbe kleinere Bildseite 61,27 | 44,75 = 0,7 × 63,92 |
| Blender: Strg+Alt+Q | Oben / Vorne / Links / Perspektive | Oben / Perspektive / Vorne / Rechts |
| Blender: Gizmo Zoom 80 hoch | Faktor 0,656 | 0,710 (Dolly aus der Bildmitte) |
| Blender: Gizmo Mitte bei gerader Sicht (Perspektive) | Vorne | Hinten (Gegenseite) |

Fehler des Tests, im ersten Grün-Lauf gefunden (121 OK, 8 FEHLER, alle im Test): (1) M9 nahm die Sollgrößen, bevor die vier Ansichten ihre Größe
kannten; (2) sechs untere Ziele des ViewCube trafen von unten gesehen den Kompassring, der dort vor dem Würfel liegt und wie in der Vorlage vorgeht -
die 26 Ziele laufen darum ohne Kompass (auf dem alten Stand dieselben sechs).

## Schlussrunde (Regel 11.11.: je Zweig nur das Berührte)

| Schritt | Ergebnis |
|---|---|
| Bau (inkrementell, zuletzt alle Ziele nach dem neuen Text: `bau_c`, 1163 s) | 0 Fehler, 0 Warnungen (/W4 /WX); die Bauten davor `bau_a`, `bau_b` ebenso |
| ctest (-j 12, alle 246) | 245 von 246 bestanden; rot nur `q3map2_teilbau` mit 2 Fehlschlägen (zweiraum -bounce 2) - derselbe Stand wie auf der Basis (Notizen MBKOPF und MBRASTER), dieser Zweig ändert an q3map2 und dem Test nichts |
| eigener Selbsttest `nav` (letzter Bau) | 132 OK, 0 FEHLER |
| `kopf` (letzter Bau: das Rechtsklick-Menü bekam das Untermenü Navigation) | 115 OK, 0 FEHLER |
| Gruppen, die Ansichten mit Maus oder Tasten bedienen, auf `bau_b` (danach nur noch der Text der Meldung, das Untermenü im Rechtsklick-Menü und der Test) | kopfrot 25/0, raster 40/0, blender 67/0, tasten 43/0, funktionen 61/0, mb2 37/0, md 86/0, pt 41/0 |
| Lints (alle `tools/lint_*.py`, gegen 76808a596 verglichen) | zeilengleich bis auf Zählerstände (msvclink 112 statt 111 Dateien, settings 105/93 statt 100/88 Schlüssel/Felder); efxflags, fremdprogramm, portability, pruefcodes schon auf der Basis mit gleicher Ausgabe beanstandet |

Bewusst nicht gelaufen (Regel 11.11.: die volle Runde läuft einmal auf dem zusammengeführten Stand): sauberer Neubau, Modus 1, menueeditor und die
modellblock-Gruppen ohne Rad, Mitteltaste oder Navigationstasten (bild, bild2, mb3, rundlauf, rig, rigrundlauf, mb5, kurvenrundlauf, mb6, ikrundlauf, mb7,
mb7textur, ph, rt, rtjka, pr, ai, uv, uvmd3, uvrundlauf, mdkarte, ptfix, ptfix2, ptfix3, ptstufen, pfix, pfix2, pfixkarte, die JKA-/JK2-Läufe).
Geänderte Erwartungen vorhandener Tests: `kopf` drei Prüfzeilen (Ziffernblock 4 → Gier 75 statt 105, Umschalt+Ziffernblock 4 → Roll +15 statt −15,
Gizmo-Knopf Persp/Ortho → Orthographisch in derselben Richtung statt nächste Achsenansicht) und Pausen über den weichen Wechsel; `blender` wartet
0,4 statt 0,15 s nach jeder Taste.

## Gegen das Vorbild geprüft / nicht geprüft

- Geprüft gegen den Blender-Quelltext (Formeln und Vorgaben, Stellen in `gui/app_modelltab_nav.cpp`) und gegen die Autodesk-Hilfe (Tasten, was eine
  Bedienung tut). 3ds Max und Blender wurden nicht gestartet: kein Vergleichslauf, keine Messung am Vorbild.
- Nicht geprüft: Bildzeit (die Navigation rechnet je Bild wenige Vektoren; der Wächter des Verlaufs einen Vergleich je sichtbarer Ansicht).

## Offen / Fragen

- SteeringWheels und Durchgangsmodus (3ds Max), Gehen/Fliegen (Blender): nachbauen?
- Vorgabe des Orbit-Flyouts von 3ds Max 2027 (hier Orbit SubObject).
- Strg+Umschalt+Z im Modus wie 3ds Max: Wiederholen (heute) oder Zoom Extents All (wie Max)?
- Strg+Mitteltaste (schnelleres Schieben in Max): Faktor? Trackball-Orbit und view3d.dolly von Blender; Kameraansichten.
- Handbuchbilder: neu sind h_mbn_max, h_mbn_orbit, h_mbn_blender in sechs Sprachen (2538 breit; en alle drei und de h_mbn_max angesehen). Die übrigen
  Bilder des Model-Tabs (35 je Sprache) zeigen über den Ansichten noch die Hinweiszeile ohne die Maus-Navigation am Anfang - sonst unverändert;
  neu aufnehmen auf dem zusammengeführten Stand (andere Zweige ändern die Oberfläche ebenfalls).

## Changelog

## English

- Model tab: the views are now navigated exactly like 3ds Max or like Blender, depending on the control mode. Before, both modes used the same mouse set, orbiting up and down was mirrored, panning in the perspective view did not follow the mouse exactly and many controls were missing.
- Model tab, 3ds Max mode: the middle button pans (Shift: one axis only), Alt+middle button orbits - now also in the views Top, Front, Left ..., which become the Orthographic view (Shift: one axis only) - about the selection (Orbit SubObject; Orbit, Orbit Selected and Orbit Point of Interest under [view name] > Navigation > Orbit around), the wheel, [ ] and Ctrl+= / Ctrl+- zoom about the mouse point, Z fits all views (Ctrl+Alt+Z the active one, Zoom extents all in the menu), Ctrl+Alt+Shift+Z / Alt+Shift+Z zoom 2x, the navigation modes Zoom (Alt+Z), Pan (Ctrl+P), Orbit (Ctrl+R, the yellow circle with its handles) and Zoom Region (Ctrl+W) work with the left button until a right-click or Esc, I pans to the mouse pointer, Shift+Z / Shift+Y undo and redo view changes of each view, V opens the menu of the views, Alt+Home and Alt+Ctrl+H go Home, Alt+Ctrl+V shows the ViewCube.
- Model tab, Blender mode: the middle button orbits (turntable, over the top, Alt snaps to the axis views), Shift+middle button pans, Ctrl+middle button zooms, Alt+middle button dragged turns to the neighbouring axis view and clicked centres the view on the point under the mouse; the wheel, Numpad + / - and Ctrl+= / Ctrl+- zoom by 1.2; Numpad 1, 3, 7 turn smoothly; Numpad 5 keeps the direction; Numpad 4 / 6 and Shift+Numpad 4 / 6 turn the right way round (they were mirrored); Ctrl+Numpad 4 / 6 / 8 / 2 pan; Numpad . , Home and Shift+C frame like Blender (Numpad . does nothing without a selection); ` opens the View pie menu; Shift+B zooms to a region; the quad view (Ctrl+Alt+Q) is arranged like Blender's; the zoom, move and perspective buttons of the gizmo work like Blender's; Orbit Around Selection and Zoom to Mouse Position are under View > Navigation.
- Model tab: dragging the ViewCube or the gizmo now turns the view the same way as the mouse (up and down was mirrored).
- Model tab: the line above the views names the mouse navigation first; every navigation command is in the menu of the view header under Navigation, in the right-click menu and in the search.

## Deutsch

- Model-Tab: Die Ansichten lassen sich jetzt genau wie in 3ds Max bzw. wie in Blender bewegen, je nach Bedienmodus. Bisher galt in beiden Modi dieselbe Mausbelegung, das Kreisen nach oben und unten war gespiegelt, das Schieben in der Perspektive folgte der Maus nicht genau, und viele Bedienungen fehlten.
- Model-Tab, Modus wie 3ds Max: Die Mitteltaste schiebt (Umschalt: nur eine Achse), Alt+Mitteltaste kreist - jetzt auch in den Ansichten Oben, Vorne, Links ..., die dabei zur Ansicht Orthographisch werden (Umschalt: nur eine Achse) - um die Auswahl (Orbit SubObject; Orbit, Orbit Selected und Orbit Point of Interest unter [Name der Ansicht] > Navigation > Kreisen um), Rad, [ ] und Strg+= / Strg+- zoomen um den Mauspunkt, Z passt alle Ansichten ein (Strg+Alt+Z die aktive, Zoom Grenzen: Alle im Menü), Strg+Alt+Umschalt+Z / Alt+Umschalt+Z zoomen 2x, die Navigationsmodi Zoom (Alt+Z), Schieben (Strg+P), Orbit (Strg+R, der gelbe Kreis mit seinen Griffen) und Bereichszoom (Strg+W) arbeiten mit der linken Taste bis zum Rechtsklick oder Esc, I schiebt zum Mauszeiger, Umschalt+Z / Umschalt+Y nehmen Ansichtsänderungen je Ansicht zurück und stellen sie wieder her, V öffnet das Menü der Ansichten, Alt+Pos1 und Alt+Strg+H gehen zur Home-Ansicht, Alt+Strg+V zeigt den ViewCube.
- Model-Tab, Modus wie Blender: Die Mitteltaste kreist (Turntable, auch über den Pol, Alt rastet auf die Achsenansichten), Umschalt+Mitteltaste schiebt, Strg+Mitteltaste zoomt, Alt+Mitteltaste gezogen dreht auf die benachbarte Achsenansicht und geklickt legt den Punkt unter der Maus in die Bildmitte; Rad, Ziffernblock + / - und Strg+= / Strg+- zoomen um 1,2; Ziffernblock 1, 3, 7 drehen weich; Ziffernblock 5 behält die Richtung; Ziffernblock 4 / 6 und Umschalt+Ziffernblock 4 / 6 drehen richtig herum (sie waren gespiegelt); Strg+Ziffernblock 4 / 6 / 8 / 2 schieben; Ziffernblock , , Pos1 und Umschalt+C passen wie Blender ein (Ziffernblock , tut ohne Auswahl nichts); ` öffnet das Kuchenmenü View; Umschalt+B zoomt auf einen Bereich; die Vierer-Ansicht (Strg+Alt+Q) ist wie in Blender angeordnet; die Knöpfe Zoom, Verschieben und Perspektive des Gizmos arbeiten wie in Blender; Um die Auswahl kreisen und Zur Mausposition zoomen stehen unter View > Navigation.
- Model-Tab: Ziehen am ViewCube oder am Gizmo dreht die Ansicht jetzt in dieselbe Richtung wie die Maus (hoch und runter war gespiegelt).
- Model-Tab: Die Zeile über den Ansichten nennt zuerst die Navigation mit der Maus; jeder Navigationsbefehl steht im Menü der Kopfzeile unter Navigation, im Rechtsklick-Menü und in der Suche.

## 中文

- 模型选项卡: 现在可以完全按照 3ds Max 或 Blender 的方式操作视图, 取决于操作模式。以前两种模式使用同一套鼠标操作, 上下环绕方向相反, 透视图中的平移不能精确跟随鼠标, 并且缺少许多操作。
- 模型选项卡, 3ds Max 模式: 中键平移 (Shift: 仅一个轴), Alt+中键环绕——现在在顶、前、左等视图中也可以, 它们会变为正交视图 (Shift: 仅一个轴)——围绕选择 (Orbit SubObject; Orbit、Orbit Selected 和 Orbit Point of Interest 位于 [视图名] > 导航 > 环绕中心), 滚轮、[ ] 和 Ctrl+= / Ctrl+- 围绕鼠标点缩放, Z 适配所有视图 (Ctrl+Alt+Z 适配活动视图, 所有视口最大化显示在菜单中), Ctrl+Alt+Shift+Z / Alt+Shift+Z 缩放 2 倍, 导航模式缩放 (Alt+Z)、平移 (Ctrl+P)、环绕 (Ctrl+R, 带手柄的黄色圆圈) 和区域缩放 (Ctrl+W) 用左键操作, 直到右键单击或 Esc, I 平移到鼠标指针, Shift+Z / Shift+Y 撤销和重做各视图的视图更改, V 打开视图菜单, Alt+Home 和 Alt+Ctrl+H 回到主视图, Alt+Ctrl+V 显示 ViewCube。
- 模型选项卡, Blender 模式: 中键环绕 (转盘, 可越过顶点, Alt 吸附到轴向视图), Shift+中键平移, Ctrl+中键缩放, Alt+中键拖动转到相邻的轴向视图, 单击则将鼠标下的点置于视图中央; 滚轮、小键盘 + / - 和 Ctrl+= / Ctrl+- 按 1.2 缩放; 小键盘 1、3、7 平滑旋转; 小键盘 5 保持方向; 小键盘 4 / 6 和 Shift+小键盘 4 / 6 的方向已更正 (以前相反); Ctrl+小键盘 4 / 6 / 8 / 2 平移; 小键盘 . 、Home 和 Shift+C 的框显与 Blender 相同 (未选择时小键盘 . 无操作); ` 打开视图饼菜单; Shift+B 缩放到区域; 四视图 (Ctrl+Alt+Q) 的排列与 Blender 相同; Gizmo 的缩放、移动和透视按钮与 Blender 相同; 围绕选择物体旋转和缩放至鼠标位置位于 View > 导航。
- 模型选项卡: 拖动 ViewCube 或 Gizmo 时视图的旋转方向现在与鼠标一致 (以前上下方向相反)。
- 模型选项卡: 视图上方的一行首先列出鼠标导航; 每个导航命令都在视图标题栏菜单的导航下、右键菜单和搜索中。

## 日本語

- モデルタブ: 操作モードに応じて、ビューを 3ds Max または Blender とまったく同じように操作できるようになりました。これまでは両モードで同じマウス操作で、上下のオービットが逆になっており、パースでのパンがマウスに正確に追従せず、多くの操作がありませんでした。
- モデルタブ、3ds Max モード: 中ボタンでパン (Shift: 1 軸のみ)、Alt+中ボタンでオービット - 上・前・左などのビューでも可能になり、平行投影ビューになります (Shift: 1 軸のみ) - 選択を中心に (Orbit SubObject。Orbit、Orbit Selected、Orbit Point of Interest は [ビュー名] > ナビゲーション > オービットの中心)、ホイール、[ ]、Ctrl+= / Ctrl+- はマウス位置を中心にズーム、Z はすべてのビューを全体表示 (Ctrl+Alt+Z はアクティブビュー、全ビューで全体表示はメニュー)、Ctrl+Alt+Shift+Z / Alt+Shift+Z は 2 倍ズーム、ナビゲーションモードのズーム (Alt+Z)、パン (Ctrl+P)、オービット (Ctrl+R、ハンドル付きの黄色い円)、領域ズーム (Ctrl+W) は右クリックか Esc まで左ボタンで操作、I はマウスポインタへパン、Shift+Z / Shift+Y はビューごとのビュー変更を元に戻す / やり直す、V はビューのメニュー、Alt+Home と Alt+Ctrl+H はホームへ、Alt+Ctrl+V は ViewCube を表示。
- モデルタブ、Blender モード: 中ボタンでオービット (ターンテーブル、真上も越えられ、Alt で軸ビューにスナップ)、Shift+中ボタンでパン、Ctrl+中ボタンでズーム、Alt+中ボタンのドラッグで隣の軸ビューへ、クリックでマウスの下の点をビューの中央に。ホイール、テンキー + / -、Ctrl+= / Ctrl+- は 1.2 倍でズーム。テンキー 1、3、7 はなめらかに回転。テンキー 5 は向きを保ちます。テンキー 4 / 6 と Shift+テンキー 4 / 6 は正しい向きに回ります (以前は逆でした)。Ctrl+テンキー 4 / 6 / 8 / 2 でパン。テンキー . 、Home、Shift+C は Blender と同じ全体表示 (選択がなければテンキー . は何もしません)。` はビューパイメニュー、Shift+B は領域ズーム、4 分割表示 (Ctrl+Alt+Q) は Blender と同じ配置、ギズモのズーム・移動・透視ボタンは Blender と同じ動作。選択部分を中心に回転とマウス位置でズームは View > ナビゲーションにあります。
- モデルタブ: ViewCube やギズモのドラッグがマウスと同じ向きにビューを回すようになりました (上下が逆でした)。
- モデルタブ: ビューの上の行はまずマウスのナビゲーションを表示します。すべてのナビゲーションコマンドはビューヘッダーのメニューのナビゲーション、右クリックメニュー、検索にあります。

## Español

- Pestaña Modelo: las vistas se manejan ahora exactamente como en 3ds Max o como en Blender, según el modo de control. Antes ambos modos usaban el mismo ratón, la órbita hacia arriba y abajo estaba invertida, el encuadre en perspectiva no seguía exactamente al ratón y faltaban muchos controles.
- Pestaña Modelo, modo 3ds Max: el botón central encuadra (Mayús: solo un eje), Alt+botón central orbita - ahora también en las vistas Superior, Frontal, Izquierda ..., que pasan a ser la vista Ortográfica (Mayús: solo un eje) - alrededor de la selección (Orbit SubObject; Orbit, Orbit Selected y Orbit Point of Interest en [nombre de la vista] > Navegación > Orbitar alrededor de), la rueda, [ ] y Ctrl+= / Ctrl+- amplían sobre el punto del ratón, Z encuadra todas las vistas (Ctrl+Alt+Z la activa, Encuadrar todo en todas las vistas en el menú), Ctrl+Alt+Mayús+Z / Alt+Mayús+Z zoom 2x, los modos de navegación Zoom (Alt+Z), Encuadre (Ctrl+P), Orbitar (Ctrl+R, el círculo amarillo con sus tiradores) y Zoom de región (Ctrl+W) funcionan con el botón izquierdo hasta un clic derecho o Esc, I encuadra hacia el puntero, Mayús+Z / Mayús+Y deshacen y rehacen los cambios de cada vista, V abre el menú de las vistas, Alt+Inicio y Alt+Ctrl+H van a Inicio, Alt+Ctrl+V muestra el ViewCube.
- Pestaña Modelo, modo Blender: el botón central orbita (plato giratorio, también por el polo, Alt ajusta a las vistas de los ejes), Mayús+botón central encuadra, Ctrl+botón central hace zoom, Alt+botón central arrastrado gira a la vista de eje vecina y con un clic centra la vista en el punto bajo el ratón; la rueda, numérico + / - y Ctrl+= / Ctrl+- amplían por 1,2; numérico 1, 3, 7 giran suavemente; numérico 5 mantiene la dirección; numérico 4 / 6 y Mayús+numérico 4 / 6 giran en el sentido correcto (estaban invertidos); Ctrl+numérico 4 / 6 / 8 / 2 encuadran; numérico . , Inicio y Mayús+C encuadran como Blender (numérico . no hace nada sin selección); ` abre el menú circular Vista; Mayús+B amplía una región; la vista cuádruple (Ctrl+Alt+Q) se dispone como en Blender; los botones zoom, mover y perspectiva del gizmo funcionan como en Blender; Orbitar alrededor de la selección y Zoom hacia la posición del ratón están en View > Navegación.
- Pestaña Modelo: arrastrar el ViewCube o el gizmo gira ahora la vista en el mismo sentido que el ratón (arriba y abajo estaba invertido).
- Pestaña Modelo: la línea sobre las vistas nombra primero la navegación con el ratón; cada orden de navegación está en el menú de la cabecera de la vista en Navegación, en el menú contextual y en la búsqueda.

## Čeština

- Karta Model: pohledy se nyní ovládají přesně jako v 3ds Max nebo jako v Blenderu, podle režimu ovládání. Dříve oba režimy používaly stejnou myš, obíhání nahoru a dolů bylo zrcadlené, posun v perspektivě nesledoval myš přesně a mnoho ovládacích prvků chybělo.
- Karta Model, režim 3ds Max: prostřední tlačítko posouvá (Shift: jen jedna osa), Alt+prostřední tlačítko obíhá - nyní i v pohledech Shora, Zepředu, Zleva ..., které se stanou pohledem Ortografický (Shift: jen jedna osa) - kolem výběru (Orbit SubObject; Orbit, Orbit Selected a Orbit Point of Interest v [název pohledu] > Navigace > Obíhat kolem), kolečko, [ ] a Ctrl+= / Ctrl+- přibližují kolem bodu myši, Z přizpůsobí všechny pohledy (Ctrl+Alt+Z aktivní, Přiblížit vše ve všech pohledech v nabídce), Ctrl+Alt+Shift+Z / Alt+Shift+Z zoom 2x, navigační režimy Zoom (Alt+Z), Posun (Ctrl+P), Obíhání (Ctrl+R, žlutý kruh s úchyty) a Zoom oblasti (Ctrl+W) pracují levým tlačítkem do klepnutí pravým nebo Esc, I posune ke kurzoru, Shift+Z / Shift+Y vracejí a znovu provádějí změny každého pohledu, V otevře nabídku pohledů, Alt+Home a Alt+Ctrl+H jdou Domů, Alt+Ctrl+V zobrazí ViewCube.
- Karta Model, režim Blender: prostřední tlačítko obíhá (točna, i přes pól, Alt přichytí k osovým pohledům), Shift+prostřední tlačítko posouvá, Ctrl+prostřední tlačítko přibližuje, Alt+prostřední tlačítko tažením otočí na sousední osový pohled a klepnutím vystředí pohled na bod pod myší; kolečko, numerická + / - a Ctrl+= / Ctrl+- přibližují 1,2krát; numerická 1, 3, 7 otáčejí plynule; numerická 5 zachová směr; numerická 4 / 6 a Shift+numerická 4 / 6 otáčejí správným směrem (byly zrcadlené); Ctrl+numerická 4 / 6 / 8 / 2 posouvají; numerická , , Home a Shift+C zobrazují jako Blender (numerická , bez výběru nedělá nic); ` otevře kruhovou nabídku Pohled; Shift+B přiblíží oblast; čtyři pohledy (Ctrl+Alt+Q) jsou uspořádány jako v Blenderu; tlačítka zoom, posun a perspektiva gizma pracují jako v Blenderu; Obíhat kolem výběru a Zoom na pozici myši jsou ve View > Navigace.
- Karta Model: tažení ViewCube nebo gizma nyní otáčí pohled stejným směrem jako myš (nahoru a dolů bylo zrcadlené).
- Karta Model: řádek nad pohledy uvádí nejprve navigaci myší; každý navigační příkaz je v nabídce záhlaví pohledu pod Navigace, v nabídce pravého tlačítka a ve hledání.
