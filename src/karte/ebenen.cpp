// ebenen.cpp - Karten-Editor: die Ebenen (Layers).
//
// Portiert aus NetRadiant-Custom 20260114 (Quellen in ebenen.h).
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   For a list of contributors, see the accompanying CONTRIBUTORS file.
//   This file is part of GtkRadiant.
//
//   GtkRadiant is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. GtkRadiant is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#include "bhed/karte/ebenen.h"

#include "bhed/karte/karte.h"
#include "bhed/karte/komponenten.h"
#include "bhed/karte/pinsel.h"

#include <algorithm>
#include <memory>

namespace bhed::karte {

namespace {

// Der Baum zum Bearbeiten (std::list<Layer> m_children je Ebene)
struct Baum {
    std::vector<Schicht> daten;              // je alter Nummer (neue hinten)
    std::vector<std::vector<int>> kinder;    // je Nummer
    std::vector<int> wurzel;                 // Layers::m_children
    std::vector<int>& liste(int eltern) { return eltern < 0 ? wurzel : kinder[static_cast<std::size_t>(eltern)]; }
};

Baum baumAus(const Karte& k) {
    Baum b;
    b.daten = k.schichten;
    b.kinder.resize(k.schichten.size());
    for (std::size_t i = 0; i < k.schichten.size(); ++i) { b.liste(k.schichten[i].eltern).push_back(static_cast<int>(i)); }
    return b;
}

void unterbaumSammeln(const Baum& b, int ebene, std::vector<int>& aus) {   // items_iterate_recursively
    aus.push_back(ebene);
    for (const int c : b.kinder[static_cast<std::size_t>(ebene)]) { unterbaumSammeln(b, c, aus); }
}

// update_ownIndices: neu ablegen, Knoten umnummerieren, aktuelle Ebene mitnehmen
EbenenNummern ablegen(Karte& k, const Baum& b, std::size_t alteAnzahl) {
    std::vector<Schicht> neu;
    EbenenNummern nummer(b.daten.size(), -1);
    const auto besuchen = [&](auto&& selbst, const std::vector<int>& liste, int eltern) -> void {
        for (const int i : liste) {
            nummer[static_cast<std::size_t>(i)] = static_cast<int>(neu.size());
            Schicht s = b.daten[static_cast<std::size_t>(i)];
            s.eltern = eltern;
            neu.push_back(s);
            selbst(selbst, b.kinder[static_cast<std::size_t>(i)], nummer[static_cast<std::size_t>(i)]);
        }
    };
    besuchen(besuchen, b.wurzel, -1);
    const auto umnummern = [&](int s) { return s >= 0 && static_cast<std::size_t>(s) < nummer.size() ? nummer[static_cast<std::size_t>(s)] : s; };
    for (const auto& ent : k.entitaeten()) {
        ent->schicht = umnummern(ent->schicht);
        for (const auto& p : ent->kinder()) { p->schicht = umnummern(p->schicht); }
    }
    const int aktuell = umnummern(static_cast<int>(k.aktuelleSchicht));
    k.aktuelleSchicht = static_cast<std::size_t>(std::max(0, aktuell));
    k.schichten = std::move(neu);
    nummer.resize(alteAnzahl);
    return nummer;
}

bool enthalten(const std::vector<int>& v, int x) { return std::find(v.begin(), v.end(), x) != v.end(); }

}  // namespace

std::vector<int> ebenenKinder(const Karte& k, int eltern) {
    std::vector<int> v;
    for (std::size_t i = 0; i < k.schichten.size(); ++i) {
        if (k.schichten[i].eltern == eltern) { v.push_back(static_cast<int>(i)); }
    }
    return v;
}

std::vector<int> ebenenUnterbaum(const Karte& k, int ebene) {
    std::vector<int> v;
    if (ebene < 0 || static_cast<std::size_t>(ebene) >= k.schichten.size()) { return v; }
    unterbaumSammeln(baumAus(k), ebene, v);
    return v;
}

int ebenenTiefe(const Karte& k, int ebene) {
    int t = 0;
    while (ebene >= 0 && static_cast<std::size_t>(ebene) < k.schichten.size() && k.schichten[static_cast<std::size_t>(ebene)].eltern >= 0) {
        ebene = k.schichten[static_cast<std::size_t>(ebene)].eltern;
        ++t;
    }
    return t;
}

EbenenNummern ebeneNeu(Karte& k, int eltern, const std::string& name, int* neu) {   // layer_new
    Baum b = baumAus(k);
    const std::size_t alt = k.schichten.size();
    b.daten.push_back(Schicht{name, eltern, {0, 0, 0}});
    b.kinder.emplace_back();
    const int id = static_cast<int>(alt);
    b.liste(eltern).push_back(id);   // m_children.emplace_back
    EbenenNummern n = ablegen(k, b, alt + 1);
    const int nn = n[static_cast<std::size_t>(id)];
    k.aktuelleSchicht = static_cast<std::size_t>(nn);   // item_setCurrent( newItem )
    if (neu != nullptr) { *neu = nn; }
    n.resize(alt);
    return n;
}

EbenenNummern ebeneLoeschen(Karte& k, int ebene) {   // context_menu "Delete"
    if (ebene < 0 || static_cast<std::size_t>(ebene) >= k.schichten.size()) { return {}; }
    Baum b = baumAus(k);
    if (k.schichten[static_cast<std::size_t>(ebene)].eltern < 0 && b.wurzel.size() == 1) { return {}; }   // die einzige oberste
    std::vector<int> weg;
    unterbaumSammeln(b, ebene, weg);
    std::vector<int>& liste = b.liste(k.schichten[static_cast<std::size_t>(ebene)].eltern);
    liste.erase(std::remove(liste.begin(), liste.end(), ebene), liste.end());   // delete item
    // die aktuelle Ebene zuerst: sie ist gleich das Ziel
    if (enthalten(weg, static_cast<int>(k.aktuelleSchicht))) { k.aktuelleSchicht = static_cast<std::size_t>(b.wurzel.front()); }
    const int ziel = static_cast<int>(k.aktuelleSchicht);
    for (const auto& ent : k.entitaeten()) {   // LayerDeletetWalker
        if (enthalten(weg, ent->schicht)) { ent->schicht = ziel; }
        for (const auto& p : ent->kinder()) {
            if (enthalten(weg, p->schicht)) { p->schicht = ziel; }
        }
    }
    return ablegen(k, b, k.schichten.size());
}

EbenenNummern ebeneVerschieben(Karte& k, int ebene, int neuerEltern, int position) {   // rowsInserted
    if (ebene < 0 || static_cast<std::size_t>(ebene) >= k.schichten.size()) { return {}; }
    Baum b = baumAus(k);
    std::vector<int>& quelle = b.liste(k.schichten[static_cast<std::size_t>(ebene)].eltern);
    quelle.erase(std::remove(quelle.begin(), quelle.end(), ebene), quelle.end());
    std::vector<int>& ziel = b.liste(neuerEltern);
    position = std::clamp(position, 0, static_cast<int>(ziel.size()));
    ziel.insert(ziel.begin() + position, ebene);
    return ablegen(k, b, k.schichten.size());
}

void ebeneUmbenennen(Karte& k, int ebene, const std::string& name) {
    if (ebene >= 0 && static_cast<std::size_t>(ebene) < k.schichten.size()) { k.schichten[static_cast<std::size_t>(ebene)].name = name; }
}

void ebeneFarbe(Karte& k, int ebene, const std::array<int, 3>& farbe) {
    if (ebene >= 0 && static_cast<std::size_t>(ebene) < k.schichten.size()) { k.schichten[static_cast<std::size_t>(ebene)].farbe = farbe; }
}

std::string ebenenName(const std::string& text) {
    std::string s = text;
    s.erase(std::remove(s.begin(), s.end(), '"'), s.end());
    return s;
}

void auswahlAufEbene(Karte& k, int ebene) {   // LayerAssignVisitor
    for (const auto& el : k.auswahl.elemente()) {
        const auto e = el.entitaet.lock();
        const auto p = el.primitiv.lock();
        if (p) {
            p->schicht = ebene;
        } else if (e && !e->istBehaelter()) {
            e->schicht = ebene;
        }
    }
}

void ebenenWaehlen(Karte& k, const std::vector<int>& ebenen, bool waehlen) {   // LayerSelectWalker
    for (const auto& ent : k.entitaeten()) {
        if (ent->sichtbar() && enthalten(ebenen, ent->schicht)) {
            if (waehlen) {
                k.auswahl.waehlen(ent);
            } else {
                k.auswahl.abwaehlen(ent);
            }
        }
        for (const auto& p : ent->kinder()) {
            if (p->sichtbar() && enthalten(ebenen, p->schicht)) {
                if (waehlen) {
                    k.auswahl.waehlen(ent, p);
                } else {
                    k.auswahl.abwaehlen(ent, p);
                }
            }
        }
    }
}

void ebenenSichtbar(Karte& k, const std::vector<int>& ebenen, bool sichtbar) {   // LayersSetVisible::setVisible
    for (const auto& ent : k.entitaeten()) {
        if (enthalten(ebenen, ent->schicht)) {   // pre
            ent->ebeneVersteckt = !sichtbar;
            if (!sichtbar) { k.auswahl.abwaehlen(ent); }
        }
        for (const auto& p : ent->kinder()) {
            if (enthalten(ebenen, p->schicht)) {
                p->ebeneVersteckt = !sichtbar;
                if (!sichtbar) { k.auswahl.abwaehlen(ent, p); }
            }
        }
        if (ent->istBehaelter()) {   // post: Gruppen nach ihrem Inhalt
            const auto& c = ent->kinder();
            if (!sichtbar && std::all_of(c.begin(), c.end(), [](const auto& p) { return p->ebeneVersteckt; })) { ent->ebeneVersteckt = true; }
            if (sichtbar && std::any_of(c.begin(), c.end(), [](const auto& p) { return !p->ebeneVersteckt; })) { ent->ebeneVersteckt = false; }
        }
    }
    if (!sichtbar) {
        // not hiding worldspawn node so that newly created brushes are visible
        if (const auto w = k.welt()) { w->ebeneVersteckt = false; }
        if (komponentenAnzahl(k) != 0) { alleKomponentenAbwaehlen(k); }
    }
}

void auswahlAufEbenenErweitern(Karte& k) {   // Scene_ExpandSelectionToLayers
    std::vector<int> ebenen;
    for (const auto& el : k.auswahl.elemente()) {   // LayerCollectVisitor
        const auto e = el.entitaet.lock();
        const auto p = el.primitiv.lock();
        const int s = p ? p->schicht : (e ? e->schicht : kSchichtKeine);
        if (s != kSchichtKeine && !enthalten(ebenen, s)) { ebenen.push_back(s); }
    }
    const Baum b = baumAus(k);
    for (std::size_t i = 0; i < ebenen.size(); ++i) {   // vector may be changed during loop
        if (ebenen[i] < 0) { continue; }
        std::vector<int> unter;
        unterbaumSammeln(b, ebenen[i], unter);
        for (std::size_t j = 1; j < unter.size(); ++j) {   // forEach: alle darunter
            if (!enthalten(ebenen, unter[j])) { ebenen.push_back(unter[j]); }
        }
    }
    ebenenWaehlen(k, ebenen, true);
}

}  // namespace bhed::karte
