// prefabverbund.h - Karten-Editor, Stufe 9 Punkt 6: verknuepfte Prefabs (eigene Funktion,
// nicht in NetRadiant-Custom).
//
// Portiert aus NetRadiant-Custom 20260114 sind nur die Bausteine (func_group, Brush::copy,
// Brush::transform mit Texture Lock, die Undo-Befehle); der Verbund selbst ist neu und folgt
// dem Spiegel-Modus der Stufe 8b (spiegel.h): innerhalb desselben Befehls abgleichen.
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   This program is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free Software
//   Foundation; either version 2 of the License, or (at your option) any later
//   version (LICENSE).
//
// Ein Verbund sind func_group-Entities mit demselben "_bhed_link"; jede hat ihren Ursprung in
// "_bhed_link_origin". Aendert ein Befehl den Inhalt eines Exemplars (relativ zu seinem
// Ursprung), schreibt abgleichen() alle anderen Exemplare neu (Kopien, um den Unterschied der
// Ursprunge verschoben, mit Texture Lock) - im selben Befehl ueber Rueckgaengig::vorAbschluss,
// also ein Rueckgaengig-Schritt. Wird ein ganzes Exemplar nur verschoben, wandert sein Ursprung
// mit (die anderen bleiben). Ausserhalb eines Befehls (Rueckgaengig, Wiederholen, Laden) wird
// nur der Stand gemerkt. q3map2 behandelt die func_group wie immer (in den worldspawn), die
// eigenen Schluessel stoeren nicht.
#ifndef BHED_KARTE_PREFABVERBUND_H
#define BHED_KARTE_PREFABVERBUND_H

#include "bhed/karte/mathe.h"

#include <map>
#include <memory>
#include <string>

namespace bhed::karte {

class Karte;
class Entitaet;

inline constexpr const char* kVerbundName = "_bhed_link";
inline constexpr const char* kVerbundUrsprung = "_bhed_link_origin";

// Die gewaehlten Pinsel/Patches (aus dem worldspawn) in eine neue func_group des Verbunds
// "name" mit Ursprung; ist schon eine Verbund-Gruppe gewaehlt, wird sie genommen. Danach eine
// Kopie um "versatz" daneben. Ein Befehl. Gibt die Kopie zurueck (nullptr: nichts gewaehlt).
std::shared_ptr<Entitaet> verbundKopieAnlegen(Karte& k, const std::string& name, const Vektor3f& versatz, float raster);

// Die gewaehlte Verbund-Gruppe loesen (Schluessel weg) - sie bleibt eine func_group.
bool verbundLoesen(Karte& k);

class PrefabVerbund {
public:
    // Jeden Frame mit befehlOffen = false (nur merken), aus vorAbschluss mit true (abgleichen).
    void abgleichen(Karte& k, bool befehlOffen);
    void vergessen() { m_stand.clear(); }
    [[nodiscard]] std::size_t exemplare() const { return m_stand.size(); }

private:
    struct Stand {
        std::weak_ptr<Entitaet> e;
        std::string name;
        std::string text;      // Inhalt relativ zum Ursprung
        Vektor3f ursprung;
        Vektor3f minimum;      // kleinste Ecke des Inhalts (fuer "nur verschoben")
    };
    std::map<const Entitaet*, Stand> m_stand;
    std::size_t m_aenderungen = static_cast<std::size_t>(-1);
};

}  // namespace bhed::karte

#endif
