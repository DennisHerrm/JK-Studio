// modell/animkurven.cpp - Kurven, Animationswerkzeuge, Aufraeumen und Reduzieren (siehe bhed/modell/animkurven.h)
//
//   Ports from Blender (https://projects.blender.org/blender/blender, commit 792201f45edfa057f84fc92d4341683e62d5223b,
//   GNU General Public License version 2 or later): blenkernel/intern/curve.cc (calchandleNurb_intern,
//   BKE_nurb_handle_smooth_fcurve and helpers), blenkernel/intern/fcurve.cc (BKE_fcurve_handles_recalc_ex,
//   BKE_fcurve_correct_bezpart, solve_cubic, findzero, berekeny, fcurve_eval_keyframes_interpolate), blenlib
//   math_solvers.cc (BLI_tridiagonal_solve, BLI_tridiagonal_solve_cyclic), math_rotation_c.cc (interp_qt_qtqt),
//   editors/animation/keyframes_general.cc (clean_fcurve, breakdown/blend/push_pull/ease/smooth/time_offset segments,
//   ED_ANIM_get_1d_gauss_kernel), editors/armature/pose_slide.cc (pose sliders). This file is part of BehavEd-Remake
//   and licensed under the GNU General Public License version 2 or later (LICENSE). Uses G2C (third_party/g2c, GPL v2)
//   for compressBone.
#include "bhed/modell/animkurven.h"
#include "bhed/modell/ik.h"

#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <limits>
#include <map>
#include <set>

#include "g2/compress.h"

namespace bhed::modell {
namespace {

// --- BezTriple wie Blender (nur, was eine F-Kurve braucht) -------------------------------------------------------------
struct Bt {
    float v[3][2]{};   // linker Henkel, Schluessel, rechter Henkel (x = Bild, y = Wert)
    Henkel h1 = Henkel::AutoGeklemmt;
    Henkel h2 = Henkel::AutoGeklemmt;
    bool gesperrt = false;   // auto_handle_type == HD_AUTOTYPE_LOCKED_FINAL
};

bool istAuto(Henkel h) { return h == Henkel::Auto || h == Henkel::AutoGeklemmt; }
bool beideAuto(const Bt& b) { return istAuto(b.h1) && istAuto(b.h2); }   // BEZT_IS_AUTOH

float lenV2(const float a[2], const float b[2]) { return std::sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1])); }

// curve.cc calchandleNurb_intern, is_fcurve = true (z = 0), skip_align = false. linksGewaehlt: bezt->f1 & handle_sel_flag.
void calchandle(Bt& bezt, const Bt* prev, const Bt* next, bool glatt, bool linksGewaehlt) {
    constexpr float eps = 1e-5F;
    bezt.gesperrt = false;
    if (prev == nullptr && next == nullptr) { return; }
    float* p2 = bezt.v[1];
    float* h1 = bezt.v[0];
    float* h2 = bezt.v[2];
    float pt[2];
    const float* p1;
    const float* p3;
    if (prev == nullptr) {
        p3 = next->v[1];
        pt[0] = 2.0F * p2[0] - p3[0];
        pt[1] = 2.0F * p2[1] - p3[1];
        p1 = pt;
    } else {
        p1 = prev->v[1];
    }
    float pt2[2];
    if (next == nullptr) {
        pt2[0] = 2.0F * p2[0] - p1[0];
        pt2[1] = 2.0F * p2[1] - p1[1];
        p3 = pt2;
    } else {
        p3 = next->v[1];
    }
    const float dvecA[2] = {p2[0] - p1[0], p2[1] - p1[1]};
    const float dvecB[2] = {p3[0] - p2[0], p3[1] - p2[1]};
    float lenA = dvecA[0];
    float lenB = dvecB[0];
    if (lenA == 0.0F) { lenA = 1.0F; }
    if (lenB == 0.0F) { lenB = 1.0F; }
    if (istAuto(bezt.h1) || istAuto(bezt.h2)) {
        const float tvec[2] = {dvecB[0] / lenB + dvecA[0] / lenA, dvecB[1] / lenB + dvecA[1] / lenA};
        float len = glatt ? 6.0F / 2.5614F : tvec[0];
        len *= 2.5614F;
        if (len != 0.0F) {
            bool leftviolate = false;
            bool rightviolate = false;
            if (!glatt) {
                lenA = std::min(lenA, 5.0F * lenB);
                lenB = std::min(lenB, 5.0F * lenA);
            }
            if (istAuto(bezt.h1)) {
                lenA /= len;
                h1[0] = p2[0] - tvec[0] * lenA;
                h1[1] = p2[1] - tvec[1] * lenA;
                if (bezt.h1 == Henkel::AutoGeklemmt && next != nullptr && prev != nullptr) {
                    const float ydiff1 = prev->v[1][1] - bezt.v[1][1];
                    const float ydiff2 = next->v[1][1] - bezt.v[1][1];
                    if ((ydiff1 <= 0.0F && ydiff2 <= 0.0F) || (ydiff1 >= 0.0F && ydiff2 >= 0.0F)) {
                        bezt.v[0][1] = bezt.v[1][1];
                        bezt.gesperrt = true;
                    } else if (ydiff1 <= 0.0F) {
                        if (prev->v[1][1] > bezt.v[0][1]) {
                            bezt.v[0][1] = prev->v[1][1];
                            leftviolate = true;
                        }
                    } else if (prev->v[1][1] < bezt.v[0][1]) {
                        bezt.v[0][1] = prev->v[1][1];
                        leftviolate = true;
                    }
                }
            }
            if (istAuto(bezt.h2)) {
                lenB /= len;
                h2[0] = p2[0] + tvec[0] * lenB;
                h2[1] = p2[1] + tvec[1] * lenB;
                if (bezt.h2 == Henkel::AutoGeklemmt && next != nullptr && prev != nullptr) {
                    const float ydiff1 = prev->v[1][1] - bezt.v[1][1];
                    const float ydiff2 = next->v[1][1] - bezt.v[1][1];
                    if ((ydiff1 <= 0.0F && ydiff2 <= 0.0F) || (ydiff1 >= 0.0F && ydiff2 >= 0.0F)) {
                        bezt.v[2][1] = bezt.v[1][1];
                        bezt.gesperrt = true;
                    } else if (ydiff1 <= 0.0F) {
                        if (next->v[1][1] < bezt.v[2][1]) {
                            bezt.v[2][1] = next->v[1][1];
                            rightviolate = true;
                        }
                    } else if (next->v[1][1] > bezt.v[2][1]) {
                        bezt.v[2][1] = next->v[1][1];
                        rightviolate = true;
                    }
                }
            }
            if (leftviolate || rightviolate) {
                const float h1x = h1[0] - p2[0];
                const float h2x = p2[0] - h2[0];
                if (leftviolate) {
                    h2[1] = p2[1] + ((p2[1] - h1[1]) / h1x) * h2x;
                } else {
                    h1[1] = p2[1] + ((p2[1] - h2[1]) / h2x) * h1x;
                }
            }
        }
    }
    if (bezt.h1 == Henkel::Vektor) {
        h1[0] = p2[0] - dvecA[0] / 3.0F;
        h1[1] = p2[1] - dvecA[1] / 3.0F;
    }
    if (bezt.h2 == Henkel::Vektor) {
        h2[0] = p2[0] + dvecB[0] / 3.0F;
        h2[1] = p2[1] + dvecB[1] / 3.0F;
    }
    if (bezt.h1 == Henkel::Frei || bezt.h2 == Henkel::Frei || (bezt.h1 != Henkel::Ausgerichtet && bezt.h2 != Henkel::Ausgerichtet)) { return; }
    lenA = lenV2(p2, h1);
    lenB = lenV2(p2, h2);
    if (lenA == 0.0F) { lenA = 1.0F; }
    if (lenB == 0.0F) { lenB = 1.0F; }
    const float ratio = lenA / lenB;
    const auto rechtsAusLinks = [&] {
        if (bezt.h2 == Henkel::Ausgerichtet && lenA > eps) {
            const float l = 1.0F / ratio;
            h2[0] = p2[0] + l * (p2[0] - h1[0]);
            h2[1] = p2[1] + l * (p2[1] - h1[1]);
        }
    };
    const auto linksAusRechts = [&] {
        if (bezt.h1 == Henkel::Ausgerichtet && lenB > eps) {
            h1[0] = p2[0] + ratio * (p2[0] - h2[0]);
            h1[1] = p2[1] + ratio * (p2[1] - h2[1]);
        }
    };
    if (linksGewaehlt) {
        rechtsAusLinks();
        linksAusRechts();
    } else {
        linksAusRechts();
        rechtsAusLinks();
    }
}

// --- BKE_nurb_handle_smooth_fcurve (curve.cc), ohne Zyklus ------------------------------------------------------------
bool tridiagonal(const float* a, const float* b, const float* c, const float* d, float* x, int count) {   // BLI_tridiagonal_solve
    if (count < 1) { return false; }
    std::vector<double> c1(static_cast<std::size_t>(count));
    std::vector<double> d1(static_cast<std::size_t>(count));
    double cPrev = static_cast<double>(c[0]) / b[0];
    double dPrev = static_cast<double>(d[0]) / b[0];
    c1[0] = cPrev;
    d1[0] = dPrev;
    int i = 1;
    for (; i < count; ++i) {
        const double denum = b[i] - a[i] * cPrev;
        c1[static_cast<std::size_t>(i)] = cPrev = c[i] / denum;
        d1[static_cast<std::size_t>(i)] = dPrev = (d[i] - a[i] * dPrev) / denum;
    }
    double xPrev = dPrev;
    x[--i] = static_cast<float>(xPrev);
    while (--i >= 0) {
        xPrev = d1[static_cast<std::size_t>(i)] - c1[static_cast<std::size_t>(i)] * xPrev;
        x[i] = static_cast<float>(xPrev);
    }
    return std::isfinite(xPrev);
}

bool tridiagonalZyklisch(const float* a, const float* b, const float* c, const float* d, float* x, int count) {   // BLI_tridiagonal_solve_cyclic
    if (count < 1) { return false; }
    if (count == 1) {
        x[0] = d[0] / (a[0] + b[0] + c[0]);
        return std::isfinite(x[0]);
    }
    if (count == 2) {
        const float a2[2] = {0.0F, a[1] + c[1]};
        const float c2[2] = {a[0] + c[0], 0.0F};
        return tridiagonal(a2, b, c2, d, x, count);
    }
    const float a0 = a[0];
    const float cN = c[count - 1];
    if (a0 == 0.0F && cN == 0.0F) { return tridiagonal(a, b, c, d, x, count); }
    std::vector<float> tmp(static_cast<std::size_t>(count), 0.0F);
    std::vector<float> b2(b, b + count);
    b2[0] -= a0;
    b2[static_cast<std::size_t>(count - 1)] -= cN;
    tmp[0] = a0;
    tmp[static_cast<std::size_t>(count - 1)] = cN;
    const bool ok = tridiagonal(a, b2.data(), c, tmp.data(), tmp.data(), count) && tridiagonal(a, b2.data(), c, d, x, count);
    if (ok) {
        const float coeff = (x[0] + x[count - 1]) / (1.0F + tmp[0] + tmp[static_cast<std::size_t>(count - 1)]);
        for (int i = 0; i < count; ++i) { x[i] -= coeff * tmp[static_cast<std::size_t>(i)]; }
    }
    return ok;
}

float relaxRichtung(const float* a, const float* b, const float* c, const float* d, const float* h, int i, int count) {
    const float state = a[i] * h[(i + count - 1) % count] + b[i] * h[i] + c[i] * h[(i + 1) % count] - d[i];
    return -state * b[i];
}

void sperre(float* a, float* b, float* c, float* d, int i, float wert) {   // bezier_lock_unknown
    a[i] = c[i] = 0.0F;
    b[i] = 1.0F;
    d[i] = wert;
}

bool tridiagonalMitGrenzen(float* a, float* b, float* c, float* d, float* h, const float* hmin, const float* hmax, int n) {
    const std::vector<float> a0(a, a + n);
    const std::vector<float> b0(b, b + n);
    const std::vector<float> c0(c, c + n);
    const std::vector<float> d0(d, d + n);
    std::vector<char> gesperrt(static_cast<std::size_t>(n), 0);
    std::vector<char> geloest(static_cast<std::size_t>(n), 0);
    bool overshoot = false;
    bool unlocked = false;
    do {
        if (!tridiagonalZyklisch(a, b, c, d, h, n)) { return false; }
        bool all = false;
        bool locked = false;
        overshoot = unlocked = false;
        do {
            for (int i = 0; i < n; ++i) {
                if (h[i] >= hmin[i] && h[i] <= hmax[i]) { continue; }
                overshoot = true;
                const float target = h[i] > hmax[i] ? hmax[i] : hmin[i];
                if (target != 0.0F || all) {
                    gesperrt[static_cast<std::size_t>(i)] = 1;
                    sperre(a, b, c, d, i, target);
                    locked = true;
                }
            }
            all = true;
        } while (overshoot && !locked);
        if (!locked) {
            for (int i = 0; i < n; ++i) {
                if (gesperrt[static_cast<std::size_t>(i)] == 0 || geloest[static_cast<std::size_t>(i)] >= 2) { continue; }
                const float relax = relaxRichtung(a0.data(), b0.data(), c0.data(), d0.data(), h, i, n);
                if ((relax > 0 && h[i] < hmax[i]) || (relax < 0 && h[i] > hmin[i])) {
                    a[i] = a0[static_cast<std::size_t>(i)];
                    b[i] = b0[static_cast<std::size_t>(i)];
                    c[i] = c0[static_cast<std::size_t>(i)];
                    d[i] = d0[static_cast<std::size_t>(i)];
                    gesperrt[static_cast<std::size_t>(i)] = 0;
                    ++geloest[static_cast<std::size_t>(i)];
                    unlocked = true;
                }
            }
        }
    } while (overshoot || unlocked);
    return true;
}

void eqStetig(float* a, float* b, float* c, float* d, const float* dy, const float* l, int i) {   // bezier_eq_continuous
    a[i] = l[i] * l[i];
    b[i] = 2.0F * (l[i] + 1);
    c[i] = 1.0F / l[i + 1];
    d[i] = dy[i] * l[i] * l[i] + dy[i + 1];
}
void eqRechts(float* a, float* b, float* c, float* d, const float* dy, const float* l, int i) {   // bezier_eq_noaccel_right
    a[i] = 0.0F;
    b[i] = 2.0F;
    c[i] = 1.0F / l[i + 1];
    d[i] = dy[i + 1];
}
void eqLinks(float* a, float* b, float* c, float* d, const float* dy, const float* l, int i) {   // bezier_eq_noaccel_left
    a[i] = l[i] * l[i];
    b[i] = 2.0F * l[i];
    c[i] = 0.0F;
    d[i] = dy[i] * l[i] * l[i];
}
void klemme(float* hmax, float* hmin, int i, float dy, bool noReverse, bool noOvershoot) {   // bezier_clamp
    if (dy > 0) {
        if (noOvershoot) { hmax[i] = std::min(hmax[i], dy); }
        if (noReverse) { hmin[i] = 0.0F; }
    } else if (dy < 0) {
        if (noReverse) { hmax[i] = 0.0F; }
        if (noOvershoot) { hmin[i] = std::max(hmin[i], dy); }
    } else if (noReverse || noOvershoot) {
        hmax[i] = hmin[i] = 0.0F;
    }
}
void henkelAusgeben(Bt& b, bool rechts, float dy, bool endpunkt) {   // bezier_output_handle(_inner)
    const int idx = rechts ? 2 : 0;
    float neu[2] = {b.v[idx][0], b.v[1][1] + dy};
    const Henkel hr = rechts ? b.h2 : b.h1;
    const Henkel hm = rechts ? b.h1 : b.h2;
    if (!istAuto(hr) && hr != Henkel::Vektor) { return; }
    b.v[idx][0] = neu[0];
    b.v[idx][1] = neu[1];
    if (hm == Henkel::Ausgerichtet) {
        const float hlen = lenV2(b.v[1], b.v[2 - idx]);
        const float h2len = lenV2(b.v[1], b.v[idx]);
        const float tmp[2] = {b.v[1][0] - b.v[idx][0], b.v[1][1] - b.v[idx][1]};
        b.v[2 - idx][0] = b.v[1][0] + tmp[0] * (hlen / h2len);
        b.v[2 - idx][1] = b.v[1][1] + tmp[1] * (hlen / h2len);
    } else if (endpunkt && (istAuto(hm) || hm == Henkel::Vektor)) {
        b.v[2 - idx][0] = b.v[1][0] + (b.v[1][0] - b.v[idx][0]);
        b.v[2 - idx][1] = b.v[1][1] + (b.v[1][1] - b.v[idx][1]);
    }
}
bool endeLoesen(const Bt& b, Henkel h, bool ende) { return h == Henkel::Vektor || (ende && istAuto(h) && !b.gesperrt); }
float henkelAnpassung(float hsize[2], float dx) {   // bezier_calc_handle_adj
    const float fac = dx / (hsize[0] + dx / 3.0F);
    if (fac < 1.0F) {
        hsize[0] *= fac;
        hsize[1] *= fac;
    }
    return 1.0F - 3.0F * hsize[0] / dx;
}

void glaettenTeil(std::vector<Bt>& bezt, int start, int count) {   // bezier_handle_calc_smooth_fcurve, cycle = false
    const int total = static_cast<int>(bezt.size());
    if (count < 2) { return; }
    Bt& first = bezt[static_cast<std::size_t>(start)];
    Bt& last = bezt[static_cast<std::size_t>(start + count - 1)];
    bool solveFirst = endeLoesen(first, first.h2, start == 0);
    bool solveLast = endeLoesen(last, last.h1, start + count == total);
    if (count == 2 && solveFirst == solveLast) { return; }
    const auto n = static_cast<std::size_t>(count);
    std::vector<float> dx(n), dy(n), l(n), a(n), b(n), c(n), d(n), h(n), hmax(n), hmin(n);
    dx[0] = dy[0] = std::numeric_limits<float>::quiet_NaN();
    for (int i = 1, j = start + 1; i < count; ++i, ++j) {
        dx[static_cast<std::size_t>(i)] = bezt[static_cast<std::size_t>(j)].v[1][0] - bezt[static_cast<std::size_t>(j - 1)].v[1][0];
        dy[static_cast<std::size_t>(i)] = bezt[static_cast<std::size_t>(j)].v[1][1] - bezt[static_cast<std::size_t>(j - 1)].v[1][1];
    }
    l[0] = l[n - 1] = 1.0F;
    for (std::size_t i = 1; i + 1 < n; ++i) { l[i] = dx[i + 1] / dx[i]; }
    bool clampedPrev = false;
    bool clampedCur = first.h1 == Henkel::AutoGeklemmt || first.h2 == Henkel::AutoGeklemmt;
    for (std::size_t i = 0; i < n; ++i) {
        hmax[i] = FLT_MAX;
        hmin[i] = -FLT_MAX;
    }
    for (int i = 1, j = start + 1; i < count; ++i, ++j) {
        clampedPrev = clampedCur;
        const Bt& bj = bezt[static_cast<std::size_t>(j)];
        clampedCur = bj.h1 == Henkel::AutoGeklemmt || bj.h2 == Henkel::AutoGeklemmt;
        klemme(hmax.data(), hmin.data(), i - 1, dy[static_cast<std::size_t>(i)], clampedPrev, clampedPrev);
        klemme(hmax.data(), hmin.data(), i, dy[static_cast<std::size_t>(i)] * l[static_cast<std::size_t>(i)], clampedCur, clampedCur);
    }
    float firstAdj = 0.0F;
    float lastAdj = 0.0F;
    if (!solveFirst) {
        float tmp[2] = {first.v[2][0] - first.v[1][0], first.v[2][1] - first.v[1][1]};
        firstAdj = henkelAnpassung(tmp, dx[1]);
        sperre(a.data(), b.data(), c.data(), d.data(), 0, tmp[1]);
    } else {
        eqRechts(a.data(), b.data(), c.data(), d.data(), dy.data(), l.data(), 0);
    }
    if (!solveLast) {
        float tmp[2] = {last.v[1][0] - last.v[0][0], last.v[1][1] - last.v[0][1]};
        lastAdj = henkelAnpassung(tmp, dx[n - 1]);
        sperre(a.data(), b.data(), c.data(), d.data(), count - 1, tmp[1]);
    } else {
        eqLinks(a.data(), b.data(), c.data(), d.data(), dy.data(), l.data(), count - 1);
    }
    for (int i = 1; i < count - 1; ++i) { eqStetig(a.data(), b.data(), c.data(), d.data(), dy.data(), l.data(), i); }
    if (count > 2 || solveLast) { b[1] += l[1] * firstAdj; }
    if (count > 2 || solveFirst) { b[n - 2] += lastAdj; }
    if (tridiagonalMitGrenzen(a.data(), b.data(), c.data(), d.data(), h.data(), hmin.data(), hmax.data(), count)) {
        for (int i = 1, j = start + 1; i < count - 1; ++i, ++j) {
            const bool ende = (j == total - 1);
            henkelAusgeben(bezt[static_cast<std::size_t>(j)], false, -h[static_cast<std::size_t>(i)] / l[static_cast<std::size_t>(i)], ende);
            henkelAusgeben(bezt[static_cast<std::size_t>(j)], true, h[static_cast<std::size_t>(i)], ende);
        }
        if (solveFirst) { henkelAusgeben(first, true, h[0], start == 0); }
        if (solveLast) { henkelAusgeben(last, false, -h[n - 1] / l[n - 1], start + count == total); }
    }
}

void glaetten(std::vector<Bt>& bezt) {   // BKE_nurb_handle_smooth_fcurve, cyclic = false
    const int total = static_cast<int>(bezt.size());
    const auto frei = [](const Bt& b) { return beideAuto(b) && !b.gesperrt; };   // is_free_auto_point
    int start = 0;
    int count = 1;
    for (int i = 1, j = start + 1; i < total; ++i, ++j) {
        if (!frei(bezt[static_cast<std::size_t>(j)])) {
            glaettenTeil(bezt, start, count + 1);
            start = j;
            count = 1;
        } else {
            ++count;
        }
    }
    if (count > 1) { glaettenTeil(bezt, start, count); }
}

// Die Zeit relativ zum ersten Schluessel: die Rechnung von Blender haengt nur von Abstaenden ab, und float traegt bei den
// Bildnummern der Spiel-GLA (Movie Duels _humanoid bis 60861) nur noch 0,002-0,008 (mb5_ui6: Henkel 3,33398 statt 10/3).
std::vector<Bt> kanalKurve(const std::vector<RigSchluessel>& v, int c) {
    std::vector<Bt> b(v.size());
    const int basis = v.empty() ? 0 : v.front().bild;
    for (std::size_t i = 0; i < v.size(); ++i) {
        const float x = static_cast<float>(v[i].bild - basis);
        const float y = kanalWert(v[i], c);
        b[i].v[1][0] = x;
        b[i].v[1][1] = y;
        b[i].v[0][0] = x + v[i].hl[c][0];
        b[i].v[0][1] = y + v[i].hl[c][1];
        b[i].v[2][0] = x + v[i].hr[c][0];
        b[i].v[2][1] = y + v[i].hr[c][1];
        b[i].h1 = v[i].henkelL[c];
        b[i].h2 = v[i].henkelR[c];
    }
    return b;
}

// fcurve.cc fcurve_eval_keyframes_interpolate, Abschnitt zwischen a (links) und b (rechts), nur Bezier.
float bezierWert(const Bt& prev, const Bt& bezt, float x) {
    float v1[2] = {prev.v[1][0], prev.v[1][1]};
    float v2[2] = {prev.v[2][0], prev.v[2][1]};
    float v3[2] = {bezt.v[0][0], bezt.v[0][1]};
    float v4[2] = {bezt.v[1][0], bezt.v[1][1]};
    if (std::fabs(v1[1] - v4[1]) < FLT_EPSILON && std::fabs(v2[1] - v3[1]) < FLT_EPSILON && std::fabs(v3[1] - v4[1]) < FLT_EPSILON) { return v1[1]; }
    bez::korrigiere(v1, v2, v3, v4);
    float opl[32];
    if (bez::findeNull(x, v1[0], v2[0], v3[0], v4[0], opl) == 0) { return 0.0F; }
    return bez::yBei(v1[1], v2[1], v3[1], v4[1], opl[0]);
}

double sqrt3d(double d) {
    if (d == 0.0) { return 0.0; }
    if (d < 0.0) { return -std::exp(std::log(-d) / 3.0); }
    return std::exp(std::log(d) / 3.0);
}

int solveCubic(double c0, double c1, double c2, double c3, float* o) {   // fcurve.cc solve_cubic
    constexpr double kSmall = -1.0e-10;
    double a;
    double b;
    double c;
    double p;
    double q;
    double d;
    double t;
    int nr = 0;
    const auto drin = [&](float x) { return x >= static_cast<float>(kSmall) && x <= 1.000001F; };
    if (c3 != 0.0) {
        a = c2 / c3;
        b = c1 / c3;
        c = c0 / c3;
        a = a / 3;
        p = b / 3 - a * a;
        q = (2 * a * a * a - a * b + c) / 2;
        d = q * q + p * p * p;
        if (d > 0.0) {
            t = std::sqrt(d);
            o[0] = static_cast<float>(sqrt3d(-q + t) + sqrt3d(-q - t) - a);
            return drin(o[0]) ? 1 : 0;
        }
        if (d == 0.0) {
            t = sqrt3d(-q);
            o[0] = static_cast<float>(2 * t - a);
            if (drin(o[0])) { nr++; }
            o[nr] = static_cast<float>(-t - a);
            return drin(o[nr]) ? nr + 1 : nr;
        }
        const double phi = std::acos(-q / std::sqrt(-(p * p * p)));
        t = std::sqrt(-p);
        p = std::cos(phi / 3);
        q = std::sqrt(3 - 3 * p * p);
        o[0] = static_cast<float>(2 * t * p - a);
        if (drin(o[0])) { nr++; }
        o[nr] = static_cast<float>(-t * (p + q) - a);
        if (drin(o[nr])) { nr++; }
        o[nr] = static_cast<float>(-t * (p - q) - a);
        return drin(o[nr]) ? nr + 1 : nr;
    }
    a = c2;
    b = c1;
    c = c0;
    if (a != 0.0) {
        p = b * b - 4 * a * c;
        if (p > 0) {
            p = std::sqrt(p);
            o[0] = static_cast<float>((-b - p) / (2 * a));
            if (drin(o[0])) { nr++; }
            o[nr] = static_cast<float>((-b + p) / (2 * a));
            return drin(o[nr]) ? nr + 1 : nr;
        }
        if (p == 0) {
            o[0] = static_cast<float>(-b / (2 * a));
            if (drin(o[0])) { return 1; }
        }
        return 0;
    }
    if (b != 0.0) {
        o[0] = static_cast<float>(-c / b);
        return drin(o[0]) ? 1 : 0;
    }
    if (c == 0.0) {
        o[0] = 0.0F;
        return 1;
    }
    return 0;
}

// math_rotation_c.cc interp_qt_qtqt (mit interp_dot_slerp), auch mit t ausserhalb 0..1 (Push).
void interpQt(float q[4], const float a[4], const float b[4], float t) {
    float cosom = a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
    float quat[4];
    if (cosom < 0.0F) {
        cosom = -cosom;
        for (int k = 0; k < 4; ++k) { quat[k] = -a[k]; }
    } else {
        for (int k = 0; k < 4; ++k) { quat[k] = a[k]; }
    }
    float w0;
    float w1;
    if (std::fabs(cosom) < 1.0F - 1e-4F) {
        const float omega = std::acos(cosom);
        const float sinom = std::sin(omega);
        w0 = std::sin((1.0F - t) * omega) / sinom;
        w1 = std::sin(t * omega) / sinom;
    } else {
        w0 = 1.0F - t;
        w1 = t;
    }
    for (int k = 0; k < 4; ++k) { q[k] = w0 * quat[k] + w1 * b[k]; }
}

void normiere(float q[4]) {
    const float l = std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
    if (l > 0.0F) {
        for (int k = 0; k < 4; ++k) { q[k] /= l; }
    }
}

float interpf(float ziel, float ursprung, float f) { return (f * ziel) + (1.0F - f) * ursprung; }   // BLI interpf

// Der Wert (q normiert, t) eines Knochens am Bild aus seiner Liste.
bool wertAm(const std::vector<RigSchluessel>& v, float bild, float q[4], float t[3]) {
    BoneMatrix m;
    if (!schluesselWert(v, bild, m)) { return false; }
    quatAusMatrix(m, q);
    t[0] = m.m[0][3];
    t[1] = m.m[1][3];
    t[2] = m.m[2][3];
    return true;
}

// Einen Wert als Schluessel am Bild (neu oder ersetzt), mit der Halbkugel des Nachbarn.
void setzeWert(std::vector<RigSchluessel>& v, int bild, const float q[4], const float t[3], Interp interp) {
    const auto it = std::lower_bound(v.begin(), v.end(), bild, [](const RigSchluessel& a, int b) { return a.bild < b; });
    RigSchluessel* k = nullptr;
    if (it != v.end() && it->bild == bild) {
        k = &*it;
    } else {
        RigSchluessel n;
        n.bild = bild;
        n.interp = interp;
        k = &*v.insert(it, n);
    }
    std::memcpy(k->q, q, sizeof k->q);
    std::memcpy(k->t, t, sizeof k->t);
    const auto i = static_cast<std::size_t>(k - v.data());
    const RigSchluessel* nb = i > 0 ? &v[i - 1] : (i + 1 < v.size() ? &v[i + 1] : nullptr);
    if (nb != nullptr && k->q[0] * nb->q[0] + k->q[1] * nb->q[1] + k->q[2] * nb->q[2] + k->q[3] * nb->q[3] < 0.0F) {
        for (float& x : k->q) { x = -x; }
    }
}

std::vector<RigSchluessel>* liste(RigSchicht& s, int k) {
    if (k < 0 || static_cast<std::size_t>(k) >= s.knochen.size()) { return nullptr; }
    return &s.knochen[static_cast<std::size_t>(k)];
}

// Henkel im Zeitverlauf gespiegelt (Umkehren, Spiegeln am Cursor): links <-> rechts, dx negiert.
void henkelZeitSpiegeln(RigSchluessel& k) {
    for (int c = 0; c < kKanaele; ++c) {
        const float l0 = k.hl[c][0];
        const float l1 = k.hl[c][1];
        k.hl[c][0] = -k.hr[c][0];
        k.hl[c][1] = k.hr[c][1];
        k.hr[c][0] = -l0;
        k.hr[c][1] = l1;
        std::swap(k.henkelL[c], k.henkelR[c]);
    }
}

void sortieren(std::vector<RigSchluessel>& v) {
    std::stable_sort(v.begin(), v.end(), [](const RigSchluessel& a, const RigSchluessel& b) { return a.bild < b.bild; });
}

}  // namespace

// --- Kanaele ---------------------------------------------------------------------------------------------------------

float kanalWert(const RigSchluessel& k, int kanal) noexcept { return kanal < 4 ? k.q[std::clamp(kanal, 0, 3)] : k.t[std::clamp(kanal - 4, 0, 2)]; }

void kanalSetzen(RigSchluessel& k, int kanal, float wert) noexcept {
    if (kanal < 4) {
        k.q[std::clamp(kanal, 0, 3)] = wert;
    } else {
        k.t[std::clamp(kanal - 4, 0, 2)] = wert;
    }
}

namespace bez {
int findeNull(float x, float q0, float q1, float q2, float q3, float* o) {   // fcurve.cc findzero
    const double c0 = q0 - x;
    const double c1 = 3.0F * (q1 - q0);
    const double c2 = 3.0F * (q0 - 2.0F * q1 + q2);
    const double c3 = q3 - q0 + 3.0F * (q1 - q2);
    return solveCubic(c0, c1, c2, c3, o);
}
float yBei(float f1, float f2, float f3, float f4, float t) {   // fcurve.cc berekeny (b = 1)
    const float c0 = f1;
    const float c1 = 3.0F * (f2 - f1);
    const float c2 = 3.0F * (f1 - 2.0F * f2 + f3);
    const float c3 = f4 - f1 + 3.0F * (f2 - f3);
    return c0 + t * c1 + t * t * c2 + t * t * t * c3;
}
void korrigiere(const float v1[2], float v2[2], float v3[2], const float v4[2]) {   // BKE_fcurve_correct_bezpart
    const float h1[2] = {v1[0] - v2[0], v1[1] - v2[1]};
    const float h2[2] = {v4[0] - v3[0], v4[1] - v3[1]};
    const float len = v4[0] - v1[0];
    const float len1 = std::fabs(h1[0]);
    const float len2 = std::fabs(h2[0]);
    if ((len1 + len2) == 0.0F) { return; }
    if (len1 > len) {
        const float fac = len / len1;
        v2[0] = (v1[0] - fac * h1[0]);
        v2[1] = (v1[1] - fac * h1[1]);
    }
    if (len2 > len) {
        const float fac = len / len2;
        v3[0] = (v4[0] - fac * h2[0]);
        v3[1] = (v4[1] - fac * h2[1]);
    }
}
}  // namespace bez

void henkelNeuGezogen(std::vector<RigSchluessel>& v, bool glatt, int kanalGezogen, int indexGezogen, bool linksGezogen) {
    if (v.size() < 2) { return; }   // BKE_fcurve_handles_recalc_ex: erst ab zwei Punkten
    for (int c = 0; c < kKanaele; ++c) {
        std::vector<Bt> b = kanalKurve(v, c);
        const int n = static_cast<int>(b.size());
        for (int i = 0; i < n; ++i) {
            Bt& bezt = b[static_cast<std::size_t>(i)];
            const Bt* prev = i > 0 ? &b[static_cast<std::size_t>(i - 1)] : nullptr;
            const Bt* next = i + 1 < n ? &b[static_cast<std::size_t>(i + 1)] : nullptr;
            // Henkelzeiten beidseits des Schluessels halten (Schwelle 0,001 mit einem ulp)
            const float schwelle = 0.001F;
            bezt.v[0][0] = std::min(bezt.v[0][0], std::nextafter(bezt.v[1][0] - schwelle, -FLT_MAX));
            bezt.v[2][0] = std::max(bezt.v[2][0], std::nextafter(bezt.v[1][0] + schwelle, FLT_MAX));
            calchandle(bezt, prev, next, glatt, c == kanalGezogen && i == indexGezogen && linksGezogen);
            if (beideAuto(bezt) && (i == 0 || i == n - 1)) {   // konstante Extrapolation: Enden waagerecht
                bezt.v[0][1] = bezt.v[2][1] = bezt.v[1][1];
                bezt.gesperrt = true;
            }
            if (i > 0 && b[static_cast<std::size_t>(i - 1)].v[1][0] >= bezt.v[1][0]) { b[static_cast<std::size_t>(i - 1)].gesperrt = bezt.gesperrt = true; }
        }
        if (glatt) { glaetten(b); }
        for (int i = 0; i < n; ++i) {
            RigSchluessel& k = v[static_cast<std::size_t>(i)];
            const Bt& bezt = b[static_cast<std::size_t>(i)];
            k.hl[c][0] = bezt.v[0][0] - bezt.v[1][0];
            k.hl[c][1] = bezt.v[0][1] - bezt.v[1][1];
            k.hr[c][0] = bezt.v[2][0] - bezt.v[1][0];
            k.hr[c][1] = bezt.v[2][1] - bezt.v[1][1];
        }
    }
}

void henkelNeu(std::vector<RigSchluessel>& v, bool glatt) { henkelNeuGezogen(v, glatt, -1, -1, false); }

void henkelNeu(RigSchicht& s) {
    for (auto& v : s.knochen) { henkelNeu(v); }
}

float kanalAuswerten(const std::vector<RigSchluessel>& v, int kanal, float x) {
    if (v.empty()) { return kanal == 0 ? 1.0F : 0.0F; }
    if (x <= static_cast<float>(v.front().bild)) { return kanalWert(v.front(), kanal); }
    if (x >= static_cast<float>(v.back().bild)) { return kanalWert(v.back(), kanal); }
    std::size_t i = 0;
    while (i + 1 < v.size() && static_cast<float>(v[i + 1].bild) <= x) { ++i; }
    const RigSchluessel& a = v[i];
    if (std::fabs(static_cast<float>(a.bild) - x) < 0.0001F) { return kanalWert(a, kanal); }
    const RigSchluessel& b = v[i + 1];
    const float begin = kanalWert(a, kanal);
    const float change = kanalWert(b, kanal) - begin;
    const float duration = static_cast<float>(b.bild - a.bild);
    if (a.interp == Interp::Konstant || duration == 0.0F) { return begin; }
    if (a.interp == Interp::Linear) { return change * (x - static_cast<float>(a.bild)) / duration + begin; }   // BLI_easing_linear_ease
    // relativ zum linken Schluessel gerechnet (siehe kanalKurve)
    Bt pa;
    Bt pb;
    pa.v[1][0] = 0.0F;
    pa.v[1][1] = begin;
    pa.v[2][0] = a.hr[kanal][0];
    pa.v[2][1] = begin + a.hr[kanal][1];
    pb.v[1][0] = duration;
    pb.v[1][1] = kanalWert(b, kanal);
    pb.v[0][0] = duration + b.hl[kanal][0];
    pb.v[0][1] = pb.v[1][1] + b.hl[kanal][1];
    return bezierWert(pa, pb, x - static_cast<float>(a.bild));
}

int quatFolge(std::vector<RigSchluessel>& v) {
    int gewendet = 0;
    for (std::size_t i = 1; i < v.size(); ++i) {
        const float d = v[i].q[0] * v[i - 1].q[0] + v[i].q[1] * v[i - 1].q[1] + v[i].q[2] * v[i - 1].q[2] + v[i].q[3] * v[i - 1].q[3];
        if (d >= 0.0F) { continue; }
        for (int c = 0; c < 4; ++c) {
            v[i].q[c] = -v[i].q[c];
            v[i].hl[c][1] = -v[i].hl[c][1];
            v[i].hr[c][1] = -v[i].hr[c][1];
        }
        ++gewendet;
    }
    return gewendet;
}

// --- Auswahl ---------------------------------------------------------------------------------------------------------

std::vector<int> wahlIndizes(const RigSchicht& s, const Wahl& w, int knochen) {
    std::vector<int> aus;
    const int sp = ikWahlSpur(knochen);
    if (sp >= 0 ? static_cast<std::size_t>(sp) >= s.ik.size() : (knochen < 0 || static_cast<std::size_t>(knochen) >= s.knochen.size())) { return aus; }
    const auto& v = sp >= 0 ? s.ik[static_cast<std::size_t>(sp)].ziel : s.knochen[static_cast<std::size_t>(knochen)];
    for (std::size_t i = 0; i < v.size(); ++i) {
        for (const SchluesselWahl& x : w) {
            if (x.knochen == knochen && x.bild == v[i].bild) {
                aus.push_back(static_cast<int>(i));
                break;
            }
        }
    }
    return aus;
}

Wahl wahlMitIk(const RigSchicht& s, const Wahl& w) {
    Wahl aus = w;
    for (std::size_t sp = 0; sp < s.ik.size(); ++sp) {
        const IkSpur& spur = s.ik[sp];
        const int code = ikWahlCode(static_cast<int>(sp));
        for (const SchluesselWahl& x : w) {
            const bool kette = x.knochen >= 0 && (x.knochen == spur.oben.knochen || x.knochen == spur.unten.knochen || x.knochen == spur.ende.knochen ||
                                                  x.knochen == spur.zehen.knochen);
            if (!kette || !s.hatSchluessel(code, x.bild) || std::find(aus.begin(), aus.end(), SchluesselWahl{code, x.bild}) != aus.end()) { continue; }
            aus.push_back({code, x.bild});
        }
        // und umgekehrt: ein gewaehlter IK-Schluessel nimmt die FK-Schluessel der Kette am selben Bild mit
        for (const SchluesselWahl& x : w) {
            if (x.knochen != code) { continue; }
            for (const int k : {spur.oben.knochen, spur.unten.knochen, spur.ende.knochen, spur.zehen.knochen}) {
                if (k < 0 || !s.hatSchluessel(k, x.bild) || std::find(aus.begin(), aus.end(), SchluesselWahl{k, x.bild}) != aus.end()) { continue; }
                aus.push_back({k, x.bild});
            }
        }
    }
    return aus;
}

std::vector<int> wahlKnochen(const Wahl& w) {
    std::set<int> k;
    for (const SchluesselWahl& x : w) { k.insert(x.knochen); }
    return {k.begin(), k.end()};
}

// --- Werkzeuge auf Schluesseln -----------------------------------------------------------------------------------------

int kurvenWerkzeug(RigSchicht& s, const Wahl& w, KurvenWerkzeug art, float faktor, float zusatz) {
    int geaendert = 0;
    for (const int k : wahlKnochen(w)) {
        std::vector<RigSchluessel>* vp = liste(s, k);
        if (vp == nullptr) { continue; }
        std::vector<RigSchluessel>& v = *vp;
        const std::vector<int> idx = wahlIndizes(s, w, k);
        const std::vector<RigSchluessel> orig = v;   // Blender rechnet je Kurve auf den Werten vor dem Schieber
        const int n = static_cast<int>(v.size());
        // Abschnitte aufeinanderfolgender gewaehlter Schluessel (find_fcurve_segments)
        std::size_t p = 0;
        while (p < idx.size()) {
            std::size_t q = p;
            while (q + 1 < idx.size() && idx[q + 1] == idx[q] + 1) { ++q; }
            const int start = idx[p];
            const int laenge = static_cast<int>(q - p) + 1;
            const int li = start - 1 >= 0 ? start - 1 : start;                  // fcurve_segment_start_get
            const int ri = start + laenge < n ? start + laenge : start + laenge - 1;   // fcurve_segment_end_get
            for (int c = 0; c < kKanaele; ++c) {
                const float lx = static_cast<float>(orig[static_cast<std::size_t>(li)].bild);
                const float ly = kanalWert(orig[static_cast<std::size_t>(li)], c);
                const float rx = static_cast<float>(orig[static_cast<std::size_t>(ri)].bild);
                const float ry = kanalWert(orig[static_cast<std::size_t>(ri)], c);
                if (art == KurvenWerkzeug::Glaetten) {
                    // gaussian_smooth_graph_keys + smooth_fcurve_segment, Filterbreite 6
                    const int fw = 6;
                    const float sigma = zusatz > 0.0F ? zusatz : 0.33F;
                    std::vector<double> kern(static_cast<std::size_t>(fw + 1));
                    {   // ED_ANIM_get_1d_gauss_kernel
                        const double sq = 2.0 * sigma * sigma;
                        double summe = 0.0;
                        for (int i = 0; i < fw + 1; ++i) {
                            const double ni = static_cast<double>(i) / fw;
                            kern[static_cast<std::size_t>(i)] = std::exp(-ni * ni / sq);
                            summe += i == 0 ? kern[0] : kern[static_cast<std::size_t>(i)] * 2;
                        }
                        for (double& x : kern) { x /= summe; }
                    }
                    const float x0 = static_cast<float>(orig[static_cast<std::size_t>(start)].bild);
                    const float x1 = static_cast<float>(orig[static_cast<std::size_t>(start + laenge - 1)].bild);
                    const int zahl = static_cast<int>(x1 - x0) + (fw * 2 + 1);
                    std::vector<float> proben(static_cast<std::size_t>(zahl));
                    for (int i = 0; i < zahl; ++i) { proben[static_cast<std::size_t>(i)] = kanalAuswerten(orig, c, x0 - static_cast<float>(fw) + static_cast<float>(i)); }
                    std::vector<float> gefiltert = proben;
                    for (int i = fw; i < zahl - fw; ++i) {
                        double r = proben[static_cast<std::size_t>(i)] * kern[0];
                        for (int j = 1; j <= fw; ++j) {
                            r += proben[static_cast<std::size_t>(i + j)] * kern[static_cast<std::size_t>(j)];
                            r += proben[static_cast<std::size_t>(i - j)] * kern[static_cast<std::size_t>(j)];
                        }
                        gefiltert[static_cast<std::size_t>(i)] = static_cast<float>(r);
                    }
                    for (int i = start; i < start + laenge; ++i) {
                        const int si = static_cast<int>(std::lround(static_cast<float>(orig[static_cast<std::size_t>(i)].bild) - x0)) + fw;
                        const float wert = gefiltert[static_cast<std::size_t>(std::clamp(si, 0, zahl - 1))];
                        kanalSetzen(v[static_cast<std::size_t>(i)], c, interpf(wert, kanalWert(orig[static_cast<std::size_t>(i)], c), faktor));
                    }
                    continue;
                }
                for (int i = start; i < start + laenge; ++i) {
                    const float x = static_cast<float>(orig[static_cast<std::size_t>(i)].bild);
                    const float y = kanalWert(orig[static_cast<std::size_t>(i)], c);
                    float neu = y;
                    switch (art) {
                        case KurvenWerkzeug::Breakdown: neu = interpf(ry, ly, (faktor + 1.0F) / 2.0F); break;
                        case KurvenWerkzeug::BlendNachbar: neu = interpf(faktor < 0.0F ? ly : ry, y, std::fabs(faktor)); break;
                        case KurvenWerkzeug::BlendVorgabe: neu = interpf(c == 0 ? 1.0F : 0.0F, y, faktor); break;
                        case KurvenWerkzeug::PushPull: {
                            const float xr = rx - lx;
                            if (std::fabs(xr) < FLT_EPSILON) { break; }
                            const float lin = ly + (ry - ly) * ((x - lx) / xr);
                            neu = lin + (y - lin) * faktor;
                            break;
                        }
                        case KurvenWerkzeug::Ease: {
                            const float xr = rx - lx;
                            if (std::fabs(xr) < FLT_EPSILON) { break; }
                            const float breite = zusatz > 0.0F ? zusatz : 2.0F;
                            const float shift = -faktor;
                            const auto sig = [&](float xx) {   // ease_sigmoid_function
                                const float xs = (xx - shift) * breite;
                                return (xs / std::sqrt(1.0F + xs * xs) + 1.0F) * 0.5F;
                            };
                            const float ymin = sig(-1.0F);
                            const float ymax = sig(1.0F);
                            const float xn = ((x - lx) / xr) * 2.0F - 1.0F;
                            neu = ly + (ry - ly) * ((sig(xn) - ymin) * (1.0F / (ymax - ymin)));
                            break;
                        }
                        case KurvenWerkzeug::Glaetten: break;
                    }
                    kanalSetzen(v[static_cast<std::size_t>(i)], c, neu);
                }
            }
            geaendert += laenge;
            p = q + 1;
        }
        henkelNeu(v);
    }
    return geaendert;
}

// --- Posen-Schieber ----------------------------------------------------------------------------------------------------

int poseSchieben(RigSchicht& s, const std::vector<int>& knochen, int bild, PosenSchieber art, float faktor) {
    std::vector<int> ks = knochen;
    if (ks.empty()) {
        for (std::size_t k = 0; k < s.knochen.size(); ++k) { ks.push_back(static_cast<int>(k)); }
    }
    ks.erase(std::remove_if(ks.begin(), ks.end(), [&](int k) { return liste(s, k) == nullptr || liste(s, k)->empty(); }), ks.end());
    if (ks.empty()) { return 0; }
    // die Schluesselliste aller dieser Knochen (pose_slide_invoke_common: ED_keylist_find_prev/next)
    std::set<int> zeiten;
    for (const int k : ks) {
        for (const RigSchluessel& x : *liste(s, k)) { zeiten.insert(x.bild); }
    }
    int vor = bild - 1;
    int nach = bild + 1;
    {
        const auto it = zeiten.lower_bound(bild);
        if (it != zeiten.begin()) { vor = *std::prev(it); }
        const auto jt = zeiten.upper_bound(bild);
        if (jt != zeiten.end()) { nach = *jt; }
    }
    const float cf = static_cast<float>(bild - vor) / static_cast<float>(nach - vor);
    int gesetzt = 0;
    for (const int k : ks) {
        std::vector<RigSchluessel>& v = *liste(s, k);
        float qv[4];
        float tv[3];
        float qn[4];
        float tn[3];
        float qc[4];
        float tc[3];
        if (!wertAm(v, static_cast<float>(vor), qv, tv) || !wertAm(v, static_cast<float>(nach), qn, tn) || !wertAm(v, static_cast<float>(bild), qc, tc)) { continue; }
        float q[4];
        float t[3];
        switch (art) {
            case PosenSchieber::Push:
            case PosenSchieber::Relax: {
                float qb[4];
                interpQt(qb, qv, qn, cf);
                const float f = art == PosenSchieber::Push ? -faktor : faktor;
                interpQt(q, qc, qb, f);
                for (int a = 0; a < 3; ++a) { t[a] = interpf(interpf(tn[a], tv[a], cf), tc[a], f); }
                break;
            }
            case PosenSchieber::Breakdown:
                interpQt(q, qv, qn, faktor);
                for (int a = 0; a < 3; ++a) { t[a] = interpf(tn[a], tv[a], faktor); }
                break;
            case PosenSchieber::Blend: {
                const float bf = std::fabs((faktor - 0.5F) * 2.0F);
                const float* zq = faktor < 0.5F ? qv : qn;
                const float* zt = faktor < 0.5F ? tv : tn;
                interpQt(q, qc, zq, bf);
                for (int a = 0; a < 3; ++a) { t[a] = interpf(zt[a], tc[a], bf); }
                break;
            }
            case PosenSchieber::BlendRuhe: {
                const float eins[4] = {1.0F, 0.0F, 0.0F, 0.0F};
                interpQt(q, qc, eins, faktor);
                for (int a = 0; a < 3; ++a) { t[a] = interpf(0.0F, tc[a], faktor); }
                break;
            }
        }
        setzeWert(v, bild, q, t, s.neuInterp);
        henkelNeu(v);
        ++gesetzt;
    }
    return gesetzt;
}

// --- Timing ------------------------------------------------------------------------------------------------------------

namespace {
// Die gewaehlten Schluessel eines Knochens herausnehmen, mit f(bild) neu einsetzen; Treffer auf nicht gewaehlte ersetzen
// sie (BKE_fcurve_merge_duplicate_keys behaelt den gewaehlten).
// Stufe 7: eine IK-Spur in der Wahl (Ziel und Wert je Schluessel gehoeren zusammen) - wie die Liste eines Knochens.
void umsetzenIk(RigSchicht& s, const Wahl& w, int code, const std::function<int(int, int, int)>& f, Wahl& neu) {
    const int sp = ikWahlSpur(code);
    if (sp < 0 || static_cast<std::size_t>(sp) >= s.ik.size()) { return; }
    IkSpur& spur = s.ik[static_cast<std::size_t>(sp)];
    const std::vector<int> idx = wahlIndizes(s, w, code);
    if (idx.empty() || spur.wert.size() != spur.ziel.size()) { return; }
    const int erster = spur.ziel[static_cast<std::size_t>(idx.front())].bild;
    const int letzter = spur.ziel[static_cast<std::size_t>(idx.back())].bild;
    std::vector<std::pair<RigSchluessel, IkWert>> bleiben;
    std::map<int, std::pair<RigSchluessel, IkWert>> neuJe;
    for (std::size_t i = 0; i < spur.ziel.size(); ++i) {
        if (std::find(idx.begin(), idx.end(), static_cast<int>(i)) != idx.end()) {
            RigSchluessel x = spur.ziel[i];
            x.bild = f(x.bild, erster, letzter);
            neuJe[x.bild] = {x, spur.wert[i]};
        } else {
            bleiben.emplace_back(spur.ziel[i], spur.wert[i]);
        }
    }
    bleiben.erase(std::remove_if(bleiben.begin(), bleiben.end(), [&](const auto& x) { return neuJe.count(x.first.bild) != 0U; }), bleiben.end());
    for (const auto& [b, x] : neuJe) {
        bleiben.push_back(x);
        neu.push_back({code, b});
    }
    std::stable_sort(bleiben.begin(), bleiben.end(), [](const auto& a, const auto& b) { return a.first.bild < b.first.bild; });
    spur.ziel.clear();
    spur.wert.clear();
    for (const auto& [z, v] : bleiben) {
        spur.ziel.push_back(z);
        spur.wert.push_back(v);
    }
    ikHenkelNeu(spur);
}

Wahl umsetzen(RigSchicht& s, const Wahl& wahl, const std::function<int(int, int, int)>& f) {
    Wahl neu;
    const Wahl w = wahlMitIk(s, wahl);
    for (const int k : wahlKnochen(w)) {
        if (ikWahlSpur(k) >= 0) {
            umsetzenIk(s, w, k, f, neu);
            continue;
        }
        std::vector<RigSchluessel>* vp = liste(s, k);
        if (vp == nullptr) { continue; }
        std::vector<RigSchluessel>& v = *vp;
        const std::vector<int> idx = wahlIndizes(s, w, k);
        if (idx.empty()) { continue; }
        const int erster = v[static_cast<std::size_t>(idx.front())].bild;
        const int letzter = v[static_cast<std::size_t>(idx.back())].bild;
        std::vector<RigSchluessel> raus;
        std::vector<RigSchluessel> bleiben;
        for (std::size_t i = 0; i < v.size(); ++i) {
            if (std::find(idx.begin(), idx.end(), static_cast<int>(i)) != idx.end()) {
                raus.push_back(v[i]);
            } else {
                bleiben.push_back(v[i]);
            }
        }
        std::map<int, RigSchluessel> neuJe;
        for (RigSchluessel x : raus) {
            x.bild = f(x.bild, erster, letzter);
            neuJe[x.bild] = x;   // zwei gewaehlte auf einem Bild: der spaetere bleibt
        }
        bleiben.erase(std::remove_if(bleiben.begin(), bleiben.end(), [&](const RigSchluessel& x) { return neuJe.count(x.bild) != 0U; }), bleiben.end());
        for (const auto& [b, x] : neuJe) {
            bleiben.push_back(x);
            neu.push_back({k, b});
        }
        sortieren(bleiben);
        v = std::move(bleiben);
        (void)quatFolge(v);
        henkelNeu(v);
    }
    return neu;
}
}  // namespace

Wahl schluesselVerschieben(RigSchicht& s, const Wahl& w, int d) {
    return umsetzen(s, w, [d](int b, int, int) { return b + d; });
}

Wahl schluesselSkalieren(RigSchicht& s, const Wahl& w, float faktor, float drehpunkt) {
    return umsetzen(s, w, [=](int b, int, int) { return static_cast<int>(std::lround(drehpunkt + (static_cast<float>(b) - drehpunkt) * faktor)); });
}

Wahl schluesselEinrasten(RigSchicht& s, const Wahl& w, Einrasten art, int cursor) {
    if (art == Einrasten::AufCursor) {
        return umsetzen(s, w, [cursor](int, int, int) { return cursor; });
    }
    // erster bzw. letzter gewaehlter (ueber alle Knochen) auf den Cursor, alle gleich weit
    int lo = 1 << 30;
    int hi = -(1 << 30);
    for (const SchluesselWahl& x : w) {
        lo = std::min(lo, x.bild);
        hi = std::max(hi, x.bild);
    }
    if (w.empty()) { return w; }
    return schluesselVerschieben(s, w, cursor - (art == Einrasten::ErsterAufCursor ? lo : hi));
}

int zeitEinfuegen(RigSchicht& s, int ab, int n) {
    int bewegt = 0;
    for (auto& v : s.knochen) {
        for (RigSchluessel& k : v) {
            if (k.bild >= ab) {
                k.bild += n;
                ++bewegt;
            }
        }
        henkelNeu(v);
    }
    return bewegt;
}

int zeitEntfernen(RigSchicht& s, int ab, int n) {
    int bewegt = 0;
    for (auto& v : s.knochen) {
        v.erase(std::remove_if(v.begin(), v.end(), [&](const RigSchluessel& k) { return k.bild >= ab && k.bild < ab + n; }), v.end());
        for (RigSchluessel& k : v) {
            if (k.bild >= ab + n) {
                k.bild -= n;
                ++bewegt;
            }
        }
        henkelNeu(v);
    }
    return bewegt;
}

int zeitVersatz(RigSchicht& s, const Wahl& w, float bilder) {
    int geaendert = 0;
    for (const int k : wahlKnochen(w)) {
        std::vector<RigSchluessel>* vp = liste(s, k);
        if (vp == nullptr || vp->size() < 2) { continue; }
        std::vector<RigSchluessel>& v = *vp;
        const std::vector<RigSchluessel> orig = v;
        const float x0 = static_cast<float>(orig.front().bild);
        const float xr = static_cast<float>(orig.back().bild) - x0;
        if (xr <= 0.0F) { continue; }
        for (const int i : wahlIndizes(s, w, k)) {
            const float zeit = static_cast<float>(orig[static_cast<std::size_t>(i)].bild) + bilder;
            const float rel = zeit - x0;
            const float gewickelt = rel - std::floor(rel / xr) * xr + x0;   // floored_fmod
            for (int c = 0; c < kKanaele; ++c) {
                const float yr = kanalWert(orig.back(), c) - kanalWert(orig.front(), c);
                kanalSetzen(v[static_cast<std::size_t>(i)], c, kanalAuswerten(orig, c, gewickelt) + yr * std::floor(rel / xr));
            }
            ++geaendert;
        }
        henkelNeu(v);
    }
    return geaendert;
}

int zeitenTeilen(RigSchicht& s, const std::vector<int>& knochen, int vomLetzten) {
    std::set<int> zeiten;
    if (vomLetzten >= 0) {
        if (liste(s, vomLetzten) == nullptr) { return 0; }
        for (const RigSchluessel& x : *liste(s, vomLetzten)) { zeiten.insert(x.bild); }
    } else {
        for (const int k : knochen) {
            if (liste(s, k) == nullptr) { continue; }
            for (const RigSchluessel& x : *liste(s, k)) { zeiten.insert(x.bild); }
        }
    }
    int gesetzt = 0;
    for (const int k : knochen) {
        if (k == vomLetzten) { continue; }
        std::vector<RigSchluessel>* vp = liste(s, k);
        if (vp == nullptr || vp->empty()) { continue; }
        const std::vector<RigSchluessel> orig = *vp;
        std::vector<RigSchluessel>& v = *vp;
        if (vomLetzten >= 0) {
            v.erase(std::remove_if(v.begin(), v.end(), [&](const RigSchluessel& x) { return zeiten.count(x.bild) == 0U; }), v.end());
        }
        for (const int z : zeiten) {
            if (std::any_of(v.begin(), v.end(), [z](const RigSchluessel& x) { return x.bild == z; })) { continue; }
            float q[4];
            float t[3];
            if (!wertAm(orig, static_cast<float>(z), q, t)) { continue; }
            setzeWert(v, z, q, t, s.neuInterp);
            ++gesetzt;
        }
        henkelNeu(v);
    }
    return gesetzt;
}

// --- Werte -------------------------------------------------------------------------------------------------------------

Wahl werteAendern(RigSchicht& s, const Wahl& w, WertArt art, int cursor) {
    if (art == WertArt::Umkehren || art == WertArt::SpiegelnCursor) {
        Wahl neu = umsetzen(s, w, [=](int b, int erster, int letzter) { return art == WertArt::Umkehren ? erster + letzter - b : 2 * cursor - b; });
        for (const SchluesselWahl& x : neu) {
            std::vector<RigSchluessel>* vp = liste(s, x.knochen);
            for (RigSchluessel& k : *vp) {
                if (k.bild == x.bild) { henkelZeitSpiegeln(k); }
            }
        }
        for (const int k : wahlKnochen(neu)) { henkelNeu(*liste(s, k)); }
        return neu;
    }
    for (const int k : wahlKnochen(w)) {
        std::vector<RigSchluessel>* vp = liste(s, k);
        if (vp == nullptr) { continue; }
        std::vector<RigSchluessel>& v = *vp;
        float qc[4] = {1.0F, 0.0F, 0.0F, 0.0F};
        float tc[3] = {0.0F, 0.0F, 0.0F};
        if (art == WertArt::AktuellerWert && !wertAm(v, static_cast<float>(cursor), qc, tc)) { continue; }
        for (const int i : wahlIndizes(s, w, k)) {
            RigSchluessel& x = v[static_cast<std::size_t>(i)];
            switch (art) {
                case WertArt::Null:
                    x.q[0] = 1.0F;
                    x.q[1] = x.q[2] = x.q[3] = 0.0F;
                    x.t[0] = x.t[1] = x.t[2] = 0.0F;
                    break;
                case WertArt::AktuellerWert:
                    std::memcpy(x.q, qc, sizeof qc);
                    std::memcpy(x.t, tc, sizeof tc);
                    break;
                case WertArt::Negieren:
                    for (int a = 1; a < 4; ++a) { x.q[a] = -x.q[a]; }
                    for (float& tt : x.t) { tt = -tt; }
                    break;
                case WertArt::Umkehren:
                case WertArt::SpiegelnCursor: break;
            }
        }
        (void)quatFolge(v);
        henkelNeu(v);
    }
    return w;
}

// --- Anim Offset -------------------------------------------------------------------------------------------------------

int animOffset(RigSchicht& s, int knochen, const RigSchluessel& vorher, const RigSchluessel& nachher, const std::vector<int>& nurBilder) {
    std::vector<RigSchluessel>* vp = liste(s, knochen);
    if (vp == nullptr) { return 0; }
    // Delta im Rahmen des Knochens: q_d = q_vorher^-1 q_nachher (Einheitsquaternionen), t_d = t_nachher - t_vorher
    float qa[4] = {vorher.q[0], vorher.q[1], vorher.q[2], vorher.q[3]};
    float qb[4] = {nachher.q[0], nachher.q[1], nachher.q[2], nachher.q[3]};
    normiere(qa);
    normiere(qb);
    const float inv[4] = {qa[0], -qa[1], -qa[2], -qa[3]};
    const auto mal = [](const float a[4], const float b[4], float r[4]) {
        r[0] = a[0] * b[0] - a[1] * b[1] - a[2] * b[2] - a[3] * b[3];
        r[1] = a[0] * b[1] + a[1] * b[0] + a[2] * b[3] - a[3] * b[2];
        r[2] = a[0] * b[2] - a[1] * b[3] + a[2] * b[0] + a[3] * b[1];
        r[3] = a[0] * b[3] + a[1] * b[2] - a[2] * b[1] + a[3] * b[0];
    };
    float qd[4];
    mal(inv, qb, qd);
    int n = 0;
    for (RigSchluessel& k : *vp) {
        if (k.bild == nachher.bild) { continue; }
        if (!nurBilder.empty() && std::find(nurBilder.begin(), nurBilder.end(), k.bild) == nurBilder.end()) { continue; }
        float r[4];
        mal(k.q, qd, r);
        std::memcpy(k.q, r, sizeof r);
        for (int a = 0; a < 3; ++a) { k.t[a] += nachher.t[a] - vorher.t[a]; }
        ++n;
    }
    (void)quatFolge(*vp);
    henkelNeu(*vp);
    return n;
}

// --- Spiegeln, Bibliothek, Marker ----------------------------------------------------------------------------------------

int animationSpiegeln(const GlaAnimation& g, const Rig& r, RigSchicht& s, int erstes, int anzahl) {
    // erst alle Posen der Sequenz (die Schicht aendert sich beim Einfuegen)
    std::vector<RigPose> posen;
    std::vector<BoneMatrix> welt;
    RigSchicht ohnePhysik = s;   // Stufe 6: gespiegelt wird die Schicht ohne die Physik-Spur (sie kaeme sonst doppelt)
    ohnePhysik.physik.e.an = false;
    for (int f = erstes; f < erstes + anzahl; ++f) {
        rigWelt(g, ohnePhysik, f, f, 0.0F, welt);
        posen.push_back(poseAus(g, welt));
    }
    int gesetzt = 0;
    for (int f = erstes; f < erstes + anzahl; ++f) { gesetzt += poseEinfuegen(g, r, posen[static_cast<std::size_t>(f - erstes)], true, f, {}, s); }
    return gesetzt;
}

BibliothekPose bibliothekPose(const std::string& name, const RigPose& p) {
    BibliothekPose b;
    b.name = name;
    b.namen = p.namen;
    for (const BoneMatrix& m : p.welt) {
        std::array<float, 12> a{};
        std::memcpy(a.data(), m.m, sizeof m.m);
        b.welt.push_back(a);
    }
    return b;
}

RigPose ausBibliothek(const BibliothekPose& b) {
    RigPose p;
    p.namen = b.namen;
    for (const auto& a : b.welt) {
        BoneMatrix m;
        std::memcpy(m.m, a.data(), sizeof m.m);
        p.welt.push_back(m);
    }
    return p;
}

int markerSchluessel(RigSchicht& s, const std::vector<int>& knochen, int erstes, int letztes) {
    int n = 0;
    for (const int k : knochen) {
        std::vector<RigSchluessel>* vp = liste(s, k);
        if (vp == nullptr || vp->empty()) { continue; }
        const std::vector<RigSchluessel> orig = *vp;
        for (const int b : {erstes, letztes}) {
            if (std::any_of(vp->begin(), vp->end(), [b](const RigSchluessel& x) { return x.bild == b; })) { continue; }
            float q[4];
            float t[3];
            if (!wertAm(orig, static_cast<float>(b), q, t)) { continue; }
            setzeWert(*vp, b, q, t, s.neuInterp);
            ++n;
        }
        henkelNeu(*vp);
    }
    return n;
}

int schleifeSchliessen(RigSchicht& s, const std::vector<int>& knochen, int vonBild, int letztes) {
    int n = 0;
    for (const int k : knochen) {
        std::vector<RigSchluessel>* vp = liste(s, k);
        if (vp == nullptr || vp->empty()) { continue; }
        float q[4];
        float t[3];
        if (!wertAm(*vp, static_cast<float>(vonBild), q, t)) { continue; }
        setzeWert(*vp, letztes, q, t, s.neuInterp);
        henkelNeu(*vp);
        ++n;
    }
    return n;
}

// --- Posen-Bibliothek als Datei (Stufe 4) --------------------------------------------------------------------------------

std::string posenText(const std::vector<BibliothekPose>& posen) {
    std::string t = "JKStudioPosen 1\n";
    char z[64];
    for (const BibliothekPose& p : posen) {
        t += "pose " + p.name + "\n";
        t += "knochen " + std::to_string(p.namen.size()) + "\n";
        for (std::size_t k = 0; k < p.namen.size() && k < p.welt.size(); ++k) {
            t += p.namen[k];
            for (const float v : p.welt[k]) {
                std::snprintf(z, sizeof z, " %.9g", static_cast<double>(v));
                t += z;
            }
            t += "\n";
        }
    }
    return t;
}

bool posenAusText(const std::string& text, std::vector<BibliothekPose>& posen, std::string& fehler) {
    std::vector<std::string> zeilen;
    std::size_t a = 0;
    while (a <= text.size()) {
        std::size_t e = text.find('\n', a);
        if (e == std::string::npos) { e = text.size(); }
        std::string z = text.substr(a, e - a);
        if (!z.empty() && z.back() == '\r') { z.pop_back(); }
        if (!z.empty()) { zeilen.push_back(z); }
        a = e + 1;
    }
    if (zeilen.empty() || zeilen[0] != "JKStudioPosen 1") {
        fehler = "kopf";
        return false;
    }
    std::vector<BibliothekPose> neu;
    for (std::size_t i = 1; i < zeilen.size();) {
        if (zeilen[i].rfind("pose ", 0) != 0 || i + 1 >= zeilen.size() || zeilen[i + 1].rfind("knochen ", 0) != 0) {
            fehler = "zeile " + std::to_string(i + 1);
            return false;
        }
        BibliothekPose p;
        p.name = zeilen[i].substr(5);
        const long n = std::strtol(zeilen[i + 1].c_str() + 8, nullptr, 10);
        i += 2;
        if (n < 0 || static_cast<std::size_t>(n) > zeilen.size() - i) {
            fehler = "knochen";
            return false;
        }
        for (long k = 0; k < n; ++k, ++i) {
            const std::string& z = zeilen[i];
            const std::size_t leer = z.find(' ');
            if (leer == std::string::npos) {
                fehler = "zeile " + std::to_string(i + 1);
                return false;
            }
            std::array<float, 12> w{};
            const char* c = z.c_str() + leer;
            for (float& v : w) {
                char* ende = nullptr;
                v = std::strtof(c, &ende);
                if (ende == c) {
                    fehler = "zeile " + std::to_string(i + 1);
                    return false;
                }
                c = ende;
            }
            p.namen.push_back(z.substr(0, leer));
            p.welt.push_back(w);
        }
        neu.push_back(std::move(p));
    }
    for (BibliothekPose& p : neu) {
        posen.erase(std::remove_if(posen.begin(), posen.end(), [&](const BibliothekPose& x) { return x.name == p.name; }), posen.end());
        posen.push_back(std::move(p));
    }
    return true;
}

// --- Aufraeumen ----------------------------------------------------------------------------------------------------------

namespace {
std::vector<int> alleOder(const RigSchicht& s, const std::vector<int>& knochen) {
    if (!knochen.empty()) { return knochen; }
    std::vector<int> a;
    for (std::size_t k = 0; k < s.knochen.size(); ++k) { a.push_back(static_cast<int>(k)); }
    return a;
}
bool gleich(float a, float b, float s) { return std::fabs(a - b) <= s; }   // IS_EQT
}  // namespace

int ueberfluessigeEntfernen(RigSchicht& s, const std::vector<int>& knochen, float schwelle) {
    int weg = 0;
    for (const int k : alleOder(s, knochen)) {
        std::vector<RigSchluessel>* vp = liste(s, k);
        if (vp == nullptr || vp->size() < 2) { continue; }
        const std::vector<RigSchluessel> alt = *vp;
        std::vector<RigSchluessel> neu{alt.front()};
        for (std::size_t i = 1; i < alt.size(); ++i) {
            const RigSchluessel& last = neu.back();
            const RigSchluessel* nx = i + 1 < alt.size() ? &alt[i + 1] : nullptr;
            bool behalten = false;
            for (int c = 0; c < kKanaele && !behalten; ++c) {
                // clean_fcurve: der Schluessel bleibt, wenn sein Wert vom vorigen behaltenen (oder, mitten drin, vom
                // naechsten) abweicht
                const float prev = kanalWert(last, c);
                const float cur = kanalWert(alt[i], c);
                if (nx != nullptr) {
                    behalten = !gleich(cur, prev, schwelle) || !gleich(cur, kanalWert(*nx, c), schwelle);
                } else {
                    behalten = !gleich(cur, prev, schwelle);
                }
            }
            if (behalten) {
                neu.push_back(alt[i]);
            } else {
                ++weg;
            }
        }
        *vp = std::move(neu);
        henkelNeu(*vp);
    }
    return weg;
}

int statischeEntfernen(RigSchicht& s, const std::vector<int>& knochen, float schwelle) {
    int weg = 0;
    for (const int k : alleOder(s, knochen)) {
        std::vector<RigSchluessel>* vp = liste(s, k);
        if (vp == nullptr || vp->empty()) { continue; }
        std::vector<RigSchluessel>& v = *vp;
        bool statisch = true;
        for (std::size_t i = 1; i < v.size() && statisch; ++i) {
            for (int c = 0; c < kKanaele; ++c) { statisch = statisch && gleich(kanalWert(v[i], c), kanalWert(v[0], c), schwelle); }
        }
        if (!statisch) { continue; }
        weg += static_cast<int>(v.size()) - 1;
        v.resize(1);
        bool einheit = true;
        for (int c = 0; c < kKanaele; ++c) { einheit = einheit && gleich(kanalWert(v[0], c), c == 0 ? 1.0F : 0.0F, schwelle); }
        if (einheit && !s.bindeBasis) {   // clean_fcurve cleardefault: ein Schluessel auf dem Vorgabewert faellt
            v.clear();
            ++weg;
        }
    }
    return weg;
}

int quatFilter(RigSchicht& s, const std::vector<int>& knochen) {
    int n = 0;
    for (const int k : alleOder(s, knochen)) {
        std::vector<RigSchluessel>* vp = liste(s, k);
        if (vp == nullptr) { continue; }
        n += quatFolge(*vp);
        henkelNeu(*vp);
    }
    return n;
}

// --- Kompression ---------------------------------------------------------------------------------------------------------

namespace {
std::uint16_t u16(const std::uint8_t* p) { return static_cast<std::uint16_t>(p[0] | (p[1] << 8)); }
void quatAus14(const std::uint8_t* c, double q[4]) {   // MC_UnCompressQuat: /16383 - 2
    for (int k = 0; k < 4; ++k) { q[k] = static_cast<double>(u16(c + 2 * k)) / 16383.0 - 2.0; }
}
void dreh33(const double q[4], double r[3][3]) {   // ohne Normieren, wie MC_UnCompressQuat
    const double w = q[0], x = q[1], y = q[2], z = q[3];
    r[0][0] = 1 - 2 * (y * y + z * z);
    r[0][1] = 2 * (x * y - w * z);
    r[0][2] = 2 * (x * z + w * y);
    r[1][0] = 2 * (x * y + w * z);
    r[1][1] = 1 - 2 * (x * x + z * z);
    r[1][2] = 2 * (y * z - w * x);
    r[2][0] = 2 * (x * z - w * y);
    r[2][1] = 2 * (y * z + w * x);
    r[2][2] = 1 - 2 * (x * x + y * y);
}
constexpr double kQuant = (4.0 * 3.0 + 2.0 * 3.0 * 2.0 * 1.4142135623730951) / 16383.0;
}  // namespace

double kompressionsGrenze(const std::uint8_t* c) noexcept {
    double q[4];
    quatAus14(c, q);
    const double l2 = q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];
    const double l = std::sqrt(std::max(l2, 1e-12));
    const double qn[4] = {q[0] / l, q[1] / l, q[2] / l, q[3] / l};
    double r[3][3];
    dreh33(qn, r);
    double ir = 0.0;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) { ir += ((i == j ? 1.0 : 0.0) - r[i][j]) * ((i == j ? 1.0 : 0.0) - r[i][j]); }
    }
    return std::fabs(1.0 - l2) * std::sqrt(ir) + kQuant;
}

double drehAbstand(const std::uint8_t* a, const std::uint8_t* b) noexcept {
    double qa[4];
    double qb[4];
    quatAus14(a, qa);
    quatAus14(b, qb);
    double ra[3][3];
    double rb[3][3];
    dreh33(qa, ra);
    dreh33(qb, rb);
    double d = 0.0;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) { d += (ra[i][j] - rb[i][j]) * (ra[i][j] - rb[i][j]); }
    }
    return std::sqrt(d);
}

int verschiebungsStufen(const std::uint8_t* a, const std::uint8_t* b) noexcept {
    int d = 0;
    for (int k = 0; k < 3; ++k) { d = std::max(d, std::abs(static_cast<int>(u16(a + 8 + 2 * k)) - static_cast<int>(u16(b + 8 + 2 * k)))); }
    return d;
}

// --- Reduzieren ----------------------------------------------------------------------------------------------------------

namespace {

struct Ziel {
    const std::uint8_t* bytes = nullptr;
    double grenze = 0.0;
    double r[3][3]{};   // entpackt (wie das Spiel)
    float t[3]{};
};

// Gebacken innerhalb der Grenze? Schnell ueber die Drehmatrix vor der Kompression (|C - P| <= kQuant fuer eine Drehung
// P), nur im Zweifel ueber compressBone. Rueckgabe: Anteil an der Grenze (> 1 heisst daneben), Verschiebung daneben = 2+.
double pruefe(const BoneMatrix& a, const Ziel& z, bool genau) {
    // Verschiebung: (t + 512) * 64 gerundet, Grenze eine Stufe; sicher drin bis 1/128 Abstand vom Ziel, sicher daneben ab
    // 3/128 (dann rundet es mindestens zwei Stufen weg)
    bool tUnsicher = false;
    for (int k = 0; k < 3; ++k) {
        const double d = std::fabs(static_cast<double>(a.m[k][3]) - z.t[k]);
        if (d > 3.0 / 128.0 + 1e-4) { return 2.0 + d; }
        if (d > 1.0 / 128.0 - 1e-4) { tUnsicher = true; }
    }
    double e = 0.0;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) { e += (a.m[i][j] - z.r[i][j]) * (a.m[i][j] - z.r[i][j]); }
    }
    e = std::sqrt(e);
    if (!genau && !tUnsicher) {
        if (e + kQuant <= z.grenze) { return (e + kQuant) / z.grenze * 0.999; }   // sicher drin (Rangfolge: Abstand)
        if (e - kQuant > z.grenze) { return (e - kQuant) / z.grenze; }           // sicher daneben
    }
    g2::Mat3x4 m;
    std::memcpy(m.m, a.m, sizeof m.m);
    g2::CompressStats st;
    const g2::fmt::CompQuatBone c = g2::compressBone(m, g2::CompressOptions{}, st);
    if (verschiebungsStufen(c.comp, z.bytes) > 1) { return 2.0; }
    return drehAbstand(c.comp, z.bytes) / z.grenze;
}

}  // namespace

bool rigReduzieren(const GlaAnimation& g, const RigSchicht& quelle, int erstes, int anzahl, RigSchicht& aus, ReduzierBericht* bericht,
                   const ReduzierAuswahl* auswahl) {
    ReduzierBericht leer;
    ReduzierBericht& br = bericht != nullptr ? *bericht : leer;
    br = ReduzierBericht{};
    aus = RigSchicht{};
    GlaAnimation gebacken;
    BackBericht bb;
    if (!rigBacken(g, quelle, erstes, anzahl, false, g.name, gebacken, &bb)) {
        br.fehler = bb.fehler;
        return false;
    }
    const int nb = static_cast<int>(g.bones.size());
    br.knochen = nb;
    br.bilder = anzahl;
    br.vorher = static_cast<long long>(nb) * anzahl;
    aus.bindeBasis = true;
    aus.neuInterp = Interp::Bezier;
    aus.knochen.resize(static_cast<std::size_t>(nb));
    aus.ik = quelle.ik;   // Stufe 4: die IK wirkt auf den reduzierten Schluesseln wie vorher
    // Stufe 4: die Spanne (relativ) und die Bytes der Quell-GLA (Anhang: Karten-GLA hinter _humanoid)
    int sv = 0;
    int sb = anzahl - 1;
    if (auswahl != nullptr && auswahl->von >= 0) { sv = std::clamp(auswahl->von - erstes, 0, anzahl - 1); }
    if (auswahl != nullptr && auswahl->bis >= 0) { sb = std::clamp(auswahl->bis - erstes, 0, anzahl - 1); }
    if (sb < sv) { std::swap(sv, sb); }
    const bool ganz = sv == 0 && sb == anzahl - 1;
    const auto quellBytes = [&](int f, int b) -> const std::uint8_t* {
        const GlaAnimation* q = &g;
        int fl = erstes + f;
        while (fl >= q->numFrames && q->anhang != nullptr) {
            fl -= q->numFrames;
            q = q->anhang;
        }
        const std::size_t at = static_cast<std::size_t>(fl) * static_cast<std::size_t>(nb) + static_cast<std::size_t>(b);
        if (at >= q->indexes.size() || static_cast<std::size_t>(q->indexes[at]) * 14U + 14U > q->bonePool.size()) { return nullptr; }
        return q->bonePool.data() + static_cast<std::size_t>(q->indexes[at]) * 14U;
    };
    static const std::vector<RigSchluessel> kKeine;
    for (int b = 0; b < nb; ++b) {
        const GlaBone& kb = g.bones[static_cast<std::size_t>(b)];
        std::vector<Ziel> ziel(static_cast<std::size_t>(anzahl));
        std::vector<RigSchluessel> wert(static_cast<std::size_t>(anzahl));   // M je Bild (Bindebasis)
        bool statisch = true;
        bool wieGla = true;   // die gebackenen Bytes sind die der GLA (keine Schluessel, keine IK an diesem Knochen)
        for (int f = 0; f < anzahl; ++f) {
            Ziel& z = ziel[static_cast<std::size_t>(f)];
            z.bytes = gebacken.bonePool.data() + static_cast<std::size_t>(gebacken.indexes[static_cast<std::size_t>(f) * static_cast<std::size_t>(nb) + static_cast<std::size_t>(b)]) * 14U;
            z.grenze = kompressionsGrenze(z.bytes);
            double q[4];
            quatAus14(z.bytes, q);
            dreh33(q, z.r);
            for (int k = 0; k < 3; ++k) { z.t[k] = static_cast<float>(u16(z.bytes + 8 + 2 * k)) / 64.0F - 512.0F; }
            // M = B^-1 A B mit A = Drehung des normierten q und der Verschiebung
            const double l = std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
            const float qn[4] = {static_cast<float>(q[0] / l), static_cast<float>(q[1] / l), static_cast<float>(q[2] / l), static_cast<float>(q[3] / l)};
            BoneMatrix a = matrixAusQuat(qn);
            for (int k = 0; k < 3; ++k) { a.m[k][3] = z.t[k]; }
            const BoneMatrix m = kb.basePoseInv * a * kb.basePose;
            RigSchluessel& w = wert[static_cast<std::size_t>(f)];
            w.bild = erstes + f;
            quatAusMatrix(m, w.q);
            for (int k = 0; k < 3; ++k) { w.t[k] = m.m[k][3]; }
            if (f > 0) {
                const RigSchluessel& p = wert[static_cast<std::size_t>(f - 1)];
                if (w.q[0] * p.q[0] + w.q[1] * p.q[1] + w.q[2] * p.q[2] + w.q[3] * p.q[3] < 0.0F) {
                    for (float& x : w.q) { x = -x; }
                }
                statisch = statisch && std::memcmp(z.bytes, ziel[0].bytes, 14U) == 0;
            }
            const std::uint8_t* qb = quellBytes(f, b);
            wieGla = wieGla && qb != nullptr && std::memcmp(qb, z.bytes, 14U) == 0;
        }
        const auto& qk = static_cast<std::size_t>(b) < quelle.knochen.size() ? quelle.knochen[static_cast<std::size_t>(b)] : kKeine;
        wieGla = wieGla && qk.empty();
        const bool gewaehlt = auswahl == nullptr || auswahl->knochen.empty() || std::find(auswahl->knochen.begin(), auswahl->knochen.end(), b) != auswahl->knochen.end();
        auto& ziele = aus.knochen[static_cast<std::size_t>(b)];
        if (!gewaehlt) {
            // nicht gewaehlt: bleibt (ohne Schluessel die GLA; Bindebasis: die Schluessel der Quelle; sonst je Bild einer)
            if (wieGla) {
                ziele.clear();
            } else if (quelle.bindeBasis) {
                ziele = qk;
                br.behalten += static_cast<long long>(qk.size());
            } else {
                ziele = wert;
                for (RigSchluessel& k : ziele) { k.interp = Interp::Linear; }
                henkelNeu(ziele);
                br.dicht += anzahl;
            }
            br.nachher += static_cast<long long>(ziele.size());
            continue;
        }
        ++br.gewaehlt;
        // Stufe 4, Spanne: ausserhalb feste Schluessel - die der Quelle (Bindebasis) oder einer je Bild
        std::vector<char> fest(static_cast<std::size_t>(anzahl), 0);
        std::vector<const RigSchluessel*> orig(static_cast<std::size_t>(anzahl), nullptr);
        if (!ganz) {
            if (quelle.bindeBasis && !qk.empty()) {
                for (const RigSchluessel& k : qk) {
                    const int f = k.bild - erstes;
                    if (f >= 0 && f < anzahl && (f < sv || f > sb)) {
                        fest[static_cast<std::size_t>(f)] = 1;
                        orig[static_cast<std::size_t>(f)] = &k;
                        ++br.behalten;
                    }
                }
            } else {
                for (int f = 0; f < anzahl; ++f) {
                    if (f < sv || f > sb) {
                        fest[static_cast<std::size_t>(f)] = 1;
                        ++br.dicht;
                    }
                }
            }
        }
        // die Schluessel zu einer Wahl der Bilder (feste Schluessel der Quelle unveraendert); das schlimmste Bild ohne
        // Schluessel (-1: alle innerhalb der Grenze; an den Schluesseln selbst prueft reduziertPruefen mit compressBone).
        // von/bis: nur dort pruefen (linear wirkt ein Schluessel nur bis zu seinen Nachbarn)
        const auto bauen = [&](Interp art, const std::vector<char>& ist, std::vector<RigSchluessel>& v) {
            v.clear();
            for (int f = 0; f < anzahl; ++f) {
                if (ist[static_cast<std::size_t>(f)] == 0) { continue; }
                if (orig[static_cast<std::size_t>(f)] != nullptr) {
                    v.push_back(*orig[static_cast<std::size_t>(f)]);
                    continue;
                }
                RigSchluessel k = wert[static_cast<std::size_t>(f)];
                k.interp = art;
                v.push_back(k);
            }
            henkelNeu(v);
        };
        const auto schlimmstes = [&](const std::vector<char>& ist, const std::vector<RigSchluessel>& v, int von, int bis) {
            double schlimm = 1.0;
            int wo = -1;
            for (int f = von; f <= bis; ++f) {
                if (ist[static_cast<std::size_t>(f)] != 0) { continue; }
                BoneMatrix m;
                (void)schluesselWert(v, static_cast<float>(erstes + f), m);
                const double anteil = pruefe(kb.basePose * m * kb.basePoseInv, ziel[static_cast<std::size_t>(f)], false);
                if (anteil > schlimm) {
                    schlimm = anteil;
                    wo = f;
                }
            }
            return wo;
        };
        // gierig: Schluessel an Anfang und Ende (der Spanne), dann an das Bild mit dem groessten Anteil; danach jeden inneren
        // Schluessel der Spanne probeweise entfernen und nur behalten, wenn ohne ihn ein Bild die Grenze reisst
        const auto gierig = [&](Interp art, std::vector<RigSchluessel>& v, std::vector<char>& ist) {
            ist = fest;
            ist[static_cast<std::size_t>(sv)] = 1;
            if (!statisch || !ganz) { ist[static_cast<std::size_t>(sb)] = 1; }
            for (;;) {
                bauen(art, ist, v);
                const int wo = schlimmstes(ist, v, 0, anzahl - 1);
                if (wo < 0) { break; }
                ist[static_cast<std::size_t>(wo)] = 1;
            }
            std::vector<RigSchluessel> probe;
            for (int f = std::max(sv + 1, 1); f < std::min(sb, anzahl - 1); ++f) {
                if (ist[static_cast<std::size_t>(f)] == 0) { continue; }
                ist[static_cast<std::size_t>(f)] = 0;
                bauen(art, ist, probe);
                int von = 0;
                int bis = anzahl - 1;
                if (art == Interp::Linear && ganz) {
                    von = f - 1;
                    while (von > 0 && ist[static_cast<std::size_t>(von)] == 0) { --von; }
                    bis = f + 1;
                    while (bis < anzahl - 1 && ist[static_cast<std::size_t>(bis)] == 0) { ++bis; }
                }
                if (schlimmstes(ist, probe, von, bis) >= 0) { ist[static_cast<std::size_t>(f)] = 1; }
            }
            bauen(art, ist, v);
        };
        std::vector<RigSchluessel> lin;
        std::vector<RigSchluessel> bz;
        std::vector<char> istLin;
        std::vector<char> istBz;
        gierig(Interp::Linear, lin, istLin);
        const std::vector<char>* ist = &istLin;
        if (statisch && ganz) {
            ++br.statisch;
            ziele = lin;
        } else {
            gierig(Interp::Bezier, bz, istBz);
            const bool nimmLinear = lin.size() < bz.size();
            br.linear += nimmLinear ? 1 : 0;
            br.bezier += nimmLinear ? 0 : 1;
            ziele = nimmLinear ? lin : bz;
            ist = nimmLinear ? &istLin : &istBz;
        }
        for (int f = 0; f < anzahl; ++f) {
            if ((f < sv || f > sb) && (*ist)[static_cast<std::size_t>(f)] != 0 && fest[static_cast<std::size_t>(f)] == 0) { ++br.aussen; }
        }
        br.nachher += static_cast<long long>(ziele.size());
    }
    ReduzierBericht pr;
    if (!reduziertPruefen(g, quelle, aus, erstes, anzahl, pr)) {
        br.fehler = pr.fehler;
        return false;
    }
    br.maxAnteil = pr.maxAnteil;
    br.ueberGrenze += pr.ueberGrenze;
    br.schlimmster = pr.schlimmster;
    return true;
}

bool reduziertPruefen(const GlaAnimation& g, const RigSchicht& quelle, const RigSchicht& reduziert, int erstes, int anzahl, ReduzierBericht& br) {
    GlaAnimation a;
    GlaAnimation b;
    BackBericht ba;
    BackBericht bb;
    if (!rigBacken(g, quelle, erstes, anzahl, false, g.name, a, &ba) || !rigBacken(g, reduziert, erstes, anzahl, false, g.name, b, &bb)) {
        br.fehler = ba.fehler.empty() ? bb.fehler : ba.fehler;
        return false;
    }
    const int nb = static_cast<int>(g.bones.size());
    br.knochen = nb;
    br.bilder = anzahl;
    br.vorher = static_cast<long long>(nb) * anzahl;
    br.nachher = reduziert.schluesselZahl();
    for (std::size_t at = 0; at < a.indexes.size(); ++at) {
        const std::uint8_t* x = a.bonePool.data() + static_cast<std::size_t>(a.indexes[at]) * 14U;
        const std::uint8_t* y = b.bonePool.data() + static_cast<std::size_t>(b.indexes[at]) * 14U;
        const double anteil = drehAbstand(x, y) / kompressionsGrenze(x);
        const int stufen = verschiebungsStufen(x, y);
        br.eineStufe += stufen == 1 ? 1 : 0;
        if (anteil > br.maxAnteil) {
            br.maxAnteil = anteil;
            br.schlimmster = g.bones[at % static_cast<std::size_t>(nb)].name + " Bild " + std::to_string(erstes + static_cast<int>(at / static_cast<std::size_t>(nb)));
        }
        if (anteil > 1.0 || stufen > 1) { ++br.ueberGrenze; }
    }
    return true;
}

}  // namespace bhed::modell
