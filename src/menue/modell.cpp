// modell.cpp - Menue-Editor M2 Punkt 1: ITEM_TYPE_MODEL zeichnen.
// Herkunft der Kamera (OpenJK code/ui/ui_shared.cpp Item_Model_Paint) und
// Lizenz siehe include/bhed/menue/modell.h; der Rasterer ist eigener Code.
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
bool istZeichen(const Item& it) { return klein(it.window.name) == "character"; }   // ITF_ISCHARACTER
std::string cvarOder(const CvarLeser& c, const char* n, const char* std) {
    std::string v = c ? c(n) : std::string();
    return v.empty() ? std::string(std) : v;
}
}  // namespace

std::string modellSchluessel(const Item& it, const CvarLeser& cvar) {
    if (istZeichen(it)) {
        // UI_UpdateCharacterSkin: models/players/%s/|%s|%s|%s
        const std::string m = cvarOder(cvar, "ui_char_model", "jedi_hf");
        return "models/players/" + m + "/model.glm|" + cvarOder(cvar, "ui_char_skin_head", "head_a1") + "|" +
               cvarOder(cvar, "ui_char_skin_torso", "torso_a1") + "|" + cvarOder(cvar, "ui_char_skin_legs", "lower_a1") + "|" + it.g2anim;
    }
    return it.assetModel + "|" + it.g2skin + "|" + it.g2anim;
}

bool ladeMenueModell(const Spieldaten& d, const Item& it, const CvarLeser& cvar, const ModellBildLader& bild, MenueModell& out) {
    out = MenueModell{};
    out.schluessel = modellSchluessel(it, cvar);
    std::string pfad = it.assetModel;
    std::vector<std::string> haeute;
    if (istZeichen(it)) {
        const std::string m = cvarOder(cvar, "ui_char_model", "jedi_hf");
        pfad = "models/players/" + m + "/model.glm";
        for (const char* teil : {"ui_char_skin_head", "ui_char_skin_torso", "ui_char_skin_legs"}) {
            const char* std = teil[12] == 'h' ? "head_a1" : teil[12] == 't' ? "torso_a1" : "lower_a1";
            haeute.push_back("models/players/" + m + "/" + cvarOder(cvar, teil, std) + ".skin");
        }
    } else if (!it.g2skin.empty()) {
        haeute.push_back(it.g2skin);
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
    if (haeute.empty()) {   // ohne Haut: model_default.skin wie die Engine
        const std::size_t sl = pfad.rfind('/');
        haeute.push_back(pfad.substr(0, sl + 1) + "model_default.skin");
    }
    for (const std::string& h : haeute) {
        std::string t;
        if (d.lies(h, t)) { applySkin(t, out.glm); }
    }
    // Skelett und animation.cfg
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
    out.texturen.resize(out.glm.surfaces.size());
    if (bild) {
        for (std::size_t i = 0; i < out.glm.surfaces.size(); ++i) {
            const GlmSurface& s = out.glm.surfaces[i];
            const std::string n = !s.texture.empty() ? s.texture : s.shader;
            if (!n.empty() && s.isVisible(false)) { (void)bild(n, out.texturen[i]); }
        }
    }
    out.ok = true;
    return true;
}

int zeichneMenueModell(const MenueModell& m, const Item& it, int zeitMs, int breite, int hoehe, std::vector<std::uint8_t>& rgba) {
    rgba.assign(static_cast<std::size_t>(std::max(0, breite * hoehe * 4)), 0);
    if (!m.ok || breite <= 0 || hoehe <= 0) { return 0; }
    // --- refdef wie Item_Model_Paint (x = rect.x + 1, w = rect.w - 2 ...) ---
    const float w = it.window.rect.w - 2;
    const float h = it.window.rect.h - 2;
    float mins[3] = {it.g2mins[0], it.g2mins[1], it.g2mins[2]};
    float maxs[3] = {it.g2maxs[0], it.g2maxs[1], it.g2maxs[2]};
    if (mins[0] == 0 && mins[1] == 0 && mins[2] == 0 && maxs[0] == 0 && maxs[1] == 0 && maxs[2] == 0) {
        mins[0] = -16; mins[1] = -16; mins[2] = -24;   // "we'll use defaults then I suppose"
        maxs[0] = 16; maxs[1] = 16; maxs[2] = 32;
    }
    float origin[3];
    origin[2] = -0.5F * (mins[2] + maxs[2]);
    origin[1] = 0.5F * (mins[1] + maxs[1]);
    const float pi = 3.14159265358979323846F;
    // refdef.width ist in Bildschirmpunkten (w * xscale); fov_x nach 640 umgerechnet
    const float fovX = it.fovx != 0 ? it.fovx : static_cast<float>(static_cast<int>(w / 640.0F * 90.0F));
    const float fovY = it.fovy != 0 ? it.fovy : std::atan2(h, w / std::tan(fovX / 360.0F * pi)) * (360.0F / pi);
    const float len = 0.5F * (maxs[2] - mins[2]);
    origin[0] = len / 0.268F;
    float yaw = static_cast<float>(it.angle);
    if (it.rotationSpeed != 0) { yaw += static_cast<float>(zeitMs) / static_cast<float>(it.rotationSpeed); }
    const float ya = yaw * pi / 180.0F;
    const float cy = std::cos(ya), sy = std::sin(ya);
    // --- Knochen ---------------------------------------------------------------------
    std::vector<BoneMatrix> knochen;
    if (!m.gla.empty()) {
        int frame = 0;
        if (!it.g2anim.empty() && !m.anims.empty()) {
            frame = frameForAnimationIn(m.anims, it.g2anim, static_cast<double>(zeitMs));
            if (frame < 0) { frame = 0; }
        }
        m.gla.worldMatrices(std::clamp(frame, 0, m.gla.numFrames - 1), knochen);
    }
    // --- Rasterer: Augpunkt 0, Achsen Quake (vorn +X, links +Y, oben +Z) ---------
    const float tx = std::tan(fovX / 360.0F * pi), ty = std::tan(fovY / 360.0F * pi);
    const float bw = static_cast<float>(breite), bh = static_cast<float>(hoehe);
    std::vector<float> tiefe(static_cast<std::size_t>(breite * hoehe), 1e30F);
    int dreiecke = 0;
    for (std::size_t si = 0; si < m.glm.surfaces.size(); ++si) {
        const GlmSurface& s = m.glm.surfaces[si];
        if (!s.isVisible(false)) { continue; }
        const MenueModell::Bild* tex = si < m.texturen.size() && !m.texturen[si].rgba.empty() ? &m.texturen[si] : nullptr;
        struct P {
            float sx, sy, z, u, v, licht;
        };
        std::vector<P> pv(s.verts.size());
        for (std::size_t vi = 0; vi < s.verts.size(); ++vi) {
            const GlmVertex& v = s.verts[vi];
            float p[3] = {0, 0, 0}, n[3] = {0, 0, 0};
            if (!knochen.empty()) {
                for (int k = 0; k < 4; ++k) {
                    if (v.weights[k] <= 0.0F || v.bones[k] >= knochen.size()) { continue; }
                    float q[3], r[3];
                    knochen[v.bones[k]].transform(v.xyz, q);
                    const BoneMatrix& b = knochen[v.bones[k]];
                    for (int a = 0; a < 3; ++a) {
                        r[a] = b.m[a][0] * v.normal[0] + b.m[a][1] * v.normal[1] + b.m[a][2] * v.normal[2];
                        p[a] += q[a] * v.weights[k];
                        n[a] += r[a] * v.weights[k];
                    }
                }
            } else {
                for (int a = 0; a < 3; ++a) {
                    p[a] = v.xyz[a];
                    n[a] = v.normal[a];
                }
            }
            // ent.axis aus angles (0, yaw, 0), dann + origin
            const float wx = p[0] * cy - p[1] * sy + origin[0];
            const float wy = p[0] * sy + p[1] * cy + origin[1];
            const float wz = p[2] + origin[2];
            const float nx = n[0] * cy - n[1] * sy;
            const float nl = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
            P& o = pv[vi];
            o.z = wx;
            o.sx = (0.5F - (wy / std::max(wx, 0.01F)) / tx * 0.5F) * bw;
            o.sy = (0.5F - (wz / std::max(wx, 0.01F)) / ty * 0.5F) * bh;
            o.u = v.st[0];
            o.v = v.st[1];
            // Licht am Augpunkt: Lambert (-nx zur Kamera) + Umgebung
            const float lam = nl > 0 ? std::max(0.0F, -nx / nl) : 0.0F;
            o.licht = std::min(1.0F, 0.45F + 0.75F * lam);
        }
        for (std::size_t t = 0; t + 2 < s.indexes.size(); t += 3) {
            const P& a = pv[s.indexes[t]];
            const P& b = pv[s.indexes[t + 1]];
            const P& c = pv[s.indexes[t + 2]];
            if (a.z <= 1.0F || b.z <= 1.0F || c.z <= 1.0F) { continue; }
            const float flaeche = (b.sx - a.sx) * (c.sy - a.sy) - (c.sx - a.sx) * (b.sy - a.sy);
            if (std::fabs(flaeche) < 1e-6F) { continue; }
            ++dreiecke;
            const int x0 = std::max(0, static_cast<int>(std::floor(std::min({a.sx, b.sx, c.sx}))));
            const int x1 = std::min(breite - 1, static_cast<int>(std::ceil(std::max({a.sx, b.sx, c.sx}))));
            const int y0 = std::max(0, static_cast<int>(std::floor(std::min({a.sy, b.sy, c.sy}))));
            const int y1 = std::min(hoehe - 1, static_cast<int>(std::ceil(std::max({a.sy, b.sy, c.sy}))));
            for (int y = y0; y <= y1; ++y) {
                for (int x = x0; x <= x1; ++x) {
                    const float px = static_cast<float>(x) + 0.5F, py = static_cast<float>(y) + 0.5F;
                    const float l0 = ((b.sx - px) * (c.sy - py) - (c.sx - px) * (b.sy - py)) / flaeche;
                    const float l1 = ((c.sx - px) * (a.sy - py) - (a.sx - px) * (c.sy - py)) / flaeche;
                    const float l2 = 1.0F - l0 - l1;
                    if (l0 < 0 || l1 < 0 || l2 < 0) { continue; }
                    // perspektivisch richtig: 1/z linear
                    const float iz = l0 / a.z + l1 / b.z + l2 / c.z;
                    const float z = 1.0F / iz;
                    float& zb = tiefe[static_cast<std::size_t>(y * breite + x)];
                    if (z >= zb) { continue; }
                    zb = z;
                    const float u = (l0 * a.u / a.z + l1 * b.u / b.z + l2 * c.u / c.z) * z;
                    const float v = (l0 * a.v / a.z + l1 * b.v / b.z + l2 * c.v / c.z) * z;
                    const float li = l0 * a.licht + l1 * b.licht + l2 * c.licht;
                    float col[3] = {226.0F, 220.0F, 208.0F};   // Ersatzfarbe des Modellfensters
                    if (tex != nullptr) {
                        float fu = u - std::floor(u), fv = v - std::floor(v);
                        const int ix = std::clamp(static_cast<int>(fu * static_cast<float>(tex->w)), 0, tex->w - 1);
                        const int iy = std::clamp(static_cast<int>(fv * static_cast<float>(tex->h)), 0, tex->h - 1);
                        const std::uint8_t* q = &tex->rgba[static_cast<std::size_t>((iy * tex->w + ix) * 4)];
                        col[0] = q[0];
                        col[1] = q[1];
                        col[2] = q[2];
                    }
                    std::uint8_t* o = &rgba[static_cast<std::size_t>((y * breite + x) * 4)];
                    for (int k = 0; k < 3; ++k) { o[k] = static_cast<std::uint8_t>(std::clamp(col[k] * li, 0.0F, 255.0F)); }
                    o[3] = 255;
                }
            }
        }
    }
    return dreiecke;
}

}  // namespace bhed::menue
