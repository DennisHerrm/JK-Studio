// modell.h - Karten-Editor: Modelle fuer misc_model (Anzeige, Grenzen, Wahl).
//
// Portiert aus NetRadiant-Custom 20260114: radiant/nullmodel.cpp (NullModel:
// Kasten 8 mit "nomodel"), radiant/referencecache.cpp:182-202, 280-305
// (ModelResource_load: "Loaded Model:", "Model load failed:", "Model type not
// supported:"), plugins/md3model/md3.cpp:220-290 (MD3Surface_read: Bild 0,
// parseTextureName), plugins/entity/modelskinkey.h:31-33 (parseTextureName:
// PathCleaned( PathExtensionless( name ) )), plugins/entity/miscmodel.cpp:
// 55-118 (RemapKeysObserver: "_remap"-Schluessel, getRemap wie q3map2),
// 140-177 (MiscModel: model/angle/angles/modelscale/modelscale_vec,
// updateTransform: matrix4_transform_by_euler_xyz_degrees),
// plugins/assmodel/model.cpp:217-263 (CopyPicoSurface: der Shadername aus
// Material und Diffuse-Bild), libs/picomodel/pm_ase.c und picointernal.c
// (das ASE-Format wie q3map2: Materialien, Untermaterialien, Ecken, Flaechen
// mit umgekehrter Reihenfolge, t = 1 - t), tools/quake3/q3map2/model.cpp:
// 1227-1301, 1571-1581 (InsertModel: "_skin"/"skin" und die .skin-Datei).
//
// Ueber NRC hinaus (wie im Spiel): .glm-Modelle in ihrer Grundhaltung mit
// model_default.skin (NRC laedt .glm nicht) und die .skin-Datei von q3map2
// fuer "_skin"/"skin".
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
#ifndef BHED_KARTE_MODELL_H
#define BHED_KARTE_MODELL_H

#include "bhed/karte/mathe.h"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace bhed::karte {

class Entitaet;
class AuswahlVolumen;
class Treffer;

struct ModellPunkt {   // ArbitraryMeshVertex
    Vektor3f ort{0, 0, 0};
    Vektor3f normale{0, 0, 1};
    Vektor2f st{0, 0};
};
struct ModellFlaeche {   // Surface / PicoSurface
    std::string shader;
    std::vector<ModellPunkt> punkte;
    std::vector<std::uint32_t> indizes;   // je drei ein Dreieck
    Kasten grenzen;                       // localAABB
};
struct Modell {
    std::vector<ModellFlaeche> flaechen;
    Kasten grenzen;                       // localAABB (Vereinigung der Flaechen)
    bool null = false;                    // NullModel
};

// NullModel: Kasten (0 0 0) +- 8 mit dem Shader "nomodel"
[[nodiscard]] Modell nullModell();
// parseTextureName: Rueckstriche zu '/', ohne Endung
[[nodiscard]] std::string texturName(const std::string& name);

// Die Lader (nach der Endung): .md3 (Bild 0), .ase, .glm (Grundhaltung,
// skinText = Inhalt von model_default.skin oder leer). false = nicht lesbar.
bool md3Lesen(const std::string& bytes, Modell& m, std::string* fehler = nullptr);
bool aseLesen(const std::string& text, const std::string& modellPfad, Modell& m, std::string* fehler = nullptr);
bool glmLesen(const std::string& bytes, const std::string& skinText, Modell& m, std::string* fehler = nullptr);
// Kennt der Editor die Endung (ModelLoader_forType)?
[[nodiscard]] bool modellArtBekannt(const std::string& pfad);

// q3map2 InsertModel: der Name der .skin-Datei fuer "_skin"/"skin" (Ziffern:
// <modell>_<n>.skin, sonst <modell ohne Endung>_<name>.skin) und ihr Inhalt
// ("replace a b" oder "a,b"; ungueltige Zeilen werden verworfen).
using Umbenennungen = std::vector<std::pair<std::string, std::string>>;
[[nodiscard]] std::string skinDateiName(const std::string& modell, const std::string& skin);
[[nodiscard]] Umbenennungen skinLesen(const std::string& text, std::vector<std::string>* verworfen = nullptr);
// RemapKeysObserver::getRemap: "_remap*" = "von;nach" (nach mit PathCleaned);
// "*" nur, wenn nichts passt; die laengste Endung gewinnt. "" = keine.
[[nodiscard]] std::string remapFuer(const Entitaet& e, const std::string& shader);

// Was eine misc_model-Entitaet zeigt: das Modell und je Flaeche der Shader
// nach Skin und _remap ("" = die Skin bildet die Flaeche nicht ab -> fehlt).
struct ModellAnsicht {
    std::shared_ptr<const Modell> modell;
    std::vector<std::string> shader;
};
// Die Quelle der Modelle (die Oberflaeche: Dateisystem und Ablage)
class ModellQuelle {
public:
    virtual ~ModellQuelle() = default;
    [[nodiscard]] virtual ModellAnsicht ansicht(const Entitaet& e) const = 0;
};
// MiscModel::updateTransform: Ursprung, angle/angles, modelscale(_vec)
[[nodiscard]] Matrix4 miscModellMatrix(const Entitaet& e);
// Instance::worldAABB des Modells (aabb_for_oriented_aabb)
[[nodiscard]] Kasten modellWeltKasten(const ModellAnsicht& a, const Matrix4& lokalZuWelt);
// PicoModelInstance::testSelect / NullModel::testSelect
void modellTesten(const ModellAnsicht& a, const Matrix4& lokalZuWelt, AuswahlVolumen& test, Treffer& bester);

}  // namespace bhed::karte

#endif
