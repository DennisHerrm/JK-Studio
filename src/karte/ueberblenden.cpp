// ueberblenden.cpp - Karten-Editor, Stufe 9 Punkt 9: Texturen ueberblenden (siehe ueberblenden.h)
//
// Portiert aus q3map2 (NetRadiant-Custom 20260114): map.cpp LoadEntityIndexMap und die Grenzen
// davor (eMinmax), surface.cpp GetShaderIndexForPoint / GetIndexedShader.
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   This program is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free Software
//   Foundation; either version 2 of the License, or (at your option) any later
//   version (LICENSE).
#include "bhed/karte/ueberblenden.h"

#include "bhed/karte/karte.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/pinsel.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>

namespace bhed::karte {

bool blendGrenzen(const Entitaet& e, const Kontext& k, float mins[3], float maxs[3]) {
    bool da = false;
    const auto dazu = [&](const Vektor3f& p) {
        for (std::size_t i = 0; i < 3; ++i) {
            mins[i] = da ? std::min(mins[i], p[i]) : p[i];
            maxs[i] = da ? std::max(maxs[i], p[i]) : p[i];
        }
        da = true;
    };
    for (const auto& c : e.kinder()) {
        if (!c) { continue; }
        if (c->art() == Primitiv::Art::Pinsel) {   // b.minmax
            const auto& p = static_cast<const Pinsel&>(*c);
            if (p.entartet(k)) { continue; }
            const Kasten& g = p.grenzen(k);
            dazu(g.origin - g.extents);
            dazu(g.origin + g.extents);
        } else if (c->art() == Primitiv::Art::Patch) {   // p.mesh: die Kontrollpunkte
            const auto& p = static_cast<const Patch&>(*c);
            for (std::size_t r = 0; r < p.hoehe(); ++r) {
                for (std::size_t s = 0; s < p.breite(); ++s) { dazu(p.punkt(r, s).vertex); }
            }
        }
    }
    // e.read_keyvalue( minmax.mins, "min" ); e.read_keyvalue( minmax.maxs, "max" );
    float v[3];
    if (std::sscanf(e.wert("min").c_str(), "%f %f %f", &v[0], &v[1], &v[2]) == 3) { std::copy(v, v + 3, mins); }
    if (std::sscanf(e.wert("max").c_str(), "%f %f %f", &v[0], &v[1], &v[2]) == 3) { std::copy(v, v + 3, maxs); }
    return da && maxs[0] > mins[0] && maxs[1] > mins[1];
}

void blendBildpunkt(const float mins[3], const float maxs[3], int breite, int hoehe, float x, float y, int& px, int& py) {
    // GetShaderIndexForPoint (der #else-Zweig)
    const float s = (x - mins[0]) / (maxs[0] - mins[0]);
    const float t = (maxs[1] - y) / (maxs[1] - mins[1]);
    px = std::clamp(static_cast<int>(s * static_cast<float>(breite)), 0, breite - 1);
    py = std::clamp(static_cast<int>(t * static_cast<float>(hoehe)), 0, hoehe - 1);
}

int blendMalen(IndexKarte& im, const float mins[3], const float maxs[3], float x, float y, float radius, int schicht) {
    if (im.breite <= 0 || im.hoehe <= 0 || schicht < 0 || schicht >= im.schichten) { return 0; }
    const float bx = (maxs[0] - mins[0]) / static_cast<float>(im.breite);   // Welteinheiten je Bildpunkt
    const float by = (maxs[1] - mins[1]) / static_cast<float>(im.hoehe);
    int n = 0;
    for (int py = 0; py < im.hoehe; ++py) {
        const float wy = maxs[1] - (static_cast<float>(py) + 0.5F) * by;
        for (int px = 0; px < im.breite; ++px) {
            const float wx = mins[0] + (static_cast<float>(px) + 0.5F) * bx;
            if ((wx - x) * (wx - x) + (wy - y) * (wy - y) > radius * radius) { continue; }
            unsigned char& z = im.index[static_cast<std::size_t>(py * im.breite + px)];
            if (z != schicht) {
                z = static_cast<unsigned char>(schicht);
                ++n;
            }
        }
    }
    return n;
}

std::vector<unsigned char> blendTga(const IndexKarte& im) {
    std::vector<unsigned char> d(18, 0);
    d[2] = 2;   // unkomprimiert, Echtfarben
    d[12] = static_cast<unsigned char>(im.breite & 0xFF);
    d[13] = static_cast<unsigned char>(im.breite >> 8);
    d[14] = static_cast<unsigned char>(im.hoehe & 0xFF);
    d[15] = static_cast<unsigned char>(im.hoehe >> 8);
    d[16] = 32;
    d[17] = 0x28;   // 8 Alpha-Bits, Zeile 0 oben
    d.reserve(18 + static_cast<std::size_t>(im.breite * im.hoehe) * 4);
    const int n = std::max(1, im.schichten);
    for (const unsigned char i : im.index) {
        const auto rot = static_cast<unsigned char>(std::min(255, (i * 256 + 128) / n));
        d.push_back(rot);   // B
        d.push_back(rot);   // G
        d.push_back(rot);   // R: LoadEntityIndexMap nimmt ( pixels32 & 0xFF ) = Rot
        d.push_back(255);
    }
    return d;
}

bool blendTgaLesen(const std::vector<unsigned char>& d, int schichten, IndexKarte& aus) {
    if (d.size() < 18 || d[2] != 2 || (d[16] != 32 && d[16] != 24) || (d[17] & 0x10) != 0) { return false; }
    const int w = d[12] | (d[13] << 8);
    const int h = d[14] | (d[15] << 8);
    const std::size_t bpp = d[16] / 8U;
    const std::size_t start = 18U + d[0];
    if (w < 2 || h < 2 || d.size() < start + static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * bpp) { return false; }
    aus.breite = w;
    aus.hoehe = h;
    aus.schichten = std::max(1, schichten);
    aus.index.assign(static_cast<std::size_t>(w * h), 0);
    const bool obenZuerst = (d[17] & 0x20) != 0;
    for (int y = 0; y < h; ++y) {
        const int zeile = obenZuerst ? y : h - 1 - y;
        for (int x = 0; x < w; ++x) {
            const unsigned char rot = d[start + (static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x)) * bpp + 2];
            aus.index[static_cast<std::size_t>(zeile * w + x)] = static_cast<unsigned char>(std::min(aus.schichten - 1, (rot * aus.schichten) / 256));
        }
    }
    return true;
}

std::string blendShaderText(const std::string& basis, const std::vector<std::string>& texturen) {
    // basis ohne "textures/" (GetIndexedShader setzt es davor); Texturen als Shader-Namen
    const auto bild = [](const std::string& t) { return t + ".tga"; };
    const std::string kopf = "textures/" + basis;
    std::ostringstream o;
    o << "// JK Studio: Texturen ueberblenden (q3map2 _indexmap, Schluessel _layers und _shader \"" << basis << "\")\n\n";
    o << kopf << "\n{\n\tqer_editorimage " << (texturen.empty() ? std::string("textures/system/caulk") : texturen[0]) << "\n\tq3map_terrain\n";
    if (!texturen.empty()) { o << "\t{\n\t\tmap " << bild(texturen[0]) << "\n\t\trgbGen identity\n\t}\n"; }
    o << "\t{\n\t\tmap $lightmap\n\t\tblendFunc GL_DST_COLOR GL_ZERO\n\t\trgbGen identity\n\t}\n}\n\n";
    const auto lightmap = [&o] { o << "\t{\n\t\tmap $lightmap\n\t\tblendFunc GL_DST_COLOR GL_ZERO\n\t\trgbGen identity\n\t}\n}\n\n"; };
    for (std::size_t i = 0; i < texturen.size(); ++i) {
        o << kopf << "_" << i << "\n{\n\tq3map_baseShader " << kopf << "\n\tqer_editorimage " << texturen[i] << "\n";
        o << "\t{\n\t\tmap " << bild(texturen[i]) << "\n\t\trgbGen identity\n\t}\n";
        lightmap();
    }
    for (std::size_t a = 0; a < texturen.size(); ++a) {
        for (std::size_t b = a + 1; b < texturen.size(); ++b) {
            o << kopf << "_" << a << "to" << b << "\n{\n\tq3map_baseShader " << kopf << "\n\tqer_editorimage " << texturen[a] << "\n";
            o << "\t{\n\t\tmap " << bild(texturen[a]) << "\n\t\trgbGen identity\n\t}\n";
            o << "\t{\n\t\tmap " << bild(texturen[b]) << "\n\t\tblendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA\n\t\talphaGen vertex\n\t\trgbGen identity\n\t}\n";
            lightmap();
        }
    }
    return o.str();
}

}  // namespace bhed::karte
