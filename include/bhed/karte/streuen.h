// streuen.h - Karten-Editor, Stufe 9 Punkt 8: Modelle verteilen (eigene Funktion, nicht in
// NetRadiant-Custom).
//
// Portiert aus OpenJK: die Grenze ist MAX_MISC_ENTS (code/cgame/cg_main.cpp: jedes
// misc_model_static wird ein cgame-"misc ent", beim 2001. Com_Error), begehbar heisst
// MIN_WALK_NORMAL 0.7 (bg_public.h); die Streuung selbst ist neu.
//
//   Copyright (C) 1999-2005 Id Software, Inc.; 2000-2013 Raven Software; 2013-2026 OpenJK contributors.
//   This program is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free Software
//   Foundation; either version 2 of the License, or (at your option) any later
//   version (LICENSE).
//
// Im Bereich (z. B. die gewaehlten Brushes) faellt je Versuch ein Punkt senkrecht auf die
// Kollision der Karte; ein Treffer auf einer begehbaren Flaeche, weit genug von den anderen,
// wird ein Ort. Hoechstens "erlaubt" Orte (der Rest bis zur Grenze, die der Optimierer zaehlt).
#ifndef BHED_KARTE_STREUEN_H
#define BHED_KARTE_STREUEN_H

#include "bhed/kollision.h"
#include "bhed/karte/mathe.h"

#include <string>
#include <vector>

namespace bhed::karte {

class Karte;

struct StreuOptionen {
    int anzahl = 20;
    float minAbstand = 48.0F;
    bool drehen = true;           // zufaelliger Gierwinkel ("angle")
    float skalaMin = 1.0F;        // "modelscale"
    float skalaMax = 1.0F;
    float minNormaleZ = 0.7F;     // MIN_WALK_NORMAL
    unsigned startwert = 1;
};

struct StreuOrt {
    Vektor3f ort;
    float gier = 0.0F;
    float skala = 1.0F;
};

struct StreuErgebnis {
    std::vector<StreuOrt> orte;
    bool begrenzt = false;        // die Grenze hat weniger erlaubt als gewuenscht
};

[[nodiscard]] StreuErgebnis streuOrte(const KollisionsKarte& koll, const Vektor3f& bereichMin, const Vektor3f& bereichMax, const StreuOptionen& o,
                                      int erlaubt);

// Die misc_model_static der Karte (fuer den Rest bis zur Grenze).
[[nodiscard]] int miscModelStaticZahl(const Karte& k);

// Einfuegen (ein Befehl "scatterModels"): je Ort ein Entity der Klasse mit "model" (und angle,
// modelscale); alle gewaehlt.
std::size_t streuEinfuegen(Karte& k, const StreuErgebnis& e, const std::string& klasse, const std::string& modell);

}  // namespace bhed::karte

#endif
