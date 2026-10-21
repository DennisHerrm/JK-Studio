// xyansicht.h - Karten-Editor: die 2D-Ansichten XY / XZ / YZ.
//
// Portiert aus NetRadiant-Custom 20260114: radiant/xywindow.cpp (XYWnd:
// SetOrigin, Scroll, SetScale, ZoomIn/Out(WithMouse), ZoomCompensateOrigin,
// FocusOnBounds, XY_ToPoint, updateProjection, updateModelview, XY_DrawGrid,
// XY_DrawAxis, XY_DrawBlockGrid, DrawCameraIcon, PaintSizeInfo, overlayDraw:
// Fensterumriss und Fadenkreuz, XYWnd_PositionCamera, XYWnd_OrientCamera,
// XY_NextView), radiant/xywindow.h (VIEWTYPE, ViewType_getTitle,
// xywindow_globals_t), include/qerplugin.h (NDIM1NDIM2), radiant/brush.h
// (update_wireframe: sichtbare Kanten), libs/entitylib.h (aabb_draw_wire,
// arrow_draw, aabb_fits_view), libs/math/matrix.h
// (matrix4_rotation_for_euler_xyz), plugins/entity/angles.h (read_angle,
// read_group_angle, read_angles), plugins/entity/light.cpp (Oktaeder),
// plugins/entity/targetable.h (Verbindungslinien).
//
// Hier steht nur Rechnung - welche Linie wohin, welcher Text wohin. Das
// Zeichnen (Direct3D 11) macht die Oberflaeche.
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
#ifndef BHED_KARTE_XYANSICHT_H
#define BHED_KARTE_XYANSICHT_H

#include "bhed/karte/mathe.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace bhed::karte {

class Pinsel;
class Patch;
struct Kontext;

// VIEWTYPE (qerplugin.h:94-102): der Wert ist zugleich die Blickachse.
enum class Sicht : int { YZ = 0, XZ = 1, XY = 2 };
// NDIM1NDIM2
[[nodiscard]] inline int dim1(Sicht s) { return s == Sicht::YZ ? 1 : 0; }
[[nodiscard]] inline int dim2(Sicht s) { return s == Sicht::XY ? 1 : 2; }
[[nodiscard]] inline int blickachse(Sicht s) { return static_cast<int>(s); }
[[nodiscard]] const char* sichtTitel(Sicht s);   // ViewType_getTitle
// XY_NextView: XY -> XZ -> YZ -> XY
[[nodiscard]] inline Sicht naechsteSicht(Sicht s) { return static_cast<Sicht>((static_cast<int>(s) + 2) % 3); }

inline constexpr float kMaxWeltKoord = 64.0F * 1024.0F;    // g_MaxWorldCoord (map.h:98)
inline constexpr float kMinWeltKoord = -64.0F * 1024.0F;   // g_MinWorldCoord

// Farben als RGBA (Rot im niedrigsten Byte, wie IM_COL32).
using Farbe = std::uint32_t;
[[nodiscard]] Farbe farbeAus(float r, float g, float b, float a = 1.0F);

// Eine Linie in Weltkoordinaten (2D-Ansicht: dritte Koordinate egal).
struct Linie {
    Vektor3f a;
    Vektor3f b;
    Farbe farbe = 0;
};
// Ein Text: linke Unterkante in Bildpunkten (Ursprung links oben, y nach
// unten), wie glRasterPos + drawString.
struct Text {
    float x = 0;
    float y = 0;
    Farbe farbe = 0;
    std::string text;
};

// Der Zustand einer 2D-Ansicht (XYWnd).
class XyAnsicht {
public:
    Sicht typ = Sicht::XY;                  // m_viewType
    float massstab = 1.0F;                  // m_fScale (Bildpunkte je Einheit)
    Vektor3f ursprung{0.0F, 20.0F, 46.0F};  // m_vOrigin (Weltpunkt in der Mitte)
    int breite = 0;                         // m_nWidth (Geraetepunkte)
    int hoehe = 0;                          // m_nHeight
    float maxRaster = 65536.0F;             // GetMaxGridCoord()
    bool zumZeiger = true;                  // m_bZoomToPointer

    void setzeUrsprung(const Vektor3f& o);   // SetOrigin
    void rollen(int x, int y);               // Scroll
    void setzeMassstab(float f);             // SetScale (min/max, Epsilon)
    void hinein();                           // ZoomIn  (* 5/4)
    void hinaus();                           // ZoomOut (* 4/5)
    void hineinMaus(int x, int y);           // ZoomInWithMouse
    void hinausMaus(int x, int y);           // ZoomOutWithMouse
    void aufGrenzen(const Kasten& k);        // FocusOnBounds
    // XY_ToPoint: Geraetepunkte (y nach unten) -> Welt, Blickachse 0;
    // fang > 0: eingerastet (GetSnapGridSize).
    [[nodiscard]] Vektor3f zuPunkt(int x, int y, float fang = 0.0F) const;
    // Welt -> Bildpunkte (Umkehrung, NRC rechnet es ueber m_view).
    [[nodiscard]] std::array<float, 2> zuBild(const Vektor3f& p) const;
    // updateProjection / updateModelview (spaltenweise, m[spalte*4+zeile])
    [[nodiscard]] Matrix4 projektion() const;
    [[nodiscard]] Matrix4 modelview() const;
    // Halbe Sichtbreite/-hoehe in Welteinheiten: (m_nWidth / 2) / m_fScale
    // mit Ganzzahldivision wie NRC.
    [[nodiscard]] float halbB() const { return static_cast<float>(breite / 2) / massstab; }
    [[nodiscard]] float halbH() const { return static_cast<float>(hoehe / 2) / massstab; }

private:
    void ausgleichen(int x, int y, float alt);   // ZoomCompensateOrigin
};

// Was XY_DrawGrid und die Dekoration (Achsen, Titel, Workzone, Bloecke)
// zeichnen - Linien in Ebenenkoordinaten (nDim1, nDim2) als Welt mit 0 auf
// der Blickachse, Texte in Bildpunkten.
struct RasterEinstellungen {
    float raster = 16.0F;           // GetGridSize()
    float fang = 16.0F;             // GetSnapGridSize() (0 = aus)
    int potenz = 4;                 // Grid_getPower()
    bool zeigeRaster = true;        // d_showgrid (ToggleGrid)
    bool zeigeKoordinaten = true;   // show_coordinates
    bool zeigeAchsen = true;        // show_axis
    bool zeigeWorkzone = false;     // show_workzone
    bool zeigeBloecke = false;      // show_blocks
    bool aktiv = false;             // Active()
    bool regionAktiv = false;       // g_region_active
    Vektor3f regionMin{-65536.0F, -65536.0F, -65536.0F};   // g_region_mins
    Vektor3f regionMax{65536.0F, 65536.0F, 65536.0F};      // g_region_maxs
    Vektor3f workMin{-64.0F, -64.0F, -64.0F};               // Select_getWorkZone()
    Vektor3f workMax{64.0F, 64.0F, 64.0F};
    std::array<int, 3> blockGroesse{1024, 1024, 1024};      // _blocksize
    bool weltDa = true;             // Map_FindWorldspawn (Bloecke nur mit worldspawn)
    // Farben (xywindow_globals_t, Werte aus dem Farbschema des Programms)
    std::array<float, 3> hinten{.225803F, .225803F, .225803F};   // color_gridback
    std::array<float, 3> klein{.254902F, .254902F, .254902F};    // color_gridminor
    std::array<float, 3> gross{.294118F, .294118F, .294118F};    // color_gridmajor
    std::array<float, 3> block{1.0F, 1.0F, 1.0F};               // color_gridblock
    std::array<float, 3> text{.972549F, .972549F, .972549F};    // color_gridtext
    std::array<float, 3> name{.516136F, .516136F, .516136F};    // color_viewname
    Farbe achseX = 0xFF0000FFU;     // g_colour_x (255,0,0)
    Farbe achseY = 0xFF00FF00U;     // g_colour_y
    Farbe achseZ = 0xFFFF0000U;     // g_colour_z
    // Schriftmasse (getPixelHeight, getPixelDescent)
    int schriftHoehe = 13;
    int schriftUnterlaenge = 3;
    // fuer den Test: die Breite eines Textes in Bildpunkten (Achsenbuchstaben
    // nehmen fontHeight * .55)
};
struct RasterZeichnung {
    float kleinSchritt = 0;        // minor_step
    float grossSchritt = 0;        // step
    int maske = 0;                 // mask
    float textSchrittX = 0;        // stepx
    float textSchrittY = 0;        // stepy
    float alpha = 1;               // a (0.3 ohne Fang)
    std::vector<Linie> linien;     // in Zeichenreihenfolge, Breite 1
    std::vector<Linie> linien2;    // Breite 2 (Achsen, Bloecke)
    std::vector<Text> texte;       // Koordinaten, Achsenbuchstaben, Titel
};
[[nodiscard]] RasterZeichnung rasterZeichnen(const XyAnsicht& v, const RasterEinstellungen& e);

// Kamera-Symbol (DrawCameraIcon): Linienzuege in der Ebene.
[[nodiscard]] std::vector<Linie> kameraSymbol(const XyAnsicht& v, const Vektor3f& ursprung, const Vektor3f& winkel,
                                              Farbe farbe);
// Fadenkreuz (overlayDraw, g_bCrossHairs) durch die eingerastete Mausposition.
[[nodiscard]] std::vector<Linie> fadenkreuz(const XyAnsicht& v, const Vektor3f& maus);
// Groessenanzeige (PaintSizeInfo): Masslinien und drei Texte.
struct GroessenAnzeige {
    std::vector<Linie> linien;
    std::vector<Text> texte;
};
[[nodiscard]] GroessenAnzeige groessenAnzeige(const XyAnsicht& v, const Kasten& auswahl, Farbe farbe, int schriftHoehe);
// Fensterumriss der aktiven Ansicht (Farbe der Blickachse, Split: Name).
[[nodiscard]] Farbe umrissFarbe(Sicht s, bool split, const RasterEinstellungen& e);

// XYWnd_PositionCamera / XYWnd_OrientCamera: neue Lage bzw. Winkel der
// 3D-Kamera aus einem Mauspunkt (Winkel: pitch, yaw, roll in Grad).
[[nodiscard]] Vektor3f kameraVersetzen(const XyAnsicht& v, int x, int y, float fang, const Vektor3f& kameraUrsprung);
[[nodiscard]] Vektor3f kameraDrehen(const XyAnsicht& v, int x, int y, const Vektor3f& kameraUrsprung,
                                    const Vektor3f& kameraWinkel);

// --- Szene -------------------------------------------------------------------

// Die sichtbaren Kanten eines Pinsels in einer Ansicht (update_wireframe):
// eine Kante, wenn eine ihrer Flaechen zum Betrachter zeigt
// (viewer_test_plane, im Ortho strikt > 0). Haengt Linien an "aus".
void pinselKanten(const Pinsel& p, const Kontext& k, Sicht s, Farbe farbe, std::vector<Linie>& aus);
// Das Drahtgitter eines Patches (RenderablePatchWireframe): Linienzuege
// entlang der Zeilen und Spalten der Tesselierung.
void patchGitter(const Patch& p, Farbe farbe, std::vector<Linie>& aus);
// aabb_draw_wire: 12 Kanten und die Diagonale max-min.
void kastenDraht(const Kasten& k, Farbe farbe, std::vector<Linie>& aus);
// matrix4_rotation_for_euler_xyz_degrees
[[nodiscard]] Matrix4 drehungEulerXyzGrad(const Vektor3f& winkel);
// Winkel einer Entitaet (AnglesKey): "angles" (wenn vorhanden), sonst
// "angle" (Gruppen: -1 hoch, -2 runter).
[[nodiscard]] Vektor3f entitaetsWinkel(const std::string& angle, const std::string& angles, bool hatAngles, bool gruppe);
// arrow_draw: Pfeil 32 lang ab "ursprung" in Richtung der Winkel.
void pfeil(const Vektor3f& ursprung, const Vektor3f& winkel, Farbe farbe, std::vector<Linie>& aus);
// aabb_fits_view (Namen nur, wenn die Entitaet gross genug erscheint).
[[nodiscard]] bool kastenPasst(const Kasten& k, const XyAnsicht& v, int verhaeltnis);
// Licht (light.cpp): Oktaeder aus der Licht-AABB.
void lichtOktaeder(const Kasten& k, Farbe farbe, std::vector<Linie>& aus);
// Verbindung target -> targetname (targetable.h): Linie mit Pfeilspitze(n).
void verbindung(const Vektor3f& von, const Vektor3f& nach, Farbe farbe, std::vector<Linie>& aus);

}  // namespace bhed::karte

#endif
