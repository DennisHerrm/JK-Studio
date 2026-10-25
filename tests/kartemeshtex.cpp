// kartemeshtex.cpp - Karten-Editor Stufe 4 Punkt 3: das Plugin MeshTex ohne
// Oberflaeche (bhed/karte/meshtex.h).
//
// Geprueft gegen NRC 20260114 contrib/meshtex/MeshEntity.cpp:
//   - Min/Max Align (verschieben bis zur ganzen Zahl), Stretch / Shrink /
//     Auto (auf ganze Wiederholungen skalieren, Minimum bleibt);
//   - SetScale natuerlich (Laenge / (Textur / 2)), Spalten mit umgekehrtem
//     Vorzeichen, Kacheln, Kontrollpunkte aus den Flaechenwerten (ungerade
//     Punkte 2 s - (Nachbarn) / 2), Fehler bei falscher Nummer und 0;
//   - die Laenge auf gekruemmter Flaeche (Gauss-Legendre, Fehler <= 0,5);
//   - GetInfo (Texte, Rueckruf fuer Set S/T Scale), GeneralFunction.
//
// Aufruf: kartemeshtex
#include "bhed/karte/meshtex.h"
#include "bhed/karte/patch.h"

#include <cmath>
#include <cstdio>
#include <string>

using namespace bhed::karte;
namespace mt = bhed::karte::meshtex;

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
bool nah(float a, float b, float eps = 1e-4F) { return std::fabs(a - b) <= eps; }

// 3 x 3 (Zeilen x Spalten) flach: x = 32 * Spalte, y = 32 * Zeile; s, t gegeben
void flach(Patch& p, float s0, float ds, float t0, float dt, std::size_t n = 3) {
    p.setzeGroesse(n, n);
    for (std::size_t r = 0; r < n; ++r) {
        for (std::size_t c = 0; c < n; ++c) {
            PatchPunkt& q = p.punkt(r, c);
            q.vertex = Vektor3f(32.0F * static_cast<float>(c), 32.0F * static_cast<float>(r), 0);
            q.texcoord = Vektor2f(s0 + ds * static_cast<float>(c), t0 + dt * static_cast<float>(r));
        }
    }
    p.setzeShader("textures/test/patch");
    p.punkteGeaendert();
}

float s(const Patch& p, std::size_t r, std::size_t c) { return p.punkt(r, c).texcoord[0]; }
float t(const Patch& p, std::size_t r, std::size_t c) { return p.punkt(r, c).texcoord[1]; }

}  // namespace

int main() {
    mt::Meldungen m;
    // Min / Max Align
    {
        Patch p;
        flach(p, 0.3F, 0.5F, 0.25F, 0.5F);
        mt::MeshEntity e(p, 64, 64, m);
        pruefe(e.IsValid() && m.empty(), "3x3 mit Textur: gueltig");
        e.MinAlign(mt::S_TEX_AXIS_ONLY);
        pruefe(nah(s(p, 0, 0), 1.0F) && nah(s(p, 0, 2), 2.0F) && nah(t(p, 0, 0), 0.25F), "S Min Align: S um 0,7 verschoben, T bleibt");
        mt::MeshEntity e2(p, 64, 64, m);
        e2.MaxAlign(mt::T_TEX_AXIS_ONLY);
        pruefe(nah(t(p, 2, 0), 2.0F) && nah(t(p, 0, 0), 1.0F), "T Max Align: T max 1,25 -> 2");
    }
    // Stretch / Shrink / Auto
    {
        Patch p;
        flach(p, 0.3F, 1.15F, 0, 1);   // S 0,3 .. 2,6
        mt::MeshEntity e(p, 64, 64, m);
        e.MinMaxAlignStretch(mt::S_TEX_AXIS_ONLY);
        pruefe(nah(s(p, 0, 0), 1.0F) && nah(s(p, 0, 2), 3.0F, 1e-3F), "S Align Stretch: 1 .. 3,3 -> 1 .. 3");
        Patch q;
        flach(q, 0.3F, 1.15F, 0, 1);
        mt::MeshEntity e2(q, 64, 64, m);
        e2.MinMaxAlignShrink(mt::S_TEX_AXIS_ONLY);
        pruefe(nah(s(q, 0, 2), 4.0F, 1e-3F), "S Align Shrink: -> 1 .. 4");
        Patch a;
        flach(a, 0.3F, 0.95F, 0.25F, 1.65F);   // S 0,3 .. 2,2 (Bruchteil 0,2), T 0,25 .. 3,55 (0,55)
        mt::MeshEntity e3(a, 64, 64, m);
        e3.MinMaxAlignAutoScale(mt::ALL_TEX_AXES);
        // Auto entscheidet am alten Maximum: S Stretch (1 .. 2,9 -> 1 .. 2), T Shrink (1 .. 4,3 -> 1 .. 5)
        pruefe(nah(s(a, 0, 2), 2.0F, 1e-3F) && nah(t(a, 0, 0), 1.0F) && nah(t(a, 2, 0), 5.0F, 1e-3F), "S&T Align Auto: S Stretch, T Shrink");
    }
    // SetScale
    {
        Patch p;
        flach(p, 5, 0, 7, 0);
        mt::MeshEntity e(p, 64, 64, m);
        mt::SliceDesignation null0;
        e.SetScale(mt::ROW_SLICE_TYPE, &null0, nullptr, true, 1.0F);
        pruefe(nah(s(p, 1, 0), 0) && nah(s(p, 1, 1), 1) && nah(s(p, 1, 2), 2) && nah(t(p, 0, 0), 7), "Set S natuerlich 1: 64 Einheiten / 32 = 0 .. 2");
        e.SetScale(mt::COL_SLICE_TYPE, &null0, nullptr, true, 1.0F);
        pruefe(nah(t(p, 0, 1), 0) && nah(t(p, 1, 1), -1) && nah(t(p, 2, 1), -2), "Set T natuerlich 1: Spalten umgekehrt (0 .. -2)");
        e.SetScale(mt::ROW_SLICE_TYPE, &null0, nullptr, false, 4.0F);
        pruefe(nah(s(p, 0, 2), 4) && nah(s(p, 0, 1), 2), "Set S Kacheln 4: 0 .. 4");
        mt::SliceDesignation max;
        max.maxSlice = true;
        e.SetScale(mt::ROW_SLICE_TYPE, &max, nullptr, true, 1.0F);
        pruefe(nah(s(p, 0, 2), 0) && nah(s(p, 0, 0), -2), "\"Zero\" col Max: die letzte Spalte ist 0");
        mt::RefSliceDescriptor ref;
        ref.designation.index = 1;
        ref.totalLengthOnly = false;
        e.SetScale(mt::ROW_SLICE_TYPE, &null0, &ref, true, 2.0F);
        pruefe(nah(s(p, 0, 2), 1) && nah(s(p, 2, 2), 1), "Referenzzeile 1, natuerlich 2: alle Zeilen wie die Referenz (0 .. 1)");
        m.clear();
        mt::SliceDesignation falsch;
        falsch.index = 7;
        e.SetScale(mt::ROW_SLICE_TYPE, &falsch, nullptr, true, 1.0F);
        pruefe(m.size() == 1 && m[0].art == 2 && m[0].text == "Illegal column # specified.", "Set S mit \"Zero\" col 7: Fehler (Spalte)");
        m.clear();
        e.SetScale(mt::COL_SLICE_TYPE, &null0, nullptr, false, 0.0F);
        pruefe(m.size() == 1 && m[0].text == "A tile count of zero cannot be applied; the T values of the column will not be changed.", "Set T Kacheln 0: Fehler");
        m.clear();
    }
    // ungleich verteilt: Spalten bei x = 0, 16, 64 - die Flaeche bei u = 0,5 liegt bei x = 24
    {
        Patch p;
        flach(p, 0, 0, 0, 0);
        for (std::size_t r = 0; r < 3; ++r) { p.punkt(r, 1).vertex[0] = 16; }
        p.punkteGeaendert();
        mt::MeshEntity e(p, 64, 64, m);
        mt::SliceDesignation null0;
        e.SetScale(mt::ROW_SLICE_TYPE, &null0, nullptr, true, 1.0F);
        // Flaechenwerte 0 / 24 / 64 durch 32 = 0 / 0,75 / 2; der mittlere Kontrollpunkt 2 * 0,75 - (0 + 2) / 2 = 0,5
        pruefe(nah(s(p, 0, 1), 0.5F, 1e-3F) && nah(s(p, 0, 2), 2.0F, 1e-3F), "Kontrollpunkte aus Flaechenwerten (GenControlTexFromSurface): 0 / 0,5 / 2");
    }
    // gekruemmt: quadratische Bezier (0,0) - (32,0,32) - (64,0,0) je Zeile
    {
        Patch p;
        flach(p, 0, 0, 0, 0);
        for (std::size_t r = 0; r < 3; ++r) { p.punkt(r, 1).vertex[2] = 32; }
        p.punkteGeaendert();
        mt::MeshEntity e(p, 64, 64, m);
        mt::SliceDesignation null0;
        e.SetScale(mt::ROW_SLICE_TYPE, &null0, nullptr, false, 1.0F);   // 1 Kachel: s = Laenge bis hier / Gesamtlaenge
        double laenge = 0;   // die Laenge der Kurve fein abgetastet
        for (int i = 0; i < 20000; ++i) {
            const auto bez = [](double u) {
                const double x = 2 * (1 - u) * u * 32 + u * u * 64;
                const double z = 2 * (1 - u) * u * 32;
                return std::pair<double, double>{x, z};
            };
            const auto [x0, z0] = bez(i / 20000.0);
            const auto [x1, z1] = bez((i + 1) / 20000.0);
            laenge += std::sqrt((x1 - x0) * (x1 - x0) + (z1 - z0) * (z1 - z0));
        }
        mt::MeshEntity e2(p, 64, 64, m);
        m.clear();
        int zeile = 0;
        e2.GetInfo(&zeile, nullptr, nullptr, nullptr);
        float skala = 0;
        std::sscanf(m.empty() ? "" : m[0].text.c_str(), "On row 0:\n  S natural scale: %f", &skala);
        pruefe(std::fabs(skala * 32.0F - static_cast<float>(laenge)) <= 0.5F, "Laenge auf der Kurve: natural scale * 32 = " + std::to_string(skala * 32.0F) +
                                                                                 " gegen " + std::to_string(laenge));
        pruefe(nah(s(p, 0, 2), 1) && nah(s(p, 0, 0), 0), "1 Kachel ueber die Kurve: 0 .. 1");
    }
    // GetInfo
    {
        Patch p;
        flach(p, 0, 1, 0, -1);   // S 0..2 (natuerlich 1), T 0..-2
        mt::MeshEntity e(p, 64, 64, m);
        m.clear();
        int zeile = 0;
        int spalte = 2;
        float sk = 0;
        float ka = 0;
        const mt::TexInfoCallback rueck = [&](float a, float b) {
            sk = a;
            ka = b;
        };
        e.GetInfo(&zeile, &spalte, &rueck, nullptr);
        pruefe(m.size() == 1 && m[0].art == 0 &&
                   m[0].text.rfind("On row 0:\n  S natural scale: 1.000000\n  S tiles: 2.000000\n  S min: 0.000000\n  S max: 2.000000\n\n"
                                   "On column 2:\n  T natural scale: 1.000000\n  T tiles: -2.000000\n",
                                   0) == 0 &&
                   m[0].text.find("Over entire mesh:\n  Rows: 3\n  S min: 0.000000\n  S max: 2.000000\n  Columns: 3\n  T min: -2.000000\n  T max: 0.000000\n\n"
                                  "Worldspace extents: (0.00/64.00,0.00/64.00,0.00/0.00)") != std::string::npos,
               "Get Info: Zeile 0, Spalte 2, das ganze Netz");
        pruefe(nah(sk, 1) && nah(ka, 2), "Get Info: Rueckruf fuer Set S/T Scale (natuerlich 1, 2 Kacheln)");
        m.clear();
        int falsch = 9;
        e.GetInfo(&falsch, nullptr, nullptr, nullptr);
        pruefe(m.size() == 2 && m[0].art == 2 && m[0].text == "Illegal row # specified.", "Get Info Zeile 9: Fehler, dann das Netz");
    }
    // GeneralFunction
    {
        Patch p;
        flach(p, 0.5F, 0.25F, 0, 0);
        mt::MeshEntity e(p, 64, 64, m);
        mt::GeneralFunctionFactors sf;
        sf.oldValue = 1;
        sf.constant = 0.5F;
        mt::GeneralFunctionFactors tf;
        tf.rowNumber = 1;
        mt::SliceDesignation null0;
        e.GeneralFunction(&sf, &tf, &null0, &null0, nullptr, nullptr, false);
        pruefe(nah(s(p, 0, 0), 1.0F) && nah(s(p, 0, 2), 1.5F) && nah(t(p, 2, 0), 2) && nah(t(p, 1, 2), 1), "General Function (Kontrollwerte): S = old + 0,5, T = row_num");
        Patch q;
        flach(q, 0.5F, 0.25F, 0, 0);
        q.punkt(1, 1).texcoord[0] = 3;   // ein ungerader Punkt: Flaechenwerte != Kontrollwerte
        mt::MeshEntity e2(q, 64, 64, m);
        mt::GeneralFunctionFactors id;
        id.oldValue = 1;
        e2.GeneralFunction(&id, nullptr, &null0, &null0, nullptr, nullptr, true);
        pruefe(nah(s(q, 1, 1), 3.0F, 1e-4F) && nah(s(q, 0, 0), 0.5F), "General Function (Flaechenwerte) mit old_S * 1: Kontrollpunkte unveraendert");
        mt::GeneralFunctionFactors dist;
        dist.rowDistance = 1;
        Patch d;
        flach(d, 0, 0, 0, 0);
        mt::MeshEntity e3(d, 64, 64, m);
        e3.GeneralFunction(&dist, nullptr, &null0, &null0, nullptr, nullptr, false);
        pruefe(nah(s(d, 0, 2), 64) && nah(s(d, 2, 1), 32), "General Function: S = row_dist (Einheiten entlang der Zeile)");
    }
    // ungueltig
    {
        m.clear();
        Patch p;
        flach(p, 0, 1, 0, 1);
        mt::MeshEntity e(p, 0, 0, m);
        pruefe(!e.IsValid() && m.size() == 1 && m[0].text == "Unable to read patch mesh.", "ohne Textur: ungueltig, Fehler");
    }
    pruefe(mt::voreinstellungen().size() == 11 && std::string(mt::voreinstellungen()[0].befehl) == "STMinMaxAlignAuto", "die elf Voreinstellungen");
    std::printf("kartemeshtex: %d Pruefungen, %d Fehlschlaege\n", g_pruefungen, g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
