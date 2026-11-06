// modell/gewichte.cpp - Gewichte malen und fuers Spiel festlegen (siehe bhed/modell/gewichte.h)
//
//   Ported from OpenJK (codemp/rd-vanilla/G2_misc.cpp, tr_ghoul2.cpp, mdx_format.h; Copyright (C) 1999-2005 Id Software,
//   Inc., 2000-2013 Raven Software, Inc., 2013-2015 OpenJK contributors) and Blender (paint_vertex_weight_utils,
//   brush falloff; Copyright (C) Blender Authors). Both are licensed under the GNU General Public License version 2 (Blender:
//   or later). This file is part of BehavEd-Remake and licensed under the GNU General Public License version 2 or later
//   (LICENSE).
#include "bhed/modell/gewichte.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <tuple>

namespace bhed::modell {
namespace {

// fG2_BONEWEIGHT_RECIPROCAL_MULT ((float)(1.0f/1023.0f)), mdx_format.h:82
constexpr float kKehrwert = static_cast<float>(1.0F / 1023.0F);

void gewichtSetzen(GewichtListe& l, int knochen, float w) {
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
    return true;
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

int malen(Netz& n, const std::vector<Tupfer>& t, int knochen, PinselArt art, float wert) {
    if (t.empty() || knochen < 0) { return 0; }
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
            gewichtSetzen(n.mal[e], knochen, neu);
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
    const std::vector<std::vector<EckOrt>> karte = spiegelKarte(in, stufe, toleranz);
    // Erst alle Quellen lesen, dann schreiben (ein Teil kann Quelle und Ziel zugleich sein).
    struct Schreiben {
        EckOrt ziel;
        GewichtListe l;
    };
    std::vector<Schreiben> neu;
    for (std::size_t t = 0; t < karte.size(); ++t) {
        for (std::size_t e = 0; e < karte[t].size(); ++e) {
            const EckOrt p = karte[t][e];
            if (p.teil < 0) { continue; }
            const Netz& nq = in.teile[t].stufen[static_cast<std::size_t>(stufe)];
            const float x = nq.ecken[e].ort[0];
            if (!(quelle > 0 ? x > toleranz : x < -toleranz)) { continue; }
            GewichtListe l = e < nq.mal.size() ? nq.mal[e] : listeAus(nq.ecken[e]);
            for (KnochenGewicht& g : l) {
                const int m = spiegelKnochen(knochenNamen, g.knochen);
                if (m >= 0) { g.knochen = m; }
            }
            neu.push_back(Schreiben{p, std::move(l)});
        }
    }
    for (Schreiben& s : neu) {
        Netz& nz = in.teile[static_cast<std::size_t>(s.ziel.teil)].stufen[static_cast<std::size_t>(stufe)];
        malAnlegen(nz);
        nz.mal[static_cast<std::size_t>(s.ziel.ecke)] = std::move(s.l);
    }
    return static_cast<int>(neu.size());
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

}  // namespace bhed::modell
