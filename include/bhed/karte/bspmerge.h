// bspmerge.h - rc609 B1: Brush-Entities in fertige Karten einfuegen (Teil-Compile und BSP-Merge),
// Stufe 1: unsichtbare Brush-Entities (trigger_*, func_usable mit nodraw, Brushes mit origin).
//
// Ablauf (eigene Funktion, nicht in NetRadiant-Custom):
//   1. Im Karten-Editor bei geoeffneter .bsp neue Brush-Entities bauen.
//   2. Nur diese Entities kommen in eine leere Karte (worldspawn mit einem Platzhalter-Brush), q3map2 -bsp baut
//      sie (ProcessSubModel, bsp.cpp: Untermodelle ohne Flood-Fill, Lecks egal, kein Licht).
//   3. bspModelleAnhaengen haengt die Untermodelle des Teils als neue *N an die Original-.bsp:
//      models, brushes, brushsides, planes, shaders (vorhandene wiederverwendet wie EmitShader),
//      dazu je Modell ein eigenes Blatt mit seinen leafbrushes wie q3map2 (EndModel ->
//      EmitDrawNode_r -> EmitLeaf, writebsp.cpp), und die Entities mit "model" "*N" ins
//      Entity-Lump. Alle anderen Lumps (Baum, VIS, Lightmaps, Flaechen der Welt, Lichtgitter)
//      bleiben Byte fuer Byte, an die geaenderten wird nur angehaengt.
//
// Portiert aus q3map2 (NetRadiant-Custom 20260114): das Umrechnen der Indizes aus MergeBSPMain
// (convert_bsp.cpp, -mergebsp ohne -world), EmitShader und EmitLeaf (writebsp.cpp); aus OpenJK und
// Movie Duels 9 der Ladeweg und die Grenzen (cm_load.cpp, cm_local.h). Abweichungen von MergeBSPMain (je mit Grund):
//   - Shader: vorhandene Eintraege werden wiederverwendet (Regel von EmitShader, writebsp.cpp:
//     gleiche surfaceFlags, gleiche contentFlags, Name ohne Gross/klein); MergeBSPMain haengt alle an.
//   - Ebenen: nur die benutzten, als Paar (n, n^1 wie FindFloatPlane sie anlegt); ein Paar, das
//     Byte-gleich schon da ist, wird wiederverwendet. MergeBSPMain haengt alle an.
//   - surfaceNum einer Seite ohne Flaeche bleibt, wie EmitBrushes ihn schreibt (0); MergeBSPMain zaehlt auch dort
//     die Flaechenzahl auf (das Spiel liest den Wert nicht, CMod_LoadBrushSides).
//   - je Modell ein Blatt wie beim Bau (MergeBSPMain laesst die Blaetter weg).
//   - die Datei wird nicht neu geschrieben (WriteBSPFile packt das Lichtgitter neu), sondern mit
//     bspMitLumps: nur die geaenderten Lumps sind neu.
//
// Ladeweg im Spiel (OpenJK und Movie Duels 9, code/ und codemp/qcommon/cm_load.cpp):
// CMod_LoadSubmodels baut fuer jedes Modell > 0 ein eigenes "Blatt" aus firstBrush..numBrushes
// (die Blaetter des Lumps braucht das Spiel fuer Untermodelle nicht), CMod_LoadBrushes/-BrushSides/
// -Planes/-Shaders lesen die angehaengten Eintraege, SV_SetBrushModel -> CM_InlineModel(atoi(*N)).
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   Copyright (C) 1999 - 2005, Id Software, Inc.
//   Copyright (C) 2000 - 2013, Raven Software, Inc.
//   Copyright (C) 2001 - 2013, Activision, Inc.
//   Copyright (C) 2013 - 2015, OpenJK contributors
//   Copyright (C) 2026 BehavEd-Remake contributors.
//
//   This program is free software; you can redistribute it and/or modify it
//   under the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. This program is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#ifndef BHED_KARTE_BSPMERGE_H
#define BHED_KARTE_BSPMERGE_H

#include <cstdint>
#include <string>
#include <vector>

namespace bhed::karte {

class Entitaet;
class Karte;

// Fuer welches Spiel die Grenzen gelten.
enum class MergeSpiel { Jka, Jk2, MovieDuels };

// Hoechstzahl der Modelle (Welt + Untermodelle), die das Spiel laden und benutzen kann:
//   CMod_LoadSubmodels: count > MAX_SUBMODELS (512) -> ERR_DROP (OJK code/ und codemp/, MD9 ebenso;
//     JK2 laeuft in OpenJK ueber code/qcommon).
//   CM_ClipHandleToModel prueft handle < numSubModels VOR BOX_MODEL_HANDLE (MAX_SUBMODELS-1):
//     ab 512 Modellen traefe CM_TempBoxModel das Modell 511 statt des Kastens (SP und MP);
//     im MP liegt dazu CAPSULE_MODEL_HANDLE auf MAX_SUBMODELS-2 (codemp/qcommon/cm_local.h:32).
struct ModellGrenze {
    const char* name;   // "JKA SP", "JKA MP", ...
    int hoechstens;     // erlaubte Zahl der Modelle
    const char* quelle;
};
[[nodiscard]] std::vector<ModellGrenze> modellGrenzen(MergeSpiel s);

struct MergeMeldung {
    bool fehler = false;    // true: nicht eingefuegt
    std::string pruefung;   // "format", "sichtbar", "grenze", "ueberschneidung", "ziel"
    std::string text;       // englisch, mit Zahlen
};

struct MergeErgebnis {
    std::string bsp;                   // die gemischte Datei; "" bei Fehler
    int ersteNummer = 0;               // *N des ersten neuen Modells
    int neueModelle = 0;
    std::vector<std::string> klassen;  // classname je neuer Entity (Reihenfolge wie *N)
    std::vector<MergeMeldung> meldungen;
    [[nodiscard]] bool ok() const { return !bsp.empty(); }
    [[nodiscard]] bool hatFehler() const;
};

// Haengt die Untermodelle von teil (eine q3map2-.bsp; ihre Welt, Modell 0, bleibt weg) an original.
// spiel: welche Grenzen gelten. Ueberschneidungen mit soliden Welt-Brushes und Ziele (target), die
// es nicht gibt, kommen als Warnung; Fehler (kein Ergebnis): andere Kennung/Fassung, sichtbare
// Flaechen (Stufe 3), Grenze voll, kaputte Lumps. Clips in der Welt (Stufe 2) faengt der Karten-Editor ab.
[[nodiscard]] MergeErgebnis bspModelleAnhaengen(const std::string& original, const std::string& teil, MergeSpiel spiel);

// Ueberschneiden sich zwei konvexe Brushes (Halbraeume n . x <= d) mit mehr als epsilon Tiefe?
// Genau: eine Ecke des um epsilon verkleinerten Schnitts (drei Ebenen, alle anderen erfuellt).
struct Halbraum {
    double n[3]{};
    double d = 0.0;
};
[[nodiscard]] bool pinselUeberschneiden(const std::vector<Halbraum>& a, const std::vector<Halbraum>& b, double epsilon = 0.1);

// --- Karten-Editor: welche Brush-Entities sind neu? -----------------------------------------------
// Abdruck der Brushes einer Entity (ohne Schluessel), wie geometrieFingerabdruck ihn mischt.
[[nodiscard]] std::uint64_t pinselAbdruck(const Karte& k, const Entitaet& e);
// Die Abdruecke aller Brush-Entities ausser worldspawn und den eingebackenen (beim Oeffnen gemerkt).
[[nodiscard]] std::vector<std::uint64_t> pinselAbdruecke(const Karte& k);
// Neu = Brush-Entity (nicht worldspawn, nicht eingebacken), deren Abdruck in vorher nicht (mehr) frei ist.
[[nodiscard]] std::vector<const Entitaet*> neuePinselEntitaeten(const Karte& k, const std::vector<std::uint64_t>& vorher);
// Neue Punkt-Entities (ohne Brushes, nicht worldspawn, nicht eingebacken): ihr Abdruck (Schluessel)
// ist in vorher nicht (mehr) frei. Sie kommen mit in die Teilkarte (q3map2 schreibt sie wie beim Bau ins
// Entity-Lump des Teils, MergeBSPMain uebernimmt alle Entities des Teils) - so bekommt ein neuer
// Trigger sein neues Ziel (target_print, target_relay ...) im selben Schritt. geaendert: wie viele
// Punkt-Entities der Karte seit dem Oeffnen geaendert oder entfernt wurden (das kann nur
// "Update entities only").
[[nodiscard]] std::uint64_t entitaetAbdruck(const Karte& k, const Entitaet& e);
[[nodiscard]] std::vector<std::uint64_t> punktAbdruecke(const Karte& k);
[[nodiscard]] std::vector<const Entitaet*> neuePunktEntitaeten(const Karte& k, const std::vector<std::uint64_t>& vorher, int* geaendert = nullptr);
// Die Teilkarte: worldspawn (ihre Schluessel und als Platzhalter eine Kopie des ersten neuen Brushes - ohne
// Brush in der Welt ueberspringt q3map2 die worldspawn, ProcessModels, und die *N verrutschen) und die
// neuen Entities mit ihren Brushes.
[[nodiscard]] std::string teilKarte(const Karte& k, const std::vector<const Entitaet*>& neu);

}  // namespace bhed::karte

#endif
