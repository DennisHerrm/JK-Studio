// modell_gewichte2.cpp - AP8 (MBENTWURF Teil B, Abgleich W2-W6, W12): die Werkzeuge des Bereichs Weights
//
//   Fund AP12     fuersSpiel laesst keinen Knochen mit Rest 0 stehen; was "Uebernehmen" in die Ecke legt, liest das Spiel
//                 aus der geschriebenen .glm bitgleich
//   Schweissnetz  Ecken gleichen Orts ueber Flaechen hinweg sind ein Punkt
//   Regeln        Sperre je Knochen, Auto-Normalisieren wie Blender (do_weight_paint_normalize_all_locked_try_active)
//   Pinsel        Verwischen (Smear), Verlauf (Gradient) linear und radial
//   Werkzeug      Wachsen, Schrumpfen, Ring, Schleife, Setzen, Skalieren wie das Weight Tool von 3ds Max
//   Bereinigen    Alles normalisieren, Saeubern, Glaetten (= Blend von 3ds Max), Begrenzen wie Blender
//   Spiegeln      Seiten und Partner, Toleranz, Topologie, wie 3ds Max (Seite -> Seite, Tauschen, nur Auswahl),
//                 wie Blender (dvert_mirror_op: vier Faelle), beim Malen
//   Grenzen       nach jedem Werkzeug und "Uebernehmen": hoechstens 4 Gewichte je Ecke, 10 Bit wie G2_GetVertBoneWeight,
//                 Summe 1; Knochen je Flaeche gegen die Grenze des Zielspiels (JKA und Movie Duels: 32)
//   Raven         mit <JA base>: kyle - wo links liegt, Paare, Spiegeln und Grenzen an einer Figur des Spiels
//
// Aufruf: modellgewichte2test <model.glm> [<JA base>]
#include "bhed/gla.h"
#include "bhed/glm.h"
#include "bhed/modell/dokument.h"
#include "bhed/modell/g2.h"
#include "bhed/modell/gewichte.h"
#include "bhed/modell/glmdatei.h"
#include "bhed/pk3.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdarg>
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
std::string f(const char* format, ...) {
    char z[600];
    va_list a;
    va_start(a, format);
    std::vsnprintf(z, sizeof z, format, a);
    va_end(a);
    return z;
}
std::string slurp(const std::string& p) {
    std::ifstream d(p, std::ios::binary);
    std::ostringstream s;
    s << d.rdbuf();
    return s.str();
}

using namespace bhed;
using namespace bhed::modell;

Ecke ecke(float x, float y, float z, int knochen = 0) {
    Ecke e;
    e.ort[0] = x;
    e.ort[1] = y;
    e.ort[2] = z;
    e.normale[2] = 1.0F;
    e.knochen[0] = static_cast<std::uint8_t>(knochen);
    e.gewicht[0] = 1.0F;
    return e;
}

bool nah(float a, float b, float eps = 1e-6F) { return std::fabs(a - b) <= eps; }

float summe(const GewichtListe& l) {
    float s = 0.0F;
    for (const KnochenGewicht& g : l) { s += g.gewicht; }
    return s;
}

float in(const GewichtListe& l, int knochen) {
    float w = 0.0F;
    for (const KnochenGewicht& g : l) {
        if (g.knochen == knochen) { w += g.gewicht; }
    }
    return w;
}

// Ein Gitter aus nx mal ny Vierecken in der Ebene z = 0, je Viereck zwei Dreiecke (Diagonale immer gleich), Ecke (i, j)
// hat die Nummer j * (nx + 1) + i und liegt bei (x0 + i, j, 0).
Netz gitter(int nx, int ny, float x0 = 0.0F, int knochen = 0) {
    std::vector<Ecke> e;
    for (int j = 0; j <= ny; ++j) {
        for (int i = 0; i <= nx; ++i) { e.push_back(ecke(x0 + static_cast<float>(i), static_cast<float>(j), 0.0F, knochen)); }
    }
    std::vector<std::uint32_t> idx;
    const auto nr = [nx](int i, int j) { return static_cast<std::uint32_t>(j * (nx + 1) + i); };
    for (int j = 0; j < ny; ++j) {
        for (int i = 0; i < nx; ++i) {
            for (const std::uint32_t v : {nr(i, j), nr(i + 1, j), nr(i + 1, j + 1), nr(i, j), nr(i + 1, j + 1), nr(i, j + 1)}) { idx.push_back(v); }
        }
    }
    Netz n;
    n.aufbauen(std::move(e), idx);
    return n;
}

Inhalt mitTeilen(std::vector<std::pair<std::string, Netz>> teile) {
    Inhalt x;
    x.stufenZahl = 1;
    for (auto& [name, netz] : teile) {
        Teil t;
        t.name = name;
        t.stufen.push_back(std::move(netz));
        x.teile.push_back(std::move(t));
    }
    return x;
}

// Jede Ecke traegt, was das Spiel aus der Datei liest: hoechstens vier Knochen, die ersten n-1 Gewichte ein 10-Bit-Wert
// mal 1/1023 (G2_GetVertBoneWeight), das letzte der Rest, kein Knochen ohne Gewicht, Summe 1.
bool wieImSpiel(const Inhalt& x, int* gewichteMax = nullptr) {
    bool ok = true;
    for (const Teil& t : x.teile) {
        if (t.istTag()) { continue; }
        for (const Netz& n : t.stufen) {
            for (const Ecke& e : n.ecken) {
                int zahl = 0;
                for (int k = 0; k < 4; ++k) { zahl += e.gewicht[k] > 0.0F ? 1 : 0; }
                if (gewichteMax != nullptr) { *gewichteMax = std::max(*gewichteMax, zahl); }
                float total = 0.0F;
                for (int k = 0; k < zahl; ++k) {
                    if (k == zahl - 1) {
                        ok = ok && e.gewicht[k] == 1.0F - total;
                    } else {
                        const long bits = std::lround(static_cast<double>(e.gewicht[k]) * 1023.0);
                        ok = ok && bits >= 0 && bits <= 1023 && e.gewicht[k] == g2Gewicht(static_cast<int>(bits));
                        total += e.gewicht[k];
                    }
                }
                for (int k = zahl; k < 4; ++k) { ok = ok && e.gewicht[k] == 0.0F; }
                ok = ok && zahl >= 1;
            }
        }
    }
    return ok;
}

int knochenMax(const Inhalt& x, std::string* name = nullptr) {
    int m = 0;
    for (const Teil& t : x.teile) {
        if (t.istTag()) { continue; }
        for (const Netz& n : t.stufen) {
            const int k = mass(n).knochen;
            if (k > m) {
                m = k;
                if (name != nullptr) { *name = t.name; }
            }
        }
    }
    return m;
}

// --- Fund aus AP12: ein Knochen mit Rest 0 --------------------------------------------------------------------------------
void pruefeRestNull(const std::string& glm) {
    std::printf("Fund AP12: fuersSpiel und ein letztes Gewicht mit Rest 0\n");
    // in 1023steln: 994,6 + 24,6 + 2,6 + 1,2 = 1023 - die ersten drei runden auf 995 + 25 + 3 = 1023, fuer das vierte bleibt 0
    const GewichtListe vier = {{1, 994.6F / 1023.0F}, {2, 24.6F / 1023.0F}, {3, 2.6F / 1023.0F}, {4, 1.2F / 1023.0F}};
    // drei: 1000,6 + 21,6 + 0,8 -> 1001 + 22 = 1023, Rest 0
    const GewichtListe drei = {{1, 1000.6F / 1023.0F}, {2, 21.6F / 1023.0F}, {3, 0.8F / 1023.0F}};
    for (const GewichtListe* l : {&vier, &drei}) {
        SpielGewichte s;
        const bool da = fuersSpiel(*l, s);
        bool positiv = da;
        float sum = 0.0F;
        for (int k = 0; k < s.zahl; ++k) {
            positiv = positiv && s.gewicht[k] > 0.0F;
            sum += s.gewicht[k];
        }
        expect(f("%d Gewichte, die auf 1023 runden: kein Knochen ohne Gewicht (uebernommen %d, letztes %.6f), Summe 1", static_cast<int>(l->size()),
                 s.zahl, s.zahl > 0 ? static_cast<double>(s.gewicht[s.zahl - 1]) : -1.0),
               positiv && nah(sum, 1.0F));
    }
    // Der Weg "Uebernehmen" bis in die Datei: malen, uebernehmen, schreiben, lesen - die Ecke des Dokuments ist bitgleich die
    // der gelesenen Datei.
    Inhalt x;
    if (glm.empty() || !ausGlm(slurp(glm), x) || x.knochenZahl < 5) {
        expect("Aufruf mit einer .glm mit wenigstens 5 Knochen", false);
        return;
    }
    int teil = -1;
    for (std::size_t t = 0; t < x.teile.size() && teil < 0; ++t) {
        if (!x.teile[t].istTag() && !x.teile[t].stufen.empty() && !x.teile[t].stufen[0].ecken.empty()) { teil = static_cast<int>(t); }
    }
    if (teil < 0) {
        expect("die Probe hat eine Flaeche", false);
        return;
    }
    // Knochen, die die Flaeche schon benutzt (die Datei fuehrt je Flaeche hoechstens 32 Verweise): vier davon
    std::vector<int> benutzt;
    for (const Ecke& e : x.teile[static_cast<std::size_t>(teil)].stufen[0].ecken) {
        for (int k = 0; k < 4; ++k) {
            if (e.gewicht[k] > 0.0F && std::find(benutzt.begin(), benutzt.end(), e.knochen[k]) == benutzt.end()) { benutzt.push_back(e.knochen[k]); }
        }
    }
    for (int k = 0; benutzt.size() < 4U && k < x.knochenZahl; ++k) {
        if (std::find(benutzt.begin(), benutzt.end(), k) == benutzt.end()) { benutzt.push_back(k); }
    }
    Netz& n = x.teile[static_cast<std::size_t>(teil)].stufen[0];
    malAnlegen(n);
    n.mal[0] = {{benutzt[0], 994.6F / 1023.0F}, {benutzt[1], 24.6F / 1023.0F}, {benutzt[2], 2.6F / 1023.0F}, {benutzt[3], 1.2F / 1023.0F}};
    (void)uebernehmen(x);
    const Ecke dok = x.teile[static_cast<std::size_t>(teil)].stufen[0].ecken[0];
    std::string bytes;
    Inhalt gelesen;
    const bool geschrieben = glmSchreiben(x, bytes) && ausGlm(bytes, gelesen);
    bool gleich = geschrieben;
    if (geschrieben) {
        const Ecke& datei = gelesen.teile[static_cast<std::size_t>(teil)].stufen[0].ecken[0];
        for (int k = 0; k < 4; ++k) {
            const bool beideLeer = dok.gewicht[k] == 0.0F && datei.gewicht[k] == 0.0F;
            gleich = gleich && (beideLeer || (dok.knochen[k] == datei.knochen[k] && std::memcmp(&dok.gewicht[k], &datei.gewicht[k], 4) == 0));
        }
        std::printf("    Dokument %d:%.9g %d:%.9g %d:%.9g %d:%.9g | Datei %d:%.9g %d:%.9g %d:%.9g %d:%.9g\n", dok.knochen[0], static_cast<double>(dok.gewicht[0]),
                    dok.knochen[1], static_cast<double>(dok.gewicht[1]), dok.knochen[2], static_cast<double>(dok.gewicht[2]), dok.knochen[3],
                    static_cast<double>(dok.gewicht[3]), datei.knochen[0], static_cast<double>(datei.gewicht[0]), datei.knochen[1],
                    static_cast<double>(datei.gewicht[1]), datei.knochen[2], static_cast<double>(datei.gewicht[2]), datei.knochen[3],
                    static_cast<double>(datei.gewicht[3]));
    }
    expect("Uebernehmen -> .glm schreiben -> lesen: die Ecke des Dokuments ist bitgleich die der Datei", gleich);
    expect("nach Uebernehmen traegt jede Ecke, was das Spiel liest", wieImSpiel(x));
}

// --- Schweissnetz, Auswahl ----------------------------------------------------------------------------------------------------
void pruefeSchweissnetz() {
    std::printf("Schweissnetz und Auswahl\n");
    // zwei Flaechen teilen die Kante x = 2: links ein Gitter 2 x 2 (x 0..2), rechts 2 x 2 (x 2..4)
    Inhalt x = mitTeilen({{"torso", gitter(2, 2, 0.0F)}, {"l_arm", gitter(2, 2, 2.0F)}});
    const Schweissnetz s = schweissen(x, 0);
    expect(f("zwei Gitter mit 9 Ecken, drei am selben Ort: %d Punkte", s.zahl()), s.zahl() == 15);
    const int p = s.punktVon[0][2];   // (2, 0, 0): torso Ecke 2 und l_arm Ecke 0
    expect("der Punkt an der Grenze fuehrt beide Ecken", p == s.punktVon[1][0] && s.ecken[static_cast<std::size_t>(p)].size() == 2U);
    // Nachbarn von (2, 1, 0) (torso Ecke 5 = l_arm Ecke 3): ueber beide Flaechen
    const std::vector<int>& nb = s.nachbarn[static_cast<std::size_t>(s.punktVon[0][5])];
    expect(f("ein Punkt der Grenze hat Nachbarn in beiden Flaechen (%d)", static_cast<int>(nb.size())),
           std::find(nb.begin(), nb.end(), s.punktVon[0][4]) != nb.end() && std::find(nb.begin(), nb.end(), s.punktVon[1][4]) != nb.end());
    // Wachsen ueber die Grenze: von (1, 1) im torso
    const std::vector<EckOrt> gewachsen = eckenWachsen(x, 0, {EckOrt{0, 4}});
    const EckenMaske m = eckenMaske(x, 0, gewachsen);
    // Diagonale der Vierecke von (i, j) nach (i + 1, j + 1): Nachbarn von (1, 1) sind (0,0) (1,0) (0,1) (2,1) (1,2) (2,2)
    int zahl = 0;
    for (const char c : m[0]) { zahl += c != 0 ? 1 : 0; }
    expect(f("Wachsen von (1, 1): die Ecke und ihre sechs Nachbarn im torso (%d)", zahl), zahl == 7 && m[0][0] != 0 && m[0][5] != 0 && m[0][8] != 0 && m[0][2] == 0);
    expect("Wachsen nimmt die Ecken gleichen Orts der Nachbarflaeche mit ((2, 1) und (2, 2) im l_arm)", m[1].size() == 9U && m[1][3] != 0 && m[1][6] != 0 && m[1][0] == 0);
    // (1, 1) bleibt - und die Randecke (0, 0): ihre drei Nachbarn (1,0), (0,1), (1,1) sind alle gewaehlt
    const std::vector<EckOrt> zurueck = eckenSchrumpfen(x, 0, gewachsen);
    expect(f("Schrumpfen danach: es bleiben (1, 1) und die Randecke (0, 0), deren Nachbarn alle gewaehlt sind (%d Ecken)", static_cast<int>(zurueck.size())),
           zurueck.size() == 2U && zurueck[0].teil == 0 && zurueck[0].ecke == 0 && zurueck[1].teil == 0 && zurueck[1].ecke == 4);
    std::vector<EckOrt> alle;
    for (int t = 0; t < 2; ++t) {
        for (int e = 0; e < 9; ++e) { alle.push_back(EckOrt{t, e}); }
    }
    expect("Schrumpfen mit allen Ecken gewaehlt: nichts geschieht", eckenSchrumpfen(x, 0, alle).size() == alle.size());
    // Ring und Schleife auf einem Gitter 4 x 3 aus Dreiecken (ohne Netz::polyVon)
    Inhalt g = mitTeilen({{"gitter", gitter(4, 3)}});
    const auto nr = [](int i, int j) { return j * 5 + i; };
    const std::vector<EckOrt> kante = {EckOrt{0, nr(1, 1)}, EckOrt{0, nr(2, 1)}};   // die waagrechte Kante (1,1)-(2,1)
    const EckenMaske schleife = eckenMaske(g, 0, eckenSchleife(g, 0, kante));
    int inReihe = 0;
    int sonst = 0;
    for (int j = 0; j <= 3; ++j) {
        for (int i = 0; i <= 4; ++i) { (j == 1 ? inReihe : sonst) += schleife[0][static_cast<std::size_t>(nr(i, j))] != 0 ? 1 : 0; }
    }
    expect(f("Schleife ueber die Kante (1,1)-(2,1): die ganze Reihe j = 1 (%d von 5, sonst %d)", inReihe, sonst), inReihe == 5 && sonst == 0);
    const EckenMaske ring = eckenMaske(g, 0, eckenRing(g, 0, kante));
    int inSpalten = 0;
    sonst = 0;
    for (int j = 0; j <= 3; ++j) {
        for (int i = 0; i <= 4; ++i) { ((i == 1 || i == 2) ? inSpalten : sonst) += ring[0][static_cast<std::size_t>(nr(i, j))] != 0 ? 1 : 0; }
    }
    expect(f("Ring ueber dieselbe Kante: die parallelen Kanten, also die Spalten i = 1 und i = 2 (%d von 8, sonst %d)", inSpalten, sonst),
           inSpalten == 8 && sonst == 0);
    expect("Ring und Schleife mit einer einzelnen Ecke: die Auswahl bleibt", eckenRing(g, 0, {EckOrt{0, nr(1, 1)}}).size() == 1U &&
                                                                               eckenSchleife(g, 0, {EckOrt{0, nr(1, 1)}}).size() == 1U);
    expect("Ring und Schleife aendern das Dokument nicht (kein polyVon)", g.teile[0].stufen[0].polyVon.empty());
}

// --- Regeln: Sperre und Auto-Normalisieren --------------------------------------------------------------------------------
void pruefeRegeln() {
    std::printf("Regeln: Sperre je Knochen, Auto-Normalisieren\n");
    {
        GewichtListe l = {{1, 1.0F}, {0, 0.4F}};   // Knochen 0 eben auf 0,4 gemalt
        normalisierenNachMalen(l, 0, {});
        expect("gemalt 0,4 neben einem Knochen mit 1: der gemalte bleibt, der andere wird 0,6", nah(in(l, 0), 0.4F) && nah(in(l, 1), 0.6F));
        GewichtListe einer = {{0, 0.3F}};
        normalisierenNachMalen(einer, 0, {});
        expect("der einzige Knochen einer Ecke bleibt 1 (auch nach Abziehen)", einer.size() == 1U && einer[0].gewicht == 1.0F);
        GewichtListe null = {{1, 0.6F}, {2, 0.4F}};
        normalisierenNachMalen(null, 0, {});   // auf 0 gemalt: der Eintrag fehlt
        expect("auf 0 gemalt: die anderen behalten Summe 1, kein Eintrag mit 0", null.size() == 2U && nah(summe(null), 1.0F));
        std::vector<char> sperre(4, 0);
        sperre[2] = 1;
        GewichtListe g = {{1, 0.6F}, {2, 0.4F}, {0, 0.8F}};   // 2 gesperrt, 0 auf 0,8 gemalt
        normalisierenNachMalen(g, 0, sperre);
        expect(f("gesperrt 0,4, gemalt 0,8: der gesperrte bleibt 0,4, der gemalte wird 0,6, der freie 0 (%.3f %.3f %.3f)", static_cast<double>(in(g, 2)),
                 static_cast<double>(in(g, 0)), static_cast<double>(in(g, 1))),
               in(g, 2) == 0.4F && nah(in(g, 0), 0.6F) && in(g, 1) == 0.0F && nah(summe(g), 1.0F));
        GewichtListe h = {{1, 0.5F}, {2, 0.3F}, {3, 0.2F}, {0, 0.2F}};   // 2 gesperrt, 0 auf 0,2 gemalt
        normalisierenNachMalen(h, 0, sperre);
        // frei bleiben 1 und 3 mit zusammen 0,7; Rest 1 - 0,3 - 0,2 = 0,5 -> 1: 0,5 * 0,5/0,7, 3: 0,2 * 0,5/0,7
        expect("gesperrt 0,3, gemalt 0,2: die freien teilen sich 0,5 im Verhaeltnis 5 : 2", in(h, 2) == 0.3F && in(h, 0) == 0.2F &&
                                                                                           nah(in(h, 1), 0.5F * 0.5F / 0.7F) && nah(in(h, 3), 0.2F * 0.5F / 0.7F));
    }
    {
        Netz n = gitter(1, 1);   // vier Ecken an Knochen 0
        MalRegeln r;
        r.autoNormalisieren = true;
        expect("Malen mit Auto-Normalisieren: Hinzufuegen 0,25 an Knochen 3", malen(n, {{0, 1.0F}}, 3, PinselArt::Hinzufuegen, 0.25F, r) == 1);
        expect("... Knochen 3 traegt 0,25, Knochen 0 den Rest 0,75", nah(gewichtVon(n, 0, 3), 0.25F) && nah(gewichtVon(n, 0, 0), 0.75F));
        r.gesperrt.assign(8, 0);
        r.gesperrt[3] = 1;
        expect("ein gesperrter Knochen laesst sich nicht malen", malen(n, {{0, 1.0F}}, 3, PinselArt::Setzen, 1.0F, r) == 0 && nah(gewichtVon(n, 0, 3), 0.25F));
        expect("... ein anderer daneben schon: der gesperrte bleibt 0,25", malen(n, {{0, 1.0F}}, 5, PinselArt::Setzen, 0.5F, r) == 1 &&
                                                                         gewichtVon(n, 0, 3) == 0.25F && nah(gewichtVon(n, 0, 5), 0.5F) && nah(gewichtVon(n, 0, 0), 0.25F));
        MalRegeln ohne;
        (void)malen(n, {{1, 1.0F}}, 3, PinselArt::Hinzufuegen, 0.25F, ohne);
        expect("ohne Auto-Normalisieren aendert der Pinsel nur seinen Knochen (wie bis rc611)", gewichtVon(n, 1, 0) == 1.0F && gewichtVon(n, 1, 3) == 0.25F);
    }
}

// --- Pinsel: Verwischen und Verlauf -------------------------------------------------------------------------------------------
void pruefePinsel() {
    std::printf("Pinsel wie Blender: Verwischen, Verlauf\n");
    {
        // ein Streifen 4 x 1: Knochen 3 mit 0 / 0,25 / 0,5 / 0,75 / 1 von links nach rechts (beide Reihen)
        Netz n = gitter(4, 1);
        malAnlegen(n);
        std::vector<std::array<float, 2>> ebene(n.ecken.size());
        for (std::size_t e = 0; e < n.ecken.size(); ++e) {
            const int i = static_cast<int>(e % 5U);
            n.mal[e] = {{0, 1.0F}};
            if (i > 0) { n.mal[e].push_back({3, 0.25F * static_cast<float>(i)}); }
            ebene[e] = {n.ecken[e].ort[0], n.ecken[e].ort[1]};
        }
        const float nachRechts[2] = {1.0F, 0.0F};
        Netz a = n;
        expect("Verwischen nach rechts an (2, 0) mit Staerke 1", verwischen(a, {{2, 1.0F}}, 3, ebene, nachRechts) == 1);
        expect(f("... die Ecke nimmt das Gewicht ihres linken Nachbarn: 0,25 (%.3f)", static_cast<double>(gewichtVon(a, 2, 3))), nah(gewichtVon(a, 2, 3), 0.25F));
        const float nachLinks[2] = {-3.0F, 0.0F};
        Netz b = n;
        (void)verwischen(b, {{2, 0.5F}}, 3, ebene, nachLinks);
        expect(f("nach links mit Staerke 0,5: halb zum rechten Nachbarn, 0,625 (%.3f)", static_cast<double>(gewichtVon(b, 2, 3))), nah(gewichtVon(b, 2, 3), 0.625F));
        const float still[2] = {0.0F, 0.0F};
        Netz c = n;
        expect("ohne Bewegung des Pinsels geschieht nichts", verwischen(c, {{2, 1.0F}}, 3, ebene, still) == 0);
        const float nachOben[2] = {0.0F, 1.0F};
        Netz d = n;
        // unter (2, 0) liegt nichts: kein Nachbar entgegen der Bewegung (der Nachbar (2,1) liegt in Richtung der Bewegung)
        expect("kein Nachbar entgegen der Bewegung: die Ecke bleibt", verwischen(d, {{2, 1.0F}}, 3, ebene, nachOben) == 0 && nah(gewichtVon(d, 2, 3), 0.5F));
        MalRegeln r;
        r.gesperrt.assign(4, 0);
        r.gesperrt[3] = 1;
        Netz g = n;
        expect("Verwischen an einem gesperrten Knochen: nichts", verwischen(g, {{2, 1.0F}}, 3, ebene, nachRechts, r) == 0);
    }
    {
        // Verlauf ueber einen Streifen 8 x 1 von x = 2 nach x = 6: davor ganz, bei x = 4 die Haelfte der Kurve, ab x = 6 nichts
        Netz n = gitter(8, 1);
        std::vector<std::array<float, 2>> schirm(n.ecken.size());
        std::vector<char> sichtbar(n.ecken.size(), 1);
        for (std::size_t e = 0; e < n.ecken.size(); ++e) { schirm[e] = {n.ecken[e].ort[0] * 10.0F, n.ecken[e].ort[1] * 10.0F}; }
        const float start[2] = {20.0F, 5.0F};
        const float ende[2] = {60.0F, 5.0F};
        Netz a = n;
        const int geaendert = verlauf(a, schirm, sichtbar, start, ende, VerlaufArt::Linear, 3, PinselArt::Setzen, 0.8F, 1.0F);
        expect(f("Verlauf linear: 12 Ecken geaendert (x 0..5 in beiden Reihen) (%d)", geaendert), geaendert == 12);
        expect("... vor dem Anfang und am Anfang das ganze Gewicht 0,8", nah(gewichtVon(a, 0, 3), 0.8F) && nah(gewichtVon(a, 2, 3), 0.8F));
        // x = 3: Lage 0,25 -> p = 0,75 -> 3p^2 - 2p^3 = 0,84375; x = 4: 0,5; x = 5: 0,15625
        expect(f("... dazwischen nach der Pinselkurve: 0,675 / 0,4 / 0,125 (%.4f %.4f %.4f)", static_cast<double>(gewichtVon(a, 3, 3)),
                 static_cast<double>(gewichtVon(a, 4, 3)), static_cast<double>(gewichtVon(a, 5, 3))),
               nah(gewichtVon(a, 3, 3), 0.8F * 0.84375F) && nah(gewichtVon(a, 4, 3), 0.4F) && nah(gewichtVon(a, 5, 3), 0.8F * 0.15625F));
        expect("... am Ende und dahinter bleibt die Ecke", gewichtVon(a, 6, 3) == 0.0F && gewichtVon(a, 8, 3) == 0.0F);
        Netz b = n;
        (void)verlauf(b, schirm, sichtbar, start, ende, VerlaufArt::Linear, 3, PinselArt::Hinzufuegen, 0.5F, 0.5F);
        expect("Mischung Hinzufuegen mit Staerke 0,5: am Anfang 0,25", nah(gewichtVon(b, 2, 3), 0.25F));
        Netz c = n;
        std::vector<char> halb = sichtbar;
        halb[1] = 0;
        halb[10] = 0;   // (1, 0) und (1, 1)
        (void)verlauf(c, schirm, halb, start, ende, VerlaufArt::Linear, 3, PinselArt::Setzen, 0.8F, 1.0F);
        expect("eine verdeckte Ecke bleibt", gewichtVon(c, 1, 3) == 0.0F && nah(gewichtVon(c, 0, 3), 0.8F));
        Netz d = n;
        const float mitte[2] = {40.0F, 0.0F};
        const float rand[2] = {60.0F, 0.0F};
        (void)verlauf(d, schirm, sichtbar, mitte, rand, VerlaufArt::Radial, 3, PinselArt::Setzen, 1.0F, 1.0F);
        expect(f("Verlauf radial um (4, 0) mit Radius 2: Mitte 1, Abstand 1 nach beiden Seiten 0,5, Abstand 2 nichts (%.3f %.3f %.3f %.3f)",
                 static_cast<double>(gewichtVon(d, 4, 3)), static_cast<double>(gewichtVon(d, 3, 3)), static_cast<double>(gewichtVon(d, 5, 3)),
                 static_cast<double>(gewichtVon(d, 6, 3))),
               nah(gewichtVon(d, 4, 3), 1.0F) && nah(gewichtVon(d, 3, 3), 0.5F) && nah(gewichtVon(d, 5, 3), 0.5F) && gewichtVon(d, 6, 3) == 0.0F);
        MalRegeln r;
        r.autoNormalisieren = true;
        Netz g = n;
        (void)verlauf(g, schirm, sichtbar, start, ende, VerlaufArt::Linear, 3, PinselArt::Setzen, 0.8F, 1.0F, r);
        expect("Verlauf mit Auto-Normalisieren: der andere Knochen traegt den Rest", nah(gewichtVon(g, 4, 3), 0.4F) && nah(gewichtVon(g, 4, 0), 0.6F));
        const float gleich[2] = {20.0F, 5.0F};
        Netz h = n;
        expect("Anfang gleich Ende: nichts", verlauf(h, schirm, sichtbar, start, gleich, VerlaufArt::Linear, 3, PinselArt::Setzen, 0.8F, 1.0F) == 0);
    }
}

// --- Gewichts-Werkzeug und Bereinigen -----------------------------------------------------------------------------------------
void pruefeWerkzeug() {
    std::printf("Gewichts-Werkzeug (Setzen, Skalieren) und Bereinigen\n");
    Inhalt x = mitTeilen({{"torso", gitter(2, 2, 0.0F)}, {"l_arm", gitter(2, 2, 2.0F)}});
    MalRegeln r;
    r.autoNormalisieren = true;
    // gewaehlt: nur torso Ecke 5 = (2, 1) - an der Grenze zum l_arm (dort Ecke 3)
    const EckenMaske grenze = eckenMaske(x, 0, {EckOrt{0, 5}});
    expect("Setzen 0,5 an Knochen 3 auf eine Ecke der Grenze: beide Ecken des Punkts", gewichtSetzen(x, 0, grenze, 3, 0.5F, r) == 2);
    const Netz& torso = x.teile[0].stufen[0];
    const Netz& arm = x.teile[1].stufen[0];
    expect("... die Ecke gleichen Orts im l_arm traegt dasselbe (kein Riss)", gewichtVon(torso, 5, 3) == 0.5F && gewichtVon(arm, 3, 3) == 0.5F &&
                                                                               nah(gewichtVon(arm, 3, 0), 0.5F));
    expect("Skalieren mit 0,95 (Vorgabe von 3ds Max): 0,475, der andere Knochen 0,525", gewichtSkalieren(x, 0, grenze, 3, 0.95F, r) == 2 &&
                                                                                      nah(gewichtVon(torso, 5, 3), 0.475F) && nah(gewichtVon(arm, 3, 0), 0.525F));
    expect("Skalieren ueber 1 hinaus endet bei 1", gewichtSkalieren(x, 0, grenze, 3, 100.0F, r) == 2 && gewichtVon(torso, 5, 3) == 1.0F);
    MalRegeln gesperrt = r;
    gesperrt.gesperrt.assign(8, 0);
    gesperrt.gesperrt[3] = 1;
    expect("Setzen an einem gesperrten Knochen: nichts", gewichtSetzen(x, 0, grenze, 3, 0.1F, gesperrt) == 0 && gewichtVon(torso, 5, 3) == 1.0F);

    // Bereinigen: eine Ecke mit sechs Knochen, nicht normiert
    Inhalt y = mitTeilen({{"torso", gitter(2, 2)}});
    Netz& n = y.teile[0].stufen[0];
    malAnlegen(n);
    n.mal[4] = {{0, 0.5F}, {1, 0.4F}, {2, 0.3F}, {3, 0.2F}, {4, 0.06F}, {5, 0.04F}};
    {
        Inhalt a = y;
        const int entfernt = begrenzen(a, 0, nullptr, 4);
        const GewichtListe& l = a.teile[0].stufen[0].mal[4];
        expect(f("Begrenzen auf 4: die zwei kleinsten fallen weg (%d), die Gewichte bleiben (nicht normiert wie in Blender)", entfernt),
               entfernt == 2 && l.size() == 4U && in(l, 4) == 0.0F && in(l, 5) == 0.0F && in(l, 0) == 0.5F && nah(summe(l), 1.4F));
        Inhalt b = y;
        (void)begrenzen(b, 0, nullptr, 4, r);
        expect("Begrenzen mit Auto-Normalisieren: Summe 1, Verhaeltnis bleibt", nah(summe(b.teile[0].stufen[0].mal[4]), 1.0F) &&
                                                                               nah(in(b.teile[0].stufen[0].mal[4], 0), 0.5F / 1.4F));
    }
    {
        Inhalt a = y;
        const int entfernt = saeubern(a, 0, nullptr, 0.05F, true);
        const GewichtListe& l = a.teile[0].stufen[0].mal[4];
        expect(f("Saeubern mit Grenze 0,05: ein Gewicht faellt weg (0,04), 0,06 bleibt (%d)", entfernt), entfernt == 1 && in(l, 5) == 0.0F && in(l, 4) == 0.06F);
        Inhalt b = y;
        expect("Saeubern mit Grenze 0 (Vorgabe von Blender und 3ds Max): nichts faellt weg", saeubern(b, 0, nullptr, 0.0F, true) == 0);
        Inhalt c = y;
        (void)saeubern(c, 0, nullptr, 0.99F, true);
        expect("Saeubern mit Grenze 0,99: jeder Ecke bleibt eine Zuordnung", c.teile[0].stufen[0].mal[4].size() == 1U);
        Inhalt d = y;
        MalRegeln s5;
        s5.gesperrt.assign(8, 0);
        s5.gesperrt[5] = 1;
        expect("ein gesperrter Knochen wird nicht gesaeubert", saeubern(d, 0, nullptr, 0.05F, true, s5) == 0 && in(d.teile[0].stufen[0].mal[4], 5) == 0.04F);
    }
    {
        Inhalt a = y;
        const int z = normalisierenAlle(a, 0, nullptr, 1, true);
        const GewichtListe& l = a.teile[0].stufen[0].mal[4];
        // der aktive Knochen 1 (0,4) bleibt, die uebrigen (Summe 1,1) teilen sich 0,6
        expect(f("Alles normalisieren mit gehaltenem aktivem Knochen: er bleibt 0,4, Summe 1 (%d Ecken)", z),
               z == 1 && in(l, 1) == 0.4F && nah(summe(l), 1.0F) && nah(in(l, 0), 0.5F * 0.6F / 1.1F));
        Inhalt b = y;
        (void)normalisierenAlle(b, 0, nullptr, -1, false);
        expect("ohne aktiven Knochen: alle im selben Verhaeltnis", nah(in(b.teile[0].stufen[0].mal[4], 0), 0.5F / 1.5F) && nah(summe(b.teile[0].stufen[0].mal[4]), 1.0F));
        MalRegeln alle;
        alle.gesperrt.assign(6, 1);
        Inhalt c = y;
        expect("alle Knochen gesperrt: nichts geschieht, Rueckgabe -1", normalisierenAlle(c, 0, nullptr, -1, false, alle) == -1 && nah(summe(c.teile[0].stufen[0].mal[4]), 1.5F));
        const EckenMaske nurAndere = eckenMaske(y, 0, {EckOrt{0, 0}});
        Inhalt d = y;
        (void)normalisierenAlle(d, 0, &nurAndere, -1, false);
        expect("mit Auswahl nur die gewaehlten Ecken", nah(summe(d.teile[0].stufen[0].mal[4]), 1.5F));
    }
    {
        // Glaetten: die Mitte (1, 1) eines Gitters 2 x 2 hat Knochen 3 mit 0,8, ihre sechs Nachbarn 0
        Inhalt a = mitTeilen({{"torso", gitter(2, 2)}});
        Netz& na = a.teile[0].stufen[0];
        malAnlegen(na);
        na.mal[4] = {{0, 0.2F}, {3, 0.8F}};
        Inhalt b = a;
        const EckenMaske mitte = eckenMaske(b, 0, {EckOrt{0, 4}});
        (void)glaettenAlle(b, 0, &mitte, 0.5F, 1);
        const GewichtListe& l = b.teile[0].stufen[0].mal[4];
        // Knochen 3: 0,8 * 0,5 + 0 * 0,5 = 0,4; Knochen 0: 0,2 * 0,5 + 1 * 0,5 = 0,6
        expect(f("Glaetten der Mitte mit Faktor 0,5: 0,8 -> 0,4, der andere Knochen 0,2 -> 0,6 (%.3f %.3f)", static_cast<double>(in(l, 3)), static_cast<double>(in(l, 0))),
               nah(in(l, 3), 0.4F) && nah(in(l, 0), 0.6F));
        expect("... die nicht gewaehlten Nachbarn bleiben", b.teile[0].stufen[0].mal[0].size() == 1U && gewichtVon(b.teile[0].stufen[0], 0, 0) == 1.0F);
        Inhalt c = a;
        (void)glaettenAlle(c, 0, nullptr, 0.5F, 1);
        // (0, 0) hat die Nachbarn (1,0), (0,1), (1,1): Knochen 3 = 0 * 0,5 + (0 + 0 + 0,8) / 3 * 0,5
        expect("ohne Auswahl alle Ecken: (0, 0) bekommt ein Sechstel von 0,8", nah(gewichtVon(c.teile[0].stufen[0], 0, 3), 0.8F / 6.0F) &&
                                                                              nah(gewichtVon(c.teile[0].stufen[0], 0, 0) + gewichtVon(c.teile[0].stufen[0], 0, 3), 1.0F));
        Inhalt d = a;
        (void)glaettenAlle(d, 0, &mitte, 0.5F, 2);
        expect("zwei Wiederholungen: 0,8 -> 0,4 -> 0,2", nah(gewichtVon(d.teile[0].stufen[0], 4, 3), 0.2F));
        MalRegeln s3;
        s3.gesperrt.assign(8, 0);
        s3.gesperrt[3] = 1;
        s3.autoNormalisieren = true;
        Inhalt e = a;
        (void)glaettenAlle(e, 0, &mitte, 0.5F, 1, s3);
        expect("ein gesperrter Knochen wird nicht geglaettet (0,8 bleibt, der freie fuellt auf 0,2)", gewichtVon(e.teile[0].stufen[0], 4, 3) == 0.8F &&
                                                                                                      nah(gewichtVon(e.teile[0].stufen[0], 4, 0), 0.2F));
    }
}

// --- Spiegeln ---------------------------------------------------------------------------------------------------------------------
const std::vector<std::string> kNamen = {"model_root", "lhand", "rhand", "lower_lumbar", "l_arm", "r_arm", "knee_l", "knee_r", "lonely", "l_only"};

// Zwei Arme (x 5..6 und -6..-5) und ein Rumpf in der Mitte (x -1..1); die Arme je mit einer zweiten Ecke am Ort (6, 0) bzw.
// (-6, 0) (UV-Naht) und der Rumpf mit einer Ecke am Ort (-5, 0) (Grenze zur rechten Flaeche).
Inhalt figur() {
    Netz links;
    links.aufbauen({ecke(5, 0, 0, 1), ecke(6, 0, 0, 1), ecke(5, 1, 0, 1), ecke(6, 0, 0, 1), ecke(6, 1, 0, 1)}, {0, 1, 2, 2, 3, 4});
    Netz rechts;
    rechts.aufbauen({ecke(-5, 0, 0, 0), ecke(-6, 0, 0, 0), ecke(-5, 1, 0, 0), ecke(-6, 0, 0, 0), ecke(-6, 1, 0, 0)}, {0, 2, 1, 2, 4, 3});
    Netz rumpf;
    rumpf.aufbauen({ecke(-1, 0, 0, 0), ecke(1, 0, 0, 0), ecke(0, 1, 0, 0), ecke(-5, 0, 0, 0), ecke(0, 0, 0, 0)}, {0, 1, 2, 3, 0, 2, 0, 4, 2});
    return mitTeilen({{"l_arm", std::move(links)}, {"r_arm", std::move(rechts)}, {"torso", std::move(rumpf)}});
}

void pruefeSpiegeln() {
    std::printf("Spiegeln\n");
    expect("Seiten aus den Namen: lhand links, rhand rechts, l_arm links, knee_r rechts, lower_lumbar und lonely keine, l_only links",
           knochenSeite(kNamen, 1) == 1 && knochenSeite(kNamen, 2) == -1 && knochenSeite(kNamen, 4) == 1 && knochenSeite(kNamen, 7) == -1 &&
               knochenSeite(kNamen, 3) == 0 && knochenSeite(kNamen, 8) == 0 && knochenSeite(kNamen, 0) == 0 && knochenSeite(kNamen, 9) == 1);
    {
        // der Fund am Spiegeln von rc611: am Ziel liegen zwei Ecken am selben Ort (UV-Naht), dazu eine in der Nachbarflaeche
        Inhalt x = figur();
        Netz& l = x.teile[0].stufen[0];
        malAnlegen(l);
        l.mal[1] = {{1, 0.75F}, {3, 0.25F}};
        l.mal[3] = l.mal[1];
        l.mal[0] = {{1, 0.5F}, {4, 0.5F}};
        const int z = spiegeln(x, 0, kNamen, 1, 0.01F);
        const Netz& r = x.teile[1].stufen[0];
        const Netz& rumpf = x.teile[2].stufen[0];
        expect(f("von +X nach -X (%d Ecken): BEIDE Ecken am Ort (-6, 0) tragen rhand 0,75", z), gewichtVon(r, 1, 2) == 0.75F && gewichtVon(r, 3, 2) == 0.75F);
        expect("... und die Ecke gleichen Orts im Rumpf (-5, 0) traegt dasselbe wie die im r_arm (kein Riss)", gewichtVon(r, 0, 2) == 0.5F && gewichtVon(r, 0, 5) == 0.5F &&
                                                                                                               gewichtVon(rumpf, 3, 2) == 0.5F && gewichtVon(rumpf, 3, 5) == 0.5F);
        expect("Punkte der Mitte bleiben", rumpf.mal.empty() || (gewichtVon(rumpf, 2, 0) == 1.0F && gewichtVon(rumpf, 4, 0) == 1.0F));
    }
    {
        Inhalt x = figur();
        const Schweissnetz s = schweissen(x, 0);
        const std::vector<int> partner = spiegelPartner(x, 0, s, 0.01F);
        const SpiegelBefund b = spiegelBefund(kNamen, s, partner);
        expect(f("Befund: 3 Knochenpaare von 4 (l_only ohne Partner): %d / %d", b.knochenPaare, b.knochenPaareSoll), b.knochenPaare == 3 && b.knochenPaareSoll == 4);
        // Punkte: l_arm 4, r_arm 4 (einer davon mit dem Rumpf geteilt), Rumpf 4 eigene: (-1,0) (1,0) (0,1) (0,0)
        expect(f("... %d Punkte, %d ohne Gegenstueck, %d in der Mitte", b.punkte, b.ohneGegenstueck, b.mitte), b.punkte == 12 && b.ohneGegenstueck == 0 && b.mitte == 2);
        bool armPaar = false;
        for (const auto& [a, c] : b.teilPaare) { armPaar = armPaar || (a == 0 && c == 1); }
        expect("... flaechenuebergreifend: l_arm <-> r_arm", armPaar);
        // ein Punkt verschoben: ohne Gegenstueck bei kleiner Toleranz, mit bei grosser
        Inhalt y = figur();
        y.teile[0].stufen[0].ecken[4].ort[1] = 1.3F;   // (6, 1) -> (6, 1,3)
        const Schweissnetz sy = schweissen(y, 0);
        const SpiegelBefund eng = spiegelBefund(kNamen, sy, spiegelPartner(y, 0, sy, 0.1F));
        const SpiegelBefund weit = spiegelBefund(kNamen, sy, spiegelPartner(y, 0, sy, 0.5F));
        expect(f("Toleranz 0,1: zwei Punkte ohne Gegenstueck (%d, %d Ecken); Toleranz 0,5: keiner (%d)", eng.ohneGegenstueck, eng.eckenOhne, weit.ohneGegenstueck),
               eng.ohneGegenstueck == 2 && eng.eckenOhne == 2 && weit.ohneGegenstueck == 0);
        // Topologie, ohne Lage: zwei getrennte, gleich gebaute Arme, links ein Punkt verschoben. Der Arm ist ein Streifen aus
        // vier Dreiecken (v0..v5) mit einem Ohr (w) an der Kante v0-v2 - ein Netz ohne innere Symmetrie (ein blosser Streifen
        // oder Faecher laesst sich in sich spiegeln; dann tragen vier Punkte dieselbe Kennzahl und es gibt keine Paare).
        const auto arm = [](float vz, float verschoben) {
            Netz n;
            n.aufbauen({ecke(vz * 5, 0, 0), ecke(vz * 6, 0, 0), ecke(vz * 5, 1, 0), ecke(vz * 6, 1, 0), ecke(vz * 5, 2, 0), ecke(vz * 6, 2 + verschoben, 0), ecke(vz * 4, 1, 0)},
                       {0, 1, 2, 1, 2, 3, 2, 3, 4, 3, 4, 5, 0, 2, 6});
            return n;
        };
        Inhalt arme = mitTeilen({{"l_arm", arm(1.0F, 0.3F)}, {"r_arm", arm(-1.0F, 0.0F)}});
        const Schweissnetz sa = schweissen(arme, 0);
        const std::vector<int> topo = spiegelPartnerTopologie(sa);
        bool jeder = topo.size() == 14U;
        for (int e = 0; e < 7 && jeder; ++e) { jeder = topo[static_cast<std::size_t>(sa.punktVon[0][static_cast<std::size_t>(e)])] == sa.punktVon[1][static_cast<std::size_t>(e)]; }
        const SpiegelBefund lage = spiegelBefund(kNamen, sa, spiegelPartner(arme, 0, sa, 0.1F));
        expect(f("Topologie-Spiegeln zweier gleich gebauter Arme: jeder der 7 Punkte findet seinen Zwilling, auch der verschobene (ueber die Lage fehlen %d)", lage.ohneGegenstueck),
               jeder && lage.ohneGegenstueck == 2);
        // ein Streifen allein ist in sich spiegelbar: keine Paare (so verhaelt sich auch Blender - "matching, unique topology")
        Inhalt streifen = mitTeilen({{"l_arm", gitter(2, 1, 5.0F)}, {"r_arm", gitter(2, 1, -7.0F)}});
        const std::vector<int> ts = spiegelPartnerTopologie(schweissen(streifen, 0));
        expect("zwei Streifen mit innerer Symmetrie: kein Punkt bekommt einen Partner", std::all_of(ts.begin(), ts.end(), [](int p) { return p == -1; }));
    }
    {
        // wie 3ds Max: Seite -> Seite, Tauschen, nur Auswahl
        Inhalt x = figur();
        Netz& l = x.teile[0].stufen[0];
        malAnlegen(l);
        for (GewichtListe& g : l.mal) { g = {{1, 0.75F}, {3, 0.25F}}; }
        const Schweissnetz s = schweissen(x, 0);
        const std::vector<int> partner = spiegelPartner(x, 0, s, 0.01F);
        Inhalt a = x;
        const int z = spiegelnSeite(a, 0, kNamen, s, partner, 1, 0.01F);
        expect(f("+X -> -X: die vier Punkte des rechten Arms (sechs Ecken mit der im Rumpf) und der Rumpfpunkt (-1, 0) (%d)", z),
               z == 7 && gewichtVon(a.teile[1].stufen[0], 4, 2) == 0.75F);
        Inhalt b = x;
        (void)spiegelnSeite(b, 0, kNamen, s, partner, -1, 0.01F);
        expect("-X -> +X: der linke Arm bekommt die Gewichte des rechten (model_root)", gewichtVon(b.teile[0].stufen[0], 1, 0) == 1.0F && gewichtVon(b.teile[0].stufen[0], 1, 1) == 0.0F);
        Inhalt c = x;
        (void)spiegelnSeite(c, 0, kNamen, s, partner, 0, 0.01F);
        expect("Tauschen: links model_root, rechts rhand 0,75", gewichtVon(c.teile[0].stufen[0], 1, 0) == 1.0F && gewichtVon(c.teile[1].stufen[0], 1, 2) == 0.75F &&
                                                                gewichtVon(c.teile[1].stufen[0], 1, 3) == 0.25F);
        Inhalt d = x;
        const EckenMaske nur = eckenMaske(d, 0, {EckOrt{0, 4}});   // (6, 1) im linken Arm
        const int zn = spiegelnSeite(d, 0, kNamen, s, partner, 1, 0.01F, &nur);
        expect(f("nur Auswahl: allein der Partner der gewaehlten Ecke (%d Ecke)", zn), zn == 1 && gewichtVon(d.teile[1].stufen[0], 4, 2) == 0.75F &&
                                                                                      gewichtVon(d.teile[1].stufen[0], 1, 2) == 0.0F);
        // Tauschen an einem Punkt der Mitte: l <-> r
        Inhalt e = x;
        Netz& rumpf = e.teile[2].stufen[0];
        malAnlegen(rumpf);
        rumpf.mal[2] = {{1, 0.7F}, {2, 0.3F}};   // (0, 1): lhand 0,7, rhand 0,3
        (void)spiegelnSeite(e, 0, kNamen, s, partner, 0, 0.01F);
        expect("Tauschen: ein Punkt der Mitte tauscht seine Knochen l <-> r", gewichtVon(e.teile[2].stufen[0], 2, 2) == 0.7F && gewichtVon(e.teile[2].stufen[0], 2, 1) == 0.3F);
    }
    {
        // wie Blender: dvert_mirror_op
        Inhalt x = figur();
        Netz& l = x.teile[0].stufen[0];
        Netz& r = x.teile[1].stufen[0];
        malAnlegen(l);
        malAnlegen(r);
        for (GewichtListe& g : l.mal) { g = {{1, 0.75F}, {3, 0.25F}}; }   // links lhand 0,75
        for (GewichtListe& g : r.mal) { g = {{1, 0.1F}, {2, 0.2F}, {0, 0.7F}}; }   // rechts lhand 0,1, rhand 0,2
        const Schweissnetz s = schweissen(x, 0);
        const std::vector<int> partner = spiegelPartner(x, 0, s, 0.01F);
        BlenderSpiegel o;
        o.aktiv = 1;   // lhand
        Inhalt a = x;
        const SpiegelZahl za = spiegelnBlender(a, 0, kNamen, s, partner, o);
        // ohne Auswahl sind beide Seiten gewaehlt: tauschen. Aktive Gruppe: 0,75 <-> 0,1, dann die Namen des Paars
        // lhand/rhand an beiden Punkten: links lhand 0,1 -> rhand 0,1; rechts lhand 0,75 -> rhand 0,75 und rhand 0,2 -> lhand 0,2
        expect(f("Gruppe spiegeln mit Namentausch, ohne Auswahl (%d Paare, %d ohne): rechts rhand 0,75 / lhand 0,2, links rhand 0,1", za.gespiegelt, za.ohnePartner),
               za.gespiegelt == 5 && za.ohnePartner == 0 && gewichtVon(a.teile[1].stufen[0], 1, 2) == 0.75F && gewichtVon(a.teile[1].stufen[0], 1, 1) == 0.2F &&
                   gewichtVon(a.teile[0].stufen[0], 1, 2) == 0.1F && gewichtVon(a.teile[0].stufen[0], 1, 1) == 0.0F && gewichtVon(a.teile[0].stufen[0], 1, 3) == 0.25F);
        BlenderSpiegel ohneNamen = o;
        ohneNamen.namenTauschen = false;
        Inhalt b = x;
        (void)spiegelnBlender(b, 0, kNamen, s, partner, ohneNamen);
        expect("ohne Namentausch: nur das Gewicht der aktiven Gruppe wechselt die Seite (links 0,1, rechts 0,75)", gewichtVon(b.teile[0].stufen[0], 1, 1) == 0.1F &&
                                                                                                             gewichtVon(b.teile[1].stufen[0], 1, 1) == 0.75F &&
                                                                                                             gewichtVon(b.teile[1].stufen[0], 1, 2) == 0.2F);
        BlenderSpiegel alle = o;
        alle.alleGruppen = true;
        Inhalt c = x;
        (void)spiegelnBlender(c, 0, kNamen, s, partner, alle);
        // ganze Listen tauschen, dann alle Namen: links {lhand 0,1, rhand 0,2, root 0,7} -> {rhand 0,1, lhand 0,2, root 0,7}
        expect("alle Gruppen: links lhand 0,2 / rhand 0,1 / model_root 0,7, rechts rhand 0,75 / lower_lumbar 0,25",
               gewichtVon(c.teile[0].stufen[0], 1, 1) == 0.2F && gewichtVon(c.teile[0].stufen[0], 1, 2) == 0.1F && gewichtVon(c.teile[0].stufen[0], 1, 0) == 0.7F &&
                   gewichtVon(c.teile[1].stufen[0], 1, 2) == 0.75F && gewichtVon(c.teile[1].stufen[0], 1, 3) == 0.25F);
        // nur die rechte Seite gewaehlt: sie ist das Ziel, links bleibt
        Inhalt d = x;
        const EckenMaske rechts = eckenMaske(d, 0, {EckOrt{1, 0}, EckOrt{1, 1}, EckOrt{1, 2}, EckOrt{1, 4}});
        const SpiegelZahl zd = spiegelnBlender(d, 0, kNamen, s, partner, alle, &rechts);
        expect(f("nur eine Seite gewaehlt: sie bekommt die Gewichte der anderen, die bleibt (%d Paare)", zd.gespiegelt),
               zd.gespiegelt == 4 && gewichtVon(d.teile[1].stufen[0], 1, 2) == 0.75F && gewichtVon(d.teile[2].stufen[0], 0, 0) == 1.0F && gewichtVon(d.teile[1].stufen[0], 1, 3) == 0.25F &&
                   gewichtVon(d.teile[0].stufen[0], 1, 1) == 0.75F);
        // Punkte der Mitte bleiben
        Inhalt e = x;
        Netz& rumpf = e.teile[2].stufen[0];
        malAnlegen(rumpf);
        rumpf.mal[2] = {{1, 0.7F}, {2, 0.3F}};
        (void)spiegelnBlender(e, 0, kNamen, s, partner, alle);
        expect("ein Punkt der Mitte bleibt, wie er ist (wie Blender)", gewichtVon(e.teile[2].stufen[0], 2, 1) == 0.7F && gewichtVon(e.teile[2].stufen[0], 2, 2) == 0.3F);
        BlenderSpiegel nichts;
        nichts.gewichte = false;
        nichts.namenTauschen = false;
        Inhalt g = x;
        expect("weder Gewichte noch Namen: nichts", spiegelnBlender(g, 0, kNamen, s, partner, nichts).gespiegelt == 0);
    }
    {
        // Spiegeln beim Malen: der Partner bekommt genau das Gewicht des getupften Punkts, an allen seinen Ecken
        Inhalt x = figur();
        const Schweissnetz s = schweissen(x, 0);
        const std::vector<int> partner = spiegelPartner(x, 0, s, 0.01F);
        MalRegeln r;
        r.autoNormalisieren = true;
        std::vector<std::vector<Tupfer>> je(x.teile.size());
        je[0] = {{0, 0.5F}, {1, 1.0F}};   // (5, 0) halb, (6, 0) ganz
        (void)malen(x.teile[0].stufen[0], je[0], 4, PinselArt::Setzen, 0.6F, r);   // l_arm (Knochen 4)
        const int z = spiegelMalen(x, 0, s, partner, je, 4, spiegelKnochen(kNamen, 4), r);
        const Netz& rr = x.teile[1].stufen[0];
        expect(f("Spiegeln beim Malen: (-6, 0) an beiden Ecken r_arm 0,6, (-5, 0) 0,3 - auch im Rumpf (%d Ecken)", z),
               z == 4 && gewichtVon(rr, 1, 5) == 0.6F && gewichtVon(rr, 3, 5) == 0.6F && nah(gewichtVon(rr, 0, 5), 0.3F) && nah(gewichtVon(x.teile[2].stufen[0], 3, 5), 0.3F));
        expect("... normalisiert: der andere Knochen traegt den Rest", nah(gewichtVon(rr, 1, 0), 0.4F));
        // ein Punkt der Mitte mit einem Knochen ohne Seite: nichts doppelt
        std::vector<std::vector<Tupfer>> mitte(x.teile.size());
        mitte[2] = {{2, 1.0F}};   // (0, 1)
        (void)malen(x.teile[2].stufen[0], mitte[2], 3, PinselArt::Hinzufuegen, 0.25F, r);
        const int zm = spiegelMalen(x, 0, s, partner, mitte, 3, spiegelKnochen(kNamen, 3), r);
        expect("ein Punkt der Mitte mit einem Knochen ohne Seite wird nicht zweimal gemalt", zm == 0 && nah(gewichtVon(x.teile[2].stufen[0], 2, 3), 0.25F));
        // ein Punkt der Mitte mit einem Knochenpaar: beide Knochen gleich
        (void)malen(x.teile[2].stufen[0], mitte[2], 1, PinselArt::Setzen, 0.3F, r);
        (void)spiegelMalen(x, 0, s, partner, mitte, 1, spiegelKnochen(kNamen, 1), r);
        expect("ein Punkt der Mitte mit einem Knochenpaar: lhand und rhand gleich", nah(gewichtVon(x.teile[2].stufen[0], 2, 1), gewichtVon(x.teile[2].stufen[0], 2, 2)) &&
                                                                                     gewichtVon(x.teile[2].stufen[0], 2, 1) > 0.0F);
    }
}

// --- Grenzen des Spiels nach den Werkzeugen ---------------------------------------------------------------------------------------
void pruefeGrenzen() {
    std::printf("Grenzen des Spiels nach den Werkzeugen\n");
    // ein Gitter, an dem jede Ecke nacheinander viele Knochen bekommt; dann jedes Werkzeug und Uebernehmen
    Inhalt x = mitTeilen({{"torso", gitter(3, 3)}, {"l_arm", gitter(3, 3, 3.0F)}});
    MalRegeln r;
    r.autoNormalisieren = true;
    for (int t = 0; t < 2; ++t) {
        Netz& n = x.teile[static_cast<std::size_t>(t)].stufen[0];
        for (int e = 0; e < static_cast<int>(n.ecken.size()); ++e) {
            // die Knochen haengen am Ort: Ecken gleichen Orts der beiden Flaechen beginnen mit denselben Gewichten
            const int ort = static_cast<int>(n.ecken[static_cast<std::size_t>(e)].ort[0]) * 5 + static_cast<int>(n.ecken[static_cast<std::size_t>(e)].ort[1]);
            for (int k = 1; k <= 6; ++k) { (void)malen(n, {{e, 1.0F}}, (ort + k * 3) % 20 + 1, PinselArt::Hinzufuegen, 0.07F * static_cast<float>(k), r); }
        }
    }
    int ueberVier = 0;
    for (const Teil& t : x.teile) { ueberVier += eckenUeberVier(t.stufen[0]); }
    expect(f("vor den Werkzeugen: %d Ecken mit mehr als vier Knochen in der Malliste", ueberVier), ueberVier > 0);
    struct Fall {
        const char* name;
        void (*tun)(Inhalt&, const MalRegeln&);
    };
    const Fall faelle[] = {
        {"Begrenzen 4", [](Inhalt& y, const MalRegeln& rr) { (void)begrenzen(y, 0, nullptr, 4, rr); }},
        {"Saeubern 0,05", [](Inhalt& y, const MalRegeln& rr) { (void)saeubern(y, 0, nullptr, 0.05F, true, rr); }},
        {"Glaetten", [](Inhalt& y, const MalRegeln& rr) { (void)glaettenAlle(y, 0, nullptr, 0.5F, 2, rr); }},
        {"Alles normalisieren", [](Inhalt& y, const MalRegeln& rr) { (void)normalisierenAlle(y, 0, nullptr, 3, true, rr); }},
        {"Skalieren 0,95", [](Inhalt& y, const MalRegeln& rr) {
             std::vector<EckOrt> alle;
             for (int e = 0; e < 16; ++e) { alle.push_back(EckOrt{0, e}); }
             (void)gewichtSkalieren(y, 0, eckenMaske(y, 0, alle), 4, 0.95F, rr);
         }},
        {"Spiegeln tauschen", [](Inhalt& y, const MalRegeln&) {
             const Schweissnetz s = schweissen(y, 0);
             (void)spiegelnSeite(y, 0, std::vector<std::string>(32, "knochen"), s, spiegelPartner(y, 0, s, 0.5F), 0, 0.5F);
         }},
    };
    for (const Fall& fall : faelle) {
        Inhalt y = x;
        fall.tun(y, r);
        // die Ecken gleichen Orts der beiden Flaechen tragen dasselbe
        bool naht = true;
        const Schweissnetz s = schweissen(y, 0);
        for (const std::vector<EckOrt>& ecken : s.ecken) {
            for (std::size_t i = 1; i < ecken.size(); ++i) {
                const Netz& a = y.teile[static_cast<std::size_t>(ecken[0].teil)].stufen[0];
                const Netz& b = y.teile[static_cast<std::size_t>(ecken[i].teil)].stufen[0];
                for (int k = 0; k < 24; ++k) { naht = naht && gewichtVon(a, ecken[0].ecke, k) == gewichtVon(b, ecken[i].ecke, k); }
            }
        }
        (void)uebernehmen(y);
        int gewichteMax = 0;
        const bool spiel = wieImSpiel(y, &gewichteMax);
        const int knochen = knochenMax(y);
        const bool jka = knochen <= grenzen(Zielspiel::JkaSp).knochenJeFlaeche && gewichteMax <= grenzen(Zielspiel::JkaSp).gewichteJeEcke;
        const bool md = knochen <= grenzen(Zielspiel::MdSp).knochenJeFlaeche && gewichteMax <= grenzen(Zielspiel::MdSp).gewichteJeEcke;
        expect(f("%s, dann Uebernehmen: hoechstens %d Gewichte je Ecke, 10 Bit wie das Spiel, %d Knochen je Flaeche (JKA %d, Movie Duels %d), Naht gleich", fall.name, gewichteMax, knochen,
                 grenzen(Zielspiel::JkaSp).knochenJeFlaeche, grenzen(Zielspiel::MdSp).knochenJeFlaeche),
               spiel && gewichteMax <= 4 && jka && md && naht);
    }
    // Spiegeln kann eine Flaeche ueber die Grenze der Knochen bringen - die Pruefung des Spiels meldet es
    {
        std::vector<std::string> viele;
        for (int k = 0; k < 40; ++k) { viele.push_back((k < 20 ? "l_k" : "r_k") + std::to_string(k % 20)); }
        // eine Flaeche ueber die Mitte (x -6,5 .. 6,5, keine Ecke auf der Ebene), jede Ecke an einem von 20 linken Knochen
        Inhalt z = mitTeilen({{"torso", gitter(13, 2, -6.5F)}});
        Netz& nz = z.teile[0].stufen[0];
        malAnlegen(nz);
        for (std::size_t e = 0; e < nz.ecken.size(); ++e) { nz.mal[e] = {{static_cast<int>(e) % 20, 1.0F}}; }
        Inhalt vorher = z;
        (void)uebernehmen(vorher);
        const int davor = knochenMax(vorher);
        const Schweissnetz sz = schweissen(z, 0);
        (void)spiegelnSeite(z, 0, viele, sz, spiegelPartner(z, 0, sz, 0.01F), 1, 0.01F);   // +X -> -X: die Seite -X bekommt r_k*
        (void)uebernehmen(z);
        const int danach = knochenMax(z);
        const auto hatKnochen = [](const std::vector<Befund>& l) {
            return std::any_of(l.begin(), l.end(), [](const Befund& b) { return b.art == BefundArt::Knochen; });
        };
        expect(f("Spiegeln bringt eine Flaeche von %d auf %d Knochen: die Pruefung meldet die Grenze 32 fuer JKA und Movie Duels", davor, danach),
               davor == 20 && danach > 32 && hatKnochen(pruefeMass(mass(z.teile[0].stufen[0]), grenzen(Zielspiel::JkaSp))) &&
                   hatKnochen(pruefeMass(mass(z.teile[0].stufen[0]), grenzen(Zielspiel::MdSp))) && wieImSpiel(z));
    }
}

// --- Raven: eine Figur des Spiels -----------------------------------------------------------------------------------------------------
void pruefeRaven(const std::string& jaBase) {
    std::printf("Raven (%s)\n", jaBase.c_str());
    std::vector<GamePath> ordner(1);
    if (jaBase.empty() || !scanGamePath(jaBase, ordner[0]) || ordner[0].archives.empty()) {
        std::printf("  uebersprungen: keine .pk3 in \"%s\"\n", jaBase.c_str());
        return;
    }
    std::string bytes;
    GlaAnimation g;
    Inhalt kyle;
    if (!liesWieSpiel(ordner, "models/players/_humanoid/_humanoid.gla", false, bytes) || !readGla(bytes, g) ||
        !liesWieSpiel(ordner, "models/players/kyle/model.glm", false, bytes) || !ausGlm(bytes, kyle)) {
        std::printf("  uebersprungen: _humanoid.gla oder kyle nicht gefunden\n");
        return;
    }
    std::vector<std::string> namen;
    for (const GlaBone& b : g.bones) { namen.push_back(b.name); }
    const int links = linkeSeite(kyle, 0, namen);
    const Schweissnetz s = schweissen(kyle, 0);
    int ecken = 0;
    for (const std::vector<EckOrt>& e : s.ecken) { ecken += static_cast<int>(e.size()); }
    std::printf("    kyle: %d Ecken, %d Punkte, die linke Seite der Figur liegt bei x %s 0\n", ecken, s.zahl(), links > 0 ? ">" : (links < 0 ? "<" : "?"));
    expect("die linke Seite der Figur ist aus den Gewichten bestimmbar", links != 0);
    for (const float toleranz : {0.1F, 0.5F, 1.0F}) {
        const SpiegelBefund b = spiegelBefund(namen, s, spiegelPartner(kyle, 0, s, toleranz));
        std::printf("    Toleranz %.1f: Knochenpaare %d / %d, Punkte ohne Gegenstueck %d von %d (Ecken %d), Mitte %d, Flaechenpaare %d\n", static_cast<double>(toleranz),
                    b.knochenPaare, b.knochenPaareSoll, b.ohneGegenstueck, b.punkte, b.eckenOhne, b.mitte, static_cast<int>(b.teilPaare.size()));
        if (toleranz == 0.5F) {
            expect(f("Toleranz 0,5: jeder Knochen mit Seite hat seinen Partner (%d / %d)", b.knochenPaare, b.knochenPaareSoll), b.knochenPaare > 10 && b.knochenPaare == b.knochenPaareSoll);
            bool arme = false;
            const int la = kyle.teilIndex("l_arm");
            const int ra = kyle.teilIndex("r_arm");
            for (const auto& [a, c] : b.teilPaare) { arme = arme || (a == std::min(la, ra) && c == std::max(la, ra)); }
            expect("flaechenuebergreifend: l_arm <-> r_arm", la >= 0 && ra >= 0 && arme);
        }
    }
    const std::vector<int> topo = spiegelPartnerTopologie(s);
    int topoPaare = 0;
    for (std::size_t p = 0; p < topo.size(); ++p) { topoPaare += topo[p] >= 0 && topo[p] != static_cast<int>(p) ? 1 : 0; }
    std::printf("    Topologie-Spiegeln: %d von %d Punkten in Paaren (kyle ist nicht aus zwei gleich gebauten Haelften)\n", topoPaare, s.zahl());
    // links -> rechts mit der Vorgabe 0,5, Uebernehmen: Grenzen des Spiels, Naehte zu
    Inhalt x = kyle;
    const std::vector<int> partner = spiegelPartner(x, 0, s, 0.5F);
    const int z = spiegelnSeite(x, 0, namen, s, partner, links, 0.5F);
    bool naht = true;
    int zielPunkte = 0;
    for (std::size_t p = 0; p < s.ecken.size(); ++p) {
        const std::vector<EckOrt>& e = s.ecken[p];
        const float px = s.ort(kyle, 0, static_cast<int>(p))[0];
        // beschrieben: ein Punkt der Zielseite mit einem Gegenstueck auf der Quellseite
        if (partner[p] < 0 || partner[p] == static_cast<int>(p) || (px > 0.5F ? 1 : (px < -0.5F ? -1 : 0)) != -links) { continue; }
        const float qx = s.ort(kyle, 0, partner[p])[0];
        if ((qx > 0.5F ? 1 : (qx < -0.5F ? -1 : 0)) != links) { continue; }
        ++zielPunkte;
        for (std::size_t i = 1; i < e.size(); ++i) {
            const Netz& a = x.teile[static_cast<std::size_t>(e[0].teil)].stufen[0];
            const Netz& b = x.teile[static_cast<std::size_t>(e[i].teil)].stufen[0];
            for (int k = 0; k < static_cast<int>(namen.size()); ++k) { naht = naht && gewichtVon(a, e[0].ecke, k) == gewichtVon(b, e[i].ecke, k); }
        }
    }
    (void)uebernehmen(x);
    int gewichteMax = 0;
    const bool spiel = wieImSpiel(x, &gewichteMax);
    std::string flaeche;
    const int knochen = knochenMax(x, &flaeche);
    expect(f("links -> rechts gespiegelt (%d Ecken), Uebernehmen: hoechstens %d Gewichte je Ecke, wie das Spiel liest, %d Knochen in %s (Grenze %d)", z, gewichteMax, knochen,
             flaeche.c_str(), grenzen(Zielspiel::JkaSp).knochenJeFlaeche),
           z > 500 && spiel && gewichteMax <= 4 && knochen <= grenzen(Zielspiel::JkaSp).knochenJeFlaeche);
    expect(f("... die %d beschriebenen Punkte tragen an allen ihren Ecken dasselbe (Naehte zwischen Flaechen zu)", zielPunkte), naht && zielPunkte > 300);
    std::string geschrieben;
    Inhalt gelesen;
    expect("... die .glm laesst sich schreiben und liest sich wie das Dokument", glmSchreiben(x, geschrieben) && ausGlm(geschrieben, gelesen) && wieImSpiel(gelesen));
}

}  // namespace

int main(int argc, char** argv) {
    pruefeRestNull(argc > 1 ? argv[1] : "");
    pruefeSchweissnetz();
    pruefeRegeln();
    pruefePinsel();
    pruefeWerkzeug();
    pruefeSpiegeln();
    pruefeGrenzen();
    pruefeRaven(argc > 2 ? argv[2] : "");
    std::printf("%s (%d Fehler)\n", fails == 0 ? "modell_gewichte2: alles ok" : "modell_gewichte2: FEHLER", fails);
    return fails == 0 ? 0 : 1;
}
