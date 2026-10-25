// portale.cpp - Karten-Editor: der Portal Viewer ohne Oberflaeche (Stufe 3 Punkt 5).
//
// Portiert aus NetRadiant-Custom 20260114: contrib/prtview/portals.cpp
// (CBspPortal::Build, CPortals::Load, CPortals::FixColors), prtview.cpp
// (LoadConfig, SaveConfig), libs/profile/profile2.h (IniFile).
//
//   PrtView plugin for GtkRadiant
//   Copyright (C) 2001 Geoffrey Dewan, Loki software and qeradiant.com
//
//   Urspruenglich unter der GNU Lesser General Public License 2.1 (oder
//   spaeter); nach deren Abschnitt 3 hier unter der GNU General Public
//   License, Version 2:
//   This program is free software; you can redistribute it and/or modify it
//   under the terms of the GNU General Public License as published by the
//   Free Software Foundation; version 2 of the License. This program is
//   distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
//   without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
//   PARTICULAR PURPOSE. See the GNU General Public License for more details
//   (LICENSE).
//
//   IniFile: Copyright (c) 2001, Loki software, inc. All rights reserved
//   (BSD-Lizenz, siehe libs/profile/profile2.h in NetRadiant-Custom).
#include "bhed/karte/portale.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace bhed::karte {

// --- IniFile (profile2.h) -----------------------------------------------------------

namespace {

// isSpace: c <= 32 mit char (vorzeichenbehaftet: auch Bytes ab 0x80)
bool istLeer(char c) { return static_cast<signed char>(c) <= 32; }

void hintenKuerzen(std::string& s) {
    while (!s.empty() && istLeer(s.back())) { s.pop_back(); }
}

}  // namespace

void IniDatei::lesen(const std::string& text) {
    std::size_t i = 0;
    std::string zeile;
    std::string abschnitt;
    // filters comments and leading spaces
    const auto zeileHolen = [&]() -> bool {
        zeile.clear();
        while (i < text.size()) {
            const char c = text[i++];
            if (c == '\n') { return true; }   // got line
            if (c == ';' && (zeile.empty() || istLeer(zeile.back()))) {   // entire or trailing comment
                while (i < text.size() && text[i++] != '\n') {}
                return true;
            }
            if (zeile.empty() && istLeer(c)) { continue; }   // skip leading spaces
            zeile.push_back(c);
        }
        return !zeile.empty();   // last line w/o '\n'
    };
    while (zeileHolen()) {
        hintenKuerzen(zeile);
        if (zeile.empty()) { continue; }
        if (zeile.front() == '[') {   // section
            if (zeile.back() == ']') { abschnitt = zeile.substr(1, zeile.size() - 2); }
        } else if (const std::size_t gleich = zeile.find('='); gleich != std::string::npos) {   // key
            std::string schluessel = zeile.substr(0, gleich);
            hintenKuerzen(schluessel);
            std::size_t p = gleich + 1;
            while (p < zeile.size() && istLeer(zeile[p])) { ++p; }   // skip leading value spaces
            if (!schluessel.empty() && !abschnitt.empty()) { m_abschnitte[abschnitt][schluessel] = zeile.substr(p); }
        }
    }
}

std::string IniDatei::schreiben() const {
    std::string s;
    for (const auto& [abschnitt, werte] : m_abschnitte) {
        s += '[' + abschnitt + "]\n";
        for (const auto& [schluessel, wert] : werte) { s += schluessel + '=' + wert + '\n'; }
    }
    return s;
}

void IniDatei::setzen(const std::string& abschnitt, const std::string& schluessel, const std::string& wert) {
    m_abschnitte[abschnitt][schluessel] = wert;
}

std::optional<std::string> IniDatei::wert(const std::string& abschnitt, const std::string& schluessel) const {
    if (const auto a = m_abschnitte.find(abschnitt); a != m_abschnitte.end()) {
        if (const auto k = a->second.find(schluessel); k != a->second.end()) { return k->second; }
    }
    return std::nullopt;
}

// --- Einstellungen (prtview.cpp LoadConfig / SaveConfig, portals.cpp FixColors) -------

namespace {

constexpr const char* kAbschnitt = "Configuration";   // CONFIG_SECTION

int ganzzahl(const IniDatei& ini, const char* schluessel, int vorgabe) {   // PrtViewIniFile::GetInt
    const auto w = ini.wert(kAbschnitt, schluessel);
    return w ? std::atoi(w->c_str()) : vorgabe;
}

void ganzzahlSetzen(IniDatei& ini, const char* schluessel, int wert, const char* kommentar) {   // PrtViewIniFile::SetInt
    char s[512];
    std::snprintf(s, sizeof(s), "%d        ; %s", wert, kommentar);
    ini.setzen(kAbschnitt, schluessel, s);
}

void farbeEntpacken(GepackteFarbe c, float* f) {
    f[0] = static_cast<float>(c & 0xFFU) / 255.0F;           // RGB_UNPACK_R
    f[1] = static_cast<float>((c >> 8) & 0xFFU) / 255.0F;    // RGB_UNPACK_G
    f[2] = static_cast<float>((c >> 16) & 0xFFU) / 255.0F;   // RGB_UNPACK_B
    f[3] = 1;
}

}  // namespace

void PortalEinstellungen::farbenSetzen() {
    farbeEntpacken(farbe2d, fp2d);
    farbeEntpacken(farbe3d, fp3d);
    farbeEntpacken(farbeNebel, fpNebel);
}

PortalEinstellungen portalEinstellungenAus(const IniDatei& ini) {
    PortalEinstellungen p;
    p.zeige2d = ganzzahl(ini, "Render2D", 0) != 0;
    p.breite2d = std::clamp(ganzzahl(ini, "Width2D", 3), 1, 10);
    p.farbe2d = static_cast<GepackteFarbe>(ganzzahl(ini, "Color2D", static_cast<int>(rgbPacken(0, 0, 255)))) & 0xFFFFFFU;
    p.hints = ganzzahl(ini, "DrawHints", 1) != 0;
    p.nichtHints = ganzzahl(ini, "DrawNonHints", 1) != 0;
    p.zeige3d = ganzzahl(ini, "Render3D", 1) != 0;
    p.zPuffer = ganzzahl(ini, "ZBuffer", 1);
    p.nebel = ganzzahl(ini, "Fog", 0) != 0;
    p.polygone = ganzzahl(ini, "Polygons", 1) != 0;
    p.linien = ganzzahl(ini, "Lines", 1) != 0;
    p.breite3d = std::clamp(ganzzahl(ini, "Width3D", 3), 1, 10);
    p.farbe3d = static_cast<GepackteFarbe>(ganzzahl(ini, "Color3D", static_cast<int>(rgbPacken(255, 255, 0)))) & 0xFFFFFFU;
    p.farbeNebel = static_cast<GepackteFarbe>(ganzzahl(ini, "ColorFog", static_cast<int>(rgbPacken(127, 127, 127)))) & 0xFFFFFFU;
    p.deckkraft = std::clamp(ganzzahl(ini, "Opacity", 50), 0, 100);
    p.ausschnitt = ganzzahl(ini, "Clip", 0) != 0;
    p.ausschnittWeite = std::clamp(ganzzahl(ini, "ClipRange", 1024), 64, 8192);
    if (p.zPuffer < 0 || p.zPuffer > 2) { p.zPuffer = 0; }
    p.farbenSetzen();
    return p;
}

IniDatei portalEinstellungenIni(const PortalEinstellungen& p) {
    IniDatei ini;
    ganzzahlSetzen(ini, "Render2D", p.zeige2d ? 1 : 0, "Draw in 2D windows");
    ganzzahlSetzen(ini, "Width2D", p.breite2d, "Width of lines in 2D windows");
    ganzzahlSetzen(ini, "Color2D", static_cast<int>(p.farbe2d), "Color of lines in 2D windows");
    ganzzahlSetzen(ini, "DrawHints", p.hints ? 1 : 0, "Draw Hint Portals");
    ganzzahlSetzen(ini, "DrawNonHints", p.nichtHints ? 1 : 0, "Draw Regular Portals");
    ganzzahlSetzen(ini, "Render3D", p.zeige3d ? 1 : 0, "Draw in 3D windows");
    ganzzahlSetzen(ini, "ZBuffer", p.zPuffer, "ZBuffer level in 3D window");
    ganzzahlSetzen(ini, "Fog", p.nebel ? 1 : 0, "Use depth cueing in 3D window");
    ganzzahlSetzen(ini, "Polygons", p.polygone ? 1 : 0, "Render using polygons in 3D window");
    ganzzahlSetzen(ini, "Lines", p.linien ? 1 : 0, "Render using lines in 3D window");
    ganzzahlSetzen(ini, "Width3D", p.breite3d, "Width of lines in 3D window");
    ganzzahlSetzen(ini, "Color3D", static_cast<int>(p.farbe3d), "Color of lines/polygons in 3D window");
    ganzzahlSetzen(ini, "ColorFog", static_cast<int>(p.farbeNebel), "Color of distant lines/polygons in 3D window");
    ganzzahlSetzen(ini, "Opacity", p.deckkraft, "Opacity in 3d view (0 = invisible, 100 = solid)");
    ganzzahlSetzen(ini, "Clip", p.ausschnitt ? 1 : 0, "Cubic clipper active for portal viewer");
    ganzzahlSetzen(ini, "ClipRange", p.ausschnittWeite, "Portal viewer cubic clip distance (in units of 64)");
    return ini;
}

int PortalZufall::naechste() {
    m_zustand = m_zustand * 214013U + 2531011U;
    return static_cast<int>((m_zustand >> 16) & 0x7FFFU);
}

// --- CBspPortal::Build, CPortals::Load (portals.cpp) ---------------------------------

namespace {

constexpr std::size_t kZeilenPuffer = 4096;   // LINE_BUF

// fgets( buf, LINE_BUF, in ) ueber dem Text (Textmodus: CR LF ist schon LF)
class Zeilen {
public:
    explicit Zeilen(const std::string& t) : m_t(t) {}
    bool holen(std::string& zeile) {
        zeile.clear();
        if (m_i >= m_t.size()) { return false; }
        while (m_i < m_t.size() && zeile.size() + 1 < kZeilenPuffer) {
            const char c = m_t[m_i++];
            zeile.push_back(c);
            if (c == '\n') { break; }
        }
        return true;
    }

private:
    const std::string& m_t;
    std::size_t m_i = 0;
};

bool portalBauen(BspPortal& prt, const std::string& def, bool hintFlags, PortalZufall& zufall) {
    const char* c = def.c_str();
    unsigned int punktZahl = 0;
    int dummy1 = 0;
    int dummy2 = 0;
    int resAnz = 0;
    if (hintFlags) {
        int hint = 0;
        resAnz = std::sscanf(def.c_str(), "%u %d %d %d", &punktZahl, &dummy1, &dummy2, &hint);
        prt.hint = (hint & 0xFF) != 0;   // (int *)&hint auf einen bool: das niedrigste Byte zaehlt
    } else {
        (void)std::sscanf(def.c_str(), "%u", &punktZahl);
        prt.hint = false;
    }
    if (punktZahl < 3 || (hintFlags && resAnz < 4)) { return false; }
    prt.punkte.resize(punktZahl);
    prt.innen.reserve(punktZahl);
    for (std::size_t i = 0; i < prt.punkte.size(); ++i) {
        Vektor3f& p = prt.punkte[i];
        while (*c != 0 && *c != '(') { ++c; }
        if (*c == 0) { return false; }
        ++c;
        (void)std::sscanf(c, "%f %f %f", &p[0], &p[1], &p[2]);
        prt.mitte += p;
        if (i == 0) {
            prt.min = p;
            prt.max = p;
        } else {
            for (std::size_t k = 0; k < 3; ++k) {
                prt.min[k] = std::min(prt.min[k], p[k]);
                prt.max[k] = std::max(prt.max[k], p[k]);
            }
        }
    }
    prt.mitte /= prt.punkte.size();
    for (const Vektor3f& p : prt.punkte) { prt.innen.push_back(prt.mitte * 0.01F + p * 0.99F); }
    prt.farbe[0] = static_cast<float>(zufall.naechste() & 0xFF) / 255.0F;
    prt.farbe[1] = static_cast<float>(zufall.naechste() & 0xFF) / 255.0F;
    prt.farbe[2] = static_cast<float>(zufall.naechste() & 0xFF) / 255.0F;
    prt.farbe[3] = 1;
    return true;
}

}  // namespace

std::vector<PortalMeldung> portaleLaden(const std::string& dateiName, const std::string* inhalt, std::vector<BspPortal>& portale,
                                        bool& hintFlags, PortalZufall& zufall) {
    std::vector<PortalMeldung> m;
    const auto aus = [&m](int art, const std::string& t) { m.push_back({art, t}); };
    portale.clear();   // Purge
    aus(0, std::string(kPortalMeldungVorn) + "Loading portal file " + dateiName + ".\n");
    if (inhalt == nullptr) {
        aus(2, "  ERROR - could not open file.\n");
        return m;
    }
    // fopen( fn, "rt" ): CR LF -> LF
    std::string text;
    text.reserve(inhalt->size());
    for (std::size_t i = 0; i < inhalt->size(); ++i) {
        if ((*inhalt)[i] == '\r' && i + 1 < inhalt->size() && (*inhalt)[i + 1] == '\n') { continue; }
        text.push_back((*inhalt)[i]);
    }
    Zeilen z(text);
    std::string buf;
    const auto zeile = [&]() {   // GETLINE
        if (!z.holen(buf)) {
            aus(2, "  ERROR - File ended prematurely.\n");
            return false;
        }
        return true;
    };
    unsigned int portalZahl = 0;
    unsigned int knotenZahl = 0;
    if (!zeile()) { return m; }
    enum { PRT1, PRT2, PRT1AM } format = PRT1;
    if (std::strncmp("PRT1-AM", buf.c_str(), 7) == 0) {
        format = PRT1AM;
    } else if (std::strncmp("PRT1", buf.c_str(), 4) == 0) {
        format = PRT1;
    } else if (std::strncmp("PRT2", buf.c_str(), 4) == 0) {
        format = PRT2;
    } else {
        aus(2, "  ERROR - File header indicates wrong file type (should be \"PRT1\" or \"PRT2\" or \"PRT1-AM\").\n");
        return m;
    }
    switch (format) {
    case PRT1:
        if (!zeile()) { return m; }   // leafs count (clusters in q3)
        (void)std::sscanf(buf.c_str(), "%u", &knotenZahl);
        if (!zeile()) { return m; }   // portals count
        (void)std::sscanf(buf.c_str(), "%u", &portalZahl);
        break;
    case PRT2:
        if (!zeile()) { return m; }   // leafs count
        (void)std::sscanf(buf.c_str(), "%u", &knotenZahl);
        if (!zeile()) { return m; }   // clusters count
        if (!zeile()) { return m; }   // portals count
        (void)std::sscanf(buf.c_str(), "%u", &portalZahl);
        break;
    case PRT1AM:
        if (!zeile()) { return m; }   // clusters count
        if (!zeile()) { return m; }   // portals count
        (void)std::sscanf(buf.c_str(), "%u", &portalZahl);
        if (!zeile()) { return m; }   // leafs count
        (void)std::sscanf(buf.c_str(), "%u", &knotenZahl);
        break;
    }
    (void)knotenZahl;
    if (portalZahl > 0xFFFF) {
        aus(2, "  ERROR - Extreme number of portals, aborting.\n");
        return m;
    }
    if (portalZahl == 0) {
        aus(2, "  ERROR - number of portals equals 0, aborting.\n");
        return m;
    }
    portale.resize(portalZahl);
    hintFlags = false;
    for (unsigned int n = 0; n < portalZahl;) {
        if (!z.holen(buf)) {
            portale.clear();
            aus(2, "  ERROR - Could not find information for portal number " + std::to_string(n + 1) + " of " + std::to_string(portalZahl) + ".\n");
            return m;
        }
        if (!portalBauen(portale[n], buf, hintFlags, zufall)) {
            int t1 = 0;
            int t2 = 0;
            if (n == 0 && std::sscanf(buf.c_str(), "%d %d", &t1, &t2) == 1) {   // skip additional counts of later data
                hintFlags = true;   // We can count on hint flags being in the file
                continue;
            }
            aus(2, "  ERROR - Information for portal number " + std::to_string(n + 1) + " of " + std::to_string(portalZahl) +
                       " is not formatted correctly.\n");
            portale.clear();
            return m;
        }
        ++n;
    }
    aus(0, "  " + std::to_string(portalZahl) + " portals read in.\n");
    return m;
}

}  // namespace bhed::karte
