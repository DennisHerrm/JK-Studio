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

MenueModellBild menueModellBild(const MenueModell& m, const Item& it, int zeitMs, int breite, int hoehe, std::vector<BoneMatrix>& weltKnochen) {
    MenueModellBild b;
    // Grenzen (ghoul2 hat keine: g2mins/g2maxs oder die Vorgabe)
    float mins[3] = {it.g2mins[0], it.g2mins[1], it.g2mins[2]};
    float maxs[3] = {it.g2maxs[0], it.g2maxs[1], it.g2maxs[2]};
    if (mins[0] == 0 && mins[1] == 0 && mins[2] == 0 && maxs[0] == 0 && maxs[1] == 0 && maxs[2] == 0) {
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
    const float fovX = it.fovx != 0 ? it.fovx : static_cast<float>(static_cast<int>(w / 640.0F * 90.0F));
    const float fovY = it.fovy != 0 ? it.fovy : std::atan2(h, w / std::tan(fovX / 360.0F * kPi)) * (360.0F / kPi);
    const float len = 0.5F * (maxs[2] - mins[2]);
    origin[0] = len / 0.268F;
    float yaw = static_cast<float>(it.angle);
    if (it.rotationSpeed != 0) { yaw += static_cast<float>(zeitMs) / static_cast<float>(it.rotationSpeed); }
    b.yawGrad = yaw;
    // "hack to put saber on it's side": ITF_ISANYSABER ohne ITF_ISCHARACTER -> (curYaw, 0, 90)
    float winkel[3] = {0, yaw, 0};
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
    float amb = 150.0F + 32.0F;
    float dirLicht = 150.0F;
    float ld[3] = {0, 0, 0};   // tr.sunDirection * |directedLight|: ohne Karte ist die Sonne null
    {
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
    // Knochen: model_g2anim aus der animation.cfg, sonst Bild 0
    weltKnochen.clear();
    if (!m.gla.empty()) {
        int bild = 0;
        if (!it.g2anim.empty() && !m.anims.empty()) {
            bild = frameForAnimationIn(m.anims, it.g2anim, static_cast<double>(zeitMs));
            if (bild < 0) { bild = 0; }
        }
        b.bild = std::clamp(bild, 0, m.gla.numFrames - 1);
        m.gla.worldMatrices(b.bild, weltKnochen);
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

UiKlingenMasse uiKlingenMasseMD(int stil, float laenge, float laengeMax, float radius, float glanzGroesse, float kernGroesse) {
    UiKlingenMasse m;
    if (laenge < 0.5F || (stil != 0 && stil != 4)) { return m; }
    m.da = true;
    float radiusmult = 1.0F;
    if (laenge < laengeMax) { radiusmult = stil == 0 ? 1.0F + 2.0F / laenge : 0.5F + laenge / laengeMax / 2.0F; }
    const float glanz = radius * 1.6F * radiusmult * glanzGroesse * 0.75F;
    m.kernRadius = radius * 0.4F * radiusmult * kernGroesse * (stil == 4 ? 1.15F : 1.0F);
    m.kernMal = stil == 4 ? 2 : 1;
    if (laenge - glanz / 2.0F > 0.0F) {
        m.glanz = true;
        m.glanzRadius = glanz;
        m.glanzLaenge = laenge - glanz / 2.0F;
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
        float r = m.glanzRadius;
        if (r > 0.0F) {
            for (float i = m.glanzLaenge; i > 0; i -= r * 0.65F) {
                float end[3];
                for (int k = 0; k < 3; ++k) { end[k] = org[k] + i * dir[k]; }
                sprite(end, r);
                r += 0.017F;
            }
        }
        sprite(org, m.griffRadius);   // Big hilt sprite
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
