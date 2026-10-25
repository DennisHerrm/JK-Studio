// brushexport.cpp - Karten-Editor, Stufe 4 Punkt 3: das Plugin brushexport2
// (siehe brushexport.h).
//
// Portiert aus NetRadiant-Custom 20260114, contrib/brushexport/export.cpp und
// plugin.cpp:62-70 (About), libs/stream/textstream.h (FloatFormat "%*.*lf",
// float/double "%g"), radiant/winding.h:234-236 (Edge_isDegenerate).
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
#include "bhed/karte/brushexport.h"

#include "bhed/karte/karte.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/wicklung.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <list>
#include <map>
#include <memory>

namespace bhed::karte::brushexport {

const char* const kAbout = "Brushexport plugin v 3.0 by namespace (http://www.codecreator.net)\nEnjoy!\n\nSend feedback to spam@codecreator.net";

namespace {

constexpr std::size_t MAX_MATERIAL_NAME = 20;   // export.cpp:15

std::string fliess(double f) {   // FloatFormat( f, 1, 6 )
    char b[32];
    std::snprintf(b, sizeof(b), "%*.*lf", 1, 6, f);
    return b;
}
std::string g(double f) {   // ostream_write( double ): "%g"
    char b[16];
    std::snprintf(b, sizeof(b), "%g", f);
    return b;
}

bool kleinerOhneFall(const std::string& a, const std::string& b) {   // string_less_nocase
    const std::size_t n = std::min(a.size(), b.size());
    for (std::size_t i = 0; i < n; ++i) {
        const int x = std::tolower(static_cast<unsigned char>(a[i]));
        const int y = std::tolower(static_cast<unsigned char>(b[i]));
        if (x != y) { return x < y; }
    }
    return a.size() < b.size();
}

// "textures/common/caulk" -> "caulk" (GetShaderNameFromShaderPath)
std::string kurzName(const std::string& pfad) {
    const std::size_t s = pfad.find_last_of('/');
    if (s == std::string::npos || s == pfad.size() - 1) { return pfad; }
    return pfad.substr(s + 1);
}

struct Gruppe {
    std::string name;
    std::list<const Flaeche*> flaechen;
};

// ExportData
class Sammler {
public:
    Sammler(const std::vector<std::string>& ignoriert, Zusammen modus) : m_modus(modus) {
        for (const auto& s : ignoriert) {
            if (std::none_of(m_ignoriert.begin(), m_ignoriert.end(), [&](const std::string& x) { return !kleinerOhneFall(x, s) && !kleinerOhneFall(s, x); })) {
                m_ignoriert.push_back(s);
            }
        }
        if (m_modus == Zusammen::alle) {   // in this mode, we need just one group
            gruppen.emplace_back();
            m_aktuell = &gruppen.back();
            m_aktuell->name = "all";
        }
    }
    void pinselAnfang() {   // BeginBrush
        if (m_modus == Zusammen::keine) {
            gruppen.emplace_back();
            m_aktuell = &gruppen.back();
            m_aktuell->name = "Brush" + std::to_string(gruppen.size());
        }
    }
    void pinselEnde() {   // EndBrush: alle Flaechen ignoriert -> leere Gruppe weg
        if (m_modus == Zusammen::keine && m_aktuell != nullptr && m_aktuell->flaechen.empty()) {
            gruppen.pop_back();
            m_aktuell = nullptr;
        }
    }
    void flaeche(const Flaeche& f) {   // AddBrushFace
        const std::string name = kurzName(f.shader());
        if (std::any_of(m_ignoriert.begin(), m_ignoriert.end(), [&](const std::string& x) { return !kleinerOhneFall(x, name) && !kleinerOhneFall(name, x); })) {
            return;
        }
        if (m_modus == Zusammen::nachMaterial) {
            m_aktuell = nullptr;
            for (Gruppe& gr : gruppen) {
                if (gr.name == name) { m_aktuell = &gr; }
            }
            if (m_aktuell == nullptr) {
                gruppen.emplace_back();
                m_aktuell = &gruppen.back();
                m_aktuell->name = name;
            }
        }
        if (m_aktuell != nullptr) { m_aktuell->flaechen.push_back(&f); }
    }
    std::list<Gruppe> gruppen;

private:
    Zusammen m_modus;
    std::vector<std::string> m_ignoriert;   // StringSetWithLambda (string_less_nocase)
    Gruppe* m_aktuell = nullptr;
};

bool endetMitObj(const std::string& s) {
    if (s.size() < 4) { return false; }
    std::string e = s.substr(s.size() - 4);
    for (char& c : e) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return e == ".obj";
}

}  // namespace

std::string materialName(const std::string& eingabe) {   // path_get_filename_start
    const std::size_t s = eingabe.find_last_of("/\\");
    return s == std::string::npos ? eingabe : eingabe.substr(s + 1);
}

Ergebnis exportieren(const Karte& k, const Optionen& o, const std::string& pfad, const TexturFarbe& farbe) {
    Ergebnis r;
    Sammler sammler(o.ignoriert, o.modus);
    // ForEachSelected: nur Pinsel (BrushInstance), je Flaeche, die beitraegt
    std::vector<std::shared_ptr<const Pinsel>> halten;
    for (const AuswahlElement& a : k.auswahl.elemente()) {
        const auto p = a.primitiv.lock();
        if (!p || p->art() != Primitiv::Art::Pinsel) { continue; }
        const auto b = std::static_pointer_cast<const Pinsel>(p);
        b->bRep(k.kontext);
        halten.push_back(b);
        sammler.pinselAnfang();
        for (std::size_t i = 0; i < b->anzahl(); ++i) {
            if ((*b)[i].traegtBei()) { sammler.flaeche((*b)[i]); }
        }
        sammler.pinselEnde();
    }
    // WriteToFile
    r.objPfad = pfad;
    if (!endetMitObj(r.objPfad)) { r.objPfad += ".obj"; }
    r.mtlPfad = r.objPfad.substr(0, r.objPfad.size() - 4) + ".mtl";
    std::map<std::string, std::array<float, 3>> materialien;   // materials_comparator: lhs < rhs
    std::string& out = r.obj;
    out += "# Wavefront Objectfile exported with radiants brushexport plugin 3.0 by Thomas 'namespace' Nitschke, spam@codecreator.net\n\n";
    if (o.materialien) {
        const std::size_t last = r.mtlPfad.find_last_of("//");
        out += "mtllib " + r.mtlPfad.substr(last + 1) + "\n";
    }
    unsigned vertexAnzahl = 0;
    unsigned texAnzahl = 0;
    for (const Gruppe& gr : sammler.gruppen) {
        std::multimap<std::string, std::string> pinselMaterialien;
        std::vector<Vektor3d> ecken;   // unique vertices list for welding
        out += o.objekte ? "\no " : "\ng ";
        out += gr.name + "\n";
        if (o.materialien && o.modus == Zusammen::alle) {
            out += "usemtl material\n\n";
            materialien.emplace("material", std::array<float, 3>{0.5F, 0.5F, 0.5F});
        }
        for (const Flaeche* f : gr.flaechen) {
            const Wicklung& w = f->wicklung();
            std::string zeile = "\nf";
            std::size_t i = w.numpoints;
            do {
                --i;
                ++texAnzahl;
                std::size_t nummer = 0;   // 0 = kein Punkt zum Verschweissen
                const Vektor3d& v = w.points[i].vertex;
                if (o.verschweissen) {
                    const auto gefunden = std::find_if(ecken.begin(), ecken.end(), [&v](const Vektor3d& x) {   // Edge_isDegenerate
                        const Vektor3d d = x - v;
                        return (d[0] * d[0]) + (d[1] * d[1]) + (d[2] * d[2]) < (kAufEps * kAufEps);
                    });
                    if (gefunden == ecken.end()) {
                        ecken.push_back(v);
                    } else {
                        nummer = vertexAnzahl - static_cast<std::size_t>(std::distance(gefunden, ecken.end())) + 1;
                    }
                }
                if (nummer == 0) {
                    nummer = ++vertexAnzahl;
                    out += "v " + fliess(v[0]) + " " + fliess(v[2]) + " " + fliess(-v[1]) + "\n";
                }
                zeile += " " + std::to_string(nummer) + "/" + std::to_string(texAnzahl);
            } while (i != 0);
            if (o.modus != Zusammen::alle) { materialien.emplace(f->shader(), farbe ? farbe(f->shader()) : std::array<float, 3>{0, 0, 0}); }
            pinselMaterialien.emplace(f->shader(), zeile);
        }
        out += "\n";
        for (const Flaeche* f : gr.flaechen) {   // texcoords
            const Wicklung& w = f->wicklung();
            std::size_t i = w.numpoints;
            do {
                --i;
                out += "vt " + fliess(w.points[i].texcoord[0]) + " " + fliess(-w.points[i].texcoord[1]) + "\n";
            } while (i != 0);
        }
        std::string letztes;
        for (const auto& [mat, flaechen] : pinselMaterialien) {
            if (o.modus != Zusammen::alle && mat != letztes) {
                if (o.kurzeNamen && mat.size() > MAX_MATERIAL_NAME) {
                    out += "\nusemtl " + mat.substr(mat.size() - MAX_MATERIAL_NAME);
                } else {
                    out += "\nusemtl " + mat;
                }
                letztes = mat;
            }
            out += flaechen;
        }
        out += "\n";
    }
    if (o.materialien) {
        r.mtl += "# Wavefront material file exported with NetRadiants brushexport plugin.\n";
        r.mtl += "# Material Count: " + std::to_string(materialien.size()) + "\n\n";
        for (const auto& [name, c] : materialien) {
            if (o.kurzeNamen && name.size() > MAX_MATERIAL_NAME) {
                r.mtl += "newmtl " + name.substr(name.size() - MAX_MATERIAL_NAME) + "\n";
            } else {
                r.mtl += "newmtl " + name + "\n";
            }
            r.mtl += "Kd " + g(c[0]) + " " + g(c[1]) + " " + g(c[2]) + "\n";
            r.mtl += "map_Kd " + name + "\n";
        }
    }
    r.ok = true;
    return r;
}

}  // namespace bhed::karte::brushexport
