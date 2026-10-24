// modell.cpp - Karten-Editor: Modelle fuer misc_model.
//
// Portiert aus NetRadiant-Custom 20260114 (Quellen in modell.h).
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
#include "bhed/karte/modell.h"

#include "bhed/glm.h"
#include "bhed/md3.h"
#include "bhed/karte/auswahlsystem.h"
#include "bhed/karte/auswahltest.h"
#include "bhed/karte/eklasse.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/manipulator.h"
#include "bhed/karte/xyansicht.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace bhed::karte {

namespace {

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

bool gleichOhneFall(const std::string& a, const std::string& b) { return klein(a) == klein(b); }

bool praefixOhneFall(const std::string& s, const char* p) {   // string_equal_prefix_nocase
    const std::size_t n = std::strlen(p);
    return s.size() >= n && gleichOhneFall(s.substr(0, n), p);
}

bool endungOhneFall(const std::string& s, const std::string& e) {   // string_equal_suffix_nocase
    return s.size() >= e.size() && gleichOhneFall(s.substr(s.size() - e.size()), e);
}

std::string bereinigt(std::string s) {   // PathCleaned
    std::replace(s.begin(), s.end(), '\\', '/');
    return s;
}

std::size_t dateinameAnfang(const std::string& p) {   // path_get_filename_start
    std::size_t i = p.size();
    while (i != 0 && p[i - 1] != '/' && p[i - 1] != '\\') { --i; }
    return i;
}

std::string ohneEndung(const std::string& p) {   // PathExtensionless (path_get_filename_base_end)
    std::size_t i = p.size();
    while (i != 0) {
        --i;
        if (p[i] == '/' || p[i] == '\\') { break; }
        if (p[i] == '.') { return p.substr(0, i); }
    }
    return p;
}

std::string endung(const std::string& p) {
    const std::string o = ohneEndung(p);
    return o.size() < p.size() ? klein(p.substr(o.size() + 1)) : std::string();
}

void grenzenRechnen(Modell& m) {   // updateAABB
    m.grenzen = Kasten();
    for (ModellFlaeche& f : m.flaechen) {
        f.grenzen = Kasten();
        for (const ModellPunkt& p : f.punkte) { kastenErweiternSicher(f.grenzen, p.ort); }
        kastenErweiternSicher(m.grenzen, f.grenzen);
    }
}

// --- picointernal.c: der ASE-Leser ----------------------------------------------------------

class PicoLeser {   // picoParser_t
public:
    explicit PicoLeser(const std::string& text) : m_text(text) {}
    std::string token;
    int zeile = 1;

    // _pico_parse_ex
    bool weiter(bool ueberZeilen, bool anfuehrung) {
        token.clear();
        if (m_pos >= m_text.size()) { return false; }
        const std::size_t alt = m_pos;
        int lfs = 0;
        while (m_pos < m_text.size() && static_cast<unsigned char>(m_text[m_pos]) <= 32) {
            if (m_text[m_pos] == '\n') {
                ++zeile;
                ++lfs;
            }
            ++m_pos;
        }
        if (lfs > 0 && !ueberZeilen) {
            m_pos = alt;
            zeile -= lfs;
            return false;
        }
        if (m_pos < m_text.size() && m_text[m_pos] == '"' && anfuehrung) {
            ++m_pos;
            while (m_pos < m_text.size() && m_text[m_pos] != 0) {
                if (m_text[m_pos] == '\\') {
                    if (m_pos + 1 < m_text.size() && m_text[m_pos + 1] == '"') { ++m_pos; }
                    token += m_text[m_pos++];
                    continue;
                }
                if (m_text[m_pos] == '"') {
                    ++m_pos;
                    break;
                }
                if (m_text[m_pos] == '\n') { ++zeile; }
                token += m_text[m_pos++];
            }
            return true;
        }
        while (m_pos < m_text.size() && static_cast<unsigned char>(m_text[m_pos]) > 32) { token += m_text[m_pos++]; }
        return true;
    }
    bool erstes() { return weiter(true, true); }                       // _pico_parse_first
    const char* naechstes(bool ueberZeilen) { return weiter(ueberZeilen, true) ? token.c_str() : nullptr; }   // _pico_parse
    void restUeberspringen() {   // _pico_parse_skip_rest
        while (weiter(false, false)) {}
    }
    bool ganzzahl(int& aus) {   // _pico_parse_int
        aus = 0;
        if (naechstes(false) == nullptr) { return false; }
        aus = std::atoi(token.c_str());
        return true;
    }
    bool zahl(float& aus) {   // _pico_parse_float
        aus = 0.0F;
        if (naechstes(false) == nullptr) { return false; }
        aus = static_cast<float>(std::atof(token.c_str()));
        return true;
    }
    bool vektor(Vektor3f& aus) {   // _pico_parse_vec
        aus = Vektor3f(0, 0, 0);
        for (std::size_t i = 0; i < 3; ++i) {
            if (naechstes(false) == nullptr) {
                aus = Vektor3f(0, 0, 0);
                return false;
            }
            aus[i] = static_cast<float>(std::atof(token.c_str()));
        }
        return true;
    }
    bool pruefen(bool ueberZeilen, const char* s) { return weiter(ueberZeilen, true) && token == s; }   // _pico_parse_check

private:
    const std::string& m_text;
    std::size_t m_pos = 0;
};

bool istGleich(const std::string& t, const char* s) { return gleichOhneFall(t, s); }   // _pico_stricmp == 0

struct AseShader {   // picoShader_t (nur, was die Anzeige braucht)
    std::string name;
    std::string bild;   // mapName
};
struct AseUntermaterial {   // aseSubMaterial_t
    int id = 0;
    int shader = -1;
};
struct AseMaterial {   // aseMaterial_t
    int id = 0;
    std::vector<AseUntermaterial> unter;   // vorn das zuletzt hinzugefuegte (Liste mit next)
};
struct AsePunkt {
    Vektor3f xyz{0, 0, 0};
    Vektor3f normale{0, 0, 0};
};
struct AseFlaeche {   // aseFace_t
    std::array<int, 9> indizes{};
    int glaettung = 0;
    int material = 0;
    int untermaterial = 0;
};

const AseUntermaterial* untermaterial(const std::vector<AseMaterial>& m, int mtl, int sub) {   // _ase_get_submaterial
    for (const AseMaterial& x : m) {
        if (x.id != mtl) { continue; }
        for (const AseUntermaterial& u : x.unter) {
            if (u.id == sub) { return &u; }
        }
        return nullptr;
    }
    return nullptr;
}

// CopyPicoSurface (assmodel/model.cpp:217-263): der Shader aus Material und Bild
std::string aseShaderName(const AseShader& s, const std::string& wurzel) {
    std::string name;
    const std::string& mat = s.name;
    if (!s.bild.empty() && !praefixOhneFall(mat, "textures/") && !praefixOhneFall(mat, "textures\\") && !praefixOhneFall(mat, "models/") &&
        !praefixOhneFall(mat, "models\\")) {
        name = bereinigt(ohneEndung(s.bild));
    } else {
        name = bereinigt(ohneEndung(mat));
    }
    if (name.find('/') == std::string::npos) {   // texture is likely in the folder, where model is
        name = wurzel + name;
    } else if (name[0] == '/' || (name.size() > 1 && name[1] == ':') || name.find("..") != std::string::npos) {   // absolute path or with ..
        const std::string k = klein(name);
        std::size_t p = k.find("/models/");
        if (p == std::string::npos) { p = k.find("/textures/"); }
        name = p != std::string::npos ? name.substr(p + 1) : wurzel + name.substr(dateinameAnfang(name));
    }
    return name;
}

// PicoAddTriangleToModel / PicoFindSurface: je Knotenname und Shader eine Flaeche
void dreieckDazu(Modell& m, std::vector<std::pair<std::string, int>>& schluessel, const std::string& knoten, const std::string& shader,
                 const std::array<ModellPunkt, 3>& punkte) {
    std::size_t i = 0;
    for (; i < schluessel.size(); ++i) {
        if (schluessel[i].first == knoten && m.flaechen[static_cast<std::size_t>(schluessel[i].second)].shader == shader) { break; }
    }
    if (i == schluessel.size()) {
        schluessel.emplace_back(knoten, static_cast<int>(m.flaechen.size()));
        m.flaechen.emplace_back();
        m.flaechen.back().shader = shader;
    }
    ModellFlaeche& f = m.flaechen[static_cast<std::size_t>(schluessel[i].second)];
    for (const ModellPunkt& p : punkte) {
        f.indizes.push_back(static_cast<std::uint32_t>(f.punkte.size()));
        f.punkte.push_back(p);
    }
}

}  // namespace

Modell nullModell() {   // NullModel / constructNull
    Modell m;
    m.null = true;
    ModellFlaeche f;
    f.shader = "nomodel";
    const Kasten k(Vektor3f(0, 0, 0), Vektor3f(8, 8, 8));
    const Vektor3f mn = k.origin - k.extents;
    const Vektor3f mx = k.origin + k.extents;
    const std::array<Vektor3f, 8> p = {Vektor3f(mn[0], mx[1], mx[2]), Vektor3f(mx[0], mx[1], mx[2]), Vektor3f(mx[0], mn[1], mx[2]),
                                       Vektor3f(mn[0], mn[1], mx[2]), Vektor3f(mn[0], mx[1], mn[2]), Vektor3f(mx[0], mx[1], mn[2]),
                                       Vektor3f(mx[0], mn[1], mn[2]), Vektor3f(mn[0], mn[1], mn[2])};   // aabb_corners
    const std::array<std::array<int, 4>, 6> vierecke = {{{2, 1, 5, 6}, {1, 0, 4, 5}, {0, 1, 2, 3}, {0, 3, 7, 4}, {3, 2, 6, 7}, {7, 6, 5, 4}}};
    const std::array<Vektor3f, 6> normalen = {Vektor3f(1, 0, 0), Vektor3f(0, 1, 0), Vektor3f(0, 0, 1),
                                              Vektor3f(-1, 0, 0), Vektor3f(0, -1, 0), Vektor3f(0, 0, -1)};   // aabb_normals
    const std::array<Vektor2f, 4> st = {Vektor2f(0, 0), Vektor2f(1, 0), Vektor2f(1, 1), Vektor2f(0, 1)};   // topleft, topright, botright, botleft
    for (std::size_t q = 0; q < 6; ++q) {
        for (std::size_t j = 0; j < 4; ++j) {
            f.punkte.push_back(ModellPunkt{p[static_cast<std::size_t>(vierecke[q][j])], normalen[q], st[j]});
        }
        const auto b = static_cast<std::uint32_t>(q * 4);
        for (const std::uint32_t i : {b, b + 1, b + 2, b, b + 2, b + 3}) { f.indizes.push_back(i); }
    }
    m.flaechen.push_back(std::move(f));
    grenzenRechnen(m);
    return m;
}

std::string texturName(const std::string& name) { return bereinigt(ohneEndung(name)); }

bool modellArtBekannt(const std::string& pfad) {
    const std::string e = endung(pfad);
    return e == "md3" || e == "ase" || e == "glm";
}

bool md3Lesen(const std::string& bytes, Modell& m, std::string* fehler) {   // MD3Model_read
    Md3Model md3;
    if (!readMd3(bytes, md3, fehler)) { return false; }
    m = Modell{};
    for (const Md3Surface& s : md3.surfaces) {
        ModellFlaeche f;
        f.shader = texturName(s.shader);   // setShader -> parseTextureName
        const Md3Vertex* v = s.numFrames > 0 ? s.frame(0) : nullptr;
        if (v != nullptr) {
            for (int i = 0; i < s.numVerts; ++i) {
                f.punkte.push_back(ModellPunkt{Vektor3f(v[i].xyz[0], v[i].xyz[1], v[i].xyz[2]), Vektor3f(v[i].normal[0], v[i].normal[1], v[i].normal[2]),
                                               Vektor2f(v[i].st[0], v[i].st[1])});
            }
        }
        f.indizes = s.indexes;
        m.flaechen.push_back(std::move(f));
    }
    grenzenRechnen(m);
    return true;
}

bool glmLesen(const std::string& bytes, const std::string& skinText, Modell& m, std::string* fehler) {
    GlmModel g;
    if (!readGlm(bytes, g, fehler)) { return false; }
    if (!skinText.empty()) { applySkin(skinText, g); }
    m = Modell{};
    for (const GlmSurface& s : g.surfaces) {
        if (!s.isVisible(false)) { continue; }   // Tags, abgeschaltete und "_off"-Flaechen
        ModellFlaeche f;
        f.shader = texturName(!s.texture.empty() ? s.texture : s.shader);
        for (const GlmVertex& v : s.verts) {
            f.punkte.push_back(ModellPunkt{Vektor3f(v.xyz[0], v.xyz[1], v.xyz[2]), Vektor3f(v.normal[0], v.normal[1], v.normal[2]), Vektor2f(v.st[0], v.st[1])});
        }
        f.indizes = s.indexes;
        m.flaechen.push_back(std::move(f));
    }
    grenzenRechnen(m);
    return true;
}

bool aseLesen(const std::string& text, const std::string& modellPfad, Modell& m, std::string* fehler) {   // _ase_load
    m = Modell{};
    const std::string wurzel = bereinigt(modellPfad.substr(0, dateinameAnfang(modellPfad)));   // scene.m_rootPath
    PicoLeser p(text);
    std::string knoten;
    std::vector<AsePunkt> punkte;
    std::vector<Vektor2f> st;
    std::vector<AseFlaeche> flaechen;
    std::vector<AseShader> shader;
    std::vector<AseMaterial> materialien;   // vorn das zuletzt hinzugefuegte
    std::vector<std::pair<std::string, int>> schluessel;
    int eckenId = 0;
    const auto abbruch = [&](const char* text) {
        if (fehler != nullptr) { *fehler = std::string(text) + " in ASE, line " + std::to_string(p.zeile) + "."; }
        m = Modell{};
        return false;
    };
    // _ase_submit_triangles
    const auto abgeben = [&]() {
        for (const AseFlaeche& f : flaechen) {
            const AseUntermaterial* u = untermaterial(materialien, f.material, f.untermaterial);
            if (u == nullptr) { u = untermaterial(materialien, f.material, 0); }   // default submaterial
            if (u == nullptr) { return; }
            const std::string name = aseShaderName(shader[static_cast<std::size_t>(u->shader)], wurzel);
            std::array<ModellPunkt, 3> d;
            for (std::size_t j = 0; j < 3; ++j) {
                const int vi = f.indizes[j];
                const int ti = f.indizes[j + 3];
                if (vi >= 0 && static_cast<std::size_t>(vi) < punkte.size()) {
                    d[j].ort = punkte[static_cast<std::size_t>(vi)].xyz;
                    d[j].normale = punkte[static_cast<std::size_t>(vi)].normale;
                }
                if (ti >= 0 && static_cast<std::size_t>(ti) < st.size()) { d[j].st = st[static_cast<std::size_t>(ti)]; }
            }
            dreieckDazu(m, schluessel, knoten, name, d);
        }
    };
    while (p.erstes()) {
        const std::string& t = p.token;
        if (t.empty()) { continue; }
        if (t[0] != '*' && t[0] != '{' && t[0] != '}') {   // invalid ase statements
            p.restUeberspringen();
            continue;
        }
        if (istGleich(t, "*node_name")) {
            if (p.naechstes(false) == nullptr) { return abbruch("Node name parse error"); }
            knoten = p.token.substr(0, 1023);
        } else if (istGleich(t, "*mesh")) {   // finish existing surface
            abgeben();
            flaechen.clear();
            punkte.clear();
            st.clear();
        } else if (istGleich(t, "*mesh_numvertex")) {
            int n = 0;
            if (!p.ganzzahl(n)) { return abbruch("Missing MESH_NUMVERTEX value"); }
            punkte.assign(static_cast<std::size_t>(std::max(0, n)), AsePunkt{});
        } else if (istGleich(t, "*mesh_numfaces")) {
            int n = 0;
            if (!p.ganzzahl(n)) { return abbruch("Missing MESH_NUMFACES value"); }
            flaechen.assign(static_cast<std::size_t>(std::max(0, n)), AseFlaeche{});
        } else if (istGleich(t, "*mesh_numtvertex")) {
            int n = 0;
            if (!p.ganzzahl(n)) { return abbruch("Missing MESH_NUMTVERTEX value"); }
            st.assign(static_cast<std::size_t>(std::max(0, n)), Vektor2f(0, 0));
        } else if (istGleich(t, "*mesh_numtvfaces")) {
            int n = 0;
            if (!p.ganzzahl(n)) { return abbruch("Missing MESH_NUMTVFACES value"); }
        } else if (istGleich(t, "*mesh_numcvertex")) {
            int n = 0;
            if (!p.ganzzahl(n)) { return abbruch("Missing MESH_NUMCVERTEX value"); }
        } else if (istGleich(t, "*mesh_numcvfaces")) {
            int n = 0;
            if (!p.ganzzahl(n)) { return abbruch("Missing MESH_NUMCVFACES value"); }
        } else if (istGleich(t, "*material_ref")) {
            int id = 0;
            if (!p.ganzzahl(id)) { return abbruch("Missing material reference ID"); }
            for (AseFlaeche& f : flaechen) { f.material = id; }
        } else if (istGleich(t, "*mesh_vertex")) {
            int i = 0;
            if (punkte.empty()) { return abbruch("Vertex parse error"); }
            if (!p.ganzzahl(i)) { return abbruch("Vertex parse error"); }
            Vektor3f v;
            if (!p.vektor(v)) { return abbruch("Vertex parse error"); }
            if (i >= 0 && static_cast<std::size_t>(i) < punkte.size()) { punkte[static_cast<std::size_t>(i)].xyz = v; }
            ++eckenId;
        } else if (istGleich(t, "*mesh_vertexnormal")) {
            int i = 0;
            if (punkte.empty()) { return abbruch("Vertex parse error"); }
            if (!p.ganzzahl(i)) { return abbruch("Vertex parse error"); }
            Vektor3f v;
            if (!p.vektor(v)) { return abbruch("Vertex parse error"); }
            if (i >= 0 && static_cast<std::size_t>(i) < punkte.size()) { punkte[static_cast<std::size_t>(i)].normale = v; }
        } else if (istGleich(t, "*mesh_face")) {
            int i = 0;
            std::array<int, 3> e{};
            if (flaechen.empty()) { return abbruch("Face parse error"); }
            if (!p.ganzzahl(i)) { return abbruch("Face parse error"); }
            for (int j = 0; j < 3; ++j) {
                (void)p.naechstes(false);   // "A:"
                if (!p.ganzzahl(e[static_cast<std::size_t>(j)])) { return abbruch("Face parse error"); }
            }
            int glatt = 0;
            int sub = 0;
            while (p.naechstes(false) != nullptr) {   // parse to the subMaterial ID
                if (istGleich(p.token, "*MESH_SMOOTHING")) { (void)p.ganzzahl(glatt); }
                if (istGleich(p.token, "*MESH_MTLID")) { (void)p.ganzzahl(sub); }
            }
            if (i >= 0 && static_cast<std::size_t>(i) < flaechen.size()) {
                AseFlaeche& f = flaechen[static_cast<std::size_t>(i)];
                f.glaettung = glatt;
                f.untermaterial = sub;
                f.material = 0;
                f.indizes[0] = e[2];
                f.indizes[1] = e[1];
                f.indizes[2] = e[0];
            }
        } else if (istGleich(t, "*mesh_tvert")) {
            int i = 0;
            if (punkte.empty()) { return abbruch("Texture Vertex parse error"); }
            if (!p.ganzzahl(i) || i >= static_cast<int>(st.size())) { return abbruch("Texture vertex parse error"); }
            float s = 0;
            float tt = 0;
            if (!p.zahl(s) || !p.zahl(tt)) { return abbruch("Texture vertex parse error"); }
            if (i >= 0) { st[static_cast<std::size_t>(i)] = Vektor2f(s, 1.0F - tt); }   // ydnar: invert t
        } else if (istGleich(t, "*mesh_tface")) {
            int i = 0;
            std::array<int, 3> e{};
            if (flaechen.empty()) { return abbruch("Texture face parse error"); }
            if (!p.ganzzahl(i) || !p.ganzzahl(e[0]) || !p.ganzzahl(e[1]) || !p.ganzzahl(e[2])) { return abbruch("Texture face parse error"); }
            if (i >= 0 && static_cast<std::size_t>(i) < flaechen.size()) {
                flaechen[static_cast<std::size_t>(i)].indizes[3] = e[2];
                flaechen[static_cast<std::size_t>(i)].indizes[4] = e[1];
                flaechen[static_cast<std::size_t>(i)].indizes[5] = e[0];
            }
        } else if (istGleich(t, "*material")) {
            int index = 0;
            (void)p.ganzzahl(index);
            if (!p.pruefen(true, "{")) { return abbruch("Material missing opening brace"); }
            int ebene = 1;
            int unterEbene = -1;
            int unterId = 0;
            int unterShader = -1;
            bool unterDa = false;
            std::string matName;
            std::string bild;
            while (p.naechstes(true) != nullptr) {
                if (p.token.empty()) { continue; }
                if (p.token[0] == '{') { ++ebene; }
                if (p.token[0] == '}') { --ebene; }
                if (ebene == 0) { break; }
                if (ebene == unterEbene) {   // die Untermaterial-Klammer ist zu
                    std::string n = matName.substr(0, matName.find_first_of(" \t\r\n\v\f"));   // _pico_first_token
                    shader[static_cast<std::size_t>(unterShader)].name = bereinigt(n);
                    shader[static_cast<std::size_t>(unterShader)].bild = bild;
                    // _ase_add_submaterial
                    auto it = std::find_if(materialien.begin(), materialien.end(), [&](const AseMaterial& x) { return x.id == index; });
                    if (it == materialien.end()) {
                        materialien.insert(materialien.begin(), AseMaterial{index, {}});
                        it = materialien.begin();
                    }
                    it->unter.insert(it->unter.begin(), AseUntermaterial{unterId, unterShader});
                    unterDa = true;
                    unterEbene = -1;
                }
                if (istGleich(p.token, "*submaterial")) {
                    (void)p.ganzzahl(unterId);
                    shader.emplace_back();   // PicoNewShader
                    unterShader = static_cast<int>(shader.size()) - 1;
                    unterEbene = ebene;
                } else if (istGleich(p.token, "*material_name")) {
                    if (p.naechstes(false) == nullptr) { return abbruch("Missing material name"); }
                    matName = p.token;
                    p.restUeberspringen();
                    continue;
                } else if (istGleich(p.token, "*map_diffuse")) {
                    int unter = 0;
                    while (p.naechstes(true) != nullptr) {
                        if (p.token.empty()) { continue; }
                        if (p.token[0] == '{') { ++unter; }
                        if (p.token[0] == '}') { --unter; }
                        if (unter == 0) { break; }
                        if (istGleich(p.token, "*bitmap")) {
                            if (p.naechstes(false) == nullptr) { return abbruch("Missing material map bitmap name"); }
                            bild = istGleich(p.token, "none") ? std::string() : p.token;   // standard empty bitmap name
                            p.restUeberspringen();
                            continue;
                        }
                    }
                } else if (istGleich(p.token, "*material_transparency") || istGleich(p.token, "*material_shine") ||
                           istGleich(p.token, "*material_ambient") || istGleich(p.token, "*material_diffuse") ||
                           istGleich(p.token, "*material_specular")) {
                    p.restUeberspringen();   // fuer die Anzeige ohne Belang
                    continue;
                }
            }
            if (!unterDa) {   // this is just a material with 1 submaterial
                shader.push_back(AseShader{bereinigt(matName), bild});
                auto it = std::find_if(materialien.begin(), materialien.end(), [&](const AseMaterial& x) { return x.id == index; });
                if (it == materialien.end()) {
                    materialien.insert(materialien.begin(), AseMaterial{index, {}});
                    it = materialien.begin();
                }
                it->unter.insert(it->unter.begin(), AseUntermaterial{0, static_cast<int>(shader.size()) - 1});
            }
        }
        p.restUeberspringen();   // skip unparsed rest of line
    }
    abgeben();   // ydnar: finish existing surface
    grenzenRechnen(m);
    (void)eckenId;
    return true;
}

std::string skinDateiName(const std::string& modell, const std::string& skin) {   // InsertModel
    const bool ziffern = !skin.empty() && std::all_of(skin.begin(), skin.end(), [](char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; });
    return ziffern ? modell + "_" + skin + ".skin" : ohneEndung(modell) + "_" + skin + ".skin";
}

Umbenennungen skinLesen(const std::string& text, std::vector<std::string>* verworfen) {
    Umbenennungen u;
    std::size_t pos = 0;
    while (pos < text.size()) {
        std::size_t ende = text.find('\r', pos);
        std::size_t naechste = 0;
        if (ende != std::string::npos) {
            naechste = ende + 1;
            if (naechste < text.size() && text[naechste] == '\n') { ++naechste; }   // handle \r\n
        } else {
            ende = text.find('\n', pos);
            naechste = ende != std::string::npos ? ende + 1 : text.size();
            if (ende == std::string::npos) { ende = text.size(); }
        }
        const std::string z = text.substr(pos, ende - pos);
        pos = naechste;
        char a[1024] = {0};   // remap_t: from[ 1024 ], to[ MAX_QPATH ]
        char b[64] = {0};
        if (std::sscanf(z.c_str(), "replace %1023s %63s", a, b) == 2) {
            u.emplace_back(a, b);
            continue;
        }
        if (std::sscanf(z.c_str(), " %1023[^,  ] ,%63s", a, b) == 2) {
            u.emplace_back(a, b);
            continue;
        }
        if (verworfen != nullptr) { verworfen->push_back(z); }   // "Discarding skin directive"
    }
    return u;
}

std::string remapFuer(const Entitaet& e, const std::string& shader) {   // RemapKeysObserver::getRemap
    std::string nach;
    std::size_t laenge = 0;
    for (const auto& [k, v] : e.schluessel()) {
        if (k.compare(0, 6, "_remap") != 0) { continue; }   // string_equal_prefix( key, "_remap" )
        const std::size_t trenner = v.find(';');
        if (trenner == std::string::npos) { continue; }   // m_from = m_to = ""
        const std::string von = v.substr(0, trenner);
        const std::string zu = bereinigt(v.substr(trenner + 1));
        if (von == "*" && laenge == 0) {   // only globbing, if no respective match
            nach = zu;
        } else if (endungOhneFall(shader, von) && von.size() > laenge) {   // longer match has priority
            nach = zu;
            laenge = von.size();
        }
    }
    return nach;
}

Matrix4 miscModellMatrix(const Entitaet& e) {   // matrix4_transform_by_euler_xyz_degrees( origin, angles, scale )
    Vektor3f s(1, 1, 1);
    if (e.hat("modelscale_vec")) {   // ScaleKey: read_scalevec
        float a = 0;
        float b = 0;
        float c = 0;
        if (std::sscanf(e.wert("modelscale_vec").c_str(), "%f %f %f", &a, &b, &c) == 3 && a != 0 && b != 0 && c != 0) { s = Vektor3f(a, b, c); }
    } else {
        float x = 0;
        char rest = 0;
        const std::string w = e.wert("modelscale");
        if (!w.empty() && std::sscanf(w.c_str(), "%f%c", &x, &rest) == 1 && x != 0) { s = Vektor3f(x, x, x); }
    }
    Matrix4 m = verschiebungsMatrix(entitaetUrsprung(e));
    multipliziere(m, drehungEulerXyzGrad(entitaetsWinkel(e.wert("angle"), e.wert("angles"), e.hat("angles"), false)));
    skaliereUm(m, s);
    return m;
}

Kasten modellWeltKasten(const ModellAnsicht& a, const Matrix4& lokalZuWelt) {
    if (!a.modell || !kastenGueltig(a.modell->grenzen)) { return Kasten(); }   // aabb_for_oriented_aabb_safe
    return kastenFuerGedrehtenKasten(a.modell->grenzen, lokalZuWelt);
}

void modellTesten(const ModellAnsicht& a, const Matrix4& lokalZuWelt, AuswahlVolumen& test, Treffer& bester) {
    if (!a.modell) { return; }
    test.netzBeginnen(lokalZuWelt, !a.modell->null);   // BeginMesh( localToWorld, true ) bzw. NullModel ohne
    for (std::size_t i = 0; i < a.modell->flaechen.size(); ++i) {
        const ModellFlaeche& f = a.modell->flaechen[i];
        if (i < a.shader.size() && a.shader[i].empty()) { continue; }
        std::vector<Vektor3f> punkte;
        punkte.reserve(f.punkte.size());
        for (const ModellPunkt& p : f.punkte) { punkte.push_back(p.ort); }
        test.testDreiecke(punkte.data(), f.indizes.data(), f.indizes.size(), bester);
    }
}

}  // namespace bhed::karte
