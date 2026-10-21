// eklasse.h - Karten-Editor: Entitaetsklassen (Farbe, Groesse, Modell,
// Spawnflags, Schluessel, Beschreibung) aus .def (QUAKED) und .ent (XML).
//
// Portiert aus NetRadiant-Custom (Stand master, 2026): libs/eclasslib.h,
// radiant/eclass.cpp (Klassenliste, Eclass_ForName, Ladereihenfolge, SP/MP-
// Filter), radiant/eclass_def.cpp (QUAKED-Leser), radiant/eclass_xml.cpp
// (.ent-Leser), include/ieclass.h.
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
#ifndef BHED_KARTE_EKLASSE_H
#define BHED_KARTE_EKLASSE_H

#include "bhed/karte/mathe.h"

#include <array>
#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace bhed::karte {

constexpr std::size_t kMaxFlags = 32;   // MAX_FLAGS (include/ieclass.h:30)

// EntityClassAttribute (eclasslib.h:65-76)
struct KlassenAttribut {
    std::string typ;
    std::string name;
    std::string wert;
    std::string beschreibung;
};

// ListAttributeType (eclasslib.h:36-63): Auswahlliste name -> wert
using ListenTyp = std::vector<std::pair<std::string, std::string>>;

// EntityClass (eclasslib.h:96-154)
struct EntitaetsKlasse {
    std::string name;
    std::vector<std::string> eltern;   // m_parent
    bool fixedsize = false;            // Punktentitaet mit fester Groesse
    bool unknown = false;              // nicht in den Definitionen gefunden
    bool miscmodelIs = false;          // misc_*model, model_static
    std::string miscmodelKey = "model";
    bool hasAngles = false;
    bool hasAnglesKey = false;
    bool hasDirectionKey = false;
    Vektor3f mins{1, 1, 1};            // Eclass_Alloc: mins (1,1,1), maxs (-1,-1,-1)
    Vektor3f maxs{-1, -1, -1};
    Vektor3f farbe;                    // color
    std::string kommentar;             // m_comments
    std::array<std::string, kMaxFlags> flagnamen{};   // flagnames (je 31 Zeichen)
    std::array<int, kMaxFlags> flagAttribut{};        // flagAttributes: Index in attribute, -1 = keins
    std::string modell;                // m_modelpath
    std::string skin;                  // m_skin
    std::vector<std::pair<std::string, KlassenAttribut>> attribute;   // m_attributes (Reihenfolge der Datei)
    bool vererbungAufgeloest = true;
    bool groesseAngegeben = false;
    bool farbeAngegeben = false;
    std::string quelle;                // Datei, aus der die Klasse stammt (fuer Meldungen)

    EntitaetsKlasse() { flagAttribut.fill(-1); }
    void setzeName(const std::string& n);   // name_set: setzt auch miscmodelIs
    // EntityClass_valueForKey
    [[nodiscard]] std::string wertFuer(const std::string& schluessel) const;
};

// Der Klassenbestand (g_entityClasses + EntityClassCollector). Namen werden
// ohne Gross/klein verglichen (RawStringLessNoCase); eine spaeter eingefuegte
// Klasse gleichen Namens wird verworfen (EClass_InsertSortedList).
class EntitaetsKlassen {
public:
    EntitaetsKlassen();

    // Eclass_ForName: findet die Klasse oder legt eine Ersatzklasse an
    // ("Not found in source.", Farbe 0 0.5 0; ohne Pinsel 16er-Wuerfel).
    // Leerer Name -> "UNKNOWN_CLASS" (eclass_bad, kein fester Kasten).
    EntitaetsKlasse* findenOderEinfuegen(const std::string& name, bool hatPinsel);
    [[nodiscard]] const EntitaetsKlasse* finden(const std::string& name) const;
    // EClass_InsertSortedList: false, wenn der Name schon da war
    bool einfuegen(std::unique_ptr<EntitaetsKlasse> k);
    void listeEinfuegen(const std::string& name, const ListenTyp& liste);   // EntityClassCollector::insert (Liste)
    [[nodiscard]] const ListenTyp* liste(const std::string& name) const;     // EntityClass_findListType
    void leeren();                                                           // Eclass_Clear

    // Eclass_ScanFile (eclass_def.cpp): QUAKED-Bloecke einer .def lesen.
    // Gibt die Zahl der Fehler ("Error parsing") zurueck.
    std::size_t defLesen(const std::string& text, const std::string& quelle);
    // ScanFile (eclass_xml.cpp): <classes> einer .ent lesen.
    std::size_t entLesen(const std::string& text, const std::string& quelle);

    // EntityClassQuake3_Construct + EntityClassesLoadFile: alle .def und .ent
    // eines Ordners in NRCs Reihenfolge (erst alle .def, dann alle .ent, je
    // nach Pfad sortiert); im Spielmodus "sp" werden Dateien, die mit "mp_"
    // beginnen, uebergangen, im Modus "mp" die mit "sp_" (eclass_filter_gamemode).
    std::size_t ordnerLaden(const std::string& ordner, const std::string& spielmodus);

    // Movie Duels: die Klassen aus bhed/entdefs.h (QUAKED-Fakten des
    // Movie-Duels-Quelltexts) als QUAKED-Text durch defLesen - nach den
    // JA-Klassen geladen, also nur die, die JA nicht kennt.
    std::size_t entdefsErgaenzen();

    // Alle Klassen in der Ordnung der Liste (Namen ohne Gross/klein sortiert).
    [[nodiscard]] std::vector<const EntitaetsKlasse*> alle() const;
    [[nodiscard]] std::size_t anzahl() const { return m_klassen.size(); }
    [[nodiscard]] const EntitaetsKlasse* unbekannt() const { return m_unbekannt.get(); }

private:
    struct OhneFall {
        bool operator()(const std::string& a, const std::string& b) const;
    };
    std::map<std::string, std::unique_ptr<EntitaetsKlasse>, OhneFall> m_klassen;
    std::map<std::string, ListenTyp> m_listen;
    std::unique_ptr<EntitaetsKlasse> m_unbekannt;   // eclass_bad
};

// EntityClass_Create_Default (eclasslib.h:313-320)
std::unique_ptr<EntitaetsKlasse> ersatzKlasse(const std::string& name, bool hatPinsel);

}  // namespace bhed::karte
#endif
