// modellbrowser.h - Karten-Editor: was der Modell-Browser rechnet (Ordnerbaum,
// Zellen, Lage der Modelle in ihren Zellen).
//
// Portiert aus NetRadiant-Custom 20260114: radiant/modelwindow.cpp:456-477
// (ModelFS), 479-525 (CellPos), 670-702 (models_set_transforms), 1149-1186
// (ModelFolders: "*ordner/tiefe*"), 1188-1214 (ModelPaths_ArchiveVisitor,
// ModelPaths_addFromArchive), 1216-1252 (ModelBrowser_constructTree).
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
#ifndef BHED_KARTE_MODELLBROWSER_H
#define BHED_KARTE_MODELLBROWSER_H

#include "bhed/karte/mathe.h"

#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace bhed::karte {

class Dateisystem;

// ModelFS: ein Ordner mit Unterordnern und Dateien (std::set, string_less)
struct ModellOrdner {
    std::string name;                               // m_folderName
    std::map<std::string, ModellOrdner> ordner;     // m_folders
    std::set<std::string> dateien;                  // m_files
    void einfuegen(const std::string& pfad);        // insert
};

// ModelFolders: "*ordner/tiefe*ordner2/tiefe*" (PathCleaned), "*/tiefe*" fuer
// die Wurzel; leer -> "models/" 99.
[[nodiscard]] std::map<std::string, std::size_t> modellOrdnerListe(const std::string& text);

// ModelBrowser_constructTree: die Modelldateien (Endungen ohne Gross/klein)
// aus allen Archiven in deren Reihenfolge, je Ordner der Liste bis zur Tiefe.
[[nodiscard]] ModellOrdner modellBaum(const Dateisystem& fs, const std::string& ordnerListe, const std::vector<std::string>& endungen);

// CellPos (Zellen: halbe Groesse zelle, Schrift hoehe/unterlaenge, 8 Rand links)
class ModellZellen {
public:
    ModellZellen(int breite, int zelle, int schriftHoehe, int schriftUnterlaenge);
    [[nodiscard]] Vektor3f ursprung(int index) const;   // getOrigin: Mitte des Modellquadrats (x, 0, -z)
    [[nodiscard]] Vektor3f textOrt(int index) const;    // getTextPos
    [[nodiscard]] int zelle() const { return m_zelle; }
    [[nodiscard]] int gesamtHoehe(int hoehe, int anzahl) const;   // totalHeight
    [[nodiscard]] int treffer(int x, int z) const;                // testSelect
    [[nodiscard]] int jeZeile() const { return m_jeZeile; }

private:
    int m_zelle;
    int m_schrift;
    int m_unterlaenge;
    int m_randLinks = 8;   // m_plusWidth
    int m_randOben = 0;    // m_plusHeight
    int m_jeZeile;
};

// models_set_transforms: das Modell (localAABB) in seine Zelle: auf die
// groesste Ausdehnung skaliert, nach der kleinsten (z doppelt gewichtet)
// gedreht, mittig auf ursprung( index ).
[[nodiscard]] Matrix4 modellInZelle(const Kasten& grenzen, const ModellZellen& z, int index);
// trackingDelta (linke Taste): die Drehung um den Zellmittelpunkt,
// Vector3( y, 0, x ) * ( 45 / zelle ) Grad, von links an die Lage multipliziert.
[[nodiscard]] Matrix4 modellDrehen(const Matrix4& lage, int dx, int dy, const ModellZellen& z, int index);

}  // namespace bhed::karte

#endif
