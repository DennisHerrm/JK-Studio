// spiegel.h - Karten-Editor, Stufe 8b Punkt 6: der Spiegel-Modus (nicht in
// NetRadiant-Custom). Portiert aus NetRadiant-Custom ist nur das Spiegeln
// selbst - wie "Flip" (Select_FlipAxis -> scaleSelected mit -1, das ueber
// Brush::transform / Patch::transform mit Texture Lock und Umkehr der
// Flaechen bzw. Zeilen laeuft); der Modus, die Paare und der Abgleich sind
// eigener Code.
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   For a list of contributors, see the accompanying CONTRIBUTORS file.
//   This file is part of GtkRadiant.
//
//   GtkRadiant is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version (LICENSE).
//
// Ablauf: Eine Spiegelebene senkrecht zu X, Y oder Z durch einen Punkt. Jedes
// gewaehlte Objekt (Pinsel oder Patch einzeln, Punktentitaet oder Gruppe als
// Ganzes) bekommt ein Gegenstueck: ein schon vorhandenes, das genau seinem
// Spiegelbild entspricht, oder - bei der ersten Aenderung - eine gespiegelte
// Kopie. Aendert ein Befehl eine Seite eines Paares, schreibt abgleichen() die
// andere Seite neu, und zwar innerhalb desselben Befehls (live beim Ziehen,
// sonst kurz vor dem Abschluss ueber Rueckgaengig::vorAbschluss) - so bleibt
// es ein Rueckgaengig-Schritt, und Rueckgaengig stellt beide Seiten her.
// Ausserhalb eines Befehls (Rueckgaengig, Wiederholen) wird nur der neue Stand
// gemerkt.
#ifndef BHED_KARTE_SPIEGEL_H
#define BHED_KARTE_SPIEGEL_H

#include "bhed/karte/mathe.h"

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace bhed::karte {

class Karte;
class Entitaet;
class Primitiv;
struct Kontext;

// Die Spiegelmatrix x' = 2*ebene - x auf der Achse (0 x, 1 y, 2 z).
[[nodiscard]] Matrix4 spiegelMatrix(int achse, double ebene);
// Gespiegelte Kopie eines Pinsels/Patches (wie Flip: Texture Lock nach k).
[[nodiscard]] std::shared_ptr<Primitiv> primitivGespiegelt(const Primitiv& p, int achse, double ebene, const Kontext& k);
// Die Schluessel einer Entitaet gespiegelt: origin, angle, angles. Die Lage der
// Schluessel bleibt; "angle" -1/-2 (hoch/runter) tauscht nur die Z-Ebene.
[[nodiscard]] std::vector<std::pair<std::string, std::string>> schluesselGespiegelt(
    const std::vector<std::pair<std::string, std::string>>& s, int achse, double ebene);

class SpiegelModus {
public:
    bool an = false;     // eigener Schalter, Vorgabe aus (NRC hat keinen)
    int achse = 0;       // 0 X, 1 Y, 2 Z
    double ebene = 0;    // Lage der Ebene auf der Achse (ein Rasterpunkt)
    // Hoechstzahl gewaehlter Objekte, die der Modus verfolgt (darueber: keine neuen Paare).
    std::size_t hoechstens = 1024;

    // Jeden Frame mit befehlOffen = k.undo.befehlOffen(), und aus
    // Rueckgaengig::vorAbschluss mit true.
    void abgleichen(Karte& k, bool befehlOffen);
    void vergessen();   // alle Paare und gemerkten Staende weg (Karte neu, Ebene geaendert)

    [[nodiscard]] std::size_t paarAnzahl() const { return m_paare.size(); }
    // Fuer Tests und Anzeige: das Gegenstueck eines Objekts (nullptr: keins).
    [[nodiscard]] std::shared_ptr<Primitiv> gegenstueck(const Primitiv* p) const;
    [[nodiscard]] std::shared_ptr<Entitaet> gegenstueck(const Entitaet* e) const;
    // Gewaehlte Objekte, die schon ihr eigenes Spiegelbild sind (bekommen kein Gegenstueck).
    [[nodiscard]] std::size_t selbstSymmetrisch() const;
    // Uebertragungen seit dem Start (fuer Tests).
    [[nodiscard]] std::size_t uebertragungen() const { return m_uebertragungen; }

private:
    struct Seite {
        std::weak_ptr<Entitaet> e;   // Eltern (Primitiv) bzw. die Entitaet selbst
        std::weak_ptr<Primitiv> p;   // leer: die ganze Entitaet
        std::string text;            // gemerkter Stand (Kartentext)
    };
    struct Paar {
        Seite a, b;
    };
    struct Einzel {
        std::weak_ptr<Entitaet> e;
        std::weak_ptr<Primitiv> p;
        std::string text;
        bool selbst = false;
        bool gesehen = false;
    };
    void paarBilden(Karte& k, const Seite& a, const Seite& b);

    std::vector<Paar> m_paare;
    std::unordered_map<const void*, Einzel> m_einzeln;
    std::size_t m_aenderungen = static_cast<std::size_t>(-1);
    int m_achseAlt = -1;
    double m_ebeneAlt = 0;
    bool m_laeuft = false;
    std::size_t m_uebertragungen = 0;
};

}  // namespace bhed::karte
#endif
