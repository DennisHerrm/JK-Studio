// komponenten.cpp - Karten-Editor: die Komponenten der Auswahl.
// Portiert aus NetRadiant-Custom 20260114 (Quellen in komponenten.h).
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
#include "bhed/karte/komponenten.h"

#include "bhed/karte/karte.h"
#include "bhed/karte/patch.h"

#include <algorithm>

namespace bhed::karte {

namespace {

bool gewaehlt(const Karte& k, const Entitaet& e, const Primitiv& p) { return k.auswahl.gewaehlt(&e, &p); }

}  // namespace

std::vector<GewaehltesPrimitiv> gewaehltePrimitiveListe(const Karte& k) {
    std::vector<GewaehltesPrimitiv> r;
    for (const auto& el : k.auswahl.elemente()) {
        auto ent = el.entitaet.lock();
        auto p = el.primitiv.lock();
        if (ent && p) { r.push_back(GewaehltesPrimitiv{ent, p}); }
    }
    return r;
}

bool pinselKomponentenGewaehlt(const Karte& k, const Pinsel& p) {
    return std::any_of(p.flaechen().begin(), p.flaechen().end(), [&](const auto& f) {
        return k.auswahl.flaecheGewaehlt(f.get()) || !f->eckenWahl.empty() || !f->kantenWahl.empty();   // FaceInstance::selectedComponents
    });
}

std::size_t komponentenAnzahl(const Karte& k) {
    std::size_t n = k.auswahl.flaechen().size();   // FaceInstance::m_selectable
    for (const auto& ent : k.entitaeten()) {
        for (const auto& p : ent->kinder()) {
            if (p->art() == Primitiv::Art::Pinsel) {
                for (const auto& f : static_cast<const Pinsel&>(*p).flaechen()) {
                    if (!f->eckenWahl.empty()) { ++n; }    // m_selectableVertices
                    if (!f->kantenWahl.empty()) { ++n; }   // m_selectableEdges
                }
            } else {
                for (const char c : static_cast<const Patch&>(*p).punktWahl) {
                    if (c != 0) { ++n; }
                }
            }
        }
    }
    return n;
}

// select_all_component (selection.cpp:966-981) -> BrushInstance::setSelectedComponents
// (brush.h:3669-3674, FaceInstance::setSelected) / PatchInstance::setSelectedComponents
void komponentenAbwaehlen(Karte& k, KomponentenModus modus) {
    komponentenWahlGeaendert();
    if (modus == KomponentenModus::Flaeche) {
        k.auswahl.flaechenLeeren();
        return;   // PatchInstance: m_dragPlanes (AuswahlBearbeitung)
    }
    for (const auto& ent : k.entitaeten()) {
        for (const auto& p : ent->kinder()) {
            if (p->art() == Primitiv::Art::Pinsel) {
                for (const auto& f : static_cast<Pinsel&>(*p).flaechen()) {
                    if (modus == KomponentenModus::Ecke) {
                        f->eckenWahl.clear();
                    } else if (modus == KomponentenModus::Kante) {
                        f->kantenWahl.clear();
                    }
                }
            } else if (modus == KomponentenModus::Ecke) {
                auto& wahl = static_cast<Patch&>(*p).punktWahl;
                std::fill(wahl.begin(), wahl.end(), 0);
            }
        }
    }
}

void alleKomponentenAbwaehlen(Karte& k) {
    komponentenAbwaehlen(k, KomponentenModus::Ecke);
    komponentenAbwaehlen(k, KomponentenModus::Kante);
    komponentenAbwaehlen(k, KomponentenModus::Flaeche);
}

// BrushInstance::invertComponentSelection (brush.h:3789-3820),
// PatchInstance::invertComponentSelection (patch.h:1608-1613)
void komponentenUmkehren(Karte& k, KomponentenModus modus) {
    for (const auto& ent : k.entitaeten()) {
        if (!ent->sichtbar()) { continue; }
        for (const auto& p : ent->kinder()) {
            if (!p->sichtbar() || !gewaehlt(k, *ent, *p)) { continue; }
            if (p->art() == Primitiv::Art::Patch) {
                if (modus != KomponentenModus::Ecke) { continue; }
                auto& patch = static_cast<Patch&>(*p);
                for (std::size_t i = 0; i < patch.punkte().size(); ++i) { patch.punktWaehlen(i, !patch.punktGewaehlt(i)); }
                continue;
            }
            const auto pinsel = std::static_pointer_cast<Pinsel>(p);
            pinsel->bRep(k.kontext);
            switch (modus) {
            case KomponentenModus::Ecke:
                for (std::size_t i = 0; i < pinsel->ecken(k.kontext).size(); ++i) { pinsel->eckeWaehlen(i, !pinsel->eckeGewaehlt(i)); }
                break;
            case KomponentenModus::Kante:
                for (std::size_t i = 0; i < pinsel->kanten(k.kontext).size(); ++i) { pinsel->kanteWaehlen(i, !pinsel->kanteGewaehlt(i)); }
                break;
            case KomponentenModus::Flaeche:
                for (const auto& f : pinsel->flaechen()) {   // ohne gefilterte Flaechen (isFiltered)
                    if (k.auswahl.flaecheGewaehlt(f.get())) {
                        k.auswahl.flaecheAbwaehlen(f.get());
                    } else {
                        k.auswahl.flaecheWaehlen(pinsel, f);
                    }
                }
                break;
            default: break;
            }
        }
    }
}

Kasten komponentenGrenzen(const Karte& k) {
    Kasten b;
    for (const GewaehltesPrimitiv& g : gewaehltePrimitiveListe(k)) {
        if (g.primitiv->art() == Primitiv::Art::Patch) {   // PatchInstance::getSelectedComponentsBounds (patch.h:1565-1576)
            const auto& patch = static_cast<const Patch&>(*g.primitiv);
            for (std::size_t i = 0; i < patch.punkte().size(); ++i) {
                if (patch.punktGewaehlt(i)) { kastenErweiternSicher(b, patch.punkte()[i].vertex); }
            }
            continue;
        }
        const auto& pinsel = static_cast<const Pinsel&>(*g.primitiv);
        pinsel.bRep(k.kontext);
        for (const auto& f : pinsel.flaechen()) {   // FaceInstance::iterate_selected -> SelectedComponents_foreach
            const Wicklung& w = f->wicklung();
            for (const std::size_t nachbar : f->eckenWahl) {   // SelectedVertices_foreach
                const std::size_t i = wicklungFindeNachbar(w, nachbar);
                if (i != kMaxFlaechen) { kastenErweiternSicher(b, Vektor3f(w[i].vertex)); }
            }
            for (const std::size_t nachbar : f->kantenWahl) {   // SelectedEdges_foreach
                const std::size_t i = wicklungFindeNachbar(w, nachbar);
                if (i != kMaxFlaechen) { kastenErweiternSicher(b, Vektor3f(mitte(w[i].vertex, w[wicklungNaechster(w, i)].vertex))); }
            }
            if (k.auswahl.flaecheGewaehlt(f.get())) { kastenErweiternSicher(b, f->schwerpunkt()); }   // SelectedFaces_foreach
        }
    }
    return b;
}

void komponentenSammeln(const Karte& k, const Pinsel& p, const std::function<void(const Vektor3d&)>& fertig) {
    for (const auto& f : p.flaechen()) {
        const Wicklung& w = f->wicklung();
        if (k.auswahl.flaecheGewaehlt(f.get())) {
            for (std::size_t i = 0; i < w.numpoints; ++i) { fertig(w[i].vertex); }
        }
        for (const std::size_t nachbar : f->eckenWahl) {
            const std::size_t i = wicklungFindeNachbar(w, nachbar);
            if (i != kMaxFlaechen) { fertig(w[i].vertex); }
        }
        for (const std::size_t nachbar : f->kantenWahl) {
            const std::size_t i = wicklungFindeNachbar(w, nachbar);
            if (i != kMaxFlaechen) {
                fertig(w[i].vertex);
                fertig(w[wicklungNaechster(w, i)].vertex);
            }
        }
    }
}

void komponentenEinrasten(Karte& k, float raster, bool eckenSperre) {
    for (const auto& ent : k.entitaeten()) {
        if (!ent->sichtbar()) { continue; }
        for (const auto& p : ent->kinder()) {
            if (!p->sichtbar() || !gewaehlt(k, *ent, *p)) { continue; }
            if (p->art() == Primitiv::Art::Patch) {   // PatchInstance::snapComponents (patch.h:1641-1652)
                auto& patch = static_cast<Patch&>(*p);
                if (!patch.punkteGewaehlt()) { continue; }
                std::vector<PatchPunkt> neu = patch.punkte();
                for (std::size_t i = 0; i < neu.size(); ++i) {
                    if (patch.punktGewaehlt(i)) { einrasten(neu[i].vertex, raster); }   // PatchControlInstance::snapto
                }
                patch.punkteSetzen(neu);   // undoSave + controlPointsChanged
                continue;
            }
            auto& pinsel = static_cast<Pinsel&>(*p);   // BrushInstance::snapComponents (brush.h:3976-3986)
            pinsel.bRep(k.kontext);
            if (std::any_of(pinsel.flaechen().begin(), pinsel.flaechen().end(), [](const auto& f) { return !f->eckenWahl.empty(); })) {
                pinsel.eckenEinrasten(raster, k.kontext, eckenSperre, false);   // vertex_snap( snap, false )
                continue;
            }
            for (const auto& f : pinsel.flaechen()) {   // FaceInstance::snapComponents (brush.h:2891-2911)
                if (k.auswahl.flaecheGewaehlt(f.get())) { (void)f->einrasten(raster); }
                if (!f->kantenWahl.empty()) {
                    for (auto& q : f->bewegPunkte) { einrasten(q, raster); }
                    f->setzeEbenenPunkte(f->bewegPunkte);
                    f->bewegPunkteT = f->bewegPunkte;
                }
            }
        }
    }
}

void eckenEntfernen(Karte& k, bool eckenSperre) {
    for (const GewaehltesPrimitiv& g : gewaehltePrimitiveListe(k)) {
        if (g.primitiv->art() != Primitiv::Art::Pinsel) { continue; }
        auto& pinsel = static_cast<Pinsel&>(*g.primitiv);
        pinsel.bRep(k.kontext);
        if (pinsel.ecken(k.kontext).empty()) { continue; }
        pinsel.eckenModusBeginnen();
        pinsel.eckenModusSammeln(k.kontext);
        std::vector<Pinsel::EckenModusPunkt> v;
        v.reserve(pinsel.eckenModusPunkte.size());
        for (auto& e : pinsel.eckenModusPunkte) {
            if (!e.gewaehlt) { v.push_back(std::move(e)); }   // gather + pop_back der gewaehlten
        }
        std::vector<bool> ok(v.size(), true);
        komponentenSammeln(k, pinsel, [&](const Vektor3d& wert) {
            for (std::size_t i = 0; i < v.size(); ++i) {
                if (laengeQuadrat(v[i].ecke - wert) < 0.05 * 0.05) { ok[i] = false; }
            }
        });
        pinsel.eckenModusPunkte.clear();
        for (std::size_t i = 0; i < v.size(); ++i) {
            if (ok[i]) { pinsel.eckenModusPunkte.push_back(std::move(v[i])); }
        }
        pinsel.eckenModusHuelle(k.kontext, eckenSperre);
        pinsel.bRep(k.kontext);
        pinsel.eckenModusEnde();
    }
}

void eckenEinfuegen(Pinsel& p, const std::vector<Pinsel::EckenModusPunkt>& neue, const Kontext& k, bool eckenSperre) {
    p.bRep(k);
    if (p.ecken(k).empty()) { return; }
    p.eckenModusBeginnen();
    p.eckenModusSammeln(k);
    for (const auto& e : neue) {
        p.eckenModusPunkte.push_back(e);
        if (e.flaechen.empty()) { p.eckenModusPunkte.back().flaechen.push_back(p.eckenModusPunkte[0].flaechen[0]); }
    }
    p.eckenModusTransformieren(kEinheitsMatrix, k, eckenSperre);
    p.bRep(k);
}

}  // namespace bhed::karte
