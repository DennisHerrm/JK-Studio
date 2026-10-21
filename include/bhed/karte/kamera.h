// kamera.h - Karten-Editor: die 3D-Kamera (CamWnd).
//
// Portiert aus NetRadiant-Custom 20260114: radiant/camwindow.cpp (camera_t,
// camwindow_globals_private_t, projection_for_camera, Camera_getFarClipPlane,
// Camera_updateProjection, Camera_updateModelview/-Vectors,
// Camera_Move_updateAxes, Camera_Freemove_updateAxes, Camera_FreeMove,
// Camera_motionDelta, Cam_KeyControl, Camera_setMovementFlags/
// clearMovementFlags, die Camera_*_Discrete-Schritte, camera_zoom,
// wheelmove_scroll, camera_orbit_scroll, Camera_getFocusPos,
// GlobalCamera_ResetAngles, FloorHeightWalker/Cam_ChangeFloor, Camera_CubeIn/
// Out, CameraModeNext/Prev, CameraSpeed_increase/decrease,
// disable_freelook_button_release), radiant/camwindow.h
// (camwindow_globals_t), radiant/view.h (View::Construct: Viewport),
// libs/math/frustum.h (matrix4_frustum, frustum_from_viewproj).
//
// Hier steht nur Rechnung. Das Zeichnen (Direct3D 11), Maus und Tasten macht
// die Oberflaeche (gui/app_karte_cam.cpp).
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
#ifndef BHED_KARTE_KAMERA_H
#define BHED_KARTE_KAMERA_H

#include "bhed/karte/mathe.h"

#include <vector>

namespace bhed::karte {

// camera_draw_mode (camwindow.cpp:197-205)
enum class KameraModus : int { Draht = 0, Flach = 1, Textur = 2, TexturDraht = 3, Licht = 4 };
inline constexpr int kKameraModusAnzahl = 5;   // camera_draw_mode_count

// MOVE_* (camwindow.cpp:436-448)
inline constexpr unsigned kBewegVor = 1U << 0;
inline constexpr unsigned kBewegZurueck = 1U << 1;
inline constexpr unsigned kBewegDrehRechts = 1U << 2;
inline constexpr unsigned kBewegDrehLinks = 1U << 3;
inline constexpr unsigned kBewegRechts = 1U << 4;
inline constexpr unsigned kBewegLinks = 1U << 5;
inline constexpr unsigned kBewegHoch = 1U << 6;
inline constexpr unsigned kBewegRunter = 1U << 7;
inline constexpr unsigned kBewegNeigHoch = 1U << 8;
inline constexpr unsigned kBewegNeigRunter = 1U << 9;
inline constexpr unsigned kBewegFokus = 1U << 10;
inline constexpr unsigned kBewegAlle = (1U << 11) - 1U;

// SPEED_MOVE, SPEED_TURN, CAM_MIN_SPEED, CAM_MAX_SPEED, CAM_SPEED_STEP
inline constexpr float kSchrittWeg = 32.0F;
inline constexpr float kSchrittWinkel = 22.5F;
inline constexpr int kTempoMin = 50;
inline constexpr int kTempoMax = 9999;
inline constexpr int kTempoSchritt = 250;
inline constexpr float kNahEbene = 1.0F;   // camera_t::near_z

// Die Einstellungen der Kamera: camwindow_globals_private_t,
// camwindow_globals_t (ohne Farben) und die statischen Werte von camera_t.
struct KameraEinstellungen {
    int tempo = 500;              // m_nMoveSpeed (CamMoveSpeed)
    int zeitBisMax = 200;         // m_time_toMaxSpeed in ms (CamMoveTimeToMaxSpeed)
    int radTempo = 100;           // m_nScrollMoveSpeed (ScrollMoveSpeed)
    bool zumZeiger = true;        // m_bZoomToPointer (3DZoomInToPointer)
    float strafeTempo = 1.0F;     // m_strafeSpeed (CamStrafeSpeed)
    float winkelTempo = 3.0F;     // m_angleSpeed (Sensitivity)
    bool mausUmkehren = false;    // m_bCamInverseMouse (CamInverseMouse)
    bool diskret = true;          // m_bCamDiscrete (CamDiscrete)
    bool kubisch = false;         // m_bCubicClipping (CubicClipping)
    int strafeModus = 3;          // m_strafeMode (StrafeMode): None, Up, Forward, Both, Both Inverted
    bool flaechenDraht = true;    // m_bFaceWire (CameraFaceWire)
    bool faerben = true;          // m_bFaceFill (CameraFaceFill)
    int msaa = 8;                 // m_MSAA (CameraMSAA)
    bool zeigeWorkzone = true;    // m_bShowWorkzone (ShowWorkzone3d)
    bool zeigeGroesse = true;     // m_bShowSize (ShowSize3d)
    int kubikSkala = 14;          // m_nCubicScale (CubicScale)
    bool zeigeStatistik = false;  // m_showStats (ShowStats)
    float sichtfeld = 100.0F;     // camera_t::fieldOfView
    KameraModus modus = KameraModus::Textur;   // camera_t::draw_mode (CameraRenderMode)
};

// matrix4_frustum (OpenGL glFrustum, NDC-z -1..1)
[[nodiscard]] Matrix4 frustumMatrix(float links, float rechts, float unten, float oben, float nah, float fern);
// projection_for_camera: das Sichtfeld gilt fuer die groessere Seite.
[[nodiscard]] Matrix4 kameraProjektion(float nah, float fern, float sichtfeld, int breite, int hoehe);
// Camera_getFarClipPlane
[[nodiscard]] float fernEbene(const KameraEinstellungen& e);

// struct Frustum (frustum.h:412-414), aus frustum_from_viewproj.
struct Frustum {
    Ebene3d rechts, links, unten, oben, hinten, vorn;
};
[[nodiscard]] Frustum frustumAusViewProj(const Matrix4& viewproj);

// Ein Knoten fuer den FloorHeightWalker: Grenzen in der Welt, ausgewaehlt?
// (Gruppen-Entities wie worldspawn sind keine Kandidaten, ihre Pinsel schon.)
struct EtagenKandidat {
    Kasten grenzen;
    bool gewaehlt = false;
};

// camera_t (camwindow.cpp:207-263)
class Kamera {
public:
    int breite = 0;   // width (Geraetepunkte)
    int hoehe = 0;    // height
    Vektor3f ursprung{0, 0, 0};   // origin
    Vektor3f winkel{0, 0, 0};     // angles: pitch, yaw, roll (Grad)
    Vektor3f vorn{0, 0, 0};       // forward
    Vektor3f seite{0, 0, 0};      // right
    Vektor3f vup{0, 0, 0};
    Vektor3f vpn{0, 0, 0};
    Vektor3f vright{0, 0, 0};
    Matrix4 projektion = kEinheitsMatrix;
    Matrix4 modelview = kEinheitsMatrix;
    bool strafe = false;            // m_strafe
    bool strafeVor = false;         // m_strafe_forward
    bool strafeVorUmkehr = false;   // m_strafe_forward_invert
    bool orbit = false;             // m_orbit
    Vektor3f orbitMitte{0, 0, 0};   // m_orbit_center
    Vektor3f orbitStart{0, 0, 0};   // m_orbit_initial_pos
    int orbitVersatz = 0;           // m_orbit_offset
    int fokusVersatz = 0;           // m_focus_offset
    unsigned bewegung = 0;          // movementflags
    float tempo = 0.0F;             // m_keymove_speed_current

    // Camera_updateModelview (mit Camera_updateVectors)
    void modelviewAktualisieren();
    // Camera_updateProjection
    void projektionAktualisieren(const KameraEinstellungen& e);
    // View::GetViewMatrix: projection * modelview (m_scissor ist die Einheit)
    [[nodiscard]] Matrix4 sicht() const { return multipliziert(projektion, modelview); }
    // View::GetViewport (Construct: width / 2 ganzzahlig)
    [[nodiscard]] Matrix4 bildfeld() const;
    // Camera_Move_updateAxes (right[2] bleibt stehen) / Camera_Freemove_updateAxes
    void bewegungsAchsen();
    void freiAchsen();
    // Camera_setOrigin / Camera_setAngles
    void setzeUrsprung(const Vektor3f& o);
    void setzeWinkel(const Vektor3f& w);

    // Camera_motionDelta: Strafe-Art aus Zusatztasten und gedrueckten Knoepfen.
    void strafeAusMaus(bool alt, bool strg, bool shift, bool linksGedrueckt, bool rechtsGedrueckt, int strafeModus);
    // Camera_FreeMove (dx, dy wie uebergeben) und Camera_mouseMove (-x, -y).
    void freiBewegen(int dx, int dy, const KameraEinstellungen& e);
    void mausBewegen(int x, int y, const KameraEinstellungen& e) { freiBewegen(-x, -y, e); }

    // Camera_setMovementFlags: true, wenn damit die Schleife beginnt (dann
    // m_keycontrol_timer.start() und m_keymove_speed_current = 0 - das
    // Tempo setzt diese Funktion schon zurueck).
    bool flagsSetzen(unsigned maske);
    // Camera_clearMovementFlags: true, wenn damit die Schleife endet.
    bool flagsLoeschen(unsigned maske);
    // Cam_KeyControl (dt in Sekunden; workzone fuer MOVE_FOCUS)
    void tastenSteuern(float dt, const KameraEinstellungen& e, const Kasten& workzone);

    // Camera_*_Discrete
    void vorDiskret();
    void zurueckDiskret();
    void hochDiskret();
    void runterDiskret();
    void linksDiskret();
    void rechtsDiskret();
    void drehLinksDiskret();
    void drehRechtsDiskret();
    void neigHochDiskret();
    void neigRunterDiskret();

    // camera_zoom (x, y in Geraetepunkten)
    void zoom(float x, float y, float schritt, bool freiBewegung, bool zumZeiger);
    // wheelmove_scroll fuer ein Rad-Ereignis mit Vorzeichen richtung.
    void rad(int richtung, float x, float y, bool freiBewegung, const KameraEinstellungen& e);
    // camera_orbit_init (mit dem Punkt aus Scene_Intersect) und camera_orbit_scroll
    void orbitBeginnen(const Vektor3f& mitte);
    // Scene_Intersect ohne Treffer: 256 Einheiten vor der Nahebene auf dem
    // Strahl durch den Geraetepunkt (x, y in -1..1).
    [[nodiscard]] Vektor3f strahlOhneTreffer(float x, float y) const;
    // window_to_normalised_device (camera_orbit_init, selection.cpp)
    [[nodiscard]] Vektor2f geraetePunkt(float x, float y) const;
    void orbitRollen();

    // Camera_getFocusPos: die Workzone (Select_getWorkZone)
    [[nodiscard]] Vektor3f fokusPosition(const Kasten& workzone) const;
    // GlobalCamera_ResetAngles
    void winkelZuruecksetzen();
    // Cam_ChangeFloor: true, wenn sich die Hoehe geaendert hat.
    bool etageWechseln(bool hoch, const std::vector<EtagenKandidat>& kandidaten);
};

// Camera_CubeIn / Camera_CubeOut
void kubikHinein(KameraEinstellungen& e);
void kubikHinaus(KameraEinstellungen& e);
// CameraSpeed_increase / CameraSpeed_decrease
void tempoHoeher(KameraEinstellungen& e);
void tempoNiedriger(KameraEinstellungen& e);
// CameraModeNext / CameraModePrev (Quake 3: ohne Lighting)
[[nodiscard]] KameraModus naechsterModus(KameraModus m, bool doom3 = false);
[[nodiscard]] KameraModus vorherigerModus(KameraModus m, bool doom3 = false);
// RenderModeImport: ungueltige Werte werden Textured.
[[nodiscard]] KameraModus modusAusZahl(int wert);

// disable_freelook_button_release: Freelook endet, wenn (kurz und ohne
// Weg) == !beimDruckEingeschaltet - kurz: < 300 ms und < 56 Punkte Weg.
[[nodiscard]] bool freelookEndetBeimLoslassen(double millisekunden, double weg, bool beimDruckEingeschaltet);
// context_menu_try: zweiter Rechtsklick innerhalb 250 ms.
[[nodiscard]] inline bool kontextmenueVersuch(double millisekunden) { return millisekunden < 250.0; }

// Das Licht der festen OpenGL-Pipeline in der Kamera (abgeleitet aus
// Cam_Draw: ein gerichtetes Licht aus Blickrichtung, Umgebung 0,4, diffus 0,4,
// globale Umgebung 0,2, GL_COLOR_MATERIAL): Faktor fuer die Farbe.
[[nodiscard]] float lichtFaktor(const Vektor3f& normale, const Vektor3f& vpn);

}  // namespace bhed::karte

#endif
