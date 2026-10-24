# -*- coding: utf-8 -*-
"""Erzeugt src/karte/befehle_liste.inc - die registrierten Befehle von
NetRadiant-Custom (Name, Art, Standardtaste, Quelle, Stufe) - aus der
Checkliste tools/karte/nrc_inventar.md, und setzt dort den Stand der
Befehle, deren Funktion erst in einer spaeteren Stufe kommt, auf [~].

NRC haelt alle Tastenkuerzel in EINER Tabelle nach Namen (g_shortcuts,
radiant/commands.cpp): ein Name kann zugleich Command und KeyEvent sein
(CameraForward), die Taste setzt der erste Eintrag (GlobalShortcuts_insert
gibt den vorhandenen zurueck). Hier: je Name ein Eintrag, die Taste ist die
nicht leere unter den Registrierungen.

Qt-Ausdruecke werden in Qt-Schreibweise uebersetzt (QKeySequence::toString,
PortableText: Meta+Ctrl+Alt+Shift+Num+Taste):
  +Qt::SHIFT + Qt::Key_Plus + Qt::KeypadModifier   ->  Shift+Num++

  python -I tools/karte/gen_befehle.py
"""
import os
import re
import sys

HIER = os.path.dirname(os.path.abspath(__file__))
WURZEL = os.path.dirname(os.path.dirname(HIER))
MD = os.path.join(HIER, 'nrc_inventar.md')
INC = os.path.join(WURZEL, 'src', 'karte', 'befehle_liste.inc')
MENUE_INC = os.path.join(WURZEL, 'src', 'karte', 'menues.inc')
sys.path.insert(0, os.path.join(WURZEL, 'tools'))
sys.path.insert(0, HIER)
import karte_menues as km  # noqa: E402
import nrc_inventar as inv  # noqa: E402

BEDINGUNG = {'': 'kImmer', 'floating': 'kNurFloating', 'nichtRegular': 'kNichtRegular', 'texturen': 'kTexturen',
             'nichtSplit': 'kNichtSplit', 'split': 'kNurSplit'}

# Befehle, deren Funktion in Stufe 1b steht (gepflegt beim Nachbauen).
STUFE_1B = set('''
NewMap OpenMap SaveMap SaveMapAs Exit Preferences
ToggleCamera ToggleView ToggleFrontView ToggleSideView MaximizeView
ToggleEntityInspector ToggleConsole ToggleTextures ToggleModelBrowser ToggleLayersBrowser
Shortcuts GridUp GridDown ToggleGridSnap SetGrid0.125 SetGrid0.25 SetGrid0.5 SetGrid1 SetGrid2 SetGrid4 SetGrid8
SetGrid16 SetGrid32 SetGrid64 SetGrid128 SetGrid256 SetGrid512 SetGrid1024
NextView ViewTop ViewFront ViewSide ZoomIn ZoomOut Zoom100 CenterXYView XYFocusOnSelected
ShowSize2d ToggleCrosshairs ToggleGrid ShowAngles ShowNames ShowBboxes ShowConnections ShowLightRadiuses
ShowBlocks ShowCoordinates ShowWindowOutline ShowAxes ShowWorkzone2d
CenterView CameraFocusOnSelected ToggleCubicClip CubicClipZoomIn CubicClipZoomOut UpFloor DownFloor
CameraModeNext CameraModePrev CameraSpeedInc CameraSpeedDec ShowStats ShowWorkzone3d ShowSize3d
CameraForward CameraBack CameraLeft CameraRight CameraStrafeRight CameraStrafeLeft CameraUp CameraDown
CameraAngleUp CameraAngleDown CameraFreeMoveForward CameraFreeMoveBack CameraFreeMoveLeft CameraFreeMoveRight
CameraFreeMoveForward2 CameraFreeMoveBack2 CameraFreeMoveLeft2 CameraFreeMoveRight2 CameraFreeMoveUp
CameraFreeMoveDown CameraFreeFocus
Undo Redo Redo2 Copy Paste PasteToCamera MoveToCamera CloneSelection CloneSelectionAndMakeUnique
DeleteSelection DeleteSelection2 DeleteSelection3 UnSelectSelection UnSelectSelection2 InvertSelection
SelectAllOfType HideSelected ShowHidden SnapToGrid SelectNudgeLeft SelectNudgeRight SelectNudgeUp SelectNudgeDown
MoveSelectionUP MoveSelectionDOWN MouseDrag MouseTranslate MouseRotate MouseScale MouseTransform
MouseDragOrTransform MouseRotateOrScale
ShowInUse ShowAllTextures ToggleShowShaders ToggleShowTextures ToggleShowShaderlistOnly FixedSize FilterNotex
EnableAlpha SearchFromStart RefreshShaders FindReplaceTextures
EntitiesConnect EntityColorSet EntityColorNormalize EntityMovePrimitivesToLast EntityMovePrimitivesToFirst
EntityUngroup EntityUngroupPrimitives EntityReloadDefinitions ExpandSelectionToEntities SelectConnectedEntities
ToggleEntityList
'''.split())

# Befehle, deren Funktion in Stufe 2 nachgebaut ist (gepflegt beim Nachbauen);
# sie gelten wie die der Stufe 1b als fertig (Stufe 1 in befehle_liste.inc).
STUFE_2_FERTIG = set('''
DragVertices DragEdges DragFaces MouseBuild MouseUV TogTexLock TogTexVertexLock
ToggleClipper ClipperClip ClipperSplit ClipperFlip
CSGSubtract CSGMerge CSGWrapMerge CSGIntersect CSGroom CSGTool MakeDetail MakeStructural
BrushPrism BrushCone BrushSphere BrushRock BrushIcosahedron Brush3Sided Brush4Sided Brush5Sided Brush6Sided Brush7Sided
Brush8Sided Brush9Sided
RotateSelectionX RotateSelectionY RotateSelectionZ RotateSelectionClockwise RotateSelectionAnticlockwise MirrorSelectionX
MirrorSelectionY MirrorSelectionZ MirrorSelectionHorizontally MirrorSelectionVertically ArbitraryRotation ArbitraryScale
RepeatTransforms ResetTransforms
SimplePatchMesh PatchBevel PatchEndCap PatchCylinder PatchSquareCylinder PatchXactCylinder PatchCone PatchXactCone PatchSphere
PatchXactSphere CapCurrentCurve PatchInsertFirstColumn PatchInsertLastColumn PatchInsertFirstRow PatchInsertLastRow
PatchDeleteFirstColumn PatchDeleteLastColumn PatchDeleteFirstRow PatchDeleteLastRow InvertCurve MatrixTranspose RedisperseRows
RedisperseCols SmoothRows SmoothCols TextureReset/Cap NaturalizePatch InvertCurveTextureX InvertCurveTextureY PatchDeform
PatchThicken
SurfaceInspector FitTexture FitTextureWidth FitTextureHeight FitTextureWidthOnly FitTextureHeightOnly TextureProjectAxial
TextureProjectOrtho TextureProjectCam TexRotateClock TexRotateCounter TexScaleUp TexScaleDown TexScaleLeft TexScaleRight
TexShiftUp TexShiftDown TexShiftLeft TexShiftRight SelectTextured
'''.split())

# Stufe 3: Kompilieren, Lecksuche, Plugins, Fenster, die es erst mit dem
# Kompilieren gibt. Alles andere ohne Eintrag oben: Stufe 2 (Werkzeuge).
STUFE_3 = set('''
BuildMenuCustomize Build_runRecentExecutedBuild TogglePointfile NextLeakSpot PrevLeakSpot
CheckForUpdate OpenManual
AutoCaulkSelected
'''.split())

QT_TASTEN = {
    'Key_Plus': '+', 'Key_Minus': '-', 'Key_1': '1', 'Key_3': '3', 'Key_7': '7',
}


def qt_text(ausdruck):
    teile = [t.strip().lstrip('+').strip() for t in ausdruck.split('+') if t.strip()]
    mods = set()
    taste = ''
    for t in teile:
        if t == 'Qt::CTRL':
            mods.add('Ctrl')
        elif t == 'Qt::SHIFT':
            mods.add('Shift')
        elif t == 'Qt::ALT':
            mods.add('Alt')
        elif t == 'Qt::KeypadModifier':
            mods.add('Num')
        elif t.startswith('Qt::Key_'):
            taste = QT_TASTEN[t[4:]]
        else:
            raise SystemExit('unbekannter Qt-Ausdruck: ' + ausdruck)
    reihe = [m for m in ('Meta', 'Ctrl', 'Alt', 'Shift', 'Num') if m in mods]
    return '+'.join(reihe + [taste])


def stufe(name):
    if name in STUFE_1B or name in STUFE_2_FERTIG:
        return 1
    if name in STUFE_3 or '::' in name:
        return 3
    return 2


def lesen():
    abschnitt = ''
    befehle = {}
    reihenfolge = []
    for z in open(MD, encoding='utf-8').read().split('\n'):
        if z.startswith('## '):
            abschnitt = z[3:].strip()
            continue
        if abschnitt != 'Registrierte Befehle' or not z.startswith('- ['):
            continue
        m = re.match(r'- \[.\] `([^`]+)` \| (\w+) \| (.*?) \| \((.*?)\)', z)
        if not m:
            raise SystemExit('Zeile nicht lesbar: ' + z)
        name, art, taste, quelle = m.groups()
        taste = '' if taste == '-' else taste.replace(chr(92) * 2, chr(92))   # C-Zeichenkette: "\\\\" ist ein "\\"
        if 'Qt::' in taste:
            taste = qt_text(taste)
        if name not in befehle:
            befehle[name] = {'arten': set(), 'taste': '', 'quelle': quelle}
            reihenfolge.append(name)
        b = befehle[name]
        if art != 'Shortcuts':
            b['arten'].add(art)
            if art != 'KeyEvents' or not b['quelle']:
                b['quelle'] = quelle
        if taste and not b['taste']:
            b['taste'] = taste
    return befehle, reihenfolge


def cstr(s):
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n').replace('\t', '\\t') + '"'


def menues_inc(befehle):
    z = ['// Erzeugt von tools/karte/gen_befehle.py aus tools/karte_menues.py - nicht von Hand aendern.',
         '// Menues (create_main_menu, mainframe.cpp:1225) und Werkzeugleisten von NetRadiant-Custom.']
    z.append('const MenueEintrag kMenues[] = {')
    ids = {}

    def str_id(praefix, text):
        k = km.kennung(praefix, text)
        t = km.ohne_mnemonik(text)
        if ids.setdefault(k, t) != t:
            raise SystemExit('Kennung doppelt: %s fuer "%s" und "%s"' % (k, ids[k], t))
        return 'Str::' + k

    def lauf(eintraege):
        for e in eintraege:
            art = e[0]
            if art == 'SEP':
                bed = e[1] if len(e) > 1 else ''
                z.append('    {MenueArt::Trenner, "", "", Str::Count, %s},' % BEDINGUNG[bed])
            elif art in ('ITEM', 'CHECK'):
                if e[2] not in befehle:
                    raise SystemExit('Menue nennt unbekannten Befehl: ' + e[2])
                z.append('    {MenueArt::%s, %s, "%s", %s, %s},' % ('Punkt' if art == 'ITEM' else 'Haken', cstr(e[1]),
                                                                    e[2], str_id('KeL', e[1]), BEDINGUNG[e[3]]))
            elif art == 'RUF':
                z.append('    {MenueArt::Ruf, %s, "%s", %s, %s},' % (cstr(e[1]), e[2], str_id('KeL', e[1]),
                                                                   BEDINGUNG[e[3]]))
            elif art == 'LISTE':
                z.append('    {MenueArt::Liste, "", "%s", Str::Count, kImmer},' % e[1])
            elif art == 'SUB':
                z.append('    {MenueArt::Unter, %s, "", %s, kImmer},' % (cstr(e[1]), str_id('KeL', e[1])))
                lauf(e[2])
                z.append('    {MenueArt::Ende, "", "", Str::Count, kImmer},')
            else:
                raise SystemExit('unbekannter Eintrag: %r' % (e,))

    for name, liste in (('kMenues', km.MENUES), ('kTexturMenues', km.TEXTUR_MENUES)):
        if name != 'kMenues':
            z.append('const MenueEintrag %s[] = {' % name)
        for titel, quelle, eintraege in liste:
            z.append('    // %s (%s)' % (km.ohne_mnemonik(titel), quelle))
            z.append('    {MenueArt::Menue, %s, "", %s, kImmer},' % (cstr(titel), str_id('KeL', titel)))
            lauf(eintraege)
            z.append('    {MenueArt::Ende, "", "", Str::Count, kImmer},')
        z.append('};')

    def knopf(e, filter_):
        if e[0] == 'TRENNER':
            bed = e[1] if (len(e) > 1 and not filter_) else ''
            return '    {"", "", "", false, %s, "", true, {Str::Count, Str::Count, Str::Count, Str::Count, Str::Count}},' % \
                BEDINGUNG[bed]
        _, befehl, beschr, bild, schalter, extra = e
        if befehl and befehl not in befehle:
            raise SystemExit('Knopf nennt unbekannten Befehl: ' + befehl)
        zeilen = [x.strip() for x in beschr.split('\n')]
        strs = []
        for x in zeilen:
            k = km.kennung('KeT', x)
            strs.append('Str::' + k if x in km.UEBERSETZUNG else 'Str::Count')
        while len(strs) < 5:
            strs.append('Str::Count')
        bed = 'kImmer' if filter_ else BEDINGUNG[extra]
        rk = extra if filter_ else ''
        return '    {"%s", %s, "%s", %s, %s, "%s", false, {%s}},' % (befehl, cstr(beschr), bild,
                                                                      'true' if schalter else 'false', bed, rk,
                                                                      ', '.join(strs[:5]))

    z.append('const Knopf kWerkzeuge[] = {')
    for e in km.WERKZEUGE:
        z.append(knopf(e, False))
    z.append('};')
    z.append('const Knopf kFilter[] = {')
    for e in km.FILTER:
        z.append(knopf(e, True))
    z.append('};')
    z.append('const Knopf kTexturKnoepfe[] = {')
    for e in km.TEXTUR_KNOEPFE:
        z.append(knopf(e, False))
    z.append('};')
    z.append('const char* const kVersteckt[] = {' + ', '.join('"%s"' % v for v in km.VERSTECKT) + '};')
    # Plugin-Menues (PlugInMenu_Add, radiant/pluginmenu.cpp:37-90): je Plugin
    # die Eintraege von hinten nach vorn; Titel "" = Trenner.
    z.append('const PluginEintrag kPluginEintraege[] = {')
    for name, quelle, liste in inv.PLUGINS:
        z.append('    // %s (%s)' % (name, quelle))
        for e in reversed(liste):
            if e == '-':
                z.append('    {"%s", "", ""},' % name)
            else:
                befehl = inv.plugin_befehlsname(name, e[0])
                if befehl not in befehle:
                    raise SystemExit('Plugin-Befehl fehlt in der Checkliste: ' + befehl)
                z.append('    {"%s", %s, "%s"},' % (name, cstr(e[1]), befehl))
    z.append('};')
    return '\n'.join(z) + '\n'


def schreib_wenn_anders(pfad, text):
    alt = open(pfad, encoding='utf-8').read() if os.path.exists(pfad) else ''
    if alt != text:
        open(pfad, 'w', encoding='utf-8', newline='\n').write(text)


def checkliste_markieren(befehle):
    """Eintraege der Checkliste, deren Befehl erst spaeter kommt: [~] mit Stufe.
    Bereits fertige ([x]) bleiben; Stufe-1b-Befehle ohne [x] bleiben offen."""
    zeilen = open(MD, encoding='utf-8').read().split('\n')
    abschnitt = ''
    n = 0
    for i, z in enumerate(zeilen):
        if z.startswith('## '):
            abschnitt = z[3:].strip()
            continue
        m = re.match(r'^(\s*)- \[([ x~])\] (.*)$', z)
        if not m or m.group(2) == 'x':
            continue
        text = m.group(3).split(' -- ')[0]
        name = None
        if abschnitt == 'Registrierte Befehle':
            mm = re.match(r'`([^`]+)`', text)
            name = mm.group(1) if mm else None
        elif abschnitt in ('Menues', 'Werkzeugleisten', 'Plugins'):
            mm = re.search(r'-> `([^`]+)`', text)
            name = mm.group(1) if mm else None
        if name is None or name not in befehle:
            continue
        s = stufe(name)
        if s >= 2:
            neu = '%s- [~] %s -- Stufe %d' % (m.group(1), text, s)
        else:
            neu = '%s- [ ] %s' % (m.group(1), text)
        if neu != z:
            zeilen[i] = neu
            n += 1
    open(MD, 'w', encoding='utf-8', newline='\n').write('\n'.join(zeilen))
    return n


def main():
    befehle, reihenfolge = lesen()
    zeilen = ['// Erzeugt von tools/karte/gen_befehle.py aus tools/karte/nrc_inventar.md -',
              '// nicht von Hand aendern. Name, Art (Bits: 1 Command, 2 Toggle, 4 KeyEvent),',
              '// NRC-Standardtaste (Qt-Schreibweise), Quelle, Stufe.']
    bits = {'Commands': 1, 'Toggles': 2, 'KeyEvents': 4}
    for name in sorted(reihenfolge, key=str.lower):
        b = befehle[name]
        art = sum(bits[a] for a in b['arten']) or 4   # nur GlobalShortcuts_insert: KeyEvent-Platzhalter
        zeilen.append('{"%s", %d, "%s", "%s", %d},' % (name, art, b['taste'].replace('\\', '\\\\'),
                                                               b['quelle'], stufe(name)))
    schreib_wenn_anders(INC, '\n'.join(zeilen) + '\n')
    schreib_wenn_anders(MENUE_INC, menues_inc(befehle))
    n = checkliste_markieren(befehle)
    print('%d Befehle -> %s, Menues -> %s, %d Eintraege der Checkliste neu markiert' % (
        len(reihenfolge), os.path.relpath(INC, WURZEL), os.path.relpath(MENUE_INC, WURZEL), n))


if __name__ == '__main__':
    sys.exit(main())
