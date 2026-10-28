// kartesprung.cpp - Stufe 9 Punkt 4: Machtsprung im Spieler-Pmove, Sprungweite und Platz
// (ctest karte_sprung)
//
// Eine Welt aus Kaesten (Kastenspur wie CM_Trace gegen achsparallele Brushes):
//   - der normale Sprung steigt wie bisher 31,6 Einheiten (JUMP_VELOCITY 225 bei 800);
//   - mit gehaltener Taste steigt der Machtsprung bis forceJumpHeight der Stufe (96/192/384;
//     METROID_JUMP und die Kappe in PM_AirMove), losgelassen weniger; JK2 rechnet gleich;
//   - sprungPruefen findet Stehplaetze und Ducken-Plaetze (unter einer niedrigen Decke),
//     Kanten, einen normalen Sprung ueber eine Luecke und einen Sims, den nur der Machtsprung
//     erreicht.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

#include "bhed/karte/sprungpruefung.h"
#include "bhed/navlauf.h"

namespace {
int g_fehler = 0;
void expect(const char* was, bool ok) {
    std::printf("  %s  %s\n", ok ? "ok  " : "FEHL", was);
    if (!ok) { ++g_fehler; }
}
using bhed::NavSpur;
using bhed::NavVec;

struct Kasten {
    NavVec mi, ma;
};
std::vector<Kasten> g_welt;

// Kastenspur gegen die Kaesten (Minkowski-Summe, Strahl gegen Quader; 1/32 Abstand wie SURFACE_CLIP_EPSILON)
NavSpur spur(const NavVec& a, const NavVec& mins, const NavVec& maxs, const NavVec& b, int, std::uint32_t) {
    NavSpur r;
    r.endpos = b;
    float best = 1.0F;
    NavVec n{};
    for (const Kasten& k : g_welt) {
        NavVec lo{}, hi{};
        for (int i = 0; i < 3; ++i) {
            lo[i] = k.mi[i] - maxs[i];
            hi[i] = k.ma[i] - mins[i];
        }
        bool innen = true;
        for (int i = 0; i < 3; ++i) { innen = innen && a[i] > lo[i] && a[i] < hi[i]; }
        if (innen) {
            r.startsolid = true;
            bool endeInnen = true;
            for (int i = 0; i < 3; ++i) { endeInnen = endeInnen && b[i] > lo[i] && b[i] < hi[i]; }
            r.allsolid = endeInnen;
            r.fraction = 0.0F;
            r.endpos = a;
            return r;
        }
        float t0 = 0.0F, t1 = 1.0F;
        int achse = -1;
        float vorz = 0.0F;
        bool weg = false;
        for (int i = 0; i < 3 && !weg; ++i) {
            const float d = b[i] - a[i];
            if (std::fabs(d) < 1e-9F) {
                if (a[i] <= lo[i] || a[i] >= hi[i]) { weg = true; }
                continue;
            }
            float ta = (lo[i] - a[i]) / d, tb = (hi[i] - a[i]) / d;
            float s = -1.0F;
            if (ta > tb) {
                std::swap(ta, tb);
                s = 1.0F;
            }
            if (ta > t0) {
                t0 = ta;
                achse = i;
                vorz = s;
            }
            t1 = std::min(t1, tb);
            if (t0 >= t1) { weg = true; }
        }
        if (weg || achse < 0 || t0 >= best) { continue; }
        best = t0;
        n = {0, 0, 0};
        n[static_cast<std::size_t>(achse)] = vorz;
    }
    if (best < 1.0F) {
        const float len = std::sqrt((b[0] - a[0]) * (b[0] - a[0]) + (b[1] - a[1]) * (b[1] - a[1]) + (b[2] - a[2]) * (b[2] - a[2]));
        const float f = std::max(0.0F, best - (len > 0 ? 0.03125F / len : 0.0F));
        r.fraction = f;
        for (int i = 0; i < 3; ++i) { r.endpos[i] = a[i] + (b[i] - a[i]) * f; }
        r.normal = n;
        r.entity = bhed::kNavWeltEntity;
        r.contents = bhed::kNavSolid;
    }
    return r;
}

float aufstieg(int stufe, bool jk2, int halteMs) {
    bhed::NavSpieler s;
    s.ort = {0, 0, 24.03125F};
    s.amBoden = true;
    s.machtStufe = stufe;
    s.jk2 = jk2;
    float oben = s.ort[2];
    for (int t = 0; t < 3000; t += 16) {
        bhed::NavSpielerEingabe e;
        e.hoch = t < halteMs ? 127 : 0;
        bhed::navSpielerPmove(s, spur, e, 16);
        oben = std::max(oben, s.ort[2]);
        if (t > 100 && s.amBoden) { break; }
    }
    return oben - 24.03125F;
}
}  // namespace

int main() {
    g_welt = {{{-2048, -2048, -64}, {2048, 2048, 0}}};   // ein Boden, Oberkante 0
    std::printf("--- Machtsprung (gehalten) ---\n");
    const float h0 = aufstieg(0, false, 3000), h1 = aufstieg(1, false, 3000), h2 = aufstieg(2, false, 3000), h3 = aufstieg(3, false, 3000);
    std::printf("      Aufstieg: 0 %.1f, 1 %.1f, 2 %.1f, 3 %.1f\n", h0, h1, h2, h3);
    expect("ohne Machtsprung wie bisher 31,6 (225^2 / 1600)", std::fabs(h0 - 31.6F) < 1.5F);
    expect("Stufe 1 bis forceJumpHeight 96", h1 > 92 && h1 < 104);
    expect("Stufe 2 bis 192", h2 > 186 && h2 < 202);
    expect("Stufe 3 bis 384", h3 > 376 && h3 < 396);
    const float kurz = aufstieg(3, false, 150);
    std::printf("      Stufe 3, 150 ms gehalten: %.1f\n", kurz);
    expect("losgelassen: hoeher als der normale Sprung, viel niedriger als 384", kurz > 40 && kurz < 200);
    expect("JK2 rechnet gleich (codeJK2)", std::fabs(aufstieg(2, true, 3000) - h2) < 0.5F);

    std::printf("--- Sprungweite und Platz ---\n");
    // Plattform A (x -512..0), Luecke 96, Plattform B (x 96..512); ein Sims C (Oberkante 160)
    // hinter B (x 512..768); ueber A links eine niedrige Decke (Unterkante 48)
    g_welt = {{{-512, -128, -64}, {0, 128, 0}},
              {{96, -128, -64}, {512, 128, 0}},
              {{512, -128, -64}, {768, 128, 160}},
              {{-512, -128, 48}, {-320, 128, 96}}};
    bhed::karte::SprungOptionen o;
    o.bereichMin = {-512, -128, -128};
    o.bereichMax = {768, 128, 400};
    o.raster = 32.0F;
    o.machtStufe = 2;
    o.richtungen = 8;
    o.maxKanten = 400;
    const bhed::karte::SprungBericht r = bhed::karte::sprungPruefen(spur, o);
    int stehen = 0, ducken = 0;
    for (const auto& p : r.punkte) { (p.stehen ? stehen : ducken) += 1; }
    bool ueberLuecke = false, simsNormal = false, simsMacht = false;
    for (const auto& s : r.spruenge) {
        if (s.start[0] < 0 && s.landung[0] > 96 && s.landung[0] < 512 && std::fabs(s.hoehe) < 1 && !s.macht) { ueberLuecke = true; }
        if (s.landung[2] > 150) { (s.macht ? simsMacht : simsNormal) = true; }
    }
    std::printf("      %zu Punkte (%d stehen, %d geduckt), %zu Kanten, %zu Laeufe, %zu Spruenge\n", r.punkte.size(), stehen, ducken, r.kanten,
                r.simuliert, r.spruenge.size());
    expect("Stehplaetze auf A, B und C, Ducken-Plaetze unter der Decke", stehen > 50 && ducken >= 10);
    expect("Kanten an der Luecke und am Sims gefunden", r.kanten > 10);
    expect("ein normaler Sprung ueber die Luecke von A nach B", ueberLuecke);
    expect("den Sims (160 hoch) erreicht nur der Machtsprung", simsMacht && !simsNormal);
    std::printf("%s (%d Fehler)\n", g_fehler == 0 ? "OK" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
