// xyansicht.cpp - Karten-Editor: die 2D-Ansichten XY / XZ / YZ.
// Portiert aus NetRadiant-Custom 20260114 (Quellen in xyansicht.h).
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
#include "bhed/karte/xyansicht.h"

#include "bhed/karte/patch.h"
#include "bhed/karte/pinsel.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace bhed::karte {

namespace {



std::string zahl(float f) {   // sprintf( "%g" )
    char z[32];
    std::snprintf(z, sizeof(z), "%g", static_cast<double>(f));
    return z;
}

Vektor3f ebenenPunkt(Sicht s, float a, float b) {   // glVertex2f im 2D-Raster
    Vektor3f v(0, 0, 0);
    v[static_cast<std::size_t>(dim1(s))] = a;
    v[static_cast<std::size_t>(dim2(s))] = b;
    return v;
}

}  // namespace

const char* sichtTitel(Sicht s) {
    switch (s) {
    case Sicht::XY: return "XY Top";
    case Sicht::XZ: return "XZ Front";
    case Sicht::YZ: return "YZ Side";
    }
    return "";
}

Farbe farbeAus(float r, float g, float b, float a) {
    const auto k = [](float f) { return static_cast<Farbe>(std::lround(std::clamp(f, 0.0F, 1.0F) * 255.0F)); };
    return k(r) | (k(g) << 8) | (k(b) << 16) | (k(a) << 24);
}

// --- Zustand (xywindow.cpp:222-273, 604-618, 1084-1101, 1706-1781) ------------

void XyAnsicht::setzeUrsprung(const Vektor3f& o) {
    for (std::size_t i = 0; i < 3; ++i) { ursprung[i] = std::min(maxRaster, std::max(-maxRaster, o[i])); }
}

void XyAnsicht::rollen(int x, int y) {
    const auto d1 = static_cast<std::size_t>(dim1(typ));
    const auto d2 = static_cast<std::size_t>(dim2(typ));
    ursprung[d1] += static_cast<float>(x) / massstab;
    ursprung[d2] += static_cast<float>(y) / massstab;
    setzeUrsprung(ursprung);
}

void XyAnsicht::setzeMassstab(float f) {
    const float maxScale = 64.0F;
    // adaptive min scale factor: from 2.0375 with 4096 grid to 1.1 with 64*1024
    const float minScale = static_cast<float>(std::min(breite, hoehe)) /
                           ((1.1F + (1.0F - maxRaster / kMaxWeltKoord)) * 2.0F * maxRaster);
    f = std::min(maxScale, std::max(minScale, f));
    // float_equal_epsilon( m_fScale, f, float_mid( m_fScale, f ) * 1e-5f )
    if (!(std::fabs(f - massstab) < (massstab + f) * 0.5F * 1e-5F)) { massstab = f; }
}

void XyAnsicht::hinein() { setzeMassstab(massstab * 5.0F / 4.0F); }
void XyAnsicht::hinaus() { setzeMassstab(massstab * 4.0F / 5.0F); }

void XyAnsicht::hineinMaus(int x, int y) {
    const float alt = massstab;
    hinein();
    ausgleichen(x, y, alt);
}

void XyAnsicht::hinausMaus(int x, int y) {
    const float alt = massstab;
    hinaus();
    ausgleichen(x, y, alt);
}

void XyAnsicht::ausgleichen(int x, int y, float alt) {
    if (zumZeiger && alt != massstab) {
        const double scaleDiff = 1.0 / alt - 1.0 / massstab;
        Vektor3f o = ursprung;
        o[static_cast<std::size_t>(dim1(typ))] += static_cast<float>(scaleDiff * (x - 0.5 * breite));
        o[static_cast<std::size_t>(dim2(typ))] -= static_cast<float>(scaleDiff * (y - 0.5 * hoehe));
        setzeUrsprung(o);
    }
}

void XyAnsicht::aufGrenzen(const Kasten& k) {
    setzeUrsprung(k.origin);
    const auto d1 = static_cast<std::size_t>(dim1(typ));
    const auto d2 = static_cast<std::size_t>(dim2(typ));
    setzeMassstab(std::min(static_cast<float>(breite) / (3.0F * std::max(128.0F, k.extents[d1])),
                           static_cast<float>(hoehe) / (3.0F * std::max(128.0F, k.extents[d2]))));
}

Vektor3f XyAnsicht::zuPunkt(int x, int y, float fang) const {
    // screen_normalised / normalised_to_world, (m_nWidth / 2) ganzzahlig
    const auto norm = [](int pos, int groesse) { return ((2.0F * static_cast<float>(pos)) / static_cast<float>(groesse)) - 1.0F; };
    Vektor3f p(0, 0, 0);
    const float sx = static_cast<float>(breite / 2) / massstab;
    const float sy = static_cast<float>(hoehe / 2) / massstab;
    const auto d1 = static_cast<std::size_t>(dim1(typ));
    const auto d2 = static_cast<std::size_t>(dim2(typ));
    p[d1] = ursprung[d1] + norm(x, breite) * sx;
    p[d2] = ursprung[d2] + -norm(y, hoehe) * sy;
    return fang > 0 ? eingerastet(p, fang) : p;
}

std::array<float, 2> XyAnsicht::zuBild(const Vektor3f& p) const {
    const auto d1 = static_cast<std::size_t>(dim1(typ));
    const auto d2 = static_cast<std::size_t>(dim2(typ));
    // Viewport: (W/2) und (H/2) ganzzahlig wie View::Construct
    return {(p[d1] - ursprung[d1]) * massstab + static_cast<float>(breite / 2),
            static_cast<float>(hoehe / 2) - (p[d2] - ursprung[d2]) * massstab};
}

Matrix4 XyAnsicht::projektion() const {
    Matrix4 m;
    m[0] = 1.0F / static_cast<float>(breite / 2);
    m[5] = 1.0F / static_cast<float>(hoehe / 2);
    m[10] = 1.0F / (kMaxWeltKoord * massstab);
    m[12] = 0;
    m[13] = 0;
    m[14] = -1;
    m[15] = 1;
    return m;
}

Matrix4 XyAnsicht::modelview() const {
    Matrix4 m;
    const auto d1 = static_cast<std::size_t>(dim1(typ));
    const auto d2 = static_cast<std::size_t>(dim2(typ));
    const float s = massstab;
    m[12] = -ursprung[d1] * s;
    m[13] = -ursprung[d2] * s;
    m[14] = kMaxWeltKoord * s;
    switch (typ) {
    case Sicht::XY:
        m[0] = s;
        m[5] = s;
        m[10] = -s;
        break;
    case Sicht::XZ:
        m[0] = s;
        m[6] = s;
        m[9] = s;
        break;
    case Sicht::YZ:
        m[2] = -s;
        m[4] = s;
        m[9] = s;
        break;
    }
    m[15] = 1;
    return m;
}

// --- Raster (XY_DrawGrid, xywindow.cpp:1279-1460) ------------------------------

RasterZeichnung rasterZeichnen(const XyAnsicht& v, const RasterEinstellungen& e) {
    RasterZeichnung r;
    const Sicht s = v.typ;
    const float sc = v.massstab;
    float step = e.raster;
    float minorStep = e.raster;
    float stepx = e.raster;
    float stepy = e.raster;
    int minorPower = e.potenz;
    while ((minorStep * sc) <= 4.0F) {   // minor grid spacing at least 4 pixels
        ++minorPower;
        minorStep *= 2;
    }
    int power = minorPower;
    while ((power % 3) != 0 || (step * sc) <= 32.0F) {   // major grid spacing at least 32 pixels
        ++power;
        step = std::pow(2.0F, static_cast<float>(power));
    }
    const int mask = (1 << (power - minorPower)) - 1;
    while ((stepx * sc) <= 32.0F) { stepx *= 2; }
    while ((stepy * sc) <= 32.0F) { stepy *= 2; }
    const float a = (e.fang > 0) ? 1.0F : 0.3F;
    r.kleinSchritt = minorStep;
    r.grossSchritt = step;
    r.maske = mask;
    r.textSchrittX = stepx;
    r.textSchrittY = stepy;
    r.alpha = a;

    const float w = v.halbB();
    const float h = v.halbH();
    const auto d1 = static_cast<std::size_t>(dim1(s));
    const auto d2 = static_cast<std::size_t>(dim2(s));
    const float xb = step * std::floor(std::max(v.ursprung[d1] - w, e.regionMin[d1]) / step);
    const float xe = step * std::ceil(std::min(v.ursprung[d1] + w, e.regionMax[d1]) / step);
    const float yb = step * std::floor(std::max(v.ursprung[d2] - h, e.regionMin[d2]) / step);
    const float ye = step * std::ceil(std::min(v.ursprung[d2] + h, e.regionMax[d2]) / step);

    const auto linie = [&](std::vector<Linie>& ziel, float x0, float y0, float x1, float y1, Farbe f) {
        ziel.push_back({ebenenPunkt(s, x0, y0), ebenenPunkt(s, x1, y1), f});
    };
    const auto gitter = [&](float xb_, float xe_, float yb_, float ye_, float alpha) {
        // draw minor blocks
        if (e.klein != e.hinten) {
            const Farbe f = farbeAus(e.klein[0], e.klein[1], e.klein[2], alpha);
            int i = 0;
            for (float x = xb_; x < xe_; x += minorStep, ++i) {
                if ((i & mask) != 0) { linie(r.linien, x, yb_, x, ye_, f); }
            }
            i = 0;
            for (float y = yb_; y < ye_; y += minorStep, ++i) {
                if ((i & mask) != 0) { linie(r.linien, xb_, y, xe_, y, f); }
            }
        }
        // draw major blocks
        if (e.gross != e.klein) {
            const Farbe f = farbeAus(e.gross[0], e.gross[1], e.gross[2], alpha);
            for (float x = xb_; x <= xe_; x += step) { linie(r.linien, x, yb_, x, ye_, f); }
            for (float y = yb_; y <= ye_; y += step) { linie(r.linien, xb_, y, xe_, y, f); }
        }
    };
    if (e.zeigeRaster) {
        gitter(xb, xe, yb, ye, a);
        if (e.regionAktiv) {
            const float xb2 = step * std::floor(std::max(v.ursprung[d1] - w, -v.maxRaster) / step);
            const float xe2 = step * std::ceil(std::min(v.ursprung[d1] + w, v.maxRaster) / step);
            const float yb2 = step * std::floor(std::max(v.ursprung[d2] - h, -v.maxRaster) / step);
            const float ye2 = step * std::ceil(std::min(v.ursprung[d2] + h, v.maxRaster) / step);
            gitter(xb2, xe2, yb2, ye2, 0.5F);
        }
    }

    // Weltpunkt in der Ebene -> Bildpunkt (fuer glRasterPos)
    const auto bild = [&](float a1, float a2) { return v.zuBild(ebenenPunkt(s, a1, a2)); };
    const auto innen = [&](const std::array<float, 2>& p) {   // GL_CURRENT_RASTER_POSITION_VALID
        return p[0] >= 0 && p[0] <= static_cast<float>(v.breite) && p[1] >= 0 && p[1] <= static_cast<float>(v.hoehe);
    };
    const auto text = [&](float a1, float a2, Farbe f, const std::string& t) {
        const auto p = bild(a1, a2);
        if (innen(p)) { r.texte.push_back({p[0], p[1], f, t}); }
    };
    // draw coordinate text if needed
    if (e.zeigeKoordinaten) {
        const Farbe f = farbeAus(e.text[0], e.text[1], e.text[2], 1);
        const float offx = v.ursprung[d2] + h - static_cast<float>(1 + e.schriftHoehe) / sc;
        const float offy = v.ursprung[d1] - w + 4.0F / sc;
        const float fontDescent = static_cast<float>(e.schriftUnterlaenge - 1) / sc;
        for (float x = xb - std::fmod(xb, stepx); x <= xe; x += stepx) { text(x, offx, f, zahl(x)); }
        for (float y = yb - std::fmod(yb, stepy); y <= ye; y += stepy) { text(offy, y - fontDescent, f, zahl(y)); }
    }

    if (e.zeigeAchsen) {
        // XY_DrawAxis (xywindow.cpp:1232-1277)
        const char achsenName[3] = {'X', 'Y', 'Z'};
        const Farbe fx = (s == Sicht::YZ) ? e.achseY : e.achseX;
        const Farbe fy = (s == Sicht::XY) ? e.achseY : e.achseZ;
        const float ox = v.ursprung[d1] - w;
        const float oy = v.ursprung[d2] + h;
        linie(r.linien2, ox + 40 / sc, oy - 45 / sc, ox + 65 / sc, oy - 45 / sc, fx);
        linie(r.linien2, 0, 0, 32 / sc, 0, fx);
        linie(r.linien2, ox + 40 / sc, oy - 45 / sc, ox + 40 / sc, oy - 20 / sc, fy);
        linie(r.linien2, 0, 0, 0, 32 / sc, fy);
        const int fontHeight = e.schriftHoehe;
        const float fontWidth = static_cast<float>(fontHeight) * .55F;
        const auto zeichen = [&](float a1, float a2, Farbe f, char c) { text(a1, a2, f, std::string(1, c)); };
        zeichen(ox + (65 - 3 - fontWidth) / sc, oy - static_cast<float>(45 + 3 + fontHeight) / sc, fx, achsenName[d1]);
        zeichen((32 - fontWidth / 2) / sc, -static_cast<float>(0 + 3 + fontHeight) / sc, fx, achsenName[d1]);
        zeichen(ox + (40 - 4 - fontWidth) / sc, oy - static_cast<float>(20 + 3 + fontHeight) / sc, fy, achsenName[d2]);
        zeichen((0 - 3 - fontWidth) / sc, static_cast<float>(32 - fontHeight / 2) / sc, fy, achsenName[d2]);
    } else {
        const auto& c = e.aktiv ? e.name : e.text;
        text(v.ursprung[d1] - w + 35 / sc, v.ursprung[d2] + h - static_cast<float>(e.schriftHoehe * 2) / sc,
             farbeAus(c[0], c[1], c[2], 1), sichtTitel(s));
    }

    // show current work zone?
    if (e.zeigeWorkzone) {
        const Farbe f = farbeAus(1, 0, 0, 1);
        linie(r.linien, xb, e.workMin[d2], xe, e.workMin[d2], f);
        linie(r.linien, xb, e.workMax[d2], xe, e.workMax[d2], f);
        linie(r.linien, e.workMin[d1], yb, e.workMin[d1], ye, f);
        linie(r.linien, e.workMax[d1], yb, e.workMax[d1], ye, f);
    }

    // XY_DrawBlockGrid (xywindow.cpp:1467-1552)
    if (e.zeigeBloecke && e.weltDa) {
        int bs1 = e.blockGroesse[d1];
        int bs2 = e.blockGroesse[d2];
        if (!(bs1 <= 0 && bs2 <= 0)) {
            float bxb = std::max(v.ursprung[d1] - w, e.regionMin[d1]);
            float bxe = std::min(v.ursprung[d1] + w, e.regionMax[d1]);
            float byb = std::max(v.ursprung[d2] - h, e.regionMin[d2]);
            float bye = std::min(v.ursprung[d2] + h, e.regionMax[d2]);
            if (bs1 > 0) {
                bs1 = std::clamp(bs1, 256, 65536);
                bxb = static_cast<float>(bs1) * std::floor(bxb / static_cast<float>(bs1));
                bxe = static_cast<float>(bs1) * std::ceil(bxe / static_cast<float>(bs1));
            }
            if (bs2 > 0) {
                bs2 = std::clamp(bs2, 256, 65536);
                byb = static_cast<float>(bs2) * std::floor(byb / static_cast<float>(bs2));
                bye = static_cast<float>(bs2) * std::ceil(bye / static_cast<float>(bs2));
            }
            const Farbe f = farbeAus(e.block[0], e.block[1], e.block[2], 1);
            if (bs1 > 0) {
                for (float x = bxb; x <= bxe; x += static_cast<float>(bs1)) { linie(r.linien2, x, byb, x, bye, f); }
            }
            if (bs2 > 0) {
                for (float y = byb; y <= bye; y += static_cast<float>(bs2)) { linie(r.linien2, bxb, y, bxe, y, f); }
            }
        }
    }
    return r;
}

// --- Overlay (DrawCameraIcon, overlayDraw, PaintSizeInfo) ------------------------

std::vector<Linie> kameraSymbol(const XyAnsicht& v, const Vektor3f& ursprung, const Vektor3f& winkel, Farbe farbe) {
    constexpr std::size_t kPitch = 0;   // CAMERA_PITCH
    constexpr std::size_t kYaw = 1;     // CAMERA_YAW
    const float fov = 48 / v.massstab;
    const float box = 16 / v.massstab;
    const Sicht s = v.typ;
    const float x = ursprung[static_cast<std::size_t>(dim1(s))];
    const float y = ursprung[static_cast<std::size_t>(dim2(s))];
    const double a = (s == Sicht::XY) ? gradZuBogen(winkel[kYaw])
                     : (s == Sicht::YZ)
                         ? gradZuBogen((winkel[kYaw] > 180) ? (180.0F - winkel[kPitch]) : winkel[kPitch])
                         : gradZuBogen((winkel[kYaw] < 270 && winkel[kYaw] > 90) ? (180.0F - winkel[kPitch]) : winkel[kPitch]);
    std::vector<Linie> l;
    const auto zug = [&](std::initializer_list<std::array<float, 2>> punkte) {
        const std::array<float, 2>* vorher = nullptr;
        for (const auto& p : punkte) {
            if (vorher != nullptr) { l.push_back({ebenenPunkt(s, (*vorher)[0], (*vorher)[1]), ebenenPunkt(s, p[0], p[1]), farbe}); }
            vorher = &p;
        }
    };
    zug({{x - box, y}, {x, y + (box / 2)}, {x + box, y}, {x, y - (box / 2)}, {x - box, y}, {x + box, y}});
    zug({{x + fov * static_cast<float>(std::cos(a + kPi / 4)), y + fov * static_cast<float>(std::sin(a + kPi / 4))},
         {x, y},
         {x + fov * static_cast<float>(std::cos(a - kPi / 4)), y + fov * static_cast<float>(std::sin(a - kPi / 4))}});
    return l;
}

std::vector<Linie> fadenkreuz(const XyAnsicht& v, const Vektor3f& maus) {
    std::vector<Linie> l;
    const Farbe f = farbeAus(0.2F, 0.9F, 0.2F, 0.8F);
    int d1 = dim1(v.typ);
    int d2 = dim2(v.typ);
    for (int i = 0; i < 2; ++i, std::swap(d1, d2)) {
        Vektor3f a(0, 0, 0);
        a[static_cast<std::size_t>(d1)] = maus[static_cast<std::size_t>(d1)];
        a[static_cast<std::size_t>(d2)] = 2.0F * -v.maxRaster;
        Vektor3f b = a;
        b[static_cast<std::size_t>(d2)] = 2.0F * v.maxRaster;
        l.push_back({a, b, f});
    }
    return l;
}

GroessenAnzeige groessenAnzeige(const XyAnsicht& v, const Kasten& auswahl, Farbe farbe, int schriftHoehe) {
    GroessenAnzeige g;
    if (auswahl.extents == Vektor3f(0, 0, 0)) { return g; }
    const Vektor3f mn = auswahl.origin - auswahl.extents;
    const Vektor3f mx = auswahl.origin + auswahl.extents;
    const Vektor3f mid = auswahl.origin;
    const Vektor3f size = auswahl.extents * 2.0F;
    const char* dim[] = {"x:", "y:", "z:"};
    const auto d1 = static_cast<std::size_t>(dim1(v.typ));
    const auto d2 = static_cast<std::size_t>(dim2(v.typ));
    const float sc = v.massstab;
    const auto zug = [&](std::initializer_list<Vektor3f> punkte) {
        const Vektor3f* vorher = nullptr;
        for (const Vektor3f& p : punkte) {
            if (vorher != nullptr) { g.linien.push_back({*vorher, p, farbe}); }
            vorher = &p;
        }
    };
    Vektor3f p(0, 0, 0);
    Vektor3f q1;
    Vektor3f q2;
    Vektor3f q3;
    Vektor3f q4;
    p[d1] = mn[d1];
    p[d2] = mn[d2] - 6.0F / sc;
    q1 = p;
    p[d2] = mn[d2] - 10.0F / sc;
    q2 = p;
    p[d1] = mx[d1];
    q3 = p;
    p[d2] = mn[d2] - 6.0F / sc;
    q4 = p;
    zug({q1, q2, q3, q4});
    p[d2] = mx[d2];
    p[d1] = mx[d1] + 6.0F / sc;
    q1 = p;
    p[d1] = mx[d1] + 10.0F / sc;
    q2 = p;
    p[d2] = mn[d2];
    q3 = p;
    p[d1] = mx[d1] + 6.0F / sc;
    q4 = p;
    zug({q1, q2, q3, q4});
    const auto text = [&](const Vektor3f& at, const std::string& t) {
        const auto b = v.zuBild(at);
        if (b[0] >= 0 && b[0] <= static_cast<float>(v.breite) && b[1] >= 0 && b[1] <= static_cast<float>(v.hoehe)) {
            g.texte.push_back({b[0], b[1], farbe, t});
        }
    };
    p[d1] = mid[d1];
    p[d2] = mn[d2] - static_cast<float>(10 + 2 + schriftHoehe) / sc;
    text(p, std::string(dim[d1]) + zahl(size[d1]));
    p[d1] = mx[d1] + 16.0F / sc;
    p[d2] = mid[d2] - static_cast<float>(schriftHoehe) / sc / 2;
    text(p, std::string(dim[d2]) + zahl(size[d2]));
    p[d1] = mn[d1] + 4.0F / sc;
    p[d2] = mx[d2] + 5.0F / sc;
    text(p, "(" + std::string(dim[d1]) + zahl(mn[d1]) + "  " + dim[d2] + zahl(mx[d2]) + ")");
    return g;
}

Farbe umrissFarbe(Sicht s, bool split, const RasterEinstellungen& e) {
    if (split) { return farbeAus(e.name[0], e.name[1], e.name[2], 1); }
    return s == Sicht::YZ ? e.achseX : s == Sicht::XZ ? e.achseY : e.achseZ;
}

Vektor3f kameraVersetzen(const XyAnsicht& v, int x, int y, float fang, const Vektor3f& kameraUrsprung) {
    Vektor3f o = v.zuPunkt(x, y, fang);
    const auto n = static_cast<std::size_t>(blickachse(v.typ));
    o[n] = kameraUrsprung[n];
    return o;
}

Vektor3f kameraDrehen(const XyAnsicht& v, int x, int y, const Vektor3f& kameraUrsprung, const Vektor3f& kameraWinkel) {
    constexpr std::size_t kPitch = 0;
    constexpr std::size_t kYaw = 1;
    const Vektor3f point = v.zuPunkt(x, y) - kameraUrsprung;
    const auto d1 = static_cast<std::size_t>(dim1(v.typ));
    const auto d2 = static_cast<std::size_t>(dim2(v.typ));
    const std::size_t nAngle = (v.typ == Sicht::XY) ? kYaw : kPitch;
    Vektor3f angles = kameraWinkel;
    if (point[d2] != 0 || point[d1] != 0) {
        angles[nAngle] = static_cast<float>(bogenZuGrad(std::atan2(point[d2], point[d1])));
        float& p = angles[kPitch];
        float& yw = angles[kYaw];
        if (yw < 0) { yw += 360; }   // normalize yaw to 0..360
        if (nAngle == kPitch) {
            const bool flipPitch = std::fabs(p) > 90;
            if (flipPitch) { p = -p + std::copysign(180.0F, p); }
            if (v.typ == Sicht::YZ) {
                if ((yw < 180) == flipPitch) { yw = 360 - yw; }   // mirror yaw along X
            } else if ((yw < 90 || yw > 270) == flipPitch) {      // mirror yaw along Y
                yw = 180 - yw;
            }
        }
    }
    return angles;
}

// --- Szene ------------------------------------------------------------------------

void pinselKanten(const Pinsel& p, const Kontext& k, Sicht s, Farbe farbe, std::vector<Linie>& aus) {
    const auto& ecken = p.ecken(k);
    const auto& indizes = p.kantenIndizes(k);
    const auto& flaechen = p.kantenFlaechen(k);
    // viewer_test_plane im Ortho: Normale zum Betrachter (XY +z, XZ -y, YZ +x)
    const auto sichtbar = [&](std::size_t f) {
        if (f >= p.anzahl()) { return false; }
        const Vektor3d& n = p.flaechen()[f]->ebene().normal();
        switch (s) {
        case Sicht::XY: return n[2] > 0;
        case Sicht::XZ: return -n[1] > 0;
        case Sicht::YZ: return n[0] > 0;
        }
        return false;
    };
    for (std::size_t i = 0; i < flaechen.size() && i < indizes.size(); ++i) {
        if (sichtbar(flaechen[i].erste) || sichtbar(flaechen[i].zweite)) {
            const Vektor3d a = p.eckpunkt(ecken[indizes[i].erste]);
            const Vektor3d b = p.eckpunkt(ecken[indizes[i].zweite]);
            aus.push_back({Vektor3f(static_cast<float>(a[0]), static_cast<float>(a[1]), static_cast<float>(a[2])),
                           Vektor3f(static_cast<float>(b[0]), static_cast<float>(b[1]), static_cast<float>(b[2])), farbe});
        }
    }
}

void patchGitter(const Patch& p, Farbe farbe, std::vector<Linie>& aus) {
    std::vector<Vektor3f> paare;
    p.drahtgitter(paare);
    for (std::size_t i = 0; i + 1 < paare.size(); i += 2) { aus.push_back({paare[i], paare[i + 1], farbe}); }
}

void kastenDraht(const Kasten& k, Farbe farbe, std::vector<Linie>& aus) {
    // aabb_corners (aabb.h:187-200) und aabb_draw_wire (entitylib.h:119-130)
    const Vektor3f mn = k.origin - k.extents;
    const Vektor3f mx = k.origin + k.extents;
    const Vektor3f pt[8] = {
        Vektor3f(mn[0], mx[1], mx[2]), Vektor3f(mx[0], mx[1], mx[2]), Vektor3f(mx[0], mn[1], mx[2]),
        Vektor3f(mn[0], mn[1], mx[2]), Vektor3f(mn[0], mx[1], mn[2]), Vektor3f(mx[0], mx[1], mn[2]),
        Vektor3f(mx[0], mn[1], mn[2]), Vektor3f(mn[0], mn[1], mn[2]),
    };
    const unsigned int idx[26] = {0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6, 6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7, 1, 7};
    for (std::size_t i = 0; i < 26; i += 2) { aus.push_back({pt[idx[i]], pt[idx[i + 1]], farbe}); }
}

Matrix4 drehungEulerXyzGrad(const Vektor3f& winkel) {
    const double cx = std::cos(gradZuBogen(winkel[0]));
    const double sx = std::sin(gradZuBogen(winkel[0]));
    const double cy = std::cos(gradZuBogen(winkel[1]));
    const double sy = std::sin(gradZuBogen(winkel[1]));
    const double cz = std::cos(gradZuBogen(winkel[2]));
    const double sz = std::sin(gradZuBogen(winkel[2]));
    const auto f = [](double d) { return static_cast<float>(d); };
    return Matrix4(f(cy * cz), f(cy * sz), f(-sy), 0, f(sx * sy * cz + cx * -sz), f(sx * sy * sz + cx * cz), f(sx * cy), 0,
                   f(cx * sy * cz + sx * sz), f(cx * sy * sz + -sx * cz), f(cx * cy), 0, 0, 0, 0, 1);
}

Vektor3f entitaetsWinkel(const std::string& angle, const std::string& angles, bool hatAngles, bool gruppe) {
    const auto normalisieren = [](Vektor3f& a) {   // normalise_angles: float_mod( a, 360 )
        for (std::size_t i = 0; i < 3; ++i) {
            float m = std::fmod(a[i], 360.0F);
            if (m < 0) { m += 360.0F; }
            a[i] = m;
        }
    };
    Vektor3f w(0, 0, 0);
    if (hatAngles) {   // read_angles: "pitch yaw roll" -> (roll, pitch, yaw)
        float p = 0;
        float y = 0;
        float r = 0;
        if (std::sscanf(angles.c_str(), "%f %f %f", &p, &y, &r) == 3) {
            w = Vektor3f(r, p, y);
            normalisieren(w);
        }
        return w;
    }
    if (gruppe && angle == "-1") { return Vektor3f(0, -90, 0); }   // read_group_angle
    if (gruppe && angle == "-2") { return Vektor3f(0, 90, 0); }
    float yaw = 0;
    char rest = 0;
    if (!angle.empty() && std::sscanf(angle.c_str(), "%f%c", &yaw, &rest) == 1) {   // string_parse_float
        w = Vektor3f(0, 0, yaw);
        normalisieren(w);
    }
    return w;
}

void pfeil(const Vektor3f& ursprung, const Vektor3f& winkel, Farbe farbe, std::vector<Linie>& aus) {
    const Matrix4 m = drehungEulerXyzGrad(winkel);
    const Vektor3f vorn = transformierteRichtung(m, Vektor3f(1, 0, 0));
    const Vektor3f links = transformierteRichtung(m, Vektor3f(0, 1, 0));
    const Vektor3f oben = transformierteRichtung(m, Vektor3f(0, 0, 1));
    const Vektor3f end = ursprung + vorn * 32.0F;
    const Vektor3f tip1 = end + vorn * -8.0F + oben * -4.0F;
    const Vektor3f tip2 = tip1 + oben * 8.0F;
    const Vektor3f tip3 = end + vorn * -8.0F + links * -4.0F;
    const Vektor3f tip4 = tip3 + links * 8.0F;
    for (const auto& [a, b] : {std::pair{ursprung, end}, std::pair{end, tip1}, std::pair{end, tip2}, std::pair{end, tip3},
                               std::pair{end, tip4}, std::pair{tip1, tip3}, std::pair{tip3, tip2}, std::pair{tip2, tip4},
                               std::pair{tip4, tip1}}) {
        aus.push_back({a, b, farbe});
    }
}

bool kastenPasst(const Kasten& k, const XyAnsicht& v, int verhaeltnis) {
    // aabb_for_oriented_aabb ueber die Modelview: Ausdehnungen in Bildpunkten
    const Matrix4 mv = v.modelview();
    const Vektor3f e(std::max(k.extents[0], 8.0F), std::max(k.extents[1], 8.0F), std::max(k.extents[2], 8.0F));
    const float ex = std::fabs(mv[0] * e[0]) + std::fabs(mv[4] * e[1]) + std::fabs(mv[8] * e[2]);
    const float ey = std::fabs(mv[1] * e[0]) + std::fabs(mv[5] * e[1]) + std::fabs(mv[9] * e[2]);
    const float vp0 = static_cast<float>(v.breite / 2);
    const float vp5 = static_cast<float>(v.hoehe / 2);
    return (vp0 + vp5) / (ex + ey) < static_cast<float>(verhaeltnis);
}

void lichtOktaeder(const Kasten& k, Farbe farbe, std::vector<Linie>& aus) {
    // light_vertices: top, bottom, middle-up, middle-right, middle-down, middle-left
    const Vektor3f mx = k.origin + k.extents;
    const Vektor3f mn = k.origin - k.extents;
    const Vektor3f& m = k.origin;
    const Vektor3f p[6] = {Vektor3f(m[0], m[1], mx[2]), Vektor3f(m[0], m[1], mn[2]), Vektor3f(m[0], mx[1], m[2]),
                           Vektor3f(mx[0], m[1], m[2]), Vektor3f(m[0], mn[1], m[2]), Vektor3f(mn[0], m[1], m[2])};
    // Dreiecke 0,2,3 0,3,4 0,4,5 0,5,2 1,2,5 1,5,4 1,4,3 1,3,2 als Umrisse
    const unsigned int idx[24] = {0, 2, 3, 0, 3, 4, 0, 4, 5, 0, 5, 2, 1, 2, 5, 1, 5, 4, 1, 4, 3, 1, 3, 2};
    for (std::size_t t = 0; t < 24; t += 3) {
        aus.push_back({p[idx[t]], p[idx[t + 1]], farbe});
        aus.push_back({p[idx[t + 1]], p[idx[t + 2]], farbe});
        aus.push_back({p[idx[t + 2]], p[idx[t]], farbe});
    }
}

void verbindung(const Vektor3f& von, const Vektor3f& nach, Farbe farbe, std::vector<Linie>& aus) {
    // TargetLinesPushBack (targetable.h:185-238)
    Vektor3f dir = nach - von;
    const double len = std::sqrt(static_cast<double>(dir[0]) * dir[0] + static_cast<double>(dir[1]) * dir[1] +
                                 static_cast<double>(dir[2]) * dir[2]);
    if (len == 0) { return; }
    aus.push_back({von, nach, farbe});
    const Vektor3f mid = (nach + von) * .5F;
    dir = dir * static_cast<float>(1.0 / len);
    Vektor3f hack(0.57735F, 0.57735F, 0.57735F);
    int maxI = 0;
    float mxv = 0;
    for (int i = 0; i < 3; ++i) {
        if (dir[static_cast<std::size_t>(i)] < 0) { hack[static_cast<std::size_t>(i)] *= -1; }
        if (std::fabs(dir[static_cast<std::size_t>(i)]) > mxv) {
            maxI = i;
            mxv = std::fabs(dir[static_cast<std::size_t>(i)]);
        }
    }
    hack[static_cast<std::size_t>(maxI)] *= -1;
    const Vektor3f ort(dir[1] * hack[2] - dir[2] * hack[1], dir[2] * hack[0] - dir[0] * hack[2],
                       dir[0] * hack[1] - dir[1] * hack[0]);
    Vektor3f wing1 = mid - dir * 12.0F + ort * 6.0F;
    Vektor3f wing2 = wing1 - ort * 12.0F;
    if (len <= 512 || len > 768) {
        aus.push_back({mid, wing1, farbe});
        aus.push_back({mid, wing2, farbe});
    }
    if (len > 512) {
        const Vektor3f d1 = mid - wing1;
        const Vektor3f d2 = mid - wing2;
        Vektor3f point = von + dir * 256.0F;
        aus.push_back({point, point - d1, farbe});
        aus.push_back({point, point - d2, farbe});
        point = nach - dir * 256.0F;
        aus.push_back({point, point - d1, farbe});
        aus.push_back({point, point - d2, farbe});
    }
}

}  // namespace bhed::karte
