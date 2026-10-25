// kartekamera.cpp - Karten-Editor Stufe 1b, Punkt 6: die 3D-Kamera rechnet
// wie NetRadiant-Custom (bhed/karte/kamera.h, bhed/karte/kameraszene.h).
//
// Sollwerte aus radiant/camwindow.cpp (projection_for_camera,
// Camera_getFarClipPlane, Camera_updateModelview, Camera_FreeMove,
// Cam_KeyControl, Camera_*_Discrete, camera_zoom, Camera_getFocusPos,
// GlobalCamera_ResetAngles, Cam_ChangeFloor, Camera_CubeIn/Out,
// CameraModeNext/Prev, disable_freelook_button_release), libs/math/frustum.h,
// libs/entitylib.h (aabb_draw_flatshade), plugins/entity/light.cpp
// (light_draw, sphere_construct_fill, LightRadii) und
// radiant/camwindow.cpp:811-918 (CamDrawWorkzone).
#include "bhed/karte/kamera.h"
#include "bhed/karte/kameraszene.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/modell.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/texdef.h"

#include <cmath>
#include <cstdio>
#include <memory>
#include <string>

using namespace bhed::karte;

namespace {

int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}
bool nah(float a, float b, float eps = 1e-3F) { return std::fabs(a - b) <= eps; }
bool nah(const Vektor3f& a, const Vektor3f& b, float eps = 1e-3F) { return nah(a[0], b[0], eps) && nah(a[1], b[1], eps) && nah(a[2], b[2], eps); }
std::string txt(const Vektor3f& v) {
    char z[96];
    std::snprintf(z, sizeof(z), "(%g %g %g)", static_cast<double>(v[0]), static_cast<double>(v[1]), static_cast<double>(v[2]));
    return z;
}

Kamera kamera(int b, int h, const Vektor3f& o, const Vektor3f& w, const KameraEinstellungen& e) {
    Kamera k;
    k.breite = b;
    k.hoehe = h;
    k.ursprung = o;
    k.winkel = w;
    k.projektionAktualisieren(e);
    k.modelviewAktualisieren();
    return k;
}

void projektion() {
    KameraEinstellungen e;
    pruefe(nah(fernEbene(e), 227023.36F, 0.05F), "Fernebene ohne Cubic Clip: 131072 * sqrt(3)");
    e.kubisch = true;
    pruefe(nah(fernEbene(e), 1448.1547F, 0.01F), "Cubic Clip 14: 2^10.5");
    e.kubikSkala = 1;
    pruefe(nah(fernEbene(e), 16.0F), "Cubic Clip 1: 16");
    e.kubikSkala = 23;
    pruefe(nah(fernEbene(e), 32768.0F), "Cubic Clip 23: 32768");
    kubikHinaus(e);
    pruefe(e.kubikSkala == 23, "CubicClipZoomOut: hoechstens 23");
    e.kubikSkala = 1;
    kubikHinein(e);
    pruefe(e.kubikSkala == 1, "CubicClipZoomIn: mindestens 1");
    // FOV 100 auf der groesseren Seite: 800 x 600 -> half_width tan(50)
    const Matrix4 p = kameraProjektion(1, 1000, 100, 800, 600);
    pruefe(nah(p[0], 1.0F / std::tan(50.0F * 3.14159265F / 180.0F)) && nah(p[5], p[0] * 800.0F / 600.0F),
           "projection_for_camera: Sichtfeld fuer die Breite, Hoehe im Seitenverhaeltnis");
    const Matrix4 q = kameraProjektion(1, 1000, 100, 600, 800);
    pruefe(nah(q[5], 1.0F / std::tan(50.0F * 3.14159265F / 180.0F)) && nah(q[0], q[5] * 800.0F / 600.0F),
           "projection_for_camera: hoch statt breit - Sichtfeld fuer die Hoehe");
    pruefe(nah(p[11], -1) && nah(p[10], -(1000.0F + 1) / (1000.0F - 1)), "matrix4_frustum wie glFrustum");
}

void achsen() {
    KameraEinstellungen e;
    // Pitch 30, Yaw 45 - geschlossene Form: -vpn = (cos p cos y, cos p sin y, sin p)
    Kamera k = kamera(800, 600, Vektor3f(10, 20, 30), Vektor3f(30, 45, 0), e);
    const float cp = std::cos(30.0F * 3.14159265F / 180.0F);
    const float sp = std::sin(30.0F * 3.14159265F / 180.0F);
    const float s = std::sqrt(0.5F);
    pruefe(nah(-k.vpn, Vektor3f(cp * s, cp * s, sp)), "Blickrichtung -vpn " + txt(-k.vpn));
    pruefe(nah(k.vright, Vektor3f(s, -s, 0)), "vright (sin y, -cos y, 0) " + txt(k.vright));
    pruefe(nah(k.vup, Vektor3f(-sp * s, -sp * s, cp)), "vup " + txt(k.vup));
    // Die Kamera steht im Ursprung der Modelview-Umkehrung
    const Vektor3f auge = transformierterPunkt(k.modelview, Vektor3f(10, 20, 30));
    pruefe(nah(auge, Vektor3f(0, 0, 0)), "Modelview: der Ursprung liegt im Auge");
    k.bewegungsAchsen();
    pruefe(nah(k.vorn, Vektor3f(s, s, 0)) && nah(k.seite[0], s) && nah(k.seite[1], -s), "Camera_Move_updateAxes: 2D");
    const Frustum f = frustumAusViewProj(k.sicht());
    pruefe(nah(static_cast<float>(laenge(f.links.normal())), 1) && nah(static_cast<float>(laenge(f.oben.normal())), 1),
           "frustum_from_viewproj: normierte Ebenen");
    const Matrix4 vp = k.bildfeld();
    pruefe(vp[0] == 400 && vp[5] == 300, "View::Construct: Viewport width/2, height/2");
    // matrix4_full_inverse: Inverse mal Matrix = Einheit
    const Matrix4 m = k.sicht();
    const Matrix4 einh = multipliziert(vollInvers(m), m);
    bool einheit = true;
    for (std::size_t i = 0; i < 16; ++i) { einheit = einheit && nah(einh[i], kEinheitsMatrix[i], 1e-3F); }
    pruefe(einheit, "matrix4_full_inverse");
    // Scene_Intersect ohne Treffer: Bildmitte = Nahebene + 256 vorwaerts
    Kamera g = kamera(800, 600, Vektor3f(0, 0, 0), Vektor3f(0, 0, 0), KameraEinstellungen());
    pruefe(nah(g.strahlOhneTreffer(0, 0), Vektor3f(257, 0, 0), 0.05F), "Scene_Intersect ohne Treffer: 1 + 256 vor dem Auge " + txt(g.strahlOhneTreffer(0, 0)));
    const Vektor2f gp = g.geraetePunkt(400, 299);
    pruefe(nah(gp[0], 0) && nah(gp[1], 0), "window_to_normalised_device: (w/2, h-1-h/2) ist die Mitte");
}

void freelook() {
    KameraEinstellungen e;
    Kamera k = kamera(800, 600, Vektor3f(0, 0, 0), Vektor3f(0, 90, 0), e);
    k.freiAchsen();
    // Maus 10 nach rechts: Yaw -1 (0.0333333 * 3 Grad je Punkt)
    k.mausBewegen(10, 0, e);
    pruefe(nah(k.winkel[1], 89.000001F, 1e-4F), "Freelook: 10 Punkte rechts -> Yaw -1 (ist " + std::to_string(k.winkel[1]) + ")");
    k.mausBewegen(0, -20, e);
    pruefe(nah(k.winkel[0], 2.0F, 1e-4F), "Freelook: Maus hoch -> Pitch +2");
    e.mausUmkehren = true;
    k.mausBewegen(0, -20, e);
    pruefe(nah(k.winkel[0], 0.0F, 1e-4F), "Freelook umgekehrt: Maus hoch -> Pitch -2");
    e.mausUmkehren = false;
    k.mausBewegen(0, -2000, e);
    pruefe(k.winkel[0] == 90, "Pitch auf +90 begrenzt");
    k.winkel = Vektor3f(0, 0.5F, 0);
    k.mausBewegen(10, 0, e);
    pruefe(nah(k.winkel[1], 359.5F, 1e-3F), "Yaw <= 0 -> +360 (ist " + std::to_string(k.winkel[1]) + ")");
    k.winkel = Vektor3f(0, 359.5F, 0);
    k.mausBewegen(-10, 0, e);
    pruefe(nah(k.winkel[1], 0.5F, 1e-3F), "Yaw >= 360 -> -360");
    // Strafe (Modus 3, Strg): Maus rechts -> Kamera nach rechts, hoch -> hoch
    k = kamera(800, 600, Vektor3f(0, 0, 0), Vektor3f(0, 0, 0), e);
    k.strafeAusMaus(false, true, false, false, false, 3);
    pruefe(k.strafe && !k.strafeVor, "Strafe-Modus 3 mit Strg: seitlich und hoch/runter");
    k.mausBewegen(5, -7, e);
    pruefe(nah(k.ursprung, Vektor3f(0, -5, 7)), "Strafe: rechts (-y bei Yaw 0) und hoch " + txt(k.ursprung));
    k.strafeAusMaus(false, false, true, false, false, 3);
    pruefe(k.strafe && k.strafeVor, "Strafe-Modus 3 mit Shift: vor/zurueck");
    k.strafeAusMaus(true, false, false, false, true, 3);
    pruefe(k.orbit && !k.strafe, "Alt + M2: Orbit statt Strafe");
    k.strafeAusMaus(false, false, false, true, false, 0);
    pruefe(!k.strafe, "Strafe-Modus 0: auch M1 strafet nicht");
    k.strafeAusMaus(false, false, false, true, false, 2);
    pruefe(k.strafe && !k.strafeVor, "M1 gehalten: seitlich/hoch");
    // Wahrheitstabelle Freelook beim Loslassen
    pruefe(!freelookEndetBeimLoslassen(100, 10, true), "M2 kurz beim Einschalten: Freelook bleibt an");
    pruefe(freelookEndetBeimLoslassen(500, 10, true), "M2 lang beim Einschalten: Freelook aus");
    pruefe(freelookEndetBeimLoslassen(100, 10, false), "M2 kurz im Freelook: aus");
    pruefe(!freelookEndetBeimLoslassen(100, 80, false), "M2 gezogen im Freelook: bleibt an");
    pruefe(kontextmenueVersuch(200) && !kontextmenueVersuch(250), "Doppel-M2 unter 250 ms");
}

void tasten() {
    KameraEinstellungen e;
    Kamera k = kamera(800, 600, Vektor3f(0, 0, 0), Vektor3f(0, 0, 0), e);
    const bool beginnt = k.flagsSetzen(kBewegVor);
    pruefe(beginnt && k.tempo == 0, "Camera_setMovementFlags: erste Taste startet bei Tempo 0");
    pruefe(!k.flagsSetzen(kBewegLinks), "weitere Taste: kein Neustart");
    k.flagsLoeschen(kBewegLinks);
    // dt 0.1: Tempo 500 * 0.1 / 200 * 1000 = 250 -> 25 Einheiten
    k.tastenSteuern(0.1F, e, Kasten());
    pruefe(nah(k.tempo, 250) && nah(k.ursprung, Vektor3f(25, 0, 0)), "Cam_KeyControl: Beschleunigung " + txt(k.ursprung));
    k.tastenSteuern(0.1F, e, Kasten());
    pruefe(nah(k.tempo, 500) && nah(k.ursprung, Vektor3f(75, 0, 0)), "Cam_KeyControl: hoechstens m_nMoveSpeed");
    pruefe(k.flagsLoeschen(kBewegVor) && k.bewegung == 0, "letzte Taste los: Schleife endet");
    k.flagsSetzen(kBewegDrehLinks);
    k.tastenSteuern(0.2F, e, Kasten());
    pruefe(nah(k.winkel[1], 15.0F), "Drehen 25 * Empfindlichkeit Grad/s (75 * 0.2 = 15)");
    k.flagsLoeschen(kBewegAlle);
    // diskret: 32 Einheiten 2D, 22.5 Grad
    k = kamera(800, 600, Vektor3f(0, 0, 0), Vektor3f(45, 90, 0), e);
    k.vorDiskret();
    pruefe(nah(k.ursprung, Vektor3f(0, 32, 0)), "CameraForward diskret: 32 in der Ebene (Pitch egal) " + txt(k.ursprung));
    k.rechtsDiskret();
    pruefe(nah(k.ursprung, Vektor3f(32, 32, 0)), "CameraStrafeRight diskret " + txt(k.ursprung));
    k.drehLinksDiskret();
    pruefe(nah(k.winkel[1], 112.5F), "CameraLeft diskret: +22.5");
    k.neigHochDiskret();
    k.neigHochDiskret();
    k.neigHochDiskret();
    pruefe(k.winkel[0] == 90, "CameraAngleUp diskret: bis 90");
    k.hochDiskret();
    pruefe(nah(k.ursprung[2], 32), "CameraUp diskret: +32 in z");
    tempoHoeher(e);
    pruefe(e.tempo == 750, "CameraSpeedInc +250");
    e.tempo = 9900;
    tempoHoeher(e);
    pruefe(e.tempo == 9999, "CameraSpeedInc hoechstens 9999");
    e.tempo = 100;
    tempoNiedriger(e);
    pruefe(e.tempo == 50, "CameraSpeedDec mindestens 50");
}

void rad() {
    KameraEinstellungen e;
    Kamera k = kamera(800, 600, Vektor3f(0, 0, 0), Vektor3f(0, 0, 0), e);
    // Zoom zur Mitte = entlang der Blickrichtung (affine Inverse wie NRC)
    k.rad(1, 400, 300, false, e);
    pruefe(nah(k.ursprung, Vektor3f(100, 0, 0), 0.01F), "Mausrad zur Bildmitte: +100 vorwaerts " + txt(k.ursprung));
    k.rad(-1, 0, 0, true, e);
    pruefe(nah(k.ursprung, Vektor3f(0, 0, 0), 0.01F), "Mausrad im Freelook: -100 entlang -vpn");
    k.rad(1, 799, 300, false, e);
    pruefe(k.ursprung[1] < -1 && k.ursprung[0] > 50, "Mausrad rechts: Richtung zum Zeiger " + txt(k.ursprung));
    k = kamera(800, 600, Vektor3f(0, 0, 0), Vektor3f(0, 0, 0), e);
    k.flagsSetzen(kBewegFokus);
    k.rad(1, 400, 300, false, e);
    pruefe(k.fokusVersatz == 1 && nah(k.ursprung, Vektor3f(0, 0, 0)), "Mausrad bei gehaltenem Fokus: nur m_focus_offset");
}

void befehle() {
    KameraEinstellungen e;
    // Focus: Wuerfel +-64 in der Mitte, Blick +x, quadratisch, FOV 100 ->
    // offset = 64 + 64 * cot(50)
    Kamera k = kamera(800, 800, Vektor3f(500, 0, 0), Vektor3f(0, 0, 0), e);
    const Vektor3f f = k.fokusPosition(Kasten(Vektor3f(0, 0, 0), Vektor3f(64, 64, 64)));
    const float soll = 64.0F + (64.0F / std::tan(50.0F * 3.14159265F / 180.0F));
    pruefe(nah(f, Vektor3f(-soll, 0, 0), 0.01F), "Camera_getFocusPos " + txt(f) + " soll " + std::to_string(-soll));
    const Vektor3f f2 = k.fokusPosition(Kasten(Vektor3f(0, 0, 0), Vektor3f(1, 1, 1)));
    pruefe(nah(f2, Vektor3f(-64, 0, 0), 0.01F), "Camera_getFocusPos: mindestens 64 " + txt(f2));
    k.fokusVersatz = 4;
    const Vektor3f f3 = k.fokusPosition(Kasten(Vektor3f(0, 0, 0), Vektor3f(1, 1, 1)));
    pruefe(nah(f3, Vektor3f(-32, 0, 0), 0.01F), "m_focus_offset 4: halber Abstand " + txt(f3));
    // CenterView
    k.winkel = Vektor3f(33, 100, 0);
    k.winkelZuruecksetzen();
    pruefe(k.winkel == Vektor3f(0, 90, 0), "CenterView: Pitch 0, Yaw 100 -> 90");
    k.winkel = Vektor3f(0, 11.3F, 0);
    k.winkelZuruecksetzen();
    pruefe(k.winkel[1] == 0, "CenterView: 11.3 -> 0 (+11, nicht +11.25)");
    k.winkel = Vektor3f(0, 11.5F, 0);
    k.winkelZuruecksetzen();
    pruefe(k.winkel[1] == 22.5F, "CenterView: 11.5 -> 22.5");
    // Floors: Boden 0..16 und Decke/Etage 100..116 unter der Kamera
    std::vector<EtagenKandidat> c;
    c.push_back({kastenAusMinMax(Vektor3f(-100, -100, 0), Vektor3f(100, 100, 16)), false});
    c.push_back({kastenAusMinMax(Vektor3f(-100, -100, 100), Vektor3f(100, 100, 116)), false});
    c.push_back({kastenAusMinMax(Vektor3f(500, 500, 400), Vektor3f(600, 600, 416)), false});   // nicht unter der Kamera
    k = kamera(800, 600, Vektor3f(0, 0, 80), Vektor3f(0, 0, 0), e);
    pruefe(k.etageWechseln(false, c) && k.ursprung[2] == 48, "DownFloor: Oberkante 16 + 32");
    pruefe(k.etageWechseln(true, c) && k.ursprung[2] == 148, "UpFloor: naechste Etage 116 + 32");
    pruefe(!k.etageWechseln(true, c) && k.ursprung[2] == 148, "UpFloor ganz oben: bleibt");
    c[2].gewaehlt = true;
    pruefe(k.etageWechseln(true, c) && k.ursprung[2] == 448, "UpFloor: Ausgewaehltes zaehlt auch ausserhalb");
    k.ursprung[2] = -500;
    pruefe(k.etageWechseln(true, c) && k.ursprung[2] == -16, "UpFloor unter allem: tiefste Unterkante - 16");
    // Darstellungsarten
    pruefe(naechsterModus(KameraModus::TexturDraht) == KameraModus::Draht, "CameraModeNext: Textured+Wire -> Wireframe (ohne Lighting)");
    pruefe(vorherigerModus(KameraModus::Draht) == KameraModus::TexturDraht, "CameraModePrev: Wireframe -> Textured+Wire");
    pruefe(naechsterModus(KameraModus::Textur) == KameraModus::TexturDraht, "CameraModeNext: Textured -> Textured+Wire");
    pruefe(modusAusZahl(9) == KameraModus::Textur && modusAusZahl(1) == KameraModus::Flach, "RenderModeImport");
    // Licht der Kamera
    pruefe(nah(lichtFaktor(Vektor3f(1, 0, 0), Vektor3f(1, 0, 0)), 1.0F) && nah(lichtFaktor(Vektor3f(0, 0, 1), Vektor3f(1, 0, 0)), 0.6F) &&
               nah(lichtFaktor(Vektor3f(-1, 0, 0), Vektor3f(1, 0, 0)), 0.6F),
           "Licht: zur Kamera 1.0, seitlich und abgewandt 0.6");
}

void szene() {
    // Kasten: 6 Vierecke = 12 Dreiecke, Oberseite mit Normale +z
    std::vector<KameraPunkt> d;
    std::vector<KameraKante> k;
    kastenFuellung(Kasten(Vektor3f(0, 0, 0), Vektor3f(8, 8, 8)), 0xFF0000FFU, d, &k);
    pruefe(d.size() == 36 && k.size() == 24, "aabb_draw_flatshade: 12 Dreiecke, 24 Umrisskanten");
    pruefe(d[12].normale == Vektor3f(0, 0, 1) && d[12].pos[2] == 8, "dritte Flaeche: Oberseite mit (0,0,1)");
    // Umlauf im Uhrzeigersinn von aussen (glFrontFace GL_CW): Oberseite von oben
    const Vektor3f n = kreuz(d[13].pos - d[12].pos, d[14].pos - d[12].pos);
    pruefe(n[2] < 0, "Oberseite: im Uhrzeigersinn von oben (Vorderseite fuer GL_CW)");
    // Licht-Oktaeder: 8 Dreiecke, erste Normale die vorige (normals[4])
    d.clear();
    k.clear();
    lichtOktaederFuellung(Kasten(Vektor3f(0, 0, 0), Vektor3f(8, 8, 8)), 0, d, &k);
    const float f = 0.70710678F;
    pruefe(d.size() == 24 && k.size() == 24, "light_draw: 8 Dreiecke");
    pruefe(nah(d[0].normale, Vektor3f(0, f, -f)) && nah(d[2].normale, Vektor3f(0, f, f)) && nah(d[3].normale, Vektor3f(0, f, f)) &&
               nah(d[5].normale, Vektor3f(f, 0, f)),
           "light_draw: glNormal vor der dritten Ecke");
    pruefe(kugelPunkte().size() == 553, "sphere_construct_fill: 553 Punkte");
    d.clear();
    kugelFuellung(Vektor3f(0, 0, 0), 10, 0, d, nullptr);
    pruefe(d.size() == 551 * 3, "sphere_draw_fill: Dreiecksstreifen mit 551 Dreiecken");
    d.clear();
    kugelFuellung(Vektor3f(0, 0, 0), 0, 0, d, nullptr);
    pruefe(d.empty(), "Radius 0: nichts");
    // LightRadii: Vorgabe 300, nicht linear
    LichtRadien r = lichtRadien("", "", "", "", "");
    pruefe(nah(r.r[0], std::sqrt(300.0F * 7500.0F)) && nah(r.r[1], std::sqrt(300.0F * 7500.0F / 48.0F)) && !r.linear,
           "LightRadii: Vorgabe 300, sqrt(I * 7500 / Toleranz)");
    pruefe(r.gezeichnet()[0] == r.r[1] && r.gezeichnet()[1] == r.r[2], "nicht linear: Radien 1 und 2 werden gezeichnet");
    r = lichtRadien("", "1000", "2", "", "1");
    pruefe(r.linear && nah(r.r[0], (2000.0F * 7500.0F / 8000.0F) - 1.0F) && r.gezeichnet()[0] == r.r[0], "linear (spawnflags 1), scale 2");
    r = lichtRadien("500", "1000", "", "2", "1");
    pruefe(nah(r.r[1], ((500.0F * 7500.0F / 8000.0F) - 48.0F) / 2.0F), "_light vor light, fade teilt linear");
    std::vector<Linie> l;
    kugelDraht(Vektor3f(0, 0, 0), 100, 0, l);
    pruefe(l.size() == 72 && nah(l[0].a, Vektor3f(100, 0, 0)), "sphere_draw_wire: 3 Kreise zu 24 Strecken");
    pruefe(nah(lichtFarbeNormiert("0.5 0.25 0"), Vektor3f(1, 0.5F, 0)) && nah(lichtFarbeNormiert("0 0 0"), Vektor3f(1, 1, 1)) &&
               nah(lichtFarbeNormiert("x"), Vektor3f(1, 1, 1)),
           "read_colour_normalized");
    // Workzone: Wuerfel +-32, Raster 16 - Striche rot/weiss, 1024 ueber die Kanten
    std::vector<KameraKante> rot;
    std::vector<KameraKante> weiss;
    workzone3d(Kasten(Vektor3f(0, 0, 0), Vektor3f(32, 32, 32)), 16, rot, weiss);
    bool achsparallel = !rot.empty() && !weiss.empty();
    float weit = 0;
    for (const KameraKante& x : rot) {
        int gleich = 0;
        for (int i = 0; i < 3; ++i) { gleich += x.a[i] == x.b[i] ? 1 : 0; }
        achsparallel = achsparallel && gleich == 2 && nah(static_cast<float>(laenge(x.b - x.a)), 16);
        weit = std::max(weit, std::fabs(x.a[0]));
    }
    pruefe(achsparallel, "Workzone: achsparallele Striche zu einer Rasterweite");
    pruefe(weit > 1000 && weit <= 32 + 1024 + 16, "Workzone: bis 1024 ueber die Auswahl");
    pruefe((rot.front().fa >> 24) == 0 || (rot.front().fb >> 24) < 255, "Workzone: aussen ausgeblendet");
    // Szene: worldspawn-Wuerfel, ausgewaehltes Licht, Gruppe mit Winkel
    Karte karte;
    TexturProjektion proj;
    texdefStandard(TexdefArt::Quake, proj);
    auto welt = karte.weltHolenOderAnlegen();
    auto p = std::make_shared<Pinsel>();
    p->quaderBauen(Kasten(Vektor3f(0, 0, 0), Vektor3f(64, 64, 8)), "textures/a/b", proj, karte.kontext);
    welt->kindEinfuegen(p);
    auto licht = karte.neueEntitaet("light", false);
    licht->setzen("origin", "0 0 64");
    licht->setzen("_color", "1 0.5 0");
    karte.entitaetEinfuegen(licht);
    karte.auswahl.waehlen(licht);
    KameraSzenenOptionen o;
    o.kanten = true;
    const KameraSzene s = kameraSzeneBauen(karte, o);
    std::size_t shader = 0;
    std::size_t fuell = 0;
    std::size_t additiv = 0;
    for (const KameraStapel& st : s.stapel) {
        if (st.art == StapelArt::Shader) {
            ++shader;
            pruefe(st.shader == "textures/a/b" && st.dreiecke.size() == 36 && st.kanten.size() == 24,
                   "Pinsel: 6 Flaechen als Faecher (12 Dreiecke), 24 Umrisskanten");
        } else if (st.art == StapelArt::Fuellung) {
            ++fuell;
            pruefe(st.farbe == farbeAus(1, 0.5F, 0) && st.dreiecke.size() == 24 && st.kantenGewaehlt.size() == 24,
                   "Licht: Oktaeder in der normierten _color");
        } else {
            ++additiv;
            pruefe(st.dreiecke.size() == 2 * 551 * 3 && st.farbe == farbeAus(1.0F / 8, 0.5F / 8, 0),
                   "ausgewaehltes Licht: zwei Radien-Kugeln in Farbe / 8 (additiv)");
        }
    }
    pruefe(shader == 1 && fuell == 1 && additiv == 1, "Szene: ein Shader, eine Fuellung, eine additive Kugel");
    pruefe(s.wahlDreiecke.size() == 24 && s.wahlKanten.size() == 24, "Auswahl: Toenung und Overlay des Lichts");
    pruefe(s.namen.size() == 1 && s.namen[0].art == 1 && s.namen[0].text == "light", "Name des ausgewaehlten Lichts (gelb)");
    pruefe(s.etagen.size() == 2, "FloorHeightWalker: Pinsel und Licht, ohne worldspawn");
    licht->setzen("target", "t1");
    const KameraSzene s2 = kameraSzeneBauen(karte, o);
    std::size_t add2 = 0;
    for (const KameraStapel& st : s2.stapel) { add2 += st.art == StapelArt::Additiv ? 1 : 0; }
    pruefe(add2 == 0, "Licht mit target: keine Radien");
}

// Stufe 4 Punkt 0a (ASan, Selbsttest karteneditor): kameraSzeneBauen merkte
// sich den Shader der letzten Flaeche als Zeiger - bei misc_model in die
// ModellAnsicht, die nach jeder Entitaet freigegeben wird. Liegt die Ansicht
// der naechsten Entitaet an derselben Stelle, verglich der Zeiger den neuen
// Shader mit sich selbst: das zweite Modell landete im Stapel des ersten
// (und las freigegebenen Speicher).
class ZweiModelle final : public ModellQuelle {
public:
    [[nodiscard]] ModellAnsicht ansicht(const Entitaet& e) const override {
        auto m = std::make_shared<Modell>();
        ModellFlaeche f;
        f.shader = "models/test/" + e.wert("model") + "_mit_langem_namen";
        f.punkte = {{Vektor3f(0, 0, 0), Vektor3f(0, 0, 1), Vektor2f(0, 0)}, {Vektor3f(8, 0, 0), Vektor3f(0, 0, 1), Vektor2f(1, 0)},
                    {Vektor3f(0, 8, 0), Vektor3f(0, 0, 1), Vektor2f(0, 1)}};
        f.indizes = {0, 1, 2};
        f.grenzen = Kasten(Vektor3f(4, 4, 0), Vektor3f(4, 4, 0));
        m->flaechen.push_back(f);
        m->grenzen = f.grenzen;
        ModellAnsicht a;
        a.modell = m;
        a.shader.push_back(f.shader);   // eigene Zeichenkette in der Ansicht (wie Skin/_remap)
        return a;
    }
};

void modellShader() {
    ZweiModelle quelle;
    Karte karte;
    karte.kontext.modelle = &quelle;
    (void)karte.weltHolenOderAnlegen();
    for (int i = 0; i < 6; ++i) {
        auto m = karte.neueEntitaet("misc_model", false);
        m->setzen("model", "m" + std::to_string(i));
        m->setzen("origin", std::to_string(i * 64) + " 0 0");
        karte.entitaetEinfuegen(m);
    }
    KameraSzenenOptionen o;
    const KameraSzene s = kameraSzeneBauen(karte, o);
    int richtig = 0;
    for (const KameraStapel& st : s.stapel) {
        if (st.art == StapelArt::Shader && st.shader.rfind("models/test/m", 0) == 0) { richtig += st.dreiecke.size() == 3 ? 1 : 0; }
    }
    pruefe(richtig == 6, "misc_model: jedes Modell in seinem eigenen Shader-Stapel (" + std::to_string(richtig) + " von 6)");
}

}  // namespace

int main() {
    modellShader();
    projektion();
    achsen();
    freelook();
    tasten();
    rad();
    befehle();
    szene();
    std::printf("kartekamera: %d Fehlschlaege\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
