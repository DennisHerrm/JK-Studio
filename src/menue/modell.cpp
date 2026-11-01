// modell.cpp - Menue-Editor M2 Punkt 1: ITEM_TYPE_MODEL - Kamera, Licht und
// Portiert aus OpenJK (code/ui/ui_shared.cpp, code/ui/ui_saber.cpp, code/rd-vanilla/tr_light.cpp, tr_surface.cpp) und Movie Duels (ui_saber.cpp).
// Knochen wie Item_Model_Paint. Herkunft und Lizenz siehe
// include/bhed/menue/modell.h.
//
//   Copyright (C) 1999 - 2005, Id Software, Inc.
//   Copyright (C) 2000 - 2013, Raven Software, Inc.
//   Copyright (C) 2001 - 2013, Activision, Inc.
//   Copyright (C) 2013 - 2015, OpenJK contributors
//
//   OpenJK is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License version 2 as published by the
//   Free Software Foundation. This program is distributed in the hope that it
//   will be useful, but WITHOUT ANY WARRANTY; without even the implied
//   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//   GNU General Public License for more details (LICENSE).
#include "bhed/menue/modell.h"

#include "bhed/md3.h"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace bhed::menue {

namespace {
std::string klein(std::string s) {
    for (char& c : s) { c = c == '\\' ? '/' : static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}
std::string cvarOder(const CvarLeser& c, const char* n) { return c ? c(n) : std::string(); }
constexpr float kPi = 3.14159265358979323846F;

// ItemParse_asset_model (code/ui/ui_shared.cpp): "ui_char_model" ist in der Engine fest
// verdrahtet - dann models/players/<g_char_model>/model.glm.
std::string modellPfad(const Item& it, const CvarLeser& cvar) {
    if (klein(it.assetModel) == "ui_char_model") { return "models/players/" + cvarOder(cvar, "g_char_model") + "/model.glm"; }
    return it.assetModel;
}

// R_RegisterSkin mit "|": models/players/x/|head|torso|legs (UI_UpdateCharacterSkin) -
// drei .skin-Dateien im Ordner; sonst eine .skin-Datei.
std::vector<std::string> hautDateien(const std::string& haut) {
    std::vector<std::string> v;
    const std::size_t s = haut.find('|');
    if (s == std::string::npos) {
        if (!haut.empty()) { v.push_back(haut); }
        return v;
    }
    const std::string ordner = haut.substr(0, s);
    std::size_t p = s + 1;
    while (p <= haut.size()) {
        const std::size_t q = std::min(haut.find('|', p), haut.size());
        const std::string teil = haut.substr(p, q - p);
        if (!teil.empty()) { v.push_back(ordner + teil + ".skin"); }
        p = q + 1;
    }
    return v;
}

// AnglesToAxis (q_math): axis[0] = vorn, axis[1] = links, axis[2] = oben
void winkelZuAchsen(const float winkel[3], float achsen[3][3]) {
    const float p = winkel[0] * kPi / 180.0F, y = winkel[1] * kPi / 180.0F, r = winkel[2] * kPi / 180.0F;
    const float sp = std::sin(p), cp = std::cos(p), sy = std::sin(y), cy = std::cos(y), sr = std::sin(r), cr = std::cos(r);
    const float vorn[3] = {cp * cy, cp * sy, -sp};
    const float rechts[3] = {-1 * sr * sp * cy + -1 * cr * -sy, -1 * sr * sp * sy + -1 * cr * cy, -1 * sr * cp};
    const float oben[3] = {cr * sp * cy + -sr * -sy, cr * sp * sy + -sr * cy, cr * cp};
    for (int k = 0; k < 3; ++k) {
        achsen[0][k] = vorn[k];
        achsen[1][k] = -rechts[k];
        achsen[2][k] = oben[k];
    }
}
}  // namespace

std::string modellSchluessel(const Item& it, const CvarLeser& cvar) { return modellPfad(it, cvar) + "|" + it.g2skin + "|" + it.g2anim; }

bool ladeMenueModell(const Spieldaten& d, const Item& it, const CvarLeser& cvar, MenueModell& out) {
    out = MenueModell{};
    out.schluessel = modellSchluessel(it, cvar);
    const std::string pfad = modellPfad(it, cvar);
    if (klein(pfad).find(".md3") != std::string::npos) {
        // M5b: Item_Model_Paint ohne Ghoul2 (JA SP else-Zweig, Jedi Outcast): RE_RegisterModel, Bild 0
        std::string bytes;
        Md3Model m;
        if (!d.lies(pfad, bytes) || !readMd3(bytes, m, &out.fehler) || m.empty()) {
            if (out.fehler.empty()) { out.fehler = pfad + " fehlt"; }
            return false;
        }
        out.md3 = true;
        out.glm.name = pfad;
        out.glm.numBones = 1;
        bool erstes = true;
        for (const Md3Surface& ms : m.surfaces) {
            const Md3Vertex* v = ms.frame(0);
            if (v == nullptr) { continue; }
            GlmSurface gs;
            gs.name = ms.name;
            gs.shader = ms.shader;
            for (int i = 0; i < ms.numVerts; ++i) {
                GlmVertex gv;
                for (int k = 0; k < 3; ++k) {
                    gv.xyz[k] = v[i].xyz[k];
                    gv.normal[k] = v[i].normal[k];
                    out.mins[k] = erstes ? v[i].xyz[k] : std::min(out.mins[k], v[i].xyz[k]);
                    out.maxs[k] = erstes ? v[i].xyz[k] : std::max(out.maxs[k], v[i].xyz[k]);
                }
                erstes = false;
                gv.st[0] = v[i].st[0];
                gv.st[1] = v[i].st[1];
                gv.weights[0] = 1.0F;   // ein Knochen (Einheitsmatrix): starr
                gs.verts.push_back(gv);
            }
            gs.indexes = ms.indexes;
            out.glm.surfaces.push_back(std::move(gs));
        }
        out.ok = !out.glm.surfaces.empty();
        return out.ok;
    }
    if (klein(pfad).find(".glm") == std::string::npos) {
        out.fehler = "kein Ghoul2-Modell";
        return false;
    }
    std::string bytes;
    if (!d.lies(pfad, bytes) || !readGlm(bytes, out.glm, &out.fehler)) {
        if (out.fehler.empty()) { out.fehler = pfad + " fehlt"; }
        return false;
    }
    std::vector<std::string> haeute = hautDateien(it.g2skin);
    if (haeute.empty()) {   // ohne model_g2skin: model_default.skin wie die Engine
        haeute.push_back(pfad.substr(0, pfad.rfind('/') + 1) + "model_default.skin");
    }
    for (const std::string& h : haeute) {
        std::string t;
        if (d.lies(h, t)) { applySkin(t, out.glm); }
    }
    std::string gla;
    if (!out.glm.animFile.empty() && d.lies(out.glm.animFile + ".gla", gla)) {
        (void)readGla(gla, out.gla, nullptr);
        for (const std::string& c : animCfgCandidates(out.glm.animFile)) {
            std::string cfg;
            if (d.lies(c, cfg)) {
                out.anims = parseAnimationCfg(cfg);
                break;
            }
        }
    }
    out.ok = true;
    return true;
}

MenueModellBild menueModellBild(const MenueModell& m, const Item& it, int zeitMs, int animSeitMs, int breite, int hoehe, std::vector<BoneMatrix>& weltKnochen, const ModellLicht& licht) {
    MenueModellBild b;
    // Grenzen (ghoul2 hat keine: g2mins/g2maxs oder die Vorgabe)
    float mins[3] = {it.g2mins[0], it.g2mins[1], it.g2mins[2]};
    float maxs[3] = {it.g2maxs[0], it.g2maxs[1], it.g2maxs[2]};
    if (m.md3) {   // DC->modelBounds( item->asset, mins, maxs )
        for (int k = 0; k < 3; ++k) {
            mins[k] = m.mins[k];
            maxs[k] = m.maxs[k];
        }
    } else if (mins[0] == 0 && mins[1] == 0 && mins[2] == 0 && maxs[0] == 0 && maxs[1] == 0 && maxs[2] == 0) {
        mins[0] = -16;
        mins[1] = -16;
        mins[2] = -24;
        maxs[0] = 16;
        maxs[1] = 16;
        maxs[2] = 32;
    }
    float origin[3];
    origin[2] = -0.5F * (mins[2] + maxs[2]);
    origin[1] = 0.5F * (mins[1] + maxs[1]);
    const float w = static_cast<float>(std::max(1, breite));
    const float h = static_cast<float>(std::max(1, hoehe));
    // refdef.fov_x: (int)(width / 640 * 90), fov_y aus atan2 (Bildpunkte des refdef)
    float fovX = it.fovx != 0 ? it.fovx : static_cast<float>(static_cast<int>(w / 640.0F * 90.0F));
    float fovY = it.fovy != 0 ? it.fovy : std::atan2(h, w / std::tan(fovX / 360.0F * kPi)) * (360.0F / kPi);
    if (m.jk2Kamera) { fovX = fovY = 45.0F; }   // JK2_MODE: refdef.fov_x = refdef.fov_y = 45
    const float len = 0.5F * (maxs[2] - mins[2]);
    origin[0] = len / 0.268F;
    float yaw = static_cast<float>(it.angle);
    if (it.rotationSpeed != 0) { yaw += static_cast<float>(zeitMs) / static_cast<float>(it.rotationSpeed); }
    if (m.jk2Kamera) { yaw = static_cast<float>(zeitMs) / 20.0F; }   // JK2_MODE: angles (0, refdef.time / 20, 0)
    b.yawGrad = yaw;
    // "hack to put saber on it's side": ITF_ISANYSABER ohne ITF_ISCHARACTER -> (curYaw, 0, 90)
    // rc608 Punkt 12: Das Skelett einer gla steht 90 Grad gegen die Achsen des Spiels: model_angle 180 ist dort von vorn, 0 von hinten (Foto)
    float winkel[3] = {0, m.gla.empty() ? yaw : yaw + 90.0F, 0};
    if ((it.itemFlags & ITF_ISANYSABER) != 0 && (it.itemFlags & ITF_ISCHARACTER) == 0) {
        winkel[0] = yaw;
        winkel[1] = 0;
        winkel[2] = 90;
    }
    float ax[3][3];
    winkelZuAchsen(winkel, ax);
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) { b.achsen[r][c] = ax[r][c]; }
        b.ursprung[r] = origin[r];
    }
    // Welt = origin + x * axis[0] + y * axis[1] + z * axis[2]
    const float e[16] = {ax[0][0], ax[1][0], ax[2][0], origin[0], ax[0][1], ax[1][1], ax[2][1], origin[1],
                         ax[0][2], ax[1][2], ax[2][2], origin[2], 0, 0, 0, 1};
    // Augpunkt 0, viewaxis Einheit (vorn +X, links +Y, oben +Z); r_znear 4
    const float tx = std::tan(fovX / 360.0F * kPi), ty = std::tan(fovY / 360.0F * kPi);
    const float n = 4.0F, f = 8192.0F;
    const float p[16] = {0, -1.0F / tx, 0, 0, 0, 0, 1.0F / ty, 0, f / (f - n), 0, 0, -f * n / (f - n), 1, 0, 0, 0};
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            float s = 0;
            for (int k = 0; k < 4; ++k) { s += p[r * 4 + k] * e[k * 4 + c]; }
            b.viewProj[r * 4 + c] = s;
            b.kamera[r * 4 + c] = p[r * 4 + c];
        }
    }
    // R_SetupEntityLighting ohne Weltmodell, ein Licht 500 (1 1 1) am Augpunkt
    float amb = licht.umgebung;
    float dirLicht = 150.0F;
    float ld[3] = {0, 0, 0};   // tr.sunDirection * |directedLight|: ohne Karte ist die Sonne null
    if (licht.augenLicht) {
        float dir[3] = {-origin[0], -origin[1], -origin[2]};   // dl->origin (0) - lightOrigin
        float d = std::sqrt(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);
        if (d > 0) {
            for (float& x : dir) { x /= d; }
        }
        const float power = 16.0F * (500.0F * 500.0F);   // DLIGHT_AT_RADIUS * r^2
        if (d < 16.0F) { d = 16.0F; }                   // DLIGHT_MINIMUM_RADIUS
        d = power / (d * d);
        dirLicht += d;
        for (int k = 0; k < 3; ++k) { ld[k] += d * dir[k]; }
    }
    amb = std::min(amb, 255.0F);   // identityLightByte
    const float l = std::sqrt(ld[0] * ld[0] + ld[1] * ld[1] + ld[2] * ld[2]);
    if (l > 0) {
        for (float& x : ld) { x /= l; }
    }
    // in Entity-Achsen (DotProduct mit ent->e.axis)
    for (int k = 0; k < 3; ++k) { b.richtung[k] = ld[0] * ax[k][0] + ld[1] * ax[k][1] + ld[2] * ax[k][2]; }
    for (int k = 0; k < 3; ++k) {
        b.umgebung[k] = amb / 255.0F;
        b.gerichtet[k] = dirLicht / 255.0F;
    }
    // Knochen: model_g2anim aus der animation.cfg (ItemParse_model_g2anim_go: Q_stricmp gegen
    // die Namen), seit dem Setzen im Takt von G2_TimingModel und zwischen zwei Bildern gemischt;
    // ohne (oder unbekannt bzw. zu Ende) Bild 0
    weltKnochen.clear();
    if (!m.gla.empty()) {
        G2Bild g;
        // rc608: wie im Spiel fotografiert: ohne Wirkung die T-Pose (Bild 0)
        const bool spielt = !it.g2animOhneWirkung || (it.itemFlags & ITF_ISCHARACTER) != 0;
        const std::string name = spielt ? klein(it.g2anim) : std::string();
        for (const AnimEntry& a : m.anims) {
            if (!name.empty() && klein(a.name) == name) {
                g = g2Bild(a, static_cast<double>(animSeitMs));
                break;
            }
        }
        const int letztes = m.gla.numFrames - 1;
        b.bild = std::clamp(g.bild, 0, letztes);
        b.folge = std::clamp(g.folge, 0, letztes);
        b.anteil = g.anteil;
        m.gla.worldMatricesLerp(b.bild, b.folge, b.anteil, weltKnochen);
    }
    return b;
}

// --- M3 Punkt 2: UI_G2SetAnim und G2_TimingModel ------------------------------------

G2Bild g2Bild(const AnimEntry& a, double seitMs) {
    G2Bild b;
    // UI_G2SetAnim: anim->numFrames <= 0 -> nichts gesetzt (Bild 0)
    if (a.numFrames <= 0) {
        b.aus = true;
        return b;
    }
    const float fps = a.fps == 0 ? 1.0F : static_cast<float>(a.fps);
    const float frameLerp = fps < 0 ? std::floor(1000.0F / fps) : std::ceil(1000.0F / fps);
    const float animSpeed = 50.0F / frameLerp;
    const int start = a.firstFrame;
    const int ende = a.firstFrame + a.numFrames;
    const bool schleife = a.loopFrame != -1;
    // G2_TimingModel (bone.startTime = Zeitpunkt des Setzens, pauseTime 0)
    float time = static_cast<float>(seitMs) / 50.0F;
    if (time < 0.0F) { time = 0.0F; }
    float g = static_cast<float>(start) + (time * animSpeed);
    const int animSize = ende - start;
    const auto endFrame = static_cast<float>(ende);
    if (animSize != 0) {
        if ((animSpeed > 0.0F && g > endFrame - 1) || (animSpeed < 0.0F && g < endFrame + 1)) {
            if (schleife) {
                if (animSpeed < 0.0F) {
                    if (g < endFrame + 1 && g >= endFrame) {
                        b.anteil = (endFrame + 1) - g;
                        b.bild = ende;
                        b.folge = start;
                    } else {
                        if (g <= endFrame + 1) { g = endFrame + std::fmod(g - endFrame, static_cast<float>(animSize)) - static_cast<float>(animSize); }
                        b.anteil = std::ceil(g) - g;
                        b.bild = static_cast<int>(std::ceil(g));
                        b.folge = b.bild <= ende + 1 ? start : b.bild - 1;
                    }
                } else {
                    if (g > endFrame - 1 && g < endFrame) {
                        b.anteil = g - static_cast<float>(static_cast<int>(g));
                        b.bild = static_cast<int>(g);
                        b.folge = start;
                    } else {
                        if (g >= endFrame) { g = endFrame + std::fmod(g - endFrame, static_cast<float>(animSize)) - static_cast<float>(animSize); }
                        b.anteil = g - static_cast<float>(static_cast<int>(g));
                        b.bild = static_cast<int>(g);
                        b.folge = g >= endFrame - 1 ? start : b.bild + 1;
                    }
                }
            } else {
                b.aus = true;   // BONE_ANIM_OVERRIDE ohne FREEZE: weggenommen
                b.bild = b.folge = 0;
                b.anteil = 0;
            }
        } else if (animSpeed > 0.0F) {
            b.bild = static_cast<int>(g);
            b.anteil = g - static_cast<float>(b.bild);
            b.folge = b.bild + 1;
            if (b.folge >= ende) { b.folge = schleife ? start : ende - 1; }
        } else {
            b.anteil = std::ceil(g) - g;
            b.bild = static_cast<int>(std::ceil(g));
            if (b.bild > start) {
                b.bild = start;
                b.folge = b.bild;
                b.anteil = 0.0F;
            } else {
                b.folge = b.bild - 1;
                if (b.folge < ende + 1) { b.folge = schleife ? start : ende + 1; }
            }
        }
    } else {
        b.bild = animSpeed < 0.0F ? ende + 1 : ende - 1;
        if (b.bild < 0) { b.bild = 0; }
        b.folge = b.bild;
        b.anteil = 0;
    }
    return b;
}

// --- Klingen ------------------------------------------------------------------------

bool uiKlingeZeichnen(int nummer, int bladeStyle2Start, bool noBlade, bool noBlade2) {
    return (bladeStyle2Start != 0 && nummer >= bladeStyle2Start) ? !noBlade2 : !noBlade;
}

bool uiKlingenStrecke(const GlmModel& griff, const std::string& saberTyp, int nummer, float xscale, float org[3], float dir[3]) {
    std::string typ = saberTyp;
    for (char& c : typ) { c = static_cast<char>(std::toupper(static_cast<unsigned char>(c))); }
    if (typ == "SABER_SITH_SWORD") { return false; }   // draw no blade
    bool tagHack = false;
    int bs = surfaceIndex(griff, "*blade" + std::to_string(nummer + 1));
    if (bs < 0) {
        tagHack = true;
        bs = surfaceIndex(griff, "*flash");
    }
    float ax[3][3] = {{-1, 0, 0}, {0, -1, 0}, {0, 0, 1}};   // ohne Bolzen: Einheitsmatrix
    float o[3] = {0, 0, 0};
    BoneMatrix m{};
    if (bs >= 0 && boltMatrixRigid(griff, bs, m)) {
        for (int k = 0; k < 3; ++k) {
            o[k] = m.m[k][3];
            ax[0][k] = -m.m[k][0];   // NEGATIVE_X: front
            ax[1][k] = -m.m[k][1];   // NEGATIVE_Y: right
            ax[2][k] = m.m[k][2];    // POSITIVE_Z: up
        }
    }
    const float s = xscale;
    auto ma = [&](float um, const float* a) {
        for (int k = 0; k < 3; ++k) { o[k] += um * a[k]; }
    };
    // VectorScale(axis[ziel], fa); VectorScale(axis[b], fb); VectorAdd(axis[ziel], axis[b], axis[ziel])
    auto mische = [&](int ziel, float fa, int b, float fb) {
        for (int k = 0; k < 3; ++k) {
            ax[b][k] *= fb;
            ax[ziel][k] = ax[ziel][k] * fa + ax[b][k];
        }
    };
    if (tagHack) {
        if (typ == "SABER_STAFF") {
            if (nummer == 1) {
                for (float& v : ax[0]) { v = -v; }
                ma(16 * s, ax[0]);
            }
        } else if (typ == "SABER_BROAD") {
            if (nummer == 0) { ma(-1 * s, ax[1]); }
            else if (nummer == 1) { ma(1 * s, ax[1]); }
        } else if (typ == "SABER_PRONG") {
            if (nummer == 0) { ma(-3 * s, ax[1]); }
            else if (nummer == 1) { ma(3 * s, ax[1]); }
        } else if (typ == "SABER_ARC") {
            for (int k = 0; k < 3; ++k) { ax[1][k] -= ax[2][k]; }
            const float l = std::sqrt(ax[1][0] * ax[1][0] + ax[1][1] * ax[1][1] + ax[1][2] * ax[1][2]);
            if (l > 0) {
                for (float& v : ax[1]) { v /= l; }
            }
            switch (nummer) {
                case 0: ma(8 * s, ax[0]); mische(0, 0.75F, 1, 0.25F); break;
                case 1: mische(0, 0.25F, 1, 0.75F); break;
                case 2: ma(-8 * s, ax[0]); mische(0, -0.25F, 1, 0.75F); break;
                case 3: ma(-16 * s, ax[0]); mische(0, -0.75F, 1, 0.25F); break;
                default: break;
            }
        } else if (typ == "SABER_SAI") {
            if (nummer == 1) { ma(-3 * s, ax[1]); }
            else if (nummer == 2) { ma(3 * s, ax[1]); }
        } else if (typ == "SABER_CLAW") {
            if (nummer >= 0 && nummer <= 2) {
                ma(2 * s, ax[0]);
                ma(2 * s, ax[2]);
                if (nummer == 1) { ma(2 * s, ax[1]); }
                if (nummer == 2) { ma(-2 * s, ax[1]); }
            }
        } else if (typ == "SABER_STAR") {
            switch (nummer) {
                case 0: ma(8 * s, ax[0]); break;
                case 1: mische(0, 0.33F, 2, 0.67F); ma(8 * s, ax[0]); break;
                case 2: mische(0, -0.33F, 2, 0.67F); ma(8 * s, ax[0]); break;
                case 3:
                    for (float& v : ax[0]) { v = -v; }
                    ma(8 * s, ax[0]);
                    break;
                case 4: mische(0, -0.33F, 2, -0.67F); ma(8 * s, ax[0]); break;
                case 5: mische(0, 0.33F, 2, -0.67F); ma(8 * s, ax[0]); break;
                default: break;
            }
        } else if (typ == "SABER_TRIDENT") {
            switch (nummer) {
                case 0: ma(24 * s, ax[0]); break;
                case 1: ma(-6 * s, ax[1]); ma(24 * s, ax[0]); break;
                case 2: ma(6 * s, ax[1]); ma(24 * s, ax[0]); break;
                case 3:
                    ma(-32 * s, ax[0]);
                    for (float& v : ax[0]) { v = -v; }
                    break;
                default: break;
            }
        }
    }
    for (int k = 0; k < 3; ++k) {
        org[k] = o[k];
        dir[k] = ax[0][k];
    }
    return true;
}

UiKlingenMasse uiKlingenMasseOpenJK(float laenge, float laengeMax, float radius) {
    UiKlingenMasse m;
    if (laenge < 0.5F) { return m; }   // if the thing is so short, just forget even adding me
    m.da = true;
    const float radiusmult = laenge < laengeMax ? 1.0F + (2.0F / laenge) : 1.0F;
    const float radiusRange = radius * 0.075F;
    const float radiusStart = radius - radiusRange;
    m.glanz = true;
    m.glanzLaenge = laenge;
    m.glanzRadius = radiusStart * radiusmult;
    m.kernRadius = (radius / 3.0F) * radiusmult;
    m.griffRadius = 5.5F + 0.5F * 0.25F;
    return m;
}

namespace {
// Erzeugt aus Movie Duels code/ui/ui_saber.cpp: je Art (cg_SFXSabers 0..11, dann UI_DoSaberUnstable)
// und Farbe (rot, orange, gelb, gruen, blau, lila, unstable_red, schwarz, weiss, custom, rgb, sonst) Glanz und Kern.
struct MdShaderPaar {
    const char* glanz;
    const char* kern;
};
const MdShaderPaar kMdShader[13][12] = {
    {  // UI_DoSaber
        {"gfx/effects/sabers/red_glow", "gfx/effects/sabers/red_line"},
        {"gfx/effects/sabers/orange_glow", "gfx/effects/sabers/orange_line"},
        {"gfx/effects/sabers/yellow_glow", "gfx/effects/sabers/yellow_line"},
        {"gfx/effects/sabers/green_glow", "gfx/effects/sabers/green_line"},
        {"gfx/effects/sabers/blue_glow", "gfx/effects/sabers/blue_line"},
        {"gfx/effects/sabers/purple_glow", "gfx/effects/sabers/purple_line"},
        {"gfx/effects/sabers/red_glow", "gfx/effects/sabers_tfa/unstable_line"},
        {"gfx/effects/sabers/black_glow", "gfx/effects/sabers/black_line"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers/rgb_line"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers/rgb_line"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers/rgb_line"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers/rgb_line"},
    },
    {  // UI_DoEp1Saber
        {"gfx/effects/sabers_ep1/red_glow", "gfx/effects/sabers_ep1/rgb_line"},
        {"gfx/effects/sabers_ep1/orange_glow", "gfx/effects/sabers_ep1/rgb_line"},
        {"gfx/effects/sabers_ep1/yellow_glow", "gfx/effects/sabers_ep1/rgb_line"},
        {"gfx/effects/sabers_ep1/green_glow", "gfx/effects/sabers_ep1/rgb_line"},
        {"gfx/effects/sabers_ep1/blue_glow", "gfx/effects/sabers_ep1/rgb_line"},
        {"gfx/effects/sabers_ep1/purple_glow", "gfx/effects/sabers_ep1/rgb_line"},
        {"gfx/effects/sabers_ep1/red_glow", "gfx/effects/sabers_tfa/unstable_line"},
        {"gfx/effects/sabers/black_glow", "gfx/effects/sabers_ep1/black_line"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers_ep1/rgb_line"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers_ep1/rgb_line"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers_ep1/rgb_line"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers_ep1/rgb_line"},
    },
    {  // UI_DoEp2Saber
        {"gfx/effects/sabers_ep2/red_glow", "gfx/effects/sabers_ep2/rgb_line"},
        {"gfx/effects/sabers_ep2/orange_glow", "gfx/effects/sabers_ep2/rgb_line"},
        {"gfx/effects/sabers_ep2/yellow_glow", "gfx/effects/sabers_ep2/rgb_line"},
        {"gfx/effects/sabers_ep2/green_glow", "gfx/effects/sabers_ep2/rgb_line"},
        {"gfx/effects/sabers_ep2/blue_glow", "gfx/effects/sabers_ep2/rgb_line"},
        {"gfx/effects/sabers_ep2/purple_glow", "gfx/effects/sabers_ep2/rgb_line"},
        {"gfx/effects/sabers_ep2/red_glow", "gfx/effects/sabers_tfa/unstable_line"},
        {"gfx/effects/sabers/black_glow", "gfx/effects/sabers_ep2/black_line"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers_ep2/rgb_line"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers_ep2/rgb_line"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers_ep2/rgb_line"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers_ep2/rgb_line"},
    },
    {  // UI_DoEp3Saber
        {"gfx/effects/sabers_ep3/red_glow", "gfx/effects/sabers_ep3/red_line"},
        {"gfx/effects/sabers_ep3/orange_glow", "gfx/effects/sabers_ep3/orange_line"},
        {"gfx/effects/sabers_ep3/yellow_glow", "gfx/effects/sabers_ep3/yellow_line"},
        {"gfx/effects/sabers_ep3/green_glow", "gfx/effects/sabers_ep3/green_line"},
        {"gfx/effects/sabers_ep3/blue_glow", "gfx/effects/sabers_ep3/blue_line"},
        {"gfx/effects/sabers_ep3/purple_glow", "gfx/effects/sabers_ep3/purple_line"},
        {"gfx/effects/sabers_ep3/red_glow", "gfx/effects/sabers_tfa/unstable_line"},
        {"gfx/effects/sabers/black_glow", "gfx/effects/sabers_ep3/black_line"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers_ep3/rgb_line"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers_ep3/rgb_line"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers_ep3/rgb_line"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers_ep3/rgb_line"},
    },
    {  // UI_DoSFXSaber
        {"gfx/effects/sabers/red_glow", "SFX_Sabers/saber_blade"},
        {"gfx/effects/sabers/orange_glow", "SFX_Sabers/saber_blade"},
        {"gfx/effects/sabers/yellow_glow", "SFX_Sabers/saber_blade"},
        {"gfx/effects/sabers/green_glow", "SFX_Sabers/saber_blade"},
        {"gfx/effects/sabers/blue_glow", "SFX_Sabers/saber_blade"},
        {"gfx/effects/sabers/purple_glow", "SFX_Sabers/saber_blade"},
        {"gfx/effects/sabers/red_glow", "gfx/effects/sabers_tfa/unstable_line"},
        {"gfx/effects/sabers/black_glow", "SFX_Sabers/saber_blade_black"},
        {"gfx/effects/sabers/rgb_glow", "SFX_Sabers/saber_blade"},
        {"gfx/effects/sabers/rgb_glow", "SFX_Sabers/saber_blade"},
        {"gfx/effects/sabers/rgb_glow", "SFX_Sabers/saber_blade"},
        {"gfx/effects/sabers/rgb_glow", "SFX_Sabers/saber_blade"},
    },
    {  // UI_DoOTSaber
        {"gfx/effects/sabers_ot/ot_redGlow", "gfx/effects/sabers_ot/ot_saberCore"},
        {"gfx/effects/sabers_ot/ot_orangeGlow", "gfx/effects/sabers_ot/ot_saberCore"},
        {"gfx/effects/sabers_ot/ot_yellowGlow", "gfx/effects/sabers_ot/ot_saberCore"},
        {"gfx/effects/sabers_ot/ot_greenGlow", "gfx/effects/sabers_ot/ot_saberCore"},
        {"gfx/effects/sabers_ot/ot_blueGlow", "gfx/effects/sabers_ot/ot_saberCore"},
        {"gfx/effects/sabers_ot/ot_purpleGlow", "gfx/effects/sabers_ot/ot_saberCore"},
        {"gfx/effects/sabers_ot/ot_redGlow", "gfx/effects/sabers_tfa/unstable_line"},
        {"gfx/effects/sabers/black_glow", "gfx/effects/sabers_ot/ot_saberBlackCore"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers_ot/ot_saberCore"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers_ot/ot_saberCore"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers_ot/ot_saberCore"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers_ot/ot_saberCore"},
    },
    {  // UI_DoRotJSaber
        {"gfx/effects/sabers_rotj/red_glow", "gfx/effects/sabers_rotj/rgb_line"},
        {"gfx/effects/sabers_rotj/orange_glow", "gfx/effects/sabers_rotj/rgb_line"},
        {"gfx/effects/sabers_rotj/yellow_glow", "gfx/effects/sabers_rotj/rgb_line"},
        {"gfx/effects/sabers_rotj/green_glow", "gfx/effects/sabers_rotj/rgb_line"},
        {"gfx/effects/sabers_rotj/blue_glow", "gfx/effects/sabers_rotj/rgb_line"},
        {"gfx/effects/sabers_rotj/purple_glow", "gfx/effects/sabers_rotj/rgb_line"},
        {"gfx/effects/sabers_rotj/red_glow", "gfx/effects/sabers_tfa/unstable_line"},
        {"gfx/effects/sabers/black_glow", "gfx/effects/sabers_rotj/black_line"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers_rotj/rgb_line"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers_rotj/rgb_line"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers_rotj/rgb_line"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers_rotj/rgb_line"},
    },
    {  // UI_DoTFASaber
        {"gfx/effects/sabers_tfa/red_glow", "gfx/effects/sabers_tfa/red_line"},
        {"gfx/effects/sabers_tfa/orange_glow", "gfx/effects/sabers_tfa/orange_line"},
        {"gfx/effects/sabers_tfa/yellow_glow", "gfx/effects/sabers_tfa/yellow_line"},
        {"gfx/effects/sabers_tfa/green_glow", "gfx/effects/sabers_tfa/green_line"},
        {"gfx/effects/sabers_tfa/blue_glow", "gfx/effects/sabers_tfa/blue_line"},
        {"gfx/effects/sabers_tfa/purple_glow", "gfx/effects/sabers_tfa/purple_line"},
        {"gfx/effects/sabers_tfa/red_glow", "gfx/effects/sabers_tfa/unstable_line"},
        {"gfx/effects/sabers/black_glow", "gfx/effects/sabers_tfa/black_line"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers_tfa/rgb_line"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers_tfa/rgb_line"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers_tfa/rgb_line"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers_tfa/rgb_line"},
    },
    {  // UI_DoUnstableSaber
        {"gfx/effects/sabers/red_glow", "gfx/effects/sabers_tfa/red_unstable_line"},
        {"gfx/effects/sabers/orange_glow", "gfx/effects/sabers_tfa/red_unstable_line"},
        {"gfx/effects/sabers/yellow_glow", "gfx/effects/sabers_tfa/red_unstable_line"},
        {"gfx/effects/sabers/green_glow", "gfx/effects/sabers_tfa/red_unstable_line"},
        {"gfx/effects/sabers/blue_glow", "gfx/effects/sabers_tfa/red_unstable_line"},
        {"gfx/effects/sabers/purple_glow", "gfx/effects/sabers_tfa/red_unstable_line"},
        {"gfx/effects/sabers/red_glow", "gfx/effects/sabers_tfa/unstable_line"},
        {"gfx/effects/sabers/black_glow", "gfx/effects/sabers/black_line"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers_tfa/red_unstable_line"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers_tfa/red_unstable_line"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers_tfa/red_unstable_line"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers_tfa/red_unstable_line"},
    },
    {  // UI_DoRebelsSaber
        {"gfx/effects/sabers_rebels/red_glow", "gfx/effects/sabers_rebels/red_line"},
        {"gfx/effects/sabers_rebels/orange_glow", "gfx/effects/sabers_rebels/orange_line"},
        {"gfx/effects/sabers_rebels/yellow_glow", "gfx/effects/sabers_rebels/yellow_line"},
        {"gfx/effects/sabers_rebels/green_glow", "gfx/effects/sabers_rebels/green_line"},
        {"gfx/effects/sabers_rebels/blue_glow", "gfx/effects/sabers_rebels/blue_line"},
        {"gfx/effects/sabers_rebels/purple_glow", "gfx/effects/sabers_rebels/purple_line"},
        {"gfx/effects/sabers/red_glow", "gfx/effects/sabers_tfa/unstable_line"},
        {"gfx/effects/sabers/black_glow", "gfx/effects/sabers_rebels/black_line"},
        {"gfx/effects/sabers_rebels/rgb_glow", "gfx/effects/sabers_rebels/rgb_line"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers/rgb_line"},
        {"gfx/effects/sabers_rebels/rgb_glow", "gfx/effects/sabers_rebels/rgb_line"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers/rgb_line"},
    },
    {  // UI_DoCWSaber
        {"gfx/effects/sabers_tcw/red_glow", "gfx/effects/sabers_tcw/red_line"},
        {"gfx/effects/sabers_tcw/orange_glow", "gfx/effects/sabers_tcw/orange_line"},
        {"gfx/effects/sabers_tcw/yellow_glow", "gfx/effects/sabers_tcw/yellow_line"},
        {"gfx/effects/sabers_tcw/green_glow", "gfx/effects/sabers_tcw/green_line"},
        {"gfx/effects/sabers_tcw/blue_glow", "gfx/effects/sabers_tcw/blue_line"},
        {"gfx/effects/sabers_tcw/purple_glow", "gfx/effects/sabers_tcw/purple_line"},
        {"gfx/effects/sabers/red_glow", "gfx/effects/sabers_tfa/unstable_line"},
        {"gfx/effects/sabers/black_glow", "gfx/effects/sabers_tcw/black_line"},
        {"gfx/effects/sabers_tcw/rgb_glow", "gfx/effects/sabers_tcw/rgb_line"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers/rgb_line"},
        {"gfx/effects/sabers_tcw/rgb_glow", "gfx/effects/sabers_tcw/rgb_line"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers/rgb_line"},
    },
    {  // UI_DoMaulSaber
        {"gfx/effects/sabers_maul/red_glow", "gfx/effects/sabers_maul/red_line"},
        {"gfx/effects/sabers_maul/orange_glow", "gfx/effects/sabers_maul/orange_line"},
        {"gfx/effects/sabers_maul/yellow_glow", "gfx/effects/sabers_maul/yellow_line"},
        {"gfx/effects/sabers_maul/green_glow", "gfx/effects/sabers_maul/green_line"},
        {"gfx/effects/sabers_maul/blue_glow", "gfx/effects/sabers_maul/blue_line"},
        {"gfx/effects/sabers_maul/purple_glow", "gfx/effects/sabers_maul/purple_line"},
        {"gfx/effects/sabers/red_glow", "gfx/effects/sabers_tfa/unstable_line"},
        {"gfx/effects/sabers/black_glow", "gfx/effects/sabers_maul/black_line"},
        {"gfx/effects/sabers_maul/rgb_glow", "gfx/effects/sabers_maul/rgb_line"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers/rgb_line"},
        {"gfx/effects/sabers_maul/rgb_glow", "gfx/effects/sabers_maul/rgb_line"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers/rgb_line"},
    },
    {  // UI_DoSaberUnstable
        {"gfx/effects/sabers/red_glow", "gfx/effects/sabers_tfa/unstable_line"},
        {"gfx/effects/sabers/orange_glow", "gfx/effects/sabers_tfa/unstable_line"},
        {"gfx/effects/sabers/yellow_glow", "gfx/effects/sabers_tfa/unstable_line"},
        {"gfx/effects/sabers/green_glow", "gfx/effects/sabers_tfa/unstable_line"},
        {"gfx/effects/sabers/blue_glow", "gfx/effects/sabers_tfa/unstable_line"},
        {"gfx/effects/sabers/purple_glow", "gfx/effects/sabers_tfa/unstable_line"},
        {"gfx/effects/sabers/red_glow", "gfx/effects/sabers_tfa/unstable_line"},
        {"gfx/effects/sabers/black_glow", "gfx/effects/sabers/black_line"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers_tfa/unstable_line"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers_tfa/unstable_line"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers_tfa/unstable_line"},
        {"gfx/effects/sabers/rgb_glow", "gfx/effects/sabers_tfa/unstable_line"},
    },
};

// Glanz- und Kernfaktor, Zufall (Mitte) und Form je Art (UI_Do*Saber nach memset(&saber))
struct MdArt {
    const char* cvar;     // Endung der Groessen-Cvars, nullptr = keine
    float glanzFaktor;    // effectradius *= ...
    float kernFaktor;     // coreradius *= ...
    float glanzZufall;    // Mitte des Zufalls zu radius * 1.6
    float kernZufall;     // Mitte des Zufalls zu radius * 0.4
    bool halbRadius;      // radiusmult = 0.5 + laenge / laengeMax / 2 (sonst 1 + 2 / laenge)
    int kernMal;
    bool versatz;         // VectorMA(origin, effectradius * 0.25f, dir, saber.origin)
    const char* kern2;    // zweiter Kern bei SABER_RGB
};
const MdArt kMdArt[13] = {
    {"", 0.75F, 1.0F, 0, 0, false, 1, false, "gfx/effects/sabers/rgb_line"},              // UI_DoSaber
    {"EP1", 0.75F, 1.4F, 0, 0, true, 2, false, nullptr},                                    // UI_DoEp1Saber
    {"EP2", 0.75F, 1.3F, 0, 0, true, 2, false, nullptr},                                    // UI_DoEp2Saber
    {"EP3", 0.75F, 0.85F, 0, 0, true, 2, false, nullptr},                                   // UI_DoEp3Saber
    {"SFX", 0.75F, 1.15F, 0, 0, true, 2, false, nullptr},                                   // UI_DoSFXSaber
    {"OT", 1.0F, 0.5F, (0.3F + 1.8F) / 2, (0.1F + 1.0F) / 2, true, 2, false, nullptr},      // UI_DoOTSaber: flrand(0.3, 1.8), flrand(0.1, 1)
    {"ROTJ", 0.75F, 0.7F, 0, 0, true, 2, false, nullptr},                                   // UI_DoRotJSaber
    {"TFA", 0.8F, 1.0F, 0, 0, true, 2, false, nullptr},                                     // UI_DoTFASaber
    {"USB", 0.75F, 1.0F, 0, 0, true, 2, false, nullptr},                                    // UI_DoUnstableSaber
    {"Rebels", 0.5F, 0.85F, 0, 0, true, 1, true, "gfx/effects/sabers_rebels/rgb_line"},    // UI_DoRebelsSaber
    {"CW", 0.42F, 0.85F, 0, 0, true, 1, true, "gfx/effects/sabers_tcw/rgb_line"},          // UI_DoCWSaber
    {"Maul", 0.42F, 0.85F, 0, 0, true, 1, true, "gfx/effects/sabers_maul/rgb_line"},       // UI_DoMaulSaber
    {nullptr, 1.0F, 1.0F, 0, 0, true, 2, false, nullptr},                                   // UI_DoSaberUnstable
};
}  // namespace

int mdKlingenArt(int sfxSabers, const std::string& saberTyp) {
    std::string t = saberTyp;
    for (char& c : t) { c = static_cast<char>(std::toupper(static_cast<unsigned char>(c))); }
    if (t == "SABER_SITH_SWORD" || sfxSabers > 11) { return -1; }
    if (t == "SABER_UNSTABLE" || t == "SABER_STAFF_UNSTABLE" || t == "SABER_STAFF_ELECTROSTAFF" || t == "SABER_SINGLE_KYLO_REN" ||
        t == "SABER_ELECTROSTAFF") {
        return kMdKlingeInstabil;
    }
    return sfxSabers < 1 ? 0 : sfxSabers;
}

std::string mdGroessenCvar(int art, bool kern) {
    if (art < 0 || art > kMdKlingeInstabil || kMdArt[art].cvar == nullptr) { return {}; }
    return std::string(kern ? "cg_SFXSabersCoreSize" : "cg_SFXSabersGlowSize") + kMdArt[art].cvar;
}

MdKlingenShader mdKlingenShader(int art, const std::string& farbe, bool serenity) {
    MdKlingenShader m;
    if (art < 0 || art > kMdKlingeInstabil) { return m; }
    // TranslateSaberColor (Q_stricmp); "random" waehlt im Spiel orange..lila - hier die Mitte (gruen)
    static const char* const namen[] = {"red", "orange", "yellow", "green", "blue", "purple", "unstable_red", "black", "white", "custom", "rgb"};
    const std::string f = klein(farbe);
    int idx = 11;   // RGB-Werte (Q_parseSaberColor): >= SABER_RGB, switch -> default
    for (int i = 0; i < 11; ++i) {
        if (f == namen[i]) { idx = i; }
    }
    if (f == "random") { idx = 3; }
    m.rgbWerte = idx >= 10;
    const MdShaderPaar& p = kMdShader[art][idx];
    m.glanz = p.glanz;
    m.kern = p.kern;
    if (art == 8 && serenity && m.kern == "gfx/effects/sabers_tfa/red_unstable_line") { m.kern = "gfx/effects/sabers_tfa/unstable_line"; }
    if (idx == 10 && kMdArt[art].kern2 != nullptr) { m.kern2 = kMdArt[art].kern2; }
    return m;
}

UiKlingenMasse uiKlingenMasseMD(int art, float laenge, float laengeMax, float radius, float glanzGroesse, float kernGroesse) {
    UiKlingenMasse m;
    if (art < 0 || art > kMdKlingeInstabil || laenge < 0.5F) { return m; }
    const MdArt& a = kMdArt[art];
    m.da = true;
    float radiusmult = 1.0F;
    if (laenge < laengeMax) { radiusmult = a.halbRadius ? 0.5F + laenge / laengeMax / 2.0F : 1.0F + 2.0F / laenge; }
    if (a.cvar == nullptr) { glanzGroesse = kernGroesse = 1.0F; }
    const float glanz = (radius * 1.6F + a.glanzZufall) * radiusmult * glanzGroesse * a.glanzFaktor;
    m.kernRadius = (radius * 0.4F + a.kernZufall) * radiusmult * kernGroesse * a.kernFaktor;
    m.kernMal = a.kernMal;
    if (laenge - glanz / 2.0F > 0.0F) {
        m.glanz = true;
        m.glanzRadius = glanz;
        m.glanzLaenge = laenge - glanz / 2.0F;
        m.glanzVersatz = a.versatz ? glanz * 0.25F : 0.0F;
        m.glanzStaerke = static_cast<float>(static_cast<unsigned char>(0xff * 0.8F)) / 255.0F;   // shaderRGBA = 0xff * effectalpha
    }
    m.griffRadius = 2.5F + 0.25F * 0.15F;
    return m;
}

void uiSaberVierecke(const float org[3], const float dir[3], float laenge, const UiKlingenMasse& m, const float auge[3], const float links[3],
                     const float oben[3], std::vector<UiKlingenViereck>& aus) {
    if (!m.da) { return; }
    // DoSprite(origin, radius, 0): left = axis[1] * r, up = axis[2] * r; RB_AddQuadStamp
    auto sprite = [&](const float o[3], float r) {
        UiKlingenViereck v;
        for (int k = 0; k < 3; ++k) {
            const float l = links[k] * r, u = oben[k] * r;
            v.ecke[0][k] = o[k] + l + u;   // s1 t1
            v.ecke[1][k] = o[k] - l + u;   // s2 t1
            v.ecke[2][k] = o[k] - l - u;   // s2 t2
            v.ecke[3][k] = o[k] + l - u;   // s1 t2
        }
        aus.push_back(v);
    };
    // RT_SABER_GLOW (RB_SurfaceSaberGlow); ohne Radius kaeme die Schleife nie voran
    if (m.glanz) {
        float g[3];   // saber.origin des Glanzes (Movie Duels Rebels/CW/Maul: um glanzVersatz vorn)
        for (int k = 0; k < 3; ++k) { g[k] = org[k] + m.glanzVersatz * dir[k]; }
        float r = m.glanzRadius;
        if (r > 0.0F) {
            for (float i = m.glanzLaenge; i > 0; i -= r * 0.65F) {
                float end[3];
                for (int k = 0; k < 3; ++k) { end[k] = g[k] + i * dir[k]; }
                sprite(end, r);
                r += 0.017F;
            }
        }
        sprite(g, m.griffRadius);   // Big hilt sprite
    }
    // RT_LINE (RB_SurfaceLine, DoLine): start = Spitze, end = 1 hinter der Wurzel
    float start[3], end[3], v1[3], v2[3];
    for (int k = 0; k < 3; ++k) {
        start[k] = org[k] + laenge * dir[k];
        end[k] = org[k] - dir[k];
        v1[k] = start[k] - auge[k];
        v2[k] = end[k] - auge[k];
    }
    float rechts[3] = {v1[1] * v2[2] - v1[2] * v2[1], v1[2] * v2[0] - v1[0] * v2[2], v1[0] * v2[1] - v1[1] * v2[0]};
    const float l = std::sqrt(rechts[0] * rechts[0] + rechts[1] * rechts[1] + rechts[2] * rechts[2]);
    if (l > 0) {
        for (float& x : rechts) { x /= l; }
    }
    const float w = m.kernRadius;
    UiKlingenViereck v;
    v.kern = true;
    for (int k = 0; k < 3; ++k) {
        v.ecke[0][k] = start[k] + w * rechts[k];   // 0 0
        v.ecke[1][k] = start[k] - w * rechts[k];   // 1 0
        v.ecke[2][k] = end[k] - w * rechts[k];     // 1 1
        v.ecke[3][k] = end[k] + w * rechts[k];     // 0 1
    }
    for (int n = 0; n < m.kernMal; ++n) { aus.push_back(v); }
}

}  // namespace bhed::menue
