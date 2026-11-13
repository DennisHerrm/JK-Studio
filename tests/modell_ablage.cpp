// modell_ablage.cpp - AP11: Flaechen von einem Modell in ein anderes kopieren (bhed/modell/ablage.h), ohne Oberflaeche
//
//   Ablage      zwei Teile (Eltern und Kind) aus data/fixtures/model.glm: die Beziehung bleibt, was nur zur Datei der Quelle gehoert
//               (Rohfelder, Ecknummern, Knochenverweise der Datei), faellt weg
//   Gewichte    gleiche Knochen (Einheit), andere Reihenfolge im Ziel (jede Ecke nennt dieselben NAMEN mit denselben Gewichten),
//               fehlende Knochen (Gewicht an den naechsten vorhandenen Eltern, Summe je Ecke bleibt, zwei auf einen werden eins,
//               der Bericht nennt jeden), Ziel ohne Skelett (.glm: Ablehnung ohne Aenderung; .md3: starr)
//   Stufen      weniger Stufen im Ziel (ueberzaehlige fallen weg), mehr (mit der groebsten gefuellt)
//   Namen       vorhandene bekommen _1, _2 ...; hoechstens 63 Zeichen
//   Stapel      Modifier bekommen neue Kennungen im Ziel
//   Grenzen     enge Grenzen melden die neuen Flaechen je Stufe, die des Spiels nicht (Flaechen der Datei liegen darunter)
//   Datei       das Ziel mit eingefuegten Flaechen schreibt sich als .glm und liest sich wieder: Eltern, Kinder, Gewichte nach Namen
//
// Aufruf: modellablagetest <model.glm>
#include "bhed/modell/ablage.h"
#include "bhed/modell/dokument.h"
#include "bhed/modell/glmdatei.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace {
int fails = 0;
void expect(const std::string& what, bool ok) {
    std::printf("%s %s\n", ok ? "OK    " : "FEHLER", what.c_str());
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

// Ein Skelett mit so vielen Knochen, wie die Ecken nennen: b0 Wurzel, jeder weitere am vorigen (eine Kette).
int knochenZahl(const Inhalt& in) {
    int n = 1;
    for (const Teil& t : in.teile) {
        for (const Netz& nz : t.stufen) {
            for (const Ecke& e : nz.ecken) {
                for (int k = 0; k < 4; ++k) {
                    if (e.gewicht[k] > 0.0F) { n = std::max(n, e.knochen[k] + 1); }
                }
            }
        }
    }
    return n;
}

// Die Gewichte einer Ecke nach Knochennamen (Summe je Name).
std::map<std::string, float> nachNamen(const Ecke& e, const std::vector<std::string>& namen) {
    std::map<std::string, float> m;
    for (int k = 0; k < 4; ++k) {
        if (e.gewicht[k] > 0.0F && e.knochen[k] < namen.size()) { m[namen[e.knochen[k]]] += e.gewicht[k]; }
    }
    return m;
}

bool gleich(const std::map<std::string, float>& a, const std::map<std::string, float>& b) {
    if (a.size() != b.size()) { return false; }
    for (const auto& [k, v] : a) {
        const auto it = b.find(k);
        if (it == b.end() || std::fabs(it->second - v) > 1e-5F) { return false; }
    }
    return true;
}

float summe(const Ecke& e) {
    float s = 0.0F;
    for (int k = 0; k < 4; ++k) { s += e.gewicht[k] > 0.0F ? e.gewicht[k] : 0.0F; }
    return s;
}

// Index des ersten Teils mit Kind (kein Tag), das Kind dazu
bool elternKind(const Inhalt& in, int& eltern, int& kind) {
    for (std::size_t i = 0; i < in.teile.size(); ++i) {
        const Teil& t = in.teile[i];
        if (t.istTag()) { continue; }
        for (const int k : t.kinder) {
            if (k >= 0 && static_cast<std::size_t>(k) < in.teile.size() && !in.teile[static_cast<std::size_t>(k)].istTag() &&
                !in.teile[static_cast<std::size_t>(k)].stufen.empty() && !in.teile[static_cast<std::size_t>(k)].stufen[0].ecken.empty()) {
                eltern = static_cast<int>(i);
                kind = k;
                return true;
            }
        }
    }
    return false;
}

void tests(const std::string& pfad) {
    Inhalt in;
    std::string fehler;
    const std::string bytes = slurp(pfad);
    expect("model.glm gelesen", !bytes.empty() && ausGlm(bytes, in, &fehler));
    if (in.teile.empty()) { return; }
    int eltern = -1;
    int kind = -1;
    expect("ein Teil mit Kind gefunden", elternKind(in, eltern, kind));
    if (eltern < 0) { return; }
    const int nk = knochenZahl(in);
    std::vector<std::string> namen;
    std::vector<int> kette;
    for (int i = 0; i < nk; ++i) {
        namen.push_back("b" + std::to_string(i));
        kette.push_back(i - 1);
    }
    std::vector<std::string> bilder(in.teile.size());
    bilder[static_cast<std::size_t>(eltern)] = "models/players/quelle/eltern";
    bilder[static_cast<std::size_t>(kind)] = "models/players/quelle/kind";

    // --- Ablage ---
    in.teile[static_cast<std::size_t>(kind)].stapel.push_back(Modifikator{ModArt::Spiegeln, 1});
    const FlaechenAblage a = ablageAus(in, {kind, eltern, kind, 9999, -3}, bilder, namen, kette, "quelle/model.glm");
    expect("Ablage: zwei Teile in der Reihenfolge des Modells (doppelte, ungueltige fallen weg)", a.teile.size() == 2U &&
           a.teile[0].name == in.teile[static_cast<std::size_t>(std::min(eltern, kind))].name);
    const int ak = eltern < kind ? 1 : 0;   // Index des Kinds in der Ablage
    expect("Ablage: das Kind bleibt unter seinem Eltern, der Eltern ist oberste Flaeche der Ablage",
           a.teile[static_cast<std::size_t>(ak)].eltern == 1 - ak && a.teile[static_cast<std::size_t>(1 - ak)].eltern == -1);
    bool dateiWeg = true;
    for (const Teil& t : a.teile) {
        dateiWeg = dateiWeg && t.dateiName.empty() && t.dateiShader.empty() && t.dateiKinder.empty() && t.dateiFlaeche == -1;
        for (const Netz& n : t.stufen) {
            dateiWeg = dateiWeg && n.dateiRefs.empty() && n.dateiRefsAufgeloest.empty() && !n.ausDatei;
            for (const Ecke& e : n.ecken) { dateiWeg = dateiWeg && !e.ausDatei && e.dateiPack == 0U && e.dateiIndex == -1; }
        }
    }
    expect("Ablage: Rohfelder, Ecknummern und Knochenverweise der Quelldatei fallen weg", dateiWeg);
    expect("Ablage: Bilder je Teil, Skelett, Quelle, Stufen", a.bilder.size() == 2U && a.knochen.size() == namen.size() && a.quelle == "quelle/model.glm" &&
           a.stufen == in.stufenZahl);
    in.teile[static_cast<std::size_t>(kind)].stapel.clear();
    const Grenzen& spiel = grenzen(Zielspiel::JkaSp);

    // --- gleiche Knochen ---
    {
        Inhalt z = in;
        z.teile[0].stapel.push_back(Modifikator{ModArt::Glaetten, 5});
        EinfuegeBericht b;
        const std::size_t vorher = z.teile.size();
        const EinfuegeFehler f = einfuegen(z, a, 0, namen, spiel, b);
        expect("gleiches Skelett: eingefuegt", f == EinfuegeFehler::Keiner && z.teile.size() == vorher + 2U && b.neu.size() == 2U);
        if (f == EinfuegeFehler::Keiner && b.neu.size() == 2U) {
            const Teil& nk2 = z.teile[static_cast<std::size_t>(b.neu[static_cast<std::size_t>(ak)])];
            const Teil& ne = z.teile[static_cast<std::size_t>(b.neu[static_cast<std::size_t>(1 - ak)])];
            expect("gleiches Skelett: der Eltern haengt am Ziel 0, das Kind am eingefuegten Eltern; Kinderlisten nachgefuehrt",
                   ne.eltern == 0 && nk2.eltern == b.neu[static_cast<std::size_t>(1 - ak)] &&
                       std::count(z.teile[0].kinder.begin(), z.teile[0].kinder.end(), b.neu[static_cast<std::size_t>(1 - ak)]) == 1 &&
                       std::count(ne.kinder.begin(), ne.kinder.end(), b.neu[static_cast<std::size_t>(ak)]) == 1);
            expect("gleiches Skelett: vorhandene Namen bekommen _1 (Bericht nennt beide)", nk2.name == in.teile[static_cast<std::size_t>(kind)].name + "_1" &&
                                                                                                b.umbenannt.size() == 2U);
            expect("gleiches Skelett: Shader = Bild der Quelle, als Bild der Haut gemerkt",
                   nk2.shader == "models/players/quelle/kind" && nk2.hautBild == nk2.shader && ne.shader == "models/players/quelle/eltern");
            bool gw = nk2.stufen.size() == static_cast<std::size_t>(z.stufenZahl);
            const Teil& q = in.teile[static_cast<std::size_t>(kind)];
            for (std::size_t s = 0; gw && s < nk2.stufen.size() && s < q.stufen.size(); ++s) {
                gw = nk2.stufen[s].ecken.size() == q.stufen[s].ecken.size() && nk2.stufen[s].kanten.size() == q.stufen[s].kanten.size();
                for (std::size_t e = 0; gw && e < q.stufen[s].ecken.size(); ++e) {
                    gw = gleich(nachNamen(nk2.stufen[s].ecken[e], namen), nachNamen(q.stufen[s].ecken[e], namen)) &&
                         nk2.stufen[s].ecken[e].ort[0] == q.stufen[s].ecken[e].ort[0] && nk2.stufen[s].ecken[e].uv[1] == q.stufen[s].ecken[e].uv[1];
                }
            }
            expect("gleiches Skelett: alle Stufen, Ecken, Dreiecke, Orte und Gewichte wie in der Quelle", gw);
            expect("gleiches Skelett: keine Ersatzknochen, keine Stufe weg, keine gefuellt", b.ersatz.empty() && b.stufenWeg == 0 && b.stufenAufgefuellt == 0);
            expect("Grenzen des Spiels: keine Befunde fuer die Flaechen der Datei", b.befunde.empty());
            std::string glm;
            GlmSchreibBericht sb;
            Inhalt zurueck;
            const bool ok = glmSchreiben(z, glm, &sb) && ausGlm(glm, zurueck, &fehler);
            expect("Datei: das Ziel mit den eingefuegten Flaechen schreibt sich als .glm und liest sich wieder (" + fehler + ")", ok);
            if (ok) {
                const int ik = zurueck.teilIndex(nk2.name);
                const int ie = zurueck.teilIndex(ne.name);
                bool rund = ik >= 0 && ie >= 0 && zurueck.teile[static_cast<std::size_t>(ik)].eltern == ie && zurueck.teile[static_cast<std::size_t>(ie)].eltern == 0;
                if (rund) {
                    const Netz& n0 = zurueck.teile[static_cast<std::size_t>(ik)].stufen[0];
                    const Netz& q0 = q.stufen[0];
                    rund = n0.ecken.size() == q0.ecken.size() && zurueck.teile[static_cast<std::size_t>(ik)].shader == "models/players/quelle/kind";
                    for (std::size_t e = 0; rund && e < q0.ecken.size(); ++e) {
                        const auto x = nachNamen(n0.ecken[e], namen);
                        const auto y = nachNamen(q0.ecken[e], namen);
                        rund = x.size() == y.size();
                        for (const auto& [k, v] : y) { rund = rund && x.count(k) == 1U && std::fabs(x.at(k) - v) < 0.01F; }   // die Datei packt Gewichte in 8 Bit
                    }
                }
                expect("Datei: Eltern, Shader und Gewichte (nach Namen, 8 Bit der Datei) kommen zurueck", rund);
            }
        }
    }
    // --- andere Reihenfolge im Ziel ---
    {
        std::vector<std::string> rueck(namen.rbegin(), namen.rend());
        Inhalt z = in;
        EinfuegeBericht b;
        const EinfuegeFehler f = einfuegen(z, a, -1, rueck, spiel, b);
        bool gw = f == EinfuegeFehler::Keiner && b.neu.size() == 2U;
        const Teil& q = in.teile[static_cast<std::size_t>(kind)];
        if (gw) {
            const Teil& n = z.teile[static_cast<std::size_t>(b.neu[static_cast<std::size_t>(ak)])];
            for (std::size_t e = 0; gw && e < q.stufen[0].ecken.size(); ++e) { gw = gleich(nachNamen(n.stufen[0].ecken[e], rueck), nachNamen(q.stufen[0].ecken[e], namen)); }
            gw = gw && z.teile[static_cast<std::size_t>(b.neu[static_cast<std::size_t>(1 - ak)])].eltern == -1;
        }
        expect("andere Reihenfolge der Knochen im Ziel: jede Ecke nennt dieselben Namen mit denselben Gewichten; oberste Ebene", gw && b.ersatz.empty());
    }
    // --- fehlende Knochen ---
    {
        // der Knochen mit dem meisten Gewicht im Kind fehlt im Ziel (sein Eltern in der Kette ist da)
        std::map<int, float> last;
        for (const Ecke& e : in.teile[static_cast<std::size_t>(kind)].stufen[0].ecken) {
            for (int k = 0; k < 4; ++k) {
                if (e.gewicht[k] > 0.0F) { last[e.knochen[k]] += e.gewicht[k]; }
            }
        }
        int weg = -1;
        float best = -1.0F;
        for (const auto& [kn, w] : last) {
            if (kn > 0 && w > best) {
                weg = kn;
                best = w;
            }
        }
        expect("fehlende Knochen: ein Knochen > 0 mit Gewicht im Kind", weg > 0);
        if (weg > 0) {
            std::vector<std::string> ohne;
            for (int i = 0; i < nk; ++i) {
                if (i != weg) { ohne.push_back(namen[static_cast<std::size_t>(i)]); }
            }
            Inhalt z = in;
            EinfuegeBericht b;
            const EinfuegeFehler f = einfuegen(z, a, 0, ohne, spiel, b);
            bool ok = f == EinfuegeFehler::Keiner && b.neu.size() == 2U;
            bool summen = ok;
            bool keinWeg = ok;
            bool vereint = ok;
            if (ok) {
                const Teil& n = z.teile[static_cast<std::size_t>(b.neu[static_cast<std::size_t>(ak)])];
                const Teil& q = in.teile[static_cast<std::size_t>(kind)];
                for (std::size_t s = 0; s < n.stufen.size() && s < q.stufen.size(); ++s) {
                    for (std::size_t e = 0; e < q.stufen[s].ecken.size(); ++e) {
                        const Ecke& x = n.stufen[s].ecken[e];
                        summen = summen && std::fabs(summe(x) - summe(q.stufen[s].ecken[e])) < 1e-5F;
                        const auto m = nachNamen(x, ohne);
                        keinWeg = keinWeg && m.count(namen[static_cast<std::size_t>(weg)]) == 0U;
                        // erwartet: das Gewicht des fehlenden liegt beim Eltern b(weg-1), mit dessen eigenem Gewicht vereint
                        std::map<std::string, float> soll;
                        for (int k = 0; k < 4; ++k) {
                            const Ecke& y = q.stufen[s].ecken[e];
                            if (y.gewicht[k] <= 0.0F) { continue; }
                            const int kn = y.knochen[k] == weg ? weg - 1 : y.knochen[k];
                            soll[namen[static_cast<std::size_t>(kn)]] += y.gewicht[k];
                        }
                        vereint = vereint && gleich(m, soll);
                    }
                }
            }
            expect("fehlende Knochen: eingefuegt, Summe der Gewichte je Ecke bleibt", ok && summen);
            expect("fehlende Knochen: kein Gewicht mehr am fehlenden; es liegt beim naechsten Eltern im Ziel, zwei auf einen sind ein Gewicht", keinWeg && vereint);
            expect("fehlende Knochen: der Bericht nennt den fehlenden und seinen Ersatz", b.ersatz.size() == 1U && b.ersatz[0].first == namen[static_cast<std::size_t>(weg)] &&
                                                                                            b.ersatz[0].second == namen[static_cast<std::size_t>(weg - 1)]);
        }
    }
    // --- Ziel ohne Skelett ---
    {
        Inhalt z = in;
        EinfuegeBericht b;
        const std::size_t vorher = z.teile.size();
        expect(".glm ohne geladenes Skelett: abgelehnt, nichts geaendert", einfuegen(z, a, 0, {}, spiel, b) == EinfuegeFehler::ZielOhneSkelett && z.teile.size() == vorher);
        z.herkunft = Herkunft::Md3;
        z.stufenZahl = 1;
        const EinfuegeFehler f = einfuegen(z, a, -1, {}, spiel, b);
        bool starr = f == EinfuegeFehler::Keiner && b.zielOhneSkelett && b.neu.size() == 2U;
        for (const int i : b.neu) {
            const Teil& t = z.teile[static_cast<std::size_t>(i)];
            starr = starr && t.stufen.size() == 1U;
            for (const Ecke& e : t.stufen[0].ecken) { starr = starr && e.knochen[0] == 0 && e.gewicht[0] == 1.0F && e.gewicht[1] == 0.0F; }
        }
        expect(".md3 als Ziel: eine Stufe, jede Ecke starr an Knochen 0 (Bericht sagt es)", starr);
        expect("weniger Stufen im Ziel: der Bericht nennt die weggefallenen", b.stufenWeg == in.stufenZahl - 1);
    }
    // --- mehr Stufen im Ziel ---
    {
        Inhalt q1 = in;
        q1.stufenZahl = 1;
        for (Teil& t : q1.teile) { t.stufen.resize(1); }
        const FlaechenAblage a1 = ablageAus(q1, {kind}, bilder, namen, kette, "q1");
        Inhalt z = in;
        EinfuegeBericht b;
        const EinfuegeFehler f = einfuegen(z, a1, 0, namen, spiel, b);
        bool ok = f == EinfuegeFehler::Keiner && b.neu.size() == 1U;
        if (ok) {
            const Teil& t = z.teile[static_cast<std::size_t>(b.neu[0])];
            ok = t.stufen.size() == static_cast<std::size_t>(std::max(z.stufenZahl, 1));
            for (const Netz& n : t.stufen) { ok = ok && n.ecken.size() == q1.teile[static_cast<std::size_t>(kind)].stufen[0].ecken.size(); }
        }
        expect("mehr Stufen im Ziel: mit der groebsten der Quelle gefuellt, der Bericht nennt die Zahl", ok && b.stufenAufgefuellt == std::max(z.stufenZahl, 1) - 1);
    }
    // --- Namen, Stapel, Grenzen ---
    {
        Inhalt q = in;
        q.teile[static_cast<std::size_t>(kind)].name = std::string(70, 'x');
        q.teile[static_cast<std::size_t>(kind)].stapel = {Modifikator{ModArt::Spiegeln, 1}, Modifikator{ModArt::Glaetten, 2}};
        const FlaechenAblage a2 = ablageAus(q, {kind}, {}, namen, kette, "q");
        Inhalt z = in;
        z.teile[0].stapel.push_back(Modifikator{ModArt::Glaetten, 7});
        EinfuegeBericht b;
        (void)einfuegen(z, a2, 0, namen, spiel, b);
        EinfuegeBericht b2;
        (void)einfuegen(z, a2, 0, namen, spiel, b2);
        const bool zwei = b.neu.size() == 1U && b2.neu.size() == 1U;
        expect("Namen: hoechstens 63 Zeichen, der zweite gleiche bekommt _1", zwei && z.teile[static_cast<std::size_t>(b.neu[0])].name == std::string(63, 'x') &&
                                                                                   z.teile[static_cast<std::size_t>(b2.neu[0])].name == std::string(61, 'x') + "_1");
        expect("Shader ohne Bild der Quelle: der der Datei, kein Bild der Haut", zwei && z.teile[static_cast<std::size_t>(b.neu[0])].shader == in.teile[static_cast<std::size_t>(kind)].shader &&
                                                                                      z.teile[static_cast<std::size_t>(b.neu[0])].hautBild == z.teile[static_cast<std::size_t>(b.neu[0])].shader);
        expect("Stapel: neue Kennungen ueber denen des Ziels (8, 9, dann 10, 11)",
               zwei && z.teile[static_cast<std::size_t>(b.neu[0])].stapel.size() == 2U && z.teile[static_cast<std::size_t>(b.neu[0])].stapel[0].kennung == 8U &&
                   z.teile[static_cast<std::size_t>(b.neu[0])].stapel[1].kennung == 9U && z.teile[static_cast<std::size_t>(b2.neu[0])].stapel[0].kennung == 10U);
        Grenzen eng = spiel;
        eng.eckenJeFlaeche = 3;
        eng.knochenJeFlaeche = 1;
        Inhalt z2 = in;
        EinfuegeBericht b3;
        (void)einfuegen(z2, a, 0, namen, eng, b3);
        bool ecken = false;
        bool knochen = false;
        bool jeStufe = true;
        for (const Befund& x : b3.befunde) {
            ecken = ecken || x.art == BefundArt::Ecken;
            knochen = knochen || x.art == BefundArt::Knochen;
            jeStufe = jeStufe && x.flaeche >= static_cast<int>(in.teile.size()) && x.stufe >= 0 && x.stufe < std::max(in.stufenZahl, 1) && !x.name.empty();
        }
        expect("Grenzen: enge Grenzen melden Ecken und Knochen der neuen Flaechen je Stufe (mit Name)", ecken && knochen && jeStufe && !b3.befunde.empty());
    }
    // --- Fehler ---
    {
        Inhalt leer;
        EinfuegeBericht b;
        expect("leeres Ziel: abgelehnt", einfuegen(leer, a, -1, namen, spiel, b) == EinfuegeFehler::ZielLeer);
        Inhalt z = in;
        expect("leere Ablage: abgelehnt", einfuegen(z, FlaechenAblage{}, 0, namen, spiel, b) == EinfuegeFehler::AblageLeer);
        expect("Eltern ausserhalb: abgelehnt, nichts geaendert", einfuegen(z, a, 99999, namen, spiel, b) == EinfuegeFehler::ElternUngueltig && z.teile.size() == in.teile.size());
    }
}

}  // namespace

int main(int argc, char** argv) {
    if (argc > 1) {
        tests(argv[1]);
    } else {
        expect("Aufruf: modellablagetest <model.glm>", false);
    }
    std::printf("%s\n", fails == 0 ? "ALLES OK" : "FEHLGESCHLAGEN");
    return fails == 0 ? 0 : 1;
}
