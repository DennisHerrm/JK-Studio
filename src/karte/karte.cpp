// karte.cpp - Karten-Editor: Entitaeten, Kartenbaum, Auswahl.
//
// Portiert aus NetRadiant-Custom (Stand master, 2026): libs/entitylib.h
// (EntityKeyValues), plugins/entity/entity.cpp (node_for_eclass),
// libs/traverselib.h (TraversableNodeSet), radiant/map.cpp
// (Map_FindWorldspawn, createWorldspawn), libs/layers.h (Layers()).
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   For a list of contributors, see the accompanying CONTRIBUTORS file.
//   Copyright (C) 2001-2006, William Joseph. All Rights Reserved.
//   This file is part of GtkRadiant.
//
//   GtkRadiant is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. GtkRadiant is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#include "bhed/karte/karte.h"

#include <unordered_map>

#include <algorithm>

namespace bhed::karte {

namespace {

struct EntitaetMemento final : Memento {
    Entitaet::Schluessel schluessel;
    Entitaet::Kinder kinder;
};

struct ListenMemento final : Memento {
    EntitaetenListe::Liste liste;
};

void primitivAnmelden(Primitiv& p, Rueckgaengig* u) {
    if (p.art() == Primitiv::Art::Pinsel) {
        static_cast<Pinsel&>(p).anmeldenMitFlaechen(u);
    } else {
        p.anmelden(u);
    }
}

void primitivAbmelden(Primitiv& p) {
    if (p.art() == Primitiv::Art::Pinsel) {
        static_cast<Pinsel&>(p).abmeldenMitFlaechen();
    } else {
        p.abmelden();
    }
}

}  // namespace

// ===================== Entitaet =============================================

// EntityKeyValues( EntityClass* ): m_isContainer( !eclass->fixedsize )
Entitaet::Entitaet(const EntitaetsKlasse* klasse) : m_klasse(klasse), m_behaelter(!klasse->fixedsize) {}

std::string Entitaet::wert(const std::string& schluessel) const {   // getKeyValue
    for (const auto& [k, v] : m_schluessel) {
        if (k == schluessel) { return v; }
    }
    return {};
}

bool Entitaet::hat(const std::string& schluessel) const {   // hasKeyValue
    return std::any_of(m_schluessel.begin(), m_schluessel.end(), [&](const auto& kv) { return kv.first == schluessel; });
}

// setKeyValue (entitylib.h:618-628) mit insert / erase / KeyValue::assign:
// gesichert wird nur, wenn sich etwas aendert.
void Entitaet::setzen(const std::string& schluessel, const std::string& wert) {
    auto it = std::find_if(m_schluessel.begin(), m_schluessel.end(), [&](const auto& kv) { return kv.first == schluessel; });
    if (wert.empty()) {
        if (it != m_schluessel.end()) {
            sichern();
            m_schluessel.erase(it);
        }
        return;
    }
    if (it != m_schluessel.end()) {
        if (it->second != wert) {
            sichern();
            it->second = wert;
        }
        return;
    }
    sichern();
    m_schluessel.emplace_back(schluessel, wert);
}

const std::string& Entitaet::klassenname() const { return m_klasse->name; }   // getClassName

// TraversableNodeSet::insert / erase (traverselib.h)
void Entitaet::kindEinfuegen(std::shared_ptr<Primitiv> p) {
    sichern();
    if (undoSystem()) { primitivAnmelden(*p, undoSystem()); }
    m_kinder.push_back(std::move(p));
}

bool Entitaet::kindEntfernen(const Primitiv* p) {
    auto it = std::find_if(m_kinder.begin(), m_kinder.end(), [&](const auto& k) { return k.get() == p; });
    if (it == m_kinder.end()) { return false; }
    sichern();
    primitivAbmelden(**it);
    m_kinder.erase(it);
    return true;
}

std::unique_ptr<Memento> Entitaet::zustandSichern() const {
    auto m = std::make_unique<EntitaetMemento>();
    m->schluessel = m_schluessel;
    m->kinder = m_kinder;
    return m;
}

void Entitaet::zustandLaden(const Memento& mm) {
    sichern();
    const auto& m = static_cast<const EntitaetMemento&>(mm);
    m_schluessel = m.schluessel;
    for (auto& k : m_kinder) {
        if (std::find(m.kinder.begin(), m.kinder.end(), k) == m.kinder.end()) { primitivAbmelden(*k); }
    }
    m_kinder = m.kinder;
    if (undoSystem()) {
        for (auto& k : m_kinder) { primitivAnmelden(*k, undoSystem()); }
    }
}

void Entitaet::anmeldenMitKindern(Rueckgaengig* u) {
    anmelden(u);
    for (auto& k : m_kinder) { primitivAnmelden(*k, u); }
}

void Entitaet::abmeldenMitKindern() {
    for (auto& k : m_kinder) { primitivAbmelden(*k); }
    abmelden();
}

// ===================== EntitaetenListe ======================================

void EntitaetenListe::einfuegen(std::shared_ptr<Entitaet> e) {
    sichern();
    if (undoSystem()) { e->anmeldenMitKindern(undoSystem()); }
    m_liste.push_back(std::move(e));
}

bool EntitaetenListe::entfernen(const Entitaet* e) {
    auto it = std::find_if(m_liste.begin(), m_liste.end(), [&](const auto& x) { return x.get() == e; });
    if (it == m_liste.end()) { return false; }
    sichern();
    (*it)->abmeldenMitKindern();
    m_liste.erase(it);
    return true;
}

std::unique_ptr<Memento> EntitaetenListe::zustandSichern() const {
    auto m = std::make_unique<ListenMemento>();
    m->liste = m_liste;
    return m;
}

void EntitaetenListe::zustandLaden(const Memento& mm) {
    sichern();
    const auto& m = static_cast<const ListenMemento&>(mm);
    for (auto& e : m_liste) {
        if (std::find(m.liste.begin(), m.liste.end(), e) == m.liste.end()) { e->abmeldenMitKindern(); }
    }
    m_liste = m.liste;
    if (undoSystem()) {
        for (auto& e : m_liste) { e->anmeldenMitKindern(undoSystem()); }
    }
}

void EntitaetenListe::anmeldenMitAllem(Rueckgaengig* u) {
    anmelden(u);
    for (auto& e : m_liste) { e->anmeldenMitKindern(u); }
}

// ===================== Auswahl ==============================================

void Auswahl::waehlen(const std::shared_ptr<Entitaet>& e, const std::shared_ptr<Primitiv>& p) {
    if (gewaehlt(e.get(), p.get())) { return; }
    m_elemente.push_back(AuswahlElement{e, p});
    // der Zwischenspeicher ist nach gewaehlt() aktuell: das neue Element dazuzaehlen
    // (lock() ergibt e und p, beide leben) - Schleifen von waehlen() bleiben linear
    ++m_speicher[SpeicherSchluessel{e.get(), p.get()}];
}

void Auswahl::abwaehlen(const std::shared_ptr<Entitaet>& e, const std::shared_ptr<Primitiv>& p) {
    speicherUngueltig();
    m_elemente.erase(std::remove_if(m_elemente.begin(), m_elemente.end(),
                                    [&](const AuswahlElement& a) {
                                        return a.entitaet.lock() == e && a.primitiv.lock() == p;
                                    }),
                     m_elemente.end());
}

void Auswahl::umschalten(const std::shared_ptr<Entitaet>& e, const std::shared_ptr<Primitiv>& p) {
    if (gewaehlt(e.get(), p.get())) {
        abwaehlen(e, p);
    } else {
        waehlen(e, p);
    }
}

void Auswahl::umschaltenAlle(const std::vector<std::pair<std::shared_ptr<Entitaet>, std::shared_ptr<Primitiv>>>& paare) {
    speicherUngueltig();
    // umschalten() je Paar: gewaehlt (ein Element mit lock() == Paar) -> alle solchen
    // Elemente entfernen, sonst hinten anfuegen. Die Paare der Liste sind verschieden,
    // also sieht kein Paar ein Element, das ein anderes angefuegt hat - es zaehlt nur,
    // was vorher gewaehlt war. Ergebnis: die verbliebenen alten Elemente in ihrer
    // Reihenfolge, dahinter die neuen in der Reihenfolge der Liste - wie nacheinander.
    struct Schluessel {
        const Entitaet* e;
        const Primitiv* p;
        bool operator==(const Schluessel& o) const { return e == o.e && p == o.p; }
    };
    struct Streuung {
        std::size_t operator()(const Schluessel& s) const {
            return std::hash<const void*>{}(s.e) * 31U ^ std::hash<const void*>{}(s.p);
        }
    };
    std::vector<Schluessel> alt;
    alt.reserve(m_elemente.size());
    std::unordered_map<Schluessel, int, Streuung> vorher;
    for (const AuswahlElement& a : m_elemente) {
        alt.push_back({a.entitaet.lock().get(), a.primitiv.lock().get()});
        ++vorher[alt.back()];
    }
    std::unordered_map<Schluessel, int, Streuung> weg;
    std::vector<AuswahlElement> neu;
    for (const auto& [e, p] : paare) {
        const Schluessel s{e.get(), p.get()};
        if (vorher.count(s) != 0) {
            weg[s] = 1;
        } else {
            neu.push_back(AuswahlElement{e, p});
        }
    }
    std::vector<AuswahlElement> ergebnis;
    ergebnis.reserve(m_elemente.size() + neu.size());
    for (std::size_t i = 0; i < m_elemente.size(); ++i) {
        if (weg.count(alt[i]) == 0) { ergebnis.push_back(m_elemente[i]); }
    }
    for (AuswahlElement& a : neu) { ergebnis.push_back(std::move(a)); }
    m_elemente = std::move(ergebnis);
}

bool Auswahl::gewaehlt(const Entitaet* e, const Primitiv* p) const {
    // = any_of( m_elemente, lock(entitaet) == e && lock(primitiv) == p ), siehe karte.h
    if (!m_speicherGueltig || m_speicherStand != objekteZerstoert()) {
        m_speicher.clear();
        for (const AuswahlElement& a : m_elemente) { ++m_speicher[SpeicherSchluessel{a.entitaet.lock().get(), a.primitiv.lock().get()}]; }
        m_speicherGueltig = true;
        m_speicherStand = objekteZerstoert();
    }
    return m_speicher.count(SpeicherSchluessel{e, p}) != 0;
}

void Auswahl::flaecheWaehlen(const std::shared_ptr<Pinsel>& pinsel, const std::shared_ptr<Flaeche>& f) {
    if (flaecheGewaehlt(f.get())) { return; }
    m_flaechen.push_back(FlaechenWahl{pinsel, f});
}

void Auswahl::flaecheAbwaehlen(const Flaeche* f) {
    m_flaechen.erase(std::remove_if(m_flaechen.begin(), m_flaechen.end(),
                                    [&](const FlaechenWahl& w) { return w.flaeche.lock().get() == f; }),
                     m_flaechen.end());
}

bool Auswahl::flaecheGewaehlt(const Flaeche* f) const {
    return std::any_of(m_flaechen.begin(), m_flaechen.end(), [&](const FlaechenWahl& w) { return w.flaeche.lock().get() == f; });
}

void Auswahl::leeren() {
    speicherUngueltig();
    m_elemente.clear();
    m_flaechen.clear();
}

void Auswahl::flaechenLeeren() { m_flaechen.clear(); }

void Auswahl::entfernenWenn(const std::function<bool(const AuswahlElement&)>& weg) {
    speicherUngueltig();
    m_elemente.erase(std::remove_if(m_elemente.begin(), m_elemente.end(), weg), m_elemente.end());
}

void Auswahl::flaechenEntfernenWenn(const std::function<bool(const FlaechenWahl&)>& weg) {
    m_flaechen.erase(std::remove_if(m_flaechen.begin(), m_flaechen.end(), weg), m_flaechen.end());
}

void Karte::auswahlBereinigen() {
    const auto inSzene = [this](const Entitaet* e) {
        return std::any_of(entitaeten().begin(), entitaeten().end(), [e](const auto& x) { return x.get() == e; });
    };
    const auto kind = [](const Entitaet& e, const Primitiv* p) {
        return std::any_of(e.kinder().begin(), e.kinder().end(), [p](const auto& x) { return x.get() == p; });
    };
    const auto gesetzt = [](const std::weak_ptr<Primitiv>& w) {
        return w.owner_before(std::weak_ptr<Primitiv>{}) || std::weak_ptr<Primitiv>{}.owner_before(w);
    };
    auswahl.entfernenWenn([&](const AuswahlElement& a) {
        const auto e = a.entitaet.lock();
        if (!e || !inSzene(e.get())) { return true; }
        if (!gesetzt(a.primitiv)) { return false; }
        const auto p = a.primitiv.lock();
        return !p || !kind(*e, p.get());
    });
    auswahl.flaechenEntfernenWenn([&](const FlaechenWahl& w) {
        const auto p = w.pinsel.lock();
        const auto f = w.flaeche.lock();
        if (!p || !f) { return true; }
        const bool pinselDa = std::any_of(entitaeten().begin(), entitaeten().end(), [&](const auto& e) { return kind(*e, p.get()); });
        const bool flaecheDa = std::any_of(p->flaechen().begin(), p->flaechen().end(), [&](const auto& x) { return x.get() == f.get(); });
        return !pinselDa || !flaecheDa;
    });
}

void Auswahl::aufraeumen() {
    speicherUngueltig();
    // ein weak_ptr, der nie gesetzt war, hat keinen Besitzer
    auto gesetzt = [](const std::weak_ptr<Primitiv>& w) {
        return w.owner_before(std::weak_ptr<Primitiv>{}) || std::weak_ptr<Primitiv>{}.owner_before(w);
    };
    m_elemente.erase(std::remove_if(m_elemente.begin(), m_elemente.end(),
                                    [&](const AuswahlElement& a) {
                                        return a.entitaet.expired() || (gesetzt(a.primitiv) && a.primitiv.expired());
                                    }),
                     m_elemente.end());
    m_flaechen.erase(std::remove_if(m_flaechen.begin(), m_flaechen.end(),
                                    [](const FlaechenWahl& w) { return w.flaeche.expired() || w.pinsel.expired(); }),
                     m_flaechen.end());
}

// ===================== Karte ================================================

Karte::Karte() : m_wurzel(std::make_shared<EntitaetenListe>()) {
    schichten.push_back(Schicht{"0", -1, {0, 0, 0}});   // Layers(): Standardebene "0"
    m_wurzel->anmelden(&undo);
}

Karte::~Karte() {
    auswahl.leeren();
    undo.clear();
    for (const auto& e : m_wurzel->liste()) { e->abmeldenMitKindern(); }
    m_wurzel->abmelden();
}

EntitaetsKlasse* Karte::klasseFuer(const std::string& name, bool hatPinsel) {
    return (klassen ? klassen : &m_eigeneKlassen)->findenOderEinfuegen(name, hatPinsel);
}

// Quake3EntityCreator::createEntity -> node_for_eclass (entity.cpp:88-110)
std::shared_ptr<Entitaet> Karte::neueEntitaet(const std::string& klassenname, bool hatPinsel) {
    const EntitaetsKlasse* k = klasseFuer(klassenname, hatPinsel);
    auto e = std::make_shared<Entitaet>(k);
    e->setzen("classname", k->name);
    e->schicht = k->fixedsize ? static_cast<int>(aktuelleSchicht) : kSchichtKeine;
    return e;
}

void Karte::entitaetEinfuegen(std::shared_ptr<Entitaet> e) { m_wurzel->einfuegen(std::move(e)); }

bool Karte::entitaetEntfernen(const Entitaet* e) { return m_wurzel->entfernen(e); }

void Karte::entitaetLaden(std::shared_ptr<Entitaet> e) {
    // ohne Undo: die Liste wird erst nach dem Laden angemeldet
    Rueckgaengig* u = m_wurzel->undoSystem();
    m_wurzel->abmelden();
    m_wurzel->einfuegen(std::move(e));
    m_wurzel->anmelden(u);
}

void Karte::anmelden() { m_wurzel->anmeldenMitAllem(&undo); }

// Map_FindWorldspawn (map.cpp:492-511): erste Entitaet der Klasse "worldspawn"
std::shared_ptr<Entitaet> Karte::welt() const {
    for (const auto& e : m_wurzel->liste()) {
        if (e->klassenname() == "worldspawn") { return e; }
    }
    return nullptr;
}

// Map_FindOrInsertWorldspawn / createWorldspawn (map.cpp:539-554): neu als
// ERSTES Kind der Wurzel.
std::shared_ptr<Entitaet> Karte::weltHolenOderAnlegen() {
    if (auto w = welt()) { return w; }
    auto w = neueEntitaet("worldspawn", true);
    // Node_insertChildFirst: alle herausnehmen, worldspawn, dann die anderen wieder
    const EntitaetenListe::Liste alt = m_wurzel->liste();
    for (const auto& e : alt) { m_wurzel->entfernen(e.get()); }
    m_wurzel->einfuegen(w);
    for (const auto& e : alt) { m_wurzel->einfuegen(e); }
    return w;
}

void Karte::leeren() {
    auswahl.leeren();
    undo.clear();
    for (const auto& e : m_wurzel->liste()) { e->abmeldenMitKindern(); }
    m_wurzel->abmelden();
    m_wurzel = std::make_shared<EntitaetenListe>();
    m_wurzel->anmelden(&undo);
    schichten.clear();
    schichten.push_back(Schicht{"0", -1, {0, 0, 0}});
    aktuelleSchicht = 0;
}

}  // namespace bhed::karte
