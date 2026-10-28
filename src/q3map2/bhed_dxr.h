// bhed_dxr.h - BehavEd Stufe 8g Punkt 3: DXR als Vortest fuer Schattenstrahlen, Schalter
// -bhedgpudxr zusammen mit -bhedgpu (Messung, Vorgabe aus).
//
// Direct3D 12 (d3d12.dll, zur Laufzeit geladen) mit Inline-Raytracing (RayQuery, Shader Model
// 6.5, DXIL vorab mit dxc uebersetzt): eine Beschleunigungsstruktur aus q3map2s
// Trace-Dreiecken, die Schatten werfen koennen (ohne Himmel). Jeder Block Schattenstrahlen, den
// -bhedgpu auf der GPU rechnet, laeuft zusaetzlich durch die Hardware; gezaehlt werden die Zeit
// und wie oft die Hardware-Entscheidung (Dreieck getroffen ja/nein, ohne q3map2s Epsilons) mit
// dem genauen Weg uebereinstimmt. Das Ergebnis des Backens bleibt das des genauen Wegs.
// Ohne DXR (Raytracing-Stufe 1.1 fehlt, z. B. Intel UHD) meldet der Vortest das und tut nichts.
//
//   Copyright (C) 2026 BehavEd-Remake contributors.
//   This program is free software; you can redistribute it and/or modify it
//   under the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version (LICENSE).
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct BhedGpuStrahl;

inline bool g_bhedDxrWunsch = false;   // -bhedgpudxr
// Szene aufbauen (je Dreieck 9 float: drei Ecken); false: kein DXR (die Meldung steht in der Ausgabe)
bool BhedDxrStart( const std::vector<float>& ecken );
// Strahlen durch die Hardware: treffer[ i ] = 1, wenn ein Dreieck zwischen inhibit und dist liegt
bool BhedDxrStrahlen( const BhedGpuStrahl *strahlen, std::size_t n, std::vector<std::uint32_t>& treffer );
bool BhedDxrBereit();
long long BhedDxrMikros();
std::string BhedDxrAdapter();
void BhedDxrEnde();
