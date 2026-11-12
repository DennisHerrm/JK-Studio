// modell/gewichte.cpp - Gewichte malen und fuers Spiel festlegen (siehe bhed/modell/gewichte.h)
//
//   Ported from OpenJK (codemp/rd-vanilla/G2_misc.cpp, tr_ghoul2.cpp, mdx_format.h; Copyright (C) 1999-2005 Id Software,
//   Inc., 2000-2013 Raven Software, Inc., 2013-2015 OpenJK contributors) and Blender (paint_vertex_weight_utils,
//   brush falloff; Copyright (C) Blender Authors). Both are licensed under the GNU General Public License version 2 (Blender:
//   or later). This file is part of BehavEd-Remake and licensed under the GNU General Public License version 2 or later
//   (LICENSE).
#include "bhed/modell/gewichte.h"

#include "bhed/modell/wof/poly.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <map>
#include <tuple>
#include <unordered_map>
#include <utility>

namespace bhed::modell {
namespace {

// fG2_BONEWEIGHT_RECIPROCAL_MULT ((float)(1.0f/1023.0f)), mdx_format.h:82
constexpr float kKehrwert = static_cast<float>(1.0F / 1023.0F);

void listeSetzen(GewichtListe& l, int knochen, float w) {
    for (auto it = l.begin(); it != l.end(); ++it) {
        if (it->knochen != knochen) { continue; }
        if (w <= 0.0F) {
            l.erase(it);
        } else {
            it->gewicht = w;
        }
        return;
    }
    if (w > 0.0F) { l.push_back(KnochenGewicht{knochen, w}); }
}

float gewichtIn(const GewichtListe& l, int knochen) {
    float w = 0.0F;
    for (const KnochenGewicht& g : l) {
        if (g.knochen == knochen) { w += g.gewicht; }
    }
    return w;
}

}  // namespace

float g2Gewicht(int bits10) noexcept { return kKehrwert * static_cast<float>(bits10); }

GewichtListe listeAus(const Ecke& e) {
    GewichtListe l;
    for (int k = 0; k < 4; ++k) {
        if (e.gewicht[k] <= 0.0F) { continue; }
        bool da = false;
        for (KnochenGewicht& g : l) {
            if (g.knochen == e.knochen[k]) {
                g.gewicht += e.gewicht[k];
                da = true;
            }
        }
        if (!da) { l.push_back(KnochenGewicht{e.knochen[k], e.gewicht[k]}); }
    }
    return l;
}

bool fuersSpiel(GewichtListe l, SpielGewichte& aus) {
    aus = SpielGewichte{};
    // gleiche Knochen zusammen, nichts <= 0
    GewichtListe z;
    for (const KnochenGewicht& g : l) {
        if (!(g.gewicht > 0.0F) || g.knochen < 0) { continue; }
        bool da = false;
        for (KnochenGewicht& x : z) {
            if (x.knochen == g.knochen) {
                x.gewicht += g.gewicht;
                da = true;
            }
        }
        if (!da) { z.push_back(g); }
    }
    if (z.empty()) { return false; }
    // absteigend wie in den Raven-Dateien; bei gleichem Gewicht der kleinere Knochen zuerst
    std::sort(z.begin(), z.end(), [](const KnochenGewicht& a, const KnochenGewicht& b) {
        return a.gewicht != b.gewicht ? a.gewicht > b.gewicht : a.knochen < b.knochen;
    });
    if (z.size() > 4U) { z.resize(4U); }   // iMAX_G2_BONEWEIGHTS_PER_VERT
    // AP8 (Fund aus AP12): runden die ersten n-1 Gewichte zusammen auf 1023 von 1023, bleibt dem letzten als Rest genau 0 -
    // ein Knochen ohne Gewicht, den der Schreiber weglaesst (glmdatei.cpp, paare). Das Spiel liest dann das vorletzte
    // Gewicht nicht mehr als 10-Bit-Wert, sondern als Rest (1 - Summe): 3/1023 steht als 0,00293255132 in der Ecke und
    // kommt als 0,00293254852 aus der Datei (tests/modell_gewichte2.cpp). Darum faellt der Knochen hier schon weg und es
    // wird neu gerechnet, bis das letzte Gewicht groesser als 0 ist.
    for (;;) {
        // Normieren; was auf 10 Bit 0 wuerde (unter einem halben 1023stel), traegt die Datei nicht.
        for (int runde = 0; runde < 2; ++runde) {
            double summe = 0.0;
            for (const KnochenGewicht& g : z) { summe += g.gewicht; }
            for (KnochenGewicht& g : z) { g.gewicht = static_cast<float>(g.gewicht / summe); }
            if (z.size() > 1U) {
                z.erase(std::remove_if(z.begin() + 1, z.end(), [](const KnochenGewicht& g) { return g.gewicht * 1023.0F < 0.5F; }), z.end());
            }
        }
        const int n = static_cast<int>(z.size());
        aus = SpielGewichte{};
        aus.zahl = n;
        int bitSumme = 0;
        for (int k = 0; k < n; ++k) {
            aus.knochen[k] = z[static_cast<std::size_t>(k)].knochen;
            if (k + 1 < n) {
                aus.bits[k] = std::clamp(static_cast<int>(std::lround(z[static_cast<std::size_t>(k)].gewicht * 1023.0F)), 0, 1023);
                bitSumme += aus.bits[k];
            }
        }
        // Der Rest fuer das letzte Gewicht darf nicht unter 0 fallen: zu viel Gerundetes vom groessten abziehen.
        while (bitSumme > 1023 && n > 1) {
            --aus.bits[0];
            --bitSumme;
        }
        // Lesen wie G2_GetVertBoneWeight: Summe in float, das letzte ist 1 - Summe.
        float summe = 0.0F;
        for (int k = 0; k < n; ++k) {
            if (k == n - 1) {
                aus.gewicht[k] = 1.0F - summe;
                aus.bits[k] = 0;
            } else {
                aus.gewicht[k] = g2Gewicht(aus.bits[k]);
                summe += aus.gewicht[k];
            }
        }
        if (n <= 1 || aus.gewicht[n - 1] > 0.0F) { return true; }
        z.pop_back();   // absteigend sortiert: der letzte ist der kleinste
    }
}

bool aufEcke(const GewichtListe& l, Ecke& e) {
    SpielGewichte s;
    if (!fuersSpiel(l, s)) { return false; }
    for (int k = 0; k < 4; ++k) {
        e.knochen[k] = k < s.zahl ? static_cast<std::uint8_t>(std::clamp(s.knochen[k], 0, 255)) : 0U;
        e.gewicht[k] = k < s.zahl ? s.gewicht[k] : 0.0F;
    }
    return true;
}

void malAnlegen(Netz& n) {
    if (n.mal.size() == n.ecken.size()) { return; }
    n.mal.clear();
    n.mal.reserve(n.ecken.size());
    for (const Ecke& e : n.ecken) { n.mal.push_back(listeAus(e)); }
}

int eckenUeberVier(const Netz& n) {
    int z = 0;
    for (const GewichtListe& l : n.mal) {
        int g = 0;
        for (const KnochenGewicht& x : l) { g += x.gewicht > 0.0F ? 1 : 0; }
        z += g > 4 ? 1 : 0;
    }
    return z;
}

float gewichtVon(const Netz& n, int ecke, int knochen) {
    if (ecke < 0 || static_cast<std::size_t>(ecke) >= n.ecken.size()) { return 0.0F; }
    if (static_cast<std::size_t>(ecke) < n.mal.size()) { return gewichtIn(n.mal[static_cast<std::size_t>(ecke)], knochen); }
    return gewichtIn(listeAus(n.ecken[static_cast<std::size_t>(ecke)]), knochen);
}

float abfall(float abstand, float radius) noexcept {
    if (radius <= 0.0F || abstand >= radius) { return 0.0F; }
    const float p = 1.0F - std::max(abstand, 0.0F) / radius;
    return 3.0F * p * p - 2.0F * p * p * p;
}

int malen(Netz& n, const std::vector<Tupfer>& t, int knochen, PinselArt art, float wert, const MalRegeln& regeln) {
    if (t.empty() || knochen < 0 || regeln.sperrt(knochen)) { return 0; }
    malAnlegen(n);
    // Je Punkt die staerkste Beruehrung - Ecken an einer UV-Naht malen gleich.
    std::vector<float> punktS(static_cast<std::size_t>(std::max(n.punktZahl, 0)), 0.0F);
    for (const Tupfer& x : t) {
        if (x.ecke < 0 || static_cast<std::size_t>(x.ecke) >= n.ecken.size()) { continue; }
        const int p = n.punktVon[static_cast<std::size_t>(x.ecke)];
        if (p >= 0 && static_cast<std::size_t>(p) < punktS.size()) {
            punktS[static_cast<std::size_t>(p)] = std::max(punktS[static_cast<std::size_t>(p)], std::clamp(x.staerke, 0.0F, 1.0F));
        }
    }
    // Werte je Punkt vor dem Tupfer (die erste Ecke des Punkts)
    std::vector<float> vorher(punktS.size(), 0.0F);
    std::vector<char> gesetzt(punktS.size(), 0);
    for (std::size_t e = 0; e < n.ecken.size(); ++e) {
        const auto p = static_cast<std::size_t>(n.punktVon[e]);
        if (p < vorher.size() && gesetzt[p] == 0) {
            vorher[p] = gewichtIn(n.mal[e], knochen);
            gesetzt[p] = 1;
        }
    }
    std::vector<float> mittel;
    if (art == PinselArt::Glaetten) {
        // Nachbarn ueber die Dreiecke: je Kante beide Richtungen
        std::vector<double> summe(punktS.size(), 0.0);
        std::vector<int> zahl(punktS.size(), 0);
        for (std::size_t h = 0; h < n.kanten.size(); ++h) {
            const auto a = static_cast<std::size_t>(n.kanten[h].punkt);
            const auto b = static_cast<std::size_t>(n.kanten[static_cast<std::size_t>(n.kanten[h].naechste)].punkt);
            if (a >= punktS.size() || b >= punktS.size() || a == b) { continue; }
            summe[a] += vorher[b];
            ++zahl[a];
            summe[b] += vorher[a];
            ++zahl[b];
        }
        mittel.resize(punktS.size());
        for (std::size_t p = 0; p < mittel.size(); ++p) { mittel[p] = zahl[p] > 0 ? static_cast<float>(summe[p] / zahl[p]) : vorher[p]; }
    }
    int geaendert = 0;
    for (std::size_t e = 0; e < n.ecken.size(); ++e) {
        const auto p = static_cast<std::size_t>(n.punktVon[e]);
        if (p >= punktS.size() || punktS[p] <= 0.0F) { continue; }
        const float s = punktS[p];
        const float w = vorher[p];
        float neu = w;
        switch (art) {
            case PinselArt::Hinzufuegen: neu = std::min(1.0F, w + wert * s); break;
            case PinselArt::Abziehen: neu = std::max(0.0F, w - wert * s); break;
            case PinselArt::Setzen: neu = w + (wert - w) * s; break;
            case PinselArt::Glaetten: neu = w + (mittel[p] - w) * s; break;
        }
        neu = std::clamp(neu, 0.0F, 1.0F);
        if (neu != gewichtIn(n.mal[e], knochen)) {
            listeSetzen(n.mal[e], knochen, neu);
            if (regeln.autoNormalisieren) { normalisierenNachMalen(n.mal[e], knochen, regeln.gesperrt); }
            ++geaendert;
        }
    }
    return geaendert;
}

int spiegelKnochen(const std::vector<std::string>& namen, int knochen) {
    if (knochen < 0 || static_cast<std::size_t>(knochen) >= namen.size()) { return -1; }
    const std::string& n = namen[static_cast<std::size_t>(knochen)];
    const auto finde = [&](const std::string& x) -> int {
        for (std::size_t i = 0; i < namen.size(); ++i) {
            if (namen[i].size() != x.size()) { continue; }
            bool gleich = true;
            for (std::size_t k = 0; k < x.size() && gleich; ++k) {
                gleich = std::tolower(static_cast<unsigned char>(namen[i][k])) == std::tolower(static_cast<unsigned char>(x[k]));
            }
            if (gleich) { return static_cast<int>(i); }
        }
        return -1;
    };
    const auto tausch = [](char c) {
        switch (c) {
            case 'l': return 'r';
            case 'r': return 'l';
            case 'L': return 'R';
            case 'R': return 'L';
            default: return c;
        }
    };
    if (n.size() > 2 && (n[1] == '_') && tausch(n[0]) != n[0]) {   // l_ / r_
        std::string m = n;
        m[0] = tausch(m[0]);
        return finde(m);
    }
    if (n.size() > 2 && n[n.size() - 2] == '_' && tausch(n.back()) != n.back()) {   // _l / _r
        std::string m = n;
        m.back() = tausch(m.back());
        return finde(m);
    }
    if (n.size() > 1 && tausch(n[0]) != n[0]) {   // lhand / rhand: nur mit Partner
        std::string m = n;
        m[0] = tausch(m[0]);
        return finde(m);
    }
    return -1;
}

std::vector<std::vector<EckOrt>> spiegelKarte(const Inhalt& in, int stufe, float toleranz) {
    std::vector<std::vector<EckOrt>> karte(in.teile.size());
    const float zelle = std::max(toleranz, 1e-4F);
    using Schluessel = std::tuple<long, long, long>;
    std::multimap<Schluessel, EckOrt> raster;
    const auto schl = [&](float x, float y, float z) {
        return Schluessel{std::lround(std::floor(x / zelle)), std::lround(std::floor(y / zelle)), std::lround(std::floor(z / zelle))};
    };
    for (std::size_t t = 0; t < in.teile.size(); ++t) {
        if (in.teile[t].istTag() || static_cast<std::size_t>(stufe) >= in.teile[t].stufen.size()) { continue; }
        const Netz& n = in.teile[t].stufen[static_cast<std::size_t>(stufe)];
        karte[t].assign(n.ecken.size(), EckOrt{});
        for (std::size_t e = 0; e < n.ecken.size(); ++e) {
            raster.emplace(schl(n.ecken[e].ort[0], n.ecken[e].ort[1], n.ecken[e].ort[2]), EckOrt{static_cast<int>(t), static_cast<int>(e)});
        }
    }
    for (std::size_t t = 0; t < karte.size(); ++t) {
        if (karte[t].empty()) { continue; }
        const Netz& n = in.teile[t].stufen[static_cast<std::size_t>(stufe)];
        for (std::size_t e = 0; e < n.ecken.size(); ++e) {
            const float* o = n.ecken[e].ort;
            const float ziel[3] = {-o[0], o[1], o[2]};
            const Schluessel s = schl(ziel[0], ziel[1], ziel[2]);
            float best = toleranz * toleranz;
            EckOrt treffer;
            for (long dx = -1; dx <= 1; ++dx) {
                for (long dy = -1; dy <= 1; ++dy) {
                    for (long dz = -1; dz <= 1; ++dz) {
                        const auto bereich = raster.equal_range(Schluessel{std::get<0>(s) + dx, std::get<1>(s) + dy, std::get<2>(s) + dz});
                        for (auto it = bereich.first; it != bereich.second; ++it) {
                            const float* q = in.teile[static_cast<std::size_t>(it->second.teil)]
                                                 .stufen[static_cast<std::size_t>(stufe)]
                                                 .ecken[static_cast<std::size_t>(it->second.ecke)]
                                                 .ort;
                            const float d = (q[0] - ziel[0]) * (q[0] - ziel[0]) + (q[1] - ziel[1]) * (q[1] - ziel[1]) +
                                            (q[2] - ziel[2]) * (q[2] - ziel[2]);
                            if (d <= best) {
                                best = d;
                                treffer = it->second;
                            }
                        }
                    }
                }
            }
            karte[t][e] = treffer;
        }
    }
    return karte;
}

int spiegeln(Inhalt& in, int stufe, const std::vector<std::string>& knochenNamen, int quelle, float toleranz) {
    // AP8: ueber die Punkte aller Teile (spiegelnSeite) - bis rc611 bekam am Ziel nur EINE von mehreren Ecken gleichen Orts
    // (UV-Naht, Grenze zweier Flaechen) die Gewichte, die andere blieb, und das Netz riss dort in der Bewegung.
    const Schweissnetz s = schweissen(in, stufe);
    return spiegelnSeite(in, stufe, knochenNamen, s, spiegelPartner(in, stufe, s, toleranz), quelle, toleranz);
}

int uebernehmen(Inhalt& in) {
    int z = 0;
    for (Teil& t : in.teile) {
        for (Netz& n : t.stufen) {
            for (std::size_t e = 0; e < n.mal.size() && e < n.ecken.size(); ++e) {
                if (aufEcke(n.mal[e], n.ecken[e])) { ++z; }
            }
            n.mal.clear();
        }
    }
    return z;
}

// =====================================================================================================================
// AP8: Werkzeuge des Bereichs Weights wie 3ds Max und wie Blender (siehe bhed/modell/gewichte.h)

namespace {

// VERTEX_WEIGHT_LOCK_EPSILON (Blender BKE_deform.hh:167)
constexpr float kSperrEpsilon = 1e-6F;

std::size_t uz(int i) noexcept { return static_cast<std::size_t>(i); }

bool istGesperrt(const std::vector<char>& gesperrt, int knochen) noexcept {
    return knochen >= 0 && uz(knochen) < gesperrt.size() && gesperrt[uz(knochen)] != 0;
}

const Netz* netzDer(const Inhalt& in, int stufe, int teil) noexcept {
    if (teil < 0 || uz(teil) >= in.teile.size() || in.teile[uz(teil)].istTag() || stufe < 0 || uz(stufe) >= in.teile[uz(teil)].stufen.size()) {
        return nullptr;
    }
    return &in.teile[uz(teil)].stufen[uz(stufe)];
}

GewichtListe leseListe(const Netz& n, std::size_t e) { return e < n.mal.size() ? n.mal[e] : listeAus(n.ecken[e]); }

// Die Gewichte eines Punkts: die seiner ersten Ecke.
GewichtListe punktListe(const Inhalt& in, int stufe, const Schweissnetz& s, int p) {
    const EckOrt o = s.ecken[uz(p)].front();
    return leseListe(in.teile[uz(o.teil)].stufen[uz(stufe)], uz(o.ecke));
}

bool gleicheListe(const GewichtListe& a, const GewichtListe& b) {
    for (const KnochenGewicht& g : a) {
        if (gewichtIn(b, g.knochen) != gewichtIn(a, g.knochen)) { return false; }
    }
    for (const KnochenGewicht& g : b) {
        if (gewichtIn(a, g.knochen) != gewichtIn(b, g.knochen)) { return false; }
    }
    return true;
}

// Auf alle Ecken des Punkts. Rueckgabe: beschriebene Ecken (ohne `immer` nur die, deren Liste anders war).
int punktSchreiben(Inhalt& in, int stufe, const Schweissnetz& s, int p, const GewichtListe& l, bool immer = false) {
    int z = 0;
    for (const EckOrt& o : s.ecken[uz(p)]) {
        Netz& n = in.teile[uz(o.teil)].stufen[uz(stufe)];
        if (!immer && gleicheListe(leseListe(n, uz(o.ecke)), l)) { continue; }
        malAnlegen(n);
        n.mal[uz(o.ecke)] = l;
        ++z;
    }
    return z;
}

void ohneNull(GewichtListe& l) {
    l.erase(std::remove_if(l.begin(), l.end(), [](const KnochenGewicht& g) { return !(g.gewicht > 0.0F); }), l.end());
}

// Gleiche Knochen einer Liste zusammen (eine Malliste fuehrt jeden Knochen einmal).
void zusammenfassen(GewichtListe& l) {
    GewichtListe z;
    for (const KnochenGewicht& g : l) {
        bool da = false;
        for (KnochenGewicht& x : z) {
            if (x.knochen == g.knochen) {
                x.gewicht += g.gewicht;
                da = true;
            }
        }
        if (!da) { z.push_back(g); }
    }
    l = std::move(z);
}

struct OrtSchl {
    std::uint32_t x = 0, y = 0, z = 0;
    bool operator==(const OrtSchl& o) const noexcept { return x == o.x && y == o.y && z == o.z; }
};
struct OrtSchlHash {
    std::size_t operator()(const OrtSchl& k) const noexcept {
        std::uint64_t h = 1469598103934665603ULL;   // FNV-1a
        for (const std::uint32_t v : {k.x, k.y, k.z}) { h = (h ^ v) * 1099511628211ULL; }
        return static_cast<std::size_t>(h);
    }
};
std::uint32_t ortBits(float f) noexcept {
    f += 0.0F;   // -0 wird +0
    std::uint32_t u = 0;
    std::memcpy(&u, &f, sizeof u);
    return u;
}

// Die Punkte, denen ein Befehl gilt: die gewaehlten, ohne Auswahl alle.
std::vector<char> zielPunkte(const Schweissnetz& s, const EckenMaske* wahl) {
    if (wahl == nullptr) { return std::vector<char>(uz(s.zahl()), 1); }
    return punktWahl(s, *wahl);
}

}  // namespace

EckenMaske eckenMaske(const Inhalt& in, int stufe, const std::vector<EckOrt>& ecken) {
    EckenMaske m(in.teile.size());
    for (const EckOrt& o : ecken) {
        const Netz* n = netzDer(in, stufe, o.teil);
        if (n == nullptr || o.ecke < 0 || uz(o.ecke) >= n->ecken.size()) { continue; }
        if (m[uz(o.teil)].empty()) { m[uz(o.teil)].assign(n->ecken.size(), 0); }
        m[uz(o.teil)][uz(o.ecke)] = 1;
    }
    return m;
}

std::vector<EckOrt> eckenListe(const EckenMaske& m) {
    std::vector<EckOrt> aus;
    for (std::size_t t = 0; t < m.size(); ++t) {
        for (std::size_t e = 0; e < m[t].size(); ++e) {
            if (m[t][e] != 0) { aus.push_back(EckOrt{static_cast<int>(t), static_cast<int>(e)}); }
        }
    }
    return aus;
}

const float* Schweissnetz::ort(const Inhalt& in, int stufe, int punkt) const {
    const EckOrt o = ecken[uz(punkt)].front();
    return in.teile[uz(o.teil)].stufen[uz(stufe)].ecken[uz(o.ecke)].ort;
}

Schweissnetz schweissen(const Inhalt& in, int stufe) {
    Schweissnetz s;
    s.punktVon.resize(in.teile.size());
    std::unordered_map<OrtSchl, int, OrtSchlHash> karte;
    for (std::size_t t = 0; t < in.teile.size(); ++t) {
        const Netz* n = netzDer(in, stufe, static_cast<int>(t));
        if (n == nullptr) { continue; }
        s.punktVon[t].resize(n->ecken.size());
        for (std::size_t e = 0; e < n->ecken.size(); ++e) {
            const float* o = n->ecken[e].ort;
            const auto [it, neu] = karte.try_emplace(OrtSchl{ortBits(o[0]), ortBits(o[1]), ortBits(o[2])}, static_cast<int>(s.ecken.size()));
            if (neu) { s.ecken.emplace_back(); }
            s.punktVon[t][e] = it->second;
            s.ecken[uz(it->second)].push_back(EckOrt{static_cast<int>(t), static_cast<int>(e)});
        }
    }
    s.nachbarn.resize(s.ecken.size());
    for (std::size_t t = 0; t < in.teile.size(); ++t) {
        const Netz* n = netzDer(in, stufe, static_cast<int>(t));
        if (n == nullptr) { continue; }
        for (const Halbkante& h : n->kanten) {
            if (h.ecke < 0 || h.naechste < 0 || uz(h.ecke) >= s.punktVon[t].size()) { continue; }
            const int zu = n->kanten[uz(h.naechste)].ecke;
            if (zu < 0 || uz(zu) >= s.punktVon[t].size()) { continue; }
            const int a = s.punktVon[t][uz(h.ecke)];
            const int b = s.punktVon[t][uz(zu)];
            if (a == b) { continue; }
            s.nachbarn[uz(a)].push_back(b);
            s.nachbarn[uz(b)].push_back(a);
        }
    }
    for (std::vector<int>& nb : s.nachbarn) {
        std::sort(nb.begin(), nb.end());
        nb.erase(std::unique(nb.begin(), nb.end()), nb.end());
    }
    return s;
}

std::vector<char> punktWahl(const Schweissnetz& s, const EckenMaske& m) {
    std::vector<char> p(uz(s.zahl()), 0);
    for (std::size_t t = 0; t < m.size() && t < s.punktVon.size(); ++t) {
        for (std::size_t e = 0; e < m[t].size() && e < s.punktVon[t].size(); ++e) {
            if (m[t][e] != 0) { p[uz(s.punktVon[t][e])] = 1; }
        }
    }
    return p;
}

EckenMaske eckenVonPunkten(const Inhalt& in, int stufe, const Schweissnetz& s, const std::vector<char>& punkte) {
    EckenMaske m(in.teile.size());
    for (std::size_t p = 0; p < punkte.size() && p < s.ecken.size(); ++p) {
        if (punkte[p] == 0) { continue; }
        for (const EckOrt& o : s.ecken[p]) {
            if (m[uz(o.teil)].empty()) { m[uz(o.teil)].assign(in.teile[uz(o.teil)].stufen[uz(stufe)].ecken.size(), 0); }
            m[uz(o.teil)][uz(o.ecke)] = 1;
        }
    }
    return m;
}

// --- Normalisieren mit Sperren -----------------------------------------------------------------------------------------

bool normalisierenGesperrt(GewichtListe& l, const std::vector<char>& gesperrt) {
    float summe = 0.0F;
    float summeFrei = 0.0F;
    float gehalten = 0.0F;
    int frei = 0;
    for (const KnochenGewicht& g : l) {
        summe += g.gewicht;
        if (istGesperrt(gesperrt, g.knochen)) {
            gehalten += g.gewicht;
        } else {
            ++frei;
            summeFrei += g.gewicht;
        }
    }
    if (summe == 1.0F) { return true; }
    if (frei == 0) { return false; }
    if (gehalten >= 1.0F - kSperrEpsilon) {
        // die gesperrten tragen schon alles: was frei ist, wird 0
        for (KnochenGewicht& g : l) {
            if (!istGesperrt(gesperrt, g.knochen)) { g.gewicht = 0.0F; }
        }
        return gehalten == 1.0F;
    }
    if (summeFrei != 0.0F) {
        const float f = (1.0F - gehalten) / summeFrei;
        for (KnochenGewicht& g : l) {
            if (!istGesperrt(gesperrt, g.knochen)) { g.gewicht = std::clamp(g.gewicht * f, 0.0F, 1.0F); }
        }
    } else {
        const float f = std::clamp((1.0F - gehalten) / static_cast<float>(frei), 0.0F, 1.0F);
        for (KnochenGewicht& g : l) {
            if (!istGesperrt(gesperrt, g.knochen)) { g.gewicht = f; }
        }
    }
    return true;
}

void normalisierenNachMalen(GewichtListe& l, int aktiv, const std::vector<char>& gesperrt) {
    if (aktiv < 0) { return; }
    // Der gemalte Knochen gilt auch mit Gewicht 0 als zugeordnet (in Blender bleibt sein Eintrag stehen).
    if (std::none_of(l.begin(), l.end(), [aktiv](const KnochenGewicht& g) { return g.knochen == aktiv; })) { l.push_back(KnochenGewicht{aktiv, 0.0F}); }
    std::vector<char> mitAktiv = gesperrt;
    if (mitAktiv.size() <= uz(aktiv)) { mitAktiv.resize(uz(aktiv) + 1U, 0); }
    mitAktiv[uz(aktiv)] = 1;
    if (!normalisierenGesperrt(l, mitAktiv)) { (void)normalisierenGesperrt(l, gesperrt); }
    ohneNull(l);
}

// --- Pinsel wie Blender ---------------------------------------------------------------------------------------------------

namespace {

// Je Punkt des Netzes die staerkste Beruehrung und das Gewicht vor dem Tupfer (wie malen).
void punktStand(const Netz& n, const std::vector<Tupfer>& t, int knochen, std::vector<float>& punktS, std::vector<float>& vorher) {
    punktS.assign(uz(std::max(n.punktZahl, 0)), 0.0F);
    for (const Tupfer& x : t) {
        if (x.ecke < 0 || uz(x.ecke) >= n.ecken.size()) { continue; }
        const int p = n.punktVon[uz(x.ecke)];
        if (p >= 0 && uz(p) < punktS.size()) { punktS[uz(p)] = std::max(punktS[uz(p)], std::clamp(x.staerke, 0.0F, 1.0F)); }
    }
    vorher.assign(punktS.size(), 0.0F);
    std::vector<char> gesetzt(punktS.size(), 0);
    for (std::size_t e = 0; e < n.ecken.size(); ++e) {
        const auto p = uz(n.punktVon[e]);
        if (p < vorher.size() && gesetzt[p] == 0) {
            vorher[p] = gewichtIn(leseListe(n, e), knochen);
            gesetzt[p] = 1;
        }
    }
}

// Das neue Gewicht je Punkt in die Mallisten (mit den Regeln). Rueckgabe: geaenderte Ecken.
int punkteSchreiben(Netz& n, int knochen, const std::vector<char>& betroffen, const std::vector<float>& neu, const MalRegeln& regeln) {
    int geaendert = 0;
    for (std::size_t e = 0; e < n.ecken.size(); ++e) {
        const auto p = uz(n.punktVon[e]);
        if (p >= betroffen.size() || betroffen[p] == 0) { continue; }
        const float w = std::clamp(neu[p], 0.0F, 1.0F);
        if (w == gewichtIn(n.mal[e], knochen)) { continue; }
        listeSetzen(n.mal[e], knochen, w);
        if (regeln.autoNormalisieren) { normalisierenNachMalen(n.mal[e], knochen, regeln.gesperrt); }
        ++geaendert;
    }
    return geaendert;
}

}  // namespace

int verwischen(Netz& n, const std::vector<Tupfer>& t, int knochen, const std::vector<std::array<float, 2>>& ebene, const float richtung[2],
               const MalRegeln& regeln) {
    if (t.empty() || knochen < 0 || regeln.sperrt(knochen) || ebene.size() < n.ecken.size()) { return 0; }
    const float laenge = std::hypot(richtung[0], richtung[1]);
    if (!(laenge > 0.0F)) { return 0; }
    const float rx = richtung[0] / laenge;
    const float ry = richtung[1] / laenge;
    malAnlegen(n);
    std::vector<float> punktS;
    std::vector<float> vorher;
    punktStand(n, t, knochen, punktS, vorher);
    // der Ort jedes Punkts in der Bildebene (seine erste Ecke)
    std::vector<std::array<float, 2>> ort(punktS.size(), std::array<float, 2>{0.0F, 0.0F});
    std::vector<char> hatOrt(punktS.size(), 0);
    for (std::size_t e = 0; e < n.ecken.size(); ++e) {
        const auto p = uz(n.punktVon[e]);
        if (p < ort.size() && hatOrt[p] == 0) {
            ort[p] = ebene[e];
            hatOrt[p] = 1;
        }
    }
    // der Nachbar entgegen der Bewegung: groesstes Skalarprodukt der Richtung Nachbar -> Punkt mit der Pinselbewegung
    // (stroke_dot_max beginnt bei 0: quer oder dahinter liegende Nachbarn zaehlen nicht)
    std::vector<float> bestes(punktS.size(), 0.0F);
    std::vector<float> ziel(punktS.size(), 0.0F);
    std::vector<char> betroffen(punktS.size(), 0);
    for (std::size_t d = 0; d + 2U < n.kanten.size(); d += 3U) {
        const int pt[3] = {n.kanten[d].punkt, n.kanten[d + 1U].punkt, n.kanten[d + 2U].punkt};
        for (int i = 0; i < 3; ++i) {
            const int p = pt[i];
            if (p < 0 || uz(p) >= punktS.size() || punktS[uz(p)] <= 0.0F) { continue; }
            for (int j = 0; j < 3; ++j) {
                const int q = pt[j];
                if (q == p || q < 0 || uz(q) >= punktS.size()) { continue; }
                const float dx = ort[uz(p)][0] - ort[uz(q)][0];
                const float dy = ort[uz(p)][1] - ort[uz(q)][1];
                const float dl = std::hypot(dx, dy);
                if (!(dl > 0.0F)) { continue; }
                const float punktProdukt = (dx * rx + dy * ry) / dl;
                if (punktProdukt > bestes[uz(p)]) {
                    bestes[uz(p)] = punktProdukt;
                    ziel[uz(p)] = vorher[uz(q)];
                    betroffen[uz(p)] = 1;
                }
            }
        }
    }
    std::vector<float> neu(punktS.size(), 0.0F);
    for (std::size_t p = 0; p < neu.size(); ++p) { neu[p] = vorher[p] + (ziel[p] - vorher[p]) * punktS[p]; }
    return punkteSchreiben(n, knochen, betroffen, neu, regeln);
}

int verlauf(Netz& n, const std::vector<std::array<float, 2>>& schirm, const std::vector<char>& sichtbar, const float start[2], const float ende[2],
            VerlaufArt art, int knochen, PinselArt mischung, float wert, float staerke, const MalRegeln& regeln) {
    if (knochen < 0 || regeln.sperrt(knochen) || schirm.size() < n.ecken.size() || sichtbar.size() < n.ecken.size()) { return 0; }
    const float dx = ende[0] - start[0];
    const float dy = ende[1] - start[1];
    const float laenge2 = dx * dx + dy * dy;
    if (!(laenge2 > 0.0F)) { return 0; }
    malAnlegen(n);
    const std::size_t np = uz(std::max(n.punktZahl, 0));
    std::vector<char> betroffen(np, 0);
    std::vector<char> gesehen(np, 0);
    std::vector<float> neu(np, 0.0F);
    for (std::size_t e = 0; e < n.ecken.size(); ++e) {
        const auto p = uz(n.punktVon[e]);
        if (p >= np || gesehen[p] != 0 || sichtbar[e] == 0) { continue; }
        gesehen[p] = 1;
        const float sx = schirm[e][0] - start[0];
        const float sy = schirm[e][1] - start[1];
        // line_point_factor_v2 bzw. Abstand zum Anfang durch die Laenge; vor dem Anfang: ganz (max(0, alpha))
        const float lage = art == VerlaufArt::Linear ? (sx * dx + sy * dy) / laenge2 : std::hypot(sx, sy) / std::sqrt(laenge2);
        const float alpha = abfall(std::max(lage, 0.0F), 1.0F) * std::clamp(staerke, 0.0F, 1.0F);
        if (alpha == 0.0F) { continue; }   // hinter dem Ende: die Ecke bleibt
        const float w = gewichtIn(n.mal[e], knochen);
        float x = w;
        switch (mischung) {   // ED_wpaint_blend_tool: Mix, Add, Subtract
            case PinselArt::Hinzufuegen: x = w + wert * alpha; break;
            case PinselArt::Abziehen: x = w - wert * alpha; break;
            case PinselArt::Glaetten:
            case PinselArt::Setzen: x = w + (wert - w) * alpha; break;
        }
        neu[p] = x;
        betroffen[p] = 1;
    }
    return punkteSchreiben(n, knochen, betroffen, neu, regeln);
}

// --- Gewichts-Werkzeug wie 3ds Max ---------------------------------------------------------------------------------------

std::vector<EckOrt> eckenWachsen(const Inhalt& in, int stufe, const std::vector<EckOrt>& wahl) {
    const Schweissnetz s = schweissen(in, stufe);
    const std::vector<char> alt = punktWahl(s, eckenMaske(in, stufe, wahl));
    std::vector<char> neu = alt;
    for (std::size_t p = 0; p < alt.size(); ++p) {
        if (alt[p] == 0) { continue; }
        for (const int q : s.nachbarn[p]) { neu[uz(q)] = 1; }
    }
    return eckenListe(eckenVonPunkten(in, stufe, s, neu));
}

std::vector<EckOrt> eckenSchrumpfen(const Inhalt& in, int stufe, const std::vector<EckOrt>& wahl) {
    const Schweissnetz s = schweissen(in, stufe);
    const std::vector<char> alt = punktWahl(s, eckenMaske(in, stufe, wahl));
    std::vector<char> neu = alt;
    for (std::size_t p = 0; p < alt.size(); ++p) {
        if (alt[p] == 0) { continue; }
        for (const int q : s.nachbarn[p]) {
            if (alt[uz(q)] == 0) { neu[p] = 0; }
        }
    }
    return eckenListe(eckenVonPunkten(in, stufe, s, neu));
}

namespace {

std::vector<EckOrt> ringOderSchleife(const Inhalt& in, int stufe, const std::vector<EckOrt>& wahl, bool schleife) {
    wof::PolyNetz p = wof::polyAus(in, stufe);
    // Ein Netz aus Dreiecken (kein Netz::polyVon): Dreieckspaare zu Vierecken, nur in diesem Arbeitsnetz. Ueber UV-Naehte
    // hinweg - die Auswahl folgt der Form, nicht der Abwicklung.
    std::vector<int> dreiecke;
    for (std::size_t t = 0; t < in.teile.size(); ++t) {
        const Netz* n = netzDer(in, stufe, static_cast<int>(t));
        if (n == nullptr || n->polyVon.size() == uz(n->dreiecke()) || t >= p.flaecheVonDreieck.size()) { continue; }
        for (const int f : p.flaecheVonDreieck[t]) {
            if (f >= 0) { dreiecke.push_back(f); }
        }
    }
    if (!dreiecke.empty()) {
        wof::QuadParam q;
        q.naehteAchten = false;
        (void)wof::zuVierecken(p, dreiecke, q);
    }
    p.kantenBauen();
    std::vector<char> gewaehlt(p.punkte.size(), 0);
    for (const EckOrt& o : wahl) {
        const Netz* n = netzDer(in, stufe, o.teil);
        if (n == nullptr || o.ecke < 0 || uz(o.ecke) >= n->ecken.size()) { continue; }
        const std::size_t punkt = uz(p.teilPunkte[uz(o.teil)] + n->punktVon[uz(o.ecke)]);
        if (punkt < gewaehlt.size()) { gewaehlt[punkt] = 1; }
    }
    std::vector<char> neu = gewaehlt;
    for (std::size_t k = 0; k < p.kanten.size(); ++k) {
        const wof::PKante& ka = p.kanten[k];
        if (ka.a < 0 || ka.b < 0 || uz(ka.a) >= gewaehlt.size() || uz(ka.b) >= gewaehlt.size() || gewaehlt[uz(ka.a)] == 0 || gewaehlt[uz(ka.b)] == 0) {
            continue;
        }
        for (const int r : schleife ? wof::kantenSchleife(p, static_cast<int>(k)) : wof::kantenRing(p, static_cast<int>(k))) {
            neu[uz(p.kanten[uz(r)].a)] = 1;
            neu[uz(p.kanten[uz(r)].b)] = 1;
        }
    }
    // zurueck auf Ecken - und ueber das Schweissnetz auf alle Ecken gleichen Orts
    EckenMaske m(in.teile.size());
    for (std::size_t t = 0; t < in.teile.size(); ++t) {
        const Netz* n = netzDer(in, stufe, static_cast<int>(t));
        if (n == nullptr) { continue; }
        m[t].assign(n->ecken.size(), 0);
        for (std::size_t e = 0; e < n->ecken.size(); ++e) {
            const std::size_t punkt = uz(p.teilPunkte[t] + n->punktVon[e]);
            if (punkt < neu.size() && neu[punkt] != 0) { m[t][e] = 1; }
        }
    }
    const Schweissnetz s = schweissen(in, stufe);
    return eckenListe(eckenVonPunkten(in, stufe, s, punktWahl(s, m)));
}

// Ein Befehl am Gewicht eines Knochens der gewaehlten Punkte: neuVon(w) -> neues Gewicht.
template <typename F>
int aufGewaehlte(Inhalt& in, int stufe, const EckenMaske& wahl, int knochen, const MalRegeln& regeln, F neuVon) {
    if (knochen < 0 || regeln.sperrt(knochen)) { return 0; }
    const Schweissnetz s = schweissen(in, stufe);
    const std::vector<char> gewaehlt = punktWahl(s, wahl);
    int z = 0;
    for (int p = 0; p < s.zahl(); ++p) {
        if (gewaehlt[uz(p)] == 0) { continue; }
        GewichtListe l = punktListe(in, stufe, s, p);
        const float w = gewichtIn(l, knochen);
        const float neu = std::clamp(neuVon(w), 0.0F, 1.0F);
        if (neu == w) { continue; }
        listeSetzen(l, knochen, neu);
        if (regeln.autoNormalisieren) { normalisierenNachMalen(l, knochen, regeln.gesperrt); }
        z += punktSchreiben(in, stufe, s, p, l);
    }
    return z;
}

}  // namespace

std::vector<EckOrt> eckenRing(const Inhalt& in, int stufe, const std::vector<EckOrt>& wahl) { return ringOderSchleife(in, stufe, wahl, false); }
std::vector<EckOrt> eckenSchleife(const Inhalt& in, int stufe, const std::vector<EckOrt>& wahl) { return ringOderSchleife(in, stufe, wahl, true); }

int gewichtSetzen(Inhalt& in, int stufe, const EckenMaske& wahl, int knochen, float wert, const MalRegeln& regeln) {
    return aufGewaehlte(in, stufe, wahl, knochen, regeln, [wert](float) { return wert; });
}

int gewichtSkalieren(Inhalt& in, int stufe, const EckenMaske& wahl, int knochen, float faktor, const MalRegeln& regeln) {
    return aufGewaehlte(in, stufe, wahl, knochen, regeln, [faktor](float w) { return w * faktor; });
}

// --- Bereinigen wie Blender ------------------------------------------------------------------------------------------------

int normalisierenAlle(Inhalt& in, int stufe, const EckenMaske* wahl, int aktiv, bool aktivHalten, const MalRegeln& regeln) {
    std::vector<char> gehalten = regeln.gesperrt;
    if (aktivHalten && aktiv >= 0) {
        if (gehalten.size() <= uz(aktiv)) { gehalten.resize(uz(aktiv) + 1U, 0); }
        gehalten[uz(aktiv)] = 1;
    }
    // "All groups are locked": mit der Sperrliste ueber alle Knochen des Skeletts
    if (!regeln.gesperrt.empty() && std::none_of(gehalten.begin(), gehalten.end(), [](char c) { return c == 0; })) { return -1; }
    const Schweissnetz s = schweissen(in, stufe);
    const std::vector<char> ziel = zielPunkte(s, wahl);
    int z = 0;
    for (int p = 0; p < s.zahl(); ++p) {
        if (ziel[uz(p)] == 0) { continue; }
        GewichtListe l = punktListe(in, stufe, s, p);
        if (l.empty()) { continue; }
        // BKE_defvert_normalize_ex ohne weiche Sperren
        if (l.size() == 1U) {
            if (!istGesperrt(gehalten, l[0].knochen)) { l[0].gewicht = 1.0F; }
        } else {
            float fest = 0.0F;
            float frei = 0.0F;
            for (const KnochenGewicht& g : l) { (istGesperrt(gehalten, g.knochen) ? fest : frei) += g.gewicht; }
            if (frei <= 0.0F) { continue; }
            const float f = std::max(0.0F, 1.0F - fest) / frei;
            for (KnochenGewicht& g : l) {
                if (!istGesperrt(gehalten, g.knochen)) { g.gewicht = std::clamp(g.gewicht * f, 0.0F, 1.0F); }
            }
        }
        ohneNull(l);
        z += punktSchreiben(in, stufe, s, p, l);
    }
    return z;
}

int saeubern(Inhalt& in, int stufe, const EckenMaske* wahl, float grenze, bool einenBehalten, const MalRegeln& regeln) {
    const Schweissnetz s = schweissen(in, stufe);
    const std::vector<char> ziel = zielPunkte(s, wahl);
    int entfernt = 0;
    for (int p = 0; p < s.zahl(); ++p) {
        if (ziel[uz(p)] == 0) { continue; }
        GewichtListe l = punktListe(in, stufe, s, p);
        int weg = 0;
        // vgroup_parray_remove_zero: von hinten, bei "Keep Single" bleibt die letzte Zuordnung
        for (std::size_t j = l.size(); j-- > 0;) {
            if (einenBehalten && l.size() == 1U) { break; }
            if (l[j].gewicht <= grenze && !regeln.sperrt(l[j].knochen)) {
                l.erase(l.begin() + static_cast<std::ptrdiff_t>(j));
                ++weg;
            }
        }
        if (weg == 0) { continue; }
        if (regeln.autoNormalisieren) { (void)normalisierenGesperrt(l, regeln.gesperrt); }
        entfernt += weg;
        (void)punktSchreiben(in, stufe, s, p, l);
    }
    return entfernt;
}

int glaettenAlle(Inhalt& in, int stufe, const EckenMaske* wahl, float faktor, int wiederholungen, const MalRegeln& regeln) {
    const Schweissnetz s = schweissen(in, stufe);
    const std::size_t np = uz(s.zahl());
    const std::vector<char> ziel = zielPunkte(s, wahl);
    // verts_used: gewaehlte Punkte mit wenigstens einem Nachbarn
    std::vector<int> benutzt;
    for (std::size_t p = 0; p < np; ++p) {
        if (ziel[p] != 0 && !s.nachbarn[p].empty()) { benutzt.push_back(static_cast<int>(p)); }
    }
    if (benutzt.empty() || wiederholungen < 1) { return 0; }
    std::vector<GewichtListe> listen(np);
    std::vector<int> knochen;   // freie Knochen, die an einem benutzten Punkt oder einem seiner Nachbarn vorkommen
    for (std::size_t p = 0; p < np; ++p) { listen[p] = punktListe(in, stufe, s, static_cast<int>(p)); }
    for (const int p : benutzt) {
        for (const KnochenGewicht& g : listen[uz(p)]) { knochen.push_back(g.knochen); }
        for (const int q : s.nachbarn[uz(p)]) {
            for (const KnochenGewicht& g : listen[uz(q)]) { knochen.push_back(g.knochen); }
        }
    }
    std::sort(knochen.begin(), knochen.end());
    knochen.erase(std::unique(knochen.begin(), knochen.end()), knochen.end());
    knochen.erase(std::remove_if(knochen.begin(), knochen.end(), [&](int k) { return regeln.sperrt(k); }), knochen.end());
    const float f = std::clamp(faktor, 0.0F, 1.0F);
    std::vector<float> vorher(np);
    std::vector<float> jetzt(np);
    std::vector<GewichtListe> neu = listen;
    for (const int k : knochen) {
        for (std::size_t p = 0; p < np; ++p) { vorher[p] = gewichtIn(listen[p], k); }
        jetzt = vorher;
        for (int runde = 0; runde < wiederholungen; ++runde) {
            for (const int p : benutzt) {
                float summe = 0.0F;
                for (const int q : s.nachbarn[uz(p)]) { summe += vorher[uz(q)]; }
                const float nachbarMittel = summe / static_cast<float>(s.nachbarn[uz(p)].size());
                jetzt[uz(p)] = std::clamp(vorher[uz(p)] * (1.0F - f) + nachbarMittel * f, 0.0F, 1.0F);
            }
            std::swap(vorher, jetzt);
        }
        for (const int p : benutzt) { listeSetzen(neu[uz(p)], k, vorher[uz(p)]); }   // ohne Gewicht: der Eintrag faellt weg
    }
    int z = 0;
    for (const int p : benutzt) {
        if (regeln.autoNormalisieren) { (void)normalisierenGesperrt(neu[uz(p)], regeln.gesperrt); }
        ohneNull(neu[uz(p)]);
        z += punktSchreiben(in, stufe, s, p, neu[uz(p)]);
    }
    return z;
}

int begrenzen(Inhalt& in, int stufe, const EckenMaske* wahl, int grenze, const MalRegeln& regeln) {
    const Schweissnetz s = schweissen(in, stufe);
    const std::vector<char> ziel = zielPunkte(s, wahl);
    int entfernt = 0;
    for (int p = 0; p < s.zahl(); ++p) {
        if (ziel[uz(p)] == 0) { continue; }
        GewichtListe l = punktListe(in, stufe, s, p);
        if (grenze < 1 || l.size() <= uz(grenze)) { continue; }
        std::sort(l.begin(), l.end(), [](const KnochenGewicht& a, const KnochenGewicht& b) {
            return a.gewicht != b.gewicht ? a.gewicht > b.gewicht : a.knochen < b.knochen;
        });
        entfernt += static_cast<int>(l.size()) - grenze;
        l.resize(uz(grenze));
        if (regeln.autoNormalisieren) { (void)normalisierenGesperrt(l, regeln.gesperrt); }
        (void)punktSchreiben(in, stufe, s, p, l);
    }
    return entfernt;
}

// --- Spiegeln ---------------------------------------------------------------------------------------------------------------

int knochenSeite(const std::vector<std::string>& namen, int knochen) {
    if (knochen < 0 || uz(knochen) >= namen.size()) { return 0; }
    const std::string& n = namen[uz(knochen)];
    const auto seite = [](char c) {
        const int k = std::tolower(static_cast<unsigned char>(c));
        return k == 'l' ? 1 : (k == 'r' ? -1 : 0);
    };
    if (n.size() > 2 && n[1] == '_' && seite(n[0]) != 0) { return seite(n[0]); }                           // l_ / r_
    if (n.size() > 2 && n[n.size() - 2] == '_' && seite(n.back()) != 0) { return seite(n.back()); }         // _l / _r
    if (n.size() > 1 && seite(n[0]) != 0 && spiegelKnochen(namen, knochen) >= 0) { return seite(n[0]); }    // lhand / rhand
    return 0;
}

int linkeSeite(const Inhalt& in, int stufe, const std::vector<std::string>& namen) {
    std::vector<int> seite(namen.size(), 0);
    for (std::size_t k = 0; k < namen.size(); ++k) { seite[k] = knochenSeite(namen, static_cast<int>(k)); }
    double summe[2] = {0.0, 0.0};     // links, rechts: Summe x * Gewicht
    double gewicht[2] = {0.0, 0.0};
    for (std::size_t t = 0; t < in.teile.size(); ++t) {
        const Netz* n = netzDer(in, stufe, static_cast<int>(t));
        if (n == nullptr) { continue; }
        for (std::size_t e = 0; e < n->ecken.size(); ++e) {
            for (const KnochenGewicht& g : leseListe(*n, e)) {
                if (g.knochen < 0 || uz(g.knochen) >= seite.size() || seite[uz(g.knochen)] == 0) { continue; }
                const int i = seite[uz(g.knochen)] > 0 ? 0 : 1;
                summe[i] += static_cast<double>(n->ecken[e].ort[0]) * static_cast<double>(g.gewicht);
                gewicht[i] += static_cast<double>(g.gewicht);
            }
        }
    }
    if (!(gewicht[0] > 0.0) || !(gewicht[1] > 0.0)) { return 0; }
    const double unterschied = summe[0] / gewicht[0] - summe[1] / gewicht[1];
    return unterschied > 0.0 ? 1 : (unterschied < 0.0 ? -1 : 0);
}

std::vector<int> spiegelPartner(const Inhalt& in, int stufe, const Schweissnetz& s, float toleranz) {
    const std::size_t np = uz(s.zahl());
    std::vector<int> partner(np, -1);
    const float zelle = std::max(toleranz, 1e-4F);
    using Schluessel = std::tuple<long, long, long>;
    std::multimap<Schluessel, int> raster;
    const auto schl = [&](const float* o, float vz) {
        return Schluessel{std::lround(std::floor(vz * o[0] / zelle)), std::lround(std::floor(o[1] / zelle)), std::lround(std::floor(o[2] / zelle))};
    };
    for (std::size_t p = 0; p < np; ++p) { raster.emplace(schl(s.ort(in, stufe, static_cast<int>(p)), 1.0F), static_cast<int>(p)); }
    for (std::size_t p = 0; p < np; ++p) {
        const float* o = s.ort(in, stufe, static_cast<int>(p));
        const Schluessel mitte = schl(o, -1.0F);
        float best = toleranz * toleranz;
        for (long dx = -1; dx <= 1; ++dx) {
            for (long dy = -1; dy <= 1; ++dy) {
                for (long dz = -1; dz <= 1; ++dz) {
                    const auto bereich = raster.equal_range(Schluessel{std::get<0>(mitte) + dx, std::get<1>(mitte) + dy, std::get<2>(mitte) + dz});
                    for (auto it = bereich.first; it != bereich.second; ++it) {
                        const float* q = s.ort(in, stufe, it->second);
                        const float d = (q[0] + o[0]) * (q[0] + o[0]) + (q[1] - o[1]) * (q[1] - o[1]) + (q[2] - o[2]) * (q[2] - o[2]);
                        // der naechste; bei gleichem Abstand der Punkt mit der kleineren Nummer (feste Reihenfolge)
                        if (d < best || (d == best && (partner[p] < 0 || it->second < partner[p]))) {
                            best = d;
                            partner[p] = it->second;
                        }
                    }
                }
            }
        }
    }
    return partner;
}

std::vector<int> spiegelPartnerTopologie(const Schweissnetz& s) {
    const std::size_t np = uz(s.zahl());
    std::vector<int> partner(np, -1);
    if (np == 0U) { return partner; }
    // die Kanten des Schweissnetzes, jede einmal
    std::vector<std::pair<int, int>> kanten;
    for (std::size_t p = 0; p < np; ++p) {
        for (const int q : s.nachbarn[p]) {
            if (static_cast<int>(p) < q) { kanten.emplace_back(static_cast<int>(p), q); }
        }
    }
    // ED_mesh_mirrtopo_init: Kennzahl = Zahl der Kanten, dann je Runde die Kennzahlen der Nachbarn mal der Rundennummer dazu
    // (die Zahlen laufen ueber - das ist gewollt), bis die Zahl verschiedener Kennzahlen nicht mehr steigt.
    std::vector<std::uint32_t> kenn(np, 0U);
    for (const auto& [a, b] : kanten) {
        ++kenn[uz(a)];
        ++kenn[uz(b)];
    }
    std::vector<std::uint32_t> vorher = kenn;
    std::vector<std::uint32_t> sortiert;
    long verschiedenVorher = -1;
    long kantenVorher = -1;
    std::uint32_t runde = 1U;
    for (;;) {
        long kantenVerschieden = 0;
        for (const auto& [a, b] : kanten) {
            kenn[uz(a)] += vorher[uz(b)] * runde;
            kenn[uz(b)] += vorher[uz(a)] * runde;
            kantenVerschieden += kenn[uz(a)] != kenn[uz(b)] ? 1 : 0;
        }
        sortiert = kenn;
        std::sort(sortiert.begin(), sortiert.end());
        const long verschieden = static_cast<long>(std::unique(sortiert.begin(), sortiert.end()) - sortiert.begin());
        if (verschieden <= verschiedenVorher && kantenVerschieden <= kantenVorher) { break; }
        verschiedenVorher = verschieden;
        kantenVorher = kantenVerschieden;
        vorher = kenn;
        ++runde;
    }
    std::vector<int> folge(np);
    for (std::size_t p = 0; p < np; ++p) { folge[p] = static_cast<int>(p); }
    std::stable_sort(folge.begin(), folge.end(), [&](int a, int b) { return kenn[uz(a)] < kenn[uz(b)]; });
    std::size_t anfang = 0;
    for (std::size_t a = 1; a <= np; ++a) {
        if (a < np && kenn[uz(folge[a - 1U])] == kenn[uz(folge[a])]) { continue; }
        if (a - anfang == 2U) {
            partner[uz(folge[a - 1U])] = folge[a - 2U];
            partner[uz(folge[a - 2U])] = folge[a - 1U];
        } else if (a - anfang == 1U) {
            partner[uz(folge[a - 1U])] = folge[a - 1U];   // einmalige Kennzahl: ein Punkt der Mitte
        }
        anfang = a;
    }
    return partner;
}

SpiegelBefund spiegelBefund(const std::vector<std::string>& namen, const Schweissnetz& s, const std::vector<int>& partner) {
    SpiegelBefund b;
    for (std::size_t k = 0; k < namen.size(); ++k) {
        if (knochenSeite(namen, static_cast<int>(k)) == 0) { continue; }
        const int m = spiegelKnochen(namen, static_cast<int>(k));
        if (m >= 0 && m != static_cast<int>(k)) {
            if (static_cast<int>(k) < m) {
                ++b.knochenPaare;
                ++b.knochenPaareSoll;
            }
        } else {
            ++b.knochenPaareSoll;
        }
    }
    b.punkte = s.zahl();
    std::vector<std::pair<int, int>> paare;
    for (std::size_t p = 0; p < partner.size() && p < s.ecken.size(); ++p) {
        const int q = partner[p];
        if (q < 0) {
            ++b.ohneGegenstueck;
            b.eckenOhne += static_cast<int>(s.ecken[p].size());
        } else if (q == static_cast<int>(p)) {
            ++b.mitte;
        } else {
            for (const EckOrt& a : s.ecken[p]) {
                for (const EckOrt& c : s.ecken[uz(q)]) {
                    if (a.teil != c.teil) { paare.emplace_back(std::min(a.teil, c.teil), std::max(a.teil, c.teil)); }
                }
            }
        }
    }
    std::sort(paare.begin(), paare.end());
    paare.erase(std::unique(paare.begin(), paare.end()), paare.end());
    b.teilPaare = std::move(paare);
    return b;
}

namespace {

// Die Knochen einer Liste auf die andere Seite (l <-> r nach den Namen; ohne Partner bleibt der Knochen).
GewichtListe gespiegelteListe(GewichtListe l, const std::vector<std::string>& namen) {
    for (KnochenGewicht& g : l) {
        const int m = spiegelKnochen(namen, g.knochen);
        if (m >= 0) { g.knochen = m; }
    }
    zusammenfassen(l);
    return l;
}

}  // namespace

int spiegelnSeite(Inhalt& in, int stufe, const std::vector<std::string>& namen, const Schweissnetz& s, const std::vector<int>& partner, int quelle,
                  float mitte, const EckenMaske* wahl) {
    const std::size_t np = std::min(uz(s.zahl()), partner.size());
    const std::vector<char> gewaehlt = zielPunkte(s, wahl);
    const auto seite = [&](int p) {
        const float x = s.ort(in, stufe, p)[0];
        return x > mitte ? 1 : (x < -mitte ? -1 : 0);
    };
    // Erst alle Quellen lesen, dann schreiben (beim Tauschen ist jeder Punkt Quelle und Ziel).
    std::vector<std::pair<int, GewichtListe>> neu;
    for (std::size_t p = 0; p < np; ++p) {
        const int q = partner[p];
        if (q < 0 || (gewaehlt[p] == 0 && gewaehlt[uz(q)] == 0)) { continue; }
        if (quelle == 0) {
            // Tauschen: jeder Punkt bekommt die gespiegelten Gewichte seines Partners; ein Punkt der Mitte tauscht l <-> r
            neu.emplace_back(static_cast<int>(p), gespiegelteListe(punktListe(in, stufe, s, q), namen));
        } else if (q != static_cast<int>(p) && seite(static_cast<int>(p)) == -quelle && seite(q) == quelle) {
            neu.emplace_back(static_cast<int>(p), gespiegelteListe(punktListe(in, stufe, s, q), namen));
        }
    }
    int z = 0;
    for (const auto& [p, l] : neu) { z += punktSchreiben(in, stufe, s, p, l, true); }
    return z;
}

SpiegelZahl spiegelnBlender(Inhalt& in, int stufe, const std::vector<std::string>& namen, const Schweissnetz& s, const std::vector<int>& partner,
                            const BlenderSpiegel& o, const EckenMaske* wahl) {
    SpiegelZahl zahl;
    if ((!o.gewichte && !o.namenTauschen) || (!o.alleGruppen && o.aktiv < 0)) { return zahl; }
    const std::size_t np = std::min(uz(s.zahl()), partner.size());
    const std::vector<char> gewaehlt = zielPunkte(s, wahl);
    // BKE_object_defgroup_flip_map (alle) bzw. _flip_map_single (nur der aktive Knochen und sein Partner); -1: bleibt
    const int aktivPartner = o.aktiv >= 0 ? spiegelKnochen(namen, o.aktiv) : -1;
    const auto anders = [&](int k) {
        if (o.alleGruppen) { return spiegelKnochen(namen, k); }
        if (aktivPartner < 0) { return -1; }
        return k == o.aktiv ? aktivPartner : (k == aktivPartner ? o.aktiv : -1);
    };
    const auto tauscheNamen = [&](GewichtListe& l) {   // BKE_defvert_flip
        for (KnochenGewicht& g : l) {
            const int m = anders(g.knochen);
            if (m >= 0) { g.knochen = m; }
        }
    };
    std::vector<char> erledigt(np, 0);
    for (std::size_t p = 0; p < np; ++p) {
        if (erledigt[p] != 0) { continue; }
        const int q = partner[p];
        if (q < 0) {
            ++zahl.ohnePartner;
            continue;
        }
        if (q == static_cast<int>(p) || uz(q) >= np || erledigt[uz(q)] != 0) { continue; }
        const bool sel = gewaehlt[p] != 0;
        const bool selPartner = gewaehlt[uz(q)] != 0;
        if (sel || selPartner) {
            // dvert_mirror_op
            GewichtListe a = punktListe(in, stufe, s, static_cast<int>(p));
            GewichtListe b = punktListe(in, stufe, s, q);
            if (sel && selPartner) {   // tauschen
                if (o.gewichte) {
                    if (o.alleGruppen) {
                        std::swap(a, b);
                    } else {
                        const float wa = gewichtIn(a, o.aktiv);
                        const float wb = gewichtIn(b, o.aktiv);
                        listeSetzen(a, o.aktiv, wb);
                        listeSetzen(b, o.aktiv, wa);
                    }
                }
                if (o.namenTauschen) {
                    tauscheNamen(a);
                    tauscheNamen(b);
                }
                (void)punktSchreiben(in, stufe, s, static_cast<int>(p), a, true);
                (void)punktSchreiben(in, stufe, s, q, b, true);
            } else {   // der gewaehlte Punkt ist das Ziel, der andere die Quelle
                GewichtListe& ziel = sel ? a : b;
                const GewichtListe& quellListe = sel ? b : a;
                if (o.gewichte) {
                    if (o.alleGruppen) {
                        ziel = quellListe;                                            // BKE_defvert_copy
                    } else {
                        listeSetzen(ziel, o.aktiv, gewichtIn(quellListe, o.aktiv));   // BKE_defvert_copy_index
                    }
                }
                if (o.namenTauschen) { tauscheNamen(ziel); }
                (void)punktSchreiben(in, stufe, s, sel ? static_cast<int>(p) : q, ziel, true);
            }
            ++zahl.gespiegelt;
        }
        erledigt[p] = 1;
        erledigt[uz(q)] = 1;
    }
    return zahl;
}

int spiegelMalen(Inhalt& in, int stufe, const Schweissnetz& s, const std::vector<int>& partner, const std::vector<std::vector<Tupfer>>& jeTeil,
                 int knochen, int spiegelKn, const MalRegeln& regeln) {
    const int zielKnochen = spiegelKn >= 0 ? spiegelKn : knochen;
    if (knochen < 0 || regeln.sperrt(zielKnochen)) { return 0; }   // "Mirror group is locked, aborting"
    std::vector<char> getupft(uz(s.zahl()), 0);
    std::vector<int> folge;
    for (std::size_t t = 0; t < jeTeil.size() && t < s.punktVon.size(); ++t) {
        for (const Tupfer& x : jeTeil[t]) {
            if (x.ecke < 0 || uz(x.ecke) >= s.punktVon[t].size() || !(x.staerke > 0.0F)) { continue; }
            const int p = s.punktVon[t][uz(x.ecke)];
            if (getupft[uz(p)] == 0) {
                getupft[uz(p)] = 1;
                folge.push_back(p);
            }
        }
    }
    int z = 0;
    for (const int p : folge) {
        const int q = uz(p) < partner.size() ? partner[uz(p)] : -1;
        if (q < 0 || (q == p && zielKnochen == knochen)) { continue; }   // derselbe Punkt, derselbe Knochen: nicht doppelt
        const float w = gewichtIn(punktListe(in, stufe, s, p), knochen);
        GewichtListe l = punktListe(in, stufe, s, q);
        if (gewichtIn(l, zielKnochen) == w) { continue; }
        listeSetzen(l, zielKnochen, w);
        if (regeln.autoNormalisieren && q == p) {
            // ein Punkt der Mitte traegt beide Knochen des Paars gleich: erst teilen sich die uebrigen den Rest, reicht das
            // nicht, werden beide im selben Verhaeltnis kleiner (Blender: "just balance out the 2 weights")
            std::vector<char> beide = regeln.gesperrt;
            if (beide.size() <= uz(std::max(knochen, zielKnochen))) { beide.resize(uz(std::max(knochen, zielKnochen)) + 1U, 0); }
            beide[uz(knochen)] = 1;
            beide[uz(zielKnochen)] = 1;
            if (!normalisierenGesperrt(l, beide)) { (void)normalisierenGesperrt(l, regeln.gesperrt); }
            ohneNull(l);
        } else if (regeln.autoNormalisieren) {
            normalisierenNachMalen(l, zielKnochen, regeln.gesperrt);
        }
        z += punktSchreiben(in, stufe, s, q, l);
    }
    return z;
}

}  // namespace bhed::modell
