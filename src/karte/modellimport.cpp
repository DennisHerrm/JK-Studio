// modellimport.cpp - Karten-Editor, Stufe 9 Punkt 3: Modell-Import per Ziehen (siehe modellimport.h)
//
// Portiert aus OpenJK (und Quake III Arena): das MD3-Format (qfiles.h) und die
// Normalen-Kodierung (tr_model.cpp R_LoadMD3); der Schreiber ist neu.
//
//   Copyright (C) 1999-2005 Id Software, Inc.; 2000-2013 Raven Software; 2013-2026 OpenJK contributors.
//   This program is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free Software
//   Foundation; either version 2 of the License, or (at your option) any later
//   version (LICENSE).
#include "bhed/karte/modellimport.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <array>
#include <map>
#include <tuple>

namespace bhed::karte {

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr std::size_t kMaxPunkte = 1000;    // SHADER_MAX_VERTEXES
constexpr std::size_t kMaxDreiecke = 2000;  // SHADER_MAX_INDEXES / 3
constexpr std::size_t kMaxFlaechen = 32;    // MD3_MAX_SURFACES
constexpr double kXyz = 64.0;               // 1 / MD3_XYZ_SCALE

void i32(std::string& s, std::int32_t v) {
    char b[4];
    std::memcpy(b, &v, 4);
    s.append(b, 4);
}
void f32(std::string& s, float v) {
    char b[4];
    std::memcpy(b, &v, 4);
    s.append(b, 4);
}
void i16(std::string& s, std::int16_t v) {
    char b[2];
    std::memcpy(b, &v, 2);
    s.append(b, 2);
}
void name(std::string& s, const std::string& n, std::size_t laenge) {
    std::string x = n.substr(0, laenge - 1);
    x.resize(laenge, '\0');
    s += x;
}
void setze32(std::string& s, std::size_t an, std::int32_t v) { std::memcpy(s.data() + an, &v, 4); }

std::string sauber(const std::string& t) {
    std::string r;
    for (const char c : t) {
        const unsigned char u = static_cast<unsigned char>(c);
        r += (std::isalnum(u) != 0 || c == '_' || c == '-') ? static_cast<char>(std::tolower(u)) : '_';
    }
    return r.empty() ? std::string("tex") : r;
}

struct Punkt {
    double x, y, z, nx, ny, nz, s, t;
};

}  // namespace

short md3Normale(double x, double y, double z) {
    const double l = std::sqrt(x * x + y * y + z * z);
    if (l <= 0) { return 0; }
    x /= l;
    y /= l;
    z /= l;
    // R_LoadMD3: X = cos(lat) * sin(lng), Y = sin(lat) * sin(lng), Z = cos(lng); 256 Stufen je Kreis
    double lat = std::atan2(y, x) * 256.0 / (2.0 * kPi);
    double lng = std::acos(std::clamp(z, -1.0, 1.0)) * 256.0 / (2.0 * kPi);
    const int a = static_cast<int>(std::lround(lat)) & 255;
    const int b = static_cast<int>(std::lround(lng)) & 255;
    return static_cast<short>(static_cast<std::uint16_t>((a << 8) | b));
}

void md3NormaleAuspacken(short n, double& x, double& y, double& z) {
    const int u = static_cast<std::uint16_t>(n);
    const double lat = static_cast<double>((u >> 8) & 255) * 2.0 * kPi / 256.0;
    const double lng = static_cast<double>(u & 255) * 2.0 * kPi / 256.0;
    x = std::cos(lat) * std::sin(lng);
    y = std::sin(lat) * std::sin(lng);
    z = std::cos(lng);
}

Md3Paket md3Paket(const ImportNetz& netz, const std::string& stammRoh) {
    Md3Paket p;
    const std::string stamm = sauber(stammRoh);
    const std::string ordner = "models/import/" + stamm;
    p.modellPfad = ordner + "/" + stamm + ".md3";
    // je Shader die Dreiecke (Punkte je Teil mit Normalen), Texturen als Dateien
    std::map<std::string, std::vector<std::array<Punkt, 3>>> jeShader;
    std::vector<std::string> reihenfolge;
    std::map<std::string, std::string> bildVon;   // Shader -> Bilddatei (mit Endung) oder leer
    double groesste = 0;
    for (const ImportTeil& t : netz.teile) {
        std::string shader;
        std::string bild;
        if (!t.texturDaten.empty()) {
            std::string basis = t.textur;
            const std::size_t sl = basis.find_last_of("/\\");
            if (sl != std::string::npos) { basis = basis.substr(sl + 1); }
            const std::size_t pk = basis.rfind('.');
            if (pk != std::string::npos && pk > 0) { basis = basis.substr(0, pk); }
            if (basis.empty() || basis[0] == '*') { basis = "tex" + basis.substr(basis.empty() ? 0 : 1); }
            shader = ordner + "/" + sauber(basis);
            bild = shader + "." + (t.texturEndung.empty() ? std::string("png") : sauber(t.texturEndung));
            bool schon = false;
            for (const Md3Paket::Datei& d : p.dateien) { schon = schon || d.pfad == bild; }
            if (!schon) { p.dateien.push_back({bild, t.texturDaten}); }
        } else {
            shader = ordner + "/" + sauber(t.material.empty() ? std::string("default") : t.material);
        }
        if (jeShader.find(shader) == jeShader.end()) {
            reihenfolge.push_back(shader);
            bildVon[shader] = bild;
        }
        // Normalen je Punkt (flaechengewichtet)
        std::vector<std::array<double, 3>> n(t.punkte.size(), {0, 0, 0});
        for (const auto& d : t.dreiecke) {
            if (d[0] >= t.punkte.size() || d[1] >= t.punkte.size() || d[2] >= t.punkte.size()) { continue; }
            const ImportPunkt &a = t.punkte[d[0]], &b = t.punkte[d[1]], &c = t.punkte[d[2]];
            const double ux = b.x - a.x, uy = b.y - a.y, uz = b.z - a.z, vx = c.x - a.x, vy = c.y - a.y, vz = c.z - a.z;
            const double fx = uy * vz - uz * vy, fy = uz * vx - ux * vz, fz = ux * vy - uy * vx;
            for (const std::uint32_t i : d) {
                n[i][0] += fx;
                n[i][1] += fy;
                n[i][2] += fz;
            }
        }
        auto& liste = jeShader[shader];
        for (const auto& d : t.dreiecke) {
            if (d[0] >= t.punkte.size() || d[1] >= t.punkte.size() || d[2] >= t.punkte.size()) { continue; }
            std::array<Punkt, 3> tri{};
            for (std::size_t k = 0; k < 3; ++k) {
                const ImportPunkt& q = t.punkte[d[k]];
                const auto& uv = d[k] < t.uv.size() ? t.uv[d[k]] : std::array<double, 2>{0, 0};
                tri[k] = {q.x, q.y, q.z, n[d[k]][0], n[d[k]][1], n[d[k]][2], uv[0], uv[1]};
                groesste = std::max({groesste, std::fabs(q.x), std::fabs(q.y), std::fabs(q.z)});
            }
            liste.push_back(tri);
        }
    }
    const double grenze = 32767.0 / kXyz - 1.0;   // 510.98
    p.modelscale = groesste > grenze ? groesste / grenze : 1.0;
    const double teiler = p.modelscale;

    // Flaechen: je Shader, in Stuecke von hoechstens 1000 Punkten / 2000 Dreiecken
    struct Flaeche {
        std::string shader;
        std::vector<Punkt> punkte;
        std::vector<std::array<std::int32_t, 3>> dreiecke;
    };
    std::vector<Flaeche> flaechen;
    for (const std::string& sh : reihenfolge) {
        Flaeche f;
        f.shader = sh;
        std::map<std::tuple<long long, long long, long long, long long, long long, short>, std::int32_t> gleich;
        for (const auto& tri : jeShader[sh]) {
            if (f.punkte.size() + 3 > kMaxPunkte || f.dreiecke.size() + 1 > kMaxDreiecke) {
                flaechen.push_back(std::move(f));
                f = Flaeche{};
                f.shader = sh;
                gleich.clear();
            }
            std::array<std::int32_t, 3> idx{};
            for (std::size_t k = 0; k < 3; ++k) {
                const Punkt& q = tri[k];
                const auto key = std::make_tuple(std::llround(q.x * kXyz / teiler), std::llround(q.y * kXyz / teiler), std::llround(q.z * kXyz / teiler),
                                                 std::llround(q.s * 65536), std::llround(q.t * 65536), md3Normale(q.nx, q.ny, q.nz));
                if (const auto it = gleich.find(key); it != gleich.end()) {
                    idx[k] = it->second;
                } else {
                    idx[k] = static_cast<std::int32_t>(f.punkte.size());
                    gleich.emplace(key, idx[k]);
                    f.punkte.push_back(q);
                }
            }
            // MD3 laeuft im Uhrzeigersinn (Q3-Renderer: Rueckseiten von CCW-Netzen) - die
            // Dreiecke der Datei sind gegen den Uhrzeiger (von aussen), also umdrehen
            f.dreiecke.push_back({idx[0], idx[2], idx[1]});
        }
        if (!f.dreiecke.empty()) { flaechen.push_back(std::move(f)); }
    }
    if (flaechen.empty()) {
        p.fehler = "no triangles";
        return p;
    }
    if (flaechen.size() > kMaxFlaechen) {
        p.fehler = "more than 32 surfaces after splitting (" + std::to_string(flaechen.size()) + ") - reduce the model or its materials";
        return p;
    }
    // --- schreiben ---
    std::string m;
    i32(m, 0x33504449);   // "IDP3"
    i32(m, 15);
    name(m, p.modellPfad, 64);
    i32(m, 0);   // flags
    i32(m, 1);   // numFrames
    i32(m, 0);   // numTags
    i32(m, static_cast<std::int32_t>(flaechen.size()));
    i32(m, 0);   // numSkins
    const std::size_t ofsFramesAn = m.size();
    i32(m, 0);   // ofsFrames
    i32(m, 0);   // ofsTags
    const std::size_t ofsSurfAn = m.size();
    i32(m, 0);   // ofsSurfaces
    const std::size_t ofsEndAn = m.size();
    i32(m, 0);   // ofsEnd
    // Rahmen: Grenzen, Ursprung, Radius, Name
    double mn[3] = {1e30, 1e30, 1e30}, mx[3] = {-1e30, -1e30, -1e30};
    for (const Flaeche& f : flaechen) {
        for (const Punkt& q : f.punkte) {
            const double v[3] = {q.x / teiler, q.y / teiler, q.z / teiler};
            for (int i = 0; i < 3; ++i) {
                mn[i] = std::min(mn[i], v[i]);
                mx[i] = std::max(mx[i], v[i]);
            }
        }
    }
    setze32(m, ofsFramesAn, static_cast<std::int32_t>(m.size()));
    for (int i = 0; i < 3; ++i) { f32(m, static_cast<float>(mn[i])); }
    for (int i = 0; i < 3; ++i) { f32(m, static_cast<float>(mx[i])); }
    for (int i = 0; i < 3; ++i) { f32(m, 0.0F); }
    double r = 0;
    for (int i = 0; i < 3; ++i) { r += std::max(mn[i] * mn[i], mx[i] * mx[i]); }
    f32(m, static_cast<float>(std::sqrt(r)));
    name(m, "frame0", 16);
    // ofsTags zeigt wie bei q3data hinter den Rahmen (0 Tags)
    setze32(m, ofsFramesAn + 4, static_cast<std::int32_t>(m.size()));
    setze32(m, ofsSurfAn, static_cast<std::int32_t>(m.size()));
    int nr = 0;
    for (const Flaeche& f : flaechen) {
        const std::size_t start = m.size();
        i32(m, 0x33504449);
        name(m, "surf" + std::to_string(nr++), 64);
        i32(m, 0);   // flags
        i32(m, 1);   // numFrames
        i32(m, 1);   // numShaders
        i32(m, static_cast<std::int32_t>(f.punkte.size()));
        i32(m, static_cast<std::int32_t>(f.dreiecke.size()));
        const std::size_t ofs = m.size();
        for (int i = 0; i < 5; ++i) { i32(m, 0); }   // ofsTriangles, ofsShaders, ofsSt, ofsXyzNormals, ofsEnd
        setze32(m, ofs + 4, static_cast<std::int32_t>(m.size() - start));
        name(m, f.shader, 64);
        i32(m, 0);
        setze32(m, ofs, static_cast<std::int32_t>(m.size() - start));
        for (const auto& d : f.dreiecke) {
            for (const std::int32_t i : d) { i32(m, i); }
        }
        setze32(m, ofs + 8, static_cast<std::int32_t>(m.size() - start));
        for (const Punkt& q : f.punkte) {
            f32(m, static_cast<float>(q.s));
            f32(m, static_cast<float>(q.t));
        }
        setze32(m, ofs + 12, static_cast<std::int32_t>(m.size() - start));
        for (const Punkt& q : f.punkte) {
            i16(m, static_cast<std::int16_t>(std::lround(q.x * kXyz / teiler)));
            i16(m, static_cast<std::int16_t>(std::lround(q.y * kXyz / teiler)));
            i16(m, static_cast<std::int16_t>(std::lround(q.z * kXyz / teiler)));
            i16(m, md3Normale(q.nx, q.ny, q.nz));
        }
        setze32(m, ofs + 16, static_cast<std::int32_t>(m.size() - start));
        p.dreiecke += f.dreiecke.size();
    }
    setze32(m, ofsEndAn, static_cast<std::int32_t>(m.size()));
    p.flaechen = flaechen.size();
    p.dateien.insert(p.dateien.begin(), {p.modellPfad, m});
    // Shader-Skript
    std::string sh = "// JK Studio: " + stamm + " (Stufe 9, Modell-Import)\n";
    for (const std::string& s : reihenfolge) {
        const std::string& bild = bildVon[s];
        sh += "\n" + s + "\n{\n";
        if (!bild.empty()) { sh += "\tqer_editorimage " + bild + "\n"; }
        sh += "\t{\n\t\tmap " + (bild.empty() ? std::string("$whiteimage") : bild) + "\n\t\trgbGen lightingDiffuse\n\t}\n}\n";
    }
    p.dateien.push_back({"scripts/" + stamm + ".shader", sh});
    return p;
}

}  // namespace bhed::karte
