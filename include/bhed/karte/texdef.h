// texdef.h - Karten-Editor: Texturprojektion einer Pinselflaeche.
//
// Portiert aus NetRadiant-Custom (Stand master, 2026): radiant/brush_primit.h,
// radiant/brush_primit.cpp, include/itexdef.h.
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
//
// Drei Formate, wie NRC (TexdefTypeId):
//   Quake          "Axial Projection": shift/scale/rotate auf der Achsenebene
//   BrushPrimitives Texturmatrix (2x3) in der Ebenenbasis (ComputeAxisBase)
//   Valve          Valve 220: Basisvektoren s/t plus shift/scale/rotate
// NRC haelt das gueltige Format in einer globalen Variablen
// (g_bp_globals.m_texdefTypeId) und schaltet sie fuer Umrechnungen kurz um.
// Hier wird es jeder Funktion mitgegeben - gleiche Wirkung, ohne Zustand.
#ifndef BHED_KARTE_TEXDEF_H
#define BHED_KARTE_TEXDEF_H

#include "bhed/karte/mathe.h"
#include "bhed/karte/wicklung.h"

#include <cstddef>

namespace bhed::karte {

// TexdefTypeId
enum class TexdefArt { Quake, BrushPrimitives, Valve };

// texdef_t (include/itexdef.h)
struct Texdef {
    float shift[2] = {0, 0};
    float rotate = 0;
    float scale[2] = {1, 1};
};

// brushprimit_texdef_t (brush_primit.h:29-63)
struct BpTexdef {
    float coords[2][3] = {{2.0f, 0.f, 0.f}, {0.f, 2.0f, 0.f}};
    // removeScale: Werte in Texturgroessen-Masstab fuer manche Operationen.
    void removeScale(std::size_t w, std::size_t h) {
        coords[0][0] *= static_cast<float>(w);
        coords[0][1] *= static_cast<float>(w);
        coords[0][2] *= static_cast<float>(w);
        coords[1][0] *= static_cast<float>(h);
        coords[1][1] *= static_cast<float>(h);
        coords[1][2] *= static_cast<float>(h);
    }
    // addScale: so steht es in der .map; Verschiebung -1..1, Texturgroesse egal.
    void addScale(std::size_t w, std::size_t h) {
        coords[0][0] /= static_cast<float>(w);
        coords[0][1] /= static_cast<float>(w);
        coords[0][2] /= static_cast<float>(w);
        coords[1][0] /= static_cast<float>(h);
        coords[1][1] /= static_cast<float>(h);
        coords[1][2] /= static_cast<float>(h);
    }
};

// TextureProjection (brush_primit.h:65-83)
struct TexturProjektion {
    Texdef texdef;
    BpTexdef bp;
    Vektor3f basisS;
    Vektor3f basisT;
};

// Texdef_getDefaultTextureScale / g_texdef_default_scale. NRC setzt 0.5 und
// ueberschreibt mit default_scale aus der .game-Datei (JA und JK2: 0.25).
float standardTexturSkalierung();
void setzeStandardTexturSkalierung(float s);

// ComputeAxisBase (brush_primit.h:143-187). Muss mit q3map2 uebereinstimmen.
template<typename E, typename O>
inline void berechneAchsenbasis(const Vektor3<E>& normal, Vektor3<O>& texS, Vektor3<O>& texT) {
    const Vektor3<E> up(0, 0, 1);
    const Vektor3<E> down(0, 0, -1);
    if (gleichEps(normal, up, E(1e-6))) {
        texS = Vektor3<O>(0, 1, 0);
        texT = Vektor3<O>(1, 0, 0);
    } else if (gleichEps(normal, down, E(1e-6))) {
        texS = Vektor3<O>(0, 1, 0);
        texT = Vektor3<O>(-1, 0, 0);
    } else {
        texS = normiert(kreuz(normal, up));
        texT = normiert(kreuz(normal, texS));
        negieren(texS);
    }
}

void normaleZuTransform(const Vektor3f& normal, Matrix4& m);                              // Normal_GetTransform
void texdefZuTransform(TexdefArt art, const TexturProjektion& p, float w, float h, Matrix4& m);   // Texdef_toTransform
void texdefNormalisieren(TexdefArt art, TexturProjektion& p, float w, float h);           // Texdef_normalise
void basisFuerNormale(TexdefArt art, const TexturProjektion& p, const Vektor3f& normal, Matrix4& basis);   // Texdef_basisForNormal
void lokalZuTextur(TexdefArt art, const TexturProjektion& p, std::size_t w, std::size_t h, const Vektor3f& normal,
                   Matrix4& m);                                                              // Texdef_Construct_local2tex
// Texdef_EmitTextureCoordinates: st, tangent und bitangent jedes Wicklungspunkts.
void texturkoordinatenSetzen(TexdefArt art, const TexturProjektion& p, std::size_t w, std::size_t h, Wicklung& wi,
                             const Vektor3f& normal, const Matrix4& lokalZuWelt);
void texturachsenAusNormale(const Vektor3f& normal, Vektor3f& s, Vektor3f& t);            // TextureAxisFromNormal

void texdefZuweisen(TexdefArt art, TexturProjektion& p, const TexturProjektion& andere, bool basisSetzen = true);   // Texdef_Assign
void texdefZuweisen(TexdefArt art, TexturProjektion& p, const float* hShift, const float* vShift, const float* hScale,
                    const float* vScale, const float* rotation);                             // Texdef_Assign
void texdefVerschieben(TexdefArt art, TexturProjektion& p, float s, float t);              // Texdef_Shift
void texdefSkalieren(TexdefArt art, TexturProjektion& p, float s, float t);                // Texdef_Scale
void texdefDrehen(TexdefArt art, TexturProjektion& p, float winkel);                       // Texdef_Rotate
// Texdef_FitTexture
void texdefEinpassen(TexdefArt art, TexturProjektion& p, std::size_t w, std::size_t h, const Vektor3f& normal,
                     const Wicklung& wi, float sWiederholung, float tWiederholung, bool nurAbmessung);
void texdefStandard(TexdefArt art, TexturProjektion& p);                                   // TexDef_Construct_Default
void schiebSkalDrehAusFlaeche(TexdefArt art, Texdef& ssr, const TexturProjektion& p);      // ShiftScaleRotate_fromFace
void schiebSkalDrehZuFlaeche(TexdefArt art, const Texdef& ssr, TexturProjektion& p);       // ShiftScaleRotate_toFace
void schiebSkalDrehAusPatch(Texdef& ssr, const TexturProjektion& p);                       // ShiftScaleRotate_fromPatch
void bpZuSchiebSkalDreh(const BpTexdef& bp, Texdef& td);                                   // TexMatToFakeTexCoords
void schiebSkalDrehZuBp(const Texdef& td, BpTexdef& bp);                                   // FakeTexCoordsToTexMat
void bpZuweisen(BpTexdef& bp, const float* hShift, const float* vShift, const float* hScale, const float* vScale,
                const float* rotation);                                                      // BPTexdef_Assign

// Texdef_transformLocked: Textur bleibt beim Transformieren auf der Flaeche
// (Texture Lock). "invariante" ist der Punkt, dessen st erhalten bleibt.
void texdefTransformiertGesperrt(TexdefArt art, TexturProjektion& p, std::size_t w, std::size_t h, const Ebene3d& ebene,
                                 const Matrix4& transform, const Vektor3f& invariante = kNullVektor);
// Texdef_transform: ohne Lock (AP: Achsen mitdrehen, BP/220: wie gesperrt).
void texdefTransformiert(TexdefArt art, TexturProjektion& p, std::size_t w, std::size_t h, const Ebene3d& ebene,
                         const Matrix4& transform, const Vektor3f& invariante = kNullVektor);
// Texdef_Construct_local2tex4projection
void lokalZuTexturFuerProjektion(const Texdef& td, std::size_t w, std::size_t h, const Vektor3f& normal,
                                 const Vektor3f* richtung, Matrix4& m);
// Texdef_ProjectTexture (Texdef entlang einer Richtung bzw. AP)
void texturProjizieren(TexdefArt art, TexturProjektion& p, std::size_t w, std::size_t h, const Ebene3d& ebene,
                       const Texdef& td, const Vektor3f* richtung);
// Texdef_ProjectTexture (eine andere Projektion entlang ihrer Normalen)
void texturProjizieren(TexdefArt art, TexturProjektion& p, std::size_t w, std::size_t h, const Ebene3d& ebene,
                       TexturProjektion andere, const Vektor3f& andereNormale);
// Texdef_Convert: Umrechnung zwischen den Formaten (Quake <-> BP <-> 220).
void texdefUmrechnen(TexdefArt von, TexdefArt nach, const Ebene3d& ebene, TexturProjektion& p, std::size_t w,
                     std::size_t h);
// Texdef_from_ST: Projektion aus drei Punkten und ihren st.
void texdefAusSt(TexdefArt art, TexturProjektion& p, const EbenenPunkte& punkte, const Vektor3d st[3], std::size_t w,
                 std::size_t h);
// BP_from_ST
void bpAusSt(BpTexdef& bp, const EbenenPunkte& punkte, const Vektor3d st[3], const Vektor3d& normal,
             bool normalisieren = true);
void lokalZuTexturAusSt(const EbenenPunkte& punkte, const Vektor3d st[3], Matrix4& m);   // Texdef_Construct_local2tex_from_ST
void bpLokalZuTextur(const BpTexdef& bp, const Ebene3d& ebene, Matrix4& m);              // BP_Construct_local2tex

}  // namespace bhed::karte
#endif
