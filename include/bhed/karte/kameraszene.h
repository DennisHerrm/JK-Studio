// kameraszene.h - Karten-Editor: was die 3D-Kamera zeichnet.
//
// Portiert aus NetRadiant-Custom 20260114: radiant/renderer.h
// (RenderHighlighted: eFace/ePrimitive fuer Ausgewaehltes und seine Kinder),
// radiant/camwindow.cpp (CamRenderer::addRenderable: $CAM_OVERLAY,
// $CAM_WIRE, $CAM_HIGHLIGHT; Cam_Draw; CamDrawWorkzone::render),
// radiant/brush.h (Winding_Draw: GL_POLYGON mit der Flaechennormale,
// FaceInstance::render: ePrimitiveWire), radiant/patch.h
// (RenderablePatchSolid: GL_QUAD_STRIP), libs/entitylib.h
// (aabb_draw_flatshade, arrow_draw), plugins/entity/generic.cpp,
// plugins/entity/group.cpp (Name und Pfeil in der Mitte der Kinder),
// plugins/entity/light.cpp (light_draw, sphere_construct_fill,
// light_draw_radius_fill, LightRadii), plugins/entity/colour.h
// (read_colour_normalized, m_colour_add = Farbe / 8),
// plugins/entity/namedentity.h (Namen) und radiant/renderstate.cpp (die
// Zustaende "(r g b)" und "{r g b}").
//
// Hier steht nur, welche Dreiecke und Linien in welchem Zustand liegen - das
// Zeichnen macht die Oberflaeche (Direct3D 11).
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
#ifndef BHED_KARTE_KAMERASZENE_H
#define BHED_KARTE_KAMERASZENE_H

#include "bhed/karte/kamera.h"
#include "bhed/karte/xyansicht.h"

#include <array>
#include <deque>
#include <string>
#include <vector>

namespace bhed::karte {

class Karte;
class Entitaet;

// Ein Eckpunkt eines Dreiecks (Position, Normale, Texturkoordinate, Farbe).
struct KameraPunkt {
    Vektor3f pos{0, 0, 0};
    Vektor3f normale{0, 0, 1};
    Vektor2f st{0, 0};
    Farbe farbe = 0xFFFFFFFFU;
};

// Eine Linie. polygon = true: die Kante eines Polygons, das im Linienmodus
// (glPolygonMode GL_LINE) gezeichnet wird - c ist ein weiterer Punkt
// derselben Flaeche (Umlaufsinn fuer das Wegschneiden der Rueckseiten und
// Neigung fuer glPolygonOffset). polygon = false: GL_LINES - c ist die
// Normale, die beim Zeichnen gerade gilt (fuer das Licht).
struct KameraKante {
    Vektor3f a{0, 0, 0};
    Vektor3f b{0, 0, 0};
    Vektor3f c{0, 0, 1};
    Farbe fa = 0xFFFFFFFFU;
    Farbe fb = 0xFFFFFFFFU;
    bool polygon = true;
};

// Ein Zustand mit seinen Objekten: ein Shader (Pinselflaechen und Patches),
// "(r g b)" (Fuellung: Entitaetskaesten, Lichter, Pfeile) oder "{r g b}"
// (Additiv: Lichtradien).
enum class StapelArt { Shader, Fuellung, Additiv };
struct KameraStapel {
    StapelArt art = StapelArt::Shader;
    std::string shader;               // Art Shader
    Farbe farbe = 0xFFFFFFFFU;        // Fuellung/Additiv: m_colour
    std::vector<KameraPunkt> dreiecke;          // je drei
    std::vector<KameraKante> kanten;            // Umrisse (nur mit Optionen::kanten), nicht ausgewaehlt
    std::vector<KameraKante> kantenGewaehlt;    // Umrisse ausgewaehlter Objekte (nur mit Optionen::kanten)
    std::vector<KameraKante> linien;            // GL_LINES (Pfeile)
    // Fuer ShowStats (renderstate.cpp count_prim / count_transform): Objekte
    // (Renderables) und Wechsel der Objekt-Matrix in der Reihenfolge des Zustands.
    std::size_t objekte = 0;
    std::size_t transforms = 0;
    const void* letzteMatrix = nullptr;   // beim Bauen: die Matrix des letzten Objekts
};

// Ein Name (RenderableNamedEntity): Ankerpunkt in der Welt, Text, Farbe.
struct KameraName {
    Vektor3f pos{0, 0, 0};
    std::string text;
    Farbe farbe = 0;
    int art = 0;   // eNameNormal 0, eNameSelected 1, eNameChildSelected 2
};

// Teil der Szene (Punkt 12, grosse Karten): die Geometrie (Pinsel und Patches
// in ihren Shader-Stapeln) haengt nicht an der Auswahl und wird nur bei einer
// Aenderung der Karte neu gebaut; der Rest (Punktentitaeten, Gruppen, Namen,
// Lichtradien und alles Ausgewaehlte) bei jedem Wechsel der Auswahl.
enum class KameraTeil { Alles, Geometrie, Rest };

struct KameraSzenenOptionen {
    KameraTeil teil = KameraTeil::Alles;
    bool kanten = false;        // Polygon-Umrisse bauen (Wireframe, Textured+Wire)
    bool winkel = true;         // g_showAngles
    bool namen = true;          // g_showNames
    bool lichtRadien = true;    // g_lightRadii
    bool zielnamen = false;     // g_showTargetNames (KeyIsName::m_nameKey "targetname" statt "classname")
};

struct KameraSzene {
    std::deque<KameraStapel> stapel;   // deque: Verweise bleiben beim Anhaengen gueltig
    // Ausgewaehltes (eFace): $CAM_HIGHLIGHT
    std::vector<KameraPunkt> wahlDreiecke;
    // Ausgewaehltes (ePrimitive): $CAM_OVERLAY - Polygon-Umrisse und Linien
    std::vector<KameraKante> wahlKanten;
    // GL_LINES ausgewaehlter Entities im Zustand $CAM_HIGHLIGHT (Pfeile)
    std::vector<KameraKante> wahlTintLinien;
    std::vector<KameraName> namen;
    std::vector<EtagenKandidat> etagen;   // FloorHeightWalker
    std::size_t dreieckAnzahl = 0;
    // ShowStats: Objekte und Matrix-Wechsel in $CAM_HIGHLIGHT und $CAM_OVERLAY
    std::size_t tintObjekte = 0;
    std::size_t tintTransforms = 0;
    std::size_t overlayObjekte = 0;
    std::size_t overlayTransforms = 0;
    std::size_t wahlPrimitive = 0;   // ausgewaehlte Pinselflaechen und Patches (nicht in $CAM_WIRE)
};

[[nodiscard]] KameraSzene kameraSzeneBauen(const Karte& karte, const KameraSzenenOptionen& o);

// --- Einzelteile (auch fuer den Test) -----------------------------------------

// Ein konvexes Polygon (GL_POLYGON): Faecher-Dreiecke mit der Normale; mit
// kanten den geschlossenen Umriss.
void polygonFuellung(const std::vector<KameraPunkt>& ecken, std::vector<KameraPunkt>& dreiecke,
                     std::vector<KameraKante>* kanten);
// aabb_draw_flatshade: 6 Vierecke mit aabb_normals.
void kastenFuellung(const Kasten& k, Farbe farbe, std::vector<KameraPunkt>& dreiecke, std::vector<KameraKante>* kanten);
// light_draw: das Oktaeder (mit der Normalenfolge des beleuchteten Zweigs).
void lichtOktaederFuellung(const Kasten& k, Farbe farbe, std::vector<KameraPunkt>& dreiecke,
                           std::vector<KameraKante>* kanten);
// sphere_draw_fill: der Dreiecksstreifen (SPHERE_FILL_POINTS = 553).
void kugelFuellung(const Vektor3f& mitte, float radius, Farbe farbe, std::vector<KameraPunkt>& dreiecke,
                   std::vector<KameraKante>* kanten);
[[nodiscard]] const std::vector<Vektor3f>& kugelPunkte();   // sphere_construct_fill
// arrow_draw als GL_LINES (normale: die beim Zeichnen geltende Normale).
void pfeilLinien(const Vektor3f& ursprung, const Vektor3f& winkel, Farbe farbe, const Vektor3f& normale,
                 std::vector<KameraKante>& aus);

// LightRadii (light.cpp:714-828): die drei Radien und welche zwei gezeichnet
// werden (m_radii_transformed + !isLinear()).
struct LichtRadien {
    std::array<float, 3> r{0, 0, 0};
    bool linear = false;
    [[nodiscard]] std::array<float, 2> gezeichnet() const { return linear ? std::array<float, 2>{r[0], r[1]} : std::array<float, 2>{r[1], r[2]}; }
};
[[nodiscard]] LichtRadien lichtRadien(const std::string& lightUnterstrich, const std::string& light, const std::string& scale,
                                      const std::string& fade, const std::string& spawnflags);
// sphere_draw_wire: drei Kreise mit je 24 Seiten (2D, ausgewaehltes Licht).
void kugelDraht(const Vektor3f& mitte, float radius, Farbe farbe, std::vector<Linie>& aus);

// read_colour_normalized fuer "_color" (Vorgabe 1 1 1).
[[nodiscard]] Vektor3f lichtFarbeNormiert(const std::string& wert);

// CamDrawWorkzone::render (camwindow.cpp:811-918): je Achse gestrichelte
// Linien auf den Kanten der Auswahl, rot und weiss im Wechsel, 1024 ueber die
// Auswahl hinaus ausgeblendet. rot mit glPolygonOffset(-2, 2), weiss mit
// (1, -1).
void workzone3d(const Kasten& grenzen, float raster, std::vector<KameraKante>& rot, std::vector<KameraKante>& weiss);

}  // namespace bhed::karte

#endif
