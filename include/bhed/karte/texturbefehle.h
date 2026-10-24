// texturbefehle.h - Karten-Editor: die Textur-Befehle der Auswahl (Surface
// Inspector, Shift/Scale/Rotate-Tasten, Fit, Project).
//
// Portiert aus NetRadiant-Custom 20260114: radiant/select.cpp:503-525
// (Select_SetTexdef), :659-685 (Select_ShiftTexture, Select_ScaleTexture,
// Select_RotateTexture), :905-933 (Select_ProjectTexture, Select_FitTexture),
// :1452-1509 (Texdef_Rotate/Scale/Shift mit den Tasten),
// radiant/surfacedialog.cpp:197-282 (SurfaceInspector_SetCurrent_FromSelected),
// :312-334 (DoSnapTToGrid, OnBtnMatchGrid), :376-498 (ProjectTexture,
// Invert, Fit), :1022-1078 (ApplyShader, ApplyTexdef_*),
// radiant/patchmanip.cpp:1006-1188 (PatchTexdefConstructor, Patch_SetTexdef),
// radiant/patchdialog.cpp:195-215 (Scene_Patch*Texture_Selected),
// radiant/brushmanip.cpp:894-919 (Scene_BrushGetShaderTexdef_Selected,
// Scene_BrushGetTexdef_Component_Selected).
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   For a list of contributors, see the accompanying CONTRIBUTORS file.
//   This file is part of GtkRadiant.
//
//   GtkRadiant is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. GtkRadiant is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#ifndef BHED_KARTE_TEXTURBEFEHLE_H
#define BHED_KARTE_TEXTURBEFEHLE_H

#include "bhed/karte/auswahlsystem.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/protokoll.h"
#include "bhed/karte/texdef.h"

#include <string>

namespace bhed::karte {

class Karte;

// si_globals_t (surfacedialog.h:38-45): die Schritte des Surface Inspectors
struct SiWerte {
    float verschiebung[2] = {8.0F, 8.0F};   // shift
    float skalierung[2] = {0.5F, 0.5F};     // scale
    float drehung = 45.0F;                  // rotate
    bool rasterAnpassen = false;            // m_bSnapTToGrid ("Surface Inspector Increments Match Grid")
};

// DoSnapTToGrid (surfacedialog.cpp:312-316): shift = float_to_integer( Raster / Skalierung )
void schritteAmRaster(SiWerte& w, float raster, float hSkalierung, float vSkalierung);

// Texdef_Shift / Texdef_Scale / Texdef_Rotate (select.cpp:1452-1509): Rueckgaengig
// "brushShiftTexture -x .. -y ..", "brushScaleTexture -x .. -y ..",
// "brushRotateTexture -angle .."; Pinsel, sichtbare Patches, gewaehlte Flaechen.
void texdefSchieben(Karte& k, const AuswahlEinstellung& e, float x, float y);
void texdefSkalieren(Karte& k, const AuswahlEinstellung& e, float x, float y);
void texdefDrehen(Karte& k, const AuswahlEinstellung& e, float winkel);

// Select_FitTexture ohne Befehl (Patches: SetTextureRepeat)
void texturEinpassenAuswahl(Karte& k, const AuswahlEinstellung& e, float h, float v, bool nurAbmessung);
// SurfaceInspector_FitTexture / FaceFitWidth / FaceFitHeight / ...Only: "textureAutoFit" usw.
void texturEinpassenBefehl(Karte& k, const AuswahlEinstellung& e, float h, float v, int art);   // 0 Fit, 1 Width, 2 Height, 3 WidthOnly, 4 HeightOnly
// SurfaceInspector_ProjectTexture: "textureProjectAxial/Ortho/Cam"; achse 0
// Axial (ohne Richtung), sonst die Richtung; Meldung bei Quake-Texdef.
void texturProjizierenBefehl(Karte& k, const AuswahlEinstellung& e, const Texdef& td, int art, const Vektor3f& richtung, Protokoll& p);

// Select_SetTexdef( hShift, vShift, hScale, vScale, rotation ) und Patch_SetTexdef
void texdefWerteSetzen(Karte& k, const AuswahlEinstellung& e, const float* hShift, const float* vShift, const float* hScale, const float* vScale,
                       const float* drehung);
// SurfaceInspector::ApplyTexdef_*: Rueckgaengig "textureProjectionSetSelected -hShift .." usw.
// (welches: 0 hShift, 1 vShift, 2 hScale, 3 vScale, 4 rotation)
void texdefWertBefehl(Karte& k, const AuswahlEinstellung& e, int welches, float wert);
// SurfaceInspector_InvertTextureHorizontally / Vertically
void texturUmkehrenBefehl(Karte& k, const AuswahlEinstellung& e, bool waagerecht, const SiWerte& schritte, float verschiebungFeld,
                          float skalierungFeld);

// PatchTexdefConstructor (patchmanip.cpp:1006-1124)
struct PatchTexdef {
    BpTexdef bp;
    Matrix4 lokalZuTex;
    Matrix4 texZuLokal;
    Ebene3d ebene;
    [[nodiscard]] bool gueltig() const;
};
[[nodiscard]] PatchTexdef patchTexdef(const Patch& p, const Kontext& k);

// SurfaceInspector_SetCurrent_FromSelected (surfacedialog.cpp:226-277)
struct SiAnzeige {
    std::string shader;
    TexturProjektion projektion;
    bool patchModus = false;
    bool gefunden = false;
};
[[nodiscard]] SiAnzeige siAusAuswahl(const Karte& k);

}  // namespace bhed::karte

#endif
