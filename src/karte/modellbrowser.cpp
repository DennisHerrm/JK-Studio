// modellbrowser.cpp - Karten-Editor: was der Modell-Browser rechnet.
//
// Portiert aus NetRadiant-Custom 20260114 (Quellen in modellbrowser.h).
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
#include "bhed/karte/modellbrowser.h"

#include "bhed/karte/texturen.h"
#include "bhed/karte/xyansicht.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>

namespace bhed::karte {

namespace {

std::size_t groessteBetragsKomponente(const Vektor3f& v) {   // vector3_max_abs_component_index
    const std::size_t maxi = (std::fabs(v[1]) > std::fabs(v[0])) ? 1 : 0;
    return (std::fabs(v[2]) > std::fabs(v[maxi])) ? 2 : maxi;
}

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

}  // namespace

void ModellOrdner::einfuegen(const std::string& pfad) {   // ModelFS::insert
    const std::size_t schraeg = pfad.find('/');
    if (schraeg == std::string::npos) {
        dateien.insert(pfad);
        return;
    }
    ModellOrdner& o = ordner[pfad.substr(0, schraeg)];
    o.name = pfad.substr(0, schraeg);
    o.einfuegen(pfad.substr(schraeg + 1));
}

std::map<std::string, std::size_t> modellOrdnerListe(const std::string& text) {   // ModelFolders
    std::string str = text;
    std::replace(str.begin(), str.end(), '\\', '/');   // PathCleaned
    std::map<std::string, std::size_t> m;
    std::size_t start = 0;
    for (;;) {
        while (start < str.size() && str[start] == '*') { ++start; }
        std::size_t ende = start;
        while (ende < str.size() && str[ende] != '*') { ++ende; }
        if (start == ende) { break; }
        std::size_t schraeg = std::string::npos;
        for (std::size_t s = start; s != ende; ++s) {
            if (str[s] == '/') { schraeg = s; }
        }
        if (schraeg != std::string::npos && ende - schraeg > 1) {
            const std::size_t tiefe = std::strtoull(str.substr(schraeg + 1, ende - schraeg - 1).c_str(), nullptr, 10);   // Size_importString
            const std::string ordner = start == schraeg ? std::string() : str.substr(start, schraeg + 1 - start);
            m.emplace(ordner, tiefe);
        }
        start = ende;
    }
    if (m.empty()) { m.emplace("models/", 99); }
    return m;
}

ModellOrdner modellBaum(const Dateisystem& fs, const std::string& ordnerListe, const std::vector<std::string>& endungen) {
    ModellOrdner wurzel;
    const auto liste = modellOrdnerListe(ordnerListe);
    std::vector<std::string> klein_endungen;
    for (const std::string& e : endungen) { klein_endungen.push_back(klein(e)); }
    for (const std::string& archiv : fs.archive(false)) {   // forEachArchive( ..., false, false )
        for (const auto& [ordner, tiefe] : liste) {
            fs.dateienImArchiv(archiv, ordner, tiefe, [&](const std::string& name) {
                const std::size_t punkt = name.find_last_of('.');
                const std::size_t schraeg = name.find_last_of('/');
                if (punkt == std::string::npos || (schraeg != std::string::npos && punkt < schraeg)) { return; }
                const std::string e = klein(name.substr(punkt + 1));
                if (std::find(klein_endungen.begin(), klein_endungen.end(), e) != klein_endungen.end()) { wurzel.einfuegen(name); }
            });
        }
    }
    return wurzel;
}

ModellZellen::ModellZellen(int breite, int zelle, int schriftHoehe, int schriftUnterlaenge)
    : m_zelle(zelle), m_schrift(schriftHoehe), m_unterlaenge(schriftUnterlaenge),
      m_jeZeile(std::max(1, (breite - 8) / (zelle * 2 + 8))) {}

Vektor3f ModellZellen::ursprung(int index) const {   // getOrigin
    const int x = (index % m_jeZeile) * m_zelle * 2 + m_zelle + (index % m_jeZeile + 1) * m_randLinks;
    const int z = (index / m_jeZeile) * m_zelle * 2 + m_zelle + (index / m_jeZeile + 1) * (m_schrift + m_randOben);
    return Vektor3f(static_cast<float>(x), 0, static_cast<float>(-z));
}

Vektor3f ModellZellen::textOrt(int index) const {   // getTextPos
    const int x = (index % m_jeZeile) * m_zelle * 2 + (index % m_jeZeile + 1) * m_randLinks;
    const int z = (index / m_jeZeile) * m_zelle * 2 + (index / m_jeZeile + 1) * (m_schrift + m_randOben) - 1 + m_unterlaenge;
    return Vektor3f(static_cast<float>(x), 0, static_cast<float>(-z));
}

int ModellZellen::gesamtHoehe(int hoehe, int anzahl) const {   // totalHeight
    return std::max(hoehe, ((anzahl - 1) / m_jeZeile + 1) * (m_zelle * 2 + m_schrift + m_randOben) + m_schrift);
}

int ModellZellen::treffer(int x, int z) const {   // testSelect
    return std::min(m_jeZeile - 1, (x / (m_zelle * 2 + m_randLinks))) + (m_jeZeile * (z / (m_zelle * 2 + m_schrift + m_randOben)));
}

Matrix4 modellInZelle(const Kasten& grenzen, const ModellZellen& z, int index) {   // models_set_transforms
    Kasten k = grenzen;
    const float skala = static_cast<float>(z.zelle()) / k.extents[groessteBetragsKomponente(k.extents)];
    k.extents[2] *= 2;   // prioritize Z for orientation
    const std::size_t kleinste = kleinsteBetragsKomponente(k.extents);
    const Matrix4 drehung = drehungEulerXyzGrad(kleinste == 0 ? Vektor3f(0, 0, -90) : kleinste == 2 ? Vektor3f(90, 0, 0) : Vektor3f(0, 0, 0));
    return multipliziert(verschiebungsMatrix(z.ursprung(index)),
                         multipliziert(drehung, multipliziert(skalierungsMatrix(Vektor3f(skala, skala, skala)), verschiebungsMatrix(-grenzen.origin))));
}

Matrix4 modellDrehen(const Matrix4& lage, int dx, int dy, const ModellZellen& z, int index) {   // trackingDelta
    const Vektor3f pivot = z.ursprung(index);
    // matrix4_pivoted_rotate_by_euler_xyz_degrees( g_matrix4_identity, ... )
    const Matrix4 rot = multipliziert(verschiebungsMatrix(pivot),
                                      multipliziert(drehungEulerXyzGrad(Vektor3f(static_cast<float>(dy), 0, static_cast<float>(dx)) *
                                                                         (45.0F / static_cast<float>(z.zelle()))),
                                                    verschiebungsMatrix(-pivot)));
    return multipliziert(rot, lage);   // matrix4_premultiply_by_matrix4
}

}  // namespace bhed::karte
