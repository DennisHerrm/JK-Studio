// eklasse.cpp - Karten-Editor: der Bestand der Entitaetsklassen.
//
// Portiert aus NetRadiant-Custom (Stand master, 2026): libs/eclasslib.h
// (Eclass_Alloc, EClass_Create, EntityClass_Create_Default, name_set,
// EntityClass_valueForKey) und radiant/eclass.cpp (EClass_InsertSortedList,
// Eclass_ForName, eclass_bad). Die Leser fuer .def und .ent stehen in
// eklasse_lesen.cpp.
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
#include "bhed/karte/eklasse.h"

#include <cctype>

namespace bhed::karte {

namespace {

bool beginntOhneFall(const std::string& s, const char* p) {   // string_equal_prefix_nocase
    std::size_t i = 0;
    for (; p[i] != '\0'; ++i) {
        if (i >= s.size() || std::tolower(static_cast<unsigned char>(s[i])) != std::tolower(static_cast<unsigned char>(p[i]))) {
            return false;
        }
    }
    return true;
}
bool endetOhneFall(const std::string& s, const char* p) {   // string_equal_suffix_nocase
    const std::string e(p);
    if (e.size() > s.size()) { return false; }
    for (std::size_t i = 0; i < e.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(s[s.size() - e.size() + i])) != std::tolower(static_cast<unsigned char>(e[i]))) {
            return false;
        }
    }
    return true;
}
bool gleichOhneFall(const std::string& a, const char* b) {   // classname_equal
    const std::string bb(b);
    if (a.size() != bb.size()) { return false; }
    return beginntOhneFall(a, b);
}

}  // namespace

// _stricmp / strcasecmp: beide Seiten klein (string_compare_nocase)
bool EntitaetsKlassen::OhneFall::operator()(const std::string& a, const std::string& b) const {
    const std::size_t n = std::min(a.size(), b.size());
    for (std::size_t i = 0; i < n; ++i) {
        const int ca = std::tolower(static_cast<unsigned char>(a[i]));
        const int cb = std::tolower(static_cast<unsigned char>(b[i]));
        if (ca != cb) { return ca < cb; }
    }
    return a.size() < b.size();
}

// name_set (eclasslib.h:126-130)
void EntitaetsKlasse::setzeName(const std::string& n) {
    name = n;
    miscmodelIs = (beginntOhneFall(name, "misc_") && endetOhneFall(name, "model")) || gleichOhneFall(name, "model_static");
}

// EntityClass_valueForKey (eclasslib.h:156-163)
std::string EntitaetsKlasse::wertFuer(const std::string& schluessel) const {
    for (const auto& [k, a] : attribute) {
        if (k == schluessel) { return a.wert; }
    }
    return {};
}

// EntityClass_Create_Default (eclasslib.h:313-320) mit EClass_Create /
// EClass_Create_FixedSize: Farbe (0, 0.5, 0), ohne Pinsel smallbox -8..8.
std::unique_ptr<EntitaetsKlasse> ersatzKlasse(const std::string& name, bool hatPinsel) {
    auto e = std::make_unique<EntitaetsKlasse>();
    e->setzeName(name);
    e->farbe = Vektor3f(0.0f, 0.5f, 0.0f);
    e->kommentar = "Not found in source.";
    if (!hatPinsel) {
        e->fixedsize = true;
        e->mins = Vektor3f(-8, -8, -8);
        e->maxs = Vektor3f(8, 8, 8);
    }
    return e;
}

EntitaetsKlassen::EntitaetsKlassen() {
    // EntityClassQuake3_construct: eclass_bad = EClass_Create( "UNKNOWN_CLASS", (0, 0.5, 0), "" )
    m_unbekannt = std::make_unique<EntitaetsKlasse>();
    m_unbekannt->setzeName("UNKNOWN_CLASS");
    m_unbekannt->farbe = Vektor3f(0.0f, 0.5f, 0.0f);
}

// EClass_InsertSortedList (eclass.cpp:70-76)
bool EntitaetsKlassen::einfuegen(std::unique_ptr<EntitaetsKlasse> k) {
    const std::string name = k->name;
    return m_klassen.emplace(name, std::move(k)).second;
}

// Eclass_ForName (eclass.cpp:215-230)
EntitaetsKlasse* EntitaetsKlassen::findenOderEinfuegen(const std::string& name, bool hatPinsel) {
    if (name.empty()) {
        return m_unbekannt.get();
    }
    auto it = m_klassen.find(name);
    if (it != m_klassen.end() && it->first == name) {
        return it->second.get();
    }
    // nicht gefunden (oder nur ohne Gross/klein gleich): Ersatz anlegen und
    // einfuegen - ist der Name ohne Gross/klein schon da, gewinnt die
    // vorhandene Klasse (EClass_InsertSortedList gibt sie zurueck)
    auto e = ersatzKlasse(name, hatPinsel);   // NRC setzt "unknown" nie (nur "Not found in source.")
    auto [pos, neu] = m_klassen.emplace(name, std::move(e));
    (void)neu;
    return pos->second.get();
}

const EntitaetsKlasse* EntitaetsKlassen::finden(const std::string& name) const {
    auto it = m_klassen.find(name);
    return it == m_klassen.end() ? nullptr : it->second.get();
}

void EntitaetsKlassen::listeEinfuegen(const std::string& name, const ListenTyp& liste) {
    m_listen.emplace(name, liste);   // g_listTypes.insert: vorhandene bleiben
}

const ListenTyp* EntitaetsKlassen::liste(const std::string& name) const {
    auto it = m_listen.find(name);
    return it == m_listen.end() ? nullptr : &it->second;
}

void EntitaetsKlassen::leeren() {
    m_klassen.clear();
    m_listen.clear();
}

std::vector<const EntitaetsKlasse*> EntitaetsKlassen::alle() const {
    std::vector<const EntitaetsKlasse*> v;
    v.reserve(m_klassen.size());
    for (const auto& [n, k] : m_klassen) {
        (void)n;
        v.push_back(k.get());
    }
    return v;
}

}  // namespace bhed::karte
