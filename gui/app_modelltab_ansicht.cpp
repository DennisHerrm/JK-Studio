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
#include "app_internal.h"
#include "gpumap.h"

#include "bhed/gpucam.h"
#include "bhed/gpuskin.h"
#include "bhed/modell/dokument.h"
#include "bhed/modell/g2.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace bhed::gui {
namespace {

ModellTab& mt() { return g_app->mtab; }

constexpr float kPi = 3.14159265F;

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
    a.worldMatricesLerp(fA, fB, anteil, ueber, g_app->modellWelt);
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
    return i < t.gezeichnet.size() && t.gezeichnet[i] != 0;
}

// Die Grenzen der gewaehlten (oder aller gezeichneten) Flaechen in der Pose.
bool grenzen(bool nurAuswahl, float mn[3], float mx[3]) {
    const ModellTab& t = mt();
    const auto& h = haut();
    bool da = false;
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

void ueberlagerung(const ModellAnsicht& a) {
    ModellTab& t = mt();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 lo{a.lo[0], a.lo[1]};
    const ImVec2 hi{a.lo[0] + a.groesse[0], a.lo[1] + a.groesse[1]};
    dl->PushClipRect(lo, hi, true);
    const ModellAnzeige& z = t.an;
    const bool brauchtHaut = z.normalen || z.gewichte || z.huelle || z.risse || z.tags || z.klammer || !t.auswahl.empty();
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
            if (!z.tags || s.indexes.size() < 3U) { continue; }
            for (int k = 0; k < 3; ++k) {
                linie(dl, a, h[i][s.indexes[static_cast<std::size_t>(k)]].data(), h[i][s.indexes[static_cast<std::size_t>((k + 1) % 3)]].data(),
                      IM_COL32(110, 230, 110, 255));
            }
            ImVec2 sp;
            if (aufSchirm(a, h[i][s.indexes[0]].data(), sp)) { dl->AddText(sp, IM_COL32(110, 230, 110, 255), s.name.c_str()); }
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
                    static const ImU32 kFarben[5] = {IM_COL32(120, 120, 120, 200), IM_COL32(80, 220, 80, 230), IM_COL32(230, 230, 60, 230),
                                                     IM_COL32(250, 150, 40, 230), IM_COL32(240, 60, 60, 230)};
                    farbe = kFarben[std::clamp(zahl, 0, 4)];
                }
                dl->AddRectFilled(ImVec2{sp.x - 1.5F, sp.y - 1.5F}, ImVec2{sp.x + 1.5F, sp.y + 1.5F}, farbe);
                ++t.ueber[2];
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
    dl->PopClipRect();
}

// --- Gizmo (W/E/R) ------------------------------------------------------------------------

bool auswahlMitte(float m[3]) {
    float mn[3];
    float mx[3];
    if (mt().auswahl.empty() || !grenzen(true, mn, mx)) { return false; }
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

// Die Teile der Auswahl umformen (alle LOD-Stufen; Normalen beim Drehen mit).
void umformen(modell::Inhalt& x, const modell::Inhalt& vorher, const std::vector<int>& teile, int art, const float m[3],
              const float wert[3]) {
    float rot[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    if (art == 2) {
        // Drehung um die Achsen X, Y, Z nacheinander (Grad).
        for (int k = 0; k < 3; ++k) {
            if (wert[k] == 0.0F) { continue; }
            const float w = wert[k] * kPi / 180.0F;
            const float c = std::cos(w);
            const float s = std::sin(w);
            float r[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
            const int u = (k + 1) % 3;
            const int v = (k + 2) % 3;
            r[u][u] = c;
            r[u][v] = -s;
            r[v][u] = s;
            r[v][v] = c;
            float neu[3][3];
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) { neu[i][j] = r[i][0] * rot[0][j] + r[i][1] * rot[1][j] + r[i][2] * rot[2][j]; }
            }
            std::memcpy(rot, neu, sizeof rot);
        }
    }
    for (const int i : teile) {
        if (i < 0 || static_cast<std::size_t>(i) >= vorher.teile.size()) { continue; }
        modell::Teil& ziel = x.teile[static_cast<std::size_t>(i)];
        ziel = vorher.teile[static_cast<std::size_t>(i)];
        for (modell::Netz& n : ziel.stufen) {
            for (modell::Ecke& e : n.ecken) {
                float d[3] = {e.ort[0] - m[0], e.ort[1] - m[1], e.ort[2] - m[2]};
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
            }
        }
    }
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
    static const Str kNamen[4] = {Str::MbUndoBewegen, Str::MbUndoBewegen, Str::MbUndoDrehen, Str::MbUndoSkalieren};
    if (t.ziehVorher.empty()) {
        t.dok.aendern(tr(kNamen[t.gizmo]), [](modell::Inhalt&) {});
        t.ziehVorher.push_back(t.dok.inhalt());
        std::memcpy(t.ziehWeg, m, sizeof t.ziehWeg);   // Drehpunkt: die Mitte beim Anfassen
    }
    const std::vector<int> teile = t.auswahl;
    const int art = t.gizmo;
    const float mitte[3] = {t.ziehWeg[0], t.ziehWeg[1], t.ziehWeg[2]};
    t.dok.vorschau([&](modell::Inhalt& x) { umformen(x, t.ziehVorher.front(), teile, art, mitte, wert); });
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
            std::memcpy(x.welt, einheit, sizeof x.welt);
            z.push_back(std::move(x));
        }
    }
    return z;
}

gpu::ModellAnsichtArt zeichenArt() {
    gpu::ModellAnsichtArt art;
    art.drahtgitter = mt().an.drahtgitter;
    art.kantenUeber = mt().an.kanten;
    art.zweiseitig = mt().an.zweiseitig;
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
    const bool hatMitte = t.gizmo != 0 && auswahlMitte(mitte);
    if (ueber && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !io.KeyAlt) {
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
        }
    }
    if (t.aktiv == n && t.rahmen && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        t.rahmen = false;
        const ImVec2 start{t.rahmenStart[0], t.rahmenStart[1]};
        if (std::hypot(io.MousePos.x - start.x, io.MousePos.y - start.y) < 4.0F) {
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
            const int aufrufe = gpu::zeichneModellAnsicht(t.anzeige, texturen(), g_knochen, a.viewProj, true, &fehler, zeichenArt(), zusaetze());
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
    ueberlagerung(a);
    if (hatMitte) { gizmoZeichnen(a, mitte); }
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (t.aktiv == n && t.rahmen && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        dl->AddRect(ImVec2{t.rahmenStart[0], t.rahmenStart[1]}, io.MousePos, IM_COL32(255, 255, 255, 200));
    }
    // Kopfzeile wie Max: [+] [Ansicht] [Standard]
    char kopf[200];
    std::snprintf(kopf, sizeof kopf, "[+] [%s] [%s]%s%s", tr(ansichtName(a.art)), tr(Str::MbAnsichtStandard), t.vierAnsichten ? "" : "  ",
                  t.vierAnsichten ? "" : tr(Str::MbAnsichtMax));
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
    const bool neu = g_app->modelDirty;
    if (neu) { posieren(); }
    g_app->modellKnochenLinien = 0;
    for (int& u : t.ueber) { u = 0; }
    modellKlickPruefen();
    // Die Bereiche (Ansehen/Pruefen, Modellieren, UV, Weights, Painting,
    // Animation) und die Tasten des Bedienmodus in einer Zeile.
    drawModellBereiche();
    ImGui::SameLine(0.0F, ImGui::GetFontSize());
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("%s", tr(Str::MbTastenMax));
    const ImVec2 frei = ImGui::GetContentRegionAvail();
    const float fs = ImGui::GetFontSize();
    // Das Befehlsfeld behaelt seine Breite (die Texte passen hinein), die Ansicht nimmt den Rest.
    const float befehlB = std::max(std::min(fs * 17.0F, frei.x - fs * 8.0F), fs * 8.0F);
    const float sp = ImGui::GetStyle().ItemSpacing.x;
    const ImVec2 ansichtenG{std::max(frei.x - befehlB - sp, 64.0F), std::max(frei.y, 64.0F)};
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
    drawModellAktionen();
    g_app->modelDirty = false;
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
    if (g_app->model.empty()) { return false; }
    ModellAnsicht a = t.ansichten[t.aktiv];
    const float seiten = a.groesse[0] > 0.0F && a.groesse[1] > 0.0F ? a.groesse[1] / a.groesse[0] : 9.0F / 16.0F;
    const int w = std::max(breite, static_cast<int>(a.groesse[0]));
    const int h = std::max(static_cast<int>(static_cast<float>(w) * seiten), 32);
    baueSicht(a, w, h, a.viewProj);
    gpu::modellZielWaehlen(4);
    std::vector<unsigned char> rgba;
    int bw = 0;
    int bh = 0;
    std::string fehler;
    const bool ok = gpu::bereiteModellZiel(w, h) &&
                    gpu::zeichneModellAnsicht(t.anzeige, texturen(), g_knochen, a.viewProj, true, &fehler, zeichenArt(), zusaetze()) > 0 &&
                    gpu::leseModellZiel(rgba, bw, bh) && modellBmp(pfad, bw, bh, rgba);
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

bool modellAnbauWelt(int anbau, float welt[16]) {
    const ModellTab& t = mt();
    if (anbau < 0 || static_cast<std::size_t>(anbau) >= t.anbauten.size()) { return false; }
    return anbauWelt(t.anbauten[static_cast<std::size_t>(anbau)], welt);
}

}  // namespace bhed::gui
