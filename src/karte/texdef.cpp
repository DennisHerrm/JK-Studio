// texdef.cpp - Karten-Editor: Texturprojektion einer Pinselflaeche.
//
// Portiert aus NetRadiant-Custom (Stand master, 2026): radiant/brush_primit.cpp.
// Uebernommen sind nur die aktiven Zweige (#if 1 / #else der #if-0-Bloecke);
// Zeilennummern beziehen sich auf brush_primit.cpp.
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
#include "bhed/karte/texdef.h"

#include <cmath>

namespace bhed::karte {

namespace {
float g_standardSkalierung = 0.5f;   // Brush_Construct (brushmodule.cpp): g_texdef_default_scale = 0.5f
}

float standardTexturSkalierung() { return g_standardSkalierung; }
void setzeStandardTexturSkalierung(float s) { g_standardSkalierung = s; }

// Normal_GetTransform (:43-88): XYZ -> ST auf einer der drei Achsenebenen.
void normaleZuTransform(const Vektor3f& normal, Matrix4& m) {
    switch (projektionsachseFuerNormale(normal)) {
    case Projektionsachse::Z:
        m[0] = 1; m[1] = 0; m[2] = 0;
        m[4] = 0; m[5] = 1; m[6] = 0;
        m[8] = 0; m[9] = 0; m[10] = 1;
        break;
    case Projektionsachse::Y:
        m[0] = 1; m[1] = 0; m[2] = 0;
        m[4] = 0; m[5] = 0; m[6] = -1;
        m[8] = 0; m[9] = 1; m[10] = 0;
        break;
    case Projektionsachse::X:
        m[0] = 0; m[1] = 0; m[2] = 1;
        m[4] = 1; m[5] = 0; m[6] = 0;
        m[8] = 0; m[9] = 1; m[10] = 0;
        break;
    }
    m[3] = m[7] = m[11] = m[12] = m[13] = m[14] = 0;
    m[15] = 1;
}

namespace {

// Texdef_toTransform (texdef_t, :95-111): (-shift)*(1/scale)*(-rotate), das
// Vorzeichen der x-Verschiebung umgedreht.
void apZuTransform(const Texdef& td, float width, float height, Matrix4& m) {
    double inverseScale[2];
    inverseScale[0] = 1 / (td.scale[0] * width);
    inverseScale[1] = 1 / (td.scale[1] * -height);
    m[12] = td.shift[0] / width;
    m[13] = -td.shift[1] / -height;
    const double c = std::cos(gradZuBogen(-td.rotate));
    const double s = std::sin(gradZuBogen(-td.rotate));
    m[0] = static_cast<float>(c * inverseScale[0]);
    m[1] = static_cast<float>(s * inverseScale[1]);
    m[4] = static_cast<float>(-s * inverseScale[0]);
    m[5] = static_cast<float>(c * inverseScale[1]);
    m[2] = m[3] = m[6] = m[7] = m[8] = m[9] = m[11] = m[14] = 0;
    m[10] = m[15] = 1;
}

// Valve220Texdef_toTransform (:113-119)
void valveZuTransform(const Texdef& td, float width, float height, Matrix4& m) {
    m = kEinheitsMatrix;
    m[12] = td.shift[0] / width;
    m[13] = -td.shift[1] / -height;
    m[0] = 1 / (td.scale[0] * width);
    m[5] = 1 / (td.scale[1] * -height);
}

// BPTexdef_toTransform (:121-129)
void bpZuTransform(const BpTexdef& bp, Matrix4& m) {
    m = kEinheitsMatrix;
    m.xx() = bp.coords[0][0];
    m.yx() = bp.coords[0][1];
    m.tx() = bp.coords[0][2];
    m.xy() = bp.coords[1][0];
    m.yy() = bp.coords[1][1];
    m.ty() = bp.coords[1][2];
}

// arctangent_yx (:147-158): Sonderfaelle, falls atan2 sie nicht kann.
double arctangentYx(double y, double x) {
    if (std::fabs(x) > 1.0E-6) { return std::atan2(y, x); }
    if (y > 0) { return kHalbPi; }
    return -kHalbPi;
}

// Texdef_fromTransform (texdef_t, :160-185)
void apAusTransform(Texdef& td, float width, float height, const Matrix4& m) {
    td.scale[0] = static_cast<float>((1.0 / laenge(Vektor2f(m[0], m[4]))) / width);
    td.scale[1] = static_cast<float>((1.0 / laenge(Vektor2f(m[1], m[5]))) / height);
    td.rotate = static_cast<float>(-bogenZuGrad(arctangentYx(-m[4], m[0])));
    if (td.rotate == -180.0f) {
        td.rotate = 180.0f;
    }
    td.shift[0] = m[12] * width;
    td.shift[1] = m[13] * height;
    // Ist das 2D-Kreuzprodukt der Achsen positiv, hat eine Achse negative Skalierung.
    if (kreuz(Vektor2f(m[0], m[4]), Vektor2f(m[1], m[5])) > 0) {
        if (td.rotate >= 180.0f) {
            td.rotate -= 180.0f;
            td.scale[0] = -td.scale[0];
        } else {
            td.scale[1] = -td.scale[1];
        }
    }
}

// BPTexdef_fromTransform (:187-195)
void bpAusTransform(BpTexdef& bp, const Matrix4& m) {
    bp.coords[0][0] = m.xx();
    bp.coords[0][1] = m.yx();
    bp.coords[0][2] = m.tx();
    bp.coords[1][0] = m.xy();
    bp.coords[1][1] = m.yy();
    bp.coords[1][2] = m.ty();
}

// Texdef_fromTransform (TextureProjection, :197-208): Valve wie Quake.
void texdefAusTransform(TexdefArt art, TexturProjektion& p, float width, float height, const Matrix4& m) {
    if (art == TexdefArt::BrushPrimitives) {
        bpAusTransform(p.bp, m);
    } else {
        apAusTransform(p.texdef, width, height, m);
    }
}

// Texdef_normalise (texdef_t, :210-215)
void apNormalisieren(Texdef& td, float width, float height) {
    td.shift[0] = zahlMod(td.shift[0], width);
    td.shift[1] = zahlMod(td.shift[1], height);
}

// BPTexdef_normalise (:217-220) - fuer removeScale()-Werte gedacht
void bpNormalisieren(BpTexdef& bp, float width, float height) {
    bp.coords[0][2] = zahlMod(bp.coords[0][2], width);
    bp.coords[1][2] = zahlMod(bp.coords[1][2], height);
}

}  // namespace

// Texdef_toTransform (TextureProjection, :131-144)
void texdefZuTransform(TexdefArt art, const TexturProjektion& p, float w, float h, Matrix4& m) {
    switch (art) {
    case TexdefArt::BrushPrimitives: bpZuTransform(p.bp, m); break;
    case TexdefArt::Valve: valveZuTransform(p.texdef, w, h, m); break;
    default: apZuTransform(p.texdef, w, h, m); break;
    }
}

// Texdef_normalise (:226-234): Verschiebung auf den kleinsten positiven Rest.
void texdefNormalisieren(TexdefArt art, TexturProjektion& p, float w, float h) {
    if (art == TexdefArt::BrushPrimitives) {
        bpNormalisieren(p.bp, w, h);
    } else {
        apNormalisieren(p.texdef, w, h);
    }
}

// Texdef_basisForNormal (:242-263)
void basisFuerNormale(TexdefArt art, const TexturProjektion& p, const Vektor3f& normal, Matrix4& basis) {
    if (art == TexdefArt::BrushPrimitives) {
        basis = kEinheitsMatrix;
        berechneAchsenbasis(normal, basis.x().vec3(), basis.y().vec3());
        basis.z().vec3() = normal;
        transponieren(basis);
    } else if (art == TexdefArt::Valve) {
        basis = kEinheitsMatrix;
        basis.x().vec3() = p.basisS;
        basis.y().vec3() = -p.basisT;
        basis.z().vec3() = normiert(kreuz(basis.x().vec3(), basis.y().vec3()));
        transponieren(basis);
    } else {
        normaleZuTransform(normal, basis);
    }
}

// Texdef_Construct_local2tex (:265-272)
void lokalZuTextur(TexdefArt art, const TexturProjektion& p, std::size_t w, std::size_t h, const Vektor3f& normal,
                   Matrix4& m) {
    texdefZuTransform(art, p, static_cast<float>(w), static_cast<float>(h), m);
    Matrix4 xyz2st;
    basisFuerNormale(art, p, normal, xyz2st);
    multipliziere(m, xyz2st);
}

// Texdef_EmitTextureCoordinates (:274-314)
void texturkoordinatenSetzen(TexdefArt art, const TexturProjektion& p, std::size_t w, std::size_t h, Wicklung& wi,
                             const Vektor3f& normal, const Matrix4& lokalZuWelt) {
    if (wi.numpoints < 3) {
        return;
    }
    Matrix4 local2tex;
    texdefZuTransform(art, p, static_cast<float>(w), static_cast<float>(h), local2tex);
    {
        Matrix4 xyz2st;
        // NRC: "we don't care if it's not normalised..."
        basisFuerNormale(art, p, transformierteRichtung(lokalZuWelt, normal), xyz2st);
        multipliziere(local2tex, xyz2st);
    }
    const Vektor3f tangent(normiert(transponiert(local2tex).x().vec3()));
    const Vektor3f bitangent(normiert(transponiert(local2tex).y().vec3()));

    multipliziere(local2tex, lokalZuWelt);

    for (WicklungsPunkt& v : wi) {
        v.texcoord = transformierterPunkt(local2tex, v.vertex).vec2();
        v.tangent = tangent;
        v.bitangent = bitangent;
    }
}

// TextureAxisFromNormal (:320-354)
void texturachsenAusNormale(const Vektor3f& normal, Vektor3f& s, Vektor3f& t) {
    switch (projektionsachseFuerNormale(normal)) {
    case Projektionsachse::Z:
        s = Vektor3f(1, 0, 0);
        t = Vektor3f(0, -1, 0);
        break;
    case Projektionsachse::Y:
        s = Vektor3f(1, 0, 0);
        t = Vektor3f(0, 0, -1);
        break;
    case Projektionsachse::X:
        s = Vektor3f(0, 1, 0);
        t = Vektor3f(0, 0, -1);
        break;
    }
}

namespace {

// Texdef_Assign (texdef_t, :361-389)
void apZuweisen(Texdef& td, const float* hShift, const float* vShift, const float* hScale, const float* vScale,
                const float* rotation) {
    if (hShift) { td.shift[0] = *hShift; }
    if (vShift) { td.shift[1] = *vShift; }
    if (hScale) {
        if (std::fabs(*hScale) > 1e-5f) {
            td.scale[0] = *hScale;
        } else {
            td.scale[0] = -td.scale[0];
        }
    }
    if (vScale) {
        if (std::fabs(*vScale) > 1e-5f) {
            td.scale[1] = *vScale;
        } else {
            td.scale[1] = -td.scale[1];
        }
    }
    if (rotation) {
        td.rotate = *rotation;
        td.rotate = std::fmod(td.rotate, 360.f);
    }
}

// Texdef_Shift / Texdef_Scale / Texdef_Rotate (texdef_t, :391-414)
void apVerschieben(Texdef& td, float s, float t) {
    td.shift[0] += s;
    td.shift[1] += t;
}
void apSkalieren(Texdef& td, float s, float t) {
    if (std::fabs(td.scale[0] + s) > 1e-5f) {
        td.scale[0] += s;
    } else {
        td.scale[0] = -td.scale[0];
    }
    if (std::fabs(td.scale[1] + t) > 1e-5f) {
        td.scale[1] += t;
    } else {
        td.scale[1] = -td.scale[1];
    }
}
void apDrehen(Texdef& td, float winkel) {
    td.rotate += winkel;
    td.rotate = std::fmod(td.rotate, 360.f);
}

// Valve220_rotate (:1027-1037)
void valveDrehen(TexturProjektion& p, float winkel) {
    const Matrix4 rotmat = drehungAchseWinkel(kreuz(p.basisS, p.basisT), gradZuBogen(-winkel));
    transformiereRichtung(rotmat, p.basisS);
    transformiereRichtung(rotmat, p.basisT);
    normieren(p.basisS);
    normieren(p.basisT);
}

// BPTexdef_Shift (:1043-1051)
void bpVerschieben(BpTexdef& bp, float s, float t) {
    bp.coords[0][2] -= s;
    bp.coords[1][2] += t;
}

// BPTexdef_Scale (:1053-1081, #else-Zweig)
void bpSkalieren(BpTexdef& bp, float s, float t) {
    Texdef td;
    bpZuSchiebSkalDreh(bp, td);
    float scaleS = -1;
    float scaleT = -1;
    if (std::fabs(td.scale[0] + s) > 1e-5f) {
        scaleS = td.scale[0] / (td.scale[0] + s);
    }
    if (std::fabs(td.scale[1] + t) > 1e-5f) {
        scaleT = td.scale[1] / (td.scale[1] + t);
    }
    bp.coords[0][0] *= scaleS;
    bp.coords[0][1] *= scaleS;
    bp.coords[1][0] *= scaleT;
    bp.coords[1][1] *= scaleT;
}

// BPTexdef_Rotate (:1083-1105, #else-Zweig)
void bpDrehen(BpTexdef& bp, float winkel) {
    const float x = bp.coords[0][0];
    const float y = bp.coords[0][1];
    const float x1 = bp.coords[1][0];
    const float y1 = bp.coords[1][1];
    const float s = static_cast<float>(std::sin(gradZuBogen(-winkel)));
    const float c = static_cast<float>(std::cos(gradZuBogen(-winkel)));
    bp.coords[0][0] = x * c - y * s;
    bp.coords[0][1] = x * s + y * c;
    bp.coords[1][0] = x1 * c - y1 * s;
    bp.coords[1][1] = x1 * s + y1 * c;
}

}  // namespace

// TexMatToFakeTexCoords (:629-675, #else-Zweig mit #if 1)
void bpZuSchiebSkalDreh(const BpTexdef& bp, Texdef& td) {
    td.scale[0] = static_cast<float>(1.0 / laenge(Vektor2f(bp.coords[0][0], bp.coords[0][1])));
    td.scale[1] = static_cast<float>(1.0 / laenge(Vektor2f(bp.coords[1][0], bp.coords[1][1])));
    if (bp.coords[0][0] < 0) {
        td.scale[0] = -td.scale[0];
    }
    if (bp.coords[1][1] < 0) {
        td.scale[1] = -td.scale[1];
    }
    // NRC ruft acos ohne std:: mit einem float; hier ausdruecklich in double.
    td.rotate = static_cast<float>(
        bogenZuGrad(std::acos(static_cast<double>(normiert(Vektor2f(bp.coords[0][0], bp.coords[0][1]))[0]))));
    if (bp.coords[0][1] > 0) {
        td.rotate = -td.rotate;
    }
    td.shift[0] = -bp.coords[0][2];
    td.shift[1] = bp.coords[1][2];
}

// FakeTexCoordsToTexMat (:678-691)
void schiebSkalDrehZuBp(const Texdef& td, BpTexdef& bp) {
    const double r = gradZuBogen(-td.rotate);
    const double c = std::cos(r);
    const double s = std::sin(r);
    const double x = 1.0f / td.scale[0];
    const double y = 1.0f / td.scale[1];
    bp.coords[0][0] = static_cast<float>(x * c);
    bp.coords[1][0] = static_cast<float>(x * s);
    bp.coords[0][1] = static_cast<float>(y * -s);
    bp.coords[1][1] = static_cast<float>(y * c);
    bp.coords[0][2] = -td.shift[0];
    bp.coords[1][2] = td.shift[1];
}

// BPTexdef_Assign (Werte, :1107-1136)
void bpZuweisen(BpTexdef& bp, const float* hShift, const float* vShift, const float* hScale, const float* vScale,
                const float* rotation) {
    Texdef td;
    bpZuSchiebSkalDreh(bp, td);
    if (hShift) { bp.coords[0][2] = -*hShift; }
    if (vShift) { bp.coords[1][2] = *vShift; }
    if (hScale) {
        float scaleS = -1;
        if (std::fabs(*hScale) > 1e-5f) { scaleS = td.scale[0] / *hScale; }
        bp.coords[0][0] *= scaleS;
        bp.coords[0][1] *= scaleS;
    }
    if (vScale) {
        float scaleT = -1;
        if (std::fabs(*vScale) > 1e-5f) { scaleT = td.scale[1] / *vScale; }
        bp.coords[1][0] *= scaleT;
        bp.coords[1][1] *= scaleT;
    }
    if (rotation) {
        bpDrehen(bp, *rotation - td.rotate);
    }
}

// Texdef_Assign (TextureProjection, :1144-1156)
void texdefZuweisen(TexdefArt art, TexturProjektion& p, const TexturProjektion& andere, bool basisSetzen) {
    if (art == TexdefArt::BrushPrimitives) {
        p.bp = andere.bp;
    } else {
        p.texdef = andere.texdef;
        if (art == TexdefArt::Valve && basisSetzen) {
            p.basisS = andere.basisS;
            p.basisT = andere.basisT;
        }
    }
}

// Texdef_Assign (Werte, :1158-1169)
void texdefZuweisen(TexdefArt art, TexturProjektion& p, const float* hShift, const float* vShift, const float* hScale,
                    const float* vScale, const float* rotation) {
    if (art == TexdefArt::BrushPrimitives) {
        bpZuweisen(p.bp, hShift, vShift, hScale, vScale, rotation);
    } else {
        if (rotation && art == TexdefArt::Valve) {
            valveDrehen(p, *rotation - p.texdef.rotate);
        }
        apZuweisen(p.texdef, hShift, vShift, hScale, vScale, rotation);
    }
}

// Texdef_Shift (:1171-1179)
void texdefVerschieben(TexdefArt art, TexturProjektion& p, float s, float t) {
    if (art == TexdefArt::BrushPrimitives) {
        bpVerschieben(p.bp, s, t);
    } else {
        apVerschieben(p.texdef, s, t);
    }
}

// Texdef_Scale (:1181-1189)
void texdefSkalieren(TexdefArt art, TexturProjektion& p, float s, float t) {
    if (art == TexdefArt::BrushPrimitives) {
        bpSkalieren(p.bp, s, t);
    } else {
        apSkalieren(p.texdef, s, t);
    }
}

// Texdef_Rotate (:1191-1202)
void texdefDrehen(TexdefArt art, TexturProjektion& p, float winkel) {
    if (art == TexdefArt::BrushPrimitives) {
        bpDrehen(p.bp, winkel);
    } else {
        if (art == TexdefArt::Valve) {
            valveDrehen(p, winkel);
        }
        apDrehen(p.texdef, winkel);
    }
}

// Texdef_FitTexture (:1204-1277)
void texdefEinpassen(TexdefArt art, TexturProjektion& p, std::size_t w, std::size_t h, const Vektor3f& normal,
                     const Wicklung& wi, float sWiederholung, float tWiederholung, bool nurAbmessung) {
    if (wi.numpoints < 3) {
        return;
    }
    Matrix4 st2tex;
    texdefZuTransform(art, p, static_cast<float>(w), static_cast<float>(h), st2tex);

    // die aktuelle Texturtransformation
    Matrix4 local2tex = st2tex;
    {
        Matrix4 xyz2st;
        basisFuerNormale(art, p, normal, xyz2st);
        multipliziere(local2tex, xyz2st);
    }

    // ihre Grenzen
    Kasten bounds;
    for (const auto& v : wi) {
        const Vektor3f texcoord = transformierterPunkt(local2tex, v.vertex);
        kastenErweiternSicher(bounds, texcoord);
    }
    bounds.origin.z() = 0;
    bounds.extents.z() = 1;

    // die Grenzen einer genau eingepassten Transformation
    Kasten perfect;
    if (tWiederholung == 0 && sWiederholung == 0) {
        // unsinnige Eingabe
        tWiederholung = sWiederholung = 1;
        perfect.origin = Vektor3f(static_cast<float>(sWiederholung * 0.5), static_cast<float>(tWiederholung * 0.5), 0);
        perfect.extents = Vektor3f(static_cast<float>(sWiederholung * 0.5), static_cast<float>(tWiederholung * 0.5), 1);
    }
    if (tWiederholung == 0) {
        if (nurAbmessung) {   // Breite einpassen, Hoehe behalten
            perfect.origin = Vektor3f(static_cast<float>(sWiederholung * 0.5), bounds.origin.y(), 0);
            perfect.extents = Vektor3f(static_cast<float>(sWiederholung * 0.5), bounds.extents.y(), 1);
        } else {   // Breite einpassen
            perfect.origin = Vektor3f(static_cast<float>(sWiederholung * 0.5),
                                      static_cast<float>(sWiederholung * 0.5 * bounds.extents.y() / bounds.extents.x()), 0);
            perfect.extents = Vektor3f(static_cast<float>(sWiederholung * 0.5),
                                       static_cast<float>(sWiederholung * 0.5 * bounds.extents.y() / bounds.extents.x()), 1);
        }
    } else if (sWiederholung == 0) {
        if (nurAbmessung) {   // Hoehe einpassen, Breite behalten
            perfect.origin = Vektor3f(bounds.origin.x(), static_cast<float>(tWiederholung * 0.5), 0);
            perfect.extents = Vektor3f(bounds.extents.x(), static_cast<float>(tWiederholung * 0.5), 1);
        } else {   // Hoehe einpassen
            perfect.origin = Vektor3f(static_cast<float>(tWiederholung * 0.5 * bounds.extents.x() / bounds.extents.y()),
                                      static_cast<float>(tWiederholung * 0.5), 0);
            perfect.extents = Vektor3f(static_cast<float>(tWiederholung * 0.5 * bounds.extents.x() / bounds.extents.y()),
                                       static_cast<float>(tWiederholung * 0.5), 1);
        }
    } else {
        perfect.origin = Vektor3f(static_cast<float>(sWiederholung * 0.5), static_cast<float>(tWiederholung * 0.5), 0);
        perfect.extents = Vektor3f(static_cast<float>(sWiederholung * 0.5), static_cast<float>(tWiederholung * 0.5), 1);
    }

    // der Unterschied zwischen aktueller und eingepasster Transformation
    Matrix4 matrix(verschiebungsMatrix(bounds.origin - perfect.origin));
    skaliereUmPunkt(matrix, bounds.extents / perfect.extents, perfect.origin);
    affinInvertieren(matrix);

    // auf die aktuelle Transformation anwenden
    vormultipliziere(st2tex, matrix);

    texdefAusTransform(art, p, static_cast<float>(w), static_cast<float>(h), st2tex);
    if (art == TexdefArt::BrushPrimitives) {
        bpNormalisieren(p.bp, 1, 1);   // NRC: "scaleApplied is!"
    } else {
        apNormalisieren(p.texdef, static_cast<float>(w), static_cast<float>(h));
    }
}

// TexDef_Construct_Default (:1283-1290)
void texdefStandard(TexdefArt art, TexturProjektion& p) {
    p.texdef.scale[0] = standardTexturSkalierung();
    p.texdef.scale[1] = standardTexturSkalierung();
    if (art == TexdefArt::BrushPrimitives) {
        schiebSkalDrehZuBp(p.texdef, p.bp);
    }
}

// ShiftScaleRotate_fromFace / toFace / fromPatch (:1294-1318)
void schiebSkalDrehAusFlaeche(TexdefArt art, Texdef& ssr, const TexturProjektion& p) {
    if (art == TexdefArt::BrushPrimitives) {
        bpZuSchiebSkalDreh(p.bp, ssr);
    } else {
        ssr = p.texdef;
    }
}
void schiebSkalDrehZuFlaeche(TexdefArt art, const Texdef& ssr, TexturProjektion& p) {
    if (art == TexdefArt::BrushPrimitives) {
        // die Matrix gilt fuer eine Textur der Groesse 2x2
        schiebSkalDrehZuBp(ssr, p.bp);
    } else {
        p.texdef = ssr;
    }
}
void schiebSkalDrehAusPatch(Texdef& ssr, const TexturProjektion& p) { bpZuSchiebSkalDreh(p.bp, ssr); }

namespace {

// Det3x3 (:1523-1530)
double det3x3(double a00, double a01, double a02, double a10, double a11, double a12, double a20, double a21,
              double a22) {
    return a00 * (a11 * a22 - a12 * a21) - a01 * (a10 * a22 - a12 * a20) + a02 * (a10 * a21 - a11 * a20);
}

// BaseAxes (:1584-1591)
const Vektor3f kBasisAchsen[] = {
    Vektor3f(0, 0, 1),  Vektor3f(1, 0, 0), Vektor3f(0, -1, 0),
    Vektor3f(0, 0, -1), Vektor3f(1, 0, 0), Vektor3f(0, -1, 0),
    Vektor3f(1, 0, 0),  Vektor3f(0, 1, 0), Vektor3f(0, 0, -1),
    Vektor3f(-1, 0, 0), Vektor3f(0, 1, 0), Vektor3f(0, 0, -1),
    Vektor3f(0, 1, 0),  Vektor3f(1, 0, 0), Vektor3f(0, 0, -1),
    Vektor3f(0, -1, 0), Vektor3f(1, 0, 0), Vektor3f(0, 0, -1),
};

// planeNormalIndex (:1593-1619, #else-Zweig)
std::size_t ebenenNormalenIndex(const Vektor3f& normal) {
    switch (projektionsachseFuerNormale(normal)) {
    case Projektionsachse::Z: return normal.z() > 0 ? 0 : 1;
    case Projektionsachse::X: return normal.x() > 0 ? 2 : 3;
    default: return normal.y() > 0 ? 4 : 5;
    }
}

// AP_from_axes (:1621-1683)
void apAusAchsen(const Vektor3f& achseX, const Vektor3f& achseY, const Vektor3d& normal, std::size_t width,
                 std::size_t height, const Vektor3f& invariante, const Vektor2f& invarianteSt, Texdef& td) {
    // Texturebene und Grundachsen
    const std::size_t index = ebenenNormalenIndex(normal);
    Vektor3f xAxis = kBasisAchsen[index * 3 + 1];
    Vektor3f yAxis = kBasisAchsen[index * 3 + 2];
    Vektor3f zAxis = kBasisAchsen[(index / 2) * 6];

    const Ebene3d texturEbene(zAxis, 0);

    // die transformierten Texturachsen auf die neue Projektionsebene legen
    const Vektor3f projectedXAxis = punktAufEbene(texturEbene, achseX);
    const Vektor3f projectedYAxis = punktAufEbene(texturEbene, achseY);

    const Vektor3f normalizedXAxis = normiert(projectedXAxis);
    const Vektor3f normalizedYAxis = normiert(projectedYAxis);

    // Drehwinkel aus dem Skalarprodukt neuer Grundachsen und projizierter Achsen
    const float cosX = static_cast<float>(punkt(xAxis, normalizedXAxis));
    const float cosY = static_cast<float>(punkt(yAxis, normalizedYAxis));

    float radX = std::acos(cosX);   // float-acos, wie NRC (std::acos mit float)
    if (punkt(kreuz(xAxis, normalizedXAxis), zAxis) < 0) {
        radX *= -1;
    }
    float radY = std::acos(cosY);
    if (punkt(kreuz(yAxis, normalizedYAxis), zAxis) < 0) {
        radY *= -1;
    }

    // X- oder Y-Drehung waehlen
    float rad = width >= height ? radX : radY;

    // ist die Texturebenen-Normale die Y-Achse, muss im Uhrzeigersinn gedreht werden
    if ((index / 2) * 6 == 12) {
        rad *= -1;
    }

    const Matrix4 rotmat = drehungAchseWinkel(kreuz(yAxis, xAxis), rad);
    transformiereRichtung(rotmat, xAxis);
    transformiereRichtung(rotmat, yAxis);

    // die Skalierung
    Vektor2f scale(static_cast<float>(laenge(projectedXAxis)), static_cast<float>(laenge(projectedYAxis)));

    // Vorzeichen aus dem Winkel zwischen neuer Achse und projizierter Achse
    if (punkt(xAxis, normalizedXAxis) < 0) {
        scale[0] *= -1;
    }
    if (punkt(yAxis, normalizedYAxis) < 0) {
        scale[1] *= -1;
    }

    // neue st der invarianten Stelle ohne Verschiebung
    const Vektor2f neueSt(static_cast<float>(punkt(xAxis / scale[0], invariante)),
                          static_cast<float>(punkt(yAxis / scale[1], invariante)));
    // die invariante Stelle bleibt: Verschiebung ist die Differenz
    td.shift[0] = invarianteSt[0] - neueSt[0];
    td.shift[1] = invarianteSt[1] - neueSt[1];
    td.scale[0] = scale[0];
    td.scale[1] = scale[1];
    td.rotate = static_cast<float>(bogenZuGrad(rad));
    apNormalisieren(td, static_cast<float>(width), static_cast<float>(height));
}

// Gemeinsamer AP-Teil von Texdef_transformLocked (:1749-1771) und
// Texdef_transform (:1844-1866); nur die Normale fuer AP_from_axes unterscheidet sich.
void apTransformieren(TexturProjektion& p, std::size_t w, std::size_t h, const Ebene3d& ebene, const Matrix4& transform,
                      const Vektor3f& invariante, const Vektor3d& neueNormale) {
    // aktuelle Texturkoordinaten des Ursprungs
    const std::size_t index = ebenenNormalenIndex(ebene.normal());
    Vektor3f xAxis = kBasisAchsen[index * 3 + 1];
    Vektor3f yAxis = kBasisAchsen[index * 3 + 2];
    Vektor3f zAxis = kBasisAchsen[(index / 2) * 6];
    Matrix4 rotmat = drehungAchseWinkel(kreuz(yAxis, xAxis), gradZuBogen(p.texdef.rotate));
    transformiereRichtung(rotmat, xAxis);
    transformiereRichtung(rotmat, yAxis);

    const Vektor2f invarianteSt(static_cast<float>(punkt(xAxis / p.texdef.scale[0], invariante) + p.texdef.shift[0]),
                                static_cast<float>(punkt(yAxis / p.texdef.scale[1], invariante) + p.texdef.shift[1]));
    // Texturachsen entlang der Textur-Z-Achse auf die Ebene projizieren
    const Vektor3f boundaryOffset = punktAufEbene(ebene, Vektor3f(0, 0, 0), zAxis);
    const Vektor3f xAxisOnBoundary = punktAufEbene(ebene, xAxis * p.texdef.scale[0], zAxis) - boundaryOffset;
    const Vektor3f yAxisOnBoundary = punktAufEbene(ebene, yAxis * p.texdef.scale[1], zAxis) - boundaryOffset;

    // projizierte Achsen transformieren, Verschiebung ausgleichen
    const Vektor3f transformedXAxis = transformierteRichtung(transform, xAxisOnBoundary);
    const Vektor3f transformedYAxis = transformierteRichtung(transform, yAxisOnBoundary);

    apAusAchsen(transformedXAxis, transformedYAxis, neueNormale, w, h, transformierterPunkt(transform, invariante),
                invarianteSt, p.texdef);
}

}  // namespace

// BP_from_ST (:1532-1582)
void bpAusSt(BpTexdef& bp, const EbenenPunkte& punkte, const Vektor3d st[3], const Vektor3d& normal,
             bool normalisieren) {
    Vektor2<double> xyI, xyJ, xyK;
    Vektor2<double> stI, stJ, stK;
    Vektor3d texX, texY;
    berechneAchsenbasis(normal, texX, texY);

    xyI[0] = punkt(punkte[0], texX);
    xyI[1] = punkt(punkte[0], texY);
    xyJ[0] = punkt(punkte[1], texX);
    xyJ[1] = punkt(punkte[1], texY);
    xyK[0] = punkt(punkte[2], texX);
    xyK[1] = punkt(punkte[2], texY);
    stI = st[0].vec2();
    stJ = st[1].vec2();
    stK = st[2].vec2();

    // lineare Gleichungen: st[i] = texMat[i][0]*x + texMat[i][1]*y + texMat[i][2]
    const double D = det3x3(xyI[0], xyI[1], 1, xyJ[0], xyJ[1], 1, xyK[0], xyK[1], 1);
    if (D != 0) {
        for (std::size_t i = 0; i < 2; ++i) {
            const double D0 = det3x3(stI[i], xyI[1], 1, stJ[i], xyJ[1], 1, stK[i], xyK[1], 1);
            const double D1 = det3x3(xyI[0], stI[i], 1, xyJ[0], stJ[i], 1, xyK[0], stK[i], 1);
            const double D2 = det3x3(xyI[0], xyI[1], stI[i], xyJ[0], xyJ[1], stJ[i], xyK[0], xyK[1], stK[i]);
            bp.coords[i][0] = static_cast<float>(D0 / D);
            bp.coords[i][1] = static_cast<float>(D1 / D);
            bp.coords[i][2] = static_cast<float>(normalisieren ? std::fmod(D2 / D, 1.0) : (D2 / D));
        }
    }
}

// Texdef_transformLocked (:1686-1825)
void texdefTransformiertGesperrt(TexdefArt art, TexturProjektion& p, std::size_t w, std::size_t h, const Ebene3d& ebene,
                                 const Matrix4& transform, const Vektor3f& invariante) {
    if (transform == kEinheitsMatrix) {
        return;   // NRC: nach jeder Transformation mit der Einheitsmatrix aufgerufen
    }
    if (art == TexdefArt::BrushPrimitives) {
        Vektor3d texX, texY;
        berechneAchsenbasis(ebene.normal(), texX, texY);

        const Vektor3d anker = ebene.normal() * ebene.dist();
        EbenenPunkte punkte = {anker, anker + texX, anker + texY};
        Vektor3d st[3];

        Matrix4 local2tex;
        lokalZuTextur(art, p, w, h, ebene.normal(), local2tex);
        for (std::size_t i = 0; i < 3; ++i) {
            st[i] = transformierterPunkt(local2tex, punkte[i]);
            transformierePunkt(transform, punkte[i]);
        }
        // behandelt auch Skalierung 0 (#else-Zweig, :1710-1715)
        Vektor3d normalTransformed(ebeneAusPunkten(punkte).normal());
        if (haendigkeit(transform) == Haendigkeit::Links) {
            negieren(normalTransformed);
        }
        bpAusSt(p.bp, punkte, st, normalTransformed);
    } else if (art == TexdefArt::Quake) {
        if (p.texdef.scale[0] == 0 || p.texdef.scale[1] == 0) {
            return;
        }
        // behandelt auch Skalierung 0 (#elif 1, :1731-1742)
        Vektor3d texX, texY;
        berechneAchsenbasis(ebene.normal(), texX, texY);
        const Vektor3d anker = ebene.normal() * ebene.dist();
        EbenenPunkte punkte = {anker, anker + texX, anker + texY};
        for (std::size_t i = 0; i < 3; ++i) {
            transformierePunkt(transform, punkte[i]);
        }
        Vektor3f newNormal(ebeneAusPunkten(punkte).normal());
        if (haendigkeit(transform) == Haendigkeit::Links) {
            negieren(newNormal);
        }
        apTransformieren(p, w, h, ebene, transform, invariante, newNormal);
    } else {   // Valve 220
        // NRC-Hack: mit negativer Skalierung oft kaputt
        if (p.texdef.scale[0] < 0) {
            p.texdef.scale[0] *= -1;
            p.basisS *= -1;
        }
        if (p.texdef.scale[1] < 0) {
            p.texdef.scale[1] *= -1;
            p.basisT *= -1;
        }
        const Matrix4 maa(normalenMatrix(transform));
        // in double transformiert und normiert, erst dann float (:1789)
        const Vektor3f normalTransformed(normiert(transformierteRichtung(maa, ebene.normal())));

        // stTransformed2stOriginal = stTransformed -> transformed -> identity -> stIdentity -> stOriginal
        Matrix4 identity2stIdentity;
        basisFuerNormale(art, p, ebene.normal(), identity2stIdentity);

        transformiereRichtung(maa, p.basisS);
        transformiereRichtung(maa, p.basisT);

        Matrix4 transformed2stTransformed;
        basisFuerNormale(art, p, normalTransformed, transformed2stTransformed);
        const Matrix4 stTransformed2identity(affinInvers(multipliziert(transformed2stTransformed, transform)));

        Matrix4 stIdentity2stOriginal;
        texdefZuTransform(art, p, static_cast<float>(w), static_cast<float>(h), stIdentity2stOriginal);
        const Matrix4 identity2stOriginal(multipliziert(stIdentity2stOriginal, identity2stIdentity));

        const Matrix4 stTransformed2stOriginal = multipliziert(identity2stOriginal, stTransformed2identity);
        if (stTransformed2stOriginal[0] == stTransformed2stOriginal[0]) {   // QNAN abfangen
            texdefAusTransform(art, p, static_cast<float>(w), static_cast<float>(h), stTransformed2stOriginal);
            texdefNormalisieren(art, p, static_cast<float>(w), static_cast<float>(h));
            p.texdef.scale[0] = static_cast<float>(p.texdef.scale[0] / laenge(p.basisS));
            p.texdef.scale[1] = static_cast<float>(p.texdef.scale[1] / laenge(p.basisT));
        }
        normieren(p.basisS);
        normieren(p.basisT);
    }
}

// Texdef_transform (:1827-1868)
void texdefTransformiert(TexdefArt art, TexturProjektion& p, std::size_t w, std::size_t h, const Ebene3d& ebene,
                         const Matrix4& transform, const Vektor3f& invariante) {
    if (transform == kEinheitsMatrix) {
        return;
    }
    if (art == TexdefArt::BrushPrimitives || art == TexdefArt::Valve) {
        texdefTransformiertGesperrt(art, p, w, h, ebene, transform, invariante);
    } else if (art == TexdefArt::Quake) {
        if (p.texdef.scale[0] == 0 || p.texdef.scale[1] == 0) {
            return;
        }
        apTransformieren(p, w, h, ebene, transform, invariante, ebene.normal());
    }
}

// Texdef_Construct_local2tex4projection (:1905-1922)
void lokalZuTexturFuerProjektion(const Texdef& td, std::size_t w, std::size_t h, const Vektor3f& normal,
                                 const Vektor3f* richtung, Matrix4& m) {
    apZuTransform(td, static_cast<float>(w), static_cast<float>(h), m);
    if (richtung) {   // beliebig
        Matrix4 basis = kEinheitsMatrix;
        berechneAchsenbasis(*richtung, basis.x().vec3(), basis.y().vec3());
        basis.z().vec3() = *richtung;
        transponieren(basis);
        multipliziere(m, basis);
    } else {   // AP
        Matrix4 xyz2st;
        normaleZuTransform(normal, xyz2st);
        multipliziere(m, xyz2st);
    }
}

namespace {

// BPTexdef_fromST011 (:1924-1948)
void bpAusSt011(TexturProjektion& p, const Ebene3d& ebene, const Matrix4& local2tex) {
    Vektor3d texX, texY;
    berechneAchsenbasis(ebene.normal(), texX, texY);
    // (0,0), (1,0), (0,1) in der Ebenenbasis -> Weltpunkte auf der Ebene
    const Vektor3d anker = ebene.normal() * ebene.dist();
    const Vektor3d punkte[3] = {anker, anker + texX, anker + texY};
    Vektor3d st[3];
    for (std::size_t i = 0; i < 3; ++i) {
        st[i] = transformierterPunkt(local2tex, punkte[i]);
    }
    p.bp.coords[0][2] = static_cast<float>(zahlMod(st[0][0], 1.0));
    p.bp.coords[1][2] = static_cast<float>(zahlMod(st[0][1], 1.0));
    p.bp.coords[0][0] = static_cast<float>(st[1][0] - st[0][0]);
    p.bp.coords[1][0] = static_cast<float>(st[1][1] - st[0][1]);
    p.bp.coords[0][1] = static_cast<float>(st[2][0] - st[0][0]);
    p.bp.coords[1][1] = static_cast<float>(st[2][1] - st[0][1]);
}

// BP_degenerate (:1992-1996)
bool bpEntartet(const BpTexdef& bp) {
    return kreuz(Vektor2f(bp.coords[0][0], bp.coords[0][1]), Vektor2f(bp.coords[1][0], bp.coords[1][1])) == 0
        || bp.coords[0][0] != bp.coords[0][0];
}

// AP_from_BP (:1999-2018) - in NRC mit m_texdefTypeId == BRUSHPRIMITIVES
void apAusBp(TexturProjektion& p, const Ebene3d& ebene, std::size_t w, std::size_t h) {
    // entartete BP-Basis: Standard
    if (bpEntartet(p.bp)) {
        p.texdef.scale[0] = standardTexturSkalierung();
        p.texdef.scale[1] = standardTexturSkalierung();
        return;
    }
    const Vektor3f invariante(ebene.normal() * ebene.dist());

    Matrix4 local2tex;
    lokalZuTextur(TexdefArt::BrushPrimitives, p, w, h, ebene.normal(), local2tex);
    const Vektor3f st = transformierterPunkt(local2tex, invariante);
    const Vektor2f invarianteSt(st[0] * static_cast<float>(w), st[1] * static_cast<float>(h));

    const Matrix4 tex2local = affinInvers(local2tex);
    apAusAchsen(tex2local.x().vec3() / w, tex2local.y().vec3() / h, ebene.normal(), w, h, invariante, invarianteSt,
                p.texdef);
}

// Valve220_from_BP (:2020-2059, #else-Zweig)
void valveAusBp(TexturProjektion& p, const Ebene3d& ebene, std::size_t w, std::size_t h) {
    Vektor3d texX, texY;
    berechneAchsenbasis(ebene.normal(), texX, texY);
    if (bpEntartet(p.bp)) {
        p.basisS = texX;
        p.basisT = texY;
        p.texdef.scale[0] = standardTexturSkalierung();
        p.texdef.scale[1] = standardTexturSkalierung();
        return;
    }
    p.basisS = normiert(texX * p.bp.coords[0][0] + texY * p.bp.coords[0][1]);
    p.basisT = normiert(texX * p.bp.coords[1][0] + texY * p.bp.coords[1][1]);
    p.bp.removeScale(w, h);
    bpZuSchiebSkalDreh(p.bp, p.texdef);
    p.texdef.shift[0] *= -1;
    if (p.bp.coords[0][0] < 0) {
        p.basisS *= -1;
    }
    if (p.bp.coords[1][1] < 0) {
        p.basisT *= -1;
    }
    p.bp.addScale(w, h);
}

}  // namespace

// Texdef_ProjectTexture (Texdef, :1950-1972)
void texturProjizieren(TexdefArt art, TexturProjektion& p, std::size_t w, std::size_t h, const Ebene3d& ebene,
                       const Texdef& td, const Vektor3f* richtung) {
    if (art == TexdefArt::BrushPrimitives) {
        Matrix4 local2tex;
        lokalZuTexturFuerProjektion(td, w, h, ebene.normal(), richtung, local2tex);
        bpAusSt011(p, ebene, local2tex);
    } else if (art == TexdefArt::Valve) {
        p.texdef = td;
        if (richtung) {   // beliebig
            berechneAchsenbasis(*richtung, p.basisS, p.basisT);
        } else {   // AP
            Matrix4 basis;
            normaleZuTransform(ebene.normal(), basis);
            p.basisS = Vektor3f(basis.xx(), basis.yx(), basis.zx());
            p.basisT = Vektor3f(-basis.xy(), -basis.yy(), -basis.zy());
        }
        valveDrehen(p, td.rotate);
    }
}

// Texdef_ProjectTexture (andere Projektion, :1974-1990)
void texturProjizieren(TexdefArt art, TexturProjektion& p, std::size_t w, std::size_t h, const Ebene3d& ebene,
                       TexturProjektion andere, const Vektor3f& andereNormale) {
    if (art == TexdefArt::BrushPrimitives) {
        andere.bp.addScale(w, h);
        Matrix4 local2tex;
        lokalZuTextur(art, andere, w, h, andereNormale, local2tex);
        bpAusSt011(p, ebene, local2tex);
    } else {
        p.texdef = andere.texdef;
        if (art == TexdefArt::Valve) {
            p.basisS = andere.basisS;
            p.basisT = andere.basisT;
        }
    }
}

// Texdef_Convert (:2062-2103) - in NRC mit m_texdefTypeId == "von"
void texdefUmrechnen(TexdefArt von, TexdefArt nach, const Ebene3d& ebene, TexturProjektion& p, std::size_t w,
                     std::size_t h) {
    switch (nach) {
    case TexdefArt::Quake:
        if (von == TexdefArt::Valve) {
            Matrix4 local2tex;
            lokalZuTextur(von, p, w, h, ebene.normal(), local2tex);
            bpAusSt011(p, ebene, local2tex);
            apAusBp(p, ebene, w, h);   // NRC schaltet dafuer kurz auf BRUSHPRIMITIVES
        } else if (von == TexdefArt::BrushPrimitives) {
            apAusBp(p, ebene, w, h);
        }
        break;
    case TexdefArt::BrushPrimitives: {
        Matrix4 local2tex;
        lokalZuTextur(von, p, w, h, ebene.normal(), local2tex);
        bpAusSt011(p, ebene, local2tex);
        break;
    }
    case TexdefArt::Valve:
        if (von == TexdefArt::Quake) {
            Matrix4 basis;
            normaleZuTransform(ebene.normal(), basis);
            p.basisS = Vektor3f(basis.xx(), basis.yx(), basis.zx());
            p.basisT = Vektor3f(-basis.xy(), -basis.yy(), -basis.zy());
            valveDrehen(p, p.texdef.rotate);
        } else if (von == TexdefArt::BrushPrimitives) {
            valveAusBp(p, ebene, w, h);
        }
        break;
    }
}

// Texdef_from_ST (:2105-2146, #else-Zweig)
void texdefAusSt(TexdefArt art, TexturProjektion& p, const EbenenPunkte& punkte, const Vektor3d st[3], std::size_t w,
                 std::size_t h) {
    const Ebene3d ebene(ebeneAusPunkten(punkte));
    BpTexdef bp;
    bpAusSt(bp, punkte, st, ebene.normal());
    if (bpEntartet(bp)) {
        return;
    }
    p.bp = bp;
    if (art == TexdefArt::Quake) {
        apAusBp(p, ebene, w, h);
    } else if (art == TexdefArt::Valve) {
        valveAusBp(p, ebene, w, h);
    }
}

// Texdef_Construct_local2tex_from_ST (:2148-2162)
void lokalZuTexturAusSt(const EbenenPunkte& punkte, const Vektor3d st[3], Matrix4& m) {
    const Ebene3d ebene(ebeneAusPunkten(punkte));
    BpTexdef bp;
    bpAusSt(bp, punkte, st, ebene.normal(), false);
    bpZuTransform(bp, m);
    Matrix4 basis = kEinheitsMatrix;
    berechneAchsenbasis(ebene.normal(), basis.x().vec3(), basis.y().vec3());
    basis.z().vec3() = ebene.normal();
    transponieren(basis);
    multipliziere(m, basis);
}

// BP_Construct_local2tex (:2164-2174)
void bpLokalZuTextur(const BpTexdef& bp, const Ebene3d& ebene, Matrix4& m) {
    bpZuTransform(bp, m);
    Matrix4 basis = kEinheitsMatrix;
    berechneAchsenbasis(ebene.normal(), basis.x().vec3(), basis.y().vec3());
    basis.z().vec3() = ebene.normal();
    transponieren(basis);
    multipliziere(m, basis);
}

}  // namespace bhed::karte
