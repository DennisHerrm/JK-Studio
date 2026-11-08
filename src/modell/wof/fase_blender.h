// SPDX-License-Identifier: GPL-2.0-or-later
//
// modell/wof/fase_blender.h - Chamfer mit Segmenten wie Blenders Bevel (MRESTE 3b)
//
// Portiert aus Blender 5.0, source/blender/bmesh/tools/bmesh_bevel.cc (SPDX-FileCopyrightText: 2023 Blender Authors,
// SPDX-License-Identifier: GPL-2.0-or-later; https://projects.blender.org/blender/blender, Tag v5.0.0). Nur die reine
// Geometrie: das Profil einer Kante (set_profile_params, calculate_profile, get_profile_point), die gleichen Sehnen der
// Superellipse (find_even_superellipse_chords, set_profile_spacing, find_profile_fullness) und das Gitter einer Ecke mit
// drei und mehr Randpunkten (bevel_build_rings: adj_vmesh, cubic_subdiv, interp_vmesh, tri_corner_adj_vmesh,
// make_cube_corner_adj_vmesh, pipe_adj_vmesh). Die Netz-Seite (welche Punkte, welche Vielecke) bleibt in
// mesh_werkzeuge.cpp. Alle Orte liegen im Raum "Betrag 1": der Randpunkt i ist v + d_i; jede Formel ist affin und
// homogen um v, ein Ort bei Betrag a ist also v + a (P - v) (der Plan bewegt linear wie jedes Werkzeug).
#ifndef BHED_MODELL_WOF_FASE_BLENDER_H
#define BHED_MODELL_WOF_FASE_BLENDER_H

#include "mesh_intern.h"

#include <cstddef>
#include <vector>

namespace bhed::modell::wof::fase {

using mi::V;

// bmesh_bevel.cc:160-163
constexpr float kRQuadrat = 1e4F;        // PRO_SQUARE_R
constexpr float kRKreis = 2.0F;          // PRO_CIRCLE_R
constexpr float kRLinie = 1.0F;          // PRO_LINE_R
constexpr float kRQuadratInnen = 0.0F;   // PRO_SQUARE_IN_R

// Exponent der Superellipse aus dem Profil (Blender "Shape", Max "Tension"): bmesh_bevel.cc:7890 und 7928-7939
[[nodiscard]] float superR(float profil);

// ProfileSpacing (set_profile_spacing, find_profile_fullness): die Parameter der Profilpunkte fuer `seg` und fuer die
// Zweierpotenz seg2 >= seg (mindestens 4), mit der das Gitter gebaut wird
struct Abstaende {
    int seg = 0;
    int seg2 = 0;
    float r = kRKreis;
    float profil = 0.5F;
    float fuelle = 0.0F;
    std::vector<double> xs, ys, xs2, ys2;
};
[[nodiscard]] Abstaende abstaende(int seg, float profil);

// Profile (bmesh_bevel.cc struct Profile): von start nach ende, Mitte und Projektion
struct Profil {
    V start, mitte, ende;
    float r = kRKreis;
    V ebeneOrt, ebeneNormale, projRichtung;
    std::vector<V> punkte;    // seg + 1 (prof_co)
    std::vector<V> punkte2;   // seg2 + 1 (prof_co_2)
};

// Die gefaste Kante, an deren Ende das Profil liegt (BoundVert::ebev mit EdgeHalf prev/next)
struct KantenDaten {
    V v;                  // der alte Punkt (bv->v)
    V anderes;            // das andere Ende der Kante
    V dVor, dNach;        // Richtungen der Nachbarkanten am Start bzw. Ende des Profils (e->prev, e->next)
    bool vorGefast = false;
    bool nachGefast = false;
    int gewaehlt = 0;     // gefaste Kanten am Punkt (bv->selcount)
};

// set_profile_params fuer eine gefaste Kante bzw. ohne Kante (gerade), danach berechnen()
void kantenProfil(Profil& p, V start, V ende, const KantenDaten& k, float r);
void linienProfil(Profil& p, V start, V ende);
// move_weld_profile_planes: zwei gefaste Kanten treffen sich ohne Ecke (selcount 2, zwei Randpunkte)
void weldEbenen(Profil& p1, Profil& p2, V v);
// build_vmesh, Fall weld: die inneren Punkte als Mitte beider Profile (von p1.start nach p1.ende)
[[nodiscard]] std::vector<V> weldPunkte(const Profil& p1, const Profil& p2, const Abstaende& a);
// calculate_profile (ohne eigenes Profil, ohne Gehrung)
void berechnen(Profil& p, const Abstaende& a);
// get_profile_point: Punkt i von nseg (nseg == seg oder eine Zweierpotenz <= seg2)
[[nodiscard]] V profilPunkt(const Profil& p, int i, int nseg, const Abstaende& a);

// Das Gitter einer Ecke (VMesh, M_ADJ). Randpunkt i ist seiten[i].start, seiten[i] laeuft von i nach i + 1 (Blenders
// Reihenfolge: gegen den Uhrzeiger von aussen). co(i, j, k) mit 0 <= j <= ns/2, 0 <= k <= ns.
struct Gitter {
    int n = 0;
    int ns = 0;
    std::vector<V> orte;
    [[nodiscard]] V& at(int i, int j, int k) { return orte[static_cast<std::size_t>((i * (ns / 2 + 1) + j) * (ns + 1) + k)]; }
    [[nodiscard]] const V& at(int i, int j, int k) const { return orte[static_cast<std::size_t>((i * (ns / 2 + 1) + j) * (ns + 1) + k)]; }
    // mesh_vert_canon: der gleichwertige Eintrag, der den Ort fuehrt
    void kanon(int i, int j, int k, int& ki, int& kj, int& kk) const;
    [[nodiscard]] static bool istKanon(int ns, int i, int j, int k);
};

struct EckDaten {
    V v;
    std::vector<Profil> seiten;   // je Randpunkt das Profil zum naechsten (berechnet)
    int gewaehlt = 0;             // selcount
    int triTest = -1;             // tri_corner_test (braucht das Netz: Kantenwinkel), -1/0/1
    int pipe = -1;                // pipe_test: Index des Randpunkts vpipe, -1 keiner
    V pipeA, pipeB;               // die Enden der Kante vpipe->ebev
};
// bevel_build_rings: welches Gitter (square_out_adj_vmesh ist nicht portiert - dann adj_vmesh)
[[nodiscard]] Gitter eckenGitter(const EckDaten& e, const Abstaende& a);

}  // namespace bhed::modell::wof::fase

#endif
