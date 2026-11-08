// SPDX-License-Identifier: GPL-2.0-or-later
//
// modell/wof/fase_blender.cpp - Chamfer mit Segmenten wie Blenders Bevel (MRESTE 3b), siehe fase_blender.h.
// Portiert aus Blender 5.0 source/blender/bmesh/tools/bmesh_bevel.cc (Copyright 2023 Blender Authors,
// GPL-2.0-or-later). Die Zeilennummern nennen die Stelle im Original (Tag v5.0.0).
#include "fase_blender.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace bhed::modell::wof::fase {

using mi::einheit;
using mi::kreuz;
using mi::laenge;
using mi::punkt;
using mi::winkel;

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kWurzel2 = 1.41421356237309504880;
constexpr double kEpsWinkel = 2.0 * kPi / 180.0;   // BEVEL_EPSILON_ANG = DEG2RADF(2)
constexpr double kEps = 1e-6;                      // BEVEL_EPSILON
constexpr double kEpsQ = 1e-12;                    // BEVEL_EPSILON_SQ

bool istNull(V a) { return a.x == 0.0 && a.y == 0.0 && a.z == 0.0; }
V mitteVon(V a, V b) { return (a + b) * 0.5; }

// nearly_parallel (:464)
bool fastParallel(V d1, V d2) {
    const double w = winkel(d1, d2);
    return std::fabs(w) < kEpsWinkel || std::fabs(w - kPi) < kEpsWinkel;
}

// isect_line_line_v3 (BLI math_geom): der naechste Punkt auf der Geraden v1-v2 zur Geraden v3-v4; 0 bei parallel
int geradenSchnitt(V v1, V v2, V v3, V v4, V& r1) {
    const V a = v2 - v1;
    const V b = v4 - v3;
    const V ab = kreuz(a, b);
    const double div = punkt(ab, ab);
    if (div == 0.0) { return 0; }
    const V c = v3 - v1;
    const V cb = kreuz(c, b);
    r1 = v1 + a * (punkt(cb, ab) / div);
    return std::fabs(punkt(c, ab)) <= 1e-6 ? 1 : 2;
}

// isect_line_plane_v3
bool geradeEbene(V l1, V l2, V ebeneOrt, V ebeneNormale, V& r) {
    const V u = l2 - l1;
    const V h = l1 - ebeneOrt;
    const double d = punkt(ebeneNormale, u);
    if (std::fabs(d) > 1.1920929e-7) {   // FLT_EPSILON
        r = l1 + u * (-punkt(ebeneNormale, h) / d);
        return true;
    }
    return false;
}

// Affine Abbildung wie Blenders 4x4 mit Spalten c[0..3] (c[3] Verschiebung)
struct Affin {
    V c[4];
    [[nodiscard]] V an(V p) const { return c[0] * p.x + c[1] * p.y + c[2] * p.z + c[3]; }
};

// make_unit_square_map (:2138)
bool einheitsQuadrat(V va, V vmid, V vb, Affin& m) {
    const V vaVmid = vmid - va;
    const V vbVmid = vmid - vb;
    if (istNull(vaVmid) || istNull(vbVmid)) { return false; }
    if (std::fabs(winkel(vaVmid, vbVmid) - kPi) <= kEpsWinkel) { return false; }
    const V vo = va - vbVmid;
    const V vd = vo + einheit(kreuz(vbVmid, vaVmid));
    m.c[0] = vmid - va;
    m.c[1] = vmid - vb;
    m.c[2] = vmid + vd - va - vb;
    m.c[3] = va + vb - vmid;
    return true;
}

// invert_m4_m4 fuer eine affine Abbildung
bool umkehren(const Affin& m, Affin& r) {
    const V a = m.c[0], b = m.c[1], c = m.c[2];
    const double det = punkt(a, kreuz(b, c));
    if (det == 0.0) { return false; }
    // Zeilen der Inversen der 3x3 mit Spalten a, b, c
    const V z0 = kreuz(b, c) * (1.0 / det);
    const V z1 = kreuz(c, a) * (1.0 / det);
    const V z2 = kreuz(a, b) * (1.0 / det);
    r.c[0] = V{z0.x, z1.x, z2.x};
    r.c[1] = V{z0.y, z1.y, z2.y};
    r.c[2] = V{z0.z, z1.z, z2.z};
    const V t = m.c[3];
    r.c[3] = V{-punkt(z0, t), -punkt(z1, t), -punkt(z2, t)};
    return true;
}

// make_unit_cube_map (:2195)
Affin einheitsWuerfel(V va, V vb, V vc, V vd) {
    Affin m;
    m.c[0] = (va - vb - vc + vd) * 0.5;
    m.c[1] = (vb - va - vc + vd) * 0.5;
    m.c[2] = (vc - va - vb + vd) * 0.5;
    m.c[3] = (va + vb + vc - vd) * 0.5;
    return m;
}

// superellipse_co (:2232)
double superellipseCo(double x, float r, bool rbig) {
    const double rd = static_cast<double>(r);
    if (rbig) { return std::pow(1.0 - std::pow(x, rd), 1.0 / rd); }
    return 1.0 - std::pow(1.0 - std::pow(1.0 - x, rd), 1.0 / rd);
}

// find_superellipse_chord_endpoint (:7243)
double sehnenEnde(double x0, double dZiel, float r, bool rbig) {
    const double y0 = superellipseCo(x0, r, rbig);
    const double tol = 1e-13;
    const int maxIter = 10;
    double xmin = std::min(x0 + kWurzel2 / 2.0 * dZiel, 1.0);
    double xmax = std::min(x0 + dZiel, 1.0);
    const double ymin = superellipseCo(xmin, r, rbig);
    const double ymax = superellipseCo(xmax, r, rbig);
    double dmaxerr = std::sqrt(std::pow(xmax - x0, 2) + std::pow(ymax - y0, 2)) - dZiel;
    double dminerr = std::sqrt(std::pow(xmin - x0, 2) + std::pow(ymin - y0, 2)) - dZiel;
    double xnew = xmax - dmaxerr * (xmax - xmin) / (dmaxerr - dminerr);
    bool obenZuletzt = true;
    for (int it = 0; it < maxIter; ++it) {
        const double ynew = superellipseCo(xnew, r, rbig);
        const double dnewerr = std::sqrt(std::pow(xnew - x0, 2) + std::pow(ynew - y0, 2)) - dZiel;
        if (std::fabs(dnewerr) < tol) { break; }
        if (dnewerr < 0) {
            xmin = xnew;
            dminerr = dnewerr;
            if (!obenZuletzt) {
                xnew = (dmaxerr / 2 * xmin - dminerr * xmax) / (dmaxerr / 2 - dminerr);
            } else {
                xnew = xmax - dmaxerr * (xmax - xmin) / (dmaxerr - dminerr);
            }
            obenZuletzt = false;
        } else {
            xmax = xnew;
            dmaxerr = dnewerr;
            if (obenZuletzt) {
                xnew = (dmaxerr * xmin - dminerr / 2 * xmax) / (dmaxerr - dminerr / 2);
            } else {
                xnew = xmax - dmaxerr * (xmax - xmin) / (dmaxerr - dminerr);
            }
            obenZuletzt = true;
        }
    }
    return xnew;
}

// find_even_superellipse_chords_general (:7307)
void sehnenAllgemein(int seg, float r, std::vector<double>& xs, std::vector<double>& ys) {
    const int smoothIterMax = 10;
    const double fehlerTol = 1e-7;
    const int imax = (seg + 1) / 2 - 1;
    const bool ungerade = seg % 2 != 0;
    const bool rbig = r > 1.0F;
    const double mx = rbig ? std::pow(0.5, 1.0 / static_cast<double>(r)) : 1 - std::pow(0.5, 1.0 / static_cast<double>(r));
    for (int i = 0; i <= imax; ++i) {
        xs[static_cast<std::size_t>(i)] = i * mx / seg * 2;
        ys[static_cast<std::size_t>(i)] = superellipseCo(xs[static_cast<std::size_t>(i)], r, rbig);
    }
    ys[0] = 1;
    for (int it = 0; it < smoothIterMax; ++it) {
        double sum = 0.0;
        double dmin = 2.0;
        double dmax = 0.0;
        for (int i = 0; i < imax; ++i) {
            const std::size_t u = static_cast<std::size_t>(i);
            const double d = std::sqrt(std::pow(xs[u + 1] - xs[u], 2) + std::pow(ys[u + 1] - ys[u], 2));
            sum += d;
            dmax = std::max(d, dmax);
            dmin = std::min(d, dmin);
        }
        double davg = 0.0;
        const std::size_t im = static_cast<std::size_t>(imax);
        if (ungerade) {
            sum += kWurzel2 / 2 * (ys[im] - xs[im]);
            davg = sum / (imax + 0.5);
        } else {
            sum += std::sqrt(std::pow(xs[im] - mx, 2) + std::pow(ys[im] - mx, 2));
            davg = sum / (imax + 1.0);
        }
        bool genau = true;
        if (dmax - davg > fehlerTol) { genau = false; }
        if (dmin - davg < fehlerTol) { genau = false; }
        if (genau) { break; }
        for (int i = 1; i <= imax; ++i) {
            const std::size_t u = static_cast<std::size_t>(i);
            xs[u] = sehnenEnde(xs[u - 1], davg, r, rbig);
            ys[u] = superellipseCo(xs[u], r, rbig);
        }
    }
    if (!ungerade) {
        xs[static_cast<std::size_t>(imax + 1)] = mx;
        ys[static_cast<std::size_t>(imax + 1)] = mx;
    }
    for (int i = imax + 1; i <= seg; ++i) {
        ys[static_cast<std::size_t>(i)] = xs[static_cast<std::size_t>(seg - i)];
        xs[static_cast<std::size_t>(i)] = ys[static_cast<std::size_t>(seg - i)];
    }
    if (!rbig) {
        for (int i = 0; i <= seg; ++i) {
            const std::size_t u = static_cast<std::size_t>(i);
            const double t = xs[u];
            xs[u] = 1.0 - ys[u];
            ys[u] = 1.0 - t;
        }
    }
}

// find_even_superellipse_chords (:7403)
void gleicheSehnen(int n, float r, std::vector<double>& xs, std::vector<double>& ys) {
    xs.assign(static_cast<std::size_t>(n + 1), 0.0);
    ys.assign(static_cast<std::size_t>(n + 1), 0.0);
    const bool ungerade = n % 2 != 0;
    const int n2 = n / 2;
    const auto x = [&](int i) -> double& { return xs[static_cast<std::size_t>(i)]; };
    const auto y = [&](int i) -> double& { return ys[static_cast<std::size_t>(i)]; };
    if (r == kRLinie) {
        for (int i = 0; i <= n; ++i) {
            x(i) = static_cast<double>(i) / n;
            y(i) = 1.0 - static_cast<double>(i) / n;
        }
        return;
    }
    if (r == kRKreis) {
        const double t = kPi / 2.0 / n;
        for (int i = 0; i <= n; ++i) {
            x(i) = std::sin(i * t);
            y(i) = std::cos(i * t);
        }
        return;
    }
    if (r == kRQuadratInnen) {
        const double t = ungerade ? 1.0 / (n2 + kWurzel2 / 2.0) : 1.0 / n2;
        for (int i = 0; i <= n2; ++i) {
            x(i) = 0.0;
            y(i) = 1.0 - static_cast<double>(i) * t;
            x(n - i) = y(i);
            y(n - i) = x(i);
        }
        return;
    }
    if (r == kRQuadrat) {
        const double t = ungerade ? 1.0 / (n2 + kWurzel2 / 2) : 1.0 / n2;
        for (int i = 0; i <= n2; ++i) {
            x(i) = static_cast<double>(i) * t;
            y(i) = 1.0;
            x(n - i) = y(i);
            y(n - i) = x(i);
        }
        return;
    }
    sehnenAllgemein(n, r, xs, ys);
}

// find_profile_fullness (:7479)
float fuelleVon(int nseg, float r, float profil) {
    static const float kKreis[11] = {0.0F, 0.559F, 0.642F, 0.551F, 0.646F, 0.624F, 0.646F, 0.619F, 0.647F, 0.639F, 0.647F};
    if (r == kRLinie) { return 0.0F; }
    if (r == kRKreis && nseg > 0 && nseg <= 11) { return kKreis[nseg - 1]; }
    if (nseg % 2 == 0) { return 2.4506F * profil - 0.00000300F * static_cast<float>(nseg) - 0.6266F; }
    return 2.3635F * profil + 0.000152F * static_cast<float>(nseg) - 0.6060F;
}

// calculate_profile_segments (:2280)
void profilStrecke(const Profil& p, const Affin& m, bool mitAbb, int ns, const std::vector<double>& xs, const std::vector<double>& ys,
                   std::vector<V>& aus) {
    aus.assign(static_cast<std::size_t>(ns + 1), V{});
    for (int k = 0; k <= ns; ++k) {
        V co;
        if (k == 0) {
            co = p.start;
        } else if (k == ns) {
            co = p.ende;
        } else if (mitAbb) {
            co = m.an(V{xs[static_cast<std::size_t>(k)], ys[static_cast<std::size_t>(k)], 0.0});
        } else {
            co = p.start + (p.ende - p.start) * (static_cast<double>(k) / static_cast<double>(ns));
        }
        V& z = aus[static_cast<std::size_t>(k)];
        if (!istNull(p.projRichtung)) {
            if (!geradeEbene(co, co + p.projRichtung, p.ebeneOrt, p.ebeneNormale, z)) { z = co; }
        } else {
            z = co;
        }
    }
}

// snap_to_superellipsoid (:2400), in float wie das Original
void aufSuperellipsoid(V& co, float superR, bool mittellinie) {
    const float r = superR;
    if (r == kRKreis) {
        co = einheit(co);
        return;
    }
    const float a = std::max(0.0F, static_cast<float>(co.x));
    const float b = std::max(0.0F, static_cast<float>(co.y));
    const float c = std::max(0.0F, static_cast<float>(co.z));
    float x = a, y = b, z = c;
    if (r == kRQuadrat || r == kRQuadratInnen) {
        z = 0.0F;
        x = std::min(1.0F, x);
        y = std::min(1.0F, y);
        if (r == kRQuadrat) {
            const float dx = 1.0F - x;
            const float dy = 1.0F - y;
            if (dx < dy) {
                x = 1.0F;
                y = mittellinie ? 1.0F : y;
            } else {
                y = 1.0F;
                x = mittellinie ? 1.0F : x;
            }
        } else {
            if (x < y) {
                x = 0.0F;
                y = mittellinie ? 0.0F : y;
            } else {
                y = 0.0F;
                x = mittellinie ? 0.0F : x;
            }
        }
    } else {
        const float rinv = 1.0F / r;
        if (a == 0.0F) {
            if (b == 0.0F) {
                x = 0.0F;
                y = 0.0F;
                z = std::pow(c, rinv);
            } else {
                x = 0.0F;
                y = std::pow(1.0F / (1.0F + std::pow(c / b, r)), rinv);
                z = c * y / b;
            }
        } else {
            x = std::pow(1.0F / (1.0F + std::pow(b / a, r) + std::pow(c / a, r)), rinv);
            y = b * x / a;
            z = c * x / a;
        }
    }
    co = V{x, y, z};
}

// --- VMesh -------------------------------------------------------------------------------------------------------

Gitter neuesGitter(int n, int ns) {   // new_adj_vmesh (:4020)
    Gitter g;
    g.n = n;
    g.ns = ns;
    g.orte.assign(static_cast<std::size_t>(n * (ns / 2 + 1) * (ns + 1)), V{});
    return g;
}

V& kanonOrt(Gitter& g, int i, int j, int k) {
    int a = 0, b = 0, c = 0;
    g.kanon(i, j, k, a, b, c);
    return g.at(a, b, c);
}

void gleichwertigeKopieren(Gitter& g) {   // vmesh_copy_equiv_verts (:4073)
    const int ns2 = g.ns / 2;
    for (int i = 0; i < g.n; ++i) {
        for (int j = 0; j <= ns2; ++j) {
            for (int k = 0; k <= g.ns; ++k) {
                if (Gitter::istKanon(g.ns, i, j, k)) { continue; }
                g.at(i, j, k) = kanonOrt(g, i, j, k);
            }
        }
    }
}

V gitterMitte(const Gitter& g) {   // vmesh_center
    const int ns2 = g.ns / 2;
    if (g.ns % 2 != 0) {
        V s;
        for (int i = 0; i < g.n; ++i) { s = s + g.at(i, ns2, ns2); }
        return s * (1.0 / g.n);
    }
    return g.at(0, ns2, ns2);
}

V mittel4(V a, V b, V c, V d) { return (a + b + c + d) * 0.25; }   // avg4

float sabinGamma(int n) {   // sabin_gamma (:4120)
    if (n < 3) { return 0.0F; }
    if (n == 3) { return 0.065247584F; }
    if (n == 4) { return 0.25F; }
    if (n == 5) { return 0.401983447F; }
    if (n == 6) { return 0.523423277F; }
    const double k = std::cos(kPi / static_cast<double>(n));
    const double k2 = k * k;
    const double k4 = k2 * k2;
    const double k6 = k4 * k2;
    const double y = std::pow(std::sqrt(3.0) * std::sqrt(64.0 * k6 - 144.0 * k4 + 135.0 * k2 - 27.0) + 9.0 * k, 1.0 / 3.0);
    const double x = 0.480749856769136 * y - (0.231120424783545 * (12.0 * k2 - 9.0)) / y;
    return static_cast<float>((k * x + 2.0 * k2 - 1.0) / (x * x * (k * x + 1.0)));
}

void gitterAnteile(const Gitter& g, std::vector<float>& frac, int i) {   // fill_vmesh_fracs (:4151)
    const int ns = g.ns;
    frac.assign(static_cast<std::size_t>(ns + 1), 0.0F);
    float total = 0.0F;
    for (int k = 0; k < ns; ++k) {
        total += static_cast<float>(laenge(g.at(i, 0, k) - g.at(i, 0, k + 1)));
        frac[static_cast<std::size_t>(k + 1)] = total;
    }
    if (total > 0.0F) {
        for (int k = 1; k <= ns; ++k) { frac[static_cast<std::size_t>(k)] /= total; }
    } else {
        frac[static_cast<std::size_t>(ns)] = 1.0F;
    }
}

void profilAnteile(const Abstaende& a, const Profil& p, std::vector<float>& frac, int ns) {   // fill_profile_fracs (:4174)
    frac.assign(static_cast<std::size_t>(ns + 1), 0.0F);
    float total = 0.0F;
    V co = p.start;
    for (int k = 0; k < ns; ++k) {
        const V weiter = profilPunkt(p, k + 1, ns, a);
        total += static_cast<float>(laenge(co - weiter));
        frac[static_cast<std::size_t>(k + 1)] = total;
        co = weiter;
    }
    if (total > 0.0F) {
        for (int k = 1; k <= ns; ++k) { frac[static_cast<std::size_t>(k)] /= total; }
    } else {
        frac[static_cast<std::size_t>(ns)] = 1.0F;
    }
}

int anteilBereich(const std::vector<float>& frac, int n, float f, float& rest) {   // interp_range (:4199)
    for (int i = 0; i < n; ++i) {
        if (f <= frac[static_cast<std::size_t>(i + 1)]) {
            const float r = f - frac[static_cast<std::size_t>(i)];
            rest = r == 0 ? 0.0F : r / (frac[static_cast<std::size_t>(i + 1)] - frac[static_cast<std::size_t>(i)]);
            if (i == n - 1 && rest == 1.0F) {
                i = n;
                rest = 0.0F;
            }
            return i;
        }
    }
    rest = 0.0F;
    return n;
}

// interp_vmesh (:4224)
Gitter gitterUmrechnen(const Abstaende& a, const std::vector<Profil>& rand, Gitter& vin, int nseg) {
    const int n = vin.n;
    const int nsIn = vin.ns;
    const int nseg2 = nseg / 2;
    const int ungerade = nseg % 2;
    Gitter vout = neuesGitter(n, nseg);
    std::vector<float> prevFrac, frac, newFrac, prevNewFrac;
    gitterAnteile(vin, prevFrac, n - 1);
    profilAnteile(a, rand[static_cast<std::size_t>(n - 1)], prevNewFrac, nseg);
    for (int i = 0; i < n; ++i) {
        gitterAnteile(vin, frac, i);
        profilAnteile(a, rand[static_cast<std::size_t>(i)], newFrac, nseg);
        for (int j = 0; j <= nseg2 - 1 + ungerade; ++j) {
            for (int k = 0; k <= nseg2; ++k) {
                float restk = 0.0F;
                float restkprev = 0.0F;
                const int kIn = anteilBereich(frac, nsIn, newFrac[static_cast<std::size_t>(k)], restk);
                const int kInPrev = anteilBereich(prevFrac, nsIn, prevNewFrac[static_cast<std::size_t>(nseg - j)], restkprev);
                int jIn = nsIn - kInPrev;
                float restj = -restkprev;
                if (restj > -static_cast<float>(kEps)) {
                    restj = 0.0F;
                } else {
                    jIn = jIn - 1;
                    restj = 1.0F + restj;
                }
                V co;
                if (restj < static_cast<float>(kEps) && restk < static_cast<float>(kEps)) {
                    co = kanonOrt(vin, i, jIn, kIn);
                } else {
                    const int j0 = (restj < static_cast<float>(kEps) || jIn == nsIn) ? 0 : 1;
                    const int k0 = (restk < static_cast<float>(kEps) || kIn == nsIn) ? 0 : 1;
                    const V q0 = kanonOrt(vin, i, jIn, kIn);
                    const V q1 = kanonOrt(vin, i, jIn, kIn + k0);
                    const V q2 = kanonOrt(vin, i, jIn + j0, kIn + k0);
                    const V q3 = kanonOrt(vin, i, jIn + j0, kIn);
                    const double u = restk, v = restj;   // interp_bilinear_quad_v3
                    co = q0 * ((1 - u) * (1 - v)) + q1 * (u * (1 - v)) + q2 * (u * v) + q3 * ((1 - u) * v);
                }
                vout.at(i, j, k) = co;
            }
        }
        prevFrac = frac;
        prevNewFrac = newFrac;
    }
    if (ungerade == 0) { vout.at(0, nseg2, nseg2) = gitterMitte(vin); }
    gleichwertigeKopieren(vout);
    return vout;
}

// cubic_subdiv (:4296), Catmull-Clark mit Randregeln (Levin 1999)
Gitter kubischTeilen(const Abstaende& a, const std::vector<Profil>& rand, Gitter& vin) {
    const int n = vin.n;
    const int nsIn = vin.ns;
    const int nsIn2 = nsIn / 2;
    const int nsOut = 2 * nsIn;
    Gitter vout = neuesGitter(n, nsOut);
    const auto glaetten = [](V co, V co1, V co2) {
        const V acc = co1 + co2 + co * -2.0;
        return co + acc * (-1.0 / 6.0);
    };
    for (int i = 0; i < n; ++i) {
        vout.at(i, 0, 0) = vin.at(i, 0, 0);
        for (int k = 1; k < nsIn; ++k) {
            const V co = glaetten(vin.at(i, 0, k), vin.at(i, 0, k - 1), vin.at(i, 0, k + 1));
            kanonOrt(vout, i, 0, 2 * k) = co;
        }
    }
    for (int i = 0; i < n; ++i) {
        for (int k = 1; k < nsOut; k += 2) {
            const V co = profilPunkt(rand[static_cast<std::size_t>(i)], k, nsOut, a);
            kanonOrt(vout, i, 0, k) = glaetten(co, kanonOrt(vout, i, 0, k - 1), kanonOrt(vout, i, 0, k + 1));
        }
    }
    gleichwertigeKopieren(vout);
    for (int i = 0; i < n; ++i) {
        for (int k = 0; k < nsIn; ++k) { vin.at(i, 0, k) = vout.at(i, 0, 2 * k); }
    }
    gleichwertigeKopieren(vin);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < nsIn2; ++j) {
            for (int k = 0; k < nsIn2; ++k) {
                vout.at(i, 2 * j + 1, 2 * k + 1) = mittel4(vin.at(i, j, k), vin.at(i, j, k + 1), vin.at(i, j + 1, k), vin.at(i, j + 1, k + 1));
            }
        }
    }
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < nsIn2; ++j) {
            for (int k = 1; k <= nsIn2; ++k) {
                vout.at(i, 2 * j + 1, 2 * k) =
                    mittel4(vin.at(i, j, k), vin.at(i, j + 1, k), kanonOrt(vout, i, 2 * j + 1, 2 * k - 1), kanonOrt(vout, i, 2 * j + 1, 2 * k + 1));
            }
        }
    }
    for (int i = 0; i < n; ++i) {
        for (int j = 1; j < nsIn2; ++j) {
            for (int k = 0; k < nsIn2; ++k) {
                vout.at(i, 2 * j, 2 * k + 1) =
                    mittel4(vin.at(i, j, k), vin.at(i, j, k + 1), kanonOrt(vout, i, 2 * j - 1, 2 * k + 1), kanonOrt(vout, i, 2 * j + 1, 2 * k + 1));
            }
        }
    }
    {
        const double gamma = 0.25;
        const double beta = -gamma;
        for (int i = 0; i < n; ++i) {
            for (int j = 1; j < nsIn2; ++j) {
                for (int k = 1; k <= nsIn2; ++k) {
                    const V co1 = mittel4(kanonOrt(vout, i, 2 * j, 2 * k - 1), kanonOrt(vout, i, 2 * j, 2 * k + 1), kanonOrt(vout, i, 2 * j - 1, 2 * k),
                                          kanonOrt(vout, i, 2 * j + 1, 2 * k));
                    const V co2 = mittel4(kanonOrt(vout, i, 2 * j - 1, 2 * k - 1), kanonOrt(vout, i, 2 * j + 1, 2 * k - 1),
                                          kanonOrt(vout, i, 2 * j - 1, 2 * k + 1), kanonOrt(vout, i, 2 * j + 1, 2 * k + 1));
                    vout.at(i, 2 * j, 2 * k) = co1 + co2 * beta + vin.at(i, j, k) * gamma;
                }
            }
        }
    }
    gleichwertigeKopieren(vout);
    {
        const double gamma = static_cast<double>(sabinGamma(n));
        const double beta = -gamma;
        V co1, co2;
        for (int i = 0; i < n; ++i) {
            co1 = co1 + vout.at(i, nsIn, nsIn - 1);
            co2 = co2 + vout.at(i, nsIn - 1, nsIn - 1);
            co2 = co2 + vout.at(i, nsIn - 1, nsIn + 1);
        }
        const V co = co1 * (1.0 / n) + co2 * (beta / (2.0 * n)) + vin.at(0, nsIn2, nsIn2) * gamma;
        for (int i = 0; i < n; ++i) { vout.at(i, nsIn, nsIn) = co; }
    }
    for (int i = 0; i < n; ++i) {
        const int inext = (i + 1) % n;
        for (int k = 0; k <= nsOut; ++k) {
            const V co = profilPunkt(rand[static_cast<std::size_t>(i)], k, nsOut, a);
            vout.at(i, 0, k) = co;
            if (k >= nsIn && k < nsOut) { vout.at(inext, nsOut - k, 0) = co; }
        }
    }
    return vout;
}

// make_cube_corner_square (:4486)
Gitter wuerfelEckeQuadrat(int nseg) {
    const int ns2 = nseg / 2;
    Gitter g = neuesGitter(3, nseg);
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j <= ns2; ++j) {
            for (int k = 0; k <= ns2; ++k) {
                if (!Gitter::istKanon(nseg, i, j, k)) { continue; }
                double c[3] = {0.0, 0.0, 0.0};
                c[i] = 1.0;
                c[(i + 1) % 3] = static_cast<double>(static_cast<float>(k) * 2.0F / static_cast<float>(nseg));
                c[(i + 2) % 3] = static_cast<double>(static_cast<float>(j) * 2.0F / static_cast<float>(nseg));
                g.at(i, j, k) = V{c[0], c[1], c[2]};
            }
        }
    }
    gleichwertigeKopieren(g);
    return g;
}

// make_cube_corner_adj_vmesh (:4550): das Gitter auf dem Achtel der Einheitskugel (Superellipsoid)
Gitter wuerfelEcke(const Abstaende& a) {
    const int nseg = a.seg;
    const float r = a.r;
    if (r == kRQuadrat) { return wuerfelEckeQuadrat(nseg); }
    std::vector<Profil> rand(3);
    const V achse[3] = {V{1, 0, 0}, V{0, 1, 0}, V{0, 0, 1}};
    Gitter vm0 = neuesGitter(3, 2);
    for (int i = 0; i < 3; ++i) {
        Profil& p = rand[static_cast<std::size_t>(i)];
        double c[3] = {0.0, 0.0, 0.0};
        c[i] = 1.0;
        c[(i + 1) % 3] = 1.0;
        p.r = r;
        p.start = achse[i];
        p.ende = achse[(i + 1) % 3];
        p.mitte = V{c[0], c[1], c[2]};
        vm0.at(i, 0, 0) = p.start;
        p.ebeneOrt = p.start;
        p.ebeneNormale = kreuz(p.start, p.ende);
        p.projRichtung = p.ebeneNormale;
        berechnen(p, a);
        vm0.at(i, 0, 1) = profilPunkt(p, 1, 2, a);
    }
    V co = V{1, 1, 1} * static_cast<double>(0.57735026918962576451F);   // M_SQRT1_3
    if (nseg > 2) {
        if (r > 1.5F) {
            co = co * static_cast<double>(1.4F);
        } else if (r < 0.75F) {
            co = co * static_cast<double>(0.6F);
        }
    }
    vm0.at(0, 1, 1) = co;
    gleichwertigeKopieren(vm0);
    Gitter vm1 = vm0;
    while (vm1.ns < nseg) { vm1 = kubischTeilen(a, rand, vm1); }
    if (vm1.ns != nseg) { vm1 = gitterUmrechnen(a, rand, vm1, nseg); }
    const int ns2 = nseg / 2;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j <= ns2; ++j) {
            for (int k = 0; k <= nseg; ++k) { aufSuperellipsoid(vm1.at(i, j, k), r, false); }
        }
    }
    return vm1;
}

// tri_corner_adj_vmesh (:4682)
Gitter dreiEcke(const EckDaten& e, const Abstaende& a) {
    const Affin m = einheitsWuerfel(e.seiten[0].start, e.seiten[1].start, e.seiten[2].start, e.v);
    Gitter g = wuerfelEcke(a);
    for (V& p : g.orte) { p = m.an(p); }
    return g;
}

// adj_vmesh (:4714)
Gitter anpassGitter(const EckDaten& e, const Abstaende& a) {
    const int n = static_cast<int>(e.seiten.size());
    if (n == 3 && e.triTest != -1 && a.r != kRQuadratInnen) { return dreiEcke(e, a); }
    const int nseg = a.seg;
    Gitter vm0 = neuesGitter(n, 2);
    V mitte;
    for (int i = 0; i < n; ++i) {
        const Profil& p = e.seiten[static_cast<std::size_t>(i)];
        vm0.at(i, 0, 0) = p.start;
        vm0.at(i, 0, 1) = profilPunkt(p, 1, 2, a);
        mitte = mitte + p.start;
    }
    mitte = mitte * (1.0 / n);
    const V richtung = e.v - mitte;
    if (punkt(richtung, richtung) > kEpsQ) {
        vm0.at(0, 1, 1) = mitte + richtung * static_cast<double>(a.fuelle);
    } else {
        vm0.at(0, 1, 1) = mitte;
    }
    gleichwertigeKopieren(vm0);
    Gitter vm1 = vm0;
    do {
        vm1 = kubischTeilen(a, e.seiten, vm1);
    } while (vm1.ns < nseg);
    if (vm1.ns != nseg) { vm1 = gitterUmrechnen(a, e.seiten, vm1, nseg); }
    return vm1;
}

// closest_to_plane_v3 / closest_to_line_segment_v3
V aufEbene(V p, V ebeneOrt, V normale) { return p - normale * (punkt(normale, p - ebeneOrt) / punkt(normale, normale)); }
V aufStrecke(V p, V a, V b) {
    const V d = b - a;
    const double dd = punkt(d, d);
    double t = dd > 0.0 ? punkt(p - a, d) / dd : 0.0;
    t = std::clamp(t, 0.0, 1.0);
    return a + d * t;
}

// snap_to_pipe_profile (:4772)
void aufRohrProfil(const Profil& pro, V kanteA, V kanteB, bool mittellinie, V& co) {
    const V d = pro.start - pro.ende;
    if (std::fabs(d.x) <= kEps && std::fabs(d.y) <= kEps && std::fabs(d.z) <= kEps) {
        co = pro.start;
        return;
    }
    const V edir = kanteA - kanteB;
    const V s = aufEbene(pro.start, co, edir);
    const V en = aufEbene(pro.ende, co, edir);
    const V mi = aufEbene(pro.mitte, co, edir);
    Affin m, minv;
    if (einheitsQuadrat(s, mi, en, m) && umkehren(m, minv)) {
        V p = minv.an(co);
        aufSuperellipsoid(p, pro.r, mittellinie);
        co = m.an(p);
    } else {
        co = aufStrecke(co, s, en);
    }
}

// pipe_adj_vmesh (:4829)
Gitter rohrGitter(const EckDaten& e, const Abstaende& a) {
    Gitter g = anpassGitter(e, a);
    const int n = g.n;
    const int ns = g.ns;
    const int ns2 = ns / 2;
    const int ip1 = e.pipe;
    const int ip2 = (e.pipe + 2) % n;
    const Profil& pro = e.seiten[static_cast<std::size_t>(e.pipe)];
    for (int i = 0; i < n; ++i) {
        for (int j = 1; j <= ns2; ++j) {
            for (int k = 0; k <= ns2; ++k) {
                if (!Gitter::istKanon(ns, i, j, k)) { continue; }
                const bool gerade = ns % 2 == 0;
                const bool mittellinie = gerade && k == ns2 && ((i == 0 && j == ns2) || i == ip1 || i == ip2);
                aufRohrProfil(pro, e.pipeA, e.pipeB, mittellinie, g.at(i, j, k));
            }
        }
    }
    return g;
}

}  // namespace

float superR(float profil) {
    float r = -std::log(2.0F) / std::log(std::sqrt(profil));
    if (profil >= 0.950F) {
        r = kRQuadrat;
    } else if (std::fabs(r - kRKreis) < 1e-4F) {
        r = kRKreis;
    } else if (std::fabs(r - kRLinie) < 1e-4F) {
        r = kRLinie;
    } else if (r < 1e-4F) {
        r = kRQuadratInnen;
    }
    return r;
}

Abstaende abstaende(int seg, float profil) {
    Abstaende a;
    a.seg = seg;
    a.profil = profil;
    a.r = superR(profil);
    if (seg <= 1) { return a; }
    int p2 = 1;
    while (p2 < seg) { p2 *= 2; }   // power_of_2_max_i
    a.seg2 = std::max(p2, 4);
    gleicheSehnen(seg, a.r, a.xs, a.ys);
    if (a.seg2 == seg) {
        a.xs2 = a.xs;
        a.ys2 = a.ys;
    } else {
        gleicheSehnen(a.seg2, a.r, a.xs2, a.ys2);
    }
    a.fuelle = fuelleVon(seg, a.r, profil);
    return a;
}

void kantenProfil(Profil& p, V start, V ende, const KantenDaten& k, float r) {
    bool linear = false;
    p.r = r;
    p.projRichtung = einheit(k.v - k.anderes);
    if (geradenSchnitt(k.v, k.anderes, start, ende, p.mitte) == 0) { p.mitte = k.v; }   // project_to_edge
    p.start = start;
    p.ende = ende;
    V d1 = einheit(p.mitte - start);
    V d2 = einheit(p.mitte - ende);
    p.ebeneNormale = einheit(kreuz(d1, d2));
    if (fastParallel(d1, d2)) {
        // Start, Mitte und Ende auf einer Geraden: die Mitte auf v oder den Treffpunkt der versetzten Nachbarlinien
        p.mitte = k.v;
        if (k.vorGefast && k.nachGefast && k.gewaehlt >= 3) {
            const V d3 = einheit(k.dVor);
            const V d4 = einheit(k.dNach);
            if (fastParallel(d3, d4)) {
                p.mitte = mitteVon(start, ende);
                linear = true;
            } else {
                V treff;
                if (geradenSchnitt(start, start + d3, ende, ende + d4, treff) != 0) {
                    p.mitte = treff;
                } else {
                    p.mitte = mitteVon(start, ende);
                    linear = true;
                }
            }
        }
        p.ende = ende;
        d1 = einheit(p.mitte - start);
        d2 = einheit(p.mitte - ende);
        p.ebeneNormale = einheit(kreuz(d1, d2));
        if (fastParallel(d1, d2)) {
            linear = true;
        } else {
            p.ebeneOrt = k.v;
            p.projRichtung = p.ebeneNormale;
        }
    }
    p.ebeneOrt = start;
    if (linear) { linienProfil(p, start, ende); }
}

void linienProfil(Profil& p, V start, V ende) {
    p.r = kRLinie;
    p.start = start;
    p.ende = ende;
    p.mitte = mitteVon(start, ende);
    p.ebeneOrt = V{};
    p.ebeneNormale = V{};
    p.projRichtung = V{};
}

void weldEbenen(Profil& p1, Profil& p2, V v) {
    if (istNull(p1.projRichtung) || istNull(p2.projRichtung)) { return; }
    const V d1 = v - p1.start;
    const V d2 = v - p2.start;
    V no = kreuz(d1, d2);
    const double l1 = laenge(no);
    no = einheit(no);
    V no2 = kreuz(d1, p1.projRichtung);
    const double l2 = laenge(no2);
    no2 = einheit(no2);
    V no3 = kreuz(d2, p2.projRichtung);
    const double l3 = laenge(no3);
    no3 = einheit(no3);
    if (l1 != 0.0 && (l2 != 0.0 || l3 != 0.0)) {
        const double dot1 = std::fabs(punkt(no, no2));
        const double dot2 = std::fabs(punkt(no, no3));
        if (std::fabs(dot1 - 1.0) > kEps) { p1.ebeneNormale = no; }
        if (std::fabs(dot2 - 1.0) > kEps) { p2.ebeneNormale = no; }
    }
}

void berechnen(Profil& p, const Abstaende& a) {
    if (a.seg <= 1) { return; }
    Affin m;
    const bool mitAbb = p.r == kRLinie ? false : einheitsQuadrat(p.start, p.mitte, p.ende, m);
    profilStrecke(p, m, mitAbb, a.seg, a.xs, a.ys, p.punkte);
    if (a.seg2 != a.seg) {
        profilStrecke(p, m, mitAbb, a.seg2, a.xs2, a.ys2, p.punkte2);
    } else {
        p.punkte2 = p.punkte;
    }
}

V profilPunkt(const Profil& p, int i, int nseg, const Abstaende& a) {
    if (a.seg <= 1) { return i == 0 ? p.start : p.ende; }
    if (nseg == a.seg) { return p.punkte[static_cast<std::size_t>(i)]; }
    return p.punkte2[static_cast<std::size_t>(i * (a.seg2 / nseg))];
}

void Gitter::kanon(int i, int j, int k, int& ki, int& kj, int& kk) const {   // mesh_vert_canon (:4044)
    const int ns2 = ns / 2;
    const int ungerade = ns % 2;
    if (ungerade == 0 && j == ns2 && k == ns2) {
        ki = 0;
        kj = j;
        kk = k;
    } else if (j <= ns2 - 1 + ungerade && k <= ns2) {
        ki = i;
        kj = j;
        kk = k;
    } else if (k <= ns2) {
        ki = (i + n - 1) % n;
        kj = k;
        kk = ns - j;
    } else {
        ki = (i + 1) % n;
        kj = ns - k;
        kk = j;
    }
}

bool Gitter::istKanon(int ns, int i, int j, int k) {   // is_canon (:4062)
    const int ns2 = ns / 2;
    if (ns % 2 == 1) { return j <= ns2 && k <= ns2; }
    return (j < ns2 && k <= ns2) || (j == ns2 && k == ns2 && i == 0);
}

Gitter eckenGitter(const EckDaten& e, const Abstaende& a) {
    Gitter g;
    if (e.pipe >= 0) {
        g = rohrGitter(e, a);
    } else if (e.triTest == 1) {
        g = dreiEcke(e, a);
    } else {
        g = anpassGitter(e, a);
    }
    gleichwertigeKopieren(g);
    return g;
}

std::vector<V> weldPunkte(const Profil& p1, const Profil& p2, const Abstaende& a) {
    // build_vmesh (:6150): je Punkt die Mitte der beiden Profile (das gerade Profil gibt nach)
    std::vector<V> aus;
    const int ns = a.seg;
    for (int k = 1; k < ns; ++k) {
        const V v1 = profilPunkt(p1, k, ns, a);
        const V v2 = profilPunkt(p2, ns - k, ns, a);
        if (p1.r == kRLinie && p2.r != kRLinie) {
            aus.push_back(v2);
        } else if (p2.r == kRLinie && p1.r != kRLinie) {
            aus.push_back(v1);
        } else {
            aus.push_back(mitteVon(v1, v2));
        }
    }
    return aus;
}

}  // namespace bhed::modell::wof::fase
