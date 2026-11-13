# Model-Tab nach dem freigegebenen Entwurf (rc612): Changelog aller Pakete, zusammengeführt

Zusammengeführt von AP14 am 12.11.2026 aus den Notizen der Pakete (`NOTIZEN_ap1_1011.md` … `NOTIZEN_ap12_1011.md`,
`NOTIZEN_ent1_1211.md`, `NOTIZEN_ent2_1211.md`, `NOTIZEN_ap13s_1211.md`, `NOTIZEN_ap13k_1211.md`, `NOTIZEN_fix612c_1211.md`, `NOTIZEN_fix612b_1211.md`) und `NOTIZEN_ap14_1211.md`. Erzeugt mit `Y:\werkzeuge\ap14\changelog_bauen.py`;
die Zeilen stehen wörtlich wie in den Paketen, geordnet nach dem Tab als Ganzem und dann nach den Bereichen in der Reihenfolge
der Bereichsleiste.

Gegenüber den Paketen geändert:
- Weggelassen: AP6 „die Zeitleiste konnte nach einer Änderung des Modells abstürzen“ - der Fehler steckte in der Zeitleiste von
  AP1 und ist nie veröffentlicht worden (rc611 hat die alte Zeitleiste).
- AP3, Gruppen in einer Form: der Satz „in Ansehen/Prüfen nutzen … sie schon“ entfällt - seit AP5-AP10 nutzen alle Bereiche die Form.
- AP5, deutsch: „Überlagerungen“ statt „Overlays“ (AP14).
- Einsortiert, nicht umgeschrieben: was ENT1 und ENT2 an AP1, AP2, AP4 und AP10 geändert haben, steht direkt hinter der Zeile des
  Pakets (Auto Key aus ab Werk hinter „Auto Key lässt sich ausschalten“, Control Rig wie CAT hinter „Rig erstellen“).

Mit den Rebases auf karte1 `133c47078` und `1c6172246` dazugekommen: AP13S (Modifier-Stapel, Bereich Modellieren), AP13K (eigener
Abschnitt Knochen und Rig-Stile), FIX612C (Animation, Painting, Tab als Ganzes), FIX612B (Weights). Noch nicht enthalten (laufen
parallel, eigene Notizen): AP11 (Modelle in Tabs), KLEIN, FIX612.


## English


### The tab as a whole

- Model tab, right panel: it now follows the control mode in its look. Like 3ds Max it is the command panel with the tabs Create, Modify, Hierarchy, Motion, Display and Utilities; like Blender it is the properties editor with a vertical column of symbols (Tool, Object, Modifiers, Data, Bone, Constraints, Material). Every area opens its tab and shows all its groups there; the other tabs show their part.
- Groups in one form everywhere: a heading that folds, with a short info on the right (triangles, drawn LOD, number of attached models, findings of the check), buttons in a grid of two equal columns with symbol and key, values as a table, limits as bars, hints in a box.
- The panel is wider (22 instead of 17 character heights) and its width can be dragged at the edge to the views; a double-click restores the default.
- The areas View/Check, Modeling, UV, Weights, Painting and Animation stand as one row of equally wide fields across the whole tab, with a short hint for the control mode and the area next to them.
- The switch like 3ds Max / like Blender has moved out of the panel: it is in the status line of the tab, in the menu of the header of a view, in the settings and in the search for all actions.
- Model tab, right panel: value tables, buttons and switches in a narrow panel wrap only between words; a grid of buttons takes fewer columns when a word would not fit.
- Model tab, right panel: in a small window the tabs of the command panel like 3ds Max stand as symbols in one row, with the name in the tooltip (like Blender's properties tabs) - as text they needed two or three rows there.
- Bottom right in the view: level of detail and drawn triangles. Like 3ds Max, Modeling, Weights, Painting and Animation start with four views; like Blender Z chooses the shading and Shift+Alt+Z hides the overlays. A .glm always opens in View/Check.
- Model tab like 3ds Max: Select bone is a switch like Measure distance - in Search all actions and the context menu (with a check mark), a hint at the top of the view while it is on; Q, W, E and R end it.

### Scene explorer

- Model tab, scene explorer: one tree for everything - the model, its surfaces with their hierarchy, tags with attached models, bones, rig handles and the sequences of the animation.cfg (the separate animation list is gone). The column is as wide as in the approved design.
- Like 3ds Max: the arrow, the eye and the symbol stand directly in front of the name, each level is indented by one symbol, a "Frozen" column with a snowflake, column headings that sort and can be dragged. Like Blender: an outliner with the eye and the "selectable" arrow in columns on the right and a filter popover; the switch reads Hierarchy / Collections.
- Freezing: frozen surfaces, tags and bones stay visible but cannot be selected in the tree or in the view; the caps are frozen from the start. "By layers" shows Body, Caps, Tags, Bones, Attachments and the further LODs, each with an eye and a snowflake.
- A click in the view goes through a frozen surface and selects the surface behind it, like 3ds Max.
- Sequences in the tree show the lock P / S for the primary and secondary sequence and a warning when their frames lie behind the end of the GLA; group list, copy the name, into the script.
- The tree never scrolls sideways: deep bone chains move closer together, long names end with "..." and stand in full in the tooltip together with the info of the row. Filter by kind, Shift+click selects a range, "Info" in the right-click menu.
- The tooltips of the mode bar on the left no longer pop up over the neighbouring panel: they appear after a short rest, next to the bar at the height of the button.

### Layout like Blender

- Model tab like Blender: the column on the left is the toolbar (T) with Select, Move, Rotate, Scale and the tools of the area - Measure, Select surface, Select bone, Photo; Extrude, Inset, Bevel, Loop Cut, Knife, Merge; Mark seam, Unwrap, Relax, Pack; Brush, Blur, Gradient, Sample weight; Draw, Fill, Clone, Smear, Mask; Copy pose, Paste mirrored, Reduce.
- The toolbar like Blender uses Blender's tool names and, in German, Spanish, Czech, Japanese and Chinese, Blender's own translations.
- Tool groups like Blender: a small triangle marks a group, holding or dragging opens it (Extrude Region / Along Normals / Individual, Knife / Bisect); a right-click on the toolbar shows the tool names.
- Like Blender the outliner is at the top right above the properties editor; the edge between them can be dragged.
- The header of the view names the mode like Blender (Object Mode, Edit Mode, Weight Paint, Texture Paint, Pose Mode); a click on it changes the area.
- N opens the sidebar: location and rotation or dimensions of the selection, the last measurement, field of view and projection.
- Like Blender the sidebar (N) has number fields: drag or click to type the location and dimensions of a surface or the location and rotation of a rig control in pose mode; Ctrl+Z undoes one input in one step.
- New tool Measure: two clicks on the model or a drag show the distance in game units; like 3ds Max as Measure distance in Search all actions and the context menu.
- Each control mode keeps its own arrangement of the windows; switching keeps selection, sequence and frame. The layout like 3ds Max is unchanged.
- German: the menu Overlays like Blender is called Überlagerungen, as in Blender's German interface.

### Timeline

- Model tab: the timeline now looks like the approved design and follows the chosen controls. Like 3ds Max: To start, Previous frame, Play, Next frame, To end, frame display from 0, Auto Key (N), Set Key (K), Time configuration, a fine line per frame in the scale and in every track, keys as bars in the colours of the track bar (red = move, green = rotate). Like Blender: jumps to the previous and next key (↓ / ↑), record button for auto keying, Start / End / Frame from 1, keys as diamonds, K opens the keying menu.
- Model tab: the tracks (dope sheet, curve editor) are part of the timeline - one ruler, frame 0 at the same place in scale and tracks; the handle above the timeline or the Tracks button opens them, in the area Animation they are open from the start. View/Check shows speed, interpolation, force loop and the primary and secondary sequence; the other areas show the short timeline.
- Model tab: Auto Key can be switched off - a pose then stays without a key until Set Key (K, like Blender I) keeps it; changing the frame discards it, as in 3ds Max and Blender.
- Model tab: Auto Key (like Blender Auto Keying) is off from the start, as in 3ds Max and Blender; the program remembers your choice.
- Model tab: status line at the bottom of the timeline (controls - click to switch, keys of the area, undo and search); Time configuration with frame rate, time display in frames or seconds, playback and the animation.cfg line of the sequence; the skin list and the surface counter moved to the group Level of detail and skin.
- Model tab: in a small window the timeline no longer takes almost half of the height - below 50 character heights of window height it keeps its share, the tracks get lower and scroll, and the panels above show more than their headings.

### View/Check

- Model tab, View/Check: the groups of the design - Model (with Import…), Selection, Level of detail and skin, Floor and camera, Attachments, Check for the game, Scene.
- All ten levels of detail can be chosen like r_lodbias in the game; a level above the last one of the model shows the last one, and the group says so. Skins are a choice in two columns.
- Floor and camera: floor at -24 or under the lowest point of the pose, field of view 10°, 80° or 90°, plain background, weight threshold, Reset view and Photo (1920+).
- Check for the game: bars of the selected surface against the limits of the target game, surfaces, sequences within the GLA, the tags the game code looks for, shaders without image, LODs and surfaces without their own shadow.
- Scene: save model, skin, attachments, sequences and switched surfaces as a ModView script (.mvs) and read ModView scripts.
- The five missing check displays of ModView are in the header menus of the view: alpha blending, texture filter, bone weight threshold, highlight surface with its bone references, non-shadow-casting surfaces.

### Modeling

- Model tab, Modeling: the panel on the right follows the approved design - the groups Selection, Tools and JKA limits. The levels are one row of fields with their key in front (3ds Max mode 1-5, Blender mode 1-3); the six main tools are buttons of equal width with a symbol and the key of the control mode, the further tools of the level follow, the rest is folded under "Edit Geometry".
- A tool that does not work on the current level is greyed out, and its tooltip names the levels it works on.
- JKA limits: bars for the selected surface (vertices, bones and weights against the limit of the game, yellow from 90 %, red above) and a short note in the heading; the list of all surfaces is folded away (for a figure with 83 parts it used to fill eight pages).
- Primitives and props stand on the tab "Create" (3ds Max mode) or "Object" (Blender mode), one button per primitive; while a prop is open, "Into the map" stays one click away.
- Ctrl+Enter applies a tool and starts it again on the result (the "+" button of the tool caddy).
- Blender mode: E extrudes as in Blender (Alt+E still works), Shift+A opens the "Add" menu with the primitives.
- Model tab: every surface has a modifier stack. In 3ds Max mode the group "Modifier stack" stands at the top of the tab Modify, in the Modeling and in the Weights area, and reads from the bottom up: the surface of the file, Edit Poly, the modifiers, Skin - a click on Edit Poly or Skin switches the area. In Blender mode the group "Modifiers" stands on the tab Modifiers and lists the modifiers from the top down.
- Two modifiers: Symmetry (Blender mode: Mirror) cuts the surface at an axis plane and mirrors one side onto the other, including the bone weights; TurboSmooth (Blender mode: Subdivision) subdivides and rounds the surface one to three times and can leave open borders in place.
- Each modifier has the eye (on/off) and "Write to the file"; smoothing is a preview only at first. Modifiers can be moved up and down (also by dragging), applied (Blender mode: Apply, 3ds Max mode: Collapse To and Collapse All) and removed; each of these is one undo step. The commands are also in the right-click menu of a row, in the right-click menu of the view and in "Search all actions".
- What the view shows for a modifier is exactly the mesh that applying it produces. As long as a surface has no modifier that goes into the file, saving writes the same file as before.
- JKA limits and the check before saving count what goes into the file, including modifiers that are written to it.
- Modeling, Symmetrize: the mirrored side now hangs on the bones of its own side for every bone pair of the skeleton (lhumerus and rhumerus, ltibia and rtibia ...); until now only names beginning with l_ and r_ were swapped.

### UV

- Model tab, UV area: the panel on the right follows the approved design - Selection, then the numbered steps 1 Set seams, 2 Unwrap, 3 Clean up, then Check. Buttons of equal width in two columns with symbol and the key of the control mode, Unwrap as the highlighted main button, a tip box in the steps.
- Selection shows the texture and the number of islands in its heading; new: Select island (extends the selection to whole islands) and Select all.
- Quick unwrap and Project are one button each; the template (Character, Prop, Hard surface) and the projection (Planar, Cylinder, Sphere, Box) are chosen right below. Commands needed less often are folded away under More in each step.
- Check: the heading says "all good" or the number of findings; stretch, overlaps, the new texture use in percent and the texel density stand as a table; new switch Thick seams for the UV editor.
- UV area: Thick seams now also thickens the seams in the 3D view.
- Like Blender: L selects the island under the mouse, A selects everything, Ctrl+A matches the density, Ctrl+P packs. Like 3ds Max: Ctrl+A selects everything.

### Weights and Auto-skin

- Model tab, Weights area: the panel on the right follows the approved design. Like 3ds Max: modifier stack, weight tool, mirror, auto-skin, bone, JKA limits and closed advanced parameters. Like Blender: brush, options, clean up, mirror, auto-skin, bone, JKA limits.
- Weight tool like 3ds Max: Paint weights switches painting on and off; the fixed values 0 to 1 set the weight on the selected vertices at once; new Scale, Blend, Grow, Shrink, Ring and Loop.
- Weights: the main button Paint weights keeps its place and height when painting is off (only its frame is highlighted then).
- New brushes like Blender: Gradient (linear or radial) and Smear; Auto-normalize keeps the weights of a vertex at 1 (on from the start).
- Clean up like Blender: Normalize all, Clean, Smooth and Limit: 4, on the selected vertices or on all.
- Mirror like 3ds Max (mirror mode left blue, right green, without counterpart yellow; blue to green, green to blue, swap sides, selection only, tolerance) and like Blender (mirror group or all groups, flip names, topology mirror); vertices at the same place on different surfaces count as one.
- Bones can be locked (like Blender: K opens the lock menu); the bones are visible in this area from the start; the limits of the game stand as bars.
- Model tab, Weights: Auto-skin. "Weight automatically" gives a mesh its bone weights from the skeleton, in one undo step. Three methods: transfer from an original of the game with the same skeleton (for example kyle), geodesic (the distance to each bone is measured through the inside of the mesh over voxels, so an arm does not stick to the body), envelope (straight distance, fast). Twist bones (lhumerusX, lradiusX, lfemurX) carry the half of their limb towards the next joint, as in Raven's figures.
- Auto-skin: the result always fits the game - at most 4 weights per vertex, at most 32 bones per surface, stored in 10 bits as the game reads them; vertices at the same place get the same weights, so seams between surfaces stay closed.
- Auto-skin: settings for resolution, stiffness, smoothing, seams, twist bones, face and attachments, mirroring afterwards and "selected surfaces only" (a new piece of armour on a finished figure). The window shows the result: the limits of the game and how far the vertices lie from the weights before, in units.
- Auto-skin: "Weight automatically" and "Auto-skin: settings" are in the search of all actions (X, F3 in Blender mode) and in the right-click of the view under Weights.
- Model tab, Weights: saving writes the weights the view shows, also while paint lists are open (after painting or a weight command, before Apply). The file gets at most four weights per vertex, normalized like the game reads them; the paint lists stay open. The check before saving and the JKA limits measure the same. Until now saving wrote the weights from before painting and said "saved".
- Weights: while a stroke, gradient, gizmo drag or tool box is open, the level of detail stays, even if the wheel zooms or Auto would change it. Mirror painting could write to wrong vertices or crash when the level changed during a stroke.
- Weights and Modeling: a selection of vertices, edges or polygons belongs to its level of detail; after a change of level it is empty, and it comes back with that level as long as its mesh was not changed. Until now the numbers stayed and commands changed other vertices.
- Modeling: a tool on a surface with painted, not yet applied weights keeps them (until now they jumped back).
- Auto-Skin: "Selected surfaces only" with "Mirror afterwards" mirrors only into the selected surfaces and leaves open paint lists of other surfaces open.
- Ctrl+Z during a weight stroke, gradient, gizmo drag or texture paint stroke ends it and takes it back; Ctrl+Y brings it back. Until now the held button kept writing without an undo step.
- Smaller fixes: a command that changes nothing keeps Redo; a stroke that starts beside the figure paints with the chosen mode; locked bones are unlocked for a newly loaded model; a typed brush radius of 0 in Painting counts as 1.

### Painting

- Model tab, Painting: the right panel follows the approved design. The groups Target, Tools, Brush, Layers and For the game stand in the order of the work; tools are a grid of buttons with symbols (the chosen one is highlighted), colour, radius, strength and spacing are rows with sliders, and hardness, mirroring, pen pressure, back faces and bleed are under Advanced.
- Layers are rows with an eye to show or hide them and a lock on the original; blend mode and opacity of the selected layer stand below the list, a tip box below the group.
- The keys [ and ] make the brush smaller and larger in both control modes (also in the search for all actions and in the status line).
- Like 3ds Max the groups stand in the tab Modify (For the game also under Utilities), like Blender in the tab Tool (Target also under Material).
- Long paths are shortened at the front (".../kyle/body.tga"), the output folder shows the end of its path; the full path is in the tooltip.
- Model tab like 3ds Max, Painting: the search and the Navigation menu name Ctrl+= and Ctrl+- for zooming there, because [ and ] set the brush size.

### Animation

- Model tab, Animation area: the panel on the right follows the approved design - Animation, Create rig, Presets, Edit existing animation, Transfer (retargeting), IK/FK, Attachments and preview, Build for the game; behind them, closed, everything else (auto physics, pose, ghosts, motion paths, curves, pose library, GLA, animation.cfg, target GLA and slots).
- Create rig: shows skeleton, scale and the recognised template; a template can be chosen by group (human, rancor, wampa, tauntaun, howler, AT-ST, droids, vehicles …) or Own skeleton. New: Animation rig (FK only) and Put on rig (the animation becomes keys on the controls, the pose stays).
- Model tab, Animation: like a CAT rig, Create control rig puts an animation of the sequence onto the rig from FK to IK - hands and feet follow their IK targets and poles, the motion looks the same (the message names the largest deviation). Then you work on the controls (hand or foot target, pole, foot roll) at any frame; only that frame changes.
- Presets: the library comes first - category, list with source and licence, target slot as a choice, Preview on figure and Put on figure (retarget); the generator opens with its button.
- Presets show the eight categories of the design (Idle, Walk, Run, Backwards, Crouch, Jump, Combat, Gestures, then Other); a category without presets yet shows a short note.
- The own generator stands in the list as Walk (generated) and Run (generated): speed and arm swing under the list, Preview on figure and Put on figure as for presets, Generator... for the rest; right-click on an entry offers its commands.
- Edit existing animation: Reduce frames to keys and Bake into the GLA in one group with source, frames and keys.
- IK/FK like 3ds Max: per limb its state in words and one IK/FK button; like Blender: slider and both snap buttons in one row.
- IK/FK per limb like CAT: the button switches the limb from the current frame up to the next switch and matches the other side first - nothing jumps.
- Build for the game shows the root travel of the sequence (how far the pelvis moves) and reminds of SET_ORIGIN.
- Animation, group for the map GLA: the fields Target, Map and Name now start at the same edge.
- Light theme: the selected preset is readable again.
- Model tab, Animation: Animation rig (FK only) after Create control rig bakes the IK of every limb into FK keys first - the motion stays exactly the same, every rotation control turns its bone, and Ctrl+Z brings the IK targets back in one step.
- Model tab: fixed a gizmo drag on a rig control that stayed open when Q removed the gizmo; fixed Ctrl+Z while dragging the speed of a generated walk; fixed the Scene Explorer drawing rows after detaching an attachment; the chosen tab of the right panel stays after a very small window.

### Bones and rig styles

- Model tab: bones are drawn in the picture of the view. Like 3ds Max as bone boxes (with fins on demand), like Blender with Display as: Octahedral, Stick, B-Bone, Envelope or Wire.
- Bones and controls lie in front of the figure by default; switch In front off and the figure hides them.
- See-through: Alt+X like 3ds Max (the figure in every view), Alt+Z like Blender (X-ray of the active view). The figure lies half opaque over what is behind it.
- Rig style in the Animation area: Standard, Control rig with rings, CAT boxes or Blender octahedra. Controls are coloured by side; the style changes only the look, never the rig.
- A click or drag hits the visible shape of a bone or control: a ring, a box edge or inside, the sphere of a pole, an octahedron.
- Everything looks the same under Direct3D 12 and Direct3D 11.

### Helper, search and guide

- Model tab, Search all actions: actions with the same name in several areas name their area on the right (Smear - Weights, Smear - Painting); like Blender, Play/Stop stands once.
- The helper (What do you want to do?) knows every group, button, switch and slider of the right panel of the Model tab and leads there: Model tab, area, group, element.
- Guide: the pages of the Model tab follow the order of the work - overview, controls, layout like Blender, timeline, files, then the areas View/Check, Modeling, UV, Weights, Auto-skin, Painting and Animation; new pages for the layout like Blender, View/Check, the Animation panel and Auto-skin.

## Deutsch


### Der Tab als Ganzes

- Model-Tab, rechtes Feld: es folgt jetzt auch im Aussehen dem Bedienmodus. Wie 3ds Max ist es das Befehlsfeld mit den Reitern Erstellen, Ändern, Hierarchie, Bewegung, Anzeige und Dienstprogramme; wie Blender der Eigenschaften-Editor mit einer senkrechten Spalte aus Symbolen (Werkzeug, Objekt, Modifier, Daten, Knochen, Constraints, Material). Jeder Bereich öffnet seinen Reiter und zeigt dort alle seine Gruppen; die anderen Reiter zeigen ihren Teil.
- Gruppen überall in einer Form: eine Überschrift zum Zuklappen mit Kurzinfo rechts (Dreiecke, gezeichnete LOD, Zahl der Anbauten, Befunde der Prüfung), Knöpfe im Raster aus zwei gleich breiten Spalten mit Symbol und Taste, Werte als Tabelle, Grenzen als Balken, Hinweise im Kasten.
- Das Feld ist breiter (22 statt 17 Zeichenhöhen), und seine Breite lässt sich an der Kante zu den Ansichten ziehen; ein Doppelklick stellt die Vorgabe wieder her.
- Die Bereiche Ansehen/Prüfen, Modellieren, UV, Weights, Painting und Animation stehen als eine Reihe gleich breiter Felder über dem ganzen Tab, daneben ein kurzer Hinweis für Bedienmodus und Bereich.
- Der Schalter wie 3ds Max / wie Blender ist aus dem Feld gewandert: er steht in der Statuszeile des Tabs, im Menü der Kopfzeile einer Ansicht, in den Einstellungen und in der Suche nach allen Aktionen.
- Model-Tab, rechtes Feld: Wertetabellen, Knöpfe und Schalter brechen in einem schmalen Feld nur zwischen Wörtern um; ein Knopfraster nimmt weniger Spalten, wenn ein Wort sonst nicht passt.
- Model-Tab, rechtes Feld: in einem kleinen Fenster stehen die Reiter des Befehlsfelds wie 3ds Max als Symbole in einer Reihe, der Name im Tooltip (wie Blenders Eigenschaften-Reiter) - als Text brauchten sie dort zwei oder drei Reihen.
- Unten rechts in der Ansicht: Detailstufe und gezeichnete Dreiecke. Wie 3ds Max beginnen Modellieren, Weights, Painting und Animation mit vier Ansichten; wie Blender wählt Z die Schattierung und Umschalt+Alt+Z blendet die Überlagerungen aus. Eine .glm öffnet immer in Ansehen/Prüfen.
- Model-Tab wie 3ds Max: Knochen wählen ist ein Schalter wie Abstand messen - in Alle Aktionen suchen und im Rechtsklick (mit Haken), oben in der Ansicht ein Hinweis, solange es an ist; Q, W, E und R beenden es.

### Szenen-Explorer

- Model-Tab, Szenen-Explorer: ein Baum für alles - das Modell, seine Flächen mit ihrer Hierarchie, Tags mit angebauten Modellen, Knochen, Rig-Griffe und die Sequenzen der animation.cfg (die eigene Animationsliste entfällt). Die Spalte ist so breit wie im freigegebenen Entwurf.
- Wie 3ds Max: Pfeil, Auge und Symbol stehen direkt vor dem Namen, jede Ebene ist um ein Symbol eingerückt, Spalte „Eingefroren“ mit Schneeflocke, Spaltenköpfe zum Sortieren und Ziehen. Wie Blender: ein Outliner mit Auge und Pfeil „wählbar“ rechts in Spalten und einem Filter-Ausklapper; der Umschalter heißt Hierarchie / Sammlungen.
- Einfrieren: eingefrorene Flächen, Tags und Knochen bleiben sichtbar, lassen sich aber in Baum und Ansicht nicht wählen; die Kappen sind von Anfang an eingefroren. „Nach Ebenen“ zeigt Körper, Kappen, Tags, Knochen, Anbauten und die weiteren LODs, jede mit Auge und Schneeflocke.
- Ein Klick in der Ansicht geht durch eine eingefrorene Fläche hindurch und wählt die Fläche dahinter, wie 3ds Max.
- Sequenzen im Baum tragen das Schloss P / S für Primär- und Sekundärsequenz und ein Warnzeichen, wenn ihre Bilder hinter dem Ende der GLA liegen; Gruppenliste, Namen kopieren, ins Skript.
- Der Baum rollt nie seitwärts: tiefe Knochenketten rücken enger zusammen, lange Namen enden mit „...“ und stehen ganz im Tooltip, zusammen mit der Info der Zeile. Filter nach Art, Umschalt+Klick wählt einen Bereich, „Info“ im Rechtsklick-Menü.
- Die Tooltips der Modusleiste links springen nicht mehr über das Nachbarfeld: sie erscheinen nach kurzer Ruhe neben der Leiste auf Höhe des Knopfs.

### Aufbau wie Blender

- Model-Tab wie Blender: die linke Spalte ist die Werkzeugleiste (T) mit Auswählen, Bewegen, Drehen, Skalieren und den Werkzeugen des Bereichs - Messen, Fläche wählen, Knochen wählen, Foto; Extrudieren, Einfügen, Fase, Schleifenschnitt, Messer, Zusammenführen; Naht, Abwickeln, Entspannen, Packen; Pinsel, Weichzeichnen, Verlauf, Gewicht holen; Malen, Füllen, Klonen, Verwischen, Maske; Pose kopieren, Gespiegelt einfügen, Ausdünnen.
- Die Werkzeugleiste wie Blender trägt Blenders Werkzeugnamen und auf Deutsch, Spanisch, Tschechisch, Japanisch und Chinesisch Blenders eigene Übersetzungen.
- Werkzeuggruppen wie Blender: ein kleines Dreieck zeigt eine Gruppe, Halten oder Ziehen öffnet sie (Region / Entlang der Normalen / Individuell extrudieren, Messer / Halbieren); ein Rechtsklick auf die Leiste zeigt die Werkzeugnamen.
- Wie Blender steht der Outliner oben rechts über dem Eigenschaften-Editor; die Kante dazwischen lässt sich ziehen.
- Der Kopf der Ansicht nennt wie Blender den Modus (Objektmodus, Bearbeitungsmodus, Weight Paint, Texture Paint, Pose-Modus); ein Klick darauf wechselt den Bereich.
- N öffnet die Seitenleiste: Position und Drehung bzw. Abmessungen der Auswahl, die letzte Messung, Sichtfeld und Projektion.
- Wie Blender hat die Seitenleiste (N) Zahlenfelder: Position und Dimensionen einer Fläche bzw. Position und Rotation eines Griffs im Posenmodus ziehen oder eintippen; Strg+Z nimmt eine Eingabe in einem Schritt zurück.
- Neues Werkzeug Messen: zwei Klicks auf das Modell oder Ziehen zeigen den Abstand in Spieleinheiten; wie 3ds Max als Abstand messen in Alle Aktionen suchen und im Rechtsklick.
- Jeder Bedienmodus behält seine eigene Fensteranordnung; beim Wechsel bleiben Auswahl, Sequenz und Bild. Der Aufbau wie 3ds Max bleibt unverändert.
- Deutsch: das Menü Overlays wie Blender heißt Überlagerungen, wie in Blenders deutscher Oberfläche.

### Zeitleiste

- Model-Tab: die Zeitleiste sieht jetzt aus wie im freigegebenen Entwurf und folgt dem Bedienmodus. Wie 3ds Max: Anfang, Bild zurück, Abspielen, Bild vor, Ende, Bildanzeige ab 0, Auto Key (N), Set Key (K), Zeitkonfiguration, ein feiner Strich je Frame in der Skala und in jeder Spur, Schlüssel als Striche in den Farben der Spurleiste (Rot = Bewegung, Grün = Drehung). Wie Blender: Sprünge zum vorigen und nächsten Schlüssel (↓ / ↑), Aufnahme-Knopf für Auto-Keying, Start / Ende / Bild ab 1, Schlüssel als Rauten, K öffnet das Keying-Menü.
- Model-Tab: die Spuren (Dopesheet, Kurveneditor) gehören zur Zeitleiste - ein Lineal, Bild 0 in Skala und Spuren an derselben Stelle; der Griff über der Zeitleiste oder der Knopf Spuren öffnet sie, im Bereich Animation stehen sie ab Werk offen. Ansehen/Prüfen zeigt Tempo, Interpolation, Schleife erzwingen und die Primär- und Sekundärsequenz; die anderen Bereiche zeigen die kurze Zeitleiste.
- Model-Tab: Auto Key lässt sich ausschalten - eine Pose bleibt dann ohne Schlüssel, bis Set Key (K, wie Blender I) sie behält; ein Bildwechsel verwirft sie, wie in 3ds Max und Blender.
- Model-Tab: Auto Key (wie Blender Auto-Keying) ist ab Werk aus wie in 3ds Max und Blender; das Programm merkt sich die Wahl.
- Model-Tab: Statuszeile unten in der Zeitleiste (Bedienmodus - Klick wechselt, Tasten des Bereichs, Undo und Suche); Zeitkonfiguration mit Bildrate, Zeitanzeige in Frames oder Sekunden, Wiedergabe und der Zeile der Sequenz in der animation.cfg; Hautliste und Flächenzähler stehen jetzt in der Gruppe Detailstufe und Skin.
- Model-Tab: in einem kleinen Fenster nimmt die Zeitleiste nicht mehr fast die halbe Höhe - unter 50 Zeichenhöhen Fensterhöhe behält sie ihren Anteil, die Spuren werden niedriger und rollen, und die Felder darüber zeigen mehr als ihre Überschriften.

### Ansehen/Prüfen

- Model-Tab, Ansehen/Prüfen: die Gruppen des Entwurfs - Modell (mit Importieren…), Auswahl, Detailstufe und Skin, Boden und Kamera, Anbauten, Prüfung für das Spiel, Szene.
- Alle zehn Detailstufen sind wählbar wie r_lodbias im Spiel; eine Stufe über der letzten des Modells zeigt die letzte, und die Gruppe sagt es. Skins sind eine Auswahl in zwei Spalten.
- Boden und Kamera: Boden auf -24 oder unter dem tiefsten Punkt der Pose, Sichtfeld 10°, 80° oder 90°, einfarbiger Hintergrund, Gewicht-Schwelle, Ansicht zurücksetzen und Foto (1920+).
- Prüfung für das Spiel: Balken der gewählten Fläche gegen die Grenzen des Zielspiels, Flächen, Sequenzen innerhalb der GLA, die vom Spielcode gesuchten Tags, Shader ohne Bild, LODs und Flächen ohne eigenen Schatten.
- Szene: Modell, Skin, Anbauten, Sequenzen und geschaltete Flächen als ModView-Skript (.mvs) speichern und ModView-Skripte lesen.
- Die fünf fehlenden Prüfanzeigen von ModView stehen in den Menüs der Kopfzeile der Ansicht: Alpha-Blending, Texturfilter, Knochengewicht-Schwelle, Fläche hervorheben mit ihren Knochenbezügen, nicht schattenwerfende Flächen.

### Modellieren

- Model-Tab, Modellieren: das Feld rechts folgt dem freigegebenen Entwurf - die Gruppen Auswahl, Werkzeuge und JKA-Grenzen. Die Ebenen sind eine Reihe von Feldern mit ihrer Taste davor (3ds-Max-Modus 1-5, Blender-Modus 1-3); die sechs Hauptwerkzeuge sind gleich breite Knöpfe mit Symbol und der Taste des Bedienmodus, danach folgen die weiteren Werkzeuge der Ebene, der Rest ist unter „Geometrie bearbeiten“ zugeklappt.
- Ein Werkzeug, das auf der laufenden Ebene nicht arbeitet, ist ausgegraut, und sein Tooltip nennt die Ebenen, auf denen es arbeitet.
- JKA-Grenzen: Balken für die gewählte Fläche (Ecken, Knochen und Gewichte gegen die Grenze des Spiels, gelb ab 90 %, rot darüber) und eine Kurzinfo in der Überschrift; die Liste aller Flächen ist eingeklappt (bei einer Figur mit 83 Teilen füllte sie acht Seiten).
- Grundkörper und Requisiten stehen im Reiter „Erstellen“ (3ds-Max-Modus) bzw. „Objekt“ (Blender-Modus), ein Knopf je Grundkörper; solange ein Requisit offen ist, bleibt „In die Karte“ ein Klick.
- Strg+Enter übernimmt ein Werkzeug und beginnt es noch einmal auf dem Ergebnis (der Knopf „+“ des Werkzeugkästchens).
- Blender-Modus: E extrudiert wie in Blender (Alt+E geht weiterhin), Umschalt+A öffnet das Menü „Hinzufügen“ mit den Grundkörpern.
- Model-Tab: jede Fläche hat einen Modifier-Stapel. Im 3ds-Max-Modus steht die Gruppe „Modifier-Stapel“ zuoberst im Reiter Ändern, im Bereich Modellieren und im Bereich Weights, und liest sich von unten nach oben: die Fläche der Datei, Edit Poly, die Modifier, Skin - ein Klick auf Edit Poly oder Skin wechselt den Bereich. Im Blender-Modus steht die Gruppe „Modifier“ im Reiter Modifikatoren und nennt die Modifier von oben nach unten.
- Zwei Modifier: Symmetrie (Blender-Modus: Spiegeln) schneidet die Fläche an einer Achsenebene und spiegelt eine Seite auf die andere, samt den Knochengewichten; TurboSmooth (Blender-Modus: Unterteilung) unterteilt und rundet die Fläche ein- bis dreimal und kann offene Ränder stehen lassen.
- Jeder Modifier hat das Auge (an/aus) und „In die Datei schreiben“; Glätten ist zunächst nur Vorschau. Modifier lassen sich nach oben und unten schieben (auch durch Ziehen), anwenden (Blender-Modus: Anwenden, 3ds-Max-Modus: Reduzieren bis und Alle reduzieren) und entfernen; jedes davon ist ein Rückgängig-Schritt. Die Befehle stehen auch im Rechtsklick-Menü einer Zeile, im Rechtsklick-Menü der Ansicht und in „Alle Aktionen suchen“.
- Was die Ansicht für einen Modifier zeigt, ist genau das Netz, das Anwenden ergibt. Solange eine Fläche keinen Modifier hat, der in die Datei geht, schreibt Speichern dieselbe Datei wie bisher.
- JKA-Grenzen und die Prüfung vor dem Speichern zählen, was in die Datei geht, samt den Modifiern, die hineingeschrieben werden.
- Modellieren, Symmetrisieren: die gespiegelte Seite hängt jetzt bei jedem Knochenpaar des Skeletts an den Knochen ihrer Seite (lhumerus und rhumerus, ltibia und rtibia ...); bisher wurden nur Namen getauscht, die mit l_ und r_ beginnen.

### UV

- Model-Tab, Bereich UV: das Feld rechts folgt dem freigegebenen Entwurf - Auswahl, dann die nummerierten Schritte 1 Nähte festlegen, 2 Abwickeln, 3 Aufräumen, dann Prüfen. Gleich breite Knöpfe in zwei Spalten mit Symbol und der Taste des Bedienmodus, Abwickeln als hervorgehobener Hauptknopf, ein Tippkasten in den Schritten.
- Auswahl zeigt in der Überschrift die Textur und die Zahl der Inseln; neu: Insel wählen (erweitert die Auswahl auf ganze Inseln) und Alles wählen.
- Schnell abwickeln und Projizieren sind je ein Knopf; die Vorlage (Figur, Requisit, Harte Fläche) und die Projektion (Eben, Zylinder, Kugel, Kasten) wählt man direkt darunter. Seltener gebrauchte Befehle stehen in jedem Schritt eingeklappt unter Weitere.
- Prüfen: die Überschrift sagt „alles gut“ oder die Zahl der Befunde; Dehnung, Überlappungen, die neue Ausnutzung der Textur in Prozent und die Texel-Dichte stehen als Tabelle; neuer Schalter Nähte dick für den UV-Editor.
- Bereich UV: Nähte dick macht jetzt auch die Nähte in der 3D-Ansicht dick.
- Wie Blender: L wählt die Insel unter der Maus, A wählt alles, Strg+A gleicht die Dichte an, Strg+P packt. Wie 3ds Max: Strg+A wählt alles.

### Weights und Auto-Skin

- Model-Tab, Bereich Weights: das Feld rechts folgt dem freigegebenen Entwurf. Wie 3ds Max: Modifier-Stapel, Gewichts-Werkzeug, Spiegeln, Auto-Skin, Knochen, JKA-Grenzen und zugeklappt Erweiterte Parameter. Wie Blender: Pinsel, Optionen, Bereinigen, Spiegeln, Auto-Skin, Knochen, JKA-Grenzen.
- Gewichts-Werkzeug wie 3ds Max: Gewichte malen schaltet das Malen an und aus; die Festwerte 0 bis 1 setzen das Gewicht sofort auf die gewählten Ecken; neu Skalieren, Mischen, Wachsen, Schrumpfen, Ring und Schleife.
- Weights: der Hauptknopf Gewichte malen behält Platz und Höhe, auch wenn das Malen aus ist (dann ist nur sein Rahmen betont).
- Neue Pinsel wie Blender: Verlauf (linear oder radial) und Verwischen; Auto-Normalisieren hält die Gewichte einer Ecke bei 1 (ab Werk an).
- Bereinigen wie Blender: Alles normalisieren, Säubern, Glätten und Begrenzen: 4, auf die gewählten Ecken oder auf alle.
- Spiegeln wie 3ds Max (Spiegelmodus links blau, rechts grün, ohne Gegenstück gelb; Blau nach Grün, Grün nach Blau, Seiten tauschen, nur Auswahl, Toleranz) und wie Blender (Gruppe oder alle Gruppen spiegeln, Namen tauschen, Topologie-Spiegeln); Ecken am selben Ort auf verschiedenen Flächen zählen als eine.
- Knochen lassen sich sperren (wie Blender: K öffnet das Sperren-Menü); die Knochen sind in diesem Bereich ab Werk zu sehen; die Grenzen des Spiels stehen als Balken da.
- Model-Tab, Weights: Auto-Skin. "Automatisch gewichten" gibt einem Netz seine Knochengewichte aus dem Skelett, in einem Rückgängig-Schritt. Drei Verfahren: von einem Original des Spiels mit demselben Skelett übertragen (zum Beispiel kyle), geodätisch (der Abstand zu jedem Knochen wird über Voxel durch das Innere des Netzes gemessen, so klebt ein Arm nicht am Körper), Hülle (gerader Abstand, schnell). Die Drehknochen (lhumerusX, lradiusX, lfemurX) tragen die Hälfte ihres Glieds zum nächsten Gelenk hin, wie bei Ravens Figuren.
- Auto-Skin: Das Ergebnis passt immer zum Spiel - höchstens 4 Gewichte je Ecke, höchstens 32 Knochen je Fläche, in 10 Bit abgelegt, wie das Spiel sie liest; Ecken an derselben Stelle bekommen dieselben Gewichte, die Nähte zwischen Flächen bleiben also zu.
- Auto-Skin: Einstellungen für Auflösung, Steifigkeit, Glätten, Nähte, Drehknochen, Gesicht und Anhänge, Spiegeln danach und "Nur ausgewählte Flächen" (ein neues Rüstungsteil an einer fertigen Figur). Das Fenster zeigt das Ergebnis: die Grenzen des Spiels und wie weit die Ecken von den Gewichten davor liegen, in Einheiten.
- Auto-Skin: "Automatisch gewichten" und "Auto-Skin: Einstellungen" stehen in der Suche aller Aktionen (X, im Blender-Modus F3) und im Rechtsklick der Ansicht unter Weights.
- Model-Tab, Weights: Speichern schreibt die Gewichte, die die Ansicht zeigt, auch solange Mallisten offen sind (nach dem Malen oder einem Gewichtsbefehl, vor „Übernehmen“). Die Datei bekommt höchstens vier Gewichte je Ecke, normiert, wie das Spiel sie liest; die Mallisten bleiben offen. Die Prüfung vor dem Speichern und die JKA-Grenzen messen dasselbe. Bisher schrieb Speichern die Gewichte von vor dem Malen und meldete „gespeichert“.
- Weights: Solange ein Strich, Verlauf, Gizmo-Zug oder Werkzeugkästchen offen ist, bleibt die Detailstufe stehen, auch wenn das Rad zoomt oder „Auto“ sie wechseln würde. Spiegeln beim Malen konnte bei einem Stufenwechsel im Strich falsche Ecken beschreiben oder abstürzen.
- Weights und Modellieren: Eine Auswahl von Ecken, Kanten oder Vielecken gehört zu ihrer Detailstufe; nach einem Stufenwechsel ist sie leer und kommt mit der Stufe zurück, solange deren Netz nicht geändert wurde. Bisher blieben die Nummern stehen, und Befehle änderten andere Ecken.
- Modellieren: Ein Werkzeug auf einer Fläche mit gemalten, noch nicht übernommenen Gewichten behält sie (bisher sprangen sie zurück).
- Auto-Skin: „Nur ausgewählte Flächen“ mit „Danach spiegeln“ spiegelt nur in die gewählten Flächen und lässt offene Mallisten anderer Flächen offen.
- Strg+Z während eines Gewichtsstrichs, Verlaufs, Gizmo-Zugs oder Strichs im Painting beendet ihn und nimmt ihn zurück; Strg+Y bringt ihn wieder. Bisher schrieb die gehaltene Taste ohne Rückgängig-Schritt weiter.
- Kleinere Korrekturen: Ein Befehl, der nichts ändert, lässt Wiederholen stehen; ein Strich, der neben der Figur beginnt, malt mit der gewählten Art; gesperrte Knochen sind bei einem neu geladenen Modell entsperrt; ein getippter Pinselradius 0 im Painting zählt als 1.

### Painting

- Model-Tab, Painting: das rechte Feld folgt dem freigegebenen Entwurf. Die Gruppen Ziel, Werkzeuge, Pinsel, Ebenen und Für das Spiel stehen in der Reihenfolge der Arbeit; die Werkzeuge sind ein Raster aus Knöpfen mit Symbol (das gewählte ist hervorgehoben), Farbe, Radius, Stärke und Abstand sind Zeilen mit Reglern, Härte, Spiegeln, Stiftdruck, Rückseiten und Ausbluten stehen unter Erweitert.
- Ebenen sind Zeilen mit einem Auge zum Ein- und Ausblenden und einem Schloss am Original; Mischart und Deckkraft der gewählten Ebene stehen unter der Liste, ein Tipp-Kasten unter der Gruppe.
- Die Tasten [ und ] machen den Pinsel in beiden Bedienmodi kleiner und größer (auch in der Suche nach allen Aktionen und in der Statuszeile).
- Wie 3ds Max stehen die Gruppen im Reiter Ändern (Für das Spiel auch unter Dienstprogramme), wie Blender im Reiter Werkzeug (Ziel auch unter Material).
- Lange Pfade werden vorne gekürzt („.../kyle/body.tga“), der Ausgabeordner zeigt das Ende seines Pfads; der ganze Pfad steht im Tooltip.
- Model-Tab wie 3ds Max, Painting: Suche und Menü Navigation nennen dort Strg+= und Strg+- zum Zoomen, weil [ und ] die Pinselgröße stellen.

### Animation

- Model-Tab, Bereich Animation: das Feld rechts folgt dem freigegebenen Entwurf - Animation, Rig erstellen, Presets, Vorhandene Animation bearbeiten, Übertragen (Retargeting), IK/FK, Anbauten und Vorschau, Für das Spiel bauen; dahinter, zugeklappt, alles Weitere (Auto-Physik, Pose, Ghosts, Bewegungspfade, Kurven, Posen-Bibliothek, GLA, animation.cfg, Ziel-GLA und Plätze).
- Rig erstellen: zeigt Skelett, Maßstab und die erkannte Vorlage; eine Vorlage lässt sich nach Gruppe wählen (Mensch, Rancor, Wampa, Tauntaun, Howler, AT-ST, Droiden, Fahrzeuge …) oder Eigenes Skelett. Neu: Animation-Rig (nur FK) und Auf Rig legen (die Animation wird zu Schlüsseln an den Griffen, die Pose bleibt).
- Model-Tab, Animation: wie bei einem CAT-Rig legt Control Rig erstellen eine Animation der Sequenz von FK auf IK auf das Rig - Hände und Füße folgen ihren IK-Zielen und Polen, die Bewegung sieht gleich aus (die Meldung nennt die größte Abweichung). Danach arbeitet man an den Griffen (Hand- oder Fußziel, Pol, Fußrolle) an jedem Frame; nur dieser Frame ändert sich.
- Presets: die Bibliothek steht vorn - Kategorie, Liste mit Quelle und Lizenz, Ziel-Platz als Wahl, Vorschau auf Figur und Auf Figur legen (Retarget); der Generator öffnet sich über seinen Knopf.
- Presets zeigen die acht Kategorien des Entwurfs (Stehen, Gehen, Laufen, Rückwärts, Ducken, Springen, Kampf, Gesten, dahinter Sonstiges); eine Kategorie ohne Presets zeigt einen kurzen Hinweis.
- Der eigene Generator steht als Gehen (erzeugt) und Laufen (erzeugt) in der Liste: Tempo und Armschwung unter der Liste, Vorschau auf Figur und Auf Figur legen wie bei Presets, Generator ... für den Rest; der Rechtsklick auf einen Eintrag bietet seine Befehle.
- Vorhandene Animation bearbeiten: Frames zu Schlüsseln reduzieren und In die GLA backen in einer Gruppe mit Quelle, Bildern und Schlüsseln.
- IK/FK wie 3ds Max: je Glied der Zustand in Worten und ein Knopf IK/FK; wie Blender: Regler und beide Einrast-Knöpfe in einer Zeile.
- IK/FK je Glied wie CAT: der Knopf schaltet das Glied ab dem aktuellen Frame bis zum nächsten Wechsel um und gleicht vorher die andere Seite an - nichts springt.
- Für das Spiel bauen zeigt den Wurzelweg der Sequenz (wie weit das Becken wandert) und erinnert an SET_ORIGIN.
- Animation, Gruppe für die Karten-GLA: die Felder Ziel, Karte und Name beginnen jetzt an derselben Kante.
- Helles Thema: das gewählte Preset ist wieder lesbar.
- Model-Tab, Animation: Animation-Rig (nur FK) nach Control Rig erstellen backt die IK jedes Glieds zuerst in FK-Schlüssel - die Bewegung bleibt genau gleich, jeder Dreh-Griff dreht seinen Knochen, und Strg+Z holt die IK-Ziele in einem Schritt zurück.
- Model-Tab: behoben, dass ein Gizmo-Zug an einem Griff offen blieb, wenn Q das Gizmo wegnahm; Strg+Z beim Ziehen des Tempos eines erzeugten Gangs; der Szenen-Explorer zeichnete nach dem Lösen eines Anbaus Zeilen hinter dem Listenende; der gewählte Reiter des rechten Felds bleibt nach einem sehr kleinen Fenster.

### Knochen und Rig-Stile

- Model-Tab: Knochen stehen im Bild der Ansicht. Wie 3ds Max als Bone-Kästen (Flossen auf Wunsch), wie Blender mit Anzeigen als: Oktaeder, Stick, B-Bone, Hülle oder Draht.
- Knochen und Griffe liegen ab Werk vor der Figur; ist „Im Vordergrund“ aus, verdeckt die Figur sie.
- Durchsicht: Alt+X wie 3ds Max (die Figur in jeder Ansicht), Alt+Z wie Blender (Röntgen der aktiven Ansicht). Die Figur liegt halb deckend über dem, was hinter ihr ist.
- Rig-Stil im Bereich Animation: Standard, Control Rig mit Ringen, CAT-Kästen oder Blender-Oktaeder. Griffe sind nach Seite gefärbt; der Stil ändert nur das Aussehen, nie das Rig.
- Klick und Ziehen treffen die sichtbare Form eines Knochens oder Griffs: Ring, Kante oder Inneres eines Kastens, Kugel eines Pols, Oktaeder.
- Alles sieht unter Direct3D 12 und Direct3D 11 gleich aus.

### Helfer, Suche und Handbuch

- Model-Tab, Alle Aktionen suchen: Aktionen, die in mehreren Bereichen gleich heißen, nennen rechts ihren Bereich (Verwischen - Weights, Verwischen - Painting); wie Blender steht Abspielen/Anhalten nur einmal da.
- Der Helfer (Was willst du tun?) kennt jede Gruppe, jeden Knopf, Schalter und Regler im rechten Feld des Model-Tabs und führt dorthin: Model-Tab, Bereich, Gruppe, Element.
- Handbuch: die Seiten des Model-Tabs stehen in der Reihenfolge der Arbeit - Überblick, Bedienung, Aufbau wie Blender, Zeitleiste, Dateien, dann die Bereiche Ansehen/Prüfen, Modellieren, UV, Weights, Auto-Skin, Painting und Animation; neue Seiten für den Aufbau wie Blender, Ansehen/Prüfen, das Feld der Animation und Auto-Skin.

## 中文


### 整个选项卡

- 模型选项卡,右侧面板:现在外观也随操作方式而变。类似 3ds Max 时是带有“创建、修改、层次、运动、显示、实用程序”选项卡的命令面板;类似 Blender 时是属性编辑器,左侧有一列竖排图标(工具、物体、修改器、数据、骨骼、约束、材质)。每个区域会打开自己的选项卡并在其中显示全部分组;其他选项卡显示各自的部分。
- 所有分组采用同一种形式:可折叠的标题,右侧有简要信息(三角形数、已绘制的 LOD、挂接模型数、检查结果),按钮排成两列等宽的网格并带图标和按键,数值以表格显示,限制以条形显示,提示放在方框里。
- 面板更宽(22 个字符高度,原为 17),并可在与视图相邻的边缘拖动调整宽度;双击恢复默认值。
- “查看/检查、建模、UV、权重、绘制、动画”这些区域作为一行等宽的格子横跨整个选项卡,旁边是针对操作方式和区域的简短提示。
- “类似 3ds Max / 类似 Blender”的开关已移出面板:它位于选项卡的状态栏、视图标题的菜单、设置以及“搜索所有操作”中。
- 模型选项卡, 右侧面板: 数值表、按钮和开关在窄面板中只在词与词之间换行; 当某个词放不下时, 按钮网格会减少列数。
- 模型选项卡, 右侧面板: 在小窗口中, 类似 3ds Max 的命令面板选项卡以图标排成一行, 名称显示在工具提示中 (与 Blender 的属性选项卡相同) - 以文字显示时需要两到三行。
- 视图右下角显示细节级别和绘制的三角形数。像 3ds Max 一样, 建模、权重、绘制和动画以四个视图开始; 像 Blender 一样, Z 选择着色, Shift+Alt+Z 隐藏叠加层。打开 .glm 时总是进入“查看/检查”。
- 模型选项卡 (像 3ds Max): “选择骨骼”与“测量距离”一样是开关 - 位于“搜索所有操作”和右键菜单 (带勾选标记), 开启期间视图顶部显示提示; Q、W、E、R 结束它。

### 场景资源管理器

- 模型选项卡,场景资源管理器:一棵树包含一切——模型、带层级的面、带挂接模型的标签、骨骼、绑定控制柄以及 animation.cfg 的序列(单独的动画列表已取消)。该列与已批准的设计一样宽。
- 与 3ds Max 相同:箭头、眼睛和符号紧挨在名称前,每一级缩进一个符号,带雪花的“已冻结”列,可排序、可拖动的列标题。与 Blender 相同:大纲视图,眼睛和“可选”箭头位于右侧的列中,并带筛选弹出面板;切换按钮为 层级 / 集合。
- 冻结:冻结的面、标签和骨骼保持可见,但在树和视图中都无法选择;封口从一开始就是冻结的。“按图层”显示身体、封口、标签、骨骼、挂接物和其他 LOD,每层都有眼睛和雪花。
- 在视图中单击会穿过冻结的面, 选中其后面的面, 与 3ds Max 相同。
- 树中的序列用锁 P / S 标出主序列和次序列,当帧位于 GLA 末尾之后时显示警告;分组列表、复制名称、写入脚本。
- 树从不横向滚动:较深的骨骼链会排得更紧,较长的名称以“...”结尾,并与该行的信息一起完整显示在工具提示中。按种类筛选,Shift+单击选择一个范围,右键菜单中有“信息”。
- 左侧模式栏的工具提示不再弹到相邻面板上:鼠标稍作停留后,它出现在栏旁、与按钮等高的位置。

### 与 Blender 相同的布局

- 模型选项卡像 Blender 时: 左侧一栏是工具栏 (T), 含选择、移动、旋转、缩放以及该区域的工具 - 测量、选择面、选择骨骼、照片; 挤出、插入、倒角、循环切割、切刀、合并; 标记接缝、展开、松弛、打包; 笔刷、模糊、渐变、采样权重; 绘制、填充、克隆、涂抹、遮罩; 复制姿势、镜像粘贴、精简。
- 像 Blender 的工具栏使用 Blender 的工具名称, 德语、西班牙语、捷克语、日语和中文使用 Blender 自己的翻译。
- 像 Blender 的工具组: 小三角标出一个组, 按住或拖动即可打开 (挤出选区 / 沿法向挤出 / 挤出各个面, 切割 / 切分); 右键单击工具栏可显示工具名称。
- 像 Blender 时大纲视图位于右上方的属性编辑器之上; 两者之间的边界可以拖动。
- 视图标题像 Blender 一样显示模式 (物体模式、编辑模式、权重绘制、纹理绘制、姿态模式); 单击即可切换区域。
- 按 N 打开侧栏: 所选内容的位置和旋转或尺寸、最近一次测量、视野和投影。
- 像 Blender 一样, 侧栏 (N) 提供数字字段: 拖动或输入面的位置和尺寸, 或姿态模式下绑定控制器的位置和旋转; Ctrl+Z 一步撤销一次输入。
- 新工具“测量”: 在模型上单击两次或拖动即可显示以游戏单位表示的距离; 像 3ds Max 时以“测量距离”出现在“搜索所有操作”和右键菜单中。
- 每种操作模式都保留自己的窗口布局; 切换时所选内容、序列和帧保持不变。像 3ds Max 的布局保持不变。
- 德语: 与 Blender 相同的 Overlays 菜单改称 Überlagerungen, 与 Blender 的德语界面一致。

### 时间轴

- 模型选项卡: 时间轴现在与已确认的设计一致, 并随所选操作方式变化。像 3ds Max: 到开头、上一帧、播放、下一帧、到结尾、从 0 计数的帧显示、Auto Key (N)、Set Key (K)、时间配置, 标尺和每条轨道中每帧一条细线, 关键帧为轨迹栏颜色的竖条 (红 = 移动, 绿 = 旋转)。像 Blender: 跳到上一个和下一个关键帧 (↓ / ↑)、自动关键帧的录制按钮、从 1 计数的起始 / 结束 / 帧、菱形关键帧, K 打开关键帧菜单。
- 模型选项卡: 轨道 (摄影表、曲线编辑器) 成为时间轴的一部分 - 共用一把标尺, 第 0 帧在标尺和轨道中位于同一位置; 时间轴上方的手柄或“轨道”按钮可打开它们, 在动画区域默认打开。“查看/检查”显示速度、插值、强制循环以及主要和次要序列; 其他区域显示简短时间轴。
- 模型选项卡: 可以关闭 Auto Key - 此时姿势没有关键帧, 直到 Set Key (K, 像 Blender 时为 I) 将其保留; 换帧会丢弃它, 与 3ds Max 和 Blender 相同。
- 模型选项卡: Auto Key (像 Blender 时为自动插帧) 默认关闭, 与 3ds Max 和 Blender 相同; 程序会记住您的选择。
- 模型选项卡: 时间轴底部新增状态行 (操作方式 - 单击切换、该区域的按键、撤销和搜索); 时间配置包含帧率、以帧或秒显示时间、播放以及序列在 animation.cfg 中的那一行; 皮肤列表和面计数移到“细节层次和皮肤”组。
- 模型选项卡: 在小窗口中时间轴不再占去近一半的高度 - 窗口高度低于 50 个字符高度时, 时间轴保持其所占比例, 轨道变矮并可滚动, 上方的面板不再只显示标题。

### 查看/检查

- 模型选项卡“查看/检查”: 设计中的各组 - 模型 (含导入……)、所选内容、细节级别和皮肤、地面和相机、附件、针对游戏检查、场景。
- 十个细节级别都可像游戏中的 r_lodbias 一样选择; 高于模型最后一级的级别显示最后一级, 组中会说明。皮肤以两列方式选择。
- 地面和相机: 地面位于 -24 或姿势最低点之下, 视野 10°、80° 或 90°, 纯色背景, 权重阈值, 重置视图和照片 (1920+)。
- 针对游戏检查: 所选面相对于目标游戏上限的进度条、面数、GLA 范围内的序列、游戏代码查找的标签、无图像的着色器、LOD 以及无自身阴影的面。
- 场景: 将模型、皮肤、附件、序列和已切换的面保存为 ModView 脚本 (.mvs), 并可读取 ModView 脚本。
- ModView 缺少的五个检查显示位于视图标题栏的菜单中: Alpha 混合、纹理过滤、骨骼权重阈值、高亮面及其骨骼引用、不投射阴影的面。

### 建模

- 模型选项卡,建模:右侧面板按已批准的设计重做——“选择”“工具”和“JKA 限制”三个组。各层级排成一行格子,前面是按键(3ds Max 模式 1-5,Blender 模式 1-3);六个主要工具是带符号和当前操作模式按键的等宽按钮,其后是该层级的其他工具,其余的折叠在“编辑几何体”下。
- 在当前层级不起作用的工具显示为灰色,其提示写明它适用的层级。
- JKA 限制:为所选面显示进度条(顶点、骨骼和权重对照游戏的限制,90 % 起为黄色,超出为红色),标题中有简短说明;全部面的列表已折叠(对于有 83 个部件的角色,它以前占满八页)。
- 基本体和道具位于“创建”选项卡(3ds Max 模式)或“物体”选项卡(Blender 模式),每个基本体一个按钮;只要打开的是道具,“放入地图”仍然只需单击一次。
- Ctrl+Enter 应用工具并在结果上再次启动它(工具框的“+”按钮)。
- Blender 模式:E 与 Blender 一样执行挤出(Alt+E 仍然可用),Shift+A 打开含基本体的“添加”菜单。
- 模型选项卡: 每个面都有一个修改器堆栈。3ds Max 模式下, “修改器堆栈”组位于“修改”选项卡的最上方 (建模区域和 Weights 区域), 自下而上读: 文件中的面、Edit Poly、各修改器、Skin - 单击 Edit Poly 或 Skin 可切换区域。Blender 模式下, “修改器”组位于“修改器”选项卡, 自上而下列出修改器。
- 两种修改器: 对称 (Blender 模式: 镜像) 在轴平面处切开面并把一侧镜像到另一侧, 骨骼权重也一并镜像; 涡轮平滑 (Blender 模式: 细分) 把面细分并圆滑一到三次, 并可让开放边界保持原位。
- 每个修改器都有眼睛图标 (开/关) 和“写入文件”; 平滑起初仅作预览。修改器可以上移和下移 (也可拖动)、应用 (Blender 模式: 应用; 3ds Max 模式: 塌陷到、全部塌陷) 和移除; 每一项都是一步撤销。这些命令也在行的右键菜单、视图的右键菜单和“搜索所有操作”中。
- 视图为修改器显示的内容, 正是应用后得到的网格。只要一个面没有写入文件的修改器, 保存时写出的文件就与以前相同。
- JKA 限制和保存前的检查按写入文件的内容计数, 包括写入文件的修改器。
- 建模, 对称: 对骨架中的每一对骨骼 (lhumerus 与 rhumerus、ltibia 与 rtibia 等), 镜像出的一侧现在都绑定到本侧的骨骼; 此前只交换以 l_ 和 r_ 开头的名称。

### UV

- 模型选项卡的 UV 区域:右侧面板按已确认的设计排列——“选择”,然后是带编号的步骤 1 设置接缝、2 展开、3 整理,最后是“检查”。按钮等宽、分两列,带图标和当前操作模式的按键,“展开”是突出显示的主按钮,步骤中有提示框。
- “选择”在标题中显示纹理和岛的数量;新增:选择岛(把选择扩展到整个岛)和全选。
- 快速展开和投影各为一个按钮;模板(角色、道具、硬表面)和投影方式(平面、圆柱、球体、长方体)就在下方选择。不常用的命令折叠在每个步骤的“更多”中。
- 检查:标题显示“一切正常”或问题数量;拉伸、重叠、新增的纹理利用率(百分比)和纹素密度以表格显示;新增用于 UV 编辑器的开关“加粗接缝”。
- UV 区域:“加粗接缝”现在也会加粗 3D 视图中的接缝。
- Blender 模式:L 选择鼠标下的岛,A 全选,Ctrl+A 统一密度,Ctrl+P 打包。3ds Max 模式:Ctrl+A 全选。

### Weights 和自动蒙皮

- 模型选项卡的 Weights 区域:右侧面板按已确认的设计排列。3ds Max 模式:修改器堆栈、权重工具、镜像、自动蒙皮、骨骼、JKA 限制,以及折叠的高级参数。Blender 模式:笔刷、选项、清理、镜像、自动蒙皮、骨骼、JKA 限制。
- 3ds Max 风格的权重工具:“绘制权重”用于开关绘制;固定值 0 到 1 立即把权重设置到所选顶点;新增缩放、混合、扩大、缩小、环形和循环。
- Weights: 主按钮“绘制权重”在关闭绘制时保持位置和高度 (此时只突出显示边框)。
- 新的 Blender 风格笔刷:渐变(线性或径向)和涂抹;自动规格化使每个顶点的权重之和保持为 1(默认开启)。
- Blender 风格的清理:全部规格化、清除、平滑和限制为 4,作用于所选顶点或全部顶点。
- 3ds Max 风格的镜像(镜像模式左蓝右绿、无对应点为黄色;蓝到绿、绿到蓝、交换两侧、仅所选、容差)以及 Blender 风格的镜像(镜像组或全部组、翻转名称、拓扑镜像);不同表面上同一位置的顶点视为一个。
- 骨骼可以锁定(Blender 模式:K 打开锁定菜单);在此区域中骨骼默认可见;游戏的限制以进度条显示。
- 模型标签页, Weights: 自动蒙皮。"自动计算权重" 根据骨架为网格计算骨骼权重, 一步撤销。三种方法: 从具有相同骨架的游戏原版模型传递 (例如 kyle)、测地 (通过体素沿网格内部测量到每根骨骼的距离, 手臂不会粘在身体上)、包络 (直线距离, 快速)。扭转骨骼 (lhumerusX、lradiusX、lfemurX) 承载其肢体靠近下一关节的一半, 与 Raven 的角色相同。
- 自动蒙皮: 结果始终符合游戏 - 每个顶点最多 4 个权重, 每个表面最多 32 根骨骼, 以游戏读取的 10 位存储; 同一位置的顶点得到相同的权重, 因此表面之间的接缝保持闭合。
- 自动蒙皮: 可设置分辨率、刚度、平滑、接缝、扭转骨骼、面部和附属骨骼、之后镜像以及 "仅所选表面" (成品角色上的一件新盔甲)。窗口显示结果: 游戏的限制, 以及顶点与之前权重所给位置的距离 (以单位计)。
- 自动蒙皮: "自动计算权重" 和 "自动蒙皮: 设置" 位于全部动作的搜索 (X, Blender 模式为 F3) 以及视图右键的 Weights 之下。
- 模型选项卡, Weights: 即使绘制列表仍处于打开状态(绘制或执行权重命令之后、“应用”之前), 保存也会写入视图显示的权重。文件中每个顶点最多四个权重, 按游戏读取的方式归一化; 绘制列表保持打开。保存前的检查和 JKA 限制测量同样的内容。此前保存写入的是绘制前的权重, 却提示“已保存”。
- Weights: 当笔划、渐变、Gizmo 拖动或工具框处于打开状态时, 细节级别保持不变, 即使滚轮缩放或“自动”本应切换。此前笔划中切换级别时, 镜像绘制可能写到错误的顶点或导致崩溃。
- Weights 和建模: 顶点、边或多边形的选择属于其细节级别; 切换级别后选择为空, 只要该级别的网格未被更改, 回到该级别时选择会恢复。此前编号保持不变, 命令会修改其他顶点。
- 建模: 在带有已绘制但尚未应用权重的面上使用工具时, 这些权重会保留(此前会跳回)。
- 自动蒙皮: “仅所选表面”配合“之后镜像”时只镜像到所选的面, 其他面的绘制列表保持打开。
- 在权重笔划、渐变、Gizmo 拖动或纹理绘制笔划过程中按 Ctrl+Z 会结束并撤销它; Ctrl+Y 可恢复。此前按住的鼠标键会在没有撤销步骤的情况下继续写入。
- 较小的修正: 不做任何更改的命令会保留“重做”; 从人物旁边开始的笔划使用所选模式绘制; 新加载的模型中锁定的骨骼会解锁; 在 Painting 中输入的画笔半径 0 按 1 处理。

### Painting

- 模型选项卡,绘制:右侧面板遵循已批准的设计。“目标、工具、画笔、图层、用于游戏”这些分组按工作顺序排列;工具是带图标的按钮网格(所选工具高亮),颜色、半径、强度和间距是带滑块的行,硬度、镜像、笔压、背面和出血位于“高级”下。
- 图层是一行行条目,带有用于显示或隐藏的眼睛图标,原图带有锁;所选图层的混合模式和不透明度位于列表下方,分组下方有提示框。
- [ 和 ] 键在两种操作方式下都能缩小和放大画笔(在“搜索所有操作”和状态栏中也有)。
- 类似 3ds Max 时,这些分组位于“修改”选项卡(“用于游戏”也在“实用程序”下);类似 Blender 时位于“工具”选项卡(“目标”也在“材质”下)。
- 长路径从前面缩短(“.../kyle/body.tga”),输出文件夹显示其路径的末尾;完整路径在工具提示中。
- 模型选项卡 (像 3ds Max), 绘制: 搜索和“导航”菜单在此处以 Ctrl+= 和 Ctrl+- 表示缩放, 因为 [ 和 ] 用于设置画笔大小。

### 动画

- 模型选项卡的动画区域: 右侧面板按已确认的设计排列 - 动画、创建绑定、预设、编辑现有动画、传递 (重定向)、IK/FK、附加物与预览、为游戏生成; 其后折叠显示其余内容 (自动物理、姿势、残影、运动路径、曲线、姿势库、GLA、animation.cfg、目标 GLA 与槽位)。
- 创建绑定: 显示骨架、比例和识别出的模板; 可按组选择模板 (人类、Rancor、Wampa、Tauntaun、Howler、AT-ST、机器人、载具 …) 或自定义骨架。新增: 动画绑定 (仅 FK) 和放到绑定上 (动画变为控制器上的关键帧, 姿势保持不变)。
- 模型选项卡的动画: 与 CAT 绑定相同, “创建控制绑定”会把序列的动画从 FK 转到 IK 放到绑定上 - 手脚跟随其 IK 目标和极向, 动作看起来不变 (消息会给出最大偏差)。之后可在任意帧上操作控制器 (手或脚的目标、极向、脚部滚动); 只有该帧会改变。
- 预设: 动画库在最前 - 类别、带来源和许可的列表、可选的目标槽位、在角色上预览和应用到角色 (重定向); 生成器通过其按钮打开。
- 预设显示设计中的八个类别 (站立、行走、跑步、后退、蹲伏、跳跃、战斗、手势, 其后是其他); 还没有预设的类别会显示简短提示。
- 自带生成器以“行走 (生成)”和“跑步 (生成)”出现在列表中: 速度和摆臂在列表下方, “在角色上预览”和“应用到角色”与预设相同, 其余设置在“生成器...”中; 右键单击条目会提供其命令。
- 编辑现有动画: 将帧精简为关键帧和烘焙到 GLA 集中在一个组中, 并显示来源、帧和关键帧。
- 3ds Max 风格的 IK/FK: 每个肢体显示文字状态和一个 IK/FK 按钮; Blender 风格: 滑块和两个对齐按钮在同一行。
- 与 CAT 相同的每个肢体 IK/FK: 按钮从当前帧起直到下一次切换, 切换此肢体, 并先匹配另一侧 - 不会跳变。
- “为游戏生成”显示序列的根位移 (骨盆移动的距离) 并提醒 SET_ORIGIN。
- 动画,地图 GLA 分组:“目标、地图、名称”三个字段现在从同一条边开始。
- 浅色主题: 所选预设重新清晰可读。
- 模型选项卡, 动画: 在“创建控制绑定”之后使用“动画绑定 (仅 FK)”时, 会先把每个肢体的 IK 烘焙为 FK 关键帧 - 动作完全不变, 每个旋转控制器都能旋转其骨骼, Ctrl+Z 一步恢复 IK 目标。
- 模型选项卡: 修复了 Q 移除操纵器时绑定控制器上的拖动不结束; 修复了拖动生成步态速度时按 Ctrl+Z; 修复了分离附加物后场景资源管理器绘制超出列表末尾的行; 窗口很小之后右侧面板所选的选项卡保持不变。

### 骨骼与绑定样式

- 模型选项卡: 骨骼绘制在视图画面中。同 3ds Max 为骨骼方盒 (可选鳍), 同 Blender 可选显示为: 八面锥、棍形、B-骨骼、封套或线框。
- 骨骼和控制器默认位于角色前面; 关闭“在前面”后角色会遮住它们。
- 透明显示: Alt+X 同 3ds Max (所有视图中的角色), Alt+Z 同 Blender (当前视图的透视模式)。角色以半透明覆盖其后方内容。
- 动画区域的绑定样式: 标准、带圆环的控制绑定、CAT 方盒或 Blender 八面锥。控制器按侧着色; 样式只改变外观, 从不改变绑定。
- 单击或拖动命中骨骼或控制器的可见形状: 圆环、方盒的边或内部、极向量的球、八面锥。
- 在 Direct3D 12 和 Direct3D 11 下外观完全相同。

### 助手、搜索和手册

- 模型选项卡, 搜索所有操作: 在多个区域中同名的操作会在右侧标出所属区域 (涂抹 - Weights, 涂抹 - Painting); 与 Blender 相同时, 播放/停止只出现一次。
- 助手 (你想做什么?) 认识模型选项卡右侧面板的每个分组、按钮、开关和滑块, 并指引到那里: 模型选项卡、区域、分组、元素。
- 手册: 模型选项卡的页面按工作顺序排列 - 概览、操作方式、与 Blender 相同的布局、时间轴、文件, 然后是查看/检查、建模、UV、Weights、自动蒙皮、Painting 和动画各区域; 新增与 Blender 相同的布局、查看/检查、动画面板和自动蒙皮页面。

## 日本語


### タブ全体

- モデルタブの右パネル: 見た目も操作モードに従うようになりました。3ds Max 風では「作成・修正・階層・モーション・表示・ユーティリティ」のタブを持つコマンドパネル、Blender 風では縦一列のアイコン(ツール、オブジェクト、モディファイアー、データ、ボーン、コンストレイント、マテリアル)を持つプロパティエディターです。各エリアは自分のタブを開いて全グループを表示し、ほかのタブにはそれぞれの担当分が表示されます。
- グループの形をすべて統一: 折りたためる見出しと右側の短い情報(三角形数、描画中の LOD、取り付けたモデルの数、チェックの結果)、アイコンとキー付きの同じ幅 2 列のボタングリッド、表形式の値、バー表示の制限、枠付きのヒント。
- パネルが広くなり(文字の高さ 17 個分から 22 個分へ)、ビューとの境目をドラッグして幅を変えられます。ダブルクリックで既定に戻ります。
- 「表示/チェック、モデリング、UV、ウェイト、ペイント、アニメーション」の各エリアは、同じ幅のフィールドが並ぶ 1 行としてタブ全体の幅に表示され、その横に操作モードとエリアに応じた短いヒントが出ます。
- 「3ds Max 風 / Blender 風」の切り替えはパネルの外に移りました。タブのステータス行、ビューのヘッダーのメニュー、設定、すべてのアクションの検索にあります。
- モデルタブ、右側のパネル: 値の表、ボタン、スイッチは、狭いパネルでも単語の間でだけ折り返します。単語が収まらないときは、ボタンのグリッドの列が減ります。
- モデルタブ、右側のパネル: 小さいウィンドウでは 3ds Max 風のコマンドパネルのタブをアイコンで 1 列に並べ、名前はツールチップに表示します (Blender のプロパティタブと同じ)。文字では 2 段か 3 段必要でした。
- ビュー右下に詳細レベルと描画中の三角形数。3ds Max 風ではモデリング、ウェイト、ペイント、アニメーションが 4 つのビューで始まり、Blender 風では Z でシェーディングを選び Shift+Alt+Z でオーバーレイを隠します。.glm は常に表示/チェックで開きます。
- モデルタブ (3ds Max 風): 「ボーンを選択」は「距離を測定」と同じスイッチです。「すべてのアクションを検索」と右クリックメニュー (チェックマーク付き) にあり、オンの間はビューの上部にヒントが出ます。Q、W、E、R で終了します。

### シーンエクスプローラー

- モデルタブのシーンエクスプローラー: すべてを 1 本のツリーに - モデル、階層付きのサーフェス、取り付けたモデル付きのタグ、ボーン、リグのハンドル、animation.cfg のシーケンス (別のアニメーション一覧はなくなりました)。列の幅は承認済みのデザインと同じです。
- 3ds Max と同じ: 矢印・目・記号が名前のすぐ前に並び、階層ごとに記号 1 つ分だけ字下げ、雪の結晶の「フリーズ」列、並べ替えとドラッグができる列見出し。Blender と同じ: 目と「選択可能」の矢印が右の列にあるアウトライナーとフィルターのポップオーバー。切り替えは 階層 / コレクション。
- フリーズ: フリーズしたサーフェス、タグ、ボーンは表示されたままですが、ツリーでもビューでも選択できません。キャップは最初からフリーズされています。「レイヤー順」はボディ、キャップ、タグ、ボーン、取り付け、その他の LOD を、それぞれ目と雪の結晶付きで表示します。
- ビューでのクリックはフリーズしたサーフェスを通り抜け、その後ろのサーフェスを選択します (3ds Max と同じ)。
- ツリーのシーケンスにはプライマリ / セカンダリを示す錠 P / S と、フレームが GLA の終わりを超える場合の警告が付きます。グループ一覧、名前のコピー、スクリプトへ。
- ツリーは横にスクロールしません: 深いボーンの連なりは間隔が詰まり、長い名前は「...」で終わって、行の情報と一緒にツールチップに全体が表示されます。種類でフィルター、Shift+クリックで範囲選択、右クリックメニューに「情報」。
- 左のモードバーのツールチップが隣のパネルの上に飛び出さなくなりました: 少し止まると、バーの横・ボタンの高さに表示されます。

### Blender 風のレイアウト

- Blender 風のモデルタブ: 左の列はツールバー (T) で、選択、移動、回転、拡大縮小とエリアのツールが並びます - メジャー、サーフェスを選択、ボーンを選択、写真、押し出し、インセット、ベベル、ループカット、ナイフ、マージ、シームを設定、展開、リラックス、パック、ブラシ、ぼかし、グラデーション、ウェイトを取得、ドロー、塗りつぶし、クローン、スミア、マスク、ポーズをコピー、ミラー貼り付け、間引き。
- Blender 風のツールバーは Blender のツール名を使い、ドイツ語、スペイン語、チェコ語、日本語、中国語では Blender 自身の翻訳を使います。
- Blender と同じツールグループ: 小さな三角がグループを示し、長押しかドラッグで開きます (押し出し（領域）/（法線方向）/（個別）、ナイフ / 二分割)。ツールバーを右クリックするとツール名を表示できます。
- Blender 風ではアウトライナーが右上、プロパティエディターの上にあります。間の境界はドラッグできます。
- ビューのヘッダーは Blender と同じくモードを示します (オブジェクトモード、編集モード、ウェイトペイント、テクスチャペイント、ポーズモード)。クリックするとエリアが切り替わります。
- N でサイドバーが開きます: 選択の位置と回転または寸法、最後の測定、視野角と投影。
- Blender と同じくサイドバー (N) は数値フィールドです: サーフェスの位置と寸法、ポーズモードのリグコントロールの位置と回転をドラッグまたは入力でき、Ctrl+Z で 1 回の入力を 1 段階で元に戻します。
- 新しいツール「メジャー」: モデル上で 2 回クリックするかドラッグすると、ゲーム単位の距離を表示します。3ds Max 風では「距離を測定」として「すべてのアクションを検索」と右クリックメニューにあります。
- 各操作モードはウィンドウの配置をそれぞれ保持します。切り替えても選択、シーケンス、フレームは保持されます。3ds Max 風のレイアウトは変わりません。
- ドイツ語: Blender 風のメニュー Overlays は、Blender のドイツ語版と同じ Überlagerungen になりました。

### タイムライン

- モデルタブ: タイムラインが承認済みのデザインどおりになり、選んだ操作モードに従います。3ds Max 風: 先頭へ、前のフレーム、再生、次のフレーム、末尾へ、0 から数えるフレーム表示、Auto Key (N)、Set Key (K)、時間設定、スケールと各トラックにフレームごとの細い線、キーはトラックバーの色の棒 (赤 = 移動、緑 = 回転)。Blender 風: 前後のキーへのジャンプ (↓ / ↑)、自動キー挿入の録画ボタン、1 から数える開始 / 終了 / フレーム、ひし形のキー、K でキーイングメニュー。
- モデルタブ: トラック (ドープシート、カーブエディター) はタイムラインの一部になりました - 定規は 1 本で、フレーム 0 はスケールとトラックで同じ位置です。タイムライン上のハンドルまたは「トラック」ボタンで開き、アニメーションエリアでは最初から開いています。「表示/確認」は速度、補間、ループを強制、プライマリとセカンダリのシーケンスを表示し、他のエリアは短いタイムラインです。
- モデルタブ: Auto Key をオフにできます - ポーズは Set Key (K、Blender 風では I) で保持するまでキーなしのままで、フレームを変えると破棄されます (3ds Max、Blender と同じ)。
- モデルタブ: Auto Key (Blender 風では自動キー挿入) は 3ds Max や Blender と同じく最初はオフです。選択は記憶されます。
- モデルタブ: タイムラインの一番下にステータス行 (操作モード - クリックで切り替え、エリアのキー、元に戻すと検索)。時間設定にはフレームレート、フレームまたは秒での時間表示、再生、animation.cfg のシーケンスの行があります。スキン一覧とサーフェス数は「詳細レベルとスキン」グループに移りました。
- モデルタブ: 小さいウィンドウでタイムラインが高さの半分近くを占めなくなりました - ウィンドウの高さが 50 文字分より低いときはタイムラインがその割合を保ち、トラックは低くなってスクロールし、上のパネルは見出しより多くを表示します。

### 表示/チェック

- モデルタブの表示/チェック: デザインのグループ - モデル (インポート... 付き)、選択、詳細レベルとスキン、床とカメラ、アタッチメント、ゲーム向けチェック、シーン。
- ゲームの r_lodbias と同じく 10 段階の詳細レベルをすべて選べます。モデルの最後のレベルより上は最後のレベルを表示し、グループがそれを示します。スキンは 2 列の選択です。
- 床とカメラ: 床を -24 またはポーズの最低点の下に、視野角 10°/80°/90°、単色の背景、ウェイトしきい値、ビューをリセット、写真 (1920+)。
- ゲーム向けチェック: 選択したサーフェスの対象ゲーム上限に対するバー、サーフェス数、GLA 内のシーケンス、ゲームコードが探すタグ、画像のないシェーダー、LOD、自前の影を落とさないサーフェス。
- シーン: モデル、スキン、アタッチメント、シーケンス、切り替えたサーフェスを ModView スクリプト (.mvs) として保存し、ModView スクリプトを読み込みます。
- ModView で足りなかった 5 つのチェック表示がビューのヘッダーメニューにあります: アルファブレンド、テクスチャフィルター、ボーンウェイトのしきい値、ボーン参照付きのサーフェス強調、影を落とさないサーフェス。

### モデリング

- モデルタブのモデリング: 右側のパネルを承認済みのデザインどおりにしました - 「選択」「ツール」「JKA の上限」の 3 グループです。レベルはキーを前に付けた 1 列のマスで並び(3ds Max 風は 1〜5、Blender 風は 1〜3)、6 つの主要ツールは記号と操作モードのキーが付いた同じ幅のボタン、その後にレベルのその他のツールが続き、残りは「ジオメトリ編集」に折りたたまれています。
- 現在のレベルで使えないツールは灰色になり、ツールチップに使えるレベルが書かれます。
- JKA の上限: 選択したサーフェスのバー(頂点・ボーン・ウェイトをゲームの上限と比べ、90 % から黄色、超えると赤)と見出しの短い表示。全サーフェスの一覧は折りたたまれています(83 パーツのキャラクターでは以前 8 ページ分ありました)。
- プリミティブと小道具は「作成」タブ(3ds Max 風)または「オブジェクト」タブ(Blender 風)にあり、プリミティブごとにボタンがあります。小道具を開いている間は「マップへ」がワンクリックのままです。
- Ctrl+Enter でツールを適用し、結果に同じツールをもう一度始めます(ツールボックスの「+」ボタン)。
- Blender 風: E が Blender と同じく押し出しです(Alt+E も引き続き使えます)。Shift+A でプリミティブの「追加」メニューが開きます。
- モデルタブ: サーフェスごとにモディファイアスタックがあります。3ds Max 風では「モディファイアスタック」グループが「修正」タブの一番上 (モデリングエリアと Weights エリア) にあり、下から上へ読みます: ファイルのサーフェス、Edit Poly、モディファイア、Skin - Edit Poly または Skin をクリックするとエリアが切り替わります。Blender 風では「モディファイア」グループが「モディファイアー」タブにあり、モディファイアを上から下へ並べます。
- モディファイアは 2 種類: シンメトリ (Blender 風: ミラー) は軸平面でサーフェスを切り、ボーンウェイトも含めて片側をもう片側にミラーします。ターボスムーズ (Blender 風: サブディビジョン) はサーフェスを 1〜3 回細分して丸め、開いた境界をその場に残すこともできます。
- 各モディファイアには目のアイコン (オン/オフ) と「ファイルに書き出す」があります。スムーズは最初プレビューのみです。モディファイアは上下に移動 (ドラッグも可)、適用 (Blender 風: 適用、3ds Max 風: ここまで集約・すべて集約)、削除ができ、どれも元に戻す 1 ステップです。コマンドは行の右クリックメニュー、ビューの右クリックメニュー、「すべての操作を検索」にもあります。
- ビューがモディファイアについて表示するものは、適用して得られるメッシュそのものです。ファイルに書き出すモディファイアがサーフェスにない間は、保存で書かれるファイルはこれまでと同じです。
- JKA の上限と保存前のチェックは、ファイルに書き出すモディファイアを含め、ファイルに入る内容を数えます。
- モデリングの対称化: ミラーされた側は、スケルトンのすべてのボーンの対 (lhumerus と rhumerus、ltibia と rtibia など) で自分の側のボーンに付くようになりました。これまでは l_ と r_ で始まる名前だけを入れ替えていました。

### UV

- モデルタブの UV エリア: 右のパネルが承認済みのデザインどおりになりました。「選択」、番号付きの手順 1 シームを決める・2 展開・3 仕上げ、そして「チェック」。ボタンは同じ幅の 2 列で、アイコンと操作モードのキー付き。「展開」は強調されたメインボタンで、手順にはヒントの枠があります。
- 「選択」は見出しにテクスチャとアイランドの数を表示します。新機能: アイランドを選択(選択をアイランド全体に広げる)とすべて選択。
- クイック展開と投影はそれぞれ 1 つのボタンになり、テンプレート(キャラクター、小道具、ハードサーフェス)と投影方法(平面、円柱、球、ボックス)をすぐ下で選びます。あまり使わないコマンドは各手順の「その他」に折りたたまれています。
- チェック: 見出しに「問題なし」または指摘の件数。ストレッチ、重なり、新しいテクスチャ使用率(パーセント)、テクセル密度が表で並びます。UV エディター用の新しいスイッチ「シームを太く」。
- UV エリア: 「シームを太く」が 3D ビューのシームにも効くようになりました。
- Blender モード: L でマウス下のアイランドを選択、A ですべて選択、Ctrl+A で密度をそろえ、Ctrl+P でパック。3ds Max モード: Ctrl+A ですべて選択。

### Weights とオートスキン

- モデルタブの Weights エリア: 右のパネルが承認済みのデザインに沿って並びます。3ds Max 風: モディファイアスタック、ウェイトツール、ミラー、オートスキン、ボーン、JKA の制限、閉じた詳細パラメータ。Blender 風: ブラシ、オプション、クリーンアップ、ミラー、オートスキン、ボーン、JKA の制限。
- 3ds Max 風のウェイトツール: 「ウェイトペイント」でペイントをオン/オフします。固定値 0〜1 は選択した頂点にすぐウェイトを設定します。新たにスケール、ブレンド、拡大、縮小、リング、ループ。
- Weights: メインボタン「ウェイトペイント」は、ペイントがオフでも位置と高さを保ちます (オフのときは枠だけが強調されます)。
- Blender 風の新しいブラシ: グラデーション (線形または放射状) とスミア。自動正規化は頂点のウェイトの合計を 1 に保ちます (初期設定でオン)。
- Blender 風のクリーンアップ: すべて正規化、クリーン、スムーズ、制限: 4 を、選択した頂点またはすべてに適用。
- 3ds Max 風のミラー (ミラーモードは左が青、右が緑、対応のない頂点は黄色。青から緑、緑から青、左右を入れ替え、選択のみ、許容値) と Blender 風のミラー (グループまたは全グループをミラー、名前を反転、トポロジーミラー)。別々のサーフェスで同じ位置にある頂点は 1 つとして扱います。
- ボーンをロックできます (Blender 風: K でロックメニュー)。このエリアではボーンが最初から表示され、ゲームの制限がバーで示されます。
- モデルタブ、Weights: オートスキン。「自動でウェイト付け」は、スケルトンからメッシュにボーンウェイトを付けます (元に戻すは 1 回)。方式は 3 つ: 同じスケルトンを持つゲームのオリジナルから転送 (例: kyle)、測地 (各ボーンまでの距離をボクセルでメッシュの内部を通して測るので、腕が体にくっつきません)、エンベロープ (直線距離、高速)。ツイストボーン (lhumerusX、lradiusX、lfemurX) は、Raven のフィギュアと同じく手足の次の関節側の半分を受け持ちます。
- オートスキン: 結果は常にゲームに合います。頂点ごとに最大 4 個、サーフェスごとに最大 32 ボーン、ゲームが読むとおり 10 ビットで保存。同じ位置の頂点は同じウェイトになるので、サーフェス間の継ぎ目は閉じたままです。
- オートスキン: 解像度、硬さ、スムーズ、継ぎ目、ツイストボーン、顔と付属ボーン、あとでミラー、「選択したサーフェスのみ」(完成したフィギュアの新しい鎧のパーツ) を設定できます。ウィンドウに結果が表示されます: ゲームの制限と、頂点が前のウェイトからどれだけ離れているか (ユニット)。
- オートスキン: 「自動でウェイト付け」と「オートスキン: 設定」は、すべてのアクションの検索 (X、Blender 風は F3) と、ビューの右クリックの Weights の下にあります。
- モデルタブ、Weights: ペイントリストが開いたまま(ペイントやウェイトのコマンドの後、「適用」の前)でも、保存するとビューに表示されているウェイトが書き込まれます。ファイルには頂点ごとに最大 4 つのウェイトが、ゲームが読む形に正規化されて入り、ペイントリストは開いたままです。保存前のチェックと JKA の上限も同じものを測ります。これまでは保存するとペイント前のウェイトが書き込まれ、「保存しました」と表示されていました。
- Weights: ストローク、グラデーション、ギズモのドラッグ、ツールボックスが開いている間は、ホイールでズームしたり「自動」が切り替えようとしても詳細レベルは変わりません。これまではストローク中にレベルが変わると、ミラーペイントが別の頂点に書き込んだりクラッシュしたりすることがありました。
- Weights とモデリング: 頂点・エッジ・ポリゴンの選択はその詳細レベルに属します。レベルが変わると選択は空になり、そのレベルのメッシュが変更されていなければ戻ったときに元に戻ります。これまでは番号が残り、コマンドが別の頂点を変更していました。
- モデリング: ペイント済みで未適用のウェイトを持つサーフェスにツールを使っても、ウェイトは保たれます(これまでは元に戻っていました)。
- オートスキン: 「選択したサーフェスのみ」と「あとでミラー」では、選択したサーフェスにだけミラーし、ほかのサーフェスの開いているペイントリストはそのままです。
- ウェイトのストローク、グラデーション、ギズモのドラッグ、テクスチャペイントのストロークの最中に Ctrl+Z を押すと、それを終えて元に戻します。Ctrl+Y でやり直せます。これまでは押したままのボタンが元に戻すステップなしで書き込みを続けていました。
- 細かな修正: 何も変えないコマンドはやり直しを残します。人物の横から始めたストロークも選んだモードで塗ります。新しく読み込んだモデルではロックされたボーンが解除されます。Painting で入力したブラシ半径 0 は 1 として扱います。

### Painting

- モデルタブ、ペイント: 右のパネルが承認済みのデザインどおりになりました。「ターゲット・ツール・ブラシ・レイヤー・ゲーム用」の各グループが作業の順に並び、ツールはアイコン付きボタンのグリッド (選択中のものを強調)、色・半径・強さ・間隔はスライダー付きの行、硬さ・ミラー・筆圧・裏面・にじみは「詳細設定」の下にあります。
- レイヤーは行として並び、目のアイコンで表示/非表示を切り替え、オリジナルには錠が付きます。選択中のレイヤーの描画モードと不透明度はリストの下に、ヒントの枠はグループの下にあります。
- [ と ] キーで、どちらの操作モードでもブラシを小さく/大きくできます (すべてのアクションの検索とステータス行にもあります)。
- 3ds Max 風ではグループは「修正」タブに (「ゲーム用」は「ユーティリティ」にも)、Blender 風では「ツール」タブに (「ターゲット」は「マテリアル」にも) あります。
- 長いパスは先頭側を省略し (「.../kyle/body.tga」)、出力フォルダーはパスの末尾を表示します。完全なパスはツールチップにあります。
- モデルタブ (3ds Max 風)、ペイント: [ と ] がブラシサイズを設定するため、ここでは検索とナビゲーションメニューがズームに Ctrl+= と Ctrl+- を示します。

### アニメーション

- モデルタブのアニメーションエリア: 右のパネルが承認済みのデザインに沿って並びます - アニメーション、リグを作成、プリセット、既存のアニメーションを編集、転送 (リターゲット)、IK/FK、取り付けとプレビュー、ゲーム用に作成。その後ろに閉じた状態でその他すべて (オートフィジックス、ポーズ、ゴースト、モーションパス、カーブ、ポーズライブラリ、GLA、animation.cfg、出力先 GLA とスロット)。
- リグを作成: スケルトン、スケール、認識されたテンプレートを表示します。テンプレートはグループ (人型、Rancor、Wampa、Tauntaun、Howler、AT-ST、ドロイド、乗り物 …) または独自スケルトンから選べます。新たにアニメーションリグ (FK のみ) とリグに載せる (アニメーションがコントロールのキーになり、ポーズはそのまま)。
- モデルタブのアニメーション: CAT リグと同じく、「コントロールリグを作成」がシーケンスのアニメーションを FK から IK に移してリグに載せます - 手足は IK ターゲットとポールに追従し、動きは同じに見えます (メッセージに最大のずれを表示)。その後、どのフレームでもコントロール (手や足のターゲット、ポール、フットロール) を操作でき、変わるのはそのフレームだけです。
- プリセット: ライブラリが先頭に - カテゴリ、出典とライセンス付きの一覧、選べる出力先スロット、フィギュアでプレビューとフィギュアに載せる (リターゲット)。ジェネレーターはボタンで開きます。
- プリセットはデザインの 8 つのカテゴリ (待機、歩き、走り、後退、しゃがみ、ジャンプ、戦闘、ジェスチャー、その後ろにその他) を表示します。まだプリセットのないカテゴリには短い案内が出ます。
- 独自ジェネレーターは「歩き (生成)」「走り (生成)」として一覧に並びます: 速度と腕の振りは一覧の下、「フィギュアでプレビュー」と「フィギュアに載せる」はプリセットと同じ、その他は「ジェネレーター...」。項目の右クリックでそのコマンドを選べます。
- 既存のアニメーションを編集: 「フレームをキーに削減」と「GLA にベイク」を、ソース・フレーム・キーと一緒に 1 つのグループに。
- 3ds Max 風の IK/FK: 肢ごとに状態を文字で示し、IK/FK ボタンが 1 つ。Blender 風: スライダーと 2 つのスナップボタンを 1 行に。
- CAT と同じ手足ごとの IK/FK: ボタンは現在のフレームから次の切り替えまで手足を切り替え、先にもう一方を合わせます - ジャンプしません。
- 「ゲーム用に作成」はシーケンスのルートの移動 (骨盤の移動距離) を示し、SET_ORIGIN を案内します。
- アニメーション、マップ GLA のグループ: 「出力先・マップ・名前」のフィールドが同じ位置から始まるようになりました。
- ライトテーマ: 選択したプリセットが再び読みやすくなりました。
- モデルタブ、アニメーション: 「コントロールリグを作成」の後の「アニメーションリグ (FK のみ)」は、各手足の IK を先に FK キーへベイクします。動きはまったく変わらず、各回転コントロールがボーンを回し、Ctrl+Z で IK ターゲットが 1 ステップで戻ります。
- モデルタブ: Q でギズモが消えたときにリグコントロールのドラッグが終わらない問題、生成した歩行の速度をドラッグ中に Ctrl+Z を押したときの問題、取り付けを外した後にシーンエクスプローラーがリストの末尾を越えて行を描く問題を修正しました。ウィンドウをとても小さくした後も右パネルで選んだタブが保たれます。

### ボーンとリグのスタイル

- モデルタブ: ボーンをビューの画像内に描きます。3ds Max 風はボーンボックス (フィンは任意)、Blender 風は表示方法: 八面体、スティック、Bボーン、エンベロープ、ワイヤー。
- ボーンとコントロールは既定でフィギュアの手前。「最前面」をオフにするとフィギュアが隠します。
- シースルー: Alt+X は 3ds Max 風 (全ビューのフィギュア)、Alt+Z は Blender 風 (アクティブビューの透過表示)。フィギュアは背後の上に半透明で重なります。
- アニメーション領域のリグのスタイル: 標準、リング付きコントロールリグ、CAT ボックス、Blender 八面体。コントロールは左右で色分け。スタイルは見た目だけを変え、リグは変えません。
- クリックとドラッグはボーンやコントロールの見えている形に当たります: リング、ボックスの辺や内側、ポールの球、八面体。
- Direct3D 12 と Direct3D 11 で同じ見た目です。

### ヘルパー、検索、ガイド

- モデルタブ、すべての操作を検索: 複数のエリアで同じ名前の操作は、右側にエリアを表示します (スミア - Weights、スミア - Painting)。Blender 風では再生/停止は 1 行だけです。
- ヘルパー (何をしたいですか?) は、モデルタブの右側のパネルのすべてのグループ、ボタン、スイッチ、スライダーを知っていて、そこへ案内します: モデルタブ、エリア、グループ、要素。
- ガイド: モデルタブのページを作業の順に並べました - 概要、操作、Blender 風のレイアウト、タイムライン、ファイル、続いて 表示/チェック、モデリング、UV、Weights、オートスキン、Painting、アニメーションの各エリア。Blender 風のレイアウト、表示/チェック、アニメーションのパネル、オートスキンのページを追加しました。

## Español


### La pestaña en conjunto

- Pestaña Model, panel derecho: ahora también sigue el modo de control en su aspecto. Como 3ds Max es el panel de comandos con las pestañas Crear, Modificar, Jerarquía, Movimiento, Presentación y Utilidades; como Blender es el editor de propiedades con una columna vertical de símbolos (Herramienta, Objeto, Modificadores, Datos, Hueso, Restricciones, Material). Cada área abre su pestaña y muestra allí todos sus grupos; las demás pestañas muestran su parte.
- Grupos con una misma forma en todas partes: un título que se pliega con un dato breve a la derecha (triángulos, LOD dibujado, número de modelos acoplados, avisos de la comprobación), botones en una cuadrícula de dos columnas iguales con símbolo y tecla, valores en tabla, límites como barras e indicaciones en un recuadro.
- El panel es más ancho (22 en lugar de 17 alturas de carácter) y su ancho se puede arrastrar en el borde con las vistas; un doble clic restablece el valor predeterminado.
- Las áreas Ver/Comprobar, Modelado, UV, Pesos, Pintura y Animación forman una fila de campos de igual ancho a lo largo de toda la pestaña, con una indicación breve para el modo de control y el área al lado.
- El conmutador como 3ds Max / como Blender ha salido del panel: está en la línea de estado de la pestaña, en el menú de la cabecera de una vista, en los ajustes y en la búsqueda de todas las acciones.
- Pestaña Model, panel derecho: las tablas de valores, los botones y los interruptores de un panel estrecho solo se parten entre palabras; una cuadrícula de botones usa menos columnas cuando una palabra no cabría.
- Pestaña Model, panel derecho: en una ventana pequeña las pestañas del panel de comandos como 3ds Max aparecen como símbolos en una sola fila, con el nombre en la información emergente (como las pestañas de propiedades de Blender); como texto necesitaban dos o tres filas.
- Abajo a la derecha en la vista: nivel de detalle y triángulos dibujados. Como 3ds Max, Modelado, Weights, Painting y Animación empiezan con cuatro vistas; como Blender, Z elige el sombreado y Mayús+Alt+Z oculta las superposiciones. Un .glm siempre se abre en Ver/Comprobar.
- Pestaña Model como 3ds Max: Seleccionar hueso es un interruptor como Medir distancia - en Buscar todas las acciones y en el menú contextual (con marca), con un aviso arriba en la vista mientras está activo; Q, W, E y R lo terminan.

### Explorador de escena

- Pestaña Model, explorador de escena: un árbol para todo: el modelo, sus superficies con su jerarquía, tags con modelos acoplados, huesos, controles del rig y las secuencias del animation.cfg (desaparece la lista de animaciones aparte). La columna es tan ancha como en el diseño aprobado.
- Como 3ds Max: la flecha, el ojo y el símbolo están justo delante del nombre, cada nivel se sangra un símbolo, columna «Congelado» con copo de nieve, encabezados de columna que ordenan y se pueden arrastrar. Como Blender: un listado con el ojo y la flecha «seleccionable» en columnas a la derecha y un desplegable de filtro; el conmutador dice Jerarquía / Colecciones.
- Congelar: las superficies, tags y huesos congelados siguen visibles, pero no se pueden seleccionar en el árbol ni en la vista; las tapas están congeladas desde el principio. «Por capas» muestra Cuerpo, Tapas, Tags, Huesos, Acoples y los demás LOD, cada una con ojo y copo de nieve.
- Un clic en la vista atraviesa una superficie congelada y selecciona la superficie de detrás, como 3ds Max.
- Las secuencias del árbol llevan el candado P / S de la secuencia primaria y secundaria y una advertencia cuando sus fotogramas quedan detrás del final del GLA; lista de grupos, copiar el nombre, al script.
- El árbol nunca se desplaza de lado: las cadenas de huesos profundas se juntan más, los nombres largos terminan en «...» y aparecen enteros en la descripción emergente junto con la información de la fila. Filtro por tipo, Mayús+clic selecciona un rango, «Información» en el menú del clic derecho.
- Las descripciones emergentes de la barra de modos de la izquierda ya no saltan sobre el panel de al lado: aparecen tras una breve pausa, junto a la barra a la altura del botón.

### Disposición como Blender

- Pestaña Model como Blender: la columna de la izquierda es la barra de herramientas (T) con Seleccionar, Mover, Rotar, Escalar y las herramientas del área - Medir, Seleccionar superficie, Seleccionar hueso, Foto; Extruir, Insertar, Bisel, Corte en bucle, Cuchillo, Fusionar; Marcar costura, Desplegar, Relajar, Empaquetar; Pincel, Desenfocar, Degradado, Muestrear peso; Dibujar, Rellenar, Clonar, Difuminar, Máscara; Copiar pose, Pegar reflejada, Reducir.
- La barra de herramientas como Blender usa los nombres de herramientas de Blender y, en alemán, español, checo, japonés y chino, las traducciones propias de Blender.
- Grupos de herramientas como Blender: un pequeño triángulo marca un grupo, mantener pulsado o arrastrar lo abre (Extruir región / según normales / individualmente, Cortar / Biseccionar); un clic derecho en la barra muestra los nombres de las herramientas.
- Como Blender, el Outliner está arriba a la derecha sobre el editor de propiedades; el borde entre ambos se puede arrastrar.
- La cabecera de la vista nombra el modo como Blender (Modo Objeto, Modo Edición, Pintura de pesos, Pintura de texturas, Modo Pose); un clic cambia el área.
- N abre la barra lateral: ubicación y rotación o dimensiones de la selección, la última medición, campo de visión y proyección.
- Como Blender, la barra lateral (N) tiene campos numéricos: arrastra o escribe la posición y las dimensiones de una superficie o la posición y la rotación de un control del rig en modo pose; Ctrl+Z deshace una entrada en un paso.
- Nueva herramienta Medir: dos clics en el modelo o un arrastre muestran la distancia en unidades del juego; como 3ds Max, como Medir distancia en Buscar todas las acciones y en el menú contextual.
- Cada modo de control conserva su propia disposición de ventanas; al cambiar se mantienen la selección, la secuencia y el fotograma. La disposición como 3ds Max no cambia.
- Alemán: el menú Overlays como en Blender se llama Überlagerungen, como en la interfaz alemana de Blender.

### Línea de tiempo

- Pestaña Model: la línea de tiempo tiene ahora el aspecto del diseño aprobado y sigue los controles elegidos. Como 3ds Max: Al inicio, Fotograma anterior, Reproducir, Fotograma siguiente, Al final, indicador de fotograma desde 0, Auto Key (N), Set Key (K), Configuración de tiempo, una línea fina por fotograma en la escala y en cada pista, claves como barras con los colores de la barra de pistas (rojo = movimiento, verde = rotación). Como Blender: saltos a la clave anterior y siguiente (↓ / ↑), botón de grabación de claves automáticas, Inicio / Fin / Fotograma desde 1, claves como rombos, K abre el menú de claves.
- Pestaña Model: las pistas (hoja de claves, editor de curvas) forman parte de la línea de tiempo: una sola regla, el fotograma 0 en el mismo sitio en la escala y en las pistas; el tirador sobre la línea de tiempo o el botón Pistas las abre, y en el área Animación vienen abiertas. Ver/Comprobar muestra velocidad, interpolación, forzar bucle y las secuencias primaria y secundaria; las demás áreas muestran la línea de tiempo corta.
- Pestaña Model: Auto Key se puede desactivar: la pose queda sin clave hasta que Set Key (K, como Blender I) la conserva; cambiar de fotograma la descarta, como en 3ds Max y Blender.
- Pestaña Model: Auto Key (como Blender, claves automáticas) viene desactivado, como en 3ds Max y Blender; el programa recuerda tu elección.
- Pestaña Model: línea de estado al pie de la línea de tiempo (controles: clic para cambiar, teclas del área, deshacer y buscar); Configuración de tiempo con velocidad de fotogramas, tiempo en fotogramas o segundos, reproducción y la línea de la secuencia en animation.cfg; la lista de skins y el contador de superficies pasan al grupo Nivel de detalle y skin.
- Pestaña Model: en una ventana pequeña la línea de tiempo ya no ocupa casi la mitad de la altura: por debajo de 50 alturas de carácter de ventana conserva su proporción, las pistas se hacen más bajas y se desplazan, y los paneles de arriba muestran más que sus títulos.

### Ver/Comprobar

- Pestaña Model, Ver/Comprobar: los grupos del diseño - Modelo (con Importar...), Selección, Nivel de detalle y skin, Suelo y cámara, Acoplados, Comprobar para el juego, Escena.
- Los diez niveles de detalle se pueden elegir como r_lodbias en el juego; un nivel por encima del último del modelo muestra el último, y el grupo lo dice. Los skins son una elección en dos columnas.
- Suelo y cámara: suelo a -24 o bajo el punto más bajo de la pose, campo de visión de 10°, 80° o 90°, fondo liso, umbral de peso, Restablecer vista y Foto (1920+).
- Comprobar para el juego: barras de la superficie elegida frente a los límites del juego de destino, superficies, secuencias dentro de la GLA, los tags que busca el código del juego, shaders sin imagen, LOD y superficies sin sombra propia.
- Escena: guardar modelo, skin, acoplados, secuencias y superficies conmutadas como script de ModView (.mvs) y leer scripts de ModView.
- Los cinco indicadores de ModView que faltaban están en los menús de la cabecera de la vista: mezcla alfa, filtro de texturas, umbral de peso de hueso, resaltar superficie con sus referencias de huesos, superficies sin sombra propia.

### Modelado

- Pestaña Model, Modelado: el panel de la derecha sigue el diseño aprobado: los grupos Selección, Herramientas y Límites de JKA. Los niveles son una fila de casillas con su tecla delante (modo 3ds Max 1-5, modo Blender 1-3); las seis herramientas principales son botones de igual ancho con símbolo y la tecla del modo de control, después vienen las demás herramientas del nivel y el resto queda plegado en «Editar geometría».
- Una herramienta que no funciona en el nivel actual aparece atenuada, y su ayuda nombra los niveles en los que funciona.
- Límites de JKA: barras para la superficie seleccionada (vértices, huesos y pesos frente al límite del juego, amarillo desde el 90 %, rojo por encima) y una nota breve en el título; la lista de todas las superficies está plegada (con una figura de 83 partes ocupaba ocho páginas).
- Las primitivas y los accesorios están en la pestaña «Crear» (modo 3ds Max) u «Objeto» (modo Blender), un botón por primitiva; mientras haya un accesorio abierto, «Al mapa» sigue a un clic.
- Ctrl+Intro aplica una herramienta y la vuelve a empezar sobre el resultado (el botón «+» de la caja de herramienta).
- Modo Blender: E extruye como en Blender (Alt+E sigue funcionando), Mayús+A abre el menú «Añadir» con las primitivas.
- Pestaña Model: cada superficie tiene una pila de modificadores. En modo 3ds Max el grupo «Pila de modificadores» está arriba del todo en la pestaña Modificar, en el área de modelado y en el área Weights, y se lee de abajo arriba: la superficie del archivo, Edit Poly, los modificadores, Skin; un clic en Edit Poly o Skin cambia de área. En modo Blender el grupo «Modificadores» está en la pestaña Modificadores y enumera los modificadores de arriba abajo.
- Dos modificadores: Simetría (modo Blender: Espejo) corta la superficie en un plano de eje y refleja un lado sobre el otro, pesos de hueso incluidos; TurboSmooth (modo Blender: Subdivisión) subdivide y redondea la superficie de una a tres veces y puede dejar los bordes abiertos en su sitio.
- Cada modificador tiene el ojo (activado/desactivado) y «Escribir en el archivo»; el suavizado es al principio solo vista previa. Los modificadores se pueden subir y bajar (también arrastrando), aplicar (modo Blender: Aplicar; modo 3ds Max: Contraer hasta y Contraer todo) y quitar; cada una de estas acciones es un paso de deshacer. Los comandos están también en el menú contextual de una fila, en el menú contextual de la vista y en «Buscar todas las acciones».
- Lo que la vista muestra para un modificador es exactamente la malla que resulta de aplicarlo. Mientras una superficie no tenga un modificador que vaya al archivo, guardar escribe el mismo archivo que antes.
- Los límites de JKA y la comprobación antes de guardar cuentan lo que va al archivo, incluidos los modificadores que se escriben en él.
- Modelado, Simetrizar: el lado reflejado queda ahora unido a los huesos de su propio lado para cada par de huesos del esqueleto (lhumerus y rhumerus, ltibia y rtibia...); hasta ahora solo se intercambiaban los nombres que empiezan por l_ y r_.

### UV

- Pestaña Model, área UV: el panel de la derecha sigue el diseño aprobado: Selección, luego los pasos numerados 1 Fijar costuras, 2 Desplegar, 3 Ordenar y por último Comprobar. Botones de igual ancho en dos columnas con símbolo y la tecla del modo de control, Desplegar como botón principal destacado y un cuadro de consejo en los pasos.
- Selección muestra en su cabecera la textura y el número de islas; nuevo: Seleccionar isla (amplía la selección a islas enteras) y Seleccionar todo.
- Despliegue rápido y Proyectar son un botón cada uno; la plantilla (Personaje, Accesorio, Superficie dura) y la proyección (Plana, Cilindro, Esfera, Caja) se eligen justo debajo. Los comandos menos usados quedan plegados bajo Más en cada paso.
- Comprobar: la cabecera dice «todo bien» o el número de avisos; el estiramiento, los solapamientos, el nuevo uso de la textura en porcentaje y la densidad de téxeles aparecen como tabla; nuevo interruptor Costuras gruesas para el editor UV.
- Área UV: Costuras gruesas ahora también engrosa las costuras en la vista 3D.
- Como Blender: L selecciona la isla bajo el ratón, A selecciona todo, Ctrl+A iguala la densidad, Ctrl+P empaqueta. Como 3ds Max: Ctrl+A selecciona todo.

### Weights y Auto-skin

- Pestaña Model, área Weights: el panel de la derecha sigue el diseño aprobado. Como 3ds Max: pila de modificadores, herramienta de pesos, reflejar, auto-skin, hueso, límites de JKA y, cerrados, los parámetros avanzados. Como Blender: pincel, opciones, limpiar, reflejar, auto-skin, hueso, límites de JKA.
- Herramienta de pesos como 3ds Max: Pintar pesos activa y desactiva la pintura; los valores fijos de 0 a 1 ponen el peso en los vértices seleccionados al instante; nuevos Escalar, Mezclar, Ampliar, Reducir, Anillo y Bucle.
- Weights: el botón principal Pintar pesos conserva su sitio y su altura cuando la pintura está desactivada (entonces solo se resalta su marco).
- Pinceles nuevos como Blender: Degradado (lineal o radial) y Difuminar; Auto-normalizar mantiene en 1 la suma de pesos de un vértice (activado de serie).
- Limpiar como Blender: Normalizar todo, Limpiar, Suavizar y Limitar: 4, en los vértices seleccionados o en todos.
- Reflejar como 3ds Max (modo espejo izquierda azul, derecha verde, sin pareja amarillo; azul a verde, verde a azul, intercambiar lados, solo selección, tolerancia) y como Blender (reflejar grupo o todos los grupos, invertir nombres, reflejo por topología); los vértices en el mismo lugar de superficies distintas cuentan como uno.
- Los huesos se pueden bloquear (como Blender: K abre el menú de bloqueo); en esta área los huesos se ven desde el principio; los límites del juego aparecen como barras.
- Pestaña Model, Weights: Auto-skin. "Pesar automáticamente" da a una malla sus pesos de hueso a partir del esqueleto, en un paso de deshacer. Tres métodos: transferir de un original del juego con el mismo esqueleto (por ejemplo kyle), geodésico (la distancia a cada hueso se mide por el interior de la malla con vóxeles, así un brazo no se pega al cuerpo), envolvente (distancia en línea recta, rápido). Los huesos de torsión (lhumerusX, lradiusX, lfemurX) llevan la mitad de su miembro hacia la articulación siguiente, como en las figuras de Raven.
- Auto-skin: el resultado siempre encaja con el juego: como máximo 4 pesos por vértice, como máximo 32 huesos por superficie, guardados en 10 bits como los lee el juego; los vértices en el mismo lugar reciben los mismos pesos, así que las costuras entre superficies siguen cerradas.
- Auto-skin: ajustes de resolución, rigidez, suavizado, costuras, huesos de torsión, cara y apéndices, reflejar después y "Solo las superficies seleccionadas" (una pieza de armadura nueva en una figura terminada). La ventana muestra el resultado: los límites del juego y a qué distancia quedan los vértices de los pesos anteriores, en unidades.
- Auto-skin: "Pesar automáticamente" y "Auto-skin: ajustes" están en la búsqueda de todas las acciones (X, F3 en modo Blender) y en el clic derecho de la vista, bajo Weights.
- Pestaña Modelo, Weights: guardar escribe los pesos que muestra la vista, también mientras hay listas de pintura abiertas (tras pintar o un comando de pesos, antes de Aplicar). El archivo recibe como máximo cuatro pesos por vértice, normalizados como los lee el juego; las listas de pintura siguen abiertas. La comprobación antes de guardar y los límites de JKA miden lo mismo. Hasta ahora guardar escribía los pesos de antes de pintar y decía «guardado».
- Weights: mientras hay un trazo, degradado, arrastre del gizmo o cuadro de herramienta abierto, el nivel de detalle se mantiene, aunque la rueda haga zoom o «Auto» fuera a cambiarlo. El espejo al pintar podía escribir en vértices equivocados o cerrarse con un error cuando el nivel cambiaba durante un trazo.
- Weights y Modelado: una selección de vértices, aristas o polígonos pertenece a su nivel de detalle; tras un cambio de nivel está vacía y vuelve con ese nivel mientras su malla no haya cambiado. Hasta ahora los números se quedaban y los comandos cambiaban otros vértices.
- Modelado: una herramienta sobre una superficie con pesos pintados y aún no aplicados los conserva (hasta ahora volvían atrás).
- Auto-Skin: «Solo las superficies seleccionadas» con «Reflejar después» refleja solo en las superficies seleccionadas y deja abiertas las listas de pintura de las demás.
- Ctrl+Z durante un trazo de pesos, un degradado, un arrastre del gizmo o un trazo de Painting lo termina y lo deshace; Ctrl+Y lo rehace. Hasta ahora el botón pulsado seguía escribiendo sin paso de deshacer.
- Correcciones menores: un comando que no cambia nada conserva Rehacer; un trazo que empieza junto a la figura pinta con el modo elegido; los huesos bloqueados se desbloquean en un modelo recién cargado; un radio de pincel 0 escrito en Painting cuenta como 1.

### Painting

- Pestaña Model, Pintura: el panel derecho sigue el diseño aprobado. Los grupos Destino, Herramientas, Pincel, Capas y Para el juego siguen el orden del trabajo; las herramientas son una cuadrícula de botones con símbolo (la elegida queda resaltada), color, radio, fuerza y espaciado son filas con deslizadores, y dureza, simetría, presión del lápiz, caras traseras y sangrado están en Avanzado.
- Las capas son filas con un ojo para mostrarlas u ocultarlas y un candado en el original; el modo de fusión y la opacidad de la capa elegida están bajo la lista, y un recuadro de consejo bajo el grupo.
- Las teclas [ y ] hacen el pincel más pequeño y más grande en ambos modos de control (también en la búsqueda de todas las acciones y en la línea de estado).
- Como 3ds Max, los grupos están en la pestaña Modificar (Para el juego también en Utilidades); como Blender, en la pestaña Herramienta (Destino también en Material).
- Las rutas largas se acortan por delante (".../kyle/body.tga") y la carpeta de salida muestra el final de su ruta; la ruta completa está en la información emergente.
- Pestaña Model como 3ds Max, Pintura: la búsqueda y el menú Navegación indican allí Ctrl+= y Ctrl+- para el zoom, porque [ y ] ajustan el tamaño del pincel.

### Animación

- Pestaña Model, área Animación: el panel de la derecha sigue el diseño aprobado - Animación, Crear rig, Presets, Editar animación existente, Transferir (retargeting), IK/FK, Acoples y vista previa, Crear para el juego; detrás, cerrado, todo lo demás (física automática, pose, fantasmas, trayectorias, curvas, biblioteca de poses, GLA, animation.cfg, GLA de destino y huecos).
- Crear rig: muestra esqueleto, escala y la plantilla reconocida; la plantilla se puede elegir por grupo (humano, rancor, wampa, tauntaun, howler, AT-ST, droides, vehículos …) o Esqueleto propio. Nuevo: Rig de animación (solo FK) y Poner en el rig (la animación pasa a claves en los controles, la pose se mantiene).
- Pestaña Model, Animación: como un rig de CAT, Crear rig de control pone la animación de la secuencia en el rig de FK a IK - manos y pies siguen sus objetivos IK y polos, el movimiento se ve igual (el mensaje da la mayor desviación). Después se trabaja con los controles (objetivo de mano o pie, polo, giro del pie) en cualquier fotograma; solo cambia ese fotograma.
- Presets: la biblioteca va primero - categoría, lista con origen y licencia, hueco de destino como elección, Vista previa en la figura y Poner en la figura (retarget); el generador se abre con su botón.
- Los presets muestran las ocho categorías del diseño (Reposo, Andar, Correr, Hacia atrás, Agachado, Salto, Combate, Gestos y detrás Otros); una categoría aún sin presets muestra una nota breve.
- El generador propio está en la lista como Andar (generado) y Correr (generado): velocidad y balanceo de brazos bajo la lista, Vista previa en la figura y Poner en la figura como en los presets, Generador... para el resto; el clic derecho en una entrada ofrece sus órdenes.
- Editar animación existente: Reducir fotogramas a claves y Hornear en el GLA en un solo grupo con origen, fotogramas y claves.
- IK/FK como 3ds Max: por extremidad su estado en palabras y un botón IK/FK; como Blender: regulador y los dos botones de ajuste en una fila.
- IK/FK por extremidad como CAT: el botón cambia la extremidad desde el fotograma actual hasta el siguiente cambio e iguala antes el otro lado - nada salta.
- Crear para el juego muestra el recorrido de la raíz de la secuencia (cuánto avanza la pelvis) y recuerda SET_ORIGIN.
- Animación, grupo del GLA de mapa: los campos Destino, Mapa y Nombre empiezan ahora en el mismo borde.
- Tema claro: el preset elegido vuelve a leerse bien.
- Pestaña Model, Animación: Rig de animación (solo FK) después de Crear rig de control hornea primero el IK de cada extremidad en claves FK; el movimiento queda exactamente igual, cada control de giro gira su hueso y Ctrl+Z recupera los objetivos IK en un paso.
- Pestaña Model: corregido que un arrastre del gizmo en un control del rig quedara abierto cuando Q quitaba el gizmo; Ctrl+Z al arrastrar la velocidad de un paso generado; el Explorador de escena dibujaba filas tras el final de la lista al soltar un acople; la pestaña elegida del panel derecho se mantiene tras una ventana muy pequeña.

### Huesos y estilos de rig

- Pestaña Model: los huesos se dibujan en la imagen de la vista. Como 3ds Max como cajas de hueso (aletas a elección), como Blender con Mostrar como: Octaédrico, Varilla, B-Bone, Envolvente o Alambre.
- Huesos y controles quedan por defecto delante de la figura; con Al frente desactivado la figura los tapa.
- Ver a través: Alt+X como 3ds Max (la figura en todas las vistas), Alt+Z como Blender (rayos X de la vista activa). La figura queda semitransparente sobre lo que hay detrás.
- Estilo de rig en el área Animación: Estándar, rig de control con anillos, cajas CAT u octaedros de Blender. Los controles se colorean por lado; el estilo solo cambia el aspecto, nunca el rig.
- Un clic o arrastre acierta la forma visible de un hueso o control: un anillo, el borde o el interior de una caja, la esfera de un polo, un octaedro.
- Todo se ve igual con Direct3D 12 y Direct3D 11.

### Asistente, búsqueda y guía

- Pestaña Model, Buscar todas las acciones: las acciones que se llaman igual en varias áreas indican su área a la derecha (Difuminar - Weights, Difuminar - Painting); como en Blender, Reproducir/Detener aparece una sola vez.
- El asistente (¿Qué quieres hacer?) conoce cada grupo, botón, interruptor y deslizador del panel derecho de la pestaña Model y lleva hasta allí: pestaña Model, área, grupo, elemento.
- Guía: las páginas de la pestaña Model siguen el orden del trabajo: vista general, controles, disposición como Blender, línea de tiempo, archivos y después las áreas Ver/Comprobar, Modelado, UV, Weights, Auto-skin, Painting y Animación; páginas nuevas para la disposición como Blender, Ver/Comprobar, el panel de Animación y Auto-skin.

## Čeština


### Karta jako celek

- Karta Model, pravý panel: nyní se režimem ovládání řídí i vzhledem. Jako 3ds Max je to panel příkazů s kartami Vytvořit, Upravit, Hierarchie, Pohyb, Zobrazení a Nástroje; jako Blender je to editor vlastností se svislým sloupcem symbolů (Nástroj, Objekt, Modifikátory, Data, Kost, Vazby, Materiál). Každá oblast otevře svou kartu a ukáže tam všechny své skupiny; ostatní karty ukazují svou část.
- Skupiny všude ve stejné podobě: nadpis, který lze sbalit, s krátkým údajem vpravo (trojúhelníky, vykreslené LOD, počet připojených modelů, nálezy kontroly), tlačítka v mřížce dvou stejně širokých sloupců se symbolem a klávesou, hodnoty v tabulce, limity jako pruhy, rady v rámečku.
- Panel je širší (22 místo 17 výšek znaku) a jeho šířku lze táhnout za hranu u pohledů; dvojklik obnoví výchozí hodnotu.
- Oblasti Prohlížet/Kontrola, Modelování, UV, Váhy, Malování a Animace stojí jako jedna řada stejně širokých polí přes celou kartu, vedle nich krátká nápověda pro režim ovládání a oblast.
- Přepínač jako 3ds Max / jako Blender se z panelu přesunul: je ve stavovém řádku karty, v nabídce záhlaví pohledu, v nastavení a v hledání všech akcí.
- Karta Model, pravý panel: tabulky hodnot, tlačítka a přepínače se v úzkém panelu zalamují jen mezi slovy; mřížka tlačítek použije méně sloupců, když by se slovo nevešlo.
- Karta Model, pravý panel: v malém okně jsou karty panelu příkazů jako v 3ds Max symboly v jedné řadě, název je v bublinové nápovědě (jako karty vlastností v Blenderu) - jako text potřebovaly dvě až tři řady.
- Vpravo dole v pohledu: úroveň detailu a vykreslené trojúhelníky. Jako 3ds Max začínají Modelování, Weights, Painting a Animace se čtyřmi pohledy; jako Blender volí Z stínování a Shift+Alt+Z skryje překryvy. Soubor .glm se vždy otevře v Prohlížení/Kontrola.
- Karta Model jako 3ds Max: Vybrat kost je přepínač jako Změřit vzdálenost - ve Vyhledat všechny akce a v místní nabídce (se zaškrtnutím), nahoře v pohledu je nápověda, dokud je zapnutý; Q, W, E a R ho ukončí.

### Průzkumník scény

- Karta Model, průzkumník scény: jeden strom pro všechno - model, jeho povrchy s hierarchií, tagy s připojenými modely, kosti, úchyty rigu a sekvence z animation.cfg (samostatný seznam animací zmizel). Sloupec je tak široký jako ve schváleném návrhu.
- Jako 3ds Max: šipka, oko a symbol stojí přímo před názvem, každá úroveň je odsazena o jeden symbol, sloupec „Zmrazeno“ s vločkou, záhlaví sloupců, která řadí a dají se táhnout. Jako Blender: outliner s okem a šipkou „lze vybrat“ ve sloupcích vpravo a s vyskakovacím filtrem; přepínač se jmenuje Hierarchie / Kolekce.
- Zmrazení: zmrazené povrchy, tagy a kosti zůstávají viditelné, ale ve stromu ani v pohledu je nelze vybrat; záslepky jsou zmrazené od začátku. „Podle vrstev“ ukazuje Tělo, Záslepky, Tagy, Kosti, Připojené a další LOD, každou s okem a vločkou.
- Kliknutí v pohledu projde zmrazenou plochou a vybere plochu za ní, jako 3ds Max.
- Sekvence ve stromu nesou zámek P / S pro primární a sekundární sekvenci a výstrahu, když jejich snímky leží za koncem GLA; seznam skupin, kopírování názvu, do skriptu.
- Strom se nikdy neposouvá do strany: hluboké řetězce kostí se srazí blíž k sobě, dlouhé názvy končí „...“ a celé stojí v bublině spolu s informacemi o řádku. Filtr podle druhu, Shift+klik vybere rozsah, „Informace“ v nabídce pravého tlačítka.
- Bubliny lišty režimů vlevo už nevyskakují přes sousední panel: objeví se po krátké pauze vedle lišty ve výšce tlačítka.

### Rozvržení jako Blender

- Karta Model jako Blender: levý sloupec je panel nástrojů (T) s Vybrat, Posunout, Otočit, Změnit měřítko a nástroji oblasti - Měřit, Vybrat plochu, Vybrat kost, Fotka; Vysunout, Vsadit, Zkosení, Řez smyčkou, Nůž, Sloučit; Označit šev, Rozbalit, Uvolnit, Zabalit; Štětec, Rozostřit, Přechod, Odebrat váhu; Kreslit, Vyplnit, Klonovat, Rozmazat, Maska; Kopírovat pózu, Vložit zrcadlově, Zredukovat.
- Panel nástrojů jako Blender používá názvy nástrojů Blenderu a v němčině, španělštině, češtině, japonštině a čínštině vlastní překlady Blenderu.
- Skupiny nástrojů jako Blender: malý trojúhelník označuje skupinu, podržení nebo tažení ji otevře (Vysunutí oblasti / Podél Normál / Jednotlivé, Nůž / Půlit); kliknutí pravým tlačítkem na panel ukáže názvy nástrojů.
- Jako Blender je Outliner vpravo nahoře nad editorem vlastností; hranu mezi nimi lze táhnout.
- Záhlaví pohledu uvádí režim jako Blender (Objektový režim, Editační režim, Malování vah, Malování textur, Režim pózy); kliknutí přepne oblast.
- N otevře postranní panel: umístění a rotace nebo rozměry výběru, poslední měření, zorné pole a projekce.
- Jako Blender má postranní panel (N) číselná pole: polohu a rozměry plochy nebo polohu a rotaci ovladače rigu v režimu pózy lze táhnout nebo napsat; Ctrl+Z vrátí jedno zadání v jednom kroku.
- Nový nástroj Měřit: dvě kliknutí na model nebo tažení ukážou vzdálenost v herních jednotkách; jako 3ds Max jako Změřit vzdálenost ve Vyhledat všechny akce a v místní nabídce.
- Každý režim ovládání si drží vlastní rozvržení oken; při přepnutí zůstane výběr, sekvence i snímek. Rozvržení jako 3ds Max se nemění.
- Němčina: nabídka Overlays jako v Blenderu se jmenuje Überlagerungen, jako v německém rozhraní Blenderu.

### Časová osa

- Karta Model: časová osa teď vypadá jako ve schváleném návrhu a řídí se zvoleným ovládáním. Jako 3ds Max: Na začátek, Předchozí snímek, Přehrát, Další snímek, Na konec, zobrazení snímku od 0, Auto Key (N), Set Key (K), Nastavení času, jemná čárka na snímek ve stupnici i v každé stopě, klíče jako proužky v barvách lišty stop (červená = posun, zelená = rotace). Jako Blender: skoky na předchozí a další klíč (↓ / ↑), tlačítko nahrávání pro automatické klíčování, Začátek / Konec / Snímek od 1, klíče jako kosočtverce, K otevře nabídku klíčování.
- Karta Model: stopy (dopesheet, editor křivek) jsou součástí časové osy - jedno pravítko, snímek 0 je ve stupnici i ve stopách na stejném místě; otevře je úchyt nad časovou osou nebo tlačítko Stopy, v oblasti Animace jsou otevřené od začátku. Zobrazit/Zkontrolovat ukazuje rychlost, interpolaci, vynucenou smyčku a primární a sekundární sekvenci; ostatní oblasti ukazují krátkou časovou osu.
- Karta Model: Auto Key lze vypnout - póza pak zůstane bez klíče, dokud ji Set Key (K, jako Blender I) nezachová; změna snímku ji zahodí, jako ve 3ds Max a v Blenderu.
- Karta Model: Auto Key (jako Blender automatické klíčování) je od začátku vypnutý jako ve 3ds Max a v Blenderu; program si volbu zapamatuje.
- Karta Model: stavový řádek dole v časové ose (ovládání - kliknutím přepnete, klávesy oblasti, zpět a hledat); Nastavení času se snímkovou frekvencí, časem ve snímcích nebo sekundách, přehráváním a řádkem sekvence v animation.cfg; seznam skinů a počítadlo povrchů jsou nově ve skupině Úroveň detailu a skin.
- Karta Model: v malém okně už časová osa nezabírá skoro polovinu výšky - pod 50 výšek znaku výšky okna si drží svůj podíl, stopy se sníží a rolují a panely nad ní ukazují víc než své nadpisy.

### Prohlížení/Kontrola

- Karta Model, Prohlížení/Kontrola: skupiny návrhu - Model (s Importovat...), Výběr, Úroveň detailu a skin, Podlaha a kamera, Připojené modely, Kontrola pro hru, Scéna.
- Všech deset úrovní detailu lze zvolit jako r_lodbias ve hře; úroveň nad poslední úrovní modelu ukazuje tu poslední a skupina to uvádí. Skiny jsou volba ve dvou sloupcích.
- Podlaha a kamera: podlaha na -24 nebo pod nejnižším bodem pózy, zorné pole 10°, 80° nebo 90°, jednobarevné pozadí, práh váhy, Obnovit pohled a Fotka (1920+).
- Kontrola pro hru: pruhy vybrané plochy proti limitům cílové hry, plochy, sekvence uvnitř GLA, tagy, které hledá kód hry, shadery bez obrázku, LOD a plochy bez vlastního stínu.
- Scéna: uložit model, skin, připojené modely, sekvence a přepnuté plochy jako skript ModView (.mvs) a načítat skripty ModView.
- Pět chybějících kontrolních zobrazení ModView je v nabídkách záhlaví pohledu: alfa prolínání, filtrování textur, práh váhy kosti, zvýraznění plochy s odkazy kostí, plochy bez vlastního stínu.

### Modelování

- Karta Model, Modelování: panel vpravo odpovídá schválenému návrhu - skupiny Výběr, Nástroje a Limity JKA. Úrovně jsou řada polí s klávesou vpředu (režim 3ds Max 1-5, režim Blender 1-3); šest hlavních nástrojů jsou stejně široká tlačítka se symbolem a klávesou režimu ovládání, za nimi následují další nástroje úrovně a zbytek je sbalený pod „Upravit geometrii“.
- Nástroj, který na aktuální úrovni nepracuje, je zašedlý a jeho nápověda jmenuje úrovně, na kterých pracuje.
- Limity JKA: pruhy pro vybraný povrch (vrcholy, kosti a váhy proti limitu hry, žlutě od 90 %, červeně nad ním) a krátká informace v nadpisu; seznam všech povrchů je sbalený (u postavy s 83 díly zabíral osm stránek).
- Základní tělesa a rekvizity jsou na kartě „Vytvořit“ (režim 3ds Max) nebo „Objekt“ (režim Blender), jedno tlačítko na těleso; dokud je otevřená rekvizita, zůstává „Do mapy“ na jedno kliknutí.
- Ctrl+Enter použije nástroj a spustí ho znovu na výsledku (tlačítko „+“ v okénku nástroje).
- Režim Blender: E vysouvá jako v Blenderu (Alt+E funguje dál), Shift+A otevře nabídku „Přidat“ se základními tělesy.
- Karta Model: každý povrch má zásobník modifikátorů. V režimu 3ds Max je skupina „Zásobník modifikátorů“ úplně nahoře na kartě Upravit, v oblasti modelování i v oblasti Weights, a čte se zdola nahoru: povrch ze souboru, Edit Poly, modifikátory, Skin - kliknutí na Edit Poly nebo Skin přepne oblast. V režimu Blender je skupina „Modifikátory“ na kartě Modifikátory a uvádí modifikátory shora dolů.
- Dva modifikátory: Symetrie (režim Blender: Zrcadlení) rozřízne povrch v osové rovině a zrcadlí jednu stranu na druhou včetně vah kostí; TurboSmooth (režim Blender: Dělení povrchu) povrch jednou až třikrát rozdělí a zaoblí a může nechat otevřené okraje na místě.
- Každý modifikátor má oko (zapnuto/vypnuto) a „Zapsat do souboru“; vyhlazení je zpočátku jen náhled. Modifikátory lze posouvat nahoru a dolů (i tažením), použít (režim Blender: Použít, režim 3ds Max: Sbalit po a Sbalit vše) a odebrat; každá z těchto akcí je jeden krok zpět. Příkazy jsou také v nabídce pravého tlačítka na řádku, v nabídce pravého tlačítka v pohledu a v „Hledat všechny akce“.
- To, co pohled pro modifikátor ukazuje, je přesně síť, která vznikne jeho použitím. Dokud povrch nemá modifikátor, který jde do souboru, zapíše uložení stejný soubor jako dosud.
- Limity JKA a kontrola před uložením počítají to, co jde do souboru, včetně modifikátorů, které se do něj zapisují.
- Modelování, Symetrizovat: zrcadlená strana je teď u každého páru kostí kostry (lhumerus a rhumerus, ltibia a rtibia ...) navázána na kosti své strany; dosud se zaměňovala jen jména začínající na l_ a r_.

### UV

- Karta Model, oblast UV: panel vpravo odpovídá schválenému návrhu - Výběr, potom očíslované kroky 1 Určit švy, 2 Rozbalit, 3 Uklidit a nakonec Kontrola. Stejně široká tlačítka ve dvou sloupcích se symbolem a klávesou režimu ovládání, Rozbalit jako zvýrazněné hlavní tlačítko, v krocích rámeček s tipem.
- Výběr ukazuje v záhlaví texturu a počet ostrovů; nově: Vybrat ostrov (rozšíří výběr na celé ostrovy) a Vybrat vše.
- Rychlé rozbalení a Promítnout jsou po jednom tlačítku; šablona (Postava, Rekvizita, Tvrdý povrch) a projekce (Rovinná, Válec, Koule, Kvádr) se volí hned pod nimi. Méně používané příkazy jsou v každém kroku sbalené pod Další.
- Kontrola: záhlaví říká „vše v pořádku“ nebo počet nálezů; natažení, překryvy, nové využití textury v procentech a hustota texelů jsou jako tabulka; nový přepínač Tlusté švy pro editor UV.
- Oblast UV: Silné švy nyní zesílí i švy ve 3D pohledu.
- Jako Blender: L vybere ostrov pod myší, A vybere vše, Ctrl+A sjednotí hustotu, Ctrl+P zabalí. Jako 3ds Max: Ctrl+A vybere vše.

### Weights a Auto-skin

- Karta Model, oblast Weights: panel vpravo odpovídá schválenému návrhu. Jako 3ds Max: zásobník modifikátorů, nástroj vah, zrcadlení, auto-skin, kost, meze JKA a zavřené pokročilé parametry. Jako Blender: štětec, možnosti, vyčištění, zrcadlení, auto-skin, kost, meze JKA.
- Nástroj vah jako 3ds Max: Malovat váhy zapíná a vypíná malování; pevné hodnoty 0 až 1 nastaví váhu na vybraných vrcholech okamžitě; nově Škálovat, Prolnout, Rozšířit, Zmenšit, Prstenec a Smyčka.
- Weights: hlavní tlačítko Malovat váhy si drží místo i výšku, i když je malování vypnuté (zvýrazněný je pak jen jeho rámeček).
- Nové štětce jako Blender: Přechod (lineární nebo radiální) a Rozmazat; Automatická normalizace drží součet vah vrcholu na 1 (zapnuto od začátku).
- Vyčištění jako Blender: Normalizovat vše, Vyčistit, Vyhladit a Omezit: 4, na vybraných vrcholech nebo na všech.
- Zrcadlení jako 3ds Max (režim zrcadlení vlevo modrá, vpravo zelená, bez protějšku žlutá; modrá na zelenou, zelená na modrou, prohodit strany, jen výběr, tolerance) a jako Blender (zrcadlit skupinu nebo všechny skupiny, prohodit názvy, zrcadlení podle topologie); vrcholy na stejném místě na různých površích se počítají jako jeden.
- Kosti lze zamknout (jako Blender: K otevře nabídku zámků); kosti jsou v této oblasti vidět od začátku; meze hry se zobrazují jako pruhy.
- Karta Model, Weights: Auto-skin. "Automaticky vážit" dá síti váhy kostí z kostry, v jednom kroku zpět. Tři metody: přenést z originálu ze hry se stejnou kostrou (například kyle), geodeticky (vzdálenost ke každé kosti se měří vnitřkem sítě přes voxely, paže se tak nepřilepí k tělu), obálka (přímá vzdálenost, rychle). Otočné kosti (lhumerusX, lradiusX, lfemurX) nesou polovinu své končetiny směrem k dalšímu kloubu, jako u postav od Ravenu.
- Auto-skin: výsledek vždy odpovídá hře - nejvýše 4 váhy na vrchol, nejvýše 32 kostí na povrch, uložené v 10 bitech tak, jak je hra čte; vrcholy na stejném místě dostanou stejné váhy, švy mezi povrchy tedy zůstanou zavřené.
- Auto-skin: nastavení rozlišení, tuhosti, vyhlazení, švů, otočných kostí, obličeje a přívěsků, zrcadlení potom a "Jen vybrané povrchy" (nový díl zbroje na hotové postavě). Okno ukazuje výsledek: meze hry a jak daleko leží vrcholy od předchozích vah, v jednotkách.
- Auto-skin: "Automaticky vážit" a "Auto-skin: nastavení" jsou ve vyhledávání všech akcí (X, v režimu Blender F3) a v pravém kliknutí pohledu pod Weights.
- Karta Model, Weights: uložení zapíše váhy, které ukazuje pohled, i když jsou seznamy malování otevřené (po malování nebo příkazu vah, před Použít). Soubor dostane nejvýše čtyři váhy na vrchol, normalizované tak, jak je čte hra; seznamy malování zůstanou otevřené. Kontrola před uložením a limity JKA měří totéž. Dosud uložení zapsalo váhy z doby před malováním a hlásilo „uloženo“.
- Weights: dokud je otevřený tah, přechod, tažení gizma nebo okénko nástroje, úroveň detailu zůstává, i když kolečko přibližuje nebo by ji „Auto“ změnilo. Zrcadlení při malování mohlo při změně úrovně během tahu zapsat do špatných vrcholů nebo spadnout.
- Weights a modelování: výběr vrcholů, hran nebo mnohoúhelníků patří ke své úrovni detailu; po změně úrovně je prázdný a vrátí se s touto úrovní, dokud se její síť nezměnila. Dosud čísla zůstala a příkazy měnily jiné vrcholy.
- Modelování: nástroj na ploše s namalovanými, dosud nepoužitými váhami je zachová (dosud se vracely zpět).
- Auto-Skin: „Jen vybrané povrchy“ s „Potom zrcadlit“ zrcadlí jen do vybraných ploch a otevřené seznamy malování ostatních ploch nechá otevřené.
- Ctrl+Z během tahu vah, přechodu, tažení gizma nebo tahu v Painting ho ukončí a vrátí; Ctrl+Y ho obnoví. Dosud držené tlačítko psalo dál bez kroku zpět.
- Drobné opravy: příkaz, který nic nezmění, zachová Znovu; tah začínající vedle postavy maluje zvoleným režimem; uzamčené kosti jsou u nově načteného modelu odemčené; zadaný poloměr štětce 0 v Painting se počítá jako 1.

### Painting

- Karta Model, Malování: pravý panel odpovídá schválenému návrhu. Skupiny Cíl, Nástroje, Štětec, Vrstvy a Pro hru stojí v pořadí práce; nástroje tvoří mřížku tlačítek se symbolem (vybraný je zvýrazněn), barva, poloměr, síla a rozestup jsou řádky s posuvníky a tvrdost, zrcadlení, přítlak pera, odvrácené plochy a přesah jsou pod Rozšířené.
- Vrstvy jsou řádky s okem pro zobrazení či skrytí a se zámkem u originálu; režim prolnutí a krytí vybrané vrstvy stojí pod seznamem, rámeček s tipem pod skupinou.
- Klávesy [ a ] zmenšují a zvětšují štětec v obou režimech ovládání (také v hledání všech akcí a ve stavovém řádku).
- Jako 3ds Max stojí skupiny na kartě Upravit (Pro hru také pod Nástroje), jako Blender na kartě Nástroj (Cíl také pod Materiál).
- Dlouhé cesty se zkracují zepředu („.../kyle/body.tga“) a výstupní složka ukazuje konec své cesty; celá cesta je v popisku.
- Karta Model jako 3ds Max, Malování: vyhledávání a nabídka Navigace tam pro přiblížení uvádějí Ctrl+= a Ctrl+-, protože [ a ] nastavují velikost štětce.

### Animace

- Karta Model, oblast Animace: panel vpravo odpovídá schválenému návrhu - Animace, Vytvořit rig, Předvolby, Upravit existující animaci, Přenos (retargeting), IK/FK, Připojené a náhled, Sestavit pro hru; za nimi zavřené vše ostatní (automatická fyzika, póza, duchové, dráhy pohybu, křivky, knihovna póz, GLA, animation.cfg, cílové GLA a sloty).
- Vytvořit rig: ukazuje kostru, měřítko a rozpoznanou šablonu; šablonu lze zvolit podle skupiny (člověk, rancor, wampa, tauntaun, howler, AT-ST, droidi, vozidla …) nebo Vlastní kostra. Nově: Animační rig (jen FK) a Položit na rig (animace se stane klíči na ovladačích, póza zůstane).
- Karta Model, Animace: jako rig CAT položí Vytvořit ovládací rig animaci sekvence na rig z FK na IK - ruce a nohy sledují své cíle IK a póly, pohyb vypadá stejně (zpráva uvede největší odchylku). Pak se na kterémkoli snímku pracuje s ovladači (cíl ruky nebo nohy, pól, odval chodidla); změní se jen tento snímek.
- Předvolby: knihovna je vpředu - kategorie, seznam se zdrojem a licencí, cílový slot jako volba, Náhled na postavě a Položit na postavu (retarget); generátor se otevře svým tlačítkem.
- Předvolby ukazují osm kategorií návrhu (Stání, Chůze, Běh, Pozpátku, Přikrčení, Skok, Boj, Gesta, za nimi Ostatní); kategorie zatím bez předvoleb ukáže krátkou poznámku.
- Vlastní generátor je v seznamu jako Chůze (generovaná) a Běh (generovaný): rychlost a švih paží pod seznamem, Náhled na postavě a Položit na postavu jako u předvoleb, Generátor... pro zbytek; pravé tlačítko na položce nabídne její příkazy.
- Upravit existující animaci: Zredukovat snímky na klíče a Zapéct do GLA v jedné skupině se zdrojem, snímky a klíči.
- IK/FK jako 3ds Max: u každé končetiny stav slovy a jedno tlačítko IK/FK; jako Blender: posuvník a obě tlačítka přichycení v jednom řádku.
- IK/FK pro každou končetinu jako CAT: tlačítko přepne končetinu od aktuálního snímku do další změny a předtím srovná druhou stranu - nic neskočí.
- Sestavit pro hru ukazuje posun kořene sekvence (jak daleko se pánev posune) a připomíná SET_ORIGIN.
- Animace, skupina pro GLA mapy: pole Cíl, Mapa a Jméno teď začínají na stejné hraně.
- Světlý motiv: zvolená předvolba je opět čitelná.
- Karta Model, Animace: Animační rig (jen FK) po Vytvořit ovládací rig nejdřív zapeče IK každé končetiny do klíčů FK - pohyb zůstane přesně stejný, každý otočný ovladač otáčí svou kostí a Ctrl+Z vrátí cíle IK jedním krokem.
- Karta Model: opraveno, že tažení gizma na ovladači rigu zůstalo otevřené, když Q gizmo odebralo; Ctrl+Z při tažení rychlosti generované chůze; Průzkumník scény po odpojení připojeného modelu kreslil řádky za koncem seznamu; zvolená karta pravého panelu zůstane i po velmi malém okně.

### Kosti a styly rigu

- Karta Model: kosti se kreslí do obrazu pohledu. Jako 3ds Max jako kvádry kostí (ploutve volitelně), jako Blender se Zobrazit jako: osmistěn, tyčinka, B-Bone, obálka nebo drát.
- Kosti a ovladače leží ve výchozím stavu před postavou; po vypnutí V popředí je postava zakryje.
- Průhlednost: Alt+X jako 3ds Max (postava ve všech pohledech), Alt+Z jako Blender (rentgen aktivního pohledu). Postava leží napůl průhledně nad tím, co je za ní.
- Styl rigu v oblasti Animace: Standardní, ovládací rig s kruhy, kvádry CAT nebo osmistěny Blenderu. Ovladače jsou obarveny podle strany; styl mění jen vzhled, nikdy rig.
- Kliknutí a tažení zasáhne viditelný tvar kosti nebo ovladače: kruh, hranu nebo vnitřek kvádru, kouli pólu, osmistěn.
- Pod Direct3D 12 a Direct3D 11 vypadá vše stejně.

### Pomocník, hledání a příručka

- Karta Model, Hledat všechny akce: akce, které se ve více oblastech jmenují stejně, uvádějí vpravo svou oblast (Rozmazat - Weights, Rozmazat - Painting); jako v Blenderu je Přehrát/Zastavit jen jednou.
- Pomocník (Co chcete udělat?) zná každou skupinu, tlačítko, přepínač a posuvník v pravém panelu karty Model a dovede tam: karta Model, oblast, skupina, prvek.
- Příručka: stránky karty Model jsou v pořadí práce - přehled, ovládání, rozvržení jako Blender, časová osa, soubory, pak oblasti Prohlížení/Kontrola, Modelování, UV, Weights, Auto-skin, Painting a Animace; nové stránky pro rozvržení jako Blender, Prohlížení/Kontrola, panel Animace a Auto-skin.
