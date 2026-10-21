// texturkopie.h - Karten-Editor: Texturen mit der mittleren Maustaste kopieren
// und auftragen (TexManipulator): die Textur der naechsten Flaeche, des
// naechsten Patches oder Lichts unter dem Zeiger holen und in die Ablage
// legen, oder die Textur aus der Ablage auftragen - nur der Shader, die
// Werte, nahtlos anschliessend oder projiziert.
//
// Portiert aus NetRadiant-Custom 20260114: radiant/surfacedialog.cpp
// (EPasteMode, pastemode_for_modifiers, pastemode_if_setShader, PatchData,
// FaceTexture, FaceTextureClipboard_setDefault, PatchEdgeIter,
// Patch_getClosestTriangle, Face_getTexture, Face_setTexture,
// Patch_getTexture, Patch_setTexture, Light_getTexture, Light_setTexture,
// Face_getClosest, OccludeSelector, BrushGetClosestFaceVisibleWalker,
// Scene_getClosestTexturable, Scene_getClosestTexture, Scene_setClosestTexture,
// Scene_copyClosestTexture, Scene_applyClosestTexture(_getUndoName)),
// radiant/patch.cpp (Patch::ProjectTexture( TextureProjection, normal )),
// radiant/select.cpp (Select_SetTexdef, Select_ProjectTexture) und
// libs/math/line.h (line_closest_point).
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
#ifndef BHED_KARTE_TEXTURKOPIE_H
#define BHED_KARTE_TEXTURKOPIE_H

#include "bhed/karte/auswahlsystem.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/texdef.h"

#include <string>
#include <vector>

namespace bhed::karte {

// EPasteMode
enum class EinfuegeArt { Nichts, Werte, Nahtlos, Projizieren };
[[nodiscard]] EinfuegeArt einfuegeArtFuer(bool shift, bool ctrl);   // pastemode_for_modifiers
[[nodiscard]] bool einfuegenMitShader(EinfuegeArt a, bool alt);      // pastemode_if_setShader
// Scene_applyClosestTexture_getUndoName
[[nodiscard]] std::string texturAuftragenName(bool shift, bool ctrl, bool alt);

// FaceTexture: die Ablage (g_faceTextureClipboard).
struct FlaechenTextur {
    TexturProjektion projektion;   // BP-Teil ohne Texturgroesse (removeScale)
    InhaltFlags flags;
    Ebene3d ebene{0, 0, 1, 0};
    Wicklung wicklung;
    std::size_t breite = 64;
    std::size_t hoehe = 64;
    // PatchData
    std::size_t patchBreite = 0;
    std::size_t patchHoehe = 0;
    std::vector<PatchPunkt> patchPunkte;
    float licht = 300;
    Vektor3f farbe{1, 1, 1};
    enum class Quelle { Pinsel, Patch };   // ePasteSource
    Quelle quelle = Quelle::Pinsel;
    FlaechenTextur();
};
// FaceTextureClipboard_setDefault (auch TextureClipboard_textureSelected)
void ablageStandard(FlaechenTextur& f, TexdefArt art);

// Was unter dem Zeiger texturiert werden kann (Texturable).
struct Texturierbar {
    enum class Art { Nichts, Flaeche, Patch, Licht };
    Art art = Art::Nichts;
    std::shared_ptr<Pinsel> pinsel;
    std::shared_ptr<Flaeche> flaeche;
    std::shared_ptr<Patch> patch;
    std::shared_ptr<Entitaet> licht;
};
// Scene_getClosestTexturable: die naechste sichtbare Pinselflaeche; ein
// davor liegender Patch, ein Licht oder eine andere Entitaet verdeckt sie.
[[nodiscard]] Texturierbar naechstesTexturierbar(const Karte& k, AuswahlVolumen& test);

// Scene_getClosestTexture: Shader und Ablage holen; false ohne Treffer.
// browserShader: der gewaehlte Shader (Light_getTexture behaelt ihn).
bool naechsteTexturHolen(const Karte& k, AuswahlVolumen& test, const std::string& browserShader, std::string& shader,
                         FlaechenTextur& ablage);
// Scene_setClosestTexture
void naechsteTexturSetzen(Karte& k, AuswahlVolumen& test, const std::string& shader, FlaechenTextur& ablage, EinfuegeArt art,
                          bool shaderSetzen);
// Scene_applyClosestTexture (ohne den Rueckgaengig-Rahmen, den der
// TexManipulator um Druck, Ziehen und Loslassen legt).
void naechsteTexturAuftragen(Karte& k, const AuswahlEinstellung& e, AuswahlVolumen& test, bool shift, bool ctrl, bool alt,
                             bool auswahlTexturieren, const std::string& browserShader, FlaechenTextur& ablage);

// Select_SetTexdef( projection, setBasis, resetBasis ) und
// Select_ProjectTexture( projection, normal ) (ohne Rueckgaengig-Rahmen).
void texdefSetzenAuswahl(Karte& k, const AuswahlEinstellung& e, const TexturProjektion& p, bool basisSetzen, bool basisZuruecksetzen);
void texturProjizierenAuswahl(Karte& k, const AuswahlEinstellung& e, const TexturProjektion& p, const Vektor3f& normal);
// Patch::ProjectTexture( TextureProjection, normal )
void patchTexturProjizieren(Patch& p, TexturProjektion proj, const Vektor3f& normal, const Kontext& k);

}  // namespace bhed::karte

#endif
