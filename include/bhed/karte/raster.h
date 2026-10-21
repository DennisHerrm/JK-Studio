// raster.h - Karten-Editor: das Raster (Grid) wie NetRadiant-Custom.
//
// Portiert aus NetRadiant-Custom (Stand master, 2026): radiant/grid.cpp
// (GridPower, g_grid_power, g_grid_snap, setGridPower, GridPrev, GridNext,
// ToggleGridSnap, GetSnapGridSize, GetGridSize, maxGridCoordPowerImport).
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
#ifndef BHED_KARTE_RASTER_H
#define BHED_KARTE_RASTER_H

#include <string>

namespace bhed::karte {

class Raster {
public:
    static constexpr int kMinPotenz = -3;   // GRIDPOWER_0125
    static constexpr int kMaxPotenz = 10;   // GRIDPOWER_1024
    static constexpr int kStandard = 7;     // g_grid_default = GridDefault_forGridPower( GRIDPOWER_16 )

    // Grid_construct: Potenz aus "GridDefault" (0..13 -> -3..10).
    void standardSetzen(int gridDefault);
    [[nodiscard]] int potenz() const { return m_potenz; }   // Grid_getPower
    [[nodiscard]] float groesse() const { return m_groesse; }   // GetGridSize
    [[nodiscard]] float einrastGroesse() const { return m_einrasten ? m_groesse : 0.0F; }   // GetSnapGridSize
    [[nodiscard]] bool einrasten() const { return m_einrasten; }
    void setzePotenz(int potenz);   // setGridPower<power> (SetGrid0.125 ... SetGrid1024)
    void kleiner();                 // GridPrev ("[")
    void groesser();                // GridNext ("]")
    void einrastenUmschalten();     // ToggleGridSnap
    // GridDefault_forGridPower: Index im Grid-Menue (0 = 0.125 ... 13 = 1024).
    [[nodiscard]] int menueIndex() const { return m_potenz + 3; }
    // g_gridnames
    [[nodiscard]] static const char* name(int menueIndex);

    // maxGridCoordPowerImport: 2^(clamp(p,0,4)+12), Vorgabe Potenz 4 = 65536.
    void setzeMaxKoordPotenz(int p);
    [[nodiscard]] float maxKoord() const { return m_maxKoord; }   // GetMaxGridCoord

private:
    void anwenden(int potenz);   // setGridPower( GridPower ) ohne g_grid_power
    int m_potenz = kStandard - 3;
    bool m_einrasten = true;
    float m_groesse = 16.0F;
    float m_maxKoord = 65536.0F;
};

}  // namespace bhed::karte
#endif
