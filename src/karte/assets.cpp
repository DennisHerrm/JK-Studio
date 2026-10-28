// assets.cpp - Karten-Editor, Stufe 9 Punkt 11: Asset-Browser (siehe assets.h)
//
// Portiert aus NetRadiant-Custom 20260114 (Bausteine: VFS GetFileList, Map_SaveSelected,
// Map_ImportFile); der Browser ist neu.
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   This program is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free Software
//   Foundation; either version 2 of the License, or (at your option) any later
//   version (LICENSE).
#include "bhed/karte/assets.h"

#include "bhed/karte/karte.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/texturen.h"
#include "bhed/md3.h"
#include "bhed/sound.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <sstream>

namespace bhed::karte {

namespace {
std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}
std::string dateiname(const std::string& p) {
    const std::size_t s = p.find_last_of("/\\");
    return s == std::string::npos ? p : p.substr(s + 1);
}
}  // namespace

std::vector<Asset> assetsSammeln(const Dateisystem* fs, const std::string& importOrdner) {
    std::vector<Asset> v;
    const auto dazu = [&](AssetArt art, const std::string& pfad) { v.push_back({art, pfad, dateiname(pfad)}); };
    if (fs != nullptr) {
        for (const char* e : {"md3", "glm", "ase", "obj"}) {
            for (const std::string& d : fs->dateien("models/", e, 99)) { dazu(AssetArt::Modell, "models/" + d); }
        }
        for (const char* e : {"wav", "mp3"}) {
            for (const std::string& d : fs->dateien("sound/", e, 99)) { dazu(AssetArt::Klang, "sound/" + d); }
        }
    }
    if (!importOrdner.empty()) {
        std::error_code ec;
        const std::filesystem::path o = std::filesystem::path(std::u8string(importOrdner.begin(), importOrdner.end())) / "prefabs";
        for (std::filesystem::directory_iterator it(o, ec), ende; !ec && it != ende; it.increment(ec)) {
            if (!it->is_regular_file(ec) || klein(it->path().extension().string()) != ".map") { continue; }
            const std::u8string u = it->path().generic_u8string();
            dazu(AssetArt::Prefab, std::string(u.begin(), u.end()));
        }
    }
    std::stable_sort(v.begin(), v.end(), [](const Asset& a, const Asset& b) { return klein(a.pfad) < klein(b.pfad); });
    return v;
}

std::vector<std::size_t> assetsFiltern(const std::vector<Asset>& a, AssetArt art, const std::string& suche) {
    std::vector<std::string> woerter;
    std::istringstream is(klein(suche));
    for (std::string w; is >> w;) { woerter.push_back(w); }
    std::vector<std::size_t> r;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (a[i].art != art) { continue; }
        const std::string p = klein(a[i].pfad);
        bool alle = true;
        for (const std::string& w : woerter) { alle = alle && p.find(w) != std::string::npos; }
        if (alle) { r.push_back(i); }
    }
    return r;
}

std::string md3ErsterShader(const std::string& md3Bytes) {
    Md3Model m;
    if (!readMd3(md3Bytes, m) || m.surfaces.empty()) { return std::string(); }
    return m.surfaces.front().shader;
}

std::array<float, 64> wellenform(const sound::Sound& s) {
    std::array<float, 64> w{};
    if (!s.ok || s.samples.empty() || s.channels <= 0) { return w; }
    const std::size_t rahmen = s.samples.size() / static_cast<std::size_t>(s.channels);
    for (std::size_t i = 0; i < rahmen; ++i) {
        const std::size_t spalte = std::min<std::size_t>(63, i * 64 / std::max<std::size_t>(1, rahmen));
        for (int c = 0; c < s.channels; ++c) {
            const float v = std::fabs(static_cast<float>(s.samples[i * static_cast<std::size_t>(s.channels) + static_cast<std::size_t>(c)]) / 32768.0F);
            w[spalte] = std::max(w[spalte], v);
        }
    }
    return w;
}

PrefabUmriss prefabUmriss(const std::string& mapText) {
    PrefabUmriss u;
    Karte k;
    (void)karteLesen(mapText, k);
    bool erster = true;
    for (const auto& e : k.entitaeten()) {
        for (const auto& p : e->kinder()) {
            if (!p || p->art() != Primitiv::Art::Pinsel) { continue; }
            const Kasten b = static_cast<const Pinsel&>(*p).grenzen(k.kontext);
            const std::array<float, 4> q = {b.origin[0] - b.extents[0], b.origin[1] - b.extents[1], b.origin[0] + b.extents[0], b.origin[1] + b.extents[1]};
            u.kaesten.push_back(q);
            if (erster) {
                u.x0 = q[0];
                u.y0 = q[1];
                u.x1 = q[2];
                u.y1 = q[3];
                erster = false;
            } else {
                u.x0 = std::min(u.x0, q[0]);
                u.y0 = std::min(u.y0, q[1]);
                u.x1 = std::max(u.x1, q[2]);
                u.y1 = std::max(u.y1, q[3]);
            }
            ++u.pinsel;
        }
    }
    return u;
}

}  // namespace bhed::karte
