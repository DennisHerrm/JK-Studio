# -*- coding: utf-8 -*-
"""Menues und Werkzeugleisten von NetRadiant-Custom (NRC, Stand master 2026)
fuer den Karten-Editor - der Aufbau 1:1 aus radiant/mainframe.cpp
(create_main_menu, create_main_toolbar) und den Funktionen, die sie rufen.

Daraus entstehen (tools/karte/gen_befehle.py):
  src/karte/menues.inc     die Menue- und Werkzeugleisten-Tabellen (C++)
  die Texte KeL<Beschriftung> und KeT<Beschreibung> fuer tools/gen_i18n.py
    (ueber tools/i18n_karte.py)

Beschriftungen stehen mit NRCs '&' (Mnemonik); die englische Spalte der
Uebersetzung ist dieselbe ohne '&' - genau das zeigt "Begriffe wie
NetRadiant".

Eintraege eines Menues:
  ('ITEM', '&Label', 'Befehl', bedingung)    create_menu_item_with_mnemonic
  ('CHECK', '&Label', 'Befehl', bedingung)   create_check_menu_item_with_mnemonic
  ('RUF', '&Label', 'name', bedingung)       Menuepunkt mit eigenem Rueckruf (makeCallbackF)
  ('SEP',)                                   addSeparator
  ('SUB', '&Label', [eintraege])             addMenu
  ('LISTE', 'name')                          dynamische Eintraege (MRU, Themen, Farben, Hilfe ...)
bedingung: '' immer; 'floating' nur Floating; 'nichtRegular' nicht Regular/RegularLeft;
'texturen' nicht Regular/RegularLeft oder Built-In Group Dialog; 'nichtSplit'
Regular/RegularLeft/Floating; 'split' nur Split (mainframe.cpp:932-1050).
"""

MENUES = [
    ('&File', 'radiant/mainframe.cpp:866', [
        ('ITEM', '&New Map', 'NewMap', ''),
        ('SEP',),
        ('ITEM', '&Open...', 'OpenMap', ''),
        ('ITEM', '&Import...', 'ImportMap', ''),
        # Stufe 4 Punkt 1 (Abweichung): eine fertige Karte (.bsp) oeffnen - NRC
        # kann sie nur importieren (mapq3 "quake3 compiled maps", can_load = false).
        ('LISTE', 'ProgrammDatei'),
        ('SEP',),
        ('ITEM', '&Save', 'SaveMap', ''),
        ('ITEM', 'Save &as...', 'SaveMapAs', ''),
        ('ITEM', 'Save s&elected...', 'SaveSelected', ''),
        ('ITEM', 'Save re&gion...', 'SaveRegion', ''),
        ('SEP',),
        ('ITEM', '&Pointfile', 'TogglePointfile', ''),
        ('SEP',),
        ('LISTE', 'MRU'),                       # MRU_constructMenu (mru.cpp:186)
        ('SEP',),
        ('ITEM', 'E&xit', 'Exit', ''),
    ]),
    ('&Edit', 'radiant/mainframe.cpp:892', [
        ('ITEM', '&Undo', 'Undo', ''),
        ('ITEM', '&Redo', 'Redo', ''),
        ('SEP',),
        ('ITEM', '&Copy', 'Copy', ''),
        ('ITEM', '&Paste', 'Paste', ''),
        ('ITEM', 'P&aste To Camera', 'PasteToCamera', ''),
        ('ITEM', 'Move To Camera', 'MoveToCamera', ''),
        ('SEP',),
        ('ITEM', '&Duplicate', 'CloneSelection', ''),
        ('ITEM', 'Duplicate, make uni&que', 'CloneSelectionAndMakeUnique', ''),
        ('ITEM', 'D&elete', 'DeleteSelection', ''),
        ('SEP',),
        ('ITEM', 'C&lear Selection', 'UnSelectSelection', ''),
        ('ITEM', '&Invert Selection', 'InvertSelection', ''),
        ('ITEM', 'Select i&nside', 'SelectInside', ''),
        ('ITEM', 'Select &touching', 'SelectTouching', ''),
        ('SEP',),
        ('ITEM', 'Select All Of Type', 'SelectAllOfType', ''),
        ('ITEM', 'Select Textured', 'SelectTextured', ''),
        ('ITEM', '&Expand Selection To Primitives', 'ExpandSelectionToPrimitives', ''),
        ('ITEM', '&Expand Selection To Entities', 'ExpandSelectionToEntities', ''),
        ('ITEM', '&Expand Selection To Layers', 'ExpandSelectionToLayers', ''),
        ('ITEM', 'Select Connected Entities', 'SelectConnectedEntities', ''),
        ('SEP',),
        ('ITEM', '&Shortcuts...', 'Shortcuts', ''),
        ('ITEM', 'Pre&ferences...', 'Preferences', ''),
    ]),
    ('Vie&w', 'radiant/mainframe.cpp:930', [
        ('CHECK', 'Camera View', 'ToggleCamera', 'floating'),
        ('CHECK', 'XY (Top) View', 'ToggleView', 'floating'),
        ('CHECK', 'XZ (Front) View', 'ToggleFrontView', 'floating'),
        ('CHECK', 'YZ (Side) View', 'ToggleSideView', 'floating'),
        ('ITEM', 'Console', 'ToggleConsole', 'nichtRegular'),
        ('ITEM', 'Texture Browser', 'ToggleTextures', 'texturen'),
        ('ITEM', 'Model Browser', 'ToggleModelBrowser', ''),
        ('ITEM', 'Entity Inspector', 'ToggleEntityInspector', ''),
        ('ITEM', 'Layers Browser', 'ToggleLayersBrowser', ''),
        ('ITEM', '&Surface Inspector', 'SurfaceInspector', ''),
        ('ITEM', 'Entity List', 'ToggleEntityList', ''),
        ('SEP',),
        ('SUB', 'Camera', [
            ('ITEM', 'Focus on Selected', 'CameraFocusOnSelected', ''),
            ('ITEM', '&Center', 'CenterView', ''),
            ('ITEM', '&Up Floor', 'UpFloor', ''),
            ('ITEM', '&Down Floor', 'DownFloor', ''),
            ('SEP',),
            ('ITEM', 'Far Clip Plane In', 'CubicClipZoomIn', ''),
            ('ITEM', 'Far Clip Plane Out', 'CubicClipZoomOut', ''),
            ('SEP',),
            ('ITEM', 'Next leak spot', 'NextLeakSpot', ''),
            ('ITEM', 'Previous leak spot', 'PrevLeakSpot', ''),
        ]),
        ('SEP',),
        ('SUB', 'Orthographic', [
            ('ITEM', '&Next (XY, XZ, YZ)', 'NextView', 'nichtSplit'),
            ('ITEM', 'XY (Top)', 'ViewTop', 'nichtSplit'),
            ('ITEM', 'XZ (Front)', 'ViewFront', 'nichtSplit'),
            ('ITEM', 'YZ (Side)', 'ViewSide', 'nichtSplit'),
            ('SEP', 'nichtSplit'),
            ('ITEM', 'Center on Selected', 'NextView', 'split'),
            ('ITEM', 'Focus on Selected', 'XYFocusOnSelected', ''),
            ('ITEM', 'Center on Selected', 'CenterXYView', ''),
            ('SEP',),
            ('ITEM', '&XY 100%', 'Zoom100', ''),
            ('ITEM', 'XY Zoom &In', 'ZoomIn', ''),
            ('ITEM', 'XY Zoom &Out', 'ZoomOut', ''),
        ]),
        ('SEP',),
        ('SUB', 'Show', [
            ('CHECK', 'Show Entity &Angles', 'ShowAngles', ''),
            ('CHECK', 'Show Entity &Names', 'ShowNames', ''),
            ('CHECK', 'Show Light Radiuses', 'ShowLightRadiuses', ''),
            ('CHECK', 'Show Entity Boxes', 'ShowBboxes', ''),
            ('CHECK', 'Show Entity Connections', 'ShowConnections', ''),
            ('SEP',),
            ('CHECK', 'Show 2D Size Info', 'ShowSize2d', ''),
            ('CHECK', 'Show 3D Size Info', 'ShowSize3d', ''),
            ('CHECK', 'Show Crosshair', 'ToggleCrosshairs', ''),
            ('CHECK', 'Show Grid', 'ToggleGrid', ''),
            ('CHECK', 'Show Blocks', 'ShowBlocks', ''),
            ('CHECK', 'Show C&oordinates', 'ShowCoordinates', ''),
            ('CHECK', 'Show Window Outline', 'ShowWindowOutline', ''),
            ('CHECK', 'Show Axes', 'ShowAxes', ''),
            ('CHECK', 'Show 2D Workzone', 'ShowWorkzone2d', ''),
            ('CHECK', 'Show 3D Workzone', 'ShowWorkzone3d', ''),
            ('CHECK', 'Show Renderer Stats', 'ShowStats', ''),
        ]),
        ('SUB', 'Filter', [                      # Filters_constructMenu (filters.cpp:159)
            ('CHECK', 'World', 'FilterWorldBrushes', ''),
            ('CHECK', 'Entities', 'FilterEntities', ''),
            ('CHECK', 'Areaportals', 'FilterAreaportals', ''),
            ('CHECK', 'Translucent', 'FilterTranslucent', ''),
            ('CHECK', 'Liquids', 'FilterLiquids', ''),
            ('CHECK', 'Caulk', 'FilterCaulk', ''),
            ('CHECK', 'Clips', 'FilterClips', ''),
            ('CHECK', 'Paths', 'FilterPaths', ''),
            ('CHECK', 'Clusterportals', 'FilterClusterportals', ''),
            ('CHECK', 'Lights', 'FilterLights', ''),
            ('CHECK', 'Structural', 'FilterStructural', ''),
            ('CHECK', 'Lightgrid', 'FilterLightgrid', ''),
            ('CHECK', 'Patches', 'FilterPatches', ''),
            ('CHECK', 'Details', 'FilterDetails', ''),
            ('CHECK', 'Hints', 'FilterHintsSkips', ''),
            ('CHECK', 'Models', 'FilterModels', ''),
            ('CHECK', 'Triggers', 'FilterTriggers', ''),
            ('CHECK', 'Botclips', 'FilterBotClips', ''),
            ('CHECK', 'Decals', 'FilterDecals', ''),
            ('CHECK', 'FuncGroups', 'FilterFuncGroups', ''),
            ('CHECK', 'Point Entities', 'FilterPointEntities', ''),
            ('CHECK', 'Sky', 'FilterSky', ''),
            ('SEP',),
            ('ITEM', 'Invert filters', 'InvertFilters', ''),
            ('ITEM', 'Reset filters', 'ResetFilters', ''),
        ]),
        ('SEP',),
        ('CHECK', 'Hide Selected', 'HideSelected', ''),
        ('ITEM', 'Show Hidden', 'ShowHidden', ''),
        ('SEP',),
        ('SUB', 'Region', [
            ('ITEM', '&Off', 'RegionOff', ''),
            ('ITEM', '&Set XY', 'RegionSetXY', ''),
            ('ITEM', 'Set &Brush', 'RegionSetBrush', ''),
            ('CHECK', 'Set Se&lection', 'RegionSetSelection', ''),
        ]),
        # Stufe 7 Punkt 6b: Zielspiel, Farbgebung, Sprache, Skalierung und
        # Knopfgroesse des Programms - nicht in NRC, als Abweichung notiert.
        ('LISTE', 'ProgrammView'),
    ]),
    ('M&odify', 'radiant/mainframe.cpp:1053', [
        ('SUB', 'Components', [
            ('CHECK', '&Edges', 'DragEdges', ''),
            ('CHECK', '&Vertices', 'DragVertices', ''),
            ('CHECK', '&Faces', 'DragFaces', ''),
        ]),
        ('SEP',),
        ('ITEM', 'Snap To Grid', 'SnapToGrid', ''),
        ('SEP',),
        ('SUB', 'Nudge', [
            ('ITEM', 'Nudge Left', 'SelectNudgeLeft', ''),
            ('ITEM', 'Nudge Right', 'SelectNudgeRight', ''),
            ('ITEM', 'Nudge Up', 'SelectNudgeUp', ''),
            ('ITEM', 'Nudge Down', 'SelectNudgeDown', ''),
            ('SEP',),
            ('ITEM', 'Nudge +Z', 'MoveSelectionUP', ''),
            ('ITEM', 'Nudge -Z', 'MoveSelectionDOWN', ''),
        ]),
        ('SUB', 'Rotate', [
            ('ITEM', 'Rotate X', 'RotateSelectionX', ''),
            ('ITEM', 'Rotate Y', 'RotateSelectionY', ''),
            ('ITEM', 'Rotate Z', 'RotateSelectionZ', ''),
            ('SEP',),
            ('ITEM', 'Rotate Clockwise', 'RotateSelectionClockwise', ''),
            ('ITEM', 'Rotate Anticlockwise', 'RotateSelectionAnticlockwise', ''),
        ]),
        ('SUB', 'Flip', [
            ('ITEM', 'Flip &X', 'MirrorSelectionX', ''),
            ('ITEM', 'Flip &Y', 'MirrorSelectionY', ''),
            ('ITEM', 'Flip &Z', 'MirrorSelectionZ', ''),
            ('SEP',),
            ('ITEM', 'Flip Horizontally', 'MirrorSelectionHorizontally', ''),
            ('ITEM', 'Flip Vertically', 'MirrorSelectionVertically', ''),
        ]),
        ('SEP',),
        ('ITEM', 'Arbitrary rotation...', 'ArbitraryRotation', ''),
        ('ITEM', 'Arbitrary scale...', 'ArbitraryScale', ''),
        ('SEP',),
        ('SUB', 'Repeat', [
            ('ITEM', 'Repeat Transforms', 'RepeatTransforms', ''),
            ('LISTE', 'Transforms'),            # SelectionSystem_connectTransformsCallbacks (mainframe.cpp:1123)
            ('ITEM', 'Reset Transforms', 'ResetTransforms', ''),
        ]),
    ]),
    ('&Build', 'radiant/mainframe.cpp:1135', [
        ('ITEM', 'Customize...', 'BuildMenuCustomize', ''),
        ('ITEM', 'Run recent build', 'Build_runRecentExecutedBuild', ''),
        ('SEP',),
        ('LISTE', 'Build'),                     # Build_constructMenu (build.cpp)
    ]),
    ('&Grid', 'radiant/mainframe.cpp:1152', [   # Grid_constructMenu (grid.cpp:205), Radio
        ('CHECK', 'Grid0.125', 'SetGrid0.125', ''),
        ('CHECK', 'Grid0.25', 'SetGrid0.25', ''),
        ('CHECK', 'Grid0.5', 'SetGrid0.5', ''),
        ('CHECK', 'Grid1', 'SetGrid1', ''),
        ('CHECK', 'Grid2', 'SetGrid2', ''),
        ('CHECK', 'Grid4', 'SetGrid4', ''),
        ('CHECK', 'Grid8', 'SetGrid8', ''),
        ('CHECK', 'Grid16', 'SetGrid16', ''),
        ('CHECK', 'Grid32', 'SetGrid32', ''),
        ('CHECK', 'Grid64', 'SetGrid64', ''),
        ('CHECK', 'Grid128', 'SetGrid128', ''),
        ('CHECK', 'Grid256', 'SetGrid256', ''),
        ('CHECK', 'Grid512', 'SetGrid512', ''),
        ('CHECK', 'Grid1024', 'SetGrid1024', ''),
    ]),
    ('M&isc', 'radiant/mainframe.cpp:1161', [
        ('SUB', 'Theming', [                     # create_colours_menu (colors.cpp:210)
            ('SUB', 'Viewports Theme', [('LISTE', 'Viewportthemen')]),
            ('SUB', 'GUI Theme', [('LISTE', 'GuiThemen')]),   # theme_construct_menu (theme.cpp:182)
            ('ITEM', 'OpenGL Font...', 'OpenGLFont', ''),
            ('SEP',),
            ('LISTE', 'Farben'),                 # g_ColoursMenu (colors.cpp:153)
        ]),
        ('ITEM', 'Find brush...', 'FindBrush', ''),   # NRC 20260114 (master: "Find Entity/Brush...")
        ('ITEM', 'Map Info...', 'MapInfo', ''),
        ('ITEM', '&Refresh models', 'RefreshReferences', ''),
        ('RUF', 'Set 2D &Background image...', 'WXY_SetBackgroundImage', ''),
        ('ITEM', 'Fullscreen', 'Fullscreen', ''),
        ('ITEM', 'Maximize view', 'MaximizeView', ''),
        # Stufe 3 Punkt 0b (eine Leiste, die umschaltet): Sprache, Farbschema
        # und Skalierung des Programms - nicht in NRC, als Abweichung notiert.
        ('LISTE', 'ProgrammMisc'),
    ]),
    ('E&ntity', 'radiant/mainframe.cpp:1180', [  # Entity_constructMenu (entity.cpp:708)
        ('ITEM', '&Connect Entities', 'EntitiesConnect', ''),
        ('ITEM', '&Move Primitives to Entity', 'EntityMovePrimitivesToLast', ''),
        ('ITEM', '&Select Color...', 'EntityColorSet', ''),
        ('ITEM', '&Normalize Color', 'EntityColorNormalize', ''),
        ('SEP',),
        ('ITEM', 'Reload Entity Definitions', 'EntityReloadDefinitions', ''),
    ]),
    ('Brush', 'radiant/mainframe.cpp:1189', [    # Brush_constructMenu (brushmanip.cpp:1497)
        ('ITEM', 'Prism...', 'BrushPrism', ''),
        ('ITEM', 'Cone...', 'BrushCone', ''),
        ('ITEM', 'Sphere...', 'BrushSphere', ''),
        ('ITEM', 'Rock...', 'BrushRock', ''),
        ('ITEM', 'Icosahedron...', 'BrushIcosahedron', ''),
        ('SEP',),
        ('SUB', 'CSG', [
            ('ITEM', 'CSG &Subtract', 'CSGSubtract', ''),
            ('ITEM', 'CSG &Merge', 'CSGMerge', ''),
            ('ITEM', 'CSG &Wrap Merge', 'CSGWrapMerge', ''),
            ('ITEM', 'CSG &Intersect', 'CSGIntersect', ''),
            ('ITEM', 'Make &Room', 'CSGroom', ''),
            ('ITEM', 'CSG &Tool', 'CSGTool', ''),
        ]),
        ('SEP',),
        ('SUB', 'Clipper', [
            ('ITEM', 'Clip selection', 'ClipperClip', ''),
            ('ITEM', 'Split selection', 'ClipperSplit', ''),
            ('ITEM', 'Flip Clip orientation', 'ClipperFlip', ''),
        ]),
        ('SEP',),
        ('ITEM', 'Make detail', 'MakeDetail', ''),
        ('ITEM', 'Make structural', 'MakeStructural', ''),
        ('SEP',),
        ('CHECK', 'Texture Lock', 'TogTexLock', ''),
        ('CHECK', 'Texture Vertex Lock', 'TogTexVertexLock', ''),
        ('ITEM', 'Reset Texture', 'TextureReset/Cap', ''),
        ('ITEM', 'AutoCaulk Selected', 'AutoCaulkSelected', ''),
    ]),
    ('&Curve', 'radiant/mainframe.cpp:1198', [   # Patch_constructMenu (patchmanip.cpp:738)
        ('ITEM', 'Simple Patch Mesh...', 'SimplePatchMesh', ''),
        ('ITEM', 'Bevel', 'PatchBevel', ''),
        ('ITEM', 'End cap', 'PatchEndCap', ''),
        ('ITEM', 'Cylinder (9x3)', 'PatchCylinder', ''),
        ('ITEM', 'Square Cylinder (9x3)', 'PatchSquareCylinder', ''),
        ('ITEM', 'Exact Cylinder...', 'PatchXactCylinder', ''),
        ('ITEM', 'Cone (9x3)', 'PatchCone', ''),
        ('ITEM', 'Exact Cone...', 'PatchXactCone', ''),
        ('ITEM', 'Sphere (9x5)', 'PatchSphere', ''),
        ('ITEM', 'Exact Sphere...', 'PatchXactSphere', ''),
        ('SEP',),
        ('ITEM', 'Cap Selection', 'CapCurrentCurve', ''),
        ('SEP',),
        ('SUB', 'Insert/Delete', [
            ('ITEM', 'Insert (2) First Columns', 'PatchInsertFirstColumn', ''),
            ('ITEM', 'Insert (2) Last Columns', 'PatchInsertLastColumn', ''),
            ('SEP',),
            ('ITEM', 'Insert (2) First Rows', 'PatchInsertFirstRow', ''),
            ('ITEM', 'Insert (2) Last Rows', 'PatchInsertLastRow', ''),
            ('SEP',),
            ('ITEM', 'Del First (2) Columns', 'PatchDeleteFirstColumn', ''),
            ('ITEM', 'Del Last (2) Columns', 'PatchDeleteLastColumn', ''),
            ('SEP',),
            ('ITEM', 'Del First (2) Rows', 'PatchDeleteFirstRow', ''),
            ('ITEM', 'Del Last (2) Rows', 'PatchDeleteLastRow', ''),
        ]),
        ('SEP',),
        ('SUB', 'Matrix', [
            ('ITEM', 'Invert', 'InvertCurve', ''),
            ('ITEM', 'Transpose', 'MatrixTranspose', ''),
            ('SEP',),
            ('ITEM', 'Re-disperse Rows', 'RedisperseRows', ''),
            ('ITEM', 'Re-disperse Columns', 'RedisperseCols', ''),
            ('SEP',),
            ('ITEM', 'Smooth Rows', 'SmoothRows', ''),
            ('ITEM', 'Smooth Columns', 'SmoothCols', ''),
        ]),
        ('SEP',),
        ('SUB', 'Texture', [
            ('ITEM', 'Reset Texture', 'TextureReset/Cap', ''),
            ('ITEM', 'Naturalize', 'NaturalizePatch', ''),
            ('ITEM', 'Invert X', 'InvertCurveTextureX', ''),
            ('ITEM', 'Invert Y', 'InvertCurveTextureY', ''),
        ]),
        ('SEP',),
        ('ITEM', 'Deform...', 'PatchDeform', ''),
        ('ITEM', 'Thicken...', 'PatchThicken', ''),
    ]),
    ('&Plugins', 'radiant/pluginmenu.cpp:131', [
        ('LISTE', 'Plugins'),
    ]),
    ('&Help', 'radiant/mainframe.cpp:1207', [
        # Stufe 3 Punkt 0b: "Manual" (OpenManual, F1) - bei NRC auskommentiert
        # (mainframe.cpp:1214), hier das Handbuch des Programms.
        ('LISTE', 'ProgrammHilfeOben'),
        ('LISTE', 'Hilfe'),                      # create_game_help_menu (help.cpp:112): global.xlink + game.xlink
        ('RUF', 'Bug report', 'OpenBugReportURL', ''),
        ('ITEM', 'Check for NetRadiant update', 'CheckForUpdate', ''),
        ('RUF', '&About', 'DoAbout', ''),
        ('LISTE', 'ProgrammHilfeUnten'),         # Stufe 3 Punkt 0b: About BehavEd
    ]),
]

# Der Texturbrowser hat eigene Menues (texwindow.cpp:1238 TextureBrowser_constructViewMenu,
# :1281 TextureBrowser_constructTagsMenu) und eine eigene Werkzeugleiste (:1788-1806).
TEXTUR_MENUES = [
    ('View', 'radiant/texwindow.cpp:1238', [
        ('CHECK', 'Hide Unused', 'ShowInUse', ''),
        ('ITEM', 'Show All', 'ShowAllTextures', ''),
        ('SEP',),
        ('CHECK', 'Show Shaders', 'ToggleShowShaders', ''),
        ('CHECK', 'Show Textures', 'ToggleShowTextures', ''),
        ('SEP',),
        ('CHECK', 'ShaderList Only', 'ToggleShowShaderlistOnly', ''),
        ('CHECK', 'Hide Image Missing', 'FilterNotex', ''),
        ('SEP',),
        ('CHECK', 'Fixed Size', 'FixedSize', ''),
        ('CHECK', 'Transparency', 'EnableAlpha', ''),
        ('SEP',),
        ('CHECK', 'Tags GUI', 'TagsToggleGui', ''),
    ]),
    ('Tags', 'radiant/texwindow.cpp:1281', [
        ('ITEM', 'Add tag', 'TagAdd', ''),
        ('ITEM', 'Rename tag', 'TagRename', ''),
        ('ITEM', 'Delete tag', 'TagDelete', ''),
        ('SEP',),
        ('ITEM', 'Copy tags from selected', 'TagCopy', ''),
        ('ITEM', 'Paste tags to selected', 'TagPaste', ''),
        ('SEP',),
        ('ITEM', 'Search tag', 'TagSearch', ''),
        ('ITEM', 'Search Untagged', 'TagSearchUntagged', ''),
    ]),
]

# Werkzeugleiste des Texturbrowsers (texwindow.cpp:1800-1806); "View" oeffnet
# das View-Menue (eigener Rueckruf, kein Befehl).
TEXTUR_KNOEPFE = [
    ('KNOPF', '', 'View', 'texbro_view', False, ''),
    ('KNOPF', 'FindReplaceTextures', 'Find / Replace...', 'texbro_find-replace', False, ''),
    ('KNOPF', 'RefreshShaders', 'Flush & Reload Shaders', 'texbro_refresh', False, ''),
]

# create_main_toolbar (mainframe.cpp:1387-1442) und CamWnd_constructToolbar
# (camwindow.cpp:2260), Patch_constructToolbar (patchmanip.cpp:734).
# ('KNOPF', Befehl, Beschreibung, Bild, schalter, bedingung) | ('TRENNER', bedingung)
WERKZEUGE = [
    ('KNOPF', 'OpenMap', 'Open an existing map', 'file_open', False, ''),
    ('KNOPF', 'SaveMap', 'Save the active map', 'file_save', False, ''),
    ('TRENNER', ''),
    ('KNOPF', 'Undo', 'Undo', 'undo', False, ''),
    ('KNOPF', 'Redo', 'Redo', 'redo', False, ''),
    ('TRENNER', ''),
    ('KNOPF', 'MirrorSelectionHorizontally', 'Flip Horizontally', 'brush_flip_hor', False, ''),
    ('KNOPF', 'MirrorSelectionVertically', 'Flip Vertically', 'brush_flip_vert', False, ''),
    ('KNOPF', 'RotateSelectionAnticlockwise', 'Rotate Anticlockwise', 'brush_rotate_anti', False, ''),
    ('KNOPF', 'RotateSelectionClockwise', 'Rotate Clockwise', 'brush_rotate_clock', False, ''),
    ('TRENNER', ''),
    ('KNOPF', 'SelectTouching', 'Select touching', 'selection_selecttouching', False, ''),
    ('KNOPF', 'SelectInside', 'Select inside', 'selection_selectinside', False, ''),
    ('TRENNER', ''),
    ('KNOPF', 'CSGSubtract', 'CSG Subtract', 'selection_csgsubtract', False, ''),
    ('KNOPF', 'CSGWrapMerge', 'CSG Wrap Merge', 'selection_csgmerge', False, ''),
    ('KNOPF', 'CSGroom', 'Room', 'selection_makeroom', False, ''),
    ('KNOPF', 'CSGTool', 'CSG Tool', 'ellipsis', False, ''),
    ('TRENNER', ''),
    ('KNOPF', 'DragVertices', 'Select Vertices', 'modify_vertices', True, ''),
    ('KNOPF', 'DragEdges', 'Select Edges', 'modify_edges', True, ''),
    ('KNOPF', 'DragFaces', 'Select Faces', 'modify_faces', True, ''),
    ('TRENNER', ''),
    ('KNOPF', 'NextView', 'Change views', 'view_change', False, 'nichtSplit'),
    ('TRENNER', 'nichtSplit'),
    ('KNOPF', 'ToggleCubicClip', 'Cubic clip the camera view', 'view_cubicclipping', True, ''),
    ('TRENNER', ''),
    ('KNOPF', 'MouseDrag', 'Resize (Q)', 'select_mouseresize', True, ''),
    ('KNOPF', 'ToggleClipper', 'Clipper', 'select_clipper', True, ''),
    ('KNOPF', 'MouseTranslate', 'Translate', 'select_mousetranslate', True, ''),
    ('KNOPF', 'MouseRotate', 'Rotate', 'select_mouserotate', True, ''),
    ('KNOPF', 'MouseScale', 'Scale', 'select_mousescale', True, ''),
    ('KNOPF', 'MouseTransform', 'Transform (Q)', 'select_mousetransform', True, ''),
    ('KNOPF', 'MouseUV', 'UV Tool', 'select_mouseuv', True, ''),
    ('TRENNER', ''),
    ('KNOPF', 'CapCurrentCurve', 'Put caps on the current patch', 'curve_cap', False, ''),
    ('TRENNER', ''),
    ('KNOPF', 'TogTexLock', 'Texture Lock', 'texture_lock', True, ''),
    ('KNOPF', 'TogTexVertexLock', 'Texture Vertex Lock', 'texture_vertexlock', True, ''),
    ('TRENNER', ''),
    ('KNOPF', 'ToggleEntityInspector', 'Entities', 'entities', False, ''),
    ('KNOPF', 'ToggleConsole', 'Console', 'console', False, 'nichtRegular'),
    ('KNOPF', 'ToggleTextures', 'Texture Browser', 'texture_browser', False, 'texturen'),
    ('TRENNER', ''),
    ('KNOPF', 'RefreshReferences', 'Refresh Models', 'refresh_models', False, ''),
]

# g_toolbarHiddenButtons (gtkmisc.cpp:147): standardmaessig versteckt.
VERSTECKT = ['OpenMap', 'SaveMap', 'Undo', 'Redo', 'SelectTouching', 'SelectInside', 'NextView',
             'ToggleCubicClip', 'CapCurrentCurve', 'ToggleEntityInspector', 'ToggleConsole', 'ToggleTextures',
             'RefreshReferences']

# create_filter_toolbar (filterbar.cpp:134-200), rechts angedockt.
# ('KNOPF', Befehl, Beschreibung, Bild, schalter, rechtsklick) | ('TRENNER',)
# rechtsklick: Befehl, 'func_group' (CommonFunc_group) oder 'tex:<a>,<b>' (CommonFunc_tex).
FILTER = [
    ('KNOPF', 'FilterWorldBrushes', 'World', 'f-world', True, ''),
    ('KNOPF', 'FilterStructural', 'Structural\nRightClick: MakeStructural', 'f-structural', True, 'MakeStructural'),
    ('KNOPF', 'FilterDetails', 'Details\nRightClick: MakeDetail', 'f-details', True, 'MakeDetail'),
    ('KNOPF', 'FilterFuncGroups', 'Func_Groups\nRightClick: create func_group', 'f-funcgroups', True, 'func_group'),
    ('KNOPF', 'FilterPatches', 'Patches', 'f-patches', True, ''),
    ('TRENNER',),
    ('KNOPF', 'FilterTranslucent', 'Translucent\nRightClick: toggle tex\n\tnoDraw\n\tnoDrawNonSolid', 'f-translucent',
     True, 'tex:nodraw,nodrawnonsolid'),
    ('KNOPF', 'FilterLiquids', 'Liquids\nRightClick: toggle tex\n\twaterCaulk\n\tlavaCaulk\n\tslimeCaulk', 'f-liquids',
     True, 'tex:watercaulk,lavacaulk,slimecaulk'),
    ('KNOPF', 'FilterCaulk', 'Caulk\nRightClick: tex Caulk', 'f-caulk', True, 'tex:caulk'),
    ('KNOPF', 'FilterClips', 'Clips\nRightClick: toggle tex\n\tplayerClip\n\tweapClip', 'f-clip', True, 'tex:clip,weapclip'),
    ('KNOPF', 'FilterHintsSkips', 'HintsSkips\nRightClick: toggle tex\n\thint\n\thintLocal\n\thintSkip', 'f-hint', True,
     'tex:hint,hintlocal,hintskip'),
    ('KNOPF', 'FilterSky', 'Sky', 'f-sky', True, ''),
    ('TRENNER',),
    ('KNOPF', 'FilterEntities', 'Entities', 'f-entities', True, ''),
    ('KNOPF', 'FilterPointEntities', 'Point Entities', 'f-pointentities', True, ''),
    ('KNOPF', 'FilterLights', 'Lights', 'f-lights', True, ''),
    ('KNOPF', 'FilterModels', 'Models', 'f-models', True, ''),
    ('KNOPF', 'FilterTriggers', 'Triggers\nRightClick: tex Trigger', 'f-triggers', True, 'tex:trigger'),
    ('TRENNER',),
    ('KNOPF', 'ResetFilters', 'ResetFilters', 'f-reset', False, ''),
    ('TRENNER',),
    ('KNOPF', 'RegionSetSelection', 'Region Set Selection\nRightClick: Region Off', 'f-region', True, 'RegionOff'),
    ('KNOPF', 'HideSelected', 'Hide Selected\nRightClick: Show Hidden', 'f-hide', True, 'ShowHidden'),
]

# Uebersetzungen: Beschriftung ohne '&' -> (Deutsch, 中文, 日本語).
UEBERSETZUNG = {
    # Menuetitel
    'File': ('Datei', '文件', 'ファイル'),
    'Edit': ('Bearbeiten', '编辑', '編集'),
    'View': ('Ansicht', '视图', '表示'),
    'Modify': ('Ändern', '修改', '変更'),
    'Build': ('Kompilieren', '编译', 'ビルド'),
    'Grid': ('Raster', '网格', 'グリッド'),
    'Misc': ('Verschiedenes', '其他', 'その他'),
    'Entity': ('Entity', '实体', 'エンティティ'),
    'Brush': ('Brush', '画刷', 'ブラシ'),
    'Curve': ('Kurve', '曲面', 'カーブ'),
    'Plugins': ('Plugins', '插件', 'プラグイン'),
    'Help': ('Hilfe', '帮助', 'ヘルプ'),
    # File
    'New Map': ('Neue Karte', '新建地图', '新規マップ'),
    'Open...': ('Öffnen...', '打开...', '開く...'),
    'Import...': ('Importieren...', '导入...', 'インポート...'),
    'Save': ('Speichern', '保存', '保存'),
    'Save as...': ('Speichern unter...', '另存为...', '名前を付けて保存...'),
    'Save selected...': ('Auswahl speichern...', '保存所选...', '選択範囲を保存...'),
    'Save region...': ('Region speichern...', '保存区域...', 'リージョンを保存...'),
    'Pointfile': ('Pointfile (Leck-Linie)', '点文件(泄漏线)', 'ポイントファイル(リーク線)'),
    'Exit': ('Beenden', '退出', '終了'),
    # Edit
    'Undo': ('Rückgängig', '撤销', '元に戻す'),
    'Redo': ('Wiederholen', '重做', 'やり直し'),
    'Copy': ('Kopieren', '复制', 'コピー'),
    'Paste': ('Einfügen', '粘贴', '貼り付け'),
    'Paste To Camera': ('An Kamera einfügen', '粘贴到摄像机', 'カメラ位置に貼り付け'),
    'Move To Camera': ('Zur Kamera verschieben', '移动到摄像机', 'カメラ位置へ移動'),
    'Duplicate': ('Duplizieren', '复制一份', '複製'),
    'Duplicate, make unique': ('Duplizieren, eindeutig machen', '复制一份并使名称唯一', '複製して名前を一意にする'),
    'Delete': ('Löschen', '删除', '削除'),
    'Clear Selection': ('Auswahl aufheben', '取消选择', '選択解除'),
    'Invert Selection': ('Auswahl umkehren', '反向选择', '選択を反転'),
    'Select inside': ('Innen liegende wählen', '选择内部', '内側を選択'),
    'Select touching': ('Berührende wählen', '选择接触的', '接触しているものを選択'),
    'Select All Of Type': ('Alle dieses Typs wählen', '选择所有同类', '同じ種類をすべて選択'),
    'Select Textured': ('Gleich texturierte wählen', '选择同纹理', '同じテクスチャを選択'),
    'Expand Selection To Primitives': ('Auswahl auf Primitive erweitern', '选择扩展到图元', '選択をプリミティブへ拡張'),
    'Expand Selection To Entities': ('Auswahl auf Entities erweitern', '选择扩展到实体', '選択をエンティティへ拡張'),
    'Expand Selection To Layers': ('Auswahl auf Ebenen erweitern', '选择扩展到图层', '選択をレイヤーへ拡張'),
    'Select Connected Entities': ('Verbundene Entities wählen', '选择相连的实体', '接続されたエンティティを選択'),
    'Shortcuts...': ('Tastenkürzel...', '快捷键...', 'ショートカット...'),
    'Preferences...': ('Einstellungen...', '首选项...', '設定...'),
    # View
    'Camera View': ('Kameraansicht', '摄像机视图', 'カメラビュー'),
    'XY (Top) View': ('XY-Ansicht (oben)', 'XY 视图(顶)', 'XY ビュー(上)'),
    'XZ (Front) View': ('XZ-Ansicht (vorn)', 'XZ 视图(前)', 'XZ ビュー(前)'),
    'YZ (Side) View': ('YZ-Ansicht (Seite)', 'YZ 视图(侧)', 'YZ ビュー(横)'),
    'Console': ('Konsole', '控制台', 'コンソール'),
    'Texture Browser': ('Texturbrowser', '纹理浏览器', 'テクスチャブラウザー'),
    'Model Browser': ('Modellbrowser', '模型浏览器', 'モデルブラウザー'),
    'Entity Inspector': ('Entity-Inspektor', '实体检查器', 'エンティティインスペクター'),
    'Layers Browser': ('Ebenen', '图层浏览器', 'レイヤーブラウザー'),
    'Surface Inspector': ('Flächeninspektor', '表面检查器', 'サーフェスインスペクター'),
    'Entity List': ('Entity-Liste', '实体列表', 'エンティティリスト'),
    'Camera': ('Kamera', '摄像机', 'カメラ'),
    'Focus on Selected': ('Auf Auswahl ausrichten', '聚焦所选', '選択に焦点'),
    'Center': ('Waagerecht', '居中', '水平に戻す'),
    'Up Floor': ('Stockwerk hoch', '上一层', '上の階へ'),
    'Down Floor': ('Stockwerk runter', '下一层', '下の階へ'),
    'Far Clip Plane In': ('Sichtweite kleiner', '远裁剪面拉近', '遠クリップ面を近づける'),
    'Far Clip Plane Out': ('Sichtweite größer', '远裁剪面推远', '遠クリップ面を遠ざける'),
    'Next leak spot': ('Nächste Leckstelle', '下一个泄漏点', '次のリーク箇所'),
    'Previous leak spot': ('Vorige Leckstelle', '上一个泄漏点', '前のリーク箇所'),
    'Orthographic': ('2D-Ansicht', '正交视图', '平行投影'),
    'Next (XY, XZ, YZ)': ('Nächste (XY, XZ, YZ)', '下一个 (XY, XZ, YZ)', '次へ (XY, XZ, YZ)'),
    'XY (Top)': ('XY (oben)', 'XY(顶)', 'XY(上)'),
    'XZ (Front)': ('XZ (vorn)', 'XZ(前)', 'XZ(前)'),
    'YZ (Side)': ('YZ (Seite)', 'YZ(侧)', 'YZ(横)'),
    'Center on Selected': ('Auswahl mittig', '所选居中', '選択を中央に'),
    'XY 100%': ('XY 100%', 'XY 100%', 'XY 100%'),
    'XY Zoom In': ('XY vergrößern', 'XY 放大', 'XY 拡大'),
    'XY Zoom Out': ('XY verkleinern', 'XY 缩小', 'XY 縮小'),
    'Show': ('Anzeigen', '显示', '表示'),
    'Show Entity Angles': ('Entity-Winkel zeigen', '显示实体角度', 'エンティティの角度を表示'),
    'Show Entity Names': ('Entity-Namen zeigen', '显示实体名称', 'エンティティ名を表示'),
    'Show Light Radiuses': ('Lichtradien zeigen', '显示光照半径', 'ライト半径を表示'),
    'Show Entity Boxes': ('Entity-Kästen zeigen', '显示实体框', 'エンティティの箱を表示'),
    'Show Entity Connections': ('Entity-Verbindungen zeigen', '显示实体连接', 'エンティティの接続を表示'),
    'Show 2D Size Info': ('2D-Größe zeigen', '显示 2D 尺寸', '2D サイズを表示'),
    'Show 3D Size Info': ('3D-Größe zeigen', '显示 3D 尺寸', '3D サイズを表示'),
    'Show Crosshair': ('Fadenkreuz zeigen', '显示十字线', '十字線を表示'),
    'Show Grid': ('Raster zeigen', '显示网格', 'グリッドを表示'),
    'Show Blocks': ('Blöcke zeigen', '显示区块', 'ブロックを表示'),
    'Show Coordinates': ('Koordinaten zeigen', '显示坐标', '座標を表示'),
    'Show Window Outline': ('Fensterrahmen zeigen', '显示窗口轮廓', 'ウィンドウ枠を表示'),
    'Show Axes': ('Achsen zeigen', '显示坐标轴', '軸を表示'),
    'Show 2D Workzone': ('2D-Arbeitsbereich zeigen', '显示 2D 工作区', '2D 作業範囲を表示'),
    'Show 3D Workzone': ('3D-Arbeitsbereich zeigen', '显示 3D 工作区', '3D 作業範囲を表示'),
    'Show Renderer Stats': ('Renderer-Statistik zeigen', '显示渲染统计', 'レンダラー統計を表示'),
    'Filter': ('Filter', '过滤', 'フィルター'),
    'World': ('Welt', '世界', 'ワールド'),
    'Entities': ('Entities', '实体', 'エンティティ'),
    'Areaportals': ('Areaportals', '区域门户', 'エリアポータル'),
    'Translucent': ('Durchscheinend', '半透明', '半透明'),
    'Liquids': ('Flüssigkeiten', '液体', '液体'),
    'Caulk': ('Caulk', '填缝', 'コーク'),
    'Clips': ('Clips', '碰撞块', 'クリップ'),
    'Paths': ('Pfade', '路径', 'パス'),
    'Clusterportals': ('Clusterportals', '簇门户', 'クラスターポータル'),
    'Lights': ('Lichter', '灯光', 'ライト'),
    'Structural': ('Strukturell', '结构', 'ストラクチャル'),
    'Lightgrid': ('Lichtgitter', '光照网格', 'ライトグリッド'),
    'Patches': ('Patches', '曲面片', 'パッチ'),
    'Details': ('Detail', '细节', 'ディテール'),
    'Hints': ('Hints', '提示面', 'ヒント'),
    'Models': ('Modelle', '模型', 'モデル'),
    'Triggers': ('Trigger', '触发器', 'トリガー'),
    'Botclips': ('Bot-Clips', '机器人碰撞块', 'ボットクリップ'),
    'Decals': ('Decals', '贴花', 'デカール'),
    'FuncGroups': ('FuncGroups', '函数组', 'FuncGroup'),
    'Point Entities': ('Punkt-Entities', '点实体', 'ポイントエンティティ'),
    'Sky': ('Himmel', '天空', 'スカイ'),
    'Invert filters': ('Filter umkehren', '反转过滤', 'フィルターを反転'),
    'Reset filters': ('Filter zurücksetzen', '重置过滤', 'フィルターをリセット'),
    'Hide Selected': ('Auswahl ausblenden', '隐藏所选', '選択を非表示'),
    'Show Hidden': ('Ausgeblendete zeigen', '显示隐藏的', '非表示を表示'),
    'Region': ('Region', '区域', 'リージョン'),
    'Off': ('Aus', '关闭', 'オフ'),
    'Set XY': ('XY setzen', '设为 XY 视图', 'XY で設定'),
    'Set Brush': ('Brush setzen', '设为画刷', 'ブラシで設定'),
    'Set Selection': ('Auswahl setzen', '设为所选', '選択で設定'),
    # Modify
    'Components': ('Komponenten', '组件', 'コンポーネント'),
    'Edges': ('Kanten', '边', '辺'),
    'Vertices': ('Ecken', '顶点', '頂点'),
    'Faces': ('Flächen', '面', '面'),
    'Snap To Grid': ('Am Raster einrasten', '对齐到网格', 'グリッドにスナップ'),
    'Nudge': ('Schieben', '微移', '少し移動'),
    'Nudge Left': ('Nach links schieben', '向左微移', '左へ移動'),
    'Nudge Right': ('Nach rechts schieben', '向右微移', '右へ移動'),
    'Nudge Up': ('Nach oben schieben', '向上微移', '上へ移動'),
    'Nudge Down': ('Nach unten schieben', '向下微移', '下へ移動'),
    'Nudge +Z': ('In +Z schieben', '沿 +Z 微移', '+Z へ移動'),
    'Nudge -Z': ('In -Z schieben', '沿 -Z 微移', '-Z へ移動'),
    'Rotate': ('Drehen', '旋转', '回転'),
    'Rotate X': ('Um X drehen', '绕 X 旋转', 'X 軸で回転'),
    'Rotate Y': ('Um Y drehen', '绕 Y 旋转', 'Y 軸で回転'),
    'Rotate Z': ('Um Z drehen', '绕 Z 旋转', 'Z 軸で回転'),
    'Rotate Clockwise': ('Im Uhrzeigersinn drehen', '顺时针旋转', '時計回りに回転'),
    'Rotate Anticlockwise': ('Gegen den Uhrzeigersinn drehen', '逆时针旋转', '反時計回りに回転'),
    'Flip': ('Spiegeln', '翻转', '反転'),
    'Flip X': ('An X spiegeln', '沿 X 翻转', 'X で反転'),
    'Flip Y': ('An Y spiegeln', '沿 Y 翻转', 'Y で反転'),
    'Flip Z': ('An Z spiegeln', '沿 Z 翻转', 'Z で反転'),
    'Flip Horizontally': ('Waagerecht spiegeln', '水平翻转', '左右反転'),
    'Flip Vertically': ('Senkrecht spiegeln', '垂直翻转', '上下反転'),
    'Arbitrary rotation...': ('Beliebig drehen...', '任意旋转...', '任意の回転...'),
    'Arbitrary scale...': ('Beliebig skalieren...', '任意缩放...', '任意の拡大縮小...'),
    'Repeat': ('Wiederholen', '重复', '繰り返し'),
    'Repeat Transforms': ('Transformationen wiederholen', '重复变换', '変換を繰り返す'),
    'Reset Transforms': ('Transformationen zurücksetzen', '重置变换', '変換をリセット'),
    # Build
    'Customize...': ('Anpassen...', '自定义...', 'カスタマイズ...'),
    'Run recent build': ('Letzten Build ausführen', '运行上次编译', '前回のビルドを実行'),
    # Grid
    'Grid0.125': ('Raster 0.125', '网格 0.125', 'グリッド 0.125'),
    'Grid0.25': ('Raster 0.25', '网格 0.25', 'グリッド 0.25'),
    'Grid0.5': ('Raster 0.5', '网格 0.5', 'グリッド 0.5'),
    'Grid1': ('Raster 1', '网格 1', 'グリッド 1'),
    'Grid2': ('Raster 2', '网格 2', 'グリッド 2'),
    'Grid4': ('Raster 4', '网格 4', 'グリッド 4'),
    'Grid8': ('Raster 8', '网格 8', 'グリッド 8'),
    'Grid16': ('Raster 16', '网格 16', 'グリッド 16'),
    'Grid32': ('Raster 32', '网格 32', 'グリッド 32'),
    'Grid64': ('Raster 64', '网格 64', 'グリッド 64'),
    'Grid128': ('Raster 128', '网格 128', 'グリッド 128'),
    'Grid256': ('Raster 256', '网格 256', 'グリッド 256'),
    'Grid512': ('Raster 512', '网格 512', 'グリッド 512'),
    'Grid1024': ('Raster 1024', '网格 1024', 'グリッド 1024'),
    # Misc
    'Theming': ('Farbgebung', '主题', 'テーマ'),
    'Viewports Theme': ('Farben der Ansichten', '视口主题', 'ビューポートのテーマ'),
    'GUI Theme': ('Oberflächen-Thema', '界面主题', 'GUI テーマ'),
    'OpenGL Font...': ('Schrift der Ansichten...', 'OpenGL 字体...', 'OpenGL フォント...'),
    'Find brush...': ('Brush suchen...', '查找画刷...', 'ブラシを検索...'),
    'Map Info...': ('Karteninfo...', '地图信息...', 'マップ情報...'),
    'Refresh models': ('Modelle neu laden', '刷新模型', 'モデルを再読み込み'),
    'Set 2D Background image...': ('2D-Hintergrundbild...', '设置 2D 背景图...', '2D 背景画像を設定...'),
    'Fullscreen': ('Vollbild', '全屏', '全画面'),
    'Maximize view': ('Ansicht maximieren', '最大化视图', 'ビューを最大化'),
    # Entity
    'Connect Entities': ('Entities verbinden', '连接实体', 'エンティティを接続'),
    'Move Primitives to Entity': ('Primitive in Entity verschieben', '将图元移入实体', 'プリミティブをエンティティへ移動'),
    'Select Color...': ('Farbe wählen...', '选择颜色...', '色を選択...'),
    'Normalize Color': ('Farbe normalisieren', '归一化颜色', '色を正規化'),
    'Reload Entity Definitions': ('Entity-Definitionen neu laden', '重新加载实体定义', 'エンティティ定義を再読み込み'),
    # Brush
    'Prism...': ('Prisma...', '棱柱...', '角柱...'),
    'Cone...': ('Kegel...', '圆锥...', '円錐...'),
    'Sphere...': ('Kugel...', '球体...', '球...'),
    'Rock...': ('Fels...', '岩石...', '岩...'),
    'Icosahedron...': ('Ikosaeder...', '二十面体...', '正二十面体...'),
    'CSG': ('CSG', 'CSG', 'CSG'),
    'CSG Subtract': ('CSG Abziehen', 'CSG 减去', 'CSG 減算'),
    'CSG Merge': ('CSG Vereinen', 'CSG 合并', 'CSG 結合'),
    'CSG Wrap Merge': ('CSG Hülle vereinen', 'CSG 包裹合并', 'CSG ラップ結合'),
    'CSG Intersect': ('CSG Schnittmenge', 'CSG 相交', 'CSG 交差'),
    'Make Room': ('Raum bauen', '生成房间', '部屋を作る'),
    'CSG Tool': ('CSG-Werkzeug', 'CSG 工具', 'CSG ツール'),
    'Clipper': ('Clipper', '裁剪器', 'クリッパー'),
    'Clip selection': ('Auswahl abschneiden', '裁剪所选', '選択をクリップ'),
    'Split selection': ('Auswahl teilen', '分割所选', '選択を分割'),
    'Flip Clip orientation': ('Schnittrichtung umdrehen', '翻转裁剪方向', 'クリップ方向を反転'),
    'Make detail': ('Detail machen', '设为细节', 'ディテールにする'),
    'Make structural': ('Strukturell machen', '设为结构', 'ストラクチャルにする'),
    'Texture Lock': ('Textur sperren', '纹理锁定', 'テクスチャロック'),
    'Texture Vertex Lock': ('Textur an Ecken sperren', '纹理顶点锁定', 'テクスチャ頂点ロック'),
    'Reset Texture': ('Textur zurücksetzen', '重置纹理', 'テクスチャをリセット'),
    'AutoCaulk Selected': ('Auswahl automatisch caulken', '自动填缝所选', '選択を自動コーク'),
    # Curve
    'Simple Patch Mesh...': ('Einfaches Patch-Netz...', '简单曲面网格...', '単純なパッチメッシュ...'),
    'Bevel': ('Fase', '斜面', 'ベベル'),
    'End cap': ('Endkappe', '端盖', 'エンドキャップ'),
    'Cylinder (9x3)': ('Zylinder (9x3)', '圆柱 (9x3)', '円柱 (9x3)'),
    'Square Cylinder (9x3)': ('Eckiger Zylinder (9x3)', '方柱 (9x3)', '角柱 (9x3)'),
    'Exact Cylinder...': ('Genauer Zylinder...', '精确圆柱...', '正確な円柱...'),
    'Cone (9x3)': ('Kegel (9x3)', '圆锥 (9x3)', '円錐 (9x3)'),
    'Exact Cone...': ('Genauer Kegel...', '精确圆锥...', '正確な円錐...'),
    'Sphere (9x5)': ('Kugel (9x5)', '球体 (9x5)', '球 (9x5)'),
    'Exact Sphere...': ('Genaue Kugel...', '精确球体...', '正確な球...'),
    'Cap Selection': ('Auswahl abdecken', '封盖所选', '選択にキャップ'),
    'Insert/Delete': ('Einfügen/Löschen', '插入/删除', '挿入/削除'),
    'Insert (2) First Columns': ('(2) erste Spalten einfügen', '插入前 (2) 列', '先頭に (2) 列を挿入'),
    'Insert (2) Last Columns': ('(2) letzte Spalten einfügen', '插入后 (2) 列', '末尾に (2) 列を挿入'),
    'Insert (2) First Rows': ('(2) erste Zeilen einfügen', '插入前 (2) 行', '先頭に (2) 行を挿入'),
    'Insert (2) Last Rows': ('(2) letzte Zeilen einfügen', '插入后 (2) 行', '末尾に (2) 行を挿入'),
    'Del First (2) Columns': ('(2) erste Spalten löschen', '删除前 (2) 列', '先頭の (2) 列を削除'),
    'Del Last (2) Columns': ('(2) letzte Spalten löschen', '删除后 (2) 列', '末尾の (2) 列を削除'),
    'Del First (2) Rows': ('(2) erste Zeilen löschen', '删除前 (2) 行', '先頭の (2) 行を削除'),
    'Del Last (2) Rows': ('(2) letzte Zeilen löschen', '删除后 (2) 行', '末尾の (2) 行を削除'),
    'Matrix': ('Matrix', '矩阵', 'マトリックス'),
    'Invert': ('Umkehren', '反转', '反転'),
    'Transpose': ('Transponieren', '转置', '転置'),
    'Re-disperse Rows': ('Zeilen neu verteilen', '重新分布行', '行を再配置'),
    'Re-disperse Columns': ('Spalten neu verteilen', '重新分布列', '列を再配置'),
    'Smooth Rows': ('Zeilen glätten', '平滑行', '行を平滑化'),
    'Smooth Columns': ('Spalten glätten', '平滑列', '列を平滑化'),
    'Texture': ('Textur', '纹理', 'テクスチャ'),
    'Naturalize': ('Natürlich ausrichten', '自然化', '自然に合わせる'),
    'Invert X': ('X umkehren', '反转 X', 'X を反転'),
    'Invert Y': ('Y umkehren', '反转 Y', 'Y を反転'),
    'Deform...': ('Verformen...', '变形...', '変形...'),
    'Thicken...': ('Verdicken...', '加厚...', '厚みを付ける...'),
    # Texturbrowser
    'Hide Unused': ('Unbenutzte ausblenden', '隐藏未使用的', '未使用を隠す'),
    'Show All': ('Alle zeigen', '显示全部', 'すべて表示'),
    'Show Shaders': ('Shader zeigen', '显示着色器', 'シェーダーを表示'),
    'Show Textures': ('Texturen zeigen', '显示纹理', 'テクスチャを表示'),
    'ShaderList Only': ('Nur shaderlist.txt', '仅着色器列表', 'シェーダーリストのみ'),
    'Hide Image Missing': ('Ohne Bild ausblenden', '隐藏缺少图像的', '画像なしを隠す'),
    'Fixed Size': ('Feste Größe', '固定大小', '固定サイズ'),
    'Transparency': ('Transparenz', '透明度', '透明度'),
    'Tags GUI': ('Schlagwörter', '标签界面', 'タグ GUI'),
    'Tags': ('Schlagwörter', '标签', 'タグ'),
    'Add tag': ('Schlagwort hinzufügen', '添加标签', 'タグを追加'),
    'Rename tag': ('Schlagwort umbenennen', '重命名标签', 'タグの名前を変更'),
    'Delete tag': ('Schlagwort löschen', '删除标签', 'タグを削除'),
    'Copy tags from selected': ('Schlagwörter der Auswahl kopieren', '从所选复制标签', '選択からタグをコピー'),
    'Paste tags to selected': ('Schlagwörter in Auswahl einfügen', '将标签粘贴到所选', '選択にタグを貼り付け'),
    'Search tag': ('Schlagwort suchen', '搜索标签', 'タグを検索'),
    'Search Untagged': ('Ohne Schlagwort suchen', '搜索无标签的', 'タグなしを検索'),
    'Find / Replace...': ('Suchen / Ersetzen...', '查找/替换...', '検索/置換...'),
    'Flush & Reload Shaders': ('Shader neu laden', '刷新并重新加载着色器', 'シェーダーを再読み込み'),
    # Help
    'Bug report': ('Fehler melden', '报告错误', 'バグ報告'),
    'Check for NetRadiant update': ('Nach NetRadiant-Update suchen', '检查 NetRadiant 更新', 'NetRadiant の更新を確認'),
    'About': ('Über', '关于', 'バージョン情報'),
    # Werkzeugleisten (Beschreibungen, erste Zeile)
    'Open an existing map': ('Vorhandene Karte öffnen', '打开现有地图', '既存のマップを開く'),
    'Save the active map': ('Aktive Karte speichern', '保存当前地图', '現在のマップを保存'),
    'Select Vertices': ('Ecken wählen', '选择顶点', '頂点を選択'),
    'Select Edges': ('Kanten wählen', '选择边', '辺を選択'),
    'Select Faces': ('Flächen wählen', '选择面', '面を選択'),
    'Change views': ('Ansicht wechseln', '切换视图', 'ビューを切り替え'),
    'Cubic clip the camera view': ('Kamerasicht würfelförmig begrenzen', '立方体裁剪摄像机视图', 'カメラビューを立方体でクリップ'),
    'Resize (Q)': ('Größe ändern (Q)', '调整大小 (Q)', 'サイズ変更 (Q)'),
    'Translate': ('Verschieben', '平移', '移動'),
    'Scale': ('Skalieren', '缩放', '拡大縮小'),
    'Transform (Q)': ('Transformieren (Q)', '变换 (Q)', '変形 (Q)'),
    'UV Tool': ('UV-Werkzeug', 'UV 工具', 'UV ツール'),
    'Put caps on the current patch': ('Aktuellen Patch abdecken', '为当前曲面片加盖', '現在のパッチにキャップ'),
    'Refresh Models': ('Modelle neu laden', '刷新模型', 'モデルを再読み込み'),
    'Room': ('Raum', '房间', '部屋'),
    'Select touching': ('Berührende wählen', '选择接触的', '接触しているものを選択'),
    'Func_Groups': ('Func_Groups', '函数组', 'Func_Group'),
    'HintsSkips': ('Hints/Skips', '提示/跳过面', 'ヒント/スキップ'),
    'ResetFilters': ('Filter zurücksetzen', '重置过滤', 'フィルターをリセット'),
    'Region Set Selection': ('Region auf Auswahl', '区域设为所选', 'リージョンを選択で設定'),
    'RightClick: MakeStructural': ('Rechtsklick: strukturell machen', '右键: 设为结构', '右クリック: ストラクチャルにする'),
    'RightClick: MakeDetail': ('Rechtsklick: Detail machen', '右键: 设为细节', '右クリック: ディテールにする'),
    'RightClick: create func_group': ('Rechtsklick: func_group anlegen', '右键: 创建 func_group', '右クリック: func_group を作成'),
    'RightClick: toggle tex': ('Rechtsklick: Textur umschalten', '右键: 切换纹理', '右クリック: テクスチャ切り替え'),
    'RightClick: tex Caulk': ('Rechtsklick: Textur Caulk', '右键: 纹理 Caulk', '右クリック: テクスチャ Caulk'),
    'RightClick: tex Trigger': ('Rechtsklick: Textur Trigger', '右键: 纹理 Trigger', '右クリック: テクスチャ Trigger'),
    'RightClick: Region Off': ('Rechtsklick: Region aus', '右键: 关闭区域', '右クリック: リージョンをオフ'),
    'RightClick: Show Hidden': ('Rechtsklick: Ausgeblendete zeigen', '右键: 显示隐藏的', '右クリック: 非表示を表示'),
}


def ohne_mnemonik(text):
    """Qt-Mnemonik entfernen: "&&" -> "&", "&x" -> "x"."""
    raus = []
    i = 0
    while i < len(text):
        if text[i] == '&' and i + 1 < len(text):
            i += 1
        raus.append(text[i])
        i += 1
    return ''.join(raus)


def kennung(praefix, text):
    """Str-Kennung aus einem englischen Text: KeL + Buchstaben/Ziffern."""
    t = ohne_mnemonik(text)
    teile = []
    wort = ''
    for c in t:
        if c.isalnum():
            wort += c
        else:
            if wort:
                teile.append(wort)
            wort = ''
            if c == '+':
                teile.append('Plus')
            elif c == '-' and not teile:
                teile.append('Minus')
            elif c == '%':
                teile.append('Prozent')
    if wort:
        teile.append(wort)
    return praefix + ''.join(w[0].upper() + w[1:] for w in teile)


def beschriftungen():
    """Alle Menue-Beschriftungen (ohne '&') in Reihenfolge, ohne Doppelte."""
    raus = []

    def lauf(eintraege):
        for e in eintraege:
            if e[0] in ('ITEM', 'CHECK', 'RUF'):
                t = ohne_mnemonik(e[1])
                if t not in raus:
                    raus.append(t)
            elif e[0] == 'SUB':
                t = ohne_mnemonik(e[1])
                if t not in raus:
                    raus.append(t)
                lauf(e[2])

    for titel, _, eintraege in MENUES + TEXTUR_MENUES:
        t = ohne_mnemonik(titel)
        if t not in raus:
            raus.append(t)
        lauf(eintraege)
    return raus


def knopf_texte():
    """Die Beschreibungen der Werkzeugknoepfe, zeilenweise (ohne Tabulator)."""
    raus = []
    for liste in (WERKZEUGE, FILTER, TEXTUR_KNOEPFE):
        for e in liste:
            if e[0] != 'KNOPF':
                continue
            for zeile in e[2].split('\n'):
                z = zeile.strip()
                if z and z not in raus:
                    raus.append(z)
    return raus


def i18n_eintraege():
    """(Kennung, English, Deutsch, 中文, 日本語) fuer tools/gen_i18n.py."""
    raus = []
    gesehen = set()
    for t in beschriftungen():
        k = kennung('KeL', t)
        if k in gesehen:
            continue
        gesehen.add(k)
        de, zh, ja = UEBERSETZUNG[t]
        raus.append((k, t, de, zh, ja))
    for t in knopf_texte():
        if t in ('noDraw', 'noDrawNonSolid', 'waterCaulk', 'lavaCaulk', 'slimeCaulk', 'playerClip', 'weapClip',
                 'hint', 'hintLocal', 'hintSkip'):
            continue   # Texturnamen, bleiben wie sie sind
        k = kennung('KeT', t)
        if k in gesehen:
            continue
        gesehen.add(k)
        de, zh, ja = UEBERSETZUNG[t]
        raus.append((k, t, de, zh, ja))
    return raus
