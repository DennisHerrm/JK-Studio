// modell/physik.cpp - Modell-Block Stufe 6: Auto-Physik (Rechnung und Quellen in bhed/modell/physik.h)
#include "bhed/modell/physik.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <utility>

#include "bhed/modell/ik.h"

namespace bhed::modell {

namespace {

constexpr double kPi = 3.14159265358979323846;

using D3 = std::array<double, 3>;
struct M3 {
    double a[3][3]{{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}};
};

bool knochenOk(const GlaAnimation& g, int b) noexcept { return b >= 0 && static_cast<std::size_t>(b) < g.bones.size(); }

Vec3 ortVon(const BoneMatrix& m) noexcept { return {m.m[0][3], m.m[1][3], m.m[2][3]}; }

Vec3 punktMal(const BoneMatrix& m, const Vec3& p) noexcept {
    Vec3 o{};
    for (int r = 0; r < 3; ++r) { o[static_cast<std::size_t>(r)] = m.m[r][0] * p[0] + m.m[r][1] * p[1] + m.m[r][2] * p[2] + m.m[r][3]; }
    return o;
}

D3 d3(const Vec3& v) noexcept { return {v[0], v[1], v[2]}; }
D3 minus(const D3& a, const D3& b) noexcept { return {a[0] - b[0], a[1] - b[1], a[2] - b[2]}; }
D3 plus(const D3& a, const D3& b) noexcept { return {a[0] + b[0], a[1] + b[1], a[2] + b[2]}; }
D3 mal(const D3& a, double s) noexcept { return {a[0] * s, a[1] * s, a[2] * s}; }
D3 kreuz(const D3& a, const D3& b) noexcept { return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]}; }
double laenge(const D3& a) noexcept { return std::sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]); }

M3 mul(const M3& x, const M3& y) noexcept {
    M3 o;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) { o.a[r][c] = x.a[r][0] * y.a[0][c] + x.a[r][1] * y.a[1][c] + x.a[r][2] * y.a[2][c]; }
    }
    return o;
}
M3 trans(const M3& x) noexcept {
    M3 o;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) { o.a[r][c] = x.a[c][r]; }
    }
    return o;
}
D3 mulV(const M3& x, const D3& v) noexcept {
    return {x.a[0][0] * v[0] + x.a[0][1] * v[1] + x.a[0][2] * v[2], x.a[1][0] * v[0] + x.a[1][1] * v[1] + x.a[1][2] * v[2],
            x.a[2][0] * v[0] + x.a[2][1] * v[1] + x.a[2][2] * v[2]};
}
// Rodrigues: Drehvektor -> Matrix
M3 expRot(const D3& w) noexcept {
    const double th = laenge(w);
    M3 o;
    if (th < 1e-12) {
        o.a[0][1] = -w[2]; o.a[0][2] = w[1];
        o.a[1][0] = w[2];  o.a[1][2] = -w[0];
        o.a[2][0] = -w[1]; o.a[2][1] = w[0];
        return o;
    }
    const D3 k = mal(w, 1.0 / th);
    const double c = std::cos(th);
    const double s = std::sin(th);
    const double t = 1.0 - c;
    o.a[0][0] = c + k[0] * k[0] * t;        o.a[0][1] = k[0] * k[1] * t - k[2] * s; o.a[0][2] = k[0] * k[2] * t + k[1] * s;
    o.a[1][0] = k[1] * k[0] * t + k[2] * s; o.a[1][1] = c + k[1] * k[1] * t;        o.a[1][2] = k[1] * k[2] * t - k[0] * s;
    o.a[2][0] = k[2] * k[0] * t - k[1] * s; o.a[2][1] = k[2] * k[1] * t + k[0] * s; o.a[2][2] = c + k[2] * k[2] * t;
    return o;
}
// Matrix -> Drehvektor (ueber das Quaternion, stabil bei kleinen und grossen Winkeln)
D3 logRot(const M3& r) noexcept {
    double q[4];
    const double tr = r.a[0][0] + r.a[1][1] + r.a[2][2];
    if (tr > 0.0) {
        const double s = std::sqrt(tr + 1.0) * 2.0;
        q[0] = 0.25 * s;
        q[1] = (r.a[2][1] - r.a[1][2]) / s;
        q[2] = (r.a[0][2] - r.a[2][0]) / s;
        q[3] = (r.a[1][0] - r.a[0][1]) / s;
    } else if (r.a[0][0] > r.a[1][1] && r.a[0][0] > r.a[2][2]) {
        const double s = std::sqrt(1.0 + r.a[0][0] - r.a[1][1] - r.a[2][2]) * 2.0;
        q[0] = (r.a[2][1] - r.a[1][2]) / s;
        q[1] = 0.25 * s;
        q[2] = (r.a[0][1] + r.a[1][0]) / s;
        q[3] = (r.a[0][2] + r.a[2][0]) / s;
    } else if (r.a[1][1] > r.a[2][2]) {
        const double s = std::sqrt(1.0 + r.a[1][1] - r.a[0][0] - r.a[2][2]) * 2.0;
        q[0] = (r.a[0][2] - r.a[2][0]) / s;
        q[1] = (r.a[0][1] + r.a[1][0]) / s;
        q[2] = 0.25 * s;
        q[3] = (r.a[1][2] + r.a[2][1]) / s;
    } else {
        const double s = std::sqrt(1.0 + r.a[2][2] - r.a[0][0] - r.a[1][1]) * 2.0;
        q[0] = (r.a[1][0] - r.a[0][1]) / s;
        q[1] = (r.a[0][2] + r.a[2][0]) / s;
        q[2] = (r.a[1][2] + r.a[2][1]) / s;
        q[3] = 0.25 * s;
    }
    if (q[0] < 0.0) {
        for (double& x : q) { x = -x; }
    }
    const double sv = std::sqrt(q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
    if (sv < 1e-15) { return {0.0, 0.0, 0.0}; }
    const double th = 2.0 * std::atan2(sv, q[0]);
    return {q[1] / sv * th, q[2] / sv * th, q[3] / sv * th};
}
// Die Drehung einer (starren) Weltmatrix
M3 drehungVon(const BoneMatrix& m) noexcept {
    const BoneMatrix s = starr(m);
    M3 o;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) { o.a[r][c] = s.m[r][c]; }
    }
    return o;
}
// T(o) R T(-o) als BoneMatrix
BoneMatrix umPunkt(const M3& r, const D3& o) noexcept {
    BoneMatrix m = BoneMatrix::identity();
    for (int i = 0; i < 3; ++i) {
        for (int c = 0; c < 3; ++c) { m.m[i][c] = static_cast<float>(r.a[i][c]); }
        const double ro = r.a[i][0] * o[0] + r.a[i][1] * o[1] + r.a[i][2] * o[2];
        m.m[i][3] = static_cast<float>(o[static_cast<std::size_t>(i)] - ro);
    }
    return m;
}

// Ein M mit dem Eintrag des Knochens multiplizieren (m = m P), sonst anhaengen.
void mulAuf(std::vector<std::pair<int, BoneMatrix>>& m, int b, const BoneMatrix& p) {
    for (auto& [k, mm] : m) {
        if (k == b) {
            mm = mm * p;
            return;
        }
    }
    m.emplace_back(b, p);
}
// Eine Weltaenderung W des Knochens b (X'' = W X') als M: P = X'^-1 W X'
void weltAuf(const GlaAnimation& g, int f, std::vector<std::pair<int, BoneMatrix>>& m, std::vector<BoneMatrix>& welt, int b, const BoneMatrix& w) {
    if (!knochenOk(g, b)) { return; }
    const BoneMatrix& x = welt[static_cast<std::size_t>(b)];
    mulAuf(m, b, starrInvers(x) * w * x);
    g.worldMatricesLerp(f, f, 0.0F, {}, m, welt);
}

bool istEinheit(const BoneMatrix& m, float eps) noexcept {
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 4; ++c) {
            if (std::fabs(m.m[r][c] - (r == c ? 1.0F : 0.0F)) > eps) { return false; }
        }
    }
    return true;
}

void slerpQ(const float a[4], const float b[4], float t, float o[4]) noexcept {
    float bb[4] = {b[0], b[1], b[2], b[3]};
    float d = a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
    if (d < 0.0F) {
        for (float& x : bb) { x = -x; }
        d = -d;
    }
    float wa = 1.0F - t;
    float wb = t;
    if (d < 0.9995F) {
        const float th = std::acos(std::min(d, 1.0F));
        const float s = std::sin(th);
        wa = std::sin((1.0F - t) * th) / s;
        wb = std::sin(t * th) / s;
    }
    float n = 0.0F;
    for (int i = 0; i < 4; ++i) {
        o[i] = wa * a[i] + wb * bb[i];
        n += o[i] * o[i];
    }
    n = std::sqrt(n);
    for (int i = 0; i < 4; ++i) { o[i] /= n > 0.0F ? n : 1.0F; }
}

int griffKnochen(const Rig& r, GriffArt art, Seite s) {
    int best = -1;
    int nr = 1 << 30;
    for (const Griff& gr : r.griffe) {
        if (gr.art == art && gr.seite == s && !gr.ik && gr.nummer < nr) {
            best = gr.knochen;
            nr = gr.nummer;
        }
    }
    return best;
}

bool unter(const GlaAnimation& g, int b, int wurzel) {
    for (int i = 0; knochenOk(g, b) && i < 512; ++i) {
        if (b == wurzel) { return true; }
        b = g.bones[static_cast<std::size_t>(b)].parent;
    }
    return false;
}

Vec3 grundOrt(const GlaAnimation& g, int b) {
    const BoneMatrix& m = g.bones[static_cast<std::size_t>(b)].basePose;
    return ortVon(m);
}

// Die Pose eines Bilds ohne Physik: M (nach FK, IK, Bindebasis) und Welt.
void pose(const GlaAnimation& g, const RigSchicht& q, int f, std::vector<std::pair<int, BoneMatrix>>& m, std::vector<BoneMatrix>& welt) {
    rigNachMalAuf(g, q, f, f, 0.0F, m);
    g.worldMatricesLerp(f, f, 0.0F, {}, m, welt);
}

struct Analyse {
    std::vector<std::vector<std::pair<int, BoneMatrix>>> m;
    std::vector<std::vector<BoneMatrix>> welt;
    std::vector<std::array<int, 2>> tiefster;      // Index des tiefsten Fusspunkts je Seite
    std::vector<std::array<Vec3, 2>> tiefOrt;
    std::vector<std::array<std::vector<std::array<float, 2>>, 2>> punkte;   // Fusspunkte (x, y) je Bild und Seite
};

float bodenUnter(const PhysikUmgebung& u, float x, float y, float zOben) {
    float z = u.bodenZ;
    if (u.boden && u.boden(x, y, zOben, z)) { return z; }
    return u.bodenZ;
}

bool analysieren(const GlaAnimation& g, const MassenModell& mm, const RigSchicht& q, int erstes, int anzahl, const PhysikUmgebung& u,
                 const PhysikEinstellung& e, PhysikBericht& b, Analyse& an) {
    b.erstes = erstes;
    b.bilder.assign(static_cast<std::size_t>(anzahl), PhysikBild{});
    an.m.resize(static_cast<std::size_t>(anzahl));
    an.welt.resize(static_cast<std::size_t>(anzahl));
    an.tiefster.assign(static_cast<std::size_t>(anzahl), {-1, -1});
    an.tiefOrt.assign(static_cast<std::size_t>(anzahl), {});
    an.punkte.assign(static_cast<std::size_t>(anzahl), {});
    for (int i = 0; i < anzahl; ++i) {
        const auto ui = static_cast<std::size_t>(i);
        pose(g, q, erstes + i, an.m[ui], an.welt[ui]);
        PhysikBild& pb = b.bilder[ui];
        pb.sp = schwerpunkt(g, mm, an.welt[ui]);
        pb.spNeu = pb.sp;
        pb.bodenZ = bodenUnter(u, pb.sp[0], pb.sp[1], pb.sp[2]);
        for (std::size_t s = 0; s < 2; ++s) {
            float tief = 1e30F;
            std::vector<std::array<float, 2>> punkte;
            for (std::size_t k = 0; k < mm.fuss[s].size(); ++k) {
                const Vec3 p = punktWelt(g, mm.fuss[s][k], an.welt[ui]);
                const float h = p[2] - bodenUnter(u, p[0], p[1], std::max(p[2], pb.sp[2]));
                punkte.push_back({p[0], p[1]});
                if (h < tief) {
                    tief = h;
                    an.tiefster[ui][s] = static_cast<int>(k);
                    an.tiefOrt[ui][s] = p;
                }
            }
            pb.fussHoehe[s] = tief;
            an.punkte[ui][s] = std::move(punkte);
        }
    }
    // Kontakt: der tiefste Fusspunkt hoechstens kontaktAbstand ueber seinem tiefsten Stand in der Sequenz - die Modelle stehen
    // verschieden hoch ueber dem Boden (BOTH_STAND1: Sturmtruppler 0,30, Kyle 0,86; Selbsttest ph)
    std::array<float, 2> bezug{1e30F, 1e30F};
    for (const PhysikBild& pb : b.bilder) {
        for (std::size_t s = 0; s < 2; ++s) { bezug[s] = std::min(bezug[s], pb.fussHoehe[s]); }
    }
    for (int i = 0; i < anzahl; ++i) {
        const auto ui = static_cast<std::size_t>(i);
        PhysikBild& pb = b.bilder[ui];
        std::vector<std::array<float, 2>> stuetz;
        for (std::size_t s = 0; s < 2; ++s) {
            pb.kontakt[s] = !an.punkte[ui][s].empty() && pb.fussHoehe[s] <= bezug[s] + e.kontaktAbstand;
            if (pb.kontakt[s]) { stuetz.insert(stuetz.end(), an.punkte[ui][s].begin(), an.punkte[ui][s].end()); }
        }
        pb.stuetz = konvexeHuelle(stuetz);
        pb.imStand = pb.kontakt[0] && pb.kontakt[1];
        // ohne Flaeche (ohne Modell nur Knoechel: eine Linie) keine Warnung
        if (pb.imStand && pb.stuetz.size() >= 3 && !imPolygon(pb.stuetz, pb.sp[0], pb.sp[1])) {
            pb.ausserhalb = true;
            pb.abstand = polygonAbstand(pb.stuetz, pb.sp[0], pb.sp[1]);
            ++b.warnungen;
            if (b.warnBild < 0) { b.warnBild = erstes + i; }
        }
    }
    // Fluege: gewaehlt, sonst jede Strecke ohne Kontakt mit Kontakt davor und danach
    b.fluege.clear();
    const int letztes = erstes + anzahl - 1;
    if (e.absprung >= 0 && e.landung > e.absprung) {
        const int a = std::clamp(e.absprung, erstes, letztes);
        const int l = std::clamp(e.landung, erstes, letztes);
        if (l > a) { b.fluege.emplace_back(a, l); }
    } else {
        int i = 0;
        while (i < anzahl) {
            const auto ui = static_cast<std::size_t>(i);
            if (b.bilder[ui].kontakt[0] || b.bilder[ui].kontakt[1]) {
                ++i;
                continue;
            }
            int j = i;
            while (j < anzahl && !b.bilder[static_cast<std::size_t>(j)].kontakt[0] && !b.bilder[static_cast<std::size_t>(j)].kontakt[1]) { ++j; }
            if (i > 0 && j < anzahl) { b.fluege.emplace_back(erstes + i - 1, erstes + j); }
            i = j;
        }
    }
    for (const auto& [a, l] : b.fluege) {
        for (int f = a + 1; f < l; ++f) { b.bilder[static_cast<std::size_t>(f - erstes)].flug = true; }
    }
    return true;
}

// --- Drehimpuls in der Luft ------------------------------------------------------------------------------------------
struct Flug {
    std::vector<double> masse;
    std::vector<std::vector<D3>> r;   // je Bild (a..l) die Orte relativ zum Schwerpunkt
    double dt = 0.05;
};

D3 lHalb(const Flug& fl, std::size_t k, const M3& ra, const M3& rb) {
    D3 l{0.0, 0.0, 0.0};
    for (std::size_t i = 0; i < fl.masse.size(); ++i) {
        const D3 xa = mulV(ra, fl.r[k][i]);
        const D3 xb = mulV(rb, fl.r[k + 1][i]);
        l = plus(l, mal(kreuz(mal(plus(xa, xb), 0.5), mal(minus(xb, xa), 1.0 / fl.dt)), fl.masse[i]));
    }
    return l;
}

bool loese3(const double j[3][3], const D3& f, D3& x) {
    const double det = j[0][0] * (j[1][1] * j[2][2] - j[1][2] * j[2][1]) - j[0][1] * (j[1][0] * j[2][2] - j[1][2] * j[2][0]) +
                       j[0][2] * (j[1][0] * j[2][1] - j[1][1] * j[2][0]);
    if (std::fabs(det) < 1e-300) { return false; }
    for (int c = 0; c < 3; ++c) {
        double m[3][3];
        std::memcpy(m, j, sizeof m);
        for (int r = 0; r < 3; ++r) { m[r][c] = f[static_cast<std::size_t>(r)]; }
        x[static_cast<std::size_t>(c)] = (m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
                                          m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0])) / det;
    }
    return true;
}

// Vorwaerts mit L0: R(a) = Einheit, je Bild Newton fuer R(k+1) = exp(phi) R(k).
std::vector<M3> vorwaerts(const Flug& fl, const D3& l0, double skala) {
    const std::size_t n = fl.r.size();
    std::vector<M3> rs(n);
    for (std::size_t k = 0; k + 1 < n; ++k) {
        D3 phi{0.0, 0.0, 0.0};
        // Schaetzung: dieselbe Drehung wie im Schritt davor
        if (k > 0) { phi = logRot(mul(rs[k], trans(rs[k - 1]))); }
        for (int it = 0; it < 12; ++it) {
            const D3 f = minus(lHalb(fl, k, rs[k], mul(expRot(phi), rs[k])), l0);
            if (laenge(f) < 1e-11 * skala) { break; }
            double j[3][3];
            const double h = 1e-6;
            for (int c = 0; c < 3; ++c) {
                D3 p = phi;
                p[static_cast<std::size_t>(c)] += h;
                const D3 fp = minus(lHalb(fl, k, rs[k], mul(expRot(p), rs[k])), l0);
                for (int r = 0; r < 3; ++r) { j[r][c] = (fp[static_cast<std::size_t>(r)] - f[static_cast<std::size_t>(r)]) / h; }
            }
            D3 dx{};
            if (!loese3(j, f, dx)) { break; }
            phi = minus(phi, dx);
        }
        rs[k + 1] = mul(expRot(phi), rs[k]);
    }
    return rs;
}

// L0 so, dass R(l) = Einheit; gibt die Drehungen je Bild a..l.
std::vector<M3> drehungenImFlug(const Flug& fl, int& iterationen) {
    const std::size_t n = fl.r.size();
    double skala = 1e-12;
    D3 l0{0.0, 0.0, 0.0};
    const M3 eins;
    for (std::size_t k = 0; k + 1 < n; ++k) {
        const D3 l = lHalb(fl, k, eins, eins);
        l0 = plus(l0, mal(l, 1.0 / static_cast<double>(n - 1)));
        skala = std::max(skala, laenge(l));
    }
    double traeg = 0.0;   // Traegheit grob (fuer die Schrittweite der Ableitung)
    for (std::size_t i = 0; i < fl.masse.size(); ++i) { traeg += fl.masse[i] * laenge(fl.r[0][i]) * laenge(fl.r[0][i]); }
    skala = std::max(skala, traeg);
    std::vector<M3> rs = vorwaerts(fl, l0, skala);
    iterationen = 0;
    for (int it = 0; it < 30; ++it) {
        const D3 g0 = logRot(rs.back());
        if (laenge(g0) < 1e-9) { break; }
        ++iterationen;
        double j[3][3];
        const double h = 1e-5 * skala;
        for (int c = 0; c < 3; ++c) {
            D3 p = l0;
            p[static_cast<std::size_t>(c)] += h;
            const D3 gp = logRot(vorwaerts(fl, p, skala).back());
            for (int r = 0; r < 3; ++r) { j[r][c] = (gp[static_cast<std::size_t>(r)] - g0[static_cast<std::size_t>(r)]) / h; }
        }
        D3 dx{};
        if (!loese3(j, g0, dx)) { break; }
        l0 = minus(l0, dx);
        rs = vorwaerts(fl, l0, skala);
    }
    return rs;
}

// --- Federketten -----------------------------------------------------------------------------------------------------
struct Feder {
    int knochen = -1;
    double w0 = 0.0;    // 2 pi f
    double zeta = 0.0;
    bool da = false;
    M3 s;               // simulierte Weltdrehung
    M3 tVor;            // animierte Weltdrehung im Bild davor
    D3 w{0.0, 0.0, 0.0};
};

}  // namespace

// --- Massenmodell ----------------------------------------------------------------------------------------------------

MassenModell massenModell(const GlaAnimation& g, const Rig& r) {
    MassenModell mm;
    const int becken = griffKnochen(r, GriffArt::Becken, Seite::Mitte);
    const int kopf = griffKnochen(r, GriffArt::Kopf, Seite::Mitte);
    const Seite seiten[2] = {Seite::Links, Seite::Rechts};
    int ob[2], un[2], ha[2], os[2], us[2], fu[2], ze[2];
    for (int s = 0; s < 2; ++s) {
        ob[s] = griffKnochen(r, GriffArt::Oberarm, seiten[s]);
        un[s] = griffKnochen(r, GriffArt::Unterarm, seiten[s]);
        ha[s] = griffKnochen(r, GriffArt::Hand, seiten[s]);
        os[s] = griffKnochen(r, GriffArt::Oberschenkel, seiten[s]);
        us[s] = griffKnochen(r, GriffArt::Unterschenkel, seiten[s]);
        fu[s] = griffKnochen(r, GriffArt::Fuss, seiten[s]);
        ze[s] = griffKnochen(r, GriffArt::Zehen, seiten[s]);
    }
    std::string fehlt;
    const auto noetig = [&](int b, const char* was) {
        if (!knochenOk(g, b)) { fehlt += std::string(fehlt.empty() ? "" : ", ") + was; }
    };
    noetig(becken, "pelvis");
    noetig(kopf, "head");
    for (int s = 0; s < 2; ++s) {
        const char* sl = s == 0 ? "_l" : "_r";
        noetig(ob[s], (std::string("upperarm") + sl).c_str());
        noetig(un[s], (std::string("forearm") + sl).c_str());
        noetig(ha[s], (std::string("hand") + sl).c_str());
        noetig(os[s], (std::string("thigh") + sl).c_str());
        noetig(us[s], (std::string("calf") + sl).c_str());
        noetig(fu[s], (std::string("foot") + sl).c_str());
    }
    if (!fehlt.empty()) {
        mm.fehler = "missing: " + fehlt;
        return mm;
    }
    mm.wurzel = becken;
    const auto knochenPunkt = [](std::vector<int> k) {
        PhysikPunkt p;
        p.knochen = std::move(k);
        return p;
    };
    // Winter 2009, Tabelle 4.1 (siehe physik.h)
    static const char* const kNamen[2][6] = {{"upperarm_l", "forearm_l", "hand_l", "thigh_l", "calf_l", "foot_l"},
                                             {"upperarm_r", "forearm_r", "hand_r", "thigh_r", "calf_r", "foot_r"}};
    for (int s = 0; s < 2; ++s) {
        mm.segmente.push_back({kNamen[s][0], knochenPunkt({ob[s]}), knochenPunkt({un[s]}), 0.028F, 0.436F});
        mm.segmente.push_back({kNamen[s][1], knochenPunkt({un[s]}), knochenPunkt({ha[s]}), 0.016F, 0.430F});
        // Hand: Handgelenk bis zum entferntesten Fingerknochen dieser Seite (Grundstellung)
        int finger = -1;
        float weit = -1.0F;
        const Vec3 h0 = grundOrt(g, ha[s]);
        for (std::size_t k = 0; k < r.erkennung.knochen.size() && k < g.bones.size(); ++k) {
            const KnochenErkennung& ke = r.erkennung.knochen[k];
            if (ke.rolle != Rolle::Finger || ke.seite != seiten[s] || !unter(g, static_cast<int>(k), ha[s]) && !unter(g, static_cast<int>(k), un[s])) { continue; }
            const Vec3 p = grundOrt(g, static_cast<int>(k));
            const float d = std::hypot(p[0] - h0[0], p[1] - h0[1], p[2] - h0[2]);
            if (d > weit) {
                weit = d;
                finger = static_cast<int>(k);
            }
        }
        mm.segmente.push_back({kNamen[s][2], knochenPunkt({ha[s]}), knochenPunkt({finger >= 0 ? finger : ha[s]}), 0.006F, 0.506F});
        mm.segmente.push_back({kNamen[s][3], knochenPunkt({os[s]}), knochenPunkt({us[s]}), 0.100F, 0.433F});
        mm.segmente.push_back({kNamen[s][4], knochenPunkt({us[s]}), knochenPunkt({fu[s]}), 0.0465F, 0.433F});
    }
    // Beine (IK-Spuren aus Stufe 4) und Fusspunkte
    for (int s = 0; s < 2; ++s) {
        IkSpur sp;
        if (ikSpurAnlegen(g, r, s == 0 ? IkArt::BeinLinks : IkArt::BeinRechts, sp)) {
            mm.bein[static_cast<std::size_t>(s)] = static_cast<int>(mm.spuren.size());
            mm.spuren.push_back(sp);
        }
        std::vector<PhysikPunkt>& fp = mm.fuss[static_cast<std::size_t>(s)];
        PhysikPunkt fussEnde = knochenPunkt({fu[s]});
        if (mm.bein[static_cast<std::size_t>(s)] >= 0) {
            const IkSpur& b = mm.spuren[static_cast<std::size_t>(mm.bein[static_cast<std::size_t>(s)])];
            const auto anFuss = [&](const float o[3]) {
                PhysikPunkt p;
                p.an = fu[s];
                p.ort = {o[0], o[1], o[2]};
                return p;
            };
            if (b.hatFerse) { fp.push_back(anFuss(b.ferse)); }
            if (b.hatBallen) { fp.push_back(anFuss(b.ballen)); }
            if (b.hatSpitze) { fp.push_back(anFuss(b.spitzePunkt)); }
            if (b.hatBallen) {
                fussEnde = anFuss(b.ballen);
            } else if (b.hatSpitze) {
                fussEnde = anFuss(b.spitzePunkt);
            }
        }
        if (knochenOk(g, ze[s])) { fussEnde = knochenPunkt({ze[s]}); }
        if (fp.empty()) {
            fp.push_back(knochenPunkt({fu[s]}));
            if (knochenOk(g, ze[s])) { fp.push_back(knochenPunkt({ze[s]})); }
        }
        mm.segmente.push_back({kNamen[s][5], knochenPunkt({fu[s]}), fussEnde, 0.0145F, 0.50F});
    }
    // Kopf und Hals: Schwerpunkt am distalen Ende (1,000), dem Kopf; Rumpf: Rollhuegel bis Schultergelenk, 0,50
    mm.segmente.push_back({"head_neck", knochenPunkt({ob[0], ob[1]}), knochenPunkt({kopf}), 0.081F, 1.0F});
    mm.segmente.push_back({"trunk", knochenPunkt({os[0], os[1]}), knochenPunkt({ob[0], ob[1]}), 0.497F, 0.50F});
    for (const PhysikSegment& sg : mm.segmente) {
        for (const PhysikPunkt* p : {&sg.proximal, &sg.distal}) {
            for (const int k : p->knochen) {
                if (!unter(g, k, becken)) { mm.fehler = std::string("not below the pelvis: ") + g.bones[static_cast<std::size_t>(k)].name; }
            }
            if (p->an >= 0 && !unter(g, p->an, becken)) { mm.fehler = std::string("not below the pelvis: ") + g.bones[static_cast<std::size_t>(p->an)].name; }
        }
    }
    return mm;
}

Vec3 punktWelt(const GlaAnimation& g, const PhysikPunkt& p, const std::vector<BoneMatrix>& welt) {
    if (p.an >= 0 && knochenOk(g, p.an) && static_cast<std::size_t>(p.an) < welt.size()) {
        return punktMal(welt[static_cast<std::size_t>(p.an)] * g.bones[static_cast<std::size_t>(p.an)].basePoseInv, p.ort);
    }
    Vec3 s{};
    int n = 0;
    for (const int k : p.knochen) {
        if (k < 0 || static_cast<std::size_t>(k) >= welt.size()) { continue; }
        const Vec3 o = ortVon(welt[static_cast<std::size_t>(k)]);
        for (int i = 0; i < 3; ++i) { s[static_cast<std::size_t>(i)] += o[static_cast<std::size_t>(i)]; }
        ++n;
    }
    for (float& x : s) { x /= n > 0 ? static_cast<float>(n) : 1.0F; }
    return s;
}

void segmentOrte(const GlaAnimation& g, const MassenModell& mm, const std::vector<BoneMatrix>& welt, std::vector<Vec3>& orte) {
    orte.clear();
    for (const PhysikSegment& s : mm.segmente) {
        const Vec3 a = punktWelt(g, s.proximal, welt);
        const Vec3 b = punktWelt(g, s.distal, welt);
        orte.push_back({a[0] + (b[0] - a[0]) * s.lage, a[1] + (b[1] - a[1]) * s.lage, a[2] + (b[2] - a[2]) * s.lage});
    }
}

Vec3 schwerpunkt(const GlaAnimation& g, const MassenModell& mm, const std::vector<BoneMatrix>& welt) {
    std::vector<Vec3> o;
    segmentOrte(g, mm, welt, o);
    double s[3] = {0.0, 0.0, 0.0};
    double m = 0.0;
    for (std::size_t i = 0; i < o.size(); ++i) {
        for (int k = 0; k < 3; ++k) { s[k] += static_cast<double>(mm.segmente[i].anteil) * o[i][static_cast<std::size_t>(k)]; }
        m += mm.segmente[i].anteil;
    }
    if (m <= 0.0) { return {}; }
    return {static_cast<float>(s[0] / m), static_cast<float>(s[1] / m), static_cast<float>(s[2] / m)};
}

Vec3 drehimpuls(const GlaAnimation& g, const MassenModell& mm, const std::vector<BoneMatrix>& welt0, const std::vector<BoneMatrix>& welt1, double dt) {
    std::vector<Vec3> o0;
    std::vector<Vec3> o1;
    segmentOrte(g, mm, welt0, o0);
    segmentOrte(g, mm, welt1, o1);
    const D3 c0 = d3(schwerpunkt(g, mm, welt0));
    const D3 c1 = d3(schwerpunkt(g, mm, welt1));
    D3 l{0.0, 0.0, 0.0};
    for (std::size_t i = 0; i < o0.size() && i < o1.size(); ++i) {
        const D3 xa = minus(d3(o0[i]), c0);
        const D3 xb = minus(d3(o1[i]), c1);
        l = plus(l, mal(kreuz(mal(plus(xa, xb), 0.5), mal(minus(xb, xa), 1.0 / dt)), mm.segmente[i].anteil));
    }
    return {static_cast<float>(l[0]), static_cast<float>(l[1]), static_cast<float>(l[2])};
}

// --- Stuetzflaeche ---------------------------------------------------------------------------------------------------

std::vector<std::array<float, 2>> konvexeHuelle(std::vector<std::array<float, 2>> p) {
    // Andrew's monotone chain
    std::sort(p.begin(), p.end());
    p.erase(std::unique(p.begin(), p.end()), p.end());
    if (p.size() < 3) { return p; }
    const auto kr = [](const std::array<float, 2>& o, const std::array<float, 2>& a, const std::array<float, 2>& b) {
        return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0]);
    };
    std::vector<std::array<float, 2>> h(2 * p.size());
    std::size_t k = 0;
    for (const auto& x : p) {
        while (k >= 2 && kr(h[k - 2], h[k - 1], x) <= 0.0F) { --k; }
        h[k++] = x;
    }
    for (std::size_t i = p.size() - 1, t = k + 1; i > 0; --i) {
        while (k >= t && kr(h[k - 2], h[k - 1], p[i - 1]) <= 0.0F) { --k; }
        h[k++] = p[i - 1];
    }
    h.resize(k - 1);
    return h;
}

float polygonAbstand(const std::vector<std::array<float, 2>>& poly, float x, float y) {
    if (poly.empty()) { return 0.0F; }
    if (imPolygon(poly, x, y)) { return 0.0F; }
    float best = 1e30F;
    for (std::size_t i = 0; i < poly.size(); ++i) {
        const auto& a = poly[i];
        const auto& b = poly[(i + 1) % poly.size()];
        const float dx = b[0] - a[0];
        const float dy = b[1] - a[1];
        const float l2 = dx * dx + dy * dy;
        float t = l2 > 0.0F ? ((x - a[0]) * dx + (y - a[1]) * dy) / l2 : 0.0F;
        t = std::clamp(t, 0.0F, 1.0F);
        best = std::min(best, std::hypot(x - (a[0] + t * dx), y - (a[1] + t * dy)));
    }
    return best;
}

bool imPolygon(const std::vector<std::array<float, 2>>& poly, float x, float y) {
    if (poly.size() < 3) { return false; }
    for (std::size_t i = 0; i < poly.size(); ++i) {
        const auto& a = poly[i];
        const auto& b = poly[(i + 1) % poly.size()];
        if ((b[0] - a[0]) * (y - a[1]) - (b[1] - a[1]) * (x - a[0]) < -1e-5F) { return false; }
    }
    return true;
}

// --- Rechnen ---------------------------------------------------------------------------------------------------------

bool physikAnalyse(const GlaAnimation& g, const Rig& r, const RigSchicht& s, int erstes, int anzahl, const PhysikUmgebung& u, PhysikBericht& bericht) {
    bericht = PhysikBericht{};
    const MassenModell mm = massenModell(g, r);
    if (!mm.gueltig() || anzahl <= 0) {
        bericht.fehler = mm.fehler.empty() ? std::string("no frames") : mm.fehler;
        return false;
    }
    RigSchicht q = s;
    q.physik = PhysikSchicht{};
    Analyse an;
    return analysieren(g, mm, q, erstes, anzahl, u, s.physik.e, bericht, an);
}

bool physikRechnen(const GlaAnimation& g, const Rig& r, RigSchicht& s, int erstes, int anzahl, const PhysikUmgebung& u, PhysikBericht* bericht) {
    PhysikBericht leer;
    PhysikBericht& b = bericht != nullptr ? *bericht : leer;
    b = PhysikBericht{};
    s.physik.knochen.clear();
    const MassenModell mm = massenModell(g, r);
    if (!mm.gueltig() || anzahl <= 0 || erstes < 0 || erstes + anzahl > g.bildZahl()) {
        b.fehler = !mm.fehler.empty() ? mm.fehler : std::string("frames outside the GLA");
        return false;
    }
    const PhysikEinstellung& e = s.physik.e;
    RigSchicht q = s;
    q.physik = PhysikSchicht{};
    Analyse an;
    (void)analysieren(g, mm, q, erstes, anzahl, u, e, b, an);
    const double dt = u.bildMs / 1000.0;
    const auto n = static_cast<std::size_t>(anzahl);

    // Weltaenderung des Ganzkoerpers je Bild (Flug): W = T(c') R T(-c)
    std::vector<BoneMatrix> ganz(n, BoneMatrix::identity());
    std::vector<Vec3> parabel(n);
    for (const auto& [a, l] : b.fluege) {
        if (!e.sprung || l <= a) { continue; }
        const auto ia = static_cast<std::size_t>(a - erstes);
        const auto il = static_cast<std::size_t>(l - erstes);
        const D3 ca = d3(b.bilder[ia].sp);
        const D3 cl = d3(b.bilder[il].sp);
        const double tt = static_cast<double>(l - a) * dt;
        const D3 gv{0.0, 0.0, -static_cast<double>(e.schwerkraft)};
        const D3 v0 = mal(minus(minus(cl, ca), mal(gv, 0.5 * tt * tt)), 1.0 / tt);
        // Drehimpuls: Punktmassen relativ zum Schwerpunkt je Bild a..l
        Flug fl;
        fl.dt = dt;
        for (const PhysikSegment& sg : mm.segmente) { fl.masse.push_back(sg.anteil); }
        for (std::size_t i = ia; i <= il; ++i) {
            std::vector<Vec3> o;
            segmentOrte(g, mm, an.welt[i], o);
            std::vector<D3> rel;
            for (const Vec3& p : o) { rel.push_back(minus(d3(p), d3(b.bilder[i].sp))); }
            fl.r.push_back(rel);
        }
        int it = 0;
        const std::vector<M3> rs = drehungenImFlug(fl, it);
        b.iterationen = std::max(b.iterationen, it);
        for (std::size_t i = ia; i <= il; ++i) {
            const double t = static_cast<double>(i - ia) * dt;
            const D3 cn = plus(plus(ca, mal(v0, t)), mal(gv, 0.5 * t * t));
            parabel[i] = {static_cast<float>(cn[0]), static_cast<float>(cn[1]), static_cast<float>(cn[2])};
            // W = T(c') R T(-c): erst um c drehen, dann c -> c'
            BoneMatrix w = umPunkt(rs[i - ia], d3(b.bilder[i].sp));
            for (int k = 0; k < 3; ++k) { w.m[k][3] += static_cast<float>(cn[static_cast<std::size_t>(k)] - b.bilder[i].sp[static_cast<std::size_t>(k)]); }
            ganz[i] = w;
        }
    }

    // Kontakte: Beginn jedes Kontakts je Seite
    std::vector<std::array<int, 2>> kontaktAb(n, {-1, -1});
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t sd = 0; sd < 2; ++sd) {
            if (!b.bilder[i].kontakt[sd] || an.tiefster[i][sd] < 0) { continue; }
            kontaktAb[i][sd] = (i > 0 && kontaktAb[i - 1][sd] >= 0 && an.tiefster[i - 1][sd] >= 0) ? kontaktAb[i - 1][sd] : static_cast<int>(i);
        }
    }

    // Federketten
    std::vector<Feder> federn;
    if (e.federn) {
        for (const PhysikKette& k : e.ketten) {
            if (!knochenOk(g, k.knochen)) { continue; }
            for (const int kb : kettenKnochen(g, k.knochen)) {
                Feder f;
                f.knochen = kb;
                f.w0 = 2.0 * kPi * std::max(0.01, static_cast<double>(k.frequenz));
                f.zeta = std::max(0.0, static_cast<double>(k.daempfung));
                federn.push_back(f);
            }
        }
    }
    b.kettenKnochen = static_cast<int>(federn.size());

    std::vector<std::vector<std::pair<int, BoneMatrix>>> korrektur(n);
    std::vector<char> beruehrt(g.bones.size(), 0);
    std::vector<BoneMatrix> welt;
    for (std::size_t i = 0; i < n; ++i) {
        const int f = erstes + static_cast<int>(i);
        std::vector<std::pair<int, BoneMatrix>> m = an.m[i];
        welt = an.welt[i];
        if (!istEinheit(ganz[i], 0.0F)) { weltAuf(g, f, m, welt, mm.wurzel, ganz[i]); }
        // Fuesse im Kontakt: der tiefste Punkt des ersten Kontaktbilds bleibt waagrecht stehen (Bein-IK)
        for (std::size_t sd = 0; sd < 2 && e.kontakte; ++sd) {
            const int ab = kontaktAb[i][sd];
            const int sp = mm.bein[sd];
            if (ab < 0 || sp < 0 || static_cast<std::size_t>(ab) == i) { continue; }
            const auto uab = static_cast<std::size_t>(ab);
            const int pk = an.tiefster[uab][sd];
            const Vec3 fest = an.tiefOrt[uab][sd];
            const Vec3 jetzt = punktWelt(g, mm.fuss[sd][static_cast<std::size_t>(pk)], welt);
            const float dx = fest[0] - jetzt[0];
            const float dy = fest[1] - jetzt[1];
            if (std::fabs(dx) < 1e-5F && std::fabs(dy) < 1e-5F) { continue; }
            const IkSpur& bein = mm.spuren[static_cast<std::size_t>(sp)];
            if (!knochenOk(g, bein.oben.knochen) || !knochenOk(g, bein.unten.knochen) || !knochenOk(g, bein.ende.knochen)) { continue; }
            BoneMatrix ziel = starr(welt[static_cast<std::size_t>(bein.ende.knochen)]);
            ziel.m[0][3] += dx;
            ziel.m[1][3] += dy;
            // Pol: das Knie in seiner jetzigen Richtung (senkrecht zur Linie Huefte - Knoechel), sonst nach vorn zum Fussende
            const D3 hu = d3(ortVon(welt[static_cast<std::size_t>(bein.oben.knochen)]));
            const D3 kn = d3(ortVon(welt[static_cast<std::size_t>(bein.unten.knochen)]));
            const D3 kh = d3(ortVon(welt[static_cast<std::size_t>(bein.ende.knochen)]));
            const D3 achse = minus(kh, hu);
            const double al = laenge(achse);
            D3 rich = minus(kn, hu);
            if (al > 1e-6) { rich = minus(rich, mal(achse, (rich[0] * achse[0] + rich[1] * achse[1] + rich[2] * achse[2]) / (al * al))); }
            if (laenge(rich) < 1e-3) { rich = minus(d3(punktWelt(g, mm.fuss[sd].back(), welt)), kh); }
            const double rl = laenge(rich);
            const D3 pol = rl > 1e-9 ? plus(kn, mal(rich, std::max(1.0, static_cast<double>(bein.polAbstand)) / rl)) : kn;
            BoneMatrix polW = BoneMatrix::identity();
            for (int k = 0; k < 3; ++k) { polW.m[k][3] = static_cast<float>(pol[static_cast<std::size_t>(k)]); }
            const BoneMatrix polR = ikImRaum(welt, bein.polRaum, polW);
            RigSchicht t;
            t.ik.push_back(bein);
            t.ik.back().ziel.clear();
            t.ik.back().wert.clear();
            IkWert w;
            w.ik = 1.0F;
            w.raum = -1;
            for (int k = 0; k < 3; ++k) { w.pol[k] = polR.m[k][3]; }
            ikSchluesselSetzen(t.ik.back(), f, ziel, w);
            (void)ikAnwenden(g, t, static_cast<float>(f), welt, m);
            g.worldMatricesLerp(f, f, 0.0F, {}, m, welt);
        }
        // Federketten: Nachlauf der Weltdrehung, 8 Teilschritte je Bild
        for (Feder& fd : federn) {
            const auto ub = static_cast<std::size_t>(fd.knochen);
            const M3 tj = drehungVon(welt[ub]);
            if (!fd.da) {
                fd.s = tj;
                fd.tVor = tj;
                fd.da = true;
                continue;
            }
            const D3 dT = logRot(mul(tj, trans(fd.tVor)));
            const D3 wT = mal(dT, 1.0 / dt);
            constexpr int kTeile = 8;
            const double h = dt / kTeile;
            for (int j = 1; j <= kTeile; ++j) {
                const M3 tz = mul(expRot(mal(dT, static_cast<double>(j) / kTeile)), fd.tVor);
                const D3 fehler = logRot(mul(fd.s, trans(tz)));
                const D3 acc = minus(mal(fehler, -fd.w0 * fd.w0), mal(minus(fd.w, wT), 2.0 * fd.zeta * fd.w0));
                fd.w = plus(fd.w, mal(acc, h));
                fd.s = mul(expRot(mal(fd.w, h)), fd.s);
            }
            fd.tVor = tj;
            const M3 dR = mul(fd.s, trans(tj));
            weltAuf(g, f, m, welt, fd.knochen, umPunkt(dR, d3(ortVon(welt[ub]))));
        }
        // P = M0^-1 M je Knochen
        for (const auto& [k, mk] : m) {
            if (!knochenOk(g, k)) { continue; }
            BoneMatrix m0 = BoneMatrix::identity();
            for (const auto& [k0, mm0] : an.m[i]) {
                if (k0 == k) { m0 = m0 * mm0; }
            }
            const BoneMatrix p = starrInvers(m0) * mk;
            if (istEinheit(p, 1e-6F)) { continue; }
            korrektur[i].emplace_back(k, p);
            beruehrt[static_cast<std::size_t>(k)] = 1;
        }
        b.bilder[i].spNeu = schwerpunkt(g, mm, welt);
        if (b.bilder[i].flug || std::any_of(b.fluege.begin(), b.fluege.end(), [&](const std::pair<int, int>& fl) { return f == fl.first || f == fl.second; })) {
            const Vec3& pz = parabel[i];
            if (e.sprung) { b.parabelAbw = std::max(b.parabelAbw, std::hypot(b.bilder[i].spNeu[0] - pz[0], b.bilder[i].spNeu[1] - pz[1], b.bilder[i].spNeu[2] - pz[2])); }
        }
    }
    // Spur: je beruehrtem Knochen ein Schluessel je Bild (Einheit, wo keine Korrektur)
    s.physik.knochen.assign(g.bones.size(), {});
    bool irgend = false;
    for (std::size_t k = 0; k < g.bones.size(); ++k) {
        if (beruehrt[k] == 0) { continue; }
        irgend = true;
        auto& v = s.physik.knochen[k];
        v.reserve(n);
        for (std::size_t i = 0; i < n; ++i) {
            RigSchluessel z;
            z.bild = erstes + static_cast<int>(i);
            z.interp = Interp::Linear;
            for (const auto& [kk, p] : korrektur[i]) {
                if (static_cast<std::size_t>(kk) != k) { continue; }
                quatAusMatrix(p, z.q);
                for (int a = 0; a < 3; ++a) { z.t[a] = p.m[a][3]; }
            }
            if (!v.empty() && z.q[0] * v.back().q[0] + z.q[1] * v.back().q[1] + z.q[2] * v.back().q[2] + z.q[3] * v.back().q[3] < 0.0F) {
                for (float& x : z.q) { x = -x; }
            }
            v.push_back(z);
        }
    }
    if (!irgend) { s.physik.knochen.clear(); }
    // Drehimpuls vorher/nachher (zur Anzeige und Pruefung): die groesste Abweichung vom Mittel, bezogen auf den Drehimpuls einer
    // Drehung um 1 rad waehrend des ganzen Flugs (Traegheit der Punktmassen beim Absprung / Flugzeit) - ein Sprung ohne Drehung hat
    // L nahe 0, bezogen auf sich selbst schiene jedes Rauschen gross
    for (const auto& [a, l] : b.fluege) {
        if (l - a < 2) { continue; }
        RigSchicht mit = s;
        float bezug = 1e-9F;
        {
            std::vector<Vec3> o;
            segmentOrte(g, mm, an.welt[static_cast<std::size_t>(a - erstes)], o);
            const Vec3 c = b.bilder[static_cast<std::size_t>(a - erstes)].sp;
            double tr = 0.0;
            for (std::size_t i = 0; i < o.size(); ++i) {
                tr += mm.segmente[i].anteil * (std::pow(o[i][0] - c[0], 2) + std::pow(o[i][1] - c[1], 2) + std::pow(o[i][2] - c[2], 2));
            }
            bezug = std::max(bezug, static_cast<float>(tr / (static_cast<double>(l - a) * dt)));
        }
        for (int pass = 0; pass < 2; ++pass) {
            std::vector<Vec3> ls;
            std::vector<BoneMatrix> w0;
            std::vector<BoneMatrix> w1;
            std::vector<std::pair<int, BoneMatrix>> mm2;
            for (int f = a; f < l; ++f) {
                if (pass == 0) {
                    w0 = an.welt[static_cast<std::size_t>(f - erstes)];
                    w1 = an.welt[static_cast<std::size_t>(f + 1 - erstes)];
                } else {
                    rigWelt(g, mit, f, f, 0.0F, w0);
                    rigWelt(g, mit, f + 1, f + 1, 0.0F, w1);
                }
                ls.push_back(drehimpuls(g, mm, w0, w1, dt));
            }
            Vec3 mittel{};
            for (const Vec3& x : ls) {
                for (int k = 0; k < 3; ++k) { mittel[static_cast<std::size_t>(k)] += x[static_cast<std::size_t>(k)] / static_cast<float>(ls.size()); }
            }
            float abw = 0.0F;
            for (const Vec3& x : ls) { abw = std::max(abw, std::hypot(x[0] - mittel[0], x[1] - mittel[1], x[2] - mittel[2]) / bezug); }
            (pass == 0 ? b.drehimpulsVorher : b.drehimpulsNachher) = std::max(pass == 0 ? b.drehimpulsVorher : b.drehimpulsNachher, abw);
        }
    }
    return true;
}

bool physikWirkt(const RigSchicht& s) noexcept { return s.physik.e.an && s.physik.e.staerke > 0.0F && !s.physik.knochen.empty(); }

void physikAuf(const RigSchicht& s, float bild, std::vector<std::pair<int, BoneMatrix>>& mEff) {
    if (!physikWirkt(s)) { return; }
    const float st = std::clamp(s.physik.e.staerke, 0.0F, 1.0F);
    for (std::size_t k = 0; k < s.physik.knochen.size(); ++k) {
        const auto& v = s.physik.knochen[k];
        if (v.empty() || bild < static_cast<float>(v.front().bild) || bild > static_cast<float>(v.back().bild)) { continue; }
        const auto it = std::upper_bound(v.begin(), v.end(), bild, [](float x, const RigSchluessel& z) { return x < static_cast<float>(z.bild); });
        const RigSchluessel& a = *(it - 1);
        const RigSchluessel& c = it == v.end() ? a : *it;
        const float t = it == v.end() || c.bild == a.bild ? 0.0F : (bild - static_cast<float>(a.bild)) / static_cast<float>(c.bild - a.bild);
        float q[4];
        slerpQ(a.q, c.q, t, q);
        float tr[3];
        for (int i = 0; i < 3; ++i) { tr[i] = a.t[i] + (c.t[i] - a.t[i]) * t; }
        if (st < 1.0F) {
            const float eins[4] = {1.0F, 0.0F, 0.0F, 0.0F};
            float qs[4];
            slerpQ(eins, q, st, qs);
            std::memcpy(q, qs, sizeof q);
            for (float& x : tr) { x *= st; }
        }
        BoneMatrix p = matrixAusQuat(q);
        for (int i = 0; i < 3; ++i) { p.m[i][3] = tr[i]; }
        if (istEinheit(p, 0.0F)) { continue; }
        mulAuf(mEff, static_cast<int>(k), p);
    }
}

std::vector<int> kettenVorschlag(const Rig& r, const GlaAnimation& g) {
    std::vector<int> aus;
    for (std::size_t k = 0; k < r.erkennung.knochen.size() && k < g.bones.size(); ++k) {
        if (r.erkennung.knochen[k].rolle != Rolle::Anhang) { continue; }
        const int p = g.bones[k].parent;
        if (knochenOk(g, p) && static_cast<std::size_t>(p) < r.erkennung.knochen.size() && r.erkennung.knochen[static_cast<std::size_t>(p)].rolle == Rolle::Anhang) { continue; }
        aus.push_back(static_cast<int>(k));
    }
    return aus;
}

std::vector<int> kettenKnochen(const GlaAnimation& g, int anfang) {
    std::vector<int> aus;
    if (!knochenOk(g, anfang)) { return aus; }
    for (const int b : g.topologicalOrder()) {
        if (unter(g, b, anfang)) { aus.push_back(b); }
    }
    return aus;
}

}  // namespace bhed::modell
