// bspmergetest.cpp - rc609 B1: Brush-Entities in fertige Karten einfuegen (gerufen von
// tools/karte/bspmerge_test.py, ctest bspmerge).
//
//   bspmergetest einheit
//       pinselUeberschneiden und die Grenzen je Spiel ohne Dateien
//   bspmergetest pruefen <original.bsp> <teil.bsp> <aus.bsp> <x y z> <x y z> <x y z> <ox oy oz>
//       Original (Welt + func_door) plus Teil (trigger_multiple, trigger_once, func_usable mit
//       origin-Brush); die drei Punkte liegen in den neuen Brushes (Weltkoordinaten), der
//       letzte Vektor ist der Ursprung des func_usable. Prueft: Welt-Lumps Byte-gleich, an die
//       uebrigen nur angehaengt, Indizes der neuen Modelle, ein Blatt je Modell, der CM-Port
//       (leseKollision) laedt sie und die Punkte liegen in ihren Brushes; die Grenzen.
//   bspmergetest befunde <original.bsp> <teil.bsp> [jka|jk2|md]
//       nur die Meldungen ("befund fehler|warnung <pruefung> <text>")
#include "bhed/karte/bspmerge.h"
#include "bhed/karte/bspoeffnen.h"
#include "bhed/kollision.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
#include <string>
#include <vector>

namespace {

using bhed::karte::BspDatei;
using bhed::karte::MergeErgebnis;
using bhed::karte::MergeSpiel;

int g_fehler = 0;

void pruefe(bool ok, const std::string& was) {
    std::printf("%s %s\n", ok ? "OK  " : "FEHL", was.c_str());
    if (!ok) { ++g_fehler; }
}

std::string lies(const char* p) {
    std::ifstream f(p, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

std::int32_t i32(const std::string& b, std::size_t at) {
    std::uint32_t v = 0;
    for (std::size_t i = 0; i < 4; ++i) { v |= static_cast<std::uint32_t>(static_cast<unsigned char>(b[at + i])) << (8 * i); }
    return static_cast<std::int32_t>(v);
}

void befunde(const MergeErgebnis& r) {
    for (const auto& m : r.meldungen) { std::printf("befund %s %s %s\n", m.fehler ? "fehler" : "warnung", m.pruefung.c_str(), m.text.c_str()); }
}

bhed::karte::Halbraum ebene(double nx, double ny, double nz, double d) {
    bhed::karte::Halbraum h;
    h.n[0] = nx;
    h.n[1] = ny;
    h.n[2] = nz;
    h.d = d;
    return h;
}
std::vector<bhed::karte::Halbraum> quader(double x0, double y0, double z0, double x1, double y1, double z1) {
    return {ebene(-1, 0, 0, -x0), ebene(1, 0, 0, x1), ebene(0, -1, 0, -y0), ebene(0, 1, 0, y1), ebene(0, 0, -1, -z0), ebene(0, 0, 1, z1)};
}

int einheit() {
    using bhed::karte::pinselUeberschneiden;
    const auto a = quader(0, 0, 0, 64, 64, 64);
    pruefe(pinselUeberschneiden(a, quader(32, 32, 32, 96, 96, 96)), "Wuerfel um 32 versetzt: ueberschneiden sich");
    pruefe(!pinselUeberschneiden(a, quader(64, 0, 0, 128, 64, 64)), "Wuerfel Flaeche an Flaeche: keine Ueberschneidung");
    pruefe(pinselUeberschneiden(a, quader(63, 0, 0, 128, 64, 64)), "Wuerfel 1 Einheit ineinander: ueberschneiden sich");
    // Keil (x + y <= 64) und ein Wuerfel in der Ecke (x, y >= 40): die Kaesten beruehren sich, die Brushes nicht
    auto keil = quader(0, 0, 0, 64, 64, 64);
    const double s = 1.0 / std::sqrt(2.0);
    keil.push_back(ebene(s, s, 0, 64 * s));
    pruefe(!pinselUeberschneiden(keil, quader(40, 40, 0, 64, 64, 64)), "Keil und Wuerfel in der freien Ecke: keine Ueberschneidung (Kaesten schon)");
    pruefe(pinselUeberschneiden(keil, quader(20, 20, 0, 64, 64, 64)), "Keil und Wuerfel ueber die Schraege: ueberschneiden sich");
    // Grenzen je Spiel (cm_load.cpp, cm_local.h)
    const auto jka = bhed::karte::modellGrenzen(MergeSpiel::Jka);
    const auto md = bhed::karte::modellGrenzen(MergeSpiel::MovieDuels);
    const auto jk2 = bhed::karte::modellGrenzen(MergeSpiel::Jk2);
    pruefe(jka.size() == 2 && jka[0].hoechstens == 511 && jka[1].hoechstens == 510, "JKA: SP 511 (BOX_MODEL_HANDLE), MP 510 (CAPSULE_MODEL_HANDLE)");
    pruefe(md.size() == 2 && md[0].hoechstens == 511 && md[1].hoechstens == 510, "Movie Duels: SP 511, MP 510");
    pruefe(jk2.size() == 2 && jk2[0].hoechstens == 511, "JK2: SP 511");
    return g_fehler == 0 ? 0 : 1;
}

// Eine Kopie des Originals mit insgesamt `modelle` Modellen (das letzte wiederholt) - fuer die Grenzen.
std::string mitModellen(const std::string& original, std::size_t modelle) {
    BspDatei d;
    if (!bhed::karte::bspLesen(original, d)) { return {}; }
    std::string md = d.lumps[7];
    const std::string letztes = md.substr(md.size() - 40);
    while (md.size() / 40 < modelle) { md += letztes; }
    return bhed::karte::bspMitLumps(original, {{7, md}});
}

bool hat(const MergeErgebnis& r, bool fehler, const std::string& pruefung, const std::string& text = {}) {
    for (const auto& m : r.meldungen) {
        if (m.fehler == fehler && m.pruefung == pruefung && m.text.find(text) != std::string::npos) { return true; }
    }
    return false;
}

int pruefen(int argc, char** argv) {
    if (argc < 17) {
        std::fprintf(stderr, "pruefen: zu wenig Argumente\n");
        return 2;
    }
    const std::string orig = lies(argv[2]);
    const std::string teil = lies(argv[3]);
    float p[3][3];
    for (int i = 0; i < 3; ++i) {
        for (int k = 0; k < 3; ++k) { p[i][k] = static_cast<float>(std::atof(argv[5 + i * 3 + k])); }
    }
    const float ursprung[3] = {static_cast<float>(std::atof(argv[14])), static_cast<float>(std::atof(argv[15])), static_cast<float>(std::atof(argv[16]))};
    const MergeErgebnis r = bhed::karte::bspModelleAnhaengen(orig, teil, MergeSpiel::Jka);
    befunde(r);
    pruefe(r.ok() && !r.hatFehler(), "zusammengefuehrt, keine Fehler");
    if (!r.ok()) { return 1; }
    {
        std::ofstream f(argv[4], std::ios::binary);
        f << r.bsp;
    }
    BspDatei a;
    BspDatei b;
    BspDatei t;
    pruefe(bhed::karte::bspLesen(orig, a) && bhed::karte::bspLesen(r.bsp, b) && bhed::karte::bspLesen(teil, t) && a.lumps.size() == 18 &&
               b.lumps.size() == 18,
           "Original, Ergebnis und Teil sind RBSP mit 18 Lumps");
    if (g_fehler != 0) { return 1; }
    // 1. Welt-Lumps Byte-gleich (nodes 3, leafsurfaces 5, drawverts 10, drawindexes 11, fogs 12,
    //    surfaces 13, lightmaps 14, lightgrid 15, visibility 16, lightarray 17)
    int gleich = 0;
    for (std::size_t i : {3U, 5U, 10U, 11U, 12U, 13U, 14U, 15U, 16U, 17U}) { gleich += a.lumps[i] == b.lumps[i] ? 1 : 0; }
    pruefe(gleich == 10, "Welt-Lumps Byte-gleich (Baum, Blattflaechen, Ecken, Indizes, Nebel, Flaechen, Lightmaps, Lichtgitter, VIS): " +
                             std::to_string(gleich) + " von 10");
    pruefe(!a.lumps[14].empty() && !a.lumps[16].empty(), "das Original hat Lightmaps (" + std::to_string(a.lumps[14].size()) + " Bytes) und VIS (" +
                                                             std::to_string(a.lumps[16].size()) + " Bytes)");
    int vorn = 0;
    for (std::size_t i : {1U, 2U, 4U, 6U, 7U, 8U, 9U}) {
        vorn += b.lumps[i].size() > a.lumps[i].size() && b.lumps[i].compare(0, a.lumps[i].size(), a.lumps[i]) == 0 ? 1 : 0;
    }
    const std::string entA = bhed::karte::bspEntitaetenText(a);
    const bool entVorn = b.lumps[0].compare(0, entA.size(), entA) == 0 && b.lumps[0].back() == '\0';
    pruefe(vorn == 7 && entVorn, "Shader, Ebenen, Blaetter, Blatt-Brushes, Modelle, Brushes, Seiten: das Original unveraendert vorn, nur angehaengt (" +
                                     std::to_string(vorn) + " von 7); Entity-Lump ebenso");
    // 2. Indizes
    const std::size_t mA = a.lumps[7].size() / 40;
    const std::size_t mB = b.lumps[7].size() / 40;
    pruefe(r.ersteNummer == static_cast<int>(mA) && r.neueModelle == 3 && mB == mA + 3,
           "3 neue Modelle *" + std::to_string(r.ersteNummer) + "..*" + std::to_string(r.ersteNummer + 2) + " (Original " + std::to_string(mA) + ")");
    const std::size_t brA = a.lumps[8].size() / 12;
    const std::size_t sdA = a.lumps[9].size() / 12;
    const std::size_t brB = b.lumps[8].size() / 12;
    const std::size_t sdB = b.lumps[9].size() / 12;
    const std::size_t plB = b.lumps[2].size() / 16;
    const std::size_t shB = b.lumps[1].size() / 72;
    const std::size_t flA = a.lumps[13].size() / 148;
    bool indizes = true;
    std::size_t naechster = brA;
    for (std::size_t m = mA; m < mB; ++m) {
        const int first = i32(b.lumps[7], m * 40 + 32);
        const int num = i32(b.lumps[7], m * 40 + 36);
        indizes = indizes && first == static_cast<int>(naechster) && num > 0 && i32(b.lumps[7], m * 40 + 24) == static_cast<int>(flA) &&
                  i32(b.lumps[7], m * 40 + 28) == 0;
        naechster += static_cast<std::size_t>(num);
    }
    indizes = indizes && naechster == brB;
    std::size_t seite = sdA;
    for (std::size_t br = brA; br < brB; ++br) {
        const int fs = i32(b.lumps[8], br * 12);
        const int ns = i32(b.lumps[8], br * 12 + 4);
        const int sh = i32(b.lumps[8], br * 12 + 8);
        indizes = indizes && fs == static_cast<int>(seite) && ns >= 6 && sh >= 0 && static_cast<std::size_t>(sh) < shB;
        for (int s = 0; s < ns; ++s) {
            const std::size_t q = (static_cast<std::size_t>(fs) + static_cast<std::size_t>(s)) * 12;
            indizes = indizes && i32(b.lumps[9], q) >= 0 && static_cast<std::size_t>(i32(b.lumps[9], q)) < plB && i32(b.lumps[9], q + 4) >= 0 &&
                      static_cast<std::size_t>(i32(b.lumps[9], q + 4)) < shB && i32(b.lumps[9], q + 8) == 0;
        }
        seite += static_cast<std::size_t>(ns);
    }
    indizes = indizes && seite == sdB;
    pruefe(indizes, "Modelle -> Brushes -> Seiten -> Ebenen/Shader: lueckenlos angehaengt, im Bereich, drawSurfNum wie q3map2 (0), keine Flaechen");
    // Shader: die beiden Trigger teilen einen Eintrag (EmitShader), der Teil hat ihn einmal
    std::set<int> triggerShader;
    for (std::size_t m = mA; m < mA + 2; ++m) {
        const int first = i32(b.lumps[7], m * 40 + 32);
        triggerShader.insert(i32(b.lumps[8], static_cast<std::size_t>(first) * 12 + 8));
    }
    pruefe(triggerShader.size() == 1 && shB < a.lumps[1].size() / 72 + t.lumps[1].size() / 72,
           "Shader wiederverwendet: beide Trigger auf demselben Eintrag, " + std::to_string(shB - a.lumps[1].size() / 72) + " neue Shader (Teil hat " +
               std::to_string(t.lumps[1].size() / 72) + ")");
    // 3. ein Blatt je Modell (EmitLeaf) mit genau den Brushes des Modells
    const std::size_t lfA = a.lumps[4].size() / 48;
    const std::size_t lfB = b.lumps[4].size() / 48;
    bool blaetter = lfB == lfA + 3;
    for (std::size_t m = mA; blaetter && m < mB; ++m) {
        const std::size_t l = lfA + (m - mA);
        const int first = i32(b.lumps[7], m * 40 + 32);
        const int num = i32(b.lumps[7], m * 40 + 36);
        const int fl = i32(b.lumps[4], l * 48 + 40);
        const int nl = i32(b.lumps[4], l * 48 + 44);
        blaetter = nl == num && i32(b.lumps[4], l * 48 + 36) == 0 && i32(b.lumps[4], l * 48 + 32) == static_cast<int>(a.lumps[5].size() / 4);
        std::set<int> ist;
        for (int i = 0; blaetter && i < nl; ++i) { ist.insert(i32(b.lumps[6], (static_cast<std::size_t>(fl) + static_cast<std::size_t>(i)) * 4)); }
        for (int i = 0; blaetter && i < num; ++i) { blaetter = ist.count(first + i) == 1U; }
    }
    pruefe(blaetter, "je neues Modell ein Blatt (EmitLeaf) mit genau seinen Brushes, ohne Flaechen");
    // 4. Entity-Lump
    const std::string entB = bhed::karte::bspEntitaetenText(b);
    const std::string neu = entB.substr(entA.size());
    const auto n = [&](int k) { return "\"model\" \"*" + std::to_string(r.ersteNummer + k) + "\""; };
    pruefe(neu.find("\"classname\" \"trigger_multiple\"") != std::string::npos && neu.find(n(0)) != std::string::npos &&
               neu.find("\"classname\" \"trigger_once\"") != std::string::npos && neu.find(n(1)) != std::string::npos &&
               neu.find("\"classname\" \"func_usable\"") != std::string::npos && neu.find(n(2)) != std::string::npos &&
               neu.find("\"origin\"") != std::string::npos,
           "Entity-Lump: trigger_multiple " + n(0) + ", trigger_once " + n(1) + ", func_usable " + n(2) + " mit origin");
    // 5. CM-Port (CM_LoadMap / CMod_LoadSubmodels): Punkt-im-Brush
    bhed::KollisionsKarte k;
    bhed::KollisionsKarte k0;
    const bool geladen = bhed::leseKollision(r.bsp, k) && bhed::leseKollision(orig, k0);
    pruefe(geladen && k.modelle.size() == mB, "der CM-Port laedt die gemischte Karte, " + std::to_string(k.modelle.size()) + " Modelle");
    if (!geladen) { return 1; }
    const float aussen[3] = {p[0][0] + 1000.0F, p[0][1], p[0][2]};
    const std::uint32_t c0 = bhed::kollisionsInhalt(k, p[0], r.ersteNummer);
    const std::uint32_t c1 = bhed::kollisionsInhalt(k, p[1], r.ersteNummer + 1);
    const std::uint32_t c2 = bhed::kollisionsInhalt(k, p[2], r.ersteNummer + 2, ursprung);
    const std::uint32_t c2ohne = bhed::kollisionsInhalt(k, p[2], r.ersteNummer + 2);
    char z[256];
    std::snprintf(z, sizeof z, "Punkt-im-Brush: trigger_multiple 0x%x, trigger_once 0x%x (CONTENTS_TRIGGER 0x400), func_usable am Ursprung 0x%x (SOLID), "
                               "ohne Ursprung 0x%x, daneben 0x%x",
                  c0, c1, c2, c2ohne, bhed::kollisionsInhalt(k, aussen, r.ersteNummer));
    pruefe((c0 & 0x400U) != 0U && (c1 & 0x400U) != 0U && (c2 & 1U) != 0U && c2ohne == 0U && bhed::kollisionsInhalt(k, aussen, r.ersteNummer) == 0U, z);
    // Die Welt fuer das Spiel gleich: CM_PointContents an einem Gitter in beiden Karten
    int anders = 0;
    int punkte = 0;
    for (float x = -512; x <= 512; x += 32) {
        for (float y = -256; y <= 256; y += 32) {
            for (float zz = -32; zz <= 288; zz += 32) {
                const float q[3] = {x, y, zz};
                anders += bhed::kollisionsInhalt(k, q) != bhed::kollisionsInhalt(k0, q) ? 1 : 0;
                ++punkte;
            }
        }
    }
    pruefe(anders == 0, "Welt (Modell 0) im CM-Port gleich: " + std::to_string(punkte) + " Punkte, " + std::to_string(anders) + " anders");
    // 6. Meldungen
    pruefe(hat(r, false, "ziel", "fehlt"), "Warnung: trigger_once target \"fehlt\" hat kein Ziel in der Karte");
    pruefe(!hat(r, false, "ziel", "drucker"), "keine Warnung fuer target \"drucker\" (target_print der Karte)");
    pruefe(hat(r, false, "ueberschneidung", "*" + std::to_string(r.ersteNummer + 2)) && !hat(r, false, "ueberschneidung", "*" + std::to_string(r.ersteNummer) + ":"),
           "Warnung: func_usable (solid) steckt im Boden; die Trigger in der Wand nicht");
    // 7. Grenzen: Original mit 507 + 3 = 510 Modellen geht, 508 + 3 = 511 nur im SP, 509 + 3 = 512 nirgends
    const MergeErgebnis g510 = bhed::karte::bspModelleAnhaengen(mitModellen(orig, 507), teil, MergeSpiel::Jka);
    const MergeErgebnis g511 = bhed::karte::bspModelleAnhaengen(mitModellen(orig, 508), teil, MergeSpiel::Jka);
    const MergeErgebnis g512 = bhed::karte::bspModelleAnhaengen(mitModellen(orig, 509), teil, MergeSpiel::MovieDuels);
    pruefe(g510.ok() && !hat(g510, true, "grenze"), "Grenze: 510 Modelle gehen (JKA SP und MP)");
    pruefe(!g511.ok() && hat(g511, true, "grenze", "JKA MP: 511") && !hat(g511, true, "grenze", "JKA SP"), "Grenze: 511 Modelle - Fehler nur fuer JKA MP");
    pruefe(!g512.ok() && hat(g512, true, "grenze", "Movie Duels SP: 512") && hat(g512, true, "grenze", "Movie Duels MP: 512"),
           "Grenze: 512 Modelle - Fehler fuer Movie Duels SP und MP");
    return g_fehler == 0 ? 0 : 1;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc >= 2 && std::strcmp(argv[1], "einheit") == 0) { return einheit(); }
    if (argc >= 2 && std::strcmp(argv[1], "pruefen") == 0) { return pruefen(argc, argv); }
    if (argc >= 4 && std::strcmp(argv[1], "befunde") == 0) {
        const std::string s = argc >= 5 ? argv[4] : "jka";
        const MergeSpiel spiel = s == "jk2" ? MergeSpiel::Jk2 : (s == "md" ? MergeSpiel::MovieDuels : MergeSpiel::Jka);
        const MergeErgebnis r = bhed::karte::bspModelleAnhaengen(lies(argv[2]), lies(argv[3]), spiel);
        befunde(r);
        std::printf("ergebnis %s\n", r.ok() ? "ok" : "nein");
        return 0;
    }
    std::fprintf(stderr, "bspmergetest einheit | pruefen ... | befunde <original> <teil> [jka|jk2|md]\n");
    return 2;
}
