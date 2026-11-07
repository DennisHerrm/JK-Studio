// ctest modell_malen - Modell-Block 1c: Texture Painting (bhed/modell/malen.h)
//
//   Strich auf bekannter UV   Quadrat 64 x 64 Einheiten auf einem 64er Bild (1 Einheit = 1 Texel): genau die Texel mit
//                             Mitte im Radius sind getroffen, harter Rand (Abfall), Abstand im Strich
//   Ausbluten                 ein halbes Bild bedeckt: die 4 Texel daneben tragen die Farbe des Rands, das 5. nicht
//   Spiegeln                  Modell von x -32 bis 32: Tupfer bei +10 und gespiegelt -> die Texel spiegelbildlich
//   Rueckgaengig              in Kacheln: zurueck = vorher Byte fuer Byte, wiederholen = nachher
//   Ebenen                    Multiplizieren mit Deckkraft 0,5; Maske schuetzt; Fuellen, Klonen, Verwischen, Weichzeichnen
//   Umbacken                  verschobene UV: die Farbe folgt
//   Speichern/Laden           TGA (24/32, roh/RLE), JPEG, PNG: unbemalt bytegleich, bemalt im Format des Originals, JPEG mit
//                             den Tabellen des Originals, wieder gelesen gleich bzw. nahe
#include "bhed/image.h"
#include "bhed/modell/malen.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <set>
#include <string>
#include <vector>

namespace {
namespace m = bhed::modell::texmalen;
int fails = 0;

void pruefe(bool ok, const char* was) {
    std::printf("%s %s\n", ok ? "  ok    " : "  FEHLER", was);
    if (!ok) { ++fails; }
}

std::size_t uz(int i) { return static_cast<std::size_t>(i); }

// Quadrat (0,0)-(n,n) in der Ebene z = 0, UV 0..1
std::vector<m::Dreieck> quadrat(float x0, float n) {
    m::Dreieck a;
    m::Dreieck b;
    const float uv[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    const float o[4][3] = {{x0, 0, 0}, {x0 + n, 0, 0}, {x0 + n, n, 0}, {x0, n, 0}};
    const int ia[3] = {0, 1, 2};
    const int ib[3] = {0, 2, 3};
    for (int e = 0; e < 3; ++e) {
        for (int k = 0; k < 2; ++k) {
            a.uv[e][k] = uv[ia[e]][k];
            b.uv[e][k] = uv[ib[e]][k];
        }
        for (int k = 0; k < 3; ++k) {
            a.ort[e][k] = o[ia[e]][k];
            b.ort[e][k] = o[ib[e]][k];
        }
    }
    return {a, b};
}

const std::uint8_t* px(const m::Leinwand& l, int x, int y) { return &l.bild[l.texel(x, y) * 4U]; }

void strichTest() {
    m::Leinwand l;
    m::leinwandLeer(l, 64, 64, {0, 0, 0, 255});
    m::dreieckeSetzen(l, quadrat(0.0F, 64.0F), true);
    m::Verlauf v;
    m::Pinsel p;
    p.radius = 8.0F;
    p.haerte = 1.0F;
    p.farbe = {1.0F, 0.0F, 0.0F, 1.0F};
    m::schrittBeginnen(v, l, "strich");
    const float mitte[3] = {32.0F, 32.0F, 0.0F};
    const int n = m::tupfer3d(l, v, p, mitte, 1.0F, nullptr, nullptr);
    m::schrittEnde(v, l);
    int erwartet = 0;
    int falsch = 0;
    for (int y = 0; y < 64; ++y) {
        for (int x = 0; x < 64; ++x) {
            const bool drin = std::hypot(static_cast<float>(x) + 0.5F - 32.0F, static_cast<float>(y) + 0.5F - 32.0F) < 8.0F;
            erwartet += drin ? 1 : 0;
            const bool rot = px(l, x, y)[0] == 255 && px(l, x, y)[1] == 0;
            falsch += (drin != rot) ? 1 : 0;
        }
    }
    char t[160];
    std::snprintf(t, sizeof t, "Tupfer r 8 harte Kante: %d Texel getroffen, erwartet %d, %d abweichend", n, erwartet, falsch);
    pruefe(n == erwartet && falsch == 0, t);
    // weiche Kante: Mitte voll, Rand schwaecher, ausserhalb nichts
    m::leinwandLeer(l, 64, 64, {0, 0, 0, 255});
    m::dreieckeSetzen(l, quadrat(0.0F, 64.0F), true);
    p.haerte = 0.0F;
    m::schrittBeginnen(v, l, "weich");
    (void)m::tupfer3d(l, v, p, mitte, 1.0F, nullptr, nullptr);
    m::schrittEnde(v, l);
    const int innen = px(l, 32, 32)[0];
    const int rand = px(l, 38, 32)[0];
    const int aussen = px(l, 41, 32)[0];
    std::snprintf(t, sizeof t, "weicher Abfall: Mitte %d > Rand %d > aussen %d = 0", innen, rand, aussen);
    pruefe(innen > 240 && rand > 0 && rand < innen && aussen == 0, t);
    // Druck halbiert die Staerke
    m::leinwandLeer(l, 64, 64, {0, 0, 0, 255});
    m::dreieckeSetzen(l, quadrat(0.0F, 64.0F), true);
    p.haerte = 1.0F;
    m::schrittBeginnen(v, l, "druck");
    (void)m::tupfer3d(l, v, p, mitte, 0.5F, nullptr, nullptr);
    m::schrittEnde(v, l);
    std::snprintf(t, sizeof t, "Druck 0,5: Rot %d (erwartet 128)", px(l, 32, 32)[0]);
    pruefe(std::abs(px(l, 32, 32)[0] - 128) <= 1, t);
    // Abstand im Strich: 10 Einheiten mit Schritt 2 -> Tupfer bei 0, 2, 4, 6, 8, 10
    m::Strich s;
    const float a[3] = {0, 0, 0};
    const float b[3] = {10, 0, 0};
    std::size_t z = s.weiter(a, 2.0F).size();
    const auto weg = s.weiter(b, 2.0F);
    z += weg.size();
    std::snprintf(t, sizeof t, "Abstand: %zu Tupfer auf 10 Einheiten mit Schritt 2 (erwartet 6), letzter bei %.2f", z,
                  weg.empty() ? -1.0 : static_cast<double>(weg.back()[0]));
    pruefe(z == 6U && !weg.empty() && std::fabs(weg.back()[0] - 10.0F) < 1e-4F, t);
    // abgewandte Dreiecke: nichts
    m::leinwandLeer(l, 64, 64, {0, 0, 0, 255});
    m::dreieckeSetzen(l, quadrat(0.0F, 64.0F), true);
    const std::vector<std::uint8_t> weg0(2, 0);
    m::schrittBeginnen(v, l, "rueck");
    const int nr = m::tupfer3d(l, v, p, mitte, 1.0F, &weg0, nullptr);
    m::schrittEnde(v, l);
    pruefe(nr == 0, "abgewandte Dreiecke werden ohne 'Rueckseiten' nicht bemalt");
}

void blutenTest() {
    m::Leinwand l;
    m::leinwandLeer(l, 64, 64, {0, 0, 0, 255});
    // nur die linke Haelfte bedeckt (u 0..0,5)
    auto q = quadrat(0.0F, 32.0F);
    for (m::Dreieck& d : q) {
        for (auto& e : d.uv) { e[0] *= 0.5F; }
    }
    m::dreieckeSetzen(l, q, true);
    m::Verlauf v;
    m::Pinsel p;
    p.radius = 100.0F;
    p.haerte = 1.0F;
    p.farbe = {0.0F, 1.0F, 0.0F, 1.0F};
    const float mitte[3] = {16.0F, 16.0F, 0.0F};
    m::schrittBeginnen(v, l, "bluten");
    (void)m::tupfer3d(l, v, p, mitte, 1.0F, nullptr, nullptr);
    m::schrittEnde(v, l);
    // Spalte 31 ist die letzte bedeckte (Mitte 31,5 < 32), 32..35 geblutet, 36 nicht
    int geblutet = 0;
    for (int x = 32; x <= 35; ++x) { geblutet += px(l, x, 20)[1] == 255 ? 1 : 0; }
    char t[160];
    std::snprintf(t, sizeof t, "Ausbluten 4: Spalten 32..35 gruen %d/4, Spalte 36 G %d (0), Spalte 31 G %d (255)", geblutet, px(l, 36, 20)[1], px(l, 31, 20)[1]);
    pruefe(geblutet == 4 && px(l, 36, 20)[1] == 0 && px(l, 31, 20)[1] == 255, t);
}

void spiegelTest() {
    m::Leinwand l;
    m::leinwandLeer(l, 64, 64, {0, 0, 0, 255});
    m::dreieckeSetzen(l, quadrat(-32.0F, 64.0F), true);
    m::Verlauf v;
    m::Pinsel p;
    p.radius = 4.0F;
    p.haerte = 1.0F;
    p.farbe = {0.0F, 0.0F, 1.0F, 1.0F};
    const float a[3] = {10.0F, 20.0F, 0.0F};
    float b[3];
    m::spiegelPunkt(a, 0, b);
    m::schrittBeginnen(v, l, "spiegeln");
    (void)m::tupfer3d(l, v, p, a, 1.0F, nullptr, nullptr);
    (void)m::tupfer3d(l, v, p, b, 1.0F, nullptr, nullptr);
    m::schrittEnde(v, l);
    int ungleich = 0;
    int blau = 0;
    for (int y = 0; y < 64; ++y) {
        for (int x = 0; x < 64; ++x) {
            const bool hier = px(l, x, y)[2] == 255;
            const bool dort = px(l, 63 - x, y)[2] == 255;
            ungleich += hier != dort ? 1 : 0;
            blau += hier ? 1 : 0;
        }
    }
    char t[160];
    std::snprintf(t, sizeof t, "Spiegeln an x: %d Texel blau, %d nicht spiegelbildlich, Gegenpunkt x %.1f", blau, ungleich, static_cast<double>(b[0]));
    pruefe(blau > 0 && ungleich == 0 && b[0] == -10.0F, t);
}

void undoTest() {
    m::Leinwand l;
    m::leinwandLeer(l, 200, 130, {10, 20, 30, 255});
    m::dreieckeSetzen(l, quadrat(0.0F, 200.0F), true);
    m::Verlauf v;
    m::Pinsel p;
    p.radius = 30.0F;
    p.farbe = {1.0F, 1.0F, 0.0F, 1.0F};
    const auto vorher = l.ebenen[0].rgba;
    m::schrittBeginnen(v, l, "a");
    const float c[3] = {100.0F, 60.0F, 0.0F};
    (void)m::tupfer3d(l, v, p, c, 1.0F, nullptr, nullptr);
    const float d[3] = {130.0F, 60.0F, 0.0F};
    (void)m::tupfer3d(l, v, p, d, 1.0F, nullptr, nullptr);
    m::schrittEnde(v, l);
    const auto nachher = l.ebenen[0].rgba;
    const std::size_t kacheln = v.zurueck.empty() ? 0U : v.zurueck.back().kacheln.size();
    const bool zur = m::rueckgaengig(v, l) && l.ebenen[0].rgba == vorher && l.bild == vorher;
    const bool wie = m::wiederholen(v, l) && l.ebenen[0].rgba == nachher;
    char t[160];
    std::snprintf(t, sizeof t, "Rueckgaengig in Kacheln: %zu Kacheln (von 12), zurueck bytegleich %d, wiederholen bytegleich %d", kacheln, zur ? 1 : 0, wie ? 1 : 0);
    pruefe(kacheln > 0U && kacheln < 12U && zur && wie, t);
}

// Rueckgaengig mit Ausbluten und einer halb bedeckten Leinwand (wie eine Figur): zurueck = vorher Byte fuer Byte; die Maske
// schuetzt auch die ausgebluteten Texel
void undoBlutTest() {
    m::Leinwand l;
    m::leinwandLeer(l, 128, 128, {40, 50, 60, 255});
    auto q = quadrat(0.0F, 64.0F);
    for (m::Dreieck& d : q) {
        for (auto& e : d.uv) { e[0] = e[0] * 0.5F + 0.1F; }
    }
    m::dreieckeSetzen(l, q, true);
    m::Verlauf v;
    m::Pinsel p;
    p.radius = 20.0F;
    p.farbe = {1.0F, 1.0F, 1.0F, 1.0F};
    const auto vorher = l.ebenen[0].rgba;
    m::schrittBeginnen(v, l, "strich");
    for (int i = 0; i < 20; ++i) {
        const float c[3] = {static_cast<float>(i) * 3.0F, 32.0F, 0.0F};
        (void)m::tupfer3d(l, v, p, c, 1.0F, nullptr, nullptr);
    }
    m::schrittEnde(v, l);
    const bool zur = m::rueckgaengig(v, l);
    int rest = 0;
    for (std::size_t i = 0; i < vorher.size(); ++i) { rest += l.ebenen[0].rgba[i] != vorher[i] ? 1 : 0; }
    char t[200];
    std::snprintf(t, sizeof t, "Rueckgaengig mit Ausbluten (halb bedeckt): %zu Schritt(e), zurueck %d, %d Bytes anders", v.vor.size(), zur ? 1 : 0, rest);
    pruefe(zur && rest == 0 && v.zurueck.empty(), t);
    // Maske ueber alles: auch die ausgebluteten Texel bleiben (Grund mit Muster, sonst saehe man das Bluten nicht)
    for (std::size_t i = 0; i < l.ebenen[0].rgba.size(); ++i) { l.ebenen[0].rgba[i] = static_cast<std::uint8_t>((i * 7U) % 251U); }
    const auto muster0 = l.ebenen[0].rgba;
    std::fill(l.maske.begin(), l.maske.end(), std::uint8_t{255});
    m::schrittBeginnen(v, l, "maske");
    const float c[3] = {0.0F, 32.0F, 0.0F};
    (void)m::tupfer3d(l, v, p, c, 1.0F, nullptr, nullptr);
    m::schrittEnde(v, l);
    rest = 0;
    for (std::size_t i = 0; i < muster0.size(); ++i) { rest += l.ebenen[0].rgba[i] != muster0[i] ? 1 : 0; }
    std::snprintf(t, sizeof t, "Maske ueberall: %d Bytes bemalt (auch nicht ausgeblutet)", rest);
    pruefe(rest == 0, t);
}

void ebenenTest() {
    m::Leinwand l;
    m::leinwandLeer(l, 32, 32, {200, 100, 50, 255});
    m::dreieckeSetzen(l, quadrat(0.0F, 32.0F), true);
    m::Verlauf v;
    std::vector<m::Ebene> st = l.ebenen;
    m::Ebene e;
    e.name = "mal";
    e.rgba.assign(32U * 32U * 4U, 0);
    e.misch = m::Mischart::Multiplizieren;
    e.deckkraft = 0.5F;
    st.push_back(e);
    m::stapelAendern(v, l, "neue Ebene", st, 1);
    m::Pinsel p;
    p.radius = 100.0F;
    p.haerte = 1.0F;
    p.farbe = {0.5F, 0.5F, 0.5F, 1.0F};
    m::schrittBeginnen(v, l, "mal");
    const float c[3] = {16, 16, 0};
    (void)m::tupfer3d(l, v, p, c, 1.0F, nullptr, nullptr);
    m::schrittEnde(v, l);
    // multiplizieren mit 0,5 bei Deckkraft 0,5: 200 -> 200 * (0,5 + 0,5 * 0,5) = 150
    const int r = px(l, 5, 5)[0];
    char t[200];
    std::snprintf(t, sizeof t, "Ebene Multiplizieren, Deckkraft 0,5: R %d (erwartet 150), Original unveraendert %d", r, l.ebenen[0].rgba[0]);
    pruefe(std::abs(r - 150) <= 1 && l.ebenen[0].rgba[0] == 200, t);
    // Ebene aus: Original sichtbar
    st = l.ebenen;
    st[1].sichtbar = false;
    m::stapelAendern(v, l, "aus", st, 1);
    pruefe(px(l, 5, 5)[0] == 200, "Ebene ausgeblendet: das Ergebnis ist wieder das Original");
    (void)m::rueckgaengig(v, l);
    pruefe(std::abs(px(l, 5, 5)[0] - 150) <= 1, "Rueckgaengig stellt die Sichtbarkeit wieder her");
    // Maske schuetzt die linke Haelfte
    m::Pinsel mk;
    mk.werkzeug = m::Werkzeug::Maske;
    mk.radius = 1.0F;
    mk.haerte = 1.0F;
    l.aktiv = 0;
    m::schrittBeginnen(v, l, "maske");
    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 16; ++x) { (void)m::tupfer2d(l, v, mk, (static_cast<float>(x) + 0.5F) / 32.0F, (static_cast<float>(y) + 0.5F) / 32.0F, 1.0F, nullptr); }
    }
    m::schrittEnde(v, l);
    m::Pinsel f;
    f.werkzeug = m::Werkzeug::Fuellen;
    f.farbe = {0.0F, 0.0F, 1.0F, 1.0F};
    f.fuellToleranz = 0.05F;
    m::schrittBeginnen(v, l, "fuellen");
    const int gefuellt = m::fuellen(l, v, f, 20, 20);
    m::schrittEnde(v, l);
    std::snprintf(t, sizeof t, "Maske links + Fuellen rechts: %d Texel gefuellt, links B %d (unveraendert 50), rechts B %d (255)", gefuellt,
                  l.ebenen[0].rgba[l.texel(4, 10) * 4U + 2U], l.ebenen[0].rgba[l.texel(25, 10) * 4U + 2U]);
    pruefe(l.ebenen[0].rgba[l.texel(4, 10) * 4U + 2U] == 50 && l.ebenen[0].rgba[l.texel(25, 10) * 4U + 2U] == 255, t);
}

void werkzeugTest() {
    // Klonen: links rot, rechts schwarz; Quelle 16 Texel links -> rechts wird rot
    m::Leinwand l;
    m::leinwandLeer(l, 32, 32, {0, 0, 0, 255});
    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 16; ++x) {
            l.ebenen[0].rgba[l.texel(x, y) * 4U] = 255;
        }
    }
    m::zusammensetzen(l);
    m::dreieckeSetzen(l, quadrat(0.0F, 32.0F), true);
    m::Verlauf v;
    m::Pinsel k;
    k.werkzeug = m::Werkzeug::Klonen;
    k.radius = 3.0F;
    k.haerte = 1.0F;
    k.klonVersatz[0] = -16.0F / 32.0F;
    m::schrittBeginnen(v, l, "klon");
    (void)m::tupfer2d(l, v, k, 24.5F / 32.0F, 10.5F / 32.0F, 1.0F, nullptr);
    m::schrittEnde(v, l);
    pruefe(px(l, 24, 10)[0] == 255 && px(l, 28, 10)[0] == 0, "Klonen: Quelle 16 Texel links wird rechts aufgetragen");
    // Weichzeichnen an der Kante: der Uebergang wird weich
    m::Pinsel w;
    w.werkzeug = m::Werkzeug::Weichzeichnen;
    w.radius = 3.0F;
    w.haerte = 1.0F;
    m::schrittBeginnen(v, l, "weich");
    (void)m::tupfer2d(l, v, w, 16.0F / 32.0F, 20.5F / 32.0F, 1.0F, nullptr);
    m::schrittEnde(v, l);
    const int a = px(l, 15, 20)[0];
    const int b = px(l, 16, 20)[0];
    char t[160];
    std::snprintf(t, sizeof t, "Weichzeichnen an der Kante: %d / %d (vorher 255 / 0)", a, b);
    pruefe(a > 0 && a < 255 && b > 0 && b < 255, t);
    // Verwischen nach rechts: Rot wandert ueber die Kante
    m::Leinwand l2;
    m::leinwandLeer(l2, 32, 32, {0, 0, 0, 255});
    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 16; ++x) { l2.ebenen[0].rgba[l2.texel(x, y) * 4U] = 255; }
    }
    m::zusammensetzen(l2);
    m::dreieckeSetzen(l2, quadrat(0.0F, 32.0F), true);
    m::Pinsel s;
    s.werkzeug = m::Werkzeug::Verwischen;
    s.radius = 3.0F;
    s.haerte = 1.0F;
    const float r[2] = {2.0F / 32.0F, 0.0F};
    m::schrittBeginnen(v, l2, "wisch");
    (void)m::tupfer2d(l2, v, s, 17.5F / 32.0F, 5.5F / 32.0F, 1.0F, r);
    m::schrittEnde(v, l2);
    pruefe(px(l2, 17, 5)[0] > 200 && px(l2, 13, 5)[0] == 255, "Verwischen: die Farbe links wird ueber die Kante gezogen");
}

void umbackTest() {
    // Bild: linke Haelfte weiss; neue UV um 0,25 nach rechts verschoben -> Weiss beginnt bei Spalte 16 statt 0
    std::vector<std::uint8_t> rgba(64U * 64U * 4U, 0);
    for (int y = 0; y < 64; ++y) {
        for (int x = 0; x < 32; ++x) {
            for (int k = 0; k < 4; ++k) { rgba[(uz(y) * 64U + uz(x)) * 4U + uz(k)] = 255; }
        }
    }
    auto alt = quadrat(0.0F, 1.0F);
    for (m::Dreieck& d : alt) {
        for (auto& e : d.uv) { e[0] *= 0.5F; }   // alte UV: u 0..0,5
    }
    auto neu = alt;
    for (m::Dreieck& d : neu) {
        for (auto& e : d.uv) { e[0] += 0.25F; }   // neue UV: u 0,25..0,75
    }
    const auto out = m::umbacken(rgba, 64, 64, alt, neu, 4);
    const int innen = out[(10U * 64U + 30U) * 4U];
    const int vorne = out[(10U * 64U + 10U) * 4U];
    const int weit = out[(10U * 64U + 60U) * 4U];
    char t[160];
    std::snprintf(t, sizeof t, "Umbacken: neue Insel 16..47 weiss (%d), davor schwarz (%d), weit rechts schwarz (%d)", innen, vorne, weit);
    pruefe(innen == 255 && vorne == 0 && weit == 0, t);
}

double mittel(const std::vector<std::uint8_t>& a, const std::vector<std::uint8_t>& b) {
    double s = 0.0;
    std::size_t n = 0;
    for (std::size_t i = 0; i < a.size() && i < b.size(); i += 4U) {
        for (std::size_t k = 0; k < 3U; ++k) {
            s += std::fabs(static_cast<double>(a[i + k]) - static_cast<double>(b[i + k]));
            ++n;
        }
    }
    return n > 0U ? s / static_cast<double>(n) : 0.0;
}

std::vector<std::uint8_t> muster(int b, int h) {
    std::vector<std::uint8_t> px(uz(b) * uz(h) * 4U);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < b; ++x) {
            std::uint8_t* p = &px[(uz(y) * uz(b) + uz(x)) * 4U];
            p[0] = static_cast<std::uint8_t>(x * 255 / std::max(1, b - 1));
            p[1] = static_cast<std::uint8_t>(y * 255 / std::max(1, h - 1));
            p[2] = static_cast<std::uint8_t>(((x / 8 + y / 8) % 2) * 120 + 60);
            p[3] = static_cast<std::uint8_t>(((x + y) % 3) == 0 ? 128 : 255);
            if (y > h / 2 && x > b / 2) {   // eine glatte Flaeche (RLE-Laeufe)
                p[0] = 30;
                p[1] = 40;
                p[2] = 50;
                p[3] = 255;
            }
        }
    }
    return px;
}

void dateiTest() {
    const int b = 100;
    const int h = 70;
    const auto bild = muster(b, h);
    char t[220];
    // TGA 32 roh, 32 RLE, 24 RLE: lesen -> unbemalt bytegleich; bemalt -> gleiche Tiefe/Kompression, gelesen gleich
    for (const auto& [bits, rle] : {std::pair{32, false}, std::pair{32, true}, std::pair{24, true}}) {
        const std::string datei = m::tgaSchreiben(bild.data(), b, h, bits, rle);
        const auto* d = reinterpret_cast<const unsigned char*>(datei.data());
        const bhed::image::Image im = bhed::image::decode(d, datei.size());
        bool gleich = im.ok && im.width == b && im.height == h;
        for (std::size_t i = 0; gleich && i < bild.size(); ++i) {
            gleich = im.rgba[i] == (bits == 24 && i % 4U == 3U ? 255 : bild[i]);
        }
        m::Leinwand l;
        std::string fehler;
        const bool geladen = m::leinwandAusDatei(l, "models/test/x.tga", datei, &fehler);
        m::dreieckeSetzen(l, quadrat(0.0F, 100.0F), true);
        std::string aus;
        const bool unbemalt = geladen && m::leinwandBytes(l, aus, nullptr) && aus == datei;
        m::Verlauf v;
        m::Pinsel p;
        p.radius = 10.0F;
        p.farbe = {1.0F, 0.0F, 1.0F, 1.0F};
        m::schrittBeginnen(v, l, "tga");
        const float c[3] = {50.0F, 35.0F, 0.0F};
        (void)m::tupfer3d(l, v, p, c, 1.0F, nullptr, nullptr);
        m::schrittEnde(v, l);
        std::string bemalt;
        (void)m::leinwandBytes(l, bemalt, nullptr);
        const auto* bd = reinterpret_cast<const unsigned char*>(bemalt.data());
        const bhed::image::Image wieder = bhed::image::decode(bd, bemalt.size());
        const bool form = bemalt.size() > 18U && bd[16] == bits && (bd[2] >= 9) == rle;
        bool treu = wieder.ok && wieder.width == b && wieder.height == h;
        for (std::size_t i = 0; treu && i < l.bild.size(); ++i) { treu = wieder.rgba[i] == (bits == 24 && i % 4U == 3U ? 255 : l.bild[i]); }
        std::snprintf(t, sizeof t, "TGA %d Bit %s: Leser %d, unbemalt bytegleich %d, bemalt %d Bit %s %d, gelesen gleich %d", bits, rle ? "RLE" : "roh", gleich ? 1 : 0,
                      unbemalt ? 1 : 0, bemalt.size() > 18U ? bd[16] : 0, rle ? "RLE" : "roh", form ? 1 : 0, treu ? 1 : 0);
        pruefe(gleich && unbemalt && form && treu, t);
    }
    // JPEG: eigener Schreiber -> eigener Leser nahe; Tabellen des Originals bleiben; unbemalt bytegleich
    {
        const std::string jpg = m::jpegSchreiben(bild.data(), b, h, {});
        const auto* d = reinterpret_cast<const unsigned char*>(jpg.data());
        const bhed::image::Image im = bhed::image::decode(d, jpg.size());
        const double f = im.ok ? mittel(im.rgba, bild) : 999.0;
        std::snprintf(t, sizeof t, "JPEG Qualitaet 90 geschrieben, gelesen: %dx%d, mittlere Abweichung %.2f (< 3)", im.width, im.height, f);
        pruefe(im.ok && im.width == b && im.height == h && f < 3.0, t);
        // "Original" mit groberen Tabellen (Anhang K, Skala 50 = die Tabelle selbst)
        std::vector<std::uint16_t> grob(128);
        const int zz[64] = {0,  1,  8,  16, 9,  2,  3,  10, 17, 24, 32, 25, 18, 11, 4,  5,  12, 19, 26, 33, 40, 48,
                            41, 34, 27, 20, 13, 6,  7,  14, 21, 28, 35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23,
                            30, 37, 44, 51, 58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63};
        const int luma[64] = {16, 11, 10, 16, 24,  40,  51,  61,  12, 12, 14, 19, 26,  58,  60,  55,  14, 13, 16, 24, 40, 57,
                              69, 56, 14, 17, 22,  29,  51,  87,  80, 62, 18, 22, 37,  56,  68,  109, 103, 77, 24, 35, 55, 64,
                              81, 104, 113, 92, 49, 64, 78, 87, 103, 121, 120, 101, 72, 92, 95, 98, 112, 100, 103, 99};
        for (int i = 0; i < 64; ++i) {
            grob[uz(i)] = static_cast<std::uint16_t>(luma[zz[i]]);
            grob[uz(64 + i)] = 99;
        }
        const std::string original = m::jpegSchreiben(bild.data(), b, h, grob);
        const bool tabGelesen = m::jpegQuant(original) == grob;
        m::Leinwand l;
        (void)m::leinwandAusDatei(l, "models/test/x.jpg", original, nullptr);
        m::dreieckeSetzen(l, quadrat(0.0F, 100.0F), true);
        std::string aus;
        const bool unbemalt = m::leinwandBytes(l, aus, nullptr) && aus == original;
        m::Verlauf v;
        m::Pinsel p;
        p.radius = 12.0F;
        p.farbe = {0.0F, 1.0F, 0.0F, 1.0F};
        m::schrittBeginnen(v, l, "jpg");
        const float c[3] = {30.0F, 30.0F, 0.0F};
        (void)m::tupfer3d(l, v, p, c, 1.0F, nullptr, nullptr);
        m::schrittEnde(v, l);
        std::string bemalt;
        (void)m::leinwandBytes(l, bemalt, nullptr);
        const auto* bd = reinterpret_cast<const unsigned char*>(bemalt.data());
        const bhed::image::Image wieder = bhed::image::decode(bd, bemalt.size());
        const bool istJpeg = bhed::image::sniff(bd, bemalt.size()) == bhed::image::Format::Jpeg;
        const bool tabGleich = m::jpegQuant(bemalt) == grob;
        const double f2 = wieder.ok ? mittel(wieder.rgba, l.bild) : 999.0;
        std::snprintf(t, sizeof t, "JPEG: Tabellen gelesen %d, unbemalt bytegleich %d, bemalt JPEG %d mit den Tabellen des Originals %d, Abweichung %.2f (< 8)",
                      tabGelesen ? 1 : 0, unbemalt ? 1 : 0, istJpeg ? 1 : 0, tabGleich ? 1 : 0, f2);
        pruefe(tabGelesen && unbemalt && istJpeg && tabGleich && f2 < 8.0, t);
    }
    // PNG mit Alpha und ohne: gelesen gleich
    for (const bool alpha : {true, false}) {
        const std::string png = m::pngSchreiben(bild.data(), b, h, alpha);
        const auto* d = reinterpret_cast<const unsigned char*>(png.data());
        const bhed::image::Image im = bhed::image::decode(d, png.size());
        bool gleich = im.ok && im.width == b && im.height == h;
        for (std::size_t i = 0; gleich && i < bild.size(); ++i) { gleich = im.rgba[i] == (!alpha && i % 4U == 3U ? 255 : bild[i]); }
        std::snprintf(t, sizeof t, "PNG %s: gelesen gleich %d", alpha ? "RGBA" : "RGB", gleich ? 1 : 0);
        pruefe(gleich, t);
    }
}

}  // namespace

int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    strichTest();
    blutenTest();
    spiegelTest();
    undoTest();
    undoBlutTest();
    ebenenTest();
    werkzeugTest();
    umbackTest();
    dateiTest();
    std::printf("%s (%d Fehler)\n", fails == 0 ? "OK" : "FEHLER", fails);
    return fails == 0 ? 0 : 1;
}
