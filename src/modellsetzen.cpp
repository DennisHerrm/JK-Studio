// modellsetzen.cpp - rc609 M0: ein vorhandenes Modell in die Karte (siehe modellsetzen.h)
//
//   Copyright (C) 1999-2005 Id Software, Inc.; 2000-2013 Raven Software; 2013-2026 OpenJK contributors.
//   This program is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free Software
//   Foundation; either version 2 of the License, or (at your option) any later
//   version (LICENSE).
#include "bhed/modellsetzen.h"

#include "bhed/pk3.h"
#include "bhed/regelwerk.h"
#include "bhed/utf8pfad.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <sstream>

namespace bhed::modellsetzen {

namespace {

namespace fs = std::filesystem;

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

// Eine Zahl wie die Editoren sie schreiben: ohne unnoetige Nullen.
std::string zahl(float v) {
    if (std::fabs(v) < 5e-5F) { v = 0.0F; }
    char t[48];
    std::snprintf(t, sizeof(t), "%.4f", static_cast<double>(v));
    std::string s = t;
    while (!s.empty() && s.back() == '0') { s.pop_back(); }
    if (!s.empty() && s.back() == '.') { s.pop_back(); }
    return s == "-0" ? std::string("0") : s;
}

const std::string* wert(const Schluessel& keys, const char* name) {
    const std::string* w = nullptr;
    for (const auto& [k, v] : keys) {
        if (klein(k) == name) { w = &v; }   // ein doppelter Schluessel: der letzte (wie ParseEntity)
    }
    return w;
}

bool endetMit(const std::string& s, const std::string& e) { return s.size() >= e.size() && s.compare(s.size() - e.size(), e.size(), e) == 0; }

}  // namespace

// --- Klassen -------------------------------------------------------------------------------------------

std::vector<KlassenAngebot> modellKlassen(int spiel, bool mitQuelle) {
    std::vector<KlassenAngebot> v;
    KlassenAngebot mm;
    mm.klasse = "misc_model";
    mm.quelle = "q3map2 model.cpp:1509-1581 AddTriangleModels (eingebacken, Lightmaps); im Spiel G_FreeEntity "
                "(code/game/g_misc.cpp:383, codeJK2/game/g_misc.cpp:362)";
    mm.da = mitQuelle;
    if (!mitQuelle) {
        mm.grund = "misc_model wird nur beim Kompilieren eingebacken (q3map2 AddTriangleModels); eine fertige Karte ohne .map "
                   "bekommt es nicht - das Spiel gibt die Entity frei (SP_misc_model: G_FreeEntity)";
    }
    v.push_back(mm);
    KlassenAngebot st;
    st.klasse = "misc_model_static";
    if (spiel == kRegelwerkJediOutcast) {
        st.da = false;
        st.quelle = "codeJK2/game/g_spawn.cpp:574";
        st.grund = "Jedi Outcast kennt kein misc_model_static (codeJK2/game/g_spawn.cpp: nur misc_model in der Spawntabelle)";
    } else if (spiel == kRegelwerkMovieDuels) {
        st.da = true;
        st.quelle = "MD9 code/game/g_misc.cpp:409-450, code/cgame/cg_main.cpp:3212-3240 (nur .md3); MP codemp/cgame/cg_spawn.c";
    } else {
        st.da = true;
        st.quelle = "code/game/g_misc.cpp:401-426, code/cgame/cg_main.cpp:1962-1990 (nur .md3); MP codemp/cgame/cg_spawn.c:170-235";
    }
    v.push_back(st);
    return v;
}

std::string vorgabeKlasse(int spiel, bool mitQuelle) {
    const std::vector<KlassenAngebot> k = modellKlassen(spiel, mitQuelle);
    for (const KlassenAngebot& a : k) {
        if (a.da && (mitQuelle ? a.klasse == "misc_model" : a.klasse == "misc_model_static")) { return a.klasse; }
    }
    for (const KlassenAngebot& a : k) {
        if (a.da) { return a.klasse; }
    }
    return std::string();
}

// --- Lage ----------------------------------------------------------------------------------------------

Schluessel lageSchluessel(const Lage& l) {
    Schluessel s;
    const auto norm = [](float w) {
        float r = std::fmod(w, 360.0F);
        if (r < 0.0F) { r += 360.0F; }
        return r >= 359.99995F ? 0.0F : r;
    };
    const float p = norm(l.winkel[0]);
    const float y = norm(l.winkel[1]);
    const float r = norm(l.winkel[2]);
    if (p == 0.0F && r == 0.0F) {
        s.emplace_back("angles", "");
        s.emplace_back("angle", y == 0.0F ? std::string() : zahl(y));
    } else {
        s.emplace_back("angle", "");
        s.emplace_back("angles", zahl(p) + ' ' + zahl(y) + ' ' + zahl(r));
    }
    const float* k = l.skala;
    if (k[0] == 1.0F && k[1] == 1.0F && k[2] == 1.0F) {
        s.emplace_back("modelscale", "");
        s.emplace_back("modelscale_vec", "");
    } else if (k[0] == k[1] && k[0] == k[2]) {
        s.emplace_back("modelscale_vec", "");
        s.emplace_back("modelscale", zahl(k[0]));
    } else {
        s.emplace_back("modelscale", "");
        s.emplace_back("modelscale_vec", zahl(k[0]) + ' ' + zahl(k[1]) + ' ' + zahl(k[2]));
    }
    return s;
}

Lage lageWieSpiel(const Schluessel& keys, bool movieDuels) {
    Lage l;
    for (const auto& [k, v] : keys) {   // G_SpawnGEntityFromSpawnVars: G_ParseField je Paar in Reihenfolge
        const std::string n = klein(k);
        if (n == "angle") {               // F_ANGLEHACK
            l.winkel[0] = 0.0F;
            l.winkel[1] = static_cast<float>(std::atof(v.c_str()));
            l.winkel[2] = 0.0F;
        } else if (n == "angles") {       // F_VECTOR (sscanf, fehlende bleiben 0)
            float a[3] = {0.0F, 0.0F, 0.0F};
            (void)std::sscanf(v.c_str(), "%f %f %f", &a[0], &a[1], &a[2]);
            for (int i = 0; i < 3; ++i) { l.winkel[i] = a[i]; }
        }
    }
    // G_SpawnString( "modelscale_vec", "1 1 1" ) - der erste Treffer in spawnVars (G_SpawnString sucht von vorn)
    std::string vec = "1 1 1";
    for (const auto& [k, v] : keys) {
        if (klein(k) == "modelscale_vec") {
            vec = v;
            break;
        }
    }
    float s[3] = {1.0F, 1.0F, 1.0F};
    const int n = std::sscanf(vec.c_str(), "%f %f %f", &s[0], &s[1], &s[2]);
    if (movieDuels && n != 3) { s[0] = s[1] = s[2] = 1.0F; }   // MD9 g_misc.cpp:420-425
    std::string einheit = "0";
    for (const auto& [k, v] : keys) {
        if (klein(k) == "modelscale") {
            einheit = v;
            break;
        }
    }
    const float t = static_cast<float>(std::atof(einheit.c_str()));
    if (t != 0.0F) { s[0] = s[1] = s[2] = t; }
    for (int i = 0; i < 3; ++i) { l.skala[i] = s[i]; }
    return l;
}

Lage lageWieQ3map2(const Schluessel& keys) {
    Lage l;
    if (const std::string* v = wert(keys, "modelscale_vec")) {
        float s[3] = {1.0F, 1.0F, 1.0F};
        (void)std::sscanf(v->c_str(), "%f %f %f", &s[0], &s[1], &s[2]);
        for (int i = 0; i < 3; ++i) { l.skala[i] = s[i]; }
    } else if (const std::string* e = wert(keys, "modelscale")) {
        const float t = static_cast<float>(std::atof(e->c_str()));
        l.skala[0] = l.skala[1] = l.skala[2] = t;
    }
    if (const std::string* a = wert(keys, "angles")) {
        float w[3] = {0.0F, 0.0F, 0.0F};
        (void)std::sscanf(a->c_str(), "%f %f %f", &w[0], &w[1], &w[2]);
        for (int i = 0; i < 3; ++i) { l.winkel[i] = w[i]; }
    } else if (const std::string* g = wert(keys, "angle")) {
        l.winkel[1] = static_cast<float>(std::atof(g->c_str()));
    }
    return l;
}

void achsen(const Lage& l, float achse[3][3]) {
    // AnglesToAxis (q_math.c): AngleVectors( angles, axis[0], right, axis[2] ); axis[1] = -right
    constexpr float kGrad = 3.14159265358979323846F / 180.0F;
    const float sy = std::sin(l.winkel[1] * kGrad);
    const float cy = std::cos(l.winkel[1] * kGrad);
    const float sp = std::sin(l.winkel[0] * kGrad);
    const float cp = std::cos(l.winkel[0] * kGrad);
    const float sr = std::sin(l.winkel[2] * kGrad);
    const float cr = std::cos(l.winkel[2] * kGrad);
    achse[0][0] = cp * cy;
    achse[0][1] = cp * sy;
    achse[0][2] = -sp;
    const float rechts[3] = {-1.0F * sr * sp * cy + -1.0F * cr * -sy, -1.0F * sr * sp * sy + -1.0F * cr * cy, -1.0F * sr * cp};
    for (int k = 0; k < 3; ++k) { achse[1][k] = -rechts[k]; }
    achse[2][0] = cr * sp * cy + -sr * -sy;
    achse[2][1] = cr * sp * sy + -sr * cy;
    achse[2][2] = cr * cp;
    // ScaleModelAxis (cg_main.cpp/tr_main): jede Achse mit modelScale[i]
    for (int i = 0; i < 3; ++i) {
        for (int k = 0; k < 3; ++k) { achse[i][k] *= l.skala[i]; }
    }
}

void weltKasten(const float mins[3], const float maxs[3], const Lage& l, const float ort[3], float aus0[3], float aus1[3]) {
    float a[3][3];
    achsen(l, a);
    for (int k = 0; k < 3; ++k) {
        aus0[k] = 1.0e30F;
        aus1[k] = -1.0e30F;
    }
    for (int ecke = 0; ecke < 8; ++ecke) {
        const float p[3] = {(ecke & 1) != 0 ? maxs[0] : mins[0], (ecke & 2) != 0 ? maxs[1] : mins[1], (ecke & 4) != 0 ? maxs[2] : mins[2]};
        for (int k = 0; k < 3; ++k) {
            // Weltpunkt = ort + p[0] * axis[0] + p[1] * axis[1] + p[2] * axis[2] (RT_MODEL: Modellraum in die Achsen)
            const float w = ort[k] + p[0] * a[0][k] + p[1] * a[1][k] + p[2] * a[2][k];
            aus0[k] = std::min(aus0[k], w);
            aus1[k] = std::max(aus1[k], w);
        }
    }
}

float bodenHoehe(float endZ, float normaleZ) {
    constexpr float kSurfaceClipEpsilon = 0.125F;   // SURFACE_CLIP_EPSILON
    return normaleZ > 0.0F ? endZ - kSurfaceClipEpsilon / normaleZ : endZ;
}

float ursprungZAufBoden(float bodenZ, const float mins[3], const float maxs[3], const Lage& l) {
    const float null[3] = {0.0F, 0.0F, 0.0F};
    float a0[3];
    float a1[3];
    weltKasten(mins, maxs, l, null, a0, a1);
    return bodenZ - a0[2];
}

// --- Browser -------------------------------------------------------------------------------------------

std::vector<ModellDatei> modellDateien(const std::vector<GamePath>& ordner, const std::vector<std::string>& endungen, bool dirVorPak) {
    std::map<std::string, ModellDatei> gefunden;   // klein -> erste Fassung in der Suchreihenfolge
    const auto passt = [&endungen](const std::string& n) {
        const std::string k = klein(n);
        if (k.rfind("models/", 0) != 0) { return false; }
        for (const std::string& e : endungen) {
            if (endetMit(k, e)) { return true; }
        }
        return false;
    };
    const auto ausPk3 = [&](const GamePath& gp) {
        for (auto a = gp.archives.rbegin(); a != gp.archives.rend(); ++a) {
            for (const Pk3Entry& e : a->entries) {
                if (!passt(e.name)) { continue; }
                gefunden.emplace(klein(e.name), ModellDatei{e.name, a->path, false});
            }
        }
    };
    const auto ausOrdner = [&](const GamePath& gp) {
        if (gp.directory.empty()) { return; }
        std::error_code ec;
        const fs::path wurzel = utf8Pfad(gp.directory);
        const fs::path m = wurzel / "models";
        if (!fs::is_directory(m, ec)) { return; }
        for (fs::recursive_directory_iterator it(m, fs::directory_options::skip_permission_denied, ec), ende; it != ende && !ec; it.increment(ec)) {
            if (!it->is_regular_file(ec)) { continue; }
            const auto rel = fs::relative(it->path(), wurzel, ec).generic_u8string();
            std::string n(rel.begin(), rel.end());
            if (!passt(n)) { continue; }
            const auto voll = it->path().u8string();
            gefunden.emplace(klein(n), ModellDatei{n, std::string(voll.begin(), voll.end()), true});
        }
    };
    for (auto gp = ordner.rbegin(); gp != ordner.rend(); ++gp) {   // spaetere Spielordner zuerst (FS_AddGameDirectory)
        if (dirVorPak) {
            ausOrdner(*gp);
            ausPk3(*gp);
        } else {
            ausPk3(*gp);
            ausOrdner(*gp);
        }
    }
    std::vector<ModellDatei> v;
    v.reserve(gefunden.size());
    for (auto& [k, d] : gefunden) { v.push_back(std::move(d)); }
    return v;
}

std::vector<std::size_t> modellSuche(const std::vector<ModellDatei>& d, const std::string& suche) {
    std::vector<std::string> woerter;
    std::istringstream in(klein(suche));
    for (std::string w; in >> w;) { woerter.push_back(w); }
    std::vector<std::size_t> r;
    for (std::size_t i = 0; i < d.size(); ++i) {
        const std::string p = klein(d[i].pfad);
        bool alle = true;
        for (const std::string& w : woerter) { alle = alle && p.find(w) != std::string::npos; }
        if (alle) { r.push_back(i); }
    }
    return r;
}

// --- Live ----------------------------------------------------------------------------------------------

DateiStand dateiStand(const std::string& vollerPfad) {
    DateiStand s;
    std::error_code ec;
    const fs::path p = utf8Pfad(vollerPfad);
    if (vollerPfad.empty() || !fs::is_regular_file(p, ec)) { return s; }
    const auto t = fs::last_write_time(p, ec);
    if (ec) { return s; }
    s.groesse = static_cast<std::uint64_t>(fs::file_size(p, ec));
    s.zeit = static_cast<std::int64_t>(t.time_since_epoch().count());
    s.da = !ec;
    return s;
}

std::string fundort(const std::vector<GamePath>& ordner, const std::string& name, bool dirVorPak, bool* lose) {
    if (lose != nullptr) { *lose = false; }
    std::string n = name;
    if (!n.empty() && (n[0] == '/' || n[0] == '\\')) { n.erase(0, 1); }   // files.cpp:1276
    if (n.empty() || n.find("..") != std::string::npos || n.find("::") != std::string::npos) { return std::string(); }
    const auto ausPk3 = [&n](const GamePath& gp) -> std::string {
        for (auto a = gp.archives.rbegin(); a != gp.archives.rend(); ++a) {
            if (a->find(n) != nullptr) { return a->path; }
        }
        return std::string();
    };
    const auto ausOrdner = [&n](const GamePath& gp) -> std::string {
        if (gp.directory.empty()) { return std::string(); }
        std::error_code ec;
        const fs::path p = utf8Pfad(gp.directory) / utf8Pfad(n);
        if (!fs::is_regular_file(p, ec)) { return std::string(); }
        const auto u = p.u8string();
        return std::string(u.begin(), u.end());
    };
    for (auto gp = ordner.rbegin(); gp != ordner.rend(); ++gp) {
        std::string a = dirVorPak ? ausOrdner(*gp) : ausPk3(*gp);
        bool istLose = dirVorPak;
        if (a.empty()) {
            a = dirVorPak ? ausPk3(*gp) : ausOrdner(*gp);
            istLose = !dirVorPak;
        }
        if (!a.empty()) {
            if (lose != nullptr) { *lose = istLose; }
            return a;
        }
    }
    return std::string();
}

}  // namespace bhed::modellsetzen
