# Notizen AP6 (11./12.11.): Bereich Modellieren, rechtes Feld nach dem Entwurf - MBENTWURF Teil B, rc612

Begonnen von AP6 (Fable 5.2, drei WIP-Stände auf `b19b7f494`), fertig gemacht von Opus 5.5 nach `Y:\werkzeuge\ap6\UEBERGABE.md`: auf
karte1 `6947182e8` (AP3, AP9, AP7) gesetzt, Tasten der Knöpfe über `KnopfOpt::tasteHalten`, zwei Funde aus den Läufen behoben (Absturz der
Zeitleiste, Wortbruch neben der Taste), Läufe, Fotos, Handbuchbilder, Notizen.

Auftrag `auftrag_mbentwurf_b_1011.md`, Paket AP6; Maßstab ist der freigegebene Entwurf (`plaene/modellblock_stufe0/entwurf_modelltab.html`,
Bereich Modellieren), shanks Wünsche vom 04.11. und 10.11. und die Abgleichtabelle `abgleich_programm_entwurf.md` (Abschnitt 8: M0a-M11).
Worktree `Y:\p_ap6`, Zweig `p_ap6`, begonnen auf `b19b7f494` (Schnittstellen-Commit von AP3), fertig auf karte1 `6947182e8`. Selbsttest `gui/selbsttest_ap6.inc`
(`BHED_EDITORTEST=modellblock`, `BHED_MODELL_NUR=ap6 | ap6foto | ap6hb`), Skripte und Läufe `Y:\werkzeuge\ap6\`.

shank 04.11. (Bild 1): „kann man das noch schöner machen? einfach benutzerfreundlicher und übersichtlicher“ - 10.11.: „es geht ja nicht nur um
controls like blender oder 3ds max sondern das aussehen“.

## Was sich geändert hat

Alles in `gui/wof/app_modelltab_mesh.cpp` (das Feld `drawModellMeshBereich`, die Aktionstabelle, Tasten, Popups); Texte `tools/i18n_ap6.py`
(6 Sprachen), 32 eigene Symbole als Block am Ende von `enum class UiIcon` (`gui/app_internal.h`) und `gui/uiicons.cpp`. Alle Knöpfe des
Felds setzen `KnopfOpt::tasteHalten` (wie der Bereich UV): die Taste bleibt am Knopf, solange jedes Wort neben ihr Platz hat. Am Baustein
`gui/ui_rollout.cpp` eine Berichtigung dieser Regel (unten „Funde“), API unverändert; `gui/modelltab.h` bekommt einen Zähler für den
Selbsttest (`ModellZeit::veralteteSequenz`). Ein seit dem neuen Feld unbenutzter eigener Text (`Ap6InfoGewichte`) ist entfernt; lint_i18n ok.

- **Gruppen wie im Entwurf** (M0a, M0b): im Reiter des Bereichs (wie 3ds Max „Modify“, wie Blender „Modifiers“) stehen **Selection**, **Tools**,
  **JKA limits** - im Stil des Bedienmodus (flache Balken mit +/- bzw. abgerundete Kästen mit Pfeil). Bis rc611: „Edit Poly“ (Blender: „Edit
  Mode“), „Edit Geometry“, „Edit Vertices / Edges / Borders / Polygons“, „JKA limits“, „Prop in the map“ als ImGui-Gruppen mit Knöpfen in
  Textbreite. Die Namen aus 3ds Max bleiben als Untertitel rechts in der Überschrift: „Edit Poly“ / „Edit Mode“ bei Selection, der Name des
  Rollouts der Ebene („Edit Polygons“ ...) bei Tools; wie Blender steht dort die Ebene (Vertex, Edge, Face - so heißen dort die Menüs).
- **Selection** (M3, M4): die Ebenen als ein Segment über die ganze Breite mit der Taste davor - wie 3ds Max „1 Vertex · 2 Edge · 3 Border ·
  4 Polygon · 5 Element“, wie Blender „1 Vertex · 2 Edge · 3 Face“ (deutsch wie im Entwurf: Ecke, Kante, Rand, Polygon, Element, Fläche).
  Darunter Ring, Loop, All, None im Raster mit Symbol und Taste (Ring und Loop nur auf den Kantenebenen bedienbar) und der Zähler der Ebene
  als Wertzeile („Polygons … 7490 · selected 4“; alle drei Zahlen im Tooltip - bis rc611 ein Satz mit sechs Zahlen).
- **Tools** (M5): Raster mit zwei gleich breiten Spalten, jeder Knopf mit Symbol und der Taste des Bedienmodus: zuerst die sechs
  Hauptwerkzeuge des Entwurfs (Extrude, Inset, Chamfer, Loop Cut, Cut, Merge), dahinter die weiteren Werkzeuge der Ebene (Ecke: Remove, Weld,
  Target Weld, Connect; Kante: Remove, Remove and clean, Connect, Bridge; Rand: Bridge, Connect; Polygon/Element: Bevel, Hinge, Bridge); der
  Rest zugeklappt unter „Edit Geometry“ (Create primitive, Detach, Symmetrize, To Quads, Collapse, Make Planar, Slice, QuickSlice, Delete,
  Copy this LOD to all). Am Ende der Tipp des Entwurfs als Kasten. Ein Werkzeug, das auf der laufenden Ebene nicht arbeitet, ist gesperrt;
  sein Tooltip nennt die Ebenen („Not at this level. Works on: Polygon, Element.“) - bis rc611 standen diese Knöpfe auf der Ebene gar nicht da.
- **Strg+Enter** (M5): übernimmt und beginnt dasselbe Werkzeug noch einmal auf dem Ergebnis - die Taste des Knopfs „+“ im Werkzeugkästchen
  (sein Tooltip nennt sie jetzt).
- **JKA limits** (M6): Balken für die gewählte Fläche (Ecken, Knochen, Gewichte gegen die Grenze des Zielspiels; gelb ab 90 %, rot darüber -
  bei den Ecken ist schon die Grenze selbst zu viel), darunter „Surfaces in the model“. Die Kurzinfo der Überschrift: rot die Zahl der Befunde
  im ganzen Modell, sonst gelb die Grenze, der die gezeigte Fläche nahe kommt („29 / 32 bones“), sonst grün „all good“. Jeder Befund steht als
  rote Zeile da (höchstens sechs Flächen, dann „+n more“), auch wenn die Balken eine andere Fläche zeigen. Ohne Auswahl zeigen die Balken
  die Fläche, die mit Ecken oder Knochen einer Grenze am nächsten kommt (Hinweis als Kasten). Die Liste aller Flächen ist ein zugeklappter
  Teil „All surfaces (n)“ (Tabelle Ecken · Knochen · Gewichte, Tooltip mit dem Satz von rc611). Bei kyle (83 Teile) war die Gruppe acht
  Bildschirmseiten lang, jetzt 11 Zeichenhöhen. Ein Requisit (.md3) hat kein Skelett: dort steht nur der Balken der Ecken.
  Die Maße je Teil (`w::grenzenPruefen`) rechnet das Feld einmal je Stand des Arbeitsnetzes statt in jedem Bild.
- **Grundkörper und Requisit** (M11): wie 3ds Max im Reiter **Create**, wie Blender im Reiter **Object**: die Gruppe „Primitives“ mit einem
  Knopf je Form (Box, Plane, Cylinder, Sphere, Cone, Tube, Torus; ein offenes Kästchen wechselt nur die Form) und „Prop in the map“ (Name,
  Into the map, New prop). Im Reiter des Bereichs stehen sie nur, solange man sie dort braucht: ein leeres Netz zeigt „Primitives“ ganz oben
  (mit dem Hinweis, damit zu beginnen), ein offenes Requisit behält „Prop in the map“ mit „Into the map“ als Hauptknopf - der eine Klick
  bleibt. Bei einer Figur (.glm) zeigt der Reiter des Bereichs genau die Gruppen des Entwurfs. „JKA limits“ steht außerdem unter
  **Utilities** bzw. **Data**, wie Blender zeigt der Reiter **Tool** Selection und Tools allein.
- **Tasten wie Blender** (M9, M11): **E** extrudiert (bis rc611 Alt+E; Alt+E geht weiter - in Blender öffnet es das Menü Extrudieren,
  dessen Arten hier im Kästchen stehen). **Umschalt+A** öffnet das Menü „Add“ mit den sieben Grundkörpern und „New prop“ (bis rc611 öffnete
  es das Kästchen mit der zuletzt gewählten Form). Quellen: Blender-Handbuch „Extrude Faces - Shortcut: E“
  (https://docs.blender.org/manual/en/latest/modeling/meshes/editing/face/extrude_faces.html), „Mesh > Extrude - Shortcut: Alt-E“
  (https://docs.blender.org/manual/en/latest/modeling/meshes/editing/mesh/extrude.html), `belege/blender_keys.txt` („Mesh | wm.call_menu |
  Alt+E name=VIEW3D_MT_edit_mesh_extrude“, „Mesh | wm.call_menu | Umschalt+A name=VIEW3D_MT_mesh_add“).
- **Meldung**: die letzte Meldung des Bereichs stand im Feld zweimal (am Ende des Bereichs und in der Zeile des Rahmens); jetzt einmal.

## Bewusst anders als Entwurf bzw. Vorschlag der Tabelle

- **„Flächen im Modell 12 (Hinweis ab 32)“**: die Zahl der Flächen steht da, der Hinweis „ab 32“ nicht - für 32 gibt es keine Quelle. OpenJK
  kennt `MD3_MAX_SURFACES 32 + 32` (qfiles.h, für .md3; in der vorliegenden Quelle nirgends geprüft) und `skin_t::surfaces[128]`
  (rd-vanilla/tr_local.h:517); eine Grenze von 32 Flächen je .glm steht nirgends. Regel „keine Zahl ohne Quelle“ - Frage an shank.
- **Namen der Werkzeuge**: Chamfer, Cut, Merge heißen weiter wie in rc611 (deutsch „Abkanten (Chamfer)“, „Schneiden“, „Zusammenführen“); der
  Entwurf schreibt „Fase“, „Messer“, „Verschmelzen“. Die Texte sind Zeile M12 der Tabelle (nicht Teil von AP6) und stehen so in Menü,
  Suche und Handbuch.
- **Mehr Knöpfe als der Entwurf**: der Entwurf zeigt sechs Werkzeuge; das Programm hat 25 weitere Aktionen - sie stehen wie im Vorschlag der
  Tabelle darunter (Ebene) bzw. zugeklappt (Edit Geometry). „Chamfer vertices“ ist kein eigener Knopf mehr: auf der Ebene Ecke tut „Chamfer“
  dasselbe; die Aktion bleibt in Menü, Rechtsklick, Suche und auf ihrer Blender-Taste.
- **Vier Ansichten als Vorgabe** (in M0a genannt): Zeile R7, Paket AP5. **Modifier-Stapel** (M1, M2): AP13. **Kurze Zeitleiste** (M10): AP1.
  **Werkzeugleiste T, Outliner rechts** (M0b): AP4.

## Funde aus den Läufen (rot -> grün)

- **Absturz der Zeitleiste nach einer Änderung des Modells** (nicht AP6-Code, gefunden im Lauf `ap6k_mb_ap7`: 0xC0000005 in `transport`,
  `gui/app_modelltab_zeit.cpp`, nach „Abwickeln“ wie Blender; die Wiederholung lief grün). Ursache: `drawModellZeitleiste(e)` bekommt die
  Sequenz als Zeiger in `g_app->animList`, ruft dann `modellCfgAbgleich()` - der setzt nach jeder Änderung des Dokuments die Liste aus der
  animation.cfg neu (`cfgAbgleich`, `gui/app_modelltab_mb3.cpp`), und die Leiste zeichnete im selben Bild mit dem Zeiger in die freigegebene
  alte Liste weiter (meist liest das still alte Werte, manchmal stürzt es ab). Nachgestellt ohne Zufall: der Zähler
  `ModellZeit::veralteteSequenz` zählt Bilder, in denen die Sequenz nicht in der gültigen Liste liegt; neue Prüfung „AP6 Fund“ (kyle,
  Polygone extrudieren, übernehmen): rot je Bedienmodus „animation.cfg da, Liste neu gelesen, 1 Bilder mit der alten Liste“ (Lauf `ap6r2`),
  nach der Behebung (`e = sequenz()` nach dem Abgleich) grün „0 Bilder“ (`ap6g2`, `ap6g3`); ap1 137/0, ap7 88/0.
- **Wortbruch neben der Taste** (Foto de, wie Blender: „Zusammenfüh / ren“ neben M). Die Bedingung von `KnopfOpt::tasteHalten` (AP7) hielt
  die Taste auch, wenn ein Wort nicht neben sie passte, sobald dem Text 4 Zeichenhöhen blieben (alte Regel ODER neue). Jetzt gilt mit
  `tasteHalten` genau die beschriebene Regel: Taste nur, wenn jedes Wort passt, sonst fällt sie weg (steht im Tooltip). Ohne `tasteHalten`
  unverändert. Foto danach: „Zusammenführen“ einzeilig, ebenso „Entfernen und aufräumen“ ohne Taste; ap7 88/0, ap3 105/0. Eingetragen in
  `Y:\werkzeuge\ap3\SCHNITTSTELLE.md`.

## Selbsttest (rot -> grün)

- Rot (`ap6_rot1`, exe des alten Felds + neuer Selbsttest): 3 OK, 77 FEHLER (Zeilen `Y:\werkzeuge\ap6\rot1_zeilen.txt`). Der Lauf endete mit
  einem Absturz, den der TEST verursachte (ungültiges Vieleck −1 in der Auswahl) - im Test behoben (`deckelWaehlen()`), der gültige Weg
  „Grundkörper, Rückgängig bis leer, Wiederholen“ ist eine eigene Prüfung.
- Erster Lauf auf dem neuen Feld (`ap6_g1`, auf `b19b7f494`): 80 OK, 16 FEHLER, alle im Requisit-Teil: die Gruppe „Grundkörper“ verschwand,
  sobald die Vorschau des ersten Grundkörpers Polygone hatte (das Feld zählt jetzt bei offenem Kästchen das Netz davor), der Test klickte den
  richtig gesperrten Knopf „Einfügen“ mit `klick()` (meldet „Klick kam nicht an“; jetzt `klickAn`), dazu Folgefehler. Keine Prüfung
  abgeschwächt.
- Grün auf karte1 (`ap6k`, `ap6g2`, `ap6g3`): **94 OK, 0 FEHLER** (92 Prüfungen der Zeilen + 2 „Fund“). Je Zeile: M0a 3, M0b 4, M3 6, M4 6,
  M5 12 + 12, M6 6 + 6, M7 1 + 1, M8 6, M9 10, M11 8 + 10.

## Angepasste vorhandene Selbsttests (nichts gelöscht)

`gui/selbsttest_md.inc` (Gruppe `md`, `mdkarte`):
- „MD Neues Requisit: … Befehlsfeld mit den Rollouts“ suchte ein Element mit der Beschriftung „Edit Poly“ -> jetzt: eine Gruppe „Selection“
  mit dem Untertitel „Edit Poly“.
- „MD Befehlsfeld: Rollout Edit Poly, Geometrie, Polygone, JKA-Grenzen und Requisit; Knöpfe mit Tooltip“ -> jetzt: Gruppen Selection, Tools
  (Untertitel „Edit Polygons“), JKA limits, Prop in the map, der Teil Edit Geometry, die Knöpfe Extrude, Inset, Into the map (weiter acht
  Elemente, im rechten Feld gesucht).
- `mdkarte`: vor dem Klick auf „Into the map“ rollt der Test die Gruppe ins Bild (das Feld ist mit den Symbolknöpfen höher).

## Fotos

`Y:\werkzeuge\ap6\fotos\` (Selbsttest `ap6foto`, 18 Bilder je Variante, 3818 × 2104 bei Oberfläche 1,5; je Lauf 0 FEHLER): en, de,
zh-Hans, ja, es, cs, hell, klein (1328 × 844 bei 1,0). Neben dem Entwurf: `vergleich\ap6_f_<variante>_ap6_<max|blender>_modell.png`,
weitere Kacheln `vergleich\de_k2.png`, `es_k2.png`, `es_mehr.png`, `hell_mehr.png`. Angesehen:
- Max en/de/cs: Reiter Ändern, Auswahl mit Segment 1-5 über die volle Breite und Untertitel „Poly bearbeiten“, Ring/Schleife/Alles/Nichts
  im Raster mit Symbol und Taste, Zählerzeile, Werkzeuge im Raster mit den sechs Hauptwerkzeugen zuerst, Tipp als blauer Kasten,
  JKA-Grenzen mit Balken und Kurzinfo - Aufbau wie im Entwurf (ohne Modifier-Stapel, AP13).
- Blender en/de/ja: Symbolspalte, Reiter Modifier gewählt, runde Kästen mit Pfeil, Segment 1-3, Tasten E/I/Strg+B/Strg+R/K/M wie im Entwurf.
  Vor der Berichtigung de „Zusammenfüh / ren“ (oben, Funde) - danach einzeilig. ja: Katakana brechen nach den CJK-Regeln zwischen den
  Zeichen („ループカッ / ト“, „リン / グ“ neben Strg+Alt+Klick) - so wie im Bereich UV; nichts abgeschnitten.
- es: lange Wörter („Quitar y limpiar“, „Corte en bucle“) brechen an Wortgrenzen um, Taste bleibt; Segment wie 3ds Max geht in zwei Reihen,
  wenn die fünf Ebenen nicht in eine passen.
- hell: Betonungsfarbe, Segment und gesperrte Knöpfe im hellen Thema lesbar.
- klein: Segment und Raster gehen auf eine Spalte, Knöpfe ohne Platz für die Taste zeigen sie nur im Tooltip; nichts abgeschnitten.
- Reiter Erstellen (Grundkörper im Raster, Requisit), Tooltip eines gesperrten Knopfs („Nicht auf dieser Ebene. Arbeitet auf: …“), Menü
  Hinzufügen wie Blender, Requisit mit offenem Kästchen: wie beschrieben.

## Handbuch

Eigene Quelle `tools/handbuch/inhalt_ap6.py` (6 Sprachen), noch nicht eingebunden - AP14 führt zusammen (die vier Zeilen stehen im Kopf der
Datei). `einhaengen()` ersetzt im ersten Absatz der Seite „Model-Tab: Modellieren“ den Satz über die Rollouts „Edit Poly“ und setzt zwei
Absätze, zwei Bilder (`h_ap6_max`, `h_ap6_blender`) und eine Notiz zu den Tasten hinter das Bild `h_md_modell`. Bilder je Sprache:
`handbuch/bilder/<sprache>/h_ap6_*.webp` (`sh Y:\werkzeuge\ap6\hb.sh`; 2538 × 1472 in 6 Sprachen, je Lauf 0 FEHLER, angesehen en wie
3ds Max und ja wie Blender); `gen_handbuch.py --pruefen` ok (der Hinweis „2 Einträge in uebersetzung/ nicht mehr gebraucht“ steht auf
karte1 genauso). Das vorhandene Bild `h_md_modell` zeigt noch das Feld von rc611
und muss beim Zusammenführen neu aufgenommen werden (`BHED_EDITORTEST=handbuch`, `BHED_HB_NUR=md`).

## Schlussrunde

| Schritt | Ergebnis |
|---|---|
| Rebase auf karte1 `6947182e8` | Konflikte nur in Listen (UiIcon, uiicons.cpp, selbsttest.cpp, gen_i18n.py, i18n_sprache_es/cs.py, test_betroffen.py): beide Seiten behalten, jede Stelle gelesen - in `uiicons.cpp` fehlte nach dem letzten UV-Symbol das `break` (eingefügt); i18n.h/.cpp mit `gen_i18n.py` neu erzeugt |
| Bau alle Ziele (/W4 /WX, inkrementell) | 0 Fehler, 0 Warnungen |
| ctest | 256 / 256 |
| `ap6` | 94 / 0 |
| Nachbargruppen (`ap6k`, `ap6g2`, `ap6g3`) | md 87 · mdkarte 8 · ap3 105 · ap7 88 · ap9 97 · ap1 137 · funktionen 62 · tasten 44 · blender 68 · pfix 18 · pfix2 6 · pfixkarte 6 · mbv 216 · nav 133 · icons 1 - je 0 FEHLER |
| ap7 im ersten Lauf | einmal Absturz (Fund oben), Wiederholung 88/0; nach der Behebung 88/0 |
| Fotos `ap6foto` | 8 Varianten, je 0 FEHLER |
| Handbuchbilder | 6 Sprachen, je 0 FEHLER |
| Lints (21) | ok; pruefcodes 11, portability 99 wie karte1 |

Bewusst nicht gelaufen (einmal auf dem zusammengeführten Stand): Modus 1, die übrigen modellblock-Gruppen, karteneditor, menueeditor,
sauberer Neubau. Messungen: keine - das Feld zeichnet dieselben Bausteine wie AP3/AP7, die Grenzen rechnet es einmal je Stand des Netzes.

## Offen

- **Frage an shank:** „Flächen im Modell 12 (Hinweis ab 32)“ - für die 32 fehlt die Quelle (siehe oben); heute steht nur die Zahl.
- Symbole in den Zeilen des Menüs „Add“ (wie Blender) - heute nur Text.
- Das Bild `h_md_modell` (Seite Modellieren) zeigt noch das Feld von rc611 - beim Zusammenführen des Handbuchs (AP14) neu aufnehmen.
- Der Segment-Baustein kennt keine kleinen Tastenfelder: die Taste der Ebene steht als Teil des Textes („1 Vertex“). Wunsch an AP3 in
  `Y:\werkzeuge\ap6\WUNSCH_AP3.md`.

## Changelog

## English

- Model tab, Modeling: the panel on the right follows the approved design - the groups Selection, Tools and JKA limits. The levels are one row of fields with their key in front (3ds Max mode 1-5, Blender mode 1-3); the six main tools are buttons of equal width with a symbol and the key of the control mode, the further tools of the level follow, the rest is folded under "Edit Geometry".
- A tool that does not work on the current level is greyed out, and its tooltip names the levels it works on.
- JKA limits: bars for the selected surface (vertices, bones and weights against the limit of the game, yellow from 90 %, red above) and a short note in the heading; the list of all surfaces is folded away (for a figure with 83 parts it used to fill eight pages).
- Primitives and props stand on the tab "Create" (3ds Max mode) or "Object" (Blender mode), one button per primitive; while a prop is open, "Into the map" stays one click away.
- Ctrl+Enter applies a tool and starts it again on the result (the "+" button of the tool caddy).
- Blender mode: E extrudes as in Blender (Alt+E still works), Shift+A opens the "Add" menu with the primitives.
- Model tab: the timeline could crash the program now and then right after a change of the model (for example after Unwrap) when the figure has an animation.cfg; fixed. Buttons in the panels of Modeling and UV keep their key only while every word fits next to it, so no word breaks in the middle.

## Deutsch

- Model-Tab, Modellieren: das Feld rechts folgt dem freigegebenen Entwurf - die Gruppen Auswahl, Werkzeuge und JKA-Grenzen. Die Ebenen sind eine Reihe von Feldern mit ihrer Taste davor (3ds-Max-Modus 1-5, Blender-Modus 1-3); die sechs Hauptwerkzeuge sind gleich breite Knöpfe mit Symbol und der Taste des Bedienmodus, danach folgen die weiteren Werkzeuge der Ebene, der Rest ist unter „Geometrie bearbeiten“ zugeklappt.
- Ein Werkzeug, das auf der laufenden Ebene nicht arbeitet, ist ausgegraut, und sein Tooltip nennt die Ebenen, auf denen es arbeitet.
- JKA-Grenzen: Balken für die gewählte Fläche (Ecken, Knochen und Gewichte gegen die Grenze des Spiels, gelb ab 90 %, rot darüber) und eine Kurzinfo in der Überschrift; die Liste aller Flächen ist eingeklappt (bei einer Figur mit 83 Teilen füllte sie acht Seiten).
- Grundkörper und Requisiten stehen im Reiter „Erstellen“ (3ds-Max-Modus) bzw. „Objekt“ (Blender-Modus), ein Knopf je Grundkörper; solange ein Requisit offen ist, bleibt „In die Karte“ ein Klick.
- Strg+Enter übernimmt ein Werkzeug und beginnt es noch einmal auf dem Ergebnis (der Knopf „+“ des Werkzeugkästchens).
- Blender-Modus: E extrudiert wie in Blender (Alt+E geht weiterhin), Umschalt+A öffnet das Menü „Hinzufügen“ mit den Grundkörpern.
- Model-Tab: die Zeitleiste konnte das Programm gleich nach einer Änderung des Modells (etwa nach Abwickeln) gelegentlich abstürzen lassen, wenn die Figur eine animation.cfg hat; behoben. Knöpfe in den Feldern Modellieren und UV behalten ihre Taste nur, solange jedes Wort daneben passt - kein Wort bricht mehr mitten im Wort um.

## 中文

- 模型选项卡,建模:右侧面板按已批准的设计重做——“选择”“工具”和“JKA 限制”三个组。各层级排成一行格子,前面是按键(3ds Max 模式 1-5,Blender 模式 1-3);六个主要工具是带符号和当前操作模式按键的等宽按钮,其后是该层级的其他工具,其余的折叠在“编辑几何体”下。
- 在当前层级不起作用的工具显示为灰色,其提示写明它适用的层级。
- JKA 限制:为所选面显示进度条(顶点、骨骼和权重对照游戏的限制,90 % 起为黄色,超出为红色),标题中有简短说明;全部面的列表已折叠(对于有 83 个部件的角色,它以前占满八页)。
- 基本体和道具位于“创建”选项卡(3ds Max 模式)或“物体”选项卡(Blender 模式),每个基本体一个按钮;只要打开的是道具,“放入地图”仍然只需单击一次。
- Ctrl+Enter 应用工具并在结果上再次启动它(工具框的“+”按钮)。
- Blender 模式:E 与 Blender 一样执行挤出(Alt+E 仍然可用),Shift+A 打开含基本体的“添加”菜单。
- 模型选项卡:当角色带有 animation.cfg 时,在修改模型之后(例如展开之后)时间轴偶尔会导致程序崩溃;已修复。建模和 UV 面板中的按钮只有在每个词都能放在按键旁边时才保留按键,单词不会再从中间断开。

## 日本語

- モデルタブのモデリング: 右側のパネルを承認済みのデザインどおりにしました - 「選択」「ツール」「JKA の上限」の 3 グループです。レベルはキーを前に付けた 1 列のマスで並び(3ds Max 風は 1〜5、Blender 風は 1〜3)、6 つの主要ツールは記号と操作モードのキーが付いた同じ幅のボタン、その後にレベルのその他のツールが続き、残りは「ジオメトリ編集」に折りたたまれています。
- 現在のレベルで使えないツールは灰色になり、ツールチップに使えるレベルが書かれます。
- JKA の上限: 選択したサーフェスのバー(頂点・ボーン・ウェイトをゲームの上限と比べ、90 % から黄色、超えると赤)と見出しの短い表示。全サーフェスの一覧は折りたたまれています(83 パーツのキャラクターでは以前 8 ページ分ありました)。
- プリミティブと小道具は「作成」タブ(3ds Max 風)または「オブジェクト」タブ(Blender 風)にあり、プリミティブごとにボタンがあります。小道具を開いている間は「マップへ」がワンクリックのままです。
- Ctrl+Enter でツールを適用し、結果に同じツールをもう一度始めます(ツールボックスの「+」ボタン)。
- Blender 風: E が Blender と同じく押し出しです(Alt+E も引き続き使えます)。Shift+A でプリミティブの「追加」メニューが開きます。
- モデルタブ: キャラクターに animation.cfg があるとき、モデルを変更した直後(展開の後など)にタイムラインがまれにプログラムを落とすことがありました。修正しました。モデリングと UV のパネルのボタンは、すべての語がキーの横に収まる場合だけキーを表示し、語の途中で改行しなくなりました。

## Español

- Pestaña Model, Modelado: el panel de la derecha sigue el diseño aprobado: los grupos Selección, Herramientas y Límites de JKA. Los niveles son una fila de casillas con su tecla delante (modo 3ds Max 1-5, modo Blender 1-3); las seis herramientas principales son botones de igual ancho con símbolo y la tecla del modo de control, después vienen las demás herramientas del nivel y el resto queda plegado en «Editar geometría».
- Una herramienta que no funciona en el nivel actual aparece atenuada, y su ayuda nombra los niveles en los que funciona.
- Límites de JKA: barras para la superficie seleccionada (vértices, huesos y pesos frente al límite del juego, amarillo desde el 90 %, rojo por encima) y una nota breve en el título; la lista de todas las superficies está plegada (con una figura de 83 partes ocupaba ocho páginas).
- Las primitivas y los accesorios están en la pestaña «Crear» (modo 3ds Max) u «Objeto» (modo Blender), un botón por primitiva; mientras haya un accesorio abierto, «Al mapa» sigue a un clic.
- Ctrl+Intro aplica una herramienta y la vuelve a empezar sobre el resultado (el botón «+» de la caja de herramienta).
- Modo Blender: E extruye como en Blender (Alt+E sigue funcionando), Mayús+A abre el menú «Añadir» con las primitivas.
- Pestaña Model: la línea de tiempo podía hacer que el programa se cerrara de vez en cuando justo después de un cambio del modelo (por ejemplo tras Desplegar) si la figura tiene animation.cfg; corregido. Los botones de los paneles de Modelado y UV solo muestran su tecla mientras cada palabra quepa a su lado, así ninguna palabra se corta por la mitad.

## Čeština

- Karta Model, Modelování: panel vpravo odpovídá schválenému návrhu - skupiny Výběr, Nástroje a Limity JKA. Úrovně jsou řada polí s klávesou vpředu (režim 3ds Max 1-5, režim Blender 1-3); šest hlavních nástrojů jsou stejně široká tlačítka se symbolem a klávesou režimu ovládání, za nimi následují další nástroje úrovně a zbytek je sbalený pod „Upravit geometrii“.
- Nástroj, který na aktuální úrovni nepracuje, je zašedlý a jeho nápověda jmenuje úrovně, na kterých pracuje.
- Limity JKA: pruhy pro vybraný povrch (vrcholy, kosti a váhy proti limitu hry, žlutě od 90 %, červeně nad ním) a krátká informace v nadpisu; seznam všech povrchů je sbalený (u postavy s 83 díly zabíral osm stránek).
- Základní tělesa a rekvizity jsou na kartě „Vytvořit“ (režim 3ds Max) nebo „Objekt“ (režim Blender), jedno tlačítko na těleso; dokud je otevřená rekvizita, zůstává „Do mapy“ na jedno kliknutí.
- Ctrl+Enter použije nástroj a spustí ho znovu na výsledku (tlačítko „+“ v okénku nástroje).
- Režim Blender: E vysouvá jako v Blenderu (Alt+E funguje dál), Shift+A otevře nabídku „Přidat“ se základními tělesy.
- Karta Model: časová osa mohla občas shodit program hned po změně modelu (například po rozbalení), když má postava animation.cfg; opraveno. Tlačítka v panelech Modelování a UV ukazují klávesu jen tehdy, když se vedle ní vejde každé slovo, takže se žádné slovo nerozdělí uprostřed.
