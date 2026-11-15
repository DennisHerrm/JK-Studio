// modell_detailstufen.cpp - CW-MODELL (15.11.2026): Detailstufen erzeugen (bhed/modell/detailstufen.h)
//
//   Gleichheit   zweimal dieselbe Eingabe -> dieselben Stufen und dieselbe .glm Byte fuer Byte
//   Grenzen      SHADER_MAX_VERTEXES je Flaeche in jeder Stufe, in der letzten Stufe einer .glm die Schattengrenze / 2
//   unberuehrt   Tags, "_off", "_cap_" in jeder Stufe gleich Stufe 0
//   Naehte       eine UV-Naht (zwei Inseln an denselben Orten) bleibt: kein Dreieck mischt die Inseln, beide Seiten treffen
//                sich an denselben Punkten
//   Gewichte     jede Ecke einer Stufe ist eine Ecke von Stufe 0 (Ort, Normale, UV, Knochen, Gewichte); hoechstens 4, Summe 1
//   Raender      die Randkanten jeder Flaeche bleiben dieselben
//   .md3         name_1.md3 traegt alle Bilder und Tags der Datei, jede Ecke in jedem Bild an ihrem Ort der Datei
//   Vorgaben     Arten, Anteile, Richtwert nur mit Wert
//
// Aufruf: modelldetailstufentest <model.glm>
#include "bhed/glm.h"
#include "bhed/md3.h"
#include "bhed/modell/detailstufen.h"
#include "bhed/modell/dokument.h"
#include "bhed/modell/g2.h"
#include "bhed/modell/glmdatei.h"
#include "bhed/modell/md3datei.h"
#include "bhed/modell/stapel.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <utility>
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

bool eckeGleich(const Ecke& a, const Ecke& b) {
    return std::memcmp(a.ort, b.ort, sizeof a.ort) == 0 && std::memcmp(a.normale, b.normale, sizeof a.normale) == 0 &&
           std::memcmp(a.uv, b.uv, sizeof a.uv) == 0 && std::memcmp(a.knochen, b.knochen, sizeof a.knochen) == 0 &&
           std::memcmp(a.gewicht, b.gewicht, sizeof a.gewicht) == 0;
}

using Ort = std::array<float, 3>;
Ort ortVon(const Ecke& e) { return {e.ort[0], e.ort[1], e.ort[2]}; }

// Randkanten als Paare von Orten (Halbkante ohne Gegenkante)
std::multiset<std::pair<Ort, Ort>> randKanten(const Netz& n) {
    std::multiset<std::pair<Ort, Ort>> r;
    for (const Halbkante& k : n.kanten) {
        if (k.gegen >= 0) { continue; }
        Ort a = ortVon(n.ecken[static_cast<std::size_t>(k.ecke)]);
        Ort b = ortVon(n.ecken[static_cast<std::size_t>(n.kanten[static_cast<std::size_t>(k.naechste)].ecke)]);
        if (b < a) { std::swap(a, b); }
        r.emplace(a, b);
    }
    return r;
}

// Ein Gitter (n x n Ecken) mit einer Naht in der Mitte: die Spalte n/2 gibt es zweimal (links UV-Insel A bei u < 0,45, rechts
// Insel B bei u > 0,55) - wie die Datei Ecken an einer Naht teilt. Leichte Welle in z, damit der Vereinfacher etwas zu tun hat.
Netz nahtGitter(int n) {
    std::vector<Ecke> e;
    std::vector<int> nummer(static_cast<std::size_t>(n * n * 2), -1);
    const int mitte = n / 2;
    const auto ecke = [&](int x, int y, int insel) {
        const std::size_t k = static_cast<std::size_t>((y * n + x) * 2 + insel);
        if (nummer[k] >= 0) { return nummer[k]; }
        Ecke v;
        v.ort[0] = static_cast<float>(x) * 4.0F;
        v.ort[1] = static_cast<float>(y) * 4.0F;
        v.ort[2] = std::sin(static_cast<float>(x) * 0.4F) * std::cos(static_cast<float>(y) * 0.3F) * 3.0F;
        v.normale[2] = 1.0F;
        const float u = static_cast<float>(x) / static_cast<float>(n - 1);
        v.uv[0] = insel == 0 ? u * 0.45F / 0.5F : 0.55F + (u - 0.5F) * 0.45F / 0.5F;
        v.uv[1] = static_cast<float>(y) / static_cast<float>(n - 1);
        v.knochen[0] = static_cast<std::uint8_t>(x < mitte ? 1 : 2);
        v.gewicht[0] = 1.0F;
        nummer[k] = static_cast<int>(e.size());
        e.push_back(v);
        return nummer[k];
    };
    std::vector<std::uint32_t> idx;
    for (int y = 0; y + 1 < n; ++y) {
        for (int x = 0; x + 1 < n; ++x) {
            const int insel = x < mitte ? 0 : 1;
            const auto a = static_cast<std::uint32_t>(ecke(x, y, insel));
            const auto b = static_cast<std::uint32_t>(ecke(x + 1, y, insel));
            const auto c = static_cast<std::uint32_t>(ecke(x + 1, y + 1, insel));
            const auto d = static_cast<std::uint32_t>(ecke(x, y + 1, insel));
            idx.insert(idx.end(), {a, b, c, a, c, d});
        }
    }
    Netz out;
    out.aufbauen(std::move(e), idx);
    return out;
}

// --- eine .md3 mit Bildern und einem Tag (qfiles.h) ----------------------------------------------------------------------------
void i32(std::string& b, std::int32_t v) { b.append(reinterpret_cast<const char*>(&v), 4); }
void f32(std::string& b, float v) { b.append(reinterpret_cast<const char*>(&v), 4); }
void i16(std::string& b, std::int16_t v) { b.append(reinterpret_cast<const char*>(&v), 2); }
void name(std::string& b, const std::string& n, std::size_t l) {
    std::string x = n;
    x.resize(l, '\0');
    b += x;
}
void setze(std::string& b, std::size_t at, std::int32_t v) { std::memcpy(b.data() + at, &v, 4); }

std::string gitterMd3(int n, int bilder) {
    std::string b;
    b += "IDP3";
    i32(b, 15);
    name(b, "models/map_objects/cw/gitter.md3", 64);
    i32(b, 0);
    i32(b, bilder);
    i32(b, 1);
    i32(b, 1);
    i32(b, 0);
    const std::size_t kopf = b.size();
    for (int k = 0; k < 4; ++k) { i32(b, 0); }
    setze(b, kopf, static_cast<std::int32_t>(b.size()));
    for (int f = 0; f < bilder; ++f) {
        for (int k = 0; k < 6; ++k) { f32(b, k < 3 ? -1.0F : 130.0F); }
        for (int k = 0; k < 3; ++k) { f32(b, 0.0F); }
        f32(b, 200.0F);
        name(b, "f" + std::to_string(f), 16);
    }
    setze(b, kopf + 4, static_cast<std::int32_t>(b.size()));
    for (int f = 0; f < bilder; ++f) {
        name(b, "tag_flash", 64);
        f32(b, 10.0F); f32(b, 20.0F); f32(b, 30.0F + static_cast<float>(f));
        f32(b, 1); f32(b, 0); f32(b, 0); f32(b, 0); f32(b, 1); f32(b, 0); f32(b, 0); f32(b, 0); f32(b, 1);
    }
    setze(b, kopf + 8, static_cast<std::int32_t>(b.size()));
    const std::size_t fl = b.size();
    b += "IDP3";
    name(b, "gitter", 64);
    i32(b, 0);
    i32(b, bilder);
    i32(b, 1);
    i32(b, n * n);
    i32(b, (n - 1) * (n - 1) * 2);
    const std::size_t ofs = b.size();
    for (int k = 0; k < 5; ++k) { i32(b, 0); }
    setze(b, ofs + 4, static_cast<std::int32_t>(b.size() - fl));
    name(b, "models/map_objects/cw/gitter", 64);
    i32(b, 0);
    setze(b, ofs, static_cast<std::int32_t>(b.size() - fl));
    for (int y = 0; y + 1 < n; ++y) {
        for (int x = 0; x + 1 < n; ++x) {
            const int a = y * n + x;
            i32(b, a); i32(b, a + 1); i32(b, a + n + 1);
            i32(b, a); i32(b, a + n + 1); i32(b, a + n);
        }
    }
    setze(b, ofs + 8, static_cast<std::int32_t>(b.size() - fl));
    for (int y = 0; y < n; ++y) {
        for (int x = 0; x < n; ++x) {
            f32(b, static_cast<float>(x) / static_cast<float>(n - 1));
            f32(b, static_cast<float>(y) / static_cast<float>(n - 1));
        }
    }
    setze(b, ofs + 12, static_cast<std::int32_t>(b.size() - fl));
    for (int f = 0; f < bilder; ++f) {
        for (int y = 0; y < n; ++y) {
            for (int x = 0; x < n; ++x) {
                const float z = std::sin(static_cast<float>(x) * 0.5F + static_cast<float>(f)) * 6.0F;
                i16(b, static_cast<std::int16_t>(x * 8 * 64));
                i16(b, static_cast<std::int16_t>(y * 8 * 64));
                i16(b, static_cast<std::int16_t>(std::lround(z * 64.0F)));
                i16(b, 0);
            }
        }
    }
    setze(b, ofs + 16, static_cast<std::int32_t>(b.size() - fl));
    setze(b, kopf + 12, static_cast<std::int32_t>(b.size()));
    return b;
}

void glmTeil(const std::string& pfad) {
    std::printf("GLM %s\n", pfad.c_str());
    const std::string bytes = slurp(pfad);
    Inhalt in;
    expect("model.glm gelesen", ausGlm(bytes, in));
    if (in.teile.empty()) { return; }
    const Grenzen& g = grenzen(Zielspiel::JkaSp);
    DetailOptionen o;
    o.stufen = 3;
    o.eckenGrenze = g.eckenJeFlaeche;
    o.schattenGrenze = schattenEckenGrenze(Zielspiel::JkaSp);
    Inhalt a = in;
    Inhalt b = in;
    DetailBericht ba;
    DetailBericht bb;
    expect("Stufen erzeugt", detailstufenErzeugen(a, o, &ba) && detailstufenErzeugen(b, o, &bb));
    std::string ga;
    std::string gb;
    const bool geschrieben = glmSchreiben(a, ga) && glmSchreiben(b, gb);
    expect("gleiche Eingabe, gleiche .glm Byte fuer Byte (" + std::to_string(ga.size()) + " Bytes)", geschrieben && ga == gb && !ga.empty());
    // noch einmal aus der gelesenen Datei
    Inhalt c;
    (void)ausGlm(bytes, c);
    (void)detailstufenErzeugen(c, o);
    std::string gc;
    expect("neu gelesen und erzeugt: dieselben Bytes", glmSchreiben(c, gc) && gc == ga);
    expect("drei Stufen im Dokument", a.stufenZahl == 3 && std::all_of(a.teile.begin(), a.teile.end(), [](const Teil& t) { return t.stufen.size() == 3U; }));
    std::printf("    Ecken je Stufe vorher %d/%d/%d/%d, nachher %d/%d/%d, Abweichung %.3f, unberuehrt %d, verfehlt %zu\n", ba.vorher[0].ecken,
                ba.vorher.size() > 1 ? ba.vorher[1].ecken : 0, ba.vorher.size() > 2 ? ba.vorher[2].ecken : 0, ba.vorher.size() > 3 ? ba.vorher[3].ecken : 0,
                ba.nachher[0].ecken, ba.nachher[1].ecken, ba.nachher[2].ecken, static_cast<double>(ba.abweichung), ba.unberuehrt,
                ba.verfehlt.size());
    expect("Stufe 0 unveraendert", [&] {
        for (std::size_t i = 0; i < a.teile.size(); ++i) {
            if (!netzGleich(a.teile[i].stufen[0], in.teile[i].stufen[0])) { return false; }
        }
        return true;
    }());
    int fest = 0;
    bool festGleich = true;
    bool teilmenge = true;
    bool gewichteOk = true;
    bool grenzeOk = true;
    bool randOk = true;
    bool geschlossen = true;
    int nahtKanten = 0;
    // Orte, die in Stufe 0 an mehr als einer Flaeche liegen (Raender zwischen Flaechen; naeher als 0,01 zaehlt mit - Rundungsreste)
    const auto nah = [](const Ort& x, const Ort& y) {
        return std::abs(x[0] - y[0]) <= 0.01F && std::abs(x[1] - y[1]) <= 0.01F && std::abs(x[2] - y[2]) <= 0.01F;
    };
    std::set<Ort> geteilt;
    {
        std::vector<std::pair<Ort, std::size_t>> alle;
        for (std::size_t i = 0; i < a.teile.size(); ++i) {
            for (const Ecke& e : a.teile[i].stufen[0].ecken) { alle.emplace_back(ortVon(e), i); }
        }
        for (const auto& [x, i] : alle) {
            for (const auto& [y, j] : alle) {
                if (i != j && nah(x, y)) {
                    geteilt.insert(x);
                    break;
                }
            }
        }
    }
    int vereinfacht = 0;
    for (std::size_t i = 0; i < a.teile.size(); ++i) {
        const Teil& t = a.teile[i];
        const StufenRolle r = stufenRolle(t);
        if (r == StufenRolle::Tag || r == StufenRolle::Aus || r == StufenRolle::Kappe) {
            ++fest;
            for (std::size_t s = 1; s < t.stufen.size(); ++s) { festGleich = festGleich && netzGleich(t.stufen[s], t.stufen[0]); }
            continue;
        }
        if (r != StufenRolle::Vereinfachen) { continue; }
        ++vereinfacht;
        // offene Raender (an keiner anderen Flaeche): bleiben, wie sie sind
        std::multiset<std::pair<Ort, Ort>> frei0;
        for (const auto& k : randKanten(t.stufen[0])) {
            if (geteilt.count(k.first) == 0U && geteilt.count(k.second) == 0U) { frei0.insert(k); }
        }
        for (std::size_t s = 1; s < t.stufen.size(); ++s) {
            const Netz& n = t.stufen[s];
            for (const Ecke& e : n.ecken) {
                bool da = false;
                for (const Ecke& q : t.stufen[0].ecken) {
                    if (eckeGleich(e, q)) {
                        da = true;
                        break;
                    }
                }
                teilmenge = teilmenge && da;
                int zahl = 0;
                float summe = 0.0F;
                for (int k = 0; k < 4; ++k) {
                    if (e.gewicht[k] > 0.0F) {
                        ++zahl;
                        summe += e.gewicht[k];
                    }
                }
                gewichteOk = gewichteOk && zahl >= 1 && zahl <= 4 && std::abs(summe - 1.0F) < 1e-3F;
            }
            const int ecken = static_cast<int>(n.ecken.size());
            const bool letzte = s + 1 == t.stufen.size();
            const bool verfehlt = std::any_of(ba.verfehlt.begin(), ba.verfehlt.end(), [&](const StufenRest& x) { return x.teil == static_cast<int>(i) && x.stufe == static_cast<int>(s); });
            if (!verfehlt) { grenzeOk = grenzeOk && ecken < o.eckenGrenze && (!letzte || ecken < o.schattenGrenze); }
            std::multiset<std::pair<Ort, Ort>> frei;
            for (const auto& k : randKanten(n)) {
                if (geteilt.count(k.first) == 0U && geteilt.count(k.second) == 0U) { frei.insert(k); }
                // eine Randkante zwischen zwei Flaechen gibt es in derselben Stufe auch an einer anderen Flaeche (kein Riss)
                if (geteilt.count(k.first) != 0U && geteilt.count(k.second) != 0U) {
                    bool gegen = false;
                    for (std::size_t j = 0; j < a.teile.size() && !gegen; ++j) {
                        if (j == i || s >= a.teile[j].stufen.size()) { continue; }
                        for (const auto& gk : randKanten(a.teile[j].stufen[s])) {
                            if ((nah(gk.first, k.first) && nah(gk.second, k.second)) || (nah(gk.first, k.second) && nah(gk.second, k.first))) {
                                gegen = true;
                                break;
                            }
                        }
                    }
                    geschlossen = geschlossen && gegen;
                    ++nahtKanten;
                }
            }
            randOk = randOk && frei == frei0;
        }
    }
    std::printf("    %d Flaechen vereinfacht, %d unberuehrt\n", vereinfacht, fest);
    expect("Tags, _off und _cap_ in jeder Stufe gleich Stufe 0 (" + std::to_string(fest) + " Flaechen)", fest > 0 && festGleich && fest == ba.unberuehrt);
    expect("jede Ecke jeder Stufe ist eine Ecke von Stufe 0 (Ort, Normale, UV, Knochen, Gewichte)", teilmenge);
    expect("Gewichte: 1 bis 4 je Ecke, Summe 1", gewichteOk);
    expect("Grenzen: jede Flaeche unter SHADER_MAX_VERTEXES, die letzte Stufe unter der Schattengrenze", grenzeOk);
    expect("offene Raender jeder Flaeche bleiben (dieselben Randkanten)", randOk);
    expect("Raender zwischen Flaechen bleiben geschlossen (" + std::to_string(nahtKanten) + " Kanten, jede auch an der Nachbarflaeche)",
           geschlossen && nahtKanten > 0);
    expect("weniger Ecken je Stufe", ba.nachher[1].ecken < ba.nachher[0].ecken && ba.nachher[2].ecken < ba.nachher[1].ecken);
    // die geschriebene Datei liest das Spiel mit drei Stufen
    std::string fehler;
    bool lesbar = true;
    for (int s = 0; s < 3; ++s) {
        GlmModel x;
        lesbar = lesbar && readGlm(ga, x, &fehler, s) && x.numLods == 3 && x.surfaces.size() == a.teile.size();
        if (lesbar) {
            for (std::size_t i = 0; i < x.surfaces.size(); ++i) { lesbar = lesbar && x.surfaces[i].verts.size() == a.teile[i].stufen[static_cast<std::size_t>(s)].ecken.size(); }
        }
    }
    expect("die .glm liest sich mit drei Stufen und den Ecken des Dokuments", lesbar);
    // Stufen entfernen -> wieder die Datei ohne Stufen 1..
    Inhalt d = a;
    detailstufenEntfernen(d);
    expect("Stufen entfernen: eine Stufe", d.stufenZahl == 1 && d.teile[0].stufen.size() == 1U);
    // Prozentzahlen der Stufen vorher (Raven) und nachher fuer die Notizen
    const double r1 = static_cast<double>(ba.nachher[1].ecken) / ba.nachher[0].ecken;
    const double r2 = static_cast<double>(ba.nachher[2].ecken) / ba.nachher[0].ecken;
    std::printf("    Anteil nachher %.3f / %.3f (Ziel %.2f / %.2f)\n", r1, r2, static_cast<double>(o.anteil[0]), static_cast<double>(o.anteil[1]));
    expect("Anteile nahe am Ziel (Raender halten etwas mehr)", r1 > 0.55 && r1 < 0.80 && r2 > 0.30 && r2 < 0.60);
    // Clone-Wars-Richtwert: die letzte Stufe hoechstens so viele gezeichnete Ecken (soweit die Raender es erlauben)
    Inhalt cw = in;
    DetailOptionen ocw = o;
    ocw.richtwertLetzte = 500;
    DetailBericht bcw;
    (void)detailstufenErzeugen(cw, ocw, &bcw);
    std::printf("    Richtwert 500: letzte Stufe %d Ecken (ohne Richtwert %d)\n", bcw.nachher[2].ecken, ba.nachher[2].ecken);
    expect("Richtwert senkt die letzte Stufe", bcw.nachher[2].ecken < ba.nachher[2].ecken && bcw.nachher[1].ecken == ba.nachher[1].ecken);
    const auto befund = stufenPruefen(cw, o.eckenGrenze, o.schattenGrenze, 500);
    const bool ueber = bcw.nachher[2].ecken > 500;
    expect("Befund Richtwert genau dann, wenn die letzte Stufe darueber liegt",
           ueber == std::any_of(befund.begin(), befund.end(), [](const StufenBefund& x) { return x.art == StufenBefundArt::Richtwert; }));
    expect("ohne Richtwert kein Richtwert-Befund", [&] {
        const auto x = stufenPruefen(a, o.eckenGrenze, o.schattenGrenze, 0);
        return std::none_of(x.begin(), x.end(), [](const StufenBefund& y) { return y.art == StufenBefundArt::Richtwert || y.art == StufenBefundArt::KeineStufen; });
    }());
    Inhalt eine = in;
    detailstufenEntfernen(eine);
    expect("Richtwert und nur eine Stufe: Befund KeineStufen", [&] {
        const auto x = stufenPruefen(eine, o.eckenGrenze, o.schattenGrenze, 500);
        return std::any_of(x.begin(), x.end(), [](const StufenBefund& y) { return y.art == StufenBefundArt::KeineStufen; });
    }());
    expect("Art: model.glm auf _humanoid ist eine Figur", modellArtVon(in, "models/players/kyle/model.glm") == ModellArt::Figur);
    // Guete gegen die Stufen der Datei (Raven) bei gleicher Eckenzahl: Abstand der Ecken von Stufe 0 zur Flaeche der Stufe (groesster
    // und mittlerer, Einheiten) - zur Auskunft (NOTIZEN_cwmodell_1511.md)
    if (in.stufenZahl >= 3) {
        const auto messen = [](const Inhalt& x, int s, double& gr, double& mi) {
            gr = 0.0;
            double summe = 0.0;
            std::size_t zahl = 0;
            for (const Teil& t : x.teile) {
                if (stufenRolle(t) != StufenRolle::Vereinfachen) { continue; }
                double m = 0.0;
                gr = std::max(gr, netzAbweichung(t.stufen[0], t.stufen[static_cast<std::size_t>(s)], &m));
                summe += m * static_cast<double>(t.stufen[0].ecken.size());
                zahl += t.stufen[0].ecken.size();
            }
            mi = zahl > 0 ? summe / static_cast<double>(zahl) : 0.0;
        };
        const StufenZahlen z0 = stufenZahlen(in, 0);
        Inhalt wir = in;
        DetailOptionen og = o;
        og.anteil[0] = static_cast<float>(stufenZahlen(in, 1).ecken) / static_cast<float>(z0.ecken);
        og.anteil[1] = static_cast<float>(stufenZahlen(in, 2).ecken) / static_cast<float>(z0.ecken);
        (void)detailstufenErzeugen(wir, og);
        for (int s = 1; s < 3; ++s) {
            double rg = 0, rm = 0, wg = 0, wm = 0;
            messen(in, s, rg, rm);
            messen(wir, s, wg, wm);
            std::printf("    Guete Stufe %d: Datei %d Ecken, Abstand groesster %.3f, mittel %.4f | erzeugt %d Ecken, groesster %.3f, mittel %.4f\n", s,
                        stufenZahlen(in, s).ecken, rg, rm, stufenZahlen(wir, s).ecken, wg, wm);
        }
    }
}

void nahtTeil() {
    std::printf("Naht, Grenzen, Gleichheit\n");
    const Netz n = nahtGitter(40);   // 40 x 40 + 40 Ecken der Naht
    VereinfachOptionen v;
    v.zielEcken = 300;
    float f1 = 0.0F;
    float f2 = 0.0F;
    const Netz a = netzVereinfachen(n, v, &f1);
    const Netz b = netzVereinfachen(n, v, &f2);
    expect("zweimal dasselbe Netz", netzGleich(a, b) && f1 == f2);
    std::printf("    %zu -> %zu Ecken, Fehler %.4f\n", n.ecken.size(), a.ecken.size(), static_cast<double>(f1));
    expect("Ziel 300 erreicht", a.ecken.size() <= 300U && a.ecken.size() > 100U);
    bool gemischt = false;
    std::set<Ort> ortA;
    std::set<Ort> ortB;
    const std::vector<std::uint32_t> idx = a.indizes();
    for (std::size_t t = 0; t + 2 < idx.size(); t += 3) {
        int inA = 0;
        for (std::size_t k = 0; k < 3; ++k) { inA += a.ecken[idx[t + k]].uv[0] < 0.5F ? 1 : 0; }
        gemischt = gemischt || (inA != 0 && inA != 3);
        for (std::size_t k = 0; k < 3; ++k) {
            const Ecke& e = a.ecken[idx[t + k]];
            if (std::abs(e.ort[0] - 80.0F) < 1e-4F) { (inA == 3 ? ortA : ortB).insert(ortVon(e)); }
        }
    }
    expect("kein Dreieck mischt die beiden UV-Inseln", !gemischt);
    expect("die Naht bleibt geschlossen: beide Seiten treffen sich an denselben Punkten (" + std::to_string(ortA.size()) + ")", !ortA.empty() && ortA == ortB);
    // Grenzen: eine Flaeche mit 1600 Ecken, Anteil 90 % -> unter 1000; letzte Stufe unter 500 (.glm)
    Inhalt in;
    in.herkunft = Herkunft::Glm;
    in.knochenZahl = 3;
    in.glaPfad = "models/players/cw_probe/cw_probe";
    in.stufenZahl = 1;
    Teil t;
    t.name = "torso";
    t.stufen.push_back(nahtGitter(40));
    in.teile.push_back(t);
    Teil tag;
    tag.name = "*r_hand";
    tag.flaggen = kG2Bolzen;
    tag.stufen.resize(1);
    std::vector<Ecke> te(3);
    te[1].ort[0] = 1.0F;
    te[2].ort[1] = 1.0F;
    tag.stufen[0].aufbauen(te, {0, 1, 2});
    in.teile.push_back(tag);
    Teil kappe;
    kappe.name = "torso_cap_head_off";
    kappe.flaggen = kG2Aus;
    kappe.stufen.push_back(nahtGitter(6));
    in.teile.push_back(kappe);
    DetailOptionen o;
    o.stufen = 3;
    o.anteil[0] = 0.9F;
    o.anteil[1] = 0.8F;
    o.eckenGrenze = 1000;
    o.schattenGrenze = 500;
    DetailBericht br;
    expect("erzeugt", detailstufenErzeugen(in, o, &br));
    std::printf("    Grenzen: %zu / %zu / %zu Ecken\n", in.teile[0].stufen[0].ecken.size(), in.teile[0].stufen[1].ecken.size(), in.teile[0].stufen[2].ecken.size());
    expect("Stufe 1 unter SHADER_MAX_VERTEXES (Anteil 90 % waere 1476)", in.teile[0].stufen[1].ecken.size() < 1000U);
    expect("letzte Stufe unter der Schattengrenze 500", in.teile[0].stufen[2].ecken.size() < 500U);
    expect("Tag und Kappe unberuehrt", br.unberuehrt == 2 && netzGleich(in.teile[1].stufen[2], in.teile[1].stufen[0]) &&
                                           netzGleich(in.teile[2].stufen[1], in.teile[2].stufen[0]));
    const auto befunde = stufenPruefen(in, 1000, 500, 0);
    expect("Stufe 0 ueber der Grenze wird gemeldet (die erzeugten nicht)", befunde.size() == 1U && befunde[0].art == StufenBefundArt::Ecken && befunde[0].stufe == 0);
    // Rollen
    expect("Rollen: Tag, _off, _cap_", stufenRolle(in.teile[1]) == StufenRolle::Tag && stufenRolle(in.teile[2]) == StufenRolle::Aus);
    Teil c2;
    c2.name = "head_cap_torso";
    c2.stufen.push_back(nahtGitter(4));
    expect("Rolle Kappe nach Namen", stufenRolle(c2) == StufenRolle::Kappe);
    expect("Art: *default ist Ausruestung, md3 ein Requisit, Waffen-Ordner Ausruestung",
           [] {
               Inhalt w;
               w.herkunft = Herkunft::Glm;
               w.glaPfad = "*default";
               w.knochenZahl = 1;
               Inhalt r;
               r.herkunft = Herkunft::Md3;
               return modellArtVon(w, "models/x/y.glm") == ModellArt::Ausruestung && modellArtVon(r, "models/map_objects/a/b.md3") == ModellArt::Requisit &&
                      modellArtVon(r, "models/weapons2/blaster_r/blaster.md3") == ModellArt::Ausruestung;
           }());
    expect("Richtwerte 500 / 300 / keiner", cwRichtwert(ModellArt::Figur) == 500 && cwRichtwert(ModellArt::Ausruestung) == 300 && cwRichtwert(ModellArt::Requisit) == 0);
    expect("Vorgaben nach den Spieldateien", std::abs(stufenVorgabe(ModellArt::Figur, 3).anteil[0] - 0.65F) < 1e-6F &&
                                                 std::abs(stufenVorgabe(ModellArt::Ausruestung, 3).anteil[1] - 0.30F) < 1e-6F);
}

void md3Teil() {
    std::printf("MD3\n");
    expect("Pfad der Stufe wie RE_RegisterModel", md3StufenPfad("models/map_objects/cw/gitter.md3", 1) == "models/map_objects/cw/gitter_1.md3" &&
                                                     md3StufenPfad("C:\\x.y\\kiste.MD3", 2) == "C:\\x.y\\kiste_2.md3");
    const std::string bytes = gitterMd3(17, 3);
    Md3Model m;
    std::string fehler;
    expect("Probe gelesen", readMd3(bytes, m, &fehler) && m.numFrames == 3);
    Inhalt in;
    ausMd3(m, in);
    in.md3Datei = std::make_shared<const std::string>(bytes);
    std::string vorher;
    expect("ohne Stufen dieselbe Datei", md3Schreiben(in, vorher, 1000) && vorher == bytes);
    DetailOptionen o;
    o.stufen = 4;   // eine .md3 hat hoechstens 3
    o.anteil[0] = 0.6F;
    o.anteil[1] = 0.3F;
    o.eckenGrenze = 1000;
    DetailBericht br;
    expect("erzeugt, hoechstens drei Stufen", detailstufenErzeugen(in, o, &br) && in.stufenZahl == 3);
    std::string null;
    expect("Stufe 0 schreibt weiter dieselbe Datei", md3Schreiben(in, null, 1000) && null == bytes);
    for (int s = 1; s < 3; ++s) {
        std::string d;
        Md3SchreibBericht mb;
        const bool ok = md3StufeSchreiben(in, s, d, 1000, &mb);
        Md3Model x;
        const bool gelesen = ok && readMd3(d, x, &fehler);
        expect("name_" + std::to_string(s) + ".md3 geschrieben und gelesen (" + std::to_string(d.size()) + " Bytes)", gelesen);
        if (!gelesen) { continue; }
        expect("  alle Bilder und Tags der Datei", x.numFrames == 3 && x.tags.size() == m.tags.size() && x.surfaces.size() == 1U);
        bool tagGleich = x.tags.size() == m.tags.size();
        for (std::size_t k = 0; tagGleich && k < x.tags.size(); ++k) {
            tagGleich = x.tags[k].name == m.tags[k].name && std::memcmp(x.tags[k].origin, m.tags[k].origin, sizeof x.tags[k].origin) == 0;
        }
        expect("  Tags je Bild an ihrem Ort", tagGleich);
        const Md3Surface& sf = x.surfaces[0];
        const Netz& nz = in.teile[0].stufen[static_cast<std::size_t>(s)];
        expect("  Ecken der Stufe (" + std::to_string(sf.numVerts) + " von " + std::to_string(m.surfaces[0].numVerts) + ")",
               sf.numVerts == static_cast<int>(nz.ecken.size()) && sf.numVerts < m.surfaces[0].numVerts);
        bool bilderGleich = true;
        for (int f = 0; f < 3; ++f) {
            const Md3Vertex* neu = sf.frame(f);
            const Md3Vertex* alt = m.surfaces[0].frame(f);
            for (int v = 0; v < sf.numVerts && neu != nullptr && alt != nullptr; ++v) {
                const int q = nz.ecken[static_cast<std::size_t>(v)].dateiIndex;
                bilderGleich = bilderGleich && q >= 0 && std::memcmp(neu[v].xyz, alt[q].xyz, sizeof neu[v].xyz) == 0;
            }
        }
        expect("  jede Ecke in jedem Bild an ihrem Ort der Datei", bilderGleich);
    }
    // Zahlen einer gelesenen .md3 und Grenzen
    const StufenZahlen z = stufenZahlen(m);
    expect("Zahlen der .md3", z.ecken == 289 && z.flaechen == 1 && z.dreiecke == 512);
    std::vector<Md3Model> stufen{m};
    const auto b = stufenPruefen(stufen, 200, 0);
    expect("Grenze je Flaeche fuer .md3 von der Platte", b.size() == 1U && b[0].art == StufenBefundArt::Ecken);
}

}  // namespace

int main(int argc, char** argv) {
    nahtTeil();
    md3Teil();
    if (argc > 1) { glmTeil(argv[1]); }
    std::printf("%d Fehlschlaege\n", fails);
    return fails == 0 ? 0 : 1;
}
