// app_modelltab_ansicht.cpp - die Ansichten des Model-Tabs (Modell-Block 1a-1)
//
//   eine oder vier Ansichten (Oben, Vorne, Links, Perspektive; Alt+W), je ein
//   eigenes Renderziel (gpu::modellZielWaehlen), gezeichnet ueber den
//   Ghoul2-Weg des Mission-Tabs (zeichneFigurIn);
//   die Pose wie im Spiel: Primaer auf dem ganzen Skelett, Sekundaer ab einem
//   Knochen (Vorgabe lower_lumbar wie SET_ANIM_UPPER), zwischen zwei Bildern
//   gemischt (tr_ghoul2.cpp:1531) oder nicht (ModView "Interpolate");
//   Anbauten am Bolzen (G2_GetBoltMatrixLow: Knochen = Endmatrix *
//   BasePose, Tag-Flaeche = G2_ProcessSurfaceBolt2);
//   die Ueberlagerung wie ModView (Knochen als Bone-Objekte wie 3ds Max,
//   Tags, Normalen, Gewichte, Huelle, Ursprung, Risse, Auswahl);
//   Maus wie 3ds Max: Mitte schieben, Alt+Mitte kreisen, Strg+Alt+Mitte und
//   Rad zoomen, links waehlen (Strg dazu, Alt weg, Rahmen ziehen), Gizmo
//   W/E/R auf den gewaehlten Flaechen (aendert das Dokument, Strg+Z).
//
//   mb1a2 ported parts: ModView (r_surface.cpp, model.cpp; Raven Software, GNU General Public License version 2) and
//   Blender (weight_to_rgb, bone_octahedral_verts; Copyright (C) Blender Authors, GNU General Public License version 2
//   or later). This file is part of BehavEd-Remake and licensed under the GNU General Public License version 2 or later
//   (LICENSE).
#include "app_internal.h"
#include "gpumap.h"

#include "bhed/gpucam.h"
#include "bhed/gpuskin.h"
#include "bhed/modell/dokument.h"
#include "bhed/modell/g2.h"
#include "bhed/modell/rig.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

namespace bhed::gui {
namespace {

ModellTab& mt() { return g_app->mtab; }

constexpr float kPi = 3.14159265F;
// mb1a2: Spiegelpartner beim Malen und Spiegeln - hoechstens so weit vom gespiegelten Ort (Einheiten); gemessen an den
// Figuren der Spiele (NOTIZEN_mb1a2.md).
constexpr float kSpiegelToleranz = 0.1F;

// Die Achsen einer Ansicht (rechts = vorn x hoch wie AngleVectors). Eine Figur
// schaut in ihrer .glm nach +Y (die Engine dreht sie beim Zeichnen um 90 Grad,
// kActorYawFix in gpuskin.h): "Vorne" zeigt ihre Vorderseite, "Links" ihre
// linke Seite (-X); nachgesehen an den Bildern der vier Ansichten (mb1_hb3).
void ansichtAchsen(AnsichtArt a, float r[3], float h[3], float v[3]) {
    const auto setze = [](float* x, float a0, float a1, float a2) {
        x[0] = a0;
        x[1] = a1;
        x[2] = a2;
    };
    switch (a) {
        case AnsichtArt::Oben:
            setze(r, 1, 0, 0), setze(h, 0, 1, 0), setze(v, 0, 0, -1);
            break;
        case AnsichtArt::Unten:
            setze(r, -1, 0, 0), setze(h, 0, 1, 0), setze(v, 0, 0, 1);
            break;
        case AnsichtArt::Vorne:
            setze(r, -1, 0, 0), setze(h, 0, 0, 1), setze(v, 0, -1, 0);
            break;
        case AnsichtArt::Links:
            setze(r, 0, -1, 0), setze(h, 0, 0, 1), setze(v, 1, 0, 0);
            break;
        case AnsichtArt::Hinten:   // mb1a2: Blender Strg+Ziffernblock 1
            setze(r, 1, 0, 0), setze(h, 0, 0, 1), setze(v, 0, 1, 0);
            break;
        case AnsichtArt::Rechts:   // mb1a2: Blender Ziffernblock 3
            setze(r, 0, 1, 0), setze(h, 0, 0, 1), setze(v, -1, 0, 0);
            break;
        case AnsichtArt::Perspektive: {
            Camera c = g_app->modelCam;
            c.right(r);
            c.up(h);
            c.forward(v);
            break;
        }
    }
}

Str ansichtName(AnsichtArt a) {
    switch (a) {
        case AnsichtArt::Oben: return Str::MbAnsichtOben;
        case AnsichtArt::Vorne: return Str::MbAnsichtVorne;
        case AnsichtArt::Links: return Str::MbAnsichtLinks;
        case AnsichtArt::Unten: return Str::MbAnsichtUnten;
        case AnsichtArt::Hinten: return Str::MbAnsichtHinten;
        case AnsichtArt::Rechts: return Str::MbAnsichtRechts;
        case AnsichtArt::Perspektive: break;
    }
    return Str::MbAnsichtPersp;
}

void baueSicht(const ModellAnsicht& a, int w, int h, float* vp) {
    if (a.art == AnsichtArt::Perspektive) {
        gpu::baueViewProj(g_app->modelCam, w, h, 1.0F, 8192.0F, vp);
        return;
    }
    float r[3];
    float o[3];
    float v[3];
    ansichtAchsen(a.art, r, o, v);
    gpu::baueOrtho(r, o, v, a.mitte, a.halbHoehe, w, h, 4096.0F, vp);
}

// Welt -> Schirm ueber die Matrix der Ansicht; `tiefe` z/w (0 nah, 1 fern).
bool aufSchirm(const ModellAnsicht& a, const float p[3], ImVec2& out, float* tiefe = nullptr) {
    const float* m = a.viewProj;
    const float x = m[0] * p[0] + m[1] * p[1] + m[2] * p[2] + m[3];
    const float y = m[4] * p[0] + m[5] * p[1] + m[6] * p[2] + m[7];
    const float z = m[8] * p[0] + m[9] * p[1] + m[10] * p[2] + m[11];
    const float w = m[12] * p[0] + m[13] * p[1] + m[14] * p[2] + m[15];
    if (w <= 1e-3F) { return false; }
    out = ImVec2{a.lo[0] + (x / w * 0.5F + 0.5F) * a.groesse[0], a.lo[1] + (0.5F - y / w * 0.5F) * a.groesse[1]};
    if (tiefe != nullptr) { *tiefe = z / w; }
    return true;
}

// --- Pose und Haut auf der CPU ----------------------------------------------------------------

std::vector<BoneMatrix> g_knochen;           // skinM je Knochen (baueKnochenmatrizen)
int g_poseStand = 0;                         // zaehlt jede neue Pose
int g_posiertBeiStand = -1;                  // Stufe 3: dok.stand() beim letzten posieren()
std::vector<std::vector<std::array<float, 3>>> g_haut;   // verformte Ecken der Anzeige je Flaeche
int g_hautPose = -1;
int g_hautAnzeige = -1;

int naechstesBild(const AnimEntry& e, int f) {
    if (f + 1 < e.firstFrame + e.numFrames) { return f + 1; }
    const bool schleife = g_app->loopMode == 1 || (g_app->loopMode == 0 && e.loopFrame >= 0);
    return schleife ? e.firstFrame + std::max(e.loopFrame, 0) : f;
}

// Die Pose wie im Spiel: Primaer auf allen Knochen, Sekundaer ab
// sekundaerKnochen (BoneFrameOverride wie SET_ANIM_UPPER auf lower_lumbar),
// gemischt nach dem Bruchteil des Bildes (Interpolation an) oder nicht.
void posieren() {
    ModellTab& t = mt();
    const GlaAnimation& a = g_app->anim;
    g_app->modellWelt.clear();
    g_knochen.clear();
    ++g_poseStand;
    if (a.bones.empty() || a.numFrames <= 0) { return; }
    const int bilder = a.bildZahl();
    const int fA = std::clamp(g_app->animFrame, 0, bilder - 1);
    int fB = fA;
    float anteil = 0.0F;
    const bool primaer = g_app->animIndex >= 0 && static_cast<std::size_t>(g_app->animIndex) < g_app->animList.size();
    if (t.interpolieren && primaer) {
        fB = std::clamp(naechstesBild(g_app->animList[static_cast<std::size_t>(g_app->animIndex)], fA), 0, bilder - 1);
        anteil = std::clamp(g_app->animTime, 0.0F, 1.0F);
    }
    std::vector<BoneFrameOverride> ueber;
    if (t.sekundaer >= 0 && static_cast<std::size_t>(t.sekundaer) < g_app->animList.size()) {
        const AnimEntry& s = g_app->animList[static_cast<std::size_t>(t.sekundaer)];
        BoneFrameOverride o;
        o.bone = boneIndex(a, t.sekundaerKnochen);
        o.frame = std::clamp(t.sekFrame, 0, bilder - 1);
        if (t.interpolieren) {
            o.next = std::clamp(naechstesBild(s, o.frame), 0, bilder - 1);
            o.fraction = std::clamp(t.sekZeit, 0.0F, 1.0F);
        }
        if (o.bone >= 0) { ueber.push_back(o); }
    }
    // Stufe 3: die Schluessel des Control Rigs (FK wie G2API_SetBoneAngles POSTMULT, siehe bhed/modell/rig.h)
    std::vector<std::pair<int, BoneMatrix>> rigM;
    modellRigNachMal(fA, fB, anteil, rigM, ueber);   // Stufe 4: mit IK (auf derselben Welt wie unten)
    a.worldMatricesLerp(fA, fB, anteil, ueber, rigM, g_app->modellWelt);
    (void)modellRtPose(g_app->modellWelt);   // Stufe 5: die Vorschau des Retargetings (gepackt wie im Spiel)
    gpu::baueKnochenmatrizen(g_app->modellWelt, a.bones, g_knochen);
}

void verforme(const GlmVertex& v, float out[3]) {
    if (g_knochen.empty()) {
        std::memcpy(out, v.xyz, sizeof(float) * 3U);
        return;
    }
    out[0] = out[1] = out[2] = 0.0F;
    for (int k = 0; k < 4; ++k) {
        if (v.weights[k] <= 0.0F || v.bones[k] >= g_knochen.size()) { continue; }
        float p[3];
        g_knochen[v.bones[k]].transform(v.xyz, p);
        for (int i = 0; i < 3; ++i) { out[i] += p[i] * v.weights[k]; }
    }
}

const std::vector<std::vector<std::array<float, 3>>>& haut() {
    const ModellTab& t = mt();
    if (g_hautPose == g_poseStand && g_hautAnzeige == t.neuGebaut) { return g_haut; }
    g_haut.assign(t.anzeige.surfaces.size(), {});
    for (std::size_t i = 0; i < t.anzeige.surfaces.size(); ++i) {
        const GlmSurface& s = t.anzeige.surfaces[i];
        g_haut[i].resize(s.verts.size());
        for (std::size_t k = 0; k < s.verts.size(); ++k) { verforme(s.verts[k], g_haut[i][k].data()); }
    }
    g_hautPose = g_poseStand;
    g_hautAnzeige = t.neuGebaut;
    return g_haut;
}

bool gezeichnet(std::size_t i) {
    const ModellTab& t = mt();
    // mb1a2: auch nicht, was im Explorer ausgeblendet ist (Auswahl, Rahmen, Pinsel, Grenzen gehen daran vorbei)
    return i < t.gezeichnet.size() && t.gezeichnet[i] != 0 && !modellTeilVersteckt(static_cast<int>(i));
}

// Die Grenzen der gewaehlten (oder aller gezeichneten) Flaechen in der Pose.
bool grenzen(bool nurAuswahl, float mn[3], float mx[3]) {
    const ModellTab& t = mt();
    const auto& h = haut();
    bool da = false;
    if (nurAuswahl && t.auswahlArt != AuswahlArt::Objekt) {   // mb1a2: die gewaehlten Ecken
        for (const modell::EckOrt& o : t.eckenWahl) {
            if (o.teil < 0 || static_cast<std::size_t>(o.teil) >= h.size() || o.ecke < 0 ||
                static_cast<std::size_t>(o.ecke) >= h[static_cast<std::size_t>(o.teil)].size()) {
                continue;
            }
            const auto& p = h[static_cast<std::size_t>(o.teil)][static_cast<std::size_t>(o.ecke)];
            for (int k = 0; k < 3; ++k) {
                mn[k] = da ? std::min(mn[k], p[static_cast<std::size_t>(k)]) : p[static_cast<std::size_t>(k)];
                mx[k] = da ? std::max(mx[k], p[static_cast<std::size_t>(k)]) : p[static_cast<std::size_t>(k)];
            }
            da = true;
        }
        return da;
    }
    for (std::size_t i = 0; i < h.size(); ++i) {
        const bool rein = nurAuswahl ? std::find(t.auswahl.begin(), t.auswahl.end(), static_cast<int>(i)) != t.auswahl.end() : gezeichnet(i);
        if (!rein) { continue; }
        for (const auto& p : h[i]) {
            for (int k = 0; k < 3; ++k) {
                mn[k] = da ? std::min(mn[k], p[static_cast<std::size_t>(k)]) : p[static_cast<std::size_t>(k)];
                mx[k] = da ? std::max(mx[k], p[static_cast<std::size_t>(k)]) : p[static_cast<std::size_t>(k)];
            }
            da = true;
        }
    }
    return da;
}

// --- Anbauten und Boden ---------------------------------------------------------------------

// Die Stellung eines Anbaus (4x4 zeilenweise) in der Figur.
bool anbauWelt(const ModellAnbau& a, float welt[16]) {
    BoneMatrix m;
    if (a.knochen) {
        // G2_GetBoltMatrixLow: Knochen = Endmatrix * BasePose - die Weltmatrix aus worldMatrices.
        const int b = boneIndex(g_app->anim, a.ziel);
        if (b < 0 || static_cast<std::size_t>(b) >= g_app->modellWelt.size()) { return false; }
        m = g_app->modellWelt[static_cast<std::size_t>(b)];
    } else {
        const int s = surfaceIndex(g_app->model, a.ziel);
        if (s < 0) { return false; }
        const bool ok = g_app->modellWelt.empty() ? boltMatrixRigid(g_app->model, s, m)
                                                  : boltMatrix(g_app->model, s, g_app->modellWelt, g_app->anim, m);
        if (!ok) { return false; }
    }
    const float einheit[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    gpu::baueGriffWelt(einheit, m, welt);
    return true;
}

// Ein Rechteck als zwei Dreiecke, beidseitig (Boden, Rasterlinien).
void viereck(GlmSurface& s, float x0, float y0, float x1, float y1, float z) {
    const auto ecke = [&](float x, float y) {
        GlmVertex v;
        v.xyz[0] = x;
        v.xyz[1] = y;
        v.xyz[2] = z;
        v.normal[2] = 1.0F;
        v.weights[0] = 1.0F;
        s.verts.push_back(v);
    };
    const auto b = static_cast<std::uint32_t>(s.verts.size());
    ecke(x0, y0);
    ecke(x1, y0);
    ecke(x1, y1);
    ecke(x0, y1);
    for (const std::uint32_t i : {0U, 1U, 2U, 0U, 2U, 3U, 0U, 2U, 1U, 0U, 3U, 2U}) { s.indexes.push_back(b + i); }
}

// Der Boden (ModView: grau, Hoehe einstellbar) und das Raster (G) als Figuren
// ohne Bild - die helle Ersatzfarbe des Modellfensters.
const GlmModel& bodenModell(float z, bool raster, float groesse) {
    static GlmModel m;
    static std::string schluessel;
    char k[64];
    std::snprintf(k, sizeof k, "%.2f|%d|%.1f", static_cast<double>(z), raster ? 1 : 0, static_cast<double>(groesse));
    if (schluessel == k) { return m; }
    GlmModel neu;
    neu.name = "boden";
    GlmSurface s;
    s.name = "boden";
    if (raster) {
        const float schritt = std::max(std::round(groesse / 8.0F), 1.0F);
        const float breite = schritt * 0.03F;
        for (int i = -8; i <= 8; ++i) {
            const float a = static_cast<float>(i) * schritt;
            viereck(s, a - breite, -groesse, a + breite, groesse, z + 0.05F);
            viereck(s, -groesse, a - breite, groesse, a + breite, z + 0.05F);
        }
    } else {
        viereck(s, -groesse, -groesse, groesse, groesse, z);
    }
    neu.surfaces.push_back(std::move(s));
    m = std::move(neu);
    schluessel = k;
    return m;
}

// --- Ueberlagerung ---------------------------------------------------------------------------

void linie(ImDrawList* dl, const ModellAnsicht& a, const float p[3], const float q[3], ImU32 farbe, float dicke = 1.0F) {
    ImVec2 sp;
    ImVec2 sq;
    if (aufSchirm(a, p, sp) && aufSchirm(a, q, sq)) { dl->AddLine(sp, sq, farbe, dicke); }
}

void kasten(ImDrawList* dl, const ModellAnsicht& a, const float mn[3], const float mx[3], ImU32 farbe) {
    for (int k = 0; k < 12; ++k) {
        // die zwoelf Kanten: je Achse vier
        const int achse = k / 4;
        const int u = (achse + 1) % 3;
        const int v = (achse + 2) % 3;
        float p[3];
        float q[3];
        p[achse] = mn[achse];
        q[achse] = mx[achse];
        p[u] = q[u] = (k & 1) != 0 ? mx[u] : mn[u];
        p[v] = q[v] = (k & 2) != 0 ? mx[v] : mn[v];
        linie(dl, a, p, q, farbe);
    }
}

// Knochen als Bone-Objekte wie 3ds Max: ein Kasten vom Gelenk des Elternteils
// zum Gelenk des Knochens, zum Ende hin auf 10 % verjuengt (Taper 90 %, wie
// beim Erstellen ueber die Oberflaeche), Flossen schaltbar. Eigene Zeichnung.
void knochenZeichnen(ImDrawList* dl, const ModellAnsicht& a) {
    const ModellTab& t = mt();
    const std::vector<BoneMatrix>& w = g_app->modellWelt;
    const GlaAnimation& an = g_app->anim;
    const std::size_t n = std::min(w.size(), an.bones.size());
    for (std::size_t b = 0; b < n; ++b) {
        if (modellKnochenVersteckt(static_cast<int>(b))) { continue; }   // mb1a2: Auge im Explorer
        const int p = an.bones[b].parent;
        const float e[3] = {w[b].m[0][3], w[b].m[1][3], w[b].m[2][3]};
        const bool gewaehlt = t.knochenWahl == static_cast<int>(b);
        if (modellSpielKnochen(an.bones[b].name)) {
            ImVec2 s;
            if (aufSchirm(a, e, s)) { dl->AddCircleFilled(s, 3.0F, IM_COL32(255, 150, 40, 230)); }
        }
        if (p < 0 || static_cast<std::size_t>(p) >= n) { continue; }
        const float o[3] = {w[static_cast<std::size_t>(p)].m[0][3], w[static_cast<std::size_t>(p)].m[1][3], w[static_cast<std::size_t>(p)].m[2][3]};
        float x[3] = {e[0] - o[0], e[1] - o[1], e[2] - o[2]};
        const float len = std::sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
        if (len < 0.05F) { continue; }
        for (float& c : x) { c /= len; }
        // Quer: die z-Achse des Elternknochens, senkrecht zur Laenge gemacht.
        float up[3] = {w[static_cast<std::size_t>(p)].m[0][2], w[static_cast<std::size_t>(p)].m[1][2], w[static_cast<std::size_t>(p)].m[2][2]};
        float d = up[0] * x[0] + up[1] * x[1] + up[2] * x[2];
        float y[3] = {up[0] - x[0] * d, up[1] - x[1] * d, up[2] - x[2] * d};
        float yl = std::sqrt(y[0] * y[0] + y[1] * y[1] + y[2] * y[2]);
        if (yl < 1e-3F) {
            const float alt[3] = {0.0F, 0.0F, 1.0F};
            d = alt[2] * x[2];
            y[0] = -x[0] * d;
            y[1] = -x[1] * d;
            y[2] = 1.0F - x[2] * d;
            yl = std::sqrt(y[0] * y[0] + y[1] * y[1] + y[2] * y[2]);
            if (yl < 1e-3F) { continue; }
        }
        for (float& c : y) { c /= yl; }
        const float z[3] = {x[1] * y[2] - x[2] * y[1], x[2] * y[0] - x[0] * y[2], x[0] * y[1] - x[1] * y[0]};
        if (g_app->settings.modellBedienung == 1) {
            // mb1a2, wie Blender: Oktaeder (display_type OCTAHEDRAL, Vorgabe) - der Kopf am Gelenk des Elternteils,
            // der Ring der vier Ecken bei 10 % der Laenge mit 10 % Abstand von der Achse, die Spitze am Gelenk des
            // Knochens (Blender draw_armature: bone_octahedral_verts), die Gelenke als Kreise. Eigene Zeichnung.
            float ring[4][3];
            static const float kRing[4][2] = {{0.1F, 0.1F}, {0.1F, -0.1F}, {-0.1F, -0.1F}, {-0.1F, 0.1F}};
            float spitze[3];
            for (int k = 0; k < 3; ++k) { spitze[k] = o[k] + x[k] * len; }
            for (int r = 0; r < 4; ++r) {
                for (int k = 0; k < 3; ++k) { ring[r][k] = o[k] + x[k] * len * 0.1F + y[k] * len * kRing[r][0] + z[k] * len * kRing[r][1]; }
            }
            const ImU32 farbe = gewaehlt ? IM_COL32(80, 160, 255, 255) : IM_COL32(170, 170, 170, 220);   // gewaehlt blau wie in Pose
            for (int r = 0; r < 4; ++r) {
                linie(dl, a, o, ring[r], farbe, gewaehlt ? 2.0F : 1.0F);
                linie(dl, a, ring[r], ring[(r + 1) % 4], farbe, gewaehlt ? 2.0F : 1.0F);
                linie(dl, a, ring[r], spitze, farbe, gewaehlt ? 2.0F : 1.0F);
            }
            ImVec2 sk;
            if (aufSchirm(a, o, sk)) { dl->AddCircle(sk, 2.5F, farbe); }
            g_app->modellKnochenLinien += 12;
            mt().ueber[0] += 12;
            continue;
        }
        const float w0 = std::clamp(len * 0.12F, 0.3F, 2.5F) * 0.5F;
        const float w1 = w0 * 0.1F;
        float ecken[8][3];
        for (int i = 0; i < 8; ++i) {
            const float* basis = i < 4 ? o : e;
            const float r = i < 4 ? w0 : w1;
            const float sy = (i & 1) != 0 ? r : -r;
            const float sz = (i & 2) != 0 ? r : -r;
            for (int k = 0; k < 3; ++k) { ecken[i][k] = basis[k] + y[k] * sy + z[k] * sz; }
        }
        const ImU32 farbe = gewaehlt ? IM_COL32(255, 230, 60, 255) : IM_COL32(200, 205, 120, 210);
        static const int kKanten[12][2] = {{0, 1}, {1, 3}, {3, 2}, {2, 0}, {4, 5}, {5, 7}, {7, 6}, {6, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
        for (const auto& k : kKanten) { linie(dl, a, ecken[k[0]], ecken[k[1]], farbe, gewaehlt ? 2.0F : 1.0F); }
        g_app->modellKnochenLinien += 12;
        mt().ueber[0] += 12;
        if (g_app->settings.modellFlossen) {
            // Seiten-, Vorder- und Hinterflosse: ein Dreieck je Seite, 10 % bis 90 % der Laenge.
            const float* richtungen[4] = {y, y, z, z};
            const float vorz[4] = {1.0F, -1.0F, 1.0F, -1.0F};
            for (int f = 0; f < 4; ++f) {
                float a0[3];
                float a1[3];
                float spitze[3];
                for (int k = 0; k < 3; ++k) {
                    a0[k] = o[k] + x[k] * len * 0.1F;
                    a1[k] = o[k] + x[k] * len * 0.9F;
                    spitze[k] = o[k] + x[k] * len * 0.3F + richtungen[f][k] * vorz[f] * w0 * 2.5F;
                }
                linie(dl, a, a0, spitze, farbe);
                linie(dl, a, spitze, a1, farbe);
            }
        }
    }
}

void ueberlagerung(const ModellAnsicht& a, ImDrawList* dl) {
    ModellTab& t = mt();
    const ImVec2 lo{a.lo[0], a.lo[1]};
    const ImVec2 hi{a.lo[0] + a.groesse[0], a.lo[1] + a.groesse[1]};
    dl->PushClipRect(lo, hi, true);
    const ModellAnzeige& z = t.an;
    const bool brauchtHaut = z.normalen || z.gewichte || z.huelle || z.risse || z.tags || z.klammer || !t.auswahl.empty() || !t.eckenWahl.empty();
    const auto& h = brauchtHaut ? haut() : g_haut;
    if (g_app->modellKnochen) { knochenZeichnen(dl, a); }
    if (z.ursprung) {
        const float o[3] = {0.0F, 0.0F, 0.0F};
        const float ax[3][3] = {{16, 0, 0}, {0, 16, 0}, {0, 0, 16}};
        const ImU32 farben[3] = {IM_COL32(230, 60, 60, 255), IM_COL32(60, 210, 60, 255), IM_COL32(70, 110, 250, 255)};
        for (int k = 0; k < 3; ++k) { linie(dl, a, o, ax[k], farben[k], 2.0F); }
        t.ueber[5] += 3;
    }
    for (std::size_t i = 0; brauchtHaut && i < h.size() && i < t.anzeige.surfaces.size(); ++i) {
        const GlmSurface& s = t.anzeige.surfaces[i];
        const bool gew = std::find(t.auswahl.begin(), t.auswahl.end(), static_cast<int>(i)) != t.auswahl.end();
        if (s.isTag()) {
            if (!z.tags || s.indexes.size() < 3U || modellTeilVersteckt(static_cast<int>(i))) { continue; }
            for (int k = 0; k < 3; ++k) {
                linie(dl, a, h[i][s.indexes[static_cast<std::size_t>(k)]].data(), h[i][s.indexes[static_cast<std::size_t>((k + 1) % 3)]].data(),
                      IM_COL32(110, 230, 110, 255));
            }
            ImVec2 sp;
            if (aufSchirm(a, h[i][s.indexes[0]].data(), sp)) {
                // mb1a2: dasselbe Symbol wie im Explorer vor dem Namen
                const float fh = ImGui::GetFontSize();
                uiIcon(dl, UiIcon::MbTag, sp, fh, IM_COL32(110, 230, 110, 255));
                dl->AddText(ImVec2{sp.x + fh * 1.1F, sp.y}, IM_COL32(110, 230, 110, 255), s.name.c_str());
            }
            t.ueber[3] += 3;
            continue;
        }
        if (!gezeichnet(i)) { continue; }
        if (gew) {
            for (std::size_t k = 0; k + 2 < s.indexes.size(); k += 3) {
                for (std::size_t e = 0; e < 3; ++e) {
                    linie(dl, a, h[i][s.indexes[k + e]].data(), h[i][s.indexes[k + (e + 1) % 3]].data(), IM_COL32(255, 160, 40, 170));
                    ++t.ueber[7];
                }
            }
        }
        if (z.normalen) {
            for (std::size_t k = 0; k < s.verts.size(); ++k) {
                float nrm[3] = {s.verts[k].normal[0], s.verts[k].normal[1], s.verts[k].normal[2]};
                if (!g_knochen.empty() && s.verts[k].bones[0] < g_knochen.size()) {
                    const BoneMatrix& m = g_knochen[s.verts[k].bones[0]];
                    const float n0[3] = {nrm[0], nrm[1], nrm[2]};
                    for (int r = 0; r < 3; ++r) { nrm[r] = m.m[r][0] * n0[0] + m.m[r][1] * n0[1] + m.m[r][2] * n0[2]; }
                }
                const float q[3] = {h[i][k][0] + nrm[0] * 2.0F, h[i][k][1] + nrm[1] * 2.0F, h[i][k][2] + nrm[2] * 2.0F};
                linie(dl, a, h[i][k].data(), q, IM_COL32(90, 160, 255, 200));
                ++t.ueber[1];
            }
        }
        if (z.gewichte) {
            for (std::size_t k = 0; k < s.verts.size(); ++k) {
                ImVec2 sp;
                if (!aufSchirm(a, h[i][k].data(), sp)) { continue; }
                const GlmVertex& v = s.verts[k];
                ImU32 farbe = 0;
                if (t.knochenWahl >= 0) {
                    // Gewicht auf dem gewaehlten Knochen: blau (wenig) bis rot (ganz).
                    float g = 0.0F;
                    for (int j = 0; j < 4; ++j) {
                        if (v.weights[j] > 0.0F && v.bones[j] == t.knochenWahl) { g += v.weights[j]; }
                    }
                    if (g <= 0.0F) { continue; }
                    farbe = IM_COL32(static_cast<int>(255.0F * g), 40, static_cast<int>(255.0F * (1.0F - g)), 230);
                } else {
                    int zahl = 0;
                    for (int j = 0; j < 4; ++j) { zahl += v.weights[j] > 0.0F ? 1 : 0; }
                    // mb1a2: die Farben von ModView (GetWeightColour, r_surface.cpp:17-55): 0 weiss, 1 hellgruen,
                    // 2 dunkelgruen, 3 gelb, 4 tuerkis, mehr rot
                    static const ImU32 kFarben[6] = {IM_COL32(255, 255, 255, 230), IM_COL32(0, 255, 0, 230), IM_COL32(0, 128, 0, 230),
                                                     IM_COL32(255, 255, 0, 230),   IM_COL32(0, 255, 255, 230), IM_COL32(255, 0, 0, 230)};
                    farbe = kFarben[std::clamp(zahl, 0, 5)];
                }
                dl->AddRectFilled(ImVec2{sp.x - 1.5F, sp.y - 1.5F}, ImVec2{sp.x + 1.5F, sp.y + 1.5F}, farbe);
                ++t.ueber[2];
            }
        }
        if (gew && (z.eckNummern || z.dreieckNummern)) {
            // mb1a2: ModView "Show vertex indexes" (rot, mit "Gewichte" in der Farbe der Gewichtszahl) und "Show triangle
            // indexes" ("T:%d" in der Mitte, 0/255/255) an der gewaehlten Flaeche (r_surface.cpp:864-935)
            if (z.eckNummern) {
                for (std::size_t k = 0; k < s.verts.size() && k < h[i].size(); ++k) {
                    ImVec2 sp;
                    if (!aufSchirm(a, h[i][k].data(), sp)) { continue; }
                    ImU32 f = IM_COL32(255, 0, 0, 255);
                    if (z.gewichte) {
                        int zahl = 0;
                        for (int j = 0; j < 4; ++j) { zahl += s.verts[k].weights[j] > 0.0F ? 1 : 0; }
                        static const ImU32 kF[6] = {IM_COL32(255, 255, 255, 255), IM_COL32(0, 255, 0, 255), IM_COL32(0, 128, 0, 255),
                                                    IM_COL32(255, 255, 0, 255),   IM_COL32(0, 255, 255, 255), IM_COL32(255, 0, 0, 255)};
                        f = kF[std::clamp(zahl, 0, 5)];
                    }
                    char n[16];
                    std::snprintf(n, sizeof n, " %d", static_cast<int>(k));
                    dl->AddText(ImVec2{sp.x, sp.y - ImGui::GetFontSize()}, f, n);   // Text_Display: ab dem Punkt, Grundlinie
                    ++t.ueber[8];
                }
            }
            if (z.dreieckNummern) {
                for (std::size_t k = 0; k + 2 < s.indexes.size(); k += 3) {
                    float m[3];
                    for (int c = 0; c < 3; ++c) {
                        m[c] = (h[i][s.indexes[k]][static_cast<std::size_t>(c)] + h[i][s.indexes[k + 1]][static_cast<std::size_t>(c)] +
                                h[i][s.indexes[k + 2]][static_cast<std::size_t>(c)]) /
                               3.0F;
                    }
                    ImVec2 sp;
                    if (!aufSchirm(a, m, sp)) { continue; }
                    char n[16];
                    std::snprintf(n, sizeof n, "T:%d", static_cast<int>(k / 3));
                    dl->AddText(ImVec2{sp.x, sp.y - ImGui::GetFontSize()}, IM_COL32(0, 255, 255, 255), n);
                    ++t.ueber[9];
                }
            }
        }
        if (z.risse && gew) {
            // ModView "crack highlight": die Ecken an offenen Kanten der gewaehlten Flaeche.
            const modell::Inhalt& in = t.dok.inhalt();
            if (i < in.teile.size() && static_cast<std::size_t>(t.lodJetzt) < in.teile[i].stufen.size()) {
                const std::vector<char> rand = in.teile[i].stufen[static_cast<std::size_t>(t.lodJetzt)].randEcken();
                for (std::size_t k = 0; k < rand.size() && k < h[i].size(); ++k) {
                    ImVec2 sp;
                    if (rand[k] == 0 || !aufSchirm(a, h[i][k].data(), sp)) { continue; }
                    dl->AddLine(ImVec2{sp.x - 3, sp.y}, ImVec2{sp.x + 3, sp.y}, IM_COL32(60, 255, 60, 255));
                    dl->AddLine(ImVec2{sp.x, sp.y - 3}, ImVec2{sp.x, sp.y + 3}, IM_COL32(60, 255, 60, 255));
                    ++t.ueber[6];
                }
            }
        }
    }
    if (z.lineal) {
        // mb1a2: ModView "Ruler" (model.cpp:2431-2462): ORIGIN_LINE_RADIUS 20, Hoehe 8 * 20 = 160 um den Ursprung, bei
        // x = -20 senkrecht, je Einheit ein Strich nach x = 0 und eine Zahl "%d __" bei x = -(20 + 20/6), Farbe 1/0.5/0.5
        constexpr int kHoehe = 20 * 8;
        const ImU32 f = IM_COL32(255, 128, 128, 255);
        const float oben[3] = {-20.0F, 0.0F, kHoehe / 2.0F};
        const float unten[3] = {-20.0F, 0.0F, -kHoehe / 2.0F};
        linie(dl, a, oben, unten, f);
        for (int i = 0; i < kHoehe; ++i) {
            const float zh = static_cast<float>(i - kHoehe / 2);
            const float p0[3] = {-20.0F, 0.0F, zh};
            const float p1[3] = {0.0F, 0.0F, zh};
            linie(dl, a, p0, p1, f);
            const float tp[3] = {-(20.0F + 20.0F / 6.0F), 0.0F, zh};
            ImVec2 sp;
            if (aufSchirm(a, tp, sp)) {
                char n[24];
                std::snprintf(n, sizeof n, "%d __", i - kHoehe / 2);
                // Text_Display (text.cpp:198-228): glRasterPos - der Text beginnt am Punkt, auf seiner Grundlinie
                dl->AddText(ImVec2{sp.x, sp.y - ImGui::GetFontSize()}, f, n);
            }
        }
        t.ueber[10] += kHoehe + 1;
    }
    for (const modell::EckOrt& o : t.eckenWahl) {   // mb1a2: die gewaehlten Ecken (orange wie die Auswahl)
        if (o.teil < 0 || static_cast<std::size_t>(o.teil) >= h.size() || o.ecke < 0 ||
            static_cast<std::size_t>(o.ecke) >= h[static_cast<std::size_t>(o.teil)].size()) {
            continue;
        }
        ImVec2 sp;
        if (!aufSchirm(a, h[static_cast<std::size_t>(o.teil)][static_cast<std::size_t>(o.ecke)].data(), sp)) { continue; }
        dl->AddRectFilled(ImVec2{sp.x - 2.5F, sp.y - 2.5F}, ImVec2{sp.x + 2.5F, sp.y + 2.5F}, IM_COL32(255, 160, 40, 255));
        ++t.ueber[12];
    }
    float mn[3];
    float mx[3];
    if (z.huelle && grenzen(false, mn, mx)) {
        kasten(dl, a, mn, mx, IM_COL32(230, 230, 230, 200));
        t.ueber[4] += 12;
    }
    if (z.klammer && !t.auswahl.empty() && grenzen(true, mn, mx)) {
        // Auswahlklammer: die acht Ecken als kurze Winkel.
        for (int c = 0; c < 8; ++c) {
            const float p[3] = {(c & 1) != 0 ? mx[0] : mn[0], (c & 2) != 0 ? mx[1] : mn[1], (c & 4) != 0 ? mx[2] : mn[2]};
            for (int k = 0; k < 3; ++k) {
                float q[3] = {p[0], p[1], p[2]};
                q[k] += (p[k] == mn[k] ? 1.0F : -1.0F) * (mx[k] - mn[k]) * 0.2F;
                linie(dl, a, p, q, IM_COL32(255, 255, 255, 230));
            }
        }
    }
    modellRigUeberlagerung(a, dl);   // Stufe 3: Griffe und Bewegungspfade
    dl->PopClipRect();
}

// --- Gizmo (W/E/R) ------------------------------------------------------------------------

bool auswahlMitte(float m[3]) {
    if (mt().bereich == ModellBereich::Animation && mt().griffWahl >= 0) { return modellRigGriffMitte(m); }   // Stufe 3
    float mn[3];
    float mx[3];
    const bool leer = mt().auswahlArt == AuswahlArt::Objekt ? mt().auswahl.empty() : mt().eckenWahl.empty();
    if (leer || !grenzen(true, mn, mx)) { return false; }
    for (int k = 0; k < 3; ++k) { m[k] = (mn[k] + mx[k]) * 0.5F; }
    return true;
}

// Weltlaenge, die bei mitte etwa `px` Punkte ergibt.
float laengeFuerPx(const ModellAnsicht& a, const float m[3], float px) {
    ImVec2 s0;
    ImVec2 s1;
    float r[3];
    float h[3];
    float v[3];
    ansichtAchsen(a.art, r, h, v);
    const float q[3] = {m[0] + h[0], m[1] + h[1], m[2] + h[2]};
    if (!aufSchirm(a, m, s0) || !aufSchirm(a, q, s1)) { return 16.0F; }
    const float proEinheit = std::hypot(s1.x - s0.x, s1.y - s0.y);
    return proEinheit > 1e-4F ? px / proEinheit : 16.0F;
}

void ringPunkt(const float m[3], int achse, float r, float w, float out[3]) {
    const int u = (achse + 1) % 3;
    const int v = (achse + 2) % 3;
    out[0] = m[0];
    out[1] = m[1];
    out[2] = m[2];
    out[u] += std::cos(w) * r;
    out[v] += std::sin(w) * r;
}

float abstandZuStrecke(ImVec2 p, ImVec2 a, ImVec2 b) {
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    const float l2 = dx * dx + dy * dy;
    float s = l2 > 0.0F ? ((p.x - a.x) * dx + (p.y - a.y) * dy) / l2 : 0.0F;
    s = std::clamp(s, 0.0F, 1.0F);
    return std::hypot(p.x - (a.x + dx * s), p.y - (a.y + dy * s));
}

// Griff unter der Maus: 1..3 Achse, 4..6 Ring (Drehen), 7 Mitte (frei).
int gizmoGriff(const ModellAnsicht& a, const float m[3], ImVec2 maus) {
    const ModellTab& t = mt();
    const float L = laengeFuerPx(a, m, 70.0F * t.gizmoGroesse);
    ImVec2 sm;
    if (!aufSchirm(a, m, sm)) { return 0; }
    const float reich = ImGui::GetFontSize() * 0.45F;
    if (std::hypot(maus.x - sm.x, maus.y - sm.y) < reich * 1.2F) { return 7; }
    if (t.gizmo == 2) {
        int best = 0;
        float bestD = reich;
        for (int k = 0; k < 3; ++k) {
            ImVec2 vor;
            bool hat = false;
            for (int i = 0; i <= 48; ++i) {
                float p[3];
                ringPunkt(m, k, L * 0.8F, static_cast<float>(i) / 48.0F * 2.0F * kPi, p);
                ImVec2 s;
                if (!aufSchirm(a, p, s)) {
                    hat = false;
                    continue;
                }
                if (hat && abstandZuStrecke(maus, vor, s) < bestD) {
                    bestD = abstandZuStrecke(maus, vor, s);
                    best = 4 + k;
                }
                vor = s;
                hat = true;
            }
        }
        return best;
    }
    for (int k = 0; k < 3; ++k) {
        float e[3] = {m[0], m[1], m[2]};
        e[k] += L;
        ImVec2 se;
        if (aufSchirm(a, e, se) && abstandZuStrecke(maus, ImVec2{sm.x + (se.x - sm.x) * 0.2F, sm.y + (se.y - sm.y) * 0.2F}, se) < reich) {
            return k + 1;
        }
    }
    return 0;
}

void gizmoZeichnen(const ModellAnsicht& a, const float m[3]) {
    const ModellTab& t = mt();
    if (t.gizmo == 0) { return; }
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float L = laengeFuerPx(a, m, 70.0F * t.gizmoGroesse);
    const ImU32 farben[3] = {IM_COL32(230, 60, 60, 255), IM_COL32(60, 210, 60, 255), IM_COL32(70, 110, 250, 255)};
    const ImU32 gelb = IM_COL32(255, 230, 60, 255);
    ImVec2 sm;
    if (!aufSchirm(a, m, sm)) { return; }
    for (int k = 0; k < 3; ++k) {
        const bool fest = t.achse == k || t.griff == k + 1 || t.griff == k + 4;
        if (t.gizmo == 2) {
            ImVec2 vor;
            bool hat = false;
            for (int i = 0; i <= 48; ++i) {
                float p[3];
                ringPunkt(m, k, L * 0.8F, static_cast<float>(i) / 48.0F * 2.0F * kPi, p);
                ImVec2 s;
                if (!aufSchirm(a, p, s)) {
                    hat = false;
                    continue;
                }
                if (hat) { dl->AddLine(vor, s, fest ? gelb : farben[k], 2.0F); }
                vor = s;
                hat = true;
            }
            continue;
        }
        float e[3] = {m[0], m[1], m[2]};
        e[k] += L;
        ImVec2 se;
        if (!aufSchirm(a, e, se)) { continue; }
        dl->AddLine(sm, se, fest ? gelb : farben[k], 2.0F);
        if (t.gizmo == 1) {
            dl->AddCircleFilled(se, 4.0F, fest ? gelb : farben[k]);
        } else {
            dl->AddRectFilled(ImVec2{se.x - 4, se.y - 4}, ImVec2{se.x + 4, se.y + 4}, fest ? gelb : farben[k]);
        }
    }
    dl->AddRect(ImVec2{sm.x - 5, sm.y - 5}, ImVec2{sm.x + 5, sm.y + 5}, t.griff == 7 ? gelb : IM_COL32(230, 230, 230, 255));
}

// mb1a2: was eine Umformung trifft - im Objektmodus die gewaehlten Teile (alle LOD-Stufen), sonst die gewaehlten Ecken
// der gezeichneten Stufe. `fn` bekommt jede Ecke (aus `vorher` frisch kopiert).
void fuerAuswahl(modell::Inhalt& x, const modell::Inhalt& vorher, const std::function<void(modell::Ecke&)>& fn) {
    const ModellTab& t = mt();
    if (t.auswahlArt == AuswahlArt::Objekt) {
        for (const int i : t.auswahl) {
            if (i < 0 || static_cast<std::size_t>(i) >= vorher.teile.size()) { continue; }
            modell::Teil& ziel = x.teile[static_cast<std::size_t>(i)];
            ziel = vorher.teile[static_cast<std::size_t>(i)];
            for (modell::Netz& n : ziel.stufen) {
                for (modell::Ecke& e : n.ecken) { fn(e); }
            }
        }
        return;
    }
    const auto st = static_cast<std::size_t>(t.lodJetzt);
    for (const modell::EckOrt& o : t.eckenWahl) {
        if (o.teil < 0 || static_cast<std::size_t>(o.teil) >= vorher.teile.size() || st >= vorher.teile[static_cast<std::size_t>(o.teil)].stufen.size()) {
            continue;
        }
        const modell::Netz& nv = vorher.teile[static_cast<std::size_t>(o.teil)].stufen[st];
        modell::Netz& nz = x.teile[static_cast<std::size_t>(o.teil)].stufen[st];
        if (o.ecke < 0 || static_cast<std::size_t>(o.ecke) >= nv.ecken.size() || nz.ecken.size() != nv.ecken.size()) { continue; }
        modell::Ecke e = nv.ecken[static_cast<std::size_t>(o.ecke)];
        fn(e);
        nz.ecken[static_cast<std::size_t>(o.ecke)] = e;
    }
}

// Eine Drehung um eine beliebige Achse (Einheitsvektor, Grad) als Matrix (Rodrigues).
void drehMatrix(const float achse[3], float grad, float r[3][3]) {
    const float w = grad * kPi / 180.0F;
    const float c = std::cos(w);
    const float s1 = std::sin(w);
    const float t1 = 1.0F - c;
    const float x = achse[0];
    const float y = achse[1];
    const float z = achse[2];
    const float m[3][3] = {{t1 * x * x + c, t1 * x * y - s1 * z, t1 * x * z + s1 * y},
                           {t1 * x * y + s1 * z, t1 * y * y + c, t1 * y * z - s1 * x},
                           {t1 * x * z - s1 * y, t1 * y * z + s1 * x, t1 * z * z + c}};
    std::memcpy(r, m, sizeof m);
}

// Umformen um die Mitte m: art 1 verschieben (wert = Weg), 2 drehen (wert = Grad um X, Y, Z nacheinander), 3 skalieren
// (wert = Faktoren je Achse). Normalen drehen mit.
void umformen(modell::Inhalt& x, const modell::Inhalt& vorher, int art, const float m[3], const float wert[3]) {
    float rot[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    if (art == 2) {
        for (int k = 0; k < 3; ++k) {
            if (wert[k] == 0.0F) { continue; }
            float ax[3] = {0.0F, 0.0F, 0.0F};
            ax[k] = 1.0F;
            float r[3][3];
            drehMatrix(ax, wert[k], r);
            float neu[3][3];
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) { neu[i][j] = r[i][0] * rot[0][j] + r[i][1] * rot[1][j] + r[i][2] * rot[2][j]; }
            }
            std::memcpy(rot, neu, sizeof rot);
        }
    }
    fuerAuswahl(x, vorher, [&](modell::Ecke& e) {
        const float d[3] = {e.ort[0] - m[0], e.ort[1] - m[1], e.ort[2] - m[2]};
        if (art == 1) {
            for (int k = 0; k < 3; ++k) { e.ort[k] += wert[k]; }
        } else if (art == 2) {
            float nn[3];
            for (int k = 0; k < 3; ++k) {
                e.ort[k] = m[k] + rot[k][0] * d[0] + rot[k][1] * d[1] + rot[k][2] * d[2];
                nn[k] = rot[k][0] * e.normale[0] + rot[k][1] * e.normale[1] + rot[k][2] * e.normale[2];
            }
            std::memcpy(e.normale, nn, sizeof nn);
        } else if (art == 3) {
            for (int k = 0; k < 3; ++k) { e.ort[k] = m[k] + d[k] * wert[k]; }
        }
    });
}

// Waehrend des Ziehens: Griff, Mausweg -> Wert (Verschiebung in Einheiten,
// Winkel in Grad, Faktoren).
void gizmoZiehen(const ModellAnsicht& a, const float m[3]) {
    ModellTab& t = mt();
    const ImVec2 maus = ImGui::GetIO().MousePos;
    const float dx = maus.x - t.ziehStart[0];
    const float dy = maus.y - t.ziehStart[1];
    if (dx == 0.0F && dy == 0.0F && t.ziehVorher.empty()) { return; }
    float wert[3] = {0.0F, 0.0F, 0.0F};
    const float L = laengeFuerPx(a, m, 70.0F * t.gizmoGroesse);
    const float einheitProPx = L / (70.0F * t.gizmoGroesse);
    int achse = t.griff >= 1 && t.griff <= 3 ? t.griff - 1 : (t.griff >= 4 && t.griff <= 6 ? t.griff - 4 : -1);
    if (achse < 0 && t.achse >= 0 && t.achse <= 2) { achse = t.achse; }
    if (t.gizmo == 1) {
        if (achse >= 0) {
            float e[3] = {m[0], m[1], m[2]};
            e[achse] += L;
            ImVec2 s0;
            ImVec2 s1;
            if (aufSchirm(a, m, s0) && aufSchirm(a, e, s1)) {
                const float sx = s1.x - s0.x;
                const float sy = s1.y - s0.y;
                const float l2 = sx * sx + sy * sy;
                if (l2 > 1e-3F) { wert[achse] = (dx * sx + dy * sy) / l2 * L; }
            }
        } else {
            float r[3];
            float h[3];
            float v[3];
            ansichtAchsen(a.art, r, h, v);
            for (int k = 0; k < 3; ++k) { wert[k] = (dx * r[k] - dy * h[k]) * einheitProPx; }
            // F8-Ebenen: XY, YZ, ZX - die dritte Achse bleibt.
            if (t.achse >= 3) { wert[(t.achse - 3 + 2) % 3] = 0.0F; }
        }
    } else if (t.gizmo == 2) {
        wert[achse >= 0 ? achse : 2] = dx * 0.5F;
    } else if (t.gizmo == 3) {
        const float f = std::max(1.0F + dx * 0.01F, 0.01F);
        for (int k = 0; k < 3; ++k) { wert[k] = (achse < 0 || achse == k) ? f : 1.0F; }
    }
    if (t.bereich == ModellBereich::Animation && t.griffWahl >= 0) {   // Stufe 3: der Griff (FK), nicht das Netz
        if (t.gizmo == 1 || t.gizmo == 2) { modellRigZiehen(t.gizmo, wert); }
        return;
    }
    static const Str kNamen[4] = {Str::MbUndoBewegen, Str::MbUndoBewegen, Str::MbUndoDrehen, Str::MbUndoSkalieren};
    if (t.ziehVorher.empty()) {
        t.dok.aendern(tr(kNamen[t.gizmo]), [](modell::Inhalt&) {});
        t.ziehVorher.push_back(t.dok.inhalt());
        std::memcpy(t.ziehWeg, m, sizeof t.ziehWeg);   // Drehpunkt: die Mitte beim Anfassen
    }
    const int art = t.gizmo;
    const float mitte[3] = {t.ziehWeg[0], t.ziehWeg[1], t.ziehWeg[2]};
    t.dok.vorschau([&](modell::Inhalt& x) { umformen(x, t.ziehVorher.front(), art, mitte, wert); });
    g_app->modelDirty = true;
}

// --- Auswaehlen ------------------------------------------------------------------------------

// Die Flaeche unter der Maus (vorderstes Dreieck), -1 keine.
int flaecheUnter(const ModellAnsicht& a, ImVec2 maus) {
    const ModellTab& t = mt();
    const auto& h = haut();
    int best = -1;
    float bestT = 2.0F;
    for (std::size_t i = 0; i < h.size() && i < t.anzeige.surfaces.size(); ++i) {
        if (!gezeichnet(i)) { continue; }
        const GlmSurface& s = t.anzeige.surfaces[i];
        for (std::size_t k = 0; k + 2 < s.indexes.size(); k += 3) {
            ImVec2 p[3];
            float d[3];
            bool ok = true;
            for (std::size_t e = 0; e < 3 && ok; ++e) { ok = aufSchirm(a, h[i][s.indexes[k + e]].data(), p[e], &d[e]); }
            if (!ok) { continue; }
            const float flaeche = (p[1].x - p[0].x) * (p[2].y - p[0].y) - (p[2].x - p[0].x) * (p[1].y - p[0].y);
            if (std::fabs(flaeche) < 1e-6F) { continue; }
            const float u = ((p[1].x - maus.x) * (p[2].y - maus.y) - (p[2].x - maus.x) * (p[1].y - maus.y)) / flaeche;
            const float v = ((p[2].x - maus.x) * (p[0].y - maus.y) - (p[0].x - maus.x) * (p[2].y - maus.y)) / flaeche;
            const float w = 1.0F - u - v;
            if (u < 0.0F || v < 0.0F || w < 0.0F) { continue; }
            const float tiefe = u * d[0] + v * d[1] + w * d[2];
            if (tiefe < bestT) {
                bestT = tiefe;
                best = static_cast<int>(i);
            }
        }
    }
    return best;
}

void auswahlAendern(const std::vector<int>& treffer) {
    ModellTab& t = mt();
    const ImGuiIO& io = ImGui::GetIO();
    if (!io.KeyCtrl && !io.KeyAlt) { t.auswahl.clear(); }
    for (const int f : treffer) {
        const auto it = std::find(t.auswahl.begin(), t.auswahl.end(), f);
        if (io.KeyAlt) {
            if (it != t.auswahl.end()) { t.auswahl.erase(it); }
        } else if (it == t.auswahl.end()) {
            t.auswahl.push_back(f);
        }
    }
    t.tagWahl = -1;
}

// Rahmen: jede gezeichnete Flaeche mit einer Ecke im Rahmen (Max: Kreuzen).
std::vector<int> flaechenImRahmen(const ModellAnsicht& a, ImVec2 p, ImVec2 q) {
    const ModellTab& t = mt();
    const auto& h = haut();
    const ImVec2 mn{std::min(p.x, q.x), std::min(p.y, q.y)};
    const ImVec2 mx{std::max(p.x, q.x), std::max(p.y, q.y)};
    std::vector<int> out;
    for (std::size_t i = 0; i < h.size() && i < t.anzeige.surfaces.size(); ++i) {
        if (!gezeichnet(i)) { continue; }
        for (const auto& e : h[i]) {
            ImVec2 s;
            if (aufSchirm(a, e.data(), s) && s.x >= mn.x && s.x <= mx.x && s.y >= mn.y && s.y <= mx.y) {
                out.push_back(static_cast<int>(i));
                break;
            }
        }
    }
    return out;
}

// --- mb1a2: Unterobjekte (Ecke, Kante, Dreieck, Rand, Element) -------------------------------------------

// Das vorderste Dreieck unter der Maus: Teil und Dreieck, false wenn keins.
bool dreieckUnter(const ModellAnsicht& a, ImVec2 maus, int& teil, int& dreieck) {
    const ModellTab& t = mt();
    const auto& h = haut();
    float bestT = 2.0F;
    teil = -1;
    for (std::size_t i = 0; i < h.size() && i < t.anzeige.surfaces.size(); ++i) {
        if (!gezeichnet(i)) { continue; }
        const GlmSurface& s = t.anzeige.surfaces[i];
        for (std::size_t k = 0; k + 2 < s.indexes.size(); k += 3) {
            ImVec2 p[3];
            float d[3];
            bool ok = true;
            for (std::size_t e = 0; e < 3 && ok; ++e) { ok = aufSchirm(a, h[i][s.indexes[k + e]].data(), p[e], &d[e]); }
            if (!ok) { continue; }
            const float f = (p[1].x - p[0].x) * (p[2].y - p[0].y) - (p[2].x - p[0].x) * (p[1].y - p[0].y);
            if (std::fabs(f) < 1e-6F) { continue; }
            const float u = ((p[1].x - maus.x) * (p[2].y - maus.y) - (p[2].x - maus.x) * (p[1].y - maus.y)) / f;
            const float v = ((p[2].x - maus.x) * (p[0].y - maus.y) - (p[0].x - maus.x) * (p[2].y - maus.y)) / f;
            const float w = 1.0F - u - v;
            if (u < 0.0F || v < 0.0F || w < 0.0F) { continue; }
            const float tiefe = u * d[0] + v * d[1] + w * d[2];
            if (tiefe < bestT) {
                bestT = tiefe;
                teil = static_cast<int>(i);
                dreieck = static_cast<int>(k / 3);
            }
        }
    }
    return teil >= 0;
}

const modell::Netz* netzVon(int teil) {
    const ModellTab& t = mt();
    const modell::Inhalt& in = t.dok.inhalt();
    if (teil < 0 || static_cast<std::size_t>(teil) >= in.teile.size()) { return nullptr; }
    const auto st = static_cast<std::size_t>(t.lodJetzt);
    return st < in.teile[static_cast<std::size_t>(teil)].stufen.size() ? &in.teile[static_cast<std::size_t>(teil)].stufen[st] : nullptr;
}

// Was ein Klick in einer Unterobjekt-Art trifft: die Ecken (Teil, Ecke).
std::vector<modell::EckOrt> eckenUnter(const ModellAnsicht& a, ImVec2 maus) {
    const ModellTab& t = mt();
    std::vector<modell::EckOrt> out;
    int teil = -1;
    int dr = -1;
    if (!dreieckUnter(a, maus, teil, dr)) { return out; }
    const modell::Netz* n = netzVon(teil);
    if (n == nullptr || static_cast<std::size_t>(dr) * 3U + 2U >= n->kanten.size()) { return out; }
    const auto& h = haut()[static_cast<std::size_t>(teil)];
    const auto schirm = [&](int ecke) {
        ImVec2 p{-1e9F, -1e9F};
        (void)aufSchirm(a, h[static_cast<std::size_t>(ecke)].data(), p);
        return p;
    };
    const int k0 = dr * 3;
    const auto eckeVon = [&](int k) { return n->kanten[static_cast<std::size_t>(k)].ecke; };
    switch (t.auswahlArt) {
        case AuswahlArt::Ecke: {   // die naechste Ecke des getroffenen Dreiecks
            int best = eckeVon(k0);
            float bd = 1e30F;
            for (int k = k0; k < k0 + 3; ++k) {
                const ImVec2 p = schirm(eckeVon(k));
                const float d = std::hypot(p.x - maus.x, p.y - maus.y);
                if (d < bd) {
                    bd = d;
                    best = eckeVon(k);
                }
            }
            out.push_back(modell::EckOrt{teil, best});
            break;
        }
        case AuswahlArt::Kante:
        case AuswahlArt::Rand: {   // die naechste Kante; Rand: die offene Kantenkette daran (3ds Max "Rand")
            int best = k0;
            float bd = 1e30F;
            for (int k = k0; k < k0 + 3; ++k) {
                const float d = abstandZuStrecke(maus, schirm(eckeVon(k)), schirm(eckeVon(n->kanten[static_cast<std::size_t>(k)].naechste)));
                if (d < bd) {
                    bd = d;
                    best = k;
                }
            }
            if (t.auswahlArt == AuswahlArt::Kante || n->kanten[static_cast<std::size_t>(best)].gegen >= 0) {
                out.push_back(modell::EckOrt{teil, eckeVon(best)});
                out.push_back(modell::EckOrt{teil, eckeVon(n->kanten[static_cast<std::size_t>(best)].naechste)});
                break;
            }
            // alle Randkanten, die ueber gemeinsame Punkte an der getroffenen haengen
            std::vector<char> punktDrin(static_cast<std::size_t>(n->punktZahl), 0);
            std::vector<int> rand;
            for (std::size_t k = 0; k < n->kanten.size(); ++k) {
                if (n->kanten[k].gegen < 0) { rand.push_back(static_cast<int>(k)); }
            }
            const auto pk = [&](int k) { return n->kanten[static_cast<std::size_t>(k)].punkt; };
            const auto pn = [&](int k) { return n->kanten[static_cast<std::size_t>(n->kanten[static_cast<std::size_t>(k)].naechste)].punkt; };
            punktDrin[static_cast<std::size_t>(pk(best))] = 1;
            punktDrin[static_cast<std::size_t>(pn(best))] = 1;
            bool weiter = true;
            while (weiter) {
                weiter = false;
                for (const int k : rand) {
                    const bool a1 = punktDrin[static_cast<std::size_t>(pk(k))] != 0;
                    const bool b1 = punktDrin[static_cast<std::size_t>(pn(k))] != 0;
                    if (a1 != b1) {
                        punktDrin[static_cast<std::size_t>(pk(k))] = 1;
                        punktDrin[static_cast<std::size_t>(pn(k))] = 1;
                        weiter = true;
                    }
                }
            }
            for (std::size_t e = 0; e < n->ecken.size(); ++e) {
                if (punktDrin[static_cast<std::size_t>(n->punktVon[e])] != 0) { out.push_back(modell::EckOrt{teil, static_cast<int>(e)}); }
            }
            break;
        }
        case AuswahlArt::Dreieck:
            for (int k = k0; k < k0 + 3; ++k) { out.push_back(modell::EckOrt{teil, eckeVon(k)}); }
            break;
        case AuswahlArt::Element: {   // 3ds Max "Element": alle Dreiecke, die ueber Kanten zusammenhaengen
            std::vector<char> drin(static_cast<std::size_t>(n->dreiecke()), 0);
            std::vector<int> offen = {dr};
            drin[static_cast<std::size_t>(dr)] = 1;
            while (!offen.empty()) {
                const int d0 = offen.back();
                offen.pop_back();
                for (int k = d0 * 3; k < d0 * 3 + 3; ++k) {
                    const int g = n->kanten[static_cast<std::size_t>(k)].gegen;
                    if (g >= 0 && drin[static_cast<std::size_t>(g / 3)] == 0) {
                        drin[static_cast<std::size_t>(g / 3)] = 1;
                        offen.push_back(g / 3);
                    }
                }
            }
            std::vector<char> ecke(n->ecken.size(), 0);
            for (std::size_t d0 = 0; d0 < drin.size(); ++d0) {
                if (drin[d0] == 0) { continue; }
                for (std::size_t k = d0 * 3; k < d0 * 3 + 3; ++k) { ecke[static_cast<std::size_t>(n->kanten[k].ecke)] = 1; }
            }
            for (std::size_t e = 0; e < ecke.size(); ++e) {
                if (ecke[e] != 0) { out.push_back(modell::EckOrt{teil, static_cast<int>(e)}); }
            }
            break;
        }
        case AuswahlArt::Objekt: break;
    }
    return out;
}

std::vector<modell::EckOrt> eckenImRahmen(const ModellAnsicht& a, ImVec2 p, ImVec2 q) {
    const ModellTab& t = mt();
    const auto& h = haut();
    const ImVec2 mn{std::min(p.x, q.x), std::min(p.y, q.y)};
    const ImVec2 mx{std::max(p.x, q.x), std::max(p.y, q.y)};
    std::vector<modell::EckOrt> out;
    for (std::size_t i = 0; i < h.size() && i < t.anzeige.surfaces.size(); ++i) {
        if (!gezeichnet(i)) { continue; }
        for (std::size_t e = 0; e < h[i].size(); ++e) {
            ImVec2 sp;
            if (aufSchirm(a, h[i][e].data(), sp) && sp.x >= mn.x && sp.x <= mx.x && sp.y >= mn.y && sp.y <= mx.y) {
                out.push_back(modell::EckOrt{static_cast<int>(i), static_cast<int>(e)});
            }
        }
    }
    return out;
}

// Max: Strg dazu, Alt weg; Blender: Umschalt dazu (Umschalt auf schon Gewaehltes nimmt es heraus).
void eckenWahlAendern(const std::vector<modell::EckOrt>& treffer) {
    ModellTab& t = mt();
    const ImGuiIO& io = ImGui::GetIO();
    const bool blender = g_app->settings.modellBedienung == 1;
    const bool dazu = blender ? io.KeyShift : io.KeyCtrl;
    const bool weg = !blender && io.KeyAlt;
    const auto gleich = [](const modell::EckOrt& x, const modell::EckOrt& y) { return x.teil == y.teil && x.ecke == y.ecke; };
    const auto drin = [&](const modell::EckOrt& o) {
        return std::any_of(t.eckenWahl.begin(), t.eckenWahl.end(), [&](const modell::EckOrt& w) { return gleich(o, w); });
    };
    const bool alleDrin = blender && dazu && !treffer.empty() && std::all_of(treffer.begin(), treffer.end(), drin);
    if (!dazu && !weg) { t.eckenWahl.clear(); }
    for (const modell::EckOrt& o : treffer) {
        const auto it = std::find_if(t.eckenWahl.begin(), t.eckenWahl.end(), [&](const modell::EckOrt& w) { return gleich(o, w); });
        if (weg || alleDrin) {
            if (it != t.eckenWahl.end()) { t.eckenWahl.erase(it); }
        } else if (it == t.eckenWahl.end()) {
            t.eckenWahl.push_back(o);
        }
    }
}

// --- mb1a2: Weights - Farbe je Knochen und der Pinsel --------------------------------------------------

// Blender weight_to_rgb: blau (0) - tuerkis - gruen - gelb - rot (1), je ein Viertel linear.
void gewichtFarbe(float w, unsigned char rgb[3]) {
    w = std::clamp(w, 0.0F, 1.0F);
    float r = 0.0F;
    float g = 0.0F;
    float b = 0.0F;
    if (w <= 0.25F) {
        g = w * 4.0F;
        b = 1.0F;
    } else if (w <= 0.5F) {
        g = 1.0F;
        b = 1.0F - (w - 0.25F) * 4.0F;
    } else if (w <= 0.75F) {
        r = (w - 0.5F) * 4.0F;
        g = 1.0F;
    } else {
        r = 1.0F;
        g = 1.0F - (w - 0.75F) * 4.0F;
    }
    rgb[0] = static_cast<unsigned char>(std::lround(r * 255.0F));
    rgb[1] = static_cast<unsigned char>(std::lround(g * 255.0F));
    rgb[2] = static_cast<unsigned char>(std::lround(b * 255.0F));
}

// Die Figur zum Malen: dieselben Ecken (Pose wie im Spiel), als Bild eine Farbleiste; die UV jeder Ecke zeigt auf ihr
// Gewicht - die Grafikkarte mischt das Gewicht ueber das Dreieck und schlaegt die Farbe nach.
GlmModel g_gewModell;
ModelTextures g_gewTex;
std::string g_gewSchluessel;

bool gewichtsAnsicht() {
    const ModellTab& t = mt();
    return t.bereich == ModellBereich::Weights && t.knochenWahl >= 0;
}

const GlmModel& gewichtsModell() {
    const ModellTab& t = mt();
    const std::string schluessel = std::to_string(t.neuGebaut) + "|" + std::to_string(t.knochenWahl) + "|" + std::to_string(t.lodJetzt);
    if (schluessel == g_gewSchluessel) { return g_gewModell; }
    // Die Farbleiste liegt in der mittleren Haelfte des Bildes (links und rechts davon die Endfarben): an den Raendern
    // mischt der Sampler (WRAP) sonst Blau und Rot. Die Stufen der Bildpyramide werden NICHT gemittelt, sondern an der
    // Mitte jedes Bildpunkts neu abgetastet - gemittelt naehme die Farbe bei kleinen Dreiecken (grosser Weg der UV je
    // Bildpunkt) den Mittelwert der ganzen Leiste an, und ein Gewicht 1 sah gruen aus (mb2_g4_f, an der Ecke 0/213/91).
    const auto stufe = [](int breite, std::vector<std::uint8_t>& rgba) {
        rgba.assign(static_cast<std::size_t>(breite) * 4U, 255);
        for (int i = 0; i < breite; ++i) {
            const float u = (static_cast<float>(i) + 0.5F) / static_cast<float>(breite);
            gewichtFarbe(std::clamp((u - 0.25F) * 2.0F, 0.0F, 1.0F), &rgba[static_cast<std::size_t>(i) * 4U]);
        }
    };
    TextureSet::Tex rampe;
    rampe.width = 512;
    rampe.height = 1;
    stufe(rampe.width, rampe.rgba);
    for (int b = rampe.width / 2; b >= 1; b /= 2) {
        TextureSet::Tex::Mip m;
        m.width = b;
        m.height = 1;
        stufe(b, m.rgba);
        rampe.mips.push_back(std::move(m));
    }
    GlmModel m = t.anzeige;
    ModelTextures tex;
    tex.bySurface.resize(m.surfaces.size());
    for (std::size_t i = 0; i < m.surfaces.size(); ++i) {
        GlmSurface& s = m.surfaces[i];
        s.shader.clear();
        s.texture = "*gewicht";
        const modell::Netz* n = netzVon(static_cast<int>(i));
        for (std::size_t k = 0; k < s.verts.size(); ++k) {
            const float w = n != nullptr ? modell::gewichtVon(*n, static_cast<int>(k), t.knochenWahl) : 0.0F;
            s.verts[k].st[0] = 0.25F + std::clamp(w, 0.0F, 1.0F) * 0.5F;   // die mittlere Haelfte der Leiste
            s.verts[k].st[1] = 0.5F;
        }
        tex.bySurface[i] = rampe;
        ++tex.found;
    }
    g_gewModell = std::move(m);
    g_gewTex = std::move(tex);
    g_gewSchluessel = schluessel;
    return g_gewModell;
}

// Zeigt die Ecke zur Kamera? Die Normale gedreht wie die Ecke (Knochen 0, wie die Normalen der Ueberlagerung).
bool zurKamera(const ModellAnsicht& a, const GlmVertex& v, const float p[3]) {
    float nrm[3] = {v.normal[0], v.normal[1], v.normal[2]};
    if (!g_knochen.empty() && v.bones[0] < g_knochen.size()) {
        const BoneMatrix& m = g_knochen[v.bones[0]];
        const float n0[3] = {nrm[0], nrm[1], nrm[2]};
        for (int r = 0; r < 3; ++r) { nrm[r] = m.m[r][0] * n0[0] + m.m[r][1] * n0[1] + m.m[r][2] * n0[2]; }
    }
    float zu[3];
    if (a.art == AnsichtArt::Perspektive) {
        for (int k = 0; k < 3; ++k) { zu[k] = g_app->modelCam.pos[k] - p[k]; }
    } else {
        float r[3];
        float h[3];
        float vv[3];
        ansichtAchsen(a.art, r, h, vv);
        for (int k = 0; k < 3; ++k) { zu[k] = -vv[k]; }
    }
    return nrm[0] * zu[0] + nrm[1] * zu[1] + nrm[2] * zu[2] > 0.0F;
}

std::vector<std::string> knochenNamen() {
    std::vector<std::string> namen;
    for (const GlaBone& b : g_app->anim.bones) { namen.push_back(b.name); }
    return namen;
}

}  // namespace

bool modellTupfer(int ansicht, ImVec2 maus, bool neuerStrich) {
    ModellTab& t = mt();
    if (ansicht < 0 || ansicht >= 4 || t.knochenWahl < 0) { return false; }
    const ModellAnsicht& a = t.ansichten[ansicht];
    const auto& h = haut();
    std::vector<std::vector<modell::Tupfer>> je(t.anzeige.surfaces.size());
    int zahl = 0;
    for (std::size_t i = 0; i < h.size() && i < t.anzeige.surfaces.size(); ++i) {
        if (!gezeichnet(i)) { continue; }
        for (std::size_t k = 0; k < h[i].size(); ++k) {
            ImVec2 sp;
            if (!aufSchirm(a, h[i][k].data(), sp)) { continue; }
            const float f = modell::abfall(std::hypot(sp.x - maus.x, sp.y - maus.y), t.pinselRadius);
            if (f <= 0.0F || !zurKamera(a, t.anzeige.surfaces[i].verts[k], h[i][k].data())) { continue; }
            je[i].push_back(modell::Tupfer{static_cast<int>(k), f * t.pinselStaerke});
            ++zahl;
        }
    }
    t.gemalteEcken = zahl;
    if (neuerStrich) {
        t.dok.aendern(tr(Str::MbUndoMalen), [](modell::Inhalt&) {});
        t.malStrich = true;
        if (t.pinselSpiegeln) {
            t.spiegel = modell::spiegelKarte(t.dok.inhalt(), t.lodJetzt, kSpiegelToleranz);
            t.spiegelStand = t.dok.stand();
        }
    }
    t.letzterTupfer[0] = maus.x;
    t.letzterTupfer[1] = maus.y;
    if (zahl == 0) { return false; }
    const int spiegelK = modell::spiegelKnochen(knochenNamen(), t.knochenWahl);
    const int knochen = t.knochenWahl;
    const auto art = static_cast<modell::PinselArt>(t.pinselArt);
    const float wert = t.pinselGewicht;
    const bool spiegeln = t.pinselSpiegeln;
    const auto& karte = t.spiegel;
    const int stufe = t.lodJetzt;
    t.dok.vorschau([&](modell::Inhalt& x) {
        std::vector<std::vector<modell::Tupfer>> gespiegelt(x.teile.size());
        for (std::size_t i = 0; i < je.size() && i < x.teile.size(); ++i) {
            if (je[i].empty() || static_cast<std::size_t>(stufe) >= x.teile[i].stufen.size()) { continue; }
            (void)modell::malen(x.teile[i].stufen[static_cast<std::size_t>(stufe)], je[i], knochen, art, wert);
            if (!spiegeln || i >= karte.size()) { continue; }
            for (const modell::Tupfer& tu : je[i]) {
                if (static_cast<std::size_t>(tu.ecke) >= karte[i].size()) { continue; }
                const modell::EckOrt p = karte[i][static_cast<std::size_t>(tu.ecke)];
                if (p.teil >= 0 && static_cast<std::size_t>(p.teil) < gespiegelt.size()) {
                    gespiegelt[static_cast<std::size_t>(p.teil)].push_back(modell::Tupfer{p.ecke, tu.staerke});
                }
            }
        }
        for (std::size_t i = 0; i < gespiegelt.size(); ++i) {
            if (gespiegelt[i].empty() || static_cast<std::size_t>(stufe) >= x.teile[i].stufen.size()) { continue; }
            (void)modell::malen(x.teile[i].stufen[static_cast<std::size_t>(stufe)], gespiegelt[i], spiegelK >= 0 ? spiegelK : knochen, art, wert);
        }
    });
    ++t.tupferZahl;
    g_app->modelDirty = true;
    return true;
}

void modellStrichEnde() { mt().malStrich = false; }

bool modellUebernehmen() {
    ModellTab& t = mt();
    int n = 0;
    t.dok.aendern(tr(Str::MbUndoUebernehmen), [&](modell::Inhalt& x) { n = modell::uebernehmen(x); });
    char z[200];
    std::snprintf(z, sizeof z, tr(Str::MbGwUebernommen), n);
    t.meldung = z;
    g_app->modelDirty = true;
    return n > 0;
}

int modellAufAuswahlSetzen() {
    ModellTab& t = mt();
    if (t.knochenWahl < 0) { return 0; }
    std::vector<std::vector<modell::Tupfer>> je(t.dok.inhalt().teile.size());
    if (t.auswahlArt == AuswahlArt::Objekt) {
        for (const int i : t.auswahl) {
            const modell::Netz* n = netzVon(i);
            if (n == nullptr) { continue; }
            for (std::size_t k = 0; k < n->ecken.size(); ++k) { je[static_cast<std::size_t>(i)].push_back(modell::Tupfer{static_cast<int>(k), 1.0F}); }
        }
    } else {
        for (const modell::EckOrt& o : t.eckenWahl) {
            if (o.teil >= 0 && static_cast<std::size_t>(o.teil) < je.size()) { je[static_cast<std::size_t>(o.teil)].push_back(modell::Tupfer{o.ecke, 1.0F}); }
        }
    }
    int z = 0;
    const int stufe = t.lodJetzt;
    const int knochen = t.knochenWahl;
    const float wert = t.pinselGewicht;
    t.dok.aendern(tr(Str::MbUndoSetzen), [&](modell::Inhalt& x) {
        for (std::size_t i = 0; i < je.size(); ++i) {
            if (je[i].empty() || static_cast<std::size_t>(stufe) >= x.teile[i].stufen.size()) { continue; }
            z += modell::malen(x.teile[i].stufen[static_cast<std::size_t>(stufe)], je[i], knochen, modell::PinselArt::Setzen, wert);
        }
    });
    g_app->modelDirty = true;
    return z;
}

int modellSpiegeln(int quelle) {
    ModellTab& t = mt();
    int n = 0;
    const std::vector<std::string> namen = knochenNamen();
    t.dok.aendern(tr(Str::MbUndoSpiegeln), [&](modell::Inhalt& x) { n = modell::spiegeln(x, t.lodJetzt, namen, quelle, kSpiegelToleranz); });
    char z[200];
    std::snprintf(z, sizeof z, tr(Str::MbGwGespiegelt), n);
    t.meldung = z;
    g_app->modelDirty = true;
    return n;
}

void modellAuswahlArtSetzen(AuswahlArt a) {
    ModellTab& t = mt();
    if (a != AuswahlArt::Objekt) { t.editArt = a; }
    if (t.auswahlArt != a) { t.eckenWahl.clear(); }
    t.auswahlArt = a;
    g_app->modelDirty = true;
}

bool modellGewichtFarbe(int flaeche, int ecke, float& gewicht) {
    const ModellTab& t = mt();
    const modell::Netz* n = netzVon(flaeche);
    if (n == nullptr || t.knochenWahl < 0 || ecke < 0 || static_cast<std::size_t>(ecke) >= n->ecken.size()) { return false; }
    gewicht = modell::gewichtVon(*n, ecke, t.knochenWahl);
    return true;
}

bool modellEckeAufSchirm(int flaeche, int ecke, ImVec2& out) {
    const ModellTab& t = mt();
    const auto& h = haut();
    if (flaeche < 0 || static_cast<std::size_t>(flaeche) >= h.size() || ecke < 0 ||
        static_cast<std::size_t>(ecke) >= h[static_cast<std::size_t>(flaeche)].size()) {
        return false;
    }
    return aufSchirm(t.ansichten[t.aktiv], h[static_cast<std::size_t>(flaeche)][static_cast<std::size_t>(ecke)].data(), out);
}

namespace {

// --- mb1a2: G/R/S wie Blender (transform.translate/rotate/resize, modal) ------------------------------------

float modalZahl(bool& da) {
    const std::string& z = mt().modal.zahl;
    da = !z.empty() && z != "-" && z != "." && z != "-.";
    return da ? static_cast<float>(std::atof(z.c_str())) : 0.0F;
}

void fuerAuswahlDrehen(const modell::Inhalt& vorher, const float rot[3][3], const float m[3]) {
    mt().dok.vorschau([&](modell::Inhalt& x) {
        fuerAuswahl(x, vorher, [&](modell::Ecke& e) {
            const float d[3] = {e.ort[0] - m[0], e.ort[1] - m[1], e.ort[2] - m[2]};
            float nn[3];
            for (int k = 0; k < 3; ++k) {
                e.ort[k] = m[k] + rot[k][0] * d[0] + rot[k][1] * d[1] + rot[k][2] * d[2];
                nn[k] = rot[k][0] * e.normale[0] + rot[k][1] * e.normale[1] + rot[k][2] * e.normale[2];
            }
            std::memcpy(e.normale, nn, sizeof nn);
        });
    });
}

// Jedes Bild waehrend G/R/S: aus Mausweg, Achse und Zahl die Umformung des Zustands beim Start.
void modalAnwenden() {
    ModellTab& t = mt();
    ModellModal& m = t.modal;
    if (m.art == 0 || t.ziehVorher.empty()) { return; }
    const ModellAnsicht& a = t.ansichten[std::clamp(m.ansicht, 0, 3)];
    const ImVec2 maus = ImGui::GetIO().MousePos;
    bool hatZahl = false;
    const float zahl = modalZahl(hatZahl);
    ImVec2 sm;
    (void)aufSchirm(a, m.mitte, sm);
    float r[3];
    float h[3];
    float v[3];
    ansichtAchsen(a.art, r, h, v);
    const modell::Inhalt& vorher = t.ziehVorher.front();
    if (m.art == 1) {
        float weg[3] = {0.0F, 0.0F, 0.0F};
        if (hatZahl) {
            weg[m.achse >= 0 ? m.achse : 0] = zahl;   // ohne Achse: die Zahl gilt X (Blender)
        } else {
            const float proPx = laengeFuerPx(a, m.mitte, 100.0F) / 100.0F;
            const float dx = maus.x - m.mausStart[0];
            const float dy = maus.y - m.mausStart[1];
            for (int k = 0; k < 3; ++k) { weg[k] = (dx * r[k] - dy * h[k]) * proPx; }
            if (m.achse >= 0 && !m.ebene) {
                const float w = weg[m.achse];
                weg[0] = weg[1] = weg[2] = 0.0F;
                weg[m.achse] = w;
                // entlang der Achse: Mausweg auf die Achse auf dem Schirm projiziert
                float e[3] = {m.mitte[0], m.mitte[1], m.mitte[2]};
                const float L = laengeFuerPx(a, m.mitte, 100.0F);
                e[m.achse] += L;
                ImVec2 se;
                if (aufSchirm(a, e, se)) {
                    const float sx = se.x - sm.x;
                    const float sy = se.y - sm.y;
                    const float l2 = sx * sx + sy * sy;
                    if (l2 > 1e-3F) { weg[m.achse] = (dx * sx + dy * sy) / l2 * L; }
                }
            } else if (m.achse >= 0 && m.ebene) {
                weg[m.achse] = 0.0F;
            }
        }
        t.dok.vorschau([&](modell::Inhalt& x) { umformen(x, vorher, 1, m.mitte, weg); });
    } else if (m.art == 2) {
        float grad = 0.0F;
        if (hatZahl) {
            grad = zahl;
        } else {
            const float w0 = std::atan2(m.mausStart[1] - sm.y, m.mausStart[0] - sm.x);
            const float w1 = std::atan2(maus.y - sm.y, maus.x - sm.x);
            grad = -(w1 - w0) * 180.0F / kPi;   // gegen den Uhrzeigersinn auf dem Schirm = positiv um die Blickachse zum Betrachter
        }
        float achse[3] = {-v[0], -v[1], -v[2]};
        if (m.achse >= 0) {
            achse[0] = achse[1] = achse[2] = 0.0F;
            achse[m.achse] = 1.0F;
        }
        float rot[3][3];
        drehMatrix(achse, grad, rot);
        fuerAuswahlDrehen(vorher, rot, m.mitte);
    } else {
        float f = 1.0F;
        if (hatZahl) {
            f = zahl;
        } else {
            const float d0 = std::max(std::hypot(m.mausStart[0] - sm.x, m.mausStart[1] - sm.y), 1.0F);
            f = std::hypot(maus.x - sm.x, maus.y - sm.y) / d0;
        }
        float fak[3] = {f, f, f};
        if (m.achse >= 0) {
            for (int k = 0; k < 3; ++k) { fak[k] = (k == m.achse) != m.ebene ? f : 1.0F; }
        }
        t.dok.vorschau([&](modell::Inhalt& x) { umformen(x, vorher, 3, m.mitte, fak); });
    }
    g_app->modelDirty = true;
}

}  // namespace

bool modellModalStarten(int art) {
    ModellTab& t = mt();
    float m[3];
    if (!auswahlMitte(m)) {
        t.meldung = tr(Str::MbModalLeer);
        return false;
    }
    static const Str kNamen[4] = {Str::MbUndoBewegen, Str::MbUndoBewegen, Str::MbUndoDrehen, Str::MbUndoSkalieren};
    t.modal = ModellModal{};
    t.modal.art = art;
    t.modal.ansicht = t.aktiv;
    std::memcpy(t.modal.mitte, m, sizeof m);
    t.modal.mausStart[0] = ImGui::GetIO().MousePos.x;
    t.modal.mausStart[1] = ImGui::GetIO().MousePos.y;
    t.dok.aendern(tr(kNamen[std::clamp(art, 0, 3)]), [](modell::Inhalt&) {});
    t.ziehVorher.clear();
    t.ziehVorher.push_back(t.dok.inhalt());
    return true;
}

void modellModalEnde(bool uebernehmen) {
    ModellTab& t = mt();
    if (t.modal.art == 0) { return; }
    if (!uebernehmen) { (void)t.dok.abbrechen(); }   // Rechtsklick/Esc: wie vorher, ohne Schritt
    t.modal = ModellModal{};
    t.ziehVorher.clear();
    g_app->modelDirty = true;
}

// Die Tasten waehrend G/R/S (Blender "Transform Modal Map"): X/Y/Z sperren (zweimal lokal, dreimal aus), Umschalt+X/Y/Z
// die Ebene ohne die Achse, Ziffern/Komma/Minus/Ruecktaste fuer die Zahl, Enter/Linksklick fertig, Esc/Rechtsklick
// zurueck.
bool modellModalTasten() {
    ModellTab& t = mt();
    ModellModal& m = t.modal;
    if (m.art == 0) { return false; }
    const ImGuiIO& io = ImGui::GetIO();
    static const ImGuiKey kAchsen[3] = {ImGuiKey_X, ImGuiKey_Y, ImGuiKey_Z};
    for (int k = 0; k < 3; ++k) {
        if (!ImGui::IsKeyPressed(kAchsen[k], false)) { continue; }
        const bool ebene = io.KeyShift;
        if (m.achse == k && m.ebene == ebene) {
            m.achsStufe = m.achsStufe == 1 ? 2 : 0;   // global -> lokal -> aus
            if (m.achsStufe == 0) { m.achse = -1; }
        } else {
            m.achse = k;
            m.ebene = ebene;
            m.achsStufe = 1;
        }
    }
    for (int z = 0; z <= 9; ++z) {
        if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(ImGuiKey_0 + z), false) || ImGui::IsKeyPressed(static_cast<ImGuiKey>(ImGuiKey_Keypad0 + z), false)) {
            m.zahl += static_cast<char>('0' + z);
        }
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Period, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadDecimal, false)) { m.zahl += '.'; }
    if (ImGui::IsKeyPressed(ImGuiKey_Minus, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadSubtract, false)) {
        m.zahl = !m.zahl.empty() && m.zahl[0] == '-' ? m.zahl.substr(1) : "-" + m.zahl;   // Minus kehrt das Vorzeichen um
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Backspace, false) && !m.zahl.empty()) { m.zahl.pop_back(); }
    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false) || ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        modellModalEnde(false);
        return true;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false) ||
        ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        modalAnwenden();
        modellModalEnde(true);
        return true;
    }
    return true;
}

namespace {

// --- Die Ansicht ------------------------------------------------------------------------------

void einpassen(ModellAnsicht& a) {
    float mn[3];
    float mx[3];
    // Zoom Grenzen wie 3ds Max: die gewaehlten Flaechen, sonst alle gezeichneten -
    // in der Pose, der Blickwinkel bleibt.
    const bool auswahl = !mt().auswahl.empty() && grenzen(true, mn, mx);
    if (!auswahl && !grenzen(false, mn, mx)) {
        // ohne gezeichnete Flaeche: die Grenzen der Datei
        for (int k = 0; k < 3; ++k) {
            mn[k] = g_app->model.mins[k];
            mx[k] = g_app->model.maxs[k];
        }
    }
    float groesse = 0.0F;
    for (int k = 0; k < 3; ++k) {
        a.mitte[k] = (mn[k] + mx[k]) * 0.5F;
        groesse = std::max(groesse, mx[k] - mn[k]);
    }
    // Das Seitenverhaeltnis zaehlt: in einer schmalen Ansicht bestimmt die
    // Breite, wie weit zurueck es geht (Kasten in Achsen der Ansicht, 10 % Rand).
    float r[3];
    float h[3];
    float v[3];
    ansichtAchsen(a.art, r, h, v);
    float halbB = 0.0F;
    float halbH = 0.0F;
    float halbT = 0.0F;
    for (int k = 0; k < 3; ++k) {
        const float e = (mx[k] - mn[k]) * 0.5F;
        halbB += std::fabs(r[k]) * e;
        halbH += std::fabs(h[k]) * e;
        halbT += std::fabs(v[k]) * e;
    }
    const float seiten = a.groesse[0] > 0.0F && a.groesse[1] > 0.0F ? a.groesse[0] / a.groesse[1] : 1.0F;
    a.halbHoehe = std::max(std::max(halbH, halbB / seiten) * 1.1F, 4.0F);
    if (a.art == AnsichtArt::Perspektive) {
        const float fy = std::tan(g_app->modelCam.fovY * kPi / 360.0F);
        const float noetig = std::max(halbH / fy, halbB / (fy * seiten)) * 1.1F + halbT;
        g_app->modelOrbit.distance = std::max(noetig, std::max(groesse * 0.5F, 8.0F));
        float vorn[3];
        g_app->modelCam.forward(vorn);
        for (int k = 0; k < 3; ++k) { g_app->modelCam.pos[k] = a.mitte[k] - vorn[k] * g_app->modelOrbit.distance; }
    }
    g_app->modelDirty = true;
}

std::string g_eingepasst;                // Modell, fuer das die Ansichten eingepasst sind
std::array<std::string, 4> g_sichtStand;   // je Ansicht: was zuletzt gezeichnet wurde

std::vector<gpu::ModellZusatz> zusaetze() {
    const ModellTab& t = mt();
    std::vector<gpu::ModellZusatz> z;
    for (const ModellAnbau& a : t.anbauten) {
        if (a.versteckt) { continue; }   // mb1a2: Auge im Explorer
        gpu::ModellZusatz x;
        if (!anbauWelt(a, x.welt)) { continue; }
        x.modell = &a.modell;
        x.texturen = g_app->showModelTextures && a.texturen.found != 0 ? &a.texturen : nullptr;
        z.push_back(std::move(x));
    }
    if (t.an.boden || t.an.raster) {
        float groesse = 0.0F;
        for (int k = 0; k < 2; ++k) { groesse = std::max(groesse, std::fabs(g_app->model.mins[k]) + std::fabs(g_app->model.maxs[k])); }
        groesse = std::max(groesse, 32.0F);
        const float einheit[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
        for (const bool raster : {false, true}) {
            if (raster ? !t.an.raster : !t.an.boden) { continue; }
            gpu::ModellZusatz x;
            x.modell = &bodenModell(t.an.bodenZ, raster, groesse);
            x.figur = false;   // Stufe 3: keine Silhouette
            std::memcpy(x.welt, einheit, sizeof x.welt);
            z.push_back(std::move(x));
        }
    }
    return z;
}

gpu::ModellAnsichtArt zeichenArt() {
    gpu::ModellAnsichtArt art;
    art.hintergrund = g_app->settings.modellHintergrund;   // mb1a2
    art.drahtgitter = mt().an.drahtgitter;
    art.kantenUeber = mt().an.kanten;
    art.zweiseitig = mt().an.zweiseitig;
    // Stufe 3: Silhouette (Figur schwarz auf hellem Grund) und Ghosts
    if (mt().silhouette) {
        art.silhouette[0] = art.silhouette[1] = art.silhouette[2] = 0.04F;
        art.silhouette[3] = 1.0F;
        art.hintergrund = 0xE8E8E8;
    }
    art.geister = modellGeister();
    return art;
}

const ModelTextures* texturen() {
    return (g_app->showModelTextures && g_app->modelTextures.found != 0) ? &g_app->modelTextures : nullptr;
}

void eineAnsicht(int n, ImVec2 groesse, bool neu) {
    ModellTab& t = mt();
    ModellAnsicht& a = t.ansichten[n];
    const int w = std::max(32, static_cast<int>(groesse.x));
    const int h = std::max(32, static_cast<int>(groesse.y));
    ImGui::PushID(n);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
    ImGui::BeginChild("mtansicht", ImVec2{static_cast<float>(w), static_cast<float>(h)}, ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar);
    ImGui::PopStyleVar();
    const ImVec2 lo = ImGui::GetCursorScreenPos();
    a.lo[0] = lo.x;
    a.lo[1] = lo.y;
    a.groesse[0] = static_cast<float>(w);
    a.groesse[1] = static_cast<float>(h);
    a.sichtbar = true;
    baueSicht(a, w, h, a.viewProj);
    ImGuiIO& io = ImGui::GetIO();
    const bool ueber = ImGui::IsWindowHovered();
    // --- Maus wie 3ds Max ---------------------------------------------------
    if (ueber && (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Middle) ||
                  ImGui::IsMouseClicked(ImGuiMouseButton_Right))) {
        t.aktiv = n;
        t.klickDrinBild = ImGui::GetFrameCount();
    }
    if (ueber && io.MouseWheel != 0.0F) {
        if (a.art == AnsichtArt::Perspektive) {
            g_app->modelOrbit.zoom(g_app->modelCam, io.MouseWheel);
        } else {
            a.halbHoehe = std::clamp(a.halbHoehe * std::pow(0.85F, io.MouseWheel), 0.5F, 8192.0F);
        }
        g_app->modelDirty = true;
    }
    if (ueber && ImGui::IsMouseDown(ImGuiMouseButton_Middle) && (io.MouseDelta.x != 0.0F || io.MouseDelta.y != 0.0F)) {
        if (io.KeyAlt && io.KeyCtrl) {
            // Strg+Alt+Mitte: Zoom ziehen
            if (a.art == AnsichtArt::Perspektive) {
                g_app->modelOrbit.zoom(g_app->modelCam, -io.MouseDelta.y * 0.05F);
            } else {
                a.halbHoehe = std::clamp(a.halbHoehe * std::pow(1.01F, io.MouseDelta.y), 0.5F, 8192.0F);
            }
        } else if (io.KeyAlt) {
            if (a.art == AnsichtArt::Perspektive) { g_app->modelOrbit.turn(g_app->modelCam, -io.MouseDelta.x * 0.4F, -io.MouseDelta.y * 0.4F); }
        } else if (a.art == AnsichtArt::Perspektive) {
            g_app->modelOrbit.pan(g_app->modelCam, io.MouseDelta.x, io.MouseDelta.y);
        } else {
            float r[3];
            float o[3];
            float v[3];
            ansichtAchsen(a.art, r, o, v);
            const float proPx = 2.0F * a.halbHoehe / static_cast<float>(h);
            for (int k = 0; k < 3; ++k) { a.mitte[k] += (-io.MouseDelta.x * r[k] + io.MouseDelta.y * o[k]) * proPx; }
        }
        g_app->modelDirty = true;
    }
    float mitte[3];
    const bool hatMitte = t.gizmo != 0 && t.modal.art == 0 && auswahlMitte(mitte);
    // mb1a2: G/R/S wie Blender laufen ueber die Maus, bis Klick/Enter/Esc (modellModalTasten)
    if (t.modal.art != 0 && t.modal.ansicht == n) { modalAnwenden(); }
    // mb1a2: im Bereich Weights malt die linke Taste mit dem Pinsel (ein Strich = ein Rueckgaengig-Schritt)
    const bool malen = gewichtsAnsicht() && t.modal.art == 0;
    if (malen && ueber && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !io.KeyAlt) {
        (void)modellTupfer(n, io.MousePos, true);
    } else if (malen && t.malStrich && t.aktiv == n && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        // Blender: ein neuer Tupfer nach einem Zehntel des Radius Weg (Pinsel "Spacing" 10 %)
        if (std::hypot(io.MousePos.x - t.letzterTupfer[0], io.MousePos.y - t.letzterTupfer[1]) >= t.pinselRadius * 0.1F) {
            (void)modellTupfer(n, io.MousePos, false);
        }
    }
    if (t.malStrich && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) { modellStrichEnde(); }
    // mb1a2: Rechtsklick ohne Ziehen oeffnet das Menue der Ansicht (Max: Quad-Menue, Blender: Kontextmenue)
    if (ueber && t.modal.art == 0 && t.modalEndeBild != ImGui::GetFrameCount() && ImGui::IsMouseReleased(ImGuiMouseButton_Right) &&
        ImGui::GetMouseDragDelta(ImGuiMouseButton_Right).x == 0.0F && ImGui::GetMouseDragDelta(ImGuiMouseButton_Right).y == 0.0F) {
        t.kontextOeffnen = true;
    }
    // Stufe 3b: ein Punkt eines Bewegungspfads (Becken/Wurzel) zieht vor Gizmo und Rahmen
    const bool pfadKlick = !malen && t.modal.art == 0 && ueber && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !io.KeyAlt && modellPfadZiehenStart(a, io.MousePos);
    if (t.aktiv == n) { modellPfadZiehenWeiter(a); }
    if (ueber && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) { modellPfadTooltip(a, io.MousePos); }
    if (!pfadKlick && !malen && t.modal.art == 0 && t.modalEndeBild != ImGui::GetFrameCount() && ueber && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        (!io.KeyAlt || t.auswahlArt != AuswahlArt::Objekt)) {
        t.griff = hatMitte ? gizmoGriff(a, mitte, io.MousePos) : 0;
        t.ziehStart[0] = io.MousePos.x;
        t.ziehStart[1] = io.MousePos.y;
        t.ziehVorher.clear();
        t.rahmen = t.griff == 0;
        t.rahmenStart[0] = io.MousePos.x;
        t.rahmenStart[1] = io.MousePos.y;
    }
    if (t.aktiv == n && t.griff != 0 && hatMitte) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            gizmoZiehen(a, mitte);
        } else {
            t.griff = 0;
            t.ziehVorher.clear();
            modellRigZiehenEnde();   // Stufe 3
        }
    }
    // Stufe 3: im Bereich Animation waehlt ein Klick auf einen Griff des Rigs (vor dem Netz)
    if (t.aktiv == n && t.rahmen && ImGui::IsMouseReleased(ImGuiMouseButton_Left) && t.bereich == ModellBereich::Animation && modellRigDa() &&
        std::hypot(io.MousePos.x - t.rahmenStart[0], io.MousePos.y - t.rahmenStart[1]) < 4.0F) {
        const int griff = modellRigGriffUnter(a, io.MousePos);
        if (griff >= 0) { t.rahmen = false; }
        modellRigGriffWaehlen(griff);   // daneben: keiner (die Netz-Auswahl geht weiter)
    }
    if (t.aktiv == n && t.rahmen && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        t.rahmen = false;
        const ImVec2 start{t.rahmenStart[0], t.rahmenStart[1]};
        const bool klick = std::hypot(io.MousePos.x - start.x, io.MousePos.y - start.y) < 4.0F;
        if (t.auswahlArt != AuswahlArt::Objekt) {   // mb1a2: Ecke, Kante, Dreieck, Rand, Element
            eckenWahlAendern(klick ? eckenUnter(a, io.MousePos) : eckenImRahmen(a, start, io.MousePos));
        } else if (klick) {
            const int f = flaecheUnter(a, io.MousePos);
            auswahlAendern(f >= 0 ? std::vector<int>{f} : std::vector<int>{});
        } else {
            auswahlAendern(flaechenImRahmen(a, start, io.MousePos));
        }
        g_app->modelDirty = true;
    }
    // --- Zeichnen -----------------------------------------------------------
    char stand[200];
    std::snprintf(stand, sizeof stand, "%d|%d|%d|%.4f|%.4f|%.4f|%.4f|%.4f|%.4f|%.4f|%.4f|%.4f|%.4f|%.4f", w, h, static_cast<int>(a.art),
                  static_cast<double>(g_app->modelCam.pos[0]), static_cast<double>(g_app->modelCam.pos[1]),
                  static_cast<double>(g_app->modelCam.pos[2]), static_cast<double>(g_app->modelCam.angles[0]),
                  static_cast<double>(g_app->modelCam.angles[1]), static_cast<double>(g_app->modelCam.fovY),
                  static_cast<double>(a.mitte[0]), static_cast<double>(a.mitte[1]), static_cast<double>(a.mitte[2]),
                  static_cast<double>(a.halbHoehe), static_cast<double>(g_app->modelOrbit.distance));
    if (neu || g_sichtStand[static_cast<std::size_t>(n)] != stand) {
        gpu::modellZielWaehlen(n);
        if (gpu::bereiteModellZiel(w, h)) {
            const auto ab = std::chrono::steady_clock::now();
            std::string fehler;
            const bool gw = gewichtsAnsicht();
            const int aufrufe = gpu::zeichneModellAnsicht(gw ? gewichtsModell() : t.anzeige, gw ? &g_gewTex : texturen(), g_knochen, a.viewProj, true,
                                                          &fehler, zeichenArt(), zusaetze());
            if (aufrufe == 0 && !fehler.empty()) { diag::detail("Modellfenster: " + fehler); }
            g_app->modellAufrufe = aufrufe;
            ++g_app->modellNeuGezeichnet;
            g_app->modellCpuMsSumme += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - ab).count();
            g_sichtStand[static_cast<std::size_t>(n)] = stand;
        }
    }
    gpu::modellZielWaehlen(n);
    if (gpu::modellZielTextur() != nullptr) {
        ImGui::Image(reinterpret_cast<ImTextureID>(gpu::modellZielTextur()), ImVec2{static_cast<float>(w), static_cast<float>(h)});
    }
    ueberlagerung(a, ImGui::GetWindowDrawList());
    if (hatMitte) { gizmoZeichnen(a, mitte); }
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (gewichtsAnsicht() && ueber) {   // mb1a2: der Pinselkreis
        dl->AddCircle(io.MousePos, t.pinselRadius, IM_COL32(255, 255, 255, 200), 48, 1.5F);
        ++t.ueber[11];
    }
    if (t.modal.art != 0 && t.modal.ansicht == n) {   // mb1a2: was G/R/S gerade tut (wie Blenders Kopfzeile)
        static const char* const kArt[4] = {"", "G", "R", "S"};
        char z[120];
        std::snprintf(z, sizeof z, "%s  %s%s  %s", kArt[std::clamp(t.modal.art, 0, 3)], t.modal.ebene ? "Shift+" : "",
                      t.modal.achse >= 0 ? (t.modal.achse == 0 ? "X" : (t.modal.achse == 1 ? "Y" : "Z")) : "", t.modal.zahl.c_str());
        dl->AddText(ImVec2{lo.x + 6.0F, lo.y + ImGui::GetFontSize() * 1.4F}, IM_COL32(255, 230, 60, 255), z);
    }
    if (t.aktiv == n && t.rahmen && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        dl->AddRect(ImVec2{t.rahmenStart[0], t.rahmenStart[1]}, io.MousePos, IM_COL32(255, 255, 255, 200));
    }
    // Kopfzeile wie Max: [+] [Ansicht] [Standard]
    char kopf[200];
    std::snprintf(kopf, sizeof kopf, "[+] [%s] [%s]%s%s", tr(ansichtName(a.art)), tr(Str::MbAnsichtStandard), t.vierAnsichten ? "" : "  ",
                  t.vierAnsichten ? "" : tr(modellBlender() ? Str::MbAnsichtMaxBlender : Str::MbAnsichtMax));
    dl->AddText(ImVec2{lo.x + 6.0F, lo.y + 4.0F}, IM_COL32(220, 220, 220, 230), kopf);
    if (t.an.statistik) {
        int fl = 0;
        int dr = 0;
        int ec = 0;
        for (std::size_t i = 0; i < t.anzeige.surfaces.size(); ++i) {
            if (!gezeichnet(i)) { continue; }
            ++fl;
            dr += static_cast<int>(t.anzeige.surfaces[i].indexes.size() / 3U);
            ec += static_cast<int>(t.anzeige.surfaces[i].verts.size());
        }
        char z[200];
        std::snprintf(z, sizeof z, trN(Str::MbStatistik, fl, dr, ec), fl, dr, ec);
        dl->AddText(ImVec2{lo.x + 6.0F, lo.y + static_cast<float>(h) - ImGui::GetFontSize() * 1.4F}, IM_COL32(230, 230, 230, 230), z);
    }
    if (t.aktiv == n) { dl->AddRect(lo, ImVec2{lo.x + static_cast<float>(w), lo.y + static_cast<float>(h)}, IM_COL32(230, 180, 40, 255), 0.0F, 0, 2.0F); }
    ImGui::EndChild();
    ImGui::PopID();
}

}  // namespace

bool modellAufSchirm(const ModellAnsicht& a, const float p[3], ImVec2& out) { return aufSchirm(a, p, out); }

void modellAnsichtEinpassenAktiv() {
    ModellTab& t = mt();
    einpassen(t.ansichten[t.aktiv]);
}

void drawModellArbeitsflaeche() {
    ModellTab& t = mt();
    modellAnzeigeAktualisieren();
    if (g_eingepasst != g_app->modelPath) {
        // Neue Figur: die vier Ansichten (Oben, Vorne, Links, Perspektive) auf sie einpassen.
        const AnsichtArt arten[4] = {AnsichtArt::Oben, AnsichtArt::Vorne, AnsichtArt::Links, AnsichtArt::Perspektive};
        for (int i = 0; i < 4; ++i) {
            t.ansichten[i].art = arten[i];
            if (i < 3) { einpassen(t.ansichten[i]); }
        }
        g_eingepasst = g_app->modelPath;
    }
    modellDrehteller();
    // Stufe 3: ein Griff, der in der Ansicht gezogen wird, aendert das Dokument erst nach posieren() dieses Bilds - und
    // modelDirty geht am Ende des Bilds aus. Darum: neu posieren, sobald sich das Dokument geaendert hat (mb4_rig1: der
    // gezogene Griff drehte die Hand nicht, 0,00 Grad statt 30).
    const bool neu = g_app->modelDirty || t.dok.stand() != g_posiertBeiStand;
    if (neu) {
        posieren();
        g_posiertBeiStand = t.dok.stand();
    }
    g_app->modellKnochenLinien = 0;
    for (int& u : t.ueber) { u = 0; }
    t.griffeGezeichnet = 0;   // Stufe 3: Zaehler je Bild
    t.ikGezeichnet = 0;       // Stufe 4
    t.pfadPunkte = 0;
    t.pfadSchluessel = 0;
    modellKlickPruefen();
    // Die Bereiche (Ansehen/Pruefen, Modellieren, UV, Weights, Painting,
    // Animation) und die Tasten des Bedienmodus in einer Zeile.
    drawModellBereiche();
    ImGui::SameLine(0.0F, ImGui::GetFontSize());
    ImGui::AlignTextToFramePadding();
    {
        // mb1a2: die Tastenzeile so weit, wie Platz ist (ganze Abschnitte zwischen " · "), der Rest im Tooltip - nichts
        // abgeschnitten
        const std::string ganz = tr(modellBlender() ? Str::MbTastenBlender : Str::MbTastenMax);
        const float frei = ImGui::GetContentRegionAvail().x;
        std::string zeigen = ganz;
        if (ImGui::CalcTextSize(ganz.c_str()).x > frei) {
            const std::string trenner = " \xc2\xb7 ";
            std::string teil;
            std::size_t ab = 0;
            zeigen.clear();
            while (ab < ganz.size()) {
                const std::size_t bis = ganz.find(trenner, ab);
                const std::string stueck = ganz.substr(ab, bis == std::string::npos ? std::string::npos : bis - ab);
                const std::string probe = (teil.empty() ? stueck : teil + trenner + stueck) + trenner + "...";
                if (ImGui::CalcTextSize(probe.c_str()).x > frei) { break; }
                teil = teil.empty() ? stueck : teil + trenner + stueck;
                if (bis == std::string::npos) { break; }
                ab = bis + trenner.size();
            }
            zeigen = teil + (teil.size() < ganz.size() ? trenner + "..." : std::string());
        }
        ImGui::TextDisabled("%s", zeigen.c_str());
        if (zeigen != ganz) { ImGui::SetItemTooltip("%s", ganz.c_str()); }
    }
    const ImVec2 frei = ImGui::GetContentRegionAvail();
    const float fs = ImGui::GetFontSize();
    // Das Befehlsfeld behaelt seine Breite (die Texte passen hinein), die Ansicht nimmt den Rest.
    const float befehlB = std::max(std::min(fs * 17.0F, frei.x - fs * 8.0F), fs * 8.0F);
    const float sp = ImGui::GetStyle().ItemSpacing.x;
    // Stufe 3b: Dopesheet/Kurveneditor unter den Ansichten, solange offen (die Ansichten werden so viel niedriger)
    if (t.kvOffen && t.kvHoehe <= 0.0F) { t.kvHoehe = fs * 16.0F; }
    const float editorH = modellAnimEditorHoehe(frei.y);
    const ImVec2 ansichtenG{std::max(frei.x - befehlB - sp, 64.0F), std::max(frei.y - (editorH > 0.0F ? editorH + ImGui::GetStyle().ItemSpacing.y : 0.0F), 64.0F)};
    ImGui::BeginChild("mtansichten", ansichtenG, ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    for (ModellAnsicht& a : t.ansichten) { a.sichtbar = false; }
    if (t.vierAnsichten) {
        const ImVec2 g{std::floor((ansichtenG.x - 2.0F) * 0.5F), std::floor((ansichtenG.y - 2.0F) * 0.5F)};
        for (int i = 0; i < 4; ++i) {
            if (i % 2 == 1) { ImGui::SameLine(0.0F, 2.0F); }
            eineAnsicht(i, g, neu);
        }
    } else {
        eineAnsicht(t.aktiv, ansichtenG, neu);
    }
    ImGui::EndChild();
    ImGui::SameLine(0.0F, sp);
    ImGui::BeginChild("mtbefehlsfeld", ImVec2{befehlB, ansichtenG.y}, ImGuiChildFlags_Borders);
    drawModellBefehlsfeld();
    ImGui::EndChild();
    if (editorH > 0.0F) { drawModellAnimEditorUnten(ImVec2{frei.x, editorH}); }
    drawModellAktionen();
    drawModellRigUebersicht();   // Stufe 3
    drawModellAnimEditor();      // Stufe 3b: Dopesheet / Kurveneditor
    drawModellModusMenue();
    if (t.kontextOeffnen) {
        ImGui::OpenPopup("##mtkontext");
        t.kontextOeffnen = false;
    }
    drawModellKontextMenue();
    g_app->modelDirty = false;
}

// mb1a2: Ansichten wie Blenders Ziffernblock (view3d.view_axis, view_persportho, view_orbit): die Mitte und die Groesse
// bleiben, nur die Richtung wechselt - anders als 3ds Max P/T/F/L, die zugleich einpassen.
void modellAnsichtSetzen(AnsichtArt art) {
    ModellTab& t = mt();
    ModellAnsicht& a = t.ansichten[t.aktiv];
    const float tanH = std::tan(g_app->modelCam.fovY * kPi / 360.0F);
    if (a.art == AnsichtArt::Perspektive && art != AnsichtArt::Perspektive) {
        g_app->modelOrbit.pivot(g_app->modelCam, a.mitte);
        a.halbHoehe = std::max(g_app->modelOrbit.distance * tanH, 0.5F);
    } else if (a.art != AnsichtArt::Perspektive && art == AnsichtArt::Perspektive) {
        // in dieselbe Richtung schauen: Quake-Winkel (pitch, yaw) aus der Blickachse der Ansicht
        float r[3];
        float h[3];
        float v[3];
        ansichtAchsen(a.art, r, h, v);
        g_app->modelCam.angles[0] = -std::asin(std::clamp(v[2], -1.0F, 1.0F)) * 180.0F / kPi;
        g_app->modelCam.angles[1] = std::atan2(v[1], v[0]) * 180.0F / kPi;
        g_app->modelCam.angles[2] = 0.0F;
        g_app->modelOrbit.distance = std::max(a.halbHoehe / std::max(tanH, 1e-3F), 8.0F);
        for (int k = 0; k < 3; ++k) { g_app->modelCam.pos[k] = a.mitte[k] - v[k] * g_app->modelOrbit.distance; }
    }
    a.art = art;
    g_app->modelDirty = true;
}

// Ziffernblock 5: Perspektive <-> die Achsenansicht, die der Blickrichtung am naechsten liegt.
void modellPerspektiveUmschalten() {
    ModellTab& t = mt();
    if (t.ansichten[t.aktiv].art != AnsichtArt::Perspektive) {
        modellAnsichtSetzen(AnsichtArt::Perspektive);
        return;
    }
    float v[3];
    g_app->modelCam.forward(v);
    static const AnsichtArt kArten[6] = {AnsichtArt::Links, AnsichtArt::Rechts, AnsichtArt::Hinten, AnsichtArt::Vorne, AnsichtArt::Unten, AnsichtArt::Oben};
    // Blickachsen: Links +X, Rechts -X, Hinten +Y, Vorne -Y, Unten +Z, Oben -Z (ansichtAchsen)
    static const float kV[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    int best = 3;
    float bd = -2.0F;
    for (int i = 0; i < 6; ++i) {
        const float d = v[0] * kV[i][0] + v[1] * kV[i][1] + v[2] * kV[i][2];
        if (d > bd) {
            bd = d;
            best = i;
        }
    }
    modellAnsichtSetzen(kArten[best]);
}

// Ziffernblock 2/4/6/8: um 15 Grad kreisen (Blender: Preferences > Navigation > Rotation Amount, Vorgabe 15);
// eine Achsenansicht wird dabei zur Perspektive (Auto Perspective).
void modellKreisenSchritt(float gier, float nick) {
    ModellTab& t = mt();
    if (t.ansichten[t.aktiv].art != AnsichtArt::Perspektive) { modellAnsichtSetzen(AnsichtArt::Perspektive); }
    g_app->modelOrbit.turn(g_app->modelCam, gier, nick);
    g_app->modelDirty = true;
}

// mb1a2: das Foto mit der Ueberlagerung (Knochen, Tags, Nummern, Lineal ...) wie in der Ansicht - die Linien und Texte
// gehen ueber dieselbe Zeichenliste wie auf dem Schirm, nur in Bildpunkten des Fotos.
bool modellFotoMitUeberlagerung(int breite, std::vector<unsigned char>& rgba, int& bw, int& bh) {
    ModellTab& t = mt();
    if (g_app->model.empty()) { return false; }
    ModellAnsicht a = t.ansichten[t.aktiv];
    const float seiten = a.groesse[0] > 0.0F && a.groesse[1] > 0.0F ? a.groesse[1] / a.groesse[0] : 9.0F / 16.0F;
    const int w = std::max(breite, static_cast<int>(a.groesse[0]));
    const int h = std::max(static_cast<int>(static_cast<float>(w) * seiten), 32);
    baueSicht(a, w, h, a.viewProj);
    a.lo[0] = 0.0F;
    a.lo[1] = 0.0F;
    a.groesse[0] = static_cast<float>(w);
    a.groesse[1] = static_cast<float>(h);
    gpu::modellZielWaehlen(4);
    std::string fehler;
    const bool gw = gewichtsAnsicht();
    if (!gpu::bereiteModellZiel(w, h) ||
        gpu::zeichneModellAnsicht(gw ? gewichtsModell() : t.anzeige, gw ? &g_gewTex : texturen(), g_knochen, a.viewProj, true, &fehler, zeichenArt(),
                                  zusaetze()) <= 0) {
        return false;
    }
    ImDrawList liste(ImGui::GetDrawListSharedData());
    liste._ResetForNewFrame();
    liste.PushTexture(ImGui::GetIO().Fonts->TexRef);
    liste.PushClipRect(ImVec2{0.0F, 0.0F}, ImVec2{static_cast<float>(w), static_cast<float>(h)});
    ueberlagerung(a, &liste);
    (void)gpu::zeichneUeberlagerung(&liste);
    return gpu::leseModellZiel(rgba, bw, bh);
}

// --- Foto --------------------------------------------------------------------------------------

bool modellBmp(const std::string& pfad, int w, int h, const std::vector<unsigned char>& rgba) {
    std::FILE* f = std::fopen(pfad.c_str(), "wb");
    if (f == nullptr) { return false; }
    const auto bild = static_cast<std::uint32_t>(w) * static_cast<std::uint32_t>(h) * 4U;
    unsigned char kopf[54] = {'B', 'M'};
    const auto u32 = [&](int at, std::uint32_t v) {
        for (int k = 0; k < 4; ++k) { kopf[at + k] = static_cast<unsigned char>(v >> (8 * k)); }
    };
    u32(2, 54U + bild);
    u32(10, 54U);
    u32(14, 40U);
    u32(18, static_cast<std::uint32_t>(w));
    u32(22, static_cast<std::uint32_t>(-h));   // von oben
    kopf[26] = 1;
    kopf[28] = 32;
    u32(34, bild);
    bool ok = std::fwrite(kopf, 1, sizeof kopf, f) == sizeof kopf;
    std::vector<unsigned char> zeile(static_cast<std::size_t>(w) * 4U);
    for (int y = 0; y < h && ok; ++y) {
        const unsigned char* q = rgba.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(w) * 4U;
        for (int x = 0; x < w; ++x) {   // RGBA -> BGRA
            zeile[static_cast<std::size_t>(x) * 4U + 0U] = q[x * 4 + 2];
            zeile[static_cast<std::size_t>(x) * 4U + 1U] = q[x * 4 + 1];
            zeile[static_cast<std::size_t>(x) * 4U + 2U] = q[x * 4 + 0];
            zeile[static_cast<std::size_t>(x) * 4U + 3U] = 255;
        }
        ok = std::fwrite(zeile.data(), 1, zeile.size(), f) == zeile.size();
    }
    std::fclose(f);
    return ok;
}

bool modellFotoSpeichern(const std::string& pfad, int breite) {
    ModellTab& t = mt();
    std::vector<unsigned char> rgba;
    int bw = 0;
    int bh = 0;
    // mb1a2: mit der Ueberlagerung der Ansicht (bis 1a-1 nur das gezeichnete Modell)
    const bool ok = modellFotoMitUeberlagerung(breite, rgba, bw, bh) && modellBmp(pfad, bw, bh, rgba);
    if (ok) {
        char z[400];
        std::snprintf(z, sizeof z, tr(Str::MbFotoGespeichert), pfad.c_str(), bw, bh);
        t.meldung = z;
        diag::detail(std::string("Model-Tab: ") + z);
    }
    return ok;
}

bool modellFotoPixel(int breite, int hoehe, std::vector<unsigned char>& rgba, bool ueberDokument) {
    ModellTab& t = mt();
    if (g_app->model.empty()) { return false; }
    float vp[16];
    gpu::baueViewProj(g_app->modelCam, breite, hoehe, 1.0F, 8192.0F, vp);
    gpu::modellZielWaehlen(5);
    if (!gpu::bereiteModellZiel(breite, hoehe)) { return false; }
    std::string fehler;
    int n = 0;
    if (ueberDokument) {
        modellAnzeigeAktualisieren();
        posieren();
        n = gpu::zeichneModellAnsicht(t.anzeige, texturen(), g_knochen, vp, true, &fehler, gpu::ModellAnsichtArt{}, {});
    } else {
        // Der Weg bis rc610 (drawModelView): das gelesene Modell, das Bild der
        // Animation ohne Mischen, Kappen nach der Leiste.
        std::vector<BoneMatrix> welt;
        std::vector<BoneMatrix> knochen;
        if (!g_app->anim.bones.empty() && g_app->anim.numFrames > 0) {
            g_app->anim.worldMatrices(std::clamp(g_app->animFrame, 0, g_app->anim.numFrames - 1), welt);
            gpu::baueKnochenmatrizen(welt, g_app->anim.bones, knochen);
        }
        n = gpu::zeichneModellAnsicht(g_app->model, texturen(), knochen, vp, g_app->showCaps, &fehler);
    }
    int w = 0;
    int h = 0;
    return n >= 0 && gpu::leseModellZiel(rgba, w, h) && w == breite && h == hoehe;
}

// --- Fuer den Selbsttest --------------------------------------------------------------------

bool modellAnsichtPixel(int ansicht, std::vector<unsigned char>& rgba, int& breite, int& hoehe) {
    if (ansicht < 0 || ansicht >= 4) { return false; }
    gpu::modellZielWaehlen(ansicht);
    return gpu::leseModellZiel(rgba, breite, hoehe);
}

bool modellFlaecheAufSchirm(int flaeche, ImVec2& out) {
    const ModellTab& t = mt();
    const auto& h = haut();
    if (flaeche < 0 || static_cast<std::size_t>(flaeche) >= h.size() || h[static_cast<std::size_t>(flaeche)].empty()) { return false; }
    float m[3] = {0.0F, 0.0F, 0.0F};
    for (const auto& p : h[static_cast<std::size_t>(flaeche)]) {
        for (int k = 0; k < 3; ++k) { m[k] += p[static_cast<std::size_t>(k)]; }
    }
    for (float& k : m) { k /= static_cast<float>(h[static_cast<std::size_t>(flaeche)].size()); }
    return aufSchirm(t.ansichten[t.aktiv], m, out);
}

bool modellPunktAufSchirm(int ansicht, const GlmModel& m, ImVec2& out) {
    if (ansicht < 0 || ansicht >= 4) { return false; }
    const float p[3] = {(m.mins[0] + m.maxs[0]) * 0.5F, (m.mins[1] + m.maxs[1]) * 0.5F, (m.mins[2] + m.maxs[2]) * 0.5F};
    return aufSchirm(mt().ansichten[ansicht], p, out);
}

bool modellAuswahlMitte(float mitte[3]) { return auswahlMitte(mitte); }

bool modellGizmoGriffOrt(int griff, ImVec2& out) {
    const ModellTab& t = mt();
    float m[3];
    if (griff < 1 || griff > 3 || !auswahlMitte(m)) { return false; }
    const ModellAnsicht& a = t.ansichten[t.aktiv];
    const float L = laengeFuerPx(a, m, 70.0F * t.gizmoGroesse);
    float e[3] = {m[0], m[1], m[2]};
    e[griff - 1] += L * 0.8F;
    return aufSchirm(a, e, out);
}

// Stufe 3, Selbsttest: ein Punkt auf dem Ring der Achse (Drehen, E) unter dem Winkel `grad`.
bool modellGizmoRingOrt(int achse, float grad, ImVec2& out) {
    const ModellTab& t = mt();
    float m[3];
    if (achse < 0 || achse > 2 || !auswahlMitte(m)) { return false; }
    const ModellAnsicht& a = t.ansichten[t.aktiv];
    float p[3];
    ringPunkt(m, achse, laengeFuerPx(a, m, 70.0F * t.gizmoGroesse) * 0.8F, grad * kPi / 180.0F, p);
    return aufSchirm(a, p, out);
}

bool modellAnbauWelt(int anbau, float welt[16]) {
    const ModellTab& t = mt();
    if (anbau < 0 || static_cast<std::size_t>(anbau) >= t.anbauten.size()) { return false; }
    return anbauWelt(t.anbauten[static_cast<std::size_t>(anbau)], welt);
}

}  // namespace bhed::gui
