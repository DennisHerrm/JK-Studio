// presetbau.cpp - baut einen Eintrag der Preset-Bibliothek (data/presets) aus einer freien Animationsdatei
//
//   presetbau <eingabe> <animation> <aus.bvh> [--fps N] [--von A --bis B] [--zyklus] [--stelle] [--einmal]
//
// <animation>  Name der Animation in der Datei (wie animImportieren sie nennt) oder #n (n-te, ab 0)
// --fps N      abtasten mit N Bildern je Sekunde (animImportieren fpsVorgabe), sonst die der Datei
// --von/--bis  Bilder innerhalb der Animation (Ausschnitt einer langen Aufnahme, z. B. 100STYLE Frame_Cuts.csv)
// --zyklus     in [von, bis) den Zyklus [a, b) suchen, dessen Ende b am besten zum Anfang a passt (Stellung aller Knochen
//              ohne die waagrechte Lage der Wurzel, dazu Hoehe und Tempo der Wurzel), Laenge 0,6 bis 1,6 s (ein
//              Doppelschritt beim Gehen/Laufen: Perry/Burnfield bzw. Novacheck wie retarget.h); ohne --zyklus der ganze
//              Ausschnitt (fertige Schleifen der Quelle, das letzte Bild weg, wenn es dem ersten gleicht)
// --stelle     auf der Stelle wie die Gaenge des Spiels (die Figur bewegt das Spiel): der waagrechte Weg der Wurzel ueber den
//              Zyklus wird gleichmaessig abgezogen
// --einmal     keine Schleife (Angriff, Tod ...): der Ausschnitt unveraendert, ohne Naht-Ausgleich
// Die Naht (Bild b gegen Bild a) wird gleichmaessig ueber den Zyklus verteilt (je Knochen die lokale Drehung/Lage linear
// zur Abweichung am Ende), danach ist Bild b = Bild a, der Zyklus nahtlos. Ausgabe als BVH (schreibBvh).
#include "bhed/gla.h"
#include "bhed/modell/animimport.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

using namespace bhed;
using namespace bhed::modell;

namespace {
struct Q {
    double w = 1, x = 0, y = 0, z = 0;
};
Q ausMatrix(const BoneMatrix& m) {
    Q q;
    const double t = m.m[0][0] + m.m[1][1] + m.m[2][2];
    if (t > 0) {
        const double s = std::sqrt(t + 1.0) * 2;
        q.w = 0.25 * s;
        q.x = (m.m[2][1] - m.m[1][2]) / s;
        q.y = (m.m[0][2] - m.m[2][0]) / s;
        q.z = (m.m[1][0] - m.m[0][1]) / s;
    } else if (m.m[0][0] > m.m[1][1] && m.m[0][0] > m.m[2][2]) {
        const double s = std::sqrt(1.0 + m.m[0][0] - m.m[1][1] - m.m[2][2]) * 2;
        q.w = (m.m[2][1] - m.m[1][2]) / s;
        q.x = 0.25 * s;
        q.y = (m.m[0][1] + m.m[1][0]) / s;
        q.z = (m.m[0][2] + m.m[2][0]) / s;
    } else if (m.m[1][1] > m.m[2][2]) {
        const double s = std::sqrt(1.0 + m.m[1][1] - m.m[0][0] - m.m[2][2]) * 2;
        q.w = (m.m[0][2] - m.m[2][0]) / s;
        q.x = (m.m[0][1] + m.m[1][0]) / s;
        q.y = 0.25 * s;
        q.z = (m.m[1][2] + m.m[2][1]) / s;
    } else {
        const double s = std::sqrt(1.0 + m.m[2][2] - m.m[0][0] - m.m[1][1]) * 2;
        q.w = (m.m[1][0] - m.m[0][1]) / s;
        q.x = (m.m[0][2] + m.m[2][0]) / s;
        q.y = (m.m[1][2] + m.m[2][1]) / s;
        q.z = 0.25 * s;
    }
    return q;
}
void inMatrix(const Q& q0, BoneMatrix& m) {
    const double n = std::sqrt(q0.w * q0.w + q0.x * q0.x + q0.y * q0.y + q0.z * q0.z);
    const Q q{q0.w / n, q0.x / n, q0.y / n, q0.z / n};
    m.m[0][0] = static_cast<float>(1 - 2 * (q.y * q.y + q.z * q.z));
    m.m[0][1] = static_cast<float>(2 * (q.x * q.y - q.z * q.w));
    m.m[0][2] = static_cast<float>(2 * (q.x * q.z + q.y * q.w));
    m.m[1][0] = static_cast<float>(2 * (q.x * q.y + q.z * q.w));
    m.m[1][1] = static_cast<float>(1 - 2 * (q.x * q.x + q.z * q.z));
    m.m[1][2] = static_cast<float>(2 * (q.y * q.z - q.x * q.w));
    m.m[2][0] = static_cast<float>(2 * (q.x * q.z - q.y * q.w));
    m.m[2][1] = static_cast<float>(2 * (q.y * q.z + q.x * q.w));
    m.m[2][2] = static_cast<float>(1 - 2 * (q.x * q.x + q.y * q.y));
}
Q mal(const Q& a, const Q& b) {
    return {a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z, a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y, a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
            a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w};
}
Q konj(const Q& a) { return {a.w, -a.x, -a.y, -a.z}; }
// Anteil t der Drehung q (von der Einheit aus)
Q teil(Q q, double t) {
    if (q.w < 0) { q = {-q.w, -q.x, -q.y, -q.z}; }
    const double w = std::acos(std::clamp(q.w, -1.0, 1.0));
    const double s = std::sin(w);
    if (s < 1e-9) { return {}; }
    const double f = std::sin(w * t) / s;
    return {std::cos(w * t), q.x * f, q.y * f, q.z * f};
}
double winkel(const Q& a, const Q& b) {
    const double d = std::fabs(a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z);
    return 2.0 * std::acos(std::clamp(d, 0.0, 1.0));
}
}  // namespace

int main(int argc, char** argv) {
    if (argc < 4) {
        std::fprintf(stderr, "presetbau <eingabe> <animation|#n> <aus.bvh> [--fps N] [--von A --bis B] [--zyklus] [--stelle]\n");
        return 2;
    }
    int fps = 0;
    int von = -1;
    int bis = -1;
    bool zyklus = false;
    bool stelle = false;
    bool einmal = false;
    for (int i = 4; i < argc; ++i) {
        if (std::strcmp(argv[i], "--fps") == 0 && i + 1 < argc) { fps = std::atoi(argv[++i]); }
        else if (std::strcmp(argv[i], "--von") == 0 && i + 1 < argc) { von = std::atoi(argv[++i]); }
        else if (std::strcmp(argv[i], "--bis") == 0 && i + 1 < argc) { bis = std::atoi(argv[++i]); }
        else if (std::strcmp(argv[i], "--zyklus") == 0) { zyklus = true; }
        else if (std::strcmp(argv[i], "--stelle") == 0) { stelle = true; }
        else if (std::strcmp(argv[i], "--einmal") == 0) { einmal = true; }
    }
    AnimImport imp;
    std::string fehler;
    if (!animImportieren(argv[1], imp, &fehler, fps)) {
        std::fprintf(stderr, "FEHLER %s\n", fehler.c_str());
        return 1;
    }
    const std::string wahl = argv[2];
    const AnimEntry* e = nullptr;
    for (std::size_t i = 0; i < imp.sequenzen.size(); ++i) {
        if (imp.sequenzen[i].name == wahl || (wahl.size() > 1 && wahl[0] == '#' && std::atoi(wahl.c_str() + 1) == static_cast<int>(i))) { e = &imp.sequenzen[i]; }
    }
    if (e == nullptr) {
        std::fprintf(stderr, "FEHLER Animation %s nicht da; vorhanden:\n", wahl.c_str());
        for (const AnimEntry& s : imp.sequenzen) { std::fprintf(stderr, "  %s %d Bilder %d fps\n", s.name.c_str(), s.numFrames, s.fps); }
        return 1;
    }
    GlaAnimation& g = imp.anim;
    const std::size_t nb = g.bones.size();
    const int f0 = e->firstFrame + std::max(von, 0);
    const int f1 = e->firstFrame + (bis > 0 ? std::min(bis, e->numFrames) : e->numFrames);
    int a = f0;
    int b = f1 - 1;   // b = Bild, das dem Anfang a gleichen soll (gehoert nicht mehr zum Zyklus)
    int wurzel = 0;
    for (std::size_t k = 0; k < nb; ++k) {
        if (g.bones[k].parent < 0) { wurzel = static_cast<int>(k); }
    }
    auto lok = [&](int f, std::size_t k) -> BoneMatrix& { return g.roh[static_cast<std::size_t>(f) * nb + k]; };
    auto abstand = [&](int x, int y) {
        double d = 0.0;
        for (std::size_t k = 0; k < nb; ++k) { d += winkel(ausMatrix(lok(x, k)), ausMatrix(lok(y, k))); }
        const auto w = static_cast<std::size_t>(wurzel);
        d += std::fabs(lok(x, w).m[2][3] - lok(y, w).m[2][3]) * 0.05;   // Hoehe (cm -> grob Bogenmass je 20 cm)
        if (x + 1 < f1 && y + 1 < f1) {
            for (int i = 0; i < 3; ++i) {
                d += std::fabs((lok(x + 1, w).m[i][3] - lok(x, w).m[i][3]) - (lok(y + 1, w).m[i][3] - lok(y, w).m[i][3])) * 0.05;
            }
        }
        return d;
    };
    if (einmal) {
        std::string bvh;
        if (!schreibBvh(g, f0, f1 - f0, e->fps, bvh, &fehler)) {
            std::fprintf(stderr, "FEHLER %s\n", fehler.c_str());
            return 1;
        }
        std::ofstream o(argv[3], std::ios::binary);
        o << bvh;
        std::printf("%s: %s %d Bilder (einmal), %d fps, %zu Knochen, %zu Bytes\n", argv[3], e->name.c_str(), f1 - f0, e->fps, nb, bvh.size());
        return 0;
    }
    if (zyklus) {
        const int lo = std::max(1, static_cast<int>(std::lround(0.6 * e->fps)));
        const int hi = std::max(lo + 1, static_cast<int>(std::lround(1.6 * e->fps)));
        double best = 1e30;
        // Anfang in der Mitte des Ausschnitts (gerade Strecke, weg vom Wenden am Rand des Aufnahmeraums)
        const int mitte = (f0 + f1) / 2 - hi / 2;
        for (int x = std::max(f0, mitte - hi); x <= std::min(f1 - hi - 2, mitte + hi); ++x) {
            for (int y = x + lo; y <= x + hi && y + 1 < f1; ++y) {
                const double d = abstand(x, y);
                if (d < best) {
                    best = d;
                    a = x;
                    b = y;
                }
            }
        }
        std::printf("Zyklus %d..%d (%d Bilder), Abstand %.4f\n", a, b, b - a, best);
    } else {
        // fertige Schleife der Quelle: gleicht das letzte Bild dem ersten (jeder Knochen unter 2 Grad), ist es das Bild b = a
        // und faellt weg; sonst gehoeren alle Bilder zum Zyklus, und b ist das gedachte Bild danach - der letzte Schritt
        // fortgesetzt (Drehung D = A(n-1) A(n-2)^-1 noch einmal, Lage ebenso), damit der Ausgleich unten eine Zielstellung hat
        double groesster = 0.0;
        for (std::size_t k = 0; k < nb; ++k) { groesster = std::max(groesster, winkel(ausMatrix(lok(f0, k)), ausMatrix(lok(f1 - 1, k)))); }
        // den Zyklus hinter alle Bilder kopieren (die naechste Animation der Datei bleibt unberuehrt)
        const int neu = g.numFrames;
        const int zahl = groesster < 2.0 * 3.14159265358979 / 180.0 ? f1 - f0 : f1 - f0 + 1;
        for (int f = f0; f < f0 + zahl && f < f1; ++f) {
            for (std::size_t k = 0; k < nb; ++k) { g.roh.push_back(lok(f, k)); }
        }
        if (zahl > f1 - f0) {
            for (std::size_t k = 0; k < nb; ++k) {
                const BoneMatrix l1 = lok(f1 - 1, k);
                const BoneMatrix l2 = lok(f1 - 2, k);
                BoneMatrix x = l1;
                inMatrix(mal(mal(ausMatrix(l1), konj(ausMatrix(l2))), ausMatrix(l1)), x);
                for (int i = 0; i < 3; ++i) { x.m[i][3] = 2.0F * l1.m[i][3] - l2.m[i][3]; }
                g.roh.push_back(x);
            }
        }
        g.numFrames += zahl;
        a = neu;
        b = neu + zahl - 1;
        std::printf("Schleife der Quelle: letztes Bild %s (groesste Abweichung %.2f Grad)\n", zahl == f1 - f0 ? "= erstes, faellt weg" : "fortgesetzt",
                    groesster * 180.0 / 3.14159265358979);
    }
    const int n = b - a;
    if (n < 2) {
        std::fprintf(stderr, "FEHLER zu kurz\n");
        return 1;
    }
    const auto w = static_cast<std::size_t>(wurzel);
    if (stelle) {
        // waagrechter Weg der Wurzel gleichmaessig abziehen, Anfang auf den Ursprung (X/Y, JKA oben = Z)
        const float sx = lok(a, w).m[0][3];
        const float sy = lok(a, w).m[1][3];
        const float dx = lok(b, w).m[0][3] - sx;
        const float dy = lok(b, w).m[1][3] - sy;
        for (int f = a; f <= b; ++f) {
            const float t = static_cast<float>(f - a) / static_cast<float>(n);
            lok(f, w).m[0][3] -= sx + dx * t;
            lok(f, w).m[1][3] -= sy + dy * t;
        }
    }
    // Naht verteilen: A'(f) = Teil(R, t) A(f), R = A(a) A(b)^-1 (Drehung), Lage linear
    for (std::size_t k = 0; k < nb; ++k) {
        const Q qa = ausMatrix(lok(a, k));
        const Q qb = ausMatrix(lok(b, k));
        const Q r = mal(qa, konj(qb));
        float da[3];
        for (int i = 0; i < 3; ++i) { da[i] = lok(a, k).m[i][3] - lok(b, k).m[i][3]; }
        for (int f = a; f <= b; ++f) {
            const double t = static_cast<double>(f - a) / n;
            BoneMatrix& m = lok(f, k);
            inMatrix(mal(teil(r, t), ausMatrix(m)), m);
            for (int i = 0; i < 3; ++i) { m.m[i][3] += std::fabs(da[i]) > 1e-3F ? static_cast<float>(t) * da[i] : 0.0F; }   // Rauschen unter 0,01 mm bleibt
        }
    }
    std::string bvh;
    if (!schreibBvh(g, a, n, e->fps, bvh, &fehler)) {
        std::fprintf(stderr, "FEHLER %s\n", fehler.c_str());
        return 1;
    }
    std::ofstream o(argv[3], std::ios::binary);
    o << bvh;
    std::printf("%s: %s %d Bilder, %d fps, %zu Knochen, %zu Bytes\n", argv[3], e->name.c_str(), n, e->fps, nb, bvh.size());
    return 0;
}
