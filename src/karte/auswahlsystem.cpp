// auswahlsystem.cpp - Karten-Editor: Auswaehlen in den Ansichten.
// Portiert aus NetRadiant-Custom 20260114 (Quellen in auswahlsystem.h).
//
//   Copyright (C) 2001-2006, William Joseph.
//   All Rights Reserved.
//   This file is part of GtkRadiant.
//
//   GtkRadiant is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. GtkRadiant is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#include "bhed/karte/auswahlsystem.h"

#include "bhed/karte/eklasse.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/komponenten.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/pinsel.h"

#include <cmath>
#include <cstdio>
#include <iterator>

namespace bhed::karte {

namespace {

constexpr float kWaehlTiefe = 1e-6F;        // SELECT_MATCHING_DEPTH
constexpr float kWaehlAbstand = 1e-6F;      // SELECT_MATCHING_DIST
constexpr float kWaehlKomponenten = .25F;   // SELECT_MATCHING_COMPONENTS_DIST

bool istLicht(const Entitaet& e) { return e.klassenname() == "light" || e.klassenname() == "lightJunior"; }

// Der lokale Kasten einer Punktentitaet (read_aabb / m_aabb_light).
Kasten lokalerKasten(const Entitaet& e) {
    const EntitaetsKlasse* kl = e.klasse();
    if (kl != nullptr && kl->fixedsize) { return kastenAusMinMax(kl->mins, kl->maxs); }
    return Kasten(Vektor3f(0, 0, 0), Vektor3f(8, 8, 8));   // default_extents
}

// aabb_corners
std::array<Vektor3f, 8> ecken(const Kasten& k) {
    const Vektor3f mn = k.origin - k.extents;
    const Vektor3f mx = k.origin + k.extents;
    return {Vektor3f(mn[0], mx[1], mx[2]), Vektor3f(mx[0], mx[1], mx[2]), Vektor3f(mx[0], mn[1], mx[2]),
            Vektor3f(mn[0], mn[1], mx[2]), Vektor3f(mn[0], mx[1], mn[2]), Vektor3f(mx[0], mx[1], mn[2]),
            Vektor3f(mx[0], mn[1], mn[2]), Vektor3f(mn[0], mn[1], mn[2])};
}

// Punktentitaet: GenericEntity::testSelect (aabb_testselect) / Light (light_testselect)
void entitaetTesten(const Entitaet& e, AuswahlVolumen& test, Treffer& bester) {
    test.netzBeginnen(verschiebungsMatrix(entitaetUrsprung(e)));
    const Kasten k = lokalerKasten(e);
    if (istLicht(e)) {
        static const unsigned int idx[24] = {0, 2, 3, 0, 3, 4, 0, 4, 5, 0, 5, 2, 1, 2, 5, 1, 5, 4, 1, 4, 3, 1, 3, 2};
        // light_vertices
        const Vektor3f mx = k.origin + k.extents;
        const Vektor3f mn = k.origin - k.extents;
        const Vektor3f& m = k.origin;
        const Vektor3f p[6] = {Vektor3f(m[0], m[1], mx[2]), Vektor3f(m[0], m[1], mn[2]), Vektor3f(m[0], mx[1], m[2]),
                               Vektor3f(mx[0], m[1], m[2]), Vektor3f(m[0], mn[1], m[2]), Vektor3f(mn[0], m[1], m[2])};
        test.testDreiecke(p, idx, 24, bester);
    } else {
        static const unsigned int idx[24] = {2, 1, 5, 6, 1, 0, 4, 5, 0, 1, 2, 3, 3, 7, 4, 0, 3, 2, 6, 7, 7, 6, 5, 4};
        const std::array<Vektor3f, 8> p = ecken(k);
        test.testVierecke(p.data(), idx, 24, bester);
    }
}

// BrushInstance::testSelect
void pinselTesten(const Pinsel& p, const Kontext& k, AuswahlVolumen& test, Treffer& bester) {
    test.netzBeginnen(kEinheitsMatrix);
    p.bRep(k);
    std::vector<Vektor3d> punkte;
    for (const auto& f : p.flaechen()) {
        const Wicklung& w = f->wicklung();
        punkte.clear();
        for (std::size_t i = 0; i < w.numpoints; ++i) { punkte.push_back(w[i].vertex); }
        test.testPolygon(punkte.data(), punkte.size(), bester, f->ebenenPunkte());
    }
}

// PatchInstance::testSelect
void patchTesten(const Patch& p, AuswahlVolumen& test, Treffer& bester) {
    test.netzBeginnen(kEinheitsMatrix, true);
    const PatchTesselierung& t = p.tesselierung();
    for (std::size_t s = 0; s < t.numStrips; ++s) {
        test.testViereckStreifen(reinterpret_cast<const unsigned char*>(&t.vertices.data()->vertex), sizeof(NetzPunkt),
                                 t.indices.data() + (s * t.lenStrips), t.lenStrips, bester);
    }
}

void hinzu(Waehler& w, const Waehlbar& b, const Treffer& t) {   // Selector_add
    w.schieben(b);
    w.trefferDazu(t);
    w.holen();
}

void poolWaehlen(Karte& k, AuswahlPool& pool, bool an, float abstandEps) {   // SelectionPool_Select
    auto bester = pool.menge().begin();
    if (istGewaehlt(k, bester->second) != an) { setzeGewaehlt(k, bester->second, an); }
    for (auto i = std::next(bester); i != pool.menge().end(); ++i) {   // SELECT_MATCHING
        if (i->first.gleichEps(bester->first, abstandEps, kWaehlTiefe)) {
            setzeGewaehlt(k, i->second, an);
        } else {
            break;
        }
    }
}

}  // namespace

// --- Waehlbar ---------------------------------------------------------------------

bool istGewaehlt(const Karte& k, const Waehlbar& w) {
    switch (w.art) {
    case Waehlbar::Art::Primitiv: return k.auswahl.gewaehlt(w.entitaet.get(), w.primitiv.get());
    case Waehlbar::Art::Entitaet: return k.auswahl.gewaehlt(w.entitaet.get());
    case Waehlbar::Art::Flaeche: return k.auswahl.flaecheGewaehlt(w.flaeche.get());
    case Waehlbar::Art::Ecke: return static_cast<const Pinsel&>(*w.primitiv).eckeGewaehlt(w.index);
    case Waehlbar::Art::Kante: return static_cast<const Pinsel&>(*w.primitiv).kanteGewaehlt(w.index);
    case Waehlbar::Art::PatchPunkt: return static_cast<const Patch&>(*w.primitiv).punktGewaehlt(w.index);
    }
    return false;
}

void setzeGewaehlt(Karte& k, const Waehlbar& w, bool an) {
    switch (w.art) {
    case Waehlbar::Art::Primitiv:
        if (an) {
            k.auswahl.waehlen(w.entitaet, w.primitiv);
        } else {
            k.auswahl.abwaehlen(w.entitaet, w.primitiv);
        }
        break;
    case Waehlbar::Art::Entitaet:
        if (an) {
            k.auswahl.waehlen(w.entitaet);
        } else {
            k.auswahl.abwaehlen(w.entitaet);
        }
        break;
    case Waehlbar::Art::Flaeche:
        if (an) {
            k.auswahl.flaecheWaehlen(std::static_pointer_cast<Pinsel>(w.primitiv), w.flaeche);
        } else {
            k.auswahl.flaecheAbwaehlen(w.flaeche.get());
        }
        break;
    case Waehlbar::Art::Ecke: static_cast<Pinsel&>(*w.primitiv).eckeWaehlen(w.index, an); break;
    case Waehlbar::Art::Kante: static_cast<Pinsel&>(*w.primitiv).kanteWaehlen(w.index, an); break;
    case Waehlbar::Art::PatchPunkt: static_cast<Patch&>(*w.primitiv).punktWaehlen(w.index, an); break;
    }
}

// --- Waehler ----------------------------------------------------------------------

void AuswahlPool::schieben(const Waehlbar& w) {
    m_treffer = Treffer();
    m_aktuell = w;
}

void AuswahlPool::holen() {
    dazu(m_treffer, m_aktuell);
    m_treffer = Treffer();
}

void AuswahlPool::dazu(const Treffer& t, const Waehlbar& w) {
    if (t.gueltig()) { m_pool.emplace(t, w); }
}

void JaNeinWaehler::trefferDazu(const Treffer& t) {
    if (istGewaehlt(m_karte, m_aktuell)) { naeherZuweisen(m_bester, t); }
}

void BesterWaehler::holen() {
    if (m_treffer.gleichEps(m_bester, 0.25F, m_tiefeEps)) {
        m_beste.push_back(m_aktuell);
        m_bester = m_treffer;
    } else if (m_treffer < m_bester) {
        m_beste.clear();
        m_beste.push_back(m_aktuell);
        m_bester = m_treffer;
    }
    m_treffer = Treffer();
}

// --- Szenentests (selection.cpp:1836-1966) -------------------------------------------

void szeneTestenPrimitive(const Karte& k, Waehler& w, AuswahlVolumen& test) {
    for (const auto& ent : k.entitaeten()) {
        if (ent->versteckt) { continue; }
        if (!ent->istBehaelter()) {
            Treffer t;
            entitaetTesten(*ent, test, t);
            Waehlbar b{Waehlbar::Art::Entitaet, ent, nullptr, nullptr};
            hinzu(w, b, t);
            continue;
        }
        // Gruppen und worldspawn haben keine eigene Form; ihre Kinder sind waehlbar.
        for (const auto& p : ent->kinder()) {
            if (p->versteckt) { continue; }
            Treffer t;
            if (p->art() == Primitiv::Art::Pinsel) {
                pinselTesten(static_cast<const Pinsel&>(*p), k.kontext, test, t);
            } else {
                patchTesten(static_cast<const Patch&>(*p), test, t);
            }
            if (t.gueltig()) { hinzu(w, Waehlbar{Waehlbar::Art::Primitiv, ent, p, nullptr}, t); }
        }
    }
}

// testselect_component_visible(_selected) (selection.cpp:1902-1951) mit
// BrushInstance::testSelectComponents (brush.h:3681-3722) und
// PatchInstance::testSelectComponents (patch.h:1524-1540).
void szeneTestenKomponenten(const Karte& k, Waehler& w, AuswahlVolumen& test, KomponentenModus komponente, bool nurGewaehlte) {
    if (komponente == KomponentenModus::Standard) { return; }
    for (const auto& ent : k.entitaeten()) {
        if (ent->versteckt) { continue; }
        for (const auto& p : ent->kinder()) {
            if (p->versteckt) { continue; }
            if (nurGewaehlte && !k.auswahl.gewaehlt(ent.get(), p.get())) { continue; }
            if (p->art() == Primitiv::Art::Patch) {
                if (komponente != KomponentenModus::Ecke) { continue; }
                const auto& patch = static_cast<const Patch&>(*p);
                test.netzBeginnen(kEinheitsMatrix);
                for (std::size_t i = 0; i < patch.punkte().size(); ++i) {   // PatchControlInstance::testSelect
                    Treffer t;
                    test.testPunkt(patch.punkte()[i].vertex, t);
                    if (t.gueltig()) { hinzu(w, Waehlbar{Waehlbar::Art::PatchPunkt, ent, p, nullptr, i}, t); }
                }
                continue;
            }
            const auto& pinsel = static_cast<const Pinsel&>(*p);
            pinsel.bRep(k.kontext);
            test.netzBeginnen(kEinheitsMatrix);
            if (komponente == KomponentenModus::Ecke) {   // VertexInstance::testSelect
                const auto& ecken = pinsel.ecken(k.kontext);
                for (std::size_t i = 0; i < ecken.size(); ++i) {
                    Treffer t;
                    test.testPunkt(Vektor3f(pinsel.eckpunkt(ecken[i])), t);
                    if (t.gueltig()) { hinzu(w, Waehlbar{Waehlbar::Art::Ecke, ent, p, nullptr, i}, t); }
                }
                continue;
            }
            if (komponente == KomponentenModus::Kante) {   // EdgeInstance::testSelect mit SelectableEdge::getEdge
                const auto& kanten = pinsel.kanten(k.kontext);
                for (std::size_t i = 0; i < kanten.size(); ++i) {
                    const Wicklung& wi = pinsel[kanten[i].flaeche].wicklung();
                    const Vektor3d mitteD = mitte(wi[kanten[i].punkt].vertex, wi[wicklungNaechster(wi, kanten[i].punkt)].vertex);
                    Treffer t;
                    test.testPunkt(Vektor3f(mitteD), t);
                    if (t.gueltig()) { hinzu(w, Waehlbar{Waehlbar::Art::Kante, ent, p, nullptr, i}, t); }
                }
                continue;
            }
            std::vector<Vektor3d> punkte;
            for (const auto& f : pinsel.flaechen()) {
                Treffer t;
                if (test.blick().fuellen()) {   // FaceInstance::testSelect
                    const Wicklung& wi = f->wicklung();
                    punkte.clear();
                    for (std::size_t i = 0; i < wi.numpoints; ++i) { punkte.push_back(wi[i].vertex); }
                    test.testPolygon(punkte.data(), punkte.size(), t, f->ebenenPunkte());
                } else if (f->traegtBei()) {    // testSelect_centroid
                    test.testPunkt(f->schwerpunkt(), t);
                }
                if (t.gueltig()) { hinzu(w, Waehlbar{Waehlbar::Art::Flaeche, ent, p, f}, t); }
            }
        }
    }
}

void szeneTesten(const Karte& k, Waehler& w, AuswahlVolumen& test, AuswahlModus modus, KomponentenModus komponente) {
    switch (modus) {
    case AuswahlModus::Entitaet:   // testselect_entity_visible: ohne worldspawn und Gruppen
        for (const auto& ent : k.entitaeten()) {
            if (ent->versteckt || ent->istBehaelter()) { continue; }
            Treffer t;
            entitaetTesten(*ent, test, t);
            hinzu(w, Waehlbar{Waehlbar::Art::Entitaet, ent, nullptr, nullptr}, t);
        }
        break;
    case AuswahlModus::Primitiv: szeneTestenPrimitive(k, w, test); break;
    case AuswahlModus::Komponente: szeneTestenKomponenten(k, w, test, komponente, true); break;
    }
}

// --- Auswaehlen (selection.cpp:1346-1546) --------------------------------------------

void alleAbwaehlen(Karte& k, const AuswahlEinstellung& e) {
    if (e.modus == AuswahlModus::Komponente) {
        alleKomponentenAbwaehlen(k);   // setSelectedAllComponents( false )
    } else {
        k.auswahl.leeren();           // setSelectedAll( false )
    }
}

void komponentenOderAlleAbwaehlen(Karte& k, const AuswahlEinstellung& e, bool komponenten) {
    if (komponenten) {
        alleKomponentenAbwaehlen(k);   // setSelectedAllComponents( false )
    } else {
        alleAbwaehlen(k, e);
    }
}

void punktWaehlen(Karte& k, const AuswahlEinstellung& e, const Blick& blick, const GeraeteVektor& punkt, const GeraeteVektor& eps,
                  AuswahlArt art, bool flaeche, bool alt) {
    if (art == AuswahlArt::Ersetzen) { komponentenOderAlleAbwaehlen(k, e, flaeche); }
    Blick geschoren(blick);
    auswahlTestBauen(geschoren, auswahlKastenPunkt(punkt, eps));
    AuswahlVolumen volumen(geschoren);
    AuswahlPool pool(k);
    const bool punktEnts = e.punktEntitaetenIn2d && e.modus == AuswahlModus::Primitiv && !blick.fuellen() && !flaeche &&
                           (art == AuswahlArt::Ersetzen || art == AuswahlArt::Auswaehlen || art == AuswahlArt::Abwaehlen);
    if (punktEnts) { szeneTesten(k, pool, volumen, AuswahlModus::Entitaet, e.komponente); }
    if (punktEnts && !pool.leer()) {
        switch (art) {
        case AuswahlArt::Ersetzen: setzeGewaehlt(k, pool.menge().begin()->second, true); break;   // select closest
        case AuswahlArt::Auswaehlen: poolWaehlen(k, pool, true, kWaehlAbstand); break;
        case AuswahlArt::Abwaehlen: poolWaehlen(k, pool, false, kWaehlAbstand); break;
        default: break;
        }
        return;
    }
    const AuswahlModus modus = alt ? AuswahlModus::Primitiv : e.modus;
    if (flaeche) {
        szeneTestenKomponenten(k, pool, volumen, KomponentenModus::Flaeche, false);
    } else {
        szeneTesten(k, pool, volumen, modus, e.komponente);
    }
    if (!pool.leer()) {
        switch (art) {
        case AuswahlArt::Ersetzen: setzeGewaehlt(k, pool.menge().begin()->second, true); break;
        case AuswahlArt::Durchlaufen: {   // select the next object in the list from the one already selected
            bool weiter = false;
            for (auto i = pool.menge().begin(); i != pool.menge().end(); ++i) {
                if (istGewaehlt(k, i->second)) {
                    komponentenOderAlleAbwaehlen(k, e, flaeche);
                    ++i;
                    if (i != pool.menge().end()) {
                        setzeGewaehlt(k, i->second, true);
                    } else {
                        setzeGewaehlt(k, pool.menge().begin()->second, true);
                    }
                    weiter = true;
                    break;
                }
            }
            if (!weiter) {
                komponentenOderAlleAbwaehlen(k, e, flaeche);
                setzeGewaehlt(k, pool.menge().begin()->second, true);
            }
            break;
        }
        case AuswahlArt::Auswaehlen: poolWaehlen(k, pool, true, modus == AuswahlModus::Komponente ? kWaehlKomponenten : kWaehlAbstand); break;
        case AuswahlArt::Abwaehlen:
            // don't deselect only primitive in component mode
            if (!(modus == AuswahlModus::Primitiv && e.modus == AuswahlModus::Komponente && k.auswahl.anzahl() == 1)) {
                poolWaehlen(k, pool, false, modus == AuswahlModus::Komponente ? kWaehlKomponenten : kWaehlAbstand);
            }
            break;
        default: break;
        }
    } else if (art == AuswahlArt::Durchlaufen) {
        komponentenOderAlleAbwaehlen(k, e, flaeche);
    }
}

AuswahlArt malenBeginnen(Karte& k, const AuswahlEinstellung& e, const Blick& blick, const GeraeteVektor& punkt, const GeraeteVektor& eps,
                         bool flaeche, bool alt) {
    Blick geschoren(blick);
    auswahlTestBauen(geschoren, auswahlKastenPunkt(punkt, eps));
    AuswahlVolumen volumen(geschoren);
    AuswahlPool pool(k);
    const bool punktEnts = e.punktEntitaetenIn2d && e.modus == AuswahlModus::Primitiv && !blick.fuellen() && !flaeche;
    if (punktEnts) { szeneTesten(k, pool, volumen, AuswahlModus::Entitaet, e.komponente); }
    if (punktEnts && !pool.leer()) {
        const bool war = istGewaehlt(k, pool.menge().begin()->second);
        poolWaehlen(k, pool, !war, kWaehlAbstand);
        return war ? AuswahlArt::Abwaehlen : AuswahlArt::Auswaehlen;
    }
    const AuswahlModus modus = alt ? AuswahlModus::Primitiv : e.modus;
    if (flaeche) {
        szeneTestenKomponenten(k, pool, volumen, KomponentenModus::Flaeche, false);
    } else {
        szeneTesten(k, pool, volumen, modus, e.komponente);
    }
    if (!pool.leer()) {
        const bool war = istGewaehlt(k, pool.menge().begin()->second);
        if (!(modus == AuswahlModus::Primitiv && e.modus == AuswahlModus::Komponente && k.auswahl.anzahl() == 1 && war)) {
            poolWaehlen(k, pool, !war, modus == AuswahlModus::Komponente ? kWaehlKomponenten : kWaehlAbstand);
        }
        return war ? AuswahlArt::Abwaehlen : AuswahlArt::Auswaehlen;
    }
    return AuswahlArt::Auswaehlen;
}

void bereichWaehlen(Karte& k, const AuswahlEinstellung& e, const Blick& blick, const AuswahlRechteck& r, bool flaeche) {
    Blick geschoren(blick);
    auswahlTestBauen(geschoren, r);
    AuswahlVolumen volumen(geschoren);
    AuswahlPool pool(k);
    if (flaeche) {
        szeneTestenKomponenten(k, pool, volumen, KomponentenModus::Flaeche, false);
    } else {
        szeneTesten(k, pool, volumen, e.modus, e.komponente);
    }
    for (auto& [treffer, w] : pool.menge()) {
        (void)treffer;
        setzeGewaehlt(k, w, r.art == AuswahlRechteck::Art::Auswaehlen ? true : r.art == AuswahlRechteck::Art::Abwaehlen ? false : !istGewaehlt(k, w));
    }
}

Vektor3f szeneSchnitt(const Karte& k, const Blick& blick, const GeraeteVektor& punkt, const GeraeteVektor& eps) {
    Blick geschoren(blick);
    auswahlTestBauen(geschoren, auswahlKastenPunkt(punkt, eps));
    AuswahlVolumen test(geschoren);
    BesterPunktWaehler w;
    szeneTestenPrimitive(k, w, test);
    test.netzBeginnen(kEinheitsMatrix, true);
    if (w.getroffen()) {
        return projiziert(transformierterVektor4(test.bildZuWelt(), Vektor4f(0, 0, w.bester().tiefe(), 1)));
    }
    const Vektor3f nah = projiziert(transformierterVektor4(test.bildZuWelt(), Vektor4f(0, 0, -1, 1)));
    const Vektor3f fern = projiziert(transformierterVektor4(test.bildZuWelt(), Vektor4f(0, 0, 1, 1)));
    return normiert(fern - nah) * 256.0F + nah;
}

// --- Grenzen ----------------------------------------------------------------------

Kasten entitaetKastenLokal(const Entitaet& e) { return lokalerKasten(e); }
bool istLichtEntitaet(const Entitaet& e) { return istLicht(e); }
void testeEntitaet(const Entitaet& e, AuswahlVolumen& test, Treffer& bester) { entitaetTesten(e, test, bester); }
void testePinsel(const Pinsel& p, const Kontext& k, AuswahlVolumen& test, Treffer& bester) { pinselTesten(p, k, test, bester); }
void testePatch(const Patch& p, AuswahlVolumen& test, Treffer& bester) { patchTesten(p, test, bester); }
void testeFlaeche(const Flaeche& f, AuswahlVolumen& test, Treffer& bester) {
    const Wicklung& w = f.wicklung();
    std::vector<Vektor3d> punkte;
    punkte.reserve(w.numpoints);
    for (std::size_t i = 0; i < w.numpoints; ++i) { punkte.push_back(w[i].vertex); }
    test.testPolygon(punkte.data(), punkte.size(), bester, f.ebenenPunkte());
}

Vektor3f entitaetUrsprung(const Entitaet& e) {
    Vektor3f o(0, 0, 0);
    float a = 0;
    float b = 0;
    float c = 0;
    if (std::sscanf(e.wert("origin").c_str(), "%f %f %f", &a, &b, &c) == 3) { o = Vektor3f(a, b, c); }
    return o;
}

namespace {

Kasten primitivGrenzen(const Primitiv& p, const Kontext& k) {
    if (p.art() == Primitiv::Art::Pinsel) { return static_cast<const Pinsel&>(p).grenzen(k); }
    return static_cast<const Patch&>(p).grenzen();
}

Kasten kinderGrenzen(const Entitaet& e, const Kontext& k) {
    Kasten b;
    for (const auto& p : e.kinder()) { kastenErweiternSicher(b, primitivGrenzen(*p, k)); }
    return b;
}

}  // namespace

Kasten auswahlGrenzenPivot(const Karte& k) {
    // bounds_selected + Instance_getPivotBounds: Punktentitaeten zaehlen mit
    // ihrem Ursprung (ohne Ausdehnung), alles andere mit worldAABB.
    Kasten b;
    for (const auto& w : k.auswahl.elemente()) {
        const auto ent = w.entitaet.lock();
        const auto p = w.primitiv.lock();
        if (p) {
            kastenErweiternSicher(b, primitivGrenzen(*p, k.kontext));
        } else if (ent) {
            const EntitaetsKlasse* kl = ent->klasse();
            if (!ent->istBehaelter() && !(kl != nullptr && kl->miscmodelIs)) {
                kastenErweiternSicher(b, Kasten(entitaetUrsprung(*ent), Vektor3f(0, 0, 0)));
            } else if (!ent->istBehaelter()) {
                const Vektor3f o = entitaetUrsprung(*ent);
                const Kasten l = lokalerKasten(*ent);
                kastenErweiternSicher(b, Kasten(l.origin + o, l.extents));
            } else {
                kastenErweiternSicher(b, kinderGrenzen(*ent, k.kontext));
            }
        }
    }
    return b;
}

Kasten auswahlGrenzenMitEntitaeten(const Karte& k) {
    Kasten b;
    for (const auto& w : k.auswahl.elemente()) {
        const auto ent = w.entitaet.lock();
        const auto p = w.primitiv.lock();
        if (p) {
            kastenErweiternSicher(b, primitivGrenzen(*p, k.kontext));
        } else if (ent) {
            const EntitaetsKlasse* kl = ent->klasse();
            if (kl != nullptr && kl->fixedsize && !kl->miscmodelIs) {
                const Vektor3f o = entitaetUrsprung(*ent);
                kastenErweiternSicher(b, kastenAusMinMax(kl->mins + o, kl->maxs + o));
            } else if (!ent->istBehaelter()) {
                const Vektor3f o = entitaetUrsprung(*ent);
                const Kasten l = lokalerKasten(*ent);
                kastenErweiternSicher(b, Kasten(l.origin + o, l.extents));
            } else {
                kastenErweiternSicher(b, kinderGrenzen(*ent, k.kontext));
            }
        }
    }
    return b;
}

}  // namespace bhed::karte
