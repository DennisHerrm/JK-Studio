// echtlicht.h - Karten-Editor Stufe 7 Punkt 5: Echtzeit-Licht (eigene Funktion,
// nicht in NetRadiant-Custom).
//
// Die Lichter einer Karte so, wie q3map2 -light sie aus den Entities und Shadern
// macht (light.cpp CreateEntityLights, CreateSunLight, CreateSkyLights,
// light_bounce.cpp RadCreateDiffuseLights / RadSubdivideDiffuseLight,
// SetupEnvelopes) und der Beitrag eines Lichts an einer Stelle nach
// LightContributionToSample (light.cpp:777-1177, mit -fast) - fuer die Vorschau
// in der Kamera, die die Grafikkarte rechnet. Die Vorschau ist KEIN Ergebnis von
// q3map2: kein Bounce (Radiosity), kein Filter, kein Supersampling, Schatten
// ueber Schattenkarten statt Strahlen. Stufe 9c: Flaechenlicht wie q3map2 -fast
// (Dreiecke der Flaeche, unterteilt nach q3map_lightSubdivide, je Stueck ein
// Flaechenlicht mit PointToPolygonFormFactor und dem Rueckschein), "sun" und
// q3map_skyLight. Das echte Ergebnis kommt immer aus bhed_q3map2
// (Schnell-Backen, Build-Menue). Stufe 9d: misc_model und Patches strahlen
// Flaechenlicht ab (RadLightForTriangles, RadLightForPatch mit dem Netz von
// TessellatedMesh), Pinselflaechen in den Dreiecken von -meta (StripFaceSurface),
// die Jitter-Kopien von _samples und q3map_sunExt mit Abweichung und Proben (mit
// dem Zufall von q3map2: srand( 0 ), MSVC rand()).
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

#include <cstdint>
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
inline constexpr float kEchtFormFaktorSkala = 3.0F;       // formFactorValueScale
inline constexpr float kEchtUnterteilung = 999.0F;        // defaultLightSubdivide
inline constexpr int kEchtWicklungMax = 8;                // Ecken je Flaechenlicht in der Vorschau

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
    // Stufe 9c: Flaechenlicht wie q3map2 (RadSubdivideDiffuseLight)
    float add = 0;                      // light.add = value * formFactorValueScale * areaScale
    std::vector<Vektor3f> wicklung;     // light.w
    bool zweiseitig = false;            // LightFlags::Twosided
    bool himmel = false;                // Sonne aus q3map_skyLight (viele schwache Sonnen)
    int sonnenGruppe = 0;               // Stufe 9d: die Proben einer Sonne (q3map_sunExt, _samples) - gleiche Zahl
    int sonnenProbe = 0;                //   ... die wievielte (0: die Richtung ohne Abweichung)
};

// Was eine Flaeche mit q3map_surfacelight ausstrahlt (Shader-Skript), fuer
// echtLichterSammeln; flaechenlicht <= 0: keine.
struct EchtFlaechenShader {
    float flaechenlicht = 0;
    Vektor3f farbe{1, 1, 1};   // q3map_lightRGB bzw. die mittlere Farbe des Lichtbilds (si.color)
    float unterteilung = 0;    // q3map_lightSubdivide (0: defaultLightSubdivide 999)
    float backsplashAnteil = 0.05F;   // q3map_backsplash <percent> / 100 (DEF_BACKSPLASH_FRACTION)
    float backsplashWeite = 23.0F;    // DEF_BACKSPLASH_DISTANCE
    bool himmel = false;       // C_SKY: kein Rueckschein
    bool zweiseitig = false;   // C_FOG oder si.twoSided (cull none): LightFlags::Twosided
    bool autosprite = false;   // Stufe 9d: CreateSurfaceLights macht daraus ein Punktlicht in der Mitte der Flaeche
};
// Sonne aus dem Himmels-Shader (q3map_sun / q3map_sunExt): Farbe, Staerke, Richtung
struct EchtSonne {
    bool da = false;
    Vektor3f farbe{1, 1, 1};
    float staerke = 0;
    float grad = 0;     // Richtung in Grad (um z)
    float hoehe = 0;    // Hoehe in Grad
    // Stufe 9c: weitere Sonnen ("sun", q3map_sun, q3map_sunExt) und q3map_skyLight
    struct Sonne {
        Vektor3f farbe{1, 1, 1};
        float staerke = 0;
        float grad = 0;
        float hoehe = 0;
        float abweichung = 0;   // Stufe 9d: q3map_sunExt <deviance> (Bogenmass, sun.deviance)
        int proben = 1;         //   ... <samples> (sun.numSamples)
    };
    std::vector<Sonne> sonnen;   // leer: nur die erste aus den Feldern oben
    struct Himmelslicht {
        float wert = 0;
        int schritte = 2;
        int horizontMin = 0;
        int horizontMax = 90;
        bool farbeAusHimmel = true;   // sample_color (Vorgabe): die Farbe je Richtung aus den skyParms-Bildern
    };
    std::vector<Himmelslicht> himmel;
    Vektor3f shaderFarbe{1, 1, 1};   // si.color (Farbe der Himmelslichter ohne Himmelsbilder)
    // SkyProbes (light.cpp): je Bildblock der sechs Himmelsbilder Farbe und Richtung
    struct Probe {
        Vektor3f farbe{0, 0, 0};
        Vektor3f normale{0, 0, 1};
    };
    std::vector<Probe> proben;
};

// Stufe 9d: der Zufall von q3map2 -light (main.cpp srand( 0 ), Random() = rand() / RAND_MAX
// mit dem rand() von MSVC: LCG, 15 Bit). Die Jitter-Kopien der Lichter mit _samples
// (CreateEntityLights) und die Sonnen von q3map_sunExt (CreateSunLight) ziehen in dieser
// Reihenfolge aus derselben Folge.
struct EchtZufall {
    std::uint32_t zustand = 0;
    float naechste() {
        zustand = zustand * 214013U + 2531011U;
        return static_cast<float>((zustand >> 16) & 0x7FFFU) / 32767.0F;
    }
};

// Stufe 9d: ein Netz wie mesh_t (Zeilen von Punkten, Breite x Hoehe)
struct EchtNetz {
    int breite = 0;
    int hoehe = 0;
    std::vector<Vektor3f> ort;
    std::vector<Vektor3f> normale;
};
// Ein Patch wie in -light: DrawSurfaceForMesh (surface.cpp: Normalen der Kontrollpunkte mit
// MakeMeshNormals vor und nach PutMeshOnCurve), longestCurve (patch.cpp ExpandLongestCurve) und
// TessellatedMesh mit IterationsForCurve(longestCurve, patchSubdivisions) (light_bounce.cpp,
// lightmaps_ydnar.cpp). kontrolle: Zeile r, Spalte c an r * breite + c (wie mesh_t). Leer, wenn
// die Groesse nicht passt.
[[nodiscard]] EchtNetz echtPatchNetz(const std::vector<Vektor3f>& kontrolle, int breite, int hoehe, int unterteilungen = 8);
// RadLightForPatch (light_bounce.cpp): je Viereck des Netzes ein Flaechenlicht, wenn es eben ist
// (PLANAR_EPSILON 0.1), sonst seine zwei Dreiecke (MeshQuadIterator).
void echtPatchFlaechenLichter(const EchtNetz& netz, const EchtFlaechenShader& s, std::vector<EchtLicht>& aus);
// StripFaceSurface (surface_meta.cpp, ohne T-Verbindungen): die Dreiecke einer Pinselflaeche
// von -meta, je drei Indizes in w. Findet sich kein Streifen ohne entartete Dreiecke, kommt
// FanFaceSurface: dann ist *mitte = true und der Index w.size() steht fuer den Mittelpunkt.
[[nodiscard]] std::vector<int> echtFlaechenStreifen(const std::vector<Vektor3f>& w, bool* mitte = nullptr);

// Stufe 9d: die Flaechen der misc_model, wie q3map2 sie in die .bsp setzt (AddTriangleModels: ohne
// target in der Welt, sonst in der Pinsel-Entitaet dieses Namens; ModelGetTransform; Shader nach Skin
// und _remap) - fuer das Flaechenlicht und die Schatten (castShadows: in der Welt 1, sonst 0, dann
// _castShadows/_cs des Modells bzw. der Eltern). Je Flaeche ein Aufruf, je drei Ecken ein Dreieck.
struct EchtModellFlaeche {
    const Entitaet* quelle = nullptr;
    std::string shader;
    int wirft = 1;
    std::vector<Vektor3f> ort;
    std::vector<Vektor3f> normale;
    std::vector<Vektor2f> st;
};
void echtModellFlaechen(const Karte& k, const std::function<void(const EchtModellFlaeche&)>& je);

// Stufe 9d: Nebel wie im Spiel. q3map2 CreateMapFogs/EmitFogs: jeder Pinsel mit einem fogParms-Shader
// ist ein Nebel, die sichtbare Seite (visibleSide) erzwingt q3map_fogDir, sonst die erste sichtbare
// achsparallele Seite von +Z rueckwaerts, dann die anderen; worldspawn "_fog"/"fog" ist der globale
// Nebel. R_LoadFogs (tr_bsp.cpp): Farbe aus fogParms, tcScale = 1 / (max(1, Tiefe) * 8), die Flaeche
// = -Normale und Abstand der sichtbaren Seite. q3map2 schneidet die Flaechen an den Seiten des
// Nebelpinsels (FogDrawSurfaces) - hier steht der Pinsel als Ebenen (aussen: n.p > d).
struct EchtNebel {
    std::string shader;
    Vektor3f farbe{1, 0, 0};
    float tcScale = 1.0F / 8.0F;
    bool global = false;
    bool hatFlaeche = false;
    float ebene[4]{0, 0, 0, 0};       // t = dot(p, ebene.xyz) + ebene.w: Tiefe unter der Flaeche
    std::vector<float> ebenen;        // je vier: Normale (nach aussen), Abstand
    const Entitaet* quelle = nullptr;
};
struct EchtNebelShader {
    bool nebel = false;               // fogParms
    Vektor3f farbe{0, 0, 0};
    float tiefe = 0;
    Vektor3f richtung{0, 0, 0};       // q3map_fogDir
    bool nichtZeichnen = false;       // C_NODRAW (die sichtbare Seite ist keine nodraw-Seite)
    bool deckend = true;              // Pinsel mit dieser Seite fuellen Blaetter (sichtbar ist, was davor frei ist)
};
[[nodiscard]] std::vector<EchtNebel> echtNebelSammeln(const Karte& k, const std::function<EchtNebelShader(const std::string&)>& shader);
// R_FogFactor (tr_image.cpp) mit den Koordinaten von RB_CalcFogTexCoords (tr_shade_calc.cpp) an einer
// Stelle: Deckkraft des Nebels 0..1 (RB_FogPass mischt GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA).
[[nodiscard]] float echtNebelDichte(const EchtNebel& n, const Vektor3f& punkt, const Vektor3f& auge, const Vektor3f& vorn);

struct EchtLichtWelt {
    std::vector<EchtLicht> lichter;
    Vektor3f umgebung{0, 0, 0};   // worldspawn _color * _ambient
    Vektor3f minimum{0, 0, 0};    // worldspawn _color * _minlight
};

// Die Lichter der Karte. shaderVon: q3map_surfacelight der Flaechen und
// q3map_sun der Himmelsflaechen (nach Shadername); darf leer sein. Stufe 9d: dazu
// die Flaechen der Patches und der misc_model (k.kontext.modelle, wie AddTriangleModels:
// ohne target in der Welt, mit target in der Pinsel-Entitaet dieses Namens).
[[nodiscard]] EchtLichtWelt echtLichterSammeln(const Karte& k,
                                               const std::function<EchtFlaechenShader(const std::string&)>& flaechenShader,
                                               const std::function<EchtSonne(const std::string&)>& sonne);

// Der Beitrag (light.color * add) an punkt mit normale, ohne Schatten -
// dieselben Formeln wie der Shader der Grafikkarte (und LightContributionToSample).
[[nodiscard]] Vektor3f echtLichtBeitrag(const EchtLicht& l, const Vektor3f& punkt, const Vektor3f& normale);

// PointToPolygonFormFactor (light.cpp:734)
[[nodiscard]] float echtFormFaktor(const Vektor3f& punkt, const Vektor3f& normale, const std::vector<Vektor3f>& w);

// RadSubdivideDiffuseLight (light_bounce.cpp) fuer ein Dreieck einer Flaeche:
// haengt die Flaechenlichter (und den Rueckschein) an "aus".
void echtFlaechenLichter(const std::vector<Vektor3f>& wicklung, const Vektor3f& normale, const EchtFlaechenShader& s,
                         std::vector<EchtLicht>& aus);
// Stufe 9d: dasselbe mit einer Normale je Ecke (Modelle, Patches): RadClipWindingEpsilon mischt
// sie mit, die Normale des Lichts ist ihr Mittel (RadSubdivideDiffuseLight).
void echtFlaechenLichter(const std::vector<Vektor3f>& wicklung, const std::vector<Vektor3f>& normalen, const EchtFlaechenShader& s,
                         std::vector<EchtLicht>& aus);

// CreateSkyLights (light.cpp:228): die Sonnen eines q3map_skyLight; mit proben (SkyProbes,
// sample_color) die Farbe je Richtung, auf das Groesste aller Sonnen normiert.
void echtHimmelsLichter(const EchtSonne::Himmelslicht& h, const Vektor3f& farbe, std::vector<EchtLicht>& aus,
                        const std::vector<EchtSonne::Probe>* proben = nullptr);

// SkyProbes (light.cpp): die sechs Bilder _lf _rt _ft _bk _up _dn (RGBA, Zeilen von oben)
// in 64 x 64 Bloecken je Seite; leer, wenn ein Bild fehlt.
[[nodiscard]] std::vector<EchtSonne::Probe> echtHimmelsProben(const std::vector<unsigned char>* const rgba[6], const int* breite, const int* hoehe);

// Der Luxel-Wert ohne Schatten: Umgebung + Summe der Beitraege, mindestens das
// Minimum (SetupGrid/minLight).
[[nodiscard]] Vektor3f echtLuxel(const EchtLichtWelt& w, const Vektor3f& punkt, const Vektor3f& normale);

}  // namespace bhed::karte

#endif
