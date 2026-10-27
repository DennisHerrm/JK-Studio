// modellrueck.h - Karten-Editor Stufe 8a Punkt 3: misc_model aus der Geometrie
// einer fertigen .bsp zurueckgewinnen (eigene Funktion, nicht in NetRadiant-Custom).
//
// q3map2 backt jedes misc_model ein (model.cpp AddTriangleModels -> InsertModel):
// je Netz des Modells eine Zeichenflaeche mit den Ecken in der Reihenfolge, in der
// assimp sie liefert, je Ecke xyz = Transform * Modellpunkt (Transform = T(origin) *
// R(angles: Euler xyz, roll pitch yaw) * S(modelscale_vec), ModelGetTransform).
// Ohne -meta bzw. spawnflags 4 (forcemeta) bleibt die Flaeche eine Dreiecksflaeche
// (surface.cpp EmitTriangleSurface: MST_TRIANGLE_SOUP), ihre Ecken bleiben in
// derselben Reihenfolge (OptimizeTriangleSurface ordnet nur die Dreiecke um).
//
// Darum genau: die Netze der Modelle kommen von bhed_q3map2 selbst (-bhedmodels,
// dieselbe LoadModel-Funktion), je Flaeche der .bsp werden Netze mit gleicher
// Eckenzahl und gleicher Dreiecksmenge gesucht, die Abbildung Ecke fuer Ecke per
// Ausgleichsrechnung (affin, kleinste Quadrate) bestimmt und in Drehung und
// Skalierung zerlegt; nur Treffer mit Restfehler unter der Schwelle zaehlen.
// Netze desselben Modells mit derselben Abbildung bilden einen Treffer.
//
// Nicht erkennbar: forcemeta-Modelle (ihre Dreiecke verschmelzen mit anderen,
// MergeMetaTriangles), Modelle, deren Datei im Spiel fehlt. Solche Dreiecke werden
// gemeldet (Flaechen, die nicht auf einer Pinselseite liegen).
//
// Portiert aus q3map2 (NetRadiant-Custom 20260114: model.cpp InsertModel,
// ModelGetTransform - Lage, Winkel und Skalierung eines misc_model;
// surface_meta.cpp - wie forcemeta-Modelle in Meta-Flaechen aufgehen).
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   Copyright (C) 2026 BehavEd-Remake contributors.
//
//   This program is free software; you can redistribute it and/or modify it
//   under the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. This program is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#ifndef BHED_KARTE_MODELLRUECK_H
#define BHED_KARTE_MODELLRUECK_H

#include <array>
#include <cstdint>
#include <memory>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace bhed::karte {

class Pinsel;
struct Kontext;

// Ein Netz, wie InsertModel es sieht (aus bhed_q3map2 -bhedmodels).
struct ModellNetz {
    std::string shader;
    std::vector<std::array<float, 3>> punkte;
    std::vector<std::array<float, 2>> st;   // Texturkoordinaten (InsertModel ohne tcGen)
    std::vector<std::array<std::uint32_t, 3>> dreiecke;
};
struct ModellDatei {
    std::string pfad;
    int frame = 0;
    bool geladen = false;
    std::vector<ModellNetz> netze;
};
[[nodiscard]] bool modellDumpLesen(const std::string& daten, std::vector<ModellDatei>& out);

// Welche Modelldateien in Frage kommen: die Endungen, die q3map2 einbackt, und
// (Vorauswahl) Ordner, aus denen die .bsp Shader hat. pfade: alle Dateien des VFS.
[[nodiscard]] std::vector<std::string> modellKandidaten(const std::vector<std::string>& pfade, const std::vector<std::string>& bspShader);

// Stufe 8b Punkt 8: der Modell-Index ueber ALLE Modelle des Spiels (nicht nur die
// Ordner der BSP-Shader). Je Modell und Netz: Eckenzahl, Signatur der
// Dreiecksmenge (wie der erste Durchgang sie vergleicht) und der Shader (wie der
// zweite, forcemeta, ihn sucht). schluessel: woran der Editor erkennt, ob der
// Eintrag noch gilt (Archiv, Groesse, Zeit) - der Index wird zwischengespeichert.
struct ModellIndexNetz {
    std::uint32_t punkte = 0;
    std::uint64_t signatur = 0;
    std::string shader;   // klein, ohne Endung
};
struct ModellIndexEintrag {
    std::string pfad;
    std::string schluessel;
    bool geladen = false;
    std::vector<ModellIndexNetz> netze;
};
[[nodiscard]] ModellIndexEintrag modellIndexEintrag(const ModellDatei& m, const std::string& schluessel);
[[nodiscard]] std::string modellIndexSchreiben(const std::vector<ModellIndexEintrag>& index);
[[nodiscard]] bool modellIndexLesen(const std::string& text, std::vector<ModellIndexEintrag>& index);
// Die Dreiecksflaechen der .bsp als (Eckenzahl, Signatur) und ihre Shader (klein).
struct BspSuchmerkmale {
    std::vector<std::pair<std::uint32_t, std::uint64_t>> suppen;   // sortiert
    std::vector<std::string> shader;
};
[[nodiscard]] BspSuchmerkmale bspSuchmerkmale(const std::string& bspDaten);
// Kandidaten mit dem Index: die Ordnerregel (modellKandidaten) und dazu jedes
// geladene Modell des Index mit einem Netz, dessen (Eckenzahl, Signatur) eine
// Dreiecksflaeche der .bsp hat (Durchgang 1) oder dessen Shader die .bsp nutzt
// (Durchgang 2, forcemeta). Genau die Modelle, die die Suche finden kann - ausser
// forcemeta-Modellen mit _remap (deren Shader traegt die .bsp nicht).
[[nodiscard]] std::vector<std::string> modellKandidatenMitIndex(const std::vector<std::string>& pfade, const BspSuchmerkmale& bsp,
                                                                const std::vector<ModellIndexEintrag>& index);

// Eine Transformation wie ModelGetTransform: p' = origin + R(winkel) * (skala * p).
struct ModellLage {
    double origin[3]{0, 0, 0};
    double winkel[3]{0, 0, 0};   // pitch yaw roll (Grad), wie "angles"
    double skala[3]{1, 1, 1};
};
// Die 3x3-Matrix (Spalten = Achsen) und zurueck (Zerlegung in Drehung und Skalierung,
// negative Skalierung bei Spiegelung).
void lageMatrix(const ModellLage& l, double m[3][3]);
[[nodiscard]] ModellLage lageAusMatrix(const double m[3][3], const double t[3]);
// Die Schluessel, wie ein Mapper sie schreibt: "angle" oder "angles", "modelscale"
// oder "modelscale_vec" (nur wenn noetig), gerundet.
[[nodiscard]] std::vector<std::pair<std::string, std::string>> lageSchluessel(const ModellLage& l);
// Lage aus den Schluesseln einer Entitaet (ModelGetTransform)
[[nodiscard]] ModellLage lageAusSchluesseln(const std::vector<std::pair<std::string, std::string>>& kv);

struct ModellTreffer {
    std::string pfad;
    int bspModell = 0;              // 0 Welt, sonst Untermodell (misc_model mit "target")
    ModellLage lage;
    double rest = 0;                // mittlerer Abstand der Ecken (Einheiten)
    double maxFehler = 0;           // groesster Abstand einer Ecke
    std::vector<int> flaechen;      // Flaechen der .bsp
    int netzeGefunden = 0;
    int netzeGesamt = 0;
    std::vector<std::pair<std::string, std::string>> remaps;   // Shader des Modells -> Shader der .bsp (_remap)
    bool mehrdeutig = false;        // ein anderes Modell passte genauso
    std::string ziel;               // Untermodell: targetname der Brush-Entity ("target" des misc_model)
    bool forceMeta = false;         // im zweiten Durchgang (verschmolzene Meta-Dreiecke) gefunden: spawnflags 4
};
// Alle Schluessel des misc_model (classname, model, origin, angle(s), modelscale(_vec), target, _remap...)
[[nodiscard]] std::vector<std::pair<std::string, std::string>> trefferSchluessel(const ModellTreffer& t);
struct RueckOptionen {
    double schwelle = 0.1;          // groesster Eckenabstand in Einheiten
    double relSchwelle = 1e-4;      // dazu je Einheit Ausdehnung des Netzes
    std::function<bool(double anteil)> fortschritt;   // false = abbrechen
};
struct RueckErgebnisDreieck {
    double p[3][3];
    double st[3][2];
    std::string shader;
    int flaeche = -1;
};
struct RueckErgebnis {
    std::vector<ModellTreffer> treffer;
    std::vector<int> unerkannt;     // Flaechen mit Dreiecken ohne Treffer, die nicht auf Pinselseiten liegen
    using Dreieck = RueckErgebnisDreieck;
    std::vector<Dreieck> unerkannteDreiecke;
    std::size_t dreiecksflaechen = 0;
    bool abgebrochen = false;
};
// Die nicht erkannten Dreiecke als ebene, konvexe Vielecke (je Flaeche, Shader und Ebene
// zusammengelegt, solange die Texturkoordinaten affin bleiben): daraus macht der Editor
// je Vieleck einen duennen Pinsel. Die Texturkoordinate eines Punkts p in der Ebene:
// s = stS[0..2] . p + stS[3], t = stT[0..2] . p + stT[3].
struct ModellVieleck {
    std::vector<std::array<double, 3>> punkte;   // gegen den Uhrzeigersinn um normale
    double normale[3]{0, 0, 1};                  // die sichtbare Seite
    double stS[4]{0, 0, 0, 0};
    double stT[4]{0, 0, 0, 0};
    std::string shader;
};

// bspDaten: die ganze .bsp (RBSP); modelle: aus modellDumpLesen.
[[nodiscard]] RueckErgebnis modelleZurueckgewinnen(const std::string& bspDaten, const std::vector<ModellDatei>& modelle, const RueckOptionen& o);

[[nodiscard]] std::vector<ModellVieleck> unerkannteVielecke(const std::vector<RueckErgebnisDreieck>& dreiecke);
// Ein duenner Pinsel aus einem Vieleck: vorn der Shader mit den Texturkoordinaten der
// .bsp (texdefAusSt, Brush Primitives genau, Quake wie NRC angenaehert), hinten und an
// den Seiten rueckShader (nicht fest, unsichtbar), detail. dicke in Einheiten.
// nullptr, wenn das Vieleck entartet ist.
[[nodiscard]] std::shared_ptr<Pinsel> vieleckPinsel(const ModellVieleck& v, double dicke, const std::string& rueckShader, const Kontext& k);

}  // namespace bhed::karte

#endif
