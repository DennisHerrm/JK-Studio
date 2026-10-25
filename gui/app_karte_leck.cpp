// app_karte_leck.cpp - Karten-Editor, Stufe 3 Punkt 5: die Leck-Linie und die
// Rueckmeldungen von q3map2 in den Ansichten.
//
// Ein Leck: q3map2 schreibt die Linie vom Leck nach draussen in <Karte>.lin
// und schickt sie ueber den Rueckkanal (Punkt 4). Der Editor zeigt sie rot
// (4 breit) in den 2D-Ansichten und in der Kamera; File > Pointfile schaltet
// sie um (liest die .lin), Next/Previous leak spot (Strg+Umschalt+K / L)
// stellen Kamera und 2D-Ansichten an die naechste Stelle und blicken die
// Linie entlang. Die Zeilen "point" und "winding" des Q3Map debug window
// zeichnen ihren Punkt bzw. ihre Flaeche in die 2D-Ansichten.
//
// Portiert aus NetRadiant-Custom 20260114: radiant/points.cpp (CPointfile,
// Pointfile_*), radiant/feedback.cpp (CPointMsg::Draw2D, CWindingMsg::Draw2D),
// radiant/renderstate.cpp ($POINTFILE).
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
#include "karteneditor.h"

#include "app_internal.h"
#include "karte_gpu.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace bhed::gui::karteneditor {

namespace {

Editor& e() { return editor(); }

bool dateiLesen(const std::string& pfad, std::string& text) {   // LoadFile( filename, &data ) ("rb")
    std::error_code ec;
    const std::u8string u(pfad.begin(), pfad.end());
    const std::filesystem::path p(u);
    if (!std::filesystem::is_regular_file(p, ec)) { return false; }
    std::ifstream f(p, std::ios::binary);
    if (!f) { return false; }
    std::ostringstream s;
    s << f.rdbuf();
    text = s.str();
    return true;
}

std::string ohneEndung(const std::string& pfad) {   // PathExtensionless
    const std::size_t punkt = pfad.rfind('.');
    const std::size_t trenner = pfad.find_last_of("/\\");
    if (punkt == std::string::npos || (trenner != std::string::npos && punkt < trenner)) { return pfad; }
    return pfad.substr(0, punkt);
}

// Pointfile_Parse (points.cpp:254-305): die Datei der gebauten Karte (Map_Name;
// mit Ziel-Ordner die Kopie dort, Punkt 4)
void punktdateiParsen() {
    Editor& ed = e();
    const std::string stamm = ohneEndung(bauKartenPfad());
    std::string name = stamm + ".lin";
    std::string text;
    if (!dateiLesen(name, text)) {
        fehler("Pointfile " + name + " not found");
        name = stamm + ".pts";   // try .pts (q1)
        if (!dateiLesen(name, text)) {
            fehler("Pointfile " + name + " not found");
            return;
        }
    }
    ausgabe("Reading pointfile " + name);
    std::string kaputt;
    ed.punktdatei = karte::punktdateiLesen(text, &kaputt);   // pointfile.Init() + PushPoint
    if (!kaputt.empty()) { warnung(kaputt); }
}

// CPointfile::show
void punktdateiZeigen(bool zeigen) {
    Editor& ed = e();
    if (zeigen && !ed.punktdateiSichtbar) {
        punktdateiParsen();
        if (!ed.punktdatei.empty()) {   // GenerateDisplayList, SceneChangeNotify
            ed.punktdateiSichtbar = true;
            ++ed.sichtStand;
        }
    } else if (!zeigen && ed.punktdateiSichtbar) {
        ed.punktdateiSichtbar = false;
        ++ed.sichtStand;
    }
}

constexpr float kGradJeBogen = 57.29577951308232F;   // radians_to_degrees

// Pointfile_UpdateViews (points.cpp:182-195)
void ansichtenAnStelle(std::size_t i) {
    Editor& ed = e();
    const karte::Vektor3f& p = ed.punktdatei[i];
    ed.kamera.setzeUrsprung(p);                                    // Camera_setOrigin
    for (XyFenster& x : ed.xy) { x.ansicht.setzeUrsprung(p); }     // forEachXYWnd: SetOrigin
    if (i + 1 < ed.punktdatei.size()) {
        const karte::Vektor3f d = ed.punktdatei[i + 1] - ed.kamera.ursprung;
        const float l = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
        const karte::Vektor3f dir = l > 0.0F ? d / l : d;          // vector3_normalised
        karte::Vektor3f winkel = ed.kamera.winkel;                 // Camera_getAngles
        winkel[1] = std::atan2(dir[1], dir[0]) * kGradJeBogen;     // CAMERA_YAW
        winkel[0] = std::asin(dir[2]) * kGradJeBogen;              // CAMERA_PITCH
        ed.kamera.setzeWinkel(winkel);
    }
    ++ed.sichtStand;
}

}  // namespace

void punktdateiLeeren() { punktdateiZeigen(false); }

void punktdateiUmschalten() {
    Editor& ed = e();
    punktdateiZeigen(!ed.punktdateiSichtbar);
    ed.punktdateiStelle = 0;   // s_check_point = begin()
}

void leckWeiter() {   // Pointfile_Next
    Editor& ed = e();
    if (!ed.punktdateiSichtbar) { return; }
    if (ed.punktdateiStelle + 2 >= ed.punktdatei.size()) {   // s_check_point + 2 == end()
        ausgabe("End of pointfile");
        return;
    }
    ansichtenAnStelle(++ed.punktdateiStelle);
}

void leckZurueck() {   // Pointfile_Prev
    Editor& ed = e();
    if (!ed.punktdateiSichtbar) { return; }
    if (ed.punktdateiStelle == 0) {   // s_check_point == begin()
        ausgabe("Start of pointfile");
        return;
    }
    ansichtenAnStelle(--ed.punktdateiStelle);
}

std::vector<karte::Linie> punktdateiLinien() {
    const Editor& ed = e();
    std::vector<karte::Linie> l;
    if (!ed.punktdateiSichtbar) { return l; }
    const karte::Farbe rot = karte::farbeAus(1, 0, 0);   // $POINTFILE: m_colour( 1, 0, 0, 1 )
    for (std::size_t i = 0; i + 1 < ed.punktdatei.size(); ++i) { l.push_back(karte::Linie{ed.punktdatei[i], ed.punktdatei[i + 1], rot}); }   // GL_LINE_STRIP
    return l;
}

void punktdateiKameraZeichnen(const float* matrix) {
    const Editor& ed = e();
    if (!ed.punktdateiSichtbar || ed.punktdatei.size() < 2) { return; }
    // $POINTFILE: RENDER_DEPTHTEST | RENDER_COLOURWRITE | RENDER_DEPTHWRITE,
    // eSortFullbright, m_linewidth 4; glColor aus dem Zustand
    std::vector<kartegpu::LinienPunkt> p;
    for (std::size_t i = 0; i + 1 < ed.punktdatei.size(); ++i) {
        for (const karte::Vektor3f& v : {ed.punktdatei[i], ed.punktdatei[i + 1]}) {
            kartegpu::LinienPunkt q{};
            q.pos[0] = v[0];
            q.pos[1] = v[1];
            q.pos[2] = v[2];
            q.farbe = 0xFFFFFFFFU;
            p.push_back(q);
        }
    }
    kartegpu::Zustand3 z;
    z.matrix = matrix;
    z.farbe[0] = 1;
    z.farbe[1] = 0;
    z.farbe[2] = 0;
    z.farbe[3] = 1;
    z.tiefe = kartegpu::TiefenTest::Kleiner;
    z.tiefeSchreiben = true;
    z.wegschneiden = false;
    z.breite = 4;
    kartegpu::linien(p, z);
}

RueckmeldungZeichnung rueckmeldungZeichnung2d(karte::Sicht sicht) {
    const Editor& ed = e();
    RueckmeldungZeichnung z;
    const int w = ed.bauRueckmeldungWahl;
    if (w < 0 || w >= static_cast<int>(ed.bauRueckmeldungen.size())) { return z; }   // m_pDraw2D == 0
    const karte::BauRueckmeldung& r = ed.bauRueckmeldungen[static_cast<std::size_t>(w)];
    const int d1 = karte::dim1(sicht);   // NDIM1NDIM2( vt )
    const int d2 = karte::dim2(sicht);
    const karte::Farbe rot = karte::farbeAus(1, 0, 0);
    if (r.art == karte::BauRueckmeldung::Art::Punkt) {   // CPointMsg::Draw2D
        z.punkte.push_back(karte::Linie{r.punkt, r.punkt, rot});
        const auto ecke = [&](float a, float b) {
            karte::Vektor3f p = r.punkt;
            p[d1] += a;
            p[d2] += b;
            return p;
        };
        const karte::Vektor3f q[4] = {ecke(-8, -8), ecke(8, -8), ecke(8, 8), ecke(-8, 8)};
        for (int i = 0; i < 4; ++i) { z.linien.push_back(karte::Linie{q[i], q[(i + 1) % 4], rot}); }
    } else if (r.art == karte::BauRueckmeldung::Art::Wicklung) {   // CWindingMsg::Draw2D
        for (const karte::Vektor3f& p : r.punkte) { z.punkte.push_back(karte::Linie{p, p, rot}); }
        if (r.punkte.size() >= 3) { z.flaechen.push_back(r.punkte); }
    }
    return z;
}

}  // namespace bhed::gui::karteneditor
