// app_modelltab_nav.cpp - MBNAV (11.11.2026): Navigation in den Ansichten des Model-Tabs 1:1 wie 3ds Max bzw. Blender
//
// shank 10.11.: "ist das controlling wie in 3ds max in 3ds max modus? mit Kamera ziehen usw.? Das alles gleich ist?"
// Abgleich je Bedienung: docs/intern/navigation_abgleich.md, Entscheidungen und Rot/Gruen: docs/intern/NOTIZEN_mbnav_1011.md.
//
// Wie 3ds Max (Autodesk-Hilfe "Zooming, Panning, and Orbiting Views", "Orbit, Orbit Selected, Orbit SubObject", "Pan View",
// Customize > Mouse, Hotkey-Tabelle 2026): Mitteltaste schiebt (Umschalt: die zuerst bewegte Achse), Alt+Mitteltaste kreist im
// gewaehlten Orbit-Modus (eine Achsenansicht wird "Orthographic"; Umschalt: die zuerst benutzte Achse), Strg+Alt+Mitteltaste
// zoomt, Rad und [ ] zoomen um den Mauspunkt (Zoom About Mouse Point, ab Werk an), die Modi Zoom (Alt+Z), Pan (Strg+P), Orbit
// (Strg+R) und Zoom Region (Strg+W) laufen mit der linken Taste bis Rechtsklick/Esc, I schiebt zum Mauszeiger, Umschalt+Z /
// Umschalt+Y nehmen Ansichtsaenderungen zurueck. 3ds Max nennt fuer Drehung je Bildpunkt und Zoom je Raste keine Zahl - dort
// bleiben die Werte des Model-Tabs (0,4 Grad je Bildpunkt wie Blender; Rad 0,9 bzw. 0,85).
//
// Wie Blender 5.0 - die Rechnungen sind aus dem Quelltext portiert (Copyright Blender Authors, GPL-2.0-or-later; JK Studio ist
// GPL v2): view3d_navigate_view_rotate.cc (viewrotate_apply: Turntable, viewrotate_apply_snap), view3d_navigate.cc
// (viewmove_apply, ED_view3d_persp_ensure), view3d_navigate_view_zoom.cc (viewzoom_scale_value, view_zoom_apply_step,
// view_zoom_to_window_xy_3d), view3d_navigate_view_orbit.cc, _view_roll.cc, _view_pan.cc, _view_axis.cc (relative),
// _view_all.cc (view3d_from_minmax, VIEW3D_MARGIN 1,4), _view_center_pick.cc; Vorgaben aus userdef_default.c (Turntable
// 0,4 Grad je Bildpunkt, pad_rot_angle 15, smooth_viewtx 200, USER_AUTOPERSP an, USER_ZOOM_DOLLY, Orbit Around Selection und
// Zoom to Mouse Position aus), Tasten aus blender_default.py (km_view3d).
#include "app_internal.h"
#include "ui_menue.h"   // KLEIN2: ui::menue - Menues bleiben im Fenster

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace bhed::gui {
namespace {

ModellTab& mt() { return g_app->mtab; }

constexpr float kPi = 3.14159265F;
constexpr float kGrad = 180.0F / kPi;
constexpr float kDrehProPx = 0.4F;      // Blender view_rotate_sensitivity_turntable = DEG2RAD(0.4); 3ds Max nennt keine Zahl
constexpr float kNahAbstand = 1.5F;     // Blender: nicht naeher als clip_start * 1,5 (die nahe Ebene des Model-Tabs liegt bei 1)
constexpr float kFernAbstand = 60000.0F;   // wie Orbit::zoom
constexpr float kHalbMin = 0.5F;        // Orthoansichten: wie bisher (eineAnsicht bis rc611)
constexpr float kHalbMax = 8192.0F;
constexpr float kBlZoom = 1.2F;         // view_zoom_apply_step: step = 1.2f
constexpr float kBlMarge = 1.4F;        // VIEW3D_MARGIN
constexpr float kMaxZug = 3.0F;         // Bildpunkte, bis ein Zug als Zug zaehlt (Blender drag_threshold_mouse 3)

struct V3 {
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
};
V3 operator+(V3 a, V3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
V3 operator-(V3 a, V3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
V3 operator*(V3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
float dot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
V3 kreuz(V3 a, V3 b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
float laenge(V3 a) { return std::sqrt(dot(a, a)); }
V3 norm(V3 a) {
    const float l = laenge(a);
    return l > 1e-12F ? a * (1.0F / l) : V3{};
}
V3 v3(const float* p) { return {p[0], p[1], p[2]}; }
void nach(V3 v, float* p) {
    p[0] = v.x;
    p[1] = v.y;
    p[2] = v.z;
}

// Rodrigues: v um die (normierte) Achse um `grad` drehen, Rechte-Hand-Regel.
V3 dreh(V3 v, V3 achse, float grad) {
    const float w = grad / kGrad;
    const float c = std::cos(w);
    const float s = std::sin(w);
    return v * c + kreuz(achse, v) * s + achse * (dot(achse, v) * (1.0F - c));
}

// Was eine Ansicht zeigt: Art, Winkel (pitch, yaw, roll wie Camera::angles), der Punkt in der Bildmitte (Perspektive: der
// Drehpunkt in Orbit-Abstand) und die halbe Bildhoehe in Einheiten in dessen Tiefe. In diesen Groessen sind Perspektive und
// Orthoansichten dasselbe - schieben, zoomen und kreisen rechnen fuer beide gleich.
struct Sicht {
    AnsichtArt art = AnsichtArt::Perspektive;
    float w[3]{};
    float p[3]{};
    float halb = 64.0F;
};

bool achsenArt(AnsichtArt a) { return a != AnsichtArt::Perspektive && a != AnsichtArt::Orthographisch; }

void basis(const float w[3], V3& f, V3& r, V3& u) {
    Camera c;
    for (int k = 0; k < 3; ++k) { c.angles[k] = w[k]; }
    float x[3];
    c.forward(x);
    f = v3(x);
    c.right(x);
    r = v3(x);
    c.up(x);
    u = v3(x);
}

// Die Umkehrung von Camera::forward/right/up. Genau senkrecht sind Gier und Roll nicht zu trennen: dann Roll 0, die
// Rechtsachse legt den Gierwinkel fest (right = (sin y, -cos y, 0), wie winkelAusAchsen in app_modelltab_ansicht.cpp).
void winkelAus(V3 f, V3 r, V3 u, float w[3]) {
    w[0] = -std::asin(std::clamp(f.z, -1.0F, 1.0F)) * kGrad;
    if (std::fabs(f.z) < 1.0F - 1e-6F) {
        w[1] = std::atan2(f.y, f.x) * kGrad;
        w[2] = std::atan2(-r.z, u.z) * kGrad;
    } else {
        w[1] = std::atan2(r.x, -r.y) * kGrad;
        w[2] = 0.0F;
    }
}

void achsenVon(const Sicht& s, V3& f, V3& r, V3& u) {
    if (achsenArt(s.art)) {
        float rr[3];
        float hh[3];
        float vv[3];
        modellAnsichtAchsenVon(s.art, rr, hh, vv);
        f = v3(vv);
        r = v3(rr);
        u = v3(hh);
        return;
    }
    basis(s.w, f, r, u);
}

float tanHalb() { return std::max(std::tan(g_app->modelCam.fovY * kPi / 360.0F), 1e-4F); }

Sicht sichtVon(int n) {
    const ModellAnsicht& a = mt().ansichten[n];
    Sicht s;
    s.art = a.art;
    modellAnsichtWinkel(a, s.w);
    if (a.art == AnsichtArt::Perspektive) {
        g_app->modelOrbit.pivot(g_app->modelCam, s.p);
        s.halb = g_app->modelOrbit.distance * tanHalb();
    } else {
        for (int k = 0; k < 3; ++k) { s.p[k] = a.mitte[k]; }
        s.halb = a.halbHoehe;
    }
    return s;
}

void halbGrenzen(const Sicht& s, float& lo, float& hi) {
    if (s.art == AnsichtArt::Perspektive) {
        lo = kNahAbstand * tanHalb();
        hi = kFernAbstand * tanHalb();
    } else {
        lo = kHalbMin;
        hi = kHalbMax;
    }
}

// Eine freie orthographische Ansicht, die genau wie eine Achsenansicht schaut, heisst wie sie (das Feld zeigt [Vorne] ...).
void benennen(Sicht& s) {
    if (s.art != AnsichtArt::Orthographisch) { return; }
    V3 f;
    V3 r;
    V3 u;
    basis(s.w, f, r, u);
    static const AnsichtArt kArten[6] = {AnsichtArt::Oben, AnsichtArt::Unten, AnsichtArt::Vorne, AnsichtArt::Hinten, AnsichtArt::Links, AnsichtArt::Rechts};
    for (const AnsichtArt art : kArten) {
        float rr[3];
        float hh[3];
        float vv[3];
        modellAnsichtAchsenVon(art, rr, hh, vv);
        if (dot(f, v3(vv)) > 1.0F - 1e-6F && dot(u, v3(hh)) > 1.0F - 1e-6F) {
            s.art = art;
            return;
        }
    }
}

void sichtSetzen(int n, const Sicht& s) {
    ModellTab& t = mt();
    ModellAnsicht& a = t.ansichten[n];
    a.animStart = -1.0;
    a.art = s.art;
    float lo = 0.0F;
    float hi = 0.0F;
    halbGrenzen(s, lo, hi);
    const float halb = std::clamp(s.halb, lo, hi);
    if (s.art == AnsichtArt::Perspektive) {
        Camera& c = g_app->modelCam;
        for (int k = 0; k < 3; ++k) { c.angles[k] = s.w[k]; }
        g_app->modelOrbit.distance = halb / tanHalb();
        float f[3];
        c.forward(f);
        for (int k = 0; k < 3; ++k) { c.pos[k] = s.p[k] - f[k] * g_app->modelOrbit.distance; }
    } else {
        if (s.art == AnsichtArt::Orthographisch) {
            for (int k = 0; k < 3; ++k) { t.orthoWinkel[k] = s.w[k]; }
        }
        for (int k = 0; k < 3; ++k) { a.mitte[k] = s.p[k]; }
        a.halbHoehe = halb;
    }
    g_app->modelDirty = true;
}

bool gleich(const Sicht& a, const Sicht& b) {
    if (a.art != b.art || std::fabs(a.halb - b.halb) > 1e-4F * std::max(a.halb, 1.0F)) { return false; }
    for (int k = 0; k < 3; ++k) {
        if (std::fabs(a.w[k] - b.w[k]) > 1e-3F || std::fabs(a.p[k] - b.p[k]) > 1e-4F * std::max(a.halb, 1.0F)) { return false; }
    }
    return true;
}

// Eine Achsenansicht, die sich drehen soll: in 3ds Max wird sie "Orthographic", in Blender (Auto Perspective, ED_view3d_
// persp_ensure) die Perspektive. Mitte und Groesse bleiben.
void freiMachen(Sicht& s, bool perspektive) {
    if (achsenArt(s.art)) { s.art = perspektive ? AnsichtArt::Perspektive : AnsichtArt::Orthographisch; }
}

// --- Schieben, Zoomen, Drehen -----------------------------------------------------------------------------------------

// Schieben um Bildpunkte: der Punkt in der Tiefe des Drehpunkts folgt der Maus genau (Blender viewmove_apply ueber
// ED_view3d_win_to_delta mit dem zfac des Drehpunkts; in den Orthoansichten war es schon so).
void schieben(Sicht& s, float dx, float dy, float hoehe) {
    V3 f;
    V3 r;
    V3 u;
    achsenVon(s, f, r, u);
    const float jePunkt = 2.0F * s.halb / std::max(hoehe, 1.0F);
    nach(v3(s.p) + r * (-dx * jePunkt) + u * (dy * jePunkt), s.p);
}

// Zoomen um einen Faktor der Bildhoehe; `um` (ein Punkt der Ebene durch den Drehpunkt) bleibt an seiner Stelle im Bild
// (Blender view_zoom_to_window_xy_3d, 3ds Max "Zoom About Mouse Point").
void zoomen(Sicht& s, float faktor, const V3* um) {
    float lo = 0.0F;
    float hi = 0.0F;
    halbGrenzen(s, lo, hi);
    const float neu = std::clamp(s.halb * faktor, lo, hi);
    const float f = neu / std::max(s.halb, 1e-9F);
    if (um != nullptr) { nach(*um + (v3(s.p) - *um) * f, s.p); }
    s.halb = neu;
}

// Die Kamera um eine Weltachse durch `um` drehen (Rechte-Hand-Regel): die Blickachsen und der Punkt in der Bildmitte.
void drehen(Sicht& s, V3 achse, float grad, V3 um) {
    V3 f;
    V3 r;
    V3 u;
    basis(s.w, f, r, u);
    winkelAus(dreh(f, achse, grad), dreh(r, achse, grad), dreh(u, achse, grad), s.w);
    nach(um + dreh(v3(s.p) - um, achse, grad), s.p);
}

// Der Punkt der Ebene durch den Drehpunkt (senkrecht zum Blick) unter einer Schirmstelle.
V3 unterMaus(const Sicht& s, const ModellAnsicht& a, ImVec2 m) {
    V3 f;
    V3 r;
    V3 u;
    achsenVon(s, f, r, u);
    const float jePunkt = 2.0F * s.halb / std::max(a.groesse[1], 1.0F);
    const float x = (m.x - (a.lo[0] + a.groesse[0] * 0.5F)) * jePunkt;
    const float y = ((a.lo[1] + a.groesse[1] * 0.5F) - m.y) * jePunkt;
    return v3(s.p) + r * x + u * y;
}

// Der Punkt der Figur unter einer Schirmstelle: der Sehstrahl gegen die Ebene des vordersten Dreiecks.
bool flaechenPunkt(const Sicht& s, const ModellAnsicht& a, ImVec2 m, V3& out) {
    int teil = -1;
    int dreieck = -1;
    if (!modellDreieckUnter(a, m, teil, dreieck) || teil < 0 || dreieck < 0) { return false; }
    const auto& haut = modellHaut();
    const auto ti = static_cast<std::size_t>(teil);
    if (ti >= haut.size() || ti >= mt().anzeige.surfaces.size()) { return false; }
    const std::vector<std::uint32_t>& idx = mt().anzeige.surfaces[ti].indexes;
    const std::size_t k = static_cast<std::size_t>(dreieck) * 3U;
    if (k + 2U >= idx.size() || idx[k] >= haut[ti].size() || idx[k + 1U] >= haut[ti].size() || idx[k + 2U] >= haut[ti].size()) { return false; }
    const V3 pa = v3(haut[ti][idx[k]].data());
    const V3 pb = v3(haut[ti][idx[k + 1U]].data());
    const V3 pc = v3(haut[ti][idx[k + 2U]].data());
    const V3 normale = kreuz(pb - pa, pc - pa);
    V3 f;
    V3 r;
    V3 u;
    achsenVon(s, f, r, u);
    const V3 ziel = unterMaus(s, a, m);
    V3 start = ziel - f * 4096.0F;
    V3 richtung = f;
    if (s.art == AnsichtArt::Perspektive) {
        start = v3(s.p) - f * (s.halb / tanHalb());
        richtung = norm(ziel - start);
    }
    const float nenner = dot(normale, richtung);
    if (std::fabs(nenner) < 1e-12F) { return false; }
    out = start + richtung * (dot(normale, pa - start) / nenner);
    return true;
}

// Worum gekreist wird. 3ds Max: das Orbit-Flyout (Orbit = Ansichtsmitte, Orbit Selected / Orbit SubObject = die Mitte der
// Auswahl - "If no objects are selected, the function reverts to the standard Orbit function" -, Orbit Point of Interest =
// der Punkt unter dem Mauszeiger). Blender: die Ansichtsmitte, mit "Orbit Around Selection" die Mitte der Auswahl.
V3 drehpunkt(const Sicht& s, const ModellAnsicht& a, ImVec2 maus) {
    const Settings& st = g_app->settings;
    float m[3];
    if (modellBlender()) { return st.modellBlOrbitAuswahl && modellAuswahlMitte(m) ? v3(m) : v3(s.p); }
    if ((st.modellOrbitArt == 1 || st.modellOrbitArt == 2) && modellAuswahlMitte(m)) { return v3(m); }
    if (st.modellOrbitArt == 3) {
        V3 q;
        return flaechenPunkt(s, a, maus, q) ? q : unterMaus(s, a, maus);
    }
    return v3(s.p);
}

// Blender viewrotate_apply (Turntable): die Achse fuer die senkrechte Mausbewegung - die waagerechte Achse quer zum Blick,
// zur Ansichts-X-Achse hin ueberblendet, je steiler man schaut (bei Rollwinkel 0 sind beide dieselbe).
V3 turntableX(V3 f, V3 r) {
    const V3 hoch{0.0F, 0.0F, 1.0F};
    const V3 hinten = f * -1.0F;   // m_inv[2]: die Z-Achse der Ansicht zeigt zum Betrachter
    const V3 d = hoch - hinten;
    if (dot(d, d) <= 0.001F) { return r; }
    V3 x = kreuz(hoch, hinten);
    if (dot(x, r) < 0.0F) { x = x * -1.0F; }
    float fac = std::acos(std::clamp(dot(hoch, hinten), -1.0F, 1.0F)) / kPi;
    fac = std::fabs(fac - 0.5F) * 2.0F;
    fac = fac * fac;
    return norm(x + (r - x) * fac);
}

// Kreisen um Bildpunkte: die Szene folgt der Maus (Trackball/Turntable) - nach rechts ziehen dreht die Kamera nach links um
// die Hochachse, nach unten ziehen hebt sie. `gradJePunkt` 0,4 (Blender: / UI_SCALE_FAC), `reverse` -1 bei Kopfstand
// (Blender ViewOpsData::reverse). 3ds Max bleibt wie bis rc611 vor dem Pol stehen (89 Grad in der Perspektive, 90 in der
// freien Orthoansicht - die Hilfe nennt dazu nichts); Blender dreht ueber den Pol.
void kreisenPx(Sicht& s, float dx, float dy, V3 um, float gradJePunkt, float reverse, bool ueberPol) {
    V3 f;
    V3 r;
    V3 u;
    basis(s.w, f, r, u);
    float nick = gradJePunkt * dy;
    if (!ueberPol && std::fabs(s.w[2]) < 0.5F) {
        const float grenze = s.art == AnsichtArt::Perspektive ? 89.0F : 90.0F;
        nick = std::clamp(s.w[0] + nick, -grenze, grenze) - s.w[0];
    }
    if (nick != 0.0F) { drehen(s, turntableX(f, r), -nick, um); }
    if (dx != 0.0F) { drehen(s, V3{0.0F, 0.0F, 1.0F}, -gradJePunkt * reverse * dx, um); }
}

float blenderGradJePunkt() { return kDrehProPx / std::max(g_app->settings.uiScale, 0.1F); }

// Blender viewrotate_apply_snap (Alt beim Kreisen): liegt die Richtung zum Betrachter naeher als 15 Grad an einer der 26
// Richtungen aus {-1, 0, 1}, rastet die Ansicht auf sie - aufrecht, senkrecht auf die naechste 45-Grad-Stellung, wenn die
// Ansicht um weniger als 15 Grad davon abweicht.
bool einrasten(Sicht& s) {
    V3 f;
    V3 r;
    V3 u;
    basis(s.w, f, r, u);
    const V3 hinten = f * -1.0F;
    const float grenze = std::cos(15.0F / kGrad);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            for (int z = -1; z <= 1; ++z) {
                if (x == 0 && y == 0 && z == 0) { continue; }
                const V3 ziel = norm(V3{static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)});
                if (dot(ziel, hinten) <= grenze) { continue; }
                const V3 mitte = v3(s.p);
                // die kleinste Drehung, die den Blick auf das Ziel legt (viewquat_align)
                const V3 achse = kreuz(hinten, ziel);
                if (laenge(achse) > 1e-6F) { drehen(s, norm(achse), std::asin(std::clamp(laenge(achse), -1.0F, 1.0F)) * kGrad, mitte); }
                float aufrecht[3] = {std::asin(std::clamp(ziel.z, -1.0F, 1.0F)) * kGrad, std::atan2(-ziel.y, -ziel.x) * kGrad, 0.0F};
                if (x == 0 && y == 0) {
                    // senkrecht: die Stellungen im 45-Grad-Raster (find best roll, j * 45)
                    aufrecht[0] = z > 0 ? 90.0F : -90.0F;
                    aufrecht[1] = std::round(s.w[1] / 45.0F) * 45.0F;
                    if (std::fabs(std::remainder(s.w[1] - aufrecht[1], 360.0F)) <= 15.0F && std::fabs(s.w[2]) < 1e-3F) { s.w[1] = aufrecht[1]; }
                    s.w[0] = aufrecht[0];
                    return true;
                }
                if (std::fabs(std::remainder(s.w[2], 360.0F)) <= 15.0F) {
                    for (int k = 0; k < 3; ++k) { s.w[k] = aufrecht[k]; }
                }
                return true;
            }
        }
    }
    return false;
}

// --- Weiche Wechsel (Blender Smooth View, smooth_viewtx 200 ms) ---------------------------------------------------------

struct Flug {
    bool aktiv = false;
    bool drehung = false;   // um eine Achse (view_orbit, view_roll) statt geradlinig (view_all, view_selected, center_pick)
    double start = 0.0;
    float dauer = 0.2F;
    Sicht von;
    Sicht zu;
    V3 achse;
    float grad = 0.0F;
    V3 um;
};
std::array<Flug, 4> g_flug;

void flugEnde(int n) {
    Flug& fl = g_flug[static_cast<std::size_t>(n)];
    if (!fl.aktiv) { return; }
    fl.aktiv = false;
    sichtSetzen(n, fl.zu);
}

// Blender ED_view3d_smooth_view_force_finish: ein neuer Befehl beendet den laufenden Wechsel sofort an seinem Ziel.
void wechselBeenden(int n) {
    modellAnsichtFertig(n);
    flugEnde(n);
}

void flugSchritt(int n) {
    Flug& fl = g_flug[static_cast<std::size_t>(n)];
    if (!fl.aktiv) { return; }
    float s = static_cast<float>((ImGui::GetTime() - fl.start) / static_cast<double>(std::max(fl.dauer, 1e-3F)));
    if (s >= 1.0F) {
        flugEnde(n);
        return;
    }
    s = std::clamp(s, 0.0F, 1.0F);
    s = 3.0F * s * s - 2.0F * s * s * s;   // view3d_smoothview_apply
    Sicht x = fl.von;
    if (fl.drehung) {
        drehen(x, fl.achse, fl.grad * s, fl.um);
    } else {
        for (int k = 0; k < 3; ++k) { x.p[k] = fl.von.p[k] + (fl.zu.p[k] - fl.von.p[k]) * s; }
        x.halb = fl.von.halb > 0.0F && fl.zu.halb > 0.0F ? fl.von.halb * std::pow(fl.zu.halb / fl.von.halb, s) : fl.zu.halb;
    }
    sichtSetzen(n, x);
}

void flugZu(int n, const Sicht& zu) {
    Flug& fl = g_flug[static_cast<std::size_t>(n)];
    fl.von = sichtVon(n);
    fl.zu = zu;
    fl.drehung = false;
    fl.dauer = modellBlickDauer();
    fl.start = ImGui::GetTime();
    fl.aktiv = true;
    if (fl.dauer <= 0.0F || gleich(fl.von, zu)) { flugEnde(n); }
    g_app->modelDirty = true;
}

void flugDrehen(int n, const Sicht& von, V3 achse, float grad, V3 um) {
    Flug& fl = g_flug[static_cast<std::size_t>(n)];
    fl.von = von;
    fl.zu = von;
    drehen(fl.zu, achse, grad, um);
    fl.achse = achse;
    fl.grad = grad;
    fl.um = um;
    fl.drehung = true;
    fl.dauer = modellBlickDauer();
    fl.start = ImGui::GetTime();
    fl.aktiv = true;
    sichtSetzen(n, von);
    if (fl.dauer <= 0.0F) { flugEnde(n); }
}

// --- Ansichtsaenderungen rueckgaengig (3ds Max: Undo / Redo Viewport Operation, Umschalt+Z / Umschalt+Y) ----------------
//
// Je Ansicht ein eigener Verlauf, unabhaengig von Strg+Z. Statt jede Stelle zu melden, die eine Ansicht aendert (Maus,
// Tasten, ViewCube, Menues), vergleicht der Waechter je Bild: beginnt eine Aenderung, kommt der Stand davor auf den Stapel;
// ein Zug, ein weicher Wechsel oder Rasten in aufeinanderfolgenden Bildern sind ein Schritt.

struct Verlauf {
    std::vector<Sicht> zurueck;
    std::vector<Sicht> vor;
    Sicht letzte;
    bool da = false;
    bool laeuft = false;
    std::string modell;
};
std::array<Verlauf, 4> g_verlauf;
constexpr std::size_t kVerlaufLaenge = 64;

void verlaufBild(int n) {
    Verlauf& v = g_verlauf[static_cast<std::size_t>(n)];
    const Sicht jetzt = sichtVon(n);
    if (!v.da || v.modell != g_app->modelPath) {
        v = Verlauf{};
        v.letzte = jetzt;
        v.da = true;
        v.modell = g_app->modelPath;
        return;
    }
    if (!gleich(jetzt, v.letzte)) {
        if (!v.laeuft) {
            v.zurueck.push_back(v.letzte);
            if (v.zurueck.size() > kVerlaufLaenge) { v.zurueck.erase(v.zurueck.begin()); }
            v.vor.clear();
            v.laeuft = true;
        }
        v.letzte = jetzt;
        return;
    }
    const ImGuiIO& io = ImGui::GetIO();
    const bool ruhig = !io.MouseDown[0] && !io.MouseDown[1] && !io.MouseDown[2] && mt().ansichten[n].animStart < 0.0 && !g_flug[static_cast<std::size_t>(n)].aktiv;
    if (ruhig) { v.laeuft = false; }
}

// --- Maus -------------------------------------------------------------------------------------------------------------

enum class ZugArt : int { Keiner = 0, Schieben, Kreisen, Zoom, Rollen, Bereich, BlAlt };

struct Zug {
    ZugArt art = ZugArt::Keiner;
    int ansicht = -1;
    int knopf = ImGuiMouseButton_Middle;
    ImVec2 start{};
    ImVec2 letzte{};
    Sicht anfang;
    Sicht roh;                // Kreisen: der Stand ohne Einrasten (Blender vod->curr.viewquat)
    V3 um;                    // Drehpunkt bzw. Punkt, um den gezoomt wird
    bool umDa = false;
    bool beschraenkt = false; // Umschalt: nur die zuerst bewegte Achse
    int achse = 0;            // 0 offen, 1 waagerecht, 2 senkrecht
    float reverse = 1.0F;
    bool bewegt = false;
    bool erledigt = false;    // Blender Alt+Mitte: die Achsenansicht ist ausgeloest
    bool perspVorher = false; // Blender: die Projektion vor dem Einrasten (persp_with_auto_persp_applied)
};
Zug g_zug;
bool g_ansichtenMenue = false;   // V
bool g_kuchen = false;           // `
ImVec2 g_kuchenMitte{};
bool g_blBereich = false;        // Blender Umschalt+B: ein Bereich, dann ist der Modus zu Ende

float hoeheVon(const ModellAnsicht& a) { return std::max(a.groesse[1], 1.0F); }
ImVec2 mitteVon(const ModellAnsicht& a) { return ImVec2{a.lo[0] + a.groesse[0] * 0.5F, a.lo[1] + a.groesse[1] * 0.5F}; }

// Der gelbe Kreis des Orbit-Modus (3ds Max: Trackball mit Griffen an den vier Viertelpunkten). 3ds Max nennt seine Groesse
// nicht: hier 0,38 der kleineren Seite - innen bleibt Platz fuer das freie Kreisen, aussen fuer das Rollen.
float orbitRadius(const ModellAnsicht& a) { return 0.38F * std::min(a.groesse[0], a.groesse[1]); }
float orbitGriff() { return std::max(ImGui::GetFontSize() * 0.4F, 5.0F); }

// Zoom-Faktor je Raste (3ds Max: keine Zahl dokumentiert - die Werte des Model-Tabs bis rc611; Blender 1,2).
float rasteFaktor(const Sicht& s, float rasten) {
    if (modellBlender()) { return std::pow(1.0F / kBlZoom, rasten); }
    return std::pow(s.art == AnsichtArt::Perspektive ? 0.9F : 0.85F, rasten);
}

bool zoomZurMaus(const Sicht& s) {
    const Settings& st = g_app->settings;
    if (modellBlender()) { return st.modellBlZoomMaus; }
    return s.art == AnsichtArt::Perspektive ? st.modellZoomMausPersp : st.modellZoomMausOrtho;
}

void zoomSchritt(int n, float rasten, const ImVec2* maus) {
    wechselBeenden(n);
    const ModellAnsicht& a = mt().ansichten[n];
    Sicht s = sichtVon(n);
    if (maus != nullptr && zoomZurMaus(s)) {
        const V3 um = unterMaus(s, a, *maus);
        zoomen(s, rasteFaktor(s, rasten), &um);
    } else {
        zoomen(s, rasteFaktor(s, rasten), nullptr);
    }
    sichtSetzen(n, s);
}

bool mausIn(const ModellAnsicht& a, ImVec2 m) {
    return a.sichtbar && m.x >= a.lo[0] && m.x < a.lo[0] + a.groesse[0] && m.y >= a.lo[1] && m.y < a.lo[1] + a.groesse[1];
}

// Blender view3d.zoom, Dolly (viewzoom_scale_value): der Abstand ist der vom Anfang mal 2 * (len_neu / len_alt - 1) + 1, len
// der Abstand der Maus zur Oberkante der Ansicht (5 * U + Bildpunkte / U, U = UI_SCALE_FAC).
float dollyFaktor(const ModellAnsicht& a, float yAlt, float yNeu) {
    const float u = std::max(g_app->settings.uiScale, 0.1F);
    const float alt = 5.0F * u + (yAlt - a.lo[1]) / u;
    const float neu = 5.0F * u + (yNeu - a.lo[1]) / u;
    return 2.0F * (neu / std::max(alt, 1.0F) - 1.0F) + 1.0F;
}

void dollyAnwenden(int n, const Sicht& anfang, float faktor, const V3* um) {
    Sicht s = anfang;
    float lo = 0.0F;
    float hi = 0.0F;
    halbGrenzen(s, lo, hi);
    zoomen(s, std::max(faktor, lo / std::max(s.halb, 1e-9F)), um);
    sichtSetzen(n, s);
}

// Blender view3d.view_axis mit "relative" (Alt+Mitteltaste ziehen): die Ansicht um 90 Grad in die Zugrichtung weiter, auf
// die naechste Achsenansicht; im Turntable stehen die Seitenansichten aufrecht, Oben/Unten behalten ihre Drehung.
void blRelativ(int n, int richtung) {   // 0 Ost, 1 Nord, 2 West, 3 Sued
    wechselBeenden(n);
    const Sicht s = sichtVon(n);
    V3 f;
    V3 r;
    V3 u;
    achsenVon(s, f, r, u);
    // RIGHT: die Welt +90 Grad um die Hochachse der Ansicht = die Kamera -90; LEFT +90; TOP: die Kamera +90 um die Rechtsachse
    const V3 achse = (richtung == 0 || richtung == 2) ? u : r;
    const float grad = (richtung == 0 || richtung == 3) ? -90.0F : 90.0F;
    const V3 f2 = dreh(f, achse, grad);
    const V3 u2 = dreh(u, achse, grad);
    static const AnsichtArt kArten[6] = {AnsichtArt::Oben, AnsichtArt::Unten, AnsichtArt::Vorne, AnsichtArt::Hinten, AnsichtArt::Links, AnsichtArt::Rechts};
    AnsichtArt best = AnsichtArt::Vorne;
    float bd = -2.0F;
    for (const AnsichtArt art : kArten) {
        float rr[3];
        float hh[3];
        float vv[3];
        modellAnsichtAchsenVon(art, rr, hh, vv);
        const float d = dot(f2, v3(vv));
        if (d > bd) {
            bd = d;
            best = art;
        }
    }
    if (best == AnsichtArt::Oben || best == AnsichtArt::Unten) {
        // senkrecht: die Drehung um die Blickachse im 90-Grad-Raster (RV3D_VIEW_AXIS_ROLL_0 .. 270)
        const bool oben = best == AnsichtArt::Oben;
        float w[3] = {oben ? 90.0F : -90.0F, std::round(std::atan2(oben ? u2.y : -u2.y, oben ? u2.x : -u2.x) * kGrad / 90.0F) * 90.0F, 0.0F};
        Sicht z = s;
        z.art = AnsichtArt::Orthographisch;
        for (int k = 0; k < 3; ++k) { z.w[k] = w[k]; }
        benennen(z);
        modellBlickSetzen(n, z.art, w, true);
        return;
    }
    modellBlickSetzen(n, best, nullptr, true);
}

// Blender WM_event_drag_direction: acht Richtungen aus dem Winkel des Zugs (y nach oben).
int zugRichtung(float dx, float dyHoch) {
    const int theta = static_cast<int>(std::lround(4.0F * std::atan2(dyHoch, dx) / kPi));
    switch (theta) {
        case 0: return 0;    // Ost
        case 2: return 1;    // Nord
        case -2: return 3;   // Sued
        case 4:
        case -4: return 2;   // West
        default: break;
    }
    return -1;   // schraeg: in der Belegung "Relative" ohne Befehl
}

void zugBeginn(int n, const ModellAnsicht& a, ZugArt art, int knopf) {
    const ImGuiIO& io = ImGui::GetIO();
    wechselBeenden(n);
    g_zug = Zug{};
    g_zug.art = art;
    g_zug.ansicht = n;
    g_zug.knopf = knopf;
    g_zug.start = io.MousePos;
    g_zug.letzte = io.MousePos;
    g_zug.anfang = sichtVon(n);
    g_zug.roh = g_zug.anfang;
    g_zug.beschraenkt = !modellBlender() && io.KeyShift && (art == ZugArt::Schieben || art == ZugArt::Kreisen);
    if (art == ZugArt::Kreisen) {
        g_zug.um = drehpunkt(g_zug.anfang, a, io.MousePos);
        g_zug.perspVorher = g_zug.anfang.art == AnsichtArt::Perspektive;
        if (modellBlender() && achsenArt(g_zug.anfang.art)) {
            // ED_view3d_persp_ensure: schon beim Druecken wird aus der Achsenansicht die Perspektive
            freiMachen(g_zug.anfang, true);
            sichtSetzen(n, g_zug.anfang);
            g_zug.perspVorher = true;
        }
        g_zug.roh = g_zug.anfang;
        V3 f;
        V3 r;
        V3 u;
        achsenVon(g_zug.anfang, f, r, u);
        g_zug.reverse = u.z < 0.0F ? -1.0F : 1.0F;
    } else if (art == ZugArt::Zoom && zoomZurMaus(g_zug.anfang)) {
        g_zug.um = unterMaus(g_zug.anfang, a, io.MousePos);
        g_zug.umDa = true;
    }
}

void zugFuehren(int n, const ModellAnsicht& a) {
    const ImGuiIO& io = ImGui::GetIO();
    const bool blender = modellBlender();
    ImVec2 gesamt{io.MousePos.x - g_zug.start.x, io.MousePos.y - g_zug.start.y};
    ImVec2 schritt{io.MousePos.x - g_zug.letzte.x, io.MousePos.y - g_zug.letzte.y};
    g_zug.letzte = io.MousePos;
    if (std::fabs(gesamt.x) + std::fabs(gesamt.y) > kMaxZug) { g_zug.bewegt = true; }
    if (g_zug.beschraenkt) {
        // 3ds Max: "Holding Shift constrains ... to the first axis you use"
        if (g_zug.achse == 0 && g_zug.bewegt) { g_zug.achse = std::fabs(gesamt.x) >= std::fabs(gesamt.y) ? 1 : 2; }
        if (g_zug.achse == 0) { return; }
        (g_zug.achse == 1 ? gesamt.y : gesamt.x) = 0.0F;
        (g_zug.achse == 1 ? schritt.y : schritt.x) = 0.0F;
    }
    switch (g_zug.art) {
        case ZugArt::Schieben: {
            Sicht s = g_zug.anfang;
            schieben(s, gesamt.x, gesamt.y, hoeheVon(a));
            sichtSetzen(n, s);
            break;
        }
        case ZugArt::Kreisen: {
            if (g_zug.beschraenkt) {
                // ein Zug vor der Entscheidung zaehlt mit: vom Anfang aus die ganze Strecke entlang der einen Achse
                g_zug.roh = g_zug.anfang;
                freiMachen(g_zug.roh, false);
                schritt = gesamt;
            } else if (schritt.x == 0.0F && schritt.y == 0.0F && !blender) {
                break;
            }
            freiMachen(g_zug.roh, blender);
            kreisenPx(g_zug.roh, schritt.x, schritt.y, g_zug.um, blender ? blenderGradJePunkt() : kDrehProPx, g_zug.reverse, blender);
            Sicht s = g_zug.roh;
            if (blender && io.KeyAlt) {
                // Alt beim Kreisen (VIEWROT_MODAL_AXIS_SNAP_ENABLE); mit Auto Perspective zeigt eine getroffene Achsenansicht
                // orthographisch, sonst die Projektion vom Anfang
                if (einrasten(s)) {
                    Sicht probe = s;
                    probe.art = AnsichtArt::Orthographisch;
                    benennen(probe);
                    if (achsenArt(probe.art)) { s = probe; }
                } else {
                    s.art = g_zug.perspVorher ? AnsichtArt::Perspektive : s.art;
                }
            }
            sichtSetzen(n, s);
            break;
        }
        case ZugArt::Zoom: {
            if (blender) {
                dollyAnwenden(n, g_zug.anfang, dollyFaktor(a, g_zug.start.y, io.MousePos.y), g_zug.umDa ? &g_zug.um : nullptr);
            } else {
                // wie bis rc611: 0,9 hoch (0,05 je Bildpunkt) in der Perspektive, 1,01 je Bildpunkt in den Orthoansichten
                Sicht s = g_zug.anfang;
                const float faktor = s.art == AnsichtArt::Perspektive ? std::pow(0.9F, -gesamt.y * 0.05F) : std::pow(1.01F, gesamt.y);
                zoomen(s, faktor, g_zug.umDa ? &g_zug.um : nullptr);
                sichtSetzen(n, s);
            }
            break;
        }
        case ZugArt::Rollen: {
            // ausserhalb des Kreises: die Ansicht dreht um die Blickachse, die Szene folgt der Maus um die Bildmitte
            const ImVec2 m = mitteVon(a);
            const float vorher = std::atan2(g_zug.start.y - m.y, g_zug.start.x - m.x);
            const float jetzt = std::atan2(io.MousePos.y - m.y, io.MousePos.x - m.x);
            Sicht s = g_zug.anfang;
            freiMachen(s, false);
            V3 f;
            V3 r;
            V3 u;
            basis(s.w, f, r, u);
            // die Szene im Uhrzeigersinn = die Kamera andersherum um ihre Blickachse (Roll nimmt ab)
            drehen(s, f, -std::remainder(jetzt - vorher, 2.0F * kPi) * kGrad, v3(s.p));
            sichtSetzen(n, s);
            break;
        }
        case ZugArt::BlAlt: {
            // CLICK_DRAG: sobald die Maus die Schwelle ueberschreitet, einmal - die Richtung des Zugs bis dahin
            const float schwelle = kMaxZug * std::max(g_app->settings.uiScale, 0.1F);
            if (!g_zug.erledigt && std::hypot(gesamt.x, gesamt.y) > schwelle) {
                g_zug.erledigt = true;
                const int richtung = zugRichtung(gesamt.x, -gesamt.y);
                if (richtung >= 0) { blRelativ(n, richtung); }
            }
            break;
        }
        case ZugArt::Bereich:
        case ZugArt::Keiner: break;
    }
}

void zugEnde(int n, const ModellAnsicht& a) {
    const ImGuiIO& io = ImGui::GetIO();
    if (g_zug.art == ZugArt::Bereich) {
        // Zoom Region: das aufgezogene Rechteck fuellt danach die Ansicht
        const float b = std::fabs(io.MousePos.x - g_zug.start.x);
        const float h = std::fabs(io.MousePos.y - g_zug.start.y);
        if (b > kMaxZug && h > kMaxZug) {
            Sicht s = g_zug.anfang;
            const V3 mitte = unterMaus(s, a, ImVec2{(io.MousePos.x + g_zug.start.x) * 0.5F, (io.MousePos.y + g_zug.start.y) * 0.5F});
            nach(mitte, s.p);
            zoomen(s, std::max(b / std::max(a.groesse[0], 1.0F), h / hoeheVon(a)), nullptr);
            sichtSetzen(n, s);
        }
        if (g_blBereich) {
            g_blBereich = false;
            mt().navModus = 0;
        }
    } else if (g_zug.art == ZugArt::BlAlt && !g_zug.erledigt) {
        // Alt+Mitteltaste geklickt: view3d.view_center_pick - der Punkt der Figur unter der Maus (sonst der Punkt in der
        // Tiefe des Drehpunkts) kommt in die Bildmitte
        const Sicht s = sichtVon(n);
        V3 q;
        if (!flaechenPunkt(s, a, io.MousePos, q)) { q = unterMaus(s, a, io.MousePos); }
        Sicht z = s;
        nach(q, z.p);
        flugZu(n, z);
    }
    g_zug = Zug{};
}

// Der Zug des Orbit-Modus beginnt im Kreis (frei), auf einem Griff (eine Achse) oder ausserhalb (rollen).
void orbitModusBeginn(int n, const ModellAnsicht& a) {
    const ImGuiIO& io = ImGui::GetIO();
    const ImVec2 m = mitteVon(a);
    const float radius = orbitRadius(a);
    const float griff = orbitGriff() * 1.5F;
    const ImVec2 griffe[4] = {ImVec2{m.x - radius, m.y}, ImVec2{m.x + radius, m.y}, ImVec2{m.x, m.y - radius}, ImVec2{m.x, m.y + radius}};
    for (int i = 0; i < 4; ++i) {
        if (std::fabs(io.MousePos.x - griffe[i].x) <= griff && std::fabs(io.MousePos.y - griffe[i].y) <= griff) {
            zugBeginn(n, a, ZugArt::Kreisen, ImGuiMouseButton_Left);
            g_zug.beschraenkt = true;
            g_zug.achse = i < 2 ? 1 : 2;   // die seitlichen Griffe waagerecht, der obere und untere senkrecht
            return;
        }
    }
    const bool innen = std::hypot(io.MousePos.x - m.x, io.MousePos.y - m.y) <= radius;
    zugBeginn(n, a, innen ? ZugArt::Kreisen : ZugArt::Rollen, ImGuiMouseButton_Left);
}

}  // namespace

// --- je Bild -----------------------------------------------------------------------------------------------------------

void modellNavVorBild(int n) {
    if (n < 0 || n > 3) { return; }
    flugSchritt(n);
    verlaufBild(n);
}

// Die Maus der Navigation in einer Ansicht. true: die linke und rechte Taste gehoeren in diesem Bild der Navigation (ein
// Modus laeuft) - die Ansicht waehlt, malt und oeffnet dann nichts.
bool modellNavMaus(int n, ModellAnsicht& a, bool ueber) {
    ModellTab& t = mt();
    const ImGuiIO& io = ImGui::GetIO();
    const bool blender = modellBlender();
    if (blender && t.navModus != 0 && t.navModus != 4) { t.navModus = 0; }   // die Modi Zoom/Pan/Orbit gibt es nur wie 3ds Max
    if (ueber && io.MouseWheel != 0.0F && g_zug.art == ZugArt::Keiner) { zoomSchritt(n, io.MouseWheel, &io.MousePos); }
    if (g_zug.art == ZugArt::Keiner && ueber) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) {
            if (blender) {
                // blender_default.py km_view3d: Mitte rotate, Umschalt move, Strg zoom, Alt view_axis (ziehen) /
                // view_center_pick (klicken); Umschalt+Strg waere view3d.dolly (nicht nachgebaut)
                if (io.KeyAlt && !io.KeyCtrl && !io.KeyShift) {
                    zugBeginn(n, a, ZugArt::BlAlt, ImGuiMouseButton_Middle);
                } else if (io.KeyCtrl && !io.KeyShift && !io.KeyAlt) {
                    zugBeginn(n, a, ZugArt::Zoom, ImGuiMouseButton_Middle);
                } else if (io.KeyShift && !io.KeyCtrl && !io.KeyAlt) {
                    zugBeginn(n, a, ZugArt::Schieben, ImGuiMouseButton_Middle);
                } else if (!io.KeyShift && !io.KeyCtrl && !io.KeyAlt) {
                    zugBeginn(n, a, ZugArt::Kreisen, ImGuiMouseButton_Middle);
                }
            } else if (io.KeyAlt && io.KeyCtrl) {
                zugBeginn(n, a, ZugArt::Zoom, ImGuiMouseButton_Middle);
            } else if (io.KeyAlt) {
                zugBeginn(n, a, ZugArt::Kreisen, ImGuiMouseButton_Middle);
            } else {
                zugBeginn(n, a, ZugArt::Schieben, ImGuiMouseButton_Middle);
            }
        } else if (t.navModus != 0 && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            t.aktiv = n;
            switch (t.navModus) {
                case 1: zugBeginn(n, a, ZugArt::Zoom, ImGuiMouseButton_Left); break;
                case 2: zugBeginn(n, a, ZugArt::Schieben, ImGuiMouseButton_Left); break;
                case 3: orbitModusBeginn(n, a); break;
                default: zugBeginn(n, a, ZugArt::Bereich, ImGuiMouseButton_Left); break;
            }
        }
    }
    bool verbraucht = t.navModus != 0;
    if (g_zug.art != ZugArt::Keiner && g_zug.ansicht == n) {
        verbraucht = verbraucht || g_zug.knopf == ImGuiMouseButton_Left;
        if (!ImGui::IsMouseDown(g_zug.knopf)) {
            zugEnde(n, a);
        } else if (io.MouseDelta.x != 0.0F || io.MouseDelta.y != 0.0F || (blender && g_zug.art == ZugArt::Kreisen && io.KeyAlt)) {
            zugFuehren(n, a);
        }
    }
    // ein Modus endet mit Rechtsklick (wie die Knoepfe der Ansichtssteuerung von 3ds Max) - das Menue der Ansicht bleibt zu
    if (t.navModus != 0 && ueber && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
        t.navModus = 0;
        g_blBereich = false;
    }
    if (t.navModus != 0 && ueber) {
        ImGui::SetMouseCursor(t.navModus == 1 ? ImGuiMouseCursor_ResizeNS : (t.navModus == 2 ? ImGuiMouseCursor_ResizeAll : (t.navModus == 3 ? ImGuiMouseCursor_Hand : ImGuiMouseCursor_Arrow)));
    }
    return verbraucht;
}

// Was ein Modus in der Ansicht zeigt: sein Name mit dem Hinweis auf das Ende, der Kreis des Orbit-Modus, der Rahmen des
// Bereichs.
void modellNavUeberlagerung(int n, const ModellAnsicht& a, ImDrawList* dl) {
    const ModellTab& t = mt();
    if (t.navModus == 0 || n != t.aktiv) { return; }
    static const Str kName[5] = {Str::MnModusZoom, Str::MnModusZoom, Str::MnModusPan, Str::MnModusOrbit, Str::MnModusBereich};
    const std::string zeile = std::string(tr(modellBlender() ? Str::MnBlBereich : kName[std::clamp(t.navModus, 0, 4)])) + " - " + tr(Str::MnModusEnde);
    const ImVec2 g = ImGui::CalcTextSize(zeile.c_str());
    const ImVec2 o{a.lo[0] + (a.groesse[0] - g.x) * 0.5F, a.lo[1] + a.groesse[1] - g.y - 8.0F};
    dl->AddRectFilled(ImVec2{o.x - 6.0F, o.y - 3.0F}, ImVec2{o.x + g.x + 6.0F, o.y + g.y + 3.0F}, IM_COL32(0, 0, 0, 150), 4.0F);
    dl->AddText(o, IM_COL32(255, 230, 60, 255), zeile.c_str());
    if (t.navModus == 3) {
        const ImVec2 m = mitteVon(a);
        const float radius = orbitRadius(a);
        const float griff = orbitGriff();
        const ImU32 gelb = IM_COL32(255, 220, 40, 255);
        dl->AddCircle(m, radius, gelb, 96, 1.5F);
        const ImVec2 griffe[4] = {ImVec2{m.x - radius, m.y}, ImVec2{m.x + radius, m.y}, ImVec2{m.x, m.y - radius}, ImVec2{m.x, m.y + radius}};
        for (const ImVec2& p : griffe) { dl->AddRect(ImVec2{p.x - griff, p.y - griff}, ImVec2{p.x + griff, p.y + griff}, gelb, 0.0F, 0, 1.5F); }
    }
    if (g_zug.art == ZugArt::Bereich && g_zug.ansicht == n) { dl->AddRect(g_zug.start, ImGui::GetIO().MousePos, IM_COL32(255, 255, 255, 220), 0.0F, 0, 1.0F); }
}

// --- Befehle (Tasten, Menues, Suche) -----------------------------------------------------------------------------------

void modellNavModusSetzen(int m) {
    ModellTab& t = mt();
    t.navModus = t.navModus == m ? 0 : std::clamp(m, 0, 4);
    g_blBereich = false;
    g_zug = Zug{};
    g_app->modelDirty = true;
}

void modellNavBlBereich() {
    mt().navModus = 4;
    g_blBereich = true;
    g_zug = Zug{};
}

// Esc beendet einen Modus (true: die Taste ist verbraucht).
bool modellNavTasten() {
    ModellTab& t = mt();
    if (t.navModus != 0 && ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
        t.navModus = 0;
        g_blBereich = false;
        g_zug = Zug{};
        return true;
    }
    return false;
}

// [ ] bzw. Ziffernblock + / -: wie eine Raste des Rads, um den Mauspunkt, wenn die Maus in der aktiven Ansicht steht.
void modellNavZoomSchritt(float rasten) {
    const ModellTab& t = mt();
    const ImVec2 maus = ImGui::GetIO().MousePos;
    zoomSchritt(t.aktiv, rasten, mausIn(t.ansichten[t.aktiv], maus) ? &maus : nullptr);
}

// 3ds Max Zoom In 2X / Zoom Out 2X.
void modellNavZoom2x(bool heran) {
    const int n = mt().aktiv;
    wechselBeenden(n);
    Sicht s = sichtVon(n);
    zoomen(s, heran ? 0.5F : 2.0F, nullptr);
    sichtSetzen(n, s);
}

// 3ds Max: 0 Zoom Extents All Selected (Z: alle Ansichten, die Auswahl, ohne Auswahl alles), 1 Zoom Extents (Strg+Alt+Z: die
// aktive Ansicht, alles), 2 Zoom Extents All (alle Ansichten, alles).
void modellNavGrenzen(int art) {
    const int aktiv = mt().aktiv;
    for (int i = 0; i < 4; ++i) {
        if (art == 1 && i != aktiv) { continue; }
        wechselBeenden(i);
        if (art == 0) {
            modellAnsichtEinpassenIn(i);
        } else {
            modellAllesEinpassenIn(i);
        }
    }
    g_app->modelDirty = true;
}

// 3ds Max Pan Viewport (I): der Punkt unter dem Mauszeiger kommt in die Bildmitte.
void modellNavZumZeiger() {
    const ModellTab& t = mt();
    const ModellAnsicht& a = t.ansichten[t.aktiv];
    const ImVec2 maus = ImGui::GetIO().MousePos;
    if (!mausIn(a, maus)) { return; }
    wechselBeenden(t.aktiv);
    Sicht s = sichtVon(t.aktiv);
    nach(unterMaus(s, a, maus), s.p);
    sichtSetzen(t.aktiv, s);
}

bool modellNavRueck(bool wieder) {
    const int n = mt().aktiv;
    Verlauf& v = g_verlauf[static_cast<std::size_t>(n)];
    std::vector<Sicht>& von = wieder ? v.vor : v.zurueck;
    std::vector<Sicht>& ziel = wieder ? v.zurueck : v.vor;
    if (von.empty()) {
        mt().meldung = tr(Str::MnVerlaufLeer);   // klare Meldung statt stillem Nichtstun
        return false;
    }
    wechselBeenden(n);
    ziel.push_back(sichtVon(n));
    const Sicht s = von.back();
    von.pop_back();
    sichtSetzen(n, s);
    v.letzte = sichtVon(n);
    v.laeuft = false;
    return true;
}

void modellNavAnsichtenMenue() { g_ansichtenMenue = true; }

void modellNavKuchen() {
    g_kuchen = true;
    g_kuchenMitte = ImGui::GetIO().MousePos;
}

// Kreisen um Bildpunkte fuer ViewCube und Gizmo: wie Alt+Mitteltaste bzw. die Mitteltaste von Blender.
void modellNavKreisenPx(int n, ImVec2 d, bool beginn) {
    if (n < 0 || n > 3) { return; }
    const bool blender = modellBlender();
    const ModellAnsicht& a = mt().ansichten[n];
    static float reverse = 1.0F;
    wechselBeenden(n);
    Sicht s = sichtVon(n);
    if (beginn) {
        V3 f;
        V3 r;
        V3 u;
        achsenVon(s, f, r, u);
        reverse = u.z < 0.0F ? -1.0F : 1.0F;
    }
    freiMachen(s, blender);
    kreisenPx(s, d.x, d.y, drehpunkt(s, a, mitteVon(a)), blender ? blenderGradJePunkt() : kDrehProPx, reverse, blender);
    sichtSetzen(n, s);
}

void modellNavSchiebenPx(int n, ImVec2 d) {
    if (n < 0 || n > 3) { return; }
    wechselBeenden(n);
    Sicht s = sichtVon(n);
    schieben(s, d.x, d.y, hoeheVon(mt().ansichten[n]));
    sichtSetzen(n, s);
}

// Der Zoom-Knopf des Gizmos: view3d.zoom mit use_cursor_init = false - als begaenne der Zug in der Bildmitte.
void modellNavBlZoomZug(int n, bool beginn, float dyGesamt) {
    if (n < 0 || n > 3) { return; }
    static Sicht anfang;
    if (beginn) {
        wechselBeenden(n);
        anfang = sichtVon(n);
        return;
    }
    const ModellAnsicht& a = mt().ansichten[n];
    const float mitteY = a.lo[1] + a.groesse[1] * 0.5F;
    dollyAnwenden(n, anfang, dollyFaktor(a, mitteY, mitteY + dyGesamt), nullptr);
}

// Blender view3d.view_persportho (Ziffernblock 5, Knopf des Gizmos): Perspektive <-> orthographisch, die Richtung bleibt.
void modellNavBlPerspOrtho() {
    const int n = mt().aktiv;
    wechselBeenden(n);
    Sicht s = sichtVon(n);
    s.art = s.art == AnsichtArt::Perspektive ? AnsichtArt::Orthographisch : AnsichtArt::Perspektive;
    benennen(s);
    sichtSetzen(n, s);
}

// Blender view3d.view_orbit (Ziffernblock 4/6/8/2): 15 Grad (pad_rot_angle) um die Welt-Hochachse bzw. die Rechtsachse der
// Ansicht, weich; 0 links (die Kamera geht nach links), 1 rechts, 2 hoch (die Kamera steigt), 3 runter.
void modellNavBlKreisen(int richtung) {
    const ModellTab& t = mt();
    const int n = t.aktiv;
    wechselBeenden(n);
    Sicht s = sichtVon(n);
    freiMachen(s, true);   // ED_view3d_persp_ensure
    V3 f;
    V3 r;
    V3 u;
    basis(s.w, f, r, u);
    const V3 achse = richtung < 2 ? V3{0.0F, 0.0F, 1.0F} : r;
    const float grad = (richtung == 0 || richtung == 2) ? -15.0F : 15.0F;
    const ModellAnsicht& a = t.ansichten[n];
    flugDrehen(n, s, achse, grad, drehpunkt(s, a, mitteVon(a)));
}

// Blender view3d.view_roll (Umschalt+Ziffernblock 4/6): 15 Grad um die Blickachse; links dreht die Szene im Bild gegen den
// Uhrzeigersinn. Eine Achsenansicht bleibt orthographisch.
void modellNavBlRollen(bool links) {
    const int n = mt().aktiv;
    wechselBeenden(n);
    Sicht s = sichtVon(n);
    freiMachen(s, false);
    V3 f;
    V3 r;
    V3 u;
    basis(s.w, f, r, u);
    flugDrehen(n, s, f, links ? 15.0F : -15.0F, v3(s.p));
}

// Blender view3d.view_pan (Strg+Ziffernblock): 32 Bildpunkte waagerecht, 25 senkrecht; 0 links, 1 rechts, 2 hoch, 3 runter.
void modellNavBlSchieben(int richtung) {
    static const float kWeg[4][2] = {{32.0F, 0.0F}, {-32.0F, 0.0F}, {0.0F, 25.0F}, {0.0F, -25.0F}};
    modellNavSchiebenPx(mt().aktiv, ImVec2{kWeg[std::clamp(richtung, 0, 3)][0], kWeg[std::clamp(richtung, 0, 3)][1]});
}

// Blender view3d.view_axis (Ziffernblock 1/3/7, Kuchenmenue): weich auf die Achsenansicht, Mitte und Groesse bleiben.
void modellNavBlAchse(AnsichtArt art) {
    const int n = mt().aktiv;
    wechselBeenden(n);
    modellBlickSetzen(n, art, nullptr, true);
}

// Blender view3d.view_selected / view_all (view3d_from_minmax): die Mitte des Kastens in die Bildmitte, der Abstand so, dass
// die groesste Kastenseite / 2 * 1,4 die halbe kleinere Bildseite ist; die Richtung bleibt. Ohne Auswahl tut view_selected
// nichts; view_all mit "center" (Umschalt+C) nimmt den Ursprung in den Kasten.
bool modellNavBlEinpassen(bool nurAuswahl, bool mitUrsprung) {
    const ModellTab& t = mt();
    const int n = t.aktiv;
    float mn[3];
    float mx[3];
    if (!modellGrenzen(nurAuswahl, mn, mx)) {
        if (nurAuswahl) { return false; }
        for (int k = 0; k < 3; ++k) {
            mn[k] = g_app->model.mins[k];
            mx[k] = g_app->model.maxs[k];
        }
    }
    if (mitUrsprung) {
        for (int k = 0; k < 3; ++k) {
            mn[k] = std::min(mn[k], 0.0F);
            mx[k] = std::max(mx[k], 0.0F);
        }
    }
    wechselBeenden(n);
    const ModellAnsicht& a = t.ansichten[n];
    Sicht z = sichtVon(n);
    const float seite = std::max({mx[0] - mn[0], mx[1] - mn[1], mx[2] - mn[2]});
    for (int k = 0; k < 3; ++k) { z.p[k] = (mn[k] + mx[k]) * 0.5F; }
    if (z.art == AnsichtArt::Perspektive || seite >= 0.0001F) {
        const float radius = seite * 0.5F * kBlMarge;
        const float seiten = a.groesse[0] > 0.0F && a.groesse[1] > 0.0F ? a.groesse[0] / a.groesse[1] : 1.0F;
        z.halb = seiten >= 1.0F ? radius : radius / seiten;
    }
    flugZu(n, z);
    return true;
}

// Blender screen.region_quadview (Strg+Alt+Q): oben links Top, oben rechts die Perspektive, unten links Front, unten rechts
// Right - die drei Orthoansichten werden dabei gesetzt.
void modellNavBlVier() {
    ModellTab& t = mt();
    t.vierAnsichten = !t.vierAnsichten;
    if (t.vierAnsichten) {
        static const AnsichtArt kArt[3] = {AnsichtArt::Oben, AnsichtArt::Vorne, AnsichtArt::Rechts};
        for (int i = 0; i < 3; ++i) {
            if (t.ansichten[i].art == kArt[i]) { continue; }
            t.ansichten[i].animStart = -1.0;
            t.ansichten[i].art = kArt[i];
            modellAllesEinpassenIn(i);
        }
        if (t.ansichten[3].art != AnsichtArt::Perspektive) { modellAnsichtSetzenIn(3, AnsichtArt::Perspektive); }
    }
    g_app->modelDirty = true;
}

// Die Reihenfolge der vier Ansichten im Bild (links oben, rechts oben, links unten, rechts unten).
// TEST1411e: der Sehstrahl wie flaechenPunkt (in der Perspektive vom Ort der Kamera, sonst senkrecht zur Ansicht) gegen die Ebene des
// Bodenrasters der Ansicht - durch den Boden (an.bodenZ) bzw. den Ursprung, wie rasterFuer sie zeichnet.
int modellNavRasterPunkt(ImVec2 maus, float out[3]) {
    const ModellTab& t = mt();
    for (int n = 0; n < 4; ++n) {
        const ModellAnsicht& a = t.ansichten[n];
        if (!mausIn(a, maus)) { continue; }
        const Sicht s = sichtVon(n);
        V3 f;
        V3 r;
        V3 u;
        achsenVon(s, f, r, u);
        const V3 ziel = unterMaus(s, a, maus);
        const bool persp = s.art == AnsichtArt::Perspektive;
        const V3 start = persp ? v3(s.p) - f * (s.halb / tanHalb()) : ziel;
        const V3 richtung = persp ? norm(ziel - start) : f;
        const int k = modellRasterNormale(a.art);
        const float ebene = k == 2 ? t.an.bodenZ : 0.0F;
        const float von[3] = {start.x, start.y, start.z};
        const float hin[3] = {richtung.x, richtung.y, richtung.z};
        if (std::fabs(hin[k]) < 1e-6F) { return 0; }
        const float l = (ebene - von[k]) / hin[k];
        if (persp && l <= 0.0F) { return 0; }   // ueber dem Horizont: die Ebene liegt hinter der Kamera
        for (int i = 0; i < 3; ++i) { out[i] = von[i] + hin[i] * l; }
        out[k] = ebene;
        return 1;
    }
    return -1;
}

int modellNavVierPlatz(int platz) {
    static const int kBlender[4] = {0, 3, 1, 2};
    return modellBlender() ? kBlender[std::clamp(platz, 0, 3)] : std::clamp(platz, 0, 3);
}

// --- Menues: Ansichten (3ds Max: V), Kuchenmenue View (Blender: `), Navigation ---------------------------------------------

namespace {

struct NavZeile {
    Str text;
    Str hinweis;
    std::function<void()> tun;
};

void navZeile(const NavZeile& z, bool an = false) {
    if (ImGui::MenuItem(tr(z.text), nullptr, an)) {
        z.tun();
        g_app->modelDirty = true;
    }
    if (z.hinweis != Str::MnModusEnde) { ImGui::SetItemTooltip("%s", tr(z.hinweis)); }
}

void ansichtWie3dsMax(int n, AnsichtArt art) {
    ModellTab& t = mt();
    ModellAnsicht& a = t.ansichten[n];
    wechselBeenden(n);
    if (art == AnsichtArt::Orthographisch) {
        float w[3];
        modellAnsichtWinkel(a, w);
        modellBlickSetzen(n, AnsichtArt::Orthographisch, w, false);
    } else {
        a.art = art;
        modellAnsichtEinpassenIn(n);
    }
    g_app->modelDirty = true;
}

}  // namespace

// Die Zeilen "Navigation" in den Menues der Kopfzeile ([Ansicht] bzw. View) und im Rechtsklick-Menue: jeder Befehl ohne
// eigenes Feld, mit Tooltip - und die Einstellungen der Navigation.
void modellNavMenue(int n) {
    ModellTab& t = mt();
    Settings& st = g_app->settings;
    if (modellBlender()) {
        navZeile({Str::MnBlZoomHeran, Str::MnBlZoomMausHint, [] { modellNavZoomSchritt(1.0F); }});
        navZeile({Str::MnBlZoomHinaus, Str::MnBlZoomMausHint, [] { modellNavZoomSchritt(-1.0F); }});
        navZeile({Str::MnBlBereich, Str::MnModusHint, [] { modellNavBlBereich(); }}, t.navModus == 4);
        navZeile({Str::MnBlAllesMitte, Str::MkHomeHint, [] { (void)modellNavBlEinpassen(false, true); }});
        navZeile({Str::MnBlKuchen, Str::MnModusEnde, [] { modellNavKuchen(); }});
        ImGui::Separator();
        if (ImGui::MenuItem(tr(Str::MnBlOrbitAuswahl), nullptr, st.modellBlOrbitAuswahl)) { st.modellBlOrbitAuswahl = !st.modellBlOrbitAuswahl; }
        ImGui::SetItemTooltip("%s", tr(Str::MnBlOrbitAuswahlHint));
        if (ImGui::MenuItem(tr(Str::MnBlZoomMaus), nullptr, st.modellBlZoomMaus)) { st.modellBlZoomMaus = !st.modellBlZoomMaus; }
        ImGui::SetItemTooltip("%s", tr(Str::MnBlZoomMausHint));
        return;
    }
    t.aktiv = n;
    navZeile({Str::MnGrenzenAuswahl, Str::MnGrenzenAuswahlHint, [] { modellNavGrenzen(0); }});
    navZeile({Str::MnGrenzen, Str::MnGrenzenHint, [] { modellNavGrenzen(1); }});
    navZeile({Str::MnGrenzenAlle, Str::MnGrenzenAlleHint, [] { modellNavGrenzen(2); }});
    // FIX612C (P3-9): im Bereich Painting stellen [ und ] den Pinsel - dort nennt die Zeile die Taste, die zoomt
    const bool malen = t.bereich == ModellBereich::Painting;
    navZeile({malen ? Str::MnZoomHeran2 : Str::MnZoomHeran, Str::MnZoomMausHint, [] { modellNavZoomSchritt(1.0F); }});
    navZeile({malen ? Str::MnZoomHinaus2 : Str::MnZoomHinaus, Str::MnZoomMausHint, [] { modellNavZoomSchritt(-1.0F); }});
    navZeile({Str::MnZoom2xHeran, Str::MnGrenzenHint, [] { modellNavZoom2x(true); }});
    navZeile({Str::MnZoom2xHinaus, Str::MnGrenzenHint, [] { modellNavZoom2x(false); }});
    ImGui::Separator();
    navZeile({Str::MnModusZoom, Str::MnModusHint, [] { modellNavModusSetzen(1); }}, t.navModus == 1);
    navZeile({Str::MnModusPan, Str::MnModusHint, [] { modellNavModusSetzen(2); }}, t.navModus == 2);
    navZeile({Str::MnModusOrbit, Str::MnModusHint, [] { modellNavModusSetzen(3); }}, t.navModus == 3);
    navZeile({Str::MnModusBereich, Str::MnModusHint, [] { modellNavModusSetzen(4); }}, t.navModus == 4);
    ImGui::Separator();
    navZeile({Str::MnRueck, Str::MnRueckHint, [] { (void)modellNavRueck(false); }});
    navZeile({Str::MnWieder, Str::MnRueckHint, [] { (void)modellNavRueck(true); }});
    ImGui::Separator();
    if (ui::menue(tr(Str::MnOrbitArt))) {
        ++t.kopfMenueZaehler;
        static const Str kArt[4] = {Str::MnOrbitMitte, Str::MnOrbitAuswahl, Str::MnOrbitUnter, Str::MnOrbitPunkt};
        for (int i = 0; i < 4; ++i) {
            if (ImGui::MenuItem(tr(kArt[i]), nullptr, st.modellOrbitArt == i)) { st.modellOrbitArt = i; }
            ImGui::SetItemTooltip("%s", tr(Str::MnOrbitArtHint));
        }
        ImGui::EndMenu();
    }
    if (ImGui::MenuItem(tr(Str::MnZoomMausOrtho), nullptr, st.modellZoomMausOrtho)) { st.modellZoomMausOrtho = !st.modellZoomMausOrtho; }
    ImGui::SetItemTooltip("%s", tr(Str::MnZoomMausHint));
    if (ImGui::MenuItem(tr(Str::MnZoomMausPersp), nullptr, st.modellZoomMausPersp)) { st.modellZoomMausPersp = !st.modellZoomMausPersp; }
    ImGui::SetItemTooltip("%s", tr(Str::MnZoomMausHint));
}

// "Alle Aktionen suchen": die Befehle der Navigation ohne eigene Taste (die mit Taste stehen in der Tastenliste). ENTSCHEID3A (B10):
// Zoom Extents All hat jetzt seine Taste wie in 3ds Max (Strg+Umschalt+Z) und steht in der Tastenliste - hier bleibt nichts.
bool modellNavAktionenSuche(const std::string& such) {
    (void)such;
    return false;
}

void drawModellNavPopups() {
    ModellTab& t = mt();
    const int n = std::clamp(t.aktiv, 0, 3);
    if (g_ansichtenMenue) {
        ImGui::OpenPopup("##mnansichten");
        g_ansichtenMenue = false;
    }
    if (ui::popup("##mnansichten")) {
        // 3ds Max: das Quad-Menue "Viewports" (V) - die Ansichten, wie die Tasten T/B/F/L/P/U
        static const struct {
            AnsichtArt art;
            const char* taste;
        } kAnsichten[8] = {{AnsichtArt::Perspektive, "P"}, {AnsichtArt::Orthographisch, "U"}, {AnsichtArt::Vorne, "F"}, {AnsichtArt::Hinten, nullptr},
                           {AnsichtArt::Oben, "T"},        {AnsichtArt::Unten, "B"},          {AnsichtArt::Links, "L"}, {AnsichtArt::Rechts, nullptr}};
        const AnsichtArt jetzt = t.ansichten[n].art;
        for (const auto& z : kAnsichten) {
            if (ImGui::MenuItem(modellAnsichtText(z.art), z.taste, jetzt == z.art)) { ansichtWie3dsMax(n, z.art); }
        }
        ImGui::EndPopup();
    }
    if (g_kuchen) {
        ImGui::OpenPopup("##mnkuchen");
        g_kuchen = false;
    }
    // Blender VIEW3D_MT_view_pie: Left (West), Right (Ost), Bottom (Sued), Top (Nord), Front (Nordwest), Back (Nordost), View
    // Camera (Suedwest), View Selected (Suedost) - die Reihenfolge der Kuchenmenues von Blender
    const float fs = ImGui::GetFontSize();
    const float radius = fs * 6.0F;
    const ImVec2 feld{fs * 7.5F, fs * 1.7F};
    const ImVec2 groesse{radius * 2.0F + feld.x + fs, radius * 2.0F + feld.y + fs};
    ImGui::SetNextWindowPos(ImVec2{g_kuchenMitte.x - groesse.x * 0.5F, g_kuchenMitte.y - groesse.y * 0.5F}, ImGuiCond_Appearing);
    ImGui::SetNextWindowSize(groesse, ImGuiCond_Appearing);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4{0.0F, 0.0F, 0.0F, 0.0F});
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4{0.0F, 0.0F, 0.0F, 0.0F});
    if (ui::popup("##mnkuchen", ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        static const struct {
            float x;
            float y;
            int was;   // 0..5 Achsenansicht, 6 Kamera, 7 Auswahl
            AnsichtArt art;
        } kStuecke[8] = {{-1.0F, 0.0F, 0, AnsichtArt::Links},    {1.0F, 0.0F, 1, AnsichtArt::Rechts},      {0.0F, 1.0F, 2, AnsichtArt::Unten},
                         {0.0F, -1.0F, 3, AnsichtArt::Oben},     {-0.72F, -0.6F, 4, AnsichtArt::Vorne},    {0.72F, -0.6F, 5, AnsichtArt::Hinten},
                         {-0.72F, 0.6F, 6, AnsichtArt::Perspektive}, {0.72F, 0.6F, 7, AnsichtArt::Perspektive}};
        const ImVec2 lo = ImGui::GetWindowPos();
        const ImVec2 mitte{lo.x + groesse.x * 0.5F, lo.y + groesse.y * 0.5F};
        for (const auto& k : kStuecke) {
            ImGui::SetCursorScreenPos(ImVec2{mitte.x + k.x * radius - feld.x * 0.5F, mitte.y + k.y * radius - feld.y * 0.5F});
            const char* name = k.was == 6 ? tr(Str::MnBlKamera) : (k.was == 7 ? tr(Str::MkBlGewaehlte) : modellAnsichtText(k.art));
            ImGui::BeginDisabled(k.was == 6);   // der Model-Tab hat keine Szenenkamera
            if (ImGui::Button(name, feld)) {
                t.aktiv = n;
                if (k.was == 7) {
                    (void)modellNavBlEinpassen(true, false);
                } else {
                    modellNavBlAchse(k.art);
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndDisabled();
            if (k.was == 6) { ImGui::SetItemTooltip("%s", tr(Str::MnBlKameraHint)); }
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();
}

// Ein neues Modell, ein anderer Bedienmodus: laufende Modi, Zuege und weiche Wechsel enden.
void modellNavZuruecksetzen() {
    mt().navModus = 0;
    g_blBereich = false;
    g_zug = Zug{};
    for (Flug& fl : g_flug) { fl.aktiv = false; }
}


// AP11 (Modelle in Tabs): der Verlauf der Ansichtsaenderungen (Umschalt+Z / Umschalt+Y) gehoert zu den Ansichten des Modells - mit ins
// Fach. Laufende weiche Wechsel, Zuege und offene Menues enden.
void modellNavTauschen(ModellFachZeiger& f) {
    struct Fach : ModellFach {
        std::array<Verlauf, 4> verlauf;
    };
    if (!f) { f = std::make_unique<Fach>(); }
    std::swap(g_verlauf, static_cast<Fach&>(*f).verlauf);
    for (Flug& fl : g_flug) { fl.aktiv = false; }
    g_zug = Zug{};
    g_ansichtenMenue = false;
    g_kuchen = false;
    g_blBereich = false;
}

}  // namespace bhed::gui
