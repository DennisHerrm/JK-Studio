// kartelichtschaetzung.cpp - Karten-Editor Stufe 8a Punkt 4: Lichter einer .bsp
// schaetzen (bhed/karte/lichtschaetzung.h, experimentell).
//
// ctest karte_lichtschaetzung: tests/karte/q3map2/raum_licht.bsp (raum.map mit
// bhed_q3map2 -meta, -vis, -light -fast gebaut) - die zwei Lichter (light 400 bei
// -256 -160 192, light 300 bei 256 160 192) werden gefunden, nahe am Ort und in der
// Staerke; jedes geschaetzte Licht traegt _bhed_geschaetzt 1.
//
// Messung (Notizen): kartelichtschaetzung <karte.bsp> <quelle.map> - Lagefehler und
// Helligkeit gegen die Lichter der Quelle, Trefferquote, Fehlschaetzungen; mit
// <aus.map> dazu die geschaetzten Lichter als .map-Anhang zum Neubacken.
#include "bhed/karte/lichtschaetzung.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace bhed::karte;

namespace {

int g_fehler = 0;
int g_pruefungen = 0;
void pruefe(bool ok, const std::string& was) {
    ++g_pruefungen;
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}

std::string lies(const std::string& p) {
    std::ifstream f(p, std::ios::binary);
    std::ostringstream s;
    s << f.rdbuf();
    return s.str();
}

struct QLicht {
    double p[3]{0, 0, 0};
    double licht = 300;
    bool spot = false;
};

std::vector<QLicht> quellLichter(const std::string& t) {
    std::vector<QLicht> out;
    std::istringstream in(t);
    std::string z;
    int tiefe = 0;
    std::vector<std::pair<std::string, std::string>> kv;
    while (std::getline(in, z)) {
        if (!z.empty() && z.back() == '\r') { z.pop_back(); }
        const std::size_t a = z.find_first_not_of(" \t");
        if (a == std::string::npos) { continue; }
        z = z.substr(a);
        if (z == "{") {
            if (++tiefe == 1) { kv.clear(); }
        } else if (z == "}") {
            if (--tiefe == 0) {
                std::string klasse;
                QLicht l;
                bool ort = false;
                for (const auto& [k, v] : kv) {
                    if (k == "classname") { klasse = v; }
                    if (k == "origin") { ort = std::sscanf(v.c_str(), "%lf %lf %lf", &l.p[0], &l.p[1], &l.p[2]) == 3; }
                    if (k == "light" || k == "_light") { l.licht = std::atof(v.c_str()); }
                    if (k == "target") { l.spot = true; }
                }
                if (klasse.rfind("light", 0) == 0 && ort) { out.push_back(l); }
            }
        } else if (tiefe == 1 && z[0] == '"') {
            const std::size_t e1 = z.find('"', 1);
            const std::size_t a2 = z.find('"', e1 + 1);
            const std::size_t e2 = z.rfind('"');
            if (e1 != std::string::npos && a2 != std::string::npos && e2 > a2) { kv.emplace_back(z.substr(1, e1 - 1), z.substr(a2 + 1, e2 - a2 - 1)); }
        }
    }
    return out;
}

double dist(const double* a, const double* b) {
    return std::sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2]));
}

struct Bewertung {
    int treffer = 0;
    std::vector<double> lage;
    std::vector<double> verhaeltnis;   // geschaetzt / Quelle
};

Bewertung bewerten(const std::vector<GeschaetztesLicht>& g, const std::vector<QLicht>& q, double radius) {
    Bewertung b;
    std::vector<bool> belegt(q.size(), false);
    // je geschaetztes Licht (staerkste zuerst) das naechste freie Quelllicht
    for (const GeschaetztesLicht& l : g) {
        int best = -1;
        double bd = radius;
        for (std::size_t i = 0; i < q.size(); ++i) {
            if (belegt[i]) { continue; }
            const double d = dist(l.origin, q[i].p);
            if (d < bd) {
                bd = d;
                best = static_cast<int>(i);
            }
        }
        if (best < 0) { continue; }
        belegt[static_cast<std::size_t>(best)] = true;
        ++b.treffer;
        b.lage.push_back(bd);
        b.verhaeltnis.push_back(l.licht / std::max(1.0, q[static_cast<std::size_t>(best)].licht));
    }
    return b;
}

double median(std::vector<double> v) {
    if (v.empty()) { return 0; }
    std::sort(v.begin(), v.end());
    return v[v.size() / 2];
}

}  // namespace

int main(int argc, char** argv) {
    if (argc == 2) {
        const std::string bsp = lies(std::string(argv[1]) + "/raum_licht.bsp");
        const std::vector<GeschaetztesLicht> g = lichterSchaetzen(bsp, LichtSchaetzOptionen{});
        const std::vector<QLicht> q = quellLichter(lies(std::string(argv[1]) + "/raum.map"));
        const Bewertung b = bewerten(g, q, 64.0);
        std::printf("INFO  raum_licht.bsp: %zu geschaetzt, %d von %zu Quelllichtern getroffen, Lage %.1f / %.1f, Staerke x%.2f / x%.2f\n", g.size(), b.treffer,
                    q.size(), b.lage.empty() ? 0.0 : b.lage[0], b.lage.size() > 1 ? b.lage[1] : 0.0, b.verhaeltnis.empty() ? 0.0 : b.verhaeltnis[0],
                    b.verhaeltnis.size() > 1 ? b.verhaeltnis[1] : 0.0);
        pruefe(q.size() == 2 && b.treffer == 2, "beide Lichter von raum.map gefunden (Radius 64)");
        bool nah = b.lage.size() == 2;
        for (const double d : b.lage) { nah = nah && d < 32.0; }
        pruefe(nah, "Lage unter 32 Einheiten");
        bool hell = b.verhaeltnis.size() == 2;
        for (const double v : b.verhaeltnis) { hell = hell && v > 0.5 && v < 2.0; }
        pruefe(hell, "Staerke zwischen halb und doppelt");
        bool markiert = !g.empty();
        for (const GeschaetztesLicht& l : g) {
            const auto kv = lichtSchluessel(l);
            markiert = markiert && std::find(kv.begin(), kv.end(), std::make_pair(std::string("_bhed_geschaetzt"), std::string("1"))) != kv.end() &&
                       kv.front() == std::make_pair(std::string("classname"), std::string("light"));
        }
        pruefe(markiert, "jedes geschaetzte Licht: classname light, _bhed_geschaetzt 1");
        pruefe(g.size() <= 6, "hoechstens 6 Lichter in einem Raum mit 2 (" + std::to_string(g.size()) + ")");
        std::printf("kartelichtschaetzung: %d Pruefungen, %d Fehlschlaege\n", g_pruefungen, g_fehler);
        return g_fehler == 0 ? 0 : 1;
    }
    if (argc < 3) {
        std::printf("Aufruf: kartelichtschaetzung <tests/karte/q3map2> | <karte.bsp> <quelle.map> [<aus.txt>]\n");
        return 2;
    }
    const std::string bsp = lies(argv[1]);
    const std::vector<GeschaetztesLicht> g = lichterSchaetzen(bsp, LichtSchaetzOptionen{});
    const std::vector<QLicht> q = quellLichter(lies(argv[2]));
    int spots = 0;
    for (const QLicht& l : q) { spots += l.spot ? 1 : 0; }
    int ziele = 0;
    for (const GeschaetztesLicht& l : g) { ziele += l.ziel.empty() ? 0 : 1; }
    std::printf("geschaetzt: %zu Lichter (%d mit target); Quelle: %zu Lichter (%d Spotlichter)\n", g.size(), ziele, q.size(), spots);
    for (const double r : {32.0, 64.0, 128.0, 256.0}) {
        const Bewertung b = bewerten(g, q, r);
        std::vector<double> v = b.verhaeltnis;
        std::sort(v.begin(), v.end());
        const double p10 = v.empty() ? 0 : v[v.size() / 10];
        const double p90 = v.empty() ? 0 : v[v.size() * 9 / 10];
        std::printf("Radius %4.0f: %3d getroffen (%.0f %% der Quelle, %.0f %% der Schaetzung), Lage Median %.1f, Staerke Median x%.2f (10..90 %%: x%.2f..x%.2f)\n", r,
                    b.treffer, 100.0 * b.treffer / std::max<std::size_t>(1, q.size()), 100.0 * b.treffer / std::max<std::size_t>(1, g.size()), median(b.lage),
                    median(b.verhaeltnis), p10, p90);
    }
    if (argc >= 4) {
        std::ofstream f(argv[3], std::ios::binary);
        for (const GeschaetztesLicht& l : g) {
            f << "{\n";
            for (const auto& [k, v] : lichtSchluessel(l)) { f << "\"" << k << "\" \"" << v << "\"\n"; }
            f << "}\n";
        }
    }
    return 0;
}
