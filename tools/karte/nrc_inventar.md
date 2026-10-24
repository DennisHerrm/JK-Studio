# NetRadiant-Custom: Checkliste fuer den Karten-Editor

Erzeugt von `tools/karte/nrc_inventar.py` aus dem NRC-Quelltext (Tag 20260114 = NRC 20260114); der Stand je Eintrag wird beim Neuerzeugen uebernommen.

Stand: `[x]` nachgebaut und getestet, `[~]` sichtbar und ausgegraut (Notiz: Stufe), `[ ]` offen.

Menuepunkte: 237, Werkzeugknoepfe: 54, registrierte Befehle: 396, Dialoge: 24, Einstellungsseiten: 17, Einstellungsfelder: 79, Plugin-Eintraege: 67


## Menues


### Brush_constructMenu (radiant/brushmanip.cpp)

  - [x] ITEM Prism... -> `BrushPrism`  (radiant/brushmanip.cpp:1498) -- Stufe 2 Punkt 4
  - [x] ITEM Cone... -> `BrushCone`  (radiant/brushmanip.cpp:1499) -- Stufe 2 Punkt 4
  - [x] ITEM Sphere... -> `BrushSphere`  (radiant/brushmanip.cpp:1500) -- Stufe 2 Punkt 4
  - [x] ITEM Rock... -> `BrushRock`  (radiant/brushmanip.cpp:1501) -- Stufe 2 Punkt 4
  - [x] ITEM Icosahedron... -> `BrushIcosahedron`  (radiant/brushmanip.cpp:1502) -- Stufe 2 Punkt 4
  * ---
  * SUB CSG  (radiant/brushmanip.cpp:1505)
    - [x] ITEM CSG &Subtract -> `CSGSubtract`  (radiant/brushmanip.cpp:1509) -- Stufe 2 Punkt 3
    - [x] ITEM CSG &Merge -> `CSGMerge`  (radiant/brushmanip.cpp:1510) -- Stufe 2 Punkt 3
    - [x] ITEM CSG &Wrap Merge -> `CSGWrapMerge`  (radiant/brushmanip.cpp:1511) -- Stufe 2 Punkt 3
    - [x] ITEM CSG &Intersect -> `CSGIntersect`  (radiant/brushmanip.cpp:1512) -- Stufe 2 Punkt 3
    - [x] ITEM Make &Room -> `CSGroom`  (radiant/brushmanip.cpp:1513) -- Stufe 2 Punkt 3
    - [x] ITEM CSG &Tool -> `CSGTool`  (radiant/brushmanip.cpp:1514) -- Stufe 2 Punkt 3
  * ---
  * SUB Clipper  (radiant/brushmanip.cpp:1518)
    - [x] ITEM Clip selection -> `ClipperClip`  (radiant/brushmanip.cpp:1522) -- Stufe 2 Punkt 2
    - [x] ITEM Split selection -> `ClipperSplit`  (radiant/brushmanip.cpp:1523) -- Stufe 2 Punkt 2
    - [x] ITEM Flip Clip orientation -> `ClipperFlip`  (radiant/brushmanip.cpp:1524) -- Stufe 2 Punkt 2
  * ---
  - [x] ITEM Make detail -> `MakeDetail`  (radiant/brushmanip.cpp:1527) -- Stufe 2 Punkt 3
  - [x] ITEM Make structural -> `MakeStructural`  (radiant/brushmanip.cpp:1528) -- Stufe 2 Punkt 3
  * ---
  - [x] CHECK Texture Lock -> `TogTexLock`  (radiant/brushmanip.cpp:1531) -- Stufe 2 Punkt 1
  - [x] CHECK Texture Vertex Lock -> `TogTexVertexLock`  (radiant/brushmanip.cpp:1532) -- Stufe 2 Punkt 1
  - [x] ITEM Reset Texture -> `TextureReset/Cap`  (radiant/brushmanip.cpp:1533) -- Stufe 2 Punkt 6
  - [~] ITEM AutoCaulk Selected -> `AutoCaulkSelected`  (radiant/brushmanip.cpp:1534) -- Stufe 3

### Build_constructMenu (radiant/build.cpp)

  * ---

### create_colours_menu (radiant/colors.cpp)

  * SUB Theming  (radiant/colors.cpp:211)
    * SUB Viewports Theme  (radiant/colors.cpp:216)
    - [~] ITEM OpenGL Font... -> `OpenGLFont`  (radiant/colors.cpp:231) -- Stufe 2
    * ---

### Entity_constructMenu (radiant/entity.cpp)

  - [x] ITEM &Connect Entities -> `EntitiesConnect`  (radiant/entity.cpp:709) -- Stufe 1b Punkt 9
  - [x] ITEM &Move Primitives to Entity -> `EntityMovePrimitivesToLast`  (radiant/entity.cpp:713) -- Stufe 1b Punkt 9
  - [x] ITEM &Select Color... -> `EntityColorSet`  (radiant/entity.cpp:714) -- Stufe 1b Punkt 9
  - [x] ITEM &Normalize Color -> `EntityColorNormalize`  (radiant/entity.cpp:715) -- Stufe 1b Punkt 9
  * ---
  - [x] ITEM Reload Entity Definitions -> `EntityReloadDefinitions`  (radiant/entity.cpp:717) -- Stufe 1b Punkt 9

### Filters_constructMenu (radiant/filters.cpp)

  - [~] CHECK World -> `FilterWorldBrushes`  (radiant/filters.cpp:161) -- Stufe 2
  - [~] CHECK Entities -> `FilterEntities`  (radiant/filters.cpp:162) -- Stufe 2
  - [~] CHECK Areaportals -> `FilterAreaportals`  (radiant/filters.cpp:168) -- Stufe 2
  - [~] CHECK Translucent -> `FilterTranslucent`  (radiant/filters.cpp:170) -- Stufe 2
  - [~] CHECK Liquids -> `FilterLiquids`  (radiant/filters.cpp:172) -- Stufe 2
  - [~] CHECK Caulk -> `FilterCaulk`  (radiant/filters.cpp:174) -- Stufe 2
  - [~] CHECK Clips -> `FilterClips`  (radiant/filters.cpp:175) -- Stufe 2
  - [~] CHECK Paths -> `FilterPaths`  (radiant/filters.cpp:176) -- Stufe 2
  - [~] CHECK Clusterportals -> `FilterClusterportals`  (radiant/filters.cpp:178) -- Stufe 2
  - [~] CHECK Lights -> `FilterLights`  (radiant/filters.cpp:180) -- Stufe 2
  - [~] CHECK Structural -> `FilterStructural`  (radiant/filters.cpp:181) -- Stufe 2
  - [~] CHECK Lightgrid -> `FilterLightgrid`  (radiant/filters.cpp:183) -- Stufe 2
  - [~] CHECK Patches -> `FilterPatches`  (radiant/filters.cpp:185) -- Stufe 2
  - [~] CHECK Details -> `FilterDetails`  (radiant/filters.cpp:186) -- Stufe 2
  - [~] CHECK Hints -> `FilterHintsSkips`  (radiant/filters.cpp:187) -- Stufe 2
  - [~] CHECK Models -> `FilterModels`  (radiant/filters.cpp:188) -- Stufe 2
  - [~] CHECK Triggers -> `FilterTriggers`  (radiant/filters.cpp:189) -- Stufe 2
  - [~] CHECK Botclips -> `FilterBotClips`  (radiant/filters.cpp:191) -- Stufe 2
  - [~] CHECK Decals -> `FilterDecals`  (radiant/filters.cpp:192) -- Stufe 2
  - [~] CHECK FuncGroups -> `FilterFuncGroups`  (radiant/filters.cpp:194) -- Stufe 2
  - [~] CHECK Point Entities -> `FilterPointEntities`  (radiant/filters.cpp:195) -- Stufe 2
  - [~] CHECK Sky -> `FilterSky`  (radiant/filters.cpp:196) -- Stufe 2
  * ---
  - [~] ITEM Invert filters -> `InvertFilters`  (radiant/filters.cpp:199) -- Stufe 2
  - [~] ITEM Reset filters -> `ResetFilters`  (radiant/filters.cpp:200) -- Stufe 2

### Grid_constructMenu (radiant/grid.cpp)

  - [x] ITEM Grid0.125 -> `SetGrid0.125`  (radiant/grid.cpp:208)
  - [x] ITEM Grid0.25 -> `SetGrid0.25`  (radiant/grid.cpp:209)
  - [x] ITEM Grid0.5 -> `SetGrid0.5`  (radiant/grid.cpp:210)
  - [x] ITEM Grid1 -> `SetGrid1`  (radiant/grid.cpp:211)
  - [x] ITEM Grid2 -> `SetGrid2`  (radiant/grid.cpp:212)
  - [x] ITEM Grid4 -> `SetGrid4`  (radiant/grid.cpp:213)
  - [x] ITEM Grid8 -> `SetGrid8`  (radiant/grid.cpp:214)
  - [x] ITEM Grid16 -> `SetGrid16`  (radiant/grid.cpp:215)
  - [x] ITEM Grid32 -> `SetGrid32`  (radiant/grid.cpp:216)
  - [x] ITEM Grid64 -> `SetGrid64`  (radiant/grid.cpp:217)
  - [x] ITEM Grid128 -> `SetGrid128`  (radiant/grid.cpp:218)
  - [x] ITEM Grid256 -> `SetGrid256`  (radiant/grid.cpp:219)
  - [x] ITEM Grid512 -> `SetGrid512`  (radiant/grid.cpp:220)
  - [x] ITEM Grid1024 -> `SetGrid1024`  (radiant/grid.cpp:221)

### toggle_remove_accelerator (radiant/gtkmisc.cpp)

  * ---

### create_file_menu (radiant/mainframe.cpp)

  * SUB &File  (radiant/mainframe.cpp:870)
    - [x] ITEM &New Map -> `NewMap`  (radiant/mainframe.cpp:874)
    * ---
    - [x] ITEM &Open... -> `OpenMap`  (radiant/mainframe.cpp:877)
    - [~] ITEM &Import... -> `ImportMap`  (radiant/mainframe.cpp:878) -- Stufe 2
    * ---
    - [x] ITEM &Save -> `SaveMap`  (radiant/mainframe.cpp:880)
    - [x] ITEM Save &as... -> `SaveMapAs`  (radiant/mainframe.cpp:881)
    - [~] ITEM Save s&elected... -> `SaveSelected`  (radiant/mainframe.cpp:882) -- Stufe 2
    - [~] ITEM Save re&gion... -> `SaveRegion`  (radiant/mainframe.cpp:883) -- Stufe 2
    * ---
    - [~] ITEM &Pointfile -> `TogglePointfile`  (radiant/mainframe.cpp:885) -- Stufe 3
    * ---
    * ---
    - [ ] ITEM E&xit -> `Exit`  (radiant/mainframe.cpp:889)

### create_edit_menu (radiant/mainframe.cpp)

  * SUB &Edit  (radiant/mainframe.cpp:894)
    - [x] ITEM &Undo -> `Undo`  (radiant/mainframe.cpp:898)
    - [x] ITEM &Redo -> `Redo`  (radiant/mainframe.cpp:899)
    * ---
    - [x] ITEM &Copy -> `Copy`  (radiant/mainframe.cpp:901)
    - [x] ITEM &Paste -> `Paste`  (radiant/mainframe.cpp:902)
    - [x] ITEM P&aste To Camera -> `PasteToCamera`  (radiant/mainframe.cpp:903)
    - [x] ITEM Move To Camera -> `MoveToCamera`  (radiant/mainframe.cpp:904)
    * ---
    - [x] ITEM &Duplicate -> `CloneSelection`  (radiant/mainframe.cpp:906)
    - [x] ITEM Duplicate, make uni&que -> `CloneSelectionAndMakeUnique`  (radiant/mainframe.cpp:907)
    - [x] ITEM D&elete -> `DeleteSelection`  (radiant/mainframe.cpp:908)
    * ---
    - [x] ITEM C&lear Selection -> `UnSelectSelection`  (radiant/mainframe.cpp:911)
    - [x] ITEM &Invert Selection -> `InvertSelection`  (radiant/mainframe.cpp:912)
    - [~] ITEM Select i&nside -> `SelectInside`  (radiant/mainframe.cpp:913) -- Stufe 2
    - [~] ITEM Select &touching -> `SelectTouching`  (radiant/mainframe.cpp:914) -- Stufe 2
    * ---
    - [x] ITEM Select All Of Type -> `SelectAllOfType`  (radiant/mainframe.cpp:918)
    - [~] ITEM Select Textured -> `SelectTextured`  (radiant/mainframe.cpp:919) -- Stufe 2
    - [~] ITEM &Expand Selection To Primitives -> `ExpandSelectionToPrimitives`  (radiant/mainframe.cpp:920) -- Stufe 2
    - [x] ITEM &Expand Selection To Entities -> `ExpandSelectionToEntities`  (radiant/mainframe.cpp:921) -- Stufe 1b Punkt 9
    - [~] ITEM &Expand Selection To Layers -> `ExpandSelectionToLayers`  (radiant/mainframe.cpp:922) -- Stufe 2
    - [x] ITEM Select Connected Entities -> `SelectConnectedEntities`  (radiant/mainframe.cpp:923) -- Stufe 1b Punkt 9
    * ---
    - [x] ITEM &Shortcuts... -> `Shortcuts`  (radiant/mainframe.cpp:926)
    - [x] ITEM Pre&ferences... -> `Preferences`  (radiant/mainframe.cpp:927)

### create_view_menu (radiant/mainframe.cpp)

  * SUB Vie&w  (radiant/mainframe.cpp:932)
    - [x] CHECK Camera View [wenn: if ( style == MainFrame::eFloating )] -> `ToggleCamera`  (radiant/mainframe.cpp:937)
    - [x] CHECK XY (Top) View [wenn: if ( style == MainFrame::eFloating )] -> `ToggleView`  (radiant/mainframe.cpp:938)
    - [x] CHECK XZ (Front) View [wenn: if ( style == MainFrame::eFloating )] -> `ToggleFrontView`  (radiant/mainframe.cpp:939)
    - [x] CHECK YZ (Side) View [wenn: if ( style == MainFrame::eFloating )] -> `ToggleSideView`  (radiant/mainframe.cpp:940)
    - [x] ITEM Console [wenn: if ( style != MainFrame::eRegular && style != MainFrame::eRegularLeft )] -> `ToggleConsole`  (radiant/mainframe.cpp:943) -- Stufe 1b Punkt 10
    - [ ] ITEM Texture Browser [wenn: if ( ( style != MainFrame::eRegular && style != MainFrame::eRegularLeft ) || g_Layout_builtInGroupDialog.m_value )] -> `ToggleTextures`  (radiant/mainframe.cpp:946)
    - [x] ITEM Model Browser -> `ToggleModelBrowser`  (radiant/mainframe.cpp:948)
    - [x] ITEM Entity Inspector -> `ToggleEntityInspector`  (radiant/mainframe.cpp:949) -- Stufe 1b Punkt 9
    - [x] ITEM Layers Browser -> `ToggleLayersBrowser`  (radiant/mainframe.cpp:950)
    - [~] ITEM &Surface Inspector -> `SurfaceInspector`  (radiant/mainframe.cpp:951) -- Stufe 2
    - [x] ITEM Entity List -> `ToggleEntityList`  (radiant/mainframe.cpp:952) -- Stufe 1b Punkt 9
    * ---
    * SUB Camera  (radiant/mainframe.cpp:956)
      - [x] ITEM Focus on Selected -> `CameraFocusOnSelected`  (radiant/mainframe.cpp:960)
      - [x] ITEM &Center -> `CenterView`  (radiant/mainframe.cpp:961)
      - [x] ITEM &Up Floor -> `UpFloor`  (radiant/mainframe.cpp:962)
      - [x] ITEM &Down Floor -> `DownFloor`  (radiant/mainframe.cpp:963)
      * ---
      - [x] ITEM Far Clip Plane In -> `CubicClipZoomIn`  (radiant/mainframe.cpp:965)
      - [x] ITEM Far Clip Plane Out -> `CubicClipZoomOut`  (radiant/mainframe.cpp:966)
      * ---
      - [~] ITEM Next leak spot -> `NextLeakSpot`  (radiant/mainframe.cpp:968) -- Stufe 3
      - [~] ITEM Previous leak spot -> `PrevLeakSpot`  (radiant/mainframe.cpp:969) -- Stufe 3
    * ---
    * SUB Orthographic  (radiant/mainframe.cpp:977)
      - [x] ITEM &Next (XY, XZ, YZ) [wenn: if ( style == MainFrame::eRegular || style == MainFrame::eRegularLeft || style == MainFrame::eFloating )] -> `NextView`  (radiant/mainframe.cpp:982) -- Stufe 1b Punkt 5
      - [x] ITEM XY (Top) [wenn: if ( style == MainFrame::eRegular || style == MainFrame::eRegularLeft || style == MainFrame::eFloating )] -> `ViewTop`  (radiant/mainframe.cpp:983) -- Stufe 1b Punkt 5
      - [x] ITEM XZ (Front) [wenn: if ( style == MainFrame::eRegular || style == MainFrame::eRegularLeft || style == MainFrame::eFloating )] -> `ViewFront`  (radiant/mainframe.cpp:984) -- Stufe 1b Punkt 5
      - [x] ITEM YZ (Side) [wenn: if ( style == MainFrame::eRegular || style == MainFrame::eRegularLeft || style == MainFrame::eFloating )] -> `ViewSide`  (radiant/mainframe.cpp:985) -- Stufe 1b Punkt 5
      * ---
      - [x] ITEM Center on Selected [wenn: nicht (if ( style == MainFrame::eRegular || style == MainFrame::eRegularLeft || style == MainFrame::eFloating ))] -> `NextView`  (radiant/mainframe.cpp:989) -- Stufe 1b Punkt 5
      - [x] ITEM Focus on Selected -> `XYFocusOnSelected`  (radiant/mainframe.cpp:992) -- Stufe 1b Punkt 5
      - [x] ITEM Center on Selected -> `CenterXYView`  (radiant/mainframe.cpp:993) -- Stufe 1b Punkt 5
      * ---
      - [x] ITEM &XY 100% -> `Zoom100`  (radiant/mainframe.cpp:995) -- Stufe 1b Punkt 5
      - [x] ITEM XY Zoom &In -> `ZoomIn`  (radiant/mainframe.cpp:996) -- Stufe 1b Punkt 5
      - [x] ITEM XY Zoom &Out -> `ZoomOut`  (radiant/mainframe.cpp:997) -- Stufe 1b Punkt 5
    * ---
    * SUB Show  (radiant/mainframe.cpp:1003)
      - [x] CHECK Show Entity &Angles -> `ShowAngles`  (radiant/mainframe.cpp:1007) -- Stufe 1b Punkt 5
      - [x] CHECK Show Entity &Names -> `ShowNames`  (radiant/mainframe.cpp:1008) -- Stufe 1b Punkt 5
      - [x] CHECK Show Light Radiuses -> `ShowLightRadiuses`  (radiant/mainframe.cpp:1009)
      - [x] CHECK Show Entity Boxes -> `ShowBboxes`  (radiant/mainframe.cpp:1010)
      - [x] CHECK Show Entity Connections -> `ShowConnections`  (radiant/mainframe.cpp:1011) -- Stufe 1b Punkt 5
      * ---
      - [x] CHECK Show 2D Size Info -> `ShowSize2d`  (radiant/mainframe.cpp:1015) -- Stufe 1b Punkt 5
      - [x] CHECK Show 3D Size Info -> `ShowSize3d`  (radiant/mainframe.cpp:1016)
      - [x] CHECK Show Crosshair -> `ToggleCrosshairs`  (radiant/mainframe.cpp:1017) -- Stufe 1b Punkt 5
      - [x] CHECK Show Grid -> `ToggleGrid`  (radiant/mainframe.cpp:1018) -- Stufe 1b Punkt 5
      - [x] CHECK Show Blocks -> `ShowBlocks`  (radiant/mainframe.cpp:1019) -- Stufe 1b Punkt 5
      - [x] CHECK Show C&oordinates -> `ShowCoordinates`  (radiant/mainframe.cpp:1020) -- Stufe 1b Punkt 5
      - [x] CHECK Show Window Outline -> `ShowWindowOutline`  (radiant/mainframe.cpp:1021) -- Stufe 1b Punkt 5
      - [x] CHECK Show Axes -> `ShowAxes`  (radiant/mainframe.cpp:1022) -- Stufe 1b Punkt 5
      - [x] CHECK Show 2D Workzone -> `ShowWorkzone2d`  (radiant/mainframe.cpp:1023) -- Stufe 1b Punkt 5
      - [x] CHECK Show 3D Workzone -> `ShowWorkzone3d`  (radiant/mainframe.cpp:1024)
      - [x] CHECK Show Renderer Stats -> `ShowStats`  (radiant/mainframe.cpp:1025)
    * SUB Filter  (radiant/mainframe.cpp:1029)
    * ---
    - [x] CHECK Hide Selected -> `HideSelected`  (radiant/mainframe.cpp:1037)
    - [x] ITEM Show Hidden -> `ShowHidden`  (radiant/mainframe.cpp:1038)
    * ---
    * SUB Region  (radiant/mainframe.cpp:1042)
      - [~] ITEM &Off -> `RegionOff`  (radiant/mainframe.cpp:1046) -- Stufe 2
      - [~] ITEM &Set XY -> `RegionSetXY`  (radiant/mainframe.cpp:1047) -- Stufe 2
      - [~] ITEM Set &Brush -> `RegionSetBrush`  (radiant/mainframe.cpp:1048) -- Stufe 2
      - [~] CHECK Set Se&lection -> `RegionSetSelection`  (radiant/mainframe.cpp:1049) -- Stufe 2

### create_selection_menu (radiant/mainframe.cpp)

  * SUB M&odify  (radiant/mainframe.cpp:1057)
    * SUB Components  (radiant/mainframe.cpp:1061)
      - [x] CHECK &Edges -> `DragEdges`  (radiant/mainframe.cpp:1065) -- Stufe 2 Punkt 1
      - [x] CHECK &Vertices -> `DragVertices`  (radiant/mainframe.cpp:1066) -- Stufe 2 Punkt 1
      - [x] CHECK &Faces -> `DragFaces`  (radiant/mainframe.cpp:1067) -- Stufe 2 Punkt 1
    * ---
    - [x] ITEM Snap To Grid -> `SnapToGrid`  (radiant/mainframe.cpp:1071)
    * ---
    * SUB Nudge  (radiant/mainframe.cpp:1076)
      - [x] ITEM Nudge Left -> `SelectNudgeLeft`  (radiant/mainframe.cpp:1080)
      - [x] ITEM Nudge Right -> `SelectNudgeRight`  (radiant/mainframe.cpp:1081)
      - [x] ITEM Nudge Up -> `SelectNudgeUp`  (radiant/mainframe.cpp:1082)
      - [x] ITEM Nudge Down -> `SelectNudgeDown`  (radiant/mainframe.cpp:1083)
      * ---
      - [x] ITEM Nudge +Z -> `MoveSelectionUP`  (radiant/mainframe.cpp:1085)
      - [x] ITEM Nudge -Z -> `MoveSelectionDOWN`  (radiant/mainframe.cpp:1086)
    * SUB Rotate  (radiant/mainframe.cpp:1089)
      - [x] ITEM Rotate X -> `RotateSelectionX`  (radiant/mainframe.cpp:1093) -- Stufe 2 Punkt 5
      - [x] ITEM Rotate Y -> `RotateSelectionY`  (radiant/mainframe.cpp:1094) -- Stufe 2 Punkt 5
      - [x] ITEM Rotate Z -> `RotateSelectionZ`  (radiant/mainframe.cpp:1095) -- Stufe 2 Punkt 5
      * ---
      - [x] ITEM Rotate Clockwise -> `RotateSelectionClockwise`  (radiant/mainframe.cpp:1097) -- Stufe 2 Punkt 5
      - [x] ITEM Rotate Anticlockwise -> `RotateSelectionAnticlockwise`  (radiant/mainframe.cpp:1098) -- Stufe 2 Punkt 5
    * SUB Flip  (radiant/mainframe.cpp:1101)
      - [x] ITEM Flip &X -> `MirrorSelectionX`  (radiant/mainframe.cpp:1105) -- Stufe 2 Punkt 5
      - [x] ITEM Flip &Y -> `MirrorSelectionY`  (radiant/mainframe.cpp:1106) -- Stufe 2 Punkt 5
      - [x] ITEM Flip &Z -> `MirrorSelectionZ`  (radiant/mainframe.cpp:1107) -- Stufe 2 Punkt 5
      * ---
      - [x] ITEM Flip Horizontally -> `MirrorSelectionHorizontally`  (radiant/mainframe.cpp:1109) -- Stufe 2 Punkt 5
      - [x] ITEM Flip Vertically -> `MirrorSelectionVertically`  (radiant/mainframe.cpp:1110) -- Stufe 2 Punkt 5
    * ---
    - [x] ITEM Arbitrary rotation... -> `ArbitraryRotation`  (radiant/mainframe.cpp:1113) -- Stufe 2 Punkt 5
    - [x] ITEM Arbitrary scale... -> `ArbitraryScale`  (radiant/mainframe.cpp:1114) -- Stufe 2 Punkt 5
    * ---
    * SUB Repeat  (radiant/mainframe.cpp:1117)
      - [x] ITEM Repeat Transforms -> `RepeatTransforms`  (radiant/mainframe.cpp:1121) -- Stufe 2 Punkt 5
      - [x] ITEM (dynamisch)  (radiant/mainframe.cpp:1125) -- Stufe 2 Punkt 5
      - [x] ITEM Reset Transforms -> `ResetTransforms`  (radiant/mainframe.cpp:1133) -- Stufe 2 Punkt 5

### create_bsp_menu (radiant/mainframe.cpp)

  * SUB &Build  (radiant/mainframe.cpp:1139)
    - [~] ITEM Customize... -> `BuildMenuCustomize`  (radiant/mainframe.cpp:1143) -- Stufe 3
    - [~] ITEM Run recent build -> `Build_runRecentExecutedBuild`  (radiant/mainframe.cpp:1144) -- Stufe 3
    * ---

### create_grid_menu (radiant/mainframe.cpp)

  * SUB &Grid  (radiant/mainframe.cpp:1156)

### create_misc_menu (radiant/mainframe.cpp)

  * SUB M&isc  (radiant/mainframe.cpp:1165)
    - [~] ITEM Find brush... -> `FindBrush`  (radiant/mainframe.cpp:1173) -- Stufe 2
    - [~] ITEM Map Info... -> `MapInfo`  (radiant/mainframe.cpp:1174) -- Stufe 2
    - [~] ITEM &Refresh models -> `RefreshReferences`  (radiant/mainframe.cpp:1175) -- Stufe 2
    - [~] ITEM Set 2D &Background image...  (radiant/mainframe.cpp:1176) -- Stufe 2 (ausgegraut, Tooltip nennt die Stufe)
    - [~] ITEM Fullscreen -> `Fullscreen`  (radiant/mainframe.cpp:1177) -- Stufe 2
    - [x] ITEM Maximize view -> `MaximizeView`  (radiant/mainframe.cpp:1178)

### create_entity_menu (radiant/mainframe.cpp)

  * SUB E&ntity  (radiant/mainframe.cpp:1183)

### create_brush_menu (radiant/mainframe.cpp)

  * SUB Brush  (radiant/mainframe.cpp:1192)

### create_patch_menu (radiant/mainframe.cpp)

  * SUB &Curve  (radiant/mainframe.cpp:1201)

### create_help_menu (radiant/mainframe.cpp)

  * SUB &Help  (radiant/mainframe.cpp:1210)
    - [~] ITEM Bug report  (radiant/mainframe.cpp:1220) -- Stufe 2 (ausgegraut, Tooltip nennt die Stufe)
    - [~] ITEM Check for NetRadiant update -> `CheckForUpdate`  (radiant/mainframe.cpp:1221) -- Stufe 3
    - [~] ITEM &About  (radiant/mainframe.cpp:1222) -- Stufe 2 (ausgegraut, Tooltip nennt die Stufe)

### Maximize_View (radiant/mainframe.cpp)

  * ---

### MRU_constructMenu (radiant/mru.cpp)

  - [x] ITEM &1  (radiant/mru.cpp:197) -- Eintrag 1 im Selbsttest angeklickt; 2-9 derselbe Weg
  - [ ] ITEM &2  (radiant/mru.cpp:202)
  - [ ] ITEM &3  (radiant/mru.cpp:207)
  - [ ] ITEM &4  (radiant/mru.cpp:212)
  - [ ] ITEM &5  (radiant/mru.cpp:217)
  - [ ] ITEM &6  (radiant/mru.cpp:222)
  - [ ] ITEM &7  (radiant/mru.cpp:227)
  - [ ] ITEM &8  (radiant/mru.cpp:232)
  - [ ] ITEM &9  (radiant/mru.cpp:237)

### Patch_constructMenu (radiant/patchmanip.cpp)

  - [x] ITEM Simple Patch Mesh... -> `SimplePatchMesh`  (radiant/patchmanip.cpp:739) -- Stufe 2 Punkt 6
  - [x] ITEM Bevel -> `PatchBevel`  (radiant/patchmanip.cpp:740) -- Stufe 2 Punkt 6
  - [x] ITEM End cap -> `PatchEndCap`  (radiant/patchmanip.cpp:741) -- Stufe 2 Punkt 6
  - [x] ITEM Cylinder (9x3) -> `PatchCylinder`  (radiant/patchmanip.cpp:742) -- Stufe 2 Punkt 6
  - [x] ITEM Square Cylinder (9x3) -> `PatchSquareCylinder`  (radiant/patchmanip.cpp:743) -- Stufe 2 Punkt 6
  - [x] ITEM Exact Cylinder... -> `PatchXactCylinder`  (radiant/patchmanip.cpp:744) -- Stufe 2 Punkt 6
  - [x] ITEM Cone (9x3) -> `PatchCone`  (radiant/patchmanip.cpp:745) -- Stufe 2 Punkt 6
  - [x] ITEM Exact Cone... -> `PatchXactCone`  (radiant/patchmanip.cpp:746) -- Stufe 2 Punkt 6
  - [x] ITEM Sphere (9x5) -> `PatchSphere`  (radiant/patchmanip.cpp:747) -- Stufe 2 Punkt 6
  - [x] ITEM Exact Sphere... -> `PatchXactSphere`  (radiant/patchmanip.cpp:748) -- Stufe 2 Punkt 6
  * ---
  - [x] ITEM Cap Selection -> `CapCurrentCurve`  (radiant/patchmanip.cpp:764) -- Stufe 2 Punkt 6
  * ---
  * SUB Insert/Delete  (radiant/patchmanip.cpp:767)
    - [x] ITEM Insert (2) First Columns -> `PatchInsertFirstColumn`  (radiant/patchmanip.cpp:771) -- Stufe 2 Punkt 6
    - [x] ITEM Insert (2) Last Columns -> `PatchInsertLastColumn`  (radiant/patchmanip.cpp:772) -- Stufe 2 Punkt 6
    * ---
    - [x] ITEM Insert (2) First Rows -> `PatchInsertFirstRow`  (radiant/patchmanip.cpp:774) -- Stufe 2 Punkt 6
    - [x] ITEM Insert (2) Last Rows -> `PatchInsertLastRow`  (radiant/patchmanip.cpp:775) -- Stufe 2 Punkt 6
    * ---
    - [x] ITEM Del First (2) Columns -> `PatchDeleteFirstColumn`  (radiant/patchmanip.cpp:777) -- Stufe 2 Punkt 6
    - [x] ITEM Del Last (2) Columns -> `PatchDeleteLastColumn`  (radiant/patchmanip.cpp:778) -- Stufe 2 Punkt 6
    * ---
    - [x] ITEM Del First (2) Rows -> `PatchDeleteFirstRow`  (radiant/patchmanip.cpp:780) -- Stufe 2 Punkt 6
    - [x] ITEM Del Last (2) Rows -> `PatchDeleteLastRow`  (radiant/patchmanip.cpp:781) -- Stufe 2 Punkt 6
  * ---
  * SUB Matrix  (radiant/patchmanip.cpp:785)
    - [x] ITEM Invert -> `InvertCurve`  (radiant/patchmanip.cpp:789) -- Stufe 2 Punkt 6
    - [x] ITEM Transpose -> `MatrixTranspose`  (radiant/patchmanip.cpp:790) -- Stufe 2 Punkt 6
    * ---
    - [x] ITEM Re-disperse Rows -> `RedisperseRows`  (radiant/patchmanip.cpp:793) -- Stufe 2 Punkt 6
    - [x] ITEM Re-disperse Columns -> `RedisperseCols`  (radiant/patchmanip.cpp:794) -- Stufe 2 Punkt 6
    * ---
    - [x] ITEM Smooth Rows -> `SmoothRows`  (radiant/patchmanip.cpp:797) -- Stufe 2 Punkt 6
    - [x] ITEM Smooth Columns -> `SmoothCols`  (radiant/patchmanip.cpp:798) -- Stufe 2 Punkt 6
  * ---
  * SUB Texture  (radiant/patchmanip.cpp:802)
    - [x] ITEM Reset Texture -> `TextureReset/Cap`  (radiant/patchmanip.cpp:806) -- Stufe 2 Punkt 6
    - [x] ITEM Naturalize -> `NaturalizePatch`  (radiant/patchmanip.cpp:807) -- Stufe 2 Punkt 6
    - [x] ITEM Invert X -> `InvertCurveTextureX`  (radiant/patchmanip.cpp:808) -- Stufe 2 Punkt 6
    - [x] ITEM Invert Y -> `InvertCurveTextureY`  (radiant/patchmanip.cpp:809) -- Stufe 2 Punkt 6
  * ---
  - [x] ITEM Deform... -> `PatchDeform`  (radiant/patchmanip.cpp:821) -- Stufe 2 Punkt 6
  - [x] ITEM Thicken... -> `PatchThicken`  (radiant/patchmanip.cpp:822) -- Stufe 2 Punkt 6

### PlugInMenu_Add (radiant/pluginmenu.cpp)

  * ---

### create_plugins_menu (radiant/pluginmenu.cpp)

  * SUB &Plugins  (radiant/pluginmenu.cpp:131)

### TextureBrowser_createTreeViewTree (radiant/texwindow.cpp)

  - [x] CHECK Hide Unused -> `ShowInUse`  (radiant/texwindow.cpp:1239)
  - [x] ITEM Show All -> `ShowAllTextures`  (radiant/texwindow.cpp:1240)
  * ---
  - [x] CHECK Show Shaders -> `ToggleShowShaders`  (radiant/texwindow.cpp:1250)
  - [x] CHECK Show Textures -> `ToggleShowTextures`  (radiant/texwindow.cpp:1251)
  * ---
  - [x] CHECK ShaderList Only -> `ToggleShowShaderlistOnly`  (radiant/texwindow.cpp:1256)
  - [x] CHECK Hide Image Missing -> `FilterNotex`  (radiant/texwindow.cpp:1259)
  * ---
  - [x] CHECK Fixed Size -> `FixedSize`  (radiant/texwindow.cpp:1263)
  - [x] CHECK Transparency -> `EnableAlpha`  (radiant/texwindow.cpp:1264)
  * ---
  - [~] CHECK Tags GUI -> `TagsToggleGui`  (radiant/texwindow.cpp:1267) -- Stufe 2
  - [~] ITEM Add tag -> `TagAdd`  (radiant/texwindow.cpp:1282) -- Stufe 2
  - [~] ITEM Rename tag -> `TagRename`  (radiant/texwindow.cpp:1283) -- Stufe 2
  - [~] ITEM Delete tag -> `TagDelete`  (radiant/texwindow.cpp:1284) -- Stufe 2
  * ---
  - [~] ITEM Copy tags from selected -> `TagCopy`  (radiant/texwindow.cpp:1286) -- Stufe 2
  - [~] ITEM Paste tags to selected -> `TagPaste`  (radiant/texwindow.cpp:1287) -- Stufe 2
  * ---
  - [~] ITEM Search tag -> `TagSearch`  (radiant/texwindow.cpp:1289) -- Stufe 2
  - [~] ITEM Search Untagged -> `TagSearchUntagged`  (radiant/texwindow.cpp:1290) -- Stufe 2

### TextureBrowser_ContextMenu (radiant/texwindow.cpp)

  * ---
  * ---
  * ---
  * SUB -> Recent  (radiant/texwindow.cpp:1620)

### theme_construct_menu (radiant/theme.cpp)

  * SUB GUI Theme  (radiant/theme.cpp:158)

## Werkzeugleisten


### CamWnd_constructToolbar (radiant/camwindow.cpp)

- [x] toggle_button Cubic clip the camera view | view_cubicclipping.png -> `ToggleCubicClip`  (radiant/camwindow.cpp:2260)

### create_filter_toolbar (radiant/filterbar.cpp)

- [~] toggle_button World | f-world.png -> `FilterWorldBrushes`  (radiant/filterbar.cpp:140) -- Stufe 2
- [~] toggle_button Structural / RightClick: MakeStructural | f-structural.png -> `FilterStructural`  (radiant/filterbar.cpp:142) -- Stufe 2
- [~] toggle_button Details / RightClick: MakeDetail | f-details.png -> `FilterDetails`  (radiant/filterbar.cpp:145) -- Stufe 2
- [~] toggle_button Func_Groups / RightClick: create func_group | f-funcgroups.png -> `FilterFuncGroups`  (radiant/filterbar.cpp:148) -- Stufe 2
- [~] toggle_button Patches | f-patches.png -> `FilterPatches`  (radiant/filterbar.cpp:151) -- Stufe 2
- [~] toggle_button Translucent / RightClick: toggle tex / noDraw / noDrawNonSolid | f-translucent.png -> `FilterTranslucent`  (radiant/filterbar.cpp:161) -- Stufe 2
- [~] toggle_button Liquids / RightClick: toggle tex / waterCaulk / lavaCaulk / slimeCaulk | f-liquids.png -> `FilterLiquids`  (radiant/filterbar.cpp:164) -- Stufe 2
- [~] toggle_button Caulk / RightClick: tex Caulk | f-caulk.png -> `FilterCaulk`  (radiant/filterbar.cpp:167) -- Stufe 2
- [~] toggle_button Clips / RightClick: toggle tex / playerClip / weapClip | f-clip.png -> `FilterClips`  (radiant/filterbar.cpp:170) -- Stufe 2
- [~] toggle_button HintsSkips / RightClick: toggle tex / hint / hintLocal / hintSkip | f-hint.png -> `FilterHintsSkips`  (radiant/filterbar.cpp:173) -- Stufe 2
- [~] toggle_button Sky | f-sky.png -> `FilterSky`  (radiant/filterbar.cpp:176) -- Stufe 2
- [~] toggle_button Entities | f-entities.png -> `FilterEntities`  (radiant/filterbar.cpp:180) -- Stufe 2
- [~] toggle_button Point Entities | f-pointentities.png -> `FilterPointEntities`  (radiant/filterbar.cpp:181) -- Stufe 2
- [~] toggle_button Lights | f-lights.png -> `FilterLights`  (radiant/filterbar.cpp:182) -- Stufe 2
- [~] toggle_button Models | f-models.png -> `FilterModels`  (radiant/filterbar.cpp:183) -- Stufe 2
- [~] toggle_button Triggers / RightClick: tex Trigger | f-triggers.png -> `FilterTriggers`  (radiant/filterbar.cpp:185) -- Stufe 2
- [~] button ResetFilters | f-reset.png -> `ResetFilters`  (radiant/filterbar.cpp:192) -- Stufe 2
- [~] toggle_button Region Set Selection / RightClick: Region Off | f-region.png -> `RegionSetSelection`  (radiant/filterbar.cpp:195) -- Stufe 2
- [x] toggle_button Hide Selected / RightClick: Show Hidden | f-hide.png -> `HideSelected`  (radiant/filterbar.cpp:198)

### File_constructToolbar (radiant/mainframe.cpp)

- [ ] button Open an existing map | file_open.png -> `OpenMap`  (radiant/mainframe.cpp:1328)
- [ ] button Save the active map | file_save.png -> `SaveMap`  (radiant/mainframe.cpp:1329)

### UndoRedo_constructToolbar (radiant/mainframe.cpp)

- [x] button Undo | undo.png -> `Undo`  (radiant/mainframe.cpp:1333)
- [x] button Redo | redo.png -> `Redo`  (radiant/mainframe.cpp:1334)

### RotateFlip_constructToolbar (radiant/mainframe.cpp)

- [x] button Flip Horizontally | brush_flip_hor.png -> `MirrorSelectionHorizontally`  (radiant/mainframe.cpp:1344) -- Stufe 2 Punkt 5
- [x] button Flip Vertically | brush_flip_vert.png -> `MirrorSelectionVertically`  (radiant/mainframe.cpp:1345) -- Stufe 2 Punkt 5
- [x] button Rotate Anticlockwise | brush_rotate_anti.png -> `RotateSelectionAnticlockwise`  (radiant/mainframe.cpp:1347) -- Stufe 2 Punkt 5
- [x] button Rotate Clockwise | brush_rotate_clock.png -> `RotateSelectionClockwise`  (radiant/mainframe.cpp:1348) -- Stufe 2 Punkt 5

### Select_constructToolbar (radiant/mainframe.cpp)

- [~] button Select touching | selection_selecttouching.png -> `SelectTouching`  (radiant/mainframe.cpp:1352) -- Stufe 2
- [~] button Select inside | selection_selectinside.png -> `SelectInside`  (radiant/mainframe.cpp:1353) -- Stufe 2

### CSG_constructToolbar (radiant/mainframe.cpp)

- [x] button CSG Subtract | selection_csgsubtract.png -> `CSGSubtract`  (radiant/mainframe.cpp:1357) -- Stufe 2 Punkt 3
- [x] button CSG Wrap Merge | selection_csgmerge.png -> `CSGWrapMerge`  (radiant/mainframe.cpp:1358) -- Stufe 2 Punkt 3
- [x] button Room | selection_makeroom.png -> `CSGroom`  (radiant/mainframe.cpp:1359) -- Stufe 2 Punkt 3
- [x] button CSG Tool | ellipsis.png -> `CSGTool`  (radiant/mainframe.cpp:1360) -- Stufe 2 Punkt 3

### ComponentModes_constructToolbar (radiant/mainframe.cpp)

- [x] toggle_button Select Vertices | modify_vertices.png -> `DragVertices`  (radiant/mainframe.cpp:1364) -- Stufe 2 Punkt 1
- [x] toggle_button Select Edges | modify_edges.png -> `DragEdges`  (radiant/mainframe.cpp:1365) -- Stufe 2 Punkt 1
- [x] toggle_button Select Faces | modify_faces.png -> `DragFaces`  (radiant/mainframe.cpp:1366) -- Stufe 2 Punkt 1

### XYWnd_constructToolbar (radiant/mainframe.cpp)

- [ ] button Change views | view_change.png -> `NextView`  (radiant/mainframe.cpp:1370)

### Manipulators_constructToolbar (radiant/mainframe.cpp)

- [x] toggle_button Resize (Q) | select_mouseresize.png -> `MouseDrag`  (radiant/mainframe.cpp:1374)
- [x] toggle_button Clipper | select_clipper.png -> `ToggleClipper`  (radiant/mainframe.cpp:1375) -- Stufe 2 Punkt 2
- [x] toggle_button Translate | select_mousetranslate.png -> `MouseTranslate`  (radiant/mainframe.cpp:1376)
- [x] toggle_button Rotate | select_mouserotate.png -> `MouseRotate`  (radiant/mainframe.cpp:1377)
- [x] toggle_button Scale | select_mousescale.png -> `MouseScale`  (radiant/mainframe.cpp:1378)
- [x] toggle_button Transform (Q) | select_mousetransform.png -> `MouseTransform`  (radiant/mainframe.cpp:1379)
- [x] toggle_button UV Tool | select_mouseuv.png -> `MouseUV`  (radiant/mainframe.cpp:1381) -- Stufe 2 Punkt 1

### create_main_toolbar (radiant/mainframe.cpp)

- [x] toggle_button Texture Lock | texture_lock.png -> `TogTexLock`  (radiant/mainframe.cpp:1424) -- Stufe 2 Punkt 1
- [x] toggle_button Texture Vertex Lock | texture_vertexlock.png -> `TogTexVertexLock`  (radiant/mainframe.cpp:1425) -- Stufe 2 Punkt 1
- [x] button Entities | entities.png -> `ToggleEntityInspector`  (radiant/mainframe.cpp:1428) -- Stufe 1b Punkt 9
- [x] button Console [wenn: if ( style != MainFrame::eRegular && style != MainFrame::eRegularLeft )] | console.png -> `ToggleConsole`  (radiant/mainframe.cpp:1431) -- Stufe 1b Punkt 10
- [ ] button Texture Browser [wenn: if ( ( style != MainFrame::eRegular && style != MainFrame::eRegularLeft ) || g_Layout_builtInGroupDialog.m_value )] | texture_browser.png -> `ToggleTextures`  (radiant/mainframe.cpp:1434)
- [~] button Refresh Models | refresh_models.png -> `RefreshReferences`  (radiant/mainframe.cpp:1441) -- Stufe 2

### Patch_constructToolbar (radiant/patchmanip.cpp)

- [x] button Put caps on the current patch | curve_cap.png -> `CapCurrentCurve`  (radiant/patchmanip.cpp:735) -- Stufe 2 Punkt 6

### TextureBrowser_filterSetModeIcon (radiant/texwindow.cpp)

- [x] button Find / Replace... | texbro_find-replace.png -> `FindReplaceTextures`  (radiant/texwindow.cpp:1798)
- [x] button Flush & Reload Shaders | texbro_refresh.png -> `RefreshShaders`  (radiant/texwindow.cpp:1800)

## Registrierte Befehle

Name | Art | Standardtaste | Quelle

- [x] `ArbitraryRotation` | Commands | Shift+R | (radiant/select.cpp:1883) -- Stufe 2 Punkt 5
- [x] `ArbitraryScale` | Commands | Ctrl+Shift+S | (radiant/select.cpp:1884) -- Stufe 2 Punkt 5
- [~] `AutoCaulkSelected` | Commands | F4 | (radiant/map.cpp:2442) -- Stufe 3
- [~] `bobToolz::About` | Commands | - | (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [~] `bobToolz::ApertureDoor` | Commands | - | (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [~] `bobToolz::BrushCleanup` | Commands | - | (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [~] `bobToolz::CaulkSelection` | Commands | - | (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [~] `bobToolz::DoorBuilder` | Commands | - | (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [~] `bobToolz::DropEntity` | Commands | - | (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [~] `bobToolz::ExplodeEntity` | Commands | - | (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [~] `bobToolz::FindDuplicates` | Commands | - | (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [~] `bobToolz::FlipTerrain` | Commands | - | (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [~] `bobToolz::Intersect` | Commands | - | (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [~] `bobToolz::MakeChain` | Commands | - | (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [~] `bobToolz::MergePatches` | Commands | - | (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [~] `bobToolz::PathPlotter` | Commands | - | (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [~] `bobToolz::PitOMatic` | Commands | - | (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [~] `bobToolz::PlotSplines` | Commands | - | (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [~] `bobToolz::PolygonBuilder` | Commands | - | (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [~] `bobToolz::ResetTextures` | Commands | - | (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [~] `bobToolz::SplitPatch` | Commands | - | (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [~] `bobToolz::SplitPatchColumns` | Commands | - | (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [~] `bobToolz::SplitPatchRows` | Commands | - | (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [~] `bobToolz::StairBuilder` | Commands | - | (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [~] `bobToolz::TreePlanter` | Commands | - | (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [~] `bobToolz::VisViewer` | Commands | - | (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [x] `Brush3Sided` | Commands | Ctrl+3 | (radiant/brushmanip.cpp:1485) -- Stufe 2 Punkt 4
- [x] `Brush4Sided` | Commands | Ctrl+4 | (radiant/brushmanip.cpp:1486) -- Stufe 2 Punkt 4
- [x] `Brush5Sided` | Commands | Ctrl+5 | (radiant/brushmanip.cpp:1487) -- Stufe 2 Punkt 4
- [x] `Brush6Sided` | Commands | Ctrl+6 | (radiant/brushmanip.cpp:1488) -- Stufe 2 Punkt 4
- [x] `Brush7Sided` | Commands | Ctrl+7 | (radiant/brushmanip.cpp:1489) -- Stufe 2 Punkt 4
- [x] `Brush8Sided` | Commands | Ctrl+8 | (radiant/brushmanip.cpp:1490) -- Stufe 2 Punkt 4
- [x] `Brush9Sided` | Commands | Ctrl+9 | (radiant/brushmanip.cpp:1491) -- Stufe 2 Punkt 4
- [x] `BrushCone` | Commands | - | (radiant/brushmanip.cpp:1480) -- Stufe 2 Punkt 4
- [~] `brushexport2::About` | Commands | - | (contrib/brushexport/plugin.cpp:56) -- Stufe 3
- [~] `brushexport2::Export.obj` | Commands | - | (contrib/brushexport/plugin.cpp:56) -- Stufe 3
- [x] `BrushIcosahedron` | Commands | - | (radiant/brushmanip.cpp:1483) -- Stufe 2 Punkt 4
- [x] `BrushPrism` | Commands | - | (radiant/brushmanip.cpp:1479) -- Stufe 2 Punkt 4
- [x] `BrushRock` | Commands | - | (radiant/brushmanip.cpp:1482) -- Stufe 2 Punkt 4
- [x] `BrushSphere` | Commands | - | (radiant/brushmanip.cpp:1481) -- Stufe 2 Punkt 4
- [~] `Build_runRecentExecutedBuild` | Commands | F5 | (radiant/mainframe.cpp:2090) -- Stufe 3
- [~] `BuildMenuCustomize` | Commands | - | (radiant/mainframe.cpp:2089) -- Stufe 3
- [x] `CameraAngleDown` | Commands | - | (radiant/camwindow.cpp:1432)
- [x] `CameraAngleDown` | KeyEvents | - | (radiant/camwindow.cpp:1369)
- [x] `CameraAngleDown` | Shortcuts | - | (radiant/camwindow.cpp:2522)
- [x] `CameraAngleUp` | Commands | - | (radiant/camwindow.cpp:1431)
- [x] `CameraAngleUp` | KeyEvents | - | (radiant/camwindow.cpp:1365)
- [x] `CameraAngleUp` | Shortcuts | - | (radiant/camwindow.cpp:2521)
- [x] `CameraBack` | Commands | - | (radiant/camwindow.cpp:1423)
- [x] `CameraBack` | KeyEvents | - | (radiant/camwindow.cpp:1337)
- [x] `CameraBack` | Shortcuts | Down | (radiant/camwindow.cpp:2513)
- [x] `CameraDown` | Commands | - | (radiant/camwindow.cpp:1430)
- [x] `CameraDown` | KeyEvents | - | (radiant/camwindow.cpp:1361)
- [x] `CameraDown` | Shortcuts | - | (radiant/camwindow.cpp:2520)
- [x] `CameraFocusOnSelected` | Commands | Tab | (radiant/camwindow.cpp:2489)
- [x] `CameraForward` | Commands | - | (radiant/camwindow.cpp:1422)
- [x] `CameraForward` | KeyEvents | - | (radiant/camwindow.cpp:1333)
- [x] `CameraForward` | Shortcuts | Up | (radiant/camwindow.cpp:2512)
- [x] `CameraFreeFocus` | KeyEvents | - | (radiant/camwindow.cpp:1417)
- [x] `CameraFreeFocus` | Shortcuts | Tab | (radiant/camwindow.cpp:2537)
- [x] `CameraFreeMoveBack` | KeyEvents | - | (radiant/camwindow.cpp:1378)
- [x] `CameraFreeMoveBack` | Shortcuts | S | (radiant/camwindow.cpp:2525)
- [x] `CameraFreeMoveBack2` | KeyEvents | - | (radiant/camwindow.cpp:1395)
- [x] `CameraFreeMoveBack2` | Shortcuts | Down | (radiant/camwindow.cpp:2530)
- [x] `CameraFreeMoveDown` | KeyEvents | - | (radiant/camwindow.cpp:1412)
- [x] `CameraFreeMoveDown` | Shortcuts | - | (radiant/camwindow.cpp:2535)
- [x] `CameraFreeMoveForward` | KeyEvents | - | (radiant/camwindow.cpp:1374)
- [x] `CameraFreeMoveForward` | Shortcuts | W | (radiant/camwindow.cpp:2524)
- [x] `CameraFreeMoveForward2` | KeyEvents | - | (radiant/camwindow.cpp:1391)
- [x] `CameraFreeMoveForward2` | Shortcuts | Up | (radiant/camwindow.cpp:2529)
- [x] `CameraFreeMoveLeft` | KeyEvents | - | (radiant/camwindow.cpp:1382)
- [x] `CameraFreeMoveLeft` | Shortcuts | A | (radiant/camwindow.cpp:2526)
- [x] `CameraFreeMoveLeft2` | KeyEvents | - | (radiant/camwindow.cpp:1399)
- [x] `CameraFreeMoveLeft2` | Shortcuts | Left | (radiant/camwindow.cpp:2531)
- [x] `CameraFreeMoveRight` | KeyEvents | - | (radiant/camwindow.cpp:1386)
- [x] `CameraFreeMoveRight` | Shortcuts | D | (radiant/camwindow.cpp:2527)
- [x] `CameraFreeMoveRight2` | KeyEvents | - | (radiant/camwindow.cpp:1403)
- [x] `CameraFreeMoveRight2` | Shortcuts | Right | (radiant/camwindow.cpp:2532)
- [x] `CameraFreeMoveUp` | KeyEvents | - | (radiant/camwindow.cpp:1408)
- [x] `CameraFreeMoveUp` | Shortcuts | - | (radiant/camwindow.cpp:2534)
- [x] `CameraLeft` | Commands | - | (radiant/camwindow.cpp:1424)
- [x] `CameraLeft` | KeyEvents | - | (radiant/camwindow.cpp:1341)
- [x] `CameraLeft` | Shortcuts | Left | (radiant/camwindow.cpp:2514)
- [x] `CameraModeNext` | Commands | Shift+] | (radiant/camwindow.cpp:2506)
- [x] `CameraModePrev` | Commands | Shift+[ | (radiant/camwindow.cpp:2507)
- [x] `CameraRight` | Commands | - | (radiant/camwindow.cpp:1425)
- [x] `CameraRight` | KeyEvents | - | (radiant/camwindow.cpp:1345)
- [x] `CameraRight` | Shortcuts | Right | (radiant/camwindow.cpp:2515)
- [x] `CameraSpeedDec` | Commands | +Qt::SHIFT + Qt::Key_Minus + Qt::KeypadModifier | (radiant/camwindow.cpp:2510)
- [x] `CameraSpeedInc` | Commands | +Qt::SHIFT + Qt::Key_Plus + Qt::KeypadModifier | (radiant/camwindow.cpp:2509)
- [x] `CameraStrafeLeft` | Commands | - | (radiant/camwindow.cpp:1427)
- [x] `CameraStrafeLeft` | KeyEvents | - | (radiant/camwindow.cpp:1353)
- [x] `CameraStrafeLeft` | Shortcuts | A | (radiant/camwindow.cpp:2517)
- [x] `CameraStrafeRight` | Commands | - | (radiant/camwindow.cpp:1426)
- [x] `CameraStrafeRight` | KeyEvents | - | (radiant/camwindow.cpp:1349)
- [x] `CameraStrafeRight` | Shortcuts | D | (radiant/camwindow.cpp:2516)
- [x] `CameraUp` | Commands | - | (radiant/camwindow.cpp:1429)
- [x] `CameraUp` | KeyEvents | - | (radiant/camwindow.cpp:1357)
- [x] `CameraUp` | Shortcuts | - | (radiant/camwindow.cpp:2519)
- [x] `CapCurrentCurve` | Commands | Shift+C | (radiant/patchmanip.cpp:727) -- Stufe 2 Punkt 6
- [x] `CenterView` | Commands | End | (radiant/camwindow.cpp:2488)
- [x] `CenterXYView` | Commands | Ctrl+Shift+Tab | (radiant/xywindow.cpp:2277) -- Stufe 1b Punkt 5
- [~] `CheckForUpdate` | Commands | - | (radiant/mainframe.cpp:2072) -- Stufe 3
- [x] `ClipperClip` | Commands | Return | (radiant/clippertool.cpp:194) -- Stufe 2 Punkt 2
- [x] `ClipperFlip` | Commands | Ctrl+Return | (radiant/clippertool.cpp:196) -- Stufe 2 Punkt 2
- [x] `ClipperSplit` | Commands | Shift+Return | (radiant/clippertool.cpp:195) -- Stufe 2 Punkt 2
- [x] `CloneSelection` | Commands | Space | (radiant/select.cpp:1866)
- [x] `CloneSelectionAndMakeUnique` | Commands | Shift+Space | (radiant/select.cpp:1867)
- [x] `Copy` | Commands | Ctrl+C | (radiant/select.cpp:1862)
- [x] `CSGIntersect` | Commands | Ctrl+Shift+U | (radiant/csg.cpp:1595) -- Stufe 2 Punkt 3
- [x] `CSGMerge` | Commands | - | (radiant/csg.cpp:1593) -- Stufe 2 Punkt 3
- [x] `CSGroom` | Commands | - | (radiant/csg.cpp:1596) -- Stufe 2 Punkt 3
- [x] `CSGSubtract` | Commands | Shift+U | (radiant/csg.cpp:1592) -- Stufe 2 Punkt 3
- [x] `CSGTool` | Commands | - | (radiant/csg.cpp:1597) -- Stufe 2 Punkt 3
- [x] `CSGWrapMerge` | Commands | Ctrl+U | (radiant/csg.cpp:1594) -- Stufe 2 Punkt 3
- [x] `CubicClipZoomIn` | Commands | Ctrl+[ | (radiant/camwindow.cpp:2492)
- [x] `CubicClipZoomOut` | Commands | Ctrl+] | (radiant/camwindow.cpp:2493)
- [x] `DeleteSelection` | Commands | Z | (radiant/select.cpp:1870)
- [x] `DeleteSelection2` | Commands | Backspace | (radiant/select.cpp:1869)
- [x] `DeleteSelection3` | Commands | Delete | (radiant/select.cpp:1868)
- [x] `DownFloor` | Commands | PgDown | (radiant/camwindow.cpp:2496)
- [x] `DragEdges` | Toggles | E | (radiant/tools.cpp:423) -- Stufe 2 Punkt 1
- [x] `DragFaces` | Toggles | F | (radiant/tools.cpp:424) -- Stufe 2 Punkt 1
- [x] `DragVertices` | Toggles | V | (radiant/tools.cpp:422) -- Stufe 2 Punkt 1
- [x] `EnableAlpha` | Toggles | - | (radiant/texwindow.cpp:2109)
- [x] `EntitiesConnect` | Commands | Ctrl+K | (radiant/entity.cpp:734) -- Stufe 1b Punkt 9
- [x] `EntityColorNormalize` | Commands | - | (radiant/entity.cpp:733) -- Stufe 1b Punkt 9
- [x] `EntityColorSet` | Commands | K | (radiant/entity.cpp:732) -- Stufe 1b Punkt 9
- [x] `EntityMovePrimitivesToFirst` | Commands | - | (radiant/entity.cpp:738) -- Stufe 1b Punkt 9
- [x] `EntityMovePrimitivesToLast` | Commands | Ctrl+M | (radiant/entity.cpp:737) -- Stufe 1b Punkt 9
- [x] `EntityReloadDefinitions` | Commands | - | (radiant/entity.cpp:741) -- Stufe 1b Punkt 9
- [x] `EntityUngroup` | Commands | - | (radiant/entity.cpp:739) -- Stufe 1b Punkt 9
- [x] `EntityUngroupPrimitives` | Commands | - | (radiant/entity.cpp:740) -- Stufe 1b Punkt 9
- [ ] `Exit` | Commands | - | (radiant/mainframe.cpp:2073)
- [x] `ExpandSelectionToEntities` | Commands | Shift+E | (radiant/select.cpp:1880) -- Stufe 1b Punkt 9
- [~] `ExpandSelectionToLayers` | Commands | Ctrl+Q | (radiant/layerswindow.cpp:580) -- Stufe 2
- [~] `ExpandSelectionToPrimitives` | Commands | Ctrl+E | (radiant/select.cpp:1879) -- Stufe 2
- [~] `FilterAreaportals` | Toggles | Alt+3 | (radiant/filters.cpp:220) -- Stufe 2
- [~] `FilterBotClips` | Toggles | Alt+M | (radiant/filters.cpp:241) -- Stufe 2
- [~] `FilterCaulk` | Toggles | Alt+6 | (radiant/filters.cpp:224) -- Stufe 2
- [~] `FilterClips` | Toggles | Alt+7 | (radiant/filters.cpp:225) -- Stufe 2
- [~] `FilterClusterportals` | Toggles | Alt+9 | (radiant/filters.cpp:228) -- Stufe 2
- [~] `FilterDecals` | Toggles | Shift+D | (radiant/filters.cpp:242) -- Stufe 2
- [~] `FilterDetails` | Toggles | Ctrl+D | (radiant/filters.cpp:236) -- Stufe 2
- [~] `FilterEntities` | Toggles | Alt+2 | (radiant/filters.cpp:214) -- Stufe 2
- [~] `FilterFuncGroups` | Toggles | - | (radiant/filters.cpp:244) -- Stufe 2
- [~] `FilterHintsSkips` | Toggles | Ctrl+H | (radiant/filters.cpp:237) -- Stufe 2
- [~] `FilterLightgrid` | Toggles | - | (radiant/filters.cpp:233) -- Stufe 2
- [~] `FilterLights` | Toggles | Alt+0 | (radiant/filters.cpp:230) -- Stufe 2
- [~] `FilterLiquids` | Toggles | Alt+5 | (radiant/filters.cpp:223) -- Stufe 2
- [~] `FilterModels` | Toggles | Shift+M | (radiant/filters.cpp:238) -- Stufe 2
- [x] `FilterNotex` | Toggles | - | (radiant/texwindow.cpp:2108)
- [~] `FilterPatches` | Toggles | Ctrl+P | (radiant/filters.cpp:235) -- Stufe 2
- [~] `FilterPaths` | Toggles | Alt+8 | (radiant/filters.cpp:226) -- Stufe 2
- [~] `FilterPointEntities` | Toggles | - | (radiant/filters.cpp:245) -- Stufe 2
- [~] `FilterSky` | Toggles | - | (radiant/filters.cpp:246) -- Stufe 2
- [~] `FilterStructural` | Toggles | Ctrl+Shift+D | (radiant/filters.cpp:231) -- Stufe 2
- [~] `FilterTranslucent` | Toggles | Alt+4 | (radiant/filters.cpp:222) -- Stufe 2
- [~] `FilterTriggers` | Toggles | Ctrl+Shift+T | (radiant/filters.cpp:239) -- Stufe 2
- [~] `FilterWorldBrushes` | Toggles | Alt+1 | (radiant/filters.cpp:213) -- Stufe 2
- [~] `FindBrush` | Commands | - | (radiant/map.cpp:2444) -- Stufe 2
- [x] `FindReplaceTextures` | Commands | - | (radiant/findtexturedialog.cpp:229)
- [~] `FitTexture` | Commands | Ctrl+F | (radiant/surfacedialog.cpp:1747) -- Stufe 2
- [~] `FitTextureHeight` | Commands | - | (radiant/surfacedialog.cpp:1749) -- Stufe 2
- [~] `FitTextureHeightOnly` | Commands | - | (radiant/surfacedialog.cpp:1751) -- Stufe 2
- [~] `FitTextureWidth` | Commands | - | (radiant/surfacedialog.cpp:1748) -- Stufe 2
- [~] `FitTextureWidthOnly` | Commands | - | (radiant/surfacedialog.cpp:1750) -- Stufe 2
- [x] `FixedSize` | Toggles | - | (radiant/texwindow.cpp:2107)
- [~] `Fullscreen` | Commands | F11 | (radiant/mainframe.cpp:2096) -- Stufe 2
- [x] `GridDown` | Commands | [ | (radiant/grid.cpp:184)
- [x] `GridUp` | Commands | ] | (radiant/grid.cpp:185)
- [x] `HideSelected` | Toggles | H | (radiant/select.cpp:1842)
- [~] `ImportMap` | Commands | - | (radiant/map.cpp:2431) -- Stufe 2
- [x] `InvertCurve` | Commands | Ctrl+I | (radiant/patchmanip.cpp:719) -- Stufe 2 Punkt 6
- [x] `InvertCurveTextureX` | Commands | Ctrl+Shift+I | (radiant/patchmanip.cpp:694) -- Stufe 2 Punkt 6
- [x] `InvertCurveTextureY` | Commands | Shift+I | (radiant/patchmanip.cpp:695) -- Stufe 2 Punkt 6
- [~] `InvertFilters` | Commands | - | (radiant/filters.cpp:210) -- Stufe 2
- [x] `InvertSelection` | Commands | I | (radiant/select.cpp:1876)
- [~] `LayersMenu` | Commands | Shift+Q | (radiant/layerswindow.cpp:581) -- Stufe 2
- [x] `MakeDetail` | Commands | Alt+D | (radiant/brushmanip.cpp:1493) -- Stufe 2 Punkt 3
- [x] `MakeStructural` | Commands | Alt+S | (radiant/brushmanip.cpp:1494) -- Stufe 2 Punkt 3
- [~] `MapInfo` | Commands | M | (radiant/map.cpp:2445) -- Stufe 2
- [x] `MatrixTranspose` | Commands | Ctrl+Shift+M | (radiant/patchmanip.cpp:726) -- Stufe 2 Punkt 6
- [x] `MaximizeView` | Commands | F12 | (radiant/mainframe.cpp:2097)
- [~] `meshTex::About` | Commands | - | (contrib/meshtex/MainMenu.cpp:76) -- Stufe 3
- [~] `meshTex::GeneralFunction` | Commands | - | (contrib/meshtex/MainMenu.cpp:76) -- Stufe 3
- [~] `meshTex::GetInfo` | Commands | - | (contrib/meshtex/MainMenu.cpp:76) -- Stufe 3
- [~] `meshTex::Help` | Commands | - | (contrib/meshtex/MainMenu.cpp:76) -- Stufe 3
- [~] `meshTex::SetScale` | Commands | - | (contrib/meshtex/MainMenu.cpp:76) -- Stufe 3
- [~] `meshTex::SMaxAlign` | Commands | - | (contrib/meshtex/MainMenu.cpp:76) -- Stufe 3
- [~] `meshTex::SMinAlign` | Commands | - | (contrib/meshtex/MainMenu.cpp:76) -- Stufe 3
- [~] `meshTex::SMinMaxAlignAuto` | Commands | - | (contrib/meshtex/MainMenu.cpp:76) -- Stufe 3
- [~] `meshTex::SMinMaxAlignShrink` | Commands | - | (contrib/meshtex/MainMenu.cpp:76) -- Stufe 3
- [~] `meshTex::SMinMaxAlignStretch` | Commands | - | (contrib/meshtex/MainMenu.cpp:76) -- Stufe 3
- [~] `meshTex::STMinMaxAlignAuto` | Commands | - | (contrib/meshtex/MainMenu.cpp:76) -- Stufe 3
- [~] `meshTex::TMaxAlign` | Commands | - | (contrib/meshtex/MainMenu.cpp:76) -- Stufe 3
- [~] `meshTex::TMinAlign` | Commands | - | (contrib/meshtex/MainMenu.cpp:76) -- Stufe 3
- [~] `meshTex::TMinMaxAlignAuto` | Commands | - | (contrib/meshtex/MainMenu.cpp:76) -- Stufe 3
- [~] `meshTex::TMinMaxAlignShrink` | Commands | - | (contrib/meshtex/MainMenu.cpp:76) -- Stufe 3
- [~] `meshTex::TMinMaxAlignStretch` | Commands | - | (contrib/meshtex/MainMenu.cpp:76) -- Stufe 3
- [x] `MirrorSelectionHorizontally` | Commands | - | (radiant/select.cpp:1851) -- Stufe 2 Punkt 5
- [x] `MirrorSelectionVertically` | Commands | - | (radiant/select.cpp:1852) -- Stufe 2 Punkt 5
- [x] `MirrorSelectionX` | Commands | - | (radiant/select.cpp:1844) -- Stufe 2 Punkt 5
- [x] `MirrorSelectionY` | Commands | - | (radiant/select.cpp:1846) -- Stufe 2 Punkt 5
- [x] `MirrorSelectionZ` | Commands | - | (radiant/select.cpp:1848) -- Stufe 2 Punkt 5
- [x] `MouseBuild` | Toggles | B | (radiant/tools.cpp:433) -- Stufe 2 Punkt 1
- [x] `MouseDrag` | Toggles | - | (radiant/tools.cpp:432)
- [x] `MouseDragOrTransform` | Commands | Q | (radiant/tools.cpp:436)
- [x] `MouseRotate` | Toggles | R | (radiant/tools.cpp:429)
- [x] `MouseRotateOrScale` | Commands | - | (radiant/tools.cpp:435)
- [x] `MouseScale` | Toggles | - | (radiant/tools.cpp:430)
- [x] `MouseTransform` | Toggles | - | (radiant/tools.cpp:431)
- [x] `MouseTranslate` | Toggles | W | (radiant/tools.cpp:428)
- [x] `MouseUV` | Toggles | G | (radiant/tools.cpp:434) -- Stufe 2 Punkt 1
- [x] `MoveSelectionDOWN` | Commands | +Qt::Key_Minus + Qt::KeypadModifier | (radiant/select.cpp:1901)
- [x] `MoveSelectionUP` | Commands | +Qt::Key_Plus + Qt::KeypadModifier | (radiant/select.cpp:1902)
- [x] `MoveToCamera` | Commands | Ctrl+Shift+V | (radiant/select.cpp:1865)
- [x] `NaturalizePatch` | Commands | Ctrl+N | (radiant/patchmanip.cpp:696) -- Stufe 2 Punkt 6
- [x] `NewMap` | Commands | - | (radiant/map.cpp:2429)
- [~] `NextLeakSpot` | Commands | Ctrl+Shift+K | (radiant/points.cpp:321) -- Stufe 3
- [x] `NextView` | Commands | Ctrl+Tab | (radiant/xywindow.cpp:2270) -- Stufe 1b Punkt 5
- [~] `OpenGLFont` | Commands | - | (radiant/mainframe.cpp:2092) -- Stufe 2
- [~] `OpenManual` | Commands | F1 | (radiant/mainframe.cpp:2069) -- Stufe 3
- [x] `OpenMap` | Commands | Ctrl+O | (radiant/map.cpp:2430)
- [x] `Paste` | Commands | Ctrl+V | (radiant/select.cpp:1863)
- [x] `PasteToCamera` | Commands | Shift+V | (radiant/select.cpp:1864)
- [x] `PatchBevel` | Commands | - | (radiant/patchmanip.cpp:705) -- Stufe 2 Punkt 6
- [x] `PatchCone` | Commands | - | (radiant/patchmanip.cpp:708) -- Stufe 2 Punkt 6
- [x] `PatchCylinder` | Commands | - | (radiant/patchmanip.cpp:697) -- Stufe 2 Punkt 6
- [x] `PatchDeform` | Commands | - | (radiant/patchmanip.cpp:730) -- Stufe 2 Punkt 6
- [x] `PatchDeleteFirstColumn` | Commands | - | (radiant/patchmanip.cpp:715) -- Stufe 2 Punkt 6
- [x] `PatchDeleteFirstRow` | Commands | - | (radiant/patchmanip.cpp:717) -- Stufe 2 Punkt 6
- [x] `PatchDeleteLastColumn` | Commands | Qt::CTRL + Qt::SHIFT + Qt::Key_Minus + Qt::KeypadModifier | (radiant/patchmanip.cpp:716) -- Stufe 2 Punkt 6
- [x] `PatchDeleteLastRow` | Commands | +Qt::CTRL + Qt::Key_Minus + Qt::KeypadModifier | (radiant/patchmanip.cpp:718) -- Stufe 2 Punkt 6
- [x] `PatchEndCap` | Commands | - | (radiant/patchmanip.cpp:704) -- Stufe 2 Punkt 6
- [x] `PatchInsertFirstColumn` | Commands | Qt::CTRL + Qt::SHIFT + Qt::Key_Plus + Qt::KeypadModifier | (radiant/patchmanip.cpp:711) -- Stufe 2 Punkt 6
- [x] `PatchInsertFirstRow` | Commands | +Qt::CTRL + Qt::Key_Plus + Qt::KeypadModifier | (radiant/patchmanip.cpp:713) -- Stufe 2 Punkt 6
- [x] `PatchInsertLastColumn` | Commands | - | (radiant/patchmanip.cpp:712) -- Stufe 2 Punkt 6
- [x] `PatchInsertLastRow` | Commands | - | (radiant/patchmanip.cpp:714) -- Stufe 2 Punkt 6
- [x] `PatchSphere` | Commands | - | (radiant/patchmanip.cpp:709) -- Stufe 2 Punkt 6
- [x] `PatchSquareCylinder` | Commands | - | (radiant/patchmanip.cpp:700) -- Stufe 2 Punkt 6
- [x] `PatchThicken` | Commands | Ctrl+T | (radiant/patchmanip.cpp:731) -- Stufe 2 Punkt 6
- [x] `PatchXactCone` | Commands | - | (radiant/patchmanip.cpp:703) -- Stufe 2 Punkt 6
- [x] `PatchXactCylinder` | Commands | - | (radiant/patchmanip.cpp:701) -- Stufe 2 Punkt 6
- [x] `PatchXactSphere` | Commands | - | (radiant/patchmanip.cpp:702) -- Stufe 2 Punkt 6
- [x] `Preferences` | Commands | P | (radiant/mainframe.cpp:2076)
- [~] `PrevLeakSpot` | Commands | Ctrl+Shift+L | (radiant/points.cpp:322) -- Stufe 3
- [~] `prtview::AboutPortalViewer` | Commands | - | (contrib/prtview/prtview.cpp:164) -- Stufe 3
- [~] `prtview::ConfigurePortalViewer` | Commands | - | (contrib/prtview/prtview.cpp:164) -- Stufe 3
- [~] `prtview::Load.prtFile` | Commands | - | (contrib/prtview/prtview.cpp:164) -- Stufe 3
- [~] `prtview::TogglePortals(2D)` | Commands | - | (contrib/prtview/prtview.cpp:164) -- Stufe 3
- [~] `prtview::TogglePortals(3D)` | Commands | - | (contrib/prtview/prtview.cpp:164) -- Stufe 3
- [~] `prtview::Unload.prtFile` | Commands | - | (contrib/prtview/prtview.cpp:164) -- Stufe 3
- [x] `RedisperseCols` | Commands | - | (radiant/patchmanip.cpp:723) -- Stufe 2 Punkt 6
- [x] `RedisperseRows` | Commands | - | (radiant/patchmanip.cpp:721) -- Stufe 2 Punkt 6
- [x] `Redo` | Commands | Ctrl+Shift+Z | (radiant/select.cpp:1860)
- [x] `Redo2` | Commands | Ctrl+Y | (radiant/select.cpp:1861)
- [~] `RefreshReferences` | Commands | - | (radiant/mainframe.cpp:2071) -- Stufe 2
- [x] `RefreshShaders` | Commands | - | (radiant/texwindow.cpp:2100)
- [~] `RegionOff` | Commands | - | (radiant/map.cpp:2437) -- Stufe 2
- [~] `RegionSetBrush` | Commands | - | (radiant/map.cpp:2439) -- Stufe 2
- [~] `RegionSetSelection` | Toggles | Ctrl+Shift+R | (radiant/map.cpp:2441) -- Stufe 2
- [~] `RegionSetXY` | Commands | - | (radiant/map.cpp:2438) -- Stufe 2
- [x] `RepeatTransforms` | Commands | Ctrl+R | (radiant/select.cpp:1871) -- Stufe 2 Punkt 5
- [~] `ResetFilters` | Commands | - | (radiant/filters.cpp:211) -- Stufe 2
- [x] `ResetTransforms` | Commands | Alt+R | (radiant/select.cpp:1872) -- Stufe 2 Punkt 5
- [x] `RotateSelectionAnticlockwise` | Commands | - | (radiant/select.cpp:1855) -- Stufe 2 Punkt 5
- [x] `RotateSelectionClockwise` | Commands | - | (radiant/select.cpp:1854) -- Stufe 2 Punkt 5
- [x] `RotateSelectionX` | Commands | - | (radiant/select.cpp:1845) -- Stufe 2 Punkt 5
- [x] `RotateSelectionY` | Commands | - | (radiant/select.cpp:1847) -- Stufe 2 Punkt 5
- [x] `RotateSelectionZ` | Commands | - | (radiant/select.cpp:1849) -- Stufe 2 Punkt 5
- [x] `SaveMap` | Commands | Ctrl+S | (radiant/map.cpp:2432)
- [x] `SaveMapAs` | Commands | - | (radiant/map.cpp:2433)
- [~] `SaveRegion` | Commands | - | (radiant/map.cpp:2435) -- Stufe 2
- [~] `SaveSelected` | Commands | - | (radiant/map.cpp:2434) -- Stufe 2
- [x] `SearchFromStart` | Toggles | - | (radiant/texwindow.cpp:2111)
- [x] `SelectAllOfType` | Commands | Shift+A | (radiant/select.cpp:1888)
- [x] `SelectConnectedEntities` | Commands | Ctrl+Shift+E | (radiant/select.cpp:1881) -- Stufe 1b Punkt 9
- [~] `SelectInside` | Commands | - | (radiant/select.cpp:1877) -- Stufe 2
- [x] `SelectNudgeDown` | Commands | Alt+Down | (radiant/select.cpp:1907)
- [x] `SelectNudgeLeft` | Commands | Alt+Left | (radiant/select.cpp:1904)
- [x] `SelectNudgeRight` | Commands | Alt+Right | (radiant/select.cpp:1905)
- [x] `SelectNudgeUp` | Commands | Alt+Up | (radiant/select.cpp:1906)
- [~] `SelectTextured` | Commands | Ctrl+Shift+A | (radiant/select.cpp:1857) -- Stufe 2
- [~] `SelectTouching` | Commands | - | (radiant/select.cpp:1878) -- Stufe 2
- [x] `SetGrid0.125` | Commands | - | (radiant/grid.cpp:189)
- [x] `SetGrid0.25` | Commands | - | (radiant/grid.cpp:190)
- [x] `SetGrid0.5` | Commands | - | (radiant/grid.cpp:191)
- [x] `SetGrid1` | Commands | 1 | (radiant/grid.cpp:192)
- [x] `SetGrid1024` | Commands | - | (radiant/grid.cpp:202)
- [x] `SetGrid128` | Commands | 8 | (radiant/grid.cpp:199)
- [x] `SetGrid16` | Commands | 5 | (radiant/grid.cpp:196)
- [x] `SetGrid2` | Commands | 2 | (radiant/grid.cpp:193)
- [x] `SetGrid256` | Commands | 9 | (radiant/grid.cpp:200)
- [x] `SetGrid32` | Commands | 6 | (radiant/grid.cpp:197)
- [x] `SetGrid4` | Commands | 3 | (radiant/grid.cpp:194)
- [x] `SetGrid512` | Commands | - | (radiant/grid.cpp:201)
- [x] `SetGrid64` | Commands | 7 | (radiant/grid.cpp:198)
- [x] `SetGrid8` | Commands | 4 | (radiant/grid.cpp:195)
- [~] `shaderPlug::About` | Commands | - | (contrib/shaderplug/shaderplug.cpp:71) -- Stufe 3
- [~] `shaderPlug::CreateTagFile` | Commands | - | (contrib/shaderplug/shaderplug.cpp:71) -- Stufe 3
- [x] `Shortcuts` | Commands | Ctrl+Shift+P | (radiant/mainframe.cpp:2075)
- [x] `ShowAllTextures` | Commands | Ctrl+A | (radiant/texwindow.cpp:2102)
- [x] `ShowAngles` | Toggles | - | (radiant/xywindow.cpp:2218) -- Stufe 1b Punkt 5
- [x] `ShowAxes` | Toggles | - | (radiant/xywindow.cpp:2225) -- Stufe 1b Punkt 5
- [x] `ShowBboxes` | Toggles | - | (radiant/xywindow.cpp:2220)
- [x] `ShowBlocks` | Toggles | - | (radiant/xywindow.cpp:2222) -- Stufe 1b Punkt 5
- [x] `ShowConnections` | Toggles | - | (radiant/xywindow.cpp:2221) -- Stufe 1b Punkt 5
- [x] `ShowCoordinates` | Toggles | - | (radiant/xywindow.cpp:2223) -- Stufe 1b Punkt 5
- [x] `ShowHidden` | Commands | Shift+H | (radiant/select.cpp:1841)
- [x] `ShowInUse` | Toggles | U | (radiant/texwindow.cpp:2101)
- [x] `ShowLightRadiuses` | Toggles | - | (radiant/entity.cpp:743)
- [x] `ShowNames` | Toggles | - | (radiant/xywindow.cpp:2219) -- Stufe 1b Punkt 5
- [x] `ShowSize2d` | Toggles | J | (radiant/xywindow.cpp:2214) -- Stufe 1b Punkt 5
- [x] `ShowSize3d` | Toggles | - | (radiant/camwindow.cpp:2541)
- [x] `ShowStats` | Toggles | - | (radiant/camwindow.cpp:2539)
- [x] `ShowWindowOutline` | Toggles | - | (radiant/xywindow.cpp:2224) -- Stufe 1b Punkt 5
- [x] `ShowWorkzone2d` | Toggles | - | (radiant/xywindow.cpp:2226) -- Stufe 1b Punkt 5
- [x] `ShowWorkzone3d` | Toggles | - | (radiant/camwindow.cpp:2540)
- [x] `SimplePatchMesh` | Commands | Shift+P | (radiant/patchmanip.cpp:710) -- Stufe 2 Punkt 6
- [x] `SmoothCols` | Commands | Ctrl+Shift+W | (radiant/patchmanip.cpp:725) -- Stufe 2 Punkt 6
- [x] `SmoothRows` | Commands | Ctrl+W | (radiant/patchmanip.cpp:724) -- Stufe 2 Punkt 6
- [x] `SnapToGrid` | Commands | Ctrl+G | (radiant/select.cpp:1886)
- [~] `sunPlug::About` | Commands | - | (contrib/sunplug/sunplug.cpp:80) -- Stufe 3
- [~] `sunPlug::ET-MapCoordinator` | Commands | - | (contrib/sunplug/sunplug.cpp:80) -- Stufe 3
- [~] `SurfaceInspector` | Commands | S | (radiant/surfacedialog.cpp:1755) -- Stufe 2
- [~] `TagAdd` | Commands | - | (radiant/texwindow.cpp:2095) -- Stufe 2
- [~] `TagCopy` | Commands | - | (radiant/texwindow.cpp:2098) -- Stufe 2
- [~] `TagDelete` | Commands | - | (radiant/texwindow.cpp:2097) -- Stufe 2
- [~] `TagPaste` | Commands | - | (radiant/texwindow.cpp:2099) -- Stufe 2
- [~] `TagRename` | Commands | - | (radiant/texwindow.cpp:2096) -- Stufe 2
- [~] `TagSearch` | Commands | - | (radiant/texwindow.cpp:2094) -- Stufe 2
- [~] `TagSearchUntagged` | Commands | - | (radiant/texwindow.cpp:2093) -- Stufe 2
- [~] `TagsToggleGui` | Toggles | - | (radiant/texwindow.cpp:2110) -- Stufe 2
- [~] `TexRotateClock` | Commands | Shift+PgDown | (radiant/select.cpp:1890) -- Stufe 2
- [~] `TexRotateCounter` | Commands | Shift+PgUp | (radiant/select.cpp:1891) -- Stufe 2
- [~] `TexScaleDown` | Commands | Ctrl+Down | (radiant/select.cpp:1893) -- Stufe 2
- [~] `TexScaleLeft` | Commands | Ctrl+Left | (radiant/select.cpp:1894) -- Stufe 2
- [~] `TexScaleRight` | Commands | Ctrl+Right | (radiant/select.cpp:1895) -- Stufe 2
- [~] `TexScaleUp` | Commands | Ctrl+Up | (radiant/select.cpp:1892) -- Stufe 2
- [~] `TexShiftDown` | Commands | Shift+Down | (radiant/select.cpp:1897) -- Stufe 2
- [~] `TexShiftLeft` | Commands | Shift+Left | (radiant/select.cpp:1898) -- Stufe 2
- [~] `TexShiftRight` | Commands | Shift+Right | (radiant/select.cpp:1899) -- Stufe 2
- [~] `TexShiftUp` | Commands | Shift+Up | (radiant/select.cpp:1896) -- Stufe 2
- [~] `TextureProjectAxial` | Commands | - | (radiant/surfacedialog.cpp:1752) -- Stufe 2
- [~] `TextureProjectCam` | Commands | - | (radiant/surfacedialog.cpp:1754) -- Stufe 2
- [~] `TextureProjectOrtho` | Commands | - | (radiant/surfacedialog.cpp:1753) -- Stufe 2
- [x] `TextureReset/Cap` | Commands | Shift+N | (radiant/surfacedialog.cpp:1746) -- Stufe 2 Punkt 6
- [x] `ToggleCamera` | Toggles | Ctrl+Shift+C | (radiant/camwindow.cpp:2498)
- [x] `ToggleClipper` | Toggles | X | (radiant/tools.cpp:426) -- Stufe 2 Punkt 2
- [x] `ToggleConsole` | Commands | O | (radiant/mainframe.cpp:2078) -- Stufe 1b Punkt 10
- [x] `ToggleCrosshairs` | Toggles | Shift+X | (radiant/xywindow.cpp:2215) -- Stufe 1b Punkt 5
- [x] `ToggleCubicClip` | Toggles | Ctrl+\\ | (radiant/camwindow.cpp:2491)
- [x] `ToggleEntityInspector` | Commands | N | (radiant/mainframe.cpp:2079) -- Stufe 1b Punkt 9
- [x] `ToggleEntityList` | Commands | Shift+L | (radiant/mainframe.cpp:2082) -- Stufe 1b Punkt 9
- [x] `ToggleFrontView` | Toggles | - | (radiant/xywindow.cpp:2269)
- [x] `ToggleGrid` | Toggles | 0 | (radiant/xywindow.cpp:2216) -- Stufe 1b Punkt 5
- [x] `ToggleGridSnap` | Commands | - | (radiant/grid.cpp:187)
- [x] `ToggleLayersBrowser` | Commands | L | (radiant/mainframe.cpp:2081)
- [x] `ToggleModelBrowser` | Commands | / | (radiant/mainframe.cpp:2080)
- [~] `TogglePointfile` | Commands | - | (radiant/points.cpp:320) -- Stufe 3
- [x] `ToggleShowShaderlistOnly` | Toggles | - | (radiant/texwindow.cpp:2106)
- [x] `ToggleShowShaders` | Toggles | - | (radiant/texwindow.cpp:2104)
- [x] `ToggleShowTextures` | Toggles | - | (radiant/texwindow.cpp:2105)
- [x] `ToggleSideView` | Toggles | - | (radiant/xywindow.cpp:2268)
- [x] `ToggleTextures` | Commands | T | (radiant/texwindow.cpp:2103)
- [x] `ToggleView` | Toggles | - | (radiant/xywindow.cpp:2267)
- [x] `TogTexLock` | Toggles | Shift+T | (radiant/brushmanip.cpp:1476) -- Stufe 2 Punkt 1
- [x] `TogTexVertexLock` | Toggles | - | (radiant/brushmanip.cpp:1477) -- Stufe 2 Punkt 1
- [~] `uFO:AI::About` | Commands | - | (contrib/ufoaiplug/ufoai.cpp:69) -- Stufe 3
- [~] `uFO:AI::ActorClip` | Commands | - | (contrib/ufoaiplug/ufoai.cpp:69) -- Stufe 3
- [~] `uFO:AI::Level1` | Commands | - | (contrib/ufoaiplug/ufoai.cpp:69) -- Stufe 3
- [~] `uFO:AI::Level2` | Commands | - | (contrib/ufoaiplug/ufoai.cpp:69) -- Stufe 3
- [~] `uFO:AI::Level3` | Commands | - | (contrib/ufoaiplug/ufoai.cpp:69) -- Stufe 3
- [~] `uFO:AI::Level4` | Commands | - | (contrib/ufoaiplug/ufoai.cpp:69) -- Stufe 3
- [~] `uFO:AI::Level5` | Commands | - | (contrib/ufoaiplug/ufoai.cpp:69) -- Stufe 3
- [~] `uFO:AI::Level6` | Commands | - | (contrib/ufoaiplug/ufoai.cpp:69) -- Stufe 3
- [~] `uFO:AI::Level7` | Commands | - | (contrib/ufoaiplug/ufoai.cpp:69) -- Stufe 3
- [~] `uFO:AI::Level8` | Commands | - | (contrib/ufoaiplug/ufoai.cpp:69) -- Stufe 3
- [~] `uFO:AI::NoDraw` | Commands | - | (contrib/ufoaiplug/ufoai.cpp:69) -- Stufe 3
- [~] `uFO:AI::PerformCheck` | Commands | - | (contrib/ufoaiplug/ufoai.cpp:69) -- Stufe 3
- [~] `uFO:AI::StepOn` | Commands | - | (contrib/ufoaiplug/ufoai.cpp:69) -- Stufe 3
- [~] `uFO:AI::WeaponClip` | Commands | - | (contrib/ufoaiplug/ufoai.cpp:69) -- Stufe 3
- [~] `uFO:AI::Worldspawn` | Commands | - | (contrib/ufoaiplug/ufoai.cpp:69) -- Stufe 3
- [~] `uFO:AI::WorldspawnReset` | Commands | - | (contrib/ufoaiplug/ufoai.cpp:69) -- Stufe 3
- [x] `Undo` | Commands | Ctrl+Z | (radiant/select.cpp:1859)
- [x] `UnSelectSelection` | Commands | C | (radiant/select.cpp:1875)
- [x] `UnSelectSelection2` | Commands | Escape | (radiant/select.cpp:1874)
- [x] `UpFloor` | Commands | PgUp | (radiant/camwindow.cpp:2495)
- [x] `ViewFront` | Commands | +Qt::Key_1 + Qt::KeypadModifier | (radiant/xywindow.cpp:2274) -- Stufe 1b Punkt 5
- [x] `ViewSide` | Commands | +Qt::Key_3 + Qt::KeypadModifier | (radiant/xywindow.cpp:2275) -- Stufe 1b Punkt 5
- [x] `ViewTop` | Commands | +Qt::Key_7 + Qt::KeypadModifier | (radiant/xywindow.cpp:2273) -- Stufe 1b Punkt 5
- [x] `XYFocusOnSelected` | Commands | ` | (radiant/xywindow.cpp:2278) -- Stufe 1b Punkt 5
- [x] `Zoom100` | Commands | - | (radiant/xywindow.cpp:2276) -- Stufe 1b Punkt 5
- [x] `ZoomIn` | Commands | - | (radiant/xywindow.cpp:2271) -- Stufe 1b Punkt 5
- [x] `ZoomOut` | Commands | Insert | (radiant/xywindow.cpp:2272) -- Stufe 1b Punkt 5

## Dialoge und Fenster

- [ ] "Build Menu"  (radiant/build.cpp:1027)
- [x] "Mapped Commands"  (radiant/commands.cpp:333)
- [x] "CSG Tool"  (radiant/csg.cpp:1486) -- Stufe 2 Punkt 3
- [x] "Entity List"  (radiant/entitylist.cpp:279) -- Stufe 1b Punkt 9
- [ ] "Q3Map debug window"  (radiant/feedback.cpp:281)
- [x] "Find / Replace Texture(s)"  (radiant/findtexturedialog.cpp:132)
- [x] "Entities"  (radiant/groupdialog.cpp:81) -- Stufe 1b Punkt 9
- [x] "Arbitrary sides"  (radiant/gtkdlgs.cpp:274) -- Stufe 2 Punkt 4
- [ ] "About NetRadiant"  (radiant/gtkdlgs.cpp:333)
- [x] "Set Light intensity"  (radiant/gtkdlgs.cpp:415) -- Stufe 1b Punkt 9
- [ ] "Install Map Developer's Files"  (radiant/gtkdlgs.cpp:492)
- [ ] "Engine Path Configuration"  (radiant/mainframe.cpp:421)
- [x] "Camera"  (radiant/mainframe.cpp:1755)
- [ ] "Map Info"  (radiant/map.cpp:908)
- [ ] "Find Brush"  (radiant/map.cpp:2085)
- [x] "Patch density"  (radiant/patchmanip.cpp:839) -- Stufe 2 Punkt 6
- [x] "Patch deform"  (radiant/patchmanip.cpp:918) -- Stufe 2 Punkt 6
- [x] "Cap"  (radiant/patchmanip.cpp:949) -- Stufe 2 Punkt 6
- [x] "Patch thicken"  (radiant/patchmanip.cpp:994) -- Stufe 2 Punkt 6
- [ ] "Global Preferences"  (radiant/preferences.cpp:271)
- [x] "NetRadiant Preferences"  (radiant/preferences.cpp:584) -- Rahmen, Baum, alle 23 Seiten (12 noch Platzhalter, siehe Einstellungsseiten)
- [x] "Arbitrary rotation"  (radiant/select.cpp:1589) -- Stufe 2 Punkt 5
- [x] "Arbitrary scale"  (radiant/select.cpp:1681) -- Stufe 2 Punkt 5
- [ ] "Surface Inspector"  (radiant/surfacedialog.cpp:637)

## Einstellungsseiten

- [ ] Autosave: "Autosave Preferences"  (radiant/autosave.cpp:173)
- [ ] Brush: "Brush Settings"  (radiant/brushmodule.cpp:113)
- [x] Camera: "Camera View Preferences"  (radiant/camwindow.cpp:2461)
- [x] Clipper: "Clipper Tool Settings"  (radiant/clippertool.cpp:186) -- Stufe 2 Punkt 2
- [x] Entities: "Entity Display Preferences"  (radiant/entity.cpp:682) -- Stufe 1b Punkt 9
- [ ] Grid: "Grid Settings"  (radiant/grid.cpp:258)
- [ ] Paths: "Path Settings"  (radiant/mainframe.cpp:404)
- [x] Layout: "Layout Preferences"  (radiant/mainframe.cpp:2051)
- [ ] Model Browser: "Model Browser Preferences"  (radiant/modelwindow.cpp:1364)
- [x] Patches: "Patch Display Preferences"  (radiant/patchmanip.cpp:676) -- Stufe 2 Punkt 6
- [x] Selection: "Selection System Settings"  (radiant/selection.cpp:2328)
- [ ] Surface Inspector: "Surface Inspector Preferences"  (radiant/surfacedialog.cpp:1738)
- [ ] Textures: "Texture Settings"  (radiant/textures.cpp:831)
- [x] Texture Browser: "Texture Browser Preferences"  (radiant/texwindow.cpp:2075)
- [ ] Undo: "Undo Queue Settings"  (radiant/undo.cpp:403)
- [ ] Build: "Build Preferences"  (radiant/watchbsp.cpp:243)
- [ ] Orthographic: "Orthographic View Preferences"  (radiant/xywindow.cpp:2254)

## Einstellungsfelder

- [ ] CheckBox Enable Autosave  (radiant/autosave.cpp:167)
- [ ] Spinner Autosave Frequency (minutes)  (radiant/autosave.cpp:168)
- [ ] CheckBox Save Snapshots  (radiant/autosave.cpp:170)
- [ ] Spinner Default texture scale  (radiant/brushmodule.cpp:86)
- [ ] Combo New map Brush Type  (radiant/brushmodule.cpp:95)
- [ ] CheckBox Always use caulk for new brushes  (radiant/brushmodule.cpp:102)
- [ ] CheckBox Dangerous! / Snap plane points to integer grid (may break brushes)  (radiant/brushmodule.cpp:106)
- [x] Spinner Movement Speed  (radiant/camwindow.cpp:2399)
- [x] Spinner Time to Max Speed  (radiant/camwindow.cpp:2400)
- [x] Spinner Scroll Move Speed  (radiant/camwindow.cpp:2401)
- [x] Spinner Strafe Speed  (radiant/camwindow.cpp:2402)
- [x] Spinner Mouse Sensitivity  (radiant/camwindow.cpp:2403)
- [x] CheckBox Invert mouse vertical axis  (radiant/camwindow.cpp:2404)
- [x] CheckBox Zoom to Mouse pointer  (radiant/camwindow.cpp:2405)
- [x] CheckBox Discrete movement  (radiant/camwindow.cpp:2406)
- [x] CheckBox Enable far-clip plane  (radiant/camwindow.cpp:2411)
- [x] CheckBox Colorize selection  (radiant/camwindow.cpp:2416)
- [ ] CheckBox Selected faces wire  (radiant/camwindow.cpp:2421) -- Wirkung mit der Flaechenauswahl (Punkt 7)
- [x] Combo Render Mode / doom3  (radiant/camwindow.cpp:2428)
- [x] Combo MSAA  (radiant/camwindow.cpp:2438)
- [x] Combo Strafe Mode  (radiant/camwindow.cpp:2448)
- [x] Spinner Field Of View  (radiant/camwindow.cpp:2454)
- [x] CheckBox Caulk Clipper Cuts  (radiant/clippertool.cpp:170) -- Stufe 2 Punkt 2
- [x] CheckBox Reset Flipped State  (radiant/clippertool.cpp:171) -- Stufe 2 Punkt 2
- [x] CheckBox Reset Points on Split  (radiant/clippertool.cpp:172) -- Stufe 2 Punkt 2
- [x] CheckBox 2 Points in 2D Views  (radiant/clippertool.cpp:174) -- Stufe 2 Punkt 2
- [x] Radio On DoubleClick do:   (radiant/clippertool.cpp:177) -- Stufe 2 Punkt 2
- [x] Spinner Names Display Distance (3D)  (radiant/entity.cpp:669) -- Stufe 1b Punkt 9
- [x] Spinner Names Display Ratio (2D)  (radiant/entity.cpp:673) -- Stufe 1b Punkt 9
- [x] CheckBox Entity Names / = Targetnames  (radiant/entity.cpp:677) -- Stufe 1b Punkt 9
- [ ] Combo Default grid spacing  (radiant/grid.cpp:241)
- [ ] Combo Max grid coordinate  (radiant/grid.cpp:249)
- [ ] Combo Select mod  (radiant/gtkdlgs.cpp:229)
- [ ] Combo fs_game  (radiant/gtkdlgs.cpp:241)
- [ ] Combo Mapping mode  (radiant/gtkdlgs.cpp:260)
- [ ] PathEntry Engine Path  (radiant/mainframe.cpp:398)
- [ ] PathEntry Extra Resource Path  (radiant/mainframe.cpp:407)
- [x] RadioIcons Window Layout  (radiant/mainframe.cpp:2031)
- [~] CheckBox Detachable Menus  (radiant/mainframe.cpp:2038) -- Stufe 2 (ImGui-Menues lassen sich nicht abreissen)
- [x] CheckBox Built-In Group Dialog  (radiant/mainframe.cpp:2043)
- [ ] CheckBox Load last map at startup  (radiant/map.cpp:2370)
- [ ] Spinner Model View Size  (radiant/modelwindow.cpp:1366)
- [ ] Entry List of *folderToLoad/depth*  (radiant/modelwindow.cpp:1369)
- [x] Spinner Patch Subdivide Threshold  (radiant/patchmanip.cpp:673) -- Stufe 2 Punkt 6
- [ ] PathEntry Shader Editor Command  (radiant/preferences.cpp:64)
- [ ] Combo Select the game  (radiant/preferences.cpp:259)
- [ ] CheckBox Startup / Show Global Preferences  (radiant/preferences.cpp:267)
- [ ] CheckBox Nudge selected after duplication  (radiant/select.cpp:1921)
- [x] Spinner Selector size (pixels)  (radiant/selection.cpp:2314)
- [x] CheckBox Prefer point entities in 2D  (radiant/selection.cpp:2315)
- [x] CheckBox Create brushes in 3D  (radiant/selection.cpp:2316)
- [x] Combo Move style in 3D  (radiant/selection.cpp:2319)
- [ ] CheckBox Surface Inspector Increments Match Grid  (radiant/surfacedialog.cpp:1735)
- [ ] Radio Texture Quality  (radiant/textures.cpp:764)
- [ ] Spinner Texture Gamma  (radiant/textures.cpp:771)
- [ ] Combo Texture Render Mode  (radiant/textures.cpp:780)
- [ ] Combo Hardware Texture Compression  (radiant/textures.cpp:818)
- [ ] CheckBox Anisotropy  (radiant/textures.cpp:826)
- [x] CheckBox Texture scrollbar  (radiant/texwindow.cpp:2045)
- [x] Combo Texture Thumbnail Scale  (radiant/texwindow.cpp:2052)
- [x] Spinner Thumbnails Max Size  (radiant/texwindow.cpp:2059)
- [x] Spinner Thumbnails Min Size  (radiant/texwindow.cpp:2060)
- [x] Spinner Mousewheel Increment  (radiant/texwindow.cpp:2061)
- [x] Combo Load Shaders at Startup  (radiant/texwindow.cpp:2064)
- [x] CheckBox "", str, g_TexBro.m_hideNonShadersInCommon )  (radiant/texwindow.cpp:2068)
- [ ] Spinner Undo Queue Size  (radiant/undo.cpp:400)
- [ ] CheckBox Enable Build Process Monitoring  (radiant/watchbsp.cpp:221)
- [ ] CheckBox Stop Compilation on Leak  (radiant/watchbsp.cpp:222)
- [ ] CheckBox Run Engine After Compile  (radiant/watchbsp.cpp:223)
- [ ] Entry Engine to Run  (radiant/watchbsp.cpp:227)
- [ ] Entry Engine Arguments  (radiant/watchbsp.cpp:229)
- [ ] Entry MP Engine to Run  (radiant/watchbsp.cpp:232)
- [ ] Entry MP Engine Arguments  (radiant/watchbsp.cpp:234)
- [ ] CheckBox Dump non Monitored Builds Log  (radiant/watchbsp.cpp:238)
- [ ] Entry Region Box Shader  (radiant/watchbsp.cpp:240)
- [ ] CheckBox Solid selection boxes ( no stipple )  (radiant/xywindow.cpp:2237)
- [ ] CheckBox Chase mouse during drags  (radiant/xywindow.cpp:2239)
- [ ] CheckBox Zoom to Mouse pointer  (radiant/xywindow.cpp:2240)
- [ ] Combo MSAA  (radiant/xywindow.cpp:2245)

## Plugins


### MeshTex (contrib/meshtex/MainMenu.cpp:76)

- [~] MeshTex: Set S/T Scale... -> `meshTex::SetScale`  (contrib/meshtex/MainMenu.cpp:76) -- Stufe 3
- [~] MeshTex: Get Info... -> `meshTex::GetInfo`  (contrib/meshtex/MainMenu.cpp:76) -- Stufe 3
* ---
- [~] MeshTex: General Function... -> `meshTex::GeneralFunction`  (contrib/meshtex/MainMenu.cpp:76) -- Stufe 3
* ---
- [~] MeshTex: S&T Align Auto -> `meshTex::STMinMaxAlignAuto`  (contrib/meshtex/MainMenu.cpp:76) -- Stufe 3
- [~] MeshTex: S Align Auto -> `meshTex::SMinMaxAlignAuto`  (contrib/meshtex/MainMenu.cpp:76) -- Stufe 3
- [~] MeshTex: T Align Auto -> `meshTex::TMinMaxAlignAuto`  (contrib/meshtex/MainMenu.cpp:76) -- Stufe 3
* ---
- [~] MeshTex: S Align Stretch -> `meshTex::SMinMaxAlignStretch`  (contrib/meshtex/MainMenu.cpp:76) -- Stufe 3
- [~] MeshTex: S Align Shrink -> `meshTex::SMinMaxAlignShrink`  (contrib/meshtex/MainMenu.cpp:76) -- Stufe 3
- [~] MeshTex: T Align Stretch -> `meshTex::TMinMaxAlignStretch`  (contrib/meshtex/MainMenu.cpp:76) -- Stufe 3
- [~] MeshTex: T Align Shrink -> `meshTex::TMinMaxAlignShrink`  (contrib/meshtex/MainMenu.cpp:76) -- Stufe 3
* ---
- [~] MeshTex: S Min Align -> `meshTex::SMinAlign`  (contrib/meshtex/MainMenu.cpp:76) -- Stufe 3
- [~] MeshTex: S Max Align -> `meshTex::SMaxAlign`  (contrib/meshtex/MainMenu.cpp:76) -- Stufe 3
- [~] MeshTex: T Min Align -> `meshTex::TMinAlign`  (contrib/meshtex/MainMenu.cpp:76) -- Stufe 3
- [~] MeshTex: T Max Align -> `meshTex::TMaxAlign`  (contrib/meshtex/MainMenu.cpp:76) -- Stufe 3
* ---
- [~] MeshTex: Help... -> `meshTex::Help`  (contrib/meshtex/MainMenu.cpp:76) -- Stufe 3
- [~] MeshTex: About... -> `meshTex::About`  (contrib/meshtex/MainMenu.cpp:76) -- Stufe 3

### ShaderPlug (contrib/shaderplug/shaderplug.cpp:71)

- [~] ShaderPlug: Create tag file -> `shaderPlug::CreateTagFile`  (contrib/shaderplug/shaderplug.cpp:71) -- Stufe 3
- [~] ShaderPlug: About -> `shaderPlug::About`  (contrib/shaderplug/shaderplug.cpp:71) -- Stufe 3

### SunPlug (contrib/sunplug/sunplug.cpp:80)

- [~] SunPlug: ET-MapCoordinator -> `sunPlug::ET-MapCoordinator`  (contrib/sunplug/sunplug.cpp:80) -- Stufe 3
- [~] SunPlug: About... -> `sunPlug::About`  (contrib/sunplug/sunplug.cpp:80) -- Stufe 3

### UFO:AI (contrib/ufoaiplug/ufoai.cpp:69)

- [~] UFO:AI: NoDraw -> `uFO:AI::NoDraw`  (contrib/ufoaiplug/ufoai.cpp:69) -- Stufe 3
- [~] UFO:AI: WeaponClip -> `uFO:AI::WeaponClip`  (contrib/ufoaiplug/ufoai.cpp:69) -- Stufe 3
- [~] UFO:AI: ActorClip -> `uFO:AI::ActorClip`  (contrib/ufoaiplug/ufoai.cpp:69) -- Stufe 3
- [~] UFO:AI: StepOn -> `uFO:AI::StepOn`  (contrib/ufoaiplug/ufoai.cpp:69) -- Stufe 3
* ---
- [~] UFO:AI: Level 8 -> `uFO:AI::Level8`  (contrib/ufoaiplug/ufoai.cpp:69) -- Stufe 3
- [~] UFO:AI: Level 7 -> `uFO:AI::Level7`  (contrib/ufoaiplug/ufoai.cpp:69) -- Stufe 3
- [~] UFO:AI: Level 6 -> `uFO:AI::Level6`  (contrib/ufoaiplug/ufoai.cpp:69) -- Stufe 3
- [~] UFO:AI: Level 5 -> `uFO:AI::Level5`  (contrib/ufoaiplug/ufoai.cpp:69) -- Stufe 3
- [~] UFO:AI: Level 4 -> `uFO:AI::Level4`  (contrib/ufoaiplug/ufoai.cpp:69) -- Stufe 3
- [~] UFO:AI: Level 3 -> `uFO:AI::Level3`  (contrib/ufoaiplug/ufoai.cpp:69) -- Stufe 3
- [~] UFO:AI: Level 2 -> `uFO:AI::Level2`  (contrib/ufoaiplug/ufoai.cpp:69) -- Stufe 3
- [~] UFO:AI: Level 1 -> `uFO:AI::Level1`  (contrib/ufoaiplug/ufoai.cpp:69) -- Stufe 3
* ---
- [~] UFO:AI: Perform check -> `uFO:AI::PerformCheck`  (contrib/ufoaiplug/ufoai.cpp:69) -- Stufe 3
- [~] UFO:AI: Worldspawn -> `uFO:AI::Worldspawn`  (contrib/ufoaiplug/ufoai.cpp:69) -- Stufe 3
- [~] UFO:AI: Worldspawn reset -> `uFO:AI::WorldspawnReset`  (contrib/ufoaiplug/ufoai.cpp:69) -- Stufe 3
* ---
- [~] UFO:AI: About -> `uFO:AI::About`  (contrib/ufoaiplug/ufoai.cpp:69) -- Stufe 3

### bobToolz (contrib/bobtoolz/bobToolz-GTK.cpp:52)

- [~] bobToolz: Flip terrain -> `bobToolz::FlipTerrain`  (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [~] bobToolz: Split patch rows -> `bobToolz::SplitPatchRows`  (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [~] bobToolz: Split patch columns -> `bobToolz::SplitPatchColumns`  (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [~] bobToolz: Split patch -> `bobToolz::SplitPatch`  (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [~] bobToolz: Merge Patches -> `bobToolz::MergePatches`  (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
* ---
- [~] bobToolz: Plot Splines -> `bobToolz::PlotSplines`  (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [~] bobToolz: Drop Entity -> `bobToolz::DropEntity`  (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [~] bobToolz: Tree Planter -> `bobToolz::TreePlanter`  (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [~] bobToolz: Explode Entity -> `bobToolz::ExplodeEntity`  (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
* ---
- [~] bobToolz: Caulk Selection -> `bobToolz::CaulkSelection`  (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [~] bobToolz: Polygon Builder -> `bobToolz::PolygonBuilder`  (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [~] bobToolz: Brush Cleanup -> `bobToolz::BrushCleanup`  (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [~] bobToolz: Vis Viewer -> `bobToolz::VisViewer`  (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
* ---
- [~] bobToolz: PitOMatic -> `bobToolz::PitOMatic`  (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [~] bobToolz: Reset Textures... -> `bobToolz::ResetTextures`  (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
* ---
- [~] bobToolz: Path Plotter... -> `bobToolz::PathPlotter`  (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [~] bobToolz: Make Chain... -> `bobToolz::MakeChain`  (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [~] bobToolz: Intersect... -> `bobToolz::Intersect`  (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [~] bobToolz: Find Duplicates -> `bobToolz::FindDuplicates`  (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [~] bobToolz: Door Builder... -> `bobToolz::DoorBuilder`  (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [~] bobToolz: Aperture Door... -> `bobToolz::ApertureDoor`  (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
- [~] bobToolz: Stair Builder... -> `bobToolz::StairBuilder`  (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3
* ---
- [~] bobToolz: About... -> `bobToolz::About`  (contrib/bobtoolz/bobToolz-GTK.cpp:52) -- Stufe 3

### brushexport2 (contrib/brushexport/plugin.cpp:56)

- [~] brushexport2: About -> `brushexport2::About`  (contrib/brushexport/plugin.cpp:56) -- Stufe 3
- [~] brushexport2: Export selected as Wavefront Object -> `brushexport2::Export.obj`  (contrib/brushexport/plugin.cpp:56) -- Stufe 3

### prtview (contrib/prtview/prtview.cpp:164)

- [~] prtview: Load .prt file -> `prtview::Load.prtFile`  (contrib/prtview/prtview.cpp:164) -- Stufe 3
- [~] prtview: Unload .prt file -> `prtview::Unload.prtFile`  (contrib/prtview/prtview.cpp:164) -- Stufe 3
* ---
- [~] prtview: Toggle portals (3D) -> `prtview::TogglePortals(3D)`  (contrib/prtview/prtview.cpp:164) -- Stufe 3
- [~] prtview: Toggle portals (2D) -> `prtview::TogglePortals(2D)`  (contrib/prtview/prtview.cpp:164) -- Stufe 3
* ---
- [~] prtview: Configure Portal Viewer -> `prtview::ConfigurePortalViewer`  (contrib/prtview/prtview.cpp:164) -- Stufe 3
* ---
- [~] prtview: About Portal Viewer -> `prtview::AboutPortalViewer`  (contrib/prtview/prtview.cpp:164) -- Stufe 3
