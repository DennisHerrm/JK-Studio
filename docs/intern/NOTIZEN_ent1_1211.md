# Notizen ENTSCHEID1 (12.11.): verbindliche Entscheidungen von shank im Model-Tab - rc612

Auftrag ENTSCHEID1 (Opus 5.5, ein Agent): die Entscheidungen „ENTSCHEIDUNGEN shank 11.11. spät“ (Punkte 3 und 5) und „ENTSCHEIDUNG shank
12.11. AP4“ („ja Bitte wie in Blender“) aus `naechste_auftraege.md`. Worktree `Y:\p_ent1`, Zweig `p_ent1` auf karte1 `ccecbe214`.
Selbsttests `ap1`, `ap2`, `ap4` (`BHED_EDITORTEST=modellblock`), Fotos `ap1foto`/`ap4foto`, Handbuch `BHED_EDITORTEST=handbuch`;
Skripte und Läufe `Y:\werkzeuge\ent1\`.

## Was sich ändert

| Punkt | Was | Vorbild / Quelle | Datei |
|---|---|---|---|
| 1 Auto Key | ab Werk **aus** (wie 3ds Max Auto Key Mode und Blender Auto-Keying); jede Wahl (Knopf, N wie 3ds Max, Aufnahme-Knopf wie Blender, Menü, Suche) landet in `behaved.cfg` `modelTimelineAutoKey` und gilt beim nächsten Start | shank 11.11.: „Aus man kann es an machen“ | `Settings::modellZeitAutoKey`, `modellZeitAutoKey`, `drawModellZeitleiste` |
| 2 Eingefroren | ein Klick in der Ansicht geht durch eingefrorene Flächen hindurch und wählt die vorderste nicht eingefrorene dahinter; liegt dahinter nichts, meldet die Statuszeile die eingefrorene (wie AP2) und es wird nichts gewählt. Der Rahmen ließ Eingefrorenes schon aus | Autodesk-Hilfe „Freezing and Unfreezing Objects“ (eingefrorene Objekte „cannot be selected“); Blender ebenso mit `hide_select` | `flaecheUnter`, neu `flaechenUnter`/`modellFlaechenUnter` (`app_modelltab_ansicht.cpp`) |
| 3 Seitenleiste (N) | im Reiter Gegenstand (Item) sind die Werte **Zahlenfelder wie Blender**: ziehen ändert (Ort wie G in der Ansicht, Drehung 0,5° je Punkt wie das Dreh-Gizmo, Dimension 1 % je Punkt wie das Skalier-Gizmo), ein Klick tippt ein (Enter übernimmt, Esc bricht ab), **Strg+Z nimmt eine Eingabe in einem Schritt zurück**. Fläche: Position (Mitte) und Dimensionen des Kastens in der Pose - verschiebt bzw. skaliert die Ecken wie das Gizmo. Griff des Rigs im Posenmodus: Position und Rotation (Euler XYZ) des Knochens - wie G/R, mit denselben Grenzen (Kopf nur drehen, Pol nur bewegen usw., `modellRigZugMoeglich`). Ein Knochen im Objektmodus und Werte, die der Griff nicht kann, stehen grau zum Ablesen da (Tooltip sagt warum) | Blender Sidebar Item-Panel (Location/Rotation/Dimensions, Pose Mode: aktiver Knochen); Blender-Zahlenfelder (UI_BTYPE_NUM: ziehen, Klick tippt) | `seitenFeld`, `seiteObjekt` (`app_modelltab_blender.cpp`), `modellAuswahlUmformen`, `modellRigDrehen` |
| 4 Begriffe | die Texte des Bedienmodus wie Blender in de, es, cs, ja, zh-Hans nach **Blenders eigener Übersetzung** (Blender 5.0 `datafiles/locale/*/LC_MESSAGES/blender.mo`, nur gelesen mit `Y:\werkzeuge\ent1\mo_lesen.py`, Vergleich `vergleich_blender.py`); Werkzeugnamen im Kontext „Operator“ wie Blender (`space_toolsystem_common.py`: `iface_(item.label, i18n_contexts.operator_default)`), fehlt dort ein Eintrag, bleibt unser Text. Die Werkzeugleiste trägt jetzt Blenders Werkzeugnamen (5.0 `space_toolsystem_toolbar.py`) | shank 12.11. „ja Bitte wie in Blender“ | `tools/i18n_ent1.py` (neu, Präfix `Be`), `i18n_ap4.py`, `i18n_ap3.py`, `i18n_mb1a2.py`, `i18n_mbkopf.py` |
| 5 Werkzeuggruppen | wie Blender: ein kleines Dreieck unten rechts zeigt eine Gruppe; kurzer Klick nimmt das gezeigte Werkzeug, **0,2 s halten oder ziehen** öffnet die Gruppe rechts daneben (Symbol und Name, doppelt hoch), Loslassen auf einem Eintrag wählt ihn, Loslassen daneben schließt ohne Wahl; der Knopf zeigt danach das gewählte Glied (für die Sitzung). Gruppen im Modellieren: **Region extrudieren / Entlang der Normalen extrudieren / Individuell extrudieren** (= die Arten Gruppe / Lokale Normale / Je Vieleck des Extrudieren-Kästchens, wie 3ds Max Group / Local Normal / By Polygon) und **Messer / Halbieren** (Halbieren = QuickSlice). Rechtsklick auf die Leiste (auch Rechtsklick der Ansicht und Alle Aktionen suchen): **Werkzeugnamen zeigen** - breitere Leiste mit dem Namen neben jedem Werkzeug, gemerkt in `modelBlenderToolNames` | Blender 5.0 `space_toolsystem_common.py` (`operator_menu_hold`, `WM_MT_toolsystem_submenu`, `layout.scale_y = 2.0`, `_tool_group_active`), `interface_handlers.cc` (`hold_action_timer` mit `BUTTON_AUTO_OPEN_THRESH` 0,2 s, beim Ziehen über die Ziehschwelle sofort; `BLOCK_POPUP_HOLD`: Loslassen außerhalb schließt), Commit 7c02008e „button triangle for hold action“; Ziehschwelle `U.drag_threshold_mouse` (3 Punkte) × Oberflächengröße | `leistenKnopf`, `gruppenMenue`, `modellieren`, `modellLinksSpalte` |

Finden (shank 06.11.): Werkzeugnamen zeigen im Rechtsklick der Leiste und der Ansicht und in „Alle Aktionen suchen“ (wie Blender), Tooltip an
jedem Feld und jedem Gruppenknopf („Gedrückt halten oder ziehen: die anderen Werkzeuge dieser Gruppe“); die Arten des Extrudierens bleiben
auch im Kästchen wählbar.

### Szenen-Explorer (Punkt 2) - geprüft, unverändert

Eine eingefrorene Zeile im Szenen-Explorer/Outliner bleibt nicht wählbar (Meldung wie AP2), wie shank am 11.11. für den Explorer
entschied („alles wie heute“) und der Auftrag es vorgibt. **Abweichung von 3ds Max, Frage an shank:** laut Autodesk-Hilfe „Scene
Explorer Right-Click Menus“ (3ds Max 2017, GUID-A0647085) lassen sich eingefrorene Objekte in der Liste des Scene Explorer **doch wählen**
(„Unfreeze Scene Explorer Selection: sets frozen objects that are selected in the Scene Explorer list to the unfrozen state … as it's not
possible to select frozen objects in the viewports“) - nur in den Ansichten nicht. Ein „hindurch“ gibt es in einer Liste nicht.

### Geänderte Texte nach Blenders Übersetzung (Punkt 4)

Blender-Eintrag in Klammern, wo er von unserem abwich; „-“ = Blender hat keine Übersetzung (unser Text bleibt).

| Kennung | en (neu) | de | es | cs | ja | zh-Hans |
|---|---|---|---|---|---|---|
| BzAuswaehlen | Select Box (vorher Select) | Auswahlbox | Seleccionar (Marco) | Obdélníkový výběr | ボックス選択 | 框选 |
| BzBewegen / BzDrehen | Move / Rotate | Verschieben / Rotieren | = | = | = | = |
| BzSkalieren | Scale | = | = | Změnit velikost | スケール | = |
| BzMessen | Measure | = | = | Změřit | = | = |
| BzFase | Bevel | = | Biselar | Zaoblit | = | = |
| BzMesser | Knife | = | Cortar | = | = | 切割 |
| BeExtrude (neu, Leiste) | Extrude Region | Region extrudieren | Extruir región | Vysunutí oblasti | 押し出し（領域） | 挤出选区 |
| BeExtrudeNormalen / BeExtrudeEinzeln (neu) | Extrude Along Normals / Extrude Individual | Entlang der Normalen extrudieren / Individuell extrudieren | Extruir según normales / Extruir individualmente | Vysunutí Podél Normál / Vysunout Jednotlivé | 押し出し（法線方向） / 押し出し（個別） | 沿法向挤出 / 挤出各个面 |
| BeInset (neu) | Inset Faces | - (Einfügen (Inset)) | Incrustar caras | Vsazení ploch | 面を差し込む | 内插面 |
| BeLoopCut (neu) | Loop Cut | Schleife schneiden | Cortar bucle | Výřez smyčkou | = | 环切 |
| BeHalbieren (neu) | Bisect | Halbieren (Kontext „Operator“ fehlt, Grundkontext) | Biseccionar | Půlit | 二分割 | 切分 |
| BeNaht (neu) | Mark Seam | = | Marcar como costura | = | シームをマーク | 标记缝合边 |
| BeEntspannen (neu) | Relax | = | Distender | - (Blender: „Relax“ unübersetzt) | = | = |
| BePacken (neu) | Pack Islands | - (Blender „Bild einpacken“: falscher Begriff) | Empacar islas | - (Blender „Přibalit obrázek“: falsch) | アイランドを梱包 | 拼排孤岛 |
| BeVerlauf (neu) | Gradient | = | Gradiente | = | = | 梯度渐变 |
| BeGewichtHolen (neu) | Sample Weight | Beispieleinfluss | Tomar muestra (influencia) | - | ウェイトを採取 | = |
| Painting Pinsel | Brush (MgRPinsel; Blender 5.0 heißt das Werkzeug Brush) | Pinsel (-) | Pincel | Štětec (-) | ブラシ | 笔刷 |
| BeFuellen / BeWischen / BeMaske (neu) | Fill / Smear / Mask | Füllen / - / Maskieren | = / Manchar / = | = / - / = | フィル / = / = | = |
| BeKopieren / BeGespiegelt (neu) | Copy Pose / Paste X-Flipped Pose | Pose kopieren / - | Copiar pose / Pegar pose (invertida en X) | = / - (Blender „Vložit“: unvollständig) | ポーズをコピー / X軸反転ポーズを貼り付け | 复制姿态 / 粘贴 X 轴向翻转姿态 |
| BzModusWeight / BzModusTexture | Weight Paint / Texture Paint | Malereieinfluss / Texturmalerei | Pintar Influencias / Pintar Texturas | Kreslit hmotnost / Kresba textur | = | = |
| BzModusPose | Pose Mode | Posenmodus | = | - (Blender „Pózovací ražim“: Tippfehler) | = | = |
| MbBlEditModus | Edit Mode | Bearbeitungsmodus (war schon so) | = | Režim úprav | = | = |
| BzReiterObjekt | Item | Gegenstand | = | Předmět | = | 条目 |
| BzTransform / BzOrt / BzDrehung / BzMasse | Transform / Location / Rotation / Dimensions | = / = / Rotation / Dimensionen | Transformación / Posición / = / = | = | = | = / = / = / 规格 |
| BzSichtfeld | Field of View | - (Blender „An Ansicht ausrichten“: falscher Begriff) | = | Zorné Pole | = | = |
| MkBlUserPersp / MkBlUserOrtho | User Perspective / User Orthographic | Benutzeransicht / Benutzerorthographisch | Personalizada (perspectiva) / Personalizada (ortogonal) | Perspektiva Uživatele / Ortografie Uživatele | ユーザー・透視投影 / ユーザー・平行投影 | = |
| MkBlPersp / MkBlGewaehlte / MkBlAlles | Perspective/Orthographic / Frame Selected / Frame All | - / - (Blender „Einzelbild ausgewählt“: falsch) / - | Perspectiva / Ortogonal / Encuadrar lo seleccionado / = | Perspektivní/Ortografický / Zarámovat vybrané / Zarámovat vše | 透視投影/平行投影 / 選択をフレームイン / 全てを表示 | = |
| MfModifier / MfConstraints (Eigenschaften-Reiter) | Modifiers / Constraints | Modifikatoren / Beschränkungen | = | = | = | = |
| BzWerkzeugleiste | Toolbar | = | = | - (Blender „Panelu nástrojů“: Genitiv) | = | = |

Unverändert, weil schon wie Blender: Seitenleiste, Outliner (Listado), Zusammenführen/Merge, Objektmodus, Eigenschaften, Ansicht, Werkzeug,
Objekt, Daten, Knochen, Material, Projektion. Die gemeinsamen Texte der Felder (`MdExtrude`, `UvPacken`, … im Max-Modus und in den Gruppen
des rechten Felds) bleiben; nur die Werkzeugleiste wie Blender zeigt die Blender-Namen (`Be*`). Overlays (MBKOPF) ist nicht Teil von AP4 und
blieb (Blender de „Überlagerungen“) - für AP14/MBKOPF vermerkt.

## Selbsttests (rot -> grün)

| Punkt | Test | rot (alter Stand) | grün |
|---|---|---|---|
| 1 | ap1 (neu `werkVorgabe`: ab Werk aus, Klick merkt sich, Datei, neuer Start an/aus; A12 N mit Einstellung) | 136 OK, **6 FEHLER** (`rot_ap1.txt`) | 142 / 0 |
| 2 | ap2 X13 (zwei Flächen hintereinander gesucht, vordere eingefroren -> die dahinter; Gegenprobe mit ausgeblendeter vorderer; nur eine eingefrorene Fläche -> nichts, Meldung) | 66 OK, **1 FEHLER** (`rot_ap2.txt`) | 67 / 0 |
| 3, 4, 5 | ap4 (Seitenleiste: Felder, Eintippen + Enter, Strg+Z, Esc, Ziehen, Tooltip, Knochen nur ablesbar, Becken/Kopf im Posenmodus; Werkzeugnamen je Bereich; Gruppen: Dreieck, kurzer Klick, Halten 0,2 s, Ziehen sofort, Werkzeugnamen zeigen mit Datei) | 78 OK, **28 FEHLER** (`rot_ap4.txt`) | 104 / 0 (`g2`) |
| Fotofunde | ap4: nur ablesbare Felder grau; ap4foto klein: Beschriftungen gekürzt mit … | ap4 104 / **1**, ap4foto klein 7 / **1** (`rot2_*.txt`) | ap4 105 / 0, ap4foto je 8 / 0 |

Messungen in den Läufen: Position X der Fläche torso eingetippt 3,70 -> 3,700; Becken X 5,20 -> 5,200; Rotation Z des Kopfs -64,3° ->
-64,30° (X/Y unverändert -4,98°/-79,79°); Dimension Z gezogen 16,51 -> 29,64 (X unverändert); Gruppe nach 0,200 s Halten offen, durch
Ziehen nach 0,015 s; Leiste mit Namen 92 -> 299 Punkte (längster Name 178).

Zwei Fehler der eigenen Umsetzung, im ersten grünen Lauf (`g1`, ap4 98/6) gefunden und behoben: (a) ImGui meldet beim Wechsel eines
Zahlenfelds vom Ziehen zum Eintippen (Klick ohne Ziehen, `TempInputText`) kurz „deaktiviert“ - die Eingabe ging verloren; jetzt über
`IsItemActive`. (b) Nach dem Öffnen der Gruppe gehörte die linke Maustaste bis zum Loslassen weiter dem Knopf (`SetKeyOwner` in
`ButtonBehavior`), der Eintrag sah das Loslassen nicht; jetzt `SetKeyOwner(MouseLeft, NoOwner)`.

## Angepasste vorhandene Tests (nichts gelöscht, keine Prüfung abgeschwächt)

- `gui/selbsttest_ap1.inc`: Z6 hieß „ab Werk an“, prüfte aber den Zustand nach dem Schritt, der Auto Key für die Zeilen einschaltet - jetzt
  ohne „ab Werk“ (die Vorgabe prüft `werkVorgabe`); A12 prüft zusätzlich die Einstellung; am Ende wird der Stand von vorher zurückgesetzt
  statt „an“; Handbuchbilder und Fotos zeigen Auto Key wie ab Werk (aus).
- `gui/selbsttest_ap2.inc` X13: „der Klick in der Ansicht wählt die Fläche nicht“ ist durch die Entscheidung überholt -> hindurch (siehe
  oben); der Rahmen-Schritt friert dafür torso selbst ein.
- `gui/selbsttest_ap4.inc`: die Sollfolgen der Werkzeugleiste und die Klicks auf Packen, Verlauf, Gewicht holen, Füllen, Pinsel, Pose kopieren
  nennen die Blender-Namen (`Be*`).
- mb5 lief ohne Anpassung grün (160 / 0): er setzt seine Schlüssel selbst.
- `gui/selbsttest_mb6.inc`: rot mit Auto Key aus (37 / 1, „MB6 Blender rot: ohne Einrasten erkennt die Prüfung den Sprung“: der Zug G an
  der Hand vor dem Bildwechsel wird jetzt - richtig - als Pose ohne Schlüssel verworfen, das IK-Ziel steht dann auf der FK-Pose, der
  Sprung, den die Rot-Probe sehen soll, fällt im Bild weg: Bildpunkte 0). Gegenprobe: dieselbe exe mit `modelTimelineAutoKey=true` in den
  Einstellungen 38 / 0, die exe von karte1 38 / 0. Der Test schaltet jetzt Auto Key für seinen Ablauf an und am Ende zurück.

## Fotos (angesehen)

`Y:\werkzeuge\ent1\fotos\ent1_f_<gruppe>_<variante>\` - ap4foto en, de, ja, hell (3818 × 2104 bei 1,5), klein (1328 × 844 bei 1,0, Mindestgröße):
beide Bedienmodi alle Bereiche, dazu wie Blender `ap4_blender_namen`, `ap4_blender_gruppe` (Gruppe offen), `ap4_blender_seite_felder`;
ap1foto en, de, ja, klein (Zeitleiste beider Modi, Auto Key aus). Angesehen: Gruppe offen en/klein/ja (Handbuch), Seitenleiste en/ja/hell/klein
(vor und nach den Fotofunden), Werkzeugnamen klein, Moduswahl en, Handbuchbilder h_ap1_max de (Auto Key grau), h_ap4_seitenleiste de.

## Handbuch

- `inhalt_ap1.py` (eingehängt): Auto Key ab Werk aus, die Wahl bleibt; `inhalt_ap2.py` (eingehängt): Klick in der Ansicht geht durch
  Eingefrorenes; `inhalt_ap4.py` (AP14 hängt ein): Werkzeugnamen als Kennungen, neuer Abschnitt „Werkzeuggruppen und Werkzeugnamen“ mit
  Bild `h_ap4_gruppen`, Seitenleiste eingebbar - je 6 Sprachen. `gen_handbuch.py` lädt die Texte von `i18n_ent1`.
- Bilder neu (6 Sprachen), weil Auto Key darin an war bzw. die Leiste/Seitenleiste sich änderte: h_ap1_max/_blender/_pruefen, h_ap3_*,
  h_ap4_max/_blender/_seitenleiste/_gruppen (neu), h_ap5_*, h_ap6_*, h_ap7_*, h_ap8_*, h_ap9_*, h_ap10_* (geprüft per Farbsuche nach dem
  roten Auto-Key-Knopf: nur diese Sätze; h_mb*, h_md*, h_uv*, h_pt* zeigen die Zeitleiste von vor AP1).
- `handbuch/` neu erzeugt; dabei kamen auch zwei ältere, nicht erzeugte Textänderungen anderer Pakete mit (AP5 „Prüfung für das Spiel“, RT
  „Pose zur Charakterisierung“). `gen_handbuch.py --pruefen` grün (der Hinweis „2 Einträge in uebersetzung/“ steht schon auf karte1).

## Schlussrunde des Zweigs

| Schritt | Ergebnis |
|---|---|
| Bau alle Ziele (inkrementell, /W4 /WX) | 0 Fehler, 0 Warnungen (`bau_alles`, `bau_schluss`) |
| ctest | 258 / 258 |
| ap1 / ap2 / ap4 | 142 / 0 · 67 / 0 · 105 / 0 (Lauf `n1`) |
| ap1foto, ap4foto | ap4foto en, de, ja, hell, klein je 8 / 0; ap1foto en, de, ja, klein je 13 / 0 |
| Handbuchbilder | ap1, ap3, ap4, ap5, ap6 (ap6hb), ap7, ap8, ap9, ap10 in 6 Sprachen je 1 / 0 (ap4 nach dem Fix des Halte-Schritts noch einmal) |
| Nachbargruppen | ap3 105 · ap5 120 · ap6 94 · ap7 88 · ap8 104 · ap9 97 · ap10 70 · mb5 160 · kopf 116 · nav 133 · blender 68 · funktionen 62 · tasten 44 · uv 51 · raster 41 · mbv 216 · md 87 · pt 42 (`n1`) · mb2 38 · mb3 22 · mb7 25 · rig 27 · ap12 35 · kopfrot 26 · ph 30 · rt 20 (`n2`) · mb6 38 (`n4`, nach Anpassung) · ai 32 · pfix 18 (`n5`) - je 0 FEHLER; Modi dock1 99 · dock4 116 · tabfenster 22 · karteneditor hilfe2 (Touren, u. a. k-skins im Model-Tab) 56 - je 0 FEHLER (danach die bekannte Zeile „Beenden: Programm läuft nach No weiter … Exit Radiant“ wie bei AP3-AP5) |
| Lints (21 aus `lints.sh`, dazu efxflags, fremdprogramm, d3dbesitz, merkmalsbits) | wie karte1: pruefcodes 11, portability 99, efxflags und fremdprogramm mit derselben Meldung; settings ok mit 2 Schlüsseln mehr (112); i18n ok |
| Handbuch | `gen_handbuch.py --pruefen` grün, `handbuch/` neu erzeugt |

Bewusst nicht gelaufen (einmal auf dem zusammengeführten Stand): Modus 1, übrige modellblock-Gruppen (Rundläufe, uvmd3, mdkarte,
pt-Varianten …), karteneditor voll, menueeditor, voller Handbuch-Bildlauf, sauberer Neubau, Sprachläufe. Keine Bildzeit-Messung: die
Werkzeugleiste zeichnet dieselbe Zahl Knöpfe (mit Namen je ein Text mehr), die Seitenleiste nur, wenn sie an ist; der Klick sortiert die
Treffer einer Stelle (eine Liste je Klick).

## Offen / Fragen an shank

1. **Szenen-Explorer:** 3ds Max lässt eingefrorene Objekte in der Liste wählen (siehe oben) - so lassen (nicht wählbar, wie 11.11.
   entschieden) oder wie 3ds Max in der Liste wählbar?
2. **Werkzeugnamen zeigen:** Blender hat dafür keinen Menüpunkt - es zeigt die Namen, wenn man die Leiste breiter als 120 Punkte zieht
   (`space_toolsystem_common.py`). Umgesetzt ist der entschiedene Rechtsklick; soll zusätzlich das Breiterziehen der Kante die Namen zeigen?
3. **Blender-Übersetzungen mit Fehlern** (nicht übernommen): de „Bild einpacken“ (Pack Islands), „An Ansicht ausrichten“ (Field of View),
   „Einzelbild ausgewählt“ (Frame Selected); cs „Přibalit obrázek“, „Metoda rozbalení“, „Panelu nástrojů“, „Pózovací ražim“, „Vložit“ - so
   lassen? Übernommen, obwohl ungewohnt: de „Malereieinfluss“ (Weight Paint), „Beispieleinfluss“ (Sample Weight), „Gegenstand“ (Item).
4. Seitenleiste: Werte in Weltachsen (wie AP4 sie zeigte); Blender zeigt für Knochen die lokalen Kanäle (Location/Rotation relativ zur
   Ruhelage, Quaternion ab Werk) - gewünscht? Maßstab gibt es nicht: die Griffe des Rigs skalieren nicht, Flächen haben keinen eigenen
   Maßstab (wie Blenders Median im Bearbeitungsmodus).
- Weitere Gruppen hat das Programm nicht (Blender: Auswahl Box/Kreis/Lasso, Schleifenschnitt/Offset, Gewicht holen/Vertexgruppe - die
  Werkzeuge fehlen).
- Vorgefunden, nicht angefasst: die gemerkte Art des Extrudieren-Kästchens gilt auch für Einfügen (Inset hat nur zwei Arten; nach
  „Individuell extrudieren“ = Art 2 zeigt dessen Auswahl leer); Unterobjekt-Klicks (Ecke/Kante/Dreieck) nehmen eingefrorene Flächen
  weiter (die Entscheidung galt dem Klick auf eine Fläche).
- Für AP14: Seite `modell-blender` einhängen (Bild h_ap4_gruppen neu), Kopf „Overlays“ nach Blender de.

## Changelog

## English

- Model tab: Auto Key (like Blender Auto Keying) is off from the start, as in 3ds Max and Blender; the program remembers your choice.
- A click in the view goes through a frozen surface and selects the surface behind it, like 3ds Max.
- Like Blender the sidebar (N) has number fields: drag or click to type the location and dimensions of a surface or the location and rotation of a rig control in pose mode; Ctrl+Z undoes one input in one step.
- The toolbar like Blender uses Blender's tool names and, in German, Spanish, Czech, Japanese and Chinese, Blender's own translations.
- Tool groups like Blender: a small triangle marks a group, holding or dragging opens it (Extrude Region / Along Normals / Individual, Knife / Bisect); a right-click on the toolbar shows the tool names.

## Deutsch

- Model-Tab: Auto Key (wie Blender Auto-Keying) ist ab Werk aus wie in 3ds Max und Blender; das Programm merkt sich die Wahl.
- Ein Klick in der Ansicht geht durch eine eingefrorene Fläche hindurch und wählt die Fläche dahinter, wie 3ds Max.
- Wie Blender hat die Seitenleiste (N) Zahlenfelder: Position und Dimensionen einer Fläche bzw. Position und Rotation eines Griffs im Posenmodus ziehen oder eintippen; Strg+Z nimmt eine Eingabe in einem Schritt zurück.
- Die Werkzeugleiste wie Blender trägt Blenders Werkzeugnamen und auf Deutsch, Spanisch, Tschechisch, Japanisch und Chinesisch Blenders eigene Übersetzungen.
- Werkzeuggruppen wie Blender: ein kleines Dreieck zeigt eine Gruppe, Halten oder Ziehen öffnet sie (Region / Entlang der Normalen / Individuell extrudieren, Messer / Halbieren); ein Rechtsklick auf die Leiste zeigt die Werkzeugnamen.

## 中文

- 模型选项卡: Auto Key (像 Blender 时为自动插帧) 默认关闭, 与 3ds Max 和 Blender 相同; 程序会记住您的选择。
- 在视图中单击会穿过冻结的面, 选中其后面的面, 与 3ds Max 相同。
- 像 Blender 一样, 侧栏 (N) 提供数字字段: 拖动或输入面的位置和尺寸, 或姿态模式下绑定控制器的位置和旋转; Ctrl+Z 一步撤销一次输入。
- 像 Blender 的工具栏使用 Blender 的工具名称, 德语、西班牙语、捷克语、日语和中文使用 Blender 自己的翻译。
- 像 Blender 的工具组: 小三角标出一个组, 按住或拖动即可打开 (挤出选区 / 沿法向挤出 / 挤出各个面, 切割 / 切分); 右键单击工具栏可显示工具名称。

## 日本語

- モデルタブ: Auto Key (Blender 風では自動キー挿入) は 3ds Max や Blender と同じく最初はオフです。選択は記憶されます。
- ビューでのクリックはフリーズしたサーフェスを通り抜け、その後ろのサーフェスを選択します (3ds Max と同じ)。
- Blender と同じくサイドバー (N) は数値フィールドです: サーフェスの位置と寸法、ポーズモードのリグコントロールの位置と回転をドラッグまたは入力でき、Ctrl+Z で 1 回の入力を 1 段階で元に戻します。
- Blender 風のツールバーは Blender のツール名を使い、ドイツ語、スペイン語、チェコ語、日本語、中国語では Blender 自身の翻訳を使います。
- Blender と同じツールグループ: 小さな三角がグループを示し、長押しかドラッグで開きます (押し出し（領域）/（法線方向）/（個別）、ナイフ / 二分割)。ツールバーを右クリックするとツール名を表示できます。

## Español

- Pestaña Model: Auto Key (como Blender, claves automáticas) viene desactivado, como en 3ds Max y Blender; el programa recuerda tu elección.
- Un clic en la vista atraviesa una superficie congelada y selecciona la superficie de detrás, como 3ds Max.
- Como Blender, la barra lateral (N) tiene campos numéricos: arrastra o escribe la posición y las dimensiones de una superficie o la posición y la rotación de un control del rig en modo pose; Ctrl+Z deshace una entrada en un paso.
- La barra de herramientas como Blender usa los nombres de herramientas de Blender y, en alemán, español, checo, japonés y chino, las traducciones propias de Blender.
- Grupos de herramientas como Blender: un pequeño triángulo marca un grupo, mantener pulsado o arrastrar lo abre (Extruir región / según normales / individualmente, Cortar / Biseccionar); un clic derecho en la barra muestra los nombres de las herramientas.

## Čeština

- Karta Model: Auto Key (jako Blender automatické klíčování) je od začátku vypnutý jako ve 3ds Max a v Blenderu; program si volbu zapamatuje.
- Kliknutí v pohledu projde zmrazenou plochou a vybere plochu za ní, jako 3ds Max.
- Jako Blender má postranní panel (N) číselná pole: polohu a rozměry plochy nebo polohu a rotaci ovladače rigu v režimu pózy lze táhnout nebo napsat; Ctrl+Z vrátí jedno zadání v jednom kroku.
- Panel nástrojů jako Blender používá názvy nástrojů Blenderu a v němčině, španělštině, češtině, japonštině a čínštině vlastní překlady Blenderu.
- Skupiny nástrojů jako Blender: malý trojúhelník označuje skupinu, podržení nebo tažení ji otevře (Vysunutí oblasti / Podél Normál / Jednotlivé, Nůž / Půlit); kliknutí pravým tlačítkem na panel ukáže názvy nástrojů.
