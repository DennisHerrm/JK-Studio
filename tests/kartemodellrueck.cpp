// kartemodellrueck.cpp - Karten-Editor Stufe 8a Punkt 3: misc_model aus der
// Geometrie einer .bsp zurueckgewinnen (bhed/karte/modellrueck.h).
//
// Ohne Argumente (ctest karte_modellrueck): die Lage-Rechnung - Schluessel ->
// Matrix -> Zerlegung -> Schluessel fuer Gier, Nicken, Rollen, gleiche und
// ungleiche, negative Skalierung (Spiegelung wie "-500 500 500"); die Auswahl der
// Kandidaten; der Dump-Leser.
//
// Messung (Notizen): kartemodellrueck <karte.bsp> <modelle.bhmd> [<quelle.map>]
// zaehlt die Treffer und vergleicht sie mit den misc_model der Quelle: Lage-,
// Dreh- und Skalierungsfehler (aus M = A * A_quelle^-1), Fehlzuordnungen.
#include "bhed/karte/modellrueck.h"
#include "bhed/karte/pinsel.h"

#include <algorithm>
#include <chrono>
#include <set>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
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

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    std::replace(s.begin(), s.end(), '\\', '/');
    return s;
}

// Abweichung zweier Lagen: Ort, Drehwinkel von M = A*B^-1 (Grad), Skalierung (Singulaerwerte von M)
struct Abweichung {
    double ort = 0;
    double winkel = 0;
    double skala = 0;
};
Abweichung abweichung(const ModellLage& a, const ModellLage& b) {
    double A[3][3];
    double B[3][3];
    lageMatrix(a, A);
    lageMatrix(b, B);
    // B^-1
    const double d = B[0][0] * (B[1][1] * B[2][2] - B[1][2] * B[2][1]) - B[0][1] * (B[1][0] * B[2][2] - B[1][2] * B[2][0]) +
                     B[0][2] * (B[1][0] * B[2][1] - B[1][1] * B[2][0]);
    double I[3][3];
    I[0][0] = (B[1][1] * B[2][2] - B[1][2] * B[2][1]) / d;
    I[0][1] = (B[0][2] * B[2][1] - B[0][1] * B[2][2]) / d;
    I[0][2] = (B[0][1] * B[1][2] - B[0][2] * B[1][1]) / d;
    I[1][0] = (B[1][2] * B[2][0] - B[1][0] * B[2][2]) / d;
    I[1][1] = (B[0][0] * B[2][2] - B[0][2] * B[2][0]) / d;
    I[1][2] = (B[0][2] * B[1][0] - B[0][0] * B[1][2]) / d;
    I[2][0] = (B[1][0] * B[2][1] - B[1][1] * B[2][0]) / d;
    I[2][1] = (B[0][1] * B[2][0] - B[0][0] * B[2][1]) / d;
    I[2][2] = (B[0][0] * B[1][1] - B[0][1] * B[1][0]) / d;
    double M[3][3];
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) { M[r][c] = A[r][0] * I[0][c] + A[r][1] * I[1][c] + A[r][2] * I[2][c]; }
    }
    Abweichung w;
    for (int k = 0; k < 3; ++k) { w.ort = std::max(w.ort, std::fabs(a.origin[k] - b.origin[k])); }
    // Skalierung: Spaltenlaengen von M, Drehung: Spur der normierten M
    double tr = 0;
    for (int c = 0; c < 3; ++c) {
        const double l = std::sqrt(M[0][c] * M[0][c] + M[1][c] * M[1][c] + M[2][c] * M[2][c]);
        w.skala = std::max(w.skala, std::fabs(l - 1.0));
        tr += M[c][c] / std::max(l, 1e-12);
    }
    w.winkel = std::acos(std::clamp((tr - 1.0) / 2.0, -1.0, 1.0)) * 57.29577951308232;
    return w;
}

std::vector<std::vector<std::pair<std::string, std::string>>> mapEntitaeten(const std::string& t) {
    std::vector<std::vector<std::pair<std::string, std::string>>> out;
    int tiefe = 0;
    std::vector<std::pair<std::string, std::string>> cur;
    std::istringstream in(t);
    std::string z;
    while (std::getline(in, z)) {
        if (!z.empty() && z.back() == '\r') { z.pop_back(); }
        std::size_t a = z.find_first_not_of(" \t");
        if (a == std::string::npos) { continue; }
        z = z.substr(a);
        if (z == "{") {
            if (++tiefe == 1) { cur.clear(); }
        } else if (z == "}") {
            if (--tiefe == 0) { out.push_back(cur); }
        } else if (tiefe == 1 && z[0] == '"') {
            const std::size_t e1 = z.find('"', 1);
            const std::size_t a2 = z.find('"', e1 + 1);
            const std::size_t e2 = z.rfind('"');
            if (e1 != std::string::npos && a2 != std::string::npos && e2 > a2) { cur.emplace_back(z.substr(1, e1 - 1), z.substr(a2 + 1, e2 - a2 - 1)); }
        }
    }
    return out;
}

std::string wert(const std::vector<std::pair<std::string, std::string>>& e, const char* k) {
    for (const auto& [a, b] : e) {
        if (klein(a) == k) { return b; }
    }
    return {};
}

void selbstPruefung() {
    const auto rund = [](std::vector<std::pair<std::string, std::string>> kv) {
        const ModellLage l = lageAusSchluesseln(kv);
        double m[3][3];
        lageMatrix(l, m);
        const ModellLage z = lageAusMatrix(m, l.origin);
        return std::make_pair(abweichung(l, z), lageSchluessel(z));
    };
    const auto text = [](const std::vector<std::pair<std::string, std::string>>& kv) {
        std::string s;
        for (const auto& [k, v] : kv) { s += k + "=" + v + " "; }
        return s;
    };
    {
        const auto [w, kv] = rund({{"origin", "10 20 30"}, {"angle", "45"}, {"modelscale", "1.5"}});
        pruefe(w.ort < 1e-9 && w.winkel < 1e-6 && w.skala < 1e-9 && text(kv) == "origin=10 20 30 angle=45 modelscale=1.5 ", "angle 45, modelscale 1.5: " + text(kv));
    }
    {
        const auto [w, kv] = rund({{"origin", "0 0 0"}, {"angles", "30 120 -15"}, {"modelscale_vec", "2 3 0.5"}});
        pruefe(w.winkel < 1e-6 && w.skala < 1e-9 && text(kv) == "origin=0 0 0 angles=30 120 -15 modelscale_vec=2 3 0.5 ", "angles 30 120 -15, modelscale_vec 2 3 0.5: " + text(kv));
    }
    {
        const auto [w, kv] = rund({{"origin", "-45297.8 1902.76 216.41"}, {"modelscale_vec", "-500 500 500"}});
        pruefe(w.winkel < 1e-6 && w.skala < 1e-9 && text(kv) == "origin=-45297.8 1902.76 216.41 modelscale_vec=-500 500 500 ",
               "Spiegelung -500 500 500 bleibt: " + text(kv));
    }
    {
        // -75 -75 75 mit angle -90 ist dieselbe Abbildung wie angle 90, modelscale 75
        const ModellLage a = lageAusSchluesseln({{"origin", "1 2 3"}, {"angle", "-90"}, {"modelscale_vec", "-75 -75 75"}});
        const ModellLage b = lageAusSchluesseln({{"origin", "1 2 3"}, {"angle", "90"}, {"modelscale", "75"}});
        const Abweichung w = abweichung(a, b);
        pruefe(w.winkel < 1e-6 && w.skala < 1e-9, "angle -90 mit -75 -75 75 = angle 90 mit modelscale 75 (dieselbe Matrix)");
    }
    {
        // Nicken 90: Gier und Rollen fallen zusammen - die Matrix bleibt dieselbe
        const auto [w, kv] = rund({{"origin", "0 0 0"}, {"angles", "90 30 0"}});
        pruefe(w.winkel < 1e-4, "angles 90 30 0 (senkrecht): dieselbe Matrix (" + text(kv) + ")");
    }
    {
        const std::vector<std::string> v = modellKandidaten(
            {"models/map_objects/a/b.md3", "models/map_objects/a/c.OBJ", "models/map_objects/x/y.md3", "models/map_objects/a/t.tga", "models/players/k/model.glm"},
            {"models/map_objects/a/b_skin", "textures/system/caulk"});
        pruefe(v.size() == 2 && v[0] == "models/map_objects/a/b.md3" && v[1] == "models/map_objects/a/c.OBJ", "Kandidaten: md3/obj aus den Ordnern der Shader");
    }
    {
        std::vector<ModellDatei> m;
        std::string d = std::string("BHMD") + std::string("\x01\0\0\0\x01\0\0\0", 8);
        const std::string name = "models/a.md3";
        const auto u32 = [&d](std::uint32_t x) { d.append(reinterpret_cast<const char*>(&x), 4); };
        u32(static_cast<std::uint32_t>(name.size()));
        d += name;
        u32(0);
        u32(1);
        u32(1);
        u32(3);
        d += "a/b";
        u32(3);
        u32(1);
        const float p[9] = {0, 0, 0, 1, 0, 0, 0, 1, 0};
        d.append(reinterpret_cast<const char*>(p), sizeof(p));
        u32(0);
        u32(1);
        u32(2);
        pruefe(modellDumpLesen(d, m) && m.size() == 1 && m[0].geladen && m[0].netze.size() == 1 && m[0].netze[0].shader == "a/b" &&
                   m[0].netze[0].punkte[1][0] == 1.0F && m[0].netze[0].dreiecke[0][2] == 2,
               "Dump: ein Modell, ein Netz mit drei Ecken");
        pruefe(!modellDumpLesen(d.substr(0, d.size() - 2), m), "Dump abgeschnitten: abgelehnt");
        // Stufe 8b Punkt 8: der Index
        pruefe(modellDumpLesen(d, m) && m.size() == 1, "Dump erneut gelesen (fuer den Index)");
        const ModellIndexEintrag ie = modellIndexEintrag(m[0], "pak0.pk3|123|456");
        std::vector<ModellIndexEintrag> idx{ie};
        ModellDatei andere;
        andere.pfad = "models/x/y.md3";
        andere.geladen = true;
        ModellNetz an;
        an.shader = "models/x/Haut.tga";
        an.punkte = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {1, 1, 0}};
        an.dreiecke = {{0, 1, 2}, {1, 3, 2}};
        andere.netze.push_back(an);
        idx.push_back(modellIndexEintrag(andere, "x"));
        ModellDatei kaputt;
        kaputt.pfad = "models/z/kaputt.md3";
        idx.push_back(modellIndexEintrag(kaputt, "z"));
        std::vector<ModellIndexEintrag> zurueck;
        pruefe(modellIndexLesen(modellIndexSchreiben(idx), zurueck) && zurueck.size() == 3 && zurueck[0].schluessel == "pak0.pk3|123|456" &&
                   zurueck[0].netze.size() == 1 && zurueck[0].netze[0].punkte == 3 && zurueck[0].netze[0].signatur == ie.netze[0].signatur &&
                   zurueck[1].netze[0].shader == "models/x/haut" && !zurueck[2].geladen,
               "Index: schreiben und lesen gibt dieselben Eintraege (Shader klein, ohne Endung)");
        pruefe(!modellIndexLesen("BHMI\t1\nM\tmodels/a.md3\tk\t1\t2\nN\t3\t0\ta\n", zurueck), "Index abgeschnitten: abgelehnt");
        BspSuchmerkmale b;
        b.shader = {"textures/system/caulk"};
        std::vector<std::string> alle{"models/a.md3", "models/x/y.md3", "models/z/kaputt.md3"};
        pruefe(modellKandidatenMitIndex(alle, b, idx).empty(), "Index: weder Signatur noch Shader - kein Kandidat");
        b.suppen = {{3, ie.netze[0].signatur}};
        auto k1 = modellKandidatenMitIndex(alle, b, idx);
        pruefe(k1.size() == 1 && k1[0] == "models/a.md3", "Index: gleiche Eckenzahl und Dreiecksmenge wie eine Dreiecksflaeche - Kandidat (Durchgang 1)");
        b.suppen.clear();
        b.shader = {"models/x/haut", "textures/system/caulk"};
        auto k2 = modellKandidatenMitIndex(alle, b, idx);
        pruefe(k2.size() == 1 && k2[0] == "models/x/y.md3", "Index: Shader eines Netzes in der .bsp - Kandidat (Durchgang 2, forcemeta), Ordner x zusaetzlich nach der Ordnerregel");
    }
    {
        // Zwei Dreiecke eines Quadrats (q3map2-Reihenfolge: im Uhrzeigersinn von vorn, vorn = +z)
        // -> ein Vieleck mit 4 Ecken -> ein Pinsel mit 6 Seiten, vorn die Flaeche auf z = 8
        RueckErgebnisDreieck a;
        RueckErgebnisDreieck b;
        const double q[4][3] = {{0, 0, 8}, {64, 0, 8}, {64, 64, 8}, {0, 64, 8}};
        const int ia[3] = {0, 2, 1};
        const int ib[3] = {0, 3, 2};
        for (int k = 0; k < 3; ++k) {
            for (int c = 0; c < 3; ++c) {
                a.p[k][c] = q[ia[k]][c];
                b.p[k][c] = q[ib[k]][c];
            }
            a.st[k][0] = q[ia[k]][0] / 64.0;
            a.st[k][1] = q[ia[k]][1] / 64.0;
            b.st[k][0] = q[ib[k]][0] / 64.0;
            b.st[k][1] = q[ib[k]][1] / 64.0;
        }
        a.shader = b.shader = "textures/test/x";
        a.flaeche = b.flaeche = 7;
        const std::vector<ModellVieleck> v = unerkannteVielecke({a, b});
        pruefe(v.size() == 1 && v[0].punkte.size() == 4 && std::fabs(v[0].normale[2] - 1.0) < 1e-9 && std::fabs(v[0].stS[0] - 1.0 / 64.0) < 1e-9 &&
                   std::fabs(v[0].stS[3]) < 1e-9,
               "zwei Dreiecke eines Quadrats: ein Vieleck mit 4 Ecken, Normale +z, s = x / 64");
        Kontext kx;
        kx.format = PinselFormat::Quake3BP;
        const std::shared_ptr<Pinsel> p = v.empty() ? nullptr : vieleckPinsel(v[0], 2.0, "textures/system/nodraw", kx);
        int tragen = 0;
        double zmin = 1e9;
        double zmax = -1e9;
        if (p) {
            (void)p->entartet(kx);   // baut die Wicklungen (bRep)
            for (std::size_t i = 0; i < p->anzahl(); ++i) {
                tragen += (*p)[i].traegtBei() ? 1 : 0;
                for (std::size_t k = 0; k < (*p)[i].wicklung().numpoints; ++k) {
                    zmin = std::min(zmin, (*p)[i].wicklung()[k].vertex.z());
                    zmax = std::max(zmax, (*p)[i].wicklung()[k].vertex.z());
                }
            }
        }
        pruefe(p != nullptr && p->anzahl() == 6 && tragen == 6 && std::fabs(zmax - 8.0) < 1e-6 && std::fabs(zmin - 6.0) < 1e-6 && p->istDetail() &&
                   (*p)[0].shader() == "textures/test/x" && (*p)[1].shader() == "textures/system/nodraw",
               "Vieleck-Pinsel: 6 Seiten mit Flaeche, z 6..8, vorn der Shader, sonst nodraw, detail");
    }
}

}  // namespace

int main(int argc, char** argv) {
    // Stufe 8b Punkt 8: kartemodellrueck --alle <karte.bsp> <alle.bhmd> [<quelle.map>] - der
    // Dump enthaelt ALLE Modelle des Spiels; daraus der Index, die Kandidaten wie im Editor
    // (Ordnerregel + Index) und nur mit ihnen die Suche.
    const bool alleModelle = argc >= 2 && std::strcmp(argv[1], "--alle") == 0;
    if (alleModelle) {
        --argc;
        ++argv;
    }
    if (argc < 3) {
        selbstPruefung();
        std::printf("kartemodellrueck: %d Pruefungen, %d Fehlschlaege\n", g_pruefungen, g_fehler);
        return g_fehler == 0 ? 0 : 1;
    }
    std::vector<ModellDatei> modelle;
    if (!modellDumpLesen(lies(argv[2]), modelle)) {
        std::printf("Dump nicht lesbar: %s\n", argv[2]);
        return 2;
    }
    const std::string bsp = lies(argv[1]);
    if (alleModelle) {
        using Uhr = std::chrono::steady_clock;
        const auto t0 = Uhr::now();
        std::vector<ModellIndexEintrag> index;
        for (const ModellDatei& m : modelle) { index.push_back(modellIndexEintrag(m, "-")); }
        const std::string text = modellIndexSchreiben(index);
        const auto t1 = Uhr::now();
        std::vector<ModellIndexEintrag> gelesen;
        const bool ok = modellIndexLesen(text, gelesen);
        const auto t2 = Uhr::now();
        const BspSuchmerkmale mk = bspSuchmerkmale(bsp);
        std::vector<std::string> pfade;
        for (const ModellDatei& m : modelle) { pfade.push_back(m.pfad); }
        const std::vector<std::string> ordner = modellKandidaten(pfade, mk.shader);
        const std::vector<std::string> kand = modellKandidatenMitIndex(pfade, mk, gelesen);
        const auto t3 = Uhr::now();
        std::set<std::string> k(kand.begin(), kand.end());
        std::vector<ModellDatei> auswahl;
        std::size_t geladen = 0;
        for (const ModellDatei& m : modelle) {
            geladen += m.geladen ? 1U : 0U;
            if (k.count(m.pfad) != 0) { auswahl.push_back(m); }
        }
        const auto ms = [](Uhr::duration d) { return std::chrono::duration<double, std::milli>(d).count(); };
        std::printf("Alle Modelle: %zu (geladen %zu), Index %zu Byte, bauen %.1f ms, lesen %.1f ms (%s), Kandidaten Ordnerregel %zu, mit Index %zu (%.1f ms)\n",
                    modelle.size(), geladen, text.size(), ms(t1 - t0), ms(t2 - t1), ok ? "ok" : "FEHLER", ordner.size(), kand.size(), ms(t3 - t2));
        modelle = std::move(auswahl);
    }
    const auto suchStart = std::chrono::steady_clock::now();
    RueckOptionen o;
    const RueckErgebnis e = modelleZurueckgewinnen(bsp, modelle, o);
    std::printf("Modelle im Dump: %zu, Dreiecksflaechen: %zu, Treffer: %zu, nicht erkannte Flaechen: %zu, Suche %.2f s\n", modelle.size(), e.dreiecksflaechen,
                e.treffer.size(), e.unerkannt.size(), std::chrono::duration<double>(std::chrono::steady_clock::now() - suchStart).count());
    double maxRest = 0;
    for (const ModellTreffer& t : e.treffer) { maxRest = std::max(maxRest, t.maxFehler); }
    std::printf("groesster Eckenfehler eines Treffers: %.5f\n", maxRest);
    {
        std::map<std::string, int> jeShader;
        for (const auto& d : e.unerkannteDreiecke) { ++jeShader[d.shader]; }
        std::vector<std::pair<int, std::string>> v;
        for (const auto& [k, n] : jeShader) { v.emplace_back(n, k); }
        std::sort(v.rbegin(), v.rend());
        std::printf("nicht erkannte Dreiecke: %zu, als Vielecke: %zu\n", e.unerkannteDreiecke.size(), unerkannteVielecke(e.unerkannteDreiecke).size());
        for (std::size_t i = 0; i < v.size() && i < 15; ++i) { std::printf("  %6d %s\n", v[i].first, v[i].second.c_str()); }
    }
    if (argc < 4) {
        for (const ModellTreffer& t : e.treffer) {
            std::string s;
            for (const auto& [k, v] : trefferSchluessel(t)) { s += "\"" + k + "\" \"" + v + "\" "; }
            std::printf("  %s (%d/%d Netze, Rest %.4f)\n", s.c_str(), t.netzeGefunden, t.netzeGesamt, t.maxFehler);
        }
        return 0;
    }
    // Vergleich mit der Quelle
    const auto ents = mapEntitaeten(lies(argv[3]));
    std::vector<std::vector<std::pair<std::string, std::string>>> quelle;
    for (const auto& en : ents) {
        if (klein(wert(en, "classname")) == "misc_model") { quelle.push_back(en); }
    }
    std::vector<bool> belegt(quelle.size(), false);
    int richtig = 0;
    int falsch = 0;
    Abweichung maxW;
    std::map<std::string, int> jeModell;
    std::map<std::string, int> jeModellGefunden;
    for (const auto& q : quelle) { ++jeModell[klein(wert(q, "model"))]; }
    for (const ModellTreffer& t : e.treffer) {
        // die naechste Quell-Entitaet mit gleichem Modell
        int best = -1;
        double bestD = 1e30;
        int bestAnders = -1;
        double bestAndersD = 1e30;
        for (std::size_t i = 0; i < quelle.size(); ++i) {
            if (belegt[i]) { continue; }
            const ModellLage l = lageAusSchluesseln(quelle[i]);
            double dd = 0;
            for (int k = 0; k < 3; ++k) { dd = std::max(dd, std::fabs(l.origin[k] - t.lage.origin[k])); }
            if (klein(wert(quelle[i], "model")) == klein(t.pfad)) {
                if (dd < bestD) {
                    bestD = dd;
                    best = static_cast<int>(i);
                }
            } else if (dd < bestAndersD) {
                bestAndersD = dd;
                bestAnders = static_cast<int>(i);
            }
        }
        if (best >= 0 && bestD < 1.0) {
            belegt[static_cast<std::size_t>(best)] = true;
            ++richtig;
            ++jeModellGefunden[klein(t.pfad)];
            const Abweichung w = abweichung(t.lage, lageAusSchluesseln(quelle[static_cast<std::size_t>(best)]));
            maxW.ort = std::max(maxW.ort, w.ort);
            maxW.winkel = std::max(maxW.winkel, w.winkel);
            maxW.skala = std::max(maxW.skala, w.skala);
        } else {
            ++falsch;
            std::printf("  Fehlzuordnung: %s bei %.1f %.1f %.1f (naechste Quelle anderes Modell: %s, %.2f)\n", t.pfad.c_str(), t.lage.origin[0], t.lage.origin[1],
                        t.lage.origin[2], bestAnders >= 0 ? wert(quelle[static_cast<std::size_t>(bestAnders)], "model").c_str() : "-", bestAndersD);
        }
    }
    std::printf("erkannt %d von %zu misc_model, Fehlzuordnungen %d\n", richtig, quelle.size(), falsch);
    std::printf("groesste Abweichung gegen die Quelle: Ort %.4f, Winkel %.5f Grad, Skalierung %.6f (relativ)\n", maxW.ort, maxW.winkel, maxW.skala);
    for (const auto& [m, n] : jeModell) { std::printf("  %-70s %d von %d\n", m.c_str(), jeModellGefunden[m], n); }
    for (std::size_t i = 0; i < quelle.size(); ++i) {
        if (!belegt[i]) {
            std::printf("  nicht erkannt: %s origin %s spawnflags %s\n", wert(quelle[i], "model").c_str(), wert(quelle[i], "origin").c_str(),
                        wert(quelle[i], "spawnflags").c_str());
        }
    }
    return 0;
}
