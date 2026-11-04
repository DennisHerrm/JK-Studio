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
//   bspmergetest welt <original.bsp> <teil.bsp> <aus.bsp> <x0 y0 z0 x1 y1 z1>        (rc609 B13 Stufe 2: Clip in der Welt)
//   bspmergetest licht <original.bsp> <teillicht.bsp> <aus.bsp>                   (rc609 B13 Stufe 3: sichtbarer Mover mit Lightmaps)
//   bspmergetest befehle <q3map2.exe> <basispfad> <original.bsp> <teillicht.map>  (die Befehle des Licht-Teils)
//   bspmergetest befunde <original.bsp> <teil.bsp> [jka|jk2|md]
//       nur die Meldungen ("befund fehler|warnung <pruefung> <text>")
#include "bhed/bsp.h"
#include "bhed/karte/bspmerge.h"
#include "bhed/karte/bspoeffnen.h"
#include "bhed/kollision.h"

#include <algorithm>
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

// --- Stufe 2: Clips in der Welt ---------------------------------------------------------------------
// bspmergetest welt <original.bsp> <teil.bsp> <aus.bsp> <x0 y0 z0 x1 y1 z1>: der Teil hat einen playerclip-Brush (Kasten x0..x1)
// als func_static mit der Marke _bhed_welt; er kommt in die Welt (Modell 0, leafbrushes der Blaetter, die er schneidet).
int welt(int argc, char** argv) {
    if (argc < 11) {
        std::fprintf(stderr, "welt: zu wenig Argumente\n");
        return 2;
    }
    const std::string orig = lies(argv[2]);
    const std::string teil = lies(argv[3]);
    float lo[3];
    float hi[3];
    for (int k = 0; k < 3; ++k) {
        lo[k] = static_cast<float>(std::atof(argv[5 + k]));
        hi[k] = static_cast<float>(std::atof(argv[8 + k]));
    }
    const MergeErgebnis r = bhed::karte::bspModelleAnhaengen(orig, teil, MergeSpiel::Jka);
    befunde(r);
    pruefe(r.ok() && !r.hatFehler() && r.weltPinsel == 1 && r.neueModelle == 0,
           "zusammengefuehrt: 1 Clip in die Welt, keine neuen Modelle (" + std::to_string(r.weltPinsel) + " Clips, " + std::to_string(r.neueModelle) + " Modelle)");
    if (!r.ok()) { return 1; }
    {
        std::ofstream f(argv[4], std::ios::binary);
        f << r.bsp;
    }
    BspDatei a;
    BspDatei b;
    if (!bhed::karte::bspLesen(orig, a) || !bhed::karte::bspLesen(r.bsp, b)) { return 1; }
    // Byte-gleich: Baum 3, Blattflaechen 5, Ecken 10, Indizes 11, Nebel 12, Flaechen 13, Lightmaps 14, Lichtgitter 15, VIS 16, Lichtfeld 17
    int gleich = 0;
    for (std::size_t i : {3U, 5U, 10U, 11U, 12U, 13U, 14U, 15U, 16U, 17U}) { gleich += a.lumps[i] == b.lumps[i] ? 1 : 0; }
    pruefe(gleich == 10, "Baum, VIS, Flaechen, Ecken, Indizes, Nebel, Lightmaps, Lichtgitter Byte-gleich: " + std::to_string(gleich) + " von 10");
    // Modell 0 hat einen Brush mehr, die Untermodelle ruecken nach, der neue Brush steht am Ende der Welt-Brushes
    const int w0 = i32(a.lumps[7], 36);
    const std::size_t mA = a.lumps[7].size() / 40;
    bool modelle = b.lumps[7].size() == a.lumps[7].size() && i32(b.lumps[7], 36) == w0 + 1 && i32(b.lumps[7], 32) == i32(a.lumps[7], 32);
    for (std::size_t m = 1; m < mA; ++m) { modelle = modelle && i32(b.lumps[7], m * 40 + 32) == i32(a.lumps[7], m * 40 + 32) + 1; }
    const int neu = i32(a.lumps[7], 32) + w0;
    pruefe(modelle && b.lumps[8].size() == a.lumps[8].size() + 12, "Modell 0: " + std::to_string(w0) + " -> " + std::to_string(i32(b.lumps[7], 36)) +
                                                                      " Brushes, Untermodelle um 1 verschoben, neuer Brush " + std::to_string(neu));
    // Blaetter: gleich viele, der Clip in r.weltBlaetter Blaettern; der Kastentest von MergeBSPMain -world haette mehr
    const std::size_t lf = a.lumps[4].size() / 48;
    int mitClip = 0;
    int kasten = 0;
    bool sonstGleich = b.lumps[4].size() == a.lumps[4].size();
    for (std::size_t l = 0; sonstGleich && l < lf; ++l) {
        const int fb = i32(b.lumps[4], l * 48 + 40);
        const int nb = i32(b.lumps[4], l * 48 + 44);
        bool hat = false;
        std::vector<int> ohne;
        for (int i = 0; i < nb; ++i) {
            const int v = i32(b.lumps[6], static_cast<std::size_t>(fb + i) * 4);
            if (v == neu) {
                hat = true;
            } else {
                ohne.push_back(v > neu ? v - 1 : v);
            }
        }
        std::vector<int> vorher;
        for (int i = 0; i < i32(a.lumps[4], l * 48 + 44); ++i) { vorher.push_back(i32(a.lumps[6], static_cast<std::size_t>(i32(a.lumps[4], l * 48 + 40) + i) * 4)); }
        sonstGleich = ohne == vorher && b.lumps[4].compare(l * 48, 32, a.lumps[4], l * 48, 32) == 0;
        mitClip += hat ? 1 : 0;
        bool beruehrt = true;
        for (int k = 0; k < 3; ++k) {
            const auto mn = static_cast<float>(i32(a.lumps[4], l * 48 + 8 + static_cast<std::size_t>(k) * 4));
            const auto mx = static_cast<float>(i32(a.lumps[4], l * 48 + 20 + static_cast<std::size_t>(k) * 4));
            beruehrt = beruehrt && mn <= hi[k] && mx >= lo[k];
        }
        kasten += beruehrt ? 1 : 0;
    }
    char z[320];
    std::snprintf(z, sizeof z, "Blaetter: der Clip in %d Blaettern (r.weltBlaetter %d), Kastentest (MergeBSPMain -world) %d; sonst jede Blattliste wie vorher",
                  mitClip, r.weltBlaetter, kasten);
    pruefe(sonstGleich && mitClip == r.weltBlaetter && mitClip > 0 && mitClip <= kasten, z);
    // CM-Port: Punkt-im-Brush und Strahl
    bhed::KollisionsKarte k;
    bhed::KollisionsKarte k0;
    if (!bhed::leseKollision(r.bsp, k) || !bhed::leseKollision(orig, k0)) {
        pruefe(false, "CM-Port laedt beide Karten");
        return 1;
    }
    const float mitte[3] = {(lo[0] + hi[0]) * 0.5F, (lo[1] + hi[1]) * 0.5F, (lo[2] + hi[2]) * 0.5F};
    const std::uint32_t c = bhed::kollisionsInhalt(k, mitte);
    const std::uint32_t c0 = bhed::kollisionsInhalt(k0, mitte);
    // Ein Spieler-Kasten (bg_public.h -15 -15 -24 .. 15 15 40) quer durch den Clip, MASK_PLAYERSOLID (SOLID | PLAYERCLIP | BODY ...)
    const float pmin[3] = {-15.0F, -15.0F, -24.0F};
    const float pmax[3] = {15.0F, 15.0F, 40.0F};
    const float start[3] = {lo[0] - 100.0F, mitte[1], lo[2] + 40.0F};
    const float ende[3] = {hi[0] + 100.0F, mitte[1], lo[2] + 40.0F};
    const bhed::KollisionsSpur s = bhed::kollisionsSpur(k, start, pmin, pmax, ende, 0x00000111U);
    const bhed::KollisionsSpur s0 = bhed::kollisionsSpur(k0, start, pmin, pmax, ende, 0x00000111U);
    const bhed::KollisionsSpur schuss = bhed::kollisionsSpur(k, start, pmin, pmax, ende, 0x00000001U);
    std::snprintf(z, sizeof z, "CM-Port: Mitte des Clips 0x%x (vorher 0x%x); Spieler-Strahl endet bei x %.2f (vorher Bruchteil %.3f, erwartet %.2f); "
                               "nur SOLID geht durch (%.3f)",
                  c, c0, static_cast<double>(s.endpos[0]), static_cast<double>(s0.fraction), static_cast<double>(lo[0] - 15.0F),
                  static_cast<double>(schuss.fraction));
    pruefe((c & 0x10U) != 0U && (c0 & 0x10U) == 0U && s0.fraction == 1.0F && std::fabs(s.endpos[0] - (lo[0] - 15.0F)) < 0.2F && schuss.fraction == 1.0F, z);
    // Die Welt an vielen Punkten unveraendert (ausser im Clip)
    // Innen muss der Clip dazukommen, aussen bleibt alles; ein Punkt genau auf einer Seite des Clips liegt in einem der Blaetter
    // beiderseits (CM_PointLeafnum_r: d >= 0 vorn) - an einer Wand im Blatt der Wand, das ihn nicht fuehrt wie nach einem vollen
    // Bau (FilterBrushIntoTree_r legt einen Brush nur in Blaetter, in die er mehr als 0.1 hineinreicht).
    int anders = 0;
    int punkte = 0;
    std::string erster;
    for (float x = -512; x <= 512; x += 16) {
        for (float y = -256; y <= 256; y += 16) {
            for (float zz = -32; zz <= 288; zz += 16) {
                const float q[3] = {x, y, zz};
                const bool drin = x > lo[0] && x < hi[0] && y > lo[1] && y < hi[1] && zz > lo[2] && zz < hi[2];
                const bool rand = !drin && x >= lo[0] && x <= hi[0] && y >= lo[1] && y <= hi[1] && zz >= lo[2] && zz <= hi[2];
                const std::uint32_t v = bhed::kollisionsInhalt(k, q);
                const std::uint32_t v0 = bhed::kollisionsInhalt(k0, q);
                const bool clipDa = (v & ~0x80000030U) == v0 && (v & 0x10U) != 0U;
                const bool falsch = drin ? !clipDa : (rand ? !clipDa && v != v0 : v != v0);
                anders += falsch ? 1 : 0;
                if (falsch && erster.empty()) {
                    std::snprintf(z, sizeof z, "; zuerst (%g %g %g) 0x%x statt 0x%x", static_cast<double>(x), static_cast<double>(y), static_cast<double>(zz), v, v0);
                    erster = z;
                }
                ++punkte;
            }
        }
    }
    pruefe(anders == 0, "Welt im CM-Port: " + std::to_string(punkte) + " Punkte, " + std::to_string(anders) +
                            " anders (im Clip nur PLAYERCLIP | MONSTERCLIP | TRANSLUCENT des Shaders dazu)" + erster);
    return g_fehler == 0 ? 0 : 1;
}

// --- Stufe 3: sichtbarer Mover mit Lightmaps -----------------------------------------------------------
// bspmergetest licht <original.bsp> <teillicht.bsp> <aus.bsp>: der Teil (Welt als Schattenwerfer, die Lichter, ein neuer func_door
// mit sichtbaren Flaechen, -bsp -meta und -light) beleuchtet; uebernommen wird nur das neue Modell mit seinen Lightmaps.
int licht(int argc, char** argv) {
    if (argc < 5) {
        std::fprintf(stderr, "licht: zu wenig Argumente\n");
        return 2;
    }
    const std::string orig = lies(argv[2]);
    const std::string teil = lies(argv[3]);
    const MergeErgebnis unbel = bhed::karte::bspModelleAnhaengen(orig, teil, MergeSpiel::Jka, false);
    pruefe(!unbel.ok() && hat(unbel, true, "sichtbar"), "unbeleuchtet: Fehler \"sichtbar\" (der Karten-Editor baut dann den Licht-Teil)");
    const MergeErgebnis r = bhed::karte::bspModelleAnhaengen(orig, teil, MergeSpiel::Jka, true);
    befunde(r);
    pruefe(r.ok() && !r.hatFehler() && r.neueModelle == 1 && r.neueFlaechen > 0 && r.neueLightmapSeiten > 0,
           "beleuchtet: 1 neues Modell *" + std::to_string(r.ersteNummer) + ", " + std::to_string(r.neueFlaechen) + " Flaechen, " +
               std::to_string(r.neueLightmapSeiten) + " Lightmap-Seiten");
    if (!r.ok()) { return 1; }
    {
        std::ofstream f(argv[4], std::ios::binary);
        f << r.bsp;
    }
    BspDatei a;
    BspDatei b;
    BspDatei t;
    if (!bhed::karte::bspLesen(orig, a) || !bhed::karte::bspLesen(r.bsp, b) || !bhed::karte::bspLesen(teil, t)) { return 1; }
    int gleich = 0;
    for (std::size_t i : {3U, 12U, 15U, 16U, 17U}) { gleich += a.lumps[i] == b.lumps[i] ? 1 : 0; }
    int vorn = 0;
    for (std::size_t i : {5U, 10U, 11U, 13U, 14U}) { vorn += b.lumps[i].size() > a.lumps[i].size() && b.lumps[i].compare(0, a.lumps[i].size(), a.lumps[i]) == 0 ? 1 : 0; }
    pruefe(gleich == 5 && vorn == 5, "Baum, Nebel, Lichtgitter, VIS Byte-gleich (" + std::to_string(gleich) + " von 5); Welt-Lightmaps, -Flaechen, Ecken, "
                                     "Indizes, Blattflaechen Byte-gleich vorn, nur angehaengt (" + std::to_string(vorn) + " von 5)");
    const std::size_t seitenA = a.lumps[14].size() / 49152;
    const std::size_t seitenB = b.lumps[14].size() / 49152;
    const std::size_t m = static_cast<std::size_t>(r.ersteNummer);
    const int ersteFl = i32(b.lumps[7], m * 40 + 24);
    const int fl = i32(b.lumps[7], m * 40 + 28);
    const std::size_t flA = a.lumps[13].size() / 148;
    const std::size_t vA = a.lumps[10].size() / 80;
    const std::size_t iA = a.lumps[11].size() / 4;
    bool ok = ersteFl == static_cast<int>(flA) && fl == r.neueFlaechen && b.lumps[13].size() / 148 == flA + static_cast<std::size_t>(fl);
    std::size_t naechsteEcke = vA;
    std::size_t naechsterIndex = iA;
    int lmMin = 1 << 30;
    int lmMax = -1;
    double hell = 0.0;
    int luxel = 0;
    for (int s = ersteFl; ok && s < ersteFl + fl; ++s) {
        const std::size_t q = static_cast<std::size_t>(s) * 148;
        const int sh = i32(b.lumps[13], q);
        const int fv = i32(b.lumps[13], q + 12);
        const int nv = i32(b.lumps[13], q + 16);
        const int fi = i32(b.lumps[13], q + 20);
        const int ni = i32(b.lumps[13], q + 24);
        ok = sh >= 0 && static_cast<std::size_t>(sh) < b.lumps[1].size() / 72 && fv == static_cast<int>(naechsteEcke) && fi == static_cast<int>(naechsterIndex) &&
             i32(b.lumps[13], q + 4) == -1;
        for (int i = 0; ok && i < ni; ++i) {
            const int v = i32(b.lumps[11], (static_cast<std::size_t>(fi) + static_cast<std::size_t>(i)) * 4);
            ok = v >= 0 && v < nv;
        }
        naechsteEcke += static_cast<std::size_t>(nv);
        naechsterIndex += static_cast<std::size_t>(ni);
        for (std::size_t k = 0; ok && k < 4; ++k) {
            const int lm = i32(b.lumps[13], q + 36 + k * 4);
            if (lm < 0) { continue; }
            ok = static_cast<std::size_t>(lm) >= seitenA && static_cast<std::size_t>(lm) < seitenB;
            lmMin = std::min(lmMin, lm);
            lmMax = std::max(lmMax, lm);
            if (k != 0) { continue; }
            // die Luxel an den Lightmap-Koordinaten der Ecken (drawVert_t.lightmap[0], RBSP ab Byte 20; q3map2 schreibt lightmapX/Y 0)
            for (int i = 0; i < nv; ++i) {
                const std::size_t e = (static_cast<std::size_t>(fv) + static_cast<std::size_t>(i)) * 80;
                float uv[2];
                std::memcpy(uv, b.lumps[10].data() + e + 20, sizeof uv);
                const int x = std::clamp(static_cast<int>(uv[0] * 128.0F), 0, 127);
                const int y = std::clamp(static_cast<int>(uv[1] * 128.0F), 0, 127);
                const std::size_t p = static_cast<std::size_t>(lm) * 49152 + (static_cast<std::size_t>(y) * 128 + static_cast<std::size_t>(x)) * 3;
                hell += (static_cast<unsigned char>(b.lumps[14][p]) + static_cast<unsigned char>(b.lumps[14][p + 1]) + static_cast<unsigned char>(b.lumps[14][p + 2])) / 3.0;
                ++luxel;
            }
        }
    }
    ok = ok && naechsteEcke == b.lumps[10].size() / 80 && naechsterIndex == b.lumps[11].size() / 4;
    char z[400];
    std::snprintf(z, sizeof z, "Flaechen *%d: %d ab %d, Ecken und Indizes lueckenlos umgerechnet, Indizes < numVerts, Shader im Bereich, kein Nebel; "
                               "Lightmaps %d..%d im Bereich %zu..%zu (Welt 0..%zu)",
                  r.ersteNummer, fl, ersteFl, lmMin, lmMax, seitenA, seitenB - 1, seitenA - 1);
    pruefe(ok && lmMax >= 0, z);
    // raum.map: "ambient" "8" allein gaebe 8; die Tuer steht 70 neben dem Licht 400 bei -256 -160 192
    std::snprintf(z, sizeof z, "Licht: mittlere Helligkeit der Tuer-Luxel an den Ecken %.1f von 255 (%d Ecken; ambient allein 8)", luxel > 0 ? hell / luxel : 0.0, luxel);
    pruefe(luxel > 0 && hell / luxel > 16.0, z);
    // Die angehaengten Seiten sind die des Teils (nur die benutzten)
    bool seitenGleich = seitenB == seitenA + static_cast<std::size_t>(r.neueLightmapSeiten);
    for (std::size_t s = seitenA; seitenGleich && s < seitenB; ++s) {
        bool gefunden = false;
        for (std::size_t u = 0; !gefunden && u < t.lumps[14].size() / 49152; ++u) { gefunden = b.lumps[14].compare(s * 49152, 49152, t.lumps[14], u * 49152, 49152) == 0; }
        seitenGleich = gefunden;
    }
    pruefe(seitenGleich, "angehaengte Lightmap-Seiten = Seiten des Teils (" + std::to_string(r.neueLightmapSeiten) + " von " +
                             std::to_string(t.lumps[14].size() / 49152) + " des Teils)");
    // FixBrushSides: Seiten der Tuer mit Flaeche zeigen auf die neuen Flaechen
    const int fb = i32(b.lumps[7], m * 40 + 32);
    const int nb = i32(b.lumps[7], m * 40 + 36);
    int aufNeu = 0;
    int seiten = 0;
    for (int br = fb; br < fb + nb; ++br) {
        const int fs = i32(b.lumps[8], static_cast<std::size_t>(br) * 12);
        const int ns = i32(b.lumps[8], static_cast<std::size_t>(br) * 12 + 4);
        for (int s = fs; s < fs + ns; ++s) {
            const int d = i32(b.lumps[9], static_cast<std::size_t>(s) * 12 + 8);
            aufNeu += d >= ersteFl && d < ersteFl + fl ? 1 : 0;
            ++seiten;
        }
    }
    pruefe(aufNeu > 0, "Brush-Seiten der Tuer: " + std::to_string(aufNeu) + " von " + std::to_string(seiten) + " zeigen auf ihre neuen Flaechen (FixBrushSides)");
    // Blatt des Modells mit seinen Flaechen
    const std::size_t l = a.lumps[4].size() / 48;
    const int fls = i32(b.lumps[4], l * 48 + 32);
    const int nls = i32(b.lumps[4], l * 48 + 36);
    bool blatt = nls == fl && fls == static_cast<int>(a.lumps[5].size() / 4);
    for (int i = 0; blatt && i < nls; ++i) {
        const int v = i32(b.lumps[5], static_cast<std::size_t>(fls + i) * 4);
        blatt = v >= ersteFl && v < ersteFl + fl;
    }
    pruefe(blatt, "Blatt des Modells (EmitLeaf): " + std::to_string(nls) + " Blattflaechen auf die neuen Flaechen");
    // CM-Port laedt; Grenzen und externe Lightmaps
    bhed::KollisionsKarte k;
    pruefe(bhed::leseKollision(r.bsp, k) && k.modelle.size() == b.lumps[7].size() / 40, "der CM-Port laedt die gemischte Karte");
    std::string voll = a.lumps[14];
    voll.resize(256 * 49152, '\0');
    const MergeErgebnis g = bhed::karte::bspModelleAnhaengen(bhed::karte::bspMitLumps(orig, {{14, voll}}), teil, MergeSpiel::Jka, true);
    pruefe(!g.ok() && hat(g, true, "grenze", "MAX_LIGHTMAPS"), "Grenze: 256 Lightmap-Seiten in der Karte + neue -> Fehler MAX_LIGHTMAPS");
    const MergeErgebnis x = bhed::karte::bspModelleAnhaengen(bhed::karte::bspMitLumps(orig, {{14, std::string()}}), teil, MergeSpiel::Jka, true);
    pruefe(!x.ok() && hat(x, true, "lightmap", "external"), "Karte mit externen Lightmaps (Lump leer, Flaechen mit Nummern): Fehler \"lightmap\"");
    return g_fehler == 0 ? 0 : 1;
}

// bspmergetest befehle <q3map2.exe> <basispfad> <original.bsp> <teillicht.map>: die Befehle des Licht-Teils (teilLichtBefehle mit
// "_q3map2_cmdline" der Karte), je Zeile einer
int befehle(int argc, char** argv) {
    if (argc < 6) { return 2; }
    BspDatei d;
    if (!bhed::karte::bspLesen(lies(argv[4]), d)) { return 1; }
    std::vector<bhed::MapEntity> ents;
    bhed::parseEntities(bhed::karte::bspEntitaetenText(d), ents);
    const std::string* cmd = ents.empty() ? nullptr : ents[0].find("_q3map2_cmdline");
    bhed::karte::Q3map2Aufruf q;
    q.programm = argv[2];
    q.spielTyp = "ja";
    q.enginePfad = argv[3];
    q.fsGame = "base";
    std::printf("cmdline %s\n", cmd != nullptr ? cmd->c_str() : "");
    for (const std::string& b : bhed::karte::teilLichtBefehle(q, argv[5], cmd != nullptr ? *cmd : std::string())) { std::printf("befehl %s\n", b.c_str()); }
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc >= 2 && std::strcmp(argv[1], "einheit") == 0) { return einheit(); }
    if (argc >= 2 && std::strcmp(argv[1], "pruefen") == 0) { return pruefen(argc, argv); }
    if (argc >= 2 && std::strcmp(argv[1], "welt") == 0) { return welt(argc, argv); }
    if (argc >= 2 && std::strcmp(argv[1], "licht") == 0) { return licht(argc, argv); }
    if (argc >= 2 && std::strcmp(argv[1], "befehle") == 0) { return befehle(argc, argv); }
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
