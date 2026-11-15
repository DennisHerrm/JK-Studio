// cwimport.h - Karten-Editor, CW-IMPORT: Teil 2 der Clone-Wars-Kette (plaene/clonewars_kette.md) - eigene Funktionen, nicht in
// NetRadiant-Custom. Alles ab Werk aus bzw. nur auf Aufruf (shank 14.11.: "das ist wichtig das es zwar drin ist aber nicht standard").
//
// Punkt 1: die Testfigur - der Kasten, mit dem die Engine die Spielfigur bewegt, in Originalgroesse (Spielfigur aus optimierer.h:
// x/y -16 ... 16 in SP und Movie Duels, -15 ... 15 im Mehrspieler, z -24 ... 40, geduckt Oberkante 16, STEPSIZE 18 - OpenJK
// code/game/g_client.cpp:55-58, code/game/bg_public.h:51-53 und :71, Movie Duels Update 9 code/game/g_client.cpp:94-96,
// bg_public.h:76-78 und :93, codemp/game/g_client.c:31), der Ursprung in Hueftenhoehe; dazu die Hoehenmarken (STEPSIZE, die
// andere Hoehe) und die Mindestmasse fuer Tueren und Gaenge aus CW-PRUEF (CwMasse, Vorgabe 110 x 64 und 128 x 96).
//
// Portiert aus OpenJK sind nur diese Masse und die Sequenzen der Spielfigur (Stand BOTH_STAND1, geduckt BOTH_CROUCH1 wie
// code/game/bg_pmove.cpp PM_Footsteps), Movie Duels Update 9 an denselben Stellen; alles andere ist eigener Code.
//
//   Copyright (C) 1999 - 2005, Id Software, Inc.
//   Copyright (C) 2000 - 2013, Raven Software, Inc.
//   Copyright (C) 2013 - 2015, OpenJK contributors
//
//   OpenJK is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License version 2
//   as published by the Free Software Foundation (LICENSE).
#ifndef BHED_KARTE_CWIMPORT_H
#define BHED_KARTE_CWIMPORT_H

#include "bhed/karte/mathe.h"
#include "bhed/karte/optimierer.h"

#include <cstdint>
#include <string>
#include <vector>

namespace bhed::karte {

// --- Punkt 1: Testfigur ---------------------------------------------------------------------------------------------------------
enum class FigurArt : std::uint8_t {
    Kasten,     // der Kasten der Engine (mins/maxs)
    Hoehe,      // die andere Hoehe: stehend die Oberkante geduckt (16), geduckt die Oberkante stehend (40)
    Stufe,      // STEPSIZE ueber den Fuessen
    Tuer,       // Mindestmass einer Tuer (Rahmen um die Figur)
    Gang,       // Mindestmass eines Gangs
    Ursprung,   // der Ursprung (Hueftenhoehe)
};
struct FigurLinie {
    Vektor3f a;
    Vektor3f b;
    FigurArt art = FigurArt::Kasten;
};
struct FigurText {
    Vektor3f ort;
    std::string text;
    FigurArt art = FigurArt::Kasten;
};
struct FigurLage {
    Vektor3f ursprung{0, 0, 0};   // ps.origin
    bool geduckt = false;         // PMF_DUCKED: maxs[2] = CROUCH_MAXS_2
    bool quer = false;            // Tuer- und Gangrahmen quer (Ebene y = const statt x = const)
    bool marken = true;           // Hoehenmarken, Tuer und Gang
};
struct FigurZeichnung {
    std::vector<FigurLinie> linien;
    std::vector<FigurText> texte;
    Vektor3f mins{0, 0, 0};   // der Kasten in der Welt
    Vektor3f maxs{0, 0, 0};
};
[[nodiscard]] FigurZeichnung testfigurZeichnung(const FigurLage& l, const Spielfigur& f, const CwMasse& m);
// Der Ursprung, wenn die Fuesse auf `boden` stehen (Ursprung = Boden - mins[2])
[[nodiscard]] Vektor3f testfigurAufBoden(const Vektor3f& boden, const Spielfigur& f);
// Die Sequenz der Pose (animation.cfg) wie PM_Footsteps ohne Bewegung: geduckt BOTH_CROUCH1 (OpenJK code/game/bg_pmove.cpp:8180,
// Movie Duels Update 9 :14393), stehend ohne Schwert BOTH_STAND1 (OpenJK :8336)
[[nodiscard]] const char* testfigurSequenz(bool geduckt);

}  // namespace bhed::karte

#endif
