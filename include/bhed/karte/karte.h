// karte.h - Karten-Editor: die Karte als Baum aus Entitaeten mit Pinseln und
// Patches, Ebenen ("Layers"), Auswahl und Rueckgaengig.
//
// Portiert aus NetRadiant-Custom (Stand master, 2026): libs/entitylib.h
// (EntityKeyValues: geordnete Schluessel, setKeyValue), plugins/entity/
// entity.cpp (node_for_eclass: "classname" zuerst), libs/traverselib.h
// (TraversableNodeSet mit Undo), libs/layers.h (Layer, Layers), radiant/
// selection.cpp / include/iselection.h (Auswahl von Primitiven und Flaechen,
// Auswahlreihenfolge).
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
//
// Fuer Stufe 1b: alle Aenderungen laufen ueber Methoden, die vorher sichern()
// rufen - eine Oberflaeche klammert sie in Befehl(karte.undo, "name") und
// bekommt Rueckgaengig/Wiederholen ohne eigenes Zutun. Die Auswahl kennt
// Entitaeten, Primitive und einzelne Flaechen (NRC: Primitive-, Entity- und
// Komponentenmodus Face); Kanten und Ecken liefern Pinsel::kanten()/ecken().
#ifndef BHED_KARTE_KARTE_H
#define BHED_KARTE_KARTE_H

#include "bhed/karte/eklasse.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/rueckgaengig.h"

#include <array>
#include <functional>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace bhed::karte {

// Layer (libs/layers.h). Die Karte haelt die Ebenen flach in der Reihenfolge,
// in der NRC sie schreibt (Tiefensuche, Layers::update_ownIndices); eltern ist
// der Index der Elternebene, -1 = Wurzel (LAYERIDXPARENT).
struct Schicht {
    std::string name;
    int eltern = -1;
    std::array<int, 3> farbe{0, 0, 0};   // 0 0 0 = Standardfarbe der Oberflaeche
};

class Entitaet final : public Rueckgaengigbar {
public:
    using Schluessel = std::vector<std::pair<std::string, std::string>>;
    using Kinder = std::vector<std::shared_ptr<Primitiv>>;

    explicit Entitaet(const EntitaetsKlasse* klasse);

    // --- EntityKeyValues ---
    [[nodiscard]] const Schluessel& schluessel() const { return m_schluessel; }
    [[nodiscard]] std::string wert(const std::string& schluessel) const;   // getKeyValue ("" wenn nicht da)
    [[nodiscard]] bool hat(const std::string& schluessel) const;           // hasKeyValue
    // setKeyValue: leerer Wert loescht, vorhandener Schluessel wird an seiner
    // Stelle ueberschrieben, neuer wird angehaengt. Mit Undo.
    void setzen(const std::string& schluessel, const std::string& wert);
    // getClassName: Name der Entitaetsklasse (nicht der Schluessel!)
    [[nodiscard]] const std::string& klassenname() const;
    [[nodiscard]] const EntitaetsKlasse* klasse() const { return m_klasse; }
    // isContainer = !eclass->fixedsize
    [[nodiscard]] bool istBehaelter() const { return m_behaelter; }

    // --- Kinder (TraversableNodeSet) ---
    [[nodiscard]] const Kinder& kinder() const { return m_kinder; }
    void kindEinfuegen(std::shared_ptr<Primitiv> p);   // insert (am Ende)
    bool kindEntfernen(const Primitiv* p);             // erase

    // Node::m_layer: NRC setzt ihn beim Lesen nur fuer Punktentitaeten
    // (parse.cpp:236-237); Gruppen haben keine (kSchichtKeine).
    int schicht = kSchichtKeine;
    // Node::eHidden (Select_Hide, select.cpp:939-975) - nicht im Undo.
    bool versteckt = false;
    // Node::eExcluded (Region: exclude_node, map.cpp:1369-1373) - nicht im Undo.
    bool ausgeschlossen = false;
    // Node::eFiltered (ClassnameFilter::updateFiltered, filter.h) - nicht im Undo.
    bool gefiltert = false;
    // Node::visible (scenelib.h:220-222): weder versteckt noch gefiltert noch ausgeschlossen.
    [[nodiscard]] bool sichtbar() const { return !versteckt && !ausgeschlossen && !gefiltert; }

    [[nodiscard]] std::unique_ptr<Memento> zustandSichern() const override;
    void zustandLaden(const Memento& m) override;

    void anmeldenMitKindern(Rueckgaengig* u);
    void abmeldenMitKindern();

private:
    Schluessel m_schluessel;
    Kinder m_kinder;
    const EntitaetsKlasse* m_klasse;
    bool m_behaelter;
};

// Auswahl (SelectionSystem): ein Element ist eine Entitaet als Ganzes
// (Punktentitaet, oder Gruppe im Entity-Modus) oder ein Primitiv darin.
struct AuswahlElement {
    std::weak_ptr<Entitaet> entitaet;
    std::weak_ptr<Primitiv> primitiv;   // leer: die ganze Entitaet
};
struct FlaechenWahl {
    std::weak_ptr<Pinsel> pinsel;
    std::weak_ptr<Flaeche> flaeche;
};

class Auswahl {
public:
    enum class Modus { Primitive, Entitaeten, Flaechen, Kanten, Ecken };   // EMode / EComponentMode

    Modus modus = Modus::Primitive;

    void waehlen(const std::shared_ptr<Entitaet>& e, const std::shared_ptr<Primitiv>& p = nullptr);
    void abwaehlen(const std::shared_ptr<Entitaet>& e, const std::shared_ptr<Primitiv>& p = nullptr);
    void umschalten(const std::shared_ptr<Entitaet>& e, const std::shared_ptr<Primitiv>& p = nullptr);
    [[nodiscard]] bool gewaehlt(const Entitaet* e, const Primitiv* p = nullptr) const;
    void flaecheWaehlen(const std::shared_ptr<Pinsel>& pinsel, const std::shared_ptr<Flaeche>& f);
    void flaecheAbwaehlen(const Flaeche* f);
    [[nodiscard]] bool flaecheGewaehlt(const Flaeche* f) const;
    void leeren();             // alles abwaehlen ("Esc")
    void flaechenLeeren();
    // Verwaiste Eintraege (geloeschte Objekte) entfernen.
    void aufraeumen();
    // Eintraege entfernen, fuer die "weg" wahr ist (Karte::auswahlBereinigen).
    void entfernenWenn(const std::function<bool(const AuswahlElement&)>& weg);
    void flaechenEntfernenWenn(const std::function<bool(const FlaechenWahl&)>& weg);

    // in Auswahlreihenfolge (NRC: SelectionSystem::ultimateSelected = letzte)
    [[nodiscard]] const std::vector<AuswahlElement>& elemente() const { return m_elemente; }
    [[nodiscard]] const std::vector<FlaechenWahl>& flaechen() const { return m_flaechen; }
    [[nodiscard]] std::size_t anzahl() const { return m_elemente.size(); }

private:
    std::vector<AuswahlElement> m_elemente;
    std::vector<FlaechenWahl> m_flaechen;
};

// Die Liste der Entitaeten (Wurzelknoten der Karte, TraversableNodeSet).
class EntitaetenListe final : public Rueckgaengigbar {
public:
    using Liste = std::vector<std::shared_ptr<Entitaet>>;
    [[nodiscard]] const Liste& liste() const { return m_liste; }
    void einfuegen(std::shared_ptr<Entitaet> e);
    bool entfernen(const Entitaet* e);
    [[nodiscard]] std::unique_ptr<Memento> zustandSichern() const override;
    void zustandLaden(const Memento& m) override;
    void anmeldenMitAllem(Rueckgaengig* u);
private:
    Liste m_liste;
};

class Karte {
public:
    Karte();
    ~Karte();
    Karte(const Karte&) = delete;
    Karte& operator=(const Karte&) = delete;

    Kontext kontext;            // Pinselformat, Texturgroessen, Texture Lock
    EntitaetsKlassen* klassen = nullptr;   // Entitaetsklassen des Spiels (nicht im Besitz)
    Rueckgaengig undo;
    Auswahl auswahl;
    // Layers: mindestens die Standardebene "0" (Layers::Layers).
    std::vector<Schicht> schichten;
    std::size_t aktuelleSchicht = 0;   // Layers::m_currentLayer

    [[nodiscard]] const EntitaetenListe::Liste& entitaeten() const { return m_wurzel->liste(); }
    // Neue Entitaet der Klasse "klassenname" (node_for_eclass: classname zuerst).
    std::shared_ptr<Entitaet> neueEntitaet(const std::string& klassenname, bool hatPinsel);
    void entitaetEinfuegen(std::shared_ptr<Entitaet> e);   // mit Undo
    bool entitaetEntfernen(const Entitaet* e);             // mit Undo
    // Die erste Entitaet mit der Klasse "worldspawn" (Map_FindWorldspawn).
    [[nodiscard]] std::shared_ptr<Entitaet> welt() const;
    // Map_FindOrInsertWorldspawn: legt worldspawn an, wenn es fehlt.
    std::shared_ptr<Entitaet> weltHolenOderAnlegen();

    // Fuer den Leser: ohne Undo und ohne Anmeldung einhaengen.
    void entitaetLaden(std::shared_ptr<Entitaet> e);
    // Nach dem Laden einmal: alle Objekte beim Undo anmelden (instanceAttach).
    void anmelden();
    void leeren();   // neue, leere Karte (nur Ebene "0")
    // Nach Rueckgaengig/Wiederholen: was nicht mehr in der Szene ist, ist
    // nicht mehr gewaehlt (NRC: die Instanz wird zerstoert und abgewaehlt).
    void auswahlBereinigen();

    // Klasse fuer einen Namen (Eclass_ForName ueber "klassen" oder eigener Vorrat).
    EntitaetsKlasse* klasseFuer(const std::string& name, bool hatPinsel);

private:
    std::shared_ptr<EntitaetenListe> m_wurzel;
    EntitaetsKlassen m_eigeneKlassen;   // wenn "klassen" nicht gesetzt ist
};

}  // namespace bhed::karte
#endif
