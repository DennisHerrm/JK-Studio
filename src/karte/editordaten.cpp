// editordaten.cpp - Karten-Editor Stufe 8a Punkt 2: "Editor-Daten behalten"
// (siehe editordaten.h).
//
// Portiert aus q3map2 (NetRadiant-Custom 20260114: bspfile_abstract.cpp
// UnparseEntities - welche Entities in den Lump kommen; writebsp.cpp SetLightStyles
// und -keeplights) und aus OpenJK (code/game/g_misc.cpp SP_light, g_utils.cpp
// G_Spawn - was das Spiel mit behaltenen Lichtern macht).
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   Copyright (C) 1999 - 2005, Id Software, Inc.
//   Copyright (C) 2000 - 2013, Raven Software, Inc.
//   Copyright (C) 2001 - 2013, Activision, Inc.
//   Copyright (C) 2013 - 2015, OpenJK contributors
//   Copyright (C) 2026 BehavEd-Remake contributors.
//
//   This program is free software; you can redistribute it and/or modify it
//   under the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. This program is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#include "bhed/karte/editordaten.h"

#include "bhed/karte/baufortschritt.h"
#include "bhed/karte/bspoeffnen.h"

#include <algorithm>
#include <vector>
#include <cctype>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>

namespace bhed::karte {

namespace {

constexpr const char* kKopf = "// BehavEd editor data 1 (not in NetRadiant-Custom): the source map of the .bsp next to this file";

std::uint64_t fnv(std::uint64_t h, const std::string& s) {
    for (const unsigned char c : s) {
        h ^= c;
        h *= 1099511628211ULL;
    }
    return h;
}

// Eine Zeile ab pos (ohne \r\n); pos danach hinter dem Zeilenende
bool zeile(const std::string& t, std::size_t& pos, std::string& z) {
    if (pos >= t.size()) { return false; }
    std::size_t e = t.find('\n', pos);
    if (e == std::string::npos) { return false; }
    z = t.substr(pos, e - pos);
    if (!z.empty() && z.back() == '\r') { z.pop_back(); }
    pos = e + 1;
    return true;
}

}  // namespace

std::uint64_t bspGeometrieFingerabdruck(const BspDatei& b) {
    std::uint64_t h = 1469598103934665603ULL;
    for (std::size_t i = 2; i <= 9 && i < b.lumps.size(); ++i) {
        const std::string n = std::to_string(b.lumps[i].size()) + ":";
        h = fnv(h, n);
        h = fnv(h, b.lumps[i]);
    }
    return h;
}

std::string editorDatenPfad(const std::string& bsp) {
    const std::size_t schnitt = bsp.find_last_of("/\\");
    const std::size_t punkt = bsp.rfind('.');
    if (punkt == std::string::npos || (schnitt != std::string::npos && punkt < schnitt)) { return bsp + ".bhed"; }
    return bsp.substr(0, punkt) + ".bhed";
}

std::string editorDatenSchreiben(const std::string& mapText, const std::string& bspDaten) {
    BspDatei b;
    const std::uint64_t f = bspLesen(bspDaten, b) ? bspGeometrieFingerabdruck(b) : 0ULL;
    char z[128];
    std::snprintf(z, sizeof(z), "// bsp-geometry %016" PRIx64 "\n// map-bytes %zu\n", f, mapText.size());
    return std::string(kKopf) + "\n" + z + mapText;
}

EditorDatenStand editorDatenLesen(const std::string& begleit, const std::string& bspDaten, std::string& mapText) {
    std::size_t pos = 0;
    std::string kopf;
    std::string geo;
    std::string bytes;
    if (!zeile(begleit, pos, kopf) || kopf != kKopf || !zeile(begleit, pos, geo) || !zeile(begleit, pos, bytes)) { return EditorDatenStand::Kaputt; }
    unsigned long long f = 0;
    std::size_t n = 0;
    if (std::sscanf(geo.c_str(), "// bsp-geometry %llx", &f) != 1 || std::sscanf(bytes.c_str(), "// map-bytes %zu", &n) != 1 || begleit.size() - pos != n) {
        return EditorDatenStand::Kaputt;
    }
    BspDatei b;
    if (!bspLesen(bspDaten, b) || bspGeometrieFingerabdruck(b) != f) { return EditorDatenStand::AndereBsp; }
    mapText = begleit.substr(pos);
    return EditorDatenStand::Passt;
}

namespace {

// schalter vor das letzte Argument (die Karte), wenn die Stufe passt und er fehlt
std::string mitSchalter(const std::string& befehl, BauStufe stufe, const std::string& schalter) {
    if (bauStufeAusBefehl(befehl) != stufe) { return befehl; }
    // schon da? (Woerter ohne Anfuehrungszeichen vergleichen)
    std::size_t letzter = std::string::npos;   // Anfang des letzten Wortes
    bool inQuote = false;
    bool imWort = false;
    std::size_t anfang = 0;
    for (std::size_t i = 0; i <= befehl.size(); ++i) {
        const char c = i < befehl.size() ? befehl[i] : ' ';
        const bool trenner = !inQuote && std::isspace(static_cast<unsigned char>(c)) != 0;
        if (c == '"') { inQuote = !inQuote; }
        if (!imWort && !trenner) {
            imWort = true;
            anfang = i;
        } else if (imWort && trenner) {
            imWort = false;
            std::string w = befehl.substr(anfang, i - anfang);
            for (char& x : w) { x = static_cast<char>(std::tolower(static_cast<unsigned char>(x))); }
            if (w == schalter) { return befehl; }
            letzter = anfang;
        }
    }
    if (letzter == std::string::npos || letzter == 0) { return befehl; }
    return befehl.substr(0, letzter) + schalter + " " + befehl.substr(letzter);
}

}  // namespace

std::string befehlMitLichtern(const std::string& befehl) { return mitSchalter(befehl, BauStufe::Bsp, "-keeplights"); }

std::string befehlMitVisWiederverwendung(const std::string& befehl) { return mitSchalter(befehl, BauStufe::Vis, "-bhedvisreuse"); }

std::string befehlMitLichtWiederverwendung(const std::string& befehl) { return mitSchalter(befehl, BauStufe::Licht, "-bhedlightcache"); }

std::string befehlMitGpuBacken(const std::string& befehl) { return mitSchalter(befehl, BauStufe::Licht, "-bhedgpubake"); }

// --- Stufe 8d Punkt 1b: GPU bake Auto ----------------------------------------
const char* gpuBackenName(GpuBacken g) {
    switch (g) {
    case GpuBacken::Aus: return "off";
    case GpuBacken::Ein: return "on";
    case GpuBacken::Auto: return "auto";
    }
    return "auto";
}

GpuBacken gpuBackenAusName(const std::string& s, GpuBacken sonst) {
    if (s == "off") { return GpuBacken::Aus; }
    if (s == "on") { return GpuBacken::Ein; }
    if (s == "auto") { return GpuBacken::Auto; }
    return sonst;
}

namespace {
std::vector<std::string> woerter(const std::string& befehl) {
    std::vector<std::string> w;
    std::string akt;
    bool inQuote = false;
    bool imWort = false;
    for (const char c : befehl) {
        if (c == '"') {
            inQuote = !inQuote;
            imWort = true;
            akt.push_back(c);
            continue;
        }
        if (!inQuote && std::isspace(static_cast<unsigned char>(c)) != 0) {
            if (imWort) { w.push_back(akt); }
            akt.clear();
            imWort = false;
            continue;
        }
        akt.push_back(c);
        imWort = true;
    }
    if (imWort) { w.push_back(akt); }
    return w;
}
}  // namespace

std::string befehlMitGpuBackenAuto(const std::string& befehl, long long cpuMs, long long gpuMs) {
    if (bauStufeAusBefehl(befehl) != BauStufe::Licht) { return befehl; }
    for (const std::string& w : woerter(befehl)) {
        if (w == "-bhedgpubakeauto") { return befehl; }   // schon da (mit seinen Zeiten)
    }
    const std::string b = mitSchalter(befehl, BauStufe::Licht, "-bhedgpubakeauto");
    return mitSchalter(b, BauStufe::Licht, "-bhedgpubakezeiten " + std::to_string(std::max(0LL, cpuMs)) + " " +
                                               std::to_string(std::max(0LL, gpuMs)));
}


std::string lichtSchalter(const std::string& befehl) {
    const std::vector<std::string> w = woerter(befehl);
    std::size_t i = 0;
    while (i < w.size() && w[i] != "-light") { ++i; }
    if (i >= w.size()) { return {}; }
    std::string s = "-light";
    for (std::size_t k = i + 1; k + 1 < w.size(); ++k) {   // ohne das letzte Wort (die Karte)
        const std::string& x = w[k];
        if (x == "-bhedgpubake" || x == "-bhedgpubakeauto") { continue; }
        if (x == "-bhedgpubakezeiten") {
            k += 2;
            continue;
        }
        if (x == "-bhedgpubakecheck" || x == "-bhedgpuadapter") {
            ++k;
            continue;
        }
        s += " " + x;
    }
    return s;
}

bool GpuZeitenSpeicher::lesen(const std::string& text) {
    eintraege_.clear();
    std::size_t a = 0;
    while (a < text.size()) {
        std::size_t b = text.find('\n', a);
        if (b == std::string::npos) { b = text.size(); }
        std::string z = text.substr(a, b - a);
        a = b + 1;
        if (!z.empty() && z.back() == '\r') { z.pop_back(); }
        // "<cpuMs>\t<gpuMs>\t<karte>\t<schalter>"
        const std::size_t t1 = z.find('\t');
        const std::size_t t2 = t1 == std::string::npos ? t1 : z.find('\t', t1 + 1);
        const std::size_t t3 = t2 == std::string::npos ? t2 : z.find('\t', t2 + 1);
        if (t3 == std::string::npos) { continue; }
        Eintrag e;
        e.z.cpuMs = std::max(0LL, std::atoll(z.substr(0, t1).c_str()));
        e.z.gpuMs = std::max(0LL, std::atoll(z.substr(t1 + 1, t2 - t1 - 1).c_str()));
        e.karte = z.substr(t2 + 1, t3 - t2 - 1);
        e.schalter = z.substr(t3 + 1);
        eintraege_.push_back(std::move(e));
    }
    while (eintraege_.size() > kMax) { eintraege_.erase(eintraege_.begin()); }
    return true;
}

std::string GpuZeitenSpeicher::schreiben() const {
    std::string s;
    for (const Eintrag& e : eintraege_) {
        s += std::to_string(e.z.cpuMs) + "\t" + std::to_string(e.z.gpuMs) + "\t" + e.karte + "\t" + e.schalter + "\n";
    }
    return s;
}

GpuZeiten GpuZeitenSpeicher::holen(const std::string& karte, const std::string& schalter) const {
    for (const Eintrag& e : eintraege_) {
        if (e.karte == karte && e.schalter == schalter) { return e.z; }
    }
    return {};
}

void GpuZeitenSpeicher::merken(const std::string& karte, const std::string& schalter, bool gpu, long long ms) {
    if (karte.empty() || schalter.empty() || ms <= 0 || karte.find_first_of("\t\n") != std::string::npos ||
        schalter.find_first_of("\t\n") != std::string::npos) {
        return;
    }
    Eintrag neu{karte, schalter, {}};
    for (auto it = eintraege_.begin(); it != eintraege_.end(); ++it) {
        if (it->karte == karte && it->schalter == schalter) {
            neu = *it;
            eintraege_.erase(it);
            break;
        }
    }
    (gpu ? neu.z.gpuMs : neu.z.cpuMs) = ms;
    eintraege_.push_back(std::move(neu));
    while (eintraege_.size() > kMax) { eintraege_.erase(eintraege_.begin()); }
}

bool gpuBackZeile(const std::string& zeile, GpuBackBericht& b) {
    static const std::string kGpu = "BehavEd GPU bake: ";
    static const std::string kBericht = "BehavEd GPU bake report: ";
    const std::size_t p = zeile.find("BehavEd GPU bake");
    if (p == std::string::npos) { return false; }
    const std::string z = zeile.substr(p);
    const auto zahlVor = [&z](const std::string& wort, long long& wert) {   // die Zahl vor " <wort>"
        const std::size_t q = z.find(" " + wort);
        if (q == std::string::npos) { return false; }
        std::size_t a = q;
        while (a > 0 && std::isdigit(static_cast<unsigned char>(z[a - 1])) != 0) { --a; }
        if (a == q) { return false; }
        wert = std::stoll(z.substr(a, q - a));
        return true;
    };
    if (z.rfind(kBericht, 0) == 0) {
        b.bericht = zahlVor("luxels in", b.luxel) && zahlVor("lightmaps checked", b.lms);
        (void)zahlVor("lightmaps with a difference", b.lmsAnders);
        (void)zahlVor("points on the GPU", b.gitterGpu);
        const std::size_t gitter = z.find("light grid: ");
        if (gitter != std::string::npos) {   // "light grid: N points on the GPU, M on the CPU"
            const std::size_t komma = z.find(", ", gitter);
            if (komma != std::string::npos) { b.gitterCpu = std::atoll(z.c_str() + komma + 2); }
        }
        const std::size_t g = z.find(" % identical");
        if (g != std::string::npos) {
            std::size_t a = g;
            while (a > 0 && (std::isdigit(static_cast<unsigned char>(z[a - 1])) != 0 || z[a - 1] == '.')) { --a; }
            b.gleich = z.substr(a, g - a);
        }
        return true;
    }
    static const std::string kAuto = "BehavEd GPU bake auto: ";
    if (z.rfind(kAuto, 0) == 0) {   // Stufe 8d: "BehavEd GPU bake auto: GPU (large) - 1286512 super luxels, ..."
        b.autoGesehen = true;
        b.autoGpu = z.compare(kAuto.size(), 3, "GPU") == 0;
        const std::size_t k1 = z.find('(', kAuto.size());
        const std::size_t k2 = k1 == std::string::npos ? k1 : z.find(')', k1);
        if (k2 != std::string::npos) { b.autoGrund = z.substr(k1 + 1, k2 - k1 - 1); }
        (void)zahlVor("super luxels", b.autoLuxel);
        return true;
    }
    if (z.rfind(kGpu, 0) != 0) { return true; }
    if (z.find("no usable GPU") != std::string::npos) {
        b.keineGpu = true;
        return true;
    }
    const std::size_t fl = z.find(" (feature level");
    if (fl != std::string::npos && b.adapter.empty()) {
        b.adapter = z.substr(kGpu.size(), fl - kGpu.size());
        return true;
    }
    if (z.find(" rounds, ") != std::string::npos) {
        b.kopf = zahlVor("lightmaps on the GPU", b.gpuLm);
        const std::size_t c = z.find("), ");
        if (c != std::string::npos) {
            const std::string rest = z.substr(c + 3);
            std::size_t e = 0;
            while (e < rest.size() && std::isdigit(static_cast<unsigned char>(rest[e])) != 0) { ++e; }
            if (e > 0 && rest.compare(e, 11, " on the CPU") == 0) { b.cpuLm = std::stoll(rest.substr(0, e)); }
        }
    }
    return true;
}

}  // namespace bhed::karte
