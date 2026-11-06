// modell/gewichte.h - Gewichte malen und fuers Spiel festlegen (Modell-Block 1a-2)
//
// Waehrend des Malens traegt jede Ecke eine eigene Liste (Netz::mal) mit
// beliebig vielen Knochen - wie die Vertexgruppen in Blender oder die
// Huellen des Skin-Modifiers in 3ds Max. Das Spiel kann das nicht: eine Ecke
// der .glm traegt hoechstens vier Knochen (iMAX_G2_BONEWEIGHTS_PER_VERT,
// OpenJK code/rd-common/mdx_format.h:85), gepackt in mdxmVertex_t:
//
//   uiNmWeightsAndBoneIndexes  Bit 30-31 Anzahl - 1, Bit 0-19 je Gewicht 5 Bit
//                              Knochenverweis, Bit 20-27 die oberen zwei Bit
//                              jedes Gewichts (mdx_format.h:302-308)
//   BoneWeightings[4]          die unteren acht Bit
//
// G2_GetVertBoneWeight (mdx_format.h:334-352) liest ein Gewicht als
// fG2_BONEWEIGHT_RECIPROCAL_MULT * iTemp (1/1023, 10 Bit) und das LETZTE nicht
// aus der Datei, sondern als 1 - Summe der vorigen. Die Summe ist im Spiel
// also immer 1.
//
// fuersSpiel() macht aus einer Malliste genau das: die vier groessten
// Gewichte (Blender "Limit Total" 4), auf 1 normiert, absteigend sortiert
// (so liegen sie in den Raven-Dateien: gemessen an allen 77 Modellen unter
// models/players von JKA, 116729 von 117100 Ecken mit mehr als einem Gewicht
// absteigend, die uebrigen bei gleichen Werten), die ersten n-1 auf 10 Bit
// gerundet, das letzte als Rest - und gelesen wie G2_GetVertBoneWeight. Was in
// der Ecke steht, ist danach bitgleich das, womit das Spiel die Ecke verformt.
//
//   Ported from OpenJK (codemp/rd-vanilla/G2_misc.cpp, tr_ghoul2.cpp, mdx_format.h; Copyright (C) 1999-2005 Id Software,
//   Inc., 2000-2013 Raven Software, Inc., 2013-2015 OpenJK contributors) and Blender (paint_vertex_weight_utils,
//   brush falloff; Copyright (C) Blender Authors). Both are licensed under the GNU General Public License version 2 (Blender:
//   or later). This file is part of BehavEd-Remake and licensed under the GNU General Public License version 2 or later
//   (LICENSE).
#ifndef BHED_MODELL_GEWICHTE_H
#define BHED_MODELL_GEWICHTE_H

#include <cstdint>
#include <string>
#include <vector>

#include "bhed/modell/dokument.h"

namespace bhed::modell {

// Ein Gewicht wie G2_GetVertBoneWeight: fG2_BONEWEIGHT_RECIPROCAL_MULT * iTemp
// (mit dem Kehrwert multipliziert, nicht geteilt - 24 der 1024 Werte waeren
// sonst um ein Bit anders).
[[nodiscard]] float g2Gewicht(int bits10) noexcept;

// Die Liste einer Ecke, wie die .glm sie traegt (nur Gewichte > 0).
[[nodiscard]] GewichtListe listeAus(const Ecke& e);

// Was die Datei fuer eine Ecke traegt: Anzahl, Knochen, die 10-Bit-Werte der
// ersten n-1 Gewichte und die Gewichte, wie das Spiel sie liest.
struct SpielGewichte {
    int zahl = 0;
    int knochen[4]{};
    int bits[4]{};
    float gewicht[4]{};
};
// false: keine Gewichte > 0 (die Ecke bleibt dann, wie sie ist).
[[nodiscard]] bool fuersSpiel(GewichtListe l, SpielGewichte& aus);
// Die Ecke bekommt die Gewichte des Spiels (Knochen 0..255).
bool aufEcke(const GewichtListe& l, Ecke& e);

// Die Mallisten eines Netzes anlegen (aus den Ecken), falls noch keine da sind.
void malAnlegen(Netz& n);
[[nodiscard]] inline bool malt(const Netz& n) noexcept { return !n.mal.empty(); }
// Ecken eines Netzes mit mehr als vier Gewichten in der Malliste.
[[nodiscard]] int eckenUeberVier(const Netz& n);
// Das Gewicht eines Knochens an einer Ecke (aus der Malliste, sonst aus der Ecke).
[[nodiscard]] float gewichtVon(const Netz& n, int ecke, int knochen);

enum class PinselArt : std::uint8_t { Hinzufuegen = 0, Abziehen, Glaetten, Setzen };
inline constexpr int kPinselArten = 4;

struct Tupfer {
    int ecke = -1;
    float staerke = 0.0F;   // Pinselstaerke mal Abfall (0..1)
};

// Ein Tupfer auf ein Netz (legt die Mallisten an):
//   Hinzufuegen  w + wert * s (hoechstens 1)      Blender "Add"
//   Abziehen     w - wert * s (mindestens 0)      Blender "Subtract"
//   Setzen       w + (wert - w) * s               Blender "Mix", Max "Set"
//   Glaetten     w + (Mittel der Nachbarn - w) * s, Nachbarn ueber die Punkte
//                (Ecken gleichen Orts zaehlen als eine), Werte von vor dem Tupfer
// Ecken desselben Punkts (UV-Naht) bekommen dasselbe. Rueckgabe: geaenderte Ecken.
int malen(Netz& n, const std::vector<Tupfer>& t, int knochen, PinselArt art, float wert);

// Die Abfallkurve des Pinsels: Blender "Smooth" (3p^2 - 2p^3, p = 1 - d/r).
[[nodiscard]] float abfall(float abstand, float radius) noexcept;

// Spiegeln ueber die Knochennamen: l_/r_ und _l/_r an Anfang und Ende, sonst
// ein einzelnes l/r am Anfang, wenn es den Partner im Skelett gibt (lhand <->
// rhand, lower_lumbar bleibt). -1: kein Partner (der Knochen bleibt).
[[nodiscard]] int spiegelKnochen(const std::vector<std::string>& namen, int knochen);

// Je Ecke einer Stufe ihr Spiegelpartner (x -> -x, Abstand <= toleranz) ueber
// alle Teile; {-1,-1} ohne Partner.
struct EckOrt {
    int teil = -1;
    int ecke = -1;
};
[[nodiscard]] std::vector<std::vector<EckOrt>> spiegelKarte(const Inhalt& in, int stufe, float toleranz);

// Die Gewichte der Seite `quelle` (+1: x > 0, -1: x < 0) auf die andere
// spiegeln (Knochen nach spiegelKnochen). Rueckgabe: geaenderte Ecken.
int spiegeln(Inhalt& in, int stufe, const std::vector<std::string>& knochenNamen, int quelle, float toleranz);

// Uebernehmen: jede Malliste aller Teile und Stufen nach fuersSpiel in die
// Ecken, die Mallisten verschwinden. Rueckgabe: Ecken.
int uebernehmen(Inhalt& in);

}  // namespace bhed::modell
#endif
