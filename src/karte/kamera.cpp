// kamera.cpp - Karten-Editor: die 3D-Kamera (CamWnd).
// Portiert aus NetRadiant-Custom 20260114 (Quellen in kamera.h).
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
#include "bhed/karte/kamera.h"

#include "bhed/karte/xyansicht.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

namespace bhed::karte {

namespace {

// g_radiant2opengl (camwindow.cpp:177-182)
const Matrix4 kRadiantZuOpenGl(0, -1, 0, 0, 0, 0, 1, 0, -1, 0, 0, 0, 0, 0, 0, 1);

// plane3_normalised
Ebene3d normiert(const Ebene3d& e) {
    const double r = 1.0 / std::sqrt((e.a * e.a) + (e.b * e.b) + (e.c * e.c));
    return Ebene3d(e.a * r, e.b * r, e.c * r, e.d * r);
}

// aabb_corners (aabb.h:187-200)
std::array<Vektor3f, 8> ecken(const Kasten& k) {
    const Vektor3f mn = k.origin - k.extents;
    const Vektor3f mx = k.origin + k.extents;
    return {Vektor3f(mn[0], mx[1], mx[2]), Vektor3f(mx[0], mx[1], mx[2]), Vektor3f(mx[0], mn[1], mx[2]),
            Vektor3f(mn[0], mn[1], mx[2]), Vektor3f(mn[0], mx[1], mn[2]), Vektor3f(mx[0], mx[1], mn[2]),
            Vektor3f(mx[0], mn[1], mn[2]), Vektor3f(mn[0], mn[1], mn[2])};
}

// Der Versatz fuer m_focus_offset / m_orbit_offset (Camera_getFocusPos,
// camera_orbit_scroll): ausserhalb 0..16 exponentiell.
float versetzt(float offset, int off) {
    if (off < 0 || off > 16) {
        return offset - (offset * static_cast<float>(off) / 8.0F *
                         std::pow(2.0F, static_cast<float>(off < 0 ? -off : off - 16) / 8.0F));
    }
    return offset - (offset * static_cast<float>(off) / 8.0F);
}

}  // namespace

// --- Projektion (camwindow.cpp:269-300, frustum.h:33-53) ---------------------

Matrix4 frustumMatrix(float links, float rechts, float unten, float oben, float nah, float fern) {
    return Matrix4((2 * nah) / (rechts - links), 0, 0, 0, 0, (2 * nah) / (oben - unten), 0, 0,
                   (rechts + links) / (rechts - links), (oben + unten) / (oben - unten), -(fern + nah) / (fern - nah), -1, 0,
                   0, -(2 * fern * nah) / (fern - nah), 0);
}

Matrix4 kameraProjektion(float nah, float fern, float sichtfeld, int breite, int hoehe) {
    float halbB = static_cast<float>(nah * std::tan(gradZuBogen(sichtfeld * 0.5)));
    const bool tauschen = hoehe > breite;
    if (tauschen) { std::swap(breite, hoehe); }
    float halbH = halbB * (static_cast<float>(hoehe) / static_cast<float>(breite));
    if (tauschen) { std::swap(halbB, halbH); }
    return frustumMatrix(-halbB, halbB, -halbH, halbH, nah, fern);
}

float fernEbene(const KameraEinstellungen& e) {
    return e.kubisch ? static_cast<float>(std::pow(2.0, (e.kubikSkala + 7) / 2.0))
                     : static_cast<float>((kMaxWeltKoord - kMinWeltKoord) * std::sqrt(3));
}

// frustum_from_viewproj (frustum.h:464-474)
Frustum frustumAusViewProj(const Matrix4& m) {
    const auto ebene = [&](int s, float vz) {
        return normiert(Ebene3d(m[3] + (vz * m[0 + s]), m[7] + (vz * m[4 + s]), m[11] + (vz * m[8 + s]), m[15] + (vz * m[12 + s])));
    };
    Frustum f;
    f.rechts = ebene(0, -1);
    f.links = ebene(0, 1);
    f.unten = ebene(1, 1);
    f.oben = ebene(1, -1);
    f.hinten = ebene(2, -1);
    f.vorn = ebene(2, 1);
    return f;
}

// --- Zustand (camwindow.cpp:302-364) ------------------------------------------

void Kamera::modelviewAktualisieren() {
    modelview = kEinheitsMatrix;
    // roll, pitch, yaw
    const Vektor3f euler(0, -winkel[0], winkel[1]);
    verschiebeUm(modelview, ursprung);
    multipliziere(modelview, drehungEulerXyzGrad(euler));
    multipliziere(modelview, kRadiantZuOpenGl);
    affinInvertieren(modelview);
    // Camera_updateVectors
    for (std::size_t i = 0; i < 3; ++i) {
        vright[i] = modelview[(i << 2) + 0];
        vup[i] = modelview[(i << 2) + 1];
        vpn[i] = modelview[(i << 2) + 2];
    }
}

void Kamera::projektionAktualisieren(const KameraEinstellungen& e) {
    projektion = kameraProjektion(kNahEbene, fernEbene(e), e.sichtfeld, breite, hoehe);
}

Matrix4 Kamera::bildfeld() const {
    Matrix4 v = kEinheitsMatrix;
    v[0] = static_cast<float>(breite / 2);
    v[5] = static_cast<float>(hoehe / 2);
    if (std::fabs(projektion[11]) > 0.0000001F) {
        v[10] = projektion[0] * v[0];
    } else {
        v[10] = 1 / projektion[10];
    }
    return v;
}

void Kamera::bewegungsAchsen() {
    const double ya = gradZuBogen(winkel[1]);
    // the movement matrix is kept 2d
    vorn[0] = static_cast<float>(std::cos(ya));
    vorn[1] = static_cast<float>(std::sin(ya));
    vorn[2] = 0;
    seite[0] = vorn[1];
    seite[1] = -vorn[0];
}

void Kamera::freiAchsen() {
    seite = vright;
    vorn = -vpn;
}

void Kamera::setzeUrsprung(const Vektor3f& o) {
    ursprung = o;
    modelviewAktualisieren();
}

void Kamera::setzeWinkel(const Vektor3f& w) {
    winkel = w;
    modelviewAktualisieren();
}

// --- Freelook (camwindow.cpp:367-434, 715-752) -------------------------------

void Kamera::strafeAusMaus(bool alt, bool strg, bool shift, bool linksGedrueckt, bool rechtsGedrueckt, int strafeModus) {
    orbit = alt && rechtsGedrueckt;
    if (orbit) {
        strafe = false;
        return;
    }
    strafeVorUmkehr = false;
    switch (strafeModus) {
    case 0:
        strafe = false;
        break;
    case 1:
        strafe = strg || rechtsGedrueckt;
        strafeVor = false;
        break;
    case 2:
        strafe = strg || rechtsGedrueckt;
        strafeVor = true;
        break;
    case 4:
        strafeVorUmkehr = true;   // fall through
        [[fallthrough]];
    default:   // 3 & 4
        strafe = strg || rechtsGedrueckt || shift;
        strafeVor = shift;
        break;
    }
    if (linksGedrueckt && strafeModus != 0) {
        strafe = true;
        strafeVor = false;
    }
}

void Kamera::freiBewegen(int dx, int dy, const KameraEinstellungen& e) {
    // free strafe mode, toggled by the ctrl key with optional shift for forward movement
    if (strafe) {
        const float st = e.strafeTempo;
        ursprung -= vright * st * static_cast<float>(dx);
        if (strafeVor) {
            ursprung += vpn * st * static_cast<float>(dy) * (strafeVorUmkehr ? 1.0F : -1.0F);
        } else {
            ursprung += vup * st * static_cast<float>(dy);
        }
    } else {   // free rotation
        const float dtime = 0.0333333F;
        const Vektor3f winkel0 = winkel;
        if (e.mausUmkehren) {
            winkel[0] -= static_cast<float>(dy) * dtime * e.winkelTempo;
        } else {
            winkel[0] += static_cast<float>(dy) * dtime * e.winkelTempo;
        }
        winkel[1] += static_cast<float>(dx) * dtime * e.winkelTempo;
        winkel[0] = std::clamp(winkel[0], -90.0F, 90.0F);
        if (winkel[1] >= 360) {
            winkel[1] -= 360;
        } else if (winkel[1] <= 0) {
            winkel[1] += 360;
        }
        if (orbit) {
            Matrix4 rot0 = drehungEulerXyzGrad(Vektor3f(0, -winkel0[0], winkel0[1]));
            affinInvertieren(rot0);
            Matrix4 rot1 = drehungEulerXyzGrad(Vektor3f(0, -winkel[0], winkel[1]));
            multipliziere(rot1, rot0);
            Matrix4 mat = verschiebungsMatrix(orbitMitte);
            multipliziere(mat, rot1);
            verschiebeUm(mat, -orbitMitte);
            ursprung = transformierterPunkt(mat, ursprung);
        }
    }
    modelviewAktualisieren();
    freiAchsen();
}

// --- Tasten (camwindow.cpp:452-532, 624-685) ----------------------------------

bool Kamera::flagsSetzen(unsigned maske) {
    bool beginnt = false;
    if ((~bewegung & maske) != 0 && bewegung == 0) {
        beginnt = true;   // m_idleDraw.startLoop, m_keycontrol_timer.start()
        tempo = 0;
    }
    bewegung |= maske;
    return beginnt;
}

bool Kamera::flagsLoeschen(unsigned maske) {
    const bool endet = (bewegung & ~maske) == 0 && bewegung != 0;   // m_idleDraw.breakLoop
    bewegung &= ~maske;
    return endet;
}

void Kamera::tastenSteuern(float dt, const KameraEinstellungen& e, const Kasten& workzone) {
    // Update angles
    const float dwinkel = 25.0F * dt * e.winkelTempo;
    if ((bewegung & kBewegDrehLinks) != 0U) { winkel[1] += dwinkel; }
    if ((bewegung & kBewegDrehRechts) != 0U) { winkel[1] -= dwinkel; }
    if ((bewegung & kBewegNeigHoch) != 0U) { winkel[0] = std::min(winkel[0] + dwinkel, 90.0F); }
    if ((bewegung & kBewegNeigRunter) != 0U) { winkel[0] = std::max(winkel[0] - dwinkel, -90.0F); }
    modelviewAktualisieren();
    freiAchsen();
    if (e.zeitBisMax == 0) {
        tempo = static_cast<float>(e.tempo);
    } else {   // accelerate
        tempo = std::min(tempo + (static_cast<float>(e.tempo) * dt / static_cast<float>(e.zeitBisMax) * 1000),
                         static_cast<float>(e.tempo));
    }
    const float dpos = dt * tempo;
    // Update position
    if ((bewegung & kBewegVor) != 0U) { ursprung += vorn * dpos; }
    if ((bewegung & kBewegZurueck) != 0U) { ursprung -= vorn * dpos; }
    if ((bewegung & kBewegRechts) != 0U) { ursprung += seite * dpos; }
    if ((bewegung & kBewegLinks) != 0U) { ursprung -= seite * dpos; }
    if ((bewegung & kBewegHoch) != 0U) { ursprung += kAchseZ * dpos; }
    if ((bewegung & kBewegRunter) != 0U) { ursprung -= kAchseZ * dpos; }
    if ((bewegung & kBewegFokus) != 0U) { ursprung = fokusPosition(workzone); }
    modelviewAktualisieren();
}

void Kamera::vorDiskret() {
    bewegungsAchsen();
    setzeUrsprung(ursprung + vorn * kSchrittWeg);
}
void Kamera::zurueckDiskret() {
    bewegungsAchsen();
    setzeUrsprung(ursprung + vorn * -kSchrittWeg);
}
void Kamera::hochDiskret() {
    Vektor3f o = ursprung;
    o[2] += kSchrittWeg;
    setzeUrsprung(o);
}
void Kamera::runterDiskret() {
    Vektor3f o = ursprung;
    o[2] -= kSchrittWeg;
    setzeUrsprung(o);
}
void Kamera::linksDiskret() {
    bewegungsAchsen();
    setzeUrsprung(ursprung + seite * -kSchrittWeg);
}
void Kamera::rechtsDiskret() {
    bewegungsAchsen();
    setzeUrsprung(ursprung + seite * kSchrittWeg);
}
void Kamera::drehLinksDiskret() {
    Vektor3f w = winkel;
    w[1] += kSchrittWinkel;
    setzeWinkel(w);
}
void Kamera::drehRechtsDiskret() {
    Vektor3f w = winkel;
    w[1] -= kSchrittWinkel;
    setzeWinkel(w);
}
void Kamera::neigHochDiskret() {
    Vektor3f w = winkel;
    w[0] += kSchrittWinkel;
    if (w[0] > 90) { w[0] = 90; }
    setzeWinkel(w);
}
void Kamera::neigRunterDiskret() {
    Vektor3f w = winkel;
    w[0] -= kSchrittWinkel;
    if (w[0] < -90) { w[0] = -90; }
    setzeWinkel(w);
}

// --- Mausrad (camwindow.cpp:1256-1318), Orbit (1112-1123, 1222-1238) ---------

void Kamera::zoom(float x, float y, float schritt, bool freiBewegung, bool zumZeiger) {
    if (freiBewegung || !zumZeiger) {
        setzeUrsprung(ursprung + vorn * schritt);
        return;
    }
    // Wie NRC: die affine Inverse der projektiven ViewProj-Matrix.
    const Matrix4 bildZuWelt = affinInvers(sicht());
    Vektor3f n(2.0F * x / static_cast<float>(breite) - 1.0F, 2.0F * y / static_cast<float>(hoehe) - 1.0F, 0);
    n[1] *= -1;
    n *= (kNahEbene * 2.0F);
    transformierePunkt(bildZuWelt, n);
    const Vektor3f richtung = normiert(n - ursprung);
    setzeUrsprung(ursprung + richtung * schritt);
}

void Kamera::rad(int richtung, float x, float y, bool freiBewegung, const KameraEinstellungen& e) {
    if (richtung == 0) { return; }
    const int d = richtung > 0 ? 1 : -1;
    if ((bewegung & kBewegFokus) != 0U) {
        fokusVersatz += d;
        return;
    }
    if (orbit) {
        orbitVersatz += d;
        orbitRollen();
        return;
    }
    freiAchsen();
    zoom(x, y, static_cast<float>(d * e.radTempo), freiBewegung, e.zumZeiger);
}

void Kamera::orbitBeginnen(const Vektor3f& mitte) {
    orbitMitte = mitte;
    orbitStart = ursprung;
    orbitVersatz = 0;
    orbit = true;
}

Vektor3f Kamera::strahlOhneTreffer(float x, float y) const {
    // Scene_Intersect (selection.cpp:1969-1986): ohne Treffer
    // normalised( pfar - pnear ) * 256 + pnear
    const Matrix4 bildZuWelt = vollInvers(sicht());
    const Vektor3f nah = projiziert(transformierterVektor4(bildZuWelt, Vektor4f(x, y, -1, 1)));
    const Vektor3f fern = projiziert(transformierterVektor4(bildZuWelt, Vektor4f(x, y, 1, 1)));
    return normiert(fern - nah) * 256.0F + nah;
}

Vektor2f Kamera::geraetePunkt(float x, float y) const {
    return Vektor2f(((2.0F * x) / static_cast<float>(breite)) - 1.0F,
                    ((2.0F * (static_cast<float>(hoehe) - 1 - y)) / static_cast<float>(hoehe)) - 1.0F);
}

void Kamera::orbitRollen() {
    Vektor3f blick = normiert(orbitMitte - ursprung);
    if (punkt(blick, -vpn) < 0) { negieren(blick); }
    float offset = static_cast<float>(laenge(orbitMitte - orbitStart));
    const int off = orbitVersatz;
    if (off == 8) {
        offset = std::min(8.0F, offset / 16.0F);   // prevent zero offset
    } else {
        offset = versetzt(offset, off);
    }
    setzeUrsprung(orbitMitte - blick * offset);
}

// --- Befehle (camwindow.cpp:2144-2206, 1708-1765, 2218-2261) ------------------

Vektor3f Kamera::fokusPosition(const Kasten& aabb) const {
    const Frustum fr = frustumAusViewProj(sicht());
    const Vektor3f blick = -vpn;   // viewvector
    // plane3_translated aendert die Normalen nicht - nur sie werden gebraucht.
    const Ebene3d ebenen[4] = {fr.links, fr.rechts, fr.oben, fr.unten};
    float offset = 64.0F;
    const std::array<Vektor3f, 8> e = ecken(aabb);
    const Vektor3f strahlUrsprung = aabb.origin;
    const Vektor3f strahlRichtung = -blick;
    for (const Ebene3d& ebene : ebenen) {
        for (const Vektor3f& ecke : e) {
            const Vektor3d n = ebene.normal();
            const double dist = punkt(n, ecke);
            const float d = static_cast<float>(punkt(strahlRichtung, n));
            if (d != 0) {
                const Vektor3d p = Vektor3d(n[0] * dist, n[1] * dist, n[2] * dist) -
                                   Vektor3d(strahlUrsprung[0], strahlUrsprung[1], strahlUrsprung[2]);
                const float s = static_cast<float>(punkt(p, n) / d);
                offset = std::max(offset, s);
            }
        }
    }
    offset = versetzt(offset, fokusVersatz);
    return aabb.origin - blick * offset;
}

void Kamera::winkelZuruecksetzen() {
    Vektor3f w;
    w[2] = w[0] = 0;
    w[1] = static_cast<float>(22.5 * std::floor((winkel[1] + 11) / 22.5));
    setzeWinkel(w);
}

bool Kamera::etageWechseln(bool hoch, const std::vector<EtagenKandidat>& kandidaten) {
    float besterHoch = kMaxWeltKoord;
    float besterRunter = kMinWeltKoord;
    float boden = kMaxWeltKoord;
    const Vektor3f& c = ursprung;
    for (const EtagenKandidat& k : kandidaten) {
        const Kasten& b = k.grenzen;
        if (k.gewaehlt || (c.x() > b.origin.x() - b.extents.x() && c.x() < b.origin.x() + b.extents.x() &&
                           c.y() > b.origin.y() - b.extents.y() && c.y() < b.origin.y() + b.extents.y())) {
            const float etage = b.origin.z() + b.extents.z() + 32.0F;
            if (etage > c.z() + 0.1F && etage < besterHoch) { besterHoch = etage; }   // 0.1f epsilon
            if (etage < c.z() - 0.1F && etage > besterRunter) { besterRunter = etage; }
            const float unten = b.origin.z() - b.extents.z() - 16.0F;
            if (boden > unten) { boden = unten; }
        }
    }
    float jetzt = ursprung.z();
    if (hoch) {
        if (boden != kMaxWeltKoord && boden > jetzt) {
            jetzt = boden;
        } else if (besterHoch != kMaxWeltKoord) {
            jetzt = besterHoch;
        }
    } else {
        if (besterRunter != kMinWeltKoord) {
            jetzt = besterRunter;
        } else if (boden != kMaxWeltKoord && boden < jetzt) {
            jetzt = boden;
        }
    }
    if (ursprung.z() != jetzt) {
        ursprung.z() = jetzt;
        modelviewAktualisieren();
        return true;
    }
    return false;
}

void kubikHinein(KameraEinstellungen& e) {
    e.kubikSkala--;
    if (e.kubikSkala < 1) { e.kubikSkala = 1; }
}

void kubikHinaus(KameraEinstellungen& e) {
    e.kubikSkala++;
    if (e.kubikSkala > 23) { e.kubikSkala = 23; }
}

void tempoHoeher(KameraEinstellungen& e) { e.tempo = std::min(e.tempo + kTempoSchritt, kTempoMax); }

void tempoNiedriger(KameraEinstellungen& e) { e.tempo = std::max(e.tempo - kTempoSchritt, kTempoMin); }

KameraModus naechsterModus(KameraModus m, bool doom3) {
    const int anzahl = kKameraModusAnzahl - (doom3 ? 0 : 1);
    return static_cast<KameraModus>((static_cast<int>(m) + 1) % anzahl);
}

KameraModus vorherigerModus(KameraModus m, bool doom3) {
    const int anzahl = kKameraModusAnzahl - (doom3 ? 0 : 1);
    return static_cast<KameraModus>((static_cast<int>(m) + anzahl - 1) % anzahl);
}

KameraModus modusAusZahl(int wert) {
    return static_cast<KameraModus>((wert < 0 || wert >= kKameraModusAnzahl) ? static_cast<int>(KameraModus::Textur) : wert);
}

bool freelookEndetBeimLoslassen(double millisekunden, double weg, bool beimDruckEingeschaltet) {
    return (millisekunden < 300 && weg < 56) == !beimDruckEingeschaltet;
}

float lichtFaktor(const Vektor3f& normale, const Vektor3f& vpn) {
    const double d = std::max(0.0, punkt(normiert(normale), vpn));
    return static_cast<float>(std::clamp(0.2 + 0.4 + (0.4 * d), 0.0, 1.0));
}

}  // namespace bhed::karte
