# Abdeckung: jeder Eintrag mit Wirkungspruefung

Erzeugt von `tools/karte/nrc_inventar.py abdeckung` aus `tools/karte/nrc_inventar.md` und dem Protokoll `abdeckung_lauf.txt`. Nicht von Hand aendern.

| Art | Eintraege | mit Wirkungspruefung | Ausnahme | fehlt | FEHLER |
|---|---|---|---|---|---|
| Menues | 237 | 236 | 1 | 0 | 0 |
| Werkzeugleisten | 54 | 54 | 0 | 0 | 0 |
| Registrierte Befehle | 398 | 397 | 1 | 0 | 0 |
| Dialoge und Fenster | 24 | 23 | 1 | 0 | 0 |
| Einstellungsseiten | 17 | 17 | 0 | 0 | 0 |
| Einstellungsfelder | 79 | 78 | 1 | 0 | 0 |
| Plugins | 69 | 69 | 0 | 0 | 0 |
| Mausbelegung | 62 | 62 | 0 | 0 | 0 |
| Eigene Eintraege | 29 | 29 | 0 | 0 | 0 |
| **Summe** | 969 | 965 | 4 | 0 | 0 |

## Begruendete Ausnahmen

- `Menue|Exit|radiant/mainframe.cpp:889`: wie der Befehl Exit (Stufe 4 prueft den Menuepunkt mit der Rueckfrage)
- `Befehl|Exit|Commands`: beendet das Programm - Stufe 4 prueft ConfirmModified (Save/Discard/Cancel) und das Beenden: siehe Pruefungen "Exit"
- `Dialog|Install Map Developer's Files`: ja.game/jk2.game haben keinen install-Ordner: DoInstallDevFilesDlg zeigt bei JA/JK2 nichts (Stand [~] seit Stufe 2)
- `Feld|radiant/textures.cpp:818`: Hardware Texture Compression: D3D11 meldet weder OpenGL- noch S3TC-Kompression, die Liste hat wie NRC ohne diese Erweiterungen nur "None" - es gibt nichts zu waehlen

## Menues

| Eintrag | Stand | Pruefung (Wirkung) |
|---|---|---|
| `Menue \| BrushPrism \| radiant/brushmanip.cpp:1498` | OK | Menue Brush > Prism... = BrushPrism: Dialog (Pinsel) |
| `Menue \| BrushCone \| radiant/brushmanip.cpp:1499` | OK | Menue Brush > Cone... = BrushCone: Dialog (Pinsel) |
| `Menue \| BrushSphere \| radiant/brushmanip.cpp:1500` | OK | Menue Brush > Sphere... = BrushSphere: Dialog (Pinsel) |
| `Menue \| BrushRock \| radiant/brushmanip.cpp:1501` | OK | Menue Brush > Rock... = BrushRock: Dialog (Pinsel) |
| `Menue \| BrushIcosahedron \| radiant/brushmanip.cpp:1502` | OK | Menue Brush > Icosahedron... = BrushIcosahedron: Dialog (Pinsel) |
| `Menue \| CSGSubtract \| radiant/brushmanip.cpp:1509` | OK | Menue Brush > CSG > CSG Subtract = CSGSubtract: Karte (Pinsel B) |
| `Menue \| CSGMerge \| radiant/brushmanip.cpp:1510` | OK | Menue Brush > CSG > CSG Merge = CSGMerge: Karte (zwei Pinsel nebeneinander) |
| `Menue \| CSGWrapMerge \| radiant/brushmanip.cpp:1511` | OK | Menue Brush > CSG > CSG Wrap Merge = CSGWrapMerge: Karte (zwei Pinsel) |
| `Menue \| CSGIntersect \| radiant/brushmanip.cpp:1512` | OK | Menue Brush > CSG > CSG Intersect = CSGIntersect: Karte (zwei Pinsel) |
| `Menue \| CSGroom \| radiant/brushmanip.cpp:1513` | OK | Menue Brush > CSG > Make Room = CSGroom: Karte (Pinsel) |
| `Menue \| CSGTool \| radiant/brushmanip.cpp:1514` | OK | Menue Brush > CSG > CSG Tool = CSGTool: Dialog (Pinsel) |
| `Menue \| ClipperClip \| radiant/brushmanip.cpp:1522` | OK | Menue Brush > Clipper > Clip selection = ClipperClip: Karte (Clipper mit zwei Punkten) |
| `Menue \| ClipperSplit \| radiant/brushmanip.cpp:1523` | OK | Menue Brush > Clipper > Split selection = ClipperSplit: Karte (Clipper mit zwei Punkten) |
| `Menue \| ClipperFlip \| radiant/brushmanip.cpp:1524` | OK | Menue Brush > Clipper > Flip Clip orientation = ClipperFlip: Modus (Clipper mit zwei Punkten) |
| `Menue \| MakeDetail \| radiant/brushmanip.cpp:1527` | OK | Menue Brush > Make detail = MakeDetail: Karte (Pinsel) |
| `Menue \| MakeStructural \| radiant/brushmanip.cpp:1528` | OK | Menue Brush > Make structural = MakeStructural: Karte (Detail-Pinsel) |
| `Menue \| TogTexLock \| radiant/brushmanip.cpp:1531` | OK | Menue Brush > Texture Lock = TogTexLock: Haken+Modus (leer) |
| `Menue \| TogTexVertexLock \| radiant/brushmanip.cpp:1532` | OK | Menue Brush > Texture Vertex Lock = TogTexVertexLock: Haken+Modus (leer) |
| `Menue \| TextureReset/Cap \| radiant/brushmanip.cpp:1533` | OK | Menue Brush > Reset Texture = TextureReset/Cap: Karte (Pinsel, Textur versetzt); (+1) |
| `Menue \| AutoCaulkSelected \| radiant/brushmanip.cpp:1534` | OK | Menue Brush > AutoCaulk Selected = AutoCaulkSelected: Datei (Pinsel) |
| `Menue \| OpenGLFont \| radiant/colors.cpp:231` | OK | Menue Misc > Theming > OpenGL Font... = OpenGLFont: Dialog (leer) |
| `Menue \| EntitiesConnect \| radiant/entity.cpp:709` | OK | Menue Entity > Connect Entities = EntitiesConnect: Karte (zwei Entitaeten) |
| `Menue \| EntityMovePrimitivesToLast \| radiant/entity.cpp:713` | OK | Menue Entity > Move Primitives to Entity = EntityMovePrimitivesToLast: Karte (Pinsel, dann func_door) |
| `Menue \| EntityColorSet \| radiant/entity.cpp:714` | OK | Menue Entity > Select Color... = EntityColorSet: Dialog (Licht) |
| `Menue \| EntityColorNormalize \| radiant/entity.cpp:715` | OK | Menue Entity > Normalize Color = EntityColorNormalize: Karte (Licht) |
| `Menue \| EntityReloadDefinitions \| radiant/entity.cpp:717` | OK | Menue Entity > Reload Entity Definitions = EntityReloadDefinitions: Neu geladen (leer) |
| `Menue \| FilterWorldBrushes \| radiant/filters.cpp:161` | OK | Menue View > Filter > World = FilterWorldBrushes: Filter+Haken+Einstellung (leer) |
| `Menue \| FilterEntities \| radiant/filters.cpp:162` | OK | Menue View > Filter > Entities = FilterEntities: Filter+Haken+Einstellung (leer) |
| `Menue \| FilterAreaportals \| radiant/filters.cpp:168` | OK | Menue View > Filter > Areaportals = FilterAreaportals: Filter+Haken+Einstellung (leer) |
| `Menue \| FilterTranslucent \| radiant/filters.cpp:170` | OK | Menue View > Filter > Translucent = FilterTranslucent: Filter+Haken+Einstellung (leer) |
| `Menue \| FilterLiquids \| radiant/filters.cpp:172` | OK | Menue View > Filter > Liquids = FilterLiquids: Filter+Haken+Einstellung (leer) |
| `Menue \| FilterCaulk \| radiant/filters.cpp:174` | OK | Menue View > Filter > Caulk = FilterCaulk: Filter+Haken+Einstellung (leer) |
| `Menue \| FilterClips \| radiant/filters.cpp:175` | OK | Menue View > Filter > Clips = FilterClips: Filter+Haken+Einstellung (leer) |
| `Menue \| FilterPaths \| radiant/filters.cpp:176` | OK | Menue View > Filter > Paths = FilterPaths: Filter+Haken+Einstellung (leer) |
| `Menue \| FilterClusterportals \| radiant/filters.cpp:178` | OK | Menue View > Filter > Clusterportals = FilterClusterportals: Filter+Haken+Einstellung (leer) |
| `Menue \| FilterLights \| radiant/filters.cpp:180` | OK | Menue View > Filter > Lights = FilterLights: Filter+Haken+Einstellung (leer) |
| `Menue \| FilterStructural \| radiant/filters.cpp:181` | OK | Menue View > Filter > Structural = FilterStructural: Filter+Haken+Einstellung (leer) |
| `Menue \| FilterLightgrid \| radiant/filters.cpp:183` | OK | Menue View > Filter > Lightgrid = FilterLightgrid: Filter+Haken+Einstellung (leer) |
| `Menue \| FilterPatches \| radiant/filters.cpp:185` | OK | Menue View > Filter > Patches = FilterPatches: Filter+Haken+Einstellung (leer) |
| `Menue \| FilterDetails \| radiant/filters.cpp:186` | OK | Menue View > Filter > Details = FilterDetails: Filter+Haken+Einstellung (leer) |
| `Menue \| FilterHintsSkips \| radiant/filters.cpp:187` | OK | Menue View > Filter > Hints = FilterHintsSkips: Filter+Haken+Einstellung (leer) |
| `Menue \| FilterModels \| radiant/filters.cpp:188` | OK | Menue View > Filter > Models = FilterModels: Filter+Haken+Einstellung (leer) |
| `Menue \| FilterTriggers \| radiant/filters.cpp:189` | OK | Menue View > Filter > Triggers = FilterTriggers: Filter+Haken+Einstellung (leer) |
| `Menue \| FilterBotClips \| radiant/filters.cpp:191` | OK | Menue View > Filter > Botclips = FilterBotClips: Filter+Haken+Einstellung (leer) |
| `Menue \| FilterDecals \| radiant/filters.cpp:192` | OK | Menue View > Filter > Decals = FilterDecals: Filter+Haken+Einstellung (leer) |
| `Menue \| FilterFuncGroups \| radiant/filters.cpp:194` | OK | Menue View > Filter > FuncGroups = FilterFuncGroups: Filter+Haken+Einstellung (leer) |
| `Menue \| FilterPointEntities \| radiant/filters.cpp:195` | OK | Menue View > Filter > Point Entities = FilterPointEntities: Filter+Haken+Einstellung (leer) |
| `Menue \| FilterSky \| radiant/filters.cpp:196` | OK | Menue View > Filter > Sky = FilterSky: Filter+Haken+Einstellung (leer) |
| `Menue \| InvertFilters \| radiant/filters.cpp:199` | OK | Menue View > Filter > Invert filters = InvertFilters: Filter (leer) |
| `Menue \| ResetFilters \| radiant/filters.cpp:200` | OK | Menue View > Filter > Reset filters = ResetFilters: Filter (Filter an) |
| `Menue \| SetGrid0.125 \| radiant/grid.cpp:208` | OK | Menue Grid > Grid0.125 = SetGrid0.125: Raster+Haken (leer) |
| `Menue \| SetGrid0.25 \| radiant/grid.cpp:209` | OK | Menue Grid > Grid0.25 = SetGrid0.25: Raster+Haken (leer) |
| `Menue \| SetGrid0.5 \| radiant/grid.cpp:210` | OK | Menue Grid > Grid0.5 = SetGrid0.5: Raster+Haken (leer) |
| `Menue \| SetGrid1 \| radiant/grid.cpp:211` | OK | Menue Grid > Grid1 = SetGrid1: Raster+Haken (leer) |
| `Menue \| SetGrid2 \| radiant/grid.cpp:212` | OK | Menue Grid > Grid2 = SetGrid2: Raster+Haken (leer) |
| `Menue \| SetGrid4 \| radiant/grid.cpp:213` | OK | Menue Grid > Grid4 = SetGrid4: Raster+Haken (leer) |
| `Menue \| SetGrid8 \| radiant/grid.cpp:214` | OK | Menue Grid > Grid8 = SetGrid8: Raster+Haken (Raster 16) |
| `Menue \| SetGrid16 \| radiant/grid.cpp:215` | OK | Menue Grid > Grid16 = SetGrid16: Raster+Haken (leer) |
| `Menue \| SetGrid32 \| radiant/grid.cpp:216` | OK | Menue Grid > Grid32 = SetGrid32: Raster+Haken (leer) |
| `Menue \| SetGrid64 \| radiant/grid.cpp:217` | OK | Menue Grid > Grid64 = SetGrid64: Raster+Haken (leer) |
| `Menue \| SetGrid128 \| radiant/grid.cpp:218` | OK | Menue Grid > Grid128 = SetGrid128: Raster+Haken (leer) |
| `Menue \| SetGrid256 \| radiant/grid.cpp:219` | OK | Menue Grid > Grid256 = SetGrid256: Raster+Haken (leer) |
| `Menue \| SetGrid512 \| radiant/grid.cpp:220` | OK | Menue Grid > Grid512 = SetGrid512: Raster+Haken (leer) |
| `Menue \| SetGrid1024 \| radiant/grid.cpp:221` | OK | Menue Grid > Grid1024 = SetGrid1024: Raster+Haken (leer) |
| `Menue \| NewMap \| radiant/mainframe.cpp:874` | OK | Menue File > New Map = NewMap: Karte (Pinsel) |
| `Menue \| OpenMap \| radiant/mainframe.cpp:877` | OK | Menue File > Open... = OpenMap: Karte (leer) |
| `Menue \| ImportMap \| radiant/mainframe.cpp:878` | OK | Menue File > Import... = ImportMap: Karte (leer) |
| `Menue \| SaveMap \| radiant/mainframe.cpp:880` | OK | Menue File > Save = SaveMap: Datei (Karte geaendert) |
| `Menue \| SaveMapAs \| radiant/mainframe.cpp:881` | OK | Menue File > Save as... = SaveMapAs: Datei (Pinsel) |
| `Menue \| SaveSelected \| radiant/mainframe.cpp:882` | OK | Menue File > Save selected... = SaveSelected: Datei (Pinsel) |
| `Menue \| SaveRegion \| radiant/mainframe.cpp:883` | OK | Menue File > Save region... = SaveRegion: Datei (Pinsel) |
| `Menue \| TogglePointfile \| radiant/mainframe.cpp:885` | OK | Menue File > Pointfile = TogglePointfile: Leck/Portale (leer) |
| `Menue \| Exit \| radiant/mainframe.cpp:889` | AUSNAHME | wie der Befehl Exit (Stufe 4 prueft den Menuepunkt mit der Rueckfrage) |
| `Menue \| Undo \| radiant/mainframe.cpp:898` | OK | Menue Edit > Undo = Undo: Karte (nach einer Aenderung) |
| `Menue \| Redo \| radiant/mainframe.cpp:899` | OK | Menue Edit > Redo = Redo: Karte (nach Rueckgaengig) |
| `Menue \| Copy \| radiant/mainframe.cpp:901` | OK | Menue Edit > Copy = Copy: Ablage (Pinsel) |
| `Menue \| Paste \| radiant/mainframe.cpp:902` | OK | Menue Edit > Paste = Paste: Karte+Auswahl (Pinsel kopiert) |
| `Menue \| PasteToCamera \| radiant/mainframe.cpp:903` | OK | Menue Edit > Paste To Camera = PasteToCamera: Karte+Auswahl (Pinsel kopiert) |
| `Menue \| MoveToCamera \| radiant/mainframe.cpp:904` | OK | Menue Edit > Move To Camera = MoveToCamera: Karte (Pinsel) |
| `Menue \| CloneSelection \| radiant/mainframe.cpp:906` | OK | Menue Edit > Duplicate = CloneSelection: Karte+Auswahl (Pinsel) |
| `Menue \| CloneSelectionAndMakeUnique \| radiant/mainframe.cpp:907` | OK | Menue Edit > Duplicate, make unique = CloneSelectionAndMakeUnique: Karte+Auswahl (Licht) |
| `Menue \| DeleteSelection \| radiant/mainframe.cpp:908` | OK | Menue Edit > Delete = DeleteSelection: Karte+Auswahl (Pinsel) |
| `Menue \| UnSelectSelection \| radiant/mainframe.cpp:911` | OK | Menue Edit > Clear Selection = UnSelectSelection: Auswahl (Pinsel) |
| `Menue \| InvertSelection \| radiant/mainframe.cpp:912` | OK | Menue Edit > Invert Selection = InvertSelection: Auswahl (Pinsel) |
| `Menue \| SelectInside \| radiant/mainframe.cpp:913` | OK | Menue Edit > Select inside = SelectInside: Karte+Auswahl (Pinsel) |
| `Menue \| SelectTouching \| radiant/mainframe.cpp:914` | OK | Menue Edit > Select touching = SelectTouching: Auswahl (Pinsel) |
| `Menue \| SelectAllOfType \| radiant/mainframe.cpp:918` | OK | Menue Edit > Select All Of Type = SelectAllOfType: Auswahl (Licht) |
| `Menue \| SelectTextured \| radiant/mainframe.cpp:919` | OK | Menue Edit > Select Textured = SelectTextured: Auswahl (Textur gewaehlt) |
| `Menue \| ExpandSelectionToPrimitives \| radiant/mainframe.cpp:920` | OK | Menue Edit > Expand Selection To Primitives = ExpandSelectionToPrimitives: Auswahl (Pinsel) |
| `Menue \| ExpandSelectionToEntities \| radiant/mainframe.cpp:921` | OK | Menue Edit > Expand Selection To Entities = ExpandSelectionToEntities: Auswahl (Pinsel der func_group) |
| `Menue \| ExpandSelectionToLayers \| radiant/mainframe.cpp:922` | OK | Menue Edit > Expand Selection To Layers = ExpandSelectionToLayers: Auswahl (Pinsel) |
| `Menue \| SelectConnectedEntities \| radiant/mainframe.cpp:923` | OK | Menue Edit > Select Connected Entities = SelectConnectedEntities: Auswahl (Licht) |
| `Menue \| Shortcuts \| radiant/mainframe.cpp:926` | OK | Menue Edit > Shortcuts... = Shortcuts: Dialog (leer) |
| `Menue \| Preferences \| radiant/mainframe.cpp:927` | OK | Menue Edit > Preferences... = Preferences: Dialog (leer) |
| `Menue \| ToggleCamera \| radiant/mainframe.cpp:937` | OK | Menue View > Camera View = ToggleCamera: Haken+Fenster (leer) |
| `Menue \| ToggleView \| radiant/mainframe.cpp:938` | OK | Menue View > XY (Top) View = ToggleView: Haken+Fenster (leer) |
| `Menue \| ToggleFrontView \| radiant/mainframe.cpp:939` | OK | Menue View > XZ (Front) View = ToggleFrontView: Haken+Fenster (leer) |
| `Menue \| ToggleSideView \| radiant/mainframe.cpp:940` | OK | Menue View > YZ (Side) View = ToggleSideView: Haken+Fenster (leer) |
| `Menue \| ToggleConsole \| radiant/mainframe.cpp:943` | OK | Menue View > Console = ToggleConsole: Fenster (leer) |
| `Menue \| ToggleTextures \| radiant/mainframe.cpp:946` | OK | Menue View > Texture Browser = ToggleTextures: Fenster (leer) |
| `Menue \| ToggleModelBrowser \| radiant/mainframe.cpp:948` | OK | Menue View > Model Browser = ToggleModelBrowser: Fenster (leer) |
| `Menue \| ToggleEntityInspector \| radiant/mainframe.cpp:949` | OK | Menue View > Entity Inspector = ToggleEntityInspector: Fenster (leer) |
| `Menue \| ToggleLayersBrowser \| radiant/mainframe.cpp:950` | OK | Menue View > Layers Browser = ToggleLayersBrowser: Fenster (leer) |
| `Menue \| SurfaceInspector \| radiant/mainframe.cpp:951` | OK | Menue View > Surface Inspector = SurfaceInspector: Dialog (leer) |
| `Menue \| ToggleEntityList \| radiant/mainframe.cpp:952` | OK | Menue View > Entity List = ToggleEntityList: Dialog (leer) |
| `Menue \| CameraFocusOnSelected \| radiant/mainframe.cpp:960` | OK | Menue View > Camera > Focus on Selected = CameraFocusOnSelected: Kamera (Pinsel) |
| `Menue \| CenterView \| radiant/mainframe.cpp:961` | OK | Menue View > Camera > Center = CenterView: Kamera (Pinsel) |
| `Menue \| UpFloor \| radiant/mainframe.cpp:962` | OK | Menue View > Camera > Up Floor = UpFloor: Kamera (Kamera unten) |
| `Menue \| DownFloor \| radiant/mainframe.cpp:963` | OK | Menue View > Camera > Down Floor = DownFloor: Kamera (Pinsel) |
| `Menue \| CubicClipZoomIn \| radiant/mainframe.cpp:965` | OK | Menue View > Camera > Far Clip Plane In = CubicClipZoomIn: Einstellung (leer) |
| `Menue \| CubicClipZoomOut \| radiant/mainframe.cpp:966` | OK | Menue View > Camera > Far Clip Plane Out = CubicClipZoomOut: Einstellung (leer) |
| `Menue \| NextLeakSpot \| radiant/mainframe.cpp:968` | OK | Menue View > Camera > Next leak spot = NextLeakSpot: 2D-Ansicht+Kamera+Leck/Portale (Leck-Linie geladen) |
| `Menue \| PrevLeakSpot \| radiant/mainframe.cpp:969` | OK | Menue View > Camera > Previous leak spot = PrevLeakSpot: 2D-Ansicht+Kamera+Leck/Portale (Leck-Linie, ein Punkt weiter) |
| `Menue \| NextView \| radiant/mainframe.cpp:982` | OK | Menue View > Orthographic > Next (XY, XZ, YZ) = NextView: 2D-Ansicht (leer); (+1) |
| `Menue \| ViewTop \| radiant/mainframe.cpp:983` | OK | Menue View > Orthographic > XY (Top) = ViewTop: 2D-Ansicht (leer) |
| `Menue \| ViewFront \| radiant/mainframe.cpp:984` | OK | Menue View > Orthographic > XZ (Front) = ViewFront: 2D-Ansicht (leer) |
| `Menue \| ViewSide \| radiant/mainframe.cpp:985` | OK | Menue View > Orthographic > YZ (Side) = ViewSide: 2D-Ansicht (leer) |
| `Menue \| NextView \| radiant/mainframe.cpp:989` | OK | Menue View > Orthographic > Next (XY, XZ, YZ) = NextView: 2D-Ansicht (leer); (+1) |
| `Menue \| XYFocusOnSelected \| radiant/mainframe.cpp:992` | OK | Menue View > Orthographic > Focus on Selected = XYFocusOnSelected: 2D-Ansicht (Pinsel) |
| `Menue \| CenterXYView \| radiant/mainframe.cpp:993` | OK | Menue View > Orthographic > Center on Selected = CenterXYView: 2D-Ansicht (Pinsel) |
| `Menue \| Zoom100 \| radiant/mainframe.cpp:995` | OK | Menue View > Orthographic > XY 100% = Zoom100: 2D-Ansicht (2D gezoomt) |
| `Menue \| ZoomIn \| radiant/mainframe.cpp:996` | OK | Menue View > Orthographic > XY Zoom In = ZoomIn: 2D-Ansicht (leer) |
| `Menue \| ZoomOut \| radiant/mainframe.cpp:997` | OK | Menue View > Orthographic > XY Zoom Out = ZoomOut: 2D-Ansicht (leer) |
| `Menue \| ShowAngles \| radiant/mainframe.cpp:1007` | OK | Menue View > Show > Show Entity Angles = ShowAngles: Haken+Anzeige (leer) |
| `Menue \| ShowNames \| radiant/mainframe.cpp:1008` | OK | Menue View > Show > Show Entity Names = ShowNames: Haken+Anzeige (leer) |
| `Menue \| ShowLightRadiuses \| radiant/mainframe.cpp:1009` | OK | Menue View > Show > Show Light Radiuses = ShowLightRadiuses: Haken+Anzeige (leer) |
| `Menue \| ShowBboxes \| radiant/mainframe.cpp:1010` | OK | Menue View > Show > Show Entity Boxes = ShowBboxes: Haken+Anzeige (leer) |
| `Menue \| ShowConnections \| radiant/mainframe.cpp:1011` | OK | Menue View > Show > Show Entity Connections = ShowConnections: Haken+Anzeige (leer) |
| `Menue \| ShowSize2d \| radiant/mainframe.cpp:1015` | OK | Menue View > Show > Show 2D Size Info = ShowSize2d: Haken+Anzeige (leer) |
| `Menue \| ShowSize3d \| radiant/mainframe.cpp:1016` | OK | Menue View > Show > Show 3D Size Info = ShowSize3d: Haken+Einstellung (leer) |
| `Menue \| ToggleCrosshairs \| radiant/mainframe.cpp:1017` | OK | Menue View > Show > Show Crosshair = ToggleCrosshairs: Haken+Anzeige (leer) |
| `Menue \| ToggleGrid \| radiant/mainframe.cpp:1018` | OK | Menue View > Show > Show Grid = ToggleGrid: Haken+Anzeige (leer) |
| `Menue \| ShowBlocks \| radiant/mainframe.cpp:1019` | OK | Menue View > Show > Show Blocks = ShowBlocks: Haken+Anzeige (leer) |
| `Menue \| ShowCoordinates \| radiant/mainframe.cpp:1020` | OK | Menue View > Show > Show Coordinates = ShowCoordinates: Haken+Anzeige (leer) |
| `Menue \| ShowWindowOutline \| radiant/mainframe.cpp:1021` | OK | Menue View > Show > Show Window Outline = ShowWindowOutline: Haken+Anzeige (leer) |
| `Menue \| ShowAxes \| radiant/mainframe.cpp:1022` | OK | Menue View > Show > Show Axes = ShowAxes: Haken+Anzeige (leer) |
| `Menue \| ShowWorkzone2d \| radiant/mainframe.cpp:1023` | OK | Menue View > Show > Show 2D Workzone = ShowWorkzone2d: Haken+Anzeige (leer) |
| `Menue \| ShowWorkzone3d \| radiant/mainframe.cpp:1024` | OK | Menue View > Show > Show 3D Workzone = ShowWorkzone3d: Haken+Einstellung (leer) |
| `Menue \| ShowStats \| radiant/mainframe.cpp:1025` | OK | Menue View > Show > Show Renderer Stats = ShowStats: Haken+Einstellung (leer) |
| `Menue \| HideSelected \| radiant/mainframe.cpp:1037` | OK | Menue View > Hide Selected = HideSelected: Auswahl+Sicht (Pinsel) |
| `Menue \| ShowHidden \| radiant/mainframe.cpp:1038` | OK | Menue View > Show Hidden = ShowHidden: Sicht (Pinsel versteckt) |
| `Menue \| RegionOff \| radiant/mainframe.cpp:1046` | OK | Menue View > Region > Off = RegionOff: Region (Region an) |
| `Menue \| RegionSetXY \| radiant/mainframe.cpp:1047` | OK | Menue View > Region > Set XY = RegionSetXY: Region (leer) |
| `Menue \| RegionSetBrush \| radiant/mainframe.cpp:1048` | OK | Menue View > Region > Set Brush = RegionSetBrush: Karte+Region (Pinsel) |
| `Menue \| RegionSetSelection \| radiant/mainframe.cpp:1049` | OK | Menue View > Region > Set Selection = RegionSetSelection: Region (Pinsel) |
| `Menue \| DragEdges \| radiant/mainframe.cpp:1065` | OK | Menue Modify > Components > Edges = DragEdges: Haken+Modus (Pinsel) |
| `Menue \| DragVertices \| radiant/mainframe.cpp:1066` | OK | Menue Modify > Components > Vertices = DragVertices: Haken+Modus (Pinsel) |
| `Menue \| DragFaces \| radiant/mainframe.cpp:1067` | OK | Menue Modify > Components > Faces = DragFaces: Haken+Modus (Pinsel) |
| `Menue \| SnapToGrid \| radiant/mainframe.cpp:1071` | OK | Menue Modify > Snap To Grid = SnapToGrid: Karte (Pinsel neben dem Raster) |
| `Menue \| SelectNudgeLeft \| radiant/mainframe.cpp:1080` | OK | Menue Modify > Nudge > Nudge Left = SelectNudgeLeft: Karte (Pinsel) |
| `Menue \| SelectNudgeRight \| radiant/mainframe.cpp:1081` | OK | Menue Modify > Nudge > Nudge Right = SelectNudgeRight: Karte (Pinsel) |
| `Menue \| SelectNudgeUp \| radiant/mainframe.cpp:1082` | OK | Menue Modify > Nudge > Nudge Up = SelectNudgeUp: Karte (Pinsel) |
| `Menue \| SelectNudgeDown \| radiant/mainframe.cpp:1083` | OK | Menue Modify > Nudge > Nudge Down = SelectNudgeDown: Karte (Pinsel) |
| `Menue \| MoveSelectionUP \| radiant/mainframe.cpp:1085` | OK | Menue Modify > Nudge > Nudge +Z = MoveSelectionUP: Karte (Pinsel) |
| `Menue \| MoveSelectionDOWN \| radiant/mainframe.cpp:1086` | OK | Menue Modify > Nudge > Nudge -Z = MoveSelectionDOWN: Karte (Pinsel) |
| `Menue \| RotateSelectionX \| radiant/mainframe.cpp:1093` | OK | Menue Modify > Rotate > Rotate X = RotateSelectionX: Karte (Pinsel) |
| `Menue \| RotateSelectionY \| radiant/mainframe.cpp:1094` | OK | Menue Modify > Rotate > Rotate Y = RotateSelectionY: Karte (Pinsel) |
| `Menue \| RotateSelectionZ \| radiant/mainframe.cpp:1095` | OK | Menue Modify > Rotate > Rotate Z = RotateSelectionZ: Karte (Pinsel) |
| `Menue \| RotateSelectionClockwise \| radiant/mainframe.cpp:1097` | OK | Menue Modify > Rotate > Rotate Clockwise = RotateSelectionClockwise: Karte (Pinsel) |
| `Menue \| RotateSelectionAnticlockwise \| radiant/mainframe.cpp:1098` | OK | Menue Modify > Rotate > Rotate Anticlockwise = RotateSelectionAnticlockwise: Karte (Pinsel) |
| `Menue \| MirrorSelectionX \| radiant/mainframe.cpp:1105` | OK | Menue Modify > Flip > Flip X = MirrorSelectionX: Karte (Pinsel) |
| `Menue \| MirrorSelectionY \| radiant/mainframe.cpp:1106` | OK | Menue Modify > Flip > Flip Y = MirrorSelectionY: Karte (Pinsel) |
| `Menue \| MirrorSelectionZ \| radiant/mainframe.cpp:1107` | OK | Menue Modify > Flip > Flip Z = MirrorSelectionZ: Karte (Pinsel) |
| `Menue \| MirrorSelectionHorizontally \| radiant/mainframe.cpp:1109` | OK | Menue Modify > Flip > Flip Horizontally = MirrorSelectionHorizontally: Karte (Pinsel) |
| `Menue \| MirrorSelectionVertically \| radiant/mainframe.cpp:1110` | OK | Menue Modify > Flip > Flip Vertically = MirrorSelectionVertically: Karte (Pinsel) |
| `Menue \| ArbitraryRotation \| radiant/mainframe.cpp:1113` | OK | Menue Modify > Arbitrary rotation... = ArbitraryRotation: Dialog (Pinsel) |
| `Menue \| ArbitraryScale \| radiant/mainframe.cpp:1114` | OK | Menue Modify > Arbitrary scale... = ArbitraryScale: Dialog (Pinsel) |
| `Menue \| RepeatTransforms \| radiant/mainframe.cpp:1121` | OK | Menue Modify > Repeat > Repeat Transforms = RepeatTransforms: Karte+Auswahl (Pinsel gedreht) |
| `Menue \| (dynamisch) \| radiant/mainframe.cpp:1125` | OK | Menue Modify > Repeat > Translate = TransformTranslate: Modus (Pinsel gedreht); Menue Modify > Repeat > Rotate = TransformRotate: Modus (Pinsel gedreht); Menue Modify > Repeat > Scale = TransformScale: Modus (Pinsel gedreht) |
| `Menue \| ResetTransforms \| radiant/mainframe.cpp:1133` | OK | Menue Modify > Repeat > Reset Transforms = ResetTransforms: Modus (Pinsel gedreht) |
| `Menue \| BuildMenuCustomize \| radiant/mainframe.cpp:1143` | OK | Menue Build > Customize... = BuildMenuCustomize: Dialog (leer) |
| `Menue \| Build_runRecentExecutedBuild \| radiant/mainframe.cpp:1144` | OK | Menue Build > Run recent build = Build_runRecentExecutedBuild: Bau (leer) |
| `Menue \| FindBrush \| radiant/mainframe.cpp:1173` | OK | Menue Misc > Find brush... = FindBrush: Dialog (leer) |
| `Menue \| MapInfo \| radiant/mainframe.cpp:1174` | OK | Menue Misc > Map Info... = MapInfo: Dialog (leer) |
| `Menue \| RefreshReferences \| radiant/mainframe.cpp:1175` | OK | Menue Misc > Refresh models = RefreshReferences: Neu geladen (leer) |
| `Menue \| Set 2D &Background image...` | OK | Menue Misc > Set 2D Background image... = WXY_SetBackgroundImage: 2D-Ansicht (Pinsel) |
| `Menue \| Fullscreen \| radiant/mainframe.cpp:1177` | OK | Menue Misc > Fullscreen = Fullscreen: Fenster (leer) |
| `Menue \| MaximizeView \| radiant/mainframe.cpp:1178` | OK | Menue Misc > Maximize view = MaximizeView: Fenster (leer) |
| `Menue \| Bug report` | OK | Menue Help > Bug report = OpenBugReportURL: Extern (leer) |
| `Menue \| CheckForUpdate \| radiant/mainframe.cpp:1221` | OK | Menue Help > Check for NetRadiant update = CheckForUpdate: Extern (leer) |
| `Menue \| &About` | OK | Menue Help > About = DoAbout: Dialog (leer) |
| `Menue \| MRU 1` | OK | Menue File > &1 = MRU1: Karte (Pinsel) |
| `Menue \| MRU 2` | OK | Menue File > &2 = MRU2: Karte (Pinsel) |
| `Menue \| MRU 3` | OK | Menue File > &3 = MRU3: Karte (Pinsel) |
| `Menue \| MRU 4` | OK | Menue File > &4 = MRU4: Karte (Pinsel) |
| `Menue \| MRU 5` | OK | Menue File > &5 = MRU5: Karte (Pinsel) |
| `Menue \| MRU 6` | OK | Menue File > &6 = MRU6: Karte (Pinsel) |
| `Menue \| MRU 7` | OK | Menue File > &7 = MRU7: Karte (Pinsel) |
| `Menue \| MRU 8` | OK | Menue File > &8 = MRU8: Karte (Pinsel) |
| `Menue \| MRU 9` | OK | Menue File > &9 = MRU9: Karte (Pinsel) |
| `Menue \| SimplePatchMesh \| radiant/patchmanip.cpp:739` | OK | Menue Curve > Simple Patch Mesh... = SimplePatchMesh: Dialog (leer) |
| `Menue \| PatchBevel \| radiant/patchmanip.cpp:740` | OK | Menue Curve > Bevel = PatchBevel: Karte+Auswahl (Pinsel) |
| `Menue \| PatchEndCap \| radiant/patchmanip.cpp:741` | OK | Menue Curve > End cap = PatchEndCap: Karte+Auswahl (Pinsel) |
| `Menue \| PatchCylinder \| radiant/patchmanip.cpp:742` | OK | Menue Curve > Cylinder (9x3) = PatchCylinder: Karte+Auswahl (Pinsel) |
| `Menue \| PatchSquareCylinder \| radiant/patchmanip.cpp:743` | OK | Menue Curve > Square Cylinder (9x3) = PatchSquareCylinder: Karte+Auswahl (Pinsel) |
| `Menue \| PatchXactCylinder \| radiant/patchmanip.cpp:744` | OK | Menue Curve > Exact Cylinder... = PatchXactCylinder: Dialog (Pinsel) |
| `Menue \| PatchCone \| radiant/patchmanip.cpp:745` | OK | Menue Curve > Cone (9x3) = PatchCone: Karte+Auswahl (Pinsel) |
| `Menue \| PatchXactCone \| radiant/patchmanip.cpp:746` | OK | Menue Curve > Exact Cone... = PatchXactCone: Dialog (Pinsel) |
| `Menue \| PatchSphere \| radiant/patchmanip.cpp:747` | OK | Menue Curve > Sphere (9x5) = PatchSphere: Karte+Auswahl (Pinsel) |
| `Menue \| PatchXactSphere \| radiant/patchmanip.cpp:748` | OK | Menue Curve > Exact Sphere... = PatchXactSphere: Dialog (Pinsel) |
| `Menue \| CapCurrentCurve \| radiant/patchmanip.cpp:764` | OK | Menue Curve > Cap Selection = CapCurrentCurve: Dialog (Patch) |
| `Menue \| PatchInsertFirstColumn \| radiant/patchmanip.cpp:771` | OK | Menue Curve > Insert/Delete > Insert (2) First Columns = PatchInsertFirstColumn: Karte (Patch) |
| `Menue \| PatchInsertLastColumn \| radiant/patchmanip.cpp:772` | OK | Menue Curve > Insert/Delete > Insert (2) Last Columns = PatchInsertLastColumn: Karte (Patch) |
| `Menue \| PatchInsertFirstRow \| radiant/patchmanip.cpp:774` | OK | Menue Curve > Insert/Delete > Insert (2) First Rows = PatchInsertFirstRow: Karte (Patch) |
| `Menue \| PatchInsertLastRow \| radiant/patchmanip.cpp:775` | OK | Menue Curve > Insert/Delete > Insert (2) Last Rows = PatchInsertLastRow: Karte (Patch) |
| `Menue \| PatchDeleteFirstColumn \| radiant/patchmanip.cpp:777` | OK | Menue Curve > Insert/Delete > Del First (2) Columns = PatchDeleteFirstColumn: Karte (Patch 5x5) |
| `Menue \| PatchDeleteLastColumn \| radiant/patchmanip.cpp:778` | OK | Menue Curve > Insert/Delete > Del Last (2) Columns = PatchDeleteLastColumn: Karte (Patch 5x5) |
| `Menue \| PatchDeleteFirstRow \| radiant/patchmanip.cpp:780` | OK | Menue Curve > Insert/Delete > Del First (2) Rows = PatchDeleteFirstRow: Karte (Patch 5x5) |
| `Menue \| PatchDeleteLastRow \| radiant/patchmanip.cpp:781` | OK | Menue Curve > Insert/Delete > Del Last (2) Rows = PatchDeleteLastRow: Karte (Patch 5x5) |
| `Menue \| InvertCurve \| radiant/patchmanip.cpp:789` | OK | Menue Curve > Matrix > Invert = InvertCurve: Karte (Patch) |
| `Menue \| MatrixTranspose \| radiant/patchmanip.cpp:790` | OK | Menue Curve > Matrix > Transpose = MatrixTranspose: Karte (Patch) |
| `Menue \| RedisperseRows \| radiant/patchmanip.cpp:793` | OK | Menue Curve > Matrix > Re-disperse Rows = RedisperseRows: Karte (Patch 5x5) |
| `Menue \| RedisperseCols \| radiant/patchmanip.cpp:794` | OK | Menue Curve > Matrix > Re-disperse Columns = RedisperseCols: Karte (Patch 5x5) |
| `Menue \| SmoothRows \| radiant/patchmanip.cpp:797` | OK | Menue Curve > Matrix > Smooth Rows = SmoothRows: Karte (Patch 5x5) |
| `Menue \| SmoothCols \| radiant/patchmanip.cpp:798` | OK | Menue Curve > Matrix > Smooth Columns = SmoothCols: Karte (Patch 5x5) |
| `Menue \| TextureReset/Cap \| radiant/patchmanip.cpp:806` | OK | Menue Brush > Reset Texture = TextureReset/Cap: Karte (Pinsel, Textur versetzt); (+1) |
| `Menue \| NaturalizePatch \| radiant/patchmanip.cpp:807` | OK | Menue Curve > Texture > Naturalize = NaturalizePatch: Karte (Patch) |
| `Menue \| InvertCurveTextureX \| radiant/patchmanip.cpp:808` | OK | Menue Curve > Texture > Invert X = InvertCurveTextureX: Karte (Patch) |
| `Menue \| InvertCurveTextureY \| radiant/patchmanip.cpp:809` | OK | Menue Curve > Texture > Invert Y = InvertCurveTextureY: Karte (Patch) |
| `Menue \| PatchDeform \| radiant/patchmanip.cpp:821` | OK | Menue Curve > Deform... = PatchDeform: Dialog (Patch) |
| `Menue \| PatchThicken \| radiant/patchmanip.cpp:822` | OK | Menue Curve > Thicken... = PatchThicken: Dialog (Patch) |
| `Menue \| ShowInUse \| radiant/texwindow.cpp:1239` | OK | Menue Texturbrowser View > Hide Unused = ShowInUse: Haken+Texturen (leer) |
| `Menue \| ShowAllTextures \| radiant/texwindow.cpp:1240` | OK | Menue Texturbrowser View > Show All = ShowAllTextures: Texturen (leer) |
| `Menue \| ToggleShowShaders \| radiant/texwindow.cpp:1250` | OK | Menue Texturbrowser View > Show Shaders = ToggleShowShaders: Haken+Einstellung (leer) |
| `Menue \| ToggleShowTextures \| radiant/texwindow.cpp:1251` | OK | Menue Texturbrowser View > Show Textures = ToggleShowTextures: Haken+Einstellung (leer) |
| `Menue \| ToggleShowShaderlistOnly \| radiant/texwindow.cpp:1256` | OK | Menue Texturbrowser View > ShaderList Only = ToggleShowShaderlistOnly: Haken+Einstellung (leer) |
| `Menue \| FilterNotex \| radiant/texwindow.cpp:1259` | OK | Menue Texturbrowser View > Hide Image Missing = FilterNotex: Haken+Einstellung (leer) |
| `Menue \| FixedSize \| radiant/texwindow.cpp:1263` | OK | Menue Texturbrowser View > Fixed Size = FixedSize: Haken+Einstellung (leer) |
| `Menue \| EnableAlpha \| radiant/texwindow.cpp:1264` | OK | Menue Texturbrowser View > Transparency = EnableAlpha: Haken+Einstellung (leer) |
| `Menue \| TagsToggleGui \| radiant/texwindow.cpp:1267` | OK | Menue Texturbrowser View > Tags GUI = TagsToggleGui: Haken+Einstellung (leer) |
| `Menue \| TagAdd \| radiant/texwindow.cpp:1282` | OK | Menue Texturbrowser Tags > Add tag = TagAdd: Texturen (Tag markiert) |
| `Menue \| TagRename \| radiant/texwindow.cpp:1283` | OK | Menue Texturbrowser Tags > Rename tag = TagRename: Texturen (Tag markiert) |
| `Menue \| TagDelete \| radiant/texwindow.cpp:1284` | OK | Menue Texturbrowser Tags > Delete tag = TagDelete: Dialog (Tag markiert) |
| `Menue \| TagCopy \| radiant/texwindow.cpp:1286` | OK | Menue Texturbrowser Tags > Copy tags from selected = TagCopy: Texturen (Tag markiert) |
| `Menue \| TagPaste \| radiant/texwindow.cpp:1287` | OK | Menue Texturbrowser Tags > Paste tags to selected = TagPaste: Texturen+Datei (Tag kopiert) |
| `Menue \| TagSearch \| radiant/texwindow.cpp:1289` | OK | Menue Texturbrowser Tags > Search tag = TagSearch: Texturen (Tag markiert) |
| `Menue \| TagSearchUntagged \| radiant/texwindow.cpp:1290` | OK | Menue Texturbrowser Tags > Search Untagged = TagSearchUntagged: Dialog (Tag markiert) |

## Werkzeugleisten

| Eintrag | Stand | Pruefung (Wirkung) |
|---|---|---|
| `Knopf \| ToggleCubicClip` | OK | Knopf ToggleCubicClip: Haken+Einstellung (leer) |
| `Knopf \| FilterWorldBrushes` | OK | Knopf FilterWorldBrushes: Filter+Haken+Einstellung (leer) |
| `Knopf \| FilterStructural` | OK | Knopf FilterStructural: Filter+Haken+Einstellung (leer) |
| `Knopf \| FilterDetails` | OK | Knopf FilterDetails: Filter+Haken+Einstellung (leer) |
| `Knopf \| FilterFuncGroups` | OK | Knopf FilterFuncGroups: Filter+Haken+Einstellung (leer) |
| `Knopf \| FilterPatches` | OK | Knopf FilterPatches: Filter+Haken+Einstellung (leer) |
| `Knopf \| FilterTranslucent` | OK | Knopf FilterTranslucent: Filter+Haken+Einstellung (leer) |
| `Knopf \| FilterLiquids` | OK | Knopf FilterLiquids: Filter+Haken+Einstellung (leer) |
| `Knopf \| FilterCaulk` | OK | Knopf FilterCaulk: Filter+Haken+Einstellung (leer) |
| `Knopf \| FilterClips` | OK | Knopf FilterClips: Filter+Haken+Einstellung (leer) |
| `Knopf \| FilterHintsSkips` | OK | Knopf FilterHintsSkips: Filter+Haken+Einstellung (leer) |
| `Knopf \| FilterSky` | OK | Knopf FilterSky: Filter+Haken+Einstellung (leer) |
| `Knopf \| FilterEntities` | OK | Knopf FilterEntities: Filter+Haken+Einstellung (leer) |
| `Knopf \| FilterPointEntities` | OK | Knopf FilterPointEntities: Filter+Haken+Einstellung (leer) |
| `Knopf \| FilterLights` | OK | Knopf FilterLights: Filter+Haken+Einstellung (leer) |
| `Knopf \| FilterModels` | OK | Knopf FilterModels: Filter+Haken+Einstellung (leer) |
| `Knopf \| FilterTriggers` | OK | Knopf FilterTriggers: Filter+Haken+Einstellung (leer) |
| `Knopf \| ResetFilters` | OK | Knopf ResetFilters: Filter (Filter an) |
| `Knopf \| RegionSetSelection` | OK | Knopf RegionSetSelection: Region (Pinsel) |
| `Knopf \| HideSelected` | OK | Knopf HideSelected: Auswahl+Sicht (Pinsel) |
| `Knopf \| OpenMap` | OK | Knopf OpenMap: Karte (leer) |
| `Knopf \| SaveMap` | OK | Knopf SaveMap: Datei (Karte geaendert) |
| `Knopf \| Undo` | OK | Knopf Undo: Klon weg, Konsole "Undo: cloneSelectedMakeUnique"; (+1) |
| `Knopf \| Redo` | OK | Knopf Redo: Klon wieder da, Konsole "Redo: cloneSelectedMakeUnique"; (+1) |
| `Knopf \| MirrorSelectionHorizontally` | OK | Knopf MirrorSelectionHorizontally: Karte (Pinsel) |
| `Knopf \| MirrorSelectionVertically` | OK | Knopf MirrorSelectionVertically: Karte (Pinsel) |
| `Knopf \| RotateSelectionAnticlockwise` | OK | Knopf RotateSelectionAnticlockwise: Karte (Pinsel) |
| `Knopf \| RotateSelectionClockwise` | OK | Knopf RotateSelectionClockwise: Karte (Pinsel) |
| `Knopf \| SelectTouching` | OK | Knopf SelectTouching: Auswahl (Pinsel) |
| `Knopf \| SelectInside` | OK | Knopf SelectInside: Karte+Auswahl (Pinsel) |
| `Knopf \| CSGSubtract` | OK | Knopf CSGSubtract: Karte (Pinsel B) |
| `Knopf \| CSGWrapMerge` | OK | Knopf CSGWrapMerge: Karte (zwei Pinsel) |
| `Knopf \| CSGroom` | OK | Knopf CSGroom: Karte (Pinsel) |
| `Knopf \| CSGTool` | OK | Knopf CSGTool: Dialog (Pinsel) |
| `Knopf \| DragVertices` | OK | Knopf DragVertices: Haken+Modus (Pinsel) |
| `Knopf \| DragEdges` | OK | Knopf DragEdges: Haken+Modus (Pinsel) |
| `Knopf \| DragFaces` | OK | Knopf DragFaces: Haken+Modus (Pinsel) |
| `Knopf \| NextView` | OK | Knopf NextView: 2D-Ansicht (leer) |
| `Knopf \| MouseDrag` | OK | Knopf MouseDrag: Werkzeug gewaehlt, Knopf gedrueckt; (+1) |
| `Knopf \| ToggleClipper` | OK | Knopf ToggleClipper: Haken+Modus (Pinsel) |
| `Knopf \| MouseTranslate` | OK | Knopf MouseTranslate: Werkzeug gewaehlt, Knopf gedrueckt; (+1) |
| `Knopf \| MouseRotate` | OK | Knopf MouseRotate: Werkzeug gewaehlt, Knopf gedrueckt; (+1) |
| `Knopf \| MouseScale` | OK | Knopf MouseScale: Werkzeug gewaehlt, Knopf gedrueckt; (+1) |
| `Knopf \| MouseTransform` | OK | Knopf MouseTransform: Werkzeug gewaehlt, Knopf gedrueckt; (+1) |
| `Knopf \| MouseUV` | OK | Knopf MouseUV: Haken+Modus (Pinsel) |
| `Knopf \| TogTexLock` | OK | Knopf TogTexLock: Haken+Modus (leer) |
| `Knopf \| TogTexVertexLock` | OK | Knopf TogTexVertexLock: Haken+Modus (leer) |
| `Knopf \| ToggleEntityInspector` | OK | Knopf ToggleEntityInspector: Fenster (leer) |
| `Knopf \| ToggleConsole` | OK | Knopf ToggleConsole: Fenster (leer) |
| `Knopf \| ToggleTextures` | OK | Knopf ToggleTextures: Fenster (leer) |
| `Knopf \| RefreshReferences` | OK | Knopf RefreshReferences: Neu geladen (leer) |
| `Knopf \| CapCurrentCurve` | OK | Knopf CapCurrentCurve: Dialog (Patch) |
| `Knopf \| FindReplaceTextures` | OK | Knopf FindReplaceTextures: Dialog (Texturbrowser offen) |
| `Knopf \| RefreshShaders` | OK | Knopf RefreshShaders: Neu geladen (Texturbrowser offen) |

## Registrierte Befehle

| Eintrag | Stand | Pruefung (Wirkung) |
|---|---|---|
| `Befehl \| ArbitraryRotation \| Commands` | OK | Wirkung ArbitraryRotation: Dialog (Pinsel); Robust ArbitraryRotation: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Shift+R = ArbitraryRotation: Dialog (Pinsel) |
| `Befehl \| ArbitraryScale \| Commands` | OK | Wirkung ArbitraryScale: Dialog (Pinsel); Robust ArbitraryScale: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+Shift+S = ArbitraryScale: Dialog (Pinsel) |
| `Befehl \| AutoCaulkSelected \| Commands` | OK | Wirkung AutoCaulkSelected: Datei (Pinsel); Robust AutoCaulkSelected: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste F4 = AutoCaulkSelected: Datei (Pinsel) |
| `Befehl \| bobToolz::About \| Commands` | OK | Wirkung bobToolz::About: Dialog (leer); Robust bobToolz::About: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| bobToolz::ApertureDoor \| Commands` | OK | Wirkung bobToolz::ApertureDoor: Dialog (Pinsel); Robust bobToolz::ApertureDoor: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| bobToolz::BrushCleanup \| Commands` | OK | Wirkung bobToolz::BrushCleanup: Auswahl+Konsole (Pinsel); Robust bobToolz::BrushCleanup: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| bobToolz::CaulkSelection \| Commands` | OK | Wirkung bobToolz::CaulkSelection: Karte (Pinsel); Robust bobToolz::CaulkSelection: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| bobToolz::DoorBuilder \| Commands` | OK | Wirkung bobToolz::DoorBuilder: Dialog (Pinsel); Robust bobToolz::DoorBuilder: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| bobToolz::DropEntity \| Commands` | OK | Wirkung bobToolz::DropEntity: Karte (Licht, Tree Planter an); Robust bobToolz::DropEntity: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| bobToolz::ExplodeEntity \| Commands` | OK | Wirkung bobToolz::ExplodeEntity: Karte (Pinsel der func_group); Robust bobToolz::ExplodeEntity: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| bobToolz::FindDuplicates \| Commands` | OK | Wirkung bobToolz::FindDuplicates: Auswahl (Pinsel doppelt); Robust bobToolz::FindDuplicates: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| bobToolz::FlipTerrain \| Commands` | OK | Wirkung bobToolz::FlipTerrain: Karte (Terrain); Robust bobToolz::FlipTerrain: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| bobToolz::Intersect \| Commands` | OK | Wirkung bobToolz::Intersect: Dialog (Pinsel); Robust bobToolz::Intersect: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| bobToolz::MakeChain \| Commands` | OK | Wirkung bobToolz::MakeChain: Dialog (Pinsel); Robust bobToolz::MakeChain: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| bobToolz::MergePatches \| Commands` | OK | Wirkung bobToolz::MergePatches: Karte (zwei Patches); Robust bobToolz::MergePatches: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| bobToolz::PathPlotter \| Commands` | OK | Wirkung bobToolz::PathPlotter: Dialog (Pinsel); Robust bobToolz::PathPlotter: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| bobToolz::PitOMatic \| Commands` | OK | Wirkung bobToolz::PitOMatic: Karte (hoher Kasten); Robust bobToolz::PitOMatic: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| bobToolz::PlotSplines \| Commands` | OK | Wirkung bobToolz::PlotSplines: Anzeige (leer); Robust bobToolz::PlotSplines: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| bobToolz::PolygonBuilder \| Commands` | OK | Wirkung bobToolz::PolygonBuilder: Dialog (Pinsel); Robust bobToolz::PolygonBuilder: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| bobToolz::ResetTextures \| Commands` | OK | Wirkung bobToolz::ResetTextures: Dialog (Pinsel); Robust bobToolz::ResetTextures: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| bobToolz::SplitPatch \| Commands` | OK | Wirkung bobToolz::SplitPatch: Karte (Patch); Robust bobToolz::SplitPatch: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| bobToolz::SplitPatchColumns \| Commands` | OK | Wirkung bobToolz::SplitPatchColumns: Karte (Patch); Robust bobToolz::SplitPatchColumns: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| bobToolz::SplitPatchRows \| Commands` | OK | Wirkung bobToolz::SplitPatchRows: Karte (Patch); Robust bobToolz::SplitPatchRows: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| bobToolz::StairBuilder \| Commands` | OK | Wirkung bobToolz::StairBuilder: Dialog (Pinsel); Robust bobToolz::StairBuilder: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| bobToolz::TreePlanter \| Commands` | OK | Wirkung bobToolz::TreePlanter: Haken+Modus (Pinsel); Robust bobToolz::TreePlanter: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| bobToolz::VisViewer \| Commands` | OK | Wirkung bobToolz::VisViewer: Dialog (Pinsel); Robust bobToolz::VisViewer: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| Brush3Sided \| Commands` | OK | Wirkung Brush3Sided: Karte (Pinsel); Robust Brush3Sided: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+3 = Brush3Sided: Karte (Pinsel) |
| `Befehl \| Brush4Sided \| Commands` | OK | Wirkung Brush4Sided: Karte (Pinsel); Robust Brush4Sided: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+4 = Brush4Sided: Karte (Pinsel) |
| `Befehl \| Brush5Sided \| Commands` | OK | Wirkung Brush5Sided: Karte (Pinsel); Robust Brush5Sided: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+5 = Brush5Sided: Karte (Pinsel) |
| `Befehl \| Brush6Sided \| Commands` | OK | Wirkung Brush6Sided: Karte (Pinsel); Robust Brush6Sided: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+6 = Brush6Sided: Karte (Pinsel) |
| `Befehl \| Brush7Sided \| Commands` | OK | Wirkung Brush7Sided: Karte (Pinsel); Robust Brush7Sided: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+7 = Brush7Sided: Karte (Pinsel) |
| `Befehl \| Brush8Sided \| Commands` | OK | Wirkung Brush8Sided: Karte (Pinsel); Robust Brush8Sided: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+8 = Brush8Sided: Karte (Pinsel) |
| `Befehl \| Brush9Sided \| Commands` | OK | Wirkung Brush9Sided: Karte (Pinsel); Robust Brush9Sided: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+9 = Brush9Sided: Karte (Pinsel) |
| `Befehl \| BrushCone \| Commands` | OK | Wirkung BrushCone: Dialog (Pinsel); Robust BrushCone: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| brushexport2::About \| Commands` | OK | Wirkung brushexport2::About: Dialog (leer); Robust brushexport2::About: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| brushexport2::Export.obj \| Commands` | OK | Wirkung brushexport2::Export.obj: Dialog (Pinsel); Robust brushexport2::Export.obj: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| BrushIcosahedron \| Commands` | OK | Wirkung BrushIcosahedron: Dialog (Pinsel); Robust BrushIcosahedron: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| BrushPrism \| Commands` | OK | Wirkung BrushPrism: Dialog (Pinsel); Robust BrushPrism: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| BrushRock \| Commands` | OK | Wirkung BrushRock: Dialog (Pinsel); Robust BrushRock: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| BrushSphere \| Commands` | OK | Wirkung BrushSphere: Dialog (Pinsel); Robust BrushSphere: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| Build_runRecentExecutedBuild \| Commands` | OK | Wirkung Build_runRecentExecutedBuild: Bau (leer); Robust Build_runRecentExecutedBuild: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste F5 = Build_runRecentExecutedBuild: Bau (leer) |
| `Befehl \| BuildMenuCustomize \| Commands` | OK | Wirkung BuildMenuCustomize: Dialog (leer); Robust BuildMenuCustomize: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| CameraAngleDown \| Commands` | OK | Wirkung CameraAngleDown: Kamera (leer); Robust CameraAngleDown: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| CameraAngleDown \| KeyEvents` | OK | Ereignis CameraAngleDown: Kamera bewegt, solange die Taste gehalten wird (nicht diskret) |
| `Befehl \| CameraAngleDown \| Shortcuts` | OK | Ereignis CameraAngleDown: Kamera bewegt, solange die Taste gehalten wird (nicht diskret) |
| `Befehl \| CameraAngleUp \| Commands` | OK | Wirkung CameraAngleUp: Kamera (leer); Robust CameraAngleUp: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| CameraAngleUp \| KeyEvents` | OK | Ereignis CameraAngleUp: Kamera bewegt, solange die Taste gehalten wird (nicht diskret) |
| `Befehl \| CameraAngleUp \| Shortcuts` | OK | Ereignis CameraAngleUp: Kamera bewegt, solange die Taste gehalten wird (nicht diskret) |
| `Befehl \| CameraBack \| Commands` | OK | Wirkung CameraBack: Kamera (leer); Robust CameraBack: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| CameraBack \| KeyEvents` | OK | Ereignis CameraBack: Kamera bewegt, solange die Taste gehalten wird (nicht diskret) |
| `Befehl \| CameraBack \| Shortcuts` | OK | Taste Down = CameraBack: Kamera (leer) |
| `Befehl \| CameraDown \| Commands` | OK | Wirkung CameraDown: Kamera (leer); Robust CameraDown: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| CameraDown \| KeyEvents` | OK | Ereignis CameraDown: Kamera bewegt, solange die Taste gehalten wird (nicht diskret) |
| `Befehl \| CameraDown \| Shortcuts` | OK | Ereignis CameraDown: Kamera bewegt, solange die Taste gehalten wird (nicht diskret) |
| `Befehl \| CameraFocusOnSelected \| Commands` | OK | Wirkung CameraFocusOnSelected: Kamera (Pinsel); Robust CameraFocusOnSelected: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Tab = CameraFocusOnSelected: Kamera (Pinsel) |
| `Befehl \| CameraForward \| Commands` | OK | Wirkung CameraForward: Kamera (leer); Robust CameraForward: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| CameraForward \| KeyEvents` | OK | Ereignis CameraForward: Kamera bewegt, solange die Taste gehalten wird (nicht diskret) |
| `Befehl \| CameraForward \| Shortcuts` | OK | Taste Up = CameraForward: Kamera (leer) |
| `Befehl \| CameraFreeFocus \| KeyEvents` | OK | Ereignis CameraFreeFocus: Kamera bewegt, solange die Taste gehalten wird (Freelook) |
| `Befehl \| CameraFreeFocus \| Shortcuts` | OK | Taste Tab = CameraFreeFocus: Kamera (Freelook) |
| `Befehl \| CameraFreeMoveBack \| KeyEvents` | OK | Ereignis CameraFreeMoveBack: Kamera bewegt, solange die Taste gehalten wird (Freelook) |
| `Befehl \| CameraFreeMoveBack \| Shortcuts` | OK | Taste S = CameraFreeMoveBack: Kamera (Freelook) |
| `Befehl \| CameraFreeMoveBack2 \| KeyEvents` | OK | Ereignis CameraFreeMoveBack2: Kamera bewegt, solange die Taste gehalten wird (Freelook) |
| `Befehl \| CameraFreeMoveBack2 \| Shortcuts` | OK | Taste Down = CameraFreeMoveBack2: Kamera (Freelook) |
| `Befehl \| CameraFreeMoveDown \| KeyEvents` | OK | Ereignis CameraFreeMoveDown: Kamera bewegt, solange die Taste gehalten wird (Freelook) |
| `Befehl \| CameraFreeMoveDown \| Shortcuts` | OK | Ereignis CameraFreeMoveDown: Kamera bewegt, solange die Taste gehalten wird (Freelook) |
| `Befehl \| CameraFreeMoveForward \| KeyEvents` | OK | Ereignis CameraFreeMoveForward: Kamera bewegt, solange die Taste gehalten wird (Freelook) |
| `Befehl \| CameraFreeMoveForward \| Shortcuts` | OK | Taste W = CameraFreeMoveForward: Kamera (Freelook) |
| `Befehl \| CameraFreeMoveForward2 \| KeyEvents` | OK | Ereignis CameraFreeMoveForward2: Kamera bewegt, solange die Taste gehalten wird (Freelook) |
| `Befehl \| CameraFreeMoveForward2 \| Shortcuts` | OK | Taste Up = CameraFreeMoveForward2: Kamera (Freelook) |
| `Befehl \| CameraFreeMoveLeft \| KeyEvents` | OK | Ereignis CameraFreeMoveLeft: Kamera bewegt, solange die Taste gehalten wird (Freelook) |
| `Befehl \| CameraFreeMoveLeft \| Shortcuts` | OK | Taste A = CameraFreeMoveLeft: Kamera (Freelook) |
| `Befehl \| CameraFreeMoveLeft2 \| KeyEvents` | OK | Ereignis CameraFreeMoveLeft2: Kamera bewegt, solange die Taste gehalten wird (Freelook) |
| `Befehl \| CameraFreeMoveLeft2 \| Shortcuts` | OK | Taste Left = CameraFreeMoveLeft2: Kamera (Freelook) |
| `Befehl \| CameraFreeMoveRight \| KeyEvents` | OK | Ereignis CameraFreeMoveRight: Kamera bewegt, solange die Taste gehalten wird (Freelook) |
| `Befehl \| CameraFreeMoveRight \| Shortcuts` | OK | Taste D = CameraFreeMoveRight: Kamera (Freelook) |
| `Befehl \| CameraFreeMoveRight2 \| KeyEvents` | OK | Ereignis CameraFreeMoveRight2: Kamera bewegt, solange die Taste gehalten wird (Freelook) |
| `Befehl \| CameraFreeMoveRight2 \| Shortcuts` | OK | Taste Right = CameraFreeMoveRight2: Kamera (Freelook) |
| `Befehl \| CameraFreeMoveUp \| KeyEvents` | OK | Ereignis CameraFreeMoveUp: Kamera bewegt, solange die Taste gehalten wird (Freelook) |
| `Befehl \| CameraFreeMoveUp \| Shortcuts` | OK | Ereignis CameraFreeMoveUp: Kamera bewegt, solange die Taste gehalten wird (Freelook) |
| `Befehl \| CameraLeft \| Commands` | OK | Wirkung CameraLeft: Kamera (leer); Robust CameraLeft: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| CameraLeft \| KeyEvents` | OK | Ereignis CameraLeft: Kamera bewegt, solange die Taste gehalten wird (nicht diskret) |
| `Befehl \| CameraLeft \| Shortcuts` | OK | Taste Left = CameraLeft: Kamera (leer) |
| `Befehl \| CameraModeNext \| Commands` | OK | Wirkung CameraModeNext: Einstellung (leer); Robust CameraModeNext: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Shift+] = CameraModeNext: Einstellung (leer) |
| `Befehl \| CameraModePrev \| Commands` | OK | Wirkung CameraModePrev: Einstellung (leer); Robust CameraModePrev: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Shift+[ = CameraModePrev: Einstellung (leer) |
| `Befehl \| CameraRight \| Commands` | OK | Wirkung CameraRight: Kamera (leer); Robust CameraRight: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| CameraRight \| KeyEvents` | OK | Ereignis CameraRight: Kamera bewegt, solange die Taste gehalten wird (nicht diskret) |
| `Befehl \| CameraRight \| Shortcuts` | OK | Taste Right = CameraRight: Kamera (leer) |
| `Befehl \| CameraSpeedDec \| Commands` | OK | Wirkung CameraSpeedDec: Einstellung (leer); Robust CameraSpeedDec: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Shift+Num+- = CameraSpeedDec: Einstellung (leer) |
| `Befehl \| CameraSpeedInc \| Commands` | OK | Wirkung CameraSpeedInc: Einstellung (leer); Robust CameraSpeedInc: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Shift+Num++ = CameraSpeedInc: Einstellung (leer) |
| `Befehl \| CameraStrafeLeft \| Commands` | OK | Wirkung CameraStrafeLeft: Kamera (leer); Robust CameraStrafeLeft: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| CameraStrafeLeft \| KeyEvents` | OK | Ereignis CameraStrafeLeft: Kamera bewegt, solange die Taste gehalten wird (nicht diskret) |
| `Befehl \| CameraStrafeLeft \| Shortcuts` | OK | Taste A = CameraStrafeLeft: Kamera (leer) |
| `Befehl \| CameraStrafeRight \| Commands` | OK | Wirkung CameraStrafeRight: Kamera (leer); Robust CameraStrafeRight: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| CameraStrafeRight \| KeyEvents` | OK | Ereignis CameraStrafeRight: Kamera bewegt, solange die Taste gehalten wird (nicht diskret) |
| `Befehl \| CameraStrafeRight \| Shortcuts` | OK | Taste D = CameraStrafeRight: Kamera (leer) |
| `Befehl \| CameraUp \| Commands` | OK | Wirkung CameraUp: Kamera (leer); Robust CameraUp: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| CameraUp \| KeyEvents` | OK | Ereignis CameraUp: Kamera bewegt, solange die Taste gehalten wird (nicht diskret) |
| `Befehl \| CameraUp \| Shortcuts` | OK | Ereignis CameraUp: Kamera bewegt, solange die Taste gehalten wird (nicht diskret) |
| `Befehl \| CapCurrentCurve \| Commands` | OK | Wirkung CapCurrentCurve: Dialog (Patch); Robust CapCurrentCurve: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Shift+C = CapCurrentCurve: Dialog (Patch) |
| `Befehl \| CenterView \| Commands` | OK | Wirkung CenterView: Kamera (Pinsel); Robust CenterView: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste End = CenterView: Kamera (Pinsel) |
| `Befehl \| CenterXYView \| Commands` | OK | Wirkung CenterXYView: 2D-Ansicht (Pinsel); Robust CenterXYView: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+Shift+Tab = CenterXYView: 2D-Ansicht (Pinsel) |
| `Befehl \| CheckForUpdate \| Commands` | OK | Wirkung CheckForUpdate: Extern (leer); Robust CheckForUpdate: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| ClipperClip \| Commands` | OK | Wirkung ClipperClip: Karte (Clipper mit zwei Punkten); Robust ClipperClip: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Return = ClipperClip: Karte (Clipper mit zwei Punkten) |
| `Befehl \| ClipperFlip \| Commands` | OK | Wirkung ClipperFlip: Modus (Clipper mit zwei Punkten); Robust ClipperFlip: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+Return = ClipperFlip: Modus (Clipper mit zwei Punkten) |
| `Befehl \| ClipperSplit \| Commands` | OK | Wirkung ClipperSplit: Karte (Clipper mit zwei Punkten); Robust ClipperSplit: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Shift+Return = ClipperSplit: Karte (Clipper mit zwei Punkten) |
| `Befehl \| CloneSelection \| Commands` | OK | Wirkung CloneSelection: Karte+Auswahl (Pinsel); Robust CloneSelection: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Space = CloneSelection: Karte+Auswahl (Pinsel) |
| `Befehl \| CloneSelectionAndMakeUnique \| Commands` | OK | Wirkung CloneSelectionAndMakeUnique: Karte+Auswahl (Licht); Robust CloneSelectionAndMakeUnique: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Shift+Space = CloneSelectionAndMakeUnique: Karte+Auswahl (Licht) |
| `Befehl \| Copy \| Commands` | OK | Wirkung Copy: Ablage (Pinsel); Robust Copy: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+C = Copy: Ablage (Pinsel) |
| `Befehl \| CSGIntersect \| Commands` | OK | Wirkung CSGIntersect: Karte (zwei Pinsel); Robust CSGIntersect: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+Shift+U = CSGIntersect: Karte (zwei Pinsel) |
| `Befehl \| CSGMerge \| Commands` | OK | Wirkung CSGMerge: Karte (zwei Pinsel nebeneinander); Robust CSGMerge: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| CSGroom \| Commands` | OK | Wirkung CSGroom: Karte (Pinsel); Robust CSGroom: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| CSGSubtract \| Commands` | OK | Wirkung CSGSubtract: Karte (Pinsel B); Robust CSGSubtract: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Shift+U = CSGSubtract: Karte (Pinsel B) |
| `Befehl \| CSGTool \| Commands` | OK | Wirkung CSGTool: Dialog (Pinsel); Robust CSGTool: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| CSGWrapMerge \| Commands` | OK | Wirkung CSGWrapMerge: Karte (zwei Pinsel); Robust CSGWrapMerge: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+U = CSGWrapMerge: Karte (zwei Pinsel) |
| `Befehl \| CubicClipZoomIn \| Commands` | OK | Wirkung CubicClipZoomIn: Einstellung (leer); Robust CubicClipZoomIn: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+[ = CubicClipZoomIn: Einstellung (leer) |
| `Befehl \| CubicClipZoomOut \| Commands` | OK | Wirkung CubicClipZoomOut: Einstellung (leer); Robust CubicClipZoomOut: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+] = CubicClipZoomOut: Einstellung (leer) |
| `Befehl \| DeleteSelection \| Commands` | OK | Wirkung DeleteSelection: Karte+Auswahl (Pinsel); Robust DeleteSelection: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Z = DeleteSelection: Karte+Auswahl (Pinsel) |
| `Befehl \| DeleteSelection2 \| Commands` | OK | Wirkung DeleteSelection2: Karte+Auswahl (Pinsel); Robust DeleteSelection2: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Backspace = DeleteSelection2: Karte+Auswahl (Pinsel) |
| `Befehl \| DeleteSelection3 \| Commands` | OK | Wirkung DeleteSelection3: Karte+Auswahl (Pinsel); Robust DeleteSelection3: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Delete = DeleteSelection3: Karte+Auswahl (Pinsel) |
| `Befehl \| DownFloor \| Commands` | OK | Wirkung DownFloor: Kamera (Pinsel); Robust DownFloor: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste PgDown = DownFloor: Kamera (Pinsel) |
| `Befehl \| DragEdges \| Toggles` | OK | Wirkung DragEdges: Haken+Modus (Pinsel); Robust DragEdges: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste E = DragEdges: Haken+Modus (Pinsel) |
| `Befehl \| DragFaces \| Toggles` | OK | Wirkung DragFaces: Haken+Modus (Pinsel); Robust DragFaces: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste F = DragFaces: Haken+Modus (Pinsel) |
| `Befehl \| DragVertices \| Toggles` | OK | Wirkung DragVertices: Haken+Modus (Pinsel); Robust DragVertices: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste V = DragVertices: Haken+Modus (Pinsel) |
| `Befehl \| EnableAlpha \| Toggles` | OK | Wirkung EnableAlpha: Haken+Einstellung (leer); Robust EnableAlpha: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| EntitiesConnect \| Commands` | OK | Wirkung EntitiesConnect: Karte (zwei Entitaeten); Robust EntitiesConnect: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+K = EntitiesConnect: Karte (zwei Entitaeten) |
| `Befehl \| EntityColorNormalize \| Commands` | OK | Wirkung EntityColorNormalize: Karte (Licht); Robust EntityColorNormalize: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| EntityColorSet \| Commands` | OK | Wirkung EntityColorSet: Dialog (Licht); Robust EntityColorSet: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste K = EntityColorSet: Dialog (Licht) |
| `Befehl \| EntityMovePrimitivesToFirst \| Commands` | OK | Wirkung EntityMovePrimitivesToFirst: Karte (func_door, dann Pinsel); Robust EntityMovePrimitivesToFirst: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| EntityMovePrimitivesToLast \| Commands` | OK | Wirkung EntityMovePrimitivesToLast: Karte (Pinsel, dann func_door); Robust EntityMovePrimitivesToLast: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+M = EntityMovePrimitivesToLast: Karte (Pinsel, dann func_door) |
| `Befehl \| EntityReloadDefinitions \| Commands` | OK | Wirkung EntityReloadDefinitions: Neu geladen (leer); Robust EntityReloadDefinitions: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| EntityUngroup \| Commands` | OK | Wirkung EntityUngroup: Karte (func_group); Robust EntityUngroup: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| EntityUngroupPrimitives \| Commands` | OK | Wirkung EntityUngroupPrimitives: Karte (Pinsel der func_door); Robust EntityUngroupPrimitives: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| Exit \| Commands` | AUSNAHME | beendet das Programm - Stufe 4 prueft ConfirmModified (Save/Discard/Cancel) und das Beenden: siehe Pruefungen "Exit" |
| `Befehl \| ExpandSelectionToEntities \| Commands` | OK | Wirkung ExpandSelectionToEntities: Auswahl (Pinsel der func_group); Robust ExpandSelectionToEntities: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Shift+E = ExpandSelectionToEntities: Auswahl (Pinsel der func_group) |
| `Befehl \| ExpandSelectionToLayers \| Commands` | OK | Wirkung ExpandSelectionToLayers: Auswahl (Pinsel); Robust ExpandSelectionToLayers: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+Q = ExpandSelectionToLayers: Auswahl (Pinsel) |
| `Befehl \| ExpandSelectionToPrimitives \| Commands` | OK | Wirkung ExpandSelectionToPrimitives: Auswahl (Pinsel); Robust ExpandSelectionToPrimitives: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+E = ExpandSelectionToPrimitives: Auswahl (Pinsel) |
| `Befehl \| FilterAreaportals \| Toggles` | OK | Wirkung FilterAreaportals: Filter+Haken+Einstellung (leer); Robust FilterAreaportals: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Alt+3 = FilterAreaportals: Filter+Haken+Einstellung (leer) |
| `Befehl \| FilterBotClips \| Toggles` | OK | Wirkung FilterBotClips: Filter+Haken+Einstellung (leer); Robust FilterBotClips: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Alt+M = FilterBotClips: Filter+Haken+Einstellung (leer) |
| `Befehl \| FilterCaulk \| Toggles` | OK | Wirkung FilterCaulk: Filter+Haken+Einstellung (leer); Robust FilterCaulk: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Alt+6 = FilterCaulk: Filter+Haken+Einstellung (leer) |
| `Befehl \| FilterClips \| Toggles` | OK | Wirkung FilterClips: Filter+Haken+Einstellung (leer); Robust FilterClips: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Alt+7 = FilterClips: Filter+Haken+Einstellung (leer) |
| `Befehl \| FilterClusterportals \| Toggles` | OK | Wirkung FilterClusterportals: Filter+Haken+Einstellung (leer); Robust FilterClusterportals: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Alt+9 = FilterClusterportals: Filter+Haken+Einstellung (leer) |
| `Befehl \| FilterDecals \| Toggles` | OK | Wirkung FilterDecals: Filter+Haken+Einstellung (leer); Robust FilterDecals: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Shift+D = FilterDecals: Filter+Haken+Einstellung (leer) |
| `Befehl \| FilterDetails \| Toggles` | OK | Wirkung FilterDetails: Filter+Haken+Einstellung (leer); Robust FilterDetails: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+D = FilterDetails: Filter+Haken+Einstellung (leer) |
| `Befehl \| FilterEntities \| Toggles` | OK | Wirkung FilterEntities: Filter+Haken+Einstellung (leer); Robust FilterEntities: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Alt+2 = FilterEntities: Filter+Haken+Einstellung (leer) |
| `Befehl \| FilterFuncGroups \| Toggles` | OK | Wirkung FilterFuncGroups: Filter+Haken+Einstellung (leer); Robust FilterFuncGroups: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| FilterHintsSkips \| Toggles` | OK | Wirkung FilterHintsSkips: Filter+Haken+Einstellung (leer); Robust FilterHintsSkips: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+H = FilterHintsSkips: Filter+Haken+Einstellung (leer) |
| `Befehl \| FilterLightgrid \| Toggles` | OK | Wirkung FilterLightgrid: Filter+Haken+Einstellung (leer); Robust FilterLightgrid: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| FilterLights \| Toggles` | OK | Wirkung FilterLights: Filter+Haken+Einstellung (leer); Robust FilterLights: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Alt+0 = FilterLights: Filter+Haken+Einstellung (leer) |
| `Befehl \| FilterLiquids \| Toggles` | OK | Wirkung FilterLiquids: Filter+Haken+Einstellung (leer); Robust FilterLiquids: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Alt+5 = FilterLiquids: Filter+Haken+Einstellung (leer) |
| `Befehl \| FilterModels \| Toggles` | OK | Wirkung FilterModels: Filter+Haken+Einstellung (leer); Robust FilterModels: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Shift+M = FilterModels: Filter+Haken+Einstellung (leer) |
| `Befehl \| FilterNotex \| Toggles` | OK | Wirkung FilterNotex: Haken+Einstellung (leer); Robust FilterNotex: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| FilterPatches \| Toggles` | OK | Wirkung FilterPatches: Filter+Haken+Einstellung (leer); Robust FilterPatches: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+P = FilterPatches: Filter+Haken+Einstellung (leer) |
| `Befehl \| FilterPaths \| Toggles` | OK | Wirkung FilterPaths: Filter+Haken+Einstellung (leer); Robust FilterPaths: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Alt+8 = FilterPaths: Filter+Haken+Einstellung (leer) |
| `Befehl \| FilterPointEntities \| Toggles` | OK | Wirkung FilterPointEntities: Filter+Haken+Einstellung (leer); Robust FilterPointEntities: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| FilterSky \| Toggles` | OK | Wirkung FilterSky: Filter+Haken+Einstellung (leer); Robust FilterSky: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| FilterStructural \| Toggles` | OK | Wirkung FilterStructural: Filter+Haken+Einstellung (leer); Robust FilterStructural: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+Shift+D = FilterStructural: Filter+Haken+Einstellung (leer) |
| `Befehl \| FilterTranslucent \| Toggles` | OK | Wirkung FilterTranslucent: Filter+Haken+Einstellung (leer); Robust FilterTranslucent: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Alt+4 = FilterTranslucent: Filter+Haken+Einstellung (leer) |
| `Befehl \| FilterTriggers \| Toggles` | OK | Wirkung FilterTriggers: Filter+Haken+Einstellung (leer); Robust FilterTriggers: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+Shift+T = FilterTriggers: Filter+Haken+Einstellung (leer) |
| `Befehl \| FilterWorldBrushes \| Toggles` | OK | Wirkung FilterWorldBrushes: Filter+Haken+Einstellung (leer); Robust FilterWorldBrushes: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Alt+1 = FilterWorldBrushes: Filter+Haken+Einstellung (leer) |
| `Befehl \| FindBrush \| Commands` | OK | Wirkung FindBrush: Dialog (leer); Robust FindBrush: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| FindReplaceTextures \| Commands` | OK | Wirkung FindReplaceTextures: Dialog (Texturbrowser offen); Robust FindReplaceTextures: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| FitTexture \| Commands` | OK | Wirkung FitTexture: Karte (Pinsel); Robust FitTexture: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+F = FitTexture: Karte (Pinsel) |
| `Befehl \| FitTextureHeight \| Commands` | OK | Wirkung FitTextureHeight: Karte (Pinsel); Robust FitTextureHeight: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| FitTextureHeightOnly \| Commands` | OK | Wirkung FitTextureHeightOnly: Karte (Pinsel); Robust FitTextureHeightOnly: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| FitTextureWidth \| Commands` | OK | Wirkung FitTextureWidth: Karte (Pinsel); Robust FitTextureWidth: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| FitTextureWidthOnly \| Commands` | OK | Wirkung FitTextureWidthOnly: Karte (Pinsel); Robust FitTextureWidthOnly: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| FixedSize \| Toggles` | OK | Wirkung FixedSize: Haken+Einstellung (leer); Robust FixedSize: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| Fullscreen \| Commands` | OK | Wirkung Fullscreen: Fenster (leer); Robust Fullscreen: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste F11 = Fullscreen: Fenster (leer) |
| `Befehl \| GridDown \| Commands` | OK | Wirkung GridDown: Raster (leer); Robust GridDown: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste [ = GridDown: Raster (leer) |
| `Befehl \| GridUp \| Commands` | OK | Wirkung GridUp: Raster (leer); Robust GridUp: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste ] = GridUp: Raster (leer) |
| `Befehl \| HideSelected \| Toggles` | OK | Wirkung HideSelected: Auswahl+Sicht (Pinsel); Robust HideSelected: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste H = HideSelected: Auswahl+Sicht (Pinsel) |
| `Befehl \| ImportMap \| Commands` | OK | Wirkung ImportMap: Karte (leer); Robust ImportMap: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| InvertCurve \| Commands` | OK | Wirkung InvertCurve: Karte (Patch); Robust InvertCurve: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+I = InvertCurve: Karte (Patch) |
| `Befehl \| InvertCurveTextureX \| Commands` | OK | Wirkung InvertCurveTextureX: Karte (Patch); Robust InvertCurveTextureX: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+Shift+I = InvertCurveTextureX: Karte (Patch) |
| `Befehl \| InvertCurveTextureY \| Commands` | OK | Wirkung InvertCurveTextureY: Karte (Patch); Robust InvertCurveTextureY: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Shift+I = InvertCurveTextureY: Karte (Patch) |
| `Befehl \| InvertFilters \| Commands` | OK | Wirkung InvertFilters: Filter (leer); Robust InvertFilters: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| InvertSelection \| Commands` | OK | Wirkung InvertSelection: Auswahl (Pinsel); Robust InvertSelection: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste I = InvertSelection: Auswahl (Pinsel) |
| `Befehl \| LayersMenu \| Commands` | OK | Wirkung LayersMenu: Dialog (leer); Robust LayersMenu: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Shift+Q = LayersMenu: Dialog (leer) |
| `Befehl \| MakeDetail \| Commands` | OK | Wirkung MakeDetail: Karte (Pinsel); Robust MakeDetail: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Alt+D = MakeDetail: Karte (Pinsel) |
| `Befehl \| MakeStructural \| Commands` | OK | Wirkung MakeStructural: Karte (Detail-Pinsel); Robust MakeStructural: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Alt+S = MakeStructural: Karte (Detail-Pinsel) |
| `Befehl \| MapInfo \| Commands` | OK | Wirkung MapInfo: Dialog (leer); Robust MapInfo: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste M = MapInfo: Dialog (leer) |
| `Befehl \| MatrixTranspose \| Commands` | OK | Wirkung MatrixTranspose: Karte (Patch); Robust MatrixTranspose: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+Shift+M = MatrixTranspose: Karte (Patch) |
| `Befehl \| MaximizeView \| Commands` | OK | Wirkung MaximizeView: Fenster (leer); Robust MaximizeView: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste F12 = MaximizeView: Fenster (leer) |
| `Befehl \| meshTex::About \| Commands` | OK | Wirkung meshTex::About: Dialog (leer); Robust meshTex::About: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| meshTex::GeneralFunction \| Commands` | OK | Wirkung meshTex::GeneralFunction: Dialog (Patch); Robust meshTex::GeneralFunction: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| meshTex::GetInfo \| Commands` | OK | Wirkung meshTex::GetInfo: Dialog (Patch); Robust meshTex::GetInfo: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| meshTex::Help \| Commands` | OK | Wirkung meshTex::Help: Dialog (leer); Robust meshTex::Help: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| meshTex::SetScale \| Commands` | OK | Wirkung meshTex::SetScale: Dialog (Patch); Robust meshTex::SetScale: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| meshTex::SMaxAlign \| Commands` | OK | Wirkung meshTex::SMaxAlign: Karte (Patch, Textur versetzt); Robust meshTex::SMaxAlign: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| meshTex::SMinAlign \| Commands` | OK | Wirkung meshTex::SMinAlign: Karte (Patch, Textur versetzt); Robust meshTex::SMinAlign: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| meshTex::SMinMaxAlignAuto \| Commands` | OK | Wirkung meshTex::SMinMaxAlignAuto: Karte (Patch, Textur versetzt); Robust meshTex::SMinMaxAlignAuto: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| meshTex::SMinMaxAlignShrink \| Commands` | OK | Wirkung meshTex::SMinMaxAlignShrink: Karte (Patch, Textur versetzt); Robust meshTex::SMinMaxAlignShrink: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| meshTex::SMinMaxAlignStretch \| Commands` | OK | Wirkung meshTex::SMinMaxAlignStretch: Karte (Patch, Textur versetzt); Robust meshTex::SMinMaxAlignStretch: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| meshTex::STMinMaxAlignAuto \| Commands` | OK | Wirkung meshTex::STMinMaxAlignAuto: Karte (Patch, Textur versetzt); Robust meshTex::STMinMaxAlignAuto: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| meshTex::TMaxAlign \| Commands` | OK | Wirkung meshTex::TMaxAlign: Karte (Patch, Textur versetzt); Robust meshTex::TMaxAlign: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| meshTex::TMinAlign \| Commands` | OK | Wirkung meshTex::TMinAlign: Karte (Patch, Textur versetzt); Robust meshTex::TMinAlign: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| meshTex::TMinMaxAlignAuto \| Commands` | OK | Wirkung meshTex::TMinMaxAlignAuto: Karte (Patch, Textur versetzt); Robust meshTex::TMinMaxAlignAuto: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| meshTex::TMinMaxAlignShrink \| Commands` | OK | Wirkung meshTex::TMinMaxAlignShrink: Karte (Patch, Textur versetzt); Robust meshTex::TMinMaxAlignShrink: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| meshTex::TMinMaxAlignStretch \| Commands` | OK | Wirkung meshTex::TMinMaxAlignStretch: Karte (Patch, Textur versetzt); Robust meshTex::TMinMaxAlignStretch: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| MirrorSelectionHorizontally \| Commands` | OK | Wirkung MirrorSelectionHorizontally: Karte (Pinsel); Robust MirrorSelectionHorizontally: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| MirrorSelectionVertically \| Commands` | OK | Wirkung MirrorSelectionVertically: Karte (Pinsel); Robust MirrorSelectionVertically: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| MirrorSelectionX \| Commands` | OK | Wirkung MirrorSelectionX: Karte (Pinsel); Robust MirrorSelectionX: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| MirrorSelectionY \| Commands` | OK | Wirkung MirrorSelectionY: Karte (Pinsel); Robust MirrorSelectionY: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| MirrorSelectionZ \| Commands` | OK | Wirkung MirrorSelectionZ: Karte (Pinsel); Robust MirrorSelectionZ: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| MouseBuild \| Toggles` | OK | Wirkung MouseBuild: Haken+Modus (Pinsel); Robust MouseBuild: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste B = MouseBuild: Haken+Modus (Pinsel) |
| `Befehl \| MouseDrag \| Toggles` | OK | Wirkung MouseDrag: Haken+Modus (Werkzeug Translate); Robust MouseDrag: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| MouseDragOrTransform \| Commands` | OK | Wirkung MouseDragOrTransform: Modus (Pinsel); Robust MouseDragOrTransform: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Q = MouseDragOrTransform: Modus (Pinsel) |
| `Befehl \| MouseRotate \| Toggles` | OK | Wirkung MouseRotate: Haken+Modus (Pinsel); Robust MouseRotate: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste R = MouseRotate: Haken+Modus (Pinsel) |
| `Befehl \| MouseRotateOrScale \| Commands` | OK | Wirkung MouseRotateOrScale: Modus (Pinsel); Robust MouseRotateOrScale: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| MouseScale \| Toggles` | OK | Wirkung MouseScale: Haken+Modus (Pinsel); Robust MouseScale: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| MouseTransform \| Toggles` | OK | Wirkung MouseTransform: Haken+Modus (Pinsel); Robust MouseTransform: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| MouseTranslate \| Toggles` | OK | Wirkung MouseTranslate: Haken+Modus (Pinsel); Robust MouseTranslate: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste W = MouseTranslate: Haken+Modus (Pinsel) |
| `Befehl \| MouseUV \| Toggles` | OK | Wirkung MouseUV: Haken+Modus (Pinsel); Robust MouseUV: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste G = MouseUV: Haken+Modus (Pinsel) |
| `Befehl \| MoveSelectionDOWN \| Commands` | OK | Wirkung MoveSelectionDOWN: Karte (Pinsel); Robust MoveSelectionDOWN: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Num+- = MoveSelectionDOWN: Karte (Pinsel) |
| `Befehl \| MoveSelectionUP \| Commands` | OK | Wirkung MoveSelectionUP: Karte (Pinsel); Robust MoveSelectionUP: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Num++ = MoveSelectionUP: Karte (Pinsel) |
| `Befehl \| MoveToCamera \| Commands` | OK | Wirkung MoveToCamera: Karte (Pinsel); Robust MoveToCamera: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+Shift+V = MoveToCamera: Karte (Pinsel) |
| `Befehl \| NaturalizePatch \| Commands` | OK | Wirkung NaturalizePatch: Karte (Patch); Robust NaturalizePatch: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+N = NaturalizePatch: Karte (Patch) |
| `Befehl \| NewMap \| Commands` | OK | Wirkung NewMap: Karte (Pinsel); Robust NewMap: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| NextLeakSpot \| Commands` | OK | Wirkung NextLeakSpot: 2D-Ansicht+Kamera+Leck/Portale (Leck-Linie geladen); Robust NextLeakSpot: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+Shift+K = NextLeakSpot: 2D-Ansicht+Kamera+Leck/Portale (Leck-Linie geladen) |
| `Befehl \| NextView \| Commands` | OK | Wirkung NextView: 2D-Ansicht (leer); Robust NextView: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+Tab = NextView: 2D-Ansicht (leer) |
| `Befehl \| OpenGLFont \| Commands` | OK | Wirkung OpenGLFont: Dialog (leer); Robust OpenGLFont: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| OpenManual \| Commands` | OK | Wirkung OpenManual: Extern (leer); Robust OpenManual: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste F1 = OpenManual: Extern (leer) |
| `Befehl \| OpenMap \| Commands` | OK | Wirkung OpenMap: Karte (leer); Robust OpenMap: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+O = OpenMap: Karte (leer) |
| `Befehl \| Paste \| Commands` | OK | Wirkung Paste: Karte+Auswahl (Pinsel kopiert); Robust Paste: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+V = Paste: Karte+Auswahl (Pinsel kopiert) |
| `Befehl \| PasteToCamera \| Commands` | OK | Wirkung PasteToCamera: Karte+Auswahl (Pinsel kopiert); Robust PasteToCamera: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Shift+V = PasteToCamera: Karte+Auswahl (Pinsel kopiert) |
| `Befehl \| PatchBevel \| Commands` | OK | Wirkung PatchBevel: Karte+Auswahl (Pinsel); Robust PatchBevel: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| PatchCone \| Commands` | OK | Wirkung PatchCone: Karte+Auswahl (Pinsel); Robust PatchCone: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| PatchCylinder \| Commands` | OK | Wirkung PatchCylinder: Karte+Auswahl (Pinsel); Robust PatchCylinder: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| PatchDeform \| Commands` | OK | Wirkung PatchDeform: Dialog (Patch); Robust PatchDeform: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| PatchDeleteFirstColumn \| Commands` | OK | Wirkung PatchDeleteFirstColumn: Karte (Patch 5x5); Robust PatchDeleteFirstColumn: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| PatchDeleteFirstRow \| Commands` | OK | Wirkung PatchDeleteFirstRow: Karte (Patch 5x5); Robust PatchDeleteFirstRow: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| PatchDeleteLastColumn \| Commands` | OK | Wirkung PatchDeleteLastColumn: Karte (Patch 5x5); Robust PatchDeleteLastColumn: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+Shift+Num+- = PatchDeleteLastColumn: Karte (Patch 5x5) |
| `Befehl \| PatchDeleteLastRow \| Commands` | OK | Wirkung PatchDeleteLastRow: Karte (Patch 5x5); Robust PatchDeleteLastRow: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+Num+- = PatchDeleteLastRow: Karte (Patch 5x5) |
| `Befehl \| PatchEndCap \| Commands` | OK | Wirkung PatchEndCap: Karte+Auswahl (Pinsel); Robust PatchEndCap: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| PatchInsertFirstColumn \| Commands` | OK | Wirkung PatchInsertFirstColumn: Karte (Patch); Robust PatchInsertFirstColumn: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+Shift+Num++ = PatchInsertFirstColumn: Karte (Patch) |
| `Befehl \| PatchInsertFirstRow \| Commands` | OK | Wirkung PatchInsertFirstRow: Karte (Patch); Robust PatchInsertFirstRow: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+Num++ = PatchInsertFirstRow: Karte (Patch) |
| `Befehl \| PatchInsertLastColumn \| Commands` | OK | Wirkung PatchInsertLastColumn: Karte (Patch); Robust PatchInsertLastColumn: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| PatchInsertLastRow \| Commands` | OK | Wirkung PatchInsertLastRow: Karte (Patch); Robust PatchInsertLastRow: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| PatchSphere \| Commands` | OK | Wirkung PatchSphere: Karte+Auswahl (Pinsel); Robust PatchSphere: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| PatchSquareCylinder \| Commands` | OK | Wirkung PatchSquareCylinder: Karte+Auswahl (Pinsel); Robust PatchSquareCylinder: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| PatchThicken \| Commands` | OK | Wirkung PatchThicken: Dialog (Patch); Robust PatchThicken: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+T = PatchThicken: Dialog (Patch) |
| `Befehl \| PatchXactCone \| Commands` | OK | Wirkung PatchXactCone: Dialog (Pinsel); Robust PatchXactCone: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| PatchXactCylinder \| Commands` | OK | Wirkung PatchXactCylinder: Dialog (Pinsel); Robust PatchXactCylinder: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| PatchXactSphere \| Commands` | OK | Wirkung PatchXactSphere: Dialog (Pinsel); Robust PatchXactSphere: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| Preferences \| Commands` | OK | Wirkung Preferences: Dialog (leer); Robust Preferences: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste P = Preferences: Dialog (leer) |
| `Befehl \| PrevLeakSpot \| Commands` | OK | Wirkung PrevLeakSpot: 2D-Ansicht+Kamera+Leck/Portale (Leck-Linie, ein Punkt weiter); Robust PrevLeakSpot: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+Shift+L = PrevLeakSpot: 2D-Ansicht+Kamera+Leck/Portale (Leck-Linie, ein Punkt weiter) |
| `Befehl \| prtview::AboutPortalViewer \| Commands` | OK | Wirkung prtview::AboutPortalViewer: Dialog (leer); Robust prtview::AboutPortalViewer: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| prtview::ConfigurePortalViewer \| Commands` | OK | Wirkung prtview::ConfigurePortalViewer: Dialog (leer); Robust prtview::ConfigurePortalViewer: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| prtview::Load.prtFile \| Commands` | OK | Wirkung prtview::Load.prtFile: Dialog (leer); Robust prtview::Load.prtFile: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| prtview::TogglePortals(2D) \| Commands` | OK | Wirkung prtview::TogglePortals(2D): Leck/Portale (leer); Robust prtview::TogglePortals(2D): leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| prtview::TogglePortals(3D) \| Commands` | OK | Wirkung prtview::TogglePortals(3D): Leck/Portale (leer); Robust prtview::TogglePortals(3D): leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| prtview::Unload.prtFile \| Commands` | OK | Wirkung prtview::Unload.prtFile: Leck/Portale (Portale geladen); Robust prtview::Unload.prtFile: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| RedisperseCols \| Commands` | OK | Wirkung RedisperseCols: Karte (Patch 5x5); Robust RedisperseCols: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| RedisperseRows \| Commands` | OK | Wirkung RedisperseRows: Karte (Patch 5x5); Robust RedisperseRows: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| Redo \| Commands` | OK | Wirkung Redo: Karte (nach Rueckgaengig); Robust Redo: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+Shift+Z = Redo: Karte (nach Rueckgaengig) |
| `Befehl \| Redo2 \| Commands` | OK | Wirkung Redo2: Karte (nach Rueckgaengig); Robust Redo2: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+Y = Redo2: Karte (nach Rueckgaengig) |
| `Befehl \| RefreshReferences \| Commands` | OK | Wirkung RefreshReferences: Neu geladen (leer); Robust RefreshReferences: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| RefreshShaders \| Commands` | OK | Wirkung RefreshShaders: Neu geladen (Texturbrowser offen); Robust RefreshShaders: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| RegionOff \| Commands` | OK | Wirkung RegionOff: Region (Region an); Robust RegionOff: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| RegionSetBrush \| Commands` | OK | Wirkung RegionSetBrush: Karte+Region (Pinsel); Robust RegionSetBrush: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| RegionSetSelection \| Toggles` | OK | Wirkung RegionSetSelection: Region (Pinsel); Robust RegionSetSelection: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+Shift+R = RegionSetSelection: Region (Pinsel) |
| `Befehl \| RegionSetXY \| Commands` | OK | Wirkung RegionSetXY: Region (leer); Robust RegionSetXY: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| RepeatTransforms \| Commands` | OK | Wirkung RepeatTransforms: Karte+Auswahl (Pinsel gedreht); Robust RepeatTransforms: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+R = RepeatTransforms: Karte+Auswahl (Pinsel gedreht) |
| `Befehl \| ResetFilters \| Commands` | OK | Wirkung ResetFilters: Filter (Filter an); Robust ResetFilters: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| ResetTransforms \| Commands` | OK | Wirkung ResetTransforms: Modus (Pinsel gedreht); Robust ResetTransforms: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Alt+R = ResetTransforms: Modus (Pinsel gedreht) |
| `Befehl \| RotateSelectionAnticlockwise \| Commands` | OK | Wirkung RotateSelectionAnticlockwise: Karte (Pinsel); Robust RotateSelectionAnticlockwise: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| RotateSelectionClockwise \| Commands` | OK | Wirkung RotateSelectionClockwise: Karte (Pinsel); Robust RotateSelectionClockwise: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| RotateSelectionX \| Commands` | OK | Wirkung RotateSelectionX: Karte (Pinsel); Robust RotateSelectionX: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| RotateSelectionY \| Commands` | OK | Wirkung RotateSelectionY: Karte (Pinsel); Robust RotateSelectionY: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| RotateSelectionZ \| Commands` | OK | Wirkung RotateSelectionZ: Karte (Pinsel); Robust RotateSelectionZ: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| SaveMap \| Commands` | OK | Wirkung SaveMap: Datei (Karte geaendert); Robust SaveMap: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+S = SaveMap: Datei (Karte geaendert) |
| `Befehl \| SaveMapAs \| Commands` | OK | Wirkung SaveMapAs: Datei (Pinsel); Robust SaveMapAs: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| SaveRegion \| Commands` | OK | Wirkung SaveRegion: Datei (Pinsel); Robust SaveRegion: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| SaveSelected \| Commands` | OK | Wirkung SaveSelected: Datei (Pinsel); Robust SaveSelected: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| SearchFromStart \| Toggles` | OK | Wirkung SearchFromStart: Haken+Einstellung (leer); Robust SearchFromStart: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| SelectAllOfType \| Commands` | OK | Wirkung SelectAllOfType: Auswahl (Licht); Robust SelectAllOfType: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Shift+A = SelectAllOfType: Auswahl (Licht) |
| `Befehl \| SelectConnectedEntities \| Commands` | OK | Wirkung SelectConnectedEntities: Auswahl (Licht); Robust SelectConnectedEntities: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+Shift+E = SelectConnectedEntities: Auswahl (Licht) |
| `Befehl \| SelectInside \| Commands` | OK | Wirkung SelectInside: Karte+Auswahl (Pinsel); Robust SelectInside: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| SelectNudgeDown \| Commands` | OK | Wirkung SelectNudgeDown: Karte (Pinsel); Robust SelectNudgeDown: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Alt+Down = SelectNudgeDown: Karte (Pinsel) |
| `Befehl \| SelectNudgeLeft \| Commands` | OK | Wirkung SelectNudgeLeft: Karte (Pinsel); Robust SelectNudgeLeft: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Alt+Left = SelectNudgeLeft: Karte (Pinsel) |
| `Befehl \| SelectNudgeRight \| Commands` | OK | Wirkung SelectNudgeRight: Karte (Pinsel); Robust SelectNudgeRight: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Alt+Right = SelectNudgeRight: Karte (Pinsel) |
| `Befehl \| SelectNudgeUp \| Commands` | OK | Wirkung SelectNudgeUp: Karte (Pinsel); Robust SelectNudgeUp: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Alt+Up = SelectNudgeUp: Karte (Pinsel) |
| `Befehl \| SelectTextured \| Commands` | OK | Wirkung SelectTextured: Auswahl (Textur gewaehlt); Robust SelectTextured: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+Shift+A = SelectTextured: Auswahl (Textur gewaehlt) |
| `Befehl \| SelectTouching \| Commands` | OK | Wirkung SelectTouching: Auswahl (Pinsel); Robust SelectTouching: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| SetGrid0.125 \| Commands` | OK | Wirkung SetGrid0.125: Raster+Haken (leer); Robust SetGrid0.125: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| SetGrid0.25 \| Commands` | OK | Wirkung SetGrid0.25: Raster+Haken (leer); Robust SetGrid0.25: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| SetGrid0.5 \| Commands` | OK | Wirkung SetGrid0.5: Raster+Haken (leer); Robust SetGrid0.5: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| SetGrid1 \| Commands` | OK | Wirkung SetGrid1: Raster+Haken (leer); Robust SetGrid1: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste 1 = SetGrid1: Raster+Haken (leer) |
| `Befehl \| SetGrid1024 \| Commands` | OK | Wirkung SetGrid1024: Raster+Haken (leer); Robust SetGrid1024: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| SetGrid128 \| Commands` | OK | Wirkung SetGrid128: Raster+Haken (leer); Robust SetGrid128: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste 8 = SetGrid128: Raster+Haken (leer) |
| `Befehl \| SetGrid16 \| Commands` | OK | Wirkung SetGrid16: Raster+Haken (leer); Robust SetGrid16: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste 5 = SetGrid16: Raster+Haken (leer) |
| `Befehl \| SetGrid2 \| Commands` | OK | Wirkung SetGrid2: Raster+Haken (leer); Robust SetGrid2: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste 2 = SetGrid2: Raster+Haken (leer) |
| `Befehl \| SetGrid256 \| Commands` | OK | Wirkung SetGrid256: Raster+Haken (leer); Robust SetGrid256: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste 9 = SetGrid256: Raster+Haken (leer) |
| `Befehl \| SetGrid32 \| Commands` | OK | Wirkung SetGrid32: Raster+Haken (leer); Robust SetGrid32: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste 6 = SetGrid32: Raster+Haken (leer) |
| `Befehl \| SetGrid4 \| Commands` | OK | Wirkung SetGrid4: Raster+Haken (leer); Robust SetGrid4: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste 3 = SetGrid4: Raster 4; (+1) |
| `Befehl \| SetGrid512 \| Commands` | OK | Wirkung SetGrid512: Raster+Haken (leer); Robust SetGrid512: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| SetGrid64 \| Commands` | OK | Wirkung SetGrid64: Raster+Haken (leer); Robust SetGrid64: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste 7 = SetGrid64: Raster+Haken (leer) |
| `Befehl \| SetGrid8 \| Commands` | OK | Wirkung SetGrid8: Raster+Haken (Raster 16); Robust SetGrid8: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste 4 = SetGrid8: Raster+Haken (Raster 16) |
| `Befehl \| shaderPlug::About \| Commands` | OK | Wirkung shaderPlug::About: Dialog (leer); Robust shaderPlug::About: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| shaderPlug::CreateTagFile \| Commands` | OK | Wirkung shaderPlug::CreateTagFile: Datei (ohne Tag-Datei); Robust shaderPlug::CreateTagFile: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| Shortcuts \| Commands` | OK | Wirkung Shortcuts: Dialog (leer); Robust Shortcuts: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+Shift+P = Shortcuts: Dialog (leer) |
| `Befehl \| ShowAllTextures \| Commands` | OK | Wirkung ShowAllTextures: Texturen (leer); Robust ShowAllTextures: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+A = ShowAllTextures: Texturen (leer) |
| `Befehl \| ShowAngles \| Toggles` | OK | Wirkung ShowAngles: Haken+Anzeige (leer); Robust ShowAngles: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| ShowAxes \| Toggles` | OK | Wirkung ShowAxes: Haken+Anzeige (leer); Robust ShowAxes: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| ShowBboxes \| Toggles` | OK | Wirkung ShowBboxes: Haken+Anzeige (leer); Robust ShowBboxes: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| ShowBlocks \| Toggles` | OK | Wirkung ShowBlocks: Haken+Anzeige (leer); Robust ShowBlocks: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| ShowConnections \| Toggles` | OK | Wirkung ShowConnections: Haken+Anzeige (leer); Robust ShowConnections: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| ShowCoordinates \| Toggles` | OK | Wirkung ShowCoordinates: Haken+Anzeige (leer); Robust ShowCoordinates: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| ShowHidden \| Commands` | OK | Wirkung ShowHidden: Sicht (Pinsel versteckt); Robust ShowHidden: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Shift+H = ShowHidden: Sicht (Pinsel versteckt) |
| `Befehl \| ShowInUse \| Toggles` | OK | Wirkung ShowInUse: Haken+Texturen (leer); Robust ShowInUse: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste U = ShowInUse: Haken+Texturen (leer) |
| `Befehl \| ShowLightRadiuses \| Toggles` | OK | Wirkung ShowLightRadiuses: Haken+Anzeige (leer); Robust ShowLightRadiuses: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| ShowNames \| Toggles` | OK | Wirkung ShowNames: Haken+Anzeige (leer); Robust ShowNames: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| ShowSize2d \| Toggles` | OK | Wirkung ShowSize2d: Haken+Anzeige (leer); Robust ShowSize2d: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste J = ShowSize2d: Haken+Anzeige (leer) |
| `Befehl \| ShowSize3d \| Toggles` | OK | Wirkung ShowSize3d: Haken+Einstellung (leer); Robust ShowSize3d: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| ShowStats \| Toggles` | OK | Wirkung ShowStats: Haken+Einstellung (leer); Robust ShowStats: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| ShowWindowOutline \| Toggles` | OK | Wirkung ShowWindowOutline: Haken+Anzeige (leer); Robust ShowWindowOutline: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| ShowWorkzone2d \| Toggles` | OK | Wirkung ShowWorkzone2d: Haken+Anzeige (leer); Robust ShowWorkzone2d: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| ShowWorkzone3d \| Toggles` | OK | Wirkung ShowWorkzone3d: Haken+Einstellung (leer); Robust ShowWorkzone3d: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| SimplePatchMesh \| Commands` | OK | Wirkung SimplePatchMesh: Dialog (leer); Robust SimplePatchMesh: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Shift+P = SimplePatchMesh: Dialog (leer) |
| `Befehl \| SmoothCols \| Commands` | OK | Wirkung SmoothCols: Karte (Patch 5x5); Robust SmoothCols: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+Shift+W = SmoothCols: Karte (Patch 5x5) |
| `Befehl \| SmoothRows \| Commands` | OK | Wirkung SmoothRows: Karte (Patch 5x5); Robust SmoothRows: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+W = SmoothRows: Karte (Patch 5x5) |
| `Befehl \| SnapToGrid \| Commands` | OK | Wirkung SnapToGrid: Karte (Pinsel neben dem Raster); Robust SnapToGrid: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+G = SnapToGrid: Karte (Pinsel neben dem Raster) |
| `Befehl \| sunPlug::About \| Commands` | OK | Wirkung sunPlug::About: Dialog (leer); Robust sunPlug::About: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| sunPlug::ET-MapCoordinator \| Commands` | OK | Wirkung sunPlug::ET-MapCoordinator: Dialog (leer); Robust sunPlug::ET-MapCoordinator: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| SurfaceInspector \| Commands` | OK | Wirkung SurfaceInspector: Dialog (leer); Robust SurfaceInspector: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste S = SurfaceInspector: Dialog (leer) |
| `Befehl \| TagAdd \| Commands` | OK | Wirkung TagAdd: Texturen (Tag markiert); Robust TagAdd: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| TagCopy \| Commands` | OK | Wirkung TagCopy: Texturen (Tag markiert); Robust TagCopy: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| TagDelete \| Commands` | OK | Wirkung TagDelete: Dialog (Tag markiert); Robust TagDelete: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| TagPaste \| Commands` | OK | Wirkung TagPaste: Texturen+Datei (Tag kopiert); Robust TagPaste: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| TagRename \| Commands` | OK | Wirkung TagRename: Texturen (Tag markiert); Robust TagRename: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| TagSearch \| Commands` | OK | Wirkung TagSearch: Texturen (Tag markiert); Robust TagSearch: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| TagSearchUntagged \| Commands` | OK | Wirkung TagSearchUntagged: Dialog (leer); Robust TagSearchUntagged: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| TagsToggleGui \| Toggles` | OK | Wirkung TagsToggleGui: Haken+Einstellung (leer); Robust TagsToggleGui: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| terrainGenerator::About \| Commands` | OK | Wirkung terrainGenerator::About: Dialog (leer); Robust terrainGenerator::About: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| terrainGenerator::GenerateTerrain \| Commands` | OK | Wirkung terrainGenerator::GenerateTerrain: Dialog (Pinsel); Robust terrainGenerator::GenerateTerrain: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| TexRotateClock \| Commands` | OK | Wirkung TexRotateClock: Karte (Pinsel); Robust TexRotateClock: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Shift+PgDown = TexRotateClock: Karte (Pinsel) |
| `Befehl \| TexRotateCounter \| Commands` | OK | Wirkung TexRotateCounter: Karte (Pinsel); Robust TexRotateCounter: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Shift+PgUp = TexRotateCounter: Karte (Pinsel) |
| `Befehl \| TexScaleDown \| Commands` | OK | Wirkung TexScaleDown: Karte (Pinsel); Robust TexScaleDown: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+Down = TexScaleDown: Karte (Pinsel) |
| `Befehl \| TexScaleLeft \| Commands` | OK | Wirkung TexScaleLeft: Karte (Pinsel); Robust TexScaleLeft: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+Left = TexScaleLeft: Karte (Pinsel) |
| `Befehl \| TexScaleRight \| Commands` | OK | Wirkung TexScaleRight: Karte (Pinsel); Robust TexScaleRight: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+Right = TexScaleRight: Karte (Pinsel) |
| `Befehl \| TexScaleUp \| Commands` | OK | Wirkung TexScaleUp: Karte (Pinsel); Robust TexScaleUp: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+Up = TexScaleUp: Karte (Pinsel) |
| `Befehl \| TexShiftDown \| Commands` | OK | Wirkung TexShiftDown: Karte (Pinsel); Robust TexShiftDown: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Shift+Down = TexShiftDown: Karte (Pinsel) |
| `Befehl \| TexShiftLeft \| Commands` | OK | Wirkung TexShiftLeft: Karte (Pinsel); Robust TexShiftLeft: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Shift+Left = TexShiftLeft: Karte (Pinsel) |
| `Befehl \| TexShiftRight \| Commands` | OK | Wirkung TexShiftRight: Karte (Pinsel); Robust TexShiftRight: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Shift+Right = TexShiftRight: Karte (Pinsel) |
| `Befehl \| TexShiftUp \| Commands` | OK | Wirkung TexShiftUp: Karte (Pinsel); Robust TexShiftUp: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Shift+Up = TexShiftUp: Karte (Pinsel) |
| `Befehl \| TextureProjectAxial \| Commands` | OK | Wirkung TextureProjectAxial: Karte (Patch); Robust TextureProjectAxial: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| TextureProjectCam \| Commands` | OK | Wirkung TextureProjectCam: Karte (Patch); Robust TextureProjectCam: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| TextureProjectOrtho \| Commands` | OK | Wirkung TextureProjectOrtho: Karte (Patch); Robust TextureProjectOrtho: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| TextureReset/Cap \| Commands` | OK | Wirkung TextureReset/Cap: Karte (Pinsel, Textur versetzt); Robust TextureReset/Cap: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Shift+N = TextureReset/Cap: Karte (Pinsel, Textur versetzt) |
| `Befehl \| ToggleCamera \| Toggles` | OK | Wirkung ToggleCamera: Haken+Fenster (leer); Robust ToggleCamera: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+Shift+C = ToggleCamera: Haken+Fenster (leer) |
| `Befehl \| ToggleClipper \| Toggles` | OK | Wirkung ToggleClipper: Haken+Modus (Pinsel); Robust ToggleClipper: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste X = ToggleClipper: Haken+Modus (Pinsel) |
| `Befehl \| ToggleConsole \| Commands` | OK | Wirkung ToggleConsole: Fenster (leer); Robust ToggleConsole: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste O = ToggleConsole: Fenster (leer) |
| `Befehl \| ToggleCrosshairs \| Toggles` | OK | Wirkung ToggleCrosshairs: Haken+Anzeige (leer); Robust ToggleCrosshairs: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Shift+X = ToggleCrosshairs: Haken+Anzeige (leer) |
| `Befehl \| ToggleCubicClip \| Toggles` | OK | Wirkung ToggleCubicClip: Haken+Einstellung (leer); Robust ToggleCubicClip: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+\ = ToggleCubicClip: Haken+Einstellung (leer) |
| `Befehl \| ToggleEntityInspector \| Commands` | OK | Wirkung ToggleEntityInspector: Fenster (leer); Robust ToggleEntityInspector: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste N = ToggleEntityInspector: Fenster (leer) |
| `Befehl \| ToggleEntityList \| Commands` | OK | Wirkung ToggleEntityList: Dialog (leer); Robust ToggleEntityList: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Shift+L = ToggleEntityList: Dialog (leer) |
| `Befehl \| ToggleFrontView \| Toggles` | OK | Wirkung ToggleFrontView: Haken+Fenster (leer); Robust ToggleFrontView: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| ToggleGrid \| Toggles` | OK | Wirkung ToggleGrid: Haken+Anzeige (leer); Robust ToggleGrid: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste 0 = ToggleGrid: Haken+Anzeige (leer) |
| `Befehl \| ToggleGridSnap \| Commands` | OK | Wirkung ToggleGridSnap: Raster (leer); Robust ToggleGridSnap: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| ToggleLayersBrowser \| Commands` | OK | Wirkung ToggleLayersBrowser: Fenster (leer); Robust ToggleLayersBrowser: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste L = ToggleLayersBrowser: Fenster (leer) |
| `Befehl \| ToggleModelBrowser \| Commands` | OK | Wirkung ToggleModelBrowser: Fenster (leer); Robust ToggleModelBrowser: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste / = ToggleModelBrowser: Fenster (leer) |
| `Befehl \| TogglePointfile \| Commands` | OK | Wirkung TogglePointfile: Leck/Portale (leer); Robust TogglePointfile: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| ToggleShowShaderlistOnly \| Toggles` | OK | Wirkung ToggleShowShaderlistOnly: Haken+Einstellung (leer); Robust ToggleShowShaderlistOnly: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| ToggleShowShaders \| Toggles` | OK | Wirkung ToggleShowShaders: Haken+Einstellung (leer); Robust ToggleShowShaders: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| ToggleShowTextures \| Toggles` | OK | Wirkung ToggleShowTextures: Haken+Einstellung (leer); Robust ToggleShowTextures: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| ToggleSideView \| Toggles` | OK | Wirkung ToggleSideView: Haken+Fenster (leer); Robust ToggleSideView: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| ToggleTextures \| Commands` | OK | Wirkung ToggleTextures: Fenster (leer); Robust ToggleTextures: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste T = ToggleTextures: Fenster (leer) |
| `Befehl \| ToggleView \| Toggles` | OK | Wirkung ToggleView: Haken+Fenster (leer); Robust ToggleView: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| TogTexLock \| Toggles` | OK | Wirkung TogTexLock: Haken+Modus (leer); Robust TogTexLock: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Shift+T = TogTexLock: Haken+Modus (leer) |
| `Befehl \| TogTexVertexLock \| Toggles` | OK | Wirkung TogTexVertexLock: Haken+Modus (leer); Robust TogTexVertexLock: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| uFO:AI::About \| Commands` | OK | Wirkung uFO:AI::About: Dialog (leer); Robust uFO:AI::About: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| uFO:AI::ActorClip \| Commands` | OK | Wirkung uFO:AI::ActorClip: Filter+Sicht (leer); Robust uFO:AI::ActorClip: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| uFO:AI::Level1 \| Commands` | OK | Wirkung uFO:AI::Level1: Filter+Sicht (leer); Robust uFO:AI::Level1: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| uFO:AI::Level2 \| Commands` | OK | Wirkung uFO:AI::Level2: Filter+Sicht (leer); Robust uFO:AI::Level2: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| uFO:AI::Level3 \| Commands` | OK | Wirkung uFO:AI::Level3: Filter+Sicht (leer); Robust uFO:AI::Level3: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| uFO:AI::Level4 \| Commands` | OK | Wirkung uFO:AI::Level4: Filter+Sicht (leer); Robust uFO:AI::Level4: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| uFO:AI::Level5 \| Commands` | OK | Wirkung uFO:AI::Level5: Filter+Sicht (leer); Robust uFO:AI::Level5: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| uFO:AI::Level6 \| Commands` | OK | Wirkung uFO:AI::Level6: Filter+Sicht (leer); Robust uFO:AI::Level6: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| uFO:AI::Level7 \| Commands` | OK | Wirkung uFO:AI::Level7: Filter+Sicht (leer); Robust uFO:AI::Level7: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| uFO:AI::Level8 \| Commands` | OK | Wirkung uFO:AI::Level8: Filter+Sicht (leer); Robust uFO:AI::Level8: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| uFO:AI::NoDraw \| Commands` | OK | Wirkung uFO:AI::NoDraw: Filter+Sicht (leer); Robust uFO:AI::NoDraw: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| uFO:AI::PerformCheck \| Commands` | OK | Wirkung uFO:AI::PerformCheck: Dialog (Pinsel); Robust uFO:AI::PerformCheck: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| uFO:AI::StepOn \| Commands` | OK | Wirkung uFO:AI::StepOn: Filter+Sicht (leer); Robust uFO:AI::StepOn: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| uFO:AI::WeaponClip \| Commands` | OK | Wirkung uFO:AI::WeaponClip: Filter+Sicht (leer); Robust uFO:AI::WeaponClip: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| uFO:AI::Worldspawn \| Commands` | OK | Wirkung uFO:AI::Worldspawn: Karte (Pinsel); Robust uFO:AI::Worldspawn: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| uFO:AI::WorldspawnReset \| Commands` | OK | Wirkung uFO:AI::WorldspawnReset: Karte (Pinsel); Robust uFO:AI::WorldspawnReset: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| Undo \| Commands` | OK | Wirkung Undo: Karte (nach einer Aenderung); Robust Undo: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Ctrl+Z = Undo: Karte (nach einer Aenderung) |
| `Befehl \| UnSelectSelection \| Commands` | OK | Wirkung UnSelectSelection: Auswahl (Pinsel); Robust UnSelectSelection: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste C = UnSelectSelection: Auswahl (Pinsel) |
| `Befehl \| UnSelectSelection2 \| Commands` | OK | Wirkung UnSelectSelection2: Auswahl (Pinsel); Robust UnSelectSelection2: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Escape = UnSelectSelection2: Auswahl (Pinsel) |
| `Befehl \| UpFloor \| Commands` | OK | Wirkung UpFloor: Kamera (Kamera unten); Robust UpFloor: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste PgUp = UpFloor: Kamera (Kamera unten) |
| `Befehl \| ViewFront \| Commands` | OK | Wirkung ViewFront: 2D-Ansicht (leer); Robust ViewFront: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Num+1 = ViewFront: 2D-Ansicht (leer) |
| `Befehl \| ViewSide \| Commands` | OK | Wirkung ViewSide: 2D-Ansicht (leer); Robust ViewSide: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Num+3 = ViewSide: 2D-Ansicht (leer) |
| `Befehl \| ViewTop \| Commands` | OK | Wirkung ViewTop: 2D-Ansicht (leer); Robust ViewTop: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Num+7 = ViewTop: 2D-Ansicht (leer) |
| `Befehl \| XYFocusOnSelected \| Commands` | OK | Wirkung XYFocusOnSelected: 2D-Ansicht (Pinsel); Robust XYFocusOnSelected: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste ` = XYFocusOnSelected: 2D-Ansicht (Pinsel) |
| `Befehl \| Zoom100 \| Commands` | OK | Wirkung Zoom100: 2D-Ansicht (2D gezoomt); Robust Zoom100: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| ZoomIn \| Commands` | OK | Wirkung ZoomIn: 2D-Ansicht (leer); Robust ZoomIn: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck |
| `Befehl \| ZoomOut \| Commands` | OK | Wirkung ZoomOut: 2D-Ansicht (leer); Robust ZoomOut: leere Auswahl, leere Karte, Pinsel+Patch+Licht - Karte unveraendert oder per Rueckgaengig zurueck; Taste Insert = ZoomOut: 2D-Ansicht (leer) |

## Dialoge und Fenster

| Eintrag | Stand | Pruefung (Wirkung) |
|---|---|---|
| `Dialog \| Build Menu` | OK | Dialog Build Menu > OK: Dialog (leer); Dialog Build Menu > Cancel: Dialog (leer); Dialog Build Menu > Reset: Dialog (leer) |
| `Dialog \| Mapped Commands` | OK | Dialog Mapped Commands > Edit: Dialog (leer); Dialog Mapped Commands > Clear: Einstellung (leer); Dialog Mapped Commands > Reset: Einstellung (leer); Dialog Mapped Commands > Reset All: Dialog (leer); Dialog Mapped Commands > Reset All > Yes: Einstellung (leer) |
| `Dialog \| CSG Tool` | OK | Dialog CSG Tool > Shrink: Karte (Pinsel); Dialog CSG Tool > Expand: Karte (Pinsel); Dialog CSG Tool > Diagonal: Karte (Pinsel); Dialog CSG Tool > Wrap: Karte (Pinsel); Dialog CSG Tool > Extrude: Karte (Pinsel); Dialog CSG Tool > Pull: Karte (Pinsel); Dialog CSG Tool > -faces: Dialog (Pinsel); Dialog CSG Tool > +faces: Dialog (Pinsel); Dialog CSG Tool > -proj: Dialog (Pinsel); Dialog CSG Tool > -cam: Dialog (Pinsel); Dialog CSG Tool > Caulk: Dialog (Pinsel); Dialog CSG Tool > Remove inner: Dialog (Pinsel); Dialog CSG Tool > Offset up: Dialog (Pinsel); Dialog CSG Tool > Offset down: Dialog (Pinsel) |
| `Dialog \| Entity List` | OK | Dialog Entity List > Auto focus: Dialog (leer); Umschalt+L: Entity List offen, Wurzel aufgeklappt, Zeilen der Entities |
| `Dialog \| Q3Map debug window` | OK | Q3Map debug window offen: "Entity leaked" (select) und "Map leaked" (die Linie); Q3Map debug window: die Zeile waehlt die Entitaet, aus der es leckt (SelectBrush), die 2D-Ansichten auf ihr |
| `Dialog \| Find / Replace Texture(s)` | OK | Dialog Find / Replace Texture(s) > Apply: Karte (leer); Dialog Find / Replace Texture(s) > Close: Dialog (leer); Dialog Find / Replace Texture(s) > Within selected brushes only: Dialog (leer) |
| `Dialog \| Entities` | OK | Licht gewaehlt: Klassenliste auf light, Zeilen classname / light / origin, Kommentar der Klasse; Value 500 + Enter: light = 500, Rueckgaengig entitySetKeyValue -key "light" -value "500"; Delete Key: light weg, entityDeleteKey -key light; Clear All: nur classname und origin bleiben, entityClear; Spawnflag linear: spawnflags 1, entitySetSpawnflags -flags 1 |
| `Dialog \| Arbitrary sides` | OK | Dialog Arbitrary sides > OK: Karte (Pinsel); Dialog Arbitrary sides > Cancel: Dialog (Pinsel); Dialog Arbitrary sides > Truncate: Dialog (Pinsel) |
| `Dialog \| About NetRadiant` | OK | Dialog About NetRadiant > OK: Dialog (leer); Dialog About NetRadiant > Changelog: Extern (leer); Dialog About NetRadiant > About Qt: Dialog (leer) |
| `Dialog \| Set Light intensity` | OK | Dialog Set Light intensity > OK: Karte (leer); Dialog Set Light intensity > Cancel: Karte (leer); Dialog Set Light intensity > Don't show: Dialog (leer) |
| `Dialog \| Install Map Developer's Files` | AUSNAHME | ja.game/jk2.game haben keinen install-Ordner: DoInstallDevFilesDlg zeigt bei JA/JK2 nichts (Stand [~] seit Stufe 2) |
| `Dialog \| Engine Path Configuration` | OK | Dialog Engine Path Configuration > OK: Dialog (leer); OK: Engine Path engine12 (setEnginePath), maps/engine12.txt im Spiel |
| `Dialog \| Camera` | OK | Wirkung ToggleCamera: Haken+Fenster (leer); Menue View > Camera View = ToggleCamera: Haken+Fenster (leer) |
| `Dialog \| Map Info` | OK | Dialog Map Info > Spalte Anzahl: Dialog (leer); Esc: Map Info zu |
| `Dialog \| Find Brush` | OK | Dialog Find Brush > Find: Auswahl (leer); Dialog Find Brush > Close: Dialog (leer) |
| `Dialog \| Patch density` | OK | Dialog Patch density > OK: Karte (leer); Dialog Patch density > Cancel: Dialog (leer) |
| `Dialog \| Patch deform` | OK | Dialog Patch deform > OK: Karte (Patch); Dialog Patch deform > Cancel: Dialog (Patch) |
| `Dialog \| Cap` | OK | Dialog Cap > OK: Karte (Patch); Dialog Cap > Cancel: Dialog (Patch) |
| `Dialog \| Patch thicken` | OK | Dialog Patch thicken > OK: Karte (Patch); Dialog Patch thicken > Cancel: Dialog (Patch); Dialog Patch thicken > Side walls: Dialog (Patch) |
| `Dialog \| Global Preferences` | OK | Dialog Global Preferences > OK: Dialog (leer) |
| `Dialog \| NetRadiant Preferences` | OK | Dialog NetRadiant Preferences > Clean: Dialog (leer); Dialog NetRadiant Preferences > Clean > Yes: Einstellung (leer); Seite Autosave Preferences: OK uebernimmt, Cancel verwirft (AutosaveMinutes bleibt); (+18) |
| `Dialog \| Arbitrary rotation` | OK | Dialog Arbitrary rotation > OK: Karte (Pinsel); Dialog Arbitrary rotation > Apply: Karte (Pinsel); Dialog Arbitrary rotation > Cancel: Dialog (Pinsel) |
| `Dialog \| Arbitrary scale` | OK | Dialog Arbitrary scale > OK: Karte (Pinsel); Dialog Arbitrary scale > Apply: Karte (Pinsel); Dialog Arbitrary scale > Cancel: Dialog (Pinsel) |
| `Dialog \| Surface Inspector` | OK | Dialog Surface Inspector > Reset: Karte (Pinsel, Textur versetzt); Dialog Surface Inspector > Fit: Karte (Pinsel); Dialog Surface Inspector > Width: Karte (Pinsel); Dialog Surface Inspector > Height: Karte (Pinsel); Dialog Surface Inspector > Axial: Karte (Patch); Dialog Surface Inspector > Ortho: Karte (Patch); Dialog Surface Inspector > Cam: Karte (Patch); Dialog Surface Inspector > Natural: Karte (Patch); Dialog Surface Inspector > Match Grid: Einstellung (Pinsel) |

## Einstellungsseiten

| Eintrag | Stand | Pruefung (Wirkung) |
|---|---|---|
| `Seite \| Autosave Preferences` | OK | Seite Autosave Preferences: OK uebernimmt, Cancel verwirft (AutosaveMinutes bleibt) |
| `Seite \| Brush Settings` | OK | Seite Brush Settings: OK uebernimmt, Cancel verwirft (TexdefDefaultScale bleibt) |
| `Seite \| Camera View Preferences` | OK | Seite Camera View Preferences: OK uebernimmt, Cancel verwirft (CamMoveSpeed bleibt) |
| `Seite \| Clipper Tool Settings` | OK | Seite Clipper Tool Settings: OK uebernimmt, Cancel verwirft (ClipperCaulk bleibt) |
| `Seite \| Entity Display Preferences` | OK | Seite Entity Display Preferences: OK uebernimmt, Cancel verwirft (SI_ShowNamesDist bleibt) |
| `Seite \| Grid Settings` | OK | Seite Grid Settings: OK uebernimmt, Cancel verwirft (GridDefault bleibt) |
| `Seite \| Path Settings` | OK | Seite Paths: Engine Path = der Ordner ueber dem ersten Spielordner, fuenf Extra Resource Path; OK: Engine Path engine12 (setEnginePath), maps/engine12.txt im Spiel |
| `Seite \| Layout Preferences` | OK | Seite Layout Preferences: OK uebernimmt, Cancel verwirft (QE4StyleWindows bleibt) |
| `Seite \| Model Browser Preferences` | OK | Seite Model Browser Preferences: OK uebernimmt, Cancel verwirft (ModelBrowserCellSize bleibt) |
| `Seite \| Patch Display Preferences` | OK | Seite Patch Display Preferences: OK uebernimmt, Cancel verwirft (Subdivisions bleibt) |
| `Seite \| Selection System Settings` | OK | Seite Selection System Settings: OK uebernimmt, Cancel verwirft (SELECT_EPSILON bleibt) |
| `Seite \| Surface Inspector Preferences` | OK | Seite Surface Inspector Preferences: OK uebernimmt, Cancel verwirft (SnapTToGrid bleibt) |
| `Seite \| Texture Settings` | OK | Seite Texture Settings: OK uebernimmt, Cancel verwirft (TextureMipLevel bleibt) |
| `Seite \| Texture Browser Preferences` | OK | Seite Texture Browser Preferences: OK uebernimmt, Cancel verwirft (TextureScrollbar bleibt) |
| `Seite \| Undo Queue Settings` | OK | Seite Undo Queue Settings: OK uebernimmt, Cancel verwirft (UndoLevels bleibt) |
| `Seite \| Build Preferences` | OK | Seite Build Preferences: OK uebernimmt, Cancel verwirft (BuildLeakStop bleibt) |
| `Seite \| Orthographic View Preferences` | OK | Seite Orthographic View Preferences: OK uebernimmt, Cancel verwirft (NoStipple bleibt) |

## Einstellungsfelder

| Eintrag | Stand | Pruefung (Wirkung) |
|---|---|---|
| `Feld \| radiant/autosave.cpp:167` | OK | Feld radiant/autosave.cpp:167: Enable Autosave - nach OK geaendert und gespeichert (Autosave) |
| `Feld \| radiant/autosave.cpp:168` | OK | Feld radiant/autosave.cpp:168: Autosave Frequency (minutes) - nach OK geaendert und gespeichert (AutosaveMinutes) |
| `Feld \| radiant/autosave.cpp:170` | OK | Feld radiant/autosave.cpp:170: Save Snapshots - nach OK geaendert und gespeichert (Snapshots) |
| `Feld \| radiant/brushmodule.cpp:86` | OK | Feld radiant/brushmodule.cpp:86: Default texture scale - nach OK geaendert und gespeichert (TexdefDefaultScale) |
| `Feld \| radiant/brushmodule.cpp:95` | OK | Feld radiant/brushmodule.cpp:95: New map Brush Type - nach OK geaendert und gespeichert (BrushType) |
| `Feld \| radiant/brushmodule.cpp:102` | OK | Feld radiant/brushmodule.cpp:102: Always use caulk for new brushes - nach OK geaendert und gespeichert (BrushAlwaysCaulk) |
| `Feld \| radiant/brushmodule.cpp:106` | OK | Feld radiant/brushmodule.cpp:106: Snap plane points to integer grid - nach OK geaendert und gespeichert (BrushSnapPlanes) |
| `Feld \| radiant/camwindow.cpp:2399` | OK | Feld radiant/camwindow.cpp:2399: Movement Speed - nach OK geaendert und gespeichert (CamMoveSpeed) |
| `Feld \| radiant/camwindow.cpp:2400` | OK | Feld radiant/camwindow.cpp:2400: Time to Max Speed - nach OK geaendert und gespeichert (CamMoveTimeToMaxSpeed) |
| `Feld \| radiant/camwindow.cpp:2401` | OK | Feld radiant/camwindow.cpp:2401: Scroll Move Speed - nach OK geaendert und gespeichert (ScrollMoveSpeed) |
| `Feld \| radiant/camwindow.cpp:2402` | OK | Feld radiant/camwindow.cpp:2402: Strafe Speed - nach OK geaendert und gespeichert (CamStrafeSpeed) |
| `Feld \| radiant/camwindow.cpp:2403` | OK | Feld radiant/camwindow.cpp:2403: Mouse Sensitivity - nach OK geaendert und gespeichert (Sensitivity) |
| `Feld \| radiant/camwindow.cpp:2404` | OK | Feld radiant/camwindow.cpp:2404: Invert mouse vertical axis - nach OK geaendert und gespeichert (CamInverseMouse) |
| `Feld \| radiant/camwindow.cpp:2405` | OK | Feld radiant/camwindow.cpp:2405: Zoom to Mouse pointer - nach OK geaendert und gespeichert (3DZoomInToPointer) |
| `Feld \| radiant/camwindow.cpp:2406` | OK | Feld radiant/camwindow.cpp:2406: Discrete movement - nach OK geaendert und gespeichert (CamDiscrete) |
| `Feld \| radiant/camwindow.cpp:2411` | OK | Feld radiant/camwindow.cpp:2411: Enable far-clip plane - nach OK geaendert und gespeichert (CubicClipping) |
| `Feld \| radiant/camwindow.cpp:2416` | OK | Feld radiant/camwindow.cpp:2416: Colorize selection - nach OK geaendert und gespeichert (CameraFaceFill) |
| `Feld \| radiant/camwindow.cpp:2421` | OK | Feld radiant/camwindow.cpp:2421: Selected faces wire - nach OK geaendert und gespeichert (CameraFaceWire) |
| `Feld \| radiant/camwindow.cpp:2428` | OK | Feld radiant/camwindow.cpp:2428: Render Mode - nach OK geaendert und gespeichert (CameraRenderMode) |
| `Feld \| radiant/camwindow.cpp:2438` | OK | Feld radiant/camwindow.cpp:2438: MSAA - nach OK geaendert und gespeichert (CameraMSAA) |
| `Feld \| radiant/camwindow.cpp:2448` | OK | Feld radiant/camwindow.cpp:2448: Strafe Mode - nach OK geaendert und gespeichert (StrafeMode) |
| `Feld \| radiant/camwindow.cpp:2454` | OK | Feld radiant/camwindow.cpp:2454: Field Of View - nach OK geaendert und gespeichert (fieldOfView) |
| `Feld \| radiant/clippertool.cpp:170` | OK | Feld radiant/clippertool.cpp:170: Caulk Clipper Cuts - nach OK geaendert und gespeichert (ClipperCaulk) |
| `Feld \| radiant/clippertool.cpp:171` | OK | Feld radiant/clippertool.cpp:171: Reset Flipped State - nach OK geaendert und gespeichert (ClipperResetFlip) |
| `Feld \| radiant/clippertool.cpp:172` | OK | Feld radiant/clippertool.cpp:172: Reset Points on Split - nach OK geaendert und gespeichert (ClipperResetPoints) |
| `Feld \| radiant/clippertool.cpp:174` | OK | Feld radiant/clippertool.cpp:174: 2 Points in 2D Views - nach OK geaendert und gespeichert (Clipper2PointsIn2D) |
| `Feld \| radiant/clippertool.cpp:177` | OK | Feld radiant/clippertool.cpp:177: On DoubleClick do: - nach OK geaendert und gespeichert (ClipperDoubleclickedSplit) |
| `Feld \| radiant/entity.cpp:669` | OK | Feld radiant/entity.cpp:669: Names Display Distance (3D) - nach OK geaendert und gespeichert (SI_ShowNamesDist) |
| `Feld \| radiant/entity.cpp:673` | OK | Feld radiant/entity.cpp:673: Names Display Ratio (2D) - nach OK geaendert und gespeichert (SI_ShowNamesRatio) |
| `Feld \| radiant/entity.cpp:677` | OK | Feld radiant/entity.cpp:677: Entity Names = Targetnames - nach OK geaendert und gespeichert (SI_ShowTargetNames) |
| `Feld \| radiant/grid.cpp:241` | OK | Feld radiant/grid.cpp:241: Default grid spacing - nach OK geaendert und gespeichert (GridDefault) |
| `Feld \| radiant/grid.cpp:249` | OK | Feld radiant/grid.cpp:249: Max grid coordinate - nach OK geaendert und gespeichert (GridMaxCoordPower) |
| `Feld \| radiant/gtkdlgs.cpp:229` | OK | Seite Game: Select mod (basegame), fs_game "base" gesperrt, Mapping mode; Custom JA modification: fs_game leer und frei (gamecombo_for_gamename) |
| `Feld \| radiant/gtkdlgs.cpp:241` | OK | OK: fs_game testmod11, mp, Extra Resource Path - das Spiel neu geladen, gespeichert |
| `Feld \| radiant/gtkdlgs.cpp:260` | OK | Mapping mode mp: sp_entities.def uebergangen (EntityClassesLoadFile) |
| `Feld \| radiant/mainframe.cpp:398` | OK | OK: Engine Path engine12 (setEnginePath), maps/engine12.txt im Spiel |
| `Feld \| radiant/mainframe.cpp:407` | OK | Extra Resource Path: maps/zusatz11.txt im Spiel (QE_InitVFS) |
| `Feld \| radiant/mainframe.cpp:2031` | OK | Feld radiant/mainframe.cpp:2031: Window Layout - nach OK geaendert und gespeichert (QE4StyleWindows) |
| `Feld \| radiant/mainframe.cpp:2038` | OK | Feld radiant/mainframe.cpp:2038: Detachable Menus - nach OK geaendert und gespeichert (DetachableMenus) |
| `Feld \| radiant/mainframe.cpp:2043` | OK | Feld radiant/mainframe.cpp:2043: Built-In Group Dialog - nach OK geaendert und gespeichert (BuiltInGroupDialog) |
| `Feld \| radiant/map.cpp:2370` | OK | Feld radiant/map.cpp:2370: Load last map at startup - nach OK geaendert und gespeichert (LoadLastMap) |
| `Feld \| radiant/modelwindow.cpp:1366` | OK | Feld radiant/modelwindow.cpp:1366: Model View Size - nach OK geaendert und gespeichert (ModelBrowserCellSize) |
| `Feld \| radiant/modelwindow.cpp:1369` | OK | Feld radiant/modelwindow.cpp:1369: List of *folderToLoad/depth* - nach OK geaendert und gespeichert (ModelBrowserFolders) |
| `Feld \| radiant/patchmanip.cpp:673` | OK | Feld radiant/patchmanip.cpp:673: Patch Subdivide Threshold - nach OK geaendert und gespeichert (Subdivisions) |
| `Feld \| radiant/preferences.cpp:64` | OK | Feld radiant/preferences.cpp:64: Shader Editor Command - nach OK geaendert und gespeichert (CustomShaderEditorCommand) |
| `Feld \| radiant/preferences.cpp:259` | OK | OK: Frage "Restart is required" mit "Selected Game" (PreferencesDialog_showDialog); Yes (Radiant_Restart): Jedi Outcast mit data/karte/jk2, die Karte wieder offen |
| `Feld \| radiant/preferences.cpp:267` | OK | Feld radiant/preferences.cpp:267: Startup / Show Global Preferences - nach OK geaendert und gespeichert (gamePrompt) |
| `Feld \| radiant/select.cpp:1921` | OK | Feld radiant/select.cpp:1921: Nudge selected after duplication - nach OK geaendert und gespeichert (NudgeAfterClone) |
| `Feld \| radiant/selection.cpp:2314` | OK | Feld radiant/selection.cpp:2314: Selector size (pixels) - nach OK geaendert und gespeichert (SELECT_EPSILON) |
| `Feld \| radiant/selection.cpp:2315` | OK | Feld radiant/selection.cpp:2315: Prefer point entities in 2D - nach OK geaendert und gespeichert (PreferPointEntsIn2D) |
| `Feld \| radiant/selection.cpp:2316` | OK | Feld radiant/selection.cpp:2316: Create brushes in 3D - nach OK geaendert und gespeichert (3DCreateBrushes) |
| `Feld \| radiant/selection.cpp:2319` | OK | Feld radiant/selection.cpp:2319: Move style in 3D - nach OK geaendert und gespeichert (3DMoveStyle) |
| `Feld \| radiant/surfacedialog.cpp:1735` | OK | Feld radiant/surfacedialog.cpp:1735: Surface Inspector Increments Match Grid - nach OK geaendert und gespeichert (SnapTToGrid) |
| `Feld \| radiant/textures.cpp:764` | OK | Feld radiant/textures.cpp:764: Texture Quality - nach OK geaendert und gespeichert (TextureMipLevel) |
| `Feld \| radiant/textures.cpp:771` | OK | Feld radiant/textures.cpp:771: Texture Gamma - nach OK geaendert und gespeichert (SI_Gamma) |
| `Feld \| radiant/textures.cpp:780` | OK | Feld radiant/textures.cpp:780: Texture Render Mode - nach OK geaendert und gespeichert (TextureFiltering) |
| `Feld \| radiant/textures.cpp:818` | AUSNAHME | Hardware Texture Compression: D3D11 meldet weder OpenGL- noch S3TC-Kompression, die Liste hat wie NRC ohne diese Erweiterungen nur "None" - es gibt nichts zu waehlen |
| `Feld \| radiant/textures.cpp:826` | OK | Feld radiant/textures.cpp:826: Anisotropy - nach OK geaendert und gespeichert (TextureAnisotropy) |
| `Feld \| radiant/texwindow.cpp:2045` | OK | Feld radiant/texwindow.cpp:2045: Texture scrollbar - nach OK geaendert und gespeichert (TextureScrollbar) |
| `Feld \| radiant/texwindow.cpp:2052` | OK | Feld radiant/texwindow.cpp:2052: Texture Thumbnail Scale - nach OK geaendert und gespeichert (TextureScale) |
| `Feld \| radiant/texwindow.cpp:2059` | OK | Feld radiant/texwindow.cpp:2059: Thumbnails Max Size - nach OK geaendert und gespeichert (UniformTextureSize) |
| `Feld \| radiant/texwindow.cpp:2060` | OK | Feld radiant/texwindow.cpp:2060: Thumbnails Min Size - nach OK geaendert und gespeichert (UniformTextureMinSize) |
| `Feld \| radiant/texwindow.cpp:2061` | OK | Feld radiant/texwindow.cpp:2061: Mousewheel Increment - nach OK geaendert und gespeichert (WheelMouseInc) |
| `Feld \| radiant/texwindow.cpp:2064` | OK | Feld radiant/texwindow.cpp:2064: Load Shaders at Startup - nach OK geaendert und gespeichert (LoadShaders) |
| `Feld \| radiant/texwindow.cpp:2068` | OK | Feld radiant/texwindow.cpp:2068: Hide nonShaders in common folder - nach OK geaendert und gespeichert (HideNonShadersInCommon) |
| `Feld \| radiant/undo.cpp:400` | OK | Feld radiant/undo.cpp:400: Undo Queue Size - nach OK geaendert und gespeichert (UndoLevels) |
| `Feld \| radiant/watchbsp.cpp:221` | OK | Feld radiant/watchbsp.cpp:221: Enable Build Process Monitoring - nach OK geaendert und gespeichert (BuildMonitor) |
| `Feld \| radiant/watchbsp.cpp:222` | OK | Feld radiant/watchbsp.cpp:222: Stop Compilation on Leak - nach OK geaendert und gespeichert (BuildLeakStop) |
| `Feld \| radiant/watchbsp.cpp:223` | OK | Feld radiant/watchbsp.cpp:223: Run Engine After Compile - nach OK geaendert und gespeichert (BuildRunGame) |
| `Feld \| radiant/watchbsp.cpp:227` | OK | Feld radiant/watchbsp.cpp:227: Engine to Run - nach OK geaendert und gespeichert (BuildEngineExecutable) |
| `Feld \| radiant/watchbsp.cpp:229` | OK | Feld radiant/watchbsp.cpp:229: Engine Arguments - nach OK geaendert und gespeichert (BuildEngineArgs) |
| `Feld \| radiant/watchbsp.cpp:232` | OK | Feld radiant/watchbsp.cpp:232: MP Engine to Run - nach OK geaendert und gespeichert (BuildEngineExecutableMP) |
| `Feld \| radiant/watchbsp.cpp:234` | OK | Feld radiant/watchbsp.cpp:234: MP Engine Arguments - nach OK geaendert und gespeichert (BuildEngineArgsMP) |
| `Feld \| radiant/watchbsp.cpp:238` | OK | Feld radiant/watchbsp.cpp:238: Dump non Monitored Builds Log - nach OK geaendert und gespeichert (BuildDumpLog) |
| `Feld \| radiant/watchbsp.cpp:240` | OK | Feld radiant/watchbsp.cpp:240: Region Box Shader - nach OK geaendert und gespeichert (RegionBoxShader) |
| `Feld \| radiant/xywindow.cpp:2237` | OK | Feld radiant/xywindow.cpp:2237: Solid selection boxes ( no stipple ) - nach OK geaendert und gespeichert (NoStipple) |
| `Feld \| radiant/xywindow.cpp:2239` | OK | Feld radiant/xywindow.cpp:2239: Chase mouse during drags - nach OK geaendert und gespeichert (ChaseMouse) |
| `Feld \| radiant/xywindow.cpp:2240` | OK | Feld radiant/xywindow.cpp:2240: Zoom to Mouse pointer - nach OK geaendert und gespeichert (2DZoomInToPointer) |
| `Feld \| radiant/xywindow.cpp:2245` | OK | Feld radiant/xywindow.cpp:2245: MSAA - nach OK geaendert und gespeichert (XYMSAA) |

## Plugins

| Eintrag | Stand | Pruefung (Wirkung) |
|---|---|---|
| `Plugin \| meshTex::SetScale` | OK | Wirkung meshTex::SetScale: Dialog (Patch); Menue Plugins > MeshTex > Set S/T Scale... = meshTex::SetScale: Dialog (Patch) |
| `Plugin \| meshTex::GetInfo` | OK | Wirkung meshTex::GetInfo: Dialog (Patch); Menue Plugins > MeshTex > Get Info... = meshTex::GetInfo: Dialog (Patch) |
| `Plugin \| meshTex::GeneralFunction` | OK | Wirkung meshTex::GeneralFunction: Dialog (Patch); Menue Plugins > MeshTex > General Function... = meshTex::GeneralFunction: Dialog (Patch) |
| `Plugin \| meshTex::STMinMaxAlignAuto` | OK | Wirkung meshTex::STMinMaxAlignAuto: Karte (Patch, Textur versetzt); Menue Plugins > MeshTex > ST Align Auto = meshTex::STMinMaxAlignAuto: Karte (Patch, Textur versetzt) |
| `Plugin \| meshTex::SMinMaxAlignAuto` | OK | Wirkung meshTex::SMinMaxAlignAuto: Karte (Patch, Textur versetzt); Menue Plugins > MeshTex > S Align Auto = meshTex::SMinMaxAlignAuto: Karte (Patch, Textur versetzt) |
| `Plugin \| meshTex::TMinMaxAlignAuto` | OK | Wirkung meshTex::TMinMaxAlignAuto: Karte (Patch, Textur versetzt); Menue Plugins > MeshTex > T Align Auto = meshTex::TMinMaxAlignAuto: Karte (Patch, Textur versetzt) |
| `Plugin \| meshTex::SMinMaxAlignStretch` | OK | Wirkung meshTex::SMinMaxAlignStretch: Karte (Patch, Textur versetzt); Menue Plugins > MeshTex > S Align Stretch = meshTex::SMinMaxAlignStretch: Karte (Patch, Textur versetzt) |
| `Plugin \| meshTex::SMinMaxAlignShrink` | OK | Wirkung meshTex::SMinMaxAlignShrink: Karte (Patch, Textur versetzt); Menue Plugins > MeshTex > S Align Shrink = meshTex::SMinMaxAlignShrink: Karte (Patch, Textur versetzt) |
| `Plugin \| meshTex::TMinMaxAlignStretch` | OK | Wirkung meshTex::TMinMaxAlignStretch: Karte (Patch, Textur versetzt); Menue Plugins > MeshTex > T Align Stretch = meshTex::TMinMaxAlignStretch: Karte (Patch, Textur versetzt) |
| `Plugin \| meshTex::TMinMaxAlignShrink` | OK | Wirkung meshTex::TMinMaxAlignShrink: Karte (Patch, Textur versetzt); Menue Plugins > MeshTex > T Align Shrink = meshTex::TMinMaxAlignShrink: Karte (Patch, Textur versetzt) |
| `Plugin \| meshTex::SMinAlign` | OK | Wirkung meshTex::SMinAlign: Karte (Patch, Textur versetzt); Menue Plugins > MeshTex > S Min Align = meshTex::SMinAlign: Karte (Patch, Textur versetzt) |
| `Plugin \| meshTex::SMaxAlign` | OK | Wirkung meshTex::SMaxAlign: Karte (Patch, Textur versetzt); Menue Plugins > MeshTex > S Max Align = meshTex::SMaxAlign: Karte (Patch, Textur versetzt) |
| `Plugin \| meshTex::TMinAlign` | OK | Wirkung meshTex::TMinAlign: Karte (Patch, Textur versetzt); Menue Plugins > MeshTex > T Min Align = meshTex::TMinAlign: Karte (Patch, Textur versetzt) |
| `Plugin \| meshTex::TMaxAlign` | OK | Wirkung meshTex::TMaxAlign: Karte (Patch, Textur versetzt); Menue Plugins > MeshTex > T Max Align = meshTex::TMaxAlign: Karte (Patch, Textur versetzt) |
| `Plugin \| meshTex::Help` | OK | Wirkung meshTex::Help: Dialog (leer); Menue Plugins > MeshTex > Help... = meshTex::Help: Dialog (leer) |
| `Plugin \| meshTex::About` | OK | Wirkung meshTex::About: Dialog (leer); Menue Plugins > MeshTex > About... = meshTex::About: Dialog (leer) |
| `Plugin \| shaderPlug::CreateTagFile` | OK | Wirkung shaderPlug::CreateTagFile: Datei (ohne Tag-Datei); Menue Plugins > ShaderPlug > Create tag file = shaderPlug::CreateTagFile: Datei (ohne Tag-Datei) |
| `Plugin \| shaderPlug::About` | OK | Wirkung shaderPlug::About: Dialog (leer); Menue Plugins > ShaderPlug > About = shaderPlug::About: Dialog (leer) |
| `Plugin \| sunPlug::ET-MapCoordinator` | OK | Wirkung sunPlug::ET-MapCoordinator: Dialog (leer); Menue Plugins > SunPlug > ET-MapCoordinator = sunPlug::ET-MapCoordinator: Dialog (leer) |
| `Plugin \| sunPlug::About` | OK | Wirkung sunPlug::About: Dialog (leer); Menue Plugins > SunPlug > About... = sunPlug::About: Dialog (leer) |
| `Plugin \| terrainGenerator::GenerateTerrain` | OK | Wirkung terrainGenerator::GenerateTerrain: Dialog (Pinsel); Menue Plugins > TerrainGenerator > Generate Terrain = terrainGenerator::GenerateTerrain: Dialog (Pinsel) |
| `Plugin \| terrainGenerator::About` | OK | Wirkung terrainGenerator::About: Dialog (leer); Menue Plugins > TerrainGenerator > About = terrainGenerator::About: Dialog (leer) |
| `Plugin \| uFO:AI::NoDraw` | OK | Wirkung uFO:AI::NoDraw: Filter+Sicht (leer); Menue Plugins > UFO:AI > NoDraw = uFO:AI::NoDraw: Filter+Sicht (leer) |
| `Plugin \| uFO:AI::WeaponClip` | OK | Wirkung uFO:AI::WeaponClip: Filter+Sicht (leer); Menue Plugins > UFO:AI > WeaponClip = uFO:AI::WeaponClip: Filter+Sicht (leer) |
| `Plugin \| uFO:AI::ActorClip` | OK | Wirkung uFO:AI::ActorClip: Filter+Sicht (leer); Menue Plugins > UFO:AI > ActorClip = uFO:AI::ActorClip: Filter+Sicht (leer) |
| `Plugin \| uFO:AI::StepOn` | OK | Wirkung uFO:AI::StepOn: Filter+Sicht (leer); Menue Plugins > UFO:AI > StepOn = uFO:AI::StepOn: Filter+Sicht (leer) |
| `Plugin \| uFO:AI::Level8` | OK | Wirkung uFO:AI::Level8: Filter+Sicht (leer); Menue Plugins > UFO:AI > Level 8 = uFO:AI::Level8: Filter+Sicht (leer) |
| `Plugin \| uFO:AI::Level7` | OK | Wirkung uFO:AI::Level7: Filter+Sicht (leer); Menue Plugins > UFO:AI > Level 7 = uFO:AI::Level7: Filter+Sicht (leer) |
| `Plugin \| uFO:AI::Level6` | OK | Wirkung uFO:AI::Level6: Filter+Sicht (leer); Menue Plugins > UFO:AI > Level 6 = uFO:AI::Level6: Filter+Sicht (leer) |
| `Plugin \| uFO:AI::Level5` | OK | Wirkung uFO:AI::Level5: Filter+Sicht (leer); Menue Plugins > UFO:AI > Level 5 = uFO:AI::Level5: Filter+Sicht (leer) |
| `Plugin \| uFO:AI::Level4` | OK | Wirkung uFO:AI::Level4: Filter+Sicht (leer); Menue Plugins > UFO:AI > Level 4 = uFO:AI::Level4: Filter+Sicht (leer) |
| `Plugin \| uFO:AI::Level3` | OK | Wirkung uFO:AI::Level3: Filter+Sicht (leer); Menue Plugins > UFO:AI > Level 3 = uFO:AI::Level3: Filter+Sicht (leer) |
| `Plugin \| uFO:AI::Level2` | OK | Wirkung uFO:AI::Level2: Filter+Sicht (leer); Menue Plugins > UFO:AI > Level 2 = uFO:AI::Level2: Filter+Sicht (leer) |
| `Plugin \| uFO:AI::Level1` | OK | Wirkung uFO:AI::Level1: Filter+Sicht (leer); Menue Plugins > UFO:AI > Level 1 = uFO:AI::Level1: Filter+Sicht (leer) |
| `Plugin \| uFO:AI::PerformCheck` | OK | Wirkung uFO:AI::PerformCheck: Dialog (Pinsel); Menue Plugins > UFO:AI > Perform check = uFO:AI::PerformCheck: Dialog (Pinsel) |
| `Plugin \| uFO:AI::Worldspawn` | OK | Wirkung uFO:AI::Worldspawn: Karte (Pinsel); Menue Plugins > UFO:AI > Worldspawn = uFO:AI::Worldspawn: Karte (Pinsel) |
| `Plugin \| uFO:AI::WorldspawnReset` | OK | Wirkung uFO:AI::WorldspawnReset: Karte (Pinsel); Menue Plugins > UFO:AI > Worldspawn reset = uFO:AI::WorldspawnReset: Karte (Pinsel) |
| `Plugin \| uFO:AI::About` | OK | Wirkung uFO:AI::About: Dialog (leer); Menue Plugins > UFO:AI > About = uFO:AI::About: Dialog (leer) |
| `Plugin \| bobToolz::FlipTerrain` | OK | Wirkung bobToolz::FlipTerrain: Karte (Terrain); Menue Plugins > bobToolz > Flip terrain = bobToolz::FlipTerrain: Karte (Terrain) |
| `Plugin \| bobToolz::SplitPatchRows` | OK | Wirkung bobToolz::SplitPatchRows: Karte (Patch); Menue Plugins > bobToolz > Split patch rows = bobToolz::SplitPatchRows: Karte (Patch) |
| `Plugin \| bobToolz::SplitPatchColumns` | OK | Wirkung bobToolz::SplitPatchColumns: Karte (Patch); Menue Plugins > bobToolz > Split patch columns = bobToolz::SplitPatchColumns: Karte (Patch) |
| `Plugin \| bobToolz::SplitPatch` | OK | Wirkung bobToolz::SplitPatch: Karte (Patch); Menue Plugins > bobToolz > Split patch = bobToolz::SplitPatch: Karte (Patch) |
| `Plugin \| bobToolz::MergePatches` | OK | Wirkung bobToolz::MergePatches: Karte (zwei Patches); Menue Plugins > bobToolz > Merge Patches = bobToolz::MergePatches: Karte (zwei Patches) |
| `Plugin \| bobToolz::PlotSplines` | OK | Wirkung bobToolz::PlotSplines: Anzeige (leer); Menue Plugins > bobToolz > Plot Splines = bobToolz::PlotSplines: Anzeige (leer) |
| `Plugin \| bobToolz::DropEntity` | OK | Wirkung bobToolz::DropEntity: Karte (Licht, Tree Planter an); Menue Plugins > bobToolz > Drop Entity = bobToolz::DropEntity: Karte (Licht, Tree Planter an) |
| `Plugin \| bobToolz::TreePlanter` | OK | Wirkung bobToolz::TreePlanter: Haken+Modus (Pinsel); Menue Plugins > bobToolz > Tree Planter = bobToolz::TreePlanter: Haken+Modus (Pinsel) |
| `Plugin \| bobToolz::ExplodeEntity` | OK | Wirkung bobToolz::ExplodeEntity: Karte (Pinsel der func_group); Menue Plugins > bobToolz > Explode Entity = bobToolz::ExplodeEntity: Karte (Pinsel der func_group) |
| `Plugin \| bobToolz::CaulkSelection` | OK | Wirkung bobToolz::CaulkSelection: Karte (Pinsel); Menue Plugins > bobToolz > Caulk Selection = bobToolz::CaulkSelection: Karte (Pinsel) |
| `Plugin \| bobToolz::PolygonBuilder` | OK | Wirkung bobToolz::PolygonBuilder: Dialog (Pinsel); Menue Plugins > bobToolz > Polygon Builder = bobToolz::PolygonBuilder: Dialog (Pinsel) |
| `Plugin \| bobToolz::BrushCleanup` | OK | Wirkung bobToolz::BrushCleanup: Auswahl+Konsole (Pinsel); Menue Plugins > bobToolz > Brush Cleanup = bobToolz::BrushCleanup: Auswahl+Konsole (Pinsel) |
| `Plugin \| bobToolz::VisViewer` | OK | Wirkung bobToolz::VisViewer: Dialog (Pinsel); Menue Plugins > bobToolz > Vis Viewer = bobToolz::VisViewer: Dialog (Pinsel) |
| `Plugin \| bobToolz::PitOMatic` | OK | Wirkung bobToolz::PitOMatic: Karte (hoher Kasten); Menue Plugins > bobToolz > PitOMatic = bobToolz::PitOMatic: Karte (hoher Kasten) |
| `Plugin \| bobToolz::ResetTextures` | OK | Wirkung bobToolz::ResetTextures: Dialog (Pinsel); Menue Plugins > bobToolz > Reset Textures... = bobToolz::ResetTextures: Dialog (Pinsel) |
| `Plugin \| bobToolz::PathPlotter` | OK | Wirkung bobToolz::PathPlotter: Dialog (Pinsel); Menue Plugins > bobToolz > Path Plotter... = bobToolz::PathPlotter: Dialog (Pinsel) |
| `Plugin \| bobToolz::MakeChain` | OK | Wirkung bobToolz::MakeChain: Dialog (Pinsel); Menue Plugins > bobToolz > Make Chain... = bobToolz::MakeChain: Dialog (Pinsel) |
| `Plugin \| bobToolz::Intersect` | OK | Wirkung bobToolz::Intersect: Dialog (Pinsel); Menue Plugins > bobToolz > Intersect... = bobToolz::Intersect: Dialog (Pinsel) |
| `Plugin \| bobToolz::FindDuplicates` | OK | Wirkung bobToolz::FindDuplicates: Auswahl (Pinsel doppelt); Menue Plugins > bobToolz > Find Duplicates = bobToolz::FindDuplicates: Auswahl (Pinsel doppelt) |
| `Plugin \| bobToolz::DoorBuilder` | OK | Wirkung bobToolz::DoorBuilder: Dialog (Pinsel); Menue Plugins > bobToolz > Door Builder... = bobToolz::DoorBuilder: Dialog (Pinsel) |
| `Plugin \| bobToolz::ApertureDoor` | OK | Wirkung bobToolz::ApertureDoor: Dialog (Pinsel); Menue Plugins > bobToolz > Aperture Door... = bobToolz::ApertureDoor: Dialog (Pinsel) |
| `Plugin \| bobToolz::StairBuilder` | OK | Wirkung bobToolz::StairBuilder: Dialog (Pinsel); Menue Plugins > bobToolz > Stair Builder... = bobToolz::StairBuilder: Dialog (Pinsel) |
| `Plugin \| bobToolz::About` | OK | Wirkung bobToolz::About: Dialog (leer); Menue Plugins > bobToolz > About... = bobToolz::About: Dialog (leer) |
| `Plugin \| brushexport2::About` | OK | Wirkung brushexport2::About: Dialog (leer); Menue Plugins > brushexport2 > About = brushexport2::About: Dialog (leer) |
| `Plugin \| brushexport2::Export.obj` | OK | Wirkung brushexport2::Export.obj: Dialog (Pinsel); Menue Plugins > brushexport2 > Export selected as Wavefront Object = brushexport2::Export.obj: Dialog (Pinsel) |
| `Plugin \| prtview::Load.prtFile` | OK | Wirkung prtview::Load.prtFile: Dialog (leer); Menue Plugins > prtview > Load .prt file = prtview::Load.prtFile: Dialog (leer) |
| `Plugin \| prtview::Unload.prtFile` | OK | Wirkung prtview::Unload.prtFile: Leck/Portale (Portale geladen); Menue Plugins > prtview > Unload .prt file = prtview::Unload.prtFile: Leck/Portale (Portale geladen) |
| `Plugin \| prtview::TogglePortals(3D)` | OK | Wirkung prtview::TogglePortals(3D): Leck/Portale (leer); Menue Plugins > prtview > Toggle portals (3D) = prtview::TogglePortals(3D): Leck/Portale (leer) |
| `Plugin \| prtview::TogglePortals(2D)` | OK | Wirkung prtview::TogglePortals(2D): Leck/Portale (leer); Menue Plugins > prtview > Toggle portals (2D) = prtview::TogglePortals(2D): Leck/Portale (leer) |
| `Plugin \| prtview::ConfigurePortalViewer` | OK | Wirkung prtview::ConfigurePortalViewer: Dialog (leer); Menue Plugins > prtview > Configure Portal Viewer = prtview::ConfigurePortalViewer: Dialog (leer) |
| `Plugin \| prtview::AboutPortalViewer` | OK | Wirkung prtview::AboutPortalViewer: Dialog (leer); Menue Plugins > prtview > About Portal Viewer = prtview::AboutPortalViewer: Dialog (leer) |

## Mausbelegung

| Eintrag | Stand | Pruefung (Wirkung) |
|---|---|---|
| `Maus \| M1 \| 2D+3D` | OK | Maus 2D: M1 = Tunnel-Auswahl, derselbe Punkt noch einmal waehlt den naechsten Treffer (C, Licht, A, dann wieder C); Kamera: Tunnel-Auswahl waehlt den Pinsel |
| `Maus \| M1 ziehen \| 2D+3D` | OK | Maus 2D: M1 ziehen + Umschalt = neuer Pinsel quadratisch (64 x 64); Kamera: neuer Pinsel (DragNewBrush), gewaehlt, Rueckgaengig "dragTool" |
| `Maus \| Umschalt+M1 \| 2D+3D` | OK | Maus 2D: Umschalt+M1 waehlt dazu (B und A/C) |
| `Maus \| Umschalt+M1 ziehen \| 2D+3D` | OK | Maus 2D: Umschalt+M1 ziehen malt die Auswahl (A/C und B gewaehlt) |
| `Maus \| Umschalt+M2 \| 2D+3D` | OK | Maus 2D: Umschalt+M2 = Tunnel-Auswahl (B gewaehlt, kein Entitaetenmenue) |
| `Maus \| Umschalt+M2 ziehen \| 2D+3D` | OK | Rechteck: Pinsel und Licht gewaehlt |
| `Maus \| Strg+M2 \| 2D+3D` | OK | Maus 2D: Strg+M2 = Flaechen-Tunnel (eine Flaeche von B gewaehlt) |
| `Maus \| Strg+M2 ziehen \| 2D+3D` | OK | Maus 2D: Strg+M2 ziehen = Rechteck fuer Flaechen (Flaechen von A, B und C, keine Pinsel) |
| `Maus \| Alt+M1 \| 2D+3D` | OK | Maus 2D: Alt+M1 waehlt im Komponenten-Modus Objekte (B und A/C gewaehlt, Ecken-Modus bleibt) |
| `Maus \| Strg+Alt+M1 \| 2D+3D` | OK | Maus 2D: Strg+Alt+M1 zieht Flaechen heraus (ein Pinsel mehr) |
| `Maus \| M1 ziehen + Umschalt \| 2D+3D` | OK | Maus 2D: M1 ziehen + Umschalt = neuer Pinsel quadratisch (64 x 64) |
| `Maus \| M1 ziehen + Strg \| 2D+3D` | OK | Maus 2D: M1 ziehen + Strg = neuer Pinsel als Wuerfel (64 x 64 x 64) |
| `Maus \| M1 x2 auf einem Clipper-Punkt \| 2D+3D` | OK | Doppelklick: Split |
| `Maus \| M2 ziehen \| 2D` | OK | Pan: Ursprung x um 200 / Qt-Massstab / Massstab nach links |
| `Maus \| M2 \| 2D` | OK | M2 ohne Bewegung: das Entitaetenmenue (Untermenues info, ...) |
| `Maus \| Alt+M2 ziehen \| 2D` | OK | Alt+M2: 80 Bildpunkte = 40 logische nach unten: zwei Schritte hinaus (je ueber 16) |
| `Maus \| Alt+M1 \| 2D` | OK | Maus 2D: Alt+M1 im QE-Werkzeug schert (eine Flaeche von B steht schraeg) |
| `Maus \| Strg+M1 \| 2D` | OK | Strg+M1: Clipper eingeschaltet (schnell) |
| `Maus \| M3 \| 2D` | OK | M3: yaw zeigt zum Zeiger (atan2) |
| `Maus \| Strg+M3 \| 2D` | OK | Ctrl+M3: Kamera an den Zeiger (eingerastet), z bleibt |
| `Maus \| Umschalt+M3 \| 2D` | OK | Maus 2D: Umschalt+M3 setzt den Ursprung der Drehung (40 24, eingerastet), z bleibt |
| `Maus \| Mausrad \| 2D` | OK | Mausrad: Massstab x 5/4, Weltpunkt unter dem Zeiger bleibt |
| `Maus \| Alt+M1 \| 3D` | OK | Maus 3D: Alt+M1 = die andere Groessenaenderung (B ist danach anders gross) |
| `Maus \| M1 + Alt \| 3D` | OK | Maus 3D: M1 + Alt verschiebt nur senkrecht (z anders, x und y gleich) |
| `Maus \| Strg+M1 \| 3D` | OK | Maus 3D: Strg+M1 waehlt eine Flaeche (die Oberseite von B) |
| `Maus \| Strg+M1 ziehen \| 3D` | OK | Maus 3D: Strg+M1 ziehen malt Flaechen (die Oberseiten von B und C) |
| `Maus \| M2 \| 3D` | OK | Freelook an (kurzer M2 = Umschalter); Freelook aus (kurzer M2 im Freelook) |
| `Maus \| M2 ziehen \| 3D` | OK | Maus 3D: M2 ziehen = Strafe seitwaerts (y anders, Winkel gleich) |
| `Maus \| Alt+M2 \| 3D` | OK | Maus 3D: Alt+M2 kreist um den angeklickten Punkt (der Abstand zu ihm bleibt, die Kamera wandert, Blickrichtung anders) |
| `Maus \| M2 x2 \| 3D` | OK | Doppel-M2 in der Kamera: das Entitaetenmenue (g_bCamEntityMenu), Freelook aus |
| `Maus \| M3 \| 3D` | OK | M3 (Scene_copyClosestTexture): der Shader der Flaeche im Browser, Status |
| `Maus \| Alt+M3 / ziehen \| 3D` | OK | Maus 3D: Alt+M3 (paintTexture) |
| `Maus \| Umschalt+M3 / ziehen \| 3D` | OK | Maus 3D: Umschalt+M3 (paintTexture,Values,LightPower) |
| `Maus \| Strg+M3 / ziehen \| 3D` | OK | Maus 3D: Strg+M3 (paintTextureSeamless,LightColor) |
| `Maus \| Strg+Umschalt+M3 / ziehen \| 3D` | OK | Maus 3D: Strg+Umschalt+M3 (projectTexture,LightColor&Power) |
| `Maus \| Alt+Umschalt+M3 \| 3D` | OK | Maus 3D: Alt+Umschalt+M3 (paintTexDefValues) |
| `Maus \| Alt+Strg+M3 \| 3D` | OK | Maus 3D: Alt+Strg+M3 (paintTexDefValuesSeamless) |
| `Maus \| Alt+Strg+Umschalt+M3 \| 3D` | OK | Maus 3D: Alt+Strg+Umschalt+M3 (projectTexDefValues) |
| `Maus \| Strg + Maus (Freelook) \| 3D` | OK | Maus 3D: Strg + Maus im Freelook = Strafe (Ursprung anders, Winkel gleich) |
| `Maus \| Strg+Umschalt + Maus (Freelook) \| 3D` | OK | Maus 3D: Strg+Umschalt + Maus im Freelook = Strafe vor/zurueck (x anders, y und Winkel gleich) |
| `Maus \| Mausrad \| 3D` | OK | Mausrad: 100 vorwaerts (Zoom to Mouse pointer) |
| `Maus \| M1 \| Texturbrowser` | OK | M1: gewaehlt, auf den Pinsel gesetzt (textureNameSetSelected), Status "system/caulk W: ..." |
| `Maus \| Umschalt+M1 \| Texturbrowser` | OK | Maus Texturbrowser: Umschalt+M1 oeffnet den Shader im eigenen Editor (system.shader) |
| `Maus \| Strg+Umschalt+M1 \| Texturbrowser` | OK | Maus Texturbrowser: Strg+Umschalt+M1 = externer Editor (loser Shader: editor.exe mit der Datei; im Paket der eigene) |
| `Maus \| M2 \| Texturbrowser` | OK | Kontextmenue: alle Eintraege da (Name, Open Shader, Shader Info, Select ..., Recent, Shader's Folder, system/) |
| `Maus \| M2 ziehen \| Texturbrowser` | OK | M2 gezogen: nach oben gerollt (originy + dy), kein Kontextmenue (Weg >= 16) |
| `Maus \| M2 ziehen + Umschalt \| Texturbrowser` | OK | Maus Texturbrowser: Umschalt+M2 ziehen rollt viermal so schnell (20 logische Punkte -> 80), kein Kontextmenue |
| `Maus \| M1 x2 \| Texturbrowser` | OK | Doppelklick: Verzeichnis des Shaders (common/) |
| `Maus \| M3 \| Texturbrowser` | OK | M3: gewaehlt, der Pinsel behaelt seinen Shader |
| `Maus \| Mausrad \| Texturbrowser` | OK | Rad: um 64 gerollt (geklemmt) |
| `Maus \| M1 \| Entitaetenmenue` | OK | info_null erzeugt bei 64 64 (Mitte der Workzone auf dem Raster), gewaehlt, entityCreate -class info_null; Maus Entitaetenmenue: M1 auf func_group macht aus dem gewaehlten Welt-Pinsel eine Gruppe |
| `Maus \| M1 auf worldspawn \| Entitaetenmenue` | OK | Maus Entitaetenmenue: M1 auf worldspawn loest die gewaehlten Primitive aus der Gruppe (zurueck in die Welt, Gruppe weg) |
| `Maus \| M2 \| Entitaetenmenue` | OK | Maus Entitaetenmenue: M2 wandelt die gewaehlte Entity um (light -> info_null, wie NRC danach nicht gewaehlt), das Menue schliesst |
| `Maus \| Strg+M1 \| Entitaetenmenue` | OK | Strg+Klick: info_notnull erzeugt, das Menue bleibt offen |
| `Maus \| Strg+M2 \| Entitaetenmenue` | OK | Maus Entitaetenmenue: Strg+M2 wandelt um (info_null -> info_notnull), das Menue bleibt offen |
| `Maus \| Alt / Umschalt / Strg druecken und loslassen \| 2D+3D` | OK | Maus: Alt gedrueckt -> g_modifiers mit Alt (onModifierDown); Maus: losgelassen -> g_modifiers leer (onModifierUp) |
| `Maus \| Return / Umschalt+Return / Strg+Return \| Clipper` | OK | Return: Clip, es bleibt x 0..64 (Normale (p2-p0) x (p1-p0) = -x); Umschalt+Return: Split in zwei Pinsel; Strg+Return: Flip |
| `Maus \| Esc \| Entitaetenmenue` | OK | Esc schliesst Untermenue und Menue, ohne Kuerzel (UnSelectSelection2): der zweite bleibt gewaehlt |
| `Maus \| Esc \| Texturbrowser-Filter` | OK | Esc im Filterfeld: leer (ShortcutOverride), keine Auswahl aufgehoben |
| `Maus \| Return / Esc / Tab / Leertaste \| Arbitrary rotation/scale` | OK | Return: OK - noch einmal 90, zu, die Werte bleiben; Esc: Cancel - zu, Werte 0, die Auswahl bleibt; Arbitrary rotation: Tab und Leertaste gehen an den Dialog (kein Klonen, kein Kamera-Fokus), er bleibt offen |
| `Maus \| Teiler ziehen / bis 0 / zurueck \| alle Anordnungen` | OK | Teiler Regular Griff:h:1: bis ans Ende gezogen klappt Kamera ein, der Griff bleibt am Rand; Teiler Regular Griff:h:1: vom Rand zurueckgezogen ist Kamera wieder da |
| `Maus \| F12 (Maximize view) und noch einmal \| Regular/Split` | OK | Maximize view (F12): die Kamera unter der Maus fuellt den Bereich, XY, Konsole und Texturen sind eingeklappt; Maximize view (F12) noch einmal: alle Groessen wie vorher |

## Eigene Eintraege

| Eintrag | Stand | Pruefung (Wirkung) |
|---|---|---|
| `Eigen \| Build > Optimizer...` | OK | Fenster "Optimizer" offen, Pruefung im Hintergrund fertig |
| `Eigen \| Seite Optimizer` | OK | Build-Menue: "Optimizer..." nach "Update entities only" (eigener Eintrag); (+14) |
| `Eigen \| File > Open .bsp...` | OK | Dialog "Open compiled map" offen, mp/duel1.bsp aus einer .pk3 in der Liste; mp/duel1 umgewandelt und geladen: 26 Entitaeten, 362 Pinsel, 66 Patches wie die Massenlast |
| `Eigen \| Build > Update entities only` | OK | duel1.bsp: jeder Lump ausser den Entities Byte-gleich zur Original-.bsp, der Entity-Lump traegt die Aenderung |
| `Eigen \| Build > Cancel build` | OK | Nach dem Abbrechen baut der naechste Eintrag wieder (Port 39000 frei) |
| `Eigen \| Karten-Editor-Modus` | OK | Kuerzel gelten nur im Karten-Editor: Raster bleibt 16 |
| `Eigen \| Begriffe wie NetRadiant` | OK | Begriffe wie NetRadiant: Menue "File", "Edit" trotz Deutsch |
| `Eigen \| Help > About BehavEd` | OK | Help > About BehavEd: der About-Dialog des Programms |
| `Eigen \| Build progress` | OK | erster Bau: Fenster "Build progress" offen, Stufe LIGHT, keine Restzeit aus einem Verlauf; zweiter Bau: Restzeit aus dem letzten Bau dieser Karte; Abbrechen im Fortschrittsfenster: q3map2 beendet, Zeile in der Konsole, Fenster zu, nichts gemeldet |
| `Eigen \| Build > Notify when a build is done` | OK | Build > Melden: aus, als BhedBuildNotify 0 gespeichert |
| `Eigen \| Build > Keep editor data` | OK | Build > Editor-Daten behalten: an, als BhedKeepEditorData 1 gespeichert; Byte-Vergleich: Lumps 1..17 (Geometrie, Lightmaps, Lichtgitter, Sicht) gleich, nur der Entity-Lump anders - mit allen 3 Lichtern; Open .bsp mit Begleitdatei: genaue Quelle - 3 Lichter, misc_model mit Schluesseln, func_group mit 2 Pinseln, 8 Pinsel |
| `Eigen \| Open .bsp > Recover misc_model` | OK | misc_model zurueck: pillar.md3 mit origin 128 128 0, angle 45, modelscale 1.5; Konsole; Update entities only moeglich |
| `Eigen \| Open .bsp > Estimate lights` | OK | geschaetzte Lichter: Ebene "Geschaetzte Lichter", _bhed_geschaetzt 1, eines nahe an 0 0 96; das schaltbare nicht doppelt |
| `Eigen \| Build > Partial compile: reuse VIS / reuse light` | OK | Build > Teil-Kompilierung: VIS und Licht wiederverwenden an, gespeichert; nach einer Entity-Aenderung: Sichtbarkeit wiederverwendet, ein Teil der Lightmaps aus dem Cache; nur die Nachbearbeitung geaendert (-gamma 0.8): alle Lightmaps und das Gitter aus dem Cache |
| `Eigen \| Build > Add to compiled map...` | OK | zusammengefuehrt: <karte>_merged.bsp, laedt im CM-Port, die Saeule stoppt den Spieler, keine Grenze ueber, keine Fehler |
| `Eigen \| Terrain Generator > About` | OK | Terrain Generator > About: Kasten mit "Developed by vallz and vld" |
| `Eigen \| Terrain Generator > Target` | OK | Terrain Generator > Target: Manual Size, die SpinBox-Zeilen 1024 / 1024 / 64 |
| `Eigen \| Terrain Generator > Base Shape` | OK | Terrain Generator > Base Shape: Hill gewaehlt, die Zeile "Peak Height:" erscheint |
| `Eigen \| Terrain Generator > Irregular Grid` | OK | Terrain Generator > Irregular Grid: an, innere Ecken nicht mehr auf dem 64er-Raster (Vielfache von 8) |
| `Eigen \| Terrain Generator > Generate` | OK | Terrain Generator > Generate: Auswahl-Pinsel weg, 1 func_group mit 64 detail-Pinseln, _terraingen_size "512 256 64", gewaehlt, Konsole; Terrain Generator > Generate, Rueckgaengig: ein Schritt stellt die Karte Byte fuer Byte her (der Pinsel ist zurueck); Terrain Generator > Generate (Tunnel): vier neue func_group (Boden, Decke, zwei Waende) |
| `Eigen \| Terrain Generator > Seed` | OK | Terrain Generator > Seed: gleicher Startwert = dieselbe Karte Byte fuer Byte |
| `Eigen \| Terrain Generator > Texture` | OK | Terrain Generator > Texture: der Texturbrowser fuellt das Feld (pick_timer) |
| `Eigen \| Terrain Generator > Pick` | OK | Terrain Generator > Pick: im Split-Layout zeigt es die Seite Textures des Gruppendialogs |
| `Eigen \| Terrain Generator > Reset to Default` | OK | Terrain Generator > Reset to Default: alle Werte auf die Vorgabe, Target bleibt Manual Size, Textur "textures/" |
| `Eigen \| Terrain Generator > Close` | OK | Terrain Generator > Close: der Dialog zu, die Werte gemerkt (static) |
| `Eigen \| Preferences > Layout > Free split windows` | OK | Preferences > Layout > Free split windows: an, als BhedSplitFree 1 gespeichert, vier Platzmenues; Split > Platzmenue > Camera unten rechts: die Kamera ist dort, XZ ging nach oben links; Split > Platzmenue > XY oben links: das Fenster dort zeigt XY - zwei XY-Fenster; Split > Strg+Tab mit Schalter: das aktive Fenster wechselt den Typ (XY -> XZ wie XY_NextView), gespeichert; Split > Teiler ziehen: die linke Spalte ist schmaler, die Groessen gespeichert (BhedSplitH); Split > nach dem simulierten Neustart: Belegung, 2D-Typen und Split-Groesse wie vorher; Regular > 2D-Ansichtstyp nach dem simulierten Neustart: XZ (mit Schalter); Free split windows aus: sofort wieder NRC (Lage, Typen XY/XZ/YZ, kein Platzmenue), nichts mehr gemerkt |
| `Eigen \| View > Play in editor (F6)` | OK | View > Play in editor (F6): aktiv, Kollision aus 5 Brushes, Start an der Kamera (Augen 36 ueber dem Kasten); Spielen > Wand: das Laufen gegen die Wand haelt bei x 512 - 16 an; Spielen > Treppe: die Stufe 16 wird gestiegen (z 40,125); Spielen > Stufe 24 haelt den Spieler an; Spielen > playerclip haelt den Spieler an (y -384 + 16); Spielen > Sprung: 31,62 ueber dem Boden - gleich dem Port mit der gebauten .bsp (ctest karte_spielen); Spielen > N (echte Taste): Noclip an, durch die Wand; Spielen > Esc (echte Taste): beendet, die Kamera genau wie vorher; View > Play in editor (Menue) mit Spawn: Start bei info_player_start (-320 320 24 + 9), Blick 90 |
| `Eigen \| View > Play: start at info_player_start` | OK | View > Play: start at info_player_start: an, als BhedPlayAtSpawn 1 gespeichert |
| `Eigen \| View > Logic view...` | OK | View > Logic view...: das Fenster mit 10 Knoten, 8 Kanten, 1 ohne Ziel; Logic view > Klick: relais gewaehlt, 2D-Ansicht und Kamera darauf zentriert; Logic view > Doppelklick auf ein fehlendes Skript: Kasten mit scripts/gibtsnicht/skript; Logic view > Doppelklick auf ein Skript des Spiels: im Skript-Editor des Programms offen; Logic view (Testerkarte, falls da): jede Verknuepfung ist eine Kante oder ein fehlendes Ziel |
