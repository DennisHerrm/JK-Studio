// modell_gewichte.cpp - Modell-Block 1a-2: Gewichte wie im Spiel
//
//   Lesen         readGlm liest jedes Gewicht bitgleich wie G2_GetVertBoneWeight
//                 (mdx_format.h:334-352: Kehrwert * 10 Bit, letztes = 1 - Summe),
//                 an der Probe und an einer Kopie mit allen 1024 Werten
//   Uebernehmen   mehr als vier -> die vier groessten, Summe 1, absteigend,
//                 gepackt und vom Spiel gelesen genau die Gewichte der Ecke
//   Malen         Hinzufuegen, Abziehen, Setzen, Glaetten, UV-Naht malt gleich
//   Spiegeln      Knochennamen l_/r_, _l/_r, lhand/rhand; Ecken x -> -x
//   Undo          Malen und Uebernehmen ueber das Dokument rueckgaengig
//   Anzeige       zuGlm zeigt waehrend des Malens die Gewichte des Spiels
//
// Aufruf: modellgewichtetest <model.glm>
#include "bhed/glm.h"
#include "bhed/modell/dokument.h"
#include "bhed/modell/gewichte.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {
int fails = 0;
void expect(const std::string& what, bool ok) {
    std::printf("  %s  %s\n", ok ? "ok  " : "FEHL", what.c_str());
    if (!ok) { ++fails; }
}
std::string slurp(const std::string& p) {
    std::ifstream f(p, std::ios::binary);
    std::ostringstream s;
    s << f.rdbuf();
    return s.str();
}

using namespace bhed;
using namespace bhed::modell;

std::int32_t i32(const std::string& b, std::size_t o) {
    std::int32_t v = 0;
    std::memcpy(&v, b.data() + o, 4);
    return v;
}

// Je Ecke der Stufe 0 der Ort ihres mdxmVertex_t (OpenJK mdx_format.h: mdxmHeader_t, mdxmLOD_t, mdxmSurface_t).
std::vector<std::vector<std::size_t>> eckenOrte(const std::string& b) {
    std::vector<std::vector<std::size_t>> aus;
    const std::size_t kopf = 4 + 4 + 64 + 64;
    const int numSurfaces = i32(b, kopf + 16);
    const std::size_t lod = static_cast<std::size_t>(i32(b, kopf + 12));
    const std::size_t lodStart = lod + 4;
    for (int s = 0; s < numSurfaces; ++s) {
        const std::size_t so = lodStart + static_cast<std::size_t>(i32(b, lodStart + 4 * static_cast<std::size_t>(s)));
        const int numVerts = i32(b, so + 12);
        const std::size_t vo = so + static_cast<std::size_t>(i32(b, so + 16));
        std::vector<std::size_t> v;
        for (int k = 0; k < numVerts; ++k) { v.push_back(vo + static_cast<std::size_t>(k) * 32U); }
        aus.push_back(std::move(v));
    }
    return aus;
}

// G2_GetVertWeights / G2_GetVertBoneWeight, Wort fuer Wort (mdx_format.h:320-352).
int spielZahl(const std::string& b, std::size_t p) {
    std::uint32_t u = 0;
    std::memcpy(&u, b.data() + p + 24, 4);
    return static_cast<int>(u >> 30) + 1;
}
float spielGewicht(const std::string& b, std::size_t p, int iWeightNum, float& fTotalWeight, int iNumWeights) {
    std::uint32_t uiNmWeightsAndBoneIndexes = 0;
    std::memcpy(&uiNmWeightsAndBoneIndexes, b.data() + p + 24, 4);
    float fBoneWeight;
    if (iWeightNum == iNumWeights - 1) {
        fBoneWeight = 1.0f - fTotalWeight;
    } else {
        int iTemp = static_cast<unsigned char>(b[p + 28 + static_cast<std::size_t>(iWeightNum)]);
        iTemp |= static_cast<int>((uiNmWeightsAndBoneIndexes >> (12 + (iWeightNum * 2))) & 0x300U);
        fBoneWeight = ((float)(1.0f / 1023.0f)) * static_cast<float>(iTemp);
        fTotalWeight += fBoneWeight;
    }
    return fBoneWeight;
}

// Wie viele Gewichte readGlm anders liest als das Spiel (bitweise).
int leseUnterschiede(const std::string& b, int& gezaehlt) {
    GlmModel m;
    std::string f;
    if (!readGlm(b, m, &f)) { return -1; }
    const auto orte = eckenOrte(b);
    int anders = 0;
    gezaehlt = 0;
    for (std::size_t s = 0; s < orte.size() && s < m.surfaces.size(); ++s) {
        for (std::size_t k = 0; k < orte[s].size() && k < m.surfaces[s].verts.size(); ++k) {
            const int n = spielZahl(b, orte[s][k]);
            float total = 0.0f;
            for (int w = 0; w < n; ++w) {
                const float g = spielGewicht(b, orte[s][k], w, total, n);
                ++gezaehlt;
                if (std::memcmp(&g, &m.surfaces[s].verts[k].weights[w], 4) != 0) { ++anders; }
            }
        }
    }
    return anders;
}

// Gepackt wie mdxmVertex_t (fuer den Rundlauf Uebernehmen -> Datei -> Spiel).
void packe(const SpielGewichte& s, std::string& b, std::size_t p) {
    std::uint32_t u = static_cast<std::uint32_t>(s.zahl - 1) << 30;
    for (int k = 0; k < s.zahl; ++k) {
        u |= static_cast<std::uint32_t>(k & 31) << (5 * k);   // Verweis k (lokal)
        if (k + 1 < s.zahl) {
            u |= static_cast<std::uint32_t>((s.bits[k] >> 8) & 3) << (20 + 2 * k);
            b[p + 28 + static_cast<std::size_t>(k)] = static_cast<char>(s.bits[k] & 0xFF);
        }
    }
    std::memcpy(&b[p + 24], &u, 4);
}

Ecke ecke(float x, float y, float z, int knochen = 0) {
    Ecke e;
    e.ort[0] = x;
    e.ort[1] = y;
    e.ort[2] = z;
    e.knochen[0] = static_cast<std::uint8_t>(knochen);
    e.gewicht[0] = 1.0F;
    return e;
}

}  // namespace

int main(int argc, char** argv) {
    // --- Lesen wie das Spiel --------------------------------------------------------
    if (argc >= 2) {
        const std::string probe = slurp(argv[1]);
        int n = 0;
        const int a = leseUnterschiede(probe, n);
        expect("Probe: readGlm liest " + std::to_string(n) + " Gewichte, anders als G2_GetVertBoneWeight: " + std::to_string(a), a == 0 && n > 0);
        // Jeder 10-Bit-Wert einmal: das erste Gewicht jeder Ecke mit zwei Gewichten bekommt Wert i (i laeuft durch 0..1023).
        std::string kopie = probe;
        int wert = 0;
        int gesetzt = 0;
        for (const auto& fl : eckenOrte(kopie)) {
            for (const std::size_t p : fl) {
                SpielGewichte s;
                s.zahl = 2;
                s.bits[0] = wert;
                packe(s, kopie, p);
                wert = (wert + 1) % 1024;
                ++gesetzt;
            }
        }
        const int b = leseUnterschiede(kopie, n);
        expect("alle 10-Bit-Werte (" + std::to_string(gesetzt) + " Ecken): anders als das Spiel " + std::to_string(b), b == 0 && gesetzt >= 1024);
    } else {
        expect("Aufruf mit einer .glm", false);
    }

    // --- Uebernehmen: vier, normiert, absteigend, wie das Spiel liest ------------------------
    {
        const GewichtListe l = {{3, 0.10F}, {7, 0.40F}, {1, 0.05F}, {9, 0.30F}, {2, 0.15F}, {5, 0.02F}};
        SpielGewichte s;
        expect("sechs Gewichte uebernommen", fuersSpiel(l, s));
        expect("die vier groessten absteigend: 7 9 2 3", s.zahl == 4 && s.knochen[0] == 7 && s.knochen[1] == 9 && s.knochen[2] == 2 && s.knochen[3] == 3);
        float summe = 0.0F;
        for (int k = 0; k < s.zahl; ++k) { summe += s.gewicht[k]; }
        expect("Summe 1 (letztes = 1 - Summe)", std::fabs(summe - 1.0F) < 1e-6F);
        // Normiert: 0.4/0.95 = 0.42105 -> 431 / 1023
        expect("normiert auf 10 Bit: 7 = 431, 9 = 323, 2 = 162", s.bits[0] == 431 && s.bits[1] == 323 && s.bits[2] == 162);
        // gepackt und vom Spiel gelesen
        std::string b(32, '\0');
        packe(s, b, 0);
        float total = 0.0f;
        bool gleich = spielZahl(b, 0) == 4;
        for (int k = 0; k < 4; ++k) {
            const float g = spielGewicht(b, 0, k, total, 4);
            gleich = gleich && std::memcmp(&g, &s.gewicht[k], 4) == 0;
        }
        expect("gepackt vom Spiel gelesen: bitgleich", gleich);
        SpielGewichte leer;
        expect("keine Gewichte: nichts uebernommen", !fuersSpiel({{4, 0.0F}}, leer));
        SpielGewichte eins;
        expect("ein Gewicht: 1", fuersSpiel({{4, 0.3F}}, eins) && eins.zahl == 1 && eins.gewicht[0] == 1.0F);
        SpielGewichte klein;
        expect("ein Gewicht unter 1/2046 faellt weg", fuersSpiel({{4, 1.0F}, {5, 0.0004F}}, klein) && klein.zahl == 1);
        SpielGewichte dopp;
        expect("doppelte Knochen zusammen", fuersSpiel({{4, 0.25F}, {4, 0.25F}, {6, 0.5F}}, dopp) && dopp.zahl == 2 && dopp.bits[0] == 512);
    }

    // --- Malen --------------------------------------------------------------------------
    {
        Netz n;   // Viereck, Ecke 4 = Ecke 2 an einer UV-Naht
        std::vector<Ecke> e = {ecke(0, 0, 0), ecke(1, 0, 0), ecke(1, 1, 0), ecke(0, 1, 0), ecke(1, 1, 0)};
        e[4].uv[0] = 0.5F;
        n.aufbauen(e, {0, 1, 2, 0, 4, 3});
        expect("Hinzufuegen 0.5 an Ecke 2 (Knochen 3): beide Ecken des Punkts", malen(n, {{2, 1.0F}}, 3, PinselArt::Hinzufuegen, 0.5F) == 2);
        expect("Gewicht 0.5 an 2 und 4, Knochen 0 bleibt 1", gewichtVon(n, 2, 3) == 0.5F && gewichtVon(n, 4, 3) == 0.5F && gewichtVon(n, 2, 0) == 1.0F);
        (void)malen(n, {{2, 0.5F}}, 3, PinselArt::Abziehen, 0.4F);
        expect("Abziehen 0.4 mit Staerke 0.5: 0.3", std::fabs(gewichtVon(n, 2, 3) - 0.3F) < 1e-6F);
        (void)malen(n, {{0, 1.0F}}, 3, PinselArt::Setzen, 0.8F);
        expect("Setzen 0.8: 0.8", std::fabs(gewichtVon(n, 0, 3) - 0.8F) < 1e-6F);
        (void)malen(n, {{1, 1.0F}}, 3, PinselArt::Glaetten, 1.0F);
        // Nachbarn von Punkt 1: Punkt 0 (0.8, zweimal ueber zwei Kanten? nur Dreieck 0) und Punkt 2 (0.3)
        expect("Glaetten: Mittel der Nachbarn (0.8 + 0.3) / 2", std::fabs(gewichtVon(n, 1, 3) - 0.55F) < 1e-6F);
        // mehr als vier
        for (int k = 4; k < 9; ++k) { (void)malen(n, {{3, 1.0F}}, k, PinselArt::Setzen, 0.1F * static_cast<float>(k)); }
        expect("Ecke 3 mit 6 Gewichten: eine Ecke ueber vier", eckenUeberVier(n) == 1);
        Ecke spiel = n.ecken[3];
        expect("aufs Spiel: vier Knochen, absteigend 0 8 7 6", aufEcke(n.mal[3], spiel) && spiel.knochen[0] == 0 && spiel.knochen[1] == 8 &&
                                                                  spiel.knochen[2] == 7 && spiel.knochen[3] == 6);
        expect("Abfall: Mitte 1, Rand 0, Haelfte 0.5", abfall(0.0F, 10.0F) == 1.0F && abfall(10.0F, 10.0F) == 0.0F && std::fabs(abfall(5.0F, 10.0F) - 0.5F) < 1e-6F);
    }

    // --- Spiegeln -----------------------------------------------------------------------
    {
        const std::vector<std::string> k = {"model_root", "lhand", "rhand", "lower_lumbar", "l_arm", "r_arm", "knee_l", "knee_r", "lonely"};
        expect("lhand <-> rhand", spiegelKnochen(k, 1) == 2 && spiegelKnochen(k, 2) == 1);
        expect("l_arm <-> r_arm, knee_l <-> knee_r", spiegelKnochen(k, 4) == 5 && spiegelKnochen(k, 7) == 6);
        expect("lower_lumbar und lonely ohne Partner", spiegelKnochen(k, 3) == -1 && spiegelKnochen(k, 8) == -1 && spiegelKnochen(k, 0) == -1);
        Inhalt in;
        in.stufenZahl = 1;
        Teil links;
        links.name = "l_arm";
        links.stufen.resize(1);
        links.stufen[0].aufbauen({ecke(5, 0, 0, 1), ecke(6, 0, 0, 1), ecke(5, 1, 0, 1)}, {0, 1, 2});
        Teil rechts = links;
        rechts.name = "r_arm";
        rechts.stufen[0].aufbauen({ecke(-5, 0, 0, 0), ecke(-6, 0, 0, 0), ecke(-5, 1, 0, 0)}, {0, 2, 1});
        in.teile = {links, rechts};
        malAnlegen(in.teile[0].stufen[0]);
        in.teile[0].stufen[0].mal[1] = {{1, 0.75F}, {3, 0.25F}};
        Dokument d;
        d.laden(in);
        d.aendern("Spiegeln", [&](Inhalt& x) { expect("von +X nach -X: drei Ecken", spiegeln(x, 0, k, 1, 0.01F) == 3); });
        const Netz& r = d.inhalt().teile[1].stufen[0];
        expect("r_arm Ecke 1 (-6,0,0): rhand 0.75, lower_lumbar 0.25", gewichtVon(r, 1, 2) == 0.75F && gewichtVon(r, 1, 3) == 0.25F && gewichtVon(r, 1, 0) == 0.0F);
        expect("Undo: r_arm ohne Malliste, Knochen 0", d.rueckgaengig() && d.inhalt().teile[1].stufen[0].mal.empty() &&
                                                         gewichtVon(d.inhalt().teile[1].stufen[0], 1, 0) == 1.0F);
        expect("Wiederholen", d.wiederholen() && gewichtVon(d.inhalt().teile[1].stufen[0], 1, 2) == 0.75F);
        // Anzeige waehrend des Malens: die Gewichte des Spiels
        const GlmModel g = zuGlm(d.inhalt(), 0);
        expect("zuGlm: r_arm Ecke 1 zeigt Knochen 2 mit 767/1023, Knochen 3 Rest",
               g.surfaces[1].verts[1].bones[0] == 2 && g.surfaces[1].verts[1].weights[0] == g2Gewicht(767) &&
                   g.surfaces[1].verts[1].bones[1] == 3 && g.surfaces[1].verts[1].weights[1] == 1.0F - g2Gewicht(767));
        // Uebernehmen
        d.aendern("Uebernehmen", [&](Inhalt& x) { expect("Uebernehmen: 6 Ecken", uebernehmen(x) == 6); });
        const Netz& r2 = d.inhalt().teile[1].stufen[0];
        expect("nach Uebernehmen: keine Malliste, Ecke traegt 2 / 3", r2.mal.empty() && r2.ecken[1].knochen[0] == 2 && r2.ecken[1].gewicht[0] == g2Gewicht(767));
        expect("Undo Uebernehmen: Malliste wieder da", d.rueckgaengig() && !d.inhalt().teile[1].stufen[0].mal.empty());
    }

    std::printf("%s (%d Fehler)\n", fails == 0 ? "modell_gewichte: alles ok" : "modell_gewichte: FEHLER", fails);
    return fails == 0 ? 0 : 1;
}
