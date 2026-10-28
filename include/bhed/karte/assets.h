// assets.h - Karten-Editor, Stufe 9 Punkt 11: Asset-Browser (eigene Funktion, nicht in
// NetRadiant-Custom).
//
// Portiert aus NetRadiant-Custom 20260114 sind die Bausteine (das virtuelle Dateisystem mit
// GetFileList, Map_SaveSelected/Map_ImportFile fuer Prefabs); der Browser ist neu.
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   This program is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free Software
//   Foundation; either version 2 of the License, or (at your option) any later
//   version (LICENSE).
//
// Modelle (models/**.md3/.glm/.ase/.obj), Klaenge (sound/**.wav/.mp3) aus dem Dateisystem des
// Spiels, Prefabs (.map) aus prefabs/ des Dateisystems und des Import-Ordners. Suche: alle Woerter
// (ohne Gross/klein) muessen im Pfad stehen. Vorschauen: der erste Shader eines MD3, die
// Wellenform eines Klangs, der Grundriss (Kaesten der Brushes) eines Prefabs.
#ifndef BHED_KARTE_ASSETS_H
#define BHED_KARTE_ASSETS_H

#include "bhed/karte/mathe.h"

#include <array>
#include <string>
#include <vector>

namespace bhed::sound {
struct Sound;
}

namespace bhed::karte {

class Dateisystem;

enum class AssetArt { Modell, Prefab, Klang };

struct Asset {
    AssetArt art = AssetArt::Modell;
    std::string pfad;   // Modell/Klang: Pfad im Spiel ("models/..."); Prefab: voller Dateipfad
    std::string name;   // Dateiname
};

[[nodiscard]] std::vector<Asset> assetsSammeln(const Dateisystem* fs, const std::string& importOrdner);
[[nodiscard]] std::vector<std::size_t> assetsFiltern(const std::vector<Asset>& a, AssetArt art, const std::string& suche);

[[nodiscard]] std::string md3ErsterShader(const std::string& md3Bytes);
[[nodiscard]] std::array<float, 64> wellenform(const sound::Sound& s);   // je Spalte der groesste Ausschlag 0..1

struct PrefabUmriss {
    std::vector<std::array<float, 4>> kaesten;   // x0 y0 x1 y1 je Brush (von oben)
    float x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    std::size_t pinsel = 0;
};
[[nodiscard]] PrefabUmriss prefabUmriss(const std::string& mapText);

}  // namespace bhed::karte

#endif
