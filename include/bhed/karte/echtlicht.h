// echtlicht.h - Karten-Editor Stufe 7 Punkt 5: Echtzeit-Licht (eigene Funktion,
// nicht in NetRadiant-Custom).
//
// Die Lichter einer Karte so, wie q3map2 -light sie aus den Entities macht
// (light.cpp CreateEntityLights, CreateSunLight, CreateSurfaceLights,
// SetupEnvelopes) und der Beitrag eines Lichts an einer Stelle nach
// LightContributionToSample (light.cpp:777-1177) - fuer die Vorschau in der
// Kamera, die die Grafikkarte rechnet. Die Vorschau ist KEIN Ergebnis von
// q3map2: kein Bounce (Radiosity), kein Filter, kein Supersampling, Schatten
// ueber Schattenwuerfel statt Strahlen, Flaechenlicht als ein Punktlicht je
// Flaeche (statt PointToPolygonFormFactor ueber unterteilte Flaechen). Das
// echte Ergebnis kommt immer aus bhed_q3map2 (Schnell-Backen, Build-Menue).
//
// Portiert aus q3map2 (NetRadiant-Custom 20260114: tools/quake3/q3map2/light.cpp,
// light_ydnar.cpp, shaders.cpp).
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   Copyright (C) 2026 BehavEd-Remake contributors.
//   This program is free software; you can redistribute it and/or modify it
//   under the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version (LICENSE).
#ifndef BHED_KARTE_ECHTLICHT_H
#define BHED_KARTE_ECHTLICHT_H

#include "bhed/karte/mathe.h"

#include <functional>
#include <string>
#include <vector>

namespace bhed::karte {

class Karte;
class Entitaet;

// q3map2.h: pointScale, spotScale, areaScale, linearScale, falloffTolerance
inline constexpr float kEchtPunktSkala = 7500.0F;
inline constexpr float kEchtSpotSkala = 7500.0F;
inline constexpr float kEchtFlaechenSkala = 0.25F;
inline constexpr float kEchtLinearSkala = 1.0F / 8000.0F;
inline constexpr float kEchtAbfallGrenze = 1.0F;

struct EchtLicht {
    enum class Art { Punkt, Spot, Sonne, Flaeche };
    Art art = Art::Punkt;
    Vektor3f ursprung{0, 0, 0};
    Vektor3f farbe{1, 1, 1};        // light.color (normiert wie ColorNormalize)
    float photonen = 0;             // light.photons (schon mit point-/spotScale)
    bool linear = false;            // LightFlags::AttenLinear
    bool winkel = true;             // LightFlags::AttenAngle
    float fade = 1;
    float winkelSkala = 0;          // _anglescale
    float extraAbstand = 0;         // _extradist
    Vektor3f normale{0, 0, -1};     // Spot: Richtung zum Ziel; Flaeche: Flaechennormale
    float radiusJeAbstand = 0;      // Spot: (radius + 16) / dist
    Vektor3f sonnenRichtung{0, 0, 1};   // Sonne: zur Sonne hin
    float huelle = 0;               // SetupEnvelopes mit -fast: Abstand, ab dem nichts mehr ankommt
    bool negativ = false;           // LightFlags::Negative
    int stil = 0;
    const Entitaet* quelle = nullptr;   // fuer Schatten-Zwischenspeicher und Auswahl
};

// Was eine Flaeche mit q3map_surfacelight ausstrahlt (Shader-Skript), fuer
// echtLichterSammeln; flaechenlicht <= 0: keine.
struct EchtFlaechenShader {
    float flaechenlicht = 0;
    Vektor3f farbe{1, 1, 1};   // q3map_lightRGB bzw. die mittlere Bildfarbe
};
// Sonne aus dem Himmels-Shader (q3map_sun / q3map_sunExt): Farbe, Staerke, Richtung
struct EchtSonne {
    bool da = false;
    Vektor3f farbe{1, 1, 1};
    float staerke = 0;
    float grad = 0;     // Richtung in Grad (um z)
    float hoehe = 0;    // Hoehe in Grad
};

struct EchtLichtWelt {
    std::vector<EchtLicht> lichter;
    Vektor3f umgebung{0, 0, 0};   // worldspawn _color * _ambient
    Vektor3f minimum{0, 0, 0};    // worldspawn _color * _minlight
};

// Die Lichter der Karte. shaderVon: q3map_surfacelight der Flaechen und
// q3map_sun der Himmelsflaechen (nach Shadername); darf leer sein.
[[nodiscard]] EchtLichtWelt echtLichterSammeln(const Karte& k,
                                               const std::function<EchtFlaechenShader(const std::string&)>& flaechenShader,
                                               const std::function<EchtSonne(const std::string&)>& sonne);

// Der Beitrag (light.color * add) an punkt mit normale, ohne Schatten -
// dieselben Formeln wie der Shader der Grafikkarte (und LightContributionToSample).
[[nodiscard]] Vektor3f echtLichtBeitrag(const EchtLicht& l, const Vektor3f& punkt, const Vektor3f& normale);

// Der Luxel-Wert ohne Schatten: Umgebung + Summe der Beitraege, mindestens das
// Minimum (SetupGrid/minLight).
[[nodiscard]] Vektor3f echtLuxel(const EchtLichtWelt& w, const Vektor3f& punkt, const Vektor3f& normale);

}  // namespace bhed::karte

#endif
