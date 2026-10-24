// bearbeitung.h - Karten-Editor: Ausgewaehltes verschieben, drehen,
// skalieren, verzerren, in der Groesse aendern und neue Pinsel aufziehen.
//
// Portiert aus NetRadiant-Custom 20260114: radiant/selection.cpp
// (RadiantSelectionSystem: SetManipulatorMode, SelectManipulator,
// HighlightManipulator, translate/rotate/scale/skew/alltransform,
// translateSelected/rotateSelected/scaleSelected, MoveSelected, endMove,
// freezeTransforms, ConstructPivot, setCustomTransformOrigin,
// getSelectionAABB, renderSolid; TranslateSelected, rotate_selected,
// scale_selected, skew_selected, transform_selected, *_component_selected,
// translation_for_pivoted_*, BuildManipulator, TransformOriginTranslate,
// TransformOriginManipulator, testselect_scene_point_unselected,
// testSelected_scene_snapped_point, testSelect_unselected_scene_point),
// radiant/selection_mtable_translate.h (TranslateAxis, TranslateAxis2,
// TranslateFree, SnapBounds, TranslateFreeXY_Z),
// radiant/selection_mtable_rotate.h (RotateFree, RotateAxis),
// radiant/selection_mtable_scale.h (ScaleAxis, ScaleFree),
// radiant/selection_mtable_skew.h (SkewAxis),
// radiant/selection_mtable_brush.h (DragNewBrush),
// radiant/selection_mtor_translate.cpp, selection_mtor_rotate.cpp,
// selection_mtor_scale.cpp, selection_mtor_skew.cpp, selection_mtor_drag.cpp
// (die Manipulatoren), libs/dragplanes.h (DragPlanes), radiant/brush.h
// (FaceInstance::trySelectPlane/selectReversedPlane/transformComponents,
// BrushInstance::selectPlanes/bestPlaneDirect/bestPlaneIndirect/
// selectByPlane/gatherPolygonsByPlane/evaluateTransform, EdgeInstance::
// bestPlaneIndirect), radiant/patch.h (PatchInstance: DragPlanes),
// radiant/brushmanip.cpp (Scene_BrushResize_Cuboid), radiant/xywindow.cpp
// (NewBrushDrag_Begin/NewBrushDrag/NewBrushDrag_End) und plugins/entity
// (generic.cpp, eclassmodel.cpp, miscmodel.cpp, light.cpp, origin.h,
// angles.h, scale.h: translate/rotate/scale/freezeTransform der
// Punktentitaeten).
//
// Wie NRC wird jede Bewegung vom Ausgangszustand aus neu gerechnet (NRC:
// revertTransform + evaluateTransform; hier: Zustand vom Beginn laden und die
// ganze Transformation anwenden). Das Rueckgaengig beginnt mit der ersten
// Bewegung und endet mit dem Loslassen (Name wie NRC, z. B.
// "translateTool -xyz 16 0 0").
//
//   Copyright (C) 2001-2006, William Joseph.
//   All Rights Reserved.
//   This file is part of GtkRadiant.
//
//   GtkRadiant is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. GtkRadiant is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#ifndef BHED_KARTE_BEARBEITUNG_H
#define BHED_KARTE_BEARBEITUNG_H

#include "bhed/karte/auswahlsystem.h"
#include "bhed/karte/manipulator.h"

#include <memory>
#include <string>

namespace bhed::karte {

class Karte;
class XyAnsicht;

// SelectionSystem::EManipulatorMode (Reihenfolge wie NRC)
enum class ManipModus { Verschieben, Drehen, Skalieren, Verzerren, Ziehen, Clipper, Bauen, UV };

class AuswahlBearbeitung {
public:
    explicit AuswahlBearbeitung(Karte& k);
    ~AuswahlBearbeitung();
    AuswahlBearbeitung(const AuswahlBearbeitung&) = delete;
    AuswahlBearbeitung& operator=(const AuswahlBearbeitung&) = delete;

    // --- Einstellungen, die NRC global haelt -----------------------------------
    AuswahlEinstellung einstellung;            // Mode(), ComponentMode(), m_bPreferPointEntsIn2D
    Zusatz zusatz;                             // g_modifiers
    float raster = 8;                          // GetGridSize
    float fang = 8;                            // GetSnapGridSize
    bool pinselIn3d = true;                    // g_3DCreateBrushes ("Create brushes in 3D")
    bool immerCaulk = false;                   // g_brush_always_caulk
    bool texturEckenSperre = false;            // g_brush_textureVertexlock_enabled ("TogTexVertexLock")
    std::string browserShader = "textures/";   // TextureBrowser_GetSelectedShader (texdef_name_default)
    std::string caulkShader = "textures/common/caulk";   // GetCaulkShader
    Kasten arbeitsbereich{Vektor3f(0, 0, 0), Vektor3f(64, 64, 64)};   // Select_getWorkZone (-64..64)
    float waehlEpsilon = 12;                   // g_SELECT_EPSILON ("Selector size (pixels)")
    int bewegStil3d = 0;                       // TranslateFreeXY_Z::m_viewdependent ("Move style in 3D")

    // --- RadiantSelectionSystem -------------------------------------------------
    void setzeModus(ManipModus m);   // SetManipulatorMode
    [[nodiscard]] ManipModus modus() const;
    [[nodiscard]] bool etwasGewaehlt() const;   // somethingSelected
    // SelectManipulator: true, wenn ein Manipulator (oder der Ursprung) greift.
    bool manipulatorWaehlen(const Blick& blick, const GeraeteVektor& punkt, const GeraeteVektor& eps);
    void manipulatorHervorheben(const Blick& blick, const GeraeteVektor& punkt, const GeraeteVektor& eps);   // HighlightManipulator
    void auswahlBewegen(const Blick& blick, const GeraeteVektor& punkt);   // MoveSelected
    bool bewegenBeenden();   // endMove: true, wenn der Transformationsursprung bewegt wurde
    void manipulatorAbwaehlen();   // m_manipulator->setSelected( false )
    void verschiebeAuswahl(const Vektor3f& v);   // translateSelected
    void dreheAuswahl(const Quaternion& q, bool ursprungEinrasten = false);   // rotateSelected
    void skaliereAuswahl(const Vektor3f& s, bool ursprungEinrasten = false);  // scaleSelected
    void pivotGeaendert();   // pivotChanged
    [[nodiscard]] const Matrix4& pivotZuWelt();   // GetPivot2World
    [[nodiscard]] const Kasten& grenzen();        // m_bounds (nach ConstructPivot)
    [[nodiscard]] bool pivotEigen() const;        // m_pivotIsCustom
    [[nodiscard]] bool bewegtSich() const;        // m_pivot_moving
    [[nodiscard]] bool rueckgaengigOffen() const;   // m_undo_begun
    // Ursprung-Manipulator verschiebbar? (transformOrigin_isTranslatable)
    [[nodiscard]] bool ursprungVerschiebbar() const;
    // renderSolid: was der Manipulator in dieser Ansicht zeichnet.
    void zeichnen(const Blick& blick, ManipZeichnung& aus);
    // setCustomTransformOrigin (XYWnd::SetCustomPivotOrigin, Shift+M3 in 2D)
    void eigenenUrsprungSetzen(const Vektor3f& ursprung, const bool gesetzt[3]);
    // Map_Free: alles vergessen, was auf die alte Karte zeigt.
    void neueKarte();

    // --- NewBrushDrag (xywindow.cpp:684-740) -------------------------------------
    // XY_MouseDown: nur ohne Auswahl und nicht im Clipper.
    [[nodiscard]] bool neuerPinselMoeglich() const;
    void neuerPinselBeginnen(const XyAnsicht& v, int x, int y);   // NewBrushDrag_Begin
    void neuerPinselZiehen(const XyAnsicht& v, int x, int y, bool quadrat, bool wuerfel);   // NewBrushDrag
    bool neuerPinselBeenden();   // NewBrushDrag_End: true, wenn ein Pinsel entstand
    [[nodiscard]] bool neuerPinselAktiv() const;   // m_bNewBrushDrag

    // Scene_BrushResize_Cuboid: legt beim ersten Aufruf einen Pinsel in
    // worldspawn an (ausgewaehlt), sonst baut es "pinsel" neu.
    void quaderGroesse(std::shared_ptr<Pinsel>& pinsel, const Kasten& grenzen);

    struct Impl;

private:
    std::unique_ptr<Impl> m;
};

// Texte der Rueckgaengig-Befehle (endMove: outputTranslation, outputRotation,
// outputScale) - "%g" wie NRCs TextOutputStream.
[[nodiscard]] std::string zahlText(float f);

}  // namespace bhed::karte

#endif
