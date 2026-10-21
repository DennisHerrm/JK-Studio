// raster.cpp - Karten-Editor: das Raster wie NetRadiant-Custom.
//
// Portiert aus NetRadiant-Custom (Stand master, 2026): radiant/grid.cpp.
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
#include "bhed/karte/raster.h"

#include <algorithm>
#include <cmath>

namespace bhed::karte {

namespace {

// g_gridnames (grid.cpp:72-87): this must match the GridPower enumeration
const char* const kNamen[] = {"0.125", "0.25", "0.5", "1", "2", "4", "8", "16", "32", "64", "128", "256", "512", "1024"};

// GridSize_forGridPower (grid.cpp:122): pow( 2.0f, gridPower )
float groesseFuer(int potenz) { return std::pow(2.0F, static_cast<float>(potenz)); }

}  // namespace

const char* Raster::name(int menueIndex) {
    return (menueIndex >= 0 && menueIndex < 14) ? kNamen[menueIndex] : "";
}

// Grid_construct (grid.cpp:266-276)
void Raster::standardSetzen(int gridDefault) {
    m_potenz = gridDefault - 3;   // GridPower_forGridDefault
    m_groesse = groesseFuer(m_potenz);
}

// setGridPower( GridPower power ) (grid.cpp:136-140)
void Raster::anwenden(int potenz) {
    m_einrasten = true;
    m_groesse = groesseFuer(potenz);
}

// template<GridPower power> void setGridPower() (grid.cpp:142-146)
void Raster::setzePotenz(int potenz) {
    m_potenz = potenz;
    anwenden(potenz);
}

// GridPrev (grid.cpp:148-154)
void Raster::kleiner() {
    m_einrasten = true;
    if (m_potenz > kMinPotenz) { anwenden(--m_potenz); }
}

// GridNext (grid.cpp:156-162)
void Raster::groesser() {
    m_einrasten = true;
    if (m_potenz < kMaxPotenz) { anwenden(++m_potenz); }
}

// ToggleGridSnap (grid.cpp:164-167)
void Raster::einrastenUmschalten() { m_einrasten = !m_einrasten; }

// maxGridCoordPowerImport (grid.cpp:176-181)
void Raster::setzeMaxKoordPotenz(int p) {
    m_maxKoord = std::pow(2.0F, static_cast<float>(std::clamp(p, 0, 4) + 12));
}

}  // namespace bhed::karte
