// modell/stapel.cpp - der Modifier-Stapel einer Flaeche rechnen (siehe bhed/modell/stapel.h)
//
//   Glaetten: Catmull-Clark-Unterteilung (E. Catmull, J. Clark, Computer-Aided Design 10(6), 1978), hier neu geschrieben auf
//   dem Arbeitsnetz des Modellierens (bhed/modell/wof/poly.h). Spiegeln ruft wof::symmetrisieren. This file is part of
//   BehavEd-Remake and licensed under the GNU General Public License version 2 or later (LICENSE); the mesh helpers it calls
//   (src/modell/wof) are BSD-3-Clause.
#include "bhed/modell/stapel.h"

#include "bhed/modell/gewichte.h"
#include "wof/mesh_intern.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <map>
#include <utility>
#include <vector>

namespace bhed::modell {

using wof::Ablehnung;
using wof::PEcke;
using wof::PFlaeche;
using wof::PKante;
using wof::PolyNetz;
using wof::mi::V;
using wof::mi::uz;
using wof::mi::zi;

namespace {

// Kanten ueber diesem Winkel bleiben nach einem Eintrag hart - derselbe Wert wie nach jedem Werkzeug des Modellierens
// (gui/wof/app_modelltab_mesh.cpp abschliessen: w::normalenNeu(n, 60), WL kDefaultShadingAngle)
constexpr float kKnickGrad = 60.0F;

// Das Mittel mehrerer Ecken eines Vielecks: UV und Normale linear; Knochen und Gewichte zusammengelegt, die vier groessten,
// Summe 1 (iMAX_G2_BONEWEIGHTS_PER_VERT 4, mdx_format.h - dieselbe Regel wie wof::mi::mischen fuer zwei Ecken). Das Ergebnis
// ist keine Ecke der Datei mehr.
Ecke mittel(const std::vector<const Ecke*>& l) {
    Ecke e;
    if (l.empty()) {
        e.gewicht[0] = 1.0F;
        return e;
    }
    const double g = 1.0 / static_cast<double>(l.size());
    double uv[2] = {0.0, 0.0};
    V nn;
    std::map<int, double> w;
    for (const Ecke* x : l) {
        uv[0] += static_cast<double>(x->uv[0]) * g;
        uv[1] += static_cast<double>(x->uv[1]) * g;
        nn = nn + wof::mi::vAus(x->normale) * g;
        for (int k = 0; k < 4; ++k) {
            if (x->gewicht[k] > 0.0F) { w[x->knochen[k]] += static_cast<double>(x->gewicht[k]) * g; }
        }
    }
    e.uv[0] = static_cast<float>(uv[0]);
    e.uv[1] = static_cast<float>(uv[1]);
    wof::mi::vNach(wof::mi::einheit(nn), e.normale);
    std::vector<std::pair<double, int>> liste;
    for (const auto& [knochen, gewicht] : w) {
        if (gewicht > 0.0) { liste.emplace_back(gewicht, knochen); }
    }
    std::stable_sort(liste.begin(), liste.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
    if (liste.size() > 4U) { liste.resize(4U); }
    double summe = 0.0;
    for (const auto& x : liste) { summe += x.first; }
    for (std::size_t k = 0; k < liste.size(); ++k) {
        e.knochen[k] = static_cast<std::uint8_t>(liste[k].second);
        e.gewicht[k] = summe > 0.0 ? static_cast<float>(liste[k].first / summe) : 0.0F;
    }
    if (liste.empty()) { e.gewicht[0] = 1.0F; }   // starr wie eine .md3 (ausMd3: Knochen 0, Gewicht 1)
    return e;
}

// Eine Unterteilung nach Catmull und Clark auf allen Vielecken des Arbeitsnetzes (es fuehrt genau eine Flaeche). Die Regeln
// stehen in stapel.h; eine Kante mit mehr als zwei Seiten (nicht mannigfaltig) und ihre Enden bleiben, wo sie sind.
void glaettenSchritt(PolyNetz& n, bool raenderHalten) {
    n.kantenBauen();
    const std::size_t pz = n.punkte.size();
    const std::size_t fz = n.flaechen.size();
    const std::size_t kz = n.kanten.size();
    std::vector<V> fOrt(fz);
    for (std::size_t f = 0; f < fz; ++f) {
        if (!n.flaechen[f].weg) { fOrt[f] = wof::mi::mitte(n, n.flaechen[f]); }
    }
    std::vector<std::vector<int>> punktKanten(pz);
    for (std::size_t k = 0; k < kz; ++k) {
        punktKanten[uz(n.kanten[k].a)].push_back(zi(k));
        punktKanten[uz(n.kanten[k].b)].push_back(zi(k));
    }
    std::vector<V> kOrt(kz);
    for (std::size_t k = 0; k < kz; ++k) {
        const PKante& ka = n.kanten[k];
        const V a = wof::mi::ort(n, ka.a);
        const V b = wof::mi::ort(n, ka.b);
        if (ka.seiten.size() == 2U) {
            kOrt[k] = (a + b + fOrt[uz(ka.seiten[0].first)] + fOrt[uz(ka.seiten[1].first)]) * 0.25;
        } else {
            kOrt[k] = (a + b) * 0.5;
        }
    }
    std::vector<V> pNeu(pz);
    for (std::size_t p = 0; p < pz; ++p) {
        const V ort = wof::mi::ort(n, zi(p));
        pNeu[p] = ort;
        if (n.punkte[p].weg || punktKanten[p].empty()) { continue; }
        int rand = 0;
        int wild = 0;
        V randSumme;
        V mittenSumme;
        for (const int k : punktKanten[p]) {
            const PKante& ka = n.kanten[uz(k)];
            const V anderer = wof::mi::ort(n, ka.a == zi(p) ? ka.b : ka.a);
            mittenSumme = mittenSumme + (ort + anderer) * 0.5;
            if (ka.seiten.size() == 1U) {
                ++rand;
                randSumme = randSumme + anderer;
            } else if (ka.seiten.size() != 2U) {
                ++wild;
            }
        }
        if (wild > 0) { continue; }
        if (rand > 0) {
            // am offenen Rand: der Punkt folgt nur seinen zwei Nachbarn am Rand (die Randkurve haengt nicht vom Inneren ab) -
            // oder er bleibt; eine Ecke des Rands (eine oder mehr als zwei Randkanten) bleibt immer
            if (rand == 2 && !raenderHalten) { pNeu[p] = (randSumme + ort * 6.0) * 0.125; }
            continue;
        }
        const std::size_t zahl = punktKanten[p].size();
        if (zahl < 3U || p >= n.punktFlaechen.size() || n.punktFlaechen[p].size() != zahl) { continue; }
        V flaechenSumme;
        for (const int f : n.punktFlaechen[p]) { flaechenSumme = flaechenSumme + fOrt[uz(f)]; }
        const double durch = 1.0 / static_cast<double>(zahl);
        pNeu[p] = (flaechenSumme * durch + mittenSumme * (2.0 * durch) + ort * (static_cast<double>(zahl) - 3.0)) * durch;
    }
    // die neuen Punkte: je Kante und je Vieleck einer
    std::vector<int> kantenPunkt(kz, -1);
    for (std::size_t k = 0; k < kz; ++k) { kantenPunkt[k] = wof::mi::neuerPunkt(n, kOrt[k], n.punkte[uz(n.kanten[k].a)].teil); }
    std::vector<int> flaechenPunkt(fz, -1);
    for (std::size_t f = 0; f < fz; ++f) {
        if (!n.flaechen[f].weg && n.flaechen[f].ecken.size() >= 3U) { flaechenPunkt[f] = wof::mi::neuerPunkt(n, fOrt[f], n.flaechen[f].teil); }
    }
    for (std::size_t p = 0; p < pz; ++p) { wof::mi::vNach(pNeu[p], n.punkte[p].ort); }
    // je Ecke eines Vielecks ein Viereck: Ecke, Kantenpunkt danach, Flaechenpunkt, Kantenpunkt davor (der Umlaufsinn bleibt)
    for (std::size_t f = 0; f < fz; ++f) {
        if (flaechenPunkt[f] < 0) { continue; }
        const PFlaeche alt = n.flaechen[f];
        const std::size_t k = alt.ecken.size();
        std::vector<int> kp(k, -1);
        std::vector<Ecke> ke(k);
        std::vector<const Ecke*> alle;
        bool ganz = true;
        for (std::size_t i = 0; i < k; ++i) {
            const PEcke& a = alt.ecken[i];
            const PEcke& b = alt.ecken[(i + 1U) % k];
            const int kante = n.kanteVon(a.punkt, b.punkt);
            if (kante < 0 || uz(kante) >= kz) {
                ganz = false;
                break;
            }
            kp[i] = kantenPunkt[uz(kante)];
            ke[i] = mittel({&a.e, &b.e});
            alle.push_back(&a.e);
        }
        if (!ganz) { continue; }
        const Ecke mitte = mittel(alle);
        for (std::size_t i = 0; i < k; ++i) {
            const std::size_t davor = (i + k - 1U) % k;
            std::vector<PEcke> viereck;
            viereck.push_back(alt.ecken[i]);
            viereck.push_back(PEcke{kp[i], ke[i]});
            viereck.push_back(PEcke{flaechenPunkt[f], mitte});
            viereck.push_back(PEcke{kp[davor], ke[davor]});
            (void)wof::mi::neueFlaeche(n, std::move(viereck), alt.teil);
        }
        n.flaechen[f].weg = true;
    }
    n.kantenBauen();
}

// Ein Netz als Arbeitsnetz mit genau einer Flaeche rechnen und zurueckschreiben (wie jedes Werkzeug des Modellierens:
// Normalen der beruehrten Vielecke, verdichten, polyZurueck).
bool rechnen(const Teil& teil, const Netz& netz, const std::function<wof::Plan(PolyNetz&)>& f, Netz& aus, Ablehnung* grund) {
    Inhalt tmp;
    tmp.stufenZahl = 1;
    Teil t;
    t.name = teil.name;
    t.flaggen = teil.flaggen;
    t.stufen.resize(1);
    t.stufen[0] = netz;
    // 1a-2: waehrend des Malens gilt, was das Spiel aus der Malliste machte (wie anzeigeVertex)
    Netz& arbeit = t.stufen[0];
    for (std::size_t ei = 0; ei < arbeit.ecken.size() && ei < arbeit.mal.size(); ++ei) {
        Ecke spiel = arbeit.ecken[ei];
        if (aufEcke(arbeit.mal[ei], spiel)) { arbeit.ecken[ei] = spiel; }
    }
    arbeit.mal.clear();
    tmp.teile.push_back(std::move(t));
    PolyNetz n = wof::polyAus(tmp, 0);
    const wof::Plan p = f(n);
    if (p.abgelehnt()) {
        if (grund != nullptr) { *grund = p.grund; }
        return false;
    }
    wof::normalenNeu(n, kKnickGrad);
    n.verdichten();
    wof::polyZurueck(n, tmp);
    aus = std::move(tmp.teile[0].stufen[0]);
    return true;
}

}  // namespace

bool stapelWirkt(const Modifikator& m, StapelZweck z) { return m.an && (z == StapelZweck::Ansicht || m.datei); }

bool stapelWirkt(const Teil& t, StapelZweck z) {
    if (t.istTag()) { return false; }
    return std::any_of(t.stapel.begin(), t.stapel.end(), [z](const Modifikator& m) { return stapelWirkt(m, z); });
}

bool stapelWirkt(const Inhalt& in, StapelZweck z) {
    return std::any_of(in.teile.begin(), in.teile.end(), [z](const Teil& t) { return stapelWirkt(t, z); });
}

bool stapelLeer(const Inhalt& in) {
    return std::all_of(in.teile.begin(), in.teile.end(), [](const Teil& t) { return t.stapel.empty(); });
}

std::uint32_t stapelNeueKennung(const Inhalt& in) {
    std::uint32_t k = 0;
    for (const Teil& t : in.teile) {
        for (const Modifikator& m : t.stapel) { k = std::max(k, m.kennung); }
    }
    return k + 1U;
}

int stapelIndex(const Teil& t, std::uint32_t kennung) {
    for (std::size_t i = 0; i < t.stapel.size(); ++i) {
        if (t.stapel[i].kennung == kennung) { return zi(i); }
    }
    return -1;
}

bool modifikatorAnwenden(const Teil& teil, const Netz& netz, const Modifikator& m, const StapelUmgebung& u, Netz& aus, Ablehnung* grund) {
    if (grund != nullptr) { *grund = Ablehnung::Keine; }
    if (teil.istTag()) {
        if (grund != nullptr) { *grund = Ablehnung::Tag; }
        return false;
    }
    if (netz.ecken.empty() || netz.dreiecke() == 0) {
        if (grund != nullptr) { *grund = Ablehnung::LeereAuswahl; }
        return false;
    }
    switch (m.art) {
        case ModArt::Spiegeln:
            return rechnen(teil, netz, [&](PolyNetz& n) {
                wof::SymmetrieParam sp;
                sp.achse = std::clamp(static_cast<int>(m.achse), 0, 2);
                sp.vonPlus = m.vonPlus;
                sp.knochenSpiegel = u.knochenSpiegel;
                return wof::symmetrisieren(n, sp, {0});
            }, aus, grund);
        case ModArt::Glaetten:
            return rechnen(teil, netz, [&](PolyNetz& n) {
                const int stufen = std::clamp(static_cast<int>(m.stufen), 1, kGlaettenStufenMax);
                for (int i = 0; i < stufen; ++i) { glaettenSchritt(n, m.raenderHalten); }
                wof::Plan p;
                p.geaendert = n.lebendeFlaechen();
                return p;
            }, aus, grund);
    }
    return false;
}

Netz stapelNetz(const Teil& teil, int stufe, StapelZweck z, const StapelUmgebung& u, std::vector<Ablehnung>* status, std::vector<NetzZahlen>* nach) {
    if (status != nullptr) { status->assign(teil.stapel.size(), Ablehnung::Keine); }
    if (nach != nullptr) { nach->assign(teil.stapel.size(), NetzZahlen{}); }
    if (stufe < 0 || uz(stufe) >= teil.stufen.size()) { return Netz{}; }
    Netz jetzt = teil.stufen[uz(stufe)];
    for (std::size_t i = 0; i < teil.stapel.size(); ++i) {
        if (stapelWirkt(teil.stapel[i], z)) {
            Netz neu;
            Ablehnung g = Ablehnung::Keine;
            if (modifikatorAnwenden(teil, jetzt, teil.stapel[i], u, neu, &g)) {
                jetzt = std::move(neu);
            } else if (status != nullptr) {
                (*status)[i] = g;
            }
        }
        if (nach != nullptr) { (*nach)[i] = netzZahlen(jetzt); }
    }
    return jetzt;
}

Inhalt stapelFuerDatei(const Inhalt& in, const StapelUmgebung& u) {
    Inhalt aus = in;
    for (std::size_t t = 0; t < in.teile.size(); ++t) {
        if (stapelWirkt(in.teile[t], StapelZweck::Datei)) {
            for (std::size_t s = 0; s < in.teile[t].stufen.size(); ++s) { aus.teile[t].stufen[s] = stapelNetz(in.teile[t], zi(s), StapelZweck::Datei, u); }
        }
        aus.teile[t].stapel.clear();
    }
    return aus;
}

bool stapelAnwenden(Inhalt& in, int teil, const std::vector<int>& eintraege, const StapelUmgebung& u, Ablehnung* grund) {
    if (grund != nullptr) { *grund = Ablehnung::Keine; }
    if (teil < 0 || uz(teil) >= in.teile.size() || eintraege.empty()) { return false; }
    Teil& t = in.teile[uz(teil)];
    if (t.istTag()) {
        if (grund != nullptr) { *grund = Ablehnung::Tag; }
        return false;
    }
    for (std::size_t i = 0; i < eintraege.size(); ++i) {
        if (eintraege[i] < 0 || uz(eintraege[i]) >= t.stapel.size() || (i > 0 && eintraege[i] <= eintraege[i - 1U])) { return false; }
    }
    std::vector<Netz> neu(t.stufen.size());
    for (std::size_t s = 0; s < t.stufen.size(); ++s) {
        Netz jetzt = t.stufen[s];
        for (const int e : eintraege) {
            Netz n2;
            Ablehnung g = Ablehnung::Keine;
            if (modifikatorAnwenden(t, jetzt, t.stapel[uz(e)], u, n2, &g)) {
                jetzt = std::move(n2);
            } else if (s == 0U) {
                if (grund != nullptr) { *grund = g; }
                return false;
            }
        }
        neu[s] = std::move(jetzt);
    }
    t.stufen = std::move(neu);
    for (std::size_t i = eintraege.size(); i-- > 0U;) { t.stapel.erase(t.stapel.begin() + eintraege[i]); }
    ++in.netzStand;
    return true;
}

NetzZahlen netzZahlen(const Netz& n) { return NetzZahlen{zi(n.ecken.size()), n.dreiecke()}; }

bool netzGleich(const Netz& a, const Netz& b) {
    if (a.ecken.size() != b.ecken.size() || a.punktVon != b.punktVon || a.polyVon != b.polyVon || a.indizes() != b.indizes()) { return false; }
    for (std::size_t i = 0; i < a.ecken.size(); ++i) {
        const Ecke& x = a.ecken[i];
        const Ecke& y = b.ecken[i];
        if (std::memcmp(x.ort, y.ort, sizeof x.ort) != 0 || std::memcmp(x.normale, y.normale, sizeof x.normale) != 0 ||
            std::memcmp(x.uv, y.uv, sizeof x.uv) != 0 || std::memcmp(x.knochen, y.knochen, sizeof x.knochen) != 0 ||
            std::memcmp(x.gewicht, y.gewicht, sizeof x.gewicht) != 0) {
            return false;
        }
    }
    return true;
}

}  // namespace bhed::modell
