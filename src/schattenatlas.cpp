// schattenatlas.cpp - rc610 LICHT1: Plaetze der Schattenwuerfel (siehe bhed/schattenatlas.h).
//
//   Copyright (C) 2026 BehavEd-Remake contributors. GPL v2 (LICENSE).
#include "bhed/schattenatlas.h"

#include <algorithm>
#include <numeric>

namespace bhed {

namespace {

int stufe(int groesse) {
    for (std::size_t i = 0; i < kWuerfelGroessen.size(); ++i) {
        if (kWuerfelGroessen[i] == groesse) { return static_cast<int>(i); }
    }
    return -1;
}

}  // namespace

int wuerfelGroesseFuer(float huelle) {
    // von klein nach gross: die erste Stufe, deren Texel am Rand fein genug ist
    for (std::size_t i = kWuerfelGroessen.size(); i-- > 0;) {
        if (2.0F * huelle / static_cast<float>(kWuerfelGroessen[i]) <= kWuerfelTexel) { return kWuerfelGroessen[i]; }
    }
    return kWuerfelGroessen.front();
}

int wuerfelJeSeite(int groesse) { return groesse > 0 ? (kAtlasBreite / groesse) * (kAtlasHoehe / groesse) / 6 : 0; }

std::size_t wuerfelBytes(int groesse) { return static_cast<std::size_t>(groesse) * static_cast<std::size_t>(groesse) * 12U; }

AtlasPlatz SchattenAtlas::belegen(int groesse) {
    AtlasPlatz p;
    if (stufe(groesse) < 0) { return p; }
    int leer = -1;
    for (std::size_t s = 0; s < m_seiten.size(); ++s) {
        Seite& seite = m_seiten[s];
        if (seite.groesse == groesse && seite.frei > 0) {
            const auto it = std::find(seite.belegt.begin(), seite.belegt.end(), false);
            *it = true;
            --seite.frei;
            p.groesse = groesse;
            p.seite = static_cast<int>(s);
            p.wuerfel = static_cast<int>(it - seite.belegt.begin());
            return p;
        }
        if (seite.groesse == 0 && leer < 0) { leer = static_cast<int>(s); }
    }
    if (leer < 0) {
        if ((m_seiten.size() + 1U) * kAtlasSeitenBytes > m_budget) { return p; }
        leer = static_cast<int>(m_seiten.size());
        m_seiten.emplace_back();
    }
    Seite& seite = m_seiten[static_cast<std::size_t>(leer)];
    seite.groesse = groesse;
    seite.belegt.assign(static_cast<std::size_t>(wuerfelJeSeite(groesse)), false);
    seite.belegt[0] = true;
    seite.frei = wuerfelJeSeite(groesse) - 1;
    p.groesse = groesse;
    p.seite = leer;
    p.wuerfel = 0;
    return p;
}

void SchattenAtlas::freigeben(const AtlasPlatz& p) {
    if (!p.gueltig() || static_cast<std::size_t>(p.seite) >= m_seiten.size()) { return; }
    Seite& seite = m_seiten[static_cast<std::size_t>(p.seite)];
    if (seite.groesse != p.groesse || p.wuerfel < 0 || static_cast<std::size_t>(p.wuerfel) >= seite.belegt.size() ||
        !seite.belegt[static_cast<std::size_t>(p.wuerfel)]) {
        return;
    }
    seite.belegt[static_cast<std::size_t>(p.wuerfel)] = false;
    ++seite.frei;
    if (seite.frei == static_cast<int>(seite.belegt.size())) {   // ganz leer: fuer jede Groesse frei
        seite.groesse = 0;
        seite.belegt.clear();
        seite.frei = 0;
    }
}

void SchattenAtlas::leeren() { m_seiten.clear(); }

int SchattenAtlas::belegteSeiten() const {
    int n = 0;
    for (const Seite& s : m_seiten) { n += s.groesse != 0 ? 1 : 0; }
    return n;
}

void SchattenAtlas::kuerzen() {
    while (!m_seiten.empty() && m_seiten.back().groesse == 0) { m_seiten.pop_back(); }
}

std::size_t SchattenAtlas::belegt() const {
    std::size_t n = 0;
    for (const Seite& s : m_seiten) { n += s.belegt.size() - static_cast<std::size_t>(s.frei); }
    return n;
}

std::size_t atlasSeitenFuer(const std::array<std::size_t, 4>& anzahl) {
    std::size_t seiten = 0;
    for (std::size_t i = 0; i < kWuerfelGroessen.size(); ++i) {
        const std::size_t je = static_cast<std::size_t>(wuerfelJeSeite(kWuerfelGroessen[i]));
        seiten += (anzahl[i] + je - 1U) / je;
    }
    return seiten;
}

std::vector<int> wuerfelGroessenVerteilen(const std::vector<float>& huellen, std::size_t budgetBytes) {
    const std::size_t n = huellen.size();
    std::vector<int> st(n);   // Stufe je Licht (Index in kWuerfelGroessen), 4: kein Wuerfel
    std::array<std::size_t, 4> anzahl{};
    for (std::size_t i = 0; i < n; ++i) {
        st[i] = stufe(wuerfelGroesseFuer(huellen[i]));
        ++anzahl[static_cast<std::size_t>(st[i])];
    }
    const std::size_t seitenMax = budgetBytes / kAtlasSeitenBytes;
    // die kleinsten Huellen zuerst (bei gleicher Huelle die spaeteren)
    std::vector<std::size_t> reihe(n);
    std::iota(reihe.begin(), reihe.end(), std::size_t{0});
    std::stable_sort(reihe.begin(), reihe.end(), [&huellen](std::size_t a, std::size_t b) { return huellen[a] < huellen[b]; });
    bool kleiner = true;
    while (atlasSeitenFuer(anzahl) > seitenMax && kleiner) {
        kleiner = false;
        for (const std::size_t i : reihe) {
            if (st[i] >= 3) { continue; }
            --anzahl[static_cast<std::size_t>(st[i])];
            ++st[i];
            ++anzahl[static_cast<std::size_t>(st[i])];
            kleiner = true;
            if (atlasSeitenFuer(anzahl) <= seitenMax) { break; }
        }
    }
    // auch die kleinste Stufe reicht nicht: die kleinsten Huellen ohne Wuerfel
    for (const std::size_t i : reihe) {
        if (atlasSeitenFuer(anzahl) <= seitenMax) { break; }
        if (st[i] > 3) { continue; }
        --anzahl[static_cast<std::size_t>(st[i])];
        st[i] = 4;
    }
    std::vector<int> r(n);
    for (std::size_t i = 0; i < n; ++i) { r[i] = st[i] < 4 ? kWuerfelGroessen[static_cast<std::size_t>(st[i])] : 0; }
    return r;
}

}  // namespace bhed
