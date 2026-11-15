// modell/detailstufen.cpp - Detailstufen erzeugen, zaehlen und pruefen (CW-MODELL, 15.11.2026) - siehe bhed/modell/detailstufen.h
//
// Der Vereinfacher ist meshoptimizer (third_party/meshoptimizer, MIT, Copyright (c) 2016-2026 Arseny Kapoulkine, v1.3):
// meshopt_simplifyWithAttributes - Kantenkollaps nach Quadriken mit Attributen auf vorhandene Ecken. Hier steht nur, was JKA dazu
// braucht: welche Flaechen bleiben, welche Attribute zaehlen, das Ziel in Ecken (der Vereinfacher zaehlt Dreiecke), das
// Zusammenschieben der Ecken in ihrer Reihenfolge und die Grenzen des Spiels.
#include "bhed/modell/detailstufen.h"

#include <meshoptimizer.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <map>
#include <set>
#include <utility>

#include "bhed/modell/g2.h"

namespace bhed::modell {
namespace {

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

std::size_t uz(int i) { return static_cast<std::size_t>(i); }

// Gezeichnet ohne Haut: kein Tag, keine Flaeche mit G2SURFACEFLAG_OFF bzw. "_off" (RenderSurfaces, offFlags = Flaggen der Datei).
bool zeichnetAbWerk(const Teil& t) {
    if (t.istTag() || (!t.name.empty() && t.name[0] == '*')) { return false; }
    if ((t.flaggen & kG2Aus) != 0U) { return false; }
    const std::string n = klein(t.name);
    return !(n.size() > 4 && n.compare(n.size() - 4, 4, "_off") == 0);
}

struct Lauf {
    std::vector<unsigned int> indizes;
    int ecken = 0;
    float fehler = 0.0F;
};

}  // namespace

StufenRolle stufenRolle(const Teil& t) {
    if (t.istTag() || (!t.name.empty() && t.name[0] == '*')) { return StufenRolle::Tag; }
    const std::string n = klein(t.name);
    if (n.size() > 4 && n.compare(n.size() - 4, 4, "_off") == 0) { return StufenRolle::Aus; }
    if (n.find("_cap_") != std::string::npos) { return StufenRolle::Kappe; }
    if (t.stufen.empty() || t.stufen[0].dreiecke() == 0) { return StufenRolle::Leer; }
    return StufenRolle::Vereinfachen;
}

ModellArt modellArtVon(const Inhalt& in, const std::string& pfad) {
    std::string p = klein(pfad);
    for (char& c : p) {
        if (c == '\\') { c = '/'; }
    }
    const std::string gla = klein(in.glaPfad);
    if (p.find("models/players/") != std::string::npos) { return ModellArt::Figur; }
    if (p.find("models/weapons") != std::string::npos || p.find("models/items/") != std::string::npos) { return ModellArt::Ausruestung; }
    if (in.herkunft == Herkunft::Glm) {
        if (gla.empty() || gla == "*default" || in.knochenZahl <= 1) { return ModellArt::Ausruestung; }
        return ModellArt::Figur;
    }
    return ModellArt::Requisit;
}

StufenVorgabe stufenVorgabe(ModellArt a, int stufen) {
    StufenVorgabe v{};
    switch (a) {
        case ModellArt::Figur:
            v = stufen >= 4 ? StufenVorgabe{{0.74F, 0.43F, 0.28F}} : StufenVorgabe{{0.65F, 0.40F, 0.28F}};
            break;
        case ModellArt::Ausruestung: v = StufenVorgabe{{0.55F, 0.30F, 0.20F}}; break;
        case ModellArt::Requisit: v = StufenVorgabe{{0.75F, 0.50F, 0.30F}}; break;
    }
    return v;
}

int cwRichtwert(ModellArt a) noexcept {
    switch (a) {
        case ModellArt::Figur: return 500;
        case ModellArt::Ausruestung: return 300;
        case ModellArt::Requisit: break;
    }
    return 0;
}

StufenZahlen stufenZahlen(const Inhalt& in, int stufe) {
    StufenZahlen z;
    for (const Teil& t : in.teile) {
        if (t.istTag() || (!t.name.empty() && t.name[0] == '*')) { continue; }
        if (stufe < 0 || uz(stufe) >= t.stufen.size()) { continue; }
        const Netz& n = t.stufen[uz(stufe)];
        ++z.flaechen;
        const int e = static_cast<int>(n.ecken.size());
        if (e > z.groesste) {
            z.groesste = e;
            z.groessteName = t.name;
        }
        if (!zeichnetAbWerk(t)) { continue; }
        z.ecken += e;
        z.dreiecke += n.dreiecke();
    }
    return z;
}

StufenZahlen stufenZahlen(const Md3Model& m) {
    StufenZahlen z;
    for (const Md3Surface& s : m.surfaces) {
        ++z.flaechen;
        z.ecken += s.numVerts;
        z.dreiecke += static_cast<int>(s.indexes.size() / 3U);
        if (s.numVerts > z.groesste) {
            z.groesste = s.numVerts;
            z.groessteName = s.name;
        }
    }
    return z;
}

namespace {

using Ort3 = std::array<float, 3>;

// Abstand eines Punkts zu einem Dreieck (C. Ericson, "Real-Time Collision Detection", 5.1.5 ClosestPtPointTriangle)
double abstandDreieck(const float* p, const float* a, const float* b, const float* c) {
    const auto sub = [](const float* x, const float* y) { return std::array<double, 3>{x[0] - static_cast<double>(y[0]), x[1] - static_cast<double>(y[1]), x[2] - static_cast<double>(y[2])}; };
    const auto dot = [](const std::array<double, 3>& x, const std::array<double, 3>& y) { return x[0] * y[0] + x[1] * y[1] + x[2] * y[2]; };
    const auto ab = sub(b, a);
    const auto ac = sub(c, a);
    const auto ap = sub(p, a);
    std::array<double, 3> q{};
    const double d1 = dot(ab, ap);
    const double d2 = dot(ac, ap);
    const auto bei = [&](double u, double v) {
        for (std::size_t k = 0; k < 3; ++k) { q[k] = a[k] + u * ab[k] + v * ac[k]; }
    };
    if (d1 <= 0.0 && d2 <= 0.0) {
        bei(0, 0);
    } else {
        const auto bp = sub(p, b);
        const double d3 = dot(ab, bp);
        const double d4 = dot(ac, bp);
        const auto cp = sub(p, c);
        const double d5 = dot(ab, cp);
        const double d6 = dot(ac, cp);
        const double vc = d1 * d4 - d3 * d2;
        const double vb = d5 * d2 - d1 * d6;
        const double va = d3 * d6 - d5 * d4;
        if (d3 >= 0.0 && d4 <= d3) {
            bei(1, 0);
        } else if (vc <= 0.0 && d1 >= 0.0 && d3 <= 0.0) {
            bei(d1 / (d1 - d3), 0);
        } else if (d6 >= 0.0 && d5 <= d6) {
            bei(0, 1);
        } else if (vb <= 0.0 && d2 >= 0.0 && d6 <= 0.0) {
            bei(0, d2 / (d2 - d6));
        } else if (va <= 0.0 && (d4 - d3) >= 0.0 && (d5 - d6) >= 0.0) {
            const double w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
            bei(1.0 - w, w);
        } else {
            const double den = 1.0 / (va + vb + vc);
            bei(vb * den, vc * den);
        }
    }
    const double x = p[0] - q[0];
    const double y = p[1] - q[1];
    const double z = p[2] - q[2];
    return std::sqrt(x * x + y * y + z * z);
}

double abweichung(const Netz& quelle, const Netz& ergebnis) { return netzAbweichung(quelle, ergebnis); }

// Mehrere Flaechen GEMEINSAM vereinfachen: alle Ecken hintereinander in einem Puffer. Ecken verschiedener Flaechen am selben Ort
// sind fuer meshoptimizer die Seiten einer Naht (buildPositionRemap, "wedge") - eine Kante an der Grenze zweier Flaechen kollabiert
// in beiden zugleich, ihre Raender bleiben aufeinander. Die Attribute (Normale, UV, Knochen) vergleicht der Vereinfacher nur
// innerhalb der Dreiecke einer Flaeche, deshalb darf jede Flaeche ihre UV eigens skalieren und ihre Knochen eigens auf die 27
// Spalten legen. `sperre`: je Ecke (aller Flaechen hintereinander) 1 = bleibt (Orte, die eine unberuehrte Flaeche mitbenutzt).
std::vector<Netz> gemeinsamVereinfachen(const std::vector<const Netz*>& netze, const std::vector<unsigned char>& sperre, int zielEcken,
                                        const VereinfachOptionen& o, int* erreicht) {
    std::vector<std::size_t> anfang;
    std::size_t ne = 0;
    std::size_t ni = 0;
    for (const Netz* n : netze) {
        anfang.push_back(ne);
        ne += n->ecken.size();
        ni += n->kanten.size();
    }
    anfang.push_back(ne);
    std::vector<unsigned int> idx;
    idx.reserve(ni);
    std::vector<float> orte(ne * 3U);
    constexpr std::size_t kKnochenMax = 32U - 5U;
    const std::size_t na = 5U + kKnochenMax;
    std::vector<float> attr(ne * na, 0.0F);
    bool knochenZaehlen = false;
    for (std::size_t f = 0; f < netze.size(); ++f) {
        const Netz& n = *netze[f];
        const std::vector<std::uint32_t> ix = n.indizes();
        for (const std::uint32_t x : ix) { idx.push_back(static_cast<unsigned int>(anfang[f] + x)); }
        // Knochen der Flaeche nach Summe der Gewichte (die schwersten zuerst, bei Gleichstand der kleinere Index)
        std::map<int, double> summe;
        for (const Ecke& e : n.ecken) {
            for (std::size_t k = 0; k < 4; ++k) {
                if (e.gewicht[k] > 0.0F) { summe[e.knochen[k]] += e.gewicht[k]; }
            }
        }
        std::vector<std::pair<int, double>> knochen(summe.begin(), summe.end());
        std::stable_sort(knochen.begin(), knochen.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
        if (knochen.size() > kKnochenMax) { knochen.resize(kKnochenMax); }
        knochenZaehlen = knochenZaehlen || knochen.size() > 1U;
        std::map<int, std::size_t> spalte;
        for (std::size_t k = 0; k < knochen.size(); ++k) { spalte[knochen[k].first] = k; }
        // UV-Skala nach meshoptimizer README ("Attributes"): Kehrwert der mittleren UV-Dichte, 1 / sqrt(mittlere UV-Flaeche)
        double uvFlaeche = 0.0;
        for (std::size_t t = 0; t + 2 < ix.size(); t += 3) {
            const float* a = n.ecken[ix[t]].uv;
            const float* b = n.ecken[ix[t + 1]].uv;
            const float* c = n.ecken[ix[t + 2]].uv;
            uvFlaeche += std::abs(static_cast<double>((b[0] - a[0]) * (c[1] - a[1]) - (c[0] - a[0]) * (b[1] - a[1]))) * 0.5;
        }
        const double mittel = ix.empty() ? 0.0 : uvFlaeche / static_cast<double>(ix.size() / 3U);
        const float uvSkala = mittel > 1e-12 ? std::min(static_cast<float>(1.0 / std::sqrt(mittel)), 1000.0F) : 1.0F;
        for (std::size_t i = 0; i < n.ecken.size(); ++i) {
            const Ecke& e = n.ecken[i];
            const std::size_t g = anfang[f] + i;
            for (std::size_t k = 0; k < 3; ++k) { orte[g * 3U + k] = e.ort[k]; }
            float* a = attr.data() + g * na;
            a[0] = e.normale[0];
            a[1] = e.normale[1];
            a[2] = e.normale[2];
            a[3] = e.uv[0] * uvSkala;
            a[4] = e.uv[1] * uvSkala;
            for (std::size_t k = 0; k < 4; ++k) {
                const auto s = spalte.find(e.knochen[k]);
                if (e.gewicht[k] > 0.0F && s != spalte.end()) { a[5U + s->second] += e.gewicht[k]; }
            }
        }
    }
    std::vector<float> gewichte(na, knochenZaehlen ? o.gewichtKnochen : 0.0F);
    gewichte[0] = gewichte[1] = gewichte[2] = o.gewichtNormale;
    gewichte[3] = gewichte[4] = o.gewichtUv;
    // ein starres Modell (.md3, Knochen 0) traegt keine Knochenspalten: nur Normale und UV gehen in den Fehler ein
    const std::size_t attribute = knochenZaehlen && o.gewichtKnochen > 0.0F ? na : 5U;
    std::vector<float> attrKurz;
    const float* attrDaten = attr.data();
    if (attribute != na) {
        attrKurz.resize(ne * attribute);
        for (std::size_t i = 0; i < ne; ++i) {
            for (std::size_t k = 0; k < attribute; ++k) { attrKurz[i * attribute + k] = attr[i * na + k]; }
        }
        attrDaten = attrKurz.data();
    }
    const unsigned int optionen = o.raenderHalten ? static_cast<unsigned int>(meshopt_SimplifyLockBorder) : 0U;
    const bool mitSperre = std::any_of(sperre.begin(), sperre.end(), [](unsigned char x) { return x != 0; });
    std::vector<unsigned char> sperrFlaggen(ne, 0);
    for (std::size_t i = 0; i < ne && i < sperre.size(); ++i) { sperrFlaggen[i] = sperre[i] != 0 ? static_cast<unsigned char>(meshopt_SimplifyVertex_Lock) : 0; }

    std::vector<unsigned int> ziel(idx.size());
    std::vector<char> benutzt(ne);
    const auto laufen = [&](std::size_t zielIndizes) {
        Lauf l;
        float f = 0.0F;
        const std::size_t m = meshopt_simplifyWithAttributes(ziel.data(), idx.data(), idx.size(), orte.data(), ne, sizeof(float) * 3U, attrDaten,
                                                             sizeof(float) * attribute, gewichte.data(), attribute,
                                                             mitSperre ? sperrFlaggen.data() : nullptr, zielIndizes, FLT_MAX, optionen, &f);
        l.indizes.assign(ziel.begin(), ziel.begin() + static_cast<std::ptrdiff_t>(m));
        std::fill(benutzt.begin(), benutzt.end(), 0);
        for (const unsigned int x : l.indizes) { benutzt[x] = 1; }
        l.ecken = static_cast<int>(std::count(benutzt.begin(), benutzt.end(), 1));
        l.fehler = f;
        return l;
    };
    // Der Vereinfacher zielt auf Dreiecke; gesucht ist das groesste Dreiecksziel, dessen Ergebnis hoechstens zielEcken Ecken hat
    // (binaere Suche - die Zahl der Ecken waechst mit dem Ziel; das Ergebnis ist das beste gefundene).
    Lauf best = laufen(0);   // so weit, wie Raender, Naehte und gesperrte Ecken es erlauben
    if (best.ecken <= zielEcken) {
        std::size_t lo = 0;
        std::size_t hi = idx.size() / 3U;
        while (lo + 1 < hi) {
            const std::size_t mitte = (lo + hi) / 2U;
            Lauf l = laufen(mitte * 3U);
            if (l.ecken <= zielEcken) {
                lo = mitte;
                if (l.ecken > best.ecken || (l.ecken == best.ecken && l.indizes.size() > best.indizes.size())) { best = std::move(l); }
            } else {
                hi = mitte;
            }
        }
    }
    if (erreicht != nullptr) { *erreicht = best.ecken; }
    // je Flaeche ihre Dreiecke (jede Ecke eines Dreiecks gehoert zu seiner Flaeche: der Vereinfacher legt Seiten einer Naht nur auf
    // Seiten derselben Flaeche) und ihre Ecken in der Reihenfolge der Quelle
    std::fill(benutzt.begin(), benutzt.end(), 0);
    for (const unsigned int x : best.indizes) { benutzt[x] = 1; }
    std::vector<int> neu(ne, -1);
    std::vector<Netz> aus(netze.size());
    std::vector<std::vector<Ecke>> ecken(netze.size());
    for (std::size_t f = 0; f < netze.size(); ++f) {
        for (std::size_t g = anfang[f]; g < anfang[f + 1]; ++g) {
            if (benutzt[g] == 0) { continue; }
            neu[g] = static_cast<int>(ecken[f].size());
            ecken[f].push_back(netze[f]->ecken[g - anfang[f]]);
        }
    }
    std::vector<std::vector<std::uint32_t>> dr(netze.size());
    for (std::size_t t = 0; t + 2 < best.indizes.size(); t += 3) {
        const unsigned int v = best.indizes[t];
        const std::size_t f = static_cast<std::size_t>(std::upper_bound(anfang.begin(), anfang.end(), static_cast<std::size_t>(v)) - anfang.begin()) - 1U;
        for (std::size_t k = 0; k < 3; ++k) {
            const unsigned int x = best.indizes[t + k];
            if (x < anfang[f] || x >= anfang[f + 1]) { return {}; }   // darf nicht sein: dann vereinfacht der Aufrufer je Flaeche
            dr[f].push_back(static_cast<std::uint32_t>(neu[x]));
        }
    }
    for (std::size_t f = 0; f < netze.size(); ++f) {
        aus[f].aufbauen(std::move(ecken[f]), dr[f]);
        // die Felder der Datei (Knochenverweise der Flaeche): jede Ecke ist eine Ecke der Quelle, ihre Gewichtsfelder passen dazu
        aus[f].dateiRefs = netze[f]->dateiRefs;
        aus[f].dateiRefsAufgeloest = netze[f]->dateiRefsAufgeloest;
        aus[f].ausDatei = netze[f]->ausDatei;
    }
    return aus;
}

}  // namespace

double netzAbweichung(const Netz& quelle, const Netz& ergebnis, double* mittel) {
    const std::vector<std::uint32_t> idx = ergebnis.indizes();
    if (mittel != nullptr) { *mittel = 0.0; }
    if (idx.empty() || quelle.ecken.empty()) { return 0.0; }
    double groesste = 0.0;
    double summe = 0.0;
    for (const Ecke& e : quelle.ecken) {
        double best = 1e30;
        for (std::size_t t = 0; t + 2 < idx.size() && best > 1e-9; t += 3) {
            best = std::min(best, abstandDreieck(e.ort, ergebnis.ecken[idx[t]].ort, ergebnis.ecken[idx[t + 1]].ort, ergebnis.ecken[idx[t + 2]].ort));
        }
        groesste = std::max(groesste, best);
        summe += best;
    }
    if (mittel != nullptr) { *mittel = summe / static_cast<double>(quelle.ecken.size()); }
    return groesste;
}

Netz netzVereinfachen(const Netz& n, const VereinfachOptionen& o, float* fehler) {
    if (fehler != nullptr) { *fehler = 0.0F; }
    if (n.ecken.empty() || n.kanten.size() < 3U || o.zielEcken >= static_cast<int>(n.ecken.size())) { return n; }
    std::vector<Netz> r = gemeinsamVereinfachen({&n}, {}, o.zielEcken, o, nullptr);
    if (r.size() != 1U) { return n; }
    if (fehler != nullptr) { *fehler = static_cast<float>(abweichung(n, r[0])); }
    return std::move(r[0]);
}

bool detailstufenErzeugen(Inhalt& in, const DetailOptionen& o, DetailBericht* bericht) {
    if (in.teile.empty()) { return false; }
    const int hoechstens = in.herkunft == Herkunft::Md3 ? kMd3StufenMax : kGlmStufenMax;
    const int stufen = std::clamp(o.stufen, 2, hoechstens);
    DetailBericht b;
    for (int s = 0; s < std::max(in.stufenZahl, 1); ++s) { b.vorher.push_back(stufenZahlen(in, s)); }
    // Stufe 0 jeder Flaeche; was vereinfacht wird; die Orte der unberuehrten Flaechen (Tags, _off, _cap_ bleiben, wie sie sind -
    // eine Kappe schliesst den Rand ihres Nachbarn nur, wenn der dort bleibt)
    std::vector<Netz> basis(in.teile.size());
    std::vector<std::size_t> wer;
    std::set<Ort3> festOrte;
    int festGezeichnet = 0;
    int n0 = 0;
    for (std::size_t ti = 0; ti < in.teile.size(); ++ti) {
        Teil& t = in.teile[ti];
        if (t.stufen.empty()) { t.stufen.resize(1); }
        basis[ti] = t.stufen[0];
        const StufenRolle r = stufenRolle(t);
        if (r == StufenRolle::Vereinfachen) {
            wer.push_back(ti);
            n0 += static_cast<int>(basis[ti].ecken.size());
            continue;
        }
        if (r != StufenRolle::Leer) { ++b.unberuehrt; }
        if (zeichnetAbWerk(t)) { festGezeichnet += static_cast<int>(basis[ti].ecken.size()); }
        for (const Ecke& e : basis[ti].ecken) { festOrte.insert(Ort3{e.ort[0], e.ort[1], e.ort[2]}); }
    }
    std::vector<const Netz*> quellen;
    std::vector<unsigned char> sperre;
    for (const std::size_t ti : wer) {
        quellen.push_back(&basis[ti]);
        for (const Ecke& e : basis[ti].ecken) { sperre.push_back(festOrte.count(Ort3{e.ort[0], e.ort[1], e.ort[2]}) != 0U ? 1 : 0); }
    }
    VereinfachOptionen v;
    v.gewichtNormale = o.gewichtNormale;
    v.gewichtUv = o.gewichtUv;
    v.gewichtKnochen = o.gewichtKnochen;
    v.raenderHalten = o.raenderHalten;
    // je Stufe die neuen Netze der vereinfachten Flaechen (in der Reihenfolge von `wer`)
    std::vector<std::vector<Netz>> neu(static_cast<std::size_t>(stufen));
    for (int s = 1; s < stufen; ++s) {
        const float anteil = std::clamp(o.anteil[uz(s - 1)], 0.01F, 1.0F);
        int ziel = static_cast<int>(std::lround(static_cast<double>(anteil) * n0));
        const bool letzte = s == stufen - 1;
        if (letzte && o.richtwertLetzte > 0) { ziel = std::min(ziel, std::max(o.richtwertLetzte - festGezeichnet, 0)); }
        ziel = std::max(ziel, 3);
        int erreicht = 0;
        std::vector<Netz> r = quellen.empty() ? std::vector<Netz>{} : gemeinsamVereinfachen(quellen, sperre, ziel, v, &erreicht);
        if (r.size() != quellen.size()) {
            // Rueckfall (sollte nicht vorkommen): jede Flaeche fuer sich, Raender fest
            r.clear();
            erreicht = 0;
            for (const Netz* q : quellen) {
                VereinfachOptionen w = v;
                w.zielEcken = std::max(3, static_cast<int>(std::lround(static_cast<double>(anteil) * static_cast<double>(q->ecken.size()))));
                r.push_back(netzVereinfachen(*q, w));
                erreicht += static_cast<int>(r.back().ecken.size());
            }
        }
        if (erreicht > ziel) { b.verfehlt.push_back(StufenRest{-1, s, erreicht, ziel}); }
        // Grenzen je Flaeche: SHADER_MAX_VERTEXES, in der letzten Stufe einer .glm die Schattengrenze - eine Flaeche darueber wird fuer
        // sich weiter vereinfacht, ihr Rand bleibt (er liegt auf dem ihrer Nachbarn)
        for (std::size_t k = 0; k < r.size(); ++k) {
            int grenze = o.eckenGrenze > 0 ? o.eckenGrenze - 1 : 0;
            if (letzte && o.schattenGrenze > 0 && in.herkunft == Herkunft::Glm) { grenze = grenze > 0 ? std::min(grenze, o.schattenGrenze - 1) : o.schattenGrenze - 1; }
            if (grenze > 0 && static_cast<int>(r[k].ecken.size()) > grenze) {
                VereinfachOptionen w = v;
                w.zielEcken = grenze;
                w.raenderHalten = true;
                r[k] = netzVereinfachen(r[k], w);
                if (static_cast<int>(r[k].ecken.size()) > grenze) {
                    b.verfehlt.push_back(StufenRest{static_cast<int>(wer[k]), s, static_cast<int>(r[k].ecken.size()), grenze});
                }
            }
        }
        neu[uz(s)] = std::move(r);
    }
    for (std::size_t ti = 0; ti < in.teile.size(); ++ti) {
        Teil& t = in.teile[ti];
        t.stufen.assign(1, basis[ti]);
        const auto it = std::find(wer.begin(), wer.end(), ti);
        for (int s = 1; s < stufen; ++s) {
            if (it == wer.end()) {
                t.stufen.push_back(basis[ti]);
            } else {
                Netz& n = neu[uz(s)][static_cast<std::size_t>(it - wer.begin())];
                b.abweichung = std::max(b.abweichung, static_cast<float>(abweichung(basis[ti], n)));
                t.stufen.push_back(std::move(n));
            }
        }
    }
    in.stufenZahl = stufen;
    ++in.netzStand;
    for (int s = 0; s < stufen; ++s) { b.nachher.push_back(stufenZahlen(in, s)); }
    if (bericht != nullptr) { *bericht = std::move(b); }
    return true;
}

void detailstufenEntfernen(Inhalt& in) {
    for (Teil& t : in.teile) {
        if (t.stufen.size() > 1U) { t.stufen.resize(1); }
    }
    in.stufenZahl = 1;
    ++in.netzStand;
}

std::string md3StufenPfad(const std::string& pfad, int stufe) {
    std::string p = pfad;
    const std::size_t punkt = p.rfind('.');
    if (punkt != std::string::npos) { p.erase(punkt); }
    return p + "_" + std::to_string(stufe) + ".md3";
}

bool md3StufeSchreiben(const Inhalt& in, int stufe, std::string& aus, int eckenGrenze, Md3SchreibBericht* bericht) {
    Inhalt x;
    x.herkunft = in.herkunft;
    x.name = in.name;
    x.md3Datei = in.md3Datei;
    x.stufenZahl = 1;
    x.teile.reserve(in.teile.size());
    for (const Teil& t : in.teile) {
        Teil k;
        k.name = t.name;
        k.shader = t.shader;
        k.flaggen = t.flaggen;
        k.eltern = t.eltern;
        k.kinder = t.kinder;
        k.dateiFlaeche = t.dateiFlaeche;
        if (stufe >= 0 && uz(stufe) < t.stufen.size()) {
            k.stufen.push_back(t.stufen[uz(stufe)]);
        } else if (!t.stufen.empty()) {
            k.stufen.push_back(t.stufen[0]);
        }
        x.teile.push_back(std::move(k));
    }
    return md3Schreiben(x, aus, eckenGrenze, bericht);
}

std::vector<StufenBefund> stufenPruefen(const Inhalt& in, int eckenGrenze, int schattenGrenze, int richtwert) {
    std::vector<StufenBefund> out;
    const int stufen = std::max(in.stufenZahl, 1);
    const bool md3 = in.herkunft == Herkunft::Md3;
    for (int s = 0; s < stufen; ++s) {
        int flaechen = 0;
        for (const Teil& t : in.teile) {
            if (t.istTag() || uz(s) >= t.stufen.size()) { continue; }
            ++flaechen;
            const int e = static_cast<int>(t.stufen[uz(s)].ecken.size());
            if (eckenGrenze > 0 && e >= eckenGrenze) { out.push_back({StufenBefundArt::Ecken, s, e, eckenGrenze - 1, t.name}); }
            if (md3 && e > kMd3MaxEcken) { out.push_back({StufenBefundArt::Md3Ecken, s, e, kMd3MaxEcken, t.name}); }
            if (!md3 && stufen > 1 && s == stufen - 1 && schattenGrenze > 0 && e >= schattenGrenze && zeichnetAbWerk(t)) {
                out.push_back({StufenBefundArt::Schatten, s, e, schattenGrenze - 1, t.name});
            }
        }
        if (md3 && flaechen > kMd3MaxFlaechen) { out.push_back({StufenBefundArt::Md3Flaechen, s, flaechen, kMd3MaxFlaechen, {}}); }
    }
    if (richtwert > 0) {
        const StufenZahlen z = stufenZahlen(in, stufen - 1);
        if (stufen < 2) {
            out.push_back({StufenBefundArt::KeineStufen, 0, stufen, 3, {}});
        }
        if (z.ecken > richtwert) { out.push_back({StufenBefundArt::Richtwert, stufen - 1, z.ecken, richtwert, {}}); }
    }
    return out;
}

std::vector<StufenBefund> stufenPruefen(const std::vector<Md3Model>& stufen, int eckenGrenze, int richtwert) {
    std::vector<StufenBefund> out;
    for (std::size_t s = 0; s < stufen.size(); ++s) {
        const int si = static_cast<int>(s);
        for (const Md3Surface& f : stufen[s].surfaces) {
            if (eckenGrenze > 0 && f.numVerts >= eckenGrenze) { out.push_back({StufenBefundArt::Ecken, si, f.numVerts, eckenGrenze - 1, f.name}); }
            if (f.numVerts > kMd3MaxEcken) { out.push_back({StufenBefundArt::Md3Ecken, si, f.numVerts, kMd3MaxEcken, f.name}); }
        }
        if (static_cast<int>(stufen[s].surfaces.size()) > kMd3MaxFlaechen) {
            out.push_back({StufenBefundArt::Md3Flaechen, si, static_cast<int>(stufen[s].surfaces.size()), kMd3MaxFlaechen, {}});
        }
    }
    if (richtwert > 0 && !stufen.empty()) {
        if (stufen.size() < 2U) { out.push_back({StufenBefundArt::KeineStufen, 0, static_cast<int>(stufen.size()), 3, {}}); }
        const StufenZahlen z = stufenZahlen(stufen.back());
        if (z.ecken > richtwert) { out.push_back({StufenBefundArt::Richtwert, static_cast<int>(stufen.size()) - 1, z.ecken, richtwert, {}}); }
    }
    return out;
}

}  // namespace bhed::modell
