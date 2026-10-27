// logik.cpp - Karten-Editor, Stufe 8b Punkt 4: die Logik-Ansicht (siehe
// logik.h). Portiert aus OpenJK (die Schluessellisten aus code/game/g_spawn.cpp
// fields[]); der Graph und seine Lage sind eigener Code.
//
//   Copyright (C) 1999 - 2005, Id Software, Inc.
//   Copyright (C) 2000 - 2013, Raven Software, Inc.
//   Copyright (C) 2013 - 2015, OpenJK contributors
//   This program is free software; you can redistribute it and/or modify it
//   under the terms of the GNU General Public License version 2 as published
//   by the Free Software Foundation (LICENSE).
#include "bhed/karte/logik.h"

#include "bhed/karte/eklasse.h"
#include "bhed/karte/karte.h"

#include <algorithm>
#include <cctype>
#include <functional>
#include <map>
#include <set>

namespace bhed::karte {

namespace {

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

bool istNamensSchluessel(const std::string& kl) {
    for (const std::string& n : logikNamensSchluessel()) {
        if (klein(n) == kl) { return true; }
    }
    return false;
}

}  // namespace

const std::vector<std::string>& logikSkriptSchluessel() {
    // g_spawn.cpp:312-327 (behaviorSet[BSET_*])
    static const std::vector<std::string> v = {"spawnscript",  "usescript",   "awakescript",     "angerscript",  "attackscript",
                                               "victoryscript", "lostenemyscript", "painscript", "fleescript",   "deathscript",
                                               "delayscript",  "blockedscript", "ffirescript",   "ffdeathscript", "mindtrickscript"};
    return v;
}

const std::vector<std::string>& logikNamensSchluessel() {
    static const std::vector<std::string> v = {"targetname", "NPC_targetname", "script_targetname"};
    return v;
}

std::vector<std::string> logikVerknuepfungen(const EntitaetsKlassen* klassen) {
    // g_spawn.cpp:245-249, 304-308, 333-335
    std::vector<std::string> v = {"target",     "target2",    "target3",   "target4",         "targetJump", "closetarget",
                                  "opentarget", "paintarget", "backwardstarget", "NPC_target", "NPC_target2", "NPC_target4"};
    std::set<std::string> da;
    for (const std::string& s : v) { da.insert(klein(s)); }
    const auto dazu = [&v, &da](const std::string& schluessel, bool typZiel) {
        const std::string kl = klein(schluessel);
        if ((!typZiel && kl.find("target") == std::string::npos) || istNamensSchluessel(kl) || da.count(kl) != 0U || kl.empty()) { return; }
        da.insert(kl);
        v.push_back(schluessel);
    };
    if (klassen != nullptr) {
        for (const EntitaetsKlasse* c : klassen->alle()) {
            // .ent (XML): Attribute vom Typ "target" bzw. mit "target" im Namen
            for (const auto& [schluessel, attr] : c->attribute) { dazu(schluessel, attr.typ == "target"); }
            // .def (QUAKED): die Schluessel stehen im Kommentar - eine Zeile, die mit
            // "schluessel" beginnt ("target" ..., "delay" - ...)
            std::size_t pos = 0;
            const std::string& k = c->kommentar;
            while (pos < k.size()) {
                std::size_t ende = k.find('\n', pos);
                if (ende == std::string::npos) { ende = k.size(); }
                const std::size_t a = k.find_first_not_of(" \t\r", pos);
                if (a < ende && k[a] == '"') {
                    const std::size_t b = k.find('"', a + 1);
                    if (b != std::string::npos && b < ende) { dazu(k.substr(a + 1, b - a - 1), false); }
                }
                pos = ende + 1;
            }
        }
    }
    return v;
}

LogikGraph logikGraph(const Karte& k, const EntitaetsKlassen* klassen) {
    LogikGraph g;
    const std::vector<std::string> verkn = logikVerknuepfungen(klassen);
    std::set<std::string> verknKlein;
    for (const std::string& s : verkn) { verknKlein.insert(klein(s)); }
    // die Knoten: Entities mit Namen, Verknuepfung oder Skript
    std::map<std::string, std::vector<int>> nachName;   // Name (klein) -> Knoten
    struct Roh {
        int knoten;
        std::string schluessel, wert;
    };
    std::vector<Roh> roh;
    std::set<std::string> benutzt;
    for (const auto& ent : k.entitaeten()) {
        if (ent->klassenname() == "worldspawn") { continue; }
        LogikKnoten n;
        n.entitaet = ent;
        n.klasse = ent->klassenname();
        std::vector<std::pair<std::string, std::string>> raus;
        for (const auto& [schluessel, wert] : ent->schluessel()) {
            if (wert.empty()) { continue; }
            const std::string kl = klein(schluessel);
            if (istNamensSchluessel(kl)) {
                n.namen.emplace_back(schluessel, wert);
                if (n.name.empty()) { n.name = wert; }
            } else if (verknKlein.count(kl) != 0U || (kl.find("target") != std::string::npos && kl != "targetname")) {
                raus.emplace_back(schluessel, wert);
            } else {
                for (const std::string& s : logikSkriptSchluessel()) {
                    if (kl == s) { n.skripte.emplace_back(schluessel, wert); }
                }
            }
        }
        if (n.namen.empty() && raus.empty() && n.skripte.empty()) { continue; }
        const int nr = static_cast<int>(g.knoten.size());
        for (const auto& [s, w] : n.namen) { nachName[klein(w)].push_back(nr); }
        for (const auto& [s, w] : raus) {
            roh.push_back({nr, s, w});
            benutzt.insert(s);
        }
        g.knoten.push_back(std::move(n));
    }
    for (const std::string& s : verkn) {
        if (benutzt.count(s) != 0U) { g.verknuepfungen.push_back(s); }
    }
    // die Kanten; fehlende Ziele als eigene Knoten "? name"
    std::map<std::string, int> fehlKnoten;
    for (const Roh& r : roh) {
        const auto it = nachName.find(klein(r.wert));
        if (it != nachName.end()) {
            for (const int z : it->second) { g.kanten.push_back({r.knoten, z, r.schluessel, r.wert}); }
            continue;
        }
        ++g.fehlend;
        auto f = fehlKnoten.find(klein(r.wert));
        if (f == fehlKnoten.end()) {
            LogikKnoten n;
            n.klasse = "?";
            n.name = r.wert;
            f = fehlKnoten.emplace(klein(r.wert), static_cast<int>(g.knoten.size())).first;
            g.knoten.push_back(std::move(n));
        }
        g.kanten.push_back({r.knoten, f->second, r.schluessel, r.wert});
    }
    // Teile (ungerichtet), dann Spalten: laengster Weg ohne Rueckkanten (DFS)
    const int n = static_cast<int>(g.knoten.size());
    std::vector<std::vector<int>> aus(static_cast<std::size_t>(n));
    std::vector<std::vector<int>> nachbarn(static_cast<std::size_t>(n));
    for (const LogikKante& e : g.kanten) {
        aus[static_cast<std::size_t>(e.von)].push_back(e.zu);
        nachbarn[static_cast<std::size_t>(e.von)].push_back(e.zu);
        nachbarn[static_cast<std::size_t>(e.zu)].push_back(e.von);
    }
    std::vector<int> gruppe(static_cast<std::size_t>(n), -1);
    int gruppen = 0;
    for (int i = 0; i < n; ++i) {
        if (gruppe[static_cast<std::size_t>(i)] >= 0) { continue; }
        std::vector<int> stapel{i};
        gruppe[static_cast<std::size_t>(i)] = gruppen;
        while (!stapel.empty()) {
            const int a = stapel.back();
            stapel.pop_back();
            for (const int b : nachbarn[static_cast<std::size_t>(a)]) {
                if (gruppe[static_cast<std::size_t>(b)] < 0) {
                    gruppe[static_cast<std::size_t>(b)] = gruppen;
                    stapel.push_back(b);
                }
            }
        }
        ++gruppen;
    }
    // Rueckkanten: DFS-Farben
    std::vector<int> farbe(static_cast<std::size_t>(n), 0);
    std::set<std::pair<int, int>> rueck;
    const std::function<void(int)> dfs = [&](int a) {
        farbe[static_cast<std::size_t>(a)] = 1;
        for (const int b : aus[static_cast<std::size_t>(a)]) {
            if (farbe[static_cast<std::size_t>(b)] == 1) {
                rueck.insert({a, b});
            } else if (farbe[static_cast<std::size_t>(b)] == 0) {
                dfs(b);
            }
        }
        farbe[static_cast<std::size_t>(a)] = 2;
    };
    for (int i = 0; i < n; ++i) {
        if (farbe[static_cast<std::size_t>(i)] == 0) { dfs(i); }
    }
    // laengster Weg: so oft entspannen, wie es Knoten gibt (DAG ohne Rueckkanten)
    std::vector<int> ebene(static_cast<std::size_t>(n), 0);
    for (int runde = 0; runde < n; ++runde) {
        bool geaendert = false;
        for (const LogikKante& e : g.kanten) {
            if (rueck.count({e.von, e.zu}) != 0U || e.von == e.zu) { continue; }
            const int soll = ebene[static_cast<std::size_t>(e.von)] + 1;
            if (ebene[static_cast<std::size_t>(e.zu)] < soll) {
                ebene[static_cast<std::size_t>(e.zu)] = soll;
                geaendert = true;
            }
        }
        if (!geaendert) { break; }
    }
    // Lage: je Teil die Spalten nebeneinander, die Teile untereinander
    constexpr float kSpalte = 240.0F;
    constexpr float kAbstand = 14.0F;
    float y0 = 0.0F;
    for (int t = 0; t < gruppen; ++t) {
        std::map<int, std::vector<int>> spalten;
        for (int i = 0; i < n; ++i) {
            if (gruppe[static_cast<std::size_t>(i)] == t) { spalten[ebene[static_cast<std::size_t>(i)]].push_back(i); }
        }
        float hoechste = 0.0F;
        for (auto& [sp, liste] : spalten) {
            std::sort(liste.begin(), liste.end(), [&g](int a, int b) {
                const LogikKnoten& x = g.knoten[static_cast<std::size_t>(a)];
                const LogikKnoten& y = g.knoten[static_cast<std::size_t>(b)];
                if (x.name != y.name) { return x.name < y.name; }
                return a < b;
            });
            float y = y0;
            int zeile = 0;
            for (const int i : liste) {
                LogikKnoten& kn = g.knoten[static_cast<std::size_t>(i)];
                kn.gruppe = t;
                kn.ebene = sp;
                kn.zeile = zeile++;
                kn.breite = 200.0F;
                kn.hoehe = 18.0F * static_cast<float>(1 + (kn.name.empty() ? 0 : 1) + kn.skripte.size());
                kn.x = static_cast<float>(sp) * kSpalte;
                kn.y = y;
                y += kn.hoehe + kAbstand;
            }
            hoechste = std::max(hoechste, y - y0);
        }
        y0 += hoechste + 2.0F * kAbstand;
    }
    return g;
}

}  // namespace bhed::karte
